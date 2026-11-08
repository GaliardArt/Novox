# NOVOX 1.0.0 release checklist

This checklist separates automated build/test evidence from checks that require the actual Windows application and audio/MIDI hardware.

## Build and package

- [ ] Configure a clean Windows x64 Release build with Visual Studio 2022.
- [ ] Build all targets successfully.
- [ ] Run CTest and confirm every test passes.
- [ ] Create the portable ZIP with CPack.
- [ ] Extract the ZIP to a clean folder and launch `NOVOX.exe`.
- [ ] If publishing the installer, compile and test `installer/NOVOX.iss` with Inno Setup 6.
- [ ] Confirm application name and version report NOVOX 1.0.0.

## Audio and MIDI

- [ ] Confirm the expected microphone/input device is shown.
- [ ] Confirm sustained low, middle, and high vocal notes are tracked reliably.
- [ ] Check note onset, transitions, and release without stuck notes.
- [ ] Confirm MIDI Note On/Off reaches the selected DAW/device.
- [ ] Confirm Pitch Bend direction and range match the receiving instrument.
- [ ] Confirm output changes and application shutdown do not leave stuck notes.
- [ ] Confirm optional percussion mapping uses channel 10 and releases drum notes.
- [ ] Exercise the application for an extended session and observe dropped audio/MIDI counters.

## Persistence and recovery

- [ ] Change settings, close NOVOX, relaunch, and confirm settings persist.
- [ ] Save, load, update, and delete presets.
- [ ] Test startup when the previously selected MIDI device is disconnected.
- [ ] Reconnect the MIDI device, refresh the list, and select it.
- [ ] Confirm no preferences or presets are required from a network service.

## Publish

- [ ] Review the release notes and quick-start guide.
- [ ] Confirm all bundled files have redistribution rights.
- [ ] Record the final source commit and package checksum.
- [ ] Publish the 1.0.0 tag and attach the tested Windows x64 ZIP (and optional installer).
- [ ] Keep the development pull request unmerged until the maintainer explicitly approves integration.


## Automated GitHub Actions pipeline

- [ ] Confirm the Windows Build, Tests and Package workflow uploaded the portable ZIP artifact.
- [ ] Download and extract the artifact; launch NOVOX on a clean Windows x64 machine.
- [ ] Confirm the intended commit is approved before creating/pushing its version tag.
- [ ] Push a tag matching the CMake project version (for example `v1.0.0`).
- [ ] Confirm the Publish Windows Release workflow completed its own clean build and CTest run.
- [ ] Download the ZIP and SHA-256 file from the GitHub Release and verify the checksum.
- [ ] Test the exact release ZIP before announcing the release.
