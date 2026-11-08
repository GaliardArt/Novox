#include "MainComponent.h"
#include "PitchBend.h"
#include <cmath>
#include <utility>

namespace
{
    juce::String noteNameFromMidi(int midi)
    {
        if (!juce::isPositiveAndBelow(midi, 128))
            return "--";
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F",
                                       "F#", "G", "G#", "A", "A#", "B" };
        return juce::String(names[midi % 12]) + juce::String(midi / 12 - 1);
    }
}

MainComponent::MainComponent()
{
    juce::PropertiesFile::Options storageOptions;
    storageOptions.applicationName = "NOVOX";
    storageOptions.filenameSuffix = ".settings";
    storageOptions.folderName = "NOVOX";
    storageOptions.osxLibrarySubFolder = "Application Support";
    storageOptions.storageFormat = juce::PropertiesFile::storeAsXML;
    storageOptions.commonToAllUsers = false;
    appProperties.setStorageParameters(storageOptions);
    settings = appProperties.getUserSettings();

    const double savedGain = juce::jlimit(0.1, 4.0, settings->getDoubleValue("inputGain", 1.0));
    const double savedSensitivity = juce::jlimit(0.0, 100.0,
        settings->getDoubleValue("pitchSensitivity", 50.0));
    const double savedSmoothing = juce::jlimit(0.0, 100.0,
        settings->getDoubleValue("pitchSmoothing", 35.0));
    const int savedBendRange = juce::jlimit(1, 12, settings->getIntValue("pitchBendRange", 2));
    const int savedChannel = juce::jlimit(1, 16, settings->getIntValue("midiChannel", 1));
    const bool savedBendEnabled = settings->getBoolValue("pitchBendEnabled", true);
    const bool savedDrumMidiEnabled = settings->getBoolValue("drumMidiEnabled", false);
    const int savedKickNote = DrumMapping::clampNote(settings->getIntValue("drumKickNote", DrumMapping::defaultKick));
    const int savedSnareNote = DrumMapping::clampNote(settings->getIntValue("drumSnareNote", DrumMapping::defaultSnare));
    const int savedHiHatNote = DrumMapping::clampNote(settings->getIntValue("drumHiHatNote", DrumMapping::defaultHiHat));
    preferredMidiOutputName = settings->getValue("midiOutputName");

    title.setText("NOVOX", juce::dontSendNotification);
    title.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(title);

    status.setText("Sing a sustained note to detect pitch.", juce::dontSendNotification);
    status.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(status);

    noteLabel.setText("--", juce::dontSendNotification);
    noteLabel.setFont(juce::FontOptions(72.0f, juce::Font::bold));
    noteLabel.setJustificationType(juce::Justification::centred);
    noteLabel.setColour(juce::Label::textColourId, juce::Colour(0xff78e0b5));
    playPage.addAndMakeVisible(noteLabel);

    frequencyLabel.setText("0.0 Hz  |  confidence 0%", juce::dontSendNotification);
    frequencyLabel.setJustificationType(juce::Justification::centred);
    frequencyLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    playPage.addAndMakeVisible(frequencyLabel);

    midiLabel.setText("MIDI: no output selected", juce::dontSendNotification);
    midiLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    midiPage.addAndMakeVisible(midiLabel);

    percussionLabel.setText("Percussion: waiting", juce::dontSendNotification);
    percussionLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    percussionLabel.setJustificationType(juce::Justification::centred);
    playPage.addAndMakeVisible(percussionLabel);

    audioDiagnosticsLabel.setText("Audio input: checking device...", juce::dontSendNotification);
    audioDiagnosticsLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    audioDiagnosticsLabel.setJustificationType(juce::Justification::centred);
    audioPage.addAndMakeVisible(audioDiagnosticsLabel);

    performanceDiagnosticsLabel.setText("Diagnostics: no dropped audio or MIDI events", juce::dontSendNotification);
    performanceDiagnosticsLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    performanceDiagnosticsLabel.setJustificationType(juce::Justification::centred);
    audioPage.addAndMakeVisible(performanceDiagnosticsLabel);

    drumMidiEnabled.setToggleState(savedDrumMidiEnabled, juce::dontSendNotification);
    drumMidiEnabled.onClick = [this] {
        settings->setValue("drumMidiEnabled", drumMidiEnabled.getToggleState());
        if (!drumMidiEnabled.getToggleState())
            releaseAllDrumNotes();
        saveSettings();
    };
    midiPage.addAndMakeVisible(drumMidiEnabled);

    drumMappingLabel.setText("General MIDI drum mapping (channel 10)", juce::dontSendNotification);
    drumMappingLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    drumMappingLabel.setJustificationType(juce::Justification::centred);
    midiPage.addAndMakeVisible(drumMappingLabel);

    const auto populateDrumNotes = [](juce::ComboBox& selector, int selectedNote) {
        for (int note = DrumMapping::minimumNote; note <= DrumMapping::maximumNote; ++note)
            selector.addItem(juce::String(note) + " - "
                + juce::String(DrumMapping::gmName(note).data()), note + 1);
        selector.setSelectedId(selectedNote + 1, juce::dontSendNotification);
    };
    populateDrumNotes(kickNoteSelector, savedKickNote);
    populateDrumNotes(snareNoteSelector, savedSnareNote);
    populateDrumNotes(hiHatNoteSelector, savedHiHatNote);
    kickNoteSelector.onChange = [this] {
        if (pendingDrumNotes[0] >= 0) {
            midiRouter.drumNoteOff(pendingDrumNotes[0]);
            pendingDrumNotes[0] = -1;
        }
        settings->setValue("drumKickNote", kickNoteSelector.getSelectedId() - 1);
        saveSettings();
    };
    snareNoteSelector.onChange = [this] {
        if (pendingDrumNotes[1] >= 0) {
            midiRouter.drumNoteOff(pendingDrumNotes[1]);
            pendingDrumNotes[1] = -1;
        }
        settings->setValue("drumSnareNote", snareNoteSelector.getSelectedId() - 1);
        saveSettings();
    };
    hiHatNoteSelector.onChange = [this] {
        if (pendingDrumNotes[2] >= 0) {
            midiRouter.drumNoteOff(pendingDrumNotes[2]);
            pendingDrumNotes[2] = -1;
        }
        settings->setValue("drumHiHatNote", hiHatNoteSelector.getSelectedId() - 1);
        saveSettings();
    };
    midiPage.addAndMakeVisible(kickNoteSelector);
    midiPage.addAndMakeVisible(snareNoteSelector);
    midiPage.addAndMakeVisible(hiHatNoteSelector);

    kickLabel.setText("Kick", juce::dontSendNotification);
    snareLabel.setText("Snare", juce::dontSendNotification);
    hiHatLabel.setText("Hi-hat", juce::dontSendNotification);
    for (auto* label : { &kickLabel, &snareLabel, &hiHatLabel }) {
        label->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(*label);
    }

    gainLabel.setText("Input gain", juce::dontSendNotification);
    gainLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    audioPage.addAndMakeVisible(gainLabel);
    gainSlider.setRange(0.1, 4.0, 0.05);
    gainSlider.setValue(savedGain, juce::dontSendNotification);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 24);
    gainSlider.onValueChange = [this] {
        const auto value = static_cast<float>(gainSlider.getValue());
        engine.setInputGain(value);
        settings->setValue("inputGain", value);
        saveSettings();
    };
    engine.setInputGain(static_cast<float>(savedGain));
    audioPage.addAndMakeVisible(gainSlider);

    sensitivityLabel.setText("Pitch sensitivity", juce::dontSendNotification);
    sensitivityLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    audioPage.addAndMakeVisible(sensitivityLabel);
    sensitivitySlider.setRange(0, 100, 1);
    sensitivitySlider.setValue(savedSensitivity, juce::dontSendNotification);
    sensitivitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 24);
    sensitivitySlider.setTextValueSuffix("%");
    sensitivitySlider.onValueChange = [this] {
        const auto value = static_cast<float>(sensitivitySlider.getValue() / 100.0);
        engine.setPitchSensitivity(value);
        settings->setValue("pitchSensitivity", sensitivitySlider.getValue());
        saveSettings();
    };
    engine.setPitchSensitivity(static_cast<float>(savedSensitivity / 100.0));
    audioPage.addAndMakeVisible(sensitivitySlider);

    smoothingLabel.setText("Pitch smoothing", juce::dontSendNotification);
    smoothingLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    audioPage.addAndMakeVisible(smoothingLabel);
    smoothingSlider.setRange(0, 100, 1);
    smoothingSlider.setValue(savedSmoothing, juce::dontSendNotification);
    smoothingSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 24);
    smoothingSlider.setTextValueSuffix("%");
    smoothingSlider.onValueChange = [this] {
        pitchSmoother.reset();
        lastPitchBendValue = -1;
        settings->setValue("pitchSmoothing", smoothingSlider.getValue());
        saveSettings();
    };
    audioPage.addAndMakeVisible(smoothingSlider);

    bendRangeLabel.setText("Pitch Bend range", juce::dontSendNotification);
    bendRangeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    midiPage.addAndMakeVisible(bendRangeLabel);
    bendRangeSlider.setRange(1, 12, 1);
    bendRangeSlider.setValue(savedBendRange, juce::dontSendNotification);
    bendRangeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 24);
    bendRangeSlider.setTextValueSuffix(" semitones");
    bendRangeSlider.onValueChange = [this] {
        lastPitchBendValue = -1;
        settings->setValue("pitchBendRange", static_cast<int>(bendRangeSlider.getValue()));
        saveSettings();
    };
    midiPage.addAndMakeVisible(bendRangeSlider);

    pitchBendEnabled.setToggleState(savedBendEnabled, juce::dontSendNotification);
    pitchBendEnabled.onClick = [this] {
        if (!pitchBendEnabled.getToggleState())
            resetPitchBend();
        else
            lastPitchBendValue = -1;
        settings->setValue("pitchBendEnabled", pitchBendEnabled.getToggleState());
        saveSettings();
    };
    midiPage.addAndMakeVisible(pitchBendEnabled);

    pitchBendLabel.setText("Pitch Bend: center", juce::dontSendNotification);
    pitchBendLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    pitchBendLabel.setJustificationType(juce::Justification::centred);
    playPage.addAndMakeVisible(pitchBendLabel);

    midiOutputSelector.onChange = [this] { selectMidiOutput(); };
    midiPage.addAndMakeVisible(midiOutputSelector);

    channelLabel.setText("MIDI channel", juce::dontSendNotification);
    channelLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    midiPage.addAndMakeVisible(channelLabel);
    for (int channel = 1; channel <= 16; ++channel)
        channelSelector.addItem("Channel " + juce::String(channel), channel);
    channelSelector.setSelectedId(savedChannel, juce::dontSendNotification);
    channelSelector.onChange = [this] { selectMidiChannel(); };
    midiPage.addAndMakeVisible(channelSelector);
    midiRouter.setChannel(savedChannel);

    refreshButton.onClick = [this] { refreshMidiOutputs(); };
    midiPage.addAndMakeVisible(refreshButton);

    presetsLabel.setText("Local presets", juce::dontSendNotification);
    presetsLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    presetsLabel.setJustificationType(juce::Justification::centred);
    presetsPage.addAndMakeVisible(presetsLabel);

    presetNameEditor.setText("My Preset", juce::dontSendNotification);
    presetNameEditor.setInputRestrictions(48);
    presetNameEditor.setTextToShowWhenEmpty("Preset name", juce::Colours::grey);
    presetsPage.addAndMakeVisible(presetNameEditor);

    presetSelector.onChange = [this] {
        const int selected = presetSelector.getSelectedId();
        if (selected > 0)
            presetNameEditor.setText(presetSelector.getItemText(
                presetSelector.indexOfItemId(selected)), juce::dontSendNotification);
    };
    presetsPage.addAndMakeVisible(presetSelector);

    savePresetButton.onClick = [this] { saveCurrentPreset(); };
    loadPresetButton.onClick = [this] { loadSelectedPreset(); };
    deletePresetButton.onClick = [this] { deleteSelectedPreset(); };
    presetsPage.addAndMakeVisible(savePresetButton);
    presetsPage.addAndMakeVisible(loadPresetButton);
    presetsPage.addAndMakeVisible(deletePresetButton);
    refreshPresetList(settings->getValue("lastPresetName"));

    refreshMidiOutputs();

    novoxLookAndFeel.setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff17191e));
    novoxLookAndFeel.setColour(juce::Label::textColourId, juce::Colour(0xffe8e9ed));
    novoxLookAndFeel.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff343841));
    novoxLookAndFeel.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe47b91));
    novoxLookAndFeel.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe8e9ed));
    novoxLookAndFeel.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    novoxLookAndFeel.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff292d35));
    novoxLookAndFeel.setColour(juce::ComboBox::textColourId, juce::Colour(0xffe8e9ed));
    novoxLookAndFeel.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff454b56));
    novoxLookAndFeel.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffe47b91));
    novoxLookAndFeel.setColour(juce::Slider::trackColourId, juce::Colour(0xffe47b91));
    novoxLookAndFeel.setColour(juce::Slider::thumbColourId, juce::Colour(0xfff0a0ae));
    novoxLookAndFeel.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe8e9ed));
    novoxLookAndFeel.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff292d35));
    novoxLookAndFeel.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff454b56));
    setLookAndFeel(&novoxLookAndFeel);
    tabs.setLookAndFeel(&novoxLookAndFeel);
    tabs.setTabBarDepth(42);
    tabs.setColour(juce::TabbedComponent::backgroundColourId, juce::Colour(0xff20232a));
    tabs.setColour(juce::TabbedComponent::outlineColourId, juce::Colour(0xff343943));
    tabs.addTab("PLAY", juce::Colour(0xffe47b91), &playPage, false);
    tabs.addTab("MIDI", juce::Colour(0xffe47b91), &midiPage, false);
    tabs.addTab("AUDIO", juce::Colour(0xffe47b91), &audioPage, false);
    tabs.addTab("PRESETS", juce::Colour(0xffe47b91), &presetsPage, false);
    addAndMakeVisible(tabs);

    const auto error = deviceManager.initialise(1, 0, nullptr, true);
    if (error.isNotEmpty())
        status.setText("Audio device error: " + error, juce::dontSendNotification);
    else
        deviceManager.addAudioCallback(&engine);

    setSize(1080, 760);
    startTimerHz(60);
}

