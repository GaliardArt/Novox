#include "MidiNoteCleanup.h"
#include <cmath>
#include <iostream>

namespace
{
    int failures = 0;
    void check(bool value, const char* message)
    {
        if (!value) { ++failures; std::cerr << "[FAIL] " << message << '\n'; }
        else std::cout << "[PASS] " << message << '\n';
    }
    PitchResult pitch(float midi, float confidence = 0.95f)
    {
        PitchResult p;
        p.frequencyHz = 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f);
        p.confidence = confidence;
        p.rms = 0.1f;
        p.voiced = true;
        return p;
    }
}

int main()
{
    MidiNoteCleanup cleanup;
    cleanup.configure(true, 60.0f, 65.0f, 25.0f);
    for (int i = 0; i < 5; ++i)
        check(!cleanup.process(pitch(69.0f), 0.01f).voiced, "short candidate is withheld before 60 ms");
    check(cleanup.process(pitch(69.0f), 0.01f).voiced, "stable pitch passes after 60 ms");

    cleanup.process(pitch(70.0f), 0.01f);
    cleanup.process(pitch(70.0f), 0.01f);
    check(std::abs(cleanup.process(pitch(69.0f), 0.01f).frequencyHz - pitch(69.0f).frequencyHz) < 0.01f,
          "brief pitch spike is discarded without disturbing the current note");

    const auto pendingTransition = cleanup.process(pitch(70.0f), 0.01f);
    check(pendingTransition.voiced && std::abs(pendingTransition.frequencyHz - pitch(69.0f).frequencyHz) < 0.01f,
          "brief transition candidate does not interrupt the confirmed note");
    cleanup.process(pitch(70.0f), 0.01f);
    cleanup.process(pitch(70.0f), 0.01f);
    cleanup.process(pitch(70.0f), 0.01f);
    cleanup.process(pitch(70.0f), 0.01f);
    check(std::abs(cleanup.process(pitch(70.0f), 0.01f).frequencyHz - pitch(70.0f).frequencyHz) < 0.01f,
          "new pitch passes after its stability window");

    check(cleanup.process(PitchResult{}, 0.01f).voiced, "short unvoiced gap is bridged");
    check(cleanup.process(PitchResult{}, 0.01f).voiced, "gap bridge holds through configured window");
    check(!cleanup.process(PitchResult{}, 0.01f).voiced, "long gap is not bridged");

    cleanup.reset();
    check(!cleanup.process(pitch(69.0f, 0.40f), 0.02f).voiced, "low-confidence pitch is rejected");
    check(!cleanup.process(pitch(69.0f, 0.40f), 0.02f).voiced, "low-confidence noise does not form a note");

    cleanup.configure(false, 60.0f, 65.0f, 25.0f);
    check(cleanup.process(pitch(72.0f), 0.01f).voiced, "disabled cleanup leaves pitch unchanged");
    std::cout << "Failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
