# Binary audio frame

Audio is sent as one binary WebSocket frame per network tick. All integer fields use little-endian byte order. Samples are IEEE-754 little-endian `float32` values, interleaved by channel.

## Header

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | Magic bytes `MMSA` |
| 4 | 1 | Protocol version (`1`) |
| 5 | 1 | Message type (`1` = audio block) |
| 6 | 2 | Header size in bytes (`32`) |
| 8 | 8 | Timestamp, Unix epoch milliseconds |
| 16 | 4 | Sample rate, unsigned integer |
| 20 | 2 | Channel count, unsigned integer |
| 22 | 2 | Samples per channel, unsigned integer |
| 24 | 4 | Sample format (`1` = little-endian float32) |
| 28 | 4 | Payload size in bytes |

The payload begins at offset `32` and contains `channels * samplesPerChannel` samples.

For stereo audio, samples are ordered `L0, R0, L1, R1, ...`. A mono input is represented as one channel.

## Backpressure

The audio thread writes one block per DAW callback (~10 ms at 48 kHz with a 512-sample block) into a four-slot queue. The network thread polls the queue and **coalesces every queued block into a single frame**, so one frame may carry several DAW blocks as consecutive samples. `samplesPerChannel` is the total for that frame and `timestamp` is the newest block's.

Sending one block per tick would deliver roughly a third of realtime at a 30 Hz tick, so a receiver should size its buffer expectations from `samplesPerChannel` and never assume a frame equals one DAW block.
