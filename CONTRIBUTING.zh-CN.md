[English](CONTRIBUTING.md) · **简体中文**

# 参与贡献

谢谢你看这个项目。本仓库是 MiniMeters Mobile 的音频服务端：一个 JUCE 插件（VST3 / CLAP / AU）
加一个独立运行版，把原始音频通过局域网推给手机。

## 动手之前

- **许可。** 本项目是 GPL v3，因为它以 JUCE 的 GPLv3 选项链接 JUCE。提交 PR 即表示你同意你的
  贡献同样以 GPL v3 授权。不要粘贴许可不兼容的项目里的代码。
- **职责边界。** 手机端负责全部绘制；本仓库负责采集、发送音频以及配对协议。需要手机端同步改动的
  就是**协议改动**（见下文）。
- **先开 issue**。比修 bug 更大的改动都先开个 issue —— 比提一个被拒的 PR 便宜，也能先把「改动该
  放在哪」谈清楚。

## 构建与测试

```bash
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

测试默认不参与构建：

```bash
cmake -B build-tests -DMINIMETERS_BUILD_TESTS=ON
cmake --build build-tests --config Release
cd build-tests && ctest -C Release --output-on-failure
```

- `build_windows.cmd`、`build_macos.sh`、`build_linux.sh` 会产出 release zip，同时也是发现「某个平台
  炸了」最快的途径。
- 提 PR 之前把测试跑一遍。确实测不了的改动（比如纯界面调整）请在描述里说明。
- 发版流程见 [`docs/RELEASING.md`](docs/RELEASING.md)。

## 音频线程

`processBlock` 跑在音频线程里。**不要**在这里分配内存、加锁、打日志或者阻塞。`AudioBlockQueue.h`
里的队列就是为此存在的：音频线程只往预分配的内存里写，其余事情交给网络线程。

## 协议改动

协议是和手机端之间的契约，而手机端装在别人设备上 —— 你没法靠「两边一起发」来解决版本不匹配。

1. 改动任何字段、消息类型或语义，都必须升 `version`。
2. 协议版本号目前还是三处裸字面量 `1` —— `PluginProcessor.cpp:130`（配对内容）、
   `WebSocketServer.cpp:362`（校验处）、`WebSocketServer.cpp:376` 和 `:496`（回复处）。改动时保持
   它们一致；把它们收成一个常量是个受欢迎的清理。同一个提交里要同步更新 `protocol/` 下的
   `.md` 和 `.schema.json`，两者必须与代码一致。
3. 旧客户端要么继续能用，要么由插件以 `unsupported_version` 明确拒绝，好让手机端给出可操作的
   提示。**静默失败是不可接受的。**
4. 在 PR 里写清这次改动对应哪个手机端版本。

## 代码风格

- 跟你要改的文件保持一致。格式化由 `.clang-format` 约束，提交前跑一遍。
- 提交信息用 Conventional Commits：`feat(scope): ...`、`fix(scope): ...`、`docs`、`refactor`、
  `test`、`chore`。一个提交只做一件事。
- 头文件里不要写 `using namespace juce;`。
- 注释解释**为什么**，不复述**做了什么**。把代码翻译一遍的注释是噪音。

## Pull Request

- 保持聚焦。顺便做的清理请另开 PR。
- 描述里说清「没有这个改动会坏在哪」，而不只是「这个改动做了什么」。
- 界面改动请附截图 —— 插件窗口本身就是产品的一部分。
- 合并前由维护者跑一次构建和测试；请不要自己合并自己的 PR。

## 报 bug

请附上：

- DAW 名称和版本、宿主缓冲区大小与采样率、Windows/macOS 版本
- 插件格式（VST3 / CLAP / AU / 独立运行版）
- 手机型号，以及手机端设置里显示的 App 版本
- 状态行上那几项：`Audio source`、`Network`、`Clients`、`Paired`
- 防火墙是否放行了入站 TCP 8422

`Network: unavailable` 几乎总是端口问题：要么还有另一个实例没退，要么被防火墙拦了。
