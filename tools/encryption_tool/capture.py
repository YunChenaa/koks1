"""CSV capture import, frame reconstruction, timeline decryption, and merging."""
from __future__ import annotations

import csv
from dataclasses import dataclass, field
from pathlib import Path
import math
import re
from typing import Iterable, Sequence

from protocol import (
    MID_INIT_ENCRYPTION,
    MID_SET_DEBUG_ENC_KEY,
    MID_SET_RELEASE_ENC_KEY,
    ProtocolError,
    PlainFrame,
    build_plain_frame,
    checksum,
    decrypt_encrypted_frame,
    derive_session_key,
    format_hex,
    parse_encrypted_frame,
    parse_plain_frame,
)


@dataclass(frozen=True)
class CaptureByte:
    timestamp: float
    timestamp_text: str
    value: int
    checksum_error: bool
    frame_error: bool
    row_number: int
    timestamp_valid: bool = True


@dataclass
class CsvImport:
    path: str
    items: list[CaptureByte] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)
    dropped_frame_error: int = 0
    invalid_rows: int = 0
    checksum_marked_rows: int = 0


@dataclass
class CapturedFrame:
    direction: str
    data: bytes
    start_time: float
    end_time: float
    start_text: str
    row_numbers: tuple[int, ...]
    frame_type: str
    checksum_ok: bool
    marked_checksum_error: bool
    diagnostic: str = ""
    decrypted: bytes | None = None
    decrypt_diagnostic: str = ""
    mid: int | None = None
    intent: str = ""
    key_source: str = ""
    timestamp_valid: bool = True

    @property
    def raw_hex(self) -> str:
        return format_hex(self.data)

    @property
    def decrypted_hex(self) -> str:
        return "" if self.decrypted is None else format_hex(self.decrypted)


@dataclass
class BatchAnalysis:
    frames: list[CapturedFrame]
    lock_import: CsvImport
    module_import: CsvImport
    warnings: list[str] = field(default_factory=list)
    missing_debug_key: bool = False
    missing_seed: bool = False
    captured_indexes: tuple[int, ...] | None = None


@dataclass
class _Candidate:
    kind: str
    length: int


KNOWN_PLAIN_MIDS = {
    MID_INIT_ENCRYPTION,
    MID_SET_RELEASE_ENC_KEY,
    MID_SET_DEBUG_ENC_KEY,
}


def _parse_timestamp(value: str, row_number: int) -> tuple[float, str | None]:
    text = value.strip()
    try:
        timestamp = float(text)
        if math.isfinite(timestamp):
            return timestamp, None
    except ValueError:
        pass
    match = re.fullmatch(r"([+-]?[0-9]+(?:\.[0-9]+)?)\s*(ns|us|µs|ms|s)?", text, re.I)
    if match:
        number = float(match.group(1))
        unit = (match.group(2) or "s").lower()
        factor = {"ns": 1e-9, "us": 1e-6, "µs": 1e-6, "ms": 1e-3, "s": 1.0}[unit]
        timestamp = number * factor
        if math.isfinite(timestamp):
            return timestamp, None
    return float(row_number), f"第 {row_number} 行时间无法解析，已按行号排序"


def _parse_byte(value: str) -> int:
    text = value.strip().lower().replace("0x", "").replace(" ", "")
    if not re.fullmatch(r"[0-9a-f]{1,2}", text):
        raise ValueError(f"非法字节: {value!r}")
    return int(text, 16)


def _is_header(row: Sequence[str]) -> bool:
    if len(row) < 2:
        return True
    try:
        _parse_byte(row[1])
    except ValueError:
        return True
    return False


