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
    lowLatencyStage = 0;
    lowLatencyFallbackActive = false;
    lastLowLatencyMode = lowLatencyMode.load(std::memory_order_relaxed);
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

    const bool lowLatency = lowLatencyMode.load(std::memory_order_relaxed);
    if (lowLatency != lastLowLatencyMode)
    {
        lowLatencyStage = 0;
        lowLatencyFallbackActive = false;
        lastLowLatencyMode = lowLatency;
    }
    if (lowLatency)
    {
        if (!lowLatencyFallbackActive)
        {
            if (lastDetectedFrequency >= 350.0f) lowLatencyStage = 0;
            else if (lastDetectedFrequency >= 180.0f) lowLatencyStage = 1;
            else if (lastDetectedFrequency >= 90.0f) lowLatencyStage = 2;
            else if (lastDetectedFrequency > 0.0f)
                lowLatencyStage = (currentSampleRate / lastDetectedFrequency > 512.0) ? 4 : 3;
        }
        lowLatencyStage = std::clamp(lowLatencyStage, 0, 4);
        currentAnalysisWindowSize = lowLatencyWindows[static_cast<size_t>(lowLatencyStage)];
    }
    else
        currentAnalysisWindowSize = (lastDetectedFrequency > 180.0f) ? shortAnalysisWindowSize : maxAnalysisWindowSize;
    const int analysisWindowSize = currentAnalysisWindowSize;
    result.analysisWindowSamples = analysisWindowSize;
    if (decimatedSamplesReceived < analysisWindowSize)
        return result;
    result.diagnosticState = 1;

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
    result.signalPresent = result.rms >= silenceThreshold;
    if (!result.signalPresent)
    {
        if (lowLatency)
        {
            lowLatencyStage = 0;
            lowLatencyFallbackActive = false;
            lastDetectedFrequency = 0.0f;
        }
        return result;
    }

    result.diagnosticState = 2;
    const int minLag = std::max(2, static_cast<int>(currentSampleRate / maxFrequency));
    const int maxLag = lowLatency
        ? std::min({ maxLagCapacity, static_cast<int>(currentSampleRate / minFrequency), analysisWindowSize / 2 })
        : std::min(maxLagCapacity, static_cast<int>(currentSampleRate / minFrequency));
    difference[0] = 0.0f;
    cumulativeMeanDifference[0] = 1.0f;

    for (int tau = 1; tau <= maxLag; ++tau)
    {
        // YIN's difference function must compare every valid sample pair for
        // this lag. A fixed (window - maxLag) length starves short lags of
        // data, which is especially damaging for high notes.
        const int comparisonLength = analysisWindowSize - tau;
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
        // A threshold crossing is preferred, but it must not be required:
        // voiced frames can have a valid periodic minimum slightly above the
        // early-exit threshold. Let the independent confidence gate decide
        // whether that minimum is reliable enough to emit a note.
        float bestValue = 1.0f;
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

    if (selectedLag < 0 || (lowLatency && lowLatencyStage < 4 && selectedLag >= maxLag - 2))
    {
        if (lowLatency && lowLatencyStage < 4)
        {
            ++lowLatencyStage;
            lowLatencyFallbackActive = true;
        }
        return result;
    }

    // YIN can lock onto a strong harmonic when the fundamental is weak.
    // Compare Hann-windowed sinusoidal projections before accepting a lower
    // subharmonic. Requiring support from multiple harmonics avoids lowering
    // clean single-tone inputs just because of spectral leakage.
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
    const double selectedAmplitude = estimateAmplitude(selectedFrequency);
    const int octaveLag = selectedLag * 2;
    if (octaveLag <= maxLag)
    {
        const double lowerFrequency = currentSampleRate / static_cast<double>(octaveLag);
        const double lowerAmplitude = estimateAmplitude(lowerFrequency);
        if (lowerAmplitude > std::max(0.0015, selectedAmplitude * 0.03))
        {
            selectedLag = octaveLag;
        }
    }

    // Also check a third-subharmonic candidate. This covers voices whose
    // second or third harmonic dominates the fundamental without assuming
    // that every strong partial is an octave relationship.
    const int thirdSubharmonicLag = selectedLag * 3;
    if (thirdSubharmonicLag <= maxLag)
    {
        const double candidateFrequency = currentSampleRate / static_cast<double>(thirdSubharmonicLag);
        const double fundamentalAmplitude = estimateAmplitude(candidateFrequency);
        const double secondHarmonicAmplitude = estimateAmplitude(candidateFrequency * 2.0);
        const double thirdHarmonicAmplitude = estimateAmplitude(candidateFrequency * 3.0);
        const double minimumSupport = std::max(0.0015, thirdHarmonicAmplitude * 0.025);

        if (fundamentalAmplitude > std::max(0.0015, thirdHarmonicAmplitude * 0.03)
            && secondHarmonicAmplitude > minimumSupport)
        {
            selectedLag = thirdSubharmonicLag;
        }
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
    result.candidateFrequencyHz = static_cast<float>(currentSampleRate / refinedLag);
    result.confidenceThreshold = confidenceThreshold;
    const bool confidencePassed = result.confidence >= confidenceThreshold;
    const bool frequencyInRange = result.candidateFrequencyHz >= minFrequency
                               && result.candidateFrequencyHz <= maxFrequency;
    result.voiced = confidencePassed && frequencyInRange;
    result.frequencyHz = result.voiced ? result.candidateFrequencyHz : 0.0f;
    result.diagnosticState = result.voiced ? 4 : (confidencePassed ? 5 : 3);
    if (!result.voiced)
    {
        if (lowLatency && lowLatencyStage < 4)
        {
            ++lowLatencyStage;
            lowLatencyFallbackActive = true;
        }
    }
    else
    {
        lastDetectedFrequency = result.frequencyHz;
        lowLatencyFallbackActive = false;
        if (lowLatency)
        {
            const double period = currentSampleRate / static_cast<double>(result.frequencyHz);
            lowLatencyStage = result.frequencyHz >= 350.0f ? 0
                : (result.frequencyHz >= 180.0f ? 1
                : (result.frequencyHz >= 90.0f ? 2 : (period > 512.0 ? 4 : 3)));
        }
    }

    return result;
}
