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
  "token": "..."
}
```

## Authentication request

```json
{
  "type": "authenticate",
  "version": 1,
  "token": "..."
}
```

## Authentication success

```json
{
  "type": "authenticated",
  "version": 1,
  "sampleFormat": "f32le",
  "channels": 2
}
```

## Error

```json
{
  "type": "error",
  "code": "invalid_token",
  "message": "Pairing token is invalid or expired"
}
```

Defined error codes currently include `invalid_token`, `unsupported_version`, and `server_error`.
