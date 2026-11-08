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
        pitchBendRange = std::clamp(pitchBendRange, 1, 12);
        midiChannel = std::clamp(midiChannel, 1, 16);
        if (midiOutputName.size() > 256)
            midiOutputName.resize(256);
    }
};
