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

## 许可

GNU General Public License v3 —— 见 [`LICENSE`](LICENSE)。本项目以 JUCE 的 GPLv3 选项使用它，因此对应源码
必须保持可获得：也就是这个仓库。第三方组件见 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。

## 参与贡献

见 [`CONTRIBUTING.zh-CN.md`](CONTRIBUTING.zh-CN.md)。
