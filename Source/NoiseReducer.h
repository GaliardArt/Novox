#pragma once

#include <juce_dsp/juce_dsp.h>
#include <rnnoise.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <memory>

class NoiseReducer final
{
public:
    static constexpr int frameSize = 480;

    NoiseReducer() = default;
    ~NoiseReducer() { release(); }

    NoiseReducer(const NoiseReducer&) = delete;
    NoiseReducer& operator=(const NoiseReducer&) = delete;

    void prepare(double rate, int maximumBlockSize)
    {
        release();
        sampleRate = rate > 0.0 ? rate : 48000.0;
        maxBlock = juce::jmax(1, maximumBlockSize);
        inputCount = 0;
        outputRead = outputWrite = 0;
        enabledWas = false;
        frameInput.fill(0.0f);
        frameOutput.fill(0.0f);
        // RNNoise's native model is trained for 48 kHz. Until the streaming
        // resampler is active, fail closed to transparent bypass at other rates.
        if (std::abs(sampleRate - 48000.0) < 0.5)
            state = rnnoise_create(nullptr);
        available.store(state != nullptr, std::memory_order_release);
    }

    void setEnabled(bool shouldEnable) noexcept
    {
        enabled.store(shouldEnable, std::memory_order_relaxed);
    }

    void setStrength(float) noexcept
    {
        // RNNoise does not expose a suppression-strength parameter. Retained
        // temporarily for source/config compatibility; intentionally no-op.
    }

    bool isAvailable() const noexcept
    {
        return available.load(std::memory_order_acquire);
    }

    void process(float* samples, int count) noexcept
    {
        if (samples == nullptr || count <= 0)
            return;

        if (!enabled.load(std::memory_order_relaxed) || state == nullptr)
        {
            if (enabledWas)
                resetState();
            enabledWas = false;
            inputCount = 0;
            outputRead = outputWrite = 0;
            return;
        }

        if (!enabledWas)
        {
            resetState();
            enabledWas = true;
        }

        // The worker is the sole owner of this instance. A bounded FIFO keeps
        // the frame accumulator allocation-free and supports arbitrary blocks.
        for (int i = 0; i < count; ++i)
        {
            const float x = std::isfinite(samples[i]) ? samples[i] : 0.0f;
            frameInput[static_cast<size_t>(inputCount++)] = x;
            if (inputCount == frameSize)
            {
                processFrame();
                inputCount = 0;
            }
            if (outputRead < outputWrite)
                samples[i] = frameOutput[static_cast<size_t>(outputRead++)];
            else
                samples[i] = 0.0f;
        }
        if (outputRead == outputWrite)
            outputRead = outputWrite = 0;
    }

private:
    void processFrame() noexcept
    {
        if (state == nullptr)
            return;
        for (int i = 0; i < frameSize; ++i)
            frameOutput[static_cast<size_t>(i)] = frameInput[static_cast<size_t>(i)] * 32768.0f;

        rnnoise_process_frame(state, frameOutput.data(), frameOutput.data());

        for (auto& sample : frameOutput)
        {
            const float normalized = sample / 32768.0f;
            sample = std::isfinite(normalized)
                ? juce::jlimit(-1.0f, 1.0f, normalized) : 0.0f;
        }
        outputRead = 0;
        outputWrite = frameSize;
    }

    void resetState() noexcept
    {
        if (state != nullptr)
        {
            rnnoise_destroy(state);
            state = rnnoise_create(nullptr);
            available.store(state != nullptr, std::memory_order_release);
        }
        inputCount = 0;
        outputRead = outputWrite = 0;
    }

    void release() noexcept
    {
        if (state != nullptr)
        {
            rnnoise_destroy(state);
            state = nullptr;
        }
        available.store(false, std::memory_order_release);
    }

    DenoiseState* state = nullptr;
    std::atomic<bool> enabled { false };
    std::atomic<bool> available { false };
    double sampleRate = 48000.0;
    int maxBlock = 512;
    int inputCount = 0;
    int outputRead = 0;
    int outputWrite = 0;
    bool enabledWas = false;
    std::array<float, frameSize> frameInput {};
    std::array<float, frameSize> frameOutput {};
};
