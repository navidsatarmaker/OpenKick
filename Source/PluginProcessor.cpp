#include "PluginProcessor.h"
#include "PluginEditor.h"

OpenKickAudioProcessor::OpenKickAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OpenKickAudioProcessor::~OpenKickAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout OpenKickAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"MIX", 1}, "Mix", 0.0f, 1.0f, 1.0f));
    
    juce::StringArray shapeChoices = { "Shape 1", "Shape 2", "Shape 3", "Shape 4", "Shape 5", "Shape 6", "Shape 7", "Shape 8",
                                       "Shape 9", "Shape 10", "Shape 11", "Shape 12", "Shape 13", "Shape 14", "Shape 15", "Custom" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"SHAPE", 1}, "Shape", shapeChoices, 0));
        
    juce::StringArray rateChoices = { "1/8 Note", "1/4 Note", "1/2 Note", "1/1 Bar" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"RATE", 1}, "Rate", rateChoices, 1));
        
    juce::StringArray triggerChoices = { "Host Sync", "Audio Sidechain" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"TRIGGER", 1}, "Trigger", triggerChoices, 0));
        
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"THRESHOLD", 1}, "Threshold", -60.0f, 0.0f, -20.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"SMOOTHNESS", 1}, "Smoothness", 0.0f, 1.0f, 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"SHIFT", 1}, "Shift", 0.0f, 1.0f, 0.0f));
    
    return { params.begin(), params.end() };
}

const juce::String OpenKickAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OpenKickAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool OpenKickAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool OpenKickAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double OpenKickAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OpenKickAudioProcessor::getNumPrograms()
{
    return 1;
}

int OpenKickAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OpenKickAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String OpenKickAudioProcessor::getProgramName (int index)
{
    return {};
}

void OpenKickAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void OpenKickAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentPhase = 0.0f;
    smoothGain = 1.0f;
    for (int i = 0; i < 512; ++i) scopeData[i].store(0.0f);
    
    // Default custom curve to a linear ramp
    for (int i = 0; i < 100; ++i) customCurveTable[i] = i / 99.0f;
}

void OpenKickAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool OpenKickAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void OpenKickAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto mixParam = parameters.getRawParameterValue("MIX")->load();
    auto shapeParam = static_cast<int>(parameters.getRawParameterValue("SHAPE")->load());
    auto rateParam = static_cast<int>(parameters.getRawParameterValue("RATE")->load());
    auto triggerMode = static_cast<int>(parameters.getRawParameterValue("TRIGGER")->load());
    float thresholdLinear = juce::Decibels::decibelsToGain(parameters.getRawParameterValue("THRESHOLD")->load());

    float rateMultiplier = 1.0f;
    switch (rateParam)
    {
        case 0: rateMultiplier = 2.0f; break;  // 1/8 triggers 2x per quarter
        case 1: rateMultiplier = 1.0f; break;  // 1/4 Note
        case 2: rateMultiplier = 0.5f; break;  // 1/2 Note
        case 3: rateMultiplier = 0.25f; break; // 1/1 Bar
    }

    // Basic ducking logic based on host tempo
    auto* playHead = getPlayHead();
    juce::AudioPlayHead::PositionInfo positionInfo;
    double bpm = 120.0;
    
    if (playHead != nullptr)
    {
        if (auto pos = playHead->getPosition())
        {
            positionInfo = *pos;
            if (positionInfo.getBpm().hasValue()) bpm = *positionInfo.getBpm();
            
            if (triggerMode == 0 && positionInfo.getIsPlaying() && positionInfo.getPpqPosition().hasValue())
            {
                double ppq = *positionInfo.getPpqPosition();
                // Map PPQ to a 0.0-1.0 phase adjusted by rate
                currentPhase = static_cast<float>(std::fmod(ppq * rateMultiplier, 1.0));
            }
            // Do NOT reset currentPhase to 0.0f here when !isPlaying! 
            // Letting it fall through allows the inter-sample DSP to free-run.
        }
    }

    // Iterate through audio samples
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Handle audio sidechain triggering
        if (triggerMode == 1)
        {
            float scSample = 0.0f;
            if (getBusCount(true) > 1) {
                auto scBus = getBusBuffer(buffer, true, 1);
                for (int ch = 0; ch < scBus.getNumChannels(); ++ch)
                    scSample = juce::jmax(scSample, std::abs(scBus.getReadPointer(ch)[sample]));
            }
            
            bool wasAboveThreshold = (envelopeFollower > thresholdLinear);

            if (scSample > envelopeFollower) envelopeFollower += (scSample - envelopeFollower) * 0.1f;
            else envelopeFollower += (scSample - envelopeFollower) * 0.005f;

            bool isAboveThreshold = (envelopeFollower > thresholdLinear);

            // True Audio Retrigger Logic (Retriggers instantly on new transient)
            if (!wasAboveThreshold && isAboveThreshold) {
                isTriggered = true;
                currentPhase = 0.0f;
            }
            
            if (isTriggered) {
                float phaseIncrement = static_cast<float>(((bpm / 60.0) / getSampleRate()) * rateMultiplier);
                currentPhase += phaseIncrement;
                if (currentPhase >= 1.0f) {
                    currentPhase = 1.0f;
                    if (!isAboveThreshold) isTriggered = false;
                }
            } else {
                currentPhase = 1.0f;
            }
        }
        // Apply Phase Shift Offset
        float shiftParam = *parameters.getRawParameterValue("SHIFT");
        float shiftedPhase = std::fmod(currentPhase + shiftParam, 1.0f);
        if (shiftedPhase < 0.0f) shiftedPhase += 1.0f;

        // Calculate curve target based on shape index and shifted phase
        float targetGain = calculateGainCurve(shiftedPhase, shapeParam); 
        
        // Apply mix
        float actualGain = 1.0f - (mixParam * (1.0f - targetGain));
        
        // Smooth gain to prevent clicks (True Linear Slew Limiting)
        float smoothParam = *parameters.getRawParameterValue("SMOOTHNESS");
        float maxDelta = std::pow(10.0f, -1.0f - (4.0f * smoothParam)); 
        
        float diff = actualGain - smoothGain;
        smoothGain += juce::jlimit(-maxDelta, maxDelta, diff);

        float outSample = 0.0f;
        // Apply Gain to all channels
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            channelData[sample] *= smoothGain;
            if (channel == 0) outSample = channelData[sample]; // Capture CH 0 for scope
        }

        // Waveform Visualizer - Phase Folded Peak Tracker
        int phaseIdx = static_cast<int>(currentPhase * 511.0f);
        if (phaseIdx >= 0 && phaseIdx < 512) {
            float absSample = std::abs(outSample);
            float currentPeak = scopeData[phaseIdx].load();
            if (absSample > currentPeak) {
                scopeData[phaseIdx].store(absSample);
            }
        }

        // Advance host sync phase inter-sample (Always free-run regardless of DAW playback!)
        if (triggerMode == 0)
        {
            float rateParam = (bpm > 0) ? bpm : 120.0f; // Default to 120bpm if Host doesn't send time
            float phaseIncrement = static_cast<float>(((rateParam / 60.0) / getSampleRate()) * rateMultiplier);
            currentPhase = std::fmod(currentPhase + phaseIncrement, 1.0f);
        }
    }
}

