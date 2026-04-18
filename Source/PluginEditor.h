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

private:
    OpenKickAudioProcessor& audioProcessor;

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
