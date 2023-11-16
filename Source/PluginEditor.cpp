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
    // Register the editor as a listener to the parameters
    const auto& params = audioProcessor.getParameters();
    for (auto param: params) {
        param->addListener(this);
    }
    startTimer(60);// 60 Hz
    setSize (600, 400);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    // De-register as parameters listener
    const auto& params = audioProcessor.getParameters();
    for (auto param:params) {
        param->removeListener(this);
    }
//    stopTimer();
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colours::black);
    
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight()*0.33);
    auto w = responseArea.getWidth();
    // Draw borders for the response curve area
    g.setColour(juce::Colours::lightyellow);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
    
    // Calculate magnitudes to be displayed in the frequency response curve
    std::vector<double> mags;
    mags.resize(w);
    audioProcessor.calculateFrequencyResponseMagnitude(mags);
    const double outMin = responseArea.getBottom();
    const double outMax = responseArea.getY();
    // Used to map values in dB to screen coordinates
    auto map = [outMin, outMax] (double input) {
        // min anx max values based on the peak gain parameter bounds
        return juce::jmap(input, (double)PEAK_GAIN_MIN, (double)PEAK_GAIN_MAX, outMin, outMax);
    };
    // Draw response curve
    juce::Path responseCurve;
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    for (size_t i = 1; i < mags.size(); i++) {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }
    g.setColour(juce::Colours::white);
    g.strokePath(responseCurve, juce::PathStrokeType(2.f));
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
    bounds.removeFromTop(bounds.getHeight()*0.33);
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

void SimpleEQAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue) {
    parametersChanged.set(true);
}

void SimpleEQAudioProcessorEditor::timerCallback() {
    // Re-draw Frequency Response curve if any processor parameter has changed
    if (parametersChanged.compareAndSetBool(false, true))
        repaint();
}
