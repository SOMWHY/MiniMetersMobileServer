# MiniMeters Mobile Protocol

This directory is the protocol contract shared by the plugin and the future React Native client.

## Transport

- Transport: WebSocket over TCP.
- Default port: `8422`.
- Default path: `/ws`.
- Control messages are UTF-8 JSON text frames.
- Audio messages are binary WebSocket frames.
- The plugin remains the source of the audio stream; the client calculates peak and RMS.

## Connection sequence

1. Scan the pairing payload from the plugin editor.
2. Connect to `ws://<address>:<port>/ws`.
3. Send an `authenticate` control message containing the pairing token.
4. Receive `authenticated` or `error`.
5. Receive binary audio frames until disconnect.

The pairing token is short-lived and only authenticates new connections. Expiration does not terminate already authenticated connections.

## Versioning

The initial protocol version is `1`.
