# Contributing

Thanks for looking. This repository is the audio server half of MiniMeters Mobile: a JUCE plugin
(VST3 / CLAP / AU) plus a standalone build that streams raw audio to the phone over the local
network.

## Before you start

- **Licence.** This project is GPL v3 because it links JUCE under JUCE's GPLv3 option. By opening a
  pull request you agree your contribution is GPL v3 as well. Do not paste code from a project
  whose licence is incompatible.
- **Scope.** The phone draws everything; this repository captures and ships audio plus the pairing
  protocol. Changes that need the client to change too are a protocol change (see below).
- **Open an issue first** for anything larger than a bug fix. It is cheaper than a rejected pull
  request, and it lets us agree on where the change belongs.

## Building and testing

```bash
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Tests are opt-in:

```bash
cmake -B build-tests -DMINIMETERS_BUILD_TESTS=ON
cmake --build build-tests --config Release
cd build-tests && ctest -C Release --output-on-failure
```

- `build_windows.cmd`, `build_macos.sh`, `build_linux.sh` produce a release zip. They are also the
  quickest way to notice that a platform broke.
- Run the tests before opening a pull request. A change that cannot be tested (a UI tweak, say)
  should say so in the description.
- The release path is documented in [`docs/RELEASING.md`](docs/RELEASING.md).

## The audio thread

`processBlock` runs in the audio thread. Do not allocate, lock, log, or block there. The queue in
`AudioBlockQueue.h` exists so the audio thread only writes into preallocated storage; the network
thread does everything else.

## Protocol changes

The protocol is a contract with the phone, and the phone is installed on someone's device — you
cannot fix a mismatch by shipping both halves at once.

1. Any change to a field, a message type or a meaning requires a `version` bump.
2. The protocol version is currently a bare literal `1` in three places —
   `PluginProcessor.cpp:130` (pairing payload), `WebSocketServer.cpp:362` (the check) and
   `WebSocketServer.cpp:376` / `:496` (the replies). Keep them in sync; folding them into one constant
   is a welcome cleanup. Update `protocol/` in the same commit: the `.md` files and the
   `.schema.json` files must agree with the code.
3. Old clients must keep working, or the plugin must reject them with `unsupported_version` so the
   phone can show an actionable message. Silent failure is not acceptable.
4. Say in the pull request which phone release the change pairs with.

## Style

- Match the file you are editing. Formatting is enforced by `.clang-format`; run it before
  committing.
- Conventional Commits: `feat(scope): ...`, `fix(scope): ...`, `docs`, `refactor`, `test`, `chore`.
  One logical change per commit.
- No `using namespace juce;` in headers.
- Comments explain *why*, not *what*. A comment that restates the code is noise.

## Pull requests

- Keep them focused. Unrelated cleanups belong in their own pull request.
- Describe what breaks without the change, not just what the change does.
- Include screenshots for editor changes — the plugin window is part of the product.
- A maintainer runs a build and the tests before merging; do not merge your own pull request.

## Reporting bugs

Include:

- DAW and version, host buffer size and sample rate, Windows/macOS version
- Plugin format (VST3 / CLAP / AU / standalone)
- Phone model and the app version shown in its settings
- What the status line says: `Audio source`, `Network`, `Clients`, `Paired`
- Whether inbound TCP 8422 is allowed through the firewall

`Network: unavailable` is almost always the port: another instance still running, or the firewall.
