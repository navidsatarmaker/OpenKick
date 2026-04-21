#include "PluginProcessor.h"
#include "PluginEditor.h"

OpenKickAudioProcessorEditor::OpenKickAudioProcessorEditor (OpenKickAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (640, 320);
    setResizable (true, true);
    setResizeLimits (640, 320, 1920, 960);
    getConstrainer()->setFixedAspectRatio(2.0);

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
        audioProcessor.scopeData[i].store(val * 0.985f);
    }
    repaint();
}

OpenKickAudioProcessorEditor::~OpenKickAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void OpenKickAudioProcessorEditor::paint (juce::Graphics& g)
{
    float scale = getWidth() / 800.0f;

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
    g.setFont(juce::Font(18.0f * scale, juce::Font::italic));
    g.drawText("OPEN", 0, 20 * scale, getWidth() * 0.35f, 30 * scale, juce::Justification::centred);
    g.setColour(yellow);
    g.setFont(juce::Font(32.0f * scale, juce::Font::bold));
    g.drawText("KICK", 0, 45 * scale, getWidth() * 0.35f, 40 * scale, juce::Justification::centred);

    // Current Mix readout
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f * scale, juce::Font::bold));
    g.drawText("MIX " + juce::String(currentMix) + "%", 0, getHeight() - (110 * scale), getWidth() * 0.35f, 40 * scale, juce::Justification::centred);

    // Interaction text lines
    g.setColour(textGrey);
    g.setFont(juce::Font(12.0f * scale, juce::Font::bold));
    g.drawText("SMOOTHNESS", 0, getHeight() - (65 * scale), getWidth() * 0.35f, 20 * scale, juce::Justification::centred);

    // Right Panel Background
    juce::Rectangle<int> rightPanel(getWidth() * 0.35f, 0, getWidth() * 0.65f, getHeight());
    g.setColour(panelGrey);
    g.fillRect(rightPanel);

    // Draw Rate Buttons
    juce::String rateTexts[6] = {"1/8", "1/4T", "1/4", "1/2T", "1/2", "1/1"};
    for (int i = 0; i < 6; ++i)
    {
        if (i == currentRate) {
            g.setColour(yellow);
            g.fillRoundedRectangle(rateBounds[i].toFloat(), 12.0f * scale);
            g.setColour(juce::Colours::black);
        } else {
            g.setColour(textGrey);
        }
        g.setFont(juce::Font(14.0f * scale, juce::Font::bold));
        g.drawText(rateTexts[i], rateBounds[i], juce::Justification::centred);
    }

    // Draw Trigger Buttons
    juce::String triggerTexts[2] = {"SYNC", "AUDIO"};
    for (int i = 0; i < 2; ++i)
    {
        if (i == currentTrigger) {
            g.setColour(yellow);
            g.fillRoundedRectangle(triggerBounds[i].toFloat(), 12.0f * scale);
            g.setColour(juce::Colours::black);
        } else {
            g.setColour(textGrey);
        }
        g.setFont(juce::Font(14.0f * scale, juce::Font::bold));
        g.drawText(triggerTexts[i], triggerBounds[i], juce::Justification::centred);
    }

    // Draw Scope Background Box
    g.setColour(panelGrey.darker(0.5f));
    g.fillRoundedRectangle(scopeBounds.toFloat(), 4.0f * scale);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawRoundedRectangle(scopeBounds.toFloat(), 4.0f * scale, 1.0f);

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
    
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.fillPath(scopePath);
    g.setColour(juce::Colours::white);
    g.strokePath(scopePath, juce::PathStrokeType(1.0f * scale, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    // Draw Shift Hover Buttons over Scope
    g.setColour(textGrey);
    juce::Path leftArrow, rightArrow;
    float lX = shiftLeftBounds.getX(); float lY = shiftLeftBounds.getY();
    float rX = shiftRightBounds.getX(); float rY = shiftRightBounds.getY();
    float aW = shiftLeftBounds.getWidth(); float aH = shiftLeftBounds.getHeight();
    
    leftArrow.startNewSubPath(lX + aW * 0.7f, lY + aH * 0.2f);
    leftArrow.lineTo(lX + aW * 0.3f, lY + aH * 0.5f);
    leftArrow.lineTo(lX + aW * 0.7f, lY + aH * 0.8f);
    
    rightArrow.startNewSubPath(rX + aW * 0.3f, rY + aH * 0.2f);
    rightArrow.lineTo(rX + aW * 0.7f, rY + aH * 0.5f);
    rightArrow.lineTo(rX + aW * 0.3f, rY + aH * 0.8f);
    
    g.strokePath(leftArrow, juce::PathStrokeType(4.0f * scale, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
    g.strokePath(rightArrow, juce::PathStrokeType(4.0f * scale, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));

    // Render Math Curve (Thick Yellow) mapped dynamically to UI Phase Shift
    juce::Path curvePath;
    float shiftParam = audioProcessor.parameters.getRawParameterValue("SHIFT")->load();
    for (int i = 0; i <= 100; ++i)
    {
        float phase = i / 100.0f;
        float shiftedPhase = std::fmod(phase + shiftParam, 1.0f);
        if (shiftedPhase < 0.0f) shiftedPhase += 1.0f; // wrap pos
        
        float gain = audioProcessor.calculateGainCurve(shiftedPhase, currentShape);
        
        float cx = startX + (phase * w);
        float cy = startY + (1.0f - gain) * h;
        
        if (i == 0) curvePath.startNewSubPath(cx, cy);
        else curvePath.lineTo(cx, cy);
    }
    g.setColour(yellow);
    g.strokePath(curvePath, juce::PathStrokeType(5.0f * scale, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    // Draw horizontal fading bottom edge
    juce::Path bottomEdge;
    bottomEdge.addLineSegment(juce::Line<float>(startX, startY + h, startX + w, startY + h), 2.0f * scale);
    g.setColour(yellow.withAlpha(0.6f));
    g.strokePath(bottomEdge, juce::PathStrokeType(2.0f * scale));

    // Handle interactive custom nodes
    if (currentShape == 8) // Index 8 is the Custom User Shape
    {
        g.setColour(juce::Colours::white);
        for (auto& node : customNodes)
        {
            float visualPhase = std::fmod(node.x - shiftParam, 1.0f);
            if (visualPhase < 0.0f) visualPhase += 1.0f;
            
            float nx = startX + (visualPhase * w);
            float ny = startY + (1.0f - node.y) * h;
            g.fillEllipse(nx - 6.0f, ny - 6.0f, 12.0f, 12.0f);
        }
    }

    // Draw Shape Blocks (Bottom of Right Panel, 2x8 Grid)
    juce::String shapeNames[16] = {
        "FAST RAMP", "SMOOTH RAMP", "DELAYED RAMP", "BLOCKY RAMP", "S-CURVE", "U-CURVE", "EXP RAMP", "LATE SPIKE",
        "CUSTOM", "LINEAR PEAK", "LATE LINEAR", "CONVEX RAMP", "DOWN RAMP", "DOUBLE PUMP", "FAST LATE", "DELAY PEAK"
    };
    
    for (int i = 0; i < 16; ++i)
    {
        g.setColour(juce::Colour(0xff222222));
        g.fillRoundedRectangle(shapeBounds[i].toFloat(), 4.0f * scale);
        
        if (i == currentShape) {
            g.setColour(yellow);
            g.fillRoundedRectangle(shapeBounds[i].toFloat(), 4.0f * scale);
        } else {
            g.setColour(juce::Colour(0xff444444));
            g.drawRoundedRectangle(shapeBounds[i].toFloat(), 4.0f * scale, 1.0f);
        }

        // Draw name inside block
        g.setColour((i == currentShape) ? juce::Colours::black : textGrey.darker(0.3f));
        g.setFont(juce::Font(7.5f * scale, juce::Font::bold));
        g.drawText(shapeNames[i], shapeBounds[i].translated(0, 2 * scale), juce::Justification::centredTop);

        // Draw miniature curve icon in each block
        g.setColour((i == currentShape) ? juce::Colours::black : textGrey);
        juce::Path iconPath;
        float bX = shapeBounds[i].getX() + (5.0f * scale);
        float bY = shapeBounds[i].getY() + (10.0f * scale);
        float bW = shapeBounds[i].getWidth() - (10.0f * scale);
        float bH = shapeBounds[i].getHeight() - (15.0f * scale);

        if (i == 8) {
            // Draw Stylized Pencil Vector Icon
            juce::Path pencil;
            pencil.startNewSubPath(bX + bW*0.2f, bY + bH*0.8f);
            pencil.lineTo(bX + bW*0.4f, bY + bH*0.9f);
            pencil.lineTo(bX + bW*0.9f, bY + bH*0.4f);
            pencil.lineTo(bX + bW*0.7f, bY + bH*0.2f);
            pencil.closeSubPath();
            
            pencil.startNewSubPath(bX + bW*0.2f, bY + bH*0.8f);
            pencil.lineTo(bX + bW*0.1f, bY + bH*0.9f); // tip
            pencil.lineTo(bX + bW*0.4f, bY + bH*0.9f);
            
            g.strokePath(pencil, juce::PathStrokeType(1.5f * scale, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        } else {
            iconPath.startNewSubPath(bX, bY + bH);
            for (int p = 0; p <= 20; ++p)
            {
                float phase = p / 20.0f;
                float gain = phase;
                switch (i) {
                    case 0: gain = std::min(1.0f, std::pow(phase * 4.0f, 2.0f)); break;
                    case 1: gain = std::min(1.0f, std::pow(phase * 2.5f, 2.0f)); break;
                    case 2: gain = phase < 0.15f ? 0.0f : std::min(1.0f, std::pow((phase - 0.15f) * 3.0f, 2.0f)); break;
                    case 3: gain = phase < 0.3f ? 0.0f : std::min(1.0f, std::pow((phase - 0.3f) * 4.0f, 2.0f)); break;
                    case 4: gain = (1.0f - std::cos(phase * juce::MathConstants<float>::pi)) * 0.5f; break;
                    case 5: gain = phase < 0.2f ? 1.0f - (phase*5.0f) : (phase > 0.8f ? (phase-0.8f)*5.0f : 0.0f); break;
                    case 6: gain = phase < 0.8f ? 0.0f : (phase - 0.8f) / 0.2f; break;
                    case 7: gain = phase < 0.6f ? 0.0f : std::pow((phase - 0.6f)/0.4f, 3.0f); break;
                    case 9: gain = std::min(1.0f, phase * 3.0f); break;
                    case 10: gain = phase < 0.2f ? 0.0f : ((phase - 0.2f) / 0.8f); break;
                    case 11: gain = std::pow(phase, 0.5f); break;
                    case 12: gain = 1.0f - phase; break;
                    case 13: gain = std::abs(std::sin(phase * juce::MathConstants<float>::pi * 2.0f)); break;
                    case 14: gain = std::pow(phase, 4.0f); break;
                    case 15: gain = phase < 0.1f ? 0.0f : std::min(1.0f, std::pow((phase - 0.1f) * 2.5f, 2.0f)); break;
                }
                iconPath.lineTo(bX + (phase * bW), bY + (1.0f - gain) * bH);
            }
            g.strokePath(iconPath, juce::PathStrokeType(2.5f * scale, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
}

void OpenKickAudioProcessorEditor::resized()
{
    float scale = getWidth() / 800.0f;
    float leftPanelRatio = 0.35f;
    int leftWidth = getWidth() * leftPanelRatio;
    
    // Mix dial dead center of left panel
    float mixSize = 240.0f * scale;
    mixSlider.setBounds(leftWidth / 2 - (mixSize/2), getHeight() / 2 - (mixSize/2), mixSize, mixSize);

    // Smoothness Slider
    smoothSlider.setBounds(leftWidth / 2 - (60 * scale), getHeight() - (45 * scale), 120 * scale, 12 * scale);

    // Right Panel Areas
    int rightX = leftWidth + 10;
    int rightWidth = getWidth() - leftWidth - 20;

    // Top Tabs (Rate & Trigger) proportionally scaled
    int tabY = 15 * scale;
    float rateW = rightWidth * 0.08f;
    for (int i = 0; i < 6; ++i) rateBounds[i] = juce::Rectangle<int>(rightX + (i * (rateW + (5*scale))), tabY, rateW, 30 * scale);
    
    float trigW = rightWidth * 0.12f;
    int trigStartX = rightX + rightWidth - (trigW * 2) - (5*scale);
    triggerBounds[0] = juce::Rectangle<int>(trigStartX, tabY, trigW, 30 * scale);
    triggerBounds[1] = juce::Rectangle<int>(trigStartX + trigW + (5*scale), tabY, trigW, 30 * scale);

    // Shape Box Grid dynamically scaled 2x8
    int numCols = 8;
    int numRows = 2;
    int shapeBoxH = 35 * scale;
    float shapeBoxW = (rightWidth - (numCols - 1) * (6*scale)) / (float)numCols; // 6px gap
    int shapeY = getHeight() - (shapeBoxH * 2) - (15*scale);
    
    for (int i = 0; i < 16; ++i) {
        int col = i % numCols;
        int row = i / numCols;
        shapeBounds[i] = juce::Rectangle<int>(rightX + (col * (shapeBoxW + (6*scale))), shapeY + (row * (shapeBoxH + (6*scale))), shapeBoxW, shapeBoxH);
    }

    // Oscilloscope dynamically scaled relative to the 2-grid
    scopeBounds = juce::Rectangle<int>(rightX, 60 * scale, rightWidth, shapeY - (75 * scale));
    
    // Shift Buttons hovering mid-screen over scope
    shiftLeftBounds = juce::Rectangle<int>(scopeBounds.getX() + (10*scale), scopeBounds.getY() + scopeBounds.getHeight() / 2 - (20*scale), 30*scale, 40*scale);
    shiftRightBounds = juce::Rectangle<int>(scopeBounds.getRight() - (40*scale), scopeBounds.getY() + scopeBounds.getHeight() / 2 - (20*scale), 30*scale, 40*scale);
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
    for (int i = 0; i < 6; ++i) {
        if (rateBounds[i].contains(pos)) {
            audioProcessor.parameters.getParameter("RATE")->setValueNotifyingHost(i / 5.0f);
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
    for (int i = 0; i < 16; ++i) {
        if (shapeBounds[i].contains(pos)) {
            audioProcessor.parameters.getParameter("SHAPE")->setValueNotifyingHost(i / 15.0f);
            repaint();
            return;
        }
    }
    
    // Check Shift Nudge Links
    if (shiftLeftBounds.contains(pos) || shiftRightBounds.contains(pos)) {
        auto* shiftParam = audioProcessor.parameters.getParameter("SHIFT");
        float currentShift = shiftParam->convertFrom0to1(shiftParam->getValue()); // Get 0.0 to 1.0 real val
        currentShift += shiftLeftBounds.contains(pos) ? -0.05f : 0.05f;
        if (currentShift < 0.0f) currentShift += 1.0f;
        if (currentShift > 1.0f) currentShift -= 1.0f;
        shiftParam->setValueNotifyingHost(shiftParam->convertTo0to1(currentShift));
        repaint();
        return;
    }

    int currentShape = static_cast<int>(audioProcessor.parameters.getRawParameterValue("SHAPE")->load());
    if (currentShape != 8) return; // Custom Nodes
    
    float w = scopeBounds.getWidth();
    float h = scopeBounds.getHeight();
    float startX = scopeBounds.getX();
    float startY = scopeBounds.getY();
    float shiftParam = audioProcessor.parameters.getRawParameterValue("SHIFT")->load();

    for (size_t i = 0; i < customNodes.size(); ++i)
    {
        float visualPhase = std::fmod(customNodes[i].x - shiftParam, 1.0f);
        if (visualPhase < 0.0f) visualPhase += 1.0f;
        
        float nx = startX + (visualPhase * w);
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
    
    float shiftParam = audioProcessor.parameters.getRawParameterValue("SHIFT")->load();
    targetX = std::fmod(targetX + shiftParam, 1.0f); // Convert visual backwards to internal logic

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
