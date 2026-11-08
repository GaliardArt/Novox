#include "VoiceEngine.h"
#include "PitchBend.h"
#include <algorithm>
#include <cmath>

VoiceEngine::VoiceEngine() : juce::Thread("NOVOX Pitch Worker") {}

VoiceEngine::~VoiceEngine()
{
    signalThreadShouldExit();
    stopThread(-1);
}

void VoiceEngine::setTransitionSmoothing(bool enabled, float confirmationMs) noexcept
{
    transitionSmoothingEnabled.store(enabled, std::memory_order_relaxed);
    transitionConfirmationMs.store(juce::jlimit(30.0f, 120.0f, confirmationMs), std::memory_order_relaxed);
}

void VoiceEngine::setAutoTune(bool enabled, int tonic, int scale) noexcept
{
    autoTuneEnabled.store(enabled, std::memory_order_relaxed);
    autoTuneTonic.store(juce::jlimit(0, 11, tonic), std::memory_order_relaxed);
    autoTuneScale.store(juce::jlimit(0, 2, scale), std::memory_order_relaxed);
}

void VoiceEngine::setPitchStabilityGuard(bool enabled, float toleranceCents, float holdMs) noexcept
{
    pitchStabilityEnabled.store(enabled, std::memory_order_relaxed);
    pitchDropToleranceCents.store(juce::jlimit(10.0f, 150.0f, toleranceCents), std::memory_order_relaxed);
    pitchDropHoldMs.store(juce::jlimit(10.0f, 80.0f, holdMs), std::memory_order_relaxed);
}

void VoiceEngine::setMidiNoteCleanup(bool enabled, float minimumDurationMs, float minimumConfidencePercent, float gapBridgeMs) noexcept
{
    midiNoteCleanupEnabled.store(enabled, std::memory_order_relaxed);
    midiNoteMinimumDurationMs.store(juce::jlimit(20.0f, 120.0f, minimumDurationMs), std::memory_order_relaxed);
    midiNoteMinimumConfidence.store(juce::jlimit(0.0f, 100.0f, minimumConfidencePercent), std::memory_order_relaxed);
    midiNoteGapBridgeMs.store(juce::jlimit(0.0f, 80.0f, gapBridgeMs), std::memory_order_relaxed);
}

void VoiceEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    signalThreadShouldExit();
    stopThread(-1);
    currentSampleRate = device != nullptr ? juce::jmax(1.0, device->getCurrentSampleRate()) : 44100.0;
    const auto deviceBlockSize = device != nullptr ? device->getCurrentBufferSizeSamples() : 512;
    detector.prepare(currentSampleRate, deviceBlockSize);
    noiseReducer.prepare(currentSampleRate, juce::jlimit(128, maxWorkerBlockSize, deviceBlockSize));
    inputEqualizer.prepare(currentSampleRate);
    monoScratch.setSize(1, juce::jmax(maxWorkerBlockSize, deviceBlockSize));
    workerBlockSize = juce::jlimit(128, maxWorkerBlockSize, deviceBlockSize);
    sampleFifo.reset();
    droppedAudioSamples.store(0);
    inputChannelSelector.reset();
    noteTracker.reset();
    pitchSmoother.reset();
    pitchStabilityGuard.reset();
    midiNoteCleanup.reset();
    lastPitchBendValue = PitchBend::center;
    latestActiveNote.store(-1, std::memory_order_relaxed);
    publishPitch(PitchResult {});
    startThread(juce::Thread::Priority::normal);
}

