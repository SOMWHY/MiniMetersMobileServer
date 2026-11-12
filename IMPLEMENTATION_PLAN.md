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
- Fixed-size audio mirror queue.
- Initial WebSocket server and binary frame format.
- Pairing token generation and LAN IPv4 discovery.
- QR code generation and editor integration.
- MiniMetersMobile editor layout without the upstream logo asset.
- Desktop Python WebSocket/audio test client.
- Basic failed-client cleanup and authenticated client count.
- Build commits are kept separate by feature.

## Remaining automated work

1. Formalize protocol schemas and fixtures.
2. Make primary-instance ownership explicit for the network service.
3. Make service start/stop and primary handoff happen outside the audio callback.
4. Fix audio frame timestamps, bounds, and queue drop policy.
5. Harden WebSocket handshake, path validation, control frames, limits, and shutdown.
6. Add automated tests for pairing, LAN filtering, frame encoding, and protocol errors.
7. Improve pairing UI status, expiry, and copyable payload.
8. Run full Ninja builds and automated tests after each major change.

## Manual validation gate

Pause only when the automated work is complete and these actions are required:

- Scan the QR code with a real phone.
- Run the React Native client, once it exists.
- Load the VST3/CLAP in a real DAW and play known test signals.
- Test primary handoff with multiple DAW/plugin instances.
- Run PluginVal at the required strictness.
- Verify long-running behavior on supported operating systems.
