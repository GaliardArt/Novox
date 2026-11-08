#pragma once

#include <algorithm>
#include <array>
#include <string_view>

namespace DrumMapping
{
    inline constexpr int midiChannel = 10;
    inline constexpr int defaultKick = 36;   // Bass Drum 1
    inline constexpr int defaultSnare = 38;  // Acoustic Snare
    inline constexpr int defaultHiHat = 42;  // Closed Hi-Hat
    inline constexpr int minimumNote = 35;
    inline constexpr int maximumNote = 81;

    enum class Part : unsigned char { kick = 0, snare = 1, hiHat = 2 };

    constexpr int clampNote(int note) noexcept
    {
        return std::clamp(note, minimumNote, maximumNote);
    }

    constexpr int defaultNote(Part part) noexcept
    {
        switch (part)
        {
            case Part::kick: return defaultKick;
            case Part::snare: return defaultSnare;
            case Part::hiHat: return defaultHiHat;
        }
        return defaultKick;
    }

    constexpr std::string_view partName(Part part) noexcept
    {
        switch (part)
        {
            case Part::kick: return "Kick";
            case Part::snare: return "Snare";
            case Part::hiHat: return "Hi-hat";
        }
        return "Unknown";
    }

    constexpr std::string_view gmName(int note) noexcept
    {
        switch (note)
        {
            case 35: return "Acoustic Bass Drum";
            case 36: return "Bass Drum 1";
            case 37: return "Side Stick";
            case 38: return "Acoustic Snare";
            case 39: return "Hand Clap";
            case 40: return "Electric Snare";
            case 41: return "Low Floor Tom";
            case 42: return "Closed Hi-Hat";
            case 43: return "High Floor Tom";
            case 44: return "Pedal Hi-Hat";
            case 45: return "Low Tom";
            case 46: return "Open Hi-Hat";
            case 47: return "Low-Mid Tom";
            case 48: return "Hi-Mid Tom";
            case 49: return "Crash Cymbal 1";
            case 50: return "High Tom";
            case 51: return "Ride Cymbal 1";
            case 52: return "Chinese Cymbal";
            case 53: return "Ride Bell";
            case 54: return "Tambourine";
            case 55: return "Splash Cymbal";
            case 56: return "Cowbell";
            case 57: return "Crash Cymbal 2";
            case 58: return "Vibraslap";
            case 59: return "Ride Cymbal 2";
            case 60: return "Hi Bongo";
            case 61: return "Low Bongo";
            case 62: return "Mute Hi Conga";
            case 63: return "Open Hi Conga";
            case 64: return "Low Conga";
            case 65: return "High Timbale";
            case 66: return "Low Timbale";
            case 67: return "High Agogo";
            case 68: return "Low Agogo";
            case 69: return "Cabasa";
            case 70: return "Maracas";
            case 71: return "Short Whistle";
            case 72: return "Long Whistle";
            case 73: return "Short Guiro";
            case 74: return "Long Guiro";
            case 75: return "Claves";
            case 76: return "Hi Wood Block";
            case 77: return "Low Wood Block";
            case 78: return "Mute Cuica";
            case 79: return "Open Cuica";
            case 80: return "Mute Triangle";
            case 81: return "Open Triangle";
            default: return "Percussion";
        }
    }
}