MainComponent::~MainComponent()
{
    stopTimer();
    tabs.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
    releaseAllDrumNotes();
    if (noteTracker.getActiveNote() >= 0)
    {
        resetPitchBend();
        midiRouter.noteOff(noteTracker.getActiveNote());
    }
    midiRouter.closeOutput();
    deviceManager.removeAudioCallback(&engine);
    deviceManager.closeAudioDevice();
    saveSettings();
}

void MainComponent::saveSettings()
{
    if (settings != nullptr)
        settings->saveIfNeeded();
}

void MainComponent::refreshMidiOutputs()
{
    releaseAllDrumNotes();
    const int activeNote = noteTracker.getActiveNote();
    if (activeNote >= 0)
    {
        resetPitchBend();
        midiRouter.noteOff(activeNote);
    }
    noteTracker.reset();
    pitchSmoother.reset();

    midiOutputSelector.clear(juce::dontSendNotification);
    const auto names = midiRouter.getOutputNames();
    midiOutputSelector.addItem("No MIDI output", 1);
    int restoredId = 1;
    for (int i = 0; i < names.size(); ++i)
    {
        midiOutputSelector.addItem(names[i], i + 2);
        if (names[i] == preferredMidiOutputName)
            restoredId = i + 2;
    }

    midiRouter.closeOutput();
    midiOutputSelector.setSelectedId(restoredId, juce::dontSendNotification);
    lastPitchBendValue = 8192;
    if (restoredId > 1)
        selectMidiOutput();
    else
        midiLabel.setText(preferredMidiOutputName.isNotEmpty()
            ? "MIDI: saved output unavailable" : "MIDI: choose an output device",
            juce::dontSendNotification);
}

