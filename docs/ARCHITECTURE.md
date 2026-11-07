# NOVOX architecture notes

## Independent application architecture

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
       +---- future MidiRouter
       +---- future TriggerTrainer
       +---- future SettingsStore
```

## Static observations from the supplied Dubler 2 executable

Static strings and RTTI-like names in the uploaded executable indicate:
- Qt 5.15.5 and QML resources are present.
- JUCE C++ class symbols are also present (for example `SynthesiserVoice`, `SamplerVoice`, `MidiOutput`, `PitchDetector`, `SynthVoice`).
- App-specific names include `LoginModel`, `TrialModel`, `checkFingerprint`, `getFingerprint`, `numMachinesMaxError`, `trialDaysSpent`, `totalTrialDays`, `PitchDetector`, `MidiOutput`, `TriggersMidiChannel`, `ChordsMidiChannel`, `pitchBendRange`, `pitchInputGain`, `voiceToSynthEnabled`, and `voiceToTriggersEnabled`.
- These are evidence of class/property names and feature areas, not proof of exact call order or internal algorithms.
- NOVOX is an independent implementation and does not reproduce proprietary login or licensing code.

## Proposed modules

- `AudioInput`: device selection, channel mapping, sample-rate management.
- `PitchDetector`: YIN/NSDF-based monophonic fundamental estimation.
- `NoteTracker`: note onset/offset, hysteresis, smoothing, vibrato tolerance.
- `MidiRouter`: MIDI note events, channels, pitch bend, output device.
- `TriggerTrainer`: user-recorded trigger samples and feature extraction.
- `PercussionClassifier`: classify trained vocal sounds into trigger slots.
- `ScaleQuantizer`: optional scale snapping and octave mapping.
- `SettingsStore`: local settings, no account or cloud dependency.
- `MainComponent`: visual feedback and controls.
