#include "PresetManager.h"
#include <cmath>
#include <iostream>

namespace
{
    int failures = 0;

    void expect(bool condition, const char* message)
    {
        if (!condition)
        {
            ++failures;
            std::cerr << "FAIL: " << message << '\n';
        }
    }
}

int main()
{
    const auto testDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("NOVOX-PresetTests-" + juce::Uuid().toString());
    PresetManager manager(testDirectory);

    expect(PresetManager::sanitiseName("  Clean Voice  ") == "Clean Voice",
           "preset name trimming");
    expect(PresetManager::sanitiseName("../unsafe:name") == "..unsafename",
           "preset name path and punctuation sanitising");
    expect(PresetManager::sanitiseName("   ").isEmpty(),
           "empty preset names are rejected");

    PresetData original;
    original.inputGain = 2.35;
    original.pitchSensitivity = 72.0;
    original.pitchSmoothing = 18.0;
    original.pitchBendEnabled = false;
    original.pitchBendRange = 7;
    original.midiChannel = 11;
    original.midiOutputName = "Virtual MIDI Device";
    original.noiseReductionEnabled = true;
    original.inputEqualizerEnabled=true;original.equalizerLowDb=-3;original.equalizerMidDb=2.5;original.equalizerHighDb=4;
    original.noiseReductionStrength = 67.0;original.inputChannelMode=2;
    original.pitchStabilityEnabled = true;
    original.pitchDropToleranceCents = 62.0;
    original.pitchDropHoldMs = 50.0;
    original.midiNoteCleanupEnabled = true;
    original.midiNoteMinimumDurationMs = 75.0;
    original.midiNoteMinimumConfidence = 72.0;
    original.midiNoteGapBridgeMs = 30.0;

    juce::String error;
    expect(manager.savePreset("Studio Voice", original, &error), "save preset");
    expect(manager.listPresets().size() == 1, "list saved preset");

    PresetData loaded;
    expect(manager.loadPreset("Studio Voice", loaded, &error), "load preset");
    expect(std::abs(loaded.inputGain - original.inputGain) < 0.0001, "input gain round trip");
    expect(std::abs(loaded.pitchSensitivity - original.pitchSensitivity) < 0.0001,
           "sensitivity round trip");
    expect(std::abs(loaded.pitchSmoothing - original.pitchSmoothing) < 0.0001,
           "smoothing round trip");
    expect(loaded.pitchBendEnabled == original.pitchBendEnabled, "Pitch Bend toggle round trip");
    expect(loaded.pitchBendRange == original.pitchBendRange, "Pitch Bend range round trip");
    expect(loaded.midiChannel == original.midiChannel, "MIDI channel round trip");
    expect(loaded.midiOutputName == original.midiOutputName, "MIDI output round trip");
    expect(loaded.noiseReductionEnabled == original.noiseReductionEnabled, "noise reduction toggle round trip");
    expect(loaded.inputEqualizerEnabled==original.inputEqualizerEnabled,"EQ toggle round trip");expect(std::abs(loaded.equalizerLowDb-original.equalizerLowDb)<.0001,"EQ low round trip");expect(std::abs(loaded.equalizerMidDb-original.equalizerMidDb)<.0001,"EQ mid round trip");expect(std::abs(loaded.equalizerHighDb-original.equalizerHighDb)<.0001,"EQ high round trip");
    expect(std::abs(loaded.noiseReductionStrength - original.noiseReductionStrength) < 0.0001, "noise reduction strength round trip");
    expect(loaded.inputChannelMode==original.inputChannelMode,"input channel mode round trip");
    expect(loaded.pitchStabilityEnabled == original.pitchStabilityEnabled, "pitch stability toggle round trip");
    expect(std::abs(loaded.pitchDropToleranceCents - original.pitchDropToleranceCents) < 0.0001, "pitch drop tolerance round trip");
    expect(std::abs(loaded.pitchDropHoldMs - original.pitchDropHoldMs) < 0.0001, "pitch drop hold duration round trip");
    expect(loaded.midiNoteCleanupEnabled == original.midiNoteCleanupEnabled, "MIDI cleanup toggle round trip");
    expect(std::abs(loaded.midiNoteMinimumDurationMs - original.midiNoteMinimumDurationMs) < 0.0001, "MIDI cleanup minimum duration round trip");
    expect(std::abs(loaded.midiNoteMinimumConfidence - original.midiNoteMinimumConfidence) < 0.0001, "MIDI cleanup confidence round trip");
    expect(std::abs(loaded.midiNoteGapBridgeMs - original.midiNoteGapBridgeMs) < 0.0001, "MIDI cleanup gap bridge round trip");


    const auto legacyPreset = testDirectory.getChildFile("Legacy Voice.novoxpreset");
    expect(legacyPreset.replaceWithText(R"({
        "schemaVersion": 1,
        "name": "Legacy Voice",
        "inputGain": 1.0,
        "pitchSensitivity": 50.0,
        "pitchSmoothing": 35.0,
        "pitchBendEnabled": true,
        "pitchBendRange": 2,
        "midiChannel": 1,
        "midiOutputName": "",
        "noteTransitionStability": 6,
        "pitchDropHoldFrames": 3
    })"), "write legacy schema preset");
    PresetData migrated;
    expect(manager.loadPreset("Legacy Voice", migrated, &error), "load legacy schema preset");
    expect(std::abs(migrated.noteTransitionDurationMs - 60.0) < 0.0001,
           "legacy note transition frames migrate to milliseconds");
    expect(std::abs(migrated.pitchDropHoldMs - 30.0) < 0.0001,
           "legacy pitch stability frames migrate to milliseconds");
    expect(manager.deletePreset("Legacy Voice", &error), "delete migrated legacy preset");

    PresetData clamped;
    clamped.inputGain = 99.0;
    clamped.pitchSensitivity = -10.0;
    clamped.pitchSmoothing = 150.0;
    clamped.pitchBendRange = 99;
    clamped.midiChannel = 0;
    clamped.midiNoteMinimumDurationMs = 999.0;
    clamped.midiNoteMinimumConfidence = -10.0;
    clamped.midiNoteGapBridgeMs = 999.0;
    clamped.sanitise();
    expect(clamped.inputGain == 4.0, "input gain clamped");
    expect(clamped.pitchSensitivity == 0.0, "sensitivity clamped");
    expect(clamped.pitchSmoothing == 100.0, "smoothing clamped");
    expect(clamped.pitchBendRange == 12, "bend range clamped");
    expect(clamped.midiChannel == 1, "MIDI channel clamped");
    clamped.equalizerLowDb=-99;clamped.equalizerMidDb=99;clamped.equalizerHighDb=99;
    clamped.noiseReductionStrength=150.0;clamped.inputChannelMode=99;clamped.pitchDropToleranceCents=999.0;clamped.pitchDropHoldMs=99.0;clamped.sanitise();expect(clamped.noiseReductionStrength==100.0,"noise reduction strength clamped");expect(clamped.equalizerLowDb==-12,"EQ low clamp");expect(clamped.equalizerMidDb==12,"EQ mid clamp");expect(clamped.equalizerHighDb==12,"EQ high clamp");expect(clamped.inputChannelMode==3,"input channel mode clamped");expect(clamped.pitchDropToleranceCents==150.0,"pitch drop tolerance clamped");expect(clamped.pitchDropHoldMs==80.0,"pitch drop hold duration clamped");expect(clamped.midiNoteMinimumDurationMs==120.0,"MIDI cleanup duration clamped");expect(clamped.midiNoteMinimumConfidence==0.0,"MIDI cleanup confidence clamped");expect(clamped.midiNoteGapBridgeMs==80.0,"MIDI cleanup gap clamped");

    expect(manager.deletePreset("Studio Voice", &error), "delete preset");
    expect(manager.listPresets().empty(), "list empty after deletion");
    expect(!manager.loadPreset("Studio Voice", loaded, &error), "missing preset rejected");
    testDirectory.deleteRecursively();

    if (failures != 0)
        std::cerr << failures << " preset test(s) failed.\n";
    return failures == 0 ? 0 : 1;
}
