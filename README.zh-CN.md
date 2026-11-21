[English](README.md) · **简体中文**

# MiniMetersMobile

一个 JUCE 音频服务端：把 DAW（或它自己的独立运行版）里的原始音频，通过局域网推流给
[MiniMeters Mobile](https://github.com/SOMWHY/minimeters-mobile) 手机端。波形、频谱、声谱、VU、LUFS 和示波器
这些视图全部由手机端用 Skia 绘制；本项目只负责采集和发送。

```
DAW ──▶ 音频回调 ──▶ AudioBlockQueue ──▶ WebSocket :8422/ws ──▶ 手机
```

## 下载

Windows 构建（VST3 + CLAP + 独立运行版）见
[Releases 页面](https://github.com/SOMWHY/MiniMetersMobileServer/releases/latest)。安装步骤： [`INSTALL.zh-CN.md`](INSTALL.zh-CN.md)。

macOS 和 Linux 的构建脚本是有的，但**不在当前发布范围内** —— 它们需要各自单独走一遍验证。

## 插件格式

| 格式 | 状态 |
| --- | --- |
| VST3 | 已发布（Windows） |
| CLAP | 已发布（Windows） |
| 独立运行版 | 已发布（Windows） |
| AU | macOS 可构建，未发布 |

插件归类在 `Fx` / `Tools` 分类下，属于效果器。它不改动音频：输入原样透传，只是被分接出来放进队列。

## 配对流程

插件界面会显示一个二维码，以及同样的文字信息 —— `Address`、`Path`（`/ws`）、`Token`，外加一个
有效期倒计时（120 秒）。手机扫码或手动输入都可以。首次握手成功后服务端会签发**设备令牌**，
之后再次启动可以静默重连；二维码令牌是短期的，设备令牌不是。

窗口上的三个按钮：

- **Refresh pairing** —— 重新生成一个二维码令牌，已配对的设备不受影响
- **Copy pairing** —— 把二维码内容复制到剪贴板（调试时方便）
- **Forget devices** —— 作废所有设备令牌

状态行：音频来源、`Network`（WebSocket 端口是否绑定成功）、`Clients`（当前连接数）、`Paired`（已配对设备数）。

Windows 防火墙必须放行入站 **TCP 8422**。

## 从源码构建

依赖：CMake 3.15+、支持 C++20 的编译器、git 子模块。

```bash
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

或者用平台脚本：

```bash
./build_windows.cmd      # 产物 releases/MiniMetersMobile-<version>-windows.zip
./build_macos.sh
./build_linux.sh
```

单元测试：

```bash
cmake -B build-tests -DMINIMETERS_BUILD_TESTS=ON
cmake --build build-tests --config Release
cd build-tests && ctest -C Release --output-on-failure
```

## 协议

`/ws` 上的带版本号 JSON。对兼容性有影响的是两条消息：

- 配对内容：`{"address", "port", "path", "token", "version": 1}`
- `authenticated`：携带协商出的 `version` 和已接受的状态

改动任何字段都必须升版本号，并成对发布手机端。插件会报告自身版本以及它接受的最低手机端版本；
细节见 `docs/RELEASING.md`。

## 致谢

本项目是 Joseph Lyncheski 的 [`direct-audio/MiniMetersServer`](https://github.com/direct-audio/MiniMetersServer)
的 fork，并完整保留了上游提交历史。原始作品的版权归 Joseph Lyncheski 所有（Copyright (C) 2022），
以 GNU GPL v3 发布；版权与修改声明记录在 [`NOTICE`](NOTICE)。

相对上游 `6a21e02`，本 fork 的改动如下（完整列表以 `git log --oneline 6a21e02..HEAD` 为准）：

- **配对与传输** —— 配对 token 在编辑器与 WebSocket 服务之间共享，吊销失败会如实上报而不是静默吞掉，
  网络服务能跨 service 切换存活（`2146331`、`e1da769`、`648c3db`）。
- **实时流** —— 广播限流到 30 Hz，所有排队 block 合并进同一帧，并关闭 Nagle 算法以免音频被 socket
  攒包拖住（`5a7315c`、`b2c5bf7`、`fcac876`）。
- **WebSocket 健壮性** —— 读写并发串行化，已认证的空闲连接保持存活，握手与帧校验加严（`c49116f`、
  `6cb44b1`、`127479c`、`a986dd9`）。
- **插件标识与界面** —— 插件 target、bundle identity 与 vendor 名改为 `MiniMetersMobile` / `somwhy`，
  编辑器改为以大号二维码为主体、状态独立成行（`d1cbe25`、`3b1c99a`、`7803e7f`、`da28f02`）。
- **手机端桥接** —— 二维码配对显示、桌面 WebSocket 测试客户端，以及原生配对与音频队列测试
  （`e879f00`、`837b5cc`、`f46c6b7`）。
- **打包** —— Windows 上的 VST3 / CLAP / standalone 构建，配一键安装器与公开校验和（`0d390bf`、
  `07df4f4`、`91737ce`）。

## 许可

GNU General Public License v3 —— 见 [`LICENSE`](LICENSE)；版权与修改声明见 [`NOTICE`](NOTICE)。
`LICENSE` 文件逐字保留原文，从不往里追加内容。本项目以 JUCE 的 GPLv3 选项使用它，因此对应源码
必须保持可获得：也就是这个仓库。第三方组件见 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。

## 参与贡献

见 [`CONTRIBUTING.zh-CN.md`](CONTRIBUTING.zh-CN.md)。
