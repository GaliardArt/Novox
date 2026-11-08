#include "DrumMapping.h"

#include <iostream>

namespace
{
    int failures = 0;
    int checks = 0;

    void check(bool condition, const char* name)
    {
        ++checks;
        if (condition)
            std::cout << "[PASS] " << name << '\n';
        else
        {
            ++failures;
            std::cerr << "[FAIL] " << name << '\n';
        }
    }
}

int main()
{
    check(DrumMapping::midiChannel == 10, "General MIDI percussion uses channel 10");
    check(DrumMapping::defaultNote(DrumMapping::Part::kick) == 36, "Kick defaults to Bass Drum 1");
    check(DrumMapping::defaultNote(DrumMapping::Part::snare) == 38, "Snare defaults to Acoustic Snare");
    check(DrumMapping::defaultNote(DrumMapping::Part::hiHat) == 42, "Hi-hat defaults to Closed Hi-Hat");
    check(DrumMapping::clampNote(1) == 35 && DrumMapping::clampNote(127) == 81,
          "Configured notes are clamped to the GM percussion range");
    check(DrumMapping::gmName(38) == "Acoustic Snare", "GM note labels are exposed for the UI");
    std::cout << "\nChecks: " << checks << ", failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
