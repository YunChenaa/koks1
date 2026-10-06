"""Additional regression checks, including offscreen Qt interactions."""
import csv
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import protocol as p
from capture import BatchAnalysis, CapturedFrame, CsvImport
from test_capture import write_capture
from main import EncryptionWindow
from PySide6.QtWidgets import QApplication, QScrollArea

INDEXES = (6, 18, 7, 3, 13, 13, 23, 4, 8, 1, 0, 25, 9, 2, 2, 7)


class RegressionTests(unittest.TestCase):
    def test_hex(self):
        self.assertEqual(p.parse_hex("0xEF 0xAA\n50"), bytes.fromhex("efaa50"))
        for text in ("", "A", "GG", "EF-AA"):
            with self.subTest(text=text), self.assertRaises(p.ProtocolError):
                p.parse_hex(text)

    def test_bad_configuration(self):
        for indexes in ((-1,) * 16, (32,) * 16, (0,) * 15):
            with self.assertRaises(p.ProtocolError):
                p.derive_session_key(bytes(4), indexes)
        for mid, payload in ((0x51, bytes(4)), (0x50, bytes(3))):
            with self.assertRaises(p.ProtocolError):
                p.parse_init_message(p.build_plain_frame(mid, payload).hex())

    def test_firmware_compatibility(self):
        key = b"5da6aadc280a1dda"
        plain = p.build_plain_frame(0x12, bytes(range(13)))
        normal = p.encrypt_plain_frame(plain, key)
        cipher = normal[4:-1] + bytes(16)
        legacy = p.HEADER + len(cipher).to_bytes(2, "big") + cipher + bytes([p.checksum(cipher)])
        with self.assertRaises(p.ProtocolError):
            p.decrypt_encrypted_frame(legacy, key)
        self.assertEqual(p.decrypt_encrypted_frame(legacy, key, compatibility=True), plain)
        with self.assertRaises(p.ProtocolError):
            p.decrypt_encrypted_frame(normal, key, compatibility=True)

    def test_encrypted_validation(self):
        key = b"5da6aadc280a1dda"
        frame = p.encrypt_plain_frame(p.build_plain_frame(0x12, b"abc"), key)
        for bad in (b"xx" + frame[2:], frame[:-1], frame[:-1] + bytes([frame[-1] ^ 1])):
            with self.assertRaises(p.ProtocolError):
                p.decrypt_encrypted_frame(bad, key)
        with self.assertRaises(p.ProtocolError):
            p.aes_ecb_decrypt(bytes(15), key)

    def test_aes_known_vector(self):
        key = bytes.fromhex("000102030405060708090a0b0c0d0e0f")
        plain = bytes.fromhex("00112233445566778899aabbccddeeff")
        expected = bytes.fromhex("69c4e0d86a7b0430d8cdb78070b4c55a")
        self.assertEqual(p.aes_ecb_encrypt(plain, key), expected)


class GuiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QApplication.instance() or QApplication([])

    def test_roundtrip_and_invalidation(self):
        window = EncryptionWindow()
        window.show()
        self.app.processEvents()
        with patch.object(window, "show_error") as error:
            window.debug_input.setPlainText(p.build_plain_frame(0x53, bytes(INDEXES)).hex())
            window.init_input.setPlainText(p.build_plain_frame(0x50, bytes([1, 2, 3, 4, 1])).hex())
            window.parse_debug()
            window.parse_init()
            self.assertEqual(window.key, b"5da6aadc280a1dda")
            plain = p.build_plain_frame(0x12, bytes([0, 10]))
            window.message_input.setPlainText(plain.hex())
            window.process_message()
            cipher = window.output.toPlainText()
            self.assertTrue(cipher)
            window.decrypt_radio.setChecked(True)
            window.message_input.setPlainText(cipher)
            window.process_message()
            self.assertEqual(p.parse_hex(window.output.toPlainText()), plain)
            error.assert_not_called()
            window.init_input.setPlainText("invalid")
            self.assertIsNone(window.key)
            self.assertEqual(window.output.toPlainText(), "")
            window.process_message()
            error.assert_called_once()
            window.clear_all()
            self.assertIsNone(window.seed)
        window.close()
    def test_batch_layout_colors_and_save(self):
        window = EncryptionWindow()
        self.app.processEvents()
        headers = [window.batch_table.horizontalHeaderItem(column).text()
                   for column in range(window.batch_table.columnCount())]
        self.assertEqual(headers, [
            "序号", "时间", "锁端→模组", "模组→锁端", "帧类型", "MID",
            "解密/明文", "原始报文", "校验", "密钥来源", "诊断",
        ])
        self.assertEqual(window.lock_path.parent().parent(), window.module_path.parent().parent())
        self.assertIsInstance(window.lock_path.window().centralWidget().widget(1), QScrollArea)
        minimum_rows_height = (window.batch_table.horizontalHeader().height()
                               + window.batch_table.verticalHeader().defaultSectionSize() * 10
                               + window.batch_table.frameWidth() * 2)
        self.assertGreaterEqual(window.batch_table.minimumHeight(), minimum_rows_height)
        self.assertEqual(window.batch_timeline.toolTip(), "")
        frames = [
            CapturedFrame("锁端 → 模组", b"\xef\xaa", 1.0, 1.1, "1.0", (2, 3), "plain", True, False,
                          decrypted=b"\xef\xaa", mid=0x50, key_source="50@1.0"),
            CapturedFrame("模组 → 锁端", b"\xef\xaa", 2.0, 2.1, "2.0", (4, 5), "encrypted", False, True,
                          diagnostic="CSV 标记校验错误"),
        ]
        result = BatchAnalysis(frames, CsvImport("lock.csv"), CsvImport("module.csv"))
        window.batch_result = result
        window._display_batch(result)
        self.assertTrue(window.save_batch_button.isEnabled())
        self.assertEqual(window.batch_table.item(0, 2).text(), "→")
        self.assertEqual(window.batch_table.item(0, 3).text(), "")
        self.assertEqual(window.batch_table.item(1, 2).text(), "")
        self.assertEqual(window.batch_table.item(1, 3).text(), "←")
        self.assertTrue(window.batch_table.item(0, 2).font().bold())
        self.assertTrue(window.batch_table.item(1, 3).font().bold())
        self.assertEqual(window.batch_table.item(0, 2).foreground().color().name(), "#15803d")
        self.assertEqual(window.batch_table.item(1, 3).foreground().color().name(), "#b91c1c")
        self.assertEqual(window.batch_table.item(0, 6).text(), "EF AA")
        self.assertEqual(window.batch_table.item(0, 7).text(), "EF AA")
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "output.csv"
            with patch("main.QFileDialog.getSaveFileName", return_value=(str(path), "CSV files (*.csv)")):
                window.save_batch()
            with path.open("r", encoding="utf-8-sig", newline="") as stream:
                rows = list(csv.reader(stream))
            self.assertEqual(rows[0], headers)
            self.assertEqual(rows[1][2:4], ["→", ""])
            self.assertEqual(rows[2][2:4], ["", "←"])
            self.assertEqual(rows[1][9], "50@1.0")
            self.assertEqual(rows[2][8], "ERROR")
            self.assertEqual(rows[2][10], "CSV 标记校验错误")
        window.clear_batch()
        self.assertFalse(window.save_batch_button.isEnabled())
        window.close()
    def test_batch_index_prefill_edit_and_export_mid(self):
        seed = b"\x01\x02\x03\x04"
        manual = tuple(reversed(INDEXES))
        _, key = p.derive_session_key(seed, manual)
        reply = p.build_plain_frame(0, b"\x53\x00")
        encrypted = p.build_encrypted_frame(reply[2:-1], key)
        with tempfile.TemporaryDirectory() as folder:
            lock = Path(folder) / "lock.csv"
            module = Path(folder) / "module.csv"
            write_capture(lock, [(1, p.build_plain_frame(0x52, bytes(INDEXES))),
                                 (2, p.build_plain_frame(0x50, seed))])
            write_capture(module, [(3, encrypted)])
            window = EncryptionWindow()
            window.lock_path.setPlainText(str(lock))
            window.module_path.setPlainText(str(module))
            window.prefill_batch_indexes()
            self.assertEqual(window.batch_indexes.toPlainText(), ",".join(map(str, INDEXES)))
            # Manual edits cancel any queued path-triggered scan before Analyze.
            window.module_path.setPlainText(str(module) + " ")
            self.assertTrue(window._prefill_timer.isActive())
            window.batch_indexes.setPlainText(",".join(map(str, manual)))
            self.assertFalse(window._prefill_timer.isActive())
            with patch.object(window, "show_error") as error:
                window.analyze_batch()
                error.assert_not_called()
            self.assertEqual(window.batch_table.item(2, 5).text(), "MID_REPLY;00;SUCCESS")
            self.assertEqual(window.batch_result.frames[-1].decrypted, reply)
            output = Path(folder) / "output.csv"
            with patch("main.QFileDialog.getSaveFileName", return_value=(str(output), "")):
                window.save_batch()
            with output.open(encoding="utf-8-sig", newline="") as stream:
                rows = list(csv.reader(stream))
            self.assertEqual(rows[0][5], "MID")
            self.assertEqual(rows[-1][5], "MID_REPLY;00;SUCCESS")
            window.clear_batch()
            self.assertEqual(window.batch_indexes.toPlainText(), "")
            window.close()

    def test_batch_empty_indexes_warn_and_paths_with_no_indexes_clear(self):
        with tempfile.TemporaryDirectory() as folder:
            lock = Path(folder) / "lock.csv"
            module = Path(folder) / "module.csv"
            write_capture(lock, [(1, p.build_plain_frame(0x50, b"\x01\x02\x03\x04"))])
            write_capture(module, [(2, p.build_plain_frame(0x01, b"\x00"))])
            window = EncryptionWindow()
            window.lock_path.setPlainText(str(lock))
            window.module_path.setPlainText(str(module))
            window.prefill_batch_indexes()
            self.assertEqual(window.batch_indexes.toPlainText(), "")
            with patch.object(window, "show_error") as error:
                window.analyze_batch()
                error.assert_called_once()
                self.assertIn("索引信息为空", error.call_args.args[0])
                window.batch_indexes.setPlainText("1,2")
                window.analyze_batch()
                self.assertIn("16 个", error.call_args.args[0])
            self.assertIsNone(window.batch_result)
            window.close()


if __name__ == "__main__":
    unittest.main()
