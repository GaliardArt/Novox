# NOVOX

NOVOX is an offline-first, standalone vocal instrument for turning voice into musical control data. It is written in C++ and uses JUCE for audio input, MIDI integration, and the desktop application.

## Current milestone (0.4.0 — implementation in validation)

- JUCE standalone desktop application and microphone capture
- YIN-style fundamental-frequency estimation with confidence and RMS level
- Adjustable pitch sensitivity to trade quiet-voice tracking against noise rejection
- Pitch analysis moved off the audio callback to a dedicated worker thread
- Preallocated SPSC audio FIFO and sequence-stamped pitch snapshots
- Note onset/release confirmation and semitone hysteresis
- Configurable pitch smoothing and micro-variation deadband for MIDI Pitch Bend
- Dedicated MIDI dispatch thread fed by a bounded SPSC event queue
- Ordered asynchronous MIDI Note On/Off and 14-bit Pitch Bend delivery
- Configurable Pitch Bend enable/disable and range from ±1 to ±12 semitones
- Configurable MIDI output channel (1–16), captured per queued event
- Automated pitch-detector, note-tracker, MIDI-queue, Pitch Bend, and smoothing regression tests

## Build on Windows

Requirements: Visual Studio 2022 with Desktop development with C++, CMake 3.22+, Git, Windows 10/11.

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

CMake fetches JUCE 8.0.6 on the first configure. An internet connection is needed for that dependency download. The application is designed to run offline after dependencies are available.

## Regression coverage

- `NOVOXPitchTests`: known frequencies, 44.1/48 kHz sample rates, sensitivity response, silence/low-level rejection, harmonic-rich input, broadband noise, and invalid input.
- `NOVOXTrackingTests`: onset debounce, transient rejection, release debounce, note transitions, and semitone hysteresis.
- `NOVOXMidiQueueTests`: empty queue, FIFO order and payload for note and bend events, bounded-capacity overflow, recovery, and ring-buffer wraparound.
- `NOVOXPitchBendTests`: center, positive/negative direction, clamping, configurable range, invalid input, and fractional semitone offsets.
- `NOVOXPitchSmootherTests`: transparent mode, convergence, micro-variation rejection, reset, invalid values, and parameter clamping.

The tests are deterministic. They do not replace microphone, room-noise, end-to-end latency, MIDI-driver, or musical-performance testing.

## Continuous integration

GitHub Actions configures and compiles the Windows x64 Release build and runs CTest on pushes and pull requests targeting `main` or `dev/**`. Workflow: `.github/workflows/windows-build.yml`.

## Engineering notes

- The audio callback performs bounded channel mixing and FIFO writes only; pitch detection runs on a worker thread.
- Pitch sensitivity is published atomically to the pitch worker. Higher values lower the RMS gate and confidence threshold, allowing quieter signals through at the cost of admitting more noise.
- Pitch smoothing is applied only to the MIDI bend control, not the raw detector display or note-onset logic.
- The smoothing control uses a one-pole response plus a small deadband. It suppresses small frame-to-frame fluctuations while larger pitch movement remains responsive; it is not intended to remove expressive vibrato completely.
- Audio FIFO overflow and callback blocks larger than the prepared scratch buffer are counted and dropped without blocking the callback.
- The MIDI queue is bounded and lock-free for one GUI producer and one MIDI dispatch consumer. The worker preserves event order and sends messages independently of the GUI timer.
- Each queued MIDI event captures its channel at enqueue time, so changing channels cannot redirect already queued note-offs.
- Pitch Bend is calculated from the fractional semitone offset relative to the tracked note and encoded using the standard 14-bit MIDI wheel value. The configured range is applied symmetrically, with asymmetric integer scaling around MIDI center 8192.
- Pitch Bend updates are sent only for new analysis frames and changes of at least two wheel units, limiting redundant queue traffic.
- Closing or changing MIDI output drains pending events and stops the dispatch worker before releasing the device handle.
- MIDI event generation still originates from the GUI timer. This milestone does not claim sample-accurate timing.
