#include "PitchDetector.h"
#include <algorithm>
#include <cmath>

void PitchDetector::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate * 0.5 : 22050.0;
    inputRing.fill(0.0f);
    analysisFrame.fill(0.0f);
    difference.fill(0.0f);
    cumulativeMeanDifference.fill(1.0f);
    writePosition = 0;
    samplesReceived = 0;
}

PitchResult PitchDetector::process(const float* samples, int numSamples)
{
    PitchResult result;
    if (samples == nullptr || numSamples <= 0)
        return result;

    for (int i = 0; i < numSamples; ++i)
    {
        inputRing[static_cast<size_t>(writePosition)] = samples[i];
        writePosition = (writePosition + 1) % inputWindowSize;
        samplesReceived = std::min(inputWindowSize, samplesReceived + 1);
    }

    if (samplesReceived < inputWindowSize)
        return result;

    // Downsample by two. The effective analysis sample rate is half the device rate.
    double energy = 0.0;
    for (int i = 0; i < analysisWindowSize; ++i)
    {
        const int sourceIndex = (writePosition + (i * 2)) % inputWindowSize;
        const float value = inputRing[static_cast<size_t>(sourceIndex)];
        analysisFrame[static_cast<size_t>(i)] = value;
        energy += static_cast<double>(value) * value;
    }

    result.rms = static_cast<float>(std::sqrt(energy / analysisWindowSize));
    if (result.rms < silenceThreshold)
        return result;

    const int minLag = std::max(2, static_cast<int>(currentSampleRate / maxFrequency));
    const int maxLag = std::min(maxLagCapacity, static_cast<int>(currentSampleRate / minFrequency));
    const int comparisonLength = analysisWindowSize - maxLag;

    difference[0] = 0.0f;
    cumulativeMeanDifference[0] = 1.0f;

    for (int tau = 1; tau <= maxLag; ++tau)
    {
        double sum = 0.0;
        for (int j = 0; j < comparisonLength; ++j)
        {
            const float delta = analysisFrame[static_cast<size_t>(j)]
                              - analysisFrame[static_cast<size_t>(j + tau)];
            sum += static_cast<double>(delta) * delta;
        }
        difference[static_cast<size_t>(tau)] = static_cast<float>(sum);
    }

    double runningSum = 0.0;
    for (int tau = 1; tau <= maxLag; ++tau)
    {
        runningSum += difference[static_cast<size_t>(tau)];
        cumulativeMeanDifference[static_cast<size_t>(tau)] =
            runningSum > 1.0e-12
                ? static_cast<float>(difference[static_cast<size_t>(tau)] * tau / runningSum)
                : 1.0f;
    }

    int selectedLag = -1;
    for (int tau = minLag; tau <= maxLag; ++tau)
    {
        if (cumulativeMeanDifference[static_cast<size_t>(tau)] < yinThreshold)
        {
            while (tau + 1 <= maxLag
                && cumulativeMeanDifference[static_cast<size_t>(tau + 1)]
                    < cumulativeMeanDifference[static_cast<size_t>(tau)])
                ++tau;
            selectedLag = tau;
            break;
        }
    }

    if (selectedLag < 0)
    {
        float bestValue = yinThreshold;
        for (int tau = minLag; tau <= maxLag; ++tau)
        {
            const float value = cumulativeMeanDifference[static_cast<size_t>(tau)];
            if (value < bestValue)
            {
                bestValue = value;
                selectedLag = tau;
            }
        }
    }

    if (selectedLag < 0)
        return result;

    float refinedLag = static_cast<float>(selectedLag);
    if (selectedLag > minLag && selectedLag < maxLag)
    {
        const float left = cumulativeMeanDifference[static_cast<size_t>(selectedLag - 1)];
        const float centre = cumulativeMeanDifference[static_cast<size_t>(selectedLag)];
        const float right = cumulativeMeanDifference[static_cast<size_t>(selectedLag + 1)];
        const float denominator = left - 2.0f * centre + right;
        if (std::abs(denominator) > 1.0e-9f)
            refinedLag += 0.5f * (left - right) / denominator;
    }

    result.confidence = std::clamp(
        1.0f - cumulativeMeanDifference[static_cast<size_t>(selectedLag)], 0.0f, 1.0f);
    result.frequencyHz = static_cast<float>(currentSampleRate / refinedLag);
    result.voiced = result.confidence >= 0.80f
                 && result.frequencyHz >= minFrequency
                 && result.frequencyHz <= maxFrequency;
    if (!result.voiced)
        result.frequencyHz = 0.0f;

    return result;
}
