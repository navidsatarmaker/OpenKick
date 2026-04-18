#include "PluginProcessor.h"
#include "PluginEditor.h"

OpenKickAudioProcessorEditor::OpenKickAudioProcessorEditor (OpenKickAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);

    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(mixSlider);

    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "MIX", mixSlider);
}

OpenKickAudioProcessorEditor::~OpenKickAudioProcessorEditor()
{
}

void OpenKickAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("OpenKick - Mix", getLocalBounds().removeFromTop(30), juce::Justification::centred, 1);

    // Draw a placeholder for the curve viewer
    g.setColour(juce::Colours::darkgrey);
    auto curveArea = getLocalBounds().withSizeKeepingCentre(300, 150).translated(0, -30);
    g.fillRect(curveArea);
    g.setColour(juce::Colours::green);
    g.drawRect(curveArea, 2.0f);
    
    // Draw dummy curve
    juce::Path dummyCurve;
    dummyCurve.startNewSubPath(curveArea.getX(), curveArea.getY() + curveArea.getHeight());
    dummyCurve.lineTo(curveArea.getX() + curveArea.getWidth(), curveArea.getY());
    g.strokePath(dummyCurve, juce::PathStrokeType(2.0f));
}

void OpenKickAudioProcessorEditor::resized()
{
    mixSlider.setBounds(getWidth() / 2 - 50, getHeight() - 100, 100, 100);
}
