# NOVOX

NOVOX is an offline-first, standalone vocal instrument for turning voice into musical control data. It is written in C++ and uses JUCE for audio input, MIDI integration, and the desktop application.

## Stable release (1.0.0)

- JUCE standalone desktop application and microphone capture
- YIN-style fundamental-frequency estimation with confidence and RMS level
- Adjustable pitch sensitivity to trade quiet-voice tracking against noise rejection
- Pitch analysis moved off the audio callback to a dedicated worker thread
- Preallocated SPSC audio FIFO and sequence-stamped pitch snapshots
- Note onset/release confirmation and semitone hysteresis
- Configurable pitch smoothing and micro-variation deadband for MIDI Pitch Bend
- Dedicated MIDI dispatch thread fed by a bounded SPSC event queue
- Ordered asynchronous MIDI Note On/Off and 14-bit Pitch Bend delivery
- Configurable Pitch Bend enable/disable and range from ±1 to ±12 semitones
- Configurable MIDI output channel (1–16), captured per queued event
- Persistent local preferences for input gain, sensitivity, smoothing, bend enable/range, MIDI channel/output, drum MIDI enable state, and drum note assignments
- Saved MIDI output is automatically restored when that device is available; unavailable devices remain safely unselected
- Real-time vocal percussion classification for kick, snare, and hi-hat with confidence feedback
- Optional MIDI drum triggering on General MIDI channel 10, with configurable kick/snare/hi-hat note assignments and velocity derived from detector confidence
- Named local presets for complete instrument configurations, with save/update, load, and delete controls
- Live audio-device diagnostics: selected input, active channel count, sample rate, and device buffer size
- Runtime counters for audio samples and MIDI events dropped by bounded real-time queues
- Presets include input gain, pitch sensitivity/smoothing, Pitch Bend settings, melodic MIDI channel/output preference, and percussion enable/mapping
- Automated pitch-detector, note-tracker, MIDI-queue, Pitch Bend, smoothing, drum-mapping, percussion, and preset-storage regression tests

## Runtime diagnostics

The main window reports the active audio input device, the number of enabled input channels, the current sample rate, and the device buffer size. It also displays cumulative counters for audio samples rejected when the audio FIFO or preallocated callback buffer cannot accept them, and MIDI events rejected when the bounded MIDI queue is full. These counters help distinguish a missing input device from signal-detection issues and expose overload instead of silently hiding it.

## Local presets

Presets are stored as individual versioned `.novoxpreset` JSON files in the current user's NOVOX application-data directory. The UI can save or update a named preset, load it, and delete it. A preset captures all instrument controls: input gain, pitch sensitivity, pitch smoothing, Pitch Bend enable/range, melodic MIDI channel/output preference, percussion MIDI enable state, and kick/snare/hi-hat note assignments. Loading a preset updates the active controls and the current local application preferences. Presets remain local and work without an account or network connection. If a preset references an unavailable MIDI output, NOVOX keeps the saved device preference without silently selecting a different output.

## MIDI drum mapping

Percussion MIDI is disabled by default. When enabled, kick/snare/hi-hat events are sent on fixed General MIDI channel 10, independently of the melodic channel and Pitch Bend. Default note assignments are 36 (Bass Drum 1), 38 (Acoustic Snare), and 42 (Closed Hi-Hat); each can be changed in the UI across the standard GM percussion range. Trigger velocity follows detection confidence. Notes are released after a short 90 ms gate, and pending notes are released when MIDI output changes, the feature is disabled, or the application closes.

## Vocal percussion detection

The lightweight transient classifier runs on the audio worker and provides the kick/snare/hi-hat events used by the MIDI mapper. It remains heuristic and requires real microphone/performance validation.

## Local settings

Preferences are stored in the current user's application settings directory using JUCE's local PropertiesFile support. Settings are written locally and restored on the next launch. NOVOX does not transmit preferences or require an account or network connection. If a saved MIDI output is not present at startup, NOVOX keeps the preference and waits for the user to reconnect or select an available device.

## Quick start

See [docs/QUICKSTART.md](docs/QUICKSTART.md) for microphone setup, MIDI routing, first-use guidance, and troubleshooting.

## Build and package on Windows

