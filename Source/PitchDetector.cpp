#include "PitchDetector.h"
#include <algorithm>
#include <cmath>

void PitchDetector::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate * 0.5 : 22050.0;
    currentAnalysisWindowSize = maxAnalysisWindowSize;
    lastDetectedFrequency = 0.0f;
    decimatedRing.fill(0.0f);
    analysisFrame.fill(0.0f);
    filterHistory.fill(0.0f);
    difference.fill(0.0f);
    cumulativeMeanDifference.fill(1.0f);
    decimatedWritePosition = 0;
    decimatedSamplesReceived = 0;
    decimationPhase = 0;
    filterHistoryCount = 0;
}

PitchResult PitchDetector::process(const float* samples, int numSamples)
{
    PitchResult result;
    if (samples == nullptr || numSamples <= 0)
        return result;

    // Filter at the device sample rate before decimating by two. This prevents
    // content above the new Nyquist frequency from folding into the pitch band.
    for (int i = 0; i < numSamples; ++i)
    {
        for (int tap = filterHistorySize - 1; tap > 0; --tap)
            filterHistory[static_cast<size_t>(tap)] = filterHistory[static_cast<size_t>(tap - 1)];
        filterHistory[0] = samples[i];
        filterHistoryCount = std::min(filterHistorySize, filterHistoryCount + 1);

        double filtered = 0.0;
        for (int tap = 0; tap < filterHistorySize; ++tap)
            filtered += static_cast<double>(antiAliasCoefficients[static_cast<size_t>(tap)])
                      * filterHistory[static_cast<size_t>(tap)];

        if (decimationPhase == 0)
        {
            decimatedRing[static_cast<size_t>(decimatedWritePosition)] = static_cast<float>(filtered);
            decimatedWritePosition = (decimatedWritePosition + 1) % maxAnalysisWindowSize;
            decimatedSamplesReceived = std::min(maxAnalysisWindowSize, decimatedSamplesReceived + 1);
        }
        decimationPhase ^= 1;
    }

    currentAnalysisWindowSize = (lastDetectedFrequency > 180.0f) ? shortAnalysisWindowSize : maxAnalysisWindowSize;
    const int analysisWindowSize = currentAnalysisWindowSize;
    result.analysisWindowSamples = analysisWindowSize;
    if (decimatedSamplesReceived < analysisWindowSize)
        return result;

    const float sensitivity = sensitivityValue.load(std::memory_order_relaxed);
    const float silenceThreshold = 0.002f + 0.012f * (1.0f - sensitivity);
    const float confidenceThreshold = 0.68f + 0.24f * (1.0f - sensitivity);
    const float yinThreshold = 0.12f + 0.06f * sensitivity;

    double energy = 0.0;
    for (int i = 0; i < analysisWindowSize; ++i)
    {
        const int sourceIndex = (decimatedWritePosition + maxAnalysisWindowSize - analysisWindowSize + i) % maxAnalysisWindowSize;
        const float value = decimatedRing[static_cast<size_t>(sourceIndex)];
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

    // YIN may select the second harmonic when the fundamental is weak.
    // Check the actual spectral energy at half the selected frequency before
    // correcting down an octave. A Hann-windowed projection reduces leakage,
    // while the relative threshold keeps clean single-frequency tones intact.
    const int lowerOctaveLag = selectedLag * 2;
    if (lowerOctaveLag <= maxLag)
    {
        const auto estimateAmplitude = [this, analysisWindowSize](double frequency)
        {
            double real = 0.0;
            double imaginary = 0.0;
            double windowSum = 0.0;
            for (int i = 0; i < analysisWindowSize; ++i)
            {
                const double window = 0.5 - 0.5 * std::cos(
                    2.0 * 3.14159265358979323846 * i / (analysisWindowSize - 1));
                const double phase = 2.0 * 3.14159265358979323846
                                   * frequency * i / currentSampleRate;
                const double value = static_cast<double>(analysisFrame[static_cast<size_t>(i)]) * window;
                real += value * std::cos(phase);
                imaginary -= value * std::sin(phase);
                windowSum += window;
            }
            return 2.0 * std::sqrt(real * real + imaginary * imaginary) / windowSum;
        };

        const double selectedFrequency = currentSampleRate / static_cast<double>(selectedLag);
        const double lowerFrequency = currentSampleRate / static_cast<double>(lowerOctaveLag);
        const double selectedAmplitude = estimateAmplitude(selectedFrequency);
        const double lowerAmplitude = estimateAmplitude(lowerFrequency);

        if (lowerAmplitude > std::max(0.001, selectedAmplitude * 0.03))
            selectedLag = lowerOctaveLag;
    }

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
    result.voiced = result.confidence >= confidenceThreshold
                 && result.frequencyHz >= minFrequency
                 && result.frequencyHz <= maxFrequency;
    if (!result.voiced)
        result.frequencyHz = 0.0f;
    else
        lastDetectedFrequency = result.frequencyHz;

    return result;
}
