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
    original.noiseReductionStrength = 67.0;original.inputChannelMode=2;

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
    expect(std::abs(loaded.noiseReductionStrength - original.noiseReductionStrength) < 0.0001, "noise reduction strength round trip");
    expect(loaded.inputChannelMode==original.inputChannelMode,"input channel mode round trip");

    PresetData clamped;
    clamped.inputGain = 99.0;
    clamped.pitchSensitivity = -10.0;
    clamped.pitchSmoothing = 150.0;
    clamped.pitchBendRange = 99;
    clamped.midiChannel = 0;
    clamped.sanitise();
    expect(clamped.inputGain == 4.0, "input gain clamped");
    expect(clamped.pitchSensitivity == 0.0, "sensitivity clamped");
    expect(clamped.pitchSmoothing == 100.0, "smoothing clamped");
    expect(clamped.pitchBendRange == 12, "bend range clamped");
    expect(clamped.midiChannel == 1, "MIDI channel clamped");
    clamped.noiseReductionStrength=150.0;clamped.inputChannelMode=99;clamped.sanitise();expect(clamped.noiseReductionStrength==100.0,"noise reduction strength clamped");expect(clamped.inputChannelMode==3,"input channel mode clamped");

    expect(manager.deletePreset("Studio Voice", &error), "delete preset");
    expect(manager.listPresets().empty(), "list empty after deletion");
    expect(!manager.loadPreset("Studio Voice", loaded, &error), "missing preset rejected");
    testDirectory.deleteRecursively();

    if (failures != 0)
        std::cerr << failures << " preset test(s) failed.\n";
    return failures == 0 ? 0 : 1;
}
