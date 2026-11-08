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
    double noteTransitionStability = 6.0;
    bool autoTuneEnabled = false;
    int autoTuneTonic = 0;
    int autoTuneScale = 1;
    bool noiseReductionEnabled = false;
    double noiseReductionStrength = 35.0;
    int inputChannelMode = 0;
    int pitchBendRange = 2;
    int midiChannel = 1;
    std::string midiOutputName;

    void sanitise() noexcept
    {
        inputGain = std::isfinite(inputGain) ? std::clamp(inputGain, 0.1, 4.0) : 1.0;
        pitchSensitivity = std::isfinite(pitchSensitivity)
            ? std::clamp(pitchSensitivity, 0.0, 100.0) : 50.0;
        pitchSmoothing = std::isfinite(pitchSmoothing)
            ? std::clamp(pitchSmoothing, 0.0, 100.0) : 35.0;
        noteTransitionStability = std::isfinite(noteTransitionStability)
            ? std::clamp(noteTransitionStability, 3.0, 12.0) : 6.0;
        autoTuneTonic=std::clamp(autoTuneTonic,0,11); autoTuneScale=std::clamp(autoTuneScale,0,2);
        noiseReductionStrength = std::isfinite(noiseReductionStrength) ? std::clamp(noiseReductionStrength, 0.0, 100.0) : 35.0;
        inputChannelMode = std::clamp(inputChannelMode, 0, 3);
        pitchBendRange = std::clamp(pitchBendRange, 1, 12);
        midiChannel = std::clamp(midiChannel, 1, 16);
        if (midiOutputName.size() > 256)
            midiOutputName.resize(256);
    }
};
