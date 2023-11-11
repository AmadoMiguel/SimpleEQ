/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

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

//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // Declare AudioProcessorValueTreeState which holds the Plugin Parameters needed
    // for the project. ParameterLayout is set separately from a function
    static juce::AudioProcessorValueTreeState::ParameterLayout createParamLayout();
    juce::AudioProcessorValueTreeState valueTreeState {*this, nullptr, "Parameters", createParamLayout()};

private:
    // Create aliases for type name shorthand
    // Filter has a response of 12 dB/Oct when configured as Lo-pass or Hi-pass Filter
    using Filter = juce::dsp::IIR::Filter<float>;
    // Processor Chain of 4 filters for the Cut Filters, since the idea is to have a chain of 4 filters,
    // in order to have Chained Filters that conform a 48 Db/Oct Filter
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    // This would represent the entire signal processing path (Low Cut, Peak, Hi Cut)
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    MonoChain lChain, rChain;
    // Used as a helper to idenfity each part of the Filter Processor Chain
    enum ChainPositions {
        LowCut,
        Peak,
        HighCut
    };
    ChainSettings getChainSettings(juce::AudioProcessorValueTreeState &apvts);
    // Helper functions to avoid repeating code
    void updatePeakFilter(const ChainSettings &chainSettings);
    void updateCutFilter(ChainPositions filterPos, const ChainSettings &chainSettings, MonoChain &chain);
    // Generic method with shared code across cut filters coefficients updates
    template<typename ChainType, typename CoefficientsType>
    void updateCutFilterCoefficients(ChainType &cutChain, const CoefficientsType &cutCoefficients, const Slope &slope);
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
