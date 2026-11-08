#include "NoteTracker.h"

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

    PitchResult note(float midiValue)
    {
        PitchResult pitch;
        pitch.frequencyHz = 440.0f * std::pow(2.0f, (midiValue - 69.0f) / 12.0f);
        pitch.confidence = 0.95f;
        pitch.rms = 0.1f;
        pitch.voiced = true;
        return pitch;
    }

    PitchResult silence()
    {
        return {};
    }

    void testOnsetDebounce()
    {
        NoteTracker tracker;
        check(tracker.process(note(69.0f)).noteOn == -1, "Onset waits for confirmation 1/3");
        check(tracker.process(note(69.0f)).noteOn == -1, "Onset waits for confirmation 2/3");
        const auto update = tracker.process(note(69.0f));
        check(update.noteOn == 69 && tracker.getActiveNote() == 69,
              "Onset emits MIDI note after three confirmed frames");
    }

    void testTransientRejection()
    {
        NoteTracker tracker;
        tracker.process(note(69.0f));
        tracker.process(note(69.0f));
        tracker.process(note(69.0f));
        tracker.process(note(70.0f));
        const auto update = tracker.process(note(69.0f));
        check(update.noteOn == -1 && tracker.getActiveNote() == 69,
              "One-frame pitch spike does not retrigger MIDI");
    }

    void testReleaseDebounce()
    {
        NoteTracker tracker;
        tracker.process(note(69.0f));
        tracker.process(note(69.0f));
        tracker.process(note(69.0f));
        tracker.process(silence());
        tracker.process(silence());
        tracker.process(silence());
        const auto update = tracker.process(silence());
        check(update.noteOff == 69 && tracker.getActiveNote() == -1,
              "Release emits note-off after four silent frames");
    }

    void testNoteTransition()
    {
        NoteTracker tracker;
        tracker.process(note(69.0f));
        tracker.process(note(69.0f));
        tracker.process(note(69.0f));
        tracker.process(note(70.0f));
        tracker.process(note(70.0f));
        const auto update = tracker.process(note(70.0f));
        check(update.noteOff == 69 && update.noteOn == 70
                  && tracker.getActiveNote() == 70,
              "Confirmed transition sends note-off then note-on");
    }

    void testOptionalTransitionSmoothing()
    {
        NoteTracker tracker;
        tracker.process(note(69.0f));
        tracker.process(note(69.0f));
        tracker.process(note(69.0f));
        tracker.setTransitionSmoothing(true, 6);
        for (int i = 0; i < 5; ++i)
            check(tracker.process(note(70.0f)).noteOn == -1,
                  "Enabled transition smoothing waits for configured stability");
        const auto update = tracker.process(note(70.0f));
        check(update.noteOff == 69 && update.noteOn == 70,
              "Enabled transition smoothing commits after six stable frames");

        tracker.setTransitionSmoothing(false, 12);
        tracker.process(note(71.0f));
        tracker.process(note(71.0f));
        const auto directUpdate = tracker.process(note(71.0f));
        check(directUpdate.noteOn == 71,
              "Disabled transition smoothing preserves original three-frame behavior");
    }

    void testScaleQuantizer(){check(ScaleQuantizer::quantize(61,0,MusicalScale::major)==60,"C major maps C# to C");check(ScaleQuantizer::quantize(63,0,MusicalScale::major)==62,"C major maps D# to E");check(ScaleQuantizer::quantize(63,0,MusicalScale::minor)==63,"C minor keeps D#");check(ScaleQuantizer::quantize(61,0,MusicalScale::chromatic)==61,"Chromatic preserves semitone");check(ScaleQuantizer::quantize(65,2,MusicalScale::major)==64,"D major uses tonic");}
    void testAutoTuneTracking(){NoteTracker t;t.setAutoTune(true,0,MusicalScale::major);t.process(note(61));t.process(note(61));check(t.process(note(61)).noteOn==60,"Auto-Tune quantizes before MIDI");t.setAutoTune(false,0,MusicalScale::major);t.reset();t.process(note(61));t.process(note(61));check(t.process(note(61)).noteOn==61,"Auto-Tune off preserves detected note");}
    void testHysteresis()
    {
        NoteTracker tracker;
        tracker.process(note(69.0f));
        tracker.process(note(69.0f));
        tracker.process(note(69.0f));

        tracker.process(note(69.54f));
        tracker.process(note(69.54f));
        tracker.process(note(69.54f));
        check(tracker.getActiveNote() == 69,
              "Small semitone-boundary fluctuation does not change active note");

        tracker.process(note(69.70f));
        tracker.process(note(69.70f));
        const auto update = tracker.process(note(69.70f));
        check(update.noteOff == 69 && update.noteOn == 70,
              "Pitch crossing hysteresis margin permits a real note change");
    }
}

int main()
{
    std::cout << "NOVOX NoteTracker regression tests\n";
    testOnsetDebounce();
    testTransientRejection();
    testReleaseDebounce();
    testNoteTransition();
    testOptionalTransitionSmoothing();testScaleQuantizer();testAutoTuneTracking();
    testHysteresis();
    std::cout << "\nChecks: " << checks << ", failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
