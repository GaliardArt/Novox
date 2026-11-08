#include "MainComponent.h"
#include <cmath>
#include <utility>

namespace
{
    juce::String noteNameFromMidi(int midi)
    {
        if (!juce::isPositiveAndBelow(midi, 128)) return "--";
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return juce::String(names[midi % 12]) + juce::String(midi / 12 - 1);
    }

    void drawText(juce::Graphics& g, const juce::String& value, juce::Rectangle<float> bounds,
                  float size, juce::Colour colour, bool bold = false,
                  juce::Justification justification = juce::Justification::centredLeft)
    {
        g.setColour(colour);
        g.setFont(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
        g.drawText(value, bounds, justification, true);
    }

    void drawCard(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setColour(NovoxLookAndFeel::surface);
        g.fillRoundedRectangle(bounds, 13.0f);
        g.setColour(juce::Colours::white.withAlpha(0.035f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 13.0f, 1.0f);
    }

    void drawPill(juce::Graphics& g, juce::Rectangle<float> bounds,
                  const juce::String& text, juce::Colour colour)
    {
        g.setColour(colour.withAlpha(0.13f));
        g.fillRoundedRectangle(bounds, 7.0f);
        drawText(g, text, bounds, 9.0f, colour, true, juce::Justification::centred);
    }
}

MainComponent::MainComponent()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "NOVOX";
    options.filenameSuffix = ".settings";
    options.folderName = "NOVOX";
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.commonToAllUsers = false;
    appProperties.setStorageParameters(options);
    settings = appProperties.getUserSettings();

    const double savedGain = juce::jlimit(0.1, 4.0, settings->getDoubleValue("inputGain", 1.0));
    const double savedSensitivity = juce::jlimit(0.0, 100.0, settings->getDoubleValue("pitchSensitivity", 50.0));
    const double savedSmoothing = juce::jlimit(0.0, 100.0, settings->getDoubleValue("pitchSmoothing", 35.0));
    const bool savedNoteTransitionEnabled = settings->getBoolValue("noteTransitionSmoothingEnabled", false);
    const double legacyTransitionFrames = settings->getDoubleValue("noteTransitionStability", 6.0);
    const double savedNoteTransitionDurationMs = juce::jlimit(30.0, 120.0, settings->getDoubleValue("noteTransitionDurationMs", legacyTransitionFrames * 10.0));
    const bool savedAutoTuneEnabled=settings->getBoolValue("autoTuneEnabled",false);
    const int savedAutoTuneTonic=juce::jlimit(0,11,settings->getIntValue("autoTuneTonic",0));
    const int savedAutoTuneScale=juce::jlimit(0,2,settings->getIntValue("autoTuneScale",1));
    const bool savedNoiseReductionEnabled = settings->getBoolValue("noiseReductionEnabled", false);
    const double savedNoiseReductionStrength = juce::jlimit(0.0, 100.0, settings->getDoubleValue("noiseReductionStrength", 35.0));
    const bool savedPitchStabilityEnabled = settings->getBoolValue("pitchStabilityEnabled", false);
    const double savedPitchDropTolerance = juce::jlimit(10.0, 150.0, settings->getDoubleValue("pitchDropToleranceCents", 45.0));
    const double legacyPitchDropFrames = settings->getDoubleValue("pitchDropHoldFrames", 3.0);
    const double savedPitchDropHoldMs = juce::jlimit(10.0, 80.0, settings->getDoubleValue("pitchDropHoldMs", legacyPitchDropFrames * 10.0));
    const int savedInputChannelMode=juce::jlimit(0,3,settings->getIntValue("inputChannelMode",0));
    const double savedNoiseFloorDb=juce::jlimit(-120.0,0.0,settings->getDoubleValue("noiseCalibrationFloorDb",-120.0));
    const int savedBendRange = juce::jlimit(1, 12, settings->getIntValue("pitchBendRange", 2));
    const int savedChannel = juce::jlimit(1, 16, settings->getIntValue("midiChannel", 1));
    const bool savedBendEnabled = settings->getBoolValue("pitchBendEnabled", true);
    preferredMidiOutputName = settings->getValue("midiOutputName");

    settings->removeValue("drumMidiEnabled");
    settings->removeValue("drumKickNote");
    settings->removeValue("drumSnareNote");
    settings->removeValue("drumHiHatNote");

    setLookAndFeel(&novoxLookAndFeel);
    const auto addNav = [this](juce::TextButton& button, Page page)
    {
        button.setComponentID("novox-nav");
        button.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        button.setColour(juce::TextButton::textColourOffId, NovoxLookAndFeel::textSecondary);
        button.setColour(juce::TextButton::textColourOnId, NovoxLookAndFeel::textPrimary);
        button.onClick = [this, page] { showPage(page); };
        addAndMakeVisible(button);
    };
    addNav(playNav, Page::play);
    addNav(midiNav, Page::midi);
    addNav(audioNav, Page::audio);
    addNav(presetsNav, Page::presets);

    midiOutputSelector.onChange = [this] { selectMidiOutput(); };
    addAndMakeVisible(midiOutputSelector);
    for (int channel = 1; channel <= 16; ++channel)
        channelSelector.addItem("Channel " + juce::String(channel), channel);
    channelSelector.setSelectedId(savedChannel, juce::dontSendNotification);
    channelSelector.onChange = [this] { selectMidiChannel(); };
    addAndMakeVisible(channelSelector);
    midiRouter.setChannel(savedChannel);
    refreshButton.onClick = [this] { refreshMidiOutputs(); };
    addAndMakeVisible(refreshButton);

    pitchBendEnabled.setToggleState(savedBendEnabled, juce::dontSendNotification);
    pitchBendEnabled.onClick = [this]
    {
        engine.setPitchBendEnabled(pitchBendEnabled.getToggleState());
        if (!pitchBendEnabled.getToggleState()) engine.requestReset();
        settings->setValue("pitchBendEnabled", pitchBendEnabled.getToggleState());
        saveSettings();
        repaint();
    };
    addAndMakeVisible(pitchBendEnabled);
    engine.setPitchBendEnabled(savedBendEnabled);

    bendRangeSlider.setRange(1, 12, 1);
    bendRangeSlider.setValue(savedBendRange, juce::dontSendNotification);
    bendRangeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 88, 25);
    bendRangeSlider.setTextValueSuffix(" semitones");
    bendRangeSlider.onValueChange = [this]
    {
        engine.setPitchBendRange(static_cast<int>(bendRangeSlider.getValue()));
        settings->setValue("pitchBendRange", static_cast<int>(bendRangeSlider.getValue()));
        saveSettings();
        repaint();
    };
    addAndMakeVisible(bendRangeSlider);
    engine.setPitchBendRange(savedBendRange);

