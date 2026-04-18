#include "PluginProcessor.h"
#include "PluginEditor.h"

OpenKickAudioProcessor::OpenKickAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
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
    
    juce::StringArray shapeChoices = { "Hard Duck", "Sine Pump", "Exponential Pluck", "Linear Ramp" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"SHAPE", 1}, "Shape", shapeChoices, 0));
    
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

    // Basic ducking logic based on host tempo
    auto* playHead = getPlayHead();
    juce::AudioPlayHead::PositionInfo positionInfo;
    
    if (playHead != nullptr)
    {
        if (auto pos = playHead->getPosition())
        {
            positionInfo = *pos;
            if (positionInfo.getIsPlaying() && positionInfo.getPpqPosition().hasValue())
            {
                double ppq = *positionInfo.getPpqPosition();
                // Map PPQ to a 0.0-1.0 phase for a quarter note
                currentPhase = static_cast<float>(std::fmod(ppq, 1.0));
            }
            else 
            {
                currentPhase = 0.0f; // Not playing
            }
        }
    }

    auto mixParam = parameters.getRawParameterValue("MIX")->load();
    auto shapeParam = static_cast<int>(parameters.getRawParameterValue("SHAPE")->load());

    // Iterate through audio samples
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Calculate curve target based on shape index
        float targetGain = calculateGainCurve(currentPhase, shapeParam); 
        
        // Apply mix
        float actualGain = 1.0f - (mixParam * (1.0f - targetGain));
        
        // Smooth gain to prevent clicks
        smoothGain += (actualGain - smoothGain) * 0.1f;

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            channelData[sample] *= smoothGain;
        }

        // Advance phase if playing (this is an approximation if we don't recalculate PPQ per sample)
        // Usually you'd calculate phase increment based on BPM and SampleRate
        if (positionInfo.getIsPlaying() && positionInfo.getBpm().hasValue())
        {
            double bpm = *positionInfo.getBpm();
            float phaseIncrement = static_cast<float>((bpm / 60.0) / getSampleRate());
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
