#pragma once
#include <JuceHeader.h>
#include "VoiceEngine.h"
#include "MidiRouter.h"
#include "NoteTracker.h"

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshMidiOutputs();
    void selectMidiOutput();
    void updateNoteTracking(const PitchResult& pitch);

    VoiceEngine engine;
    MidiRouter midiRouter;
    NoteTracker noteTracker;
    juce::Label title, status, noteLabel, frequencyLabel, midiLabel;
    juce::Slider gainSlider;
    juce::ComboBox midiOutputSelector;
    juce::TextButton refreshButton { "Refresh MIDI" };
    juce::AudioDeviceManager deviceManager;

    uint64_t lastPitchSequence = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
