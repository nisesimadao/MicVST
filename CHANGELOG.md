# Changelog

All notable changes to MicVST are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/), and this
project follows [Semantic Versioning](https://semver.org/).

## [1.4.0] - 2026-09-02

### Added
- **Audio Pads / Soundboard** with 16 pads in a collapsible 4x4 panel.
- Drag-and-drop and file picker loading for WAV, MP3, FLAC, OGG and AIFF audio.
- Simultaneous pad playback with realtime sample-rate conversion into the primary MicVST clock.
- Per-pad volume, loop, fade-in, fade-out and retrigger behaviour (`Restart`, `Stop`, `Ignore`).
- Three pad routing modes:
  - `Post FX` mixes after the DSP/VST chain and goes to the virtual microphone.
  - `Pre FX` mixes before the DSP/VST chain so the clip is processed by the same effects.
  - `Output2 only` is audible only on the local monitor and never enters `CABLE Input`.
- Global Windows pad hotkeys such as `F8`, `Numpad1`, `Ctrl+Shift+1` and `Alt+Q`, including while MicVST is hidden in the tray.
- Pad naming, playback progress, subtle color presets, master pad volume and per-pad context menus.
- Audio Pad state persistence including file path, route, hotkey, volume, loop, retrigger and fades.
- Audio Pad unit tests that generate a real WAV file and verify Post FX / Pre FX / Output2-only buses, looping, fade-stop and config round-trip.

### Changed
- App and installer version bumped to 1.4.0.
- Main How To guide now documents Audio Pads and the three routing modes.

## [1.3.0] - 2026-09-02

### Added
- Configurable **Output 2** for local monitoring of the fully processed MicVST signal
  through headphones, speakers, audio interfaces, HDMI/DP audio and other Windows outputs.
- Independent second WASAPI `AudioDeviceManager`, keeping the primary `CABLE Input`
  virtual-microphone route untouched.
- Lock-free SPSC monitor buffer with a small safety fill so the Output2 device cannot block
  MicVST's main realtime DSP/VST callback.
- Lightweight Output2 resampling and adaptive clock-drift correction for different device
  sample rates and independent hardware clocks.
- Output2 selection persistence, including preserving temporarily disconnected devices as
  unavailable instead of silently forgetting the setting.
- Output2 tests covering safety-buffer behaviour, stereo signal copying, 48 kHz -> 44.1 kHz
  monitoring and config round-trips.
- v1.3.0 release workflow publishing Setup, portable EXE and SHA256 checksums.

### Changed
- App and installer version bumped to 1.3.0.
- README now documents the difference between fixed `Output` and configurable `Output 2`,
  including monitoring latency and acoustic-feedback warnings.

## [1.2.0] - 2026-09-01

### Added
- Built-in **Wah / Auto Wah** with envelope-following, LFO and manual modes.
- Built-in **Unison** with 2-8 pitch-shifted voices, detune, stereo spread and per-voice stagger.
- Built-in **Chorus** with rate, depth, base delay, feedback, stereo phase spread and mix.
- Built-in **Delay** with stereo / ping-pong modes and low/high-cut filtering in the feedback path.
- Built-in **Reverb** with room size, decay, pre-delay, damping, stereo width and mix.
- DSP unit tests covering all built-ins, repeated finite/stability processing, state round-trips,
  delay echoes, reverb tails and 8-voice Unison.
- One-click v1.2.0 release workflow publishing Setup, portable EXE and SHA256 checksums.

### Changed
- App and installer version bumped to 1.2.0.
- README expanded in Japanese with concrete DSP parameters, use cases and example chains.
- GitHub update checks now target the `nisesimadao/MicVST` releases instead of upstream.

## [1.1.1] - 2026-07-27

### Added
- "Reset app (clear all data)..." in the Manage VST3 Folders menu: deletes all settings
  and the plugin cache, then closes MicVST for a fresh first-run setup.
- Scan diagnostics: skip reasons now include the scanner's exit code where relevant
  (e.g. "failed (exit 0xC0000005)"), and the scan log records why a file is rescanned.

### Fixed
- Plugins shipped as bundle folders (Acustica, Minimal Audio, UADx, ...) were rescanned
  on every start when vendor background services wrote into their bundles - the rescan
  check now tracks the plugin binary inside the bundle instead of the folder itself.
- A plugin that crashes the scanner mid-enumeration (e.g. Waves WaveShell) no longer
  loses everything: plugins found before the crash are kept and usable, and the file is
  marked "crashed (N plugin(s) rescued)" - use "Retry skipped plugins" to try again.

## [1.1.0] - 2026-07-22

### Added
- Searchable plugin picker: "+ Plugin" now opens a search box with live filtering
  (grouped by manufacturer when the search is empty). Built-ins are included.
- "Skip" button while scanning: skip the plugin that is currently being scanned.
- "Retry skipped plugins" (Manage VST3 Folders menu): rescans skipped plugins with a
  generous 10-minute timeout - lets huge shell plugins like Waves WaveShell finish.
- Buffer size dropdown (low-latency mode): when the selected devices support multiple
  buffer sizes, a "Buffer" row appears in the device panel ("Auto" = device default).

### Changed
- Scan timeout raised from 30 s to 120 s, so large shell plugins (e.g. Waves WaveShell)
  usually finish on the initial scan already.
- All standard VST3 locations are scanned (Program Files, %LOCALAPPDATA%, VST3_PATH).
- Audio devices now open in WASAPI low-latency mode when available (shared as fallback).

### Fixed
- Plugins that write into their own bundle during load (UAD, Acustica, ...) were
  rescanned on every start; their cache timestamp is now taken after the scan finishes.
- Plugin updates that change the internal plugin ID no longer cause endless rescans.
- Removing a custom folder like "C:\Plugins" no longer prunes plugins from sibling
  folders like "C:\Plugins2".
- Editing the plugin chain or the custom VST3 folders no longer resets the update-check
  consent and the saved window position in the config file.

## [1.0.3] - 2026-07-21

### Changed
- Window and tray icon now appear immediately; the VST3 scan runs in the background with
  visible progress (count + plugin name).
- "Manage custom VST3 Folders" is now "Manage VST3 Folders" and its menu also offers
  "Rescan all plugins" (previously only in the "+ Plugin" menu).

### Fixed
- A hanging or crashing VST3 no longer blocks startup invisibly: each plugin is scanned in an
  isolated helper process with a 30 s timeout and gets skipped (with a hint in the UI) instead.
- Scan results are cached (`plugin_cache.xml`) - startup no longer rescans every plugin.

## [1.0.2] - 2026-06-05

### Added
- Optional, **opt-in Auto-Update-Check** (off by default; one-time consent prompt on first
  start): checks the GitHub releases API once per startup, and on a newer version turns the
  version number into a download link and shows a one-time tray notification. No telemetry,
  no auto-installer.
- The window now **remembers its size and position** across restarts.
- Live **end-to-end latency** readout (now includes plugin/graph latency) shown right under
  the device list, together with the sample rate and buffer size.
- **Per-plugin latency** shown in each plugin row (left of "Bypass"), updated live.

### Changed
- Sample rate and buffer size are now **read-only** info (they're fixed by Windows shared
  audio anyway); the *Advanced* toggle and their dropdowns were removed.
- Smaller, more compact default window; extra height now extends the **plugin list**.

### Added (docs)
- README **Latency** section explaining the shared-audio path and why ASIO4All won't help.

## [1.0.1] - 2026-06-04

### Added
- **How To** screen - a quick setup guide with clickable links (download VB-Cable,
  "Check for updates on GitHub").
- Clickable **version number** in the main window (opens the GitHub repo).

### Changed
- Moved the status line (Active / sample rate / latency) out of the main window into the
  **Advanced** section of the device panel.

## [1.0.0] - 2026-06-04

### Added
- Initial public release.
- Route one microphone through a **VST3 plugin chain** into a **virtual audio cable**
  (VB-Cable, VoiceMeeter, Virtual Audio Cable), usable as a mic in any app.
- **Automatic cable detection** with an in-app download hint when none is installed.
- Built-in **Mono → Stereo** and **Stereo → Mono** nodes; channel-aware routing.
- **Drag-to-reorder** plugin list, per-row bypass, remove with confirmation, per-plugin
  editor windows.
- **Manage custom VST3 folders**; crash-resistant plugin scan (a plugin that crashes the
  scan is remembered and skipped next time).
- Horizontal **in/out level meters** with a dB scale.
- **Tray app** with silent autostart and an autostart checkbox.
- Persistent device + plugin settings (`%APPDATA%\MicVST\config.xml`).
- Portable, statically linked `.exe` (no installer, no Visual C++ Redistributable).
- GitHub Actions CI: build + unit tests on every push/PR, release build on tag.

[1.4.0]: https://github.com/nisesimadao/MicVST/releases/tag/v1.4.0
[1.3.0]: https://github.com/nisesimadao/MicVST/releases/tag/v1.3.0
[1.2.0]: https://github.com/nisesimadao/MicVST/releases/tag/v1.2.0
[1.1.1]: https://github.com/nisesimadao/MicVST/releases/tag/v1.1.1
[1.1.0]: https://github.com/philipz794/MicVST/releases/tag/v1.1.0
[1.0.3]: https://github.com/philipz794/MicVST/releases/tag/v1.0.3
[1.0.2]: https://github.com/philipz794/MicVST/releases/tag/v1.0.2
[1.0.1]: https://github.com/philipz794/MicVST/releases/tag/v1.0.1
[1.0.0]: https://github.com/philipz794/MicVST/releases/tag/v1.0.0
