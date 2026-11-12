# MiniMetersMobile bug-fix plan

## Scope

This plan addresses the first real end-to-end desktop-client test:

- WebSocket authentication succeeded.
- Audio binary frames were received continuously.
- The stream ran at approximately 100 Hz.
- Audio values were silent (`-240 dBFS`), which is expected without an input signal.
- The connection stopped after approximately 67 seconds.
- After the disconnect, the server was no longer listening on port `8422`.

No implementation changes are made by this document.

## Observed behavior

Example stream:

- Sample rate: `48000 Hz`.
- Samples per channel: `480`.
- Frame rate: approximately `100 Hz`.
- `48000 / 480 = 100` blocks per second.
- Peak/RMS: `-240 dBFS` on both channels.
- Connection ended around frame `6720`.

The successful authentication proves that the following currently work:

- LAN address resolution for `10.46.38.245`.
- TCP connection.
- WebSocket HTTP upgrade.
- Pairing token validation.
- Control response parsing.
- Binary audio frame parsing.

## Priority 0: identify why the whole service stops

### Hypotheses

1. The processor or Standalone audio lifecycle destroys/recreates the processor.
2. The WebSocket listener is closed by a service lifecycle path.
3. The listener thread exits unexpectedly without updating service state.
4. A client send failure causes an unintended server-wide shutdown.
5. IPC primary state changes trigger a processor/service transition.
6. A socket or thread exception is not currently visible.

### Planned diagnostics

Add network lifecycle diagnostics outside the audio thread:

- Service start attempt and result.
- Listener creation and listener close.
- Accept-loop exit reason.
- Broadcast-loop exit reason.
- Client handshake success/failure.
- Client authentication success/failure.
- Client send failure and socket error.
- Client-session worker exit.
- Processor construction and destruction.
- IPC primary state transitions.

Diagnostics must not perform file I/O or logging from `processBlock()`. The network and message threads may write diagnostic output.

### Acceptance criteria

- A single Standalone instance remains listening for at least 10 minutes.
- A connected desktop client remains connected for at least 10 minutes.
- If a client disconnects, the listener remains active.
- If one client fails, other clients remain connected.
- If the processor is recreated, the service either restarts cleanly or reports a clear unavailable state.

## Priority 1: fix service lifecycle and client isolation

Planned changes:

1. Make listener ownership explicit.
2. Keep the listener alive when an individual client session exits.
3. Ensure one failed `send()` only closes that client.
4. Join accept, broadcast, and client threads deterministically.
5. Make `isRunning()` reflect the real listener state.
6. Ensure a service restart cannot leave a stale port owner.
7. Ensure primary-state changes do not close healthy authenticated clients unexpectedly.

Acceptance criteria:

- Closing the desktop client does not stop port `8422`.
- Starting a second client does not disconnect the first.
- Stopping one client does not affect the listener or other clients.
- Reopening the Standalone after shutdown can bind `8422` again.

## Priority 2: define and implement the network frame rate

Current behavior sends every DAW block, which is approximately 100 Hz for `480` samples at `48000 Hz`.

Target behavior from the overview is approximately 30 Hz.

Planned design:

- The audio thread continues to copy every block into a fixed-size real-time buffer.
- The network thread sends at most one latest block approximately every `33 ms`.
- Old queued blocks may be discarded when the network rate is lower than the DAW block rate.
- The audio thread must never wait for the network thread.
- The protocol must document that network frame frequency is independent of DAW block frequency.

Acceptance criteria:

- Desktop client receives approximately 30 frames per second.
- Audio sample rate, channel count, and block metadata remain correct.
- No audio callback locks, network calls, JSON serialization, or dynamic allocation are introduced.

## Priority 3: verify real audio data

The observed `-240 dBFS` values indicate silence, not a transport failure.

Planned automated checks:

- Generate known float samples in a protocol fixture.
- Validate frame payload size and channel interleaving.
- Validate client-side peak/RMS calculations.
- Reject malformed payload lengths.

Required manual check:

- Load the VST3 or CLAP in a DAW.
- Play silence, a full-scale sine wave, and a known lower-level sine wave.
- Confirm the client reports expected peak and RMS values.

## Priority 4: improve test client diagnostics

Planned improvements:

- Accept the complete copied pairing JSON directly.
- Print whether authentication succeeded.
- Print WebSocket close code/reason when available.
- Distinguish server close, client timeout, malformed frame, and network error.
- Add a configurable test duration.
- Add a reconnect option for lifecycle tests.
- Preserve a short summary instead of requiring a continuously scrolling console.

Acceptance criteria:

- A copied pairing payload can be passed without manually extracting address/token.
- The client clearly identifies the stage at which a failure occurs.

## Priority 5: protocol and security review

Planned checks:

- Verify all control messages against the JSON schemas.
- Verify `/ws` path and protocol version handling.
- Verify token expiry and refresh behavior.
- Verify existing authenticated connections survive token refresh.
- Verify maximum client count and maximum frame sizes.
- Document that the current transport is unencrypted LAN WebSocket.
- Decide whether TLS is required before public distribution.

## Priority 6: plugin lifecycle and PluginVal

After the service fix:

- Run the full Debug and Release Ninja builds.
- Load and unload Standalone repeatedly.
- Open and close the editor repeatedly.
- Load VST3 and CLAP in a host.
- Run PluginVal at the required strictness.
- Verify no thread, socket, or memory leak warnings.

## Manual validation gate

Pause implementation when automated fixes and diagnostics are complete and request manual validation for:

1. A real DAW audio signal.
2. Multiple plugin instances in a DAW.
3. QR scanning with a real phone.
4. A real phone or future React Native client.
5. Ten-minute connection stability.
6. PluginVal results.

## Commit policy

Each priority group that changes code must be committed separately. Documentation-only updates may be committed separately. Do not mix unrelated UI, protocol, and lifecycle changes in one commit.
