#include "PluginEditor.h"
#include "PluginProcessor.h"

PassthroughTempoEditor::PassthroughTempoEditor (PassthroughTempoProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    struct BtnInfo { juce::TextButton* btn; int idx; };
    std::vector<BtnInfo> btns {
        { &b128, 0 }, { &b64, 1 }, { &b32, 2 }, { &b16, 3 },
        { &b8, 4 }, { &b4, 5 }, { &b2, 6 }, { &b1, 7 }
    };

    for (auto& bi : btns)
    {
        addAndMakeVisible (*bi.btn);
        bi.btn->setClickingTogglesState (true);
        bi.btn->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2a2a));
        bi.btn->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff4a90e2));
        bi.btn->setColour (juce::TextButton::textColourOffId, juce::Colours::lightgrey);
        bi.btn->setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        
        bi.btn->onClick = [this, idx = bi.idx, btn = bi.btn]()
        {
            if (! btn->getToggleState())
            {
                btn->setToggleState (true, juce::dontSendNotification);
                return;
            }
            
            // Force read BPM if sync is enabled
            bool syncEnabled = false;
            if (auto* pb = dynamic_cast<juce::AudioParameterBool*> (processorRef.apvts.getParameter ("syncBpm")))
                syncEnabled = pb->get();
            
            if (syncEnabled)
                processorRef.forceReadBpmFromHost();
            
            processorRef.setDivisionIndexNotifyingHost (idx);
            updateUiFromParameters();
            updateMsLabel();
        };
    }

    addAndMakeVisible (syncToggle);
    syncToggle.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    syncToggle.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xff4a90e2));
    syncToggle.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    
    syncToggle.onClick = [this]()
    {
        // When sync is enabled, immediately read BPM from host
        if (syncToggle.getToggleState())
            processorRef.forceReadBpmFromHost();
    };
    
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processorRef.apvts, "syncBpm", syncToggle);

    manualBpmSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    manualBpmSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 80, 20);
    manualBpmSlider.setRange (20.0, 300.0, 0.01);
    manualBpmSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xff4a4a4a));
    manualBpmSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xff4a90e2));
    manualBpmSlider.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff1a1a1a));
    manualBpmSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    manualBpmSlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff2a2a2a));
    manualBpmSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    
    addAndMakeVisible (manualBpmSlider);
    manualBpmAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "manualBpm", manualBpmSlider);

    addAndMakeVisible (bpmLabel);
    bpmLabel.setText ("BPM", juce::dontSendNotification);
    bpmLabel.setJustificationType (juce::Justification::centredLeft);
    bpmLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    bpmLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);

    addAndMakeVisible (msLabel);
    msLabel.setJustificationType (juce::Justification::centred);
    msLabel.setFont (juce::FontOptions (32.0f, juce::Font::bold));
    msLabel.setColour (juce::Label::textColourId, juce::Colour (0xff4a90e2));
    msLabel.setColour (juce::Label::backgroundColourId, juce::Colour (0xff1a1a1a));

    addAndMakeVisible (statusLabel);
    statusLabel.setJustificationType (juce::Justification::centred);
    statusLabel.setFont (juce::FontOptions (12.0f));
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::grey);

    setSize (680, 250);

    // Force read BPM when plugin is opened if sync is enabled
    bool syncEnabled = false;
    if (auto* pb = dynamic_cast<juce::AudioParameterBool*> (processorRef.apvts.getParameter ("syncBpm")))
        syncEnabled = pb->get();
    
    if (syncEnabled)
        processorRef.forceReadBpmFromHost();

    updateUiFromParameters();
    startTimerHz (2);
}

PassthroughTempoEditor::~PassthroughTempoEditor()
{
    stopTimer();
    syncAttachment.reset();
    manualBpmAttachment.reset();
}

void PassthroughTempoEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));
    
    auto headerArea = getLocalBounds().removeFromTop (40);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2a2a), 0, 0,
                                             juce::Colour (0xff1a1a1a), 0, (float) headerArea.getBottom(),
                                             false));
    g.fillRect (headerArea);
    
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText ("BPM to Time", headerArea.reduced (15, 0), juce::Justification::centredLeft);
    
    g.setColour (juce::Colour (0xff3a3a3a));
    g.drawLine (0, 40, (float) getWidth(), 40, 1.0f);
    
    g.setColour (juce::Colours::lightgrey);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("NOTE DIVISION", 15, 50, 150, 20, juce::Justification::centredLeft);
}

void PassthroughTempoEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (40);
    
    auto content = bounds.reduced (15, 10);
    content.removeFromTop (20);
    
    auto btnArea = content.removeFromTop (50);
    int spacing = 8;
    int btnW = (btnArea.getWidth() - spacing * 7) / 8;
    int x = btnArea.getX();
    
    for (auto* b : { &b128, &b64, &b32, &b16, &b8, &b4, &b2, &b1 })
    {
        b->setBounds (x, btnArea.getY(), btnW, btnArea.getHeight());
        x += btnW + spacing;
    }
    
    content.removeFromTop (15);
    
    auto bpmRow = content.removeFromTop (30);
    syncToggle.setBounds (bpmRow.removeFromLeft (120));
    bpmRow.removeFromLeft (10);
    bpmLabel.setBounds (bpmRow.removeFromLeft (40));
    bpmRow.removeFromLeft (5);
    manualBpmSlider.setBounds (bpmRow);
    
    content.removeFromTop (20);
    
    auto msArea = content.removeFromTop (110);
    msLabel.setBounds (msArea);
    
    content.removeFromTop (5);
    statusLabel.setBounds (content.removeFromTop (20));
}

void PassthroughTempoEditor::timerCallback()
{
    updateUiFromParameters();
    updateMsLabel();
    updateManualBpmFromHost();
}

void PassthroughTempoEditor::updateUiFromParameters()
{
    int denom = processorRef.getSelectedDenominator();

    b128.setToggleState (denom == 128, juce::dontSendNotification);
    b64.setToggleState (denom == 64, juce::dontSendNotification);
    b32.setToggleState (denom == 32, juce::dontSendNotification);
    b16.setToggleState (denom == 16, juce::dontSendNotification);
    b8.setToggleState  (denom == 8,  juce::dontSendNotification);
    b4.setToggleState  (denom == 4,  juce::dontSendNotification);
    b2.setToggleState  (denom == 2,  juce::dontSendNotification);
    b1.setToggleState  (denom == 1,  juce::dontSendNotification);

    bool sync = false;
    if (auto* pb = dynamic_cast<juce::AudioParameterBool*> (processorRef.apvts.getParameter ("syncBpm")))
        sync = pb->get();
    manualBpmSlider.setEnabled (! sync);
}

void PassthroughTempoEditor::updateManualBpmFromHost()
{
    // Get sync state
    bool syncEnabled = false;
    if (auto* pb = dynamic_cast<juce::AudioParameterBool*> (processorRef.apvts.getParameter ("syncBpm")))
        syncEnabled = pb->get();
    
    // When sync is ON: Always update slider to show current host BPM
    // When sync is OFF: Don't update slider automatically (user controls it)
    if (syncEnabled && processorRef.hostProvidedBpm())
    {
        double hostBpm = processorRef.cachedBpm;
        if (hostBpm > 0.0 && ! manualBpmSlider.isMouseButtonDown())
        {
            // Update both the slider display AND the parameter value
            manualBpmSlider.setValue (hostBpm, juce::dontSendNotification);
            
            // Also update the actual parameter so it's in sync
            if (auto* pf = processorRef.apvts.getParameter ("manualBpm"))
                pf->setValueNotifyingHost (pf->convertTo0to1 ((float) hostBpm));
        }
    }
}

void PassthroughTempoEditor::updateMsLabel()
{
    double effectiveBpm = processorRef.getEffectiveBpm();
    int denom = processorRef.getSelectedDenominator();

    if (effectiveBpm <= 0.0)
    {
        msLabel.setText ("-- ms", juce::dontSendNotification);
        statusLabel.setText ("Waiting for BPM information...", juce::dontSendNotification);
        return;
    }

    double quarterMs = 60000.0 / effectiveBpm;
    double noteMs = quarterMs * (4.0 / static_cast<double> (denom));
    
    msLabel.setText (juce::String (noteMs, 2) + " ms", juce::dontSendNotification);
    
    // Display the BPM that's actually being used for calculation
    juce::String statusText = juce::String (effectiveBpm, 1) + " BPM  •  1/" + juce::String (denom) + " note";
    
    bool syncEnabled = false;
    if (auto* pb = dynamic_cast<juce::AudioParameterBool*> (processorRef.apvts.getParameter ("syncBpm")))
        syncEnabled = pb->get();
    
    if (syncEnabled && processorRef.hostProvidedBpm())
        statusText += "  •  Synced to host";
    else if (syncEnabled)
        statusText += "  •  Waiting for host BPM";
    else
        statusText += "  •  Manual BPM";
    
    statusLabel.setText (statusText, juce::dontSendNotification);
}
