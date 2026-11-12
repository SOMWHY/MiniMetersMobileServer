# Binary audio frame

Audio is sent as one binary WebSocket frame per DAW audio block. All integer fields use little-endian byte order. Samples are IEEE-754 little-endian `float32` values, interleaved by channel.

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

For stereo audio, samples are ordered `L0, R0, L1, R1, ...`. A mono input is represented as one channel. The frame represents the same block that is written to the existing MiniMeters shared-memory path.
