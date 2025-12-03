#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PassthroughTempoEditor : public juce::AudioProcessorEditor,
                               private juce::Timer
{
public:
    explicit PassthroughTempoEditor (PassthroughTempoProcessor&);
    ~PassthroughTempoEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateUiFromParameters();
    void updateMsLabel();
    void updateManualBpmFromHost();

    PassthroughTempoProcessor& processorRef;

    juce::TextButton b128 { "1/128" };
    juce::TextButton b64 { "1/64" };
    juce::TextButton b32 { "1/32" };
    juce::TextButton b16 { "1/16" };
    juce::TextButton b8  { "1/8" };
    juce::TextButton b4  { "1/4" };
    juce::TextButton b2  { "1/2" };
    juce::TextButton b1  { "1/1" };
    
    juce::Label msLabel;
    juce::Label statusLabel;
    juce::Label bpmLabel;

    juce::ToggleButton syncToggle { "Sync to Host" };
    juce::Slider manualBpmSlider;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> manualBpmAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PassthroughTempoEditor)
};
