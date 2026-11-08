#pragma once

#include <algorithm>
#include <cmath>

namespace PitchBend
{
    constexpr int center = 8192;
    constexpr int maximum = 16383;

    // Converts a detected frequency relative to the currently held MIDI note
    // into the standard 14-bit MIDI pitch-wheel range.
    inline int to14BitValue(float frequencyHz, int activeMidiNote,
                            int rangeSemitones) noexcept
    {
        if (!(frequencyHz > 0.0f) || activeMidiNote < 0 || activeMidiNote > 127)
            return center;

        const int safeRange = std::clamp(rangeSemitones, 1, 12);
        const float midiValue = 69.0f + 12.0f * std::log2(frequencyHz / 440.0f);
        const float normalized = std::clamp(
            (midiValue - static_cast<float>(activeMidiNote)) / static_cast<float>(safeRange),
            -1.0f, 1.0f);

        if (normalized >= 0.0f)
            return center + static_cast<int>(std::lround(normalized * 8191.0f));

        return center + static_cast<int>(std::lround(normalized * 8192.0f));
    }

    inline float semitoneOffset(float frequencyHz, int activeMidiNote) noexcept
    {
        if (!(frequencyHz > 0.0f) || activeMidiNote < 0 || activeMidiNote > 127)
            return 0.0f;

        const float midiValue = 69.0f + 12.0f * std::log2(frequencyHz / 440.0f);
        return midiValue - static_cast<float>(activeMidiNote);
    }
}
