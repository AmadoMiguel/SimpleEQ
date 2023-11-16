
#include "SimpleEQProcessor.h"

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQProcessor::createParamLayout() {
    // Create Choice Object for both the Low and Hi Cut Filter Parameters.
    // Slope will have values which are factor of 12 (12 dBs/Oct, 24 dBs/Oct, etc.)
    juce::StringArray stringArray;
    int factor = 12;
    for (int i = 0; i < 4; i++) {
        juce::String str;
        str += (factor + i*factor);
        str += " dBs/Oct";
        stringArray.add(str);
    }
    // 3 Equalizer Bands (Low Cut, Hi Cut and Parametric/Peak)
    juce::AudioProcessorValueTreeState::ParameterLayout paramLayout;
    // Cut Bands: Controllable Freq/Slope
    // Skew factor of 1.0 (Linear change of the values)
    // Low Cut Filter -> 20Hz to 20kHz Freq Range. Default value of 20Hz
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(LO_CUT_FREQ, 1), LO_CUT_FREQ,
                                                                juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                                20.f));
    paramLayout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(LO_CUT_SLOPE, 1), LO_CUT_SLOPE, stringArray, 0));
    // Hi Cut Filter -> 20Hz to 20kHz Freq Range. Default value of 20kHz
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(HI_CUT_FREQ, 1), HI_CUT_FREQ,
                                                                juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                                20000.f));
    paramLayout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(HI_CUT_SLOPE, 1), HI_CUT_SLOPE, stringArray, 0));
    // Parametric Band: Controllable Freq, Gain, Quality
    // Peak Band -> 20Hz to 20kHz Freq Range. Default value of 20Hz
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(PK_FREQ, 1), PK_FREQ,
                                                                juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                                750.f));
    // Peak Band Gain
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(PK_GAIN, 1), PK_GAIN,
                                                                juce::NormalisableRange<float>(PEAK_GAIN_MIN, PEAK_GAIN_MAX, 0.5f, 1.f),
                                                                0.0f));
    // Peak Band Quality factor (Greater value, more narrow; smaller value, more wide)
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(PK_QUALITY, 1), PK_QUALITY,
                                                                juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                                1.f));
    return paramLayout;
}

void SimpleEQProcessor::prepareToPlay(double sampleRate, int samplesPerBlock, juce::AudioProcessorValueTreeState &apvts) {
    this->sampleRate = sampleRate;
    // Initialize ProcessSpec object to be passed to the processing chain for each channel
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    lChain.prepare(spec);
    rChain.prepare(spec);
    // Initialize EQ Filters configuration
    updateFilters(apvts);
    
}

void SimpleEQProcessor::process(juce::AudioBuffer<float> &buffer, juce::AudioProcessorValueTreeState &apvts) {
    updateFilters(apvts);
    // Wrap the buffer with audio samples in the AudioBlock, so that each of the
    // channels data is passed onto the processing chains
    juce::dsp::AudioBlock<float> block(buffer);
    auto lBlock = block.getSingleChannelBlock(0);
    auto rBlock = block.getSingleChannelBlock(1);
    juce::dsp::ProcessContextReplacing<float> lCtxt(lBlock);
    juce::dsp::ProcessContextReplacing<float> rCtxt(rBlock);
    lChain.process(lCtxt);
    rChain.process(rCtxt);
}

void SimpleEQProcessor::updateFilters(juce::AudioProcessorValueTreeState &apvts) {
    // Get Parameters values from ValueTreeState to feed each Filter Coefficients
    ChainSettings settings = getChainSettings(apvts);
    updateCutFilter(ChainPositions::LowCut, settings, lChain);
    updateCutFilter(ChainPositions::LowCut, settings, rChain);
    updatePeakFilter(settings, lChain);
    updatePeakFilter(settings, rChain);
    updateCutFilter(ChainPositions::HighCut, settings, lChain);
    updateCutFilter(ChainPositions::HighCut, settings, rChain);
}

ChainSettings SimpleEQProcessor::getChainSettings(juce::AudioProcessorValueTreeState &apvts) {
    ChainSettings settings;
    settings.loCutFreq = apvts.getRawParameterValue(LO_CUT_FREQ)->load();
    settings.loCutSlope = static_cast<Slope>(apvts.getRawParameterValue(LO_CUT_SLOPE)->load());
    settings.hiCutFreq = apvts.getRawParameterValue(HI_CUT_FREQ)->load();
    settings.hiCutSlope = static_cast<Slope>(apvts.getRawParameterValue(HI_CUT_SLOPE)->load());
    settings.peakFreq = apvts.getRawParameterValue(PK_FREQ)->load();
    settings.peakGainInDbs = apvts.getRawParameterValue(PK_GAIN)->load();
    settings.peakQ = apvts.getRawParameterValue(PK_QUALITY)->load();
    return settings;
}

