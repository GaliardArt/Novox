#pragma once
#include <JuceHeader.h>
#include "VoiceEngine.h"
#include "MidiRouter.h"
#include "NoteTracker.h"
#include "PitchSmoother.h"
#include "VocalPercussionDetector.h"
#include "DrumMapping.h"
#include "PresetManager.h"
#include <array>
#include <cstdint>

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
    void selectMidiChannel();
    void updateNoteTracking(const PitchResult& pitch);
    void sendPitchBend(const PitchResult& pitch);
    void resetPitchBend();
    void saveSettings();
    void dispatchPercussion(const VocalPercussionResult& percussion);
    void releaseDueDrumNotes();
    void releaseAllDrumNotes();
    PresetData capturePresetData() const;
    void applyPresetData(PresetData data, const juce::String& presetName);
    void refreshPresetList(const juce::String& selectedName = {});
    void loadSelectedPreset();
    void saveCurrentPreset();
    void deleteSelectedPreset();

    juce::ApplicationProperties appProperties;
    juce::PropertiesFile* settings = nullptr;
    juce::String preferredMidiOutputName;
    PresetManager presetManager;

    VoiceEngine engine;
    MidiRouter midiRouter;
    NoteTracker noteTracker;
    PitchSmoother pitchSmoother;

    juce::Label title, status, noteLabel, frequencyLabel, midiLabel, percussionLabel;
    juce::Label audioDiagnosticsLabel, performanceDiagnosticsLabel;
    juce::Label gainLabel, bendRangeLabel, channelLabel, pitchBendLabel;
    juce::Label sensitivityLabel, smoothingLabel;
    juce::Label drumMappingLabel, kickLabel, snareLabel, hiHatLabel;
    juce::Label presetsLabel;
    juce::TextEditor presetNameEditor;
    juce::ComboBox presetSelector;
    juce::TextButton savePresetButton { "Save / Update" };
    juce::TextButton loadPresetButton { "Load" };
    juce::TextButton deletePresetButton { "Delete" };
    juce::Slider gainSlider, bendRangeSlider, sensitivitySlider, smoothingSlider;
    juce::ComboBox midiOutputSelector, channelSelector;
    juce::ComboBox kickNoteSelector, snareNoteSelector, hiHatNoteSelector;
    juce::TextButton refreshButton { "Refresh MIDI" };
    juce::ToggleButton pitchBendEnabled { "Enable Pitch Bend" };
    juce::ToggleButton drumMidiEnabled { "Send percussion to MIDI (channel 10)" };
    juce::AudioDeviceManager deviceManager;

    uint64_t lastPitchSequence = 0;
    int lastPitchBendValue = 8192;
    uint64_t lastPercussionSequence = 0;
    std::array<int, 3> pendingDrumNotes { -1, -1, -1 };
    std::array<uint32_t, 3> drumNoteOffTimes {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
