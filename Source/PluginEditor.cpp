#include "PluginProcessor.h"
#include "PluginEditor.h"

OpenKickAudioProcessorEditor::OpenKickAudioProcessorEditor (OpenKickAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);

    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(mixSlider);

    shapeCombo.addItemList({"Hard Duck", "Sine Pump", "Exponential Pluck", "Linear Ramp"}, 1);
    shapeCombo.setSelectedId(1);
    addAndMakeVisible(shapeCombo);
    shapeCombo.onChange = [this] { repaint(); };

    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "MIX", mixSlider);
        
    shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "SHAPE", shapeCombo);
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
    
    // Draw selected curve
    juce::Path curvePath;
    float startX = curveArea.getX();
    float startY = curveArea.getY();
    float w = curveArea.getWidth();
    float h = curveArea.getHeight();
    
    int shapeIndex = shapeCombo.getSelectedId() - 1;
    if (shapeIndex < 0) shapeIndex = 0;

    curvePath.startNewSubPath(startX, startY + h);
    
    for (int i = 0; i <= 100; ++i)
    {
        float phase = i / 100.0f;
        float gain = phase;
        
        switch (shapeIndex)
        {
            case 0: gain = phase < 0.15f ? 0.0f : (phase - 0.15f) / 0.85f; break;
            case 1: gain = 1.0f - std::cos(phase * juce::MathConstants<float>::pi * 0.5f); break;
            case 2: gain = std::pow(phase, 2.0f); break;
            case 3: default: gain = phase; break;
        }
        
        float x = startX + (phase * w);
        float y = startY + (1.0f - gain) * h;
        
        curvePath.lineTo(x, y);
    }
    g.strokePath(curvePath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved));
}

void OpenKickAudioProcessorEditor::resized()
{
    mixSlider.setBounds(getWidth() / 2 - 50, getHeight() - 100, 100, 100);
    shapeCombo.setBounds(getWidth() / 2 - 75, getHeight() - 130, 150, 24);
}
