#include "PitchDetector.h"
#include <cmath>
#include <algorithm>
#include <array>

void PitchDetector::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    analysisBuffer.setSize(1, windowSize);
    analysisBuffer.clear();
    writePosition = 0;
}

PitchResult PitchDetector::process(const float* samples, int numSamples)
{
    PitchResult result;
    if (samples == nullptr || numSamples <= 0 || analysisBuffer.getNumSamples() == 0)
        return result;

    auto* dst = analysisBuffer.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i)
    {
        dst[writePosition] = samples[i];
        writePosition = (writePosition + 1) % windowSize;
    }

    std::array<float, 4096> frame {};
    for (int i = 0; i < windowSize; ++i)
        frame[static_cast<size_t>(i)] = dst[(writePosition + i) % windowSize];

    double energy = 0.0;
    for (int i = 0; i < windowSize; ++i)
        energy += static_cast<double>(frame[static_cast<size_t>(i)]) * frame[static_cast<size_t>(i)];

    const float rms = static_cast<float>(std::sqrt(energy / windowSize));
    if (rms < 0.008f)
        return result;

    const int minLag = juce::jmax(2, static_cast<int>(currentSampleRate / maxFrequency));
    const int maxLag = juce::jmin(windowSize / 2, static_cast<int>(currentSampleRate / minFrequency));
    float bestCorrelation = 0.0f;
    int bestLag = 0;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double cross = 0.0, e1 = 0.0, e2 = 0.0;
        const int limit = windowSize - lag;
        for (int i = 0; i < limit; ++i)
        {
            const float a = frame[static_cast<size_t>(i)];
            const float b = frame[static_cast<size_t>(i + lag)];
            cross += static_cast<double>(a) * b;
            e1 += static_cast<double>(a) * a;
            e2 += static_cast<double>(b) * b;
        }
        const float corr = (e1 > 1.0e-12 && e2 > 1.0e-12)
            ? static_cast<float>(cross / std::sqrt(e1 * e2)) : 0.0f;
        if (corr > bestCorrelation)
        {
            bestCorrelation = corr;
            bestLag = lag;
        }
    }

    result.confidence = bestCorrelation;
    result.voiced = bestLag > 0 && bestCorrelation > 0.72f;
    if (result.voiced)
        result.frequencyHz = static_cast<float>(currentSampleRate / bestLag);

    return result;
}