    gainSlider.setRange(0.1, 4.0, 0.05);
    gainSlider.setValue(savedGain, juce::dontSendNotification);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 76, 25);
    gainSlider.onValueChange = [this]
    {
        const auto value = static_cast<float>(gainSlider.getValue());
        engine.setInputGain(value);
        settings->setValue("inputGain", value);
        saveSettings();
        repaint();
    };
    engine.setInputGain(static_cast<float>(savedGain));
    addAndMakeVisible(gainSlider);

    sensitivitySlider.setRange(0, 100, 1);
    sensitivitySlider.setValue(savedSensitivity, juce::dontSendNotification);
    sensitivitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 76, 25);
    sensitivitySlider.setTextValueSuffix("%");
    sensitivitySlider.onValueChange = [this]
    {
        engine.setPitchSensitivity(static_cast<float>(sensitivitySlider.getValue() / 100.0));
        settings->setValue("pitchSensitivity", sensitivitySlider.getValue());
        saveSettings();
        repaint();
    };
    engine.setPitchSensitivity(static_cast<float>(savedSensitivity / 100.0));
    addAndMakeVisible(sensitivitySlider);

    smoothingSlider.setRange(0, 100, 1);
    smoothingSlider.setValue(savedSmoothing, juce::dontSendNotification);
    smoothingSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 76, 25);
    smoothingSlider.setTextValueSuffix("%");
    smoothingSlider.onValueChange = [this]
    {
        engine.setPitchSmoothing(static_cast<float>(smoothingSlider.getValue()));
        settings->setValue("pitchSmoothing", smoothingSlider.getValue());
        saveSettings();
        repaint();
    };
    addAndMakeVisible(smoothingSlider);
    engine.setPitchSmoothing(static_cast<float>(savedSmoothing));

    noteTransitionEnabled.setToggleState(savedNoteTransitionEnabled, juce::dontSendNotification);
    noteTransitionEnabled.onClick = [this]
    {
        engine.setTransitionSmoothing(noteTransitionEnabled.getToggleState(),
                                      static_cast<float>(noteTransitionSlider.getValue()));
        settings->setValue("noteTransitionSmoothingEnabled", noteTransitionEnabled.getToggleState());
        saveSettings();
        repaint();
    };
    addAndMakeVisible(noteTransitionEnabled);

    noteTransitionSlider.setRange(30, 120, 5);
    noteTransitionSlider.setValue(savedNoteTransitionDurationMs, juce::dontSendNotification);
    noteTransitionSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 58, 23);
    noteTransitionSlider.setTextValueSuffix(" ms");
    noteTransitionSlider.onValueChange = [this]
    {
        engine.setTransitionSmoothing(noteTransitionEnabled.getToggleState(),
                                      static_cast<float>(noteTransitionSlider.getValue()));
        settings->setValue("noteTransitionDurationMs", noteTransitionSlider.getValue());
        saveSettings();
        repaint();
    };
    engine.setTransitionSmoothing(savedNoteTransitionEnabled,
                                  static_cast<float>(savedNoteTransitionDurationMs));
    addAndMakeVisible(noteTransitionSlider);

    autoTuneEnabled.setToggleState(savedAutoTuneEnabled,juce::dontSendNotification);
    autoTuneEnabled.onClick=[this]{engine.setAutoTune(autoTuneEnabled.getToggleState(),autoTuneKeySelector.getSelectedId()-1,autoTuneScaleSelector.getSelectedId()-1);if(autoTuneEnabled.getToggleState())engine.requestReset();settings->setValue("autoTuneEnabled",autoTuneEnabled.getToggleState());saveSettings();repaint();};addAndMakeVisible(autoTuneEnabled);
    const char* keys[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};for(int i=0;i<12;++i)autoTuneKeySelector.addItem(keys[i],i+1);
    autoTuneKeySelector.setSelectedId(savedAutoTuneTonic+1,juce::dontSendNotification);
    autoTuneKeySelector.onChange=[this]{engine.setAutoTune(autoTuneEnabled.getToggleState(),autoTuneKeySelector.getSelectedId()-1,autoTuneScaleSelector.getSelectedId()-1);settings->setValue("autoTuneTonic",autoTuneKeySelector.getSelectedId()-1);saveSettings();repaint();};addAndMakeVisible(autoTuneKeySelector);
    autoTuneScaleSelector.addItem("Chromatic",1);autoTuneScaleSelector.addItem("Major",2);autoTuneScaleSelector.addItem("Minor",3);autoTuneScaleSelector.setSelectedId(savedAutoTuneScale+1,juce::dontSendNotification);
    autoTuneScaleSelector.onChange=[this]{engine.setAutoTune(autoTuneEnabled.getToggleState(),autoTuneKeySelector.getSelectedId()-1,autoTuneScaleSelector.getSelectedId()-1);settings->setValue("autoTuneScale",autoTuneScaleSelector.getSelectedId()-1);saveSettings();repaint();};addAndMakeVisible(autoTuneScaleSelector);
    engine.setAutoTune(savedAutoTuneEnabled, savedAutoTuneTonic, savedAutoTuneScale);
    noiseReductionEnabled.setToggleState(savedNoiseReductionEnabled, juce::dontSendNotification);
    noiseReductionEnabled.onClick = [this]
    {
        engine.setNoiseReductionEnabled(noiseReductionEnabled.getToggleState());
        settings->setValue("noiseReductionEnabled", noiseReductionEnabled.getToggleState());
        saveSettings(); repaint();
    };
    addAndMakeVisible(noiseReductionEnabled);
    noiseReductionSlider.setRange(0.0, 100.0, 1.0);
    noiseReductionSlider.setValue(savedNoiseReductionStrength, juce::dontSendNotification);
    noiseReductionSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 23);
    noiseReductionSlider.setTextValueSuffix("%");
    noiseReductionSlider.onValueChange = [this]
    {
        engine.setNoiseReductionStrength(static_cast<float>(noiseReductionSlider.getValue() / 100.0));
        settings->setValue("noiseReductionStrength", noiseReductionSlider.getValue());
        saveSettings(); repaint();
    };
    engine.setNoiseReductionStrength(static_cast<float>(savedNoiseReductionStrength / 100.0));engine.setNoiseReductionEnabled(savedNoiseReductionEnabled);addAndMakeVisible(noiseReductionSlider);
    pitchStabilityEnabled.setToggleState(savedPitchStabilityEnabled,juce::dontSendNotification);
    pitchStabilityEnabled.onClick=[this]{engine.setPitchStabilityGuard(pitchStabilityEnabled.getToggleState(),static_cast<float>(pitchDropToleranceSlider.getValue()),static_cast<float>(pitchDropHoldSlider.getValue()));settings->setValue("pitchStabilityEnabled",pitchStabilityEnabled.getToggleState());saveSettings();repaint();};addAndMakeVisible(pitchStabilityEnabled);
    pitchDropToleranceSlider.setRange(10.0,150.0,5.0);pitchDropToleranceSlider.setValue(savedPitchDropTolerance,juce::dontSendNotification);pitchDropToleranceSlider.setTextBoxStyle(juce::Slider::TextBoxRight,false,42,20);pitchDropToleranceSlider.setTextValueSuffix(" ct");
    pitchDropToleranceSlider.onValueChange=[this]{engine.setPitchStabilityGuard(pitchStabilityEnabled.getToggleState(),static_cast<float>(pitchDropToleranceSlider.getValue()),static_cast<float>(pitchDropHoldSlider.getValue()));settings->setValue("pitchDropToleranceCents",pitchDropToleranceSlider.getValue());saveSettings();repaint();};addAndMakeVisible(pitchDropToleranceSlider);
    pitchDropHoldSlider.setRange(10.0,80.0,5.0);pitchDropHoldSlider.setValue(savedPitchDropHoldMs,juce::dontSendNotification);pitchDropHoldSlider.setTextBoxStyle(juce::Slider::TextBoxRight,false,42,20);pitchDropHoldSlider.setTextValueSuffix(" ms");
    pitchDropHoldSlider.onValueChange=[this]{engine.setPitchStabilityGuard(pitchStabilityEnabled.getToggleState(),static_cast<float>(pitchDropToleranceSlider.getValue()),static_cast<float>(pitchDropHoldSlider.getValue()));settings->setValue("pitchDropHoldMs",pitchDropHoldSlider.getValue());saveSettings();repaint();};addAndMakeVisible(pitchDropHoldSlider);
    engine.setPitchStabilityGuard(savedPitchStabilityEnabled, static_cast<float>(savedPitchDropTolerance), static_cast<float>(savedPitchDropHoldMs));
    inputChannelSelector.addItem("Auto (strongest)",1);inputChannelSelector.addItem("Left",2);inputChannelSelector.addItem("Right",3);inputChannelSelector.addItem("Stereo mix",4);inputChannelSelector.setSelectedId(savedInputChannelMode+1,juce::dontSendNotification);
    inputChannelSelector.onChange=[this]{int mode=juce::jlimit(0,3,inputChannelSelector.getSelectedId()-1);engine.setInputChannelMode(mode);settings->setValue("inputChannelMode",mode);saveSettings();repaint();};engine.setInputChannelMode(savedInputChannelMode);engine.setCalibratedNoiseFloorDb((float)savedNoiseFloorDb);addAndMakeVisible(inputChannelSelector);
    calibrateNoiseButton.onClick=[this]{engine.beginNoiseCalibration();operationStatus="Measuring room noise. Keep the microphone silent for two seconds.";repaint();};addAndMakeVisible(calibrateNoiseButton);
    audioSetupButton.onClick = [this] { showAudioSetup(); }; addAndMakeVisible(audioSetupButton);

    presetNameEditor.setText("My Preset", juce::dontSendNotification);
    presetNameEditor.setInputRestrictions(48);
    presetNameEditor.setTextToShowWhenEmpty("Preset name", NovoxLookAndFeel::textSecondary);
    addAndMakeVisible(presetNameEditor);
    presetSelector.onChange = [this]
    {
        const int id = presetSelector.getSelectedId();
        if (id > 0)
            presetNameEditor.setText(presetSelector.getItemText(presetSelector.indexOfItemId(id)),
                                     juce::dontSendNotification);
    };
    addAndMakeVisible(presetSelector);
    savePresetButton.onClick = [this] { saveCurrentPreset(); };
    loadPresetButton.onClick = [this] { loadSelectedPreset(); };
    deletePresetButton.onClick = [this] { deleteSelectedPreset(); };
    addAndMakeVisible(savePresetButton);
    addAndMakeVisible(loadPresetButton);
    addAndMakeVisible(deletePresetButton);

    styleControls();
    refreshPresetList(settings->getValue("lastPresetName"));
    refreshMidiOutputs();
    engine.setMidiRouter(&midiRouter);
    lastAudioStateXml = settings->getValue("audioDeviceState");
    auto savedAudioState = juce::XmlDocument::parse(lastAudioStateXml);
    auto error=deviceManager.initialise(2,0,savedAudioState.get(),true);if(error.isNotEmpty())error=deviceManager.initialise(1,0,nullptr,true);if(error.isNotEmpty())operationStatus="Audio device error: "+error;else deviceManager.addAudioCallback(&engine);

    setSize(1160, 780);
    showPage(Page::play);
    startTimerHz(100);
}

