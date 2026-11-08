#pragma once
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <algorithm>
#include <cmath>
class NoiseReducer final
{
public:
    void prepare(double sampleRate, int maximumBlockSize)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        spec.maximumBlockSize = static_cast<juce::uint32>(juce::jmax(1, maximumBlockSize));
        spec.numChannels = 1;
        gate.prepare(spec);
        gate.setAttack(2.0f);
        gate.setRelease(110.0f);
        gate.setRatio(8.0f);
        gate.setThreshold(-49.5f);
        gate.reset();
        appliedStrength = -1.0f;
        wasEnabled = false;
    }
    void setEnabled(bool value) noexcept { enabled.store(value, std::memory_order_relaxed); }
    void setStrength(float value) noexcept { strength.store(std::clamp(value, 0.0f, 1.0f), std::memory_order_relaxed); }
    void process(float* samples, int count) noexcept
    {
        if (samples == nullptr || count <= 0) return;
        const bool active = enabled.load(std::memory_order_relaxed);
        if (!active)
        {
            if (wasEnabled) gate.reset();
            wasEnabled = false;
            return;
        }
        if (!wasEnabled) { gate.reset(); wasEnabled = true; }
        const float currentStrength = strength.load(std::memory_order_relaxed);
        if (std::abs(currentStrength - appliedStrength) >= 0.005f)
        {
            gate.setThreshold(-60.0f + currentStrength * 30.0f);
            appliedStrength = currentStrength;
        }
        for (int i = 0; i < count; ++i)
            samples[i] = gate.processSample(0, samples[i]);
    }
private:
    juce::dsp::NoiseGate<float> gate;
    std::atomic<bool> enabled { false };
    std::atomic<float> strength { 0.35f };
    float appliedStrength = -1.0f;
    bool wasEnabled = false;
};
