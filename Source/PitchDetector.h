#pragma once
#include <array>
#include <atomic>
#include <cstdint>

struct PitchResult
{
    float frequencyHz = 0.0f;
    float confidence = 0.0f;
    float rms = 0.0f;
    bool voiced = false;
    uint64_t sequence = 0;
    int analysisWindowSamples = 0;
};

class PitchDetector
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    PitchResult process(const float* samples, int numSamples);
    void setSensitivity(float sensitivity) noexcept
    {
        sensitivityValue.store(sensitivity < 0.0f ? 0.0f : (sensitivity > 1.0f ? 1.0f : sensitivity),
                               std::memory_order_relaxed);
    }

private:
    static constexpr int maxAnalysisWindowSize = 2048;
    static constexpr int shortAnalysisWindowSize = 1024;
    static constexpr int maxLagCapacity = 768;
    static constexpr int filterHistorySize = 31;

    // Symmetric 31-tap Blackman-windowed low-pass FIR, cutoff at 0.22 cycles/sample.
    // It is applied at the input rate before the 2:1 decimator.
    static constexpr std::array<float, filterHistorySize> antiAliasCoefficients {
        0.0f, 0.000043922f, -0.000316162f, -0.000821907f, 0.001074459f,
        0.003935576f, -0.000889983f, -0.011495698f, -0.004455831f,
        0.024471572f, 0.023574793f, -0.040649856f, -0.076080580f,
        0.054506457f, 0.307098913f, 0.440008648f, 0.307098913f,
        0.054506457f, -0.076080580f, -0.040649856f, 0.023574793f,
        0.024471572f, -0.004455831f, -0.011495698f, -0.000889983f,
        0.003935576f, 0.001074459f, -0.000821907f, -0.000316162f,
        0.000043922f, 0.0f
    };

    double currentSampleRate = 44100.0;
    std::array<float, maxAnalysisWindowSize> decimatedRing {};
    std::array<float, maxAnalysisWindowSize> analysisFrame {};
    std::array<float, filterHistorySize> filterHistory {};
    std::array<float, maxLagCapacity + 1> difference {};
    std::array<float, maxLagCapacity + 1> cumulativeMeanDifference {};
    int currentAnalysisWindowSize = maxAnalysisWindowSize;
    float lastDetectedFrequency = 0.0f;
    int decimatedWritePosition = 0;
    int decimatedSamplesReceived = 0;
    int decimationPhase = 0;
    int filterHistoryCount = 0;
    float minFrequency = 65.0f;
    float maxFrequency = 1000.0f;
    std::atomic<float> sensitivityValue { 0.5f };
};
