#pragma once
#include <JuceHeader.h>

class PassthroughTempoProcessor : public juce::AudioProcessor,
                                  private juce::AudioProcessorValueTreeState::Listener
{
public:
    PassthroughTempoProcessor();
    ~PassthroughTempoProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BPM2TIME"; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int /*index*/) override {}
    const juce::String getProgramName (int /*index*/) override { return {}; }
    void changeProgramName (int /*index*/, const juce::String& /*newName*/) override {}

    double getTailLengthSeconds() const override { return 0.0; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    // Public interface for editor
    double getEffectiveBpm() const;
    int getSelectedDenominator() const;
    bool hostProvidedBpm() const { return haveValidBpm; }
    void setDivisionIndexNotifyingHost (int choiceIndex);
    void forceReadBpmFromHost();  // Force immediate BPM read from host

    juce::AudioProcessorValueTreeState apvts;
    double cachedBpm = 120.0;  // Made public so editor can read it

private:
    void parameterChanged (const juce::String& paramID, float newValue) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void checkBpmFromHost();

    bool haveValidBpm = false;
    bool prevIsPlaying = false;
    double prevPpqPosition = -1.0;
    double prevBpmReported = -1.0;
    int samplesSinceLastBpmCheck = 0;
    int bpmCheckInterval = 24000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PassthroughTempoProcessor)
};
