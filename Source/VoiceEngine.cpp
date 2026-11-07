#include "VoiceEngine.h"

void VoiceEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const auto sr = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
    const auto bs = device != nullptr ? device->getCurrentBufferSizeSamples() : 512;
    detector.prepare(sr, bs);
    monoScratch.setSize(1, juce::jmax(512, bs));
}

void VoiceEngine::audioDeviceStopped()
{
    monoScratch.setSize(0, 0);
}

void VoiceEngine::audioDeviceIOCallbackWithContext(const float* const* input,
    int numInputs, float* const* output, int numOutputs, int numSamples,
    const juce::AudioIODeviceCallbackContext&)
{
    juce::ignoreUnused(output, numOutputs);
    if (numInputs <= 0 || input == nullptr || input[0] == nullptr)
        return;

    auto* mono = monoScratch.getWritePointer(0);
    const float gain = inputGain.load();
    for (int i = 0; i < numSamples; ++i)
    {
        float s = 0.0f;
        for (int ch = 0; ch < numInputs; ++ch)
            if (input[ch] != nullptr) s += input[ch][i];
        mono[i] = (s / static_cast<float>(numInputs)) * gain;
    }

    const auto result = detector.process(mono, numSamples);
    const juce::ScopedLock lock(resultLock);
    latest = result;
}

PitchResult VoiceEngine::getLatestPitch() const
{
    const juce::ScopedLock lock(resultLock);
    return latest;
}
