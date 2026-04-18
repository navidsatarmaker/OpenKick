#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class OpenKickAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    OpenKickAudioProcessorEditor (OpenKickAudioProcessor&);
    ~OpenKickAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    
    void updateCustomCurve();

private:
    OpenKickAudioProcessor& audioProcessor;

    juce::Rectangle<int> curveArea;
    std::vector<juce::Point<float>> customNodes;
    int draggedNode = -1;

    juce::Slider mixSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    juce::ComboBox shapeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;

    juce::ComboBox rateCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAttachment;

    juce::ComboBox triggerCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> triggerAttachment;

    juce::Slider thresholdSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenKickAudioProcessorEditor)
};
