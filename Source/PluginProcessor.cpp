/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    // Initialize ProcessSpec object to be passed to the processing chain for each channel
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    lChain.prepare(spec);
    rChain.prepare(spec);
    
    ChainSettings settings = getChainSettings(valueTreeState);

    // Initialize Peak Filter in the processing chain
    updatePeakFilter(settings);
    // Initialize Low and High Cut Filters for each processing chain
    updateCutFilter(ChainPositions::LowCut, settings, lChain);
    updateCutFilter(ChainPositions::LowCut, settings, rChain);
    updateCutFilter(ChainPositions::HighCut, settings, lChain);
    updateCutFilter(ChainPositions::HighCut, settings, rChain);
}

void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // Get values from ValueTreeState to feed each Filter Coefficients
    ChainSettings settings = getChainSettings(valueTreeState);
    updatePeakFilter(settings);
    updateCutFilter(ChainPositions::LowCut, settings, lChain);
    updateCutFilter(ChainPositions::LowCut, settings, rChain);
    updateCutFilter(ChainPositions::HighCut, settings, lChain);
    updateCutFilter(ChainPositions::HighCut, settings, rChain);
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

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
//    return new SimpleEQAudioProcessorEditor (*this);
    // Can be used to build generic GUI with the declared Processor Parameters (if any)
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParamLayout()
{
    // 3 Equalizer Bands (Low Cut, Hi Cut and Parametric/Peak)
    juce::AudioProcessorValueTreeState::ParameterLayout paramLayout;
    // Cut Bands: Controllable Freq/Slope
    // Skew factor of 1.0 (Linear change of the values)
    // Low Cut Filter -> 20Hz to 20kHz Freq Range. Default value of 20Hz
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(LO_CUT_FREQ, 1), LO_CUT_FREQ,
                                                                juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                                20.f));
    // Hi Cut Filter -> 20Hz to 20kHz Freq Range. Default value of 20kHz
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(HI_CUT_FREQ, 1), HI_CUT_FREQ,
                                                                juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                                20000.f));
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
    paramLayout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(LO_CUT_SLOPE, 1), LO_CUT_SLOPE, stringArray, 0));
    paramLayout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(HI_CUT_SLOPE, 1), HI_CUT_SLOPE, stringArray, 0));
    
    // Parametric Band: Controllable Freq, Gain, Quality
    // Peak Band -> 20Hz to 20kHz Freq Range. Default value of 20Hz
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(PK_FREQ, 1), PK_FREQ,
                                                                juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                                750.f));
    // Peak Band Gain
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(PK_GAIN, 1), PK_GAIN,
                                                                juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                                0.0f));
    // Peak Band Quality factor (Greater value, more narrow; smaller value, more wide)
    paramLayout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(PK_QUALITY, 1), PK_QUALITY,
                                                                juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                                1.f));
    return paramLayout;
}

ChainSettings SimpleEQAudioProcessor::getChainSettings(juce::AudioProcessorValueTreeState &apvts) {
    // Load parameter values from the Value Tree State
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

void SimpleEQAudioProcessor::updatePeakFilter(const ChainSettings &chainSettings) {
    // Setup IIR Peak Filter coefficients into the process chain for each channel
    auto pkCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
                                                                              chainSettings.peakFreq,
                                                                              chainSettings.peakQ,
                                                                              juce::Decibels::decibelsToGain(chainSettings.peakGainInDbs));
    *lChain.get<ChainPositions::Peak>().coefficients = *pkCoefficients;
    *rChain.get<ChainPositions::Peak>().coefficients = *pkCoefficients;
}

// Common method to update either Low Cut/Hi Cut filter coefficients
void SimpleEQAudioProcessor::updateCutFilter(ChainPositions filterPos, const ChainSettings &chainSettings, MonoChain &chain) {
    if (filterPos == ChainPositions::LowCut) {
        // Coefficients for Low Cut/Hi Pass filter. Order needs to be mutliple of 2 in order to have even dBs/Oct values
        auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.loCutFreq,
                                                                                                           getSampleRate(),
                                                                                                           2 * (chainSettings.loCutSlope + 1));
        // Initialize each slope Filter as by-passed
        auto &lowCut = chain.get<ChainPositions::LowCut>();
        updateCutFilterCoefficients(lowCut, cutCoefficients, chainSettings.loCutSlope);
    } else { // HighCut
        // Coefficients for Low Pass/Hi Cut filter. Order needs to be mutliple of 2 in order to have even dBs/Oct values
        auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.hiCutFreq,
                                                                                                          getSampleRate(),
                                                                                                          2 * (chainSettings.hiCutSlope + 1));
        auto &highCut = chain.get<ChainPositions::HighCut>();
        updateCutFilterCoefficients(highCut, cutCoefficients, chainSettings.hiCutSlope);
    }
    
}

template<typename ChainType, typename CoefficientsType>
void SimpleEQAudioProcessor::updateCutFilterCoefficients(ChainType &cutChain, const CoefficientsType &cutCoefficients, const Slope &slope) {
    // All Cut Filter slopes disabled initially
    cutChain.template setBypassed<0>(true);
    cutChain.template setBypassed<1>(true);
    cutChain.template setBypassed<2>(true);
    cutChain.template setBypassed<3>(true);
    // Depending on selected slope, assign coefficients and enable slopes until (and including) that slope Filter
    switch (slope) {
        case Slope_12:
            *cutChain.template get<0>().coefficients = *cutCoefficients[0];
            cutChain.template setBypassed<0>(false);
            break;
        case Slope_24:
            *cutChain.template get<0>().coefficients = *cutCoefficients[0];
            cutChain.template setBypassed<0>(false);
            *cutChain.template get<1>().coefficients = *cutCoefficients[1];
            cutChain.template setBypassed<1>(false);
            break;
        case Slope_36:
            *cutChain.template get<0>().coefficients = *cutCoefficients[0];
            cutChain.template setBypassed<0>(false);
            *cutChain.template get<1>().coefficients = *cutCoefficients[1];
            cutChain.template setBypassed<1>(false);
            *cutChain.template get<2>().coefficients = *cutCoefficients[2];
            cutChain.template setBypassed<2>(false);
            break;
        case Slope_48:
            *cutChain.template get<0>().coefficients = *cutCoefficients[0];
            cutChain.template setBypassed<0>(false);
            *cutChain.template get<1>().coefficients = *cutCoefficients[1];
            cutChain.template setBypassed<1>(false);
            *cutChain.template get<2>().coefficients = *cutCoefficients[2];
            cutChain.template setBypassed<2>(false);
            *cutChain.template get<3>().coefficients = *cutCoefficients[3];
            cutChain.template setBypassed<3>(false);
            break;
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}
