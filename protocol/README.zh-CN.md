[English](README.md) · **简体中文**

# MiniMeters Mobile 协议

本目录是插件与手机客户端之间的协议契约。

## 传输层

- 传输协议：WebSocket over TCP。
- 默认端口：`8422`。
- 默认路径：`/ws`。
- 控制消息是 UTF-8 JSON 文本帧。
- 音频消息是二进制 WebSocket 帧。
- 音频流的来源始终是插件；峰值和 RMS 由客户端自行计算。

## 连接顺序

1. 从插件界面扫出配对内容。
2. 连接 `ws://<address>:<port>/ws`。
3. 发送 `authenticate` 控制消息，带上二维码令牌（如果已签发过，再带上 `deviceToken`）。
4. 收到 `authenticated`（内含 `deviceToken`）或 `error`。
5. 持续接收二进制音频帧直到断开；没有音频流动时会收到 `heartbeat`。

二维码令牌是短期的，只用于认证**新的**配对。首次握手成功后服务端会签发一个长期有效的**设备令牌**，
手机保存它并在之后每次连接时出示，所以二维码过期不会让已配对的手机掉线。设备令牌由插件界面的
**Forget devices** 按钮作废。

一个二进制帧里可能包含不止一个 DAW block：服务端会把两次 tick 之间排队的音频全部合并发送。
详见 `audio_frame.zh-CN.md`。

## 音频来源

插件是透传的：进入 `processBlock` 的音频原样转发。在 DAW 里它就是宿主的音频。在**独立运行版**里，
来源是独立运行版的音频输入，或它的「Audio input / Play file」设置 —— 既没有输入也没有文件时它
仍然在推流（推静音），因此客户端能连上并画出平直的仪表。

## 桌面测试客户端

在手机客户端之外，标准库写的 Python 客户端可以验证连接和音频帧：

```powershell
python tools/test_client.py --address 192.168.1.20 --token <token>
```

地址和令牌从插件界面抄。这个客户端执行的 WebSocket 握手和认证与手机端完全一致，之后会计算并打印
每个音频帧的峰值/RMS。

## 版本

初始协议版本为 `1`。

机器可读的定义在：

- `pairing.schema.json`
- `control.schema.json`
- `audio_frame.schema.json`（把二进制头部表示为字段）
