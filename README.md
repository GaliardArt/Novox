<div align="center">

# NOVOX

### YOUR VOICE. YOUR INSTRUMENT.

**Transform vocal expression into MIDI — directly from your desktop.**

[![Platform](https://img.shields.io/badge/platform-Windows-1f6feb?style=for-the-badge&logo=windows&logoColor=white)](https://www.microsoft.com/windows)
[![Language](https://img.shields.io/badge/language-C%2B%2B-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![Framework](https://img.shields.io/badge/framework-JUCE-20232A?style=for-the-badge)](https://juce.com/)
[![Build](https://img.shields.io/badge/build-CMake-064F8C?style=for-the-badge&logo=cmake&logoColor=white)](https://cmake.org/)

</div>

---

## What is NOVOX?

**NOVOX is a standalone vocal instrument built to turn the human voice into musical control.**

The idea is simple: sing a note, let NOVOX recognize its pitch, and use that information as the foundation for expressive MIDI performance. Instead of treating the microphone as just an audio input, NOVOX explores the voice as an instrument that can interact directly with music production software and MIDI-compatible equipment.

Designed with a focus on responsiveness, clarity, and independence, NOVOX is being developed as a native Windows desktop application. Its core is written in **C++**, with **JUCE** handling audio devices, MIDI integration, and the desktop application.

## The concept

NOVOX is centered around one creative workflow:

**Voice → Pitch Detection → Musical Note → MIDI**

A microphone captures a monophonic vocal signal. The pitch-analysis engine estimates its fundamental frequency and translates that information into musical feedback. This initial foundation establishes the signal path that future instrument capabilities can build upon.

The goal is to make vocal performance a practical and immediate way to control digital instruments.

## What this version contains

- **Live audio capture** — receives input from the selected audio device.
- **Pitch detection** — analyzes the incoming signal using normalized autocorrelation.
- **Real-time feedback** — displays the detected note, frequency, and confidence.
- **Input gain** — provides basic control over the signal level entering the analysis stage.
- **Native desktop foundation** — a standalone JUCE application, built with CMake.

> This branch represents the initial NOVOX foundation. Advanced note tracking, MIDI performance controls, and additional audio-processing features belong to the development branch.

## Built for independence

NOVOX is designed as a local desktop application. Its core workflow does not depend on a user account, cloud processing, or a remote service.

Audio analysis happens on the machine, keeping the project focused on a direct path between the microphone and musical control.

## Technology

| Component | Role |
|---|---|
| **C++** | Core application and audio-processing logic |
| **JUCE** | Audio devices, MIDI foundation, and desktop application |
| **CMake** | Build configuration and project generation |
| **Visual Studio** | Windows C++ compilation |

## Project status

NOVOX is an actively developed project. This branch contains its initial standalone application and pitch-detection foundation; it should be understood as an early technical baseline rather than a finished music-production tool.

---

<div align="center">

**NOVOX**

*Turn the voice into an instrument.*

<sub>Built with C++ and JUCE.</sub>

</div>
