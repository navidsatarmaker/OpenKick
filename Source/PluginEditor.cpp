#include "PluginProcessor.h"
#include "PluginEditor.h"

void OpenKickLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, const float rotaryStartAngle,
                                           const float rotaryEndAngle, juce::Slider& slider)
{
    float radius = juce::jmin(width / 2.0f, height / 2.0f) - 4.0f;
    float centreX = x + width * 0.5f;
    float centreY = y + height * 0.5f;
    float rx = centreX - radius;
    float ry = centreY - radius;
    float rw = radius * 2.0f;
    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Background track
    g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));
    juce::Path trackPath;
    trackPath.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.strokePath(trackPath, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Fill arc
    auto themeParam = slider.getProperties()["ThemeId"].toString();
    juce::Colour fillColour = (themeParam == "1") ? juce::Colours::orange : juce::Colours::cyan;
    g.setColour(fillColour);
    juce::Path fillPath;
    fillPath.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
    g.strokePath(fillPath, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knob dot
    g.setColour(juce::Colours::white);
    juce::Path dotPath;
    dotPath.addRectangle(-2.0f, -radius, 4.0f, 10.0f);
    g.fillPath(dotPath, juce::AffineTransform::rotation(angle).translated(centreX, centreY));
}

OpenKickAudioProcessorEditor::OpenKickAudioProcessorEditor (OpenKickAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (450, 350);
    
    setLookAndFeel(&customLookAndFeel);

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
    
    themeCombo.addItemList({"Cyan & Dark Blue", "Orange & Dark Grey"}, 1);
    themeCombo.setSelectedId(1);
    addAndMakeVisible(themeCombo);

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
        
    themeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "THEME", themeCombo);

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
    setLookAndFeel(nullptr);
}

void OpenKickAudioProcessorEditor::paint (juce::Graphics& g)
{
    int themeId = themeCombo.getSelectedId(); // 1 = Cyan, 2 = Orange
    juce::Colour bgColour = (themeId == 2) ? juce::Colour(0xff1a1a1a) : juce::Colour(0xff0d1b2a);
    juce::Colour primaryColour = (themeId == 2) ? juce::Colours::orange : juce::Colours::cyan;
    
    mixSlider.getProperties().set("ThemeId", juce::String(themeId));

    // Dark smooth gradient background
    juce::ColourGradient bgGrad(bgColour.darker(0.2f), 0, 0, bgColour.brighter(0.1f), 0, getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillAll();

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font(22.0f, juce::Font::bold));
    g.drawFittedText ("OpenKick", getLocalBounds().removeFromTop(40), juce::Justification::centred, 1);

    // Draw Curve Viewer box
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    auto curveArea = getLocalBounds().withSizeKeepingCentre(300, 150).translated(0, -30);
    g.fillRoundedRectangle(curveArea.toFloat(), 8.0f);
    g.setColour(primaryColour.darker(0.4f));
    g.drawRoundedRectangle(curveArea.toFloat(), 8.0f, 2.0f);
    
    float startX = curveArea.getX();
    float startY = curveArea.getY();
    float w = curveArea.getWidth();
    float h = curveArea.getHeight();

    // Realtime Scrolling Waveform (History Buffer)
    juce::Path scopePath;
    int scopeSize = audioProcessor.scopeSize;
    int latestIndex = audioProcessor.scopeIndex.load();
    int renderSamples = juce::jmin(scopeSize, 44100); // Only render ~1s max history
    
    // Build path from older to newest (left to right)
    bool started = false;
    for (int p = 0; p < w; ++p)
    {
        float ratio = 1.0f - (float)p / w; // 1 to 0
        int historyOffset = (int)(ratio * renderSamples);
        int readIndex = latestIndex - historyOffset;
        if (readIndex < 0) readIndex += scopeSize;
        
        float sampleVal = audioProcessor.scopeData[readIndex];
        sampleVal = juce::jlimit(-1.0f, 1.0f, sampleVal); // Peak clamp
        float y = startY + h / 2.0f - (sampleVal * (h / 2.0f));
        float x = startX + p;
        
        if (!started) { scopePath.startNewSubPath(x, y); started = true; }
        else scopePath.lineTo(x, y);
    }
    
    g.setColour(primaryColour.withAlpha(0.3f));
    g.strokePath(scopePath, juce::PathStrokeType(1.0f));

    // Waveform fill
    juce::Path scopeFill = scopePath;
    scopeFill.lineTo(startX + w, startY + h / 2.0f);
    scopeFill.lineTo(startX, startY + h / 2.0f);
    scopeFill.closeSubPath();
    g.setColour(primaryColour.withAlpha(0.1f));
    g.fillPath(scopeFill);

    // Draw selected curve math
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
    
    g.setColour(primaryColour.brighter(0.5f));
    g.strokePath(curvePath, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved));

    // Draw interactive handles if custom shape
    if (shapeIndex == 4)
    {
        g.setColour(juce::Colours::white);
        for (auto& node : customNodes)
        {
            float nx = startX + (node.x * w);
            float ny = startY + (1.0f - node.y) * h;
            g.fillEllipse(nx - 5.0f, ny - 5.0f, 10.0f, 10.0f);
            g.drawEllipse(nx - 5.0f, ny - 5.0f, 10.0f, 10.0f, 1.5f);
        }
    }
}

void OpenKickAudioProcessorEditor::resized()
{
    mixSlider.setBounds(getWidth() / 2 - 50, getHeight() - 170, 100, 100);
    triggerCombo.setBounds(getWidth() / 2 - 155, getHeight() - 90, 150, 24);
    thresholdSlider.setBounds(getWidth() / 2 + 5, getHeight() - 90, 150, 24);
    shapeCombo.setBounds(getWidth() / 2 - 155, getHeight() - 50, 150, 24);
    rateCombo.setBounds(getWidth() / 2 + 5, getHeight() - 50, 70, 24);
    themeCombo.setBounds(getWidth() / 2 + 85, getHeight() - 50, 70, 24);
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
