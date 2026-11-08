#pragma once
#include "PitchDetector.h"
#include <algorithm>
#include <cmath>

class MidiNoteCleanup
{
public:
    void configure(bool enabled, float minimumDurationMs, float minimumConfidencePercent,
                   float gapBridgeMs) noexcept
    {
        const bool wasEnabled = this->enabled;
        this->enabled = enabled;
        minimumDurationSeconds = std::clamp(minimumDurationMs, 20.0f, 120.0f) * 0.001f;
        minimumConfidence = std::clamp(minimumConfidencePercent, 0.0f, 100.0f) * 0.01f;
        gapBridgeSeconds = std::clamp(gapBridgeMs, 0.0f, 80.0f) * 0.001f;
        if (wasEnabled != enabled)
            reset();
    }

    PitchResult process(const PitchResult& input, float elapsedSeconds) noexcept
    {
        if (!enabled)
            return input;

        const float delta = std::clamp(elapsedSeconds, 0.0f, 0.25f);
        const bool confident = input.voiced && input.confidence >= minimumConfidence
            && input.frequencyHz > 0.0f && std::isfinite(input.frequencyHz);

        if (!confident)
        {
            candidateNote = -1;
            candidateSeconds = 0.0f;
            if (confirmedNote >= 0 && gapSeconds + delta <= gapBridgeSeconds + 1.0e-6f)
            {
                gapSeconds += delta;
                auto bridged = lastConfirmedPitch;
                bridged.sequence = input.sequence;
                bridged.rms = input.rms;
                return bridged;
            }
            gapSeconds += delta;
            confirmedNote = -1;
            return {};
        }

        gapSeconds = 0.0f;
        const int note = static_cast<int>(std::lround(69.0 + 12.0 * std::log2(input.frequencyHz / 440.0)));
        if (note == confirmedNote)
        {
            candidateNote = -1;
            candidateSeconds = 0.0f;
            lastConfirmedPitch = input;
            return input;
        }

        if (note != candidateNote)
        {
            candidateNote = note;
            candidateSeconds = delta;
        }
        else
        {
            candidateSeconds += delta;
        }

        if (candidateSeconds + 1.0e-6f < minimumDurationSeconds)
            return confirmedNote >= 0 ? lastConfirmedPitch : PitchResult {};

        confirmedNote = note;
        candidateNote = -1;
        candidateSeconds = 0.0f;
        lastConfirmedPitch = input;
        return input;
    }

    void reset() noexcept
    {
        candidateNote = -1;
        confirmedNote = -1;
        candidateSeconds = 0.0f;
        gapSeconds = 0.0f;
        lastConfirmedPitch = {};
    }

private:
    bool enabled = false;
    float minimumDurationSeconds = 0.060f;
    float minimumConfidence = 0.65f;
    float gapBridgeSeconds = 0.025f;
    int candidateNote = -1;
    int confirmedNote = -1;
    float candidateSeconds = 0.0f;
    float gapSeconds = 0.0f;
    PitchResult lastConfirmedPitch {};
};
