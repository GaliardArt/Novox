# NOVOX

NOVOX is an offline-first, standalone vocal instrument for turning voice into musical control data. The project is written in C++ and uses JUCE for audio input, MIDI integration, and the desktop application.

## Current milestone (0.1)

- JUCE standalone desktop application
- Audio input callback
- Baseline normalized-autocorrelation pitch detector
- Live note, frequency, and confidence display
- Input gain control

## Planned milestones

1. Improve real-time pitch detection with YIN, confidence scoring, smoothing, and hysteresis.
2. Convert stable note tracking into MIDI note-on/note-off events.
3. Add selectable MIDI outputs and virtual MIDI routing (Windows: loopMIDI).
4. Add vocal trigger training and percussion classification.
5. Add scale quantization, octave controls, pitch bend, and calibration.
6. Add presets, local settings, diagnostics, and an installer.

## Build locally on Windows

Requirements:

- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.22 or newer
- Git
- Windows 10/11

From a Developer PowerShell in the repository directory:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
```

CMake downloads JUCE 8.0.6 during the first configure. An internet connection is needed for this initial dependency download. Once dependencies are available, the application is designed to run offline.

## Continuous integration

GitHub Actions automatically configures and builds the Windows application whenever code is pushed or a pull request targets `main`. The workflow is defined in `.github/workflows/windows-build.yml`.

A green workflow means the current source successfully configured and compiled on the GitHub-hosted Windows runner. It does not replace testing with a physical microphone and MIDI destination.

## Engineering notes

- Keep audio callbacks real-time safe: avoid allocations, blocking locks, logging, and GUI access.
- The current pitch detector is an educational baseline, not yet production-ready.
- MIDI output and automated unit tests are planned for subsequent milestones.
