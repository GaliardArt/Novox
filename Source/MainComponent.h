#pragma once
#include <JuceHeader.h>
#include "VoiceEngine.h"
#include "MidiRouter.h"
#include "PresetManager.h"
#include "NovoxLookAndFeel.h"
#include <cstdint>

class MainComponent final : public juce::Component, private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    enum class Page { play, midi, audio, presets };
    void timerCallback() override;
    void showPage(Page);
    void refreshMidiOutputs();
    void selectMidiOutput();
    void selectMidiChannel();
    void showAudioSetup();
    void saveSettings();
    PresetData capturePresetData() const;
    void applyPresetData(PresetData, const juce::String&);
    void refreshPresetList(const juce::String& selectedName = {});
    void loadSelectedPreset();
    void saveCurrentPreset();
    void deleteSelectedPreset();
    void styleControls();
    void layoutPlayPage(juce::Rectangle<int>);
    void layoutMidiPage(juce::Rectangle<int>);
    void layoutAudioPage(juce::Rectangle<int>);
    void layoutPresetsPage(juce::Rectangle<int>);

    NovoxLookAndFeel novoxLookAndFeel;
    Page currentPage = Page::play;
    juce::TextButton playNav { "PLAY" }, midiNav { "MIDI" },
                     audioNav { "AUDIO" }, presetsNav { "PRESETS" };
    juce::ApplicationProperties appProperties;
    juce::PropertiesFile* settings = nullptr;
    juce::String preferredMidiOutputName;
    PresetManager presetManager;
    VoiceEngine engine;
    MidiRouter midiRouter;
    juce::String lastAudioStateXml;
    juce::AudioDeviceManager deviceManager;

    juce::ComboBox midiOutputSelector, channelSelector, presetSelector;
    juce::TextButton refreshButton { "Refresh" }, audioSetupButton { "Audio Setup" };
    juce::ToggleButton pitchBendEnabled { "Pitch Bend" };
    juce::ToggleButton noteTransitionEnabled { "Note transition smoothing" };
    juce::ToggleButton autoTuneEnabled { "Auto-Tune" };
    juce::ToggleButton noiseReductionEnabled { "Noise reduction (gate)" };
    juce::ToggleButton inputEqualizerEnabled { "Input equalizer" };
    juce::Slider equalizerLowSlider, equalizerMidSlider, equalizerHighSlider;
    juce::Label equalizerLowLabel, equalizerMidLabel, equalizerHighLabel;
    juce::ToggleButton pitchStabilityEnabled { "Pitch Stability Guard" }, midiNoteCleanupEnabled { "MIDI Note Cleanup" };
    juce::Slider midiNoteMinimumDurationSlider, midiNoteMinimumConfidenceSlider, midiNoteGapBridgeSlider;
    juce::Label midiCleanupDurationLabel, midiCleanupConfidenceLabel, midiCleanupGapLabel;
    juce::ComboBox autoTuneKeySelector, autoTuneScaleSelector, inputChannelSelector;
    juce::TextButton calibrateNoiseButton { "Calibrate 2s" };
    juce::Slider bendRangeSlider, gainSlider, sensitivitySlider, smoothingSlider, noteTransitionSlider, noiseReductionSlider, pitchDropToleranceSlider, pitchDropHoldSlider;
    juce::TextEditor presetNameEditor;
    juce::TextButton savePresetButton { "Save / Update" },
                     loadPresetButton { "Load preset" },
                     deletePresetButton { "Delete" };

    juce::String currentNote = "--";
    juce::String audioDeviceName = "Checking audio device...";
    juce::String midiStatus = "No MIDI output selected";
    juce::String operationStatus = "Ready";
    float currentFrequency = 0.0f, currentConfidence = 0.0f, currentRms = 0.0f;
    float currentInputRms = 0.0f, currentInputPeak = 0.0f;
    bool currentInputClipping = false;
    int currentAnalysisWindowSamples = 0;
    bool audioAvailable = false;
    int audioChannels = 0, bufferSize = 0;
    double sampleRate = 0.0;
    int lastNoiseCalibrationState = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
