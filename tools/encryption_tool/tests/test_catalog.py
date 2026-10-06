import tempfile
import unittest
from pathlib import Path

import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from catalog import build_catalog, build_state_catalog, describe_mid, format_mid  # noqa: E402
from protocol import build_plain_frame, parse_plain_frame  # noqa: E402


class CatalogTests(unittest.TestCase):
    def test_project_catalog_contains_protocol_messages(self):
        root = Path(__file__).resolve().parents[3]
        catalog, warnings = build_catalog(root)
        self.assertIn(0x50, catalog)
        self.assertIn(0x52, catalog)
        self.assertIn(0x53, catalog)
        self.assertIn("MID_INIT_ENCRYPTION", describe_mid(0x50, catalog))
        self.assertTrue(describe_mid(0xFE, catalog).startswith("MID_"))
        self.assertIsInstance(warnings, list)

    def test_missing_headers_fall_back(self):
        with tempfile.TemporaryDirectory() as folder:
            catalog, warnings = build_catalog(folder)
        self.assertIn(0x50, catalog)
        self.assertTrue(warnings)
        self.assertIn("MID_SET_DEBUG_ENC_KEY", describe_mid(0x53, catalog))
        self.assertIn("未提供注释", describe_mid(0xAB, catalog))
    def test_mid_reply_and_note_status_from_sample_frames(self):
        root = Path(__file__).resolve().parents[3]
        catalog, _ = build_catalog(root)
        states = build_state_catalog(root)
        self.assertEqual(states["FACE"][1], "FACE_STATE_NOFACE")
        samples = (
            (bytes.fromhex("EF AA 01 00 01 00 00"), "MID_NOTE;01;ready"),
            (bytes.fromhex("EF AA 00 00 02 53 00 51"), "MID_REPLY;00;SUCCESS"),
            (bytes.fromhex("EF AA 01 00 11 01 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 10"),
             "MID_NOTE;01;FACE_STATE_NOFACE"),
        )
        for raw, expected in samples:
            with self.subTest(expected=expected):
                message = parse_plain_frame(raw)
                self.assertEqual(format_mid(message.mid, message.payload, catalog, states), expected)
        self.assertEqual(format_mid(0, b"\x53\x03", catalog, states), "MID_REPLY;00;FAIL")
        self.assertEqual(format_mid(0, b"\x53", catalog, states), "MID_REPLY;00")
        self.assertEqual(format_mid(0x01, b"\x01\xff\x7f", catalog, states), "MID_NOTE;01;state=32767")
        self.assertEqual(format_mid(0x01, b"\x04\x01", catalog, states), "MID_NOTE;01;state=1")
        self.assertEqual(format_mid(0x01, b"\x08\x28\x00", catalog, states), "MID_NOTE;01;PALM_STATE_NORMAL")
        self.assertEqual(format_mid(0x04, b"\x04\x28\x00", catalog, states), "MID_NOTE;04;PALM_STATE_NORMAL")
        self.assertEqual(format_mid(0x08, b"\x08\x28\x00", catalog, states), "MID_NOTE;08;PALM_STATE_NORMAL")
        self.assertEqual(format_mid(0x01, b"\x08", catalog, states), "MID_NOTE;01")
        self.assertEqual(format_mid(0x53, bytes(16), catalog, states), "MID_SET_DEBUG_ENC_KEY;53")
        self.assertEqual(format_mid(0x12, None, catalog, states), "MID_VERIFY;12")

    def test_state_catalog_missing_header_does_not_guess_states(self):
        with tempfile.TemporaryDirectory() as folder:
            catalog, _ = build_catalog(folder)
            states = build_state_catalog(folder)
        self.assertEqual(format_mid(0x01, b"\x01\x01\x00", catalog, states), "MID_NOTE;01;state=1")
        self.assertEqual(format_mid(0x53, None, catalog, states), "MID_SET_DEBUG_ENC_KEY;53")
        self.assertEqual(format_mid(0xFE, None, catalog, states), "UNKNOWN_0xFE;FE")


if __name__ == "__main__":
    unittest.main()
