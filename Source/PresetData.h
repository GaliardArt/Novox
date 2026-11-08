#pragma once

#include <algorithm>
#include <cmath>
#include <string>

struct PresetData
{
    double inputGain = 1.0;
    double pitchSensitivity = 50.0;
    double pitchSmoothing = 35.0;
    bool pitchBendEnabled = true;
    bool noteTransitionSmoothingEnabled = false;
    double noteTransitionDurationMs = 60.0;
    bool autoTuneEnabled = false;
    int autoTuneTonic = 0;
    int autoTuneScale = 1;
    bool noiseReductionEnabled = false;
    double noiseReductionStrength = 35.0;
    bool pitchStabilityEnabled = false;
    double pitchDropToleranceCents = 45.0;
    double pitchDropHoldMs = 30.0;
    bool midiNoteCleanupEnabled = false;
    double midiNoteMinimumDurationMs = 60.0;
    double midiNoteMinimumConfidence = 65.0;
    double midiNoteGapBridgeMs = 25.0;
    int inputChannelMode = 0;
    int pitchBendRange = 2;
    int midiChannel = 1;
    std::string midiOutputName;

    void sanitise() noexcept
    {
        inputGain = std::isfinite(inputGain) ? std::clamp(inputGain, 0.1, 4.0) : 1.0;
        pitchSensitivity = std::isfinite(pitchSensitivity) ? std::clamp(pitchSensitivity, 0.0, 100.0) : 50.0;
        pitchSmoothing = std::isfinite(pitchSmoothing) ? std::clamp(pitchSmoothing, 0.0, 100.0) : 35.0;
        noteTransitionDurationMs = std::isfinite(noteTransitionDurationMs)
            ? std::clamp(noteTransitionDurationMs, 30.0, 120.0) : 60.0;
        autoTuneTonic = std::clamp(autoTuneTonic, 0, 11);
        autoTuneScale = std::clamp(autoTuneScale, 0, 2);
        noiseReductionStrength = std::isfinite(noiseReductionStrength)
            ? std::clamp(noiseReductionStrength, 0.0, 100.0) : 35.0;
        pitchDropToleranceCents = std::isfinite(pitchDropToleranceCents)
            ? std::clamp(pitchDropToleranceCents, 10.0, 150.0) : 45.0;
        pitchDropHoldMs = std::isfinite(pitchDropHoldMs)
            ? std::clamp(pitchDropHoldMs, 10.0, 80.0) : 30.0;
        midiNoteMinimumDurationMs = std::isfinite(midiNoteMinimumDurationMs) ? std::clamp(midiNoteMinimumDurationMs, 20.0, 120.0) : 60.0;
        midiNoteMinimumConfidence = std::isfinite(midiNoteMinimumConfidence) ? std::clamp(midiNoteMinimumConfidence, 0.0, 100.0) : 65.0;
        midiNoteGapBridgeMs = std::isfinite(midiNoteGapBridgeMs) ? std::clamp(midiNoteGapBridgeMs, 0.0, 80.0) : 25.0;
        inputChannelMode = std::clamp(inputChannelMode, 0, 3);
        pitchBendRange = std::clamp(pitchBendRange, 1, 12);
        midiChannel = std::clamp(midiChannel, 1, 16);
        if (midiOutputName.size() > 256)
            midiOutputName.resize(256);
    }
};
