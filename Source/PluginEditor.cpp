#include "PluginProcessor.h"
#include "PluginEditor.h"

void OpenKickLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, const float rotaryStartAngle,
                                           const float rotaryEndAngle, juce::Slider& slider)
{
    float radius = juce::jmin(width / 2.0f, height / 2.0f) - 10.0f;
    float centreX = x + width * 0.5f;
    float centreY = y + height * 0.5f;
    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Thick track arc (Dark Grey)
    g.setColour(juce::Colour(0xff2a2a2a));
    juce::Path trackPath;
    trackPath.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.strokePath(trackPath, juce::PathStrokeType(12.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Fill arc (Yellow)
    g.setColour(juce::Colour(0xfffcee0a));
    juce::Path fillPath;
    fillPath.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
    g.strokePath(fillPath, juce::PathStrokeType(12.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Big central dark knob body
    g.setColour(juce::Colour(0xff222222));
    g.fillEllipse(centreX - radius + 8.0f, centreY - radius + 8.0f, (radius - 8.0f) * 2.0f, (radius - 8.0f) * 2.0f);
    
    // Inner slightly lighter ring
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawEllipse(centreX - radius + 8.0f, centreY - radius + 8.0f, (radius - 8.0f) * 2.0f, (radius - 8.0f) * 2.0f, 4.0f);

    // Huge dot
    g.setColour(juce::Colour(0xff181818));
    juce::Path dotPath;
    dotPath.addEllipse(-8.0f, -radius + 20.0f, 16.0f, 16.0f);
    g.fillPath(dotPath, juce::AffineTransform::rotation(angle).translated(centreX, centreY));
}

OpenKickAudioProcessorEditor::OpenKickAudioProcessorEditor (OpenKickAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (800, 420);
    setResizable (true, true);
    setResizeLimits (600, 315, 1600, 840);
    
    setLookAndFeel(&customLookAndFeel);

    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(mixSlider);

    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "MIX", mixSlider);

    smoothSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    smoothSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    smoothSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff2a2a2a));
    smoothSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff111111));
    smoothSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xfffcee0a));
    addAndMakeVisible(smoothSlider);

    smoothAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "SMOOTHNESS", smoothSlider);

    customNodes.push_back({0.0f, 0.0f});
    customNodes.push_back({0.15f, 0.0f});
    customNodes.push_back({0.5f, 1.0f});
    customNodes.push_back({1.0f, 1.0f});
    updateCustomCurve();

    startTimerHz(60);
}

void OpenKickAudioProcessorEditor::timerCallback()
{
    // Fade the phase-folded scope buffer over time
    for(int i = 0; i < 512; ++i) {
        float val = audioProcessor.scopeData[i].load();
        audioProcessor.scopeData[i].store(val * 0.95f);
    }
    repaint();
}