void VoiceEngine::audioDeviceStopped()
{
    signalThreadShouldExit();
    notify();
    stopThread(-1);
    sampleFifo.reset();
    monoScratch.setSize(0, 0);
    resetTrackingAndMidi();
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
    const auto mode = static_cast<InputChannelMode>(inputChannelMode.load(std::memory_order_relaxed));
    const float callbackDuration = static_cast<float>(static_cast<double>(numSamples) / currentSampleRate);
    const int selected = inputChannelSelector.process(mode, input, numInputs, numSamples, callbackDuration);
    activeInputChannel.store(selected, std::memory_order_relaxed);

    double energy = 0.0;
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;
        if (mode == InputChannelMode::stereoMix || selected < 0)
        {
            int count = 0;
            if (input != nullptr)
                for (int channel = 0; channel < numInputs; ++channel)
                    if (input[channel] != nullptr)
                    {
                        sample += input[channel][i];
                        ++count;
                    }
            if (count > 0)
                sample /= static_cast<float>(count);
        }
        else if (input != nullptr && selected < numInputs && input[selected] != nullptr)
        {
            sample = input[selected][i];
        }

        sample *= gain;
        mono[i] = sample;
        energy += static_cast<double>(sample) * sample;
        peak = juce::jmax(peak, std::abs(sample));
    }

    latestInputRms.store(static_cast<float>(std::sqrt(energy / static_cast<double>(numSamples))),
                         std::memory_order_relaxed);
    latestInputPeak.store(peak, std::memory_order_relaxed);
    latestInputClipping.store(peak >= 0.98f, std::memory_order_relaxed);

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    sampleFifo.prepareToWrite(numSamples, start1, size1, start2, size2);
    const int writable = size1 + size2;
    if (size1 > 0)
        std::copy_n(mono, size1, sampleBuffer.data() + start1);
    if (size2 > 0)
        std::copy_n(mono + size1, size2, sampleBuffer.data() + start2);
    sampleFifo.finishedWrite(writable);
    if (writable < numSamples)
        droppedAudioSamples.fetch_add(static_cast<uint64_t>(numSamples - writable), std::memory_order_relaxed);
}

