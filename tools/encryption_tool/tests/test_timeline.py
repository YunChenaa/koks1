"""Offscreen checks for the interactive message timeline."""
import os
import sys
import tempfile
import unittest
from pathlib import Path

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from PySide6.QtCore import Qt
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QApplication
from capture import BatchAnalysis, CapturedFrame, CsvImport, read_csv_capture, reassemble_stream
from main import EncryptionWindow
from protocol import build_plain_frame
from timeline import MessageTimeline
from test_capture import write_capture


def frame(time, direction, mid, *, valid=True):
    data = build_plain_frame(mid, b"\x00") if mid is not None else b"\xef\xaa"
    return CapturedFrame(direction, data, time, time, str(time), (1,), "plain",
                         True, False, decrypted=data if mid is not None else None,
                         mid=mid, timestamp_valid=valid)


class TimelineTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QApplication.instance() or QApplication([])

    def test_proportional_times_colors_dense_labels_and_zoom(self):
        timeline = MessageTimeline()
        timeline.show()
        self.app.processEvents()
        frames = [frame(10, "锁端 → 模组", 0x50),
                  frame(10, "锁端 → 模组", 0x52),
                  frame(12, "模组 → 锁端", 0x01),
                  frame(20, "模组 → 锁端", None)]
        timeline.render_frames(frames)
        points = timeline._points
        self.assertEqual(len(points), 4)
        x = [point.rect().center().x() for point in points]
        y = [point.rect().center().y() for point in points]
        self.assertEqual(x[0], x[1])
        self.assertAlmostEqual((x[2] - x[0]) / (x[3] - x[0]), .2)
        self.assertNotEqual(y[0], y[1])
        self.assertLess(y[0], y[2])
        self.assertEqual(points[0].brush().color().name(), "#2563eb")
        self.assertEqual(points[2].brush().color().name(), "#dc2626")
        self.assertEqual(abs(y[0] - timeline._axis_y), timeline._STEM)
        self.assertEqual(abs(y[2] - timeline._axis_y), timeline._STEM)
        self.assertIn("MID ??", points[3].toolTip())
        rect = timeline.scene().sceneRect()
        timeline.zoom(1.5)
        self.assertGreater(timeline.scene().sceneRect().width(), rect.width())
        self.assertEqual(len(timeline._points), len(frames))
        tick_labels = [item for item in timeline.scene().items()
                       if hasattr(item, "text") and item.text().startswith("+")]
        self.assertTrue(tick_labels)
        self.assertTrue(all(abs(item.y() - (timeline._axis_y + 7)) < 0.1 for item in tick_labels))
        timeline.close()

    def test_click_dot_and_label_selects_corresponding_table_row(self):
        window = EncryptionWindow()
        window.show()
        messages = [frame(1, "锁端 → 模组", 0x50), frame(2, "模组 → 锁端", 0x01)]
        window.batch_result = BatchAnalysis(messages, CsvImport("lock"), CsvImport("module"))
        window._display_batch(window.batch_result)
        self.app.processEvents()
        view = window.batch_timeline
        dot = view._points[1]
        position = view.mapFromScene(dot.rect().center())
        QTest.mouseClick(view.viewport(), Qt.MouseButton.LeftButton, pos=position)
        self.assertEqual(window.batch_table.currentRow(), 1)
        self.assertIn("方向：模组 → 锁端", window.batch_detail.toPlainText())
        # Click the MID label as well, not only the circle.
        label = next(item for item in view.scene().items()
                     if item.data(0) == 0 and item is not view._points[0])
        view.horizontalScrollBar().setValue(0)
        label_position = view.mapFromScene(label.sceneBoundingRect().center())
        QTest.mouseClick(view.viewport(), Qt.MouseButton.LeftButton, pos=label_position)
        self.assertEqual(window.batch_table.currentRow(), 0)
        window.batch_table.selectRow(1)
        self.assertEqual(view._selected, 1)
        self.assertEqual(view._points[1].pen().width(), 3)
        window.clear_batch()
        self.assertEqual(len(view._points), 0)
        window.close()

    def test_bad_time_is_separate_and_nonfinite_is_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            csv = Path(folder) / "times.csv"
            write_capture(csv, [(1, build_plain_frame(0x50, b"1234"))])
            rows = csv.read_text(encoding="utf-8-sig").splitlines()
            rows[1] = "nan," + rows[1].split(",", 1)[1]
            csv.write_text("\n".join(rows) + "\n", encoding="utf-8-sig")
            imported = read_csv_capture(csv)
            self.assertFalse(next(item for item in imported.items if item.row_number == 2).timestamp_valid)
            self.assertTrue(all(item.timestamp_valid for item in imported.items if item.row_number != 2))
            self.assertTrue(imported.warnings)
            frames = reassemble_stream(imported.items, "锁端 → 模组")
            # Mixed fallback and real timestamps may interrupt reassembly,
            # but any reconstructed frame inherits the first byte's validity.
            self.assertTrue(all(f.timestamp_valid == next(
                item.timestamp_valid for item in imported.items if item.row_number == f.row_numbers[0]
            ) for f in frames))
        timeline = MessageTimeline()
        timeline.render_frames([frame(1, "锁端 → 模组", 0x50),
                                frame(2, "模组 → 锁端", 0x01, valid=False)])
        self.assertIn("不代表实际时间", timeline._points[1].toolTip())
        self.assertGreater(timeline._points[1].rect().center().x(),
                           timeline._points[0].rect().center().x())
        timeline.close()


if __name__ == "__main__":
    unittest.main()
