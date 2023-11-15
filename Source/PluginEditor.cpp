/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
lowCutFreqSliderAt(p.valueTreeState, LO_CUT_FREQ, lowCutFreqSlider),
lowCutSlopeSliderAt(p.valueTreeState, LO_CUT_SLOPE, lowCutSlopeSlider),
peakFreqSliderAt(p.valueTreeState, PK_FREQ, peakFreqSlider),
peakGainSliderAt(p.valueTreeState, PK_GAIN, peakGainSlider),
peakQSliderAt(p.valueTreeState, PK_QUALITY, peakQSlider),
hiCutFreqSliderAt(p.valueTreeState, HI_CUT_FREQ, hiCutFreqSlider),
hiCutSlopeSliderAt(p.valueTreeState, HI_CUT_SLOPE, hiCutSlopeSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for (auto* comp: getComponents()) {
        addAndMakeVisible(comp);
    }
    
    setSize (600, 400);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

//    g.setColour (juce::Colours::white);
//    g.setFont (15.0f);
//    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    // Setup area for each section of the EQ
    auto bounds = getLocalBounds();
    // GUI window divided in 3 parts both vertically and horizontally
    // Frequency Response area on the top 3rd part of the window.
    // Filters will be distributed below it; each section occupies the 3rd part
    // from left to right
    auto responseArea = bounds.removeFromTop(bounds.getHeight()*0.33);
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth()*0.33);
    auto hiCutArea = bounds.removeFromRight(bounds.getWidth()*0.5);
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight()*0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    hiCutFreqSlider.setBounds(hiCutArea.removeFromTop(hiCutArea.getHeight()*0.5));
    hiCutSlopeSlider.setBounds(hiCutArea);
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight()*0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight()*0.5));
    peakQSlider.setBounds(bounds);
}
// Wrap GUI components inside vector
std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComponents() {
    return {
        &peakFreqSlider, &peakGainSlider, &peakQSlider,
        &lowCutFreqSlider, &hiCutFreqSlider, &lowCutSlopeSlider,
        &hiCutSlopeSlider
    };
}
