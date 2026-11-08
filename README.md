# NOVOX

NOVOX is an offline-first, standalone vocal instrument for turning voice into musical control data. It is written in C++ and uses JUCE for audio input, MIDI integration, and the desktop application.

## Current milestone (0.2.2 — implementation in validation)

- JUCE standalone desktop application
- Microphone/audio input capture
- YIN-style fundamental-frequency estimation with confidence and RMS level
- Pitch analysis moved off the audio callback to a dedicated worker thread
- Preallocated single-producer/single-consumer audio FIFO between capture and analysis
- Sequence-stamped pitch snapshots so note tracking only consumes new analysis frames
- Note onset/release confirmation and semitone hysteresis to reduce MIDI chatter
- Selectable MIDI output and input gain
- Automated pitch-detector and note-tracker regression tests through CTest

## Build on Windows

Requirements: Visual Studio 2022 with Desktop development with C++, CMake 3.22+, Git, Windows 10/11.

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

CMake fetches JUCE 8.0.6 on the first configure. An internet connection is needed for that dependency download. The application is designed to run offline after dependencies are available.

## Regression coverage

- `NOVOXPitchTests`: known sine-wave frequencies, 44.1/48 kHz sample rates, silence and low-level rejection, harmonic-rich input, broadband-noise rejection, and invalid input handling.
- `NOVOXTrackingTests`: onset debounce, transient rejection, release debounce, note transitions, and semitone hysteresis.

The generated-signal tests are deterministic. They do not replace microphone, room-noise, end-to-end latency, or musical-performance testing.

## Continuous integration

GitHub Actions configures and compiles the Windows x64 Release build and runs CTest on pushes and pull requests targeting `main` or `dev/**`. The workflow is in `.github/workflows/windows-build.yml`.

## Engineering notes

- The audio callback now performs bounded channel mixing and FIFO writes only; pitch detection runs on a dedicated worker thread.
- The FIFO is preallocated and lock-free for the single audio producer and single analysis consumer. If it fills, excess incoming samples are counted and dropped rather than blocking the audio callback.
- The MIDI note tracker consumes each published analysis frame once, applies onset/release confirmation, and uses a small semitone hysteresis margin.
- MIDI events are still dispatched by the GUI timer. A dedicated timestamped MIDI event queue remains a future improvement for tighter timing.
