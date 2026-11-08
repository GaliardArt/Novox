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
                  +---- PitchBend (fractional semitone -> 14-bit wheel)
                  |
                  +---- MidiRouter (GUI producer)
                              |
                              +---- bounded SPSC MIDI event queue
                                             |
                                             v
                                    MIDI dispatch thread
                                             |
                                             +---- Note On / Note Off / Pitch Wheel
```

## Core modules

- `VoiceEngine`: audio capture handoff, preallocated SPSC FIFO, worker lifecycle, and coherent latest-pitch publication.
- `PitchDetector`: YIN-style monophonic fundamental estimation.
- `NoteTracker`: note onset/offset confirmation and pitch-boundary hysteresis.
- `PitchBend`: pure conversion from frequency and active note to the standard 14-bit pitch-wheel value.
- `MidiRouter`: MIDI output selection, channel-aware bounded event queue, and dedicated MIDI dispatch thread.
- `MidiEventQueue`: fixed-capacity, lock-free SPSC queue preserving Note On, Note Off, and Pitch Bend order and payload.
- `MainComponent`: visual feedback and controls for input gain, bend enable/range, MIDI output, and channel.

## Real-time design

1. The audio callback does not execute pitch detection, allocate memory, wait on a mutex, log, or access the GUI.
2. The callback performs channel averaging and writes into a fixed-capacity SPSC FIFO.
3. A dedicated worker thread consumes audio blocks and runs the YIN detector.
4. Audio FIFO overflow and blocks larger than the prepared scratch buffer never block the callback; all such dropped samples are counted.
5. Pitch snapshots use a sequence counter so the GUI does not read a partially published result.
6. The GUI processes each new analysis frame once.
7. The GUI converts fractional pitch relative to the active MIDI note into a 14-bit Pitch Bend value.
8. The GUI enqueues MIDI events into a bounded SPSC queue. Each event captures the selected MIDI channel before entering the queue.
9. A dedicated worker sends messages to the selected MIDI device in FIFO order. Output shutdown drains pending events before releasing the device handle.

## Pitch Bend and channel behavior

- Pitch Bend is enabled by default, with a ±2-semitone range.
- The range can be set from ±1 to ±12 semitones.
- The MIDI channel can be selected from 1 to 16.
- Bend values are centered at 8192; negative and positive deviations map toward 0 and 16383 respectively.
- The wheel is recentered before Note Off, on disable, and before changing output or channel.
- Changing the channel releases the active note on the old channel before switching; the tracker then requires a fresh onset on the new channel.
- Bend events are sent only on new pitch frames and when the value changes by at least two units to reduce redundant messages.

## Validation

CTest includes deterministic pitch-detector, note-tracker, MIDI queue, and Pitch Bend executables. These tests validate signal conversion and queue behavior; they do not certify real-world microphone latency, MIDI driver behavior, CPU load, or musical performance.
