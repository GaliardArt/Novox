# NOVOX

**Version 1.2.0 — Native JUCE workspace**

NOVOX is an offline-first, standalone vocal instrument for turning voice into musical control data. It is written in C++ and uses JUCE for audio input, MIDI integration, and the desktop application.

## Native interface (1.2.0)

The redesigned Play, MIDI, Audio and Presets workspaces are implemented directly in JUCE/C++. The executable does not embed a browser or require WebView2; the former web files remain in the repository as visual references only.

## Stable release (1.0.0)

- JUCE standalone desktop application and microphone capture
- YIN-style fundamental-frequency estimation with confidence and RMS level
- Adjustable pitch sensitivity to trade quiet-voice tracking against noise rejection
- Optional JUCE noise gate on the pitch-analysis worker, with adjustable threshold strength and transparent bypass
- Pitch analysis moved off the audio callback to a dedicated worker thread
- Preallocated SPSC audio FIFO and sequence-stamped pitch snapshots
- Note onset/release confirmation and semitone hysteresis
- Configurable pitch smoothing and micro-variation deadband for MIDI Pitch Bend
- Dedicated MIDI dispatch thread fed by a bounded SPSC event queue
- Ordered asynchronous MIDI Note On/Off and 14-bit Pitch Bend delivery
- Configurable Pitch Bend enable/disable and range from ±1 to ±12 semitones
- Configurable MIDI output channel (1–16), captured per queued event
- Persistent local preferences for input gain, sensitivity, smoothing, Pitch Bend enable/range, MIDI channel, and MIDI output
- Saved MIDI output is automatically restored when that device is available; unavailable devices remain safely unselected

- Named local presets for complete instrument configurations, with save/update, load, and delete controls
- Live audio-device diagnostics: selected input, active channel count, sample rate, and device buffer size
- Runtime counters for audio samples and MIDI events dropped by bounded real-time queues
- Presets include input gain, pitch sensitivity/smoothing, optional noise-gate state/strength, Pitch Bend settings, and melodic MIDI channel/output preference
- Automated pitch-detector, note-tracker, MIDI-queue, Pitch Bend, smoothing, and preset-storage regression tests

## Runtime diagnostics

The Audio tab reports the active audio input device, the number of enabled input channels, the current sample rate, and the device buffer size. It also displays cumulative counters for audio samples rejected when the audio FIFO or preallocated callback buffer cannot accept them, and MIDI events rejected when the bounded MIDI queue is full. These counters help distinguish a missing input device from signal-detection issues and expose overload instead of silently hiding it.

## Local presets

Presets are stored as individual versioned `.novoxpreset` JSON files in the current user's NOVOX application-data directory. The Presets tab can save or update a named preset, load it, and delete it. A preset captures input gain, pitch sensitivity, pitch smoothing, Pitch Bend enable/range, and melodic MIDI channel/output preference. Loading a preset updates the active controls and local application preferences. Presets remain local and work without an account or network connection. If a preset references an unavailable MIDI output, NOVOX keeps the saved device preference without silently selecting a different output.

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

The packaging commands create a portable ZIP named `NOVOX-1.2.0-Windows-x64.zip` in the build directory and an install layout under `dist/`. An optional Inno Setup definition is provided at `installer/NOVOX.iss`; compile it from the repository root after creating `dist/` if Inno Setup 6 is installed. The portable ZIP is the canonical package and does not require the installer.

## Regression coverage

- `NOVOXPitchTests`: known frequencies, 44.1/48 kHz sample rates, sensitivity response, silence/low-level rejection, harmonic-rich input, broadband noise, and invalid input.
- `NOVOXTrackingTests`: onset debounce, transient rejection, release debounce, note transitions, and semitone hysteresis.
- `NOVOXMidiQueueTests`: empty queue, FIFO order and payload for note and bend events, bounded-capacity overflow, recovery, and ring-buffer wraparound.
- `NOVOXPitchBendTests`: center, positive/negative direction, clamping, configurable range, invalid input, and fractional semitone offsets.
- `NOVOXPitchSmootherTests`: transparent mode, convergence, micro-variation rejection, reset, invalid values, and parameter clamping.

- `NOVOXPresetManagerTests`: preset-name sanitising, full settings round trip, value clamping, listing, and deletion.

The tests are deterministic. They do not replace microphone, room-noise, end-to-end latency, MIDI-driver, or musical-performance testing. Before publishing a stable binary, complete the manual checks in [docs/RELEASE_CHECKLIST.md](docs/RELEASE_CHECKLIST.md).

