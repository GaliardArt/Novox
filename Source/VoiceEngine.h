#pragma once
#include <JuceHeader.h>
#include "PitchDetector.h"

class VoiceEngine final : public juce::AudioIODeviceCallback
{
public:
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
        int numInputChannels, float* const* outputChannelData, int numOutputChannels,
        int numSamples, const juce::AudioIODeviceCallbackContext&) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    PitchResult getLatestPitch() const noexcept;
    void setInputGain(float gain) noexcept { inputGain.store(juce::jlimit(0.1f, 8.0f, gain)); }

private:
    PitchDetector detector;
    std::atomic<float> latestFrequency { 0.0f };
    std::atomic<float> latestConfidence { 0.0f };
    std::atomic<float> latestRms { 0.0f };
    std::atomic<bool> latestVoiced { false };
    std::atomic<float> inputGain { 1.0f };
    juce::AudioBuffer<float> monoScratch;
};
