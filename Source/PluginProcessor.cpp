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
    
    juce::StringArray shapeChoices = { "Hard Duck", "Sine Pump", "Exponential Pluck", "Linear Ramp", "Custom" };
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
    for (int i = 0; i < 256; ++i) scopeData[i] = 0.0f;
    scopeIndex = 0;
    
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
            else if (triggerMode == 0 && !positionInfo.getIsPlaying())
            {
                currentPhase = 0.0f; // Not playing
            }
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
            
            if (scSample > envelopeFollower) envelopeFollower += (scSample - envelopeFollower) * 0.1f;
            else envelopeFollower += (scSample - envelopeFollower) * 0.005f;

            if (!isTriggered && envelopeFollower > thresholdLinear) {
                isTriggered = true;
                currentPhase = 0.0f;
            }
            
            if (isTriggered) {
                float phaseIncrement = static_cast<float>(((bpm / 60.0) / getSampleRate()) * rateMultiplier);
                currentPhase += phaseIncrement;
                if (currentPhase >= 1.0f) {
                    currentPhase = 1.0f;
                    if (envelopeFollower < thresholdLinear * 0.8f) isTriggered = false;
                }
            } else {
                currentPhase = 1.0f;
            }
        }
        // Calculate curve target based on shape index
        float targetGain = calculateGainCurve(currentPhase, shapeParam); 
        
        // Apply mix
        float actualGain = 1.0f - (mixParam * (1.0f - targetGain));
        
        // Smooth gain to prevent clicks
        smoothGain += (actualGain - smoothGain) * 0.1f;

        float outSample = 0.0f;
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            channelData[sample] *= smoothGain;
            if (channel == 0) outSample = channelData[sample];
        }

        // Write mono mix scope
        int idx = scopeIndex.load();
        scopeData[idx] = outSample;
        scopeIndex.store((idx + 1) % 256);

        // Advance host sync phase inter-sample (only if in host sync mode)
        if (triggerMode == 0 && positionInfo.getIsPlaying())
        {
            float phaseIncrement = static_cast<float>(((bpm / 60.0) / getSampleRate()) * rateMultiplier);
            currentPhase = std::fmod(currentPhase + phaseIncrement, 1.0f);
        }
    }
}

float OpenKickAudioProcessor::calculateGainCurve(float phase, int shapeIndex)
{
    phase = juce::jlimit(0.0f, 1.0f, phase);
    switch (shapeIndex)
    {
        case 0: // Hard Duck (Triangle/Square)
            return phase < 0.15f ? 0.0f : (phase - 0.15f) / 0.85f;
        case 1: // Sine Pump
            return 1.0f - std::cos(phase * juce::MathConstants<float>::pi * 0.5f);
        case 2: // Exponential Pluck
            return std::pow(phase, 2.0f);
        case 4: // Custom User Shape
        {
            int index = juce::jlimit(0, 99, (int)(phase * 100.0f));
            return customCurveTable[index].load();
        }
        case 3: // Linear Ramp
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