## Continuous integration

GitHub Actions configures and compiles the Windows x64 Release build and runs CTest on pushes and pull requests targeting `main` or `dev/**`. Workflow: `.github/workflows/windows-build.yml`.

## Engineering notes

- The audio callback performs bounded channel mixing and FIFO writes only; pitch detection runs on a worker thread.
- Pitch sensitivity is published atomically to the pitch worker. Higher values lower the RMS and confidence gates, allowing quieter signals through at the cost of admitting more noise.
- Optional noise reduction uses JUCE's lightweight noise gate on the worker thread before YIN analysis. It attenuates low-level input below a configurable threshold; it is not neural denoising and cannot remove steady background noise while a voice is active. It is disabled by default.
- Pitch smoothing is applied only to the MIDI bend control, not the raw detector display or note-onset logic.
- The smoothing control uses a one-pole response plus a small deadband. It suppresses small frame-to-frame fluctuations while larger pitch movement remains responsive; it is not intended to remove expressive vibrato completely.
- Audio FIFO overflow and callback blocks larger than the prepared scratch buffer are counted and dropped without blocking the callback.
- The MIDI queue is bounded and lock-free for one GUI producer and one MIDI dispatch consumer. The worker preserves event order and sends messages independently of the GUI timer.
- Each queued MIDI event captures its channel at enqueue time, so changing channels cannot redirect already queued note-offs.
- Pitch Bend is calculated from the fractional semitone offset relative to the tracked note and encoded using the standard 14-bit MIDI wheel value. The configured range is applied symmetrically, with asymmetric integer scaling around MIDI center 8192.
- Pitch Bend updates are sent only for new analysis frames and changes of at least two wheel units, limiting redundant queue traffic.
- Closing or changing MIDI output drains pending events and stops the dispatch worker before releasing the device handle.

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

## Version 1.1.0 — Modern UI

The interface is organized into four native JUCE tabs: Play, MIDI, Audio, and Presets. The redesign introduces a compact dark graphite theme with a restrained coral accent, a dedicated live pitch screen, separated MIDI routing controls, audio tuning and diagnostics, and a focused preset-management page. The audio engine, pitch detector, note tracker, MIDI routing, settings storage, and preset format remain unchanged.


## UI Design System 1.0

The shared native JUCE visual foundation uses a graphite palette, coral primary accent, teal secondary accent, consistent control styling, rounded surfaces, and clearer focus and interaction states. This layer is intentionally separate from the audio and MIDI processing pipeline; the Play, MIDI, Audio, and Presets workflows remain functionally unchanged while later interface iterations can refine each page independently.


## WebView2 interface foundation

The Windows desktop target embeds JUCE WebBrowserComponent using the Microsoft Edge WebView2 backend. The frontend in `UI/` is served locally by JUCE's resource provider; it does not load remote pages, CDN assets, or require internet at runtime. CMake downloads a pinned Microsoft.Web.WebView2 NuGet package during configuration when absent. Existing native Play, MIDI, Audio, and Presets tabs remain available during the gradual transition. The initial JavaScript bridge exposes a read-only `getNovoxStatus` function and a UI-ready event. Audio capture, pitch detection, and MIDI dispatch remain in native C++.


## Web UI — first visual pass

The first WebView2 visual pass introduced dedicated workspace navigation for Play, MIDI, Audio, and Presets, with read-only status snapshots. Subsequent MIDI bridge work now makes output selection, channel, and Pitch Bend controls interactive on the web MIDI page. The other web pages remain presentation-only while their native actions are integrated. All frontend assets are local and work without a remote server or CDN.


## Web MIDI controls

The WebView2 MIDI workspace now reads the available MIDI destinations and can refresh the device list, select or disconnect an output, change the MIDI channel, and enable/disable Pitch Bend or adjust its semitone range. These actions call validated native JUCE handlers; the C++ MIDI router remains responsible for device access and MIDI dispatch. Settings continue to use the existing local persistence path.


## Web UI visual redesign

The WebView2 workspace now follows the shared NOVOX design guide: a compact top tab bar, nested graphite surfaces (`#1C1C1E`, `#2C2C2E`, `#3A3A3C`), rounded panel hierarchy, restrained LED indicators, cyan complementary accents, coral pitch accents, and a 12-note pitch visualization. The layout is adapted for the melodic voice-to-MIDI workflow; drum triggers and formant controls from the generic reference are intentionally excluded. The frontend remains local and offline-capable.
