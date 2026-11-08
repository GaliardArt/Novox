<div align="center">

# NOVOX

### YOUR VOICE. YOUR INSTRUMENT.

**A real-time vocal-to-MIDI instrument for expressive music creation.**

[![Platform](https://img.shields.io/badge/platform-Windows-1f6feb?style=for-the-badge&logo=windows&logoColor=white)](https://www.microsoft.com/windows)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![JUCE](https://img.shields.io/badge/audio%20framework-JUCE-20232A?style=for-the-badge)](https://juce.com/)
[![CMake](https://img.shields.io/badge/build-CMake-064F8C?style=for-the-badge&logo=cmake&logoColor=white)](https://cmake.org/)

</div>

---

## The voice is an instrument.

**NOVOX turns vocal performance into MIDI control, giving your voice a direct connection to virtual instruments, DAWs, and musical workflows.**

Sing a melody into a microphone and NOVOX tracks its fundamental pitch, identifies musical notes, and translates vocal movement into MIDI messages. The result is a standalone desktop instrument designed to make vocal expression useful beyond the audio track itself.

Built in native **C++20** with **JUCE**, NOVOX brings audio capture, pitch analysis, note tracking, MIDI routing, and instrument controls together in one Windows application.

## From voice to MIDI

<div align="center">

**MICROPHONE** &nbsp; → &nbsp; **PITCH ANALYSIS** &nbsp; → &nbsp; **NOTE TRACKING** &nbsp; → &nbsp; **MIDI OUTPUT**

<sub>A real-time signal path designed for monophonic vocal performance.</sub>

</div>

NOVOX listens to the incoming audio, estimates its fundamental frequency, and follows note transitions. Stable notes are converted into MIDI Note On and Note Off messages, while continuous pitch movement can be expressed through MIDI Pitch Bend.

The processing pipeline is separated from the interface, keeping audio capture and analysis independent from presentation and user controls.

## What NOVOX does

### 🎙️ Vocal pitch tracking
- Real-time microphone and audio-device capture.
- YIN-style fundamental-frequency estimation.
- Pitch confidence and input-level monitoring.
- Adjustable sensitivity for balancing quiet-voice detection and noise rejection.
- Note-onset and note-release confirmation, with semitone hysteresis to reduce unstable transitions.

### 🎚️ Audio refinement
- Optional noise gate for attenuating low-level background sound.
- Two-second room-noise calibration that adapts the gate threshold to the measured noise floor.
- Automatic strongest-channel selection, with explicit Left, Right, and Stereo mix modes.
- Adjustable noise-gate strength and transparent bypass.
- Pitch smoothing for more controlled MIDI Pitch Bend movement.
- A micro-variation deadband to reduce unwanted pitch-wheel jitter while preserving expressive movement.

### 🎹 MIDI performance
- Selectable MIDI output devices.
- MIDI channel selection from 1 to 16.
- Ordered Note On and Note Off delivery.
- Optional MIDI Pitch Bend with a configurable range from ±1 to ±12 semitones.
- Continuous pitch expression derived from the tracked vocal pitch.

### ⚙️ Instrument controls
- Dedicated Play, MIDI, Audio, and Presets workspaces.
- Local preferences restored between sessions.
- Named presets for saving and recalling instrument configurations.
- Audio-device diagnostics and counters for dropped audio samples and MIDI events.

## Designed as a standalone instrument

NOVOX is intended to work as a self-contained desktop application. It does not require an account, cloud processing, or a remote service during normal use.

Audio capture, pitch analysis, note tracking, and MIDI dispatch run locally. Preferences and presets are also stored on the user's machine.

The application is built around a native JUCE interface and a C++ audio engine. Its interface is organized into focused workspaces so that performance, MIDI routing, audio configuration, and presets remain easy to access.

## Interface

| Workspace | Purpose |
|---|---|
| **Play** | Follow detected notes, frequency, pitch confidence, and instrument activity. |
| **MIDI** | Choose the MIDI destination, channel, and Pitch Bend behavior. |
| **Audio** | Configure input gain and sensitivity, select the input channel, calibrate optional noise reduction, and inspect device diagnostics. |
| **Presets** | Save, update, load, and delete local instrument configurations. |

## Under the hood

NOVOX uses a separated processing pipeline to keep time-sensitive audio work away from the graphical interface.

- **Audio callback:** performs bounded channel mixing and writes samples into a preallocated single-producer/single-consumer FIFO.
- **Pitch worker:** consumes audio blocks and runs pitch analysis outside the audio callback.
- **Note tracking:** confirms note transitions and applies hysteresis to reduce false retriggers.
- **MIDI dispatch:** sends queued MIDI events from a dedicated worker thread.
- **Local storage:** persists preferences and versioned `.novoxpreset` files on the machine.

This design helps avoid blocking operations in the audio callback and keeps MIDI event delivery ordered.

## Technology

| Technology | Responsibility |
|---|---|
| **C++20** | Core engine, signal processing, MIDI, and application logic |
| **JUCE 8** | Audio I/O, MIDI devices, DSP utilities, and native interface |
| **CMake** | Build system and dependency configuration |
| **CTest** | Automated regression testing |
| **GitHub Actions** | Windows build and test automation |

## Build on Windows

Requirements:

- Windows 10/11, 64-bit
- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.22 or newer
- Git

Configure, build, and run the automated tests:

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The first CMake configuration downloads the pinned JUCE dependency. After dependencies are available, NOVOX is designed to run offline.

## Testing and reliability

The project includes automated tests for pitch detection, note tracking, MIDI event queuing, Pitch Bend conversion, pitch smoothing, noise reduction, and preset storage.

These tests cover deterministic processing behavior. Real microphones, room acoustics, audio drivers, MIDI devices, and DAW routing still require practical testing on the target system.

---

<div align="center">

**NOVOX**

*Your voice. Translated into musical control.*

<sub>Native Windows application · C++20 · JUCE</sub>

</div>
