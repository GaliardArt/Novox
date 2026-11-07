# Third-party installation package audit

This note records a static inventory of the supplied supplementary installation archive. No executable or DLL was launched.

## Archive integrity and inventory

- ZIP integrity test: passed.
- Filesystem extraction paths: no absolute or parent-traversal paths found.
- The archive contains Windows PE binaries, eight OGG audio samples, installer/uninstaller data, and a MIDI virtual-port DLL.
- The PE files inspected are 32-bit x86. This is a compatibility warning for a native x64 NOVOX build, not evidence that every library is unusable in every process.

## Findings

| File/group | Identification | NOVOX decision |
|---|---|---|
| `teVirtualMIDI32.dll` | 32-bit DLL exporting Tobias Erichsen virtualMIDI API functions, including port create/close, send data, and driver-version queries | Do not bundle. Use JUCE MIDI output and let users install/configure a virtual MIDI endpoint separately if required. |
| `libEGL.dll`, `libGLESV2.dll`, `opengl32sw.dll`, `D3Dcompiler_47.dll` | Graphics/rendering runtime components | Not audio dependencies; JUCE desktop UI does not currently require these files to be copied from this archive. |
| `libcrypto-1_1.dll`, `libssl-1_1.dll` | OpenSSL 1.1 runtime | No current network/TLS requirement; do not bundle. Reassess only if a documented feature needs TLS. |
| `BugSplat.dll`, `BugSplatHD.exe`, `BugSplatRc.dll`, `BsSndRpt.exe` | Crash reporting/diagnostic components | Not required for the current app. Consider an independent crash-reporting strategy later. |
| `dbghelp.dll` | Windows debugging/symbol helper library | Not required for the current feature set. Avoid shipping a private copy without a concrete need. |
| `Resources/audio/808/*.ogg` | Eight short percussion audio samples; filenames correspond to General MIDI drum note numbers | Useful as evidence that a sample-based percussion module may be appropriate. Do not copy samples into NOVOX until redistribution rights are established; create original test samples or use properly licensed assets. |
| `unins000.exe`, `unins000.dat`, `unins000.msg` | Installer/uninstaller support files | Specific to the original installer; not reusable. |

## virtualMIDI detail

The DLL export table identifies a 32-bit interface with functions such as `virtualMIDICreatePort`, `virtualMIDIClosePort`, `virtualMIDISendData`, `virtualMIDIGetData`, and `virtualMIDIGetDriverVersion`. The archive does not contain the associated kernel driver files. Copying this DLL alone would not provide a complete virtual MIDI endpoint, and a 32-bit DLL cannot be loaded directly into the x64 NOVOX process.

## Audio observations

The OGG files are short samples with durations between approximately 0.15 and 2.22 seconds. Their filenames include GM percussion note numbers such as 36 (kick), 38 (snare), 39 (hand clap), and 42 (closed hi-hat). These are sample assets, not general-purpose audio libraries or codecs.

## Security and licensing

- Static inspection only; no files were executed.
- SHA-256 hashes were recorded during local audit, but this report does not claim a malware verdict or verify publisher signatures.
- No supplied binary, sample, installer component, or proprietary asset has been added to the NOVOX source tree.
- Before distributing any third-party dependency, verify its source, version, license, architecture, and redistribution terms.
