#include "NoteTracker.h"
#include <algorithm>
#include <cmath>

int NoteTracker::frequencyToMidiNote(float frequencyHz) noexcept
{
    if (!(frequencyHz > 0.0f))
        return -1;

    const auto midiValue = 69.0f + 12.0f * std::log2(frequencyHz / 440.0f);
    return std::clamp(static_cast<int>(std::lround(midiValue)), 0, 127);
}

NoteUpdate NoteTracker::process(const PitchResult& pitch) noexcept
{
    NoteUpdate update;
    int detectedNote = pitch.voiced ? frequencyToMidiNote(pitch.frequencyHz) : -1;

    if (detectedNote < 0)
    {
        candidateNote = -1;
        candidateFrames = 0;
        ++silentFrames;

        if (silentFrames >= releaseConfirmations && activeNote >= 0)
        {
            update.noteOff = activeNote;
            activeNote = -1;
        }
        return update;
    }

    silentFrames = 0;

    // Keep the current note through small pitch fluctuations around a
    // semitone boundary; a real transition must cross the hysteresis margin.
    if (activeNote >= 0 && detectedNote != activeNote)
    {
        const auto midiValue = 69.0f + 12.0f * std::log2(pitch.frequencyHz / 440.0f);
        if (std::abs(midiValue - static_cast<float>(activeNote))
            < noteChangeHysteresisSemitones)
            detectedNote = activeNote;
    }

    if (detectedNote == activeNote)
    {
        candidateNote = -1;
        candidateFrames = 0;
        return update;
    }

    if (detectedNote != candidateNote)
    {
        candidateNote = detectedNote;
        candidateFrames = 1;
        return update;
    }

    ++candidateFrames;
    const int requiredConfirmations = activeNote >= 0 && transitionSmoothingEnabled
        ? transitionConfirmations : onsetConfirmations;
    if (candidateFrames >= requiredConfirmations)
    {
        update.noteOff = activeNote;
        update.noteOn = candidateNote;
        activeNote = candidateNote;
        candidateNote = -1;
        candidateFrames = 0;
    }

    return update;
}

void NoteTracker::reset() noexcept
{
    activeNote = -1;
    candidateNote = -1;
    candidateFrames = 0;
    silentFrames = 0;
}
