#include "PitchBend.h"

#include <cmath>
#include <iostream>
#include <string>

namespace
{
    int checks = 0;
    int failures = 0;

    void check(bool condition, const std::string& name)
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

    float frequencyForMidi(float midi)
    {
        return 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f);
    }

    void testCenterAndInvalidInput()
    {
        check(PitchBend::to14BitValue(440.0f, 69, 2) == 8192,
              "Reference pitch maps to the 14-bit center");
        check(PitchBend::to14BitValue(0.0f, 69, 2) == 8192,
              "Zero frequency safely returns center");
        check(PitchBend::to14BitValue(440.0f, -1, 2) == 8192,
              "Invalid active note safely returns center");
        check(PitchBend::semitonesTo14BitValue(std::nanf(""), 2) == 8192,
              "Non-finite semitone offset safely returns center");
    }

    void testPositiveAndNegativeBend()
    {
        const auto upHalf = PitchBend::to14BitValue(frequencyForMidi(69.5f), 69, 2);
        const auto downHalf = PitchBend::to14BitValue(frequencyForMidi(68.5f), 69, 2);
        check(upHalf > 8192 && upHalf < 16383,
              "Positive pitch deviation maps above center");
        check(downHalf < 8192 && downHalf > 0,
              "Negative pitch deviation maps below center");
        check(PitchBend::to14BitValue(frequencyForMidi(71.0f), 69, 2) == 16383,
              "Positive bend clamps to maximum");
        check(PitchBend::to14BitValue(frequencyForMidi(67.0f), 69, 2) == 0,
              "Negative bend clamps to minimum");
    }

    void testConfigurableRange()
    {
        const auto oneSemitone = PitchBend::to14BitValue(frequencyForMidi(70.0f), 69, 1);
        const auto twoSemitones = PitchBend::to14BitValue(frequencyForMidi(70.0f), 69, 2);
        check(oneSemitone == 16383, "One-semitone range reaches maximum at +1 semitone");
        check(twoSemitones > 8192 && twoSemitones < oneSemitone,
              "Wider bend range produces a proportionally smaller wheel value");
        check(PitchBend::to14BitValue(frequencyForMidi(70.0f), 69, 0)
                  == PitchBend::to14BitValue(frequencyForMidi(70.0f), 69, 1),
              "Bend range is clamped to a minimum of one semitone");
        check(PitchBend::to14BitValue(frequencyForMidi(70.0f), 69, 99)
                  == PitchBend::to14BitValue(frequencyForMidi(70.0f), 69, 12),
              "Bend range is clamped to a maximum of twelve semitones");
    }

    void testSemitoneOffset()
    {
        check(std::abs(PitchBend::semitoneOffset(440.0f, 69)) < 0.0001f,
              "Exact MIDI note has zero semitone offset");
        check(std::abs(PitchBend::semitoneOffset(frequencyForMidi(69.25f), 69) - 0.25f) < 0.001f,
              "Semitone offset reports fractional pitch accurately");
        const auto direct = PitchBend::semitonesTo14BitValue(0.5f, 2);
        const auto fromFrequency = PitchBend::to14BitValue(frequencyForMidi(69.5f), 69, 2);
        check(direct == fromFrequency,
              "Smoothed semitone offset maps to the same wheel value as frequency input");
    }
}

int main()
{
    std::cout << "NOVOX PitchBend regression tests\n";
    testCenterAndInvalidInput();
    testPositiveAndNegativeBend();
    testConfigurableRange();
    testSemitoneOffset();
    std::cout << "\nChecks: " << checks << ", failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
