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
3. Send an `authenticate` control message containing the QR token (and the `deviceToken` once one has been issued).
4. Receive `authenticated` (carrying the `deviceToken`) or `error`.
5. Receive binary audio frames until disconnect; `heartbeat` messages arrive while no audio flows.

The QR token is short-lived and only authenticates *new* pairings. On the first successful handshake the server issues a long-lived **device token**; the phone stores it and presents it on every later connect, so an expired QR code never disconnects a paired phone. Device tokens are revoked by the editor's **Forget devices** button.

Binary frames may carry more than one DAW block: the server coalesces everything queued between ticks. See `audio_frame.md`.

## Audio source

The plugin is a pass-through: it forwards whatever reaches `processBlock`. In a DAW that is the host's audio. In the **standalone** build the source is the standalone's audio input or its “Audio input / Play file” settings — with no input and no file it still streams (silence), so the client connects and draws flat meters.

## Desktop test client

Until the React Native client exists, the standard-library Python client can verify the connection and audio frames:

```powershell
python tools/test_client.py --address 192.168.1.20 --token <token>
```

Copy the address and token from the plugin editor. The client performs the same WebSocket handshake and authentication as the future mobile client, then calculates and prints peak/RMS values from each audio frame.

## Versioning

The initial protocol version is `1`.

Machine-readable definitions are provided in:

- `pairing.schema.json`
- `control.schema.json`
- `audio_frame.schema.json` (the binary header represented as fields)