void MainComponent::selectMidiOutput()
{
    releaseAllDrumNotes();
    const int activeNote = noteTracker.getActiveNote();
    if (activeNote >= 0)
    {
        resetPitchBend();
        midiRouter.noteOff(activeNote);
    }
    noteTracker.reset();
    pitchSmoother.reset();

    const int selectedId = midiOutputSelector.getSelectedId();
    if (selectedId <= 1)
    {
        midiRouter.closeOutput();
        preferredMidiOutputName.clear();
        settings->setValue("midiOutputName", preferredMidiOutputName);
        saveSettings();
        midiLabel.setText("MIDI: no output selected", juce::dontSendNotification);
        return;
    }

    preferredMidiOutputName = midiOutputSelector.getItemText(midiOutputSelector.indexOfItemId(selectedId));
    settings->setValue("midiOutputName", preferredMidiOutputName);
    saveSettings();

    if (midiRouter.openOutput(selectedId - 2))
        midiLabel.setText("MIDI: output connected", juce::dontSendNotification);
    else
        midiLabel.setText("MIDI: could not open output", juce::dontSendNotification);

    lastPitchBendValue = 8192;
}

void MainComponent::selectMidiChannel()
{
    const int activeNote = noteTracker.getActiveNote();
    if (activeNote >= 0)
    {
        resetPitchBend();
        midiRouter.noteOff(activeNote);
    }

    midiRouter.setChannel(channelSelector.getSelectedId());
    settings->setValue("midiChannel", midiRouter.getChannel());
    saveSettings();
    noteTracker.reset();
    pitchSmoother.reset();
    lastPitchBendValue = 8192;
    midiLabel.setText("MIDI: channel " + juce::String(midiRouter.getChannel()),
                      juce::dontSendNotification);
}