Requirements: Windows 10/11 x64, Visual Studio 2022 with Desktop development with C++, CMake 3.22+, Git, and the Microsoft Visual C++ 2015–2022 Redistributable (x64) on target machines.

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix dist
cpack --config build/CPackConfig.cmake -C Release
```

CMake fetches JUCE 8.0.6 on the first configure. An internet connection is needed for that dependency download. The application is designed to run offline after dependencies are available.

The packaging commands create a portable ZIP named `NOVOX-1.0.0-Windows-x64.zip` in the build directory and an install layout under `dist/`. An optional Inno Setup definition is provided at `installer/NOVOX.iss`; compile it from the repository root after creating `dist/` if Inno Setup 6 is installed. The portable ZIP is the canonical package and does not require the installer.

## Regression coverage

- `NOVOXPitchTests`: known frequencies, 44.1/48 kHz sample rates, sensitivity response, silence/low-level rejection, harmonic-rich input, broadband noise, and invalid input.
- `NOVOXTrackingTests`: onset debounce, transient rejection, release debounce, note transitions, and semitone hysteresis.
- `NOVOXMidiQueueTests`: empty queue, FIFO order and payload for note and bend events, bounded-capacity overflow, recovery, and ring-buffer wraparound.
- `NOVOXPitchBendTests`: center, positive/negative direction, clamping, configurable range, invalid input, and fractional semitone offsets.
- `NOVOXPitchSmootherTests`: transparent mode, convergence, micro-variation rejection, reset, invalid values, and parameter clamping.
- `NOVOXDrumMappingTests`: GM channel, default note assignments, range clamping, and note labels.
- `NOVOXPresetManagerTests`: preset-name sanitising, full settings round trip, value clamping, listing, and deletion.
- `NOVOXVocalPercussionTests`: synthetic kick/snare/hi-hat transients, silence, sustained-tone rejection, and invalid input.

The tests are deterministic. They do not replace microphone, room-noise, end-to-end latency, MIDI-driver, or musical-performance testing. Before publishing a stable binary, complete the manual checks in [docs/RELEASE_CHECKLIST.md](docs/RELEASE_CHECKLIST.md).

## Continuous integration

GitHub Actions configures and compiles the Windows x64 Release build and runs CTest on pushes and pull requests targeting `main` or `dev/**`. Workflow: `.github/workflows/windows-build.yml`.

## Engineering notes

- The audio callback performs bounded channel mixing and FIFO writes only; pitch detection runs on a worker thread.
- Pitch sensitivity is published atomically to the pitch worker. Higher values lower the RMS and confidence gates, allowing quieter signals through at the cost of admitting more noise.
- Pitch smoothing is applied only to the MIDI bend control, not the raw detector display or note-onset logic.
- The smoothing control uses a one-pole response plus a small deadband. It suppresses small frame-to-frame fluctuations while larger pitch movement remains responsive; it is not intended to remove expressive vibrato completely.
- Audio FIFO overflow and callback blocks larger than the prepared scratch buffer are counted and dropped without blocking the callback.
- The MIDI queue is bounded and lock-free for one GUI producer and one MIDI dispatch consumer. The worker preserves event order and sends messages independently of the GUI timer.
- Each queued MIDI event captures its channel at enqueue time, so changing channels cannot redirect already queued note-offs.
- Pitch Bend is calculated from the fractional semitone offset relative to the tracked note and encoded using the standard 14-bit MIDI wheel value. The configured range is applied symmetrically, with asymmetric integer scaling around MIDI center 8192.
- Pitch Bend updates are sent only for new analysis frames and changes of at least two wheel units, limiting redundant queue traffic.
- Closing or changing MIDI output drains pending events and stops the dispatch worker before releasing the device handle.
- MIDI event generation still originates from the GUI timer. This milestone does not claim sample-accurate timing.
- Preferences are saved immediately after a control change and on shutdown. The saved output device is matched by its JUCE device name; if no exact match exists, NOVOX does not silently select a different device.


## GitHub Actions: build and distribution

Two workflows automate the Windows x64 pipeline:

- `.github/workflows/windows-build.yml` runs on pushes and pull requests targeting `main` or `dev/**`. It configures CMake, builds Release, runs CTest, creates the install layout and portable ZIP, then uploads the ZIP as a downloadable Actions artifact for 14 days.
- `.github/workflows/release.yml` runs when a version tag matching `vMAJOR.MINOR.PATCH` is pushed (or can be started manually for an existing tag). It verifies that the tag matches the CMake project version, rebuilds and tests that exact tagged source, creates the portable ZIP and SHA-256 checksum, and publishes both files in a GitHub Release.

### Publishing a stable version

After the maintainer has approved the exact source commit for publication:

```powershell
git checkout dev/yin-midi
git pull
git tag v1.0.0
git push origin v1.0.0
```

The tag must point to the validated source commit and match `project(NOVOX VERSION ...)` in `CMakeLists.txt`. Pushing the tag starts the release workflow. The workflow does not merge branches or create/move tags. It publishes only after the clean Windows build and all CTest cases pass.

The workflow publishes the portable ZIP as the canonical package and includes its SHA-256 checksum. The optional Inno Setup installer remains a separate maintainer-controlled step; no third-party installer or driver is bundled automatically.
