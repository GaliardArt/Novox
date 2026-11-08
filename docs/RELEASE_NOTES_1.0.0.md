# NOVOX 1.0.0 — Stable release

NOVOX 1.0.0 is the first stable milestone for the standalone Windows voice-to-MIDI instrument.

## Included

- YIN-style monophonic pitch estimation with confidence and input-level feedback.
- Adjustable input gain and pitch sensitivity.
- Stable note onset/release tracking with semitone hysteresis.
- Configurable Pitch Bend with adjustable ±1 to ±12 semitone range.
- Pitch smoothing for continuous MIDI bend response.
- Asynchronous MIDI output with configurable melodic channel.
- Optional vocal kick, snare, and hi-hat detection with General MIDI channel 10 mapping.
- Locally stored application preferences and named presets.
- Live audio-device and dropped-event diagnostics.
- Deterministic CTest regression coverage for pitch, tracking, MIDI queue, bend, smoothing, percussion, drum mapping, and presets.

## Distribution

- Windows x64 portable ZIP package.
- Optional Inno Setup 6 installer definition.
- Offline runtime; first-time source configuration downloads JUCE 8.0.6 through CMake.

## Known limitations

- Pitch detection is monophonic.
- MIDI event generation is not sample-accurate.
- Vocal percussion recognition is heuristic and may vary by voice, microphone, room, and gain.
- Real-world microphone, DAW, MIDI driver, latency, and long-duration stability checks must be completed on target Windows systems before public distribution.
