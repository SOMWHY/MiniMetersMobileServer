# MiniMetersMobile implementation plan

## Goal

Keep the existing MiniMeters shared-memory output and add a LAN WebSocket mirror that sends the same audio blocks to a future React Native client. The mobile client receives interleaved little-endian `float32` samples and calculates peak/RMS.

## Fixed decisions

- Default WebSocket port: `8422`.
- WebSocket path: `/ws`.
- Control messages: UTF-8 JSON text frames.
- Audio messages: binary frames, protocol version `1`.
- Audio samples: interleaved little-endian `float32`.
- Pairing credentials expire after 10 minutes.
- Existing authenticated connections survive credential refresh/expiry.
- Existing MiniMeters shared-memory path remains in place.
- The audio callback must not perform network I/O, allocation, locking, or JSON serialization.
- The desktop Python client is the temporary React Native replacement.

## Completed

- CMake/Ninja build and recursive submodules.
- Shared-memory audio path retained.
- Fixed-size audio mirror queue with bounds checks.
- Initial WebSocket server and binary frame format.
- WebSocket path validation, handshake buffering, authentication timeout, ping/pong, and failed-client cleanup.
- Primary-instance service start/stop handoff logic on non-audio threads.
- Pairing token generation, expiry, refresh, and LAN IPv4 discovery.
- QR code generation and editor integration.
- MiniMetersMobile editor layout without the upstream logo asset.
- Pairing expiry display and clipboard copy of the pairing payload.
- Formal pairing, control, and audio-frame protocol schemas.
- Desktop Python WebSocket/audio test client.
- Native CTest coverage for pairing tokens and the audio queue.
- Full Ninja build for Standalone, VST3, and CLAP.
- Build commits are kept separate by feature.

## Remaining automated work

- Decide and document the exact queue overflow policy for high-rate raw audio.
- Add stronger integration coverage for a valid authenticated WebSocket session and binary audio frame delivery.
- Add platform-specific LAN address tests where the host environment permits them.
- Review PluginVal-sensitive lifecycle paths and finalize release build settings.

## Manual validation gate

Automated implementation is paused here until the following actions are performed with the generated build:

- Open the Standalone and scan the QR code with a real phone.
- Run the desktop client with the copied pairing payload and verify valid-token audio delivery.
- Run the React Native client, once it exists.
- Load the VST3/CLAP in a real DAW and play known test signals.
- Test primary handoff with multiple DAW/plugin instances.
- Verify peak/RMS values against known signals.
- Run PluginVal at the required strictness.
- Verify long-running behavior on supported operating systems.

After manual validation, return to the remaining automated items with any observed protocol or lifecycle failures.
