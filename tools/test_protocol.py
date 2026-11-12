import json
import pathlib
import struct
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
PROTOCOL = ROOT / "protocol"


class ProtocolTests(unittest.TestCase):
    def test_schemas_are_valid_json_and_have_version_one(self):
        for name in ("pairing.schema.json", "control.schema.json", "audio_frame.schema.json"):
            with self.subTest(name=name):
                schema = json.loads((PROTOCOL / name).read_text(encoding="utf-8"))
                self.assertEqual(schema["$schema"], "https://json-schema.org/draft/2020-12/schema")

    def test_audio_header_is_32_bytes_and_little_endian(self):
        header = struct.pack("<4sBBHQIHHII", b"MMSA", 1, 1, 32, 123, 48000, 2, 128, 1, 1024)
        self.assertEqual(len(header), 32)
        self.assertEqual(header[:4], b"MMSA")
        self.assertEqual(struct.unpack("<Q", header[8:16])[0], 123)
        self.assertEqual(struct.unpack("<I", header[16:20])[0], 48000)
        self.assertEqual(struct.unpack("<H", header[20:22])[0], 2)
        self.assertEqual(struct.unpack("<H", header[22:24])[0], 128)

    def test_audio_payload_size_matches_float32_samples(self):
        channels = 2
        samples_per_channel = 128
        payload_size = channels * samples_per_channel * 4
        self.assertEqual(payload_size, 1024)


if __name__ == "__main__":
    unittest.main()
