# NOVOX architecture

## Application overview

```text
AudioDeviceManager
       |
       +---- VoiceEngine (audio callback / producer)
       |          |
       |          +---- channel mix + input gain
       |          +---- preallocated SPSC audio FIFO
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
                  +---- MidiRouter (GUI producer)
                              |
                              +---- bounded SPSC MIDI event queue
                                             |
                                             v
                                    MIDI dispatch thread
                                             |
                                             +---- MidiOutput::sendMessageNow
```

## Core modules

- `VoiceEngine`: audio capture handoff, preallocated SPSC FIFO, worker lifecycle, and coherent latest-pitch publication.
- `PitchDetector`: YIN-style monophonic fundamental estimation.
- `NoteTracker`: note onset/offset confirmation and pitch-boundary hysteresis.
- `MidiRouter`: MIDI output selection, bounded event queue, and dedicated MIDI dispatch thread.
- `MidiEventQueue`: fixed-capacity, lock-free SPSC queue preserving MIDI event order.
- `TriggerTrainer`, `PercussionClassifier`, `ScaleQuantizer`, and `SettingsStore`: planned modules for later milestones.
- `MainComponent`: visual feedback and controls.

## Real-time design

1. The audio callback does not execute pitch detection, allocate memory, wait on a mutex, log, or access the GUI.
2. The callback performs channel averaging and writes into a fixed-capacity SPSC FIFO.
3. A dedicated worker thread consumes audio blocks and runs the YIN detector.
4. Audio FIFO overflow and blocks larger than the prepared scratch buffer never block the callback; all such dropped samples are counted.
5. Pitch snapshots use a sequence counter so the GUI does not read a partially published result.
6. The GUI processes each new analysis frame once.
7. The GUI enqueues MIDI events into a bounded SPSC queue. A dedicated worker sends them to the selected MIDI device in FIFO order.
8. MIDI output shutdown drains pending events and waits for the dispatch thread to exit before releasing the device handle.

## Note tracking and MIDI delivery

- Three consecutive analysis frames are required for note-on or a note transition.
- Four consecutive unvoiced frames are required for note-off.
- A 0.58-semitone transition margin reduces retriggers around note boundaries.
- MIDI dispatch no longer calls the device driver from the GUI timer. Event generation still runs on the GUI timer, so this milestone isolates driver I/O from the UI but does not claim sample-accurate timing or eliminate the timer polling interval.

## Validation

CTest includes deterministic pitch-detector, note-tracker, and MIDI event queue executables. These tests validate algorithmic and queue behavior; they do not certify real-world microphone latency, MIDI driver behavior, CPU load, or performance with every audio device.
