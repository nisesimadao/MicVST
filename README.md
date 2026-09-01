# MicVST

**A lightweight Windows microphone effects rack with VST3 hosting, built-in voice effects, and a virtual microphone output.**

MicVST captures your physical microphone, processes it through an ordered chain of built-in DSP and VST3 plugins, then sends the result to the signed **VB-CABLE** virtual audio driver.

```text
Physical microphone
        ↓
      MicVST
  AutoTune / Robot / Radio / Bitcrusher / Pitch / Deep Voice / VST3...
        ↓
CABLE Input  (VB-CABLE playback endpoint)
        ↓
CABLE Output (VB-CABLE recording endpoint)
        ↓
Discord / OBS / Zoom / browser / games
```

No VoiceMeeter is required. The normal MicVST installer manages the VB-CABLE backend automatically, so the only device you normally choose inside MicVST is your physical microphone.

## Built-in effects

MicVST includes first-class effects that live in the same reorderable rack as VST3 plugins:

- **AutoTune** — key, scale, strength, retune speed and mix
- **Robot** — carrier/ring-mod style voice effect
- **Radio / Walkie-Talkie** — band limiting, crunch and static
- **Bitcrusher** — bit depth and sample-rate reduction
- **Pitch Shift** — ±12 semitones
- **Deep Voice** — downward pitch shift with warmth control
- **Mono → Stereo / Stereo → Mono** utility nodes

Built-ins support bypass, drag reorder, editor windows, and parameter persistence just like external VST3 effects.

## Recommended installation

Build or download `MicVST-Setup-*.exe` and run it as administrator.

The setup installs MicVST and, when necessary, installs the official base VB-CABLE package in the background. The VB-CABLE package is downloaded directly from VB-Audio's server, its Authenticode signature is verified before execution, and the original driver is not modified or re-signed.

A reboot may be required after the first VB-CABLE installation.

After that:

1. Open MicVST and choose your **physical microphone**.
2. Add/reorder built-in effects or VST3 plugins.
3. In Discord / OBS / Zoom / your game, choose **CABLE Output** as the microphone.

MicVST automatically routes its processed output to **CABLE Input**. The output selector is intentionally read-only so there is no manual cable routing to configure.

## VB-CABLE notice

VB-CABLE is made by **VB-Audio Software** and is separate third-party software. It is distributed as **donationware**; all contributions to VB-Audio are welcome.

- Official site: https://vb-cable.com/
- Licensing / distribution: https://vb-audio.com/Services/licensing.htm

VB-Audio permits the base VB-CABLE package to be distributed/embedded with another application when its donationware model and identity remain visible. MicVST therefore ships `installer/VB-CABLE-NOTICE.txt` and shows that notice during setup.

MicVST intentionally does **not** remove VB-CABLE when MicVST is uninstalled, because it is a shared third-party audio driver that another application may also use.

## Portable build

`MicVST.exe` can still be run as a portable application. In that case, install VB-CABLE separately first. If VB-CABLE is missing, MicVST leaves its output disconnected and shows `VB-CABLE missing / reboot required` rather than silently falling back to another driver.

## Features

- Physical mic → ordered **built-in DSP + VST3 chain** → VB-CABLE virtual microphone
- **Automatic VB-CABLE routing**; no VoiceMeeter configuration
- Channel-aware graph with Mono → Stereo / Stereo → Mono nodes
- Input/output level meters and live latency display
- Drag-to-reorder rack, bypass, removal confirmation, plugin editors
- Persistent plugin and built-in effect state
- Standard + custom VST3 folder scanning
- Out-of-process plugin scanning so broken VST3s cannot take down the main UI during discovery
- Tray mode and optional Windows autostart
- 48 kHz-oriented realtime voice processing

## Build the app

Requires Windows x64, Visual Studio 2022 with Desktop development with C++, Windows SDK, CMake, and Git. JUCE 8.0.13 is fetched automatically by CMake.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target MicVST MicVSTTests
.\build\MicVSTTests_artefacts\Release\MicVSTTests.exe
```

The application executable is generated at:

```text
build\MicVST_artefacts\Release\MicVST.exe
```

## Build the installer

Install Inno Setup 6, build the Release app first, then run:

```powershell
.\installer\build-installer.ps1
```

The installer is generated under:

```text
installer\out\MicVST-Setup-1.1.1.exe
```

The installer itself contains MicVST and the VB-CABLE bootstrap script/notice. The current base VB-CABLE ZIP is fetched from VB-Audio at install time, which avoids shipping a stale copy of the third-party driver.

## How the audio path works

MicVST uses JUCE `AudioDeviceManager` in WASAPI shared mode and an `AudioProcessorGraph`:

```text
WASAPI microphone input
       ↓
AudioProcessorGraph
       ↓
Built-in AudioProcessors + VST3 AudioPluginInstances
       ↓
WASAPI render to CABLE Input
       ↓
VB-CABLE signed kernel driver
       ↓
CABLE Output recording endpoint
```

The custom experimental MicVST driver work remains under `driver/` for future development, but the normal application does not depend on it. This keeps normal Windows security features such as Secure Boot available.

## Latency

MicVST does not add a large user-space queue. The main contributors are the Windows input/output periods, plugin-reported latency, and VB-CABLE's own buffering. The UI shows a live estimate based on the active device and graph latency.

For voice chat the one-way delay is generally not noticeable to the speaker. Smaller buffers matter more when monitoring your own processed voice in real time.

## Third-party software

- **JUCE** — used for the application/audio/VST3 framework.
- **VB-CABLE** — optional/shared third-party virtual audio driver used by the recommended MicVST setup. VB-CABLE remains under VB-Audio's own terms and donationware model.
- External VST3 plugins remain under their respective licenses.

## License

MicVST is licensed under the **GNU General Public License v3.0**; see [LICENSE](LICENSE).

The `driver/` experimental component has separate upstream/source licensing obligations documented in `driver/THIRD_PARTY_NOTICES.md`.

VB-CABLE is **not** relicensed under GPL and is not part of MicVST source code. See `installer/VB-CABLE-NOTICE.txt`.