def read_csv_capture(path: str | Path) -> CsvImport:
    """Read the first four columns of a logic-analyzer CSV."""
    path = str(path)
    result = CsvImport(path=path)
    with open(path, "r", encoding="utf-8-sig", newline="") as stream:
        rows = csv.reader(stream)
        for row_number, row in enumerate(rows, start=1):
            if not row or all(not cell.strip() for cell in row):
                continue
            if row_number == 1 and _is_header(row):
                continue
            if len(row) < 4:
                result.invalid_rows += 1
                result.warnings.append(f"第 {row_number} 行少于四列，已忽略")
                continue
            try:
                timestamp, warning = _parse_timestamp(row[0], row_number)
                value = _parse_byte(row[1])
            except ValueError as exc:
                result.invalid_rows += 1
                result.warnings.append(f"第 {row_number} 行无效：{exc}")
                continue
            if warning:
                result.warnings.append(warning)
            checksum_error = bool(row[2].strip())
            frame_error = row[3].strip().lower() == "error"
            if checksum_error:
                result.checksum_marked_rows += 1
            if frame_error:
                result.dropped_frame_error += 1
                continue
            result.items.append(CaptureByte(
                timestamp=timestamp,
                timestamp_text=row[0].strip(),
                value=value,
                checksum_error=checksum_error,
                frame_error=False,
                row_number=row_number,
                timestamp_valid=warning is None,
            ))
    result.items.sort(key=lambda item: (item.timestamp, item.row_number))
    return result


def _find_header(data: Sequence[CaptureByte], start: int) -> int | None:
    for index in range(start, len(data) - 1):
        if data[index].value == 0xEF and data[index + 1].value == 0xAA:
            return index
    return None


def _candidate_lengths(values: Sequence[int]) -> list[_Candidate]:
    if len(values) < 5 or values[0:2] != [0xEF, 0xAA]:
        return []
    candidates: list[_Candidate] = []
    if values[2] in KNOWN_PLAIN_MIDS:
        candidates.append(_Candidate("plain", 6 + (values[3] << 8 | values[4])))
        return candidates
    cipher_length = values[2] << 8 | values[3]
    if 16 <= cipher_length <= 0xFFFF and cipher_length % 16 == 0:
        candidates.append(_Candidate("encrypted", 5 + cipher_length))
    plain_length = values[3] << 8 | values[4]
    if 0 <= plain_length <= 0xFFFF:
        candidates.append(_Candidate("plain", 6 + plain_length))
    return candidates


def _checksum_ok(frame: bytes, kind: str) -> bool:
    if kind == "encrypted":
        return checksum(frame[4:-1]) == frame[-1]
    return checksum(frame[2:-1]) == frame[-1]


def reassemble_stream(items: Sequence[CaptureByte], direction: str) -> list[CapturedFrame]:
    """Reassemble one direction independently, preserving bad-checksum frames."""
    ordered = sorted(items, key=lambda item: (item.timestamp, item.row_number))
    frames: list[CapturedFrame] = []
    position = 0
    while position < len(ordered):
        start = _find_header(ordered, position)
        if start is None:
            break
        candidates: list[_Candidate] = []
        end = start + 2
        while end < len(ordered) and end - start < 5:
            end += 1
        if end - start < 5:
            break
        values = [item.value for item in ordered[start:end]]
        candidates = _candidate_lengths(values)
        if not candidates:
            position = start + 1
            continue
        complete_candidates = [
            candidate for candidate in candidates
            if start + candidate.length <= len(ordered)
        ]
        if not complete_candidates:
            # An incomplete tail is not a complete message.
            break
        valid_candidates: list[_Candidate] = []
        for candidate in complete_candidates:
            candidate_items = ordered[start:start + candidate.length]
            candidate_raw = bytes(item.value for item in candidate_items)
            if _checksum_ok(candidate_raw, candidate.kind):
                valid_candidates.append(candidate)
        if valid_candidates:
            # Prefer known-MID plain frames, then a unique valid candidate.
            plain_candidates = [
                candidate for candidate in valid_candidates
                if candidate.kind == "plain" and ordered[start].value == 0xEF
            ]
            selected = plain_candidates[0] if len(plain_candidates) == 1 else valid_candidates[0]
        else:
            selected = complete_candidates[0]

        selected_items = ordered[start:start + selected.length]
        raw = bytes(item.value for item in selected_items)
        marked_error = any(item.checksum_error for item in selected_items)
        computed_ok = _checksum_ok(raw, selected.kind)
        if not computed_ok and not marked_error:
            # A bad candidate can be caused by a false EF AA. Recover at the next header.
            next_header = _find_header(ordered, start + 1)
            if next_header is not None and next_header < start + selected.length:
                position = next_header
                continue
        diagnostic = ""
        if marked_error:
            diagnostic = "CSV 标记校验错误"
        if not computed_ok:
            diagnostic = (diagnostic + "; " if diagnostic else "") + "计算 XOR 校验失败"
        frames.append(CapturedFrame(
            direction=direction,
            data=raw,
            start_time=selected_items[0].timestamp,
            end_time=selected_items[-1].timestamp,
            start_text=selected_items[0].timestamp_text,
            row_numbers=tuple(item.row_number for item in selected_items),
            frame_type=selected.kind,
            checksum_ok=computed_ok and not marked_error,
            marked_checksum_error=marked_error,
            diagnostic=diagnostic,
            timestamp_valid=selected_items[0].timestamp_valid,
        ))
        position = start + selected.length
    return frames


