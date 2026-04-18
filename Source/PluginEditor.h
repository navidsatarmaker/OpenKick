#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class OpenKickLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle,
                          const float rotaryEndAngle, juce::Slider& slider) override;
};

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

    OpenKickLookAndFeel customLookAndFeel;

    juce::Slider mixSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    juce::ComboBox shapeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;

    juce::ComboBox rateCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAttachment;

    juce::ComboBox triggerCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> triggerAttachment;

    juce::ComboBox themeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> themeAttachment;

    juce::Slider thresholdSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenKickAudioProcessorEditor)
};
