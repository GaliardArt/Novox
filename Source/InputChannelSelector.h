#pragma once

#include <algorithm>

enum class InputChannelMode : int
{
    automatic = 0,
    left = 1,
    right = 2,
    stereoMix = 3
};

class InputChannelSelector
{
public:
    int process(InputChannelMode mode, const float* const* input, int channelCount,
                int samples, float elapsedSeconds) noexcept
    {
        if (input == nullptr || channelCount <= 0)
        {
            reset();
            return -1;
        }

        if (mode == InputChannelMode::stereoMix)
        {
            selectedChannel = -1;
            clearCandidate();
            return -1;
        }

        const int fallback = firstAvailable(input, channelCount);
        if (fallback < 0)
        {
            reset();
            return -1;
        }

        if (mode == InputChannelMode::left || mode == InputChannelMode::right)
        {
            const int requested = mode == InputChannelMode::left ? 0 : 1;
            selectedChannel = requested < channelCount && input[requested] != nullptr
                ? requested : fallback;
            clearCandidate();
            return selectedChannel;
        }

        if (samples <= 0)
            return selectedChannel >= 0 && selectedChannel < channelCount
                && input[selectedChannel] != nullptr ? selectedChannel : fallback;

        const int strongest = strongestChannel(input, channelCount, samples);
        if (strongest < 0)
            return fallback;

        if (selectedChannel < 0 || selectedChannel >= channelCount
            || input[selectedChannel] == nullptr)
        {
            selectedChannel = strongest;
            clearCandidate();
            return selectedChannel;
        }

        if (strongest == selectedChannel)
        {
            clearCandidate();
            return selectedChannel;
        }

        const double currentEnergy = channelEnergy(input[selectedChannel], samples);
        const double strongestEnergy = channelEnergy(input[strongest], samples);
        if (strongestEnergy <= currentEnergy * switchAdvantageRatio)
        {
            clearCandidate();
            return selectedChannel;
        }

        if (candidateChannel != strongest)
        {
            candidateChannel = strongest;
            candidateSeconds = 0.0f;
        }

        candidateSeconds += std::clamp(elapsedSeconds, 0.0f, 0.25f);
        if (candidateSeconds + 1.0e-6f >= switchConfirmationSeconds)
        {
            selectedChannel = strongest;
            clearCandidate();
        }

        return selectedChannel;
    }

    void reset() noexcept
    {
        selectedChannel = -1;
        clearCandidate();
    }

    int getSelectedChannel() const noexcept { return selectedChannel; }

private:
    static constexpr double switchAdvantageRatio = 1.5;
    static constexpr float switchConfirmationSeconds = 0.080f;

    int selectedChannel = -1;
    int candidateChannel = -1;
    float candidateSeconds = 0.0f;

    static int firstAvailable(const float* const* input, int channelCount) noexcept
    {
        for (int channel = 0; channel < channelCount; ++channel)
            if (input[channel] != nullptr)
                return channel;
        return -1;
    }

    static double channelEnergy(const float* samples, int sampleCount) noexcept
    {
        double energy = 0.0;
        for (int i = 0; i < sampleCount; ++i)
        {
            const double value = samples[i];
            energy += value * value;
        }
        return energy;
    }

    static int strongestChannel(const float* const* input, int channelCount,
                                int sampleCount) noexcept
    {
        int strongest = -1;
        double strongestEnergy = -1.0;
        for (int channel = 0; channel < channelCount; ++channel)
        {
            if (input[channel] == nullptr)
                continue;

            const double energy = channelEnergy(input[channel], sampleCount);
            if (energy > strongestEnergy)
            {
                strongestEnergy = energy;
                strongest = channel;
            }
        }
        return strongest;
    }

    void clearCandidate() noexcept
    {
        candidateChannel = -1;
        candidateSeconds = 0.0f;
    }
};