void MainComponent::resetPitchBend()
{
    if (midiRouter.isOpen() && lastPitchBendValue != PitchBend::center)
        midiRouter.pitchBend(PitchBend::center);
    lastPitchBendValue = PitchBend::center;
    pitchSmoother.reset();
    pitchBendLabel.setText("Pitch Bend: center", juce::dontSendNotification);
}

void MainComponent::sendPitchBend(const PitchResult& pitch)
{
    const int activeNote = noteTracker.getActiveNote();
    if (activeNote < 0 || !pitch.voiced || !pitchBendEnabled.getToggleState())
    {
        resetPitchBend();
        return;
    }

    const float rawOffset = PitchBend::semitoneOffset(pitch.frequencyHz, activeNote);
    const float offset = pitchSmoother.process(rawOffset,
        static_cast<float>(smoothingSlider.getValue()));
    const int range = static_cast<int>(bendRangeSlider.getValue());
    const int value = PitchBend::semitonesTo14BitValue(offset, range);

    if (midiRouter.isOpen() && std::abs(value - lastPitchBendValue) >= 2)
    {
        midiRouter.pitchBend(value);
        lastPitchBendValue = value;
    }

    const auto sign = offset > 0.005f ? "+" : "";
    pitchBendLabel.setText("Pitch Bend: " + juce::String(sign) + juce::String(offset, 2)
                           + " st  |  range ±" + juce::String(range) + " st",
                           juce::dontSendNotification);
}

