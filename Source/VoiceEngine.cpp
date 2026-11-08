#include "VoiceEngine.h"
#include <algorithm>

VoiceEngine::VoiceEngine() : juce::Thread("NOVOX Pitch Worker") {}

VoiceEngine::~VoiceEngine()
{
    signalThreadShouldExit();
    stopThread(-1);
}

void VoiceEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    signalThreadShouldExit();
    stopThread(-1);
    const auto sampleRate = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
    const auto deviceBlockSize = device != nullptr ? device->getCurrentBufferSizeSamples() : 512;
    detector.prepare(sampleRate, deviceBlockSize);
    monoScratch.setSize(1, juce::jmax(maxWorkerBlockSize, deviceBlockSize));
    workerBlockSize = juce::jlimit(128, maxWorkerBlockSize, deviceBlockSize);
    sampleFifo.reset();
    droppedAudioSamples.store(0);
    publishPitch(PitchResult {});
    startThread(juce::Thread::Priority::normal);
}

void VoiceEngine::audioDeviceStopped()
{
    signalThreadShouldExit();
    stopThread(-1);
    sampleFifo.reset();
    monoScratch.setSize(0, 0);
    publishPitch(PitchResult {});
}

void VoiceEngine::audioDeviceIOCallbackWithContext(const float* const* input,
    int numInputs, float* const* output, int numOutputs, int numSamples,
    const juce::AudioIODeviceCallbackContext&)
{
    juce::ignoreUnused(output, numOutputs);
    if (numSamples <= 0)
        return;
    if (monoScratch.getNumSamples() < numSamples)
    {
        droppedAudioSamples.fetch_add(static_cast<uint64_t>(numSamples), std::memory_order_relaxed);
        return;
    }

    auto* mono = monoScratch.getWritePointer(0);
    const float gain = inputGain.load(std::memory_order_relaxed);
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;
        int channelsRead = 0;
        if (input != nullptr)
        {
            for (int channel = 0; channel < numInputs; ++channel)
            {
                if (input[channel] != nullptr)
                {
                    sample += input[channel][i];
                    ++channelsRead;
                }
            }
        }
        mono[i] = channelsRead > 0 ? (sample / static_cast<float>(channelsRead)) * gain : 0.0f;
    }

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    sampleFifo.prepareToWrite(numSamples, start1, size1, start2, size2);
    const int writable = size1 + size2;
    if (size1 > 0) std::copy_n(mono, size1, sampleBuffer.data() + start1);
    if (size2 > 0) std::copy_n(mono + size1, size2, sampleBuffer.data() + start2);
    sampleFifo.finishedWrite(writable);
    if (writable < numSamples)
        droppedAudioSamples.fetch_add(static_cast<uint64_t>(numSamples - writable), std::memory_order_relaxed);
}

void VoiceEngine::run()
{
    while (!threadShouldExit())
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        const int requested = juce::jmin(sampleFifo.getNumReady(), workerBlockSize);
        if (requested <= 0)
        {
            wait(1);
            continue;
        }

        sampleFifo.prepareToRead(requested, start1, size1, start2, size2);
        const int readable = size1 + size2;
        if (size1 > 0) std::copy_n(sampleBuffer.data() + start1, size1, workerScratch.data());
        if (size2 > 0) std::copy_n(sampleBuffer.data() + start2, size2, workerScratch.data() + size1);
        sampleFifo.finishedRead(readable);

        if (readable > 0)
        {
            publishPitch(detector.process(workerScratch.data(), readable));
        }
    }
}

void VoiceEngine::publishPitch(const PitchResult& result) noexcept
{
    latestSequence.fetch_add(1, std::memory_order_acq_rel);
    latestFrequency.store(result.frequencyHz, std::memory_order_relaxed);
    latestConfidence.store(result.confidence, std::memory_order_relaxed);
    latestRms.store(result.rms, std::memory_order_relaxed);
    latestVoiced.store(result.voiced, std::memory_order_relaxed);
    latestSequence.fetch_add(1, std::memory_order_release);
}

PitchResult VoiceEngine::getLatestPitch() const noexcept
{
    PitchResult result;
    for (;;)
    {
        const auto before = latestSequence.load(std::memory_order_acquire);
        if ((before & 1U) != 0U)
            continue;
        result.frequencyHz = latestFrequency.load(std::memory_order_relaxed);
        result.confidence = latestConfidence.load(std::memory_order_relaxed);
        result.rms = latestRms.load(std::memory_order_relaxed);
        result.voiced = latestVoiced.load(std::memory_order_relaxed);
        const auto after = latestSequence.load(std::memory_order_acquire);
        if (before == after)
        {
            result.sequence = after / 2U;
            return result;
        }
    }
}

