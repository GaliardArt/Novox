# NOVOX — independent vocal instrument

NOVOX is an original, offline-first standalone application inspired by the broad workflow of vocal-to-MIDI tools. It does not contain or modify Dubler 2 code, assets, authentication, or licensing logic.

## Current milestone (0.1)
- JUCE standalone desktop shell
- Audio input callback
- Basic normalized-autocorrelation pitch detector
- Live note/frequency/confidence display
- Input gain control

## Planned milestones
1. Stabilize real-time pitch detection (YIN, confidence, smoothing, hysteresis).
2. Convert detected notes to MIDI note-on/note-off events.
3. Add selectable MIDI output and virtual MIDI routing (Windows: loopMIDI).
4. Add vocal trigger training and percussion classification.
5. Add scale quantization, octave controls, pitch bend and calibration.
6. Add presets, settings persistence, diagnostics and installer.

## Build on Windows
Requirements:
- Visual Studio 2022 with Desktop development with C++
- CMake 3.22+
- Git
- Windows 10/11

From a Developer PowerShell:
```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```
CMake fetches JUCE 8.0.6 from its official repository on first configure, so internet is needed for the initial dependency download. The application is designed to run offline after dependencies are available.

## Engineering notes
- Audio callbacks must not allocate memory, block, log, or touch GUI objects.
- This is an early learning scaffold, not production-ready low-latency software.
- The first pitch detector is a baseline and needs smoothing and improved voiced/unvoiced handling.
- MIDI output is the next implementation milestone.