void MainComponent::updateNoteTracking(const PitchResult& pitch)
{
    const auto update = noteTracker.process(pitch);
    if (update.noteOff >= 0)
    {
        resetPitchBend();
        midiRouter.noteOff(update.noteOff);
    }
    if (update.noteOn >= 0)
    {
        pitchSmoother.reset();
        midiRouter.noteOn(update.noteOn);
        lastPitchBendValue = -1;
    }

    sendPitchBend(pitch);
}


void MainComponent::dispatchPercussion(const VocalPercussionResult& percussion)
{
    if (!drumMidiEnabled.getToggleState() || !midiRouter.isOpen())
        return;

    int slot = -1;
    int note = -1;
    switch (percussion.type)
    {
        case VocalPercussionType::kick:
            slot = static_cast<int>(DrumMapping::Part::kick);
            note = kickNoteSelector.getSelectedId() - 1;
            break;
        case VocalPercussionType::snare:
            slot = static_cast<int>(DrumMapping::Part::snare);
            note = snareNoteSelector.getSelectedId() - 1;
            break;
        case VocalPercussionType::hiHat:
            slot = static_cast<int>(DrumMapping::Part::hiHat);
            note = hiHatNoteSelector.getSelectedId() - 1;
            break;
        case VocalPercussionType::none:
            return;
    }

    if (slot < 0 || note < DrumMapping::minimumNote || note > DrumMapping::maximumNote)
        return;

    if (pendingDrumNotes[static_cast<size_t>(slot)] >= 0)
        midiRouter.drumNoteOff(pendingDrumNotes[static_cast<size_t>(slot)]);

    const auto velocity = static_cast<juce::uint8>(
        juce::jlimit(1, 127, static_cast<int>(std::lround(percussion.confidence * 127.0f))));
    midiRouter.drumNoteOn(note, velocity);
    pendingDrumNotes[static_cast<size_t>(slot)] = note;
    drumNoteOffTimes[static_cast<size_t>(slot)] =
        juce::Time::getMillisecondCounter() + 90U;
}

