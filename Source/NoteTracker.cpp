#include "NoteTracker.h"
#include <algorithm>
#include <cmath>

int NoteTracker::frequencyToMidiNote(float frequencyHz) noexcept
{
    if (!(frequencyHz > 0.0f) || !std::isfinite(frequencyHz))
        return -1;

    const auto midiValue = 69.0f + 12.0f * std::log2(frequencyHz / 440.0f);
    return std::clamp(static_cast<int>(std::lround(midiValue)), 0, 127);
}

NoteUpdate NoteTracker::process(const PitchResult& pitch, float elapsedSeconds) noexcept
{
    NoteUpdate update;
    const float delta = std::clamp(elapsedSeconds, 0.0f, 0.25f);
    int detectedNote = pitch.voiced ? frequencyToMidiNote(pitch.frequencyHz) : -1;
    if (detectedNote >= 0 && autoTuneEnabled)
        detectedNote = ScaleQuantizer::quantize(detectedNote, scaleTonic, musicalScale);

    if (detectedNote < 0)
    {
        candidateNote = -1;
        candidateSeconds = 0.0f;

        // A voiced signal with an uncertain YIN estimate is not equivalent to
        // silence. Hold the current MIDI note briefly to bridge isolated pitch
        // estimation dropouts, but enforce a hard timeout to prevent stuck notes.
        if (activeNote >= 0 && pitch.signalPresent)
        {
            silentSeconds = 0.0f;
            pitchDropoutSeconds += delta;
            if (pitchDropoutSeconds + 1.0e-6f >= pitchDropoutHoldSeconds)
            {
                update.noteOff = activeNote;
                activeNote = -1;
                pitchDropoutSeconds = 0.0f;
            }
            return update;
        }

        pitchDropoutSeconds = 0.0f;
        silentSeconds += delta;
        if (silentSeconds >= releaseConfirmationSeconds && activeNote >= 0)
        {
            update.noteOff = activeNote;
            activeNote = -1;
        }
        return update;
    }

    silentSeconds = 0.0f;
    pitchDropoutSeconds = 0.0f;

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
        candidateSeconds = 0.0f;
        return update;
    }

    if (detectedNote != candidateNote)
    {
        candidateNote = detectedNote;
        candidateSeconds = delta;
    }
    else
    {
        candidateSeconds += delta;
    }

    const float requiredSeconds = activeNote >= 0 && transitionSmoothingEnabled
        ? transitionConfirmationSeconds : onsetConfirmationSeconds;

    if (candidateSeconds + 1.0e-6f >= requiredSeconds)
    {
        update.noteOff = activeNote;
        update.noteOn = candidateNote;
        activeNote = candidateNote;
        candidateNote = -1;
        candidateSeconds = 0.0f;
    }

    return update;
}

void NoteTracker::reset() noexcept
{
    activeNote = -1;
    candidateNote = -1;
    candidateSeconds = 0.0f;
    silentSeconds = 0.0f;
    pitchDropoutSeconds = 0.0f;
}
