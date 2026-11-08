#pragma once
#include "PitchDetector.h"

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
    int getActiveNote() const noexcept { return activeNote; }

private:
    static int frequencyToMidiNote(float frequencyHz) noexcept;

    int activeNote = -1;
    int candidateNote = -1;
    int candidateFrames = 0;
    int silentFrames = 0;

    static constexpr int onsetConfirmations = 3;
    static constexpr int releaseConfirmations = 4;
    static constexpr float noteChangeHysteresisSemitones = 0.58f;
};