float OpenKickAudioProcessor::calculateGainCurve(float phase, int shapeIndex)
{
    phase = juce::jlimit(0.0f, 1.0f, phase);
    switch (shapeIndex)
    {
        case 0: return std::min(1.0f, std::pow(phase * 4.0f, 2.0f)); // Fast rise flat
        case 1: return std::min(1.0f, std::pow(phase * 2.5f, 2.0f)); // Slower rise flat
        case 2: return phase < 0.15f ? 0.0f : std::min(1.0f, std::pow((phase - 0.15f) * 3.0f, 2.0f)); // Delay flat (Yellow Target)
        case 3: return phase < 0.3f ? 0.0f : std::min(1.0f, std::pow((phase - 0.3f) * 4.0f, 2.0f)); // Blocky delay
        case 4: return (1.0f - std::cos(phase * juce::MathConstants<float>::pi)) * 0.5f; // Pure S-curve
        case 5: return phase < 0.2f ? 1.0f - (phase*5.0f) : (phase > 0.8f ? (phase-0.8f)*5.0f : 0.0f); // U-shape
        case 6: return phase < 0.8f ? 0.0f : (phase - 0.8f) / 0.2f; // Late exponential
        case 7: return phase < 0.6f ? 0.0f : std::pow((phase - 0.6f)/0.4f, 3.0f); // Steep spike
        case 8: return phase < 0.1f ? 0.0f : std::min(1.0f, std::pow((phase - 0.1f) * 2.5f, 2.0f)); // Delay fast peak
        case 9: return std::min(1.0f, phase * 3.0f); // Linear jump
        case 10: return phase < 0.2f ? 0.0f : ((phase - 0.2f) / 0.8f); // Late linear
        case 11: return std::pow(phase, 0.5f); // Convex curve
        case 12: return 1.0f - phase; // Downward ramp
        case 13: return std::abs(std::sin(phase * juce::MathConstants<float>::pi * 2.0f)); // Double pump
        case 14: return std::pow(phase, 4.0f); // Fast late
        case 15: // Custom User Shape
        {
            int index = juce::jlimit(0, 99, (int)(phase * 100.0f));
            return customCurveTable[index].load();
        }
        default:
            return phase;
    }
}

bool OpenKickAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OpenKickAudioProcessor::createEditor()
{
    return new OpenKickAudioProcessorEditor (*this);
}

void OpenKickAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void OpenKickAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OpenKickAudioProcessor();
}
