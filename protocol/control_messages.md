**English** · [简体中文](control_messages.zh-CN.md)

# Control messages

All control messages are UTF-8 JSON text WebSocket frames.

## Pairing payload

The QR code will contain a JSON object encoded as UTF-8:

```json
{
  "version": 1,
  "address": "192.168.1.20",
  "port": 8422,
  "path": "/ws",
  "token": "b7f1c9d2c0e44f3a8a1d5e6f7a8b9c0d"
}
```

The QR payload has exactly these five keys. The long-lived device token described
below is **not** in the QR code — the phone receives it over the socket on its
first successful authentication.

## Authentication request

```json
{
  "type": "authenticate",
  "version": 1,
  "token": "b7f1c9d2c0e44f3a8a1d5e6f7a8b9c0d",
  "deviceToken": "5f0c6a1d9e2b47c8a3d4e5f60718293a"
}
```

`token` is the QR token. `deviceToken` is optional and only sent by a phone that
has paired before; it is what keeps a known phone connected after the QR code
expires.

### What the server accepts

| `token` (QR) | `deviceToken` | Result |
|---|---|---|
| live | any | accepted; the device token is (re)issued if unknown |
| just rotated out (≤ 120 s) | any | accepted — tolerates a scan that lands on the refresh boundary |
| expired / wrong | unknown | rejected with `invalid_token` |
| expired / wrong | known | accepted |

The QR code is a one-time door: the first handshake issues the phone its own
device token, and later connects present that instead. Tokens persist across
plugin restarts in `MiniMetersMobile/device_tokens.txt` inside the user
application-data directory, and the editor's **Forget devices** button revokes
them all.

## Authentication success

```json
{
  "type": "authenticated",
  "version": 1,
  "sampleFormat": "f32le",
  "deviceToken": "5f0c6a1d9e2b47c8a3d4e5f60718293a",
  "heartbeatIntervalMs": 5000
}
```

`deviceToken` is the credential to present on every later connect.
`heartbeatIntervalMs` is how often to expect a heartbeat while no audio is
flowing.

## Heartbeat

Sent by the server when there is no audio to publish, so silence on the wire is
never mistaken for a dead link:

```json
{ "type": "heartbeat", "version": 1, "timestampMs": 1732140800123 }
```

The phone should treat `heartbeatIntervalMs * 3` without any message as a broken
connection and reconnect.

## Error

```json
{
  "type": "error",
  "code": "invalid_token",
  "message": "Pairing token is invalid or expired"
}
```

Defined error codes currently include `invalid_token`, `unsupported_version`, and `server_error`.
