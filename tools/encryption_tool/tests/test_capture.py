import csv
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from capture import (  # noqa: E402
    analyze_csv_pair,
    latest_captured_indexes,
    merge_captures,
    parse_index_text,
    read_csv_capture,
    reassemble_stream,
)
from protocol import (  # noqa: E402
    MID_INIT_ENCRYPTION,
    MID_SET_DEBUG_ENC_KEY,
    MID_SET_RELEASE_ENC_KEY,
    ProtocolError,
    build_encrypted_frame,
    build_plain_frame,
    derive_session_key,
)


INDEXES = (6, 18, 7, 3, 13, 13, 23, 4, 8, 1, 0, 25, 9, 2, 2, 7)


def write_capture(path: Path, frames, *, title=True, bad_row=None, checksum_row=None):
    rows = [["time", "byte", "checksum", "frame"]] if title else []
    row_number = 1 if title else 0
    for timestamp, data in frames:
        for value in data:
            row_number += 1
            rows.append([str(timestamp), f"{value:02x}", "", ""])
            timestamp += 0.001
    if bad_row is not None:
        rows.insert(bad_row, ["0.25", "ef", "", "error"])
    if checksum_row is not None and checksum_row < len(rows):
        rows[checksum_row][2] = "error"
    with path.open("w", encoding="utf-8-sig", newline="") as stream:
        csv.writer(stream).writerows(rows)


