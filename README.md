**English** · [简体中文](README.zh-CN.md)

# MiniMetersMobile

A JUCE audio server that streams raw audio from your DAW (or from its own standalone build) to the
[MiniMeters Mobile](https://github.com/SOMWHY/minimeters-mobile) app over your local network. The
phone draws waveform, spectrum, spectrogram, VU, LUFS and oscilloscope views with Skia; this
project only captures audio and serves it.

```
DAW ──▶ audio callback ──▶ AudioBlockQueue ──▶ WebSocket :8422/ws ──▶ phone
```

## Download

Windows builds (VST3 + CLAP + standalone) are on the
[releases page](https://github.com/SOMWHY/MiniMetersMobileServer/releases/latest). Install
instructions: [`INSTALL.md`](INSTALL.md).

macOS and Linux builds are produced by the build scripts but are **not part of the current
release**. Expect them to need their own verification pass first.

## Formats

| Format | Status |
| --- | --- |
| VST3 | published (Windows) |
| CLAP | published (Windows) |
| Standalone | published (Windows) |
| AU | builds on macOS, not released |

The plugin is an effect in the `Fx` / `Tools` category. It does not alter audio: the input is
passed through untouched, it is only tapped and queued.

## How pairing works

The editor shows a QR code and the same values as text — `Address`, `Path` (`/ws`), `Token` and an
expiry countdown (120 s). The phone scans it or you type it in. The first successful handshake
issues a **device token** so later launches reconnect silently; the QR token is short-lived, the
device token is not.

Window controls:

- **Refresh pairing** — issue a new QR token, keep existing device tokens
- **Copy pairing** — copy the QR payload to the clipboard (handy for testing)
- **Forget devices** — invalidate every device token

Status line: audio source, `Network` (whether the WebSocket port is bound), `Clients`, `Paired`.

Windows Firewall must allow inbound **TCP 8422**.

## Build from source

Requirements: CMake 3.15+, a C++20 compiler, git submodules.

```bash
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

or use the platform script:

```bash
./build_windows.cmd      # releases/MiniMetersMobile-<version>-windows.zip
./build_macos.sh
./build_linux.sh
```

Unit tests:

```bash
cmake -B build-tests -DMINIMETERS_BUILD_TESTS=ON
cmake --build build-tests --config Release
cd build-tests && ctest -C Release --output-on-failure
```

## Protocol

Versioned JSON over WebSocket at `/ws`. Two messages matter for compatibility:

- pairing payload: `{"address", "port", "path", "token", "version": 1}`
- `authenticated`: carries the negotiated `version` and the accepted state

Any field change requires a version bump and a paired app release. The plugin reports its own
version and the minimum app version it accepts; see `docs/RELEASING.md`.

## Credits

This project is a fork of [`direct-audio/MiniMetersServer`](https://github.com/direct-audio/MiniMetersServer)
by Joseph Lyncheski, and keeps that upstream history intact. The original work is Copyright (C)
2022 Joseph Lyncheski and licensed under the GNU GPL v3; [`NOTICE`](NOTICE) records the copyright
and modification statements.

What this fork changed, on top of upstream `6a21e02` (`git log --oneline 6a21e02..HEAD` is the
full list):

- **Pairing and transport** — the pairing token is shared between the editor and the WebSocket
  server, failed revocation is reported instead of swallowed, and the network service survives
  handoff between services (`2146331`, `e1da769`, `648c3db`).
- **Realtime streaming** — broadcasts are throttled to 30 Hz, every queued block is merged into each
  frame, and Nagle's algorithm is disabled so audio is not held back by the socket (`5a7315c`,
  `b2c5bf7`, `fcac876`).
- **WebSocket robustness** — concurrent read and write is serialised, idle authenticated clients are
  kept alive, and handshake and frame validation were tightened (`c49116f`, `6cb44b1`, `127479c`,
  `a986dd9`).
- **Plugin identity and UI** — the plugin target, bundle identity and vendor name are
  `MiniMetersMobile` / `somwhy`, and the editor is built around a large QR code with a dedicated
  status row (`d1cbe25`, `3b1c99a`, `7803e7f`, `da28f02`).
- **Mobile bridge** — QR pairing display, a desktop WebSocket test client, and native pairing and
  audio-queue tests (`e879f00`, `837b5cc`, `f46c6b7`).
- **Packaging** — Windows VST3 / CLAP / standalone builds with a one-click installer and published
  checksums (`0d390bf`, `07df4f4`, `91737ce`).

## Licence

GNU General Public License v3 — see [`LICENSE`](LICENSE); copyright and modification statements are
in [`NOTICE`](NOTICE). The `LICENSE` file is reproduced verbatim and is never appended to. Because
this project uses JUCE under its GPLv3 option, the corresponding source must stay available: that is
this repository. Third-party components: [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) — in particular the protocol section before touching
anything under `protocol/`.
