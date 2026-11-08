#pragma once
#include <algorithm>
#include <cmath>

class PitchSmoother
{
public:
    float process(float semitoneOffset, float smoothing, float elapsedSeconds = 0.01f) noexcept
    {
        const float amount = std::clamp(smoothing, 0.0f, 100.0f);
        const float delta = std::clamp(elapsedSeconds, 0.0f, 0.25f);
        if (!std::isfinite(semitoneOffset))
            return initialized ? currentValue : 0.0f;

        if (!initialized || amount <= 0.0f || delta <= 0.0f)
        {
            currentValue = semitoneOffset;
            targetValue = semitoneOffset;
            initialized = true;
            return currentValue;
        }

        const float deadband = amount * 0.0003f;
        if (std::abs(semitoneOffset - targetValue) > deadband)
            targetValue = semitoneOffset;

        const float baseAlpha = 1.0f - amount * 0.0065f;
        const float alpha = 1.0f - std::pow(1.0f - baseAlpha, delta / 0.01f);
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