void SimpleEQProcessor::updatePeakFilter(const ChainSettings &chainSettings, MonoChain &chain) {
    // Setup IIR Peak Filter coefficients into the process chain for each channel
    auto pkCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                                              chainSettings.peakFreq,
                                                                              chainSettings.peakQ,
                                                                              juce::Decibels::decibelsToGain(chainSettings.peakGainInDbs));
    *chain.get<ChainPositions::Peak>().coefficients = *pkCoefficients;
}

void SimpleEQProcessor::updateCutFilter(ChainPositions filterPos, const ChainSettings &chainSettings, MonoChain &chain) {
    if (filterPos == ChainPositions::LowCut) {
        // Coefficients for Low Cut/Hi Pass filter. Order needs to be mutliple of 2 in order to have even dBs/Oct values
        auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.loCutFreq,
                                                                                                          sampleRate,
                                                                                                           2 * (chainSettings.loCutSlope + 1));
        // Initialize each slope Filter as by-passed
        auto &lowCut = chain.get<ChainPositions::LowCut>();
        updateCutFilterCoefficients(lowCut, cutCoefficients, chainSettings.loCutSlope);
    } else { // HighCut
        // Coefficients for Low Pass/Hi Cut filter. Order needs to be mutliple of 2 in order to have even dBs/Oct values
        auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.hiCutFreq,
                                                                                                          sampleRate,
                                                                                                          2 * (chainSettings.hiCutSlope + 1));
        auto &highCut = chain.get<ChainPositions::HighCut>();
        updateCutFilterCoefficients(highCut, cutCoefficients, chainSettings.hiCutSlope);
    }
}

// Helper common method to update coefficients for Cut Filters
template<typename ChainType, typename CoefficientsType>
void SimpleEQProcessor::updateCutFilterCoefficients(ChainType &cutChain, const CoefficientsType &cutCoefficients, const Slope &slope) {
    // All Cut Filter slopes disabled initially
    cutChain.template setBypassed<0>(true);
    cutChain.template setBypassed<1>(true);
    cutChain.template setBypassed<2>(true);
    cutChain.template setBypassed<3>(true);
    // Depending on selected slope, assign coefficients and enable slopes until (and including) that slope Filter
    switch (slope) {
        case Slope_48:
            enableCutSlopeFilter<3>(cutChain, cutCoefficients);
        case Slope_36:
            enableCutSlopeFilter<2>(cutChain, cutCoefficients);
        case Slope_24:
            enableCutSlopeFilter<1>(cutChain, cutCoefficients);
        case Slope_12:
            enableCutSlopeFilter<0>(cutChain, cutCoefficients);
    }
}

template<int Index, typename ChainType, typename CoefficientsType>
void SimpleEQProcessor::enableCutSlopeFilter(ChainType &cutChain, const CoefficientsType &cutCoefficients) {
    *cutChain.template get<Index>().coefficients = *cutCoefficients[Index];
    cutChain.template setBypassed<Index>(false);
}

void SimpleEQProcessor::calculateFrequencyResponseMagnitude(std::vector<double> &mags, juce::AudioProcessorValueTreeState &apvts) {
    // Set Magnitude values for the Frequency Response Curve, based on the Mono Chain Filters
    ChainSettings settings = getChainSettings(apvts);
    updateCutFilter(ChainPositions::LowCut, settings, cChain);
    updatePeakFilter(settings, cChain);
    updateCutFilter(ChainPositions::HighCut, settings, cChain);
    auto &lowCut = cChain.get<ChainPositions::LowCut>();
    auto &peak = cChain.get<ChainPositions::Peak>();
    auto &hiCut = cChain.get<ChainPositions::HighCut>();
    for (size_t i = 0; i < mags.size(); i++) {
        // Magnitude is multiplicative instead of additive
        double mag = 1.f;
        // Representation of frequency value on pixel space; conversion is needed
        auto freq = juce::mapToLog10(double(i)/double(mags.size()), 20.0, 20000.0);
        // If each band is not bypassed, do the magnitude calculation
        if (!lowCut.isBypassed<0>())
            mag *= lowCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowCut.isBypassed<1>())
            mag *= lowCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowCut.isBypassed<2>())
            mag *= lowCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowCut.isBypassed<3>())
            mag *= lowCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!cChain.isBypassed<ChainPositions::Peak>())
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!hiCut.isBypassed<0>())
            mag *= hiCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!hiCut.isBypassed<1>())
            mag *= hiCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!hiCut.isBypassed<2>())
            mag *= hiCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!hiCut.isBypassed<3>())
            mag *= hiCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        mags[i] = juce::Decibels::gainToDecibels(mag);
    }
}
