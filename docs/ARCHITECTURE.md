# NOVOX architecture

## Application overview

```text
AudioDeviceManager
       |
       +---- VoiceEngine (audio callback / producer)
       |          |
       |          +---- channel mix + input gain
       |          +---- preallocated SPSC sample FIFO
       |                         |
       |                         v
       |                VoiceEngine worker thread
       |                         |
       |                         +---- PitchDetector (YIN)
       |                         +---- sequence-stamped PitchResult
       |
       +---- MainComponent (GUI / 60 Hz refresh)
                  |
                  +---- NoteTracker (new frames only)
                  |       +---- onset/release debounce
                  |       +---- semitone hysteresis
                  |
                  +---- MidiRouter
```

## Core modules

- `VoiceEngine`: audio capture handoff, preallocated SPSC FIFO, worker lifecycle, and coherent latest-pitch publication.
- `PitchDetector`: YIN-style monophonic fundamental estimation.
- `NoteTracker`: note onset/offset confirmation and pitch-boundary hysteresis.
- `MidiRouter`: MIDI output device selection and note events.
- `TriggerTrainer`: future user-recorded trigger samples and feature extraction.
- `PercussionClassifier`: future classification of trained vocal sounds into trigger slots.
- `ScaleQuantizer`: future optional scale snapping, octave mapping, and pitch bend.
- `SettingsStore`: future local settings and presets.
- `MainComponent`: visual feedback and controls.

## Real-time design

1. The audio callback does not execute pitch detection, allocate memory, wait on a mutex, log, or access the GUI.
2. The callback performs channel averaging and writes into a fixed-capacity single-producer/single-consumer FIFO.
3. A dedicated worker thread consumes audio blocks and runs the YIN detector.
4. FIFO overflow never blocks the audio callback; dropped input samples are counted for diagnostics.
5. Pitch snapshots use a sequence counter so the GUI does not read a partially published result.
6. The GUI processes each new analysis frame once, rather than counting repeated timer reads as separate confirmations.

## Note tracking

- Three consecutive analysis frames are required to trigger note-on or a note transition.
- Four consecutive unvoiced frames are required to trigger note-off.
- A 0.58-semitone transition margin reduces retriggers caused by small fluctuations around note boundaries.
- MIDI is still sent from the GUI timer. A timestamped event queue and dedicated MIDI dispatch remain future work.

## Validation

CTest includes deterministic pitch-detector and note-tracker executables. These tests validate algorithmic behavior; they do not certify real-world microphone latency, CPU load, or performance with every audio driver.
