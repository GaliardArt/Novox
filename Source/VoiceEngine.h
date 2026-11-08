#pragma once
#include <JuceHeader.h>
#include "PitchDetector.h"
#include "NoiseReducer.h"
#include "InputChannelSelector.h"
#include "MidiRouter.h"
#include "NoteTracker.h"
#include "PitchSmoother.h"
#include "PitchStabilityGuard.h"
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
    int getActiveNote() const noexcept { return latestActiveNote.load(std::memory_order_relaxed); }
    void setMidiRouter(MidiRouter* router) noexcept { midiRouter = router; }
    void requestReset() noexcept { resetRequested.store(true, std::memory_order_release); }

    void setInputGain(float gain) noexcept { inputGain.store(juce::jlimit(0.1f, 8.0f, gain)); }
    void setPitchSensitivity(float sensitivity) noexcept { detector.setSensitivity(sensitivity); }
    void setNoiseReductionEnabled(bool enabled) noexcept { noiseReducer.setEnabled(enabled); }
    void setNoiseReductionStrength(float strength) noexcept { noiseReducer.setStrength(strength); }
    void beginNoiseCalibration() noexcept { noiseReducer.beginCalibration(); }
    int getNoiseCalibrationState() const noexcept { return noiseReducer.getCalibrationState(); }
    float getCalibratedNoiseFloorDb() const noexcept { return noiseReducer.getCalibratedNoiseFloorDb(); }
    void setCalibratedNoiseFloorDb(float value) noexcept { noiseReducer.setCalibratedNoiseFloorDb(value); }
    void setInputChannelMode(int value) noexcept { inputChannelMode.store(juce::jlimit(0, 3, value), std::memory_order_relaxed); }
    int getActiveInputChannel() const noexcept { return activeInputChannel.load(std::memory_order_relaxed); }
    uint64_t getDroppedAudioSamples() const noexcept { return droppedAudioSamples.load(); }
    float getInputRms() const noexcept { return latestInputRms.load(std::memory_order_relaxed); }
    float getInputPeak() const noexcept { return latestInputPeak.load(std::memory_order_relaxed); }
    bool isInputClipping() const noexcept { return latestInputClipping.load(std::memory_order_relaxed); }

    void setTransitionSmoothing(bool enabled, float confirmationMs) noexcept;
    void setAutoTune(bool enabled, int tonic, int scale) noexcept;
    void setPitchStabilityGuard(bool enabled, float toleranceCents, float holdMs) noexcept;
    void setPitchBendEnabled(bool enabled) noexcept { pitchBendEnabled.store(enabled, std::memory_order_relaxed); }
    void setPitchBendRange(int semitones) noexcept { pitchBendRange.store(juce::jlimit(1, 12, semitones), std::memory_order_relaxed); }
    void setPitchSmoothing(float amount) noexcept { pitchSmoothing.store(juce::jlimit(0.0f, 100.0f, amount), std::memory_order_relaxed); }

private:
    static constexpr int fifoCapacity = 8192;
    static constexpr int maxWorkerBlockSize = 2048;

    void run() override;
    void publishPitch(const PitchResult& result) noexcept;
    void resetTrackingAndMidi() noexcept;

    PitchDetector detector;
    NoiseReducer noiseReducer;
    NoteTracker noteTracker;
    PitchSmoother pitchSmoother;
    PitchStabilityGuard pitchStabilityGuard;
    MidiRouter* midiRouter = nullptr;
    juce::AbstractFifo sampleFifo { fifoCapacity };
    std::array<float, fifoCapacity> sampleBuffer {};
    juce::AudioBuffer<float> monoScratch;
    std::array<float, maxWorkerBlockSize> workerScratch {};
    int workerBlockSize = 512;
    double currentSampleRate = 44100.0;
    int lastPitchBendValue = 8192;

    std::atomic<float> latestFrequency { 0.0f };
    std::atomic<float> latestConfidence { 0.0f };
    std::atomic<float> latestRms { 0.0f };
    std::atomic<bool> latestVoiced { false };
    std::atomic<uint64_t> latestSequence { 0 };
    std::atomic<int> latestActiveNote { -1 };
    std::atomic<float> inputGain { 1.0f };
    std::atomic<int> inputChannelMode { 0 }, activeInputChannel { -1 };
    std::atomic<uint64_t> droppedAudioSamples { 0 };
    std::atomic<float> latestInputRms { 0.0f }, latestInputPeak { 0.0f };
    std::atomic<int> latestAnalysisWindowSamples { 0 };
    std::atomic<bool> latestInputClipping { false };
    std::atomic<bool> resetRequested { false };

    std::atomic<bool> transitionSmoothingEnabled { false };
    std::atomic<float> transitionConfirmationMs { 60.0f };
    std::atomic<bool> autoTuneEnabled { false };
    std::atomic<int> autoTuneTonic { 0 }, autoTuneScale { 1 };
    std::atomic<bool> pitchStabilityEnabled { false };
    std::atomic<float> pitchDropToleranceCents { 45.0f }, pitchDropHoldMs { 30.0f };
    std::atomic<bool> pitchBendEnabled { true };
    std::atomic<int> pitchBendRange { 2 };
    std::atomic<float> pitchSmoothing { 35.0f };
};
