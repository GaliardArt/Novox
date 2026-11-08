#pragma once
#include "PitchDetector.h"
#include <algorithm>

struct NoteUpdate
{
    int noteOff = -1;
    int noteOn = -1;
};

class NoteTracker
{
public:
    NoteUpdate process(const PitchResult& pitch) noexcept;
    void reset() noexcept;
    void setTransitionSmoothing(bool enabled, int confirmationFrames) noexcept
    {
        transitionSmoothingEnabled = enabled;
        transitionConfirmations = std::clamp(confirmationFrames, 3, 12);
    }
    int getActiveNote() const noexcept { return activeNote; }

private:
    static int frequencyToMidiNote(float frequencyHz) noexcept;

    int activeNote = -1;
    int candidateNote = -1;
    int candidateFrames = 0;
    int silentFrames = 0;
    int transitionConfirmations = 6;
    bool transitionSmoothingEnabled = false;

    static constexpr int onsetConfirmations = 3;
    static constexpr int releaseConfirmations = 4;
    static constexpr float noteChangeHysteresisSemitones = 0.58f;
};
