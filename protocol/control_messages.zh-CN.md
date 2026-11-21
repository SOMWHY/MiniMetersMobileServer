[English](control_messages.md) · **简体中文**

# 控制消息

所有控制消息都是 UTF-8 JSON 文本 WebSocket 帧。

## 配对内容

二维码里是一个按 UTF-8 编码的 JSON 对象：

```json
{
  "version": 1,
  "address": "192.168.1.20",
  "port": 8422,
  "path": "/ws",
  "token": "b7f1c9d2c0e44f3a8a1d5e6f7a8b9c0d"
}
```

二维码内容恰好是这五个键。下面说的长期设备令牌**不在**二维码里 —— 手机是在首次认证成功时通过
socket 收到它。

## 认证请求

```json
{
  "type": "authenticate",
  "version": 1,
  "token": "b7f1c9d2c0e44f3a8a1d5e6f7a8b9c0d",
  "deviceToken": "5f0c6a1d9e2b47c8a3d4e5f60718293a"
}
```

`token` 是二维码令牌。`deviceToken` 可选，只有配对过的手机才会带上；正是它让已知手机在二维码
过期后仍能连上。

### 服务端接受哪些组合

| `token`（二维码） | `deviceToken` | 结果 |
|---|---|---|
| 有效 | 任意 | 接受；若设备令牌未知则（重新）签发一个 |
| 刚被轮换掉（≤ 120 s） | 任意 | 接受 —— 容忍刚好卡在刷新边界上的扫码 |
| 过期 / 错误 | 未知 | 以 `invalid_token` 拒绝 |
| 过期 / 错误 | 已知 | 接受 |

二维码是一张一次性门票：首次握手会给手机签发它自己的设备令牌，之后连接改用这个令牌。令牌在插件
重启后依然有效，保存在用户应用数据目录下的 `MiniMetersMobile/device_tokens.txt`；界面的
**Forget devices** 按钮会把它们全部作废。

## 认证成功

```json
{
  "type": "authenticated",
  "version": 1,
  "sampleFormat": "f32le",
  "deviceToken": "5f0c6a1d9e2b47c8a3d4e5f60718293a",
  "heartbeatIntervalMs": 5000
}
```

`deviceToken` 是之后每次连接都要出示的凭据。`heartbeatIntervalMs` 是没有音频流动时应该多久收到
一次心跳。

## 心跳

没有音频可发时由服务端送出，避免链路上的安静被误判成断线：

```json
{ "type": "heartbeat", "version": 1, "timestampMs": 1732140800123 }
```

手机端在 `heartbeatIntervalMs * 3` 时间内没收到任何消息，就应当判定连接已断并重连。

## 错误

```json
{
  "type": "error",
  "code": "invalid_token",
  "message": "Pairing token is invalid or expired"
}
```

目前定义的错误码有 `invalid_token`、`unsupported_version` 和 `server_error`。
