# NOVOX quick start

NOVOX turns a monophonic vocal line into MIDI notes and optional Pitch Bend. It is a standalone Windows application; it does not require an account or network connection during use.

## Requirements

- Windows 10 or Windows 11, 64-bit.
- A working microphone or audio input.
- A MIDI destination, such as a DAW, hardware MIDI interface, or a separately installed virtual MIDI port.
- A compatible MIDI instrument loaded in the destination if you want to hear the generated notes.

## First launch

1. Extract the portable NOVOX ZIP to a folder you can write to, or run the optional installer.
2. Launch `NOVOX.exe`.
3. Check the **Audio input** diagnostic line. It should show the active input device, channel count, sample rate, and buffer size.
4. If no audio device is active, choose a working Windows input device and restart NOVOX.
5. Select the intended MIDI output in the MIDI output selector. Use **Refresh MIDI** after connecting or creating a MIDI destination.
6. In your DAW or MIDI destination, select the same endpoint as an input and arm/enable the receiving instrument.
7. Sing a clear, sustained single note. The detected note, frequency, confidence, and input level should update.

## Pitch and MIDI controls

- **Input gain** adjusts the signal level entering analysis.
- **Pitch sensitivity** trades quiet-voice tracking against rejection of background noise.
- **Pitch smoothing** affects the continuous MIDI Pitch Bend response; it does not smooth the raw pitch display or note-onset decisions.
- **Pitch Stability Guard** optionally holds brief downward pitch glitches and short unvoiced gaps before they reach the note display, Note Tracker, or Pitch Bend. Defaults are 45 cents and three UI frames. Persistent lower notes are accepted after the hold window; upward transitions remain immediate. Increase tolerance to catch larger dips, or increase hold only if glitches last longer. Excessive settings can delay genuine downward note changes.
- **Input channel** selects automatic strongest-channel detection, Left, Right, or Stereo mix. Automatic mode helps avoid phase cancellation when stereo channels contain different signals.
- **Noise reduction (gate)** optionally attenuates low-level background sound between vocal phrases. Increase its strength only until room noise is reduced; too much can cut quiet syllables or note tails. It is disabled by default and is not a neural background-noise remover.
- **Calibrate 2s** measures the room/microphone noise floor. Keep the microphone silent during the two-second measurement. The gate then references that measured floor; strength controls the suppression margin. Recalibrate after changing microphone gain or moving to another acoustic environment.
- **Enable Pitch Bend** turns continuous pitch-wheel output on or off.
- **Pitch Bend range** must match the receiving instrument's pitch-bend range for bends to sound in tune. The range is configurable from ±1 to ±12 semitones.
- **MIDI channel** selects the channel for melodic notes and Pitch Bend.

## Presets and local preferences

Use the local preset controls to save/update, load, and delete named instrument configurations. Application preferences and presets are stored locally in the current Windows user profile. They are not synchronized or transmitted.

If a saved MIDI output is unavailable, NOVOX leaves it unselected instead of silently routing to a different device. Reconnect the endpoint and refresh the MIDI list, or select another output explicitly.

## Troubleshooting

**No audio input shown**
- Check Windows microphone privacy permissions.
- Confirm that the intended input is enabled in Windows sound settings.
- Close applications that may exclusively hold the audio device, then restart NOVOX.

**Audio input is active but no note is detected**
- Sing one sustained note rather than chords.
- Check the input level and increase Input gain or Pitch sensitivity gradually.
- Reduce background noise and avoid clipping.

**No MIDI reaches the DAW**
- Confirm that NOVOX has a MIDI output selected.
- Confirm that the DAW is listening to that exact endpoint and that its track/instrument is armed.
- If using a virtual MIDI port, install and configure that endpoint separately; NOVOX does not bundle a virtual MIDI driver.

**Pitch Bend sounds incorrect**
- Set the receiving instrument's Pitch Bend range to the same value selected in NOVOX.
- Verify that the receiving instrument responds to Pitch Bend on the selected MIDI channel.

## Known limitations

- Pitch tracking is monophonic; simultaneous sung notes/chords are not separated.
- MIDI event generation is GUI-timer-driven and is not sample-accurate.
- The automated tests use deterministic synthetic signals and do not certify every microphone, audio driver, MIDI device, or DAW configuration.
