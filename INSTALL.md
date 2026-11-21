# Installing MiniMetersMobile (Windows)

This zip contains three builds of the same audio server:

| File | What it is |
| --- | --- |
| `install.cmd` | One-click installer (asks for administrator rights) |
| `MiniMetersMobile.vst3\` | VST3 plugin (folder — it is a bundle, not a mistake) |
| `MiniMetersMobile.clap` | CLAP plugin |
| `MiniMetersMobile.exe` | Standalone application, no DAW needed |

Windows will warn that the publisher is unknown. The builds are not code signed, so SmartScreen
and some antivirus products flag them. Verify the download with `SHA256SUMS.txt` before installing
if you want certainty that the file was not modified in transit.

## Install the plugins

### Easiest: run the installer

Double-click **`install.cmd`** and accept the administrator prompt. It copies the VST3 and CLAP
bundles into the folders below, adds the inbound firewall rule the phone needs, and tells you to
rescan your DAW. Nothing else is installed and no registry keys are written.

To remove it again:

```bat
 powershell -ExecutionPolicy Bypass -File Install-MiniMetersMobile.ps1 -Uninstall
```

### By hand

1. Copy the **whole** `MiniMetersMobile.vst3` folder to:
   ```
   C:\Program Files\Common Files\VST3\
   ```
2. Copy `MiniMetersMobile.clap` to:
   ```
   C:\Program Files\Common Files\CLAP\
   ```
   Create the `CLAP` folder if it does not exist. Both operations need administrator rights.
3. In your DAW, rescan plugins (or restart the DAW).
4. Allow inbound TCP **8422** in Windows Firewall, private networks only, or the plugin will show
   `Network: unavailable` and the phone will never connect.

Typical paths per host:

- Ableton Live — Preferences → Plug-Ins → rescan
- FL Studio — Options → Manage plugins → Find more plugins
- Reaper — Preferences → Plug-ins → VST → Re-scan
- Bitwig — Settings → Plug-ins → Scan

## Run the standalone

Double-click `MiniMetersMobile.exe`. Use **Options** to pick the audio input/output device and the
buffer size. The standalone is the simplest way to try the app without a DAW.

## Connect the phone

1. Open the plugin or standalone window. It shows a QR code plus the same values as text:
   `Address`, `Path`, `Token`, and how long the token is valid.
2. On the phone, scan the QR code (or type the address and token).
3. The first handshake issues a device token, so the next launch reconnects without the QR code.

Notes:

- The phone and the computer must be on the same local network.
- **Windows Firewall must allow inbound TCP 8422.** If the plugin window reports
  `Network: unavailable`, the port could not be opened — usually another instance already holds
  it, or the firewall blocks it.
- `Forget devices` invalidates every issued device token. Use it if a phone no longer connects and
  you suspect a stale pairing.
- `Refresh pairing` issues a new QR token without invalidating already-paired phones.

## Uninstall

Delete the `MiniMetersMobile.vst3` folder and `MiniMetersMobile.clap`, and the standalone
executable. Pairing state lives in the plugin's settings file; removing the plugin removes it.

## Licensing

GPL v3 — see `LICENSE` and `SOURCE.txt` for the corresponding source location, and
`THIRD_PARTY_NOTICES.md` for the components this build links against.
