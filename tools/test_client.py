#!/usr/bin/env python3
"""Minimal standard-library client for testing the MiniMeters mobile protocol."""

import argparse
import base64
import hashlib
import json
import math
import socket
import struct
import time
import uuid


def read_exact(sock, size):
    data = bytearray()
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise ConnectionError("server closed the connection")
        data.extend(chunk)
    return bytes(data)


def send_frame(sock, opcode, payload):
    if isinstance(payload, str):
        payload = payload.encode("utf-8")

    length = len(payload)
    mask = uuid.uuid4().bytes[:4]
    masked = bytes(value ^ mask[index % 4] for index, value in enumerate(payload))
    header = bytearray([0x80 | opcode])
    if length < 126:
        header.append(0x80 | length)
    elif length <= 0xFFFF:
        header.append(0x80 | 126)
        header.extend(struct.pack(">H", length))
    else:
        header.append(0x80 | 127)
        header.extend(struct.pack(">Q", length))
    sock.sendall(header + mask + masked)


def receive_frame(sock):
    first, second = struct.unpack("BB", read_exact(sock, 2))
    opcode = first & 0x0F
    length = second & 0x7F
    if length == 126:
        length = struct.unpack(">H", read_exact(sock, 2))[0]
    elif length == 127:
        length = struct.unpack(">Q", read_exact(sock, 8))[0]

    if second & 0x80:
        mask = read_exact(sock, 4)
        payload = bytes(value ^ mask[index % 4] for index, value in enumerate(read_exact(sock, length)))
    else:
        payload = read_exact(sock, length)
    return opcode, payload


def connect(address, port, token):
    sock = socket.create_connection((address, port), timeout=5)
    key = base64.b64encode(uuid.uuid4().bytes).decode("ascii")
    request = (
        f"GET /ws HTTP/1.1\r\n"
        f"Host: {address}:{port}\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        f"Sec-WebSocket-Key: {key}\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n"
    )
    sock.sendall(request.encode("ascii"))
    response = b""
    while b"\r\n\r\n" not in response:
        response += sock.recv(4096)
    if not response.startswith(b"HTTP/1.1 101"):
        raise ConnectionError(response.decode("utf-8", errors="replace"))

    send_frame(sock, 0x1, json.dumps({"type": "authenticate", "version": 1, "token": token}))
    opcode, payload = receive_frame(sock)
    if opcode != 0x1:
        raise ConnectionError("authentication response was not a text frame")
    response = json.loads(payload.decode("utf-8"))
    if response.get("type") != "authenticated":
        raise PermissionError(response)
    return sock


def print_audio_frame(payload):
    if len(payload) < 32 or payload[:4] != b"MMSA":
        raise ValueError("invalid audio frame")
    _, version, message_type, header_size, timestamp, sample_rate, channels, samples, sample_format, payload_size = struct.unpack(
        "<4sBBHQIHHII", payload[:32]
    )
    if version != 1 or message_type != 1 or header_size != 32 or sample_format != 1:
        raise ValueError("unsupported audio frame")
    audio = payload[header_size:header_size + payload_size]
    values = struct.unpack("<" + "f" * (channels * samples), audio)
    peaks = []
    rms = []
    for channel in range(channels):
        channel_values = values[channel::channels]
        peaks.append(max((abs(value) for value in channel_values), default=0.0))
        rms.append(math.sqrt(sum(value * value for value in channel_values) / max(1, len(channel_values))))
    peak_db = [20 * math.log10(max(value, 1e-12)) for value in peaks]
    rms_db = [20 * math.log10(max(value, 1e-12)) for value in rms]
    return timestamp, sample_rate, channels, samples, peak_db, rms_db


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--address", required=True)
    parser.add_argument("--token", required=True)
    parser.add_argument("--port", type=int, default=8422)
    args = parser.parse_args()

    sock = connect(args.address, args.port, args.token)
    print(f"Connected to {args.address}:{args.port}/ws")
    frames = 0
    started = time.monotonic()
    try:
        while True:
            opcode, payload = receive_frame(sock)
            if opcode == 0x8:
                break
            if opcode == 0x9:
                send_frame(sock, 0xA, payload)
                continue
            if opcode != 0x2:
                continue
            timestamp, sample_rate, channels, samples, peak_db, rms_db = print_audio_frame(payload)
            frames += 1
            elapsed = max(time.monotonic() - started, 1e-9)
            if frames == 1 or frames % 30 == 0:
                print(
                    f"frames={frames} rate={frames / elapsed:.1f}Hz "
                    f"sampleRate={sample_rate} channels={channels} samples={samples} "
                    f"peak={[round(value, 1) for value in peak_db]}dBFS "
                    f"rms={[round(value, 1) for value in rms_db]}dBFS"
                )
    except KeyboardInterrupt:
        pass
    finally:
        sock.close()


if __name__ == "__main__":
    main()
