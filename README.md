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

## Licence

GNU General Public License v3 — see [`LICENSE`](LICENSE). Because this project uses JUCE under its
GPLv3 option, the corresponding source must stay available: that is this repository.
Third-party components: [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) — in particular the protocol section before touching
anything under `protocol/`.
