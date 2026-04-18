#include "PluginProcessor.h"
#include "PluginEditor.h"

OpenKickAudioProcessorEditor::OpenKickAudioProcessorEditor (OpenKickAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (450, 350);

    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(mixSlider);

    shapeCombo.addItemList({"Hard Duck", "Sine Pump", "Exponential Pluck", "Linear Ramp", "Custom"}, 1);
    shapeCombo.setSelectedId(1);
    addAndMakeVisible(shapeCombo);
    shapeCombo.onChange = [this] { repaint(); };

    rateCombo.addItemList({"1/8 Note", "1/4 Note", "1/2 Note", "1/1 Bar"}, 2);
    rateCombo.setSelectedId(2);
    addAndMakeVisible(rateCombo);

    triggerCombo.addItemList({"Host Sync", "Audio Sidechain"}, 1);
    triggerCombo.setSelectedId(1);
    addAndMakeVisible(triggerCombo);

    thresholdSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    addAndMakeVisible(thresholdSlider);

    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "MIX", mixSlider);
        
    shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "SHAPE", shapeCombo);

    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "RATE", rateCombo);

    triggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "TRIGGER", triggerCombo);

    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "THRESHOLD", thresholdSlider);

    customNodes.push_back({0.0f, 0.0f});
    customNodes.push_back({0.15f, 0.0f});
    customNodes.push_back({0.5f, 1.0f});
    customNodes.push_back({1.0f, 1.0f});
    updateCustomCurve();

    startTimerHz(60);
}

void OpenKickAudioProcessorEditor::timerCallback()
{
    repaint();
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
    
    float startX = curveArea.getX();
    float startY = curveArea.getY();
    float w = curveArea.getWidth();
    float h = curveArea.getHeight();

    // Draw Oscilloscope Background
    juce::Path scopePath;
    int latestIndex = audioProcessor.scopeIndex.load();
    for (int i = 0; i < 256; ++i)
    {
        int readIndex = (latestIndex + i) % 256;
        float sampleVal = audioProcessor.scopeData[readIndex].load();
        
        float x = startX + (i / 255.0f) * w;
        // Clamp and map sample val (-1.0 to 1.0) to height
        sampleVal = juce::jlimit(-1.0f, 1.0f, sampleVal);
        float y = startY + h / 2.0f - (sampleVal * (h / 2.0f));
        
        if (i == 0) scopePath.startNewSubPath(x, y);
        else scopePath.lineTo(x, y);
    }
    g.setColour(juce::Colours::cyan.withAlpha(0.6f));
    g.strokePath(scopePath, juce::PathStrokeType(1.5f));
    
    // Draw selected curve
    juce::Path curvePath;
    
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
            case 4: gain = audioProcessor.customCurveTable[i].load(); break;
            case 3: default: gain = phase; break;
        }
        
        float x = startX + (phase * w);
        float y = startY + (1.0f - gain) * h;
        
        curvePath.lineTo(x, y);
    }
    g.strokePath(curvePath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved));

    // Draw handles if custom shape is selected
    if (shapeIndex == 4)
    {
        g.setColour(juce::Colours::yellow);
        for (auto& node : customNodes)
        {
            float nx = startX + (node.x * w);
            float ny = startY + (1.0f - node.y) * h;
            g.fillEllipse(nx - 5.0f, ny - 5.0f, 10.0f, 10.0f);
        }
    }
}

void OpenKickAudioProcessorEditor::resized()
{
    mixSlider.setBounds(getWidth() / 2 - 50, getHeight() - 170, 100, 100);
    triggerCombo.setBounds(getWidth() / 2 - 155, getHeight() - 90, 150, 24);
    thresholdSlider.setBounds(getWidth() / 2 + 5, getHeight() - 90, 150, 24);
    shapeCombo.setBounds(getWidth() / 2 - 155, getHeight() - 50, 150, 24);
    rateCombo.setBounds(getWidth() / 2 + 5, getHeight() - 50, 150, 24);
}

void OpenKickAudioProcessorEditor::updateCustomCurve()
{
    for (int i = 0; i < 100; ++i)
    {
        float x = i / 99.0f;
        float y = 0.0f;
        for (size_t n = 0; n < customNodes.size() - 1; ++n)
        {
            if (x >= customNodes[n].x && x <= customNodes[n+1].x)
            {
                float rangeX = customNodes[n+1].x - customNodes[n].x;
                if (rangeX > 0.0f) {
                    float t = (x - customNodes[n].x) / rangeX;
                    y = customNodes[n].y + t * (customNodes[n+1].y - customNodes[n].y);
                } else {
                    y = customNodes[n].y;
                }
                break;
            }
        }
        audioProcessor.customCurveTable[i].store(y);
    }
}

void OpenKickAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (shapeCombo.getSelectedId() != 5) return; // Only for custom shape
    
    auto pos = event.getPosition();
    curveArea = getLocalBounds().withSizeKeepingCentre(300, 150).translated(0, -30);
    
    float w = curveArea.getWidth();
    float h = curveArea.getHeight();
    float startX = curveArea.getX();
    float startY = curveArea.getY();

    for (size_t i = 0; i < customNodes.size(); ++i)
    {
        float nx = startX + (customNodes[i].x * w);
        float ny = startY + (1.0f - customNodes[i].y) * h;
        if (juce::Point<float>(nx, ny).getDistanceFrom(pos.toFloat()) < 15.0f)
        {
            draggedNode = i;
            break;
        }
    }
}

void OpenKickAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (draggedNode == -1) return;

    auto pos = event.getPosition();
    float w = curveArea.getWidth();
    float h = curveArea.getHeight();
    float startX = curveArea.getX();
    float startY = curveArea.getY();

    float targetX = (pos.x - startX) / w;
    float targetY = 1.0f - ((pos.y - startY) / h);

    // Keep within bounds
    targetX = juce::jlimit(0.0f, 1.0f, targetX);
    targetY = juce::jlimit(0.0f, 1.0f, targetY);

    // X axis boundary locking to keep sequence continuous
    if (draggedNode == 0) targetX = 0.0f;
    else if (draggedNode == customNodes.size() - 1) targetX = 1.0f;
    else {
        targetX = juce::jlimit(customNodes[draggedNode - 1].x + 0.01f, customNodes[draggedNode + 1].x - 0.01f, targetX);
    }

    customNodes[draggedNode].x = targetX;
    customNodes[draggedNode].y = targetY;

    updateCustomCurve();
    repaint();
}

void OpenKickAudioProcessorEditor::mouseUp(const juce::MouseEvent& event)
{
    draggedNode = -1;
}
