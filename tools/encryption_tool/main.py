"""PySide6 GUI for manual and CSV batch SenseLock AES analysis."""
from __future__ import annotations

import csv
import sys
from pathlib import Path

from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QColor
from PySide6.QtWidgets import (
    QApplication,
    QCheckBox,
    QFileDialog,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QPlainTextEdit,
    QRadioButton,
    QScrollArea,
    QTableWidget,
    QTableWidgetItem,
    QTabWidget,
    QVBoxLayout,
    QWidget,
)

if __package__ in (None, ""):
    sys.path.insert(0, str(Path(__file__).resolve().parent))

from catalog import build_catalog, build_state_catalog, format_mid  # noqa: E402
from capture import (  # noqa: E402
    BatchAnalysis, analyze_csv_pair, latest_captured_indexes, merge_captures,
    parse_index_text, read_csv_capture, reassemble_stream,
)
from protocol import (  # noqa: E402
    ProtocolError,
    decrypt_encrypted_frame,
    derive_session_key,
    encrypt_plain_frame,
    format_hex,
    parse_debug_key_message,
    parse_hex,
    parse_init_message,
    parse_plain_frame,
)
from timeline import MessageTimeline  # noqa: E402


class EncryptionWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("SenseLock AES 报文工具")
        self.resize(1260, 860)
        self.indexes: tuple[int, ...] | None = None
        self.seed: bytes | None = None
        self.key: bytes | None = None
        self.catalog, self.catalog_warnings = build_catalog(Path(__file__).resolve().parents[2])
        self.state_catalog = build_state_catalog(Path(__file__).resolve().parents[2])
        self.batch_result: BatchAnalysis | None = None
        self._prefill_timer = QTimer(self)
        self._prefill_timer.setSingleShot(True)
        self._prefill_timer.setInterval(350)
        self._prefill_timer.timeout.connect(self.prefill_batch_indexes)
        self._build_ui()

    def _build_ui(self) -> None:
        tabs = QTabWidget()
        tabs.addTab(self._build_manual_tab(), "单条消息")
        tabs.addTab(self._build_batch_tab(), "CSV 批量分析")
        self.setCentralWidget(tabs)
        self.statusBar().showMessage("请先解析完整的 53 消息和 50 消息")

    def _build_manual_tab(self) -> QWidget:
        central = QWidget()
        root = QVBoxLayout(central)

        debug_group = QGroupBox("1. MID_SET_DEBUG_ENC_KEY（完整 53 消息）")
        debug_layout = QVBoxLayout(debug_group)
        self.debug_input = QPlainTextEdit()
        self.debug_input.setPlaceholderText("EF AA 53 00 10 ... 16字节索引 ... XOR")
        self.debug_input.setFixedHeight(78)
        debug_layout.addWidget(self.debug_input)
        button = QPushButton("解析 53 消息")
        button.clicked.connect(self.parse_debug)
        debug_layout.addWidget(button, alignment=Qt.AlignmentFlag.AlignRight)
        root.addWidget(debug_group)

        init_group = QGroupBox("2. MID_INIT_ENCRYPTION（完整 50 消息）")
        init_layout = QVBoxLayout(init_group)
        self.init_input = QPlainTextEdit()
        self.init_input.setPlaceholderText("EF AA 50 00 05 01 02 03 04 01 XOR")
        self.init_input.setFixedHeight(78)
        init_layout.addWidget(self.init_input)
        button = QPushButton("解析 50 消息")
        button.clicked.connect(self.parse_init)
        init_layout.addWidget(button, alignment=Qt.AlignmentFlag.AlignRight)
        root.addWidget(init_group)

        key_group = QGroupBox("当前会话密钥")
        form = QFormLayout(key_group)
        self.index_label = QLabel("未解析")
        self.seed_label = QLabel("未解析")
        self.md5_label = QLabel("未解析")
        self.key_label = QLabel("未解析")
        for label in (self.index_label, self.seed_label, self.md5_label, self.key_label):
            label.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
            label.setWordWrap(True)
        form.addRow("索引数组：", self.index_label)
        form.addRow("seed：", self.seed_label)
        form.addRow("MD5：", self.md5_label)
        form.addRow("AES key：", self.key_label)
        root.addWidget(key_group)

        message_group = QGroupBox("3. 待处理完整 UART 消息")
        message_layout = QVBoxLayout(message_group)
        self.message_input = QPlainTextEdit()
        self.message_input.setPlaceholderText("加密模式输入明文帧；解密模式输入 AES 帧")
        self.message_input.setMinimumHeight(125)
        message_layout.addWidget(self.message_input)
        mode_layout = QHBoxLayout()
        self.encrypt_radio = QRadioButton("加密：明文帧 → AES 帧")
        self.decrypt_radio = QRadioButton("解密：AES 帧 → 明文帧")
        self.encrypt_radio.setChecked(True)
        mode_layout.addWidget(self.encrypt_radio)
        mode_layout.addWidget(self.decrypt_radio)
        mode_layout.addStretch()
        message_layout.addLayout(mode_layout)
        self.compatibility = QCheckBox("兼容固件额外全零尾块（仅解密，需明确勾选）")
        message_layout.addWidget(self.compatibility)
        root.addWidget(message_group)

        buttons = QHBoxLayout()
        run_button = QPushButton("执行")
        run_button.clicked.connect(self.process_message)
        clear_button = QPushButton("清空")
        clear_button.clicked.connect(self.clear_all)
        buttons.addWidget(run_button)
        buttons.addWidget(clear_button)
        buttons.addStretch()
        root.addLayout(buttons)

        output_group = QGroupBox("输出")
        output_layout = QVBoxLayout(output_group)
        self.output = QPlainTextEdit()
        self.output.setReadOnly(True)
        self.output.setMinimumHeight(150)
        output_layout.addWidget(self.output)
        root.addWidget(output_group)

        self.debug_input.textChanged.connect(lambda: self.invalidate_configuration(True))
        self.init_input.textChanged.connect(lambda: self.invalidate_configuration(False))
        self.message_input.textChanged.connect(self.output.clear)
        self.encrypt_radio.toggled.connect(self.output.clear)
        self.compatibility.toggled.connect(self.output.clear)
        return central

    def _build_batch_tab(self) -> QWidget:
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        page = QWidget()
        page.setMinimumWidth(980)
        root = QVBoxLayout(page)
        files = QGroupBox("逻辑分析仪 CSV")
        paths = QHBoxLayout(files)
        self.lock_path = QPlainTextEdit()
        self.lock_path.setPlaceholderText("锁端 CSV 文件路径")
        self.lock_path.setFixedHeight(32)
        self.module_path = QPlainTextEdit()
        self.module_path.setPlaceholderText("模组端 CSV 文件路径")
        self.module_path.setFixedHeight(32)
        paths.addWidget(QLabel("锁端："))
        paths.addWidget(self._path_row(self.lock_path), 1)
        paths.addWidget(QLabel("模组端："))
        paths.addWidget(self._path_row(self.module_path), 1)
        root.addWidget(files)

        index_row = QHBoxLayout()
        index_row.addWidget(QLabel("索引信息："))
        self.batch_indexes = QPlainTextEdit()
        self.batch_indexes.setPlaceholderText("52/53 中的 16 个索引会自动填入；也可输入 16 个十进制整数，以逗号分隔")
        self.batch_indexes.setFixedHeight(36)
        index_row.addWidget(self.batch_indexes, 1)
        root.addLayout(index_row)
        self.lock_path.textChanged.connect(self._prefill_timer.start)
        self.module_path.textChanged.connect(self._prefill_timer.start)
        self.batch_indexes.textChanged.connect(self._prefill_timer.stop)

        controls = QHBoxLayout()
        self.batch_button = QPushButton("分析 CSV")
        self.batch_button.clicked.connect(self.analyze_batch)
        clear = QPushButton("清空批处理结果")
        clear.clicked.connect(self.clear_batch)
        controls.addWidget(self.batch_button)
        controls.addWidget(clear)
        self.save_batch_button = QPushButton("保存输出表格")
        self.save_batch_button.setEnabled(False)
        self.save_batch_button.clicked.connect(self.save_batch)
        controls.addWidget(self.save_batch_button)
        controls.addStretch()
        root.addLayout(controls)

        overview = QGroupBox("收发时间总览")
        overview_layout = QVBoxLayout(overview)
        zoom_controls = QHBoxLayout()
        zoom_controls.addStretch()
        zoom_in = QPushButton("放大")
        zoom_in.clicked.connect(lambda: self.batch_timeline.zoom(1.5))
        zoom_out = QPushButton("缩小")
        zoom_out.clicked.connect(lambda: self.batch_timeline.zoom(1 / 1.5))
        zoom_controls.addWidget(zoom_out)
        zoom_controls.addWidget(zoom_in)
        overview_layout.addLayout(zoom_controls)
        self.batch_timeline = MessageTimeline()
        self.batch_timeline.point_clicked.connect(self.jump_to_batch_row)
        overview_layout.addWidget(self.batch_timeline)
        root.addWidget(overview)

        self.batch_table = QTableWidget(0, 11)
        self.batch_table.setHorizontalHeaderLabels([
            "序号", "时间", "锁端→模组", "模组→锁端", "帧类型", "MID",
            "解密/明文", "原始报文", "校验", "密钥来源", "诊断",
        ])
        self.batch_table.setSelectionBehavior(QTableWidget.SelectionBehavior.SelectRows)
        self.batch_table.setEditTriggers(QTableWidget.EditTrigger.NoEditTriggers)
        self.batch_table.setMinimumHeight(
            self.batch_table.horizontalHeader().height()
            + self.batch_table.verticalHeader().defaultSectionSize() * 10
            + self.batch_table.frameWidth() * 2 + 8
        )
        self.batch_table.cellClicked.connect(self.show_batch_detail)
        self.batch_table.itemSelectionChanged.connect(self._sync_timeline_selection)
        root.addWidget(self.batch_table, 1)
        self.batch_detail = QPlainTextEdit()
        self.batch_detail.setReadOnly(True)
        self.batch_detail.setPlaceholderText("选中一条消息后显示完整原始报文、解密报文和诊断")
        self.batch_detail.setFixedHeight(130)
        root.addWidget(self.batch_detail)
        scroll.setWidget(page)
        return scroll

    @staticmethod
    def _path_row(edit: QPlainTextEdit) -> QWidget:
        row = QWidget()
        layout = QHBoxLayout(row)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(edit)
        button = QPushButton("选择")
        button.clicked.connect(lambda: EncryptionWindow._choose_file(edit))
        layout.addWidget(button)
        return row

    @staticmethod
    def _choose_file(edit: QPlainTextEdit) -> None:
        path, _ = QFileDialog.getOpenFileName(None, "选择逻辑分析仪 CSV", "", "CSV files (*.csv);;All files (*)")
        if path:
            edit.setPlainText(path)

    def invalidate_configuration(self, debug: bool) -> None:
        if debug:
            self.indexes = None
            self.index_label.setText("未解析")
        else:
            self.seed = None
            self.seed_label.setText("未解析")
        self.key = None
        self.md5_label.setText("未解析")
        self.key_label.setText("未解析")
        self.output.clear()
        self.statusBar().showMessage("配置已修改，请重新解析")

    def show_error(self, message: str) -> None:
        self.statusBar().showMessage(message)
        QMessageBox.warning(self, "输入错误", message)

    def parse_debug(self) -> None:
        try:
            _, self.indexes = parse_debug_key_message(self.debug_input.toPlainText())
            self.index_label.setText("[" + ", ".join(map(str, self.indexes)) + "]")
            self.derive_key_if_ready()
            self.statusBar().showMessage("53 消息解析成功")
        except ProtocolError as exc:
            self.show_error(str(exc))

    def parse_init(self) -> None:
        try:
            _, self.seed, mode = parse_init_message(self.init_input.toPlainText())
            mode_text = "未携带 mode，按 AES 处理" if mode is None else f"0x{mode:02X}"
            if mode is not None and mode != 1:
                mode_text += "，非 AES 模式；本工具仍仅执行 AES 运算"
            self.seed_label.setText(f"{format_hex(self.seed)}（mode={mode_text}）")
            self.derive_key_if_ready()
            self.statusBar().showMessage("50 消息解析成功")
        except ProtocolError as exc:
            self.show_error(str(exc))

    def derive_key_if_ready(self) -> None:
        if self.indexes is None or self.seed is None:
            return
        md5_text, self.key = derive_session_key(self.seed, self.indexes)
        self.md5_label.setText(md5_text)
        self.key_label.setText(f"{self.key.decode('ascii')}（{format_hex(self.key)}）")

    def process_message(self) -> None:
        self.output.clear()
        try:
            if self.key is None:
                raise ProtocolError("请先成功解析 53 消息和 50 消息")
            message = parse_hex(self.message_input.toPlainText())
            if self.encrypt_radio.isChecked():
                result = encrypt_plain_frame(message, self.key)
                status = f"加密成功：{len(message)} 字节 → {len(result)} 字节"
            else:
                result = decrypt_encrypted_frame(message, self.key, compatibility=self.compatibility.isChecked())
                status = f"解密成功：{len(message)} 字节 → {len(result)} 字节"
                if self.compatibility.isChecked():
                    status += "；已兼容固件额外全零尾块"
            self.output.setPlainText(format_hex(result, 32))
            self.statusBar().showMessage(status)
        except ProtocolError as exc:
            self.show_error(str(exc))

    def prefill_batch_indexes(self) -> None:
        """Scan selected files for the latest trusted 52/53 index payload."""
        try:
            streams = []
            for edit, direction in (
                (self.lock_path, "锁端 → 模组"), (self.module_path, "模组 → 锁端"),
            ):
                path = edit.toPlainText().strip()
                if path:
                    imported = read_csv_capture(path)
                    streams.append(reassemble_stream(imported.items, direction))
                else:
                    streams.append([])
            frames = merge_captures(*streams)
            indexes = latest_captured_indexes(frames)
            self.batch_indexes.setPlainText(",".join(map(str, indexes)) if indexes else "")
        except (OSError, UnicodeError, ValueError) as exc:
            self.batch_indexes.clear()
            self.statusBar().showMessage(f"索引预填失败：{exc}")

    def analyze_batch(self) -> None:
        if self._prefill_timer.isActive():
            self._prefill_timer.stop()
            self.prefill_batch_indexes()
        lock = self.lock_path.toPlainText().strip()
        module = self.module_path.toPlainText().strip()
        if not lock or not module:
            self.show_error("请同时选择锁端和模组端 CSV 文件")
            return
        if not self.batch_indexes.toPlainText().strip():
            self.show_error("索引信息为空，请填写 16 个十进制索引后再分析 CSV")
            return
        try:
            indexes = parse_index_text(self.batch_indexes.toPlainText().strip())
        except ProtocolError as exc:
            self.show_error(str(exc))
            return
        self.batch_button.setEnabled(False)
        self.save_batch_button.setEnabled(False)
        self.batch_result = None
        self.batch_table.setRowCount(0)
        self.batch_timeline.render_frames([])
        self.batch_detail.clear()
        try:
            result = analyze_csv_pair(lock, module, indexes=indexes)
            self.batch_result = result
            self._display_batch(result)
        except (OSError, UnicodeError, ValueError) as exc:
            self.show_error(f"CSV 分析失败：{exc}")
        finally:
            self.batch_button.setEnabled(True)

    def _display_batch(self, result: BatchAnalysis) -> None:
        self.batch_table.setRowCount(0)
        for index, frame in enumerate(result.frames, start=1):
            row = self.batch_table.rowCount()
            self.batch_table.insertRow(row)
            for column, value in enumerate(self._batch_row(index, frame)):
                item = QTableWidgetItem(value)
                if (column == 2 and value) or (column == 3 and value):
                    font = item.font()
                    font.setBold(True)
                    item.setFont(font)
                    item.setTextAlignment(Qt.AlignmentFlag.AlignCenter)
                    item.setForeground(QColor("#15803d" if column == 2 else "#b91c1c"))
                self.batch_table.setItem(row, column, item)
        self.batch_table.resizeColumnsToContents()
        self.batch_timeline.render_frames(result.frames)
        self.save_batch_button.setEnabled(True)
        self.statusBar().showMessage(f"CSV 分析完成：{len(result.frames)} 条完整消息")

    def _batch_row(self, index: int, frame) -> list[str]:
        try:
            plain = parse_plain_frame(frame.decrypted) if frame.decrypted is not None else None
        except ProtocolError:
            plain = None
        lock_to_module = "→" if frame.direction == "锁端 → 模组" else ""
        module_to_lock = "←" if frame.direction == "模组 → 锁端" else ""
        return [
            str(index), frame.start_text, lock_to_module, module_to_lock, frame.frame_type,
            format_mid(plain.mid if plain else frame.mid, plain.payload if plain else None,
                       self.catalog, self.state_catalog), frame.decrypted_hex, frame.raw_hex,
            "OK" if frame.checksum_ok else "ERROR", frame.key_source,
            "; ".join(x for x in (frame.diagnostic, frame.decrypt_diagnostic) if x),
        ]

    def save_batch(self) -> None:
        if self.batch_result is None:
            return
        path, _ = QFileDialog.getSaveFileName(self, "保存分析结果", "分析结果.csv", "CSV files (*.csv)")
        if not path:
            return
        try:
            with open(path, "w", encoding="utf-8-sig", newline="") as stream:
                writer = csv.writer(stream)
                writer.writerow(self.batch_table.horizontalHeaderItem(column).text()
                                for column in range(self.batch_table.columnCount()))
                for index, frame in enumerate(self.batch_result.frames, start=1):
                    writer.writerow(self._batch_row(index, frame))
        except (OSError, UnicodeError) as exc:
            self.show_error(f"保存分析结果失败：{exc}")
            return
        self.statusBar().showMessage(f"分析结果已保存：{path}")

    def _sync_timeline_selection(self) -> None:
        rows = self.batch_table.selectionModel().selectedRows()
        self.batch_timeline.select_row(rows[0].row() if rows else None)
        if rows:
            self.show_batch_detail(rows[0].row(), 0)

    def jump_to_batch_row(self, row: int) -> None:
        if self.batch_result is None or not (0 <= row < len(self.batch_result.frames)):
            return
        self.batch_table.selectRow(row)
        self.batch_table.scrollToItem(self.batch_table.item(row, 0))
        self.show_batch_detail(row, 0)

    def show_batch_detail(self, row: int, column: int) -> None:
        del column
        if self.batch_result is None or not (0 <= row < len(self.batch_result.frames)):
            return
        frame = self.batch_result.frames[row]
        self.batch_detail.setPlainText(
            f"方向：{frame.direction}\n时间：{frame.start_text}\n参与 CSV 行：{frame.row_numbers}\n"
            f"原始报文：\n{format_hex(frame.data, 32)}\n\n"
            f"解密/明文：\n{format_hex(frame.decrypted, 32) if frame.decrypted else '未解密'}\n\n"
            f"诊断：{frame.diagnostic or '无'}\n{frame.decrypt_diagnostic}\n"
            f"密钥来源：{frame.key_source or '无'}"
        )

    def clear_batch(self) -> None:
        self._prefill_timer.stop()
        self.lock_path.clear()
        self.module_path.clear()
        self._prefill_timer.stop()
        self.batch_indexes.clear()
        self.batch_table.setRowCount(0)
        self.batch_timeline.render_frames([])
        self.batch_detail.clear()
        self.batch_result = None
        self.save_batch_button.setEnabled(False)

    def clear_all(self) -> None:
        for edit in (self.debug_input, self.init_input, self.message_input, self.output):
            edit.clear()
        self.indexes = None
        self.seed = None
        self.key = None
        for label in (self.index_label, self.seed_label, self.md5_label, self.key_label):
            label.setText("未解析")
        self.statusBar().showMessage("已清空")


def main() -> int:
    app = QApplication(sys.argv)
    window = EncryptionWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