void MainComponent::releaseDueDrumNotes()
{
    const auto now = juce::Time::getMillisecondCounter();
    for (size_t i = 0; i < pendingDrumNotes.size(); ++i)
    {
        if (pendingDrumNotes[i] >= 0
            && static_cast<int32_t>(now - drumNoteOffTimes[i]) >= 0)
        {
            midiRouter.drumNoteOff(pendingDrumNotes[i]);
            pendingDrumNotes[i] = -1;
        }
    }
}

void MainComponent::releaseAllDrumNotes()
{
    for (auto& note : pendingDrumNotes)
    {
        if (note >= 0)
        {
            midiRouter.drumNoteOff(note);
            note = -1;
        }
    }
}

PresetData MainComponent::capturePresetData() const
{
    PresetData data;
    data.inputGain = gainSlider.getValue();
    data.pitchSensitivity = sensitivitySlider.getValue();
    data.pitchSmoothing = smoothingSlider.getValue();
    data.pitchBendEnabled = pitchBendEnabled.getToggleState();
    data.pitchBendRange = static_cast<int>(bendRangeSlider.getValue());
    data.midiChannel = channelSelector.getSelectedId();
    data.midiOutputName = preferredMidiOutputName.toStdString();
    data.drumMidiEnabled = drumMidiEnabled.getToggleState();
    data.drumKickNote = kickNoteSelector.getSelectedId() - 1;
    data.drumSnareNote = snareNoteSelector.getSelectedId() - 1;
    data.drumHiHatNote = hiHatNoteSelector.getSelectedId() - 1;
    data.sanitise();
    return data;
}

void MainComponent::applyPresetData(PresetData data, const juce::String& presetName)
{
    data.sanitise();
    releaseAllDrumNotes();
    if (noteTracker.getActiveNote() >= 0)
    {
        resetPitchBend();
        midiRouter.noteOff(noteTracker.getActiveNote());
    }
    noteTracker.reset();
    pitchSmoother.reset();
    lastPitchBendValue = PitchBend::center;

    gainSlider.setValue(data.inputGain, juce::dontSendNotification);
    sensitivitySlider.setValue(data.pitchSensitivity, juce::dontSendNotification);
    smoothingSlider.setValue(data.pitchSmoothing, juce::dontSendNotification);
    pitchBendEnabled.setToggleState(data.pitchBendEnabled, juce::dontSendNotification);
    bendRangeSlider.setValue(data.pitchBendRange, juce::dontSendNotification);
    channelSelector.setSelectedId(data.midiChannel, juce::dontSendNotification);
    drumMidiEnabled.setToggleState(data.drumMidiEnabled, juce::dontSendNotification);
    kickNoteSelector.setSelectedId(data.drumKickNote + 1, juce::dontSendNotification);
    snareNoteSelector.setSelectedId(data.drumSnareNote + 1, juce::dontSendNotification);
    hiHatNoteSelector.setSelectedId(data.drumHiHatNote + 1, juce::dontSendNotification);

    engine.setInputGain(static_cast<float>(data.inputGain));
    engine.setPitchSensitivity(static_cast<float>(data.pitchSensitivity / 100.0));
    midiRouter.setChannel(data.midiChannel);
    preferredMidiOutputName = juce::String(data.midiOutputName);

    settings->setValue("inputGain", data.inputGain);
    settings->setValue("pitchSensitivity", data.pitchSensitivity);
    settings->setValue("pitchSmoothing", data.pitchSmoothing);
    settings->setValue("pitchBendEnabled", data.pitchBendEnabled);
    settings->setValue("pitchBendRange", data.pitchBendRange);
    settings->setValue("midiChannel", data.midiChannel);
    settings->setValue("drumMidiEnabled", data.drumMidiEnabled);
    settings->setValue("drumKickNote", data.drumKickNote);
    settings->setValue("drumSnareNote", data.drumSnareNote);
    settings->setValue("drumHiHatNote", data.drumHiHatNote);
    settings->setValue("midiOutputName", preferredMidiOutputName);
    settings->setValue("lastPresetName", presetName);
    refreshMidiOutputs();
    saveSettings();
    status.setText("Loaded preset: " + presetName, juce::dontSendNotification);
}