MainComponent::~MainComponent()
{
    stopTimer();
    setLookAndFeel(nullptr);
    if (auto state = deviceManager.createStateXml())
    {
        lastAudioStateXml = state->toString();
        settings->setValue("audioDeviceState", lastAudioStateXml);
    }
    engine.requestReset();
    deviceManager.removeAudioCallback(&engine);
    deviceManager.closeAudioDevice();
    midiRouter.closeOutput();
    saveSettings();
}

void MainComponent::styleControls()
{
    for (auto* box : { &midiOutputSelector, &channelSelector, &presetSelector, &autoTuneKeySelector, &autoTuneScaleSelector, &inputChannelSelector })
    {
        box->setColour(juce::ComboBox::backgroundColourId, NovoxLookAndFeel::surface);
        box->setColour(juce::ComboBox::textColourId, NovoxLookAndFeel::textPrimary);
        box->setColour(juce::ComboBox::outlineColourId, NovoxLookAndFeel::border);
        box->setColour(juce::ComboBox::arrowColourId, NovoxLookAndFeel::teal);
    }
    const auto styleButton = [](juce::TextButton& b, bool primary = false)
    {
        b.setColour(juce::TextButton::buttonColourId,
                    primary ? NovoxLookAndFeel::accent : NovoxLookAndFeel::panel.brighter(0.12f));
        b.setColour(juce::TextButton::buttonOnColourId, NovoxLookAndFeel::accentHover);
        b.setColour(juce::TextButton::textColourOffId, NovoxLookAndFeel::textPrimary);
        b.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    };
    styleButton(refreshButton);
    styleButton(savePresetButton, true);
    styleButton(loadPresetButton);
    styleButton(deletePresetButton);
    styleButton(calibrateNoiseButton);
    styleButton(audioSetupButton);
    for (auto* slider : { &gainSlider, &sensitivitySlider, &smoothingSlider, &bendRangeSlider, &noteTransitionSlider, &noiseReductionSlider, &pitchDropToleranceSlider, &pitchDropHoldSlider })
    {
        slider->setColour(juce::Slider::trackColourId, NovoxLookAndFeel::accent);
        slider->setColour(juce::Slider::thumbColourId, NovoxLookAndFeel::textPrimary);
        slider->setColour(juce::Slider::textBoxTextColourId, NovoxLookAndFeel::textPrimary);
        slider->setColour(juce::Slider::textBoxBackgroundColourId, NovoxLookAndFeel::panel);
        slider->setColour(juce::Slider::textBoxOutlineColourId, NovoxLookAndFeel::border);
    }
    pitchBendEnabled.setColour(juce::ToggleButton::textColourId, NovoxLookAndFeel::textPrimary);
    autoTuneEnabled.setColour(juce::ToggleButton::textColourId,NovoxLookAndFeel::textPrimary);
    noiseReductionEnabled.setColour(juce::ToggleButton::textColourId, NovoxLookAndFeel::textPrimary);
    noteTransitionEnabled.setColour(juce::ToggleButton::textColourId, NovoxLookAndFeel::textPrimary);
    pitchStabilityEnabled.setColour(juce::ToggleButton::textColourId, NovoxLookAndFeel::textPrimary);
    presetNameEditor.setColour(juce::TextEditor::backgroundColourId, NovoxLookAndFeel::panel);
    presetNameEditor.setColour(juce::TextEditor::textColourId, NovoxLookAndFeel::textPrimary);
    presetNameEditor.setColour(juce::TextEditor::outlineColourId, NovoxLookAndFeel::border);
    presetNameEditor.setColour(juce::TextEditor::focusedOutlineColourId, NovoxLookAndFeel::accent);
}

