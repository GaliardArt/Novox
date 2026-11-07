#include "VoiceEngine.h"

void VoiceEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const auto sr = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
    const auto bs = device != nullptr ? device->getCurrentBufferSizeSamples() : 512;
    detector.prepare(sr, bs);
    monoScratch.setSize(1, juce::jmax(512, bs));
    latestFrequency.store(0.0f);
    latestConfidence.store(0.0f);
    latestRms.store(0.0f);
    latestVoiced.store(false);
}

void VoiceEngine::audioDeviceStopped()
{
    monoScratch.setSize(0, 0);
    latestVoiced.store(false);
}

void VoiceEngine::audioDeviceIOCallbackWithContext(const float* const* input,
    int numInputs, float* const* output, int numOutputs, int numSamples,
    const juce::AudioIODeviceCallbackContext&)
{
    juce::ignoreUnused(output, numOutputs);
    if (numInputs <= 0 || input == nullptr || input[0] == nullptr
        || monoScratch.getNumSamples() < numSamples)
        return;

    auto* mono = monoScratch.getWritePointer(0);
    const float gain = inputGain.load();
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;
        int channelsRead = 0;
        for (int channel = 0; channel < numInputs; ++channel)
        {
            if (input[channel] != nullptr)
            {
                sample += input[channel][i];
                ++channelsRead;
            }
        }
        mono[i] = channelsRead > 0 ? (sample / static_cast<float>(channelsRead)) * gain : 0.0f;
    }

    const auto result = detector.process(mono, numSamples);
    latestFrequency.store(result.frequencyHz);
    latestConfidence.store(result.confidence);
    latestRms.store(result.rms);
    latestVoiced.store(result.voiced);
}

PitchResult VoiceEngine::getLatestPitch() const noexcept
{
    PitchResult result;
    result.frequencyHz = latestFrequency.load();
    result.confidence = latestConfidence.load();
    result.rms = latestRms.load();
    result.voiced = latestVoiced.load();
    return result;
}
