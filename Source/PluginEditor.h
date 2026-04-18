#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class OpenKickAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    OpenKickAudioProcessorEditor (OpenKickAudioProcessor&);
    ~OpenKickAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OpenKickAudioProcessor& audioProcessor;

    juce::Slider mixSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenKickAudioProcessorEditor)
};
