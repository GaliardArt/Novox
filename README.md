# NOVOX

NOVOX is an offline-first, standalone vocal instrument for turning voice into musical control data. It is written in C++ and uses JUCE for audio input, MIDI integration, and the desktop application.

## Current milestone (0.2.3 — implementation in validation)

- JUCE standalone desktop application and microphone capture
- YIN-style fundamental-frequency estimation with confidence and RMS level
- Pitch analysis moved off the audio callback to a dedicated worker thread
- Preallocated SPSC audio FIFO and sequence-stamped pitch snapshots
- Note onset/release confirmation and semitone hysteresis
- Dedicated MIDI dispatch thread fed by a bounded SPSC event queue
- Ordered asynchronous MIDI Note On/Off delivery, isolated from the GUI timer
- Audio FIFO overflow and oversized callback blocks counted as dropped samples
- Automated pitch-detector, note-tracker, and MIDI-queue regression tests

## Build on Windows

Requirements: Visual Studio 2022 with Desktop development with C++, CMake 3.22+, Git, Windows 10/11.

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

CMake fetches JUCE 8.0.6 on the first configure. An internet connection is needed for that dependency download. The application is designed to run offline after dependencies are available.

## Regression coverage

- `NOVOXPitchTests`: known frequencies, 44.1/48 kHz sample rates, silence/low-level rejection, harmonic-rich input, broadband noise, and invalid input.
- `NOVOXTrackingTests`: onset debounce, transient rejection, release debounce, note transitions, and semitone hysteresis.
- `NOVOXMidiQueueTests`: empty queue, FIFO order and payload, bounded-capacity overflow, recovery, and ring-buffer wraparound.

The tests are deterministic. They do not replace microphone, room-noise, end-to-end latency, MIDI-driver, or musical-performance testing.

## Continuous integration

GitHub Actions configures and compiles the Windows x64 Release build and runs CTest on pushes and pull requests targeting `main` or `dev/**`. Workflow: `.github/workflows/windows-build.yml`.

## Engineering notes

- The audio callback performs bounded channel mixing and FIFO writes only; pitch detection runs on a worker thread.
- Audio FIFO overflow and callback blocks larger than the prepared scratch buffer are counted and dropped without blocking the callback.
- The MIDI queue is bounded and lock-free for one GUI producer and one MIDI dispatch consumer. The worker preserves event order and sends messages independently of the GUI timer.
- Closing or changing MIDI output drains pending events and stops the dispatch worker before releasing the device handle.
- MIDI event generation still originates from the GUI timer. Moving pitch tracking to the analysis worker and sample-timestamped scheduling remain future improvements.
