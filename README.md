# NOVOX

NOVOX is an offline-first, standalone vocal instrument for turning voice into musical control data. It is written in C++ and uses JUCE for audio input, MIDI integration, and the desktop application.

## Current milestone (0.2)

- JUCE standalone desktop application
- Microphone/audio input capture
- YIN-style fundamental-frequency estimation with confidence and RMS level
- Live note, frequency, and input-level display
- Input gain control
- Selectable MIDI output
- Stable-note filtering before MIDI note-on/note-off events

## Roadmap

1. Improve pitch detection with test signals, noise rejection, and calibration.
2. Move note tracking into a dedicated testable module and refine note onset/offset behavior.
3. Add pitch bend and configurable MIDI channel/velocity.
4. Add scale quantization, octave range, and note hold controls.
5. Add vocal trigger training and percussion classification.
6. Add local presets, diagnostics, and a Windows installer.

## Build on Windows

Requirements: Visual Studio 2022 with Desktop development with C++, CMake 3.22+, Git, Windows 10/11.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
```

CMake fetches JUCE 8.0.6 on the first configure. An internet connection is needed for that dependency download. The application is designed to run offline after dependencies are available.

## Continuous integration

GitHub Actions configures and compiles the Windows x64 Release build on pushes and pull requests targeting `main` or `dev/**`. The workflow is in `.github/workflows/windows-build.yml`.

A successful CI build confirms configuration and compilation on the hosted runner. Hardware tests with a real microphone and MIDI destination remain necessary.

## Engineering notes

- Keep audio callbacks real-time safe: avoid allocations, blocking locks, logging, and GUI access.
- The current pitch detector is an early YIN-style implementation and needs signal-based regression tests before production use.
- MIDI events are currently dispatched by the GUI timer; a dedicated timestamped MIDI event queue is planned to improve timing.