OpenKickAudioProcessorEditor::~OpenKickAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void OpenKickAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background Pitch Black/Grey
    g.fillAll(juce::Colour(0xff181818));

    int currentRate = static_cast<int>(audioProcessor.parameters.getRawParameterValue("RATE")->load());
    int currentTrigger = static_cast<int>(audioProcessor.parameters.getRawParameterValue("TRIGGER")->load());
    int currentShape = static_cast<int>(audioProcessor.parameters.getRawParameterValue("SHAPE")->load());
    int currentMix = static_cast<int>(audioProcessor.parameters.getRawParameterValue("MIX")->load() * 100.0f);

    juce::Colour yellow = juce::Colour(0xfffcee0a);
    juce::Colour textGrey = juce::Colour(0xffaaaaaa);
    juce::Colour panelGrey = juce::Colour(0xff111111);

    // Left Panel Titles
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(18.0f, juce::Font::italic));
    g.drawText("OPEN", 0, 20, getWidth() * 0.35f, 30, juce::Justification::centred);
    g.setColour(yellow);
    g.setFont(juce::Font(32.0f, juce::Font::bold));
    g.drawText("KICK", 0, 45, getWidth() * 0.35f, 40, juce::Justification::centred);

    // Current Mix readout
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("MIX " + juce::String(currentMix) + "%", 0, getHeight() - 75, getWidth() * 0.35f, 40, juce::Justification::centred);

    // Smoothness Label
    g.setColour(textGrey);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("SMOOTHNESS", 0, getHeight() - 35, getWidth() * 0.35f, 20, juce::Justification::centred);

    // Right Panel Background
    juce::Rectangle<int> rightPanel(getWidth() * 0.35f, 0, getWidth() * 0.65f, getHeight());
    g.setColour(panelGrey);
    g.fillRect(rightPanel);

    // Draw Rate Buttons
    juce::String rateTexts[4] = {"1/8", "1/4", "1/2", "1/1"};
    for (int i = 0; i < 4; ++i)
    {
        if (i == currentRate) {
            g.setColour(yellow);
            g.fillRoundedRectangle(rateBounds[i].toFloat(), 12.0f);
            g.setColour(juce::Colours::black);
        } else {
            g.setColour(textGrey);
        }
        g.setFont(juce::Font(16.0f, juce::Font::bold));
        g.drawText(rateTexts[i], rateBounds[i], juce::Justification::centred);
    }

    // Draw Trigger Buttons
    juce::String triggerTexts[2] = {"SYNC", "AUDIO"};
    for (int i = 0; i < 2; ++i)
    {
        if (i == currentTrigger) {
            g.setColour(yellow);
            g.fillRoundedRectangle(triggerBounds[i].toFloat(), 12.0f);
            g.setColour(juce::Colours::black);
        } else {
            g.setColour(textGrey);
        }
        g.setFont(juce::Font(16.0f, juce::Font::bold));
        g.drawText(triggerTexts[i], triggerBounds[i], juce::Justification::centred);
    }

    // Draw Scope Background Box
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(scopeBounds.toFloat(), 4.0f);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawRoundedRectangle(scopeBounds.toFloat(), 4.0f, 1.0f);

    float startX = scopeBounds.getX();
    float startY = scopeBounds.getY();
    float w = scopeBounds.getWidth();
    float h = scopeBounds.getHeight();

    // Render Audio Waveform (Folded Cycle) Inside Scope
    juce::Path scopePath;
    
    bool started = false;
    for (int p = 0; p < w; ++p)
    {
        float ratio = (float)p / w; 
        int phaseIdx = static_cast<int>(ratio * 511.0f);
        
        float sampleVal = audioProcessor.scopeData[phaseIdx].load();
        sampleVal = juce::jlimit(0.0f, 1.0f, sampleVal); // Peak clamp pos
        
        float yCenter = startY + h / 2.0f;
        float heightOffset = sampleVal * (h / 2.0f); // Make vertically larger
        float x = startX + p;
        
        // Draw symmetric envelope outline
        if (!started) { scopePath.startNewSubPath(x, yCenter - heightOffset); started = true; }
        else scopePath.lineTo(x, yCenter - heightOffset);
    }
    
    // Draw mirrored bottom
    for (int p = w - 1; p >= 0; --p) {
        float ratio = (float)p / w; 
        int phaseIdx = static_cast<int>(ratio * 511.0f);
        float sampleVal = audioProcessor.scopeData[phaseIdx].load();
        sampleVal = juce::jlimit(0.0f, 1.0f, sampleVal);
        float yCenter = startY + h / 2.0f;
        float heightOffset = sampleVal * (h / 2.0f); // Make vertically larger
        scopePath.lineTo(startX + p, yCenter + heightOffset);
    }
    scopePath.closeSubPath();
    
    g.setColour(juce::Colour(0xff999999).withAlpha(0.8f));
    g.strokePath(scopePath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(juce::Colour(0xff666666).withAlpha(0.4f));
    g.fillPath(scopePath);

    // Render Math Curve (Thick Yellow)
    juce::Path curvePath;
    curvePath.startNewSubPath(startX, startY + h);
    for (int i = 0; i <= 100; ++i)
    {
        float phase = i / 100.0f;
        float gain = phase;
        
        switch (currentShape)
        {
            case 0: gain = phase < 0.15f ? 0.0f : (phase - 0.15f) / 0.85f; break;
            case 1: gain = 1.0f - std::cos(phase * juce::MathConstants<float>::pi * 0.5f); break;
            case 2: gain = std::pow(phase, 2.0f); break;
            case 4: gain = audioProcessor.customCurveTable[i].load(); break;
            case 3: default: gain = phase; break;
        }
        
        float cx = startX + (phase * w);
        float cy = startY + (1.0f - gain) * h;
        curvePath.lineTo(cx, cy);
    }
    g.setColour(yellow);
    g.strokePath(curvePath, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    // Draw horizontal fading bottom edge
    juce::Path bottomEdge;
    bottomEdge.addLineSegment(juce::Line<float>(startX, startY + h, startX + w, startY + h), 2.0f);
    g.setColour(yellow.withAlpha(0.6f));
    g.strokePath(bottomEdge, juce::PathStrokeType(2.0f));

    // Handle interactive custom nodes
    if (currentShape == 4)
    {
        g.setColour(juce::Colours::white);
        for (auto& node : customNodes)
        {
            float nx = startX + (node.x * w);
            float ny = startY + (1.0f - node.y) * h;
            g.fillEllipse(nx - 6.0f, ny - 6.0f, 12.0f, 12.0f);
        }
    }

    // Draw Shape Blocks (Bottom of Right Panel)
    for (int i = 0; i < 5; ++i)
    {
        g.setColour(juce::Colour(0xff222222));
        g.fillRoundedRectangle(shapeBounds[i].toFloat(), 4.0f);
        
        if (i == currentShape) {
            g.setColour(yellow);
            g.fillRoundedRectangle(shapeBounds[i].toFloat(), 4.0f);
        } else {
            g.setColour(juce::Colour(0xff444444));
            g.drawRoundedRectangle(shapeBounds[i].toFloat(), 4.0f, 1.0f);
        }

        // Draw miniature curve icon in each block
        g.setColour((i == currentShape) ? juce::Colours::black : textGrey);
        juce::Path iconPath;
        float bX = shapeBounds[i].getX() + 5.0f;
        float bY = shapeBounds[i].getY() + 5.0f;
        float bW = shapeBounds[i].getWidth() - 10.0f;
        float bH = shapeBounds[i].getHeight() - 10.0f;

        iconPath.startNewSubPath(bX, bY + bH);
        for (int p = 0; p <= 20; ++p)
        {
            float phase = p / 20.0f;
            float gain = phase;
            switch (i) {
                case 0: gain = phase < 0.15f ? 0.0f : (phase - 0.15f) / 0.85f; break;
                case 1: gain = 1.0f - std::cos(phase * juce::MathConstants<float>::pi * 0.5f); break;
                case 2: gain = std::pow(phase, 2.0f); break;
                case 3: gain = phase; break;
                case 4: gain = phase < 0.5f ? 0.0f : 1.0f; break; // Simplified custom icon
            }
            iconPath.lineTo(bX + (phase * bW), bY + (1.0f - gain) * bH);
        }
        g.strokePath(iconPath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        if (i == 4) {
            g.setColour(textGrey.darker(0.3f));
            g.setFont(juce::Font(10.0f, juce::Font::bold));
            g.drawText("CUSTOM", shapeBounds[i].translated(0, 26), juce::Justification::centred);
        }
    }
}

void OpenKickAudioProcessorEditor::resized()
{
    float leftPanelRatio = 0.35f;
    int leftWidth = getWidth() * leftPanelRatio;
    
    // Mix dial dead center of left panel
    mixSlider.setBounds(leftWidth / 2 - 120, getHeight() / 2 - 120, 240, 240);

    // Smoothness Slider
    smoothSlider.setBounds(leftWidth / 2 - 60, getHeight() - 20, 120, 12);

    // Right Panel Areas
    int rightX = leftWidth + 10;
    int rightWidth = getWidth() - leftWidth - 20;

    // Top Tabs (Rate & Trigger) proportionally scaled
    int tabY = 15;
    float rateW = rightWidth * 0.12f;
    for (int i = 0; i < 4; ++i) rateBounds[i] = juce::Rectangle<int>(rightX + (i * (rateW + 5)), tabY, rateW, 30);
    
    float trigW = rightWidth * 0.15f;
    int trigStartX = rightX + rightWidth - (trigW * 2) - 5;
    triggerBounds[0] = juce::Rectangle<int>(trigStartX, tabY, trigW, 30);
    triggerBounds[1] = juce::Rectangle<int>(trigStartX + trigW + 5, tabY, trigW, 30);

    // Shape Box Grid dynamically scaled
    int shapeBoxH = 40;
    float shapeBoxW = (rightWidth - 32) / 5.0f;
    int shapeY = getHeight() - shapeBoxH - 25;
    for (int i = 0; i < 5; ++i) {
        shapeBounds[i] = juce::Rectangle<int>(rightX + (i * (shapeBoxW + 8)), shapeY, shapeBoxW, shapeBoxH);
    }

    // Oscilloscope dynamically scaled
    scopeBounds = juce::Rectangle<int>(rightX, 60, rightWidth, shapeY - 75);
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
    auto pos = event.getPosition();
    
    // Check Rate clicks
    for (int i = 0; i < 4; ++i) {
        if (rateBounds[i].contains(pos)) {
            audioProcessor.parameters.getParameter("RATE")->setValueNotifyingHost(i / 3.0f);
            repaint();
            return;
        }
    }

    // Check Trigger clicks
    for (int i = 0; i < 2; ++i) {
        if (triggerBounds[i].contains(pos)) {
            audioProcessor.parameters.getParameter("TRIGGER")->setValueNotifyingHost(i / 1.0f);
            repaint();
            return;
        }
    }

    // Check Shape clicks
    for (int i = 0; i < 5; ++i) {
        if (shapeBounds[i].contains(pos)) {
            // Dropdown size is 5 elements, so param norm is i / 4.0f
            audioProcessor.parameters.getParameter("SHAPE")->setValueNotifyingHost(i / 4.0f);
            repaint();
            return;
        }
    }

    int currentShape = static_cast<int>(audioProcessor.parameters.getRawParameterValue("SHAPE")->load());
    if (currentShape != 4) return; // Custom Nodes
    
    float w = scopeBounds.getWidth();
    float h = scopeBounds.getHeight();
    float startX = scopeBounds.getX();
    float startY = scopeBounds.getY();

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
    float w = scopeBounds.getWidth();
    float h = scopeBounds.getHeight();
    float startX = scopeBounds.getX();
    float startY = scopeBounds.getY();

    float targetX = (pos.x - startX) / w;
    float targetY = 1.0f - ((pos.y - startY) / h);

    targetX = juce::jlimit(0.0f, 1.0f, targetX);
    targetY = juce::jlimit(0.0f, 1.0f, targetY);

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
