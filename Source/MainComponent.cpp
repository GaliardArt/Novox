#include "MainComponent.h"
#include "PitchBend.h"

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
    addAndMakeVisible(noteLabel);

    frequencyLabel.setText("0.0 Hz  |  confidence 0%", juce::dontSendNotification);
    frequencyLabel.setJustificationType(juce::Justification::centred);
    frequencyLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(frequencyLabel);

    midiLabel.setText("MIDI: no output selected", juce::dontSendNotification);
    midiLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(midiLabel);

    percussionLabel.setText("Percussion: waiting", juce::dontSendNotification);
    percussionLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    percussionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(percussionLabel);

    gainLabel.setText("Input gain", juce::dontSendNotification);
    gainLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(gainLabel);
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
    addAndMakeVisible(gainSlider);

    sensitivityLabel.setText("Pitch sensitivity", juce::dontSendNotification);
    sensitivityLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(sensitivityLabel);
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
    addAndMakeVisible(sensitivitySlider);

    smoothingLabel.setText("Pitch smoothing", juce::dontSendNotification);
    smoothingLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(smoothingLabel);
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
    addAndMakeVisible(smoothingSlider);

    bendRangeLabel.setText("Pitch Bend range", juce::dontSendNotification);
    bendRangeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(bendRangeLabel);
    bendRangeSlider.setRange(1, 12, 1);
    bendRangeSlider.setValue(savedBendRange, juce::dontSendNotification);
    bendRangeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 24);
    bendRangeSlider.setTextValueSuffix(" semitones");
    bendRangeSlider.onValueChange = [this] {
        lastPitchBendValue = -1;
        settings->setValue("pitchBendRange", static_cast<int>(bendRangeSlider.getValue()));
        saveSettings();
    };
    addAndMakeVisible(bendRangeSlider);

    pitchBendEnabled.setToggleState(savedBendEnabled, juce::dontSendNotification);
    pitchBendEnabled.onClick = [this] {
        if (!pitchBendEnabled.getToggleState())
            resetPitchBend();
        else
            lastPitchBendValue = -1;
        settings->setValue("pitchBendEnabled", pitchBendEnabled.getToggleState());
        saveSettings();
    };
    addAndMakeVisible(pitchBendEnabled);

    pitchBendLabel.setText("Pitch Bend: center", juce::dontSendNotification);
    pitchBendLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    pitchBendLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(pitchBendLabel);

    midiOutputSelector.onChange = [this] { selectMidiOutput(); };
    addAndMakeVisible(midiOutputSelector);

    channelLabel.setText("MIDI channel", juce::dontSendNotification);
    channelLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(channelLabel);
    for (int channel = 1; channel <= 16; ++channel)
        channelSelector.addItem("Channel " + juce::String(channel), channel);
    channelSelector.setSelectedId(savedChannel, juce::dontSendNotification);
    channelSelector.onChange = [this] { selectMidiChannel(); };
    addAndMakeVisible(channelSelector);
    midiRouter.setChannel(savedChannel);

    refreshButton.onClick = [this] { refreshMidiOutputs(); };
    addAndMakeVisible(refreshButton);
    refreshMidiOutputs();

    const auto error = deviceManager.initialise(1, 0, nullptr, true);
    if (error.isNotEmpty())
        status.setText("Audio device error: " + error, juce::dontSendNotification);
    else
        deviceManager.addAudioCallback(&engine);

    setSize(960, 900);
    startTimerHz(60);
}

MainComponent::~MainComponent()
{
    stopTimer();
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

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff15191f));
    g.setColour(juce::Colour(0xff252d37));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(24.0f), 18.0f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(48);
    title.setBounds(area.removeFromTop(44));
    status.setBounds(area.removeFromTop(30));
    area.removeFromTop(18);
    noteLabel.setBounds(area.removeFromTop(116));
    frequencyLabel.setBounds(area.removeFromTop(34));
    pitchBendLabel.setBounds(area.removeFromTop(28));
    midiLabel.setBounds(area.removeFromTop(28));
    percussionLabel.setBounds(area.removeFromTop(26));
    area.removeFromTop(12);

    gainLabel.setBounds(area.removeFromTop(22).withSizeKeepingCentre(420, 22));
    gainSlider.setBounds(area.removeFromTop(34).withSizeKeepingCentre(420, 34));
    area.removeFromTop(5);
    sensitivityLabel.setBounds(area.removeFromTop(22).withSizeKeepingCentre(420, 22));
    sensitivitySlider.setBounds(area.removeFromTop(34).withSizeKeepingCentre(420, 34));
    area.removeFromTop(5);
    smoothingLabel.setBounds(area.removeFromTop(22).withSizeKeepingCentre(420, 22));
    smoothingSlider.setBounds(area.removeFromTop(34).withSizeKeepingCentre(420, 34));
    area.removeFromTop(6);
    pitchBendEnabled.setBounds(area.removeFromTop(28).withSizeKeepingCentre(220, 28));
    bendRangeLabel.setBounds(area.removeFromTop(22).withSizeKeepingCentre(420, 22));
    bendRangeSlider.setBounds(area.removeFromTop(34).withSizeKeepingCentre(420, 34));
    area.removeFromTop(8);
    midiOutputSelector.setBounds(area.removeFromTop(34).withSizeKeepingCentre(420, 34));
    area.removeFromTop(5);
    channelLabel.setBounds(area.removeFromTop(22).withSizeKeepingCentre(420, 22));
    channelSelector.setBounds(area.removeFromTop(34).withSizeKeepingCentre(240, 34));
    area.removeFromTop(6);
    refreshButton.setBounds(area.removeFromTop(36).withSizeKeepingCentre(180, 32));
}

void MainComponent::timerCallback()
{
    const auto pitch = engine.getLatestPitch();
    const auto percussion = engine.getLatestPercussion();
    if (percussion.sequence != 0 && percussion.sequence != lastPercussionSequence)
    {
        lastPercussionSequence = percussion.sequence;
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
