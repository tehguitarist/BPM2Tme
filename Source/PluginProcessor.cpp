#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout PassthroughTempoProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    juce::StringArray divisionChoices { "1/128", "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "division", "Division", divisionChoices, 3));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "syncBpm", "Sync BPM", true));
    
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "manualBpm", "Manual BPM",
        juce::NormalisableRange<float> (20.0f, 300.0f, 0.01f),
        120.0f));

    return { params.begin(), params.end() };
}

PassthroughTempoProcessor::PassthroughTempoProcessor()
    : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
 #if ! JucePlugin_IsSynth
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
 #endif
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                     ),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    apvts.addParameterListener ("division", this);
    apvts.addParameterListener ("syncBpm", this);
    apvts.addParameterListener ("manualBpm", this);
}

PassthroughTempoProcessor::~PassthroughTempoProcessor()
{
    apvts.removeParameterListener ("division", this);
    apvts.removeParameterListener ("syncBpm", this);
    apvts.removeParameterListener ("manualBpm", this);
}

void PassthroughTempoProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    bpmCheckInterval = static_cast<int> (sampleRate * 0.5);
}

void PassthroughTempoProcessor::releaseResources()
{
}

void PassthroughTempoProcessor::parameterChanged (const juce::String& /*paramID*/, float /*newValue*/)
{
}

int PassthroughTempoProcessor::getSelectedDenominator() const
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("division")))
    {
        int idx = p->getIndex();
        const int map[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
        if (idx >= 0 && idx <= 7)
            return map[idx];
    }
    return 16;
}

void PassthroughTempoProcessor::setDivisionIndexNotifyingHost (int choiceIndex)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("division")))
    {
        int numChoices = p->choices.size();
        float normalized = 0.0f;
        if (numChoices > 1)
            normalized = (float) choiceIndex / (float) (numChoices - 1);
        p->setValueNotifyingHost (normalized);
    }
}

double PassthroughTempoProcessor::getEffectiveBpm() const
{
    bool sync = false;
    if (auto* pb = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter ("syncBpm")))
        sync = pb->get();

    if (sync)
        return cachedBpm;

    if (auto* pf = apvts.getRawParameterValue ("manualBpm"))
        return (double) pf->load();

    return 120.0;
}

bool PassthroughTempoProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    const auto& outLayout = layouts.getMainOutputChannelSet();
    const auto& inLayout  = layouts.getMainInputChannelSet();

    if (inLayout == outLayout && ! outLayout.isDisabled())
        return true;

    return false;
#endif
}

void PassthroughTempoProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PassthroughTempoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

void PassthroughTempoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numInput = getTotalNumInputChannels();
    const int numOutput = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = numInput; ch < numOutput; ++ch)
        buffer.clear (ch, 0, numSamples);

    for (int ch = 0; ch < juce::jmin (numInput, numOutput); ++ch)
    {
        float* out = buffer.getWritePointer (ch);
        const float* in = buffer.getReadPointer (ch);
        if (out != in)
            std::memmove (out, in, (size_t) numSamples * sizeof (float));
    }

    // Only check BPM when sync is enabled and transport is playing
    bool shouldCheckBpm = false;
    bool syncEnabled = false;
    
    if (auto* pb = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter ("syncBpm")))
        syncEnabled = pb->get();
    
    if (syncEnabled)
    {
        if (auto* ph = getPlayHead())
        {
            if (auto pos = ph->getPosition())
            {
                if (pos->getIsPlaying())
                    shouldCheckBpm = true;
            }
        }
    }
    
    if (shouldCheckBpm)
    {
        samplesSinceLastBpmCheck += numSamples;
        
        if (samplesSinceLastBpmCheck >= bpmCheckInterval)
        {
            samplesSinceLastBpmCheck = 0;
            checkBpmFromHost();
        }
    }
    else
    {
        // Reset counter when stopped or sync disabled so we check immediately when conditions are met
        samplesSinceLastBpmCheck = 0;
    }
}

void PassthroughTempoProcessor::checkBpmFromHost()
{
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            bool transportChanged = (pos->getIsPlaying() != prevIsPlaying);
            
            bool ppqMoved = false;
            if (auto ppq = pos->getPpqPosition())
            {
                if (prevPpqPosition >= 0.0)
                    ppqMoved = (std::abs (*ppq - prevPpqPosition) > 1.0e-5);
                else
                    ppqMoved = true;
                prevPpqPosition = *ppq;
            }
            
            bool bpmChanged = false;
            if (auto bpm = pos->getBpm())
            {
                if (std::abs (*bpm - prevBpmReported) > 1.0e-6)
                    bpmChanged = true;
                    
                if (transportChanged || ppqMoved || bpmChanged)
                {
                    cachedBpm = *bpm;
                    haveValidBpm = true;
                    prevBpmReported = *bpm;
                }
            }
            
            prevIsPlaying = pos->getIsPlaying();
        }
    }
}

void PassthroughTempoProcessor::forceReadBpmFromHost()
{
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm())
            {
                cachedBpm = *bpm;
                haveValidBpm = true;
                prevBpmReported = *bpm;
            }
            
            if (auto ppq = pos->getPpqPosition())
                prevPpqPosition = *ppq;
            
            prevIsPlaying = pos->getIsPlaying();
        }
    }
}

juce::AudioProcessorEditor* PassthroughTempoProcessor::createEditor()
{
    return new PassthroughTempoEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PassthroughTempoProcessor();
}