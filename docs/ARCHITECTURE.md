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
       |                         +---- PitchDetector (YIN + sensitivity gate)
       |                         +---- sequence-stamped PitchResult
       |
       +---- MainComponent (GUI / 60 Hz refresh + diagnostics)
                  |
                  +---- NoteTracker (new frames only)
                  |       +---- onset/release debounce
                  |       +---- semitone hysteresis
                  |
                  +---- PitchSmoother (MIDI bend only)
                  |
                  +---- PitchBend (fractional semitone -> 14-bit wheel)
                  |
                  +---- MidiRouter (GUI producer)
                  |           |
                  |           +---- bounded SPSC MIDI event queue
                  |                          |
                  |                          v
                  |                 MIDI dispatch thread
                  |
                  +---- JUCE ApplicationProperties
                              |
                              +---- local PropertiesFile
                                    (gain, sensitivity, smoothing,
                                     bend settings, MIDI channel/output)
                  |
                  +---- PresetManager
                              |
                              +---- versioned local .novoxpreset JSON files
```

## Core modules

- `VoiceEngine`: audio capture handoff, preallocated SPSC FIFO, worker lifecycle, and coherent latest-pitch publication.
- `PitchDetector`: YIN-style monophonic fundamental estimation with adjustable RMS and confidence thresholds.
- `NoteTracker`: note onset/offset confirmation and pitch-boundary hysteresis.
- `PitchSmoother`: optional one-pole smoothing and deadband applied to MIDI bend offsets; raw pitch display and note detection remain unfiltered.
- `PitchBend`: pure conversion from semitone offset to the standard 14-bit pitch-wheel value.
- `MidiRouter`: MIDI output selection, channel-aware bounded event queue, and dedicated MIDI dispatch thread.
- `MidiEventQueue`: fixed-capacity, lock-free SPSC queue preserving Note On, Note Off, and Pitch Bend order and payload.
- `MainComponent`: tabbed JUCE interface (Play, MIDI, Audio, Presets), visual feedback, controls, live audio-device diagnostics, dropped-sample/event counters, and local preference restoration/persistence through JUCE `ApplicationProperties`.
- `PresetData`: complete, validated snapshot of the active instrument configuration.
- `PresetManager`: versioned JSON preset files stored in the user's NOVOX application-data directory; supports listing, save/update, load, and delete.

## Runtime diagnostics

The GUI periodically reads the current `AudioIODevice` metadata and exposes the active input device name, enabled input-channel count, sample rate, and buffer size. It also displays the audio FIFO/callback drop counter from `VoiceEngine` and the bounded MIDI event queue overflow counter from `MidiRouter`. These counters are diagnostic telemetry only; they do not alter real-time processing or attempt recovery by blocking either worker.

## Presets

Each preset is a versioned JSON file with schema version 1 and a `.novoxpreset` extension. The snapshot includes gain, sensitivity, smoothing, Pitch Bend enable/range, melodic MIDI channel and preferred output name. Values are clamped to supported ranges when saved and loaded. Preset names are trimmed, length-limited, and stripped of filesystem-reserved characters. Loading a preset applies its values to the UI, audio engine, MIDI router, and local preferences. An unavailable saved MIDI device remains unselected; the application never substitutes another device automatically.

## Local preferences

JUCE `ApplicationProperties` owns a per-user `PropertiesFile` in the NOVOX application settings directory. The application stores input gain, pitch sensitivity, pitch smoothing, Pitch Bend enable state, Pitch Bend range, MIDI channel, preferred MIDI output name. Numeric values are clamped to supported UI ranges when loaded. Values are saved after user changes and again during shutdown.

MIDI output restoration uses an exact device-name match. If the saved device is absent, NOVOX leaves the output unselected rather than choosing another device. The preference remains saved until the user explicitly selects a different output or chooses no output.

## Real-time design

1. The audio callback does not execute pitch detection, allocate memory, wait on a mutex, log, or access the GUI.
2. The callback performs channel averaging and writes into a fixed-capacity SPSC FIFO.
3. A dedicated worker thread consumes audio blocks and runs the YIN detector.
4. Pitch sensitivity is an atomic setting read by the worker. Increasing it lowers the RMS and confidence gates, improving quiet-signal tracking while increasing noise susceptibility.
5. Audio FIFO overflow and blocks larger than the prepared scratch buffer never block the callback; all such dropped samples are counted.
6. Pitch snapshots use a sequence counter so the GUI does not read a partially published result.
7. The GUI processes each new analysis frame once.
8. Raw pitch is passed to NoteTracker. Only the continuous MIDI bend offset is smoothed, preserving note detection response.
9. The GUI enqueues MIDI events into a bounded SPSC queue. Each event captures the selected MIDI channel before entering the queue.
10. A dedicated worker sends messages to the selected MIDI device in FIFO order. Output shutdown drains pending events before releasing the device handle.

## Pitch stability and sensitivity

- Sensitivity ranges from 0–100%, default 50%. The RMS threshold varies from 0.014 at minimum to 0.002 at maximum; the confidence threshold varies from 0.92 to 0.68. Higher sensitivity can track quieter vocals but may also accept more background noise.
- Smoothing ranges from 0–100%, default 35%. Zero bypasses filtering. Higher values increase low-pass smoothing and the micro-variation deadband.
- Smoothing is applied only to Pitch Bend. It does not change the displayed raw frequency or the discrete NoteTracker decisions.
- The filter attenuates small rapid fluctuations but intentionally does not attempt to erase all vibrato, preserving expressive vocal movement.
- Filter state resets on note changes, MIDI output/channel changes, note release, and when smoothing is disabled or pitch bend is reset.

## Pitch Bend and channel behavior

- Pitch Bend is enabled by default, with a ±2-semitone range.
- The range can be set from ±1 to ±12 semitones.
- The MIDI channel can be selected from 1 to 16.
- Bend values are centered at 8192; negative and positive deviations map toward 0 and 16383 respectively.
- The wheel is recentered before Note Off, on disable, and before changing output or channel.
- Changing the channel releases the active note on the old channel before switching; the tracker then requires a fresh onset on the new channel.
- Bend events are sent only on new pitch frames and when the value changes by at least two units to reduce redundant messages.

## Validation

CTest includes deterministic pitch-detector, note-tracker, MIDI queue, Pitch Bend, pitch-smoothing, preset-storage executables. These tests validate synthetic signal response and filter behavior; they do not certify real-world microphone latency, room-noise rejection, MIDI driver behavior, CPU load, musical performance, or local settings behavior across Windows profiles.


## User interface (1.1.0)

The main window uses native JUCE components arranged in four tabs:
- **Play:** live detected note, frequency/confidence, Pitch Bend feedback.
- **MIDI:** output selection, channel, Pitch Bend configuration.
- **Audio:** input gain, pitch sensitivity, pitch smoothing, active device metadata, and dropped-sample/event diagnostics.
- **Presets:** local preset selection, save/update, load, and delete.

The UI redesign changes presentation and control placement only. Audio capture, worker-thread analysis, note tracking, MIDI event generation, and preset persistence remain in their existing modules.


## Web presentation layer (transition foundation)

On Windows, JUCE WebBrowserComponent uses WebView2 to render the local frontend in `UI/`. The resource provider serves only the allow-listed HTML, CSS, and JavaScript files from the application's adjacent UI directory. Native integration is enabled only for this bundled local content. The JavaScript bridge currently exposes a read-only status snapshot and a frontend-ready event. The WebView is presentation-only: it does not own audio callbacks, pitch analysis, note tracking, or MIDI dispatch. Existing native tabs remain intact until their web equivalents are implemented and validated.
