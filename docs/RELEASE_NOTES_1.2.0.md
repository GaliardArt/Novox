# NOVOX 1.2.0 — Native JUCE Workspace

## Interface
- Rebuilt the modern NOVOX workspace directly in JUCE/C++.
- Added native Play, MIDI, Audio and Presets navigation and page layouts.
- Recreated the dark graphite visual system with coral, teal and green accents, rounded panels, live status indicators and a native pitch-class dial.
- Added native MIDI output/channel/Pitch Bend controls, audio parameter sliders and local preset management.

## Architecture
- Removed the embedded WebView2 runtime, JavaScript bridge and UI asset packaging from the application.
- Audio capture, YIN pitch detection, note tracking and MIDI dispatch remain in the existing C++ engine.
- All interface rendering and interaction now run through JUCE components and Graphics.
- No network, account, cloud or browser dependency is required.
