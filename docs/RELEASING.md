# Releasing the plugin

The plugin and the phone app are two halves of one product: a release of either must state the
minimum version of the other it supports. See
[`../../minimeters-mobile/docs/RELEASING.md`](../../minimeters-mobile/docs/RELEASING.md) for the
app side.

Current release target: **Windows only** (VST3 + CLAP + standalone). macOS and Linux build scripts
exist but are not part of a release yet.

## 1. Bump the version

`CMakeLists.txt` is the single source of truth:

```cmake
project(MiniMetersMobile VERSION 1.0.10)
```

`build_windows.cmd` reads that value, so the zip name, the plugin's reported version and the git
tag cannot drift apart. Nothing else needs editing.

## 2. Build

```bash
./build_windows.cmd
```

Produces `releases/MiniMetersMobile-<version>-windows.zip` containing:

```
MiniMetersMobile.vst3/         VST3 bundle (directory)
MiniMetersMobile.clap          CLAP
MiniMetersMobile.exe           standalone
INSTALL.md  LICENSE  SOURCE.txt  THIRD_PARTY_NOTICES.md
```

No installer is produced on purpose: an unsigned `.exe` installer trips antivirus heuristics far
more often than a plain zip, and the copy targets are two well-known directories.

## 3. Verify before publishing

1. **Clean machine test** — copy the zip to a Windows box that has never had the plugin, follow
   `INSTALL.md` verbatim, confirm a DAW (at minimum two of Ableton / FL / Reaper / Bitwig) scans
   and opens the UI showing the new version.
2. **CLAP host** — Reaper or Bitwig loads `MiniMetersMobile.clap`.
3. **Standalone** — starts, `Options` lists audio devices, phone connects.
4. **Validator** (recommended) — run `pluginval` at strictness 5+ against the VST3. A crash there
   is a crash in someone's DAW.
5. **Paired app test** — connect the current app release; protocol `version: 1` must still be
   accepted both ways.

## 4. Checksums

```powershell
certutil -hashfile releases\MiniMetersMobile-<version>-windows.zip SHA256
```

Put the value in `SHA256SUMS.txt` and in the release notes. Users are told to verify it because
the build is unsigned.

## 5. Tag and publish

```bash
git checkout master
git tag v<version>
git push origin master --tags
```

`.github/workflows/build.yml` builds all three platforms on `v*` tags and creates the GitHub
Release with the Windows zip and `SHA256SUMS.txt` attached. If the self-hosted runners are offline,
publish manually:

```bash
gh release create v<version> \
  releases/MiniMetersMobile-<version>-windows.zip SHA256SUMS.txt \
  --title "MiniMetersMobile <version>" --notes-file docs/releases/v<version>.md
```

Release notes must state: supported formats, minimum app version, install pointer to `INSTALL.md`,
the fact that the build is **unsigned** (SmartScreen warning is expected), the SHA-256, and the
protocol version.

## 6. GPL obligations

Every published build must ship with:

- `LICENSE` — GPL v3 full text
- `SOURCE.txt` — this repository and the tag, plus the note that JUCE and clap-juce-extensions are
  unmodified submodules
- `THIRD_PARTY_NOTICES.md`

The repository must stay public. Losing that breaks the licence for every distributed copy.

## 7. Release checklist

```
[ ] CMakeLists.txt version bumped (only place a version lives)
[ ] build_windows.cmd produced a zip with 3 binaries + 4 docs
[ ] clean-machine install test passed in at least two DAWs
[ ] CLAP loads in a CLAP host
[ ] standalone runs, phone connects
[ ] pluginval (strictness 5) clean
[ ] protocol version unchanged, or bumped with a paired app release
[ ] SHA256 recorded, release notes written
[ ] tag pushed, release created, assets attached
```
