/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

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
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
