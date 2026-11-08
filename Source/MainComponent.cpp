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

    gainLabel.setText("Input gain", juce::dontSendNotification);
    gainLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(gainLabel);
    gainSlider.setRange(0.1, 4.0, 0.05);
    gainSlider.setValue(1.0);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 24);
    gainSlider.onValueChange = [this] { engine.setInputGain(static_cast<float>(gainSlider.getValue())); };
    addAndMakeVisible(gainSlider);

    bendRangeLabel.setText("Pitch Bend range", juce::dontSendNotification);
    bendRangeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(bendRangeLabel);
    bendRangeSlider.setRange(1, 12, 1);
    bendRangeSlider.setValue(2, juce::dontSendNotification);
    bendRangeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 24);
    bendRangeSlider.setTextValueSuffix(" semitones");
    bendRangeSlider.onValueChange = [this] { lastPitchBendValue = -1; };
    addAndMakeVisible(bendRangeSlider);

    pitchBendEnabled.setToggleState(true, juce::dontSendNotification);
    pitchBendEnabled.onClick = [this] {
        if (!pitchBendEnabled.getToggleState())
            resetPitchBend();
        else
            lastPitchBendValue = -1;
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
    channelSelector.setSelectedId(1, juce::dontSendNotification);
    channelSelector.onChange = [this] { selectMidiChannel(); };
    addAndMakeVisible(channelSelector);
    midiRouter.setChannel(1);

    refreshButton.onClick = [this] { refreshMidiOutputs(); };
    addAndMakeVisible(refreshButton);
    refreshMidiOutputs();

    const auto error = deviceManager.initialise(1, 0, nullptr, true);
    if (error.isNotEmpty())
        status.setText("Audio device error: " + error, juce::dontSendNotification);
    else
        deviceManager.addAudioCallback(&engine);

    setSize(960, 760);
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

    midiOutputSelector.clear(juce::dontSendNotification);
    const auto names = midiRouter.getOutputNames();
    midiOutputSelector.addItem("No MIDI output", 1);
    for (int i = 0; i < names.size(); ++i)
        midiOutputSelector.addItem(names[i], i + 2);
    midiOutputSelector.setSelectedId(1, juce::dontSendNotification);
    midiRouter.closeOutput();
    lastPitchBendValue = 8192;
    noteTracker.reset();
    midiLabel.setText("MIDI: choose an output device", juce::dontSendNotification);
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

    const int selectedId = midiOutputSelector.getSelectedId();
    if (selectedId <= 1)
    {
        midiRouter.closeOutput();
        midiLabel.setText("MIDI: no output selected", juce::dontSendNotification);
        return;
    }

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
        midiRouter.noteOff(activeNote); // Enqueued on the previous channel.
    }

    midiRouter.setChannel(channelSelector.getSelectedId());
    noteTracker.reset();
    lastPitchBendValue = 8192;
    midiLabel.setText("MIDI: channel " + juce::String(midiRouter.getChannel()),
                      juce::dontSendNotification);
}

void MainComponent::resetPitchBend()
{
    if (midiRouter.isOpen() && lastPitchBendValue != PitchBend::center)
        midiRouter.pitchBend(PitchBend::center);
    lastPitchBendValue = PitchBend::center;
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

    const int range = static_cast<int>(bendRangeSlider.getValue());
    const int value = PitchBend::to14BitValue(pitch.frequencyHz, activeNote, range);

    // Avoid filling the asynchronous MIDI queue with imperceptible changes.
    if (midiRouter.isOpen() && std::abs(value - lastPitchBendValue) >= 2)
    {
        midiRouter.pitchBend(value);
        lastPitchBendValue = value;
    }

    const auto offset = PitchBend::semitoneOffset(pitch.frequencyHz, activeNote);
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
    area.removeFromTop(16);

    gainLabel.setBounds(area.removeFromTop(22).withSizeKeepingCentre(420, 22));
    gainSlider.setBounds(area.removeFromTop(34).withSizeKeepingCentre(420, 34));
    area.removeFromTop(8);
    pitchBendEnabled.setBounds(area.removeFromTop(28).withSizeKeepingCentre(220, 28));
    bendRangeLabel.setBounds(area.removeFromTop(22).withSizeKeepingCentre(420, 22));
    bendRangeSlider.setBounds(area.removeFromTop(34).withSizeKeepingCentre(420, 34));
    area.removeFromTop(10);
    midiOutputSelector.setBounds(area.removeFromTop(34).withSizeKeepingCentre(420, 34));
    area.removeFromTop(8);
    channelLabel.setBounds(area.removeFromTop(22).withSizeKeepingCentre(420, 22));
    channelSelector.setBounds(area.removeFromTop(34).withSizeKeepingCentre(240, 34));
    area.removeFromTop(8);
    refreshButton.setBounds(area.removeFromTop(36).withSizeKeepingCentre(180, 32));
}

void MainComponent::timerCallback()
{
    const auto pitch = engine.getLatestPitch();
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
