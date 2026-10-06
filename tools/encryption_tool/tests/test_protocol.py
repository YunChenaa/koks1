import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from protocol import (  # noqa: E402
    MID_INIT_ENCRYPTION,
    MID_SET_DEBUG_ENC_KEY,
    ProtocolError,
    build_plain_frame,
    decrypt_encrypted_frame,
    derive_session_key,
    encrypt_plain_frame,
    parse_debug_key_message,
    parse_init_message,
)


class ProtocolTests(unittest.TestCase):
    indexes = (6, 18, 7, 3, 13, 13, 23, 4, 8, 1, 0, 25, 9, 2, 2, 7)
    seed = bytes((1, 2, 3, 4))

    def test_parse_configuration_messages(self):
        debug = build_plain_frame(MID_SET_DEBUG_ENC_KEY, bytes(self.indexes))
        _, actual_indexes = parse_debug_key_message(debug.hex())
        self.assertEqual(actual_indexes, self.indexes)

        init = build_plain_frame(MID_INIT_ENCRYPTION, self.seed + bytes((1,)))
        _, actual_seed, mode = parse_init_message(init.hex())
        self.assertEqual(actual_seed, self.seed)
        self.assertEqual(mode, 1)

    def test_key_derivation(self):
        md5_text, key = derive_session_key(self.seed, self.indexes)
        self.assertEqual(md5_text, "08d6c05a21512a79a1dfeb9d2a8f262f")
        self.assertEqual(key, b"5da6aadc280a1dda")

    def test_round_trip_for_varied_lengths(self):
        _, key = derive_session_key(self.seed, self.indexes)
        for payload in (b"", bytes(2), bytes(13), bytes(29), bytes(range(32))):
            plain = build_plain_frame(0x12, payload)
            encrypted = encrypt_plain_frame(plain, key)
            self.assertEqual(len(encrypted[4:-1]) % 16, 0)
            self.assertEqual(decrypt_encrypted_frame(encrypted, key), plain)

    def test_invalid_inputs(self):
        with self.assertRaises(ProtocolError):
            parse_debug_key_message("EF AA 53 00 10 " + "00 " * 15)

        bad = bytearray(build_plain_frame(0x53, bytes(self.indexes)))
        bad[-1] ^= 1
        with self.assertRaises(ProtocolError):
            parse_debug_key_message(bytes(bad).hex())

        indexes = list(self.indexes)
        indexes[0] = 32
        with self.assertRaises(ProtocolError):
            parse_debug_key_message(build_plain_frame(0x53, bytes(indexes)).hex())


if __name__ == "__main__":
    unittest.main()
