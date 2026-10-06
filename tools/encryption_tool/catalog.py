"""Message-ID catalog extracted from the project's protocol headers."""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re

from protocol import MID_INIT_ENCRYPTION, MID_SET_DEBUG_ENC_KEY, MID_SET_RELEASE_ENC_KEY


@dataclass(frozen=True)
class MessageIntent:
    mid: int
    name: str
    comment: str
    source: str

    @property
    def display(self) -> str:
        if self.comment:
            return f"{self.name}: {self.comment}"
        return self.name


_LINE_RE = re.compile(
    r"\b(?P<name>MID_[A-Za-z0-9_]+)\s*=\s*(?P<value>0[xX][0-9A-Fa-f]+|\d+)\b"
    r"(?:\s*;)?\s*(?://\s*(?P<comment>.*))?$"
)


def _parse_file(path: Path) -> tuple[list[MessageIntent], list[str]]:
    entries: list[MessageIntent] = []
    warnings: list[str] = []
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError as exc:
        return [], [f"无法读取消息目录 {path}: {exc}"]
    for line_number, line in enumerate(lines, start=1):
        match = _LINE_RE.search(line)
        if not match:
            continue
        try:
            mid = int(match.group("value"), 0)
        except ValueError:
            warnings.append(f"{path}:{line_number} 的 MID 数值无法解析")
            continue
        entries.append(MessageIntent(
            mid=mid,
            name=match.group("name"),
            comment=(match.group("comment") or "").strip(),
            source=f"{path.name}:{line_number}",
        ))
    return entries, warnings


def build_catalog(project_root: str | Path) -> tuple[dict[int, tuple[MessageIntent, ...]], list[str]]:
    root = Path(project_root)
    entries: list[MessageIntent] = []
    warnings: list[str] = []
    for relative in (
        Path("application/communication/include/message_l.h"),
        Path("application/communication/include/message_v2.h"),
    ):
        parsed, file_warnings = _parse_file(root / relative)
        entries.extend(parsed)
        warnings.extend(file_warnings)
    fallback = (
        MessageIntent(MID_INIT_ENCRYPTION, "MID_INIT_ENCRYPTION", "初始化加密", "内置"),
        MessageIntent(MID_SET_RELEASE_ENC_KEY, "MID_SET_RELEASE_ENC_KEY", "设置发布密钥", "内置"),
        MessageIntent(MID_SET_DEBUG_ENC_KEY, "MID_SET_DEBUG_ENC_KEY", "设置调试密钥索引", "内置"),
    )
    entries.extend(fallback)
    catalog: dict[int, list[MessageIntent]] = {}
    seen: set[tuple[int, str, str]] = set()
    for entry in entries:
        key = (entry.mid, entry.name, entry.comment)
        if key not in seen:
            seen.add(key)
            catalog.setdefault(entry.mid, []).append(entry)
    return {mid: tuple(values) for mid, values in catalog.items()}, warnings


_STATE_RE = re.compile(
    r"\b(?P<name>(?:FACE|PALM)_STATE_[A-Za-z0-9_]+)\s*=\s*"
    r"(?P<value>0[xX][0-9A-Fa-f]+|\d+)\s*;"
)


def build_state_catalog(project_root: str | Path) -> dict[str, dict[int, str]]:
    """Read numeric face and palm status names from the protocol header."""
    path = Path(project_root) / "application/communication/include/message_v2.h"
    try:
        contents = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return {"FACE": {}, "PALM": {}}
    states: dict[str, dict[int, str]] = {"FACE": {}, "PALM": {}}
    for match in _STATE_RE.finditer(contents):
        name = match.group("name")
        family = name.split("_", 1)[0]
        # Aliases can share a value across firmware variants; use the first.
        states[family].setdefault(int(match.group("value"), 0), name)
    return states


def format_mid(
    mid: int | None,
    payload: bytes | None,
    catalog: dict[int, tuple[MessageIntent, ...]],
    states: dict[str, dict[int, str]] | None = None,
) -> str:
    """Render MID;hex-code;reply-result-or-note-state without header comments."""
    if mid is None:
        return "未知 MID"
    note = mid in (0x01, 0x04, 0x08)
    name = "MID_NOTE" if note else (
        "MID_REPLY" if mid == 0 else
        catalog[mid][0].name if catalog.get(mid) else f"UNKNOWN_0x{mid:02X}"
    )
    fields = [name, f"{mid:02X}"]
    if payload is None:
        return ";".join(fields)
    if mid == 0x00 and len(payload) >= 2:
        fields.append("SUCCESS" if payload[1] == 0 else "FAIL")
    elif note and payload:
        nid = payload[0]
        if mid == 0x01 and nid == 0:
            fields.append("ready")
        elif nid in (1, 4, 8):
            # A face/palm state is int16_t in the protocol. NID 04 can
            # alternatively be the single-byte eye state; NID 08 can be
            # authorization with no state. Do not label either as palm
            # without both state bytes.
            if len(payload) >= 3:
                state = int.from_bytes(payload[1:3], "little", signed=True)
                family = "FACE" if nid == 1 else "PALM"
                label = (states or {}).get(family, {}).get(state)
                fields.append(label if label else f"state={state}")
            elif len(payload) == 2:
                fields.append(f"state={payload[1]}")
    return ";".join(fields)


def describe_mid(mid: int | None, catalog: dict[int, tuple[MessageIntent, ...]]) -> str:
    if mid is None:
        return "未知 MID"
    entries = catalog.get(mid)
    if not entries:
        return f"0x{mid:02X}（未提供注释）"
    return "；".join(entry.display for entry in entries)