void VoiceEngine::run()
{
    uint64_t lastObservedDroppedSamples = 0;
    while (!threadShouldExit())
    {
        if (resetRequested.exchange(false, std::memory_order_acq_rel))
            resetTrackingAndMidi();

        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        const int requested = juce::jmin(sampleFifo.getNumReady(), workerBlockSize);
        if (requested <= 0)
        {
            wait(1);
            continue;
        }

        sampleFifo.prepareToRead(requested, start1, size1, start2, size2);
        const int readable = size1 + size2;
        if (size1 > 0)
            std::copy_n(sampleBuffer.data() + start1, size1, workerScratch.data());
        if (size2 > 0)
            std::copy_n(sampleBuffer.data() + start2, size2, workerScratch.data() + size1);
        sampleFifo.finishedRead(readable);

        if (readable <= 0)
            continue;

        const auto totalDroppedSamples = droppedAudioSamples.load(std::memory_order_relaxed);
        const auto droppedSinceLastBlock = totalDroppedSamples >= lastObservedDroppedSamples
            ? totalDroppedSamples - lastObservedDroppedSamples : totalDroppedSamples;
        lastObservedDroppedSamples = totalDroppedSamples;
        const auto elapsedSamples = static_cast<uint64_t>(readable) + droppedSinceLastBlock;
        const float elapsedSeconds = static_cast<float>(static_cast<double>(elapsedSamples) / currentSampleRate);
        inputEqualizer.process(workerScratch.data(), readable);
        noiseReducer.process(workerScratch.data(), readable);
        const auto rawPitch = detector.process(workerScratch.data(), readable);

        noteTracker.setTransitionSmoothing(
            transitionSmoothingEnabled.load(std::memory_order_relaxed),
            transitionConfirmationMs.load(std::memory_order_relaxed));
        const bool tuneEnabled = autoTuneEnabled.load(std::memory_order_relaxed);
        noteTracker.setAutoTune(tuneEnabled,
            autoTuneTonic.load(std::memory_order_relaxed),
            static_cast<MusicalScale>(autoTuneScale.load(std::memory_order_relaxed)));
        pitchStabilityGuard.configure(
            pitchStabilityEnabled.load(std::memory_order_relaxed),
            pitchDropToleranceCents.load(std::memory_order_relaxed),
            pitchDropHoldMs.load(std::memory_order_relaxed));

        const auto guardedPitch = pitchStabilityGuard.process(rawPitch, elapsedSeconds);
        const auto cleanupEnabled = midiNoteCleanupEnabled.load(std::memory_order_relaxed);
        midiNoteCleanup.configure(cleanupEnabled,
            midiNoteMinimumDurationMs.load(std::memory_order_relaxed),
            midiNoteMinimumConfidence.load(std::memory_order_relaxed),
            midiNoteGapBridgeMs.load(std::memory_order_relaxed));
        noteTracker.setOnsetConfirmationMs(cleanupEnabled ? 0.0f : 30.0f);
        const auto pitch = midiNoteCleanup.process(guardedPitch, elapsedSeconds);
        const auto noteUpdate = noteTracker.process(pitch, elapsedSeconds);

        if (noteUpdate.noteOff >= 0)
        {
            if (midiRouter != nullptr)
            {
                if (lastPitchBendValue != PitchBend::center)
                    midiRouter->pitchBend(PitchBend::center);
                midiRouter->noteOff(noteUpdate.noteOff);
            }
            lastPitchBendValue = PitchBend::center;
        }

        if (noteUpdate.noteOn >= 0)
        {
            pitchSmoother.reset();
            if (midiRouter != nullptr)
                midiRouter->noteOn(noteUpdate.noteOn);
            lastPitchBendValue = PitchBend::center;
        }

        const int activeNote = noteTracker.getActiveNote();
        const bool bendEnabled = pitchBendEnabled.load(std::memory_order_relaxed);
        if (activeNote < 0 || !pitch.voiced || !bendEnabled || tuneEnabled)
        {
            if (lastPitchBendValue != PitchBend::center && midiRouter != nullptr)
                midiRouter->pitchBend(PitchBend::center);
            lastPitchBendValue = PitchBend::center;
        }
        else
        {
            const float rawOffset = PitchBend::semitoneOffset(pitch.frequencyHz, activeNote);
            const float smoothedOffset = pitchSmoother.process(
                rawOffset, pitchSmoothing.load(std::memory_order_relaxed), elapsedSeconds);
            const int bendValue = PitchBend::semitonesTo14BitValue(
                smoothedOffset, pitchBendRange.load(std::memory_order_relaxed));
            if (std::abs(bendValue - lastPitchBendValue) >= 2)
            {
                if (midiRouter != nullptr)
                    midiRouter->pitchBend(bendValue);
                lastPitchBendValue = bendValue;
            }
        }

        latestActiveNote.store(activeNote, std::memory_order_relaxed);
        publishPitch(pitch);
    }

    resetTrackingAndMidi();
}

void VoiceEngine::resetTrackingAndMidi() noexcept
{
    const int activeNote = noteTracker.getActiveNote();
    if (midiRouter != nullptr)
    {
        if (lastPitchBendValue != PitchBend::center)
            midiRouter->pitchBend(PitchBend::center);
        if (activeNote >= 0)
            midiRouter->noteOff(activeNote);
    }

    noteTracker.reset();
    pitchSmoother.reset();
    pitchStabilityGuard.reset();
    midiNoteCleanup.reset();
    lastPitchBendValue = PitchBend::center;
    latestActiveNote.store(-1, std::memory_order_relaxed);
}

void VoiceEngine::publishPitch(const PitchResult& result) noexcept
{
    latestSequence.fetch_add(1, std::memory_order_acq_rel);
    latestFrequency.store(result.frequencyHz, std::memory_order_relaxed);
    latestConfidence.store(result.confidence, std::memory_order_relaxed);
    latestRms.store(result.rms, std::memory_order_relaxed);
    latestAnalysisWindowSamples.store(result.analysisWindowSamples, std::memory_order_relaxed);
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
        result.analysisWindowSamples = latestAnalysisWindowSamples.load(std::memory_order_relaxed);
        result.voiced = latestVoiced.load(std::memory_order_relaxed);
        const auto after = latestSequence.load(std::memory_order_acquire);
        if (before == after)
        {
            result.sequence = after / 2U;
            return result;
        }
    }
}
