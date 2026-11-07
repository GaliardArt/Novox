# NOVOX architecture

## Application overview

```text
MainComponent (GUI / 30 Hz refresh)
       |
       +---- AudioDeviceManager
       |          |
       |          +---- VoiceEngine (audio callback)
       |                    |
       |                    +---- PitchDetector
       |                    |
       |                    +---- latest PitchResult
       |
       +---- future NoteTracker
       +---- future MidiRouter
       +---- future TriggerTrainer
       +---- future SettingsStore
```

## Core modules

- `AudioInput`: device selection, channel mapping, sample-rate management.
- `PitchDetector`: YIN/NSDF-based monophonic fundamental estimation.
- `NoteTracker`: note onset/offset, hysteresis, smoothing, and vibrato tolerance.
- `MidiRouter`: MIDI note events, channels, pitch bend, and output device selection.
- `TriggerTrainer`: user-recorded trigger samples and feature extraction.
- `PercussionClassifier`: classify trained vocal sounds into trigger slots.
- `ScaleQuantizer`: optional scale snapping and octave mapping.
- `SettingsStore`: local settings and presets without account or cloud dependency.
- `MainComponent`: visual feedback and controls.

## Design principles

1. **Standalone first:** the application must launch and perform its core functions without an account or network connection.
2. **Real-time safety:** the audio callback performs bounded work and does not interact directly with the GUI.
3. **Independent modules:** signal analysis, note tracking, MIDI routing, and presentation remain separable and testable.
4. **User control:** input devices, sensitivity, pitch range, scale, octave, and MIDI destinations are configurable.
5. **Observable behavior:** diagnostics expose input level, detected pitch, confidence, and MIDI event state.

## Current implementation status

The first milestone provides a JUCE desktop shell, audio capture, a baseline autocorrelation pitch detector, and a live note/frequency/confidence display. The next milestones are a robust YIN detector, note tracking, and MIDI output.
