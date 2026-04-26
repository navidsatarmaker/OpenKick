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
    

private:
    OpenKickAudioProcessor& audioProcessor;

    juce::Rectangle<int> curveArea;

    int draggedNode = -1;

    juce::Slider mixSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    juce::Slider smoothSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> smoothAttachment;

    juce::Rectangle<int> shiftLeftBounds;
    juce::Rectangle<int> shiftRightBounds;

    juce::Rectangle<int> rateBounds[6];
    juce::Rectangle<int> triggerBounds[2];
    juce::Rectangle<int> shapeBounds[16];
    juce::Rectangle<int> scopeBounds;
    juce::Rectangle<int> thresholdBounds;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenKickAudioProcessorEditor)
};
