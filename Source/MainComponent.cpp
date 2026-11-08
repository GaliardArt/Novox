#include "MainComponent.h"

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

    gainSlider.setRange(0.1, 4.0, 0.05);
    gainSlider.setValue(1.0);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 24);
    gainSlider.onValueChange = [this] { engine.setInputGain(static_cast<float>(gainSlider.getValue())); };
    addAndMakeVisible(gainSlider);

    midiOutputSelector.onChange = [this] { selectMidiOutput(); };
    addAndMakeVisible(midiOutputSelector);

    refreshButton.onClick = [this] { refreshMidiOutputs(); };
    addAndMakeVisible(refreshButton);
    refreshMidiOutputs();

    const auto error = deviceManager.initialise(1, 0, nullptr, true);
    if (error.isNotEmpty())
        status.setText("Audio device error: " + error, juce::dontSendNotification);
    else
        deviceManager.addAudioCallback(&engine);

    setSize(960, 620);
    startTimerHz(60);
}

MainComponent::~MainComponent()
{
    stopTimer();
    if (noteTracker.getActiveNote() >= 0)
        midiRouter.noteOff(noteTracker.getActiveNote());
    midiRouter.closeOutput();
    deviceManager.removeAudioCallback(&engine);
    deviceManager.closeAudioDevice();
}

void MainComponent::refreshMidiOutputs()
{
    const int activeNote = noteTracker.getActiveNote();
    if (activeNote >= 0)
        midiRouter.noteOff(activeNote);
    noteTracker.reset();

    midiOutputSelector.clear(juce::dontSendNotification);
    const auto names = midiRouter.getOutputNames();
    midiOutputSelector.addItem("No MIDI output", 1);
    for (int i = 0; i < names.size(); ++i)
        midiOutputSelector.addItem(names[i], i + 2);
    midiOutputSelector.setSelectedId(1, juce::dontSendNotification);
    midiRouter.closeOutput();
    noteTracker.reset();
    midiLabel.setText("MIDI: choose an output device", juce::dontSendNotification);
}

void MainComponent::selectMidiOutput()
{
    const int activeNote = noteTracker.getActiveNote();
    if (activeNote >= 0)
        midiRouter.noteOff(activeNote);
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
}

void MainComponent::updateNoteTracking(const PitchResult& pitch)
{
    const auto update = noteTracker.process(pitch);
    if (update.noteOff >= 0)
        midiRouter.noteOff(update.noteOff);
    if (update.noteOn >= 0)
        midiRouter.noteOn(update.noteOn);
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
    title.setBounds(area.removeFromTop(48));
    status.setBounds(area.removeFromTop(36));
    area.removeFromTop(42);
    noteLabel.setBounds(area.removeFromTop(130));
    frequencyLabel.setBounds(area.removeFromTop(40));
    midiLabel.setBounds(area.removeFromTop(32));
    area.removeFromTop(34);
    gainSlider.setBounds(area.removeFromTop(36).withSizeKeepingCentre(420, 36));
    area.removeFromTop(14);
    midiOutputSelector.setBounds(area.removeFromTop(36).withSizeKeepingCentre(420, 36));
    refreshButton.setBounds(area.removeFromTop(40).withSizeKeepingCentre(180, 34));
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