class CaptureTests(unittest.TestCase):
    def test_csv_header_bom_units_and_error_drop(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "capture.csv"
            path.write_text(
                "﻿Time,Byte,Checksum,Frame\n"
                "2 ms,ef,,\n"
                "1 ms,aa,,error\n"
                "3 ms,0x10,bad,\n",
                encoding="utf-8",
            )
            imported = read_csv_capture(path)
            self.assertEqual([item.value for item in imported.items], [0xEF, 0x10])
            self.assertEqual(imported.dropped_frame_error, 1)
            self.assertEqual(imported.checksum_marked_rows, 1)
            self.assertTrue(imported.items[0].timestamp < imported.items[1].timestamp)

    def test_reassemble_plain_and_paste_noise(self):
        frame1 = build_plain_frame(0x22, b"abc")
        frame2 = build_plain_frame(0x23, b"xyz")
        items = []
        for row, value in enumerate(b"\x00\x01" + frame1 + frame2, start=1):
            from capture import CaptureByte
            items.append(CaptureByte(row, str(row), value, False, False, row))
        frames = reassemble_stream(items, "锁端 → 模组")
        self.assertEqual([frame.data for frame in frames], [frame1, frame2])

    def test_two_direction_merge_and_timeline(self):
        seed1 = bytes((1, 2, 3, 4))
        seed2 = bytes((5, 6, 7, 8))
        _, key1 = derive_session_key(seed1, INDEXES)
        _, key2 = derive_session_key(seed2, INDEXES)
        debug = build_plain_frame(MID_SET_DEBUG_ENC_KEY, bytes(INDEXES))
        init1 = build_plain_frame(MID_INIT_ENCRYPTION, seed1 + b"\x01")
        init2 = build_plain_frame(MID_INIT_ENCRYPTION, seed2 + b"\x01")
        business1 = build_plain_frame(0x12, b"old")
        business2 = build_plain_frame(0x13, b"new")
        encrypted1 = build_encrypted_frame(business1[2:-1], key1)
        encrypted2 = build_encrypted_frame(business2[2:-1], key2)
        with tempfile.TemporaryDirectory() as folder:
            lock = Path(folder) / "lock.csv"
            module = Path(folder) / "module.csv"
            write_capture(lock, [(1, debug), (2, init1), (3, init2)])
            write_capture(module, [(2.5, encrypted1), (4.5, encrypted2)])
            result = analyze_csv_pair(lock, module)
        self.assertFalse(result.missing_debug_key)
        self.assertFalse(result.missing_seed)
        decrypted = [frame.decrypted for frame in result.frames if frame.frame_type == "encrypted"]
        self.assertEqual(decrypted, [business1, business2])
        encrypted_frames = [frame for frame in result.frames if frame.frame_type == "encrypted"]
        self.assertTrue(encrypted_frames[0].key_source.startswith("50@"))
        self.assertNotEqual(encrypted_frames[0].key_source, encrypted_frames[1].key_source)

    def test_missing_debug_key_never_uses_default(self):
        seed = bytes((1, 2, 3, 4))
        _, key = derive_session_key(seed, INDEXES)
        init = build_plain_frame(MID_INIT_ENCRYPTION, seed + b"\x01")
        encrypted = build_encrypted_frame(build_plain_frame(0x12, b"x")[2:-1], key)
        with tempfile.TemporaryDirectory() as folder:
            lock = Path(folder) / "lock.csv"
            module = Path(folder) / "module.csv"
            write_capture(lock, [(1, init)])
            write_capture(module, [(2, encrypted)])
            result = analyze_csv_pair(lock, module)
        self.assertTrue(result.missing_debug_key)
        self.assertIsNone(next(frame for frame in result.frames if frame.frame_type == "encrypted").decrypted)

    def test_checksum_marked_configuration_does_not_update_key(self):
        debug = build_plain_frame(MID_SET_DEBUG_ENC_KEY, bytes(INDEXES))
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "capture.csv"
            write_capture(path, [(1, debug)], checksum_row=2)
            imported = read_csv_capture(path)
            frames = reassemble_stream(imported.items, "锁端 → 模组")
        # The frame remains visible, but the marked byte prevents valid state update.
        self.assertEqual(len(frames), 1)
        self.assertFalse(frames[0].checksum_ok)
    def test_52_and_manual_indexes_override_captured_53(self):
        seed1 = b"\x01\x02\x03\x04"
        seed2 = b"\x05\x06\x07\x08"
        manual = tuple(reversed(INDEXES))
        _, key1 = derive_session_key(seed1, manual)
        _, key2 = derive_session_key(seed2, manual)
        message1 = build_plain_frame(0x12, b"manual")
        message2 = build_plain_frame(0x13, b"later")
        release = build_plain_frame(MID_SET_RELEASE_ENC_KEY, bytes(INDEXES))
        debug = build_plain_frame(MID_SET_DEBUG_ENC_KEY, bytes([0] * 16))
        with tempfile.TemporaryDirectory() as folder:
            lock = Path(folder) / "lock.csv"
            module = Path(folder) / "module.csv"
            write_capture(lock, [
                (1, release), (2, build_plain_frame(0x50, seed1)),
                (3, debug), (4, build_plain_frame(0x50, seed2)),
            ])
            write_capture(module, [
                (2.5, build_encrypted_frame(message1[2:-1], key1)),
                (4.5, build_encrypted_frame(message2[2:-1], key2)),
            ])
            result = analyze_csv_pair(lock, module, indexes=manual)
            self.assertEqual(result.captured_indexes, tuple([0] * 16))
            self.assertFalse(result.missing_debug_key)
            self.assertEqual(
                [f.decrypted for f in result.frames if f.frame_type == "encrypted"],
                [message1, message2],
            )
            self.assertIsNone(result.frames[0].decrypted)  # 52 remains a plain control frame
            self.assertEqual(result.frames[0].mid, MID_SET_RELEASE_ENC_KEY)
            self.assertEqual(result.frames[3].mid, MID_SET_DEBUG_ENC_KEY)

    def test_only_52_provides_indexes_without_53(self):
        seed = b"\x01\x02\x03\x04"
        _, key = derive_session_key(seed, INDEXES)
        message = build_plain_frame(0x12, b"business")
        release = build_plain_frame(MID_SET_RELEASE_ENC_KEY, bytes(INDEXES))
        with tempfile.TemporaryDirectory() as folder:
            lock = Path(folder) / "lock.csv"
            module = Path(folder) / "module.csv"
            write_capture(lock, [(1, release), (2, build_plain_frame(0x50, seed))])
            write_capture(module, [(3, build_encrypted_frame(message[2:-1], key))])
            result = analyze_csv_pair(lock, module)
        self.assertEqual(result.captured_indexes, INDEXES)
        self.assertFalse(result.missing_debug_key)
        self.assertEqual(result.frames[-1].decrypted, message)

    def test_missing_52_53_with_manual_indexes(self):
        seed = b"\x01\x02\x03\x04"
        _, key = derive_session_key(seed, INDEXES)
        message = build_plain_frame(0x12, b"manual")
        with tempfile.TemporaryDirectory() as folder:
            lock = Path(folder) / "lock.csv"
            module = Path(folder) / "module.csv"
            write_capture(lock, [(1, build_plain_frame(0x50, seed))])
            write_capture(module, [(2, build_encrypted_frame(message[2:-1], key))])
            result = analyze_csv_pair(lock, module, indexes=INDEXES)
        self.assertTrue(result.missing_debug_key)
        self.assertIsNone(result.captured_indexes)
        self.assertEqual(result.frames[-1].decrypted, message)
        self.assertNotIn("索引信息为空", "; ".join(result.warnings))

    def test_bad_index_frames_do_not_prefill(self):
        bad_value = build_plain_frame(0x52, bytes([32] * 16))
        bad_size = build_plain_frame(0x53, bytes([1] * 15))
        valid = build_plain_frame(0x53, bytes(INDEXES))
        with tempfile.TemporaryDirectory() as folder:
            lock = Path(folder) / "lock.csv"
            write_capture(lock, [(1, bad_value), (2, bad_size), (3, valid)], checksum_row=2 + len(bad_value) + len(bad_size))
            frames = reassemble_stream(read_csv_capture(lock).items, "锁端 → 模组")
        self.assertIsNone(latest_captured_indexes(frames))
        self.assertEqual(len(frames), 3)

    def test_index_field_decimal_validation(self):
        self.assertEqual(parse_index_text(", ".join(map(str, INDEXES))), INDEXES)
        for value in ("", "0," * 15, "0," * 15 + "0x1", "0," * 15 + "-1", "0," * 15 + "32"):
            with self.subTest(value=value), self.assertRaises(ProtocolError):
                parse_index_text(value)


if __name__ == "__main__":
    unittest.main()