def merge_captures(lock: Sequence[CapturedFrame], module: Sequence[CapturedFrame]) -> list[CapturedFrame]:
    merged = list(lock) + list(module)
    merged.sort(key=lambda frame: (frame.start_time, 0 if frame.direction == "锁端 → 模组" else 1))
    return merged


def parse_index_text(text: str) -> tuple[int, ...]:
    """Parse exactly 16 decimal indexes supplied by the batch index field."""
    parts = [part.strip() for part in text.split(",")]
    if len(parts) != 16 or any(not re.fullmatch(r"[0-9]+", part) for part in parts):
        raise ProtocolError("索引信息必须是以逗号分隔的 16 个十进制整数")
    indexes = tuple(int(part, 10) for part in parts)
    if any(value > 31 for value in indexes):
        raise ProtocolError("索引必须在 0..31 范围内")
    return indexes


def extract_index_message(frame: CapturedFrame) -> tuple[int, ...] | None:
    """Only a complete, trusted 52/53 can prefill the index field."""
    if frame.frame_type != "plain" or not frame.checksum_ok:
        return None
    try:
        message = parse_plain_frame(frame.data)
    except ProtocolError:
        return None
    if message.mid not in (MID_SET_RELEASE_ENC_KEY, MID_SET_DEBUG_ENC_KEY):
        return None
    if len(message.payload) != 16 or any(value > 31 for value in message.payload):
        return None
    return tuple(message.payload)


def latest_captured_indexes(frames: Sequence[CapturedFrame]) -> tuple[int, ...] | None:
    indexes = None
    for frame in frames:
        found = extract_index_message(frame)
        if found is not None:
            indexes = found
    return indexes


def load_capture_pair(
    lock_path: str | Path, module_path: str | Path,
) -> tuple[CsvImport, CsvImport, list[CapturedFrame]]:
    lock_import = read_csv_capture(lock_path)
    module_import = read_csv_capture(module_path)
    lock_frames = reassemble_stream(lock_import.items, "锁端 → 模组")
    module_frames = reassemble_stream(module_import.items, "模组 → 锁端")
    return lock_import, module_import, merge_captures(lock_frames, module_frames)