void MainComponent::refreshPresetList(const juce::String& selectedName)
{
    const auto names = presetManager.listPresets();
    presetSelector.clear(juce::dontSendNotification);
    int selectedId = 0;
    for (size_t i = 0; i < names.size(); ++i)
    {
        const int id = static_cast<int>(i) + 1;
        presetSelector.addItem(names[i], id);
        if (names[i].compareIgnoreCase(selectedName) == 0)
            selectedId = id;
    }
    presetSelector.setSelectedId(selectedId, juce::dontSendNotification);
    if (selectedId > 0)
        presetNameEditor.setText(selectedName, juce::dontSendNotification);
}

void MainComponent::saveCurrentPreset()
{
    const auto name = PresetManager::sanitiseName(presetNameEditor.getText());
    juce::String error;
    if (!presetManager.savePreset(name, capturePresetData(), &error))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                               "NOVOX preset", error);
        return;
    }

    settings->setValue("lastPresetName", name);
    saveSettings();
    refreshPresetList(name);
    status.setText("Saved preset: " + name, juce::dontSendNotification);
}

void MainComponent::loadSelectedPreset()
{
    const auto name = presetSelector.getText();
    if (name.isEmpty())
        return;

    PresetData data;
    juce::String error;
    if (!presetManager.loadPreset(name, data, &error))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                               "NOVOX preset", error);
        refreshPresetList();
        return;
    }
    applyPresetData(std::move(data), name);
}

void MainComponent::deleteSelectedPreset()
{
    const auto name = presetSelector.getText();
    if (name.isEmpty())
        return;

    juce::String error;
    if (!presetManager.deletePreset(name, &error))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                               "NOVOX preset", error);
        return;
    }

    settings->setValue("lastPresetName", "");
    saveSettings();
    refreshPresetList();
    status.setText("Deleted preset: " + name, juce::dontSendNotification);
}


void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff17191e));
    g.setColour(juce::Colour(0xff22262e));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(18.0f), 18.0f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(24);
    auto header = area.removeFromTop(66);
    title.setBounds(header.removeFromLeft(220).withSizeKeepingCentre(180, 42));
    status.setBounds(header.reduced(8, 12));
    area.removeFromTop(8);
    tabs.setBounds(area);
    auto play = playPage.getLocalBounds().reduced(30);
    noteLabel.setBounds(play.removeFromTop(210));
    frequencyLabel.setBounds(play.removeFromTop(38));
    pitchBendLabel.setBounds(play.removeFromTop(34));
    play.removeFromTop(24);
    percussionLabel.setBounds(play.removeFromTop(42));
    auto midi = midiPage.getLocalBounds().reduced(34);
    midiLabel.setBounds(midi.removeFromTop(34));
    midi.removeFromTop(12);
    auto outputRow = midi.removeFromTop(42);
    midiOutputSelector.setBounds(outputRow.removeFromLeft(outputRow.getWidth() - 150));
    outputRow.removeFromLeft(12);
    refreshButton.setBounds(outputRow);
    midi.removeFromTop(18);
    channelLabel.setBounds(midi.removeFromTop(24));
    channelSelector.setBounds(midi.removeFromTop(40).withWidth(260));
    midi.removeFromTop(18);
    pitchBendEnabled.setBounds(midi.removeFromTop(32));
    bendRangeLabel.setBounds(midi.removeFromTop(24));
    bendRangeSlider.setBounds(midi.removeFromTop(42));
    midi.removeFromTop(18);
    drumMidiEnabled.setBounds(midi.removeFromTop(32));
    drumMappingLabel.setBounds(midi.removeFromTop(30));
    midi.removeFromTop(8);
    auto mapping = midi.removeFromTop(76);
    const int cell = mapping.getWidth() / 3;
    auto kick = mapping.removeFromLeft(cell).reduced(6, 0);
    kickLabel.setBounds(kick.removeFromTop(24));
    kickNoteSelector.setBounds(kick.removeFromTop(40));
    auto snare = mapping.removeFromLeft(cell).reduced(6, 0);
    snareLabel.setBounds(snare.removeFromTop(24));
    snareNoteSelector.setBounds(snare.removeFromTop(40));
    auto hat = mapping.reduced(6, 0);
    hiHatLabel.setBounds(hat.removeFromTop(24));
    hiHatNoteSelector.setBounds(hat.removeFromTop(40));
    auto audio = audioPage.getLocalBounds().reduced(34);
    gainLabel.setBounds(audio.removeFromTop(26));
    gainSlider.setBounds(audio.removeFromTop(48));
    audio.removeFromTop(16);
    sensitivityLabel.setBounds(audio.removeFromTop(26));
    sensitivitySlider.setBounds(audio.removeFromTop(48));
    audio.removeFromTop(16);
    smoothingLabel.setBounds(audio.removeFromTop(26));
    smoothingSlider.setBounds(audio.removeFromTop(48));
    audio.removeFromTop(24);
    audioDiagnosticsLabel.setBounds(audio.removeFromTop(34));
    performanceDiagnosticsLabel.setBounds(audio.removeFromTop(34));
    auto presets = presetsPage.getLocalBounds().reduced(34);
    presetsLabel.setBounds(presets.removeFromTop(32));
    presets.removeFromTop(18);
    presetSelector.setBounds(presets.removeFromTop(42).withWidth(520));
    presets.removeFromTop(12);
    presetNameEditor.setBounds(presets.removeFromTop(42).withWidth(520));
    presets.removeFromTop(18);
    auto buttons = presets.removeFromTop(42);
    const int buttonWidth = juce::jmin(160, (buttons.getWidth() - 24) / 3);
    savePresetButton.setBounds(buttons.removeFromLeft(buttonWidth));
    buttons.removeFromLeft(12);
    loadPresetButton.setBounds(buttons.removeFromLeft(buttonWidth));
    buttons.removeFromLeft(12);
    deletePresetButton.setBounds(buttons.removeFromLeft(buttonWidth));
}

