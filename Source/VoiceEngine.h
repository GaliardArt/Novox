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

    PitchResult getLatestPitch() const;
    void setInputGain(float gain) { inputGain.store(juce::jlimit(0.1f, 8.0f, gain)); }

private:
    PitchDetector detector;
    mutable juce::CriticalSection resultLock;
    PitchResult latest;
    std::atomic<float> inputGain { 1.0f };
    juce::AudioBuffer<float> monoScratch;
};
