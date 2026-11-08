#pragma once
#include <JuceHeader.h>
#include "PitchDetector.h"
#include <array>

class VoiceEngine final : public juce::AudioIODeviceCallback,
                          private juce::Thread
{
public:
    VoiceEngine();
    ~VoiceEngine() override;

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
        int numInputChannels, float* const* outputChannelData, int numOutputChannels,
        int numSamples, const juce::AudioIODeviceCallbackContext&) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    PitchResult getLatestPitch() const noexcept;
    void setInputGain(float gain) noexcept { inputGain.store(juce::jlimit(0.1f, 8.0f, gain)); }
    uint64_t getDroppedAudioSamples() const noexcept { return droppedAudioSamples.load(); }

private:
    static constexpr int fifoCapacity = 8192;
    static constexpr int maxWorkerBlockSize = 2048;

    void run() override;
    void publishPitch(const PitchResult& result) noexcept;

    PitchDetector detector;
    juce::AbstractFifo sampleFifo { fifoCapacity };
    std::array<float, fifoCapacity> sampleBuffer {};
    juce::AudioBuffer<float> monoScratch;
    std::array<float, maxWorkerBlockSize> workerScratch {};
    int workerBlockSize = 512;

    std::atomic<float> latestFrequency { 0.0f };
    std::atomic<float> latestConfidence { 0.0f };
    std::atomic<float> latestRms { 0.0f };
    std::atomic<bool> latestVoiced { false };
    std::atomic<uint64_t> latestSequence { 0 };
    std::atomic<float> inputGain { 1.0f };
    std::atomic<uint64_t> droppedAudioSamples { 0 };
};
