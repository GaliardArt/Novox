# NOVOX quick start

NOVOX turns a monophonic vocal line into MIDI notes and optional Pitch Bend. It can also recognize vocal percussion-like sounds and send mapped drum notes. It is a standalone Windows application; it does not require an account or network connection during use.

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
- **Enable Pitch Bend** turns continuous pitch-wheel output on or off.
- **Pitch Bend range** must match the receiving instrument's pitch-bend range for bends to sound in tune. The range is configurable from ±1 to ±12 semitones.
- **MIDI channel** selects the channel for melodic notes and Pitch Bend.

## Vocal percussion

Percussion MIDI is off by default. Enable **Send percussion to MIDI (channel 10)** to send detected kick, snare, and hi-hat events on General MIDI channel 10. The three note assignments can be changed independently. The receiving instrument must support the selected GM percussion notes.

The percussion detector is heuristic. Its results depend on vocal technique, microphone response, room noise, and gain; it is not a replacement for a dedicated drum trigger or sample-based drum instrument.

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

**Drum notes do not sound**
- Enable percussion MIDI in NOVOX.
- Confirm that the receiving instrument maps the selected notes to percussion and listens on channel 10.

## Known limitations

- Pitch tracking is monophonic; simultaneous sung notes/chords are not separated.
- MIDI event generation is GUI-timer-driven and is not sample-accurate.
- Vocal percussion classification is heuristic and requires calibration to the user's voice and microphone.
- The automated tests use deterministic synthetic signals and do not certify every microphone, audio driver, MIDI device, or DAW configuration.
