
#pragma once

#include <JuceHeader.h>

// Constants needed
static const juce::String LO_CUT_FREQ = "LowCut Freq", LO_CUT_SLOPE = "LowCut Slope";
static const juce::String HI_CUT_FREQ = "HiCut Freq", HI_CUT_SLOPE = "HiCut Slope";
static const juce::String PK_FREQ = "Peak Freq", PK_GAIN = "Peak Gain", PK_QUALITY = "Peak Quality";
// Data structure that wraps the param values used by the EQ processing chain
enum Slope {
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};
struct ChainSettings {
    float peakFreq{0}, peakGainInDbs{0}, peakQ{1.f};
    float loCutFreq{0}, hiCutFreq{0};
    // Default value for Cut Filter Slopes -> 12 dB/Oct
    Slope loCutSlope{Slope::Slope_12}, hiCutSlope{Slope::Slope_12};
};

class SimpleEQProcessor {
public:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParamLayout();
    void prepareToPlay(double sampleRate, int samplesPerBlock, juce::AudioProcessorValueTreeState &apvts);
    void process(juce::AudioBuffer<float> &buffer, juce::AudioProcessorValueTreeState &apvts);
private:
    double sampleRate;
    // Used as a helper to idenfity each part of the Filter Processor Chain
    enum ChainPositions {
        LowCut,
        Peak,
        HighCut
    };
    // Create aliases for type name shorthand
    // Filter has a response of 12 dB/Oct when configured as Lo-pass or Hi-pass Filter
    using Filter = juce::dsp::IIR::Filter<float>;
    // Processor Chain of 4 filters for the Cut Filters, since the idea is to have a chain of 4 filters,
    // in order to have Chained Filters that conform a 48 Db/Oct Filter
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    // This would represent the entire signal processing path (Low Cut, Peak, Hi Cut)
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    MonoChain lChain, rChain;
    
    // Helper functions to avoid repeating code
    void updateFilters(juce::AudioProcessorValueTreeState &apvts);
    void updatePeakFilter(const ChainSettings &chainSettings);
    void updateCutFilter(ChainPositions filterPos, const ChainSettings &chainSettings, MonoChain &chain);
    // Generic method with shared code across cut filters coefficients updates
    template<typename ChainType, typename CoefficientsType>
    void updateCutFilterCoefficients(ChainType &cutChain, const CoefficientsType &cutCoefficients, const Slope &slope);
    template<int Index, typename ChainType, typename CoefficientsType>
    void enableCutSlopeFilter(ChainType &cutChain, const CoefficientsType &cutCoefficients);
};
