[English](INSTALL.md) · **简体中文**

# 安装 MiniMetersMobile（Windows）

这个压缩包里是同一个音频服务端的三种构建：

| 文件 | 是什么 |
| --- | --- |
| `install.cmd` | 一键安装脚本（会请求管理员权限） |
| `MiniMetersMobile.vst3\` | VST3 插件（是个文件夹 —— 那是 bundle，不是打错了） |
| `MiniMetersMobile.clap` | CLAP 插件 |
| `MiniMetersMobile.exe` | 独立运行版，不需要 DAW |

Windows 会提示「发布者未知」。这些构建没有代码签名，所以 SmartScreen 和部分杀毒软件会拦一下。
如果想确认文件在传输过程中没被改动，装之前用 `SHA256SUMS.txt` 校验一遍。

## 安装插件

### 最省事：跑安装脚本

双击 **`install.cmd`**，同意管理员提示。它会把 VST3 和 CLAP 复制到下面那些目录、添加手机连接所需的
入站防火墙规则，然后提示你重新扫描 DAW 的插件。除此之外什么都没装，也不写注册表。

想卸载：

```bat
 powershell -ExecutionPolicy Bypass -File Install-MiniMetersMobile.ps1 -Uninstall
```

### 手动安装

1. 把**整个** `MiniMetersMobile.vst3` 文件夹复制到：
   ```
   C:\Program Files\Common Files\VST3\
   ```
2. 把 `MiniMetersMobile.clap` 复制到：
   ```
   C:\Program Files\Common Files\CLAP\
   ```
   `CLAP` 目录不存在就自己建。这两步都需要管理员权限。
3. 在 DAW 里重新扫描插件（或者重启 DAW）。
4. 在 Windows 防火墙里放行入站 TCP **8422**，仅限专用网络。否则插件会显示
   `Network: unavailable`，手机永远连不上。

各宿主里扫描的位置：

- Ableton Live —— Preferences → Plug-Ins → rescan
- FL Studio —— Options → Manage plugins → Find more plugins
- Reaper —— Preferences → Plug-ins → VST → Re-scan
- Bitwig —— Settings → Plug-ins → Scan

## 运行独立版

双击 `MiniMetersMobile.exe`。用 **Options** 选择音频输入/输出设备和缓冲区大小。不装 DAW、只想试试手机端的话，
独立版是最省事的方式。

## 连接手机

1. 打开插件窗口或独立版窗口。它会显示二维码，以及同样的文字信息：
   `Address`、`Path`、`Token`，还有令牌的有效期。
2. 在手机上扫二维码（或者手动输入地址和令牌）。
3. 首次握手会签发设备令牌，下次启动就不用再扫码了。

注意：

- 手机和电脑必须在同一个局域网。
- **Windows 防火墙必须放行入站 TCP 8422。** 如果插件窗口显示 `Network: unavailable`，说明端口没打开 ——
  通常是已经有另一个实例占着它，或者被防火墙拦了。
- `Forget devices` 会作废所有已签发的设备令牌。手机突然连不上、怀疑配对信息过期时用它。
- `Refresh pairing` 只换新的二维码令牌，不影响已经配对过的手机。

## 卸载

删掉 `MiniMetersMobile.vst3` 文件夹、`MiniMetersMobile.clap` 和独立版可执行文件。
配对状态保存在插件自己的设置文件里，插件删掉它就跟着没了。

## 许可

GPL v3 —— 见 `LICENSE`，对应源码位置见 `SOURCE.txt`，本构建链接的第三方组件见
`THIRD_PARTY_NOTICES.md`。
