#include "VocalPercussionDetector.h"

#include <algorithm>
#include <cmath>

void VocalPercussionDetector::prepare(double sampleRate) noexcept
{
    currentSampleRate = sampleRate > 1000.0 ? sampleRate : 48000.0;
    reset();
}

void VocalPercussionDetector::reset() noexcept
{
    frame.fill(0.0f);
    framePosition = 0;
    previousRms = 0.0f;
    samplesSinceHit = static_cast<int>(currentSampleRate * 0.1);
    latest = {};
}

VocalPercussionResult VocalPercussionDetector::process(const float* samples, int numSamples) noexcept
{
    latest.type = VocalPercussionType::none;
    latest.confidence = 0.0f;
    if (samples == nullptr || numSamples <= 0)
        return latest;

    for (int i = 0; i < numSamples; ++i)
    {
        const float sample = std::isfinite(samples[i]) ? samples[i] : 0.0f;
        frame[static_cast<size_t>(framePosition++)] = sample;
        if (framePosition == analysisSize)
        {
            analyseFrame();
            framePosition = 0;
        }
        if (samplesSinceHit < static_cast<int>(currentSampleRate))
            ++samplesSinceHit;
    }
    return latest;
}

void VocalPercussionDetector::analyseFrame() noexcept
{
    double energy = 0.0, firstHalfEnergy = 0.0, secondHalfEnergy = 0.0;
    double lowEnergy = 0.0, highEnergy = 0.0;
    int zeroCrossings = 0;
    float peak = 0.0f, previous = frame[0], low = previous;
    const float coefficient = static_cast<float>(
        1.0 - std::exp(-2.0 * 3.14159265358979323846 * 180.0 / currentSampleRate));

    for (int i = 0; i < analysisSize; ++i)
    {
        const float sample = std::isfinite(frame[static_cast<size_t>(i)])
            ? frame[static_cast<size_t>(i)] : 0.0f;
        low += coefficient * (sample - low);
        const float high = sample - low;
        const double sampleEnergy = static_cast<double>(sample) * sample;
        energy += sampleEnergy;
        if (i < analysisSize / 2) firstHalfEnergy += sampleEnergy;
        else secondHalfEnergy += sampleEnergy;
        lowEnergy += static_cast<double>(low) * low;
        highEnergy += static_cast<double>(high) * high;
        peak = std::max(peak, std::abs(sample));
        if ((sample >= 0.0f && previous < 0.0f)
            || (sample < 0.0f && previous >= 0.0f))
            ++zeroCrossings;
        previous = sample;
    }

    const float rms = static_cast<float>(std::sqrt(energy / analysisSize));
    const float lowRatio = energy > 1.0e-12 ? static_cast<float>(lowEnergy / energy) : 0.0f;
    const float highRatio = energy > 1.0e-12 ? static_cast<float>(highEnergy / energy) : 0.0f;
    const float zeroCrossingRate = static_cast<float>(zeroCrossings) / analysisSize;
    const float onsetRatio = rms / std::max(previousRms, 0.004f);
    const float crestFactor = rms > 1.0e-6f ? peak / rms : 0.0f;
    const float decayRatio = static_cast<float>(std::sqrt(firstHalfEnergy)
        / std::max(std::sqrt(secondHalfEnergy), 1.0e-5));

    previousRms = previousRms * 0.72f + rms * 0.28f;
    if (rms < 0.012f || onsetRatio < 1.20f
        || samplesSinceHit < static_cast<int>(currentSampleRate * 0.075))
        return;

    VocalPercussionType detected = VocalPercussionType::none;
    float confidence = 0.0f;
    if (lowRatio >= 0.58f && zeroCrossingRate <= 0.16f && decayRatio >= 1.15f)
    {
        detected = VocalPercussionType::kick;
        confidence = std::clamp(0.55f + (lowRatio - 0.58f) * 0.8f
                                + (decayRatio - 1.15f) * 0.08f, 0.0f, 0.98f);
    }
    else if (highRatio >= 0.68f && zeroCrossingRate >= 0.18f && decayRatio >= 1.10f)
    {
        detected = rms < 0.12f ? VocalPercussionType::hiHat : VocalPercussionType::snare;
        confidence = std::clamp(0.52f + (highRatio - 0.68f) * 0.7f
                                + std::min(0.2f, zeroCrossingRate), 0.0f, 0.95f);
    }
    else if (crestFactor >= 2.8f && rms >= 0.035f && decayRatio >= 1.10f)
    {
        detected = VocalPercussionType::snare;
        confidence = std::clamp(0.48f + (crestFactor - 2.8f) * 0.08f, 0.0f, 0.85f);
    }

    if (detected != VocalPercussionType::none)
    {
        ++sequence;
        latest.type = detected;
        latest.confidence = confidence;
        latest.rms = rms;
        latest.sequence = sequence;
        samplesSinceHit = 0;
    }
}
