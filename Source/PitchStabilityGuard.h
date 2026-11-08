#pragma once
#include "PitchDetector.h"
#include <algorithm>
#include <cmath>

class PitchStabilityGuard
{
public:
    void configure(bool on, float cents, float holdMs) noexcept
    {
        const float safeHold = std::clamp(holdMs, 10.0f, 80.0f) * 0.001f;
        const float safeCents = std::clamp(cents, 10.0f, 150.0f);
        const bool changed = enabled != on
            || std::abs(holdSeconds - safeHold) > 0.0001f
            || std::abs(dropToleranceCents - safeCents) > 0.01f;
        enabled = on;
        holdSeconds = safeHold;
        dropToleranceCents = safeCents;
        if (changed)
            reset();
    }

    PitchResult process(const PitchResult& input, float elapsedSeconds = 0.01f) noexcept
    {
        if (!enabled)
        {
            reset();
            return input;
        }

        const float delta = std::clamp(elapsedSeconds, 0.0f, 0.25f);
        if (!input.voiced || !(input.frequencyHz > 0.0f))
        {
            if (stable.voiced && pendingSeconds + delta + 1.0e-6f < holdSeconds)
            {
                pendingSeconds += delta;
                return held(input);
            }

            stable = {};
            pendingSeconds = 0.0f;
            return input;
        }

        if (!stable.voiced || !(stable.frequencyHz > 0.0f))
        {
            stable = input;
            pendingSeconds = 0.0f;
            return input;
        }

        const float cents = 1200.0f * std::log2(input.frequencyHz / stable.frequencyHz);
        if (cents < -dropToleranceCents)
        {
            pendingSeconds += delta;
            if (pendingSeconds + 1.0e-6f < holdSeconds)
                return held(input);

            stable = input;
            pendingSeconds = 0.0f;
            return input;
        }

        pendingSeconds = 0.0f;
        stable = input;
        return input;
    }

    void reset() noexcept
    {
        stable = {};
        pendingSeconds = 0.0f;
    }

private:
    PitchResult held(const PitchResult& input) const noexcept
    {
        auto result = stable;
        result.sequence = input.sequence;
        result.rms = input.rms;
        result.analysisWindowSamples = input.analysisWindowSamples;
        return result;
    }

    PitchResult stable;
    float pendingSeconds = 0.0f;
    float holdSeconds = 0.030f;
    float dropToleranceCents = 45.0f;
    bool enabled = false;
};
