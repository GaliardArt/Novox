#pragma once
#include <JuceHeader.h>
#include "VoiceEngine.h"
#include "MidiRouter.h"
#include "NoteTracker.h"
#include "PitchSmoother.h"
#include "PresetManager.h"
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
    PresetData capturePresetData() const;
    void applyPresetData(PresetData data, const juce::String& presetName);
    void refreshPresetList(const juce::String& selectedName = {});
    void loadSelectedPreset();
    void saveCurrentPreset();
    void deleteSelectedPreset();

    juce::LookAndFeel_V4 novoxLookAndFeel;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::Component playPage, midiPage, audioPage, presetsPage;

    juce::ApplicationProperties appProperties;
    juce::PropertiesFile* settings = nullptr;
    juce::String preferredMidiOutputName;
    PresetManager presetManager;

    VoiceEngine engine;
    MidiRouter midiRouter;
    NoteTracker noteTracker;
    PitchSmoother pitchSmoother;

    juce::Label title, status, noteLabel, frequencyLabel, midiLabel;
    juce::Label audioDiagnosticsLabel, performanceDiagnosticsLabel;
    juce::Label gainLabel, bendRangeLabel, channelLabel, pitchBendLabel;
    juce::Label sensitivityLabel, smoothingLabel;
    juce::Label presetsLabel;
    juce::TextEditor presetNameEditor;
    juce::ComboBox presetSelector;
    juce::TextButton savePresetButton { "Save / Update" };
    juce::TextButton loadPresetButton { "Load" };
    juce::TextButton deletePresetButton { "Delete" };
    juce::Slider gainSlider, bendRangeSlider, sensitivitySlider, smoothingSlider;
    juce::ComboBox midiOutputSelector, channelSelector;
    juce::TextButton refreshButton { "Refresh MIDI" };
    juce::ToggleButton pitchBendEnabled { "Enable Pitch Bend" };
    juce::AudioDeviceManager deviceManager;

    uint64_t lastPitchSequence = 0;
    int lastPitchBendValue = 8192;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