void MainComponent::timerCallback()
{
    const auto pitch = engine.getLatestPitch();
    const auto percussion = engine.getLatestPercussion();
    releaseDueDrumNotes();

    if (auto* audioDevice = deviceManager.getCurrentAudioDevice())
    {
        const auto inputChannels = audioDevice->getActiveInputChannels().countNumberOfSetBits();
        audioDiagnosticsLabel.setText(
            "Audio input: " + audioDevice->getName()
            + "  |  " + juce::String(inputChannels) + " input channel(s)"
            + "  |  " + juce::String(audioDevice->getCurrentSampleRate() / 1000.0, 1)
            + " kHz / " + juce::String(audioDevice->getCurrentBufferSizeSamples()) + " samples",
            juce::dontSendNotification);
    }
    else
    {
        audioDiagnosticsLabel.setText("Audio input: no device active — check Audio Settings",
                                      juce::dontSendNotification);
    }

    performanceDiagnosticsLabel.setText(
        "Dropped audio samples: " + juce::String(static_cast<juce::int64>(engine.getDroppedAudioSamples()))
        + "  |  Dropped MIDI events: "
        + juce::String(static_cast<juce::int64>(midiRouter.getDroppedEvents())),
        juce::dontSendNotification);
    if (percussion.sequence != 0 && percussion.sequence != lastPercussionSequence)
    {
        lastPercussionSequence = percussion.sequence;
        dispatchPercussion(percussion);
        const char* hitName = "Unknown";
        switch (percussion.type)
        {
            case VocalPercussionType::kick: hitName = "Kick"; break;
            case VocalPercussionType::snare: hitName = "Snare"; break;
            case VocalPercussionType::hiHat: hitName = "Hi-hat"; break;
            case VocalPercussionType::none: break;
        }
        percussionLabel.setText("Percussion: " + juce::String(hitName)
            + "  |  confidence " + juce::String(percussion.confidence * 100.0f, 0) + "%",
            juce::dontSendNotification);
    }
    noteLabel.setText(pitch.voiced ? noteNameFromMidi(noteTracker.getActiveNote()) : "--",
                      juce::dontSendNotification);
    frequencyLabel.setText(juce::String(pitch.frequencyHz, 1) + " Hz  |  confidence "
        + juce::String(pitch.confidence * 100.0f, 0) + "%  |  input "
        + juce::String(pitch.rms, 3), juce::dontSendNotification);
    status.setText(pitch.voiced ? "Voice detected" : "Listening for voice",
                   juce::dontSendNotification);

    if (pitch.sequence != 0 && pitch.sequence != lastPitchSequence)
    {
        lastPitchSequence = pitch.sequence;
        updateNoteTracking(pitch);
    }
}
