#pragma once
#include <JuceHeader.h>

struct PitchResult
{
    float frequencyHz = 0.0f;
    float confidence = 0.0f;
    bool voiced = false;
};

class PitchDetector
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    PitchResult process(const float* samples, int numSamples);

private:
    double currentSampleRate = 44100.0;
    juce::AudioBuffer<float> analysisBuffer;
    int writePosition = 0;
    int windowSize = 4096;
    float minFrequency = 65.0f;
    float maxFrequency = 1000.0f;
};
