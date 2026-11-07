#pragma once
#include <array>

struct PitchResult
{
    float frequencyHz = 0.0f;
    float confidence = 0.0f;
    float rms = 0.0f;
    bool voiced = false;
};

class PitchDetector
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    PitchResult process(const float* samples, int numSamples);

private:
    static constexpr int inputWindowSize = 2048;
    static constexpr int analysisWindowSize = inputWindowSize / 2;
    static constexpr int maxLagCapacity = 384;

    double currentSampleRate = 44100.0;
    std::array<float, inputWindowSize> inputRing {};
    std::array<float, analysisWindowSize> analysisFrame {};
    std::array<float, maxLagCapacity + 1> difference {};
    std::array<float, maxLagCapacity + 1> cumulativeMeanDifference {};
    int writePosition = 0;
    int samplesReceived = 0;
    float minFrequency = 65.0f;
    float maxFrequency = 1000.0f;
    float yinThreshold = 0.15f;
    float silenceThreshold = 0.008f;
};
