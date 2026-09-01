# MicVST Virtual Microphone Driver

This directory contains the reproducible build/integration layer for the dedicated MicVST Windows virtual audio driver.

The driver is intentionally kept separate from the GPL-3.0 JUCE application. It is prepared from the MIT/MS-PL-licensed VirtualDrivers/Virtual-Audio-Driver source at a pinned upstream commit, then patched for MicVST.

## Architecture

```text
Physical microphone
      ↓
MicVST / JUCE / VST3 / built-in DSP
      ↓
MicVST Internal Output   (private render endpoint)
      ↓
[ kernel ring buffer inside the MicVST driver ]
      ↓
MicVST Microphone        (capture endpoint)
      ↓
Discord / OBS / Zoom / browser / games
```

The render and capture endpoints are both constrained to 48 kHz / stereo / 32-bit PCM at the WaveRT boundary. Windows shared-mode audio handles app-side format conversion as needed. The ring buffer drops the oldest bytes on overflow so latency cannot grow without bound, and returns silence on underflow.

## Prepare the driver source

Requires Git for Windows.

```powershell
./driver/prepare-driver.ps1
```

This clones the pinned upstream source into `driver/work/Virtual-Audio-Driver`, verifies the commit, and applies `driver/patches/micvst-routing.patch`.

## Build

Requires Visual Studio 2022 with Desktop C++ and Windows Driver Kit 11.

```powershell
./driver/build-driver.ps1 -Configuration Release -Platform x64
```

The upstream solution/package project is used, so output locations follow the upstream project layout.

## Development installation

The current driver build is not production-signed. Development installation therefore requires Windows test-signing or a locally trusted test certificate. Production distribution will require a proper Microsoft-compatible driver-signing path.

The planned final installer will install the application and the driver together; users should not need VB-CABLE, VoiceMeeter, Device Manager, or a separate routing application.

## Upstream / licensing

Pinned upstream:

- Repository: `VirtualDrivers/Virtual-Audio-Driver`
- Commit: `bb34fba15faf569a6ae9bdea360bc1cf4821354e`

See `driver/THIRD_PARTY_NOTICES.md`. The upstream project itself contains Microsoft sample-derived components under MS-PL. Do not assume the application GPL license applies to the driver component.