class SessionKeyTimeline:
    """Use the chosen index list with each valid 50 seed in time order."""

    def __init__(self, indexes: tuple[int, ...] | None = None) -> None:
        self.indexes = indexes
        self.key: bytes | None = None
        self.key_source = ""
        self.saw_seed = False

    def update(self, frame: CapturedFrame) -> None:
        if not frame.checksum_ok or frame.frame_type != "plain":
            return
        try:
            plain = parse_plain_frame(frame.data)
        except ProtocolError:
            return
        frame.mid = plain.mid
        if plain.mid in (MID_SET_DEBUG_ENC_KEY, MID_SET_RELEASE_ENC_KEY):
            if extract_index_message(frame) is None:
                frame.diagnostic = (frame.diagnostic + "; " if frame.diagnostic else "") + "无效的 52/53 索引"
            else:
                frame.intent = "索引消息（批处理以索引信息框为准）"
            return
        if plain.mid != MID_INIT_ENCRYPTION:
            return
        if len(plain.payload) < 4:
            frame.diagnostic = (frame.diagnostic + "; " if frame.diagnostic else "") + "无效 50 seed"
            return
        self.saw_seed = True
        if self.indexes is None:
            self.key = None
            self.key_source = ""
            frame.diagnostic = (frame.diagnostic + "; " if frame.diagnostic else "") + "索引信息为空，无法派生 AES key"
            return
        _, self.key = derive_session_key(plain.payload[:4], self.indexes)
        self.key_source = f"50@{frame.start_text}"
        frame.intent = "初始化加密（AES）"

    def decode(self, frame: CapturedFrame) -> None:
        if frame.frame_type == "plain":
            try:
                plain = parse_plain_frame(frame.data)
                frame.mid = plain.mid
            except ProtocolError:
                return
            if frame.mid in (MID_SET_DEBUG_ENC_KEY, MID_INIT_ENCRYPTION, MID_SET_RELEASE_ENC_KEY):
                return
            frame.decrypted = frame.data
            return
        if frame.frame_type != "encrypted":
            return
        if not frame.checksum_ok:
            frame.decrypt_diagnostic = "校验错误，未解密"
            return
        if self.key is None:
            frame.decrypt_diagnostic = "索引信息为空或此前没有有效 50 消息"
            return
        try:
            frame.decrypted = decrypt_encrypted_frame(frame.data, self.key)
            frame.mid = parse_plain_frame(frame.decrypted).mid
            frame.key_source = self.key_source
        except ProtocolError as exc:
            frame.decrypt_diagnostic = str(exc)


def analyze_frames(
    frames: Sequence[CapturedFrame], indexes: tuple[int, ...] | None = None,
) -> tuple[list[CapturedFrame], list[str]]:
    # Preserve the existing library entry point: without an explicit selection,
    # use the latest trusted 52/53 as a single index source for the batch.
    if indexes is None:
        indexes = latest_captured_indexes(frames)
    timeline = SessionKeyTimeline(indexes)
    warnings: list[str] = []
    for frame in frames:
        timeline.update(frame)
        timeline.decode(frame)
    if indexes is None:
        warnings.append("索引信息为空：请提供包含 52/53 的抓包或填写索引信息")
    if not timeline.saw_seed:
        warnings.append("缺少有效 50 消息，无法按时间生成 AES 会话密钥")
    return list(frames), warnings


def analyze_csv_pair(
    lock_path: str | Path, module_path: str | Path,
    indexes: tuple[int, ...] | None = None,
) -> BatchAnalysis:
    lock_import, module_import, frames = load_capture_pair(lock_path, module_path)
    captured_indexes = latest_captured_indexes(frames)
    if indexes is None:
        indexes = captured_indexes
    else:
        # Validate callers as well as the text-field parser.
        derive_session_key(bytes(4), indexes)
    frames, warnings = analyze_frames(frames, indexes)
    warnings = lock_import.warnings + module_import.warnings + warnings
    return BatchAnalysis(
        frames=frames,
        lock_import=lock_import,
        module_import=module_import,
        warnings=warnings,
        missing_debug_key=captured_indexes is None,
        missing_seed=not any(
            frame.checksum_ok and frame.mid == MID_INIT_ENCRYPTION
            and frame.frame_type == "plain" and len(frame.data) >= 10
            for frame in frames
        ),
        captured_indexes=captured_indexes,
    )