void MainComponent::showPage(Page page)
{
    currentPage = page;
    playNav.setToggleState(page == Page::play, juce::dontSendNotification);
    midiNav.setToggleState(page == Page::midi, juce::dontSendNotification);
    audioNav.setToggleState(page == Page::audio, juce::dontSendNotification);
    presetsNav.setToggleState(page == Page::presets, juce::dontSendNotification);
    resized();
    repaint();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(NovoxLookAndFeel::background);
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    const float contentX = 244.0f;
    const float contentW = w - contentX - 44.0f;
    const float x = contentX, y = 142.0f, cw = contentW, ch = h - 218.0f;

    g.setColour(NovoxLookAndFeel::panel);
    g.fillRoundedRectangle({ 18.0f, 18.0f, 184.0f, h - 36.0f }, 16.0f);
    g.fillRoundedRectangle({ 216.0f, 18.0f, w - 234.0f, h - 36.0f }, 16.0f);

    g.setColour(NovoxLookAndFeel::accent);
    g.fillRoundedRectangle(34.0f, 38.0f, 34.0f, 34.0f, 10.0f);
    drawText(g, "N", { 34.0f, 37.0f, 34.0f, 34.0f }, 21.0f, juce::Colours::white, true, juce::Justification::centred);
    drawText(g, "NOVOX", { 78.0f, 35.0f, 98.0f, 22.0f }, 16.0f, NovoxLookAndFeel::textPrimary, true);
    drawText(g, "YOUR VOICE. YOUR INSTRUMENT.", { 78.0f, 55.0f, 105.0f, 24.0f }, 7.0f, NovoxLookAndFeel::textSecondary);
    drawText(g, "WORKSPACE", { 34.0f, 112.0f, 135.0f, 18.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);

    const juce::String pageName = currentPage == Page::play ? "PLAY" : currentPage == Page::midi ? "MIDI" : currentPage == Page::audio ? "AUDIO" : "PRESETS";
    const juce::String pageTitle = currentPage == Page::play ? "Performance" : currentPage == Page::midi ? "MIDI routing" : currentPage == Page::audio ? "Audio environment" : "Your presets";
    const juce::String pageSubtitle = currentPage == Page::play ? "Your voice, translated into musical expression." : currentPage == Page::midi ? "Manage the connection between NOVOX and your instrument." : currentPage == Page::audio ? "Input signal, pitch detection and runtime health." : "Your preferred vocal instrument setups, stored locally.";
    drawText(g, "NOVOX   /   " + pageName, { x, 42.0f, cw * 0.65f, 16.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
    drawText(g, pageTitle, { x, 62.0f, cw * 0.65f, 31.0f }, 23.0f, NovoxLookAndFeel::textPrimary, true);
    drawText(g, pageSubtitle, { x, 94.0f, cw * 0.72f, 20.0f }, 10.0f, NovoxLookAndFeel::textSecondary);
    const float statusX = w - 210.0f;
    g.setColour(audioAvailable ? NovoxLookAndFeel::success : NovoxLookAndFeel::textSecondary);
    g.fillEllipse(statusX, 54.0f, 7.0f, 7.0f);
    drawText(g, audioAvailable ? "AUDIO ENGINE ACTIVE" : "ENGINE STARTING", { statusX + 14.0f, 49.0f, 160.0f, 17.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);

    if (currentPage == Page::play)
    {
        drawText(g, "LIVE PERFORMANCE   •   MONOPHONIC PITCH TRACKING", { x, y, cw, 17.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        const float gap = 14.0f, stageW = cw * 0.68f;
        const auto stage = juce::Rectangle<float>(x, y + 27.0f, stageW - gap, ch * 0.58f);
        const auto confidence = juce::Rectangle<float>(stage.getRight() + gap, stage.getY(), cw - stageW, (stage.getHeight() - gap) * 0.5f);
        const auto midi = juce::Rectangle<float>(confidence.getX(), confidence.getBottom() + gap, confidence.getWidth(), (stage.getHeight() - gap) * 0.5f);
        drawCard(g, stage); drawCard(g, confidence); drawCard(g, midi);
        drawText(g, "DETECTED NOTE", { stage.getX() + 22.0f, stage.getY() + 17.0f, 130.0f, 15.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        drawPill(g, { stage.getRight() - 116.0f, stage.getY() + 16.0f, 96.0f, 23.0f }, currentFrequency > 0.0f ? "VOICE DETECTED" : "LISTENING", currentFrequency > 0.0f ? NovoxLookAndFeel::success : NovoxLookAndFeel::teal);
        const float noteW = stage.getWidth() * 0.48f;
        drawText(g, currentNote, { stage.getX() + 24.0f, stage.getY() + 65.0f, noteW, 100.0f }, 68.0f, NovoxLookAndFeel::accent, true);
        drawText(g, currentFrequency > 0.0f ? juce::String(currentFrequency, 1) + " Hz" : "Waiting for signal", { stage.getX() + 28.0f, stage.getY() + 164.0f, noteW, 24.0f }, 13.0f, NovoxLookAndFeel::textSecondary);
        drawText(g, "VOICE INPUT", { stage.getX() + 22.0f, stage.getBottom() - 36.0f, 100.0f, 15.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        drawText(g, audioDeviceName, { stage.getX() + 22.0f, stage.getBottom() - 20.0f, stage.getWidth() - 44.0f, 15.0f }, 9.0f, NovoxLookAndFeel::textPrimary);

        const float dial = juce::jmin(190.0f, stage.getHeight() - 88.0f);
        const juce::Point<float> centre(stage.getRight() - dial * 0.58f, stage.getCentreY() + 4.0f);
        const float radius = dial * 0.40f;
        g.setColour(NovoxLookAndFeel::panel); g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(NovoxLookAndFeel::teal.withAlpha(0.28f)); g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);
        g.setColour(NovoxLookAndFeel::accent.withAlpha(0.13f)); g.drawEllipse(centre.x - radius * 0.72f, centre.y - radius * 0.72f, radius * 1.44f, radius * 1.44f, 1.0f);
        static const char* notes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const int activePitch = engine.getActiveNote() >= 0 ? engine.getActiveNote() % 12 : -1;
        for (int i = 0; i < 12; ++i)
        {
            const float angle = juce::MathConstants<float>::twoPi * static_cast<float>(i) / 12.0f - juce::MathConstants<float>::halfPi;
            const float px = centre.x + std::cos(angle) * radius * 0.82f, py = centre.y + std::sin(angle) * radius * 0.82f;
            const bool active = i == activePitch;
            if (active) { g.setColour(NovoxLookAndFeel::accent.withAlpha(0.22f)); g.fillEllipse(px - 16.0f, py - 16.0f, 32.0f, 32.0f); }
            drawText(g, notes[i], { px - 17.0f, py - 10.0f, 34.0f, 20.0f }, 9.0f, active ? NovoxLookAndFeel::accent : NovoxLookAndFeel::textSecondary, active, juce::Justification::centred);
        }
        g.setColour(NovoxLookAndFeel::surface.brighter(0.15f)); g.fillEllipse(centre.x - 31.0f, centre.y - 31.0f, 62.0f, 62.0f);
        drawText(g, "N", { centre.x - 24.0f, centre.y - 20.0f, 48.0f, 27.0f }, 22.0f, NovoxLookAndFeel::accent, true, juce::Justification::centred);
        drawText(g, "PITCH", { centre.x - 24.0f, centre.y + 7.0f, 48.0f, 14.0f }, 7.0f, NovoxLookAndFeel::textSecondary, true, juce::Justification::centred);

        drawText(g, "PITCH CONFIDENCE", { confidence.getX() + 17.0f, confidence.getY() + 16.0f, confidence.getWidth() - 34.0f, 16.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        drawText(g, juce::String(currentConfidence * 100.0f, 0) + "  / 100%", { confidence.getX() + 17.0f, confidence.getY() + 38.0f, confidence.getWidth() - 34.0f, 34.0f }, 25.0f, NovoxLookAndFeel::accent, true);
        const auto meter = juce::Rectangle<float>(confidence.getX() + 17.0f, confidence.getBottom() - 28.0f, confidence.getWidth() - 34.0f, 5.0f);
        g.setColour(NovoxLookAndFeel::panel); g.fillRoundedRectangle(meter, 3.0f);
        g.setColour(NovoxLookAndFeel::accent); g.fillRoundedRectangle(meter.withWidth(meter.getWidth() * juce::jlimit(0.0f, 1.0f, currentConfidence)), 3.0f);
        drawText(g, "YIN detection certainty", { confidence.getX() + 17.0f, confidence.getBottom() - 19.0f, confidence.getWidth() - 34.0f, 13.0f }, 8.0f, NovoxLookAndFeel::textSecondary);

        drawText(g, "MIDI OUTPUT", { midi.getX() + 17.0f, midi.getY() + 16.0f, midi.getWidth() - 34.0f, 16.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        g.setColour(midiRouter.isOpen() ? NovoxLookAndFeel::success : NovoxLookAndFeel::textSecondary); g.fillEllipse(midi.getX() + 18.0f, midi.getY() + 45.0f, 8.0f, 8.0f);
        drawText(g, midiRouter.isOpen() ? "Connected" : "Not connected", { midi.getX() + 34.0f, midi.getY() + 38.0f, midi.getWidth() - 48.0f, 24.0f }, 13.0f, NovoxLookAndFeel::textPrimary, true);
        drawText(g, midiRouter.isOpen() ? preferredMidiOutputName : "Choose an output in MIDI settings", { midi.getX() + 17.0f, midi.getBottom() - 27.0f, midi.getWidth() - 34.0f, 17.0f }, 8.0f, NovoxLookAndFeel::textSecondary);

        const float summaryY = stage.getBottom() + 18.0f;
        drawText(g, "AT A GLANCE", { x, summaryY, cw, 14.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        drawText(g, "Instrument status", { x, summaryY + 16.0f, cw, 22.0f }, 15.0f, NovoxLookAndFeel::textPrimary, true);
        const float cardY = summaryY + 48.0f, cardW = (cw - 24.0f) / 3.0f;
        for (int i = 0; i < 3; ++i) drawCard(g, { x + i * (cardW + 12.0f), cardY, cardW, ch - (cardY - y) });
        drawText(g, "MIDI CHANNEL", { x + 16.0f, cardY + 13.0f, cardW - 32.0f, 14.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        drawText(g, juce::String(midiRouter.getChannel()).paddedLeft('0', 2), { x + 16.0f, cardY + 31.0f, cardW - 32.0f, 28.0f }, 19.0f, NovoxLookAndFeel::textPrimary, true);
        drawText(g, "PITCH BEND RANGE", { x + cardW + 28.0f, cardY + 13.0f, cardW - 32.0f, 14.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        drawText(g, "±" + juce::String(static_cast<int>(bendRangeSlider.getValue())) + " semitones", { x + cardW + 28.0f, cardY + 31.0f, cardW - 32.0f, 28.0f }, 17.0f, NovoxLookAndFeel::teal, true);
        drawText(g, "INPUT LEVEL", { x + 2.0f * (cardW + 12.0f) + 16.0f, cardY + 13.0f, cardW - 32.0f, 14.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        drawText(g, juce::String(currentRms * 100.0f, 1) + "%", { x + 2.0f * (cardW + 12.0f) + 16.0f, cardY + 31.0f, cardW - 32.0f, 28.0f }, 19.0f, NovoxLookAndFeel::success, true);
    }
    else if (currentPage == Page::midi)
    {
        drawText(g, "ROUTING & EXPRESSION", { x, y, cw, 17.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        drawText(g, "MIDI configuration", { x, y + 24.0f, cw, 26.0f }, 18.0f, NovoxLookAndFeel::textPrimary, true);
        drawText(g, "Choose the output, channel and expressive pitch range used by NOVOX.", { x, y + 52.0f, cw, 20.0f }, 10.0f, NovoxLookAndFeel::textSecondary);
        const float top = y + 86.0f, wideH = 150.0f;
        drawCard(g, { x, top, cw, wideH });
        drawText(g, "OUTPUT DEVICE", { x + 20.0f, top + 17.0f, cw - 40.0f, 15.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        drawText(g, midiRouter.isOpen() ? preferredMidiOutputName : "No output selected", { x + 20.0f, top + 39.0f, cw - 40.0f, 25.0f }, 17.0f, NovoxLookAndFeel::textPrimary, true);
        drawPill(g, { x + cw - 126.0f, top + 16.0f, 106.0f, 22.0f }, midiRouter.isOpen() ? "CONNECTED" : "DISCONNECTED", midiRouter.isOpen() ? NovoxLookAndFeel::success : NovoxLookAndFeel::textSecondary);
        drawText(g, "NOVOX sends detected notes and Pitch Bend to this destination.", { x + 20.0f, top + 68.0f, cw - 40.0f, 17.0f }, 9.0f, NovoxLookAndFeel::textSecondary);
        const float gap = 14.0f, half = (cw - gap) * 0.5f, cardsY = top + wideH + gap, cardsH = ch - (cardsY - y);
        drawCard(g, { x, cardsY, half, cardsH }); drawCard(g, { x + half + gap, cardsY, half, cardsH });
        drawText(g, "MIDI CHANNEL", { x + 18.0f, cardsY + 18.0f, half - 36.0f, 16.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        drawText(g, juce::String(midiRouter.getChannel()).paddedLeft('0', 2), { x + 18.0f, cardsY + 42.0f, half - 36.0f, 38.0f }, 29.0f, NovoxLookAndFeel::teal, true);
        drawText(g, "Choose the channel for notes and expression.", { x + 18.0f, cardsY + 82.0f, half - 36.0f, 28.0f }, 9.0f, NovoxLookAndFeel::textSecondary);
        drawText(g, "PITCH BEND", { x + half + gap + 18.0f, cardsY + 18.0f, half - 36.0f, 16.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        drawText(g, pitchBendEnabled.getToggleState() ? "Enabled" : "Disabled", { x + half + gap + 18.0f, cardsY + 42.0f, half - 36.0f, 28.0f }, 19.0f, pitchBendEnabled.getToggleState() ? NovoxLookAndFeel::success : NovoxLookAndFeel::textSecondary, true);
        drawText(g, "Set the expressive range in semitones.", { x + half + gap + 18.0f, cardsY + 75.0f, half - 36.0f, 22.0f }, 9.0f, NovoxLookAndFeel::textSecondary);
        drawText(g, midiStatus, { x, y + ch - 17.0f, cw, 14.0f }, 8.0f, NovoxLookAndFeel::textSecondary);
    }
    else if (currentPage == Page::audio)
    {
        drawText(g, "INPUT & DETECTION", { x, y, cw, 17.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        drawText(g, "Audio environment", { x, y + 24.0f, cw, 26.0f }, 18.0f, NovoxLookAndFeel::textPrimary, true);
        drawText(g, "Input signal, pitch detection parameters and runtime health.", { x, y + 52.0f, cw - 154.0f, 20.0f }, 10.0f, NovoxLookAndFeel::textSecondary);
        const float top = y + 86.0f;
        drawCard(g, { x, top, cw, 112.0f });
        drawText(g, "ACTIVE INPUT DEVICE", { x + 18.0f, top + 15.0f, cw - 36.0f, 15.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        drawText(g,audioDeviceName,{x+18,top+38,cw-260,24},16,NovoxLookAndFeel::textPrimary,true);drawText(g,"INPUT CHANNEL MODE",{x+cw-220,top+15,200,14},7.5f,NovoxLookAndFeel::textSecondary,true);
        const float specY = top + 73.0f, specW = (cw - 36.0f) / 3.0f;
        drawText(g, "INPUT CHANNELS   " + juce::String(audioAvailable ? audioChannels : 0), { x + 18.0f, specY, specW, 16.0f }, 8.0f, NovoxLookAndFeel::textSecondary);
        drawText(g, "SAMPLE RATE   " + (audioAvailable ? juce::String(sampleRate / 1000.0, 1) + " kHz" : "—"), { x + 18.0f + specW, specY, specW, 16.0f }, 8.0f, NovoxLookAndFeel::textSecondary);
        drawText(g, "BUFFER SIZE   " + (audioAvailable ? juce::String(bufferSize) : "—"), { x + 18.0f + specW * 2.0f, specY, specW, 16.0f }, 8.0f, NovoxLookAndFeel::textSecondary);

        const float gap=8.0f,cardW=(cw-gap*4.0f)/5.0f,paramY=top+126.0f,paramH=138.0f;
        for(int i=0;i<5;++i)drawCard(g,{x+i*(cardW+gap),paramY,cardW,paramH});
        const juce::String names[]={"INPUT GAIN","PITCH SENSITIVITY","PITCH BEND SMOOTHING","NOTE TRANSITIONS","PITCH STABILITY GUARD"};
        const juce::String values[]={juce::String(gainSlider.getValue()*100.0,0)+"%",juce::String(sensitivitySlider.getValue(),0)+"%",juce::String(smoothingSlider.getValue(),0)+"%",noteTransitionEnabled.getToggleState()?"STABILIZING":"DIRECT",pitchStabilityEnabled.getToggleState()?"ACTIVE":"OFF"};
        const juce::Colour colours[]={NovoxLookAndFeel::accent,NovoxLookAndFeel::teal,NovoxLookAndFeel::success,noteTransitionEnabled.getToggleState()?NovoxLookAndFeel::success:NovoxLookAndFeel::textSecondary,pitchStabilityEnabled.getToggleState()?NovoxLookAndFeel::success:NovoxLookAndFeel::textSecondary};
        for(int i=0;i<5;++i){drawText(g,names[i],{x+i*(cardW+gap)+9.0f,paramY+12.0f,cardW-18.0f,22.0f},6.8f,NovoxLookAndFeel::textSecondary,true);if(i<4)drawText(g,values[i],{x+i*(cardW+gap)+9.0f,paramY+39.0f,cardW-18.0f,25.0f},15.0f,colours[i],true);}
        const float tuneY=paramY+paramH+12.0f;drawCard(g,{x,tuneY,cw,46.0f});
        drawText(g,"PITCH CORRECTION  /  NOISE CONTROL",{x+14.0f,tuneY+5.0f,260.0f,13.0f},7.5f,NovoxLookAndFeel::textSecondary,true);
        drawText(g,autoTuneEnabled.getToggleState()?"ON":"OFF",{x+14.0f,tuneY+20.0f,40.0f,18.0f},10.0f,autoTuneEnabled.getToggleState()?NovoxLookAndFeel::success:NovoxLookAndFeel::textSecondary,true);
        drawText(g,"Key: "+autoTuneKeySelector.getText()+"   Scale: "+autoTuneScaleSelector.getText(),{x+150.0f,tuneY+17.0f,260.0f,20.0f},9.0f,NovoxLookAndFeel::textPrimary,true);
        const float diagY=tuneY+56.0f,diagH=ch-(diagY-y);drawCard(g,{x,diagY,cw,diagH});
        drawText(g, "RUNTIME HEALTH", { x + 18.0f, diagY + 15.0f, cw - 36.0f, 15.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        const float dcol=cw/4;drawText(g,"INPUT LEVEL",{x+18,diagY+42,dcol-20,15},7.2f,NovoxLookAndFeel::textSecondary);drawText(g,juce::String(currentInputRms>0.0f?20.0f*std::log10(currentInputRms):-120.0f,1)+" dBFS",{x+18,diagY+61,dcol-20,25},14,NovoxLookAndFeel::textPrimary,true);
        drawText(g,"PEAK / CLIP",{x+dcol,diagY+42,dcol-16,15},7.2f,NovoxLookAndFeel::textSecondary);drawText(g,juce::String(currentInputPeak,3)+(currentInputClipping?"  CLIP":""),{x+dcol,diagY+61,dcol-16,25},13,currentInputClipping?NovoxLookAndFeel::accent:NovoxLookAndFeel::textPrimary,true);
        drawText(g,"YIN CONFIDENCE",{x+dcol*2,diagY+42,dcol-16,15},7.2f,NovoxLookAndFeel::textSecondary);drawText(g,juce::String(currentConfidence*100.0f,0)+"%",{x+dcol*2,diagY+61,dcol-16,25},16,NovoxLookAndFeel::textPrimary,true);
        drawText(g,"ANALYSIS WINDOW",{x+dcol*3,diagY+42,dcol-12,15},7.2f,NovoxLookAndFeel::textSecondary);drawText(g,juce::String(currentAnalysisWindowSamples)+" samples",{x+dcol*3,diagY+61,dcol-12,25},12,NovoxLookAndFeel::textPrimary,true);
        int cs=engine.getNoiseCalibrationState();float nf=engine.getCalibratedNoiseFloorDb();juce::String nt=cs==1?"Calibrating...":(nf>-119?juce::String(nf,1)+" dBFS":"Not calibrated");drawText(g,"ROOM NOISE FLOOR  ·  DROPPED AUDIO / MIDI",{x+18,diagY+101,cw-36,15},7.2f,NovoxLookAndFeel::textSecondary);drawText(g,nt+"     "+juce::String((juce::int64)engine.getDroppedAudioSamples())+" / "+juce::String((juce::int64)midiRouter.getDroppedEvents()),{x+18,diagY+117,cw-36,15},9,cs==2?NovoxLookAndFeel::success:NovoxLookAndFeel::textPrimary,true);
    }
    else
    {
        drawText(g, "SOUND & WORKFLOW", { x, y, cw, 17.0f }, 8.0f, NovoxLookAndFeel::textSecondary, true);
        drawText(g, "Your presets", { x, y + 24.0f, cw, 26.0f }, 18.0f, NovoxLookAndFeel::textPrimary, true);
        drawText(g, "Keep your preferred vocal instrument setup close at hand.", { x, y + 52.0f, cw, 20.0f }, 10.0f, NovoxLookAndFeel::textSecondary);
        const float top = y + 86.0f;
        drawCard(g, { x, top, cw, 190.0f });
        const float cx = x + 112.0f, cy = top + 94.0f;
        g.setColour(NovoxLookAndFeel::accent.withAlpha(0.08f)); g.fillEllipse(cx - 60.0f, cy - 60.0f, 120.0f, 120.0f);
        g.setColour(NovoxLookAndFeel::teal.withAlpha(0.45f)); g.drawEllipse(cx - 48.0f, cy - 25.0f, 96.0f, 50.0f, 1.0f);
        g.setColour(NovoxLookAndFeel::accent.withAlpha(0.5f)); g.drawEllipse(cx - 25.0f, cy - 48.0f, 50.0f, 96.0f, 1.0f);
        drawText(g, "✳", { cx - 28.0f, cy - 30.0f, 56.0f, 56.0f }, 39.0f, NovoxLookAndFeel::accent, true, juce::Justification::centred);
        drawText(g, "PERSONAL CONFIGURATIONS", { x + 210.0f, top + 37.0f, cw - 235.0f, 15.0f }, 8.0f, NovoxLookAndFeel::teal, true);
        drawText(g, "Make NOVOX feel like your instrument.", { x + 210.0f, top + 61.0f, cw - 235.0f, 30.0f }, 19.0f, NovoxLookAndFeel::textPrimary, true);
        drawText(g, "Presets are stored locally on this computer. They capture input gain, pitch sensitivity, smoothing, Pitch Bend and MIDI routing preferences.", { x + 210.0f, top + 99.0f, cw - 235.0f, 50.0f }, 9.0f, NovoxLookAndFeel::textSecondary);
        const float lowerY = top + 204.0f;
        drawCard(g, { x, lowerY, cw, 104.0f });
        drawText(g, "PRIVATE BY DESIGN", { x + 18.0f, lowerY + 17.0f, cw - 36.0f, 15.0f }, 8.0f, NovoxLookAndFeel::success, true);
        drawText(g, "Your presets stay on your computer. NOVOX does not need an account or cloud connection.", { x + 18.0f, lowerY + 41.0f, cw - 36.0f, 35.0f }, 10.0f, NovoxLookAndFeel::textSecondary);
        drawText(g, operationStatus, { x, y + ch - 17.0f, cw, 14.0f }, 8.0f, NovoxLookAndFeel::textSecondary);
    }

    const float footerY = h - 42.0f;
    g.setColour(NovoxLookAndFeel::border.withAlpha(0.55f));
    g.drawLine(contentX, footerY - 8.0f, w - 42.0f, footerY - 8.0f, 1.0f);
    drawText(g, "NOVOX  ·  YOUR VOICE. YOUR INSTRUMENT.", { contentX, footerY, 245.0f, 16.0f }, 7.0f, NovoxLookAndFeel::textSecondary, true);
    g.setColour(audioAvailable ? NovoxLookAndFeel::success : NovoxLookAndFeel::textSecondary); g.fillEllipse(contentX + 260.0f, footerY + 4.0f, 6.0f, 6.0f);
    drawText(g, audioAvailable ? "AUDIO ACTIVE" : "AUDIO WAITING", { contentX + 271.0f, footerY, 92.0f, 15.0f }, 7.0f, NovoxLookAndFeel::textSecondary, true);
    g.setColour(midiRouter.isOpen() ? NovoxLookAndFeel::success : NovoxLookAndFeel::textSecondary); g.fillEllipse(contentX + 374.0f, footerY + 4.0f, 6.0f, 6.0f);
    drawText(g, midiRouter.isOpen() ? "MIDI CONNECTED" : "MIDI DISCONNECTED", { contentX + 385.0f, footerY, 120.0f, 15.0f }, 7.0f, NovoxLookAndFeel::textSecondary, true);
    drawText(g, "OFFLINE FIRST", { w - 145.0f, footerY, 100.0f, 15.0f }, 7.0f, NovoxLookAndFeel::textSecondary, true, juce::Justification::centredRight);
}

void MainComponent::resized()
{
    const int width = getWidth(), height = getHeight();
    playNav.setBounds(28, 142, 164, 42);
    midiNav.setBounds(28, 190, 164, 42);
    audioNav.setBounds(28, 238, 164, 42);
    presetsNav.setBounds(28, 286, 164, 42);
    const auto area = juce::Rectangle<int>(244, 142, juce::jmax(420, width - 288), juce::jmax(360, height - 218));
    switch (currentPage)
    {
        case Page::play: layoutPlayPage(area); break;
        case Page::midi: layoutMidiPage(area); break;
        case Page::audio: layoutAudioPage(area); break;
        case Page::presets: layoutPresetsPage(area); break;
    }
}

void MainComponent::layoutPlayPage(juce::Rectangle<int>)
{
    midiOutputSelector.setVisible(false);
    audioSetupButton.setVisible(false);
    channelSelector.setVisible(false);
    refreshButton.setVisible(false);
    pitchBendEnabled.setVisible(false);
    bendRangeSlider.setVisible(false);
    gainSlider.setVisible(false);
    sensitivitySlider.setVisible(false);
    smoothingSlider.setVisible(false);
    noteTransitionEnabled.setVisible(false);
    noteTransitionSlider.setVisible(false);
    autoTuneEnabled.setVisible(false);autoTuneKeySelector.setVisible(false);autoTuneScaleSelector.setVisible(false);
    noiseReductionEnabled.setVisible(false); noiseReductionSlider.setVisible(false);
    pitchStabilityEnabled.setVisible(false);pitchDropToleranceSlider.setVisible(false);pitchDropHoldSlider.setVisible(false);
    inputChannelSelector.setVisible(false);calibrateNoiseButton.setVisible(false);
    presetSelector.setVisible(false);
    presetNameEditor.setVisible(false);
    savePresetButton.setVisible(false);
    loadPresetButton.setVisible(false);
    deletePresetButton.setVisible(false);
}

void MainComponent::layoutMidiPage(juce::Rectangle<int> a)
{
    layoutPlayPage({});
    const int top = 142 + 86, gap = 14, half = (a.getWidth() - gap) / 2;
    midiOutputSelector.setVisible(true); refreshButton.setVisible(true);
    midiOutputSelector.setBounds(a.getX() + 18, top + 91, a.getWidth() - 158, 34);
    refreshButton.setBounds(a.getRight() - 126, top + 91, 108, 34);
    const int cardsY = top + 150 + gap;
    channelSelector.setVisible(true); channelSelector.setBounds(a.getX() + 18, cardsY + 119, half - 36, 36);
    pitchBendEnabled.setVisible(true); pitchBendEnabled.setBounds(a.getX() + half + gap + 18, cardsY + 102, 150, 32);
    bendRangeSlider.setVisible(true); bendRangeSlider.setBounds(a.getX() + half + gap + 18, cardsY + 137, half - 36, 35);
}

void MainComponent::layoutAudioPage(juce::Rectangle<int> a)
{
    layoutPlayPage({});
    const int top = 142 + 86 + 126, gap = 13, cardW = (a.getWidth() - gap * 2) / 3;
    audioSetupButton.setVisible(true);
    audioSetupButton.setBounds(a.getRight() - 142, 194, 124, 30);
    gainSlider.setVisible(true); sensitivitySlider.setVisible(true); smoothingSlider.setVisible(true);
    noteTransitionEnabled.setVisible(true); noteTransitionSlider.setVisible(true);
    autoTuneEnabled.setVisible(true);autoTuneKeySelector.setVisible(true);autoTuneScaleSelector.setVisible(true);
    noiseReductionEnabled.setVisible(true);noiseReductionSlider.setVisible(true);inputChannelSelector.setVisible(true);calibrateNoiseButton.setVisible(true);
    pitchStabilityEnabled.setVisible(true);pitchDropToleranceSlider.setVisible(true);pitchDropHoldSlider.setVisible(true);
    inputChannelSelector.setBounds(a.getRight()-220,266,200,28);calibrateNoiseButton.setBounds(a.getX()+716,top+157,128,28);
    const int fiveGap=8,fiveCardW=(a.getWidth()-fiveGap*4)/5;
    gainSlider.setBounds(a.getX()+10,top+72,fiveCardW-20,30);
    sensitivitySlider.setBounds(a.getX()+fiveCardW+fiveGap+10,top+72,fiveCardW-20,30);
    smoothingSlider.setBounds(a.getX()+2*(fiveCardW+fiveGap)+10,top+72,fiveCardW-20,30);
    const int transitionX=a.getX()+3*(fiveCardW+fiveGap)+10;
    noteTransitionEnabled.setBounds(transitionX,top+69,fiveCardW-20,26);noteTransitionSlider.setBounds(transitionX,top+101,fiveCardW-20,27);
    const int guardX=a.getX()+4*(fiveCardW+fiveGap)+8;
    pitchStabilityEnabled.setBounds(guardX,top+38,fiveCardW-16,24);pitchDropToleranceSlider.setBounds(guardX,top+68,fiveCardW-16,25);pitchDropHoldSlider.setBounds(guardX,top+99,fiveCardW-16,25);
    autoTuneEnabled.setBounds(a.getX()+14,top+157,112,28);autoTuneKeySelector.setBounds(a.getX()+150,top+157,100,28);autoTuneScaleSelector.setBounds(a.getX()+264,top+157,130,28);
    noiseReductionEnabled.setBounds(a.getX()+430,top+157,142,28);noiseReductionSlider.setBounds(a.getX()+574,top+157,132,28);
}

void MainComponent::layoutPresetsPage(juce::Rectangle<int> a)
{
    layoutPlayPage({});
    const int top = 142 + 86, controlW = juce::jmin(320, a.getWidth() - 240);
    presetNameEditor.setVisible(true); presetNameEditor.setBounds(a.getX() + 210, top + 107, controlW, 34);
    presetSelector.setVisible(true); presetSelector.setBounds(a.getX() + 210, top + 150, controlW, 34);
    savePresetButton.setVisible(true); loadPresetButton.setVisible(true); deletePresetButton.setVisible(true);
    savePresetButton.setBounds(a.getX() + 210, top + 194, 132, 36);
    loadPresetButton.setBounds(a.getX() + 352, top + 194, 112, 36);
    deletePresetButton.setBounds(a.getX() + 474, top + 194, 92, 36);
}

void MainComponent::saveSettings() { if (settings != nullptr) settings->saveIfNeeded(); }

void MainComponent::refreshMidiOutputs()
{
    engine.requestReset();
    midiOutputSelector.clear(juce::dontSendNotification);
    const auto names = midiRouter.getOutputNames();
    midiOutputSelector.addItem("No MIDI output", 1);
    int restored = 1;
    for (int i = 0; i < names.size(); ++i)
    {
        midiOutputSelector.addItem(names[i], i + 2);
        if (names[i] == preferredMidiOutputName)
            restored = i + 2;
    }

    midiRouter.closeOutput();
    midiOutputSelector.setSelectedId(restored, juce::dontSendNotification);
    if (restored > 1)
        selectMidiOutput();
    else
        midiStatus = preferredMidiOutputName.isNotEmpty()
            ? "Saved MIDI output is unavailable" : "No MIDI output selected";
    repaint();
}

void MainComponent::selectMidiOutput()
{
    engine.requestReset();
    const int id = midiOutputSelector.getSelectedId();
    midiRouter.closeOutput();
    if (id <= 1)
    {
        preferredMidiOutputName.clear();
        settings->setValue("midiOutputName", preferredMidiOutputName);
        saveSettings();
        midiStatus = "No MIDI output selected";
        repaint();
        return;
    }

    preferredMidiOutputName = midiOutputSelector.getItemText(midiOutputSelector.indexOfItemId(id));
    settings->setValue("midiOutputName", preferredMidiOutputName);
    saveSettings();
    midiStatus = midiRouter.openOutput(id - 2)
        ? "MIDI output connected" : "Could not open MIDI output";
    repaint();
}

void MainComponent::selectMidiChannel()
{
    engine.requestReset();
    midiRouter.setChannel(channelSelector.getSelectedId());
    settings->setValue("midiChannel", midiRouter.getChannel());
    saveSettings();
    midiStatus = "MIDI channel " + juce::String(midiRouter.getChannel());
    repaint();
}

PresetData MainComponent::capturePresetData() const
{
    PresetData data;
    data.inputGain = gainSlider.getValue(); data.pitchSensitivity = sensitivitySlider.getValue();
    data.pitchSmoothing = smoothingSlider.getValue(); data.pitchBendEnabled = pitchBendEnabled.getToggleState();
    data.noteTransitionSmoothingEnabled = noteTransitionEnabled.getToggleState();
    data.noteTransitionDurationMs=noteTransitionSlider.getValue();data.autoTuneEnabled=autoTuneEnabled.getToggleState();data.autoTuneTonic=autoTuneKeySelector.getSelectedId()-1;data.autoTuneScale=autoTuneScaleSelector.getSelectedId()-1;
    data.noiseReductionEnabled = noiseReductionEnabled.getToggleState(); data.noiseReductionStrength = noiseReductionSlider.getValue();
    data.pitchStabilityEnabled=pitchStabilityEnabled.getToggleState();data.pitchDropToleranceCents=pitchDropToleranceSlider.getValue();data.pitchDropHoldMs=pitchDropHoldSlider.getValue();
    data.inputChannelMode=inputChannelSelector.getSelectedId()-1;
    data.pitchBendRange = static_cast<int>(bendRangeSlider.getValue());
    data.midiChannel = channelSelector.getSelectedId(); data.midiOutputName = preferredMidiOutputName.toStdString();
    data.sanitise(); return data;
}

void MainComponent::applyPresetData(PresetData data, const juce::String& name)
{
    data.sanitise();
    engine.requestReset();
    gainSlider.setValue(data.inputGain, juce::dontSendNotification);
    sensitivitySlider.setValue(data.pitchSensitivity, juce::dontSendNotification);
    smoothingSlider.setValue(data.pitchSmoothing, juce::dontSendNotification);
    noteTransitionEnabled.setToggleState(data.noteTransitionSmoothingEnabled, juce::dontSendNotification);
    noteTransitionSlider.setValue(data.noteTransitionDurationMs, juce::dontSendNotification);
    engine.setTransitionSmoothing(data.noteTransitionSmoothingEnabled,static_cast<float>(data.noteTransitionDurationMs));
    autoTuneEnabled.setToggleState(data.autoTuneEnabled,juce::dontSendNotification);autoTuneKeySelector.setSelectedId(data.autoTuneTonic+1,juce::dontSendNotification);autoTuneScaleSelector.setSelectedId(data.autoTuneScale+1,juce::dontSendNotification);
    engine.setAutoTune(data.autoTuneEnabled,data.autoTuneTonic,data.autoTuneScale);
    noiseReductionEnabled.setToggleState(data.noiseReductionEnabled, juce::dontSendNotification);
    noiseReductionSlider.setValue(data.noiseReductionStrength, juce::dontSendNotification);
    pitchStabilityEnabled.setToggleState(data.pitchStabilityEnabled,juce::dontSendNotification);pitchDropToleranceSlider.setValue(data.pitchDropToleranceCents,juce::dontSendNotification);pitchDropHoldSlider.setValue(data.pitchDropHoldMs,juce::dontSendNotification);
    engine.setPitchStabilityGuard(data.pitchStabilityEnabled,static_cast<float>(data.pitchDropToleranceCents),static_cast<float>(data.pitchDropHoldMs));
    engine.setNoiseReductionEnabled(data.noiseReductionEnabled);
    engine.setNoiseReductionStrength(static_cast<float>(data.noiseReductionStrength / 100.0));
    inputChannelSelector.setSelectedId(data.inputChannelMode+1,juce::dontSendNotification);engine.setInputChannelMode(data.inputChannelMode);
    pitchBendEnabled.setToggleState(data.pitchBendEnabled, juce::dontSendNotification);
    engine.setPitchBendEnabled(data.pitchBendEnabled);
    bendRangeSlider.setValue(data.pitchBendRange, juce::dontSendNotification);
    engine.setPitchBendRange(data.pitchBendRange);
    engine.setPitchSmoothing(static_cast<float>(data.pitchSmoothing));
    channelSelector.setSelectedId(data.midiChannel, juce::dontSendNotification);
    engine.setInputGain(static_cast<float>(data.inputGain));
    engine.setPitchSensitivity(static_cast<float>(data.pitchSensitivity / 100.0));
    midiRouter.setChannel(data.midiChannel); preferredMidiOutputName = juce::String(data.midiOutputName);
    settings->setValue("inputGain", data.inputGain); settings->setValue("pitchSensitivity", data.pitchSensitivity);
    settings->setValue("pitchSmoothing", data.pitchSmoothing); settings->setValue("pitchBendEnabled", data.pitchBendEnabled);
    settings->setValue("noteTransitionSmoothingEnabled", data.noteTransitionSmoothingEnabled);
    settings->setValue("noteTransitionDurationMs",data.noteTransitionDurationMs);settings->setValue("autoTuneEnabled",data.autoTuneEnabled);settings->setValue("autoTuneTonic",data.autoTuneTonic);settings->setValue("autoTuneScale",data.autoTuneScale);
    settings->setValue("noiseReductionEnabled", data.noiseReductionEnabled); settings->setValue("noiseReductionStrength", data.noiseReductionStrength);
    settings->setValue("pitchStabilityEnabled",data.pitchStabilityEnabled);settings->setValue("pitchDropToleranceCents",data.pitchDropToleranceCents);settings->setValue("pitchDropHoldMs",data.pitchDropHoldMs);
    settings->setValue("inputChannelMode",data.inputChannelMode);
    settings->setValue("pitchBendRange", data.pitchBendRange); settings->setValue("midiChannel", data.midiChannel);
    settings->setValue("midiOutputName", preferredMidiOutputName); settings->setValue("lastPresetName", name);
    refreshMidiOutputs(); saveSettings(); operationStatus = "Loaded preset: " + name; repaint();
}

void MainComponent::refreshPresetList(const juce::String& selectedName)
{
    const auto names = presetManager.listPresets();
    presetSelector.clear(juce::dontSendNotification);
    int selected = 0;
    for (size_t i = 0; i < names.size(); ++i)
    {
        const int id = static_cast<int>(i) + 1;
        presetSelector.addItem(names[i], id);
        if (names[i].compareIgnoreCase(selectedName) == 0) selected = id;
    }
    presetSelector.setSelectedId(selected, juce::dontSendNotification);
    if (selected > 0) presetNameEditor.setText(selectedName, juce::dontSendNotification);
}

void MainComponent::saveCurrentPreset()
{
    const auto name = PresetManager::sanitiseName(presetNameEditor.getText());
    juce::String error;
    if (!presetManager.savePreset(name, capturePresetData(), &error))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "NOVOX preset", error); return;
    }
    settings->setValue("lastPresetName", name); saveSettings(); refreshPresetList(name);
    operationStatus = "Saved preset: " + name; repaint();
}

void MainComponent::loadSelectedPreset()
{
    const auto name = presetSelector.getText();
    if (name.isEmpty()) return;
    PresetData data; juce::String error;
    if (!presetManager.loadPreset(name, data, &error))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "NOVOX preset", error);
        refreshPresetList(); return;
    }
    applyPresetData(std::move(data), name);
}

void MainComponent::deleteSelectedPreset()
{
    const auto name = presetSelector.getText();
    if (name.isEmpty()) return;
    juce::String error;
    if (!presetManager.deletePreset(name, &error))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "NOVOX preset", error); return;
    }
    settings->setValue("lastPresetName", ""); saveSettings(); refreshPresetList();
    operationStatus = "Deleted preset: " + name; repaint();
}

void MainComponent::showAudioSetup()
{
    auto* selector = new juce::AudioDeviceSelectorComponent(
        deviceManager, 1, 2, 0, 0, false, false, true, false);
    selector->setSize(560, 430);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(selector);
    options.dialogTitle = "NOVOX Audio Setup";
    options.dialogBackgroundColour = juce::Colours::darkgrey;
    options.componentToCentreAround = this;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}

void MainComponent::timerCallback()
{
    const auto pitch = engine.getLatestPitch();
    currentConfidence = pitch.voiced ? pitch.confidence : 0.0f;
    currentRms = pitch.rms;
    currentInputRms = engine.getInputRms();
    currentInputPeak = engine.getInputPeak();
    currentInputClipping = engine.isInputClipping();
    currentAnalysisWindowSamples = pitch.analysisWindowSamples;

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        audioAvailable = true;
        audioDeviceName = deviceManager.getCurrentAudioDeviceType() + " · " + device->getName();
        audioChannels = device->getActiveInputChannels().countNumberOfSetBits();
        sampleRate = device->getCurrentSampleRate();
        bufferSize = device->getCurrentBufferSizeSamples();
    }
    else
    {
        audioAvailable = false;
        audioDeviceName = "No audio input device";
        audioChannels = 0;
        sampleRate = 0.0;
        bufferSize = 0;
    }

    const int calibrationState = engine.getNoiseCalibrationState();
    if (calibrationState == 2 && lastNoiseCalibrationState != 2)
    {
        settings->setValue("noiseCalibrationFloorDb", static_cast<double>(engine.getCalibratedNoiseFloorDb()));
        saveSettings();
        operationStatus = "Room noise calibration completed.";
    }
    lastNoiseCalibrationState = calibrationState;

    currentFrequency = pitch.voiced ? pitch.frequencyHz : 0.0f;
    currentNote = engine.getActiveNote() >= 0
        ? noteNameFromMidi(engine.getActiveNote()) : "--";

    if (auto state = deviceManager.createStateXml())
    {
        const auto xml = state->toString();
        if (xml != lastAudioStateXml)
        {
            lastAudioStateXml = xml;
            settings->setValue("audioDeviceState", xml);
            saveSettings();
        }
    }

    repaint();
}
