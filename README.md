# NOVOX

NOVOX is an offline-first, standalone vocal instrument for turning voice into musical control data. It is written in C++ and uses JUCE for audio input, MIDI integration, and the desktop application.

## Current milestone (0.2.1)

- JUCE standalone desktop application
- Microphone/audio input capture
- YIN-style fundamental-frequency estimation with confidence and RMS level
- Live note, frequency, and input-level display
- Input gain control
- Selectable MIDI output
- Stable-note filtering before MIDI note-on/note-off events
- Automated pitch-detector regression tests through CTest

## Build on Windows

Requirements: Visual Studio 2022 with Desktop development with C++, CMake 3.22+, Git, Windows 10/11.

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

CMake fetches JUCE 8.0.6 on the first configure. An internet connection is needed for that dependency download. The application is designed to run offline after dependencies are available.

## Regression coverage

The `NOVOXPitchTests` target validates known sine-wave frequencies across the supported vocal range, sample-rate handling at 44.1/48 kHz, silence and low-level rejection, harmonic-rich input, broadband-noise rejection, and invalid input handling.

The tests use generated deterministic signals; they do not replace microphone, room-noise, latency, or musical-performance testing.

## Continuous integration

GitHub Actions configures and compiles the Windows x64 Release build and runs CTest on pushes and pull requests targeting `main` or `dev/**`. The workflow is in `.github/workflows/windows-build.yml`.

## Engineering notes

- Keep audio callbacks real-time safe: avoid allocations, blocking locks, logging, and GUI access.
- The pitch detector remains an early YIN-style implementation; regression coverage is a foundation, not a production-quality certification.
- MIDI events are currently dispatched by the GUI timer; a dedicated timestamped MIDI event queue is planned to improve timing.
