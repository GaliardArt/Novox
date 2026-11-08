#pragma once
#include <algorithm>
#include <cmath>

class PitchSmoother
{
public:
    // smoothing is 0..100. Zero is transparent; higher values attenuate
    // frame-to-frame jitter while retaining larger expressive pitch movement.
    float process(float semitoneOffset, float smoothing) noexcept
    {
        const float amount = std::clamp(smoothing, 0.0f, 100.0f);
        if (!std::isfinite(semitoneOffset))
            return initialized ? currentValue : 0.0f;

        if (!initialized || amount <= 0.0f)
        {
            currentValue = semitoneOffset;
            targetValue = semitoneOffset;
            initialized = true;
            return currentValue;
        }

        // Apply the deadband to changes in the incoming target, not to the
        // remaining filter error. A sustained target must still converge fully.
        const float deadband = amount * 0.0003f;
        if (std::abs(semitoneOffset - targetValue) > deadband)
            targetValue = semitoneOffset;

        const float alpha = 1.0f - amount * 0.0065f;
        currentValue += alpha * (targetValue - currentValue);
        return currentValue;
    }

    void reset() noexcept
    {
        currentValue = 0.0f;
        targetValue = 0.0f;
        initialized = false;
    }

    bool isInitialized() const noexcept { return initialized; }
    float getCurrentValue() const noexcept { return currentValue; }

private:
    float currentValue = 0.0f;
    float targetValue = 0.0f;
    bool initialized = false;
};
