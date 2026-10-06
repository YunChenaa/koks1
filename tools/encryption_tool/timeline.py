"""Interactive, scrollable overview of reconstructed CSV messages."""
from __future__ import annotations

import heapq
import math
from typing import Sequence

from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QColor, QBrush, QPainter, QPen, QTransform
from PySide6.QtWidgets import QGraphicsEllipseItem, QGraphicsScene, QGraphicsView

from capture import CapturedFrame


BLUE = QColor("#2563eb")
RED = QColor("#dc2626")
MUTED = QColor("#64748b")


class _TimelineScene(QGraphicsScene):
    point_clicked = Signal(int)

    def mousePressEvent(self, event) -> None:
        item = self.itemAt(event.scenePos(), QTransform())
        while item is not None:
            row = item.data(0)
            if row is not None:
                self.point_clicked.emit(row)
                event.accept()
                return
            item = item.parentItem()
        super().mousePressEvent(event)


class MessageTimeline(QGraphicsView):
    """Place dots at their recorded start times and link each to a table row."""

    point_clicked = Signal(int)
    _LEFT = 64
    _RIGHT = 64
    _LANE = 22
    _STEM = 12
    _RADIUS = 6

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._scene = _TimelineScene(self)
        self.setScene(self._scene)
        self._scene.point_clicked.connect(self.point_clicked)
        self.setRenderHint(QPainter.RenderHint.Antialiasing)
        self.setMinimumHeight(150)
        self.setMaximumHeight(260)
        self._frames: list[CapturedFrame] = []
        self._points: list[QGraphicsEllipseItem] = []
        self._selected: int | None = None
        self._axis_y = 0.0
        self._zoom = 1.0
        self.render_frames([])

    def render_frames(self, frames: Sequence[CapturedFrame]) -> None:
        self._frames = list(frames)
        self._zoom = 1.0
        self._selected = None
        self._render()
        self.horizontalScrollBar().setValue(0)
        self.verticalScrollBar().setValue(0)

    def zoom(self, factor: float) -> None:
        new_zoom = max(0.25, min(16.0, self._zoom * factor))
        if new_zoom == self._zoom:
            return
        self._zoom = new_zoom
        row = self._selected
        self._render()
        self.select_row(row)

    def select_row(self, row: int | None) -> None:
        if self._selected is not None and self._selected < len(self._points):
            self._points[self._selected].setPen(QPen(Qt.GlobalColor.white, 1))
        self._selected = row if row is not None and 0 <= row < len(self._points) else None
        if self._selected is not None:
            dot = self._points[self._selected]
            dot.setPen(QPen(QColor("#0f172a"), 3))
            self.ensureVisible(dot, 48, 44)

    @staticmethod
    def _tick_step(span: float, width: float) -> float:
        desired = span * 250 / max(width, 1)
        if desired <= 0:
            return 1.0
        magnitude = 10 ** math.floor(math.log10(desired))
        for unit in (1, 2, 5, 10):
            if unit * magnitude >= desired:
                return unit * magnitude
        return 10 * magnitude

    def _render(self) -> None:
        scene = self._scene
        scene.clear()
        self._points = []
        if not self._frames:
            scene.setSceneRect(0, 0, 800, 155)
            empty = scene.addText("分析 CSV 后在此显示收发时间轴")
            empty.setDefaultTextColor(MUTED)
            empty.setPos(22, 40)
            return

        valid = [f.start_time for f in self._frames if f.timestamp_valid and math.isfinite(f.start_time)]
        origin = min(valid) if valid else 0.0
        span = max(valid) - origin if valid else 0.0
        valid_count = len(valid)
        usable_width = min(900_000.0, max(780.0, valid_count * 85.0) * self._zoom)
        scale = usable_width / span if span > 0 else 0.0
        axis_end = self._LEFT + usable_width if valid else self._LEFT
        invalid_rows = [i for i, f in enumerate(self._frames) if not f.timestamp_valid or not math.isfinite(f.start_time)]
        invalid_positions = {row: pos for pos, row in enumerate(invalid_rows)}
        invalid_start = axis_end + 90 if invalid_rows and valid else self._LEFT
        positions = [
            self._LEFT + (f.start_time - origin) * scale
            if f.timestamp_valid and math.isfinite(f.start_time) else
            invalid_start + invalid_positions[i] * 85 * self._zoom
            for i, f in enumerate(self._frames)
        ]

        # Keep time coordinates exact. Sort by horizontal position so each
        # direction can reuse the first free label lane in logarithmic time.
        lanes = ([], [])
        lane_counts = [0, 0]
        layout = [None] * len(self._frames)
        for row in sorted(range(len(positions)), key=lambda i: (positions[i], i)):
            frame = self._frames[row]
            x = positions[row]
            side = 0 if frame.direction == "锁端 → 模组" else 1
            label = f"{frame.mid:02X}" if frame.mid is not None else "??"
            gap = 40 if len(label) == 2 else 52
            available = lanes[side]
            if available and available[0][0] <= x - gap / 2:
                _, lane = heapq.heappop(available)
            else:
                lane = lane_counts[side]
                lane_counts[side] += 1
            heapq.heappush(available, (x + gap / 2 + 10, lane))
            layout[row] = (row, x, side, lane, label)
        above = max(1, lane_counts[0])
        below = max(1, lane_counts[1])
        axis_y = 24 + above * self._LANE
        self._axis_y = axis_y
        bottom = axis_y + below * self._LANE + 36
        right = max(axis_end, max(positions)) + self._RIGHT
        scene.setSceneRect(0, 0, max(850, right), bottom)

        scene.addLine(self._LEFT, axis_y, axis_end, axis_y, QPen(MUTED, 1.5))
        if valid and span:
            step = self._tick_step(span, usable_width)
            for n in range(min(1000, int(span / step) + 1)):
                offset = n * step
                x = self._LEFT + offset * scale
                scene.addLine(x, axis_y - 4, x, axis_y + 4, QPen(MUTED))
                marker = scene.addSimpleText(f"+{offset:g}s")
                marker.setBrush(MUTED)
                marker.setPos(x - marker.boundingRect().width() / 2, axis_y + 7)
        elif valid:
            scene.addLine(self._LEFT, axis_y - 4, self._LEFT, axis_y + 4, QPen(MUTED))
        if valid:
            origin_label = scene.addSimpleText(f"起点 {origin:g}s")
            origin_label.setBrush(MUTED)
            origin_label.setPos(self._LEFT, axis_y - origin_label.boundingRect().height() - 6)
        if invalid_rows:
            if valid:
                scene.addLine(invalid_start - 35, 16, invalid_start - 35, bottom - 40,
                              QPen(MUTED, 1, Qt.PenStyle.DashLine))
            warning = scene.addSimpleText("时间无效 · 按顺序排列")
            warning.setBrush(MUTED)
            warning.setPos(invalid_start - 12, 1)

        for row, x, side, lane, label in layout:
            frame = self._frames[row]
            color = BLUE if side == 0 else RED
            y = (axis_y - self._STEM - lane * self._LANE
                 if side == 0 else axis_y + self._STEM + lane * self._LANE)
            scene.addLine(x, axis_y, x, y, QPen(color, 1))
            dot = scene.addEllipse(x - self._RADIUS, y - self._RADIUS,
                                   self._RADIUS * 2, self._RADIUS * 2,
                                   QPen(Qt.GlobalColor.white, 1), QBrush(color))
            dot.setData(0, row)
            dot.setZValue(1)
            tooltip = (f"{frame.direction} | MID {label} | 时间 {frame.start_text}\n"
                       f"表格第 {row + 1} 行 | {frame.frame_type} | "
                       f"{'OK' if frame.checksum_ok else 'ERROR'}")
            if not frame.timestamp_valid:
                tooltip += "\n原始时间无法解析，位置不代表实际时间"
            dot.setToolTip(tooltip)
            text = scene.addSimpleText(label)
            text.setBrush(color)
            text.setPos(x - text.boundingRect().width() / 2,
                        y - 20 if side == 0 else y + 5)
            text.setData(0, row)
            text.setToolTip(tooltip)
            self._points.append(dot)
