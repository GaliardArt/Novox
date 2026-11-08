#pragma once
#include "PitchDetector.h"
#include "ScaleQuantizer.h"
#include <algorithm>

struct NoteUpdate
{
    int noteOff = -1;
    int noteOn = -1;
};

class NoteTracker
{
public:
    NoteUpdate process(const PitchResult& pitch, float elapsedSeconds = 0.01f) noexcept;
    void reset() noexcept;

    void setTransitionSmoothing(bool enabled, float confirmationMs) noexcept
    {
        transitionSmoothingEnabled = enabled;
        transitionConfirmationSeconds = std::clamp(confirmationMs, 30.0f, 120.0f) * 0.001f;
    }

    void setAutoTune(bool enabled, int tonic, MusicalScale scale) noexcept
    {
        const int safeTonic = std::clamp(tonic, 0, 11);
        if (autoTuneEnabled != enabled || scaleTonic != safeTonic || musicalScale != scale)
        {
            autoTuneEnabled = enabled;
            scaleTonic = safeTonic;
            musicalScale = scale;
            candidateNote = -1;
            candidateSeconds = 0.0f;
        }
    }

    int getActiveNote() const noexcept { return activeNote; }

private:
    static int frequencyToMidiNote(float frequencyHz) noexcept;

    int activeNote = -1;
    int candidateNote = -1;
    float candidateSeconds = 0.0f;
    float silentSeconds = 0.0f;
    float transitionConfirmationSeconds = 0.060f;
    bool transitionSmoothingEnabled = false;
    bool autoTuneEnabled = false;
    int scaleTonic = 0;
    MusicalScale musicalScale = MusicalScale::major;

    static constexpr float onsetConfirmationSeconds = 0.030f;
    static constexpr float releaseConfirmationSeconds = 0.040f;
    static constexpr float noteChangeHysteresisSemitones = 0.58f;
};
