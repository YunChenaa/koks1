"""SenseLock UART AES protocol helpers."""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import re
from typing import Final

from Crypto.Cipher import AES

HEADER: Final[bytes] = b"\xEF\xAA"
MID_REPLY: Final[int] = 0x00
MID_INIT_ENCRYPTION: Final[int] = 0x50
MID_SET_RELEASE_ENC_KEY: Final[int] = 0x52
MID_SET_DEBUG_ENC_KEY: Final[int] = 0x53
AES_BLOCK_SIZE: Final[int] = 16


class ProtocolError(ValueError):
    """Raised when a protocol input is invalid."""


@dataclass(frozen=True)
class PlainFrame:
    mid: int
    payload: bytes

    @property
    def inner(self) -> bytes:
        return bytes((self.mid,)) + len(self.payload).to_bytes(2, "big") + self.payload


@dataclass(frozen=True)
class EncryptedFrame:
    ciphertext: bytes


def parse_hex(text: str) -> bytes:
    cleaned = re.sub(r"0x", "", text, flags=re.IGNORECASE)
    cleaned = re.sub(r"\s+", "", cleaned)
    if not cleaned:
        raise ProtocolError("十六进制输入不能为空")
    if len(cleaned) % 2:
        raise ProtocolError("十六进制输入的字符数必须为偶数")
    if re.fullmatch(r"[0-9a-fA-F]+", cleaned) is None:
        raise ProtocolError("输入包含非法十六进制字符")
    return bytes.fromhex(cleaned)


def format_hex(data: bytes, columns: int = 0) -> str:
    if columns <= 0:
        return data.hex(" ").upper()
    return "\n".join(data[i:i + columns].hex(" ").upper() for i in range(0, len(data), columns))


def checksum(data: bytes) -> int:
    result = 0
    for byte in data:
        result ^= byte
    return result


def build_plain_frame(mid: int, payload: bytes) -> bytes:
    if not 0 <= mid <= 0xFF:
        raise ProtocolError("消息 MID 必须在 0x00 到 0xFF 范围内")
    if len(payload) > 0xFFFF:
        raise ProtocolError("业务数据不能超过 65535 字节")
    inner = bytes((mid,)) + len(payload).to_bytes(2, "big") + payload
    return HEADER + inner + bytes((checksum(inner),))


def parse_plain_frame(frame: bytes, expected_mid: int | None = None) -> PlainFrame:
    if len(frame) < 6:
        raise ProtocolError("明文帧长度不足，至少需要 6 字节")
    if frame[:2] != HEADER:
        raise ProtocolError("明文帧头必须是 EF AA")
    payload_len = int.from_bytes(frame[3:5], "big")
    expected_len = 6 + payload_len
    if len(frame) != expected_len:
        raise ProtocolError(f"明文帧长度不匹配：需要 {expected_len} 字节，实际为 {len(frame)} 字节")
    if checksum(frame[2:-1]) != frame[-1]:
        raise ProtocolError("明文帧 XOR 校验失败")
    mid = frame[2]
    if expected_mid is not None and mid != expected_mid:
        raise ProtocolError(f"MID 不匹配：需要 0x{expected_mid:02X}，实际为 0x{mid:02X}")
    return PlainFrame(mid=mid, payload=frame[5:-1])


def parse_debug_key_message(text: str) -> tuple[PlainFrame, tuple[int, ...]]:
    frame = parse_plain_frame(parse_hex(text), MID_SET_DEBUG_ENC_KEY)
    if len(frame.payload) != 16:
        raise ProtocolError(f"53 消息必须携带 16 字节索引，实际为 {len(frame.payload)} 字节")
    indexes = tuple(frame.payload)
    if any(not 0 <= value <= 31 for value in indexes):
        raise ProtocolError("索引必须在 0..31 范围内")
    return frame, indexes


def parse_init_message(text: str) -> tuple[PlainFrame, bytes, int | None]:
    frame = parse_plain_frame(parse_hex(text), MID_INIT_ENCRYPTION)
    if len(frame.payload) < 4:
        raise ProtocolError("50 消息至少需要 4 字节 seed")
    mode = frame.payload[4] if len(frame.payload) >= 5 else None
    return frame, frame.payload[:4], mode


def derive_session_key(seed: bytes, indexes: tuple[int, ...]) -> tuple[str, bytes]:
    if len(seed) != 4:
        raise ProtocolError("seed 必须正好是 4 个原始字节")
    if len(indexes) != 16:
        raise ProtocolError("索引数组必须正好包含 16 个字节")
    if any(not 0 <= value <= 31 for value in indexes):
        raise ProtocolError("索引必须在 0..31 范围内")
    md5_text = hashlib.md5(seed).hexdigest()
    key_text = "".join(md5_text[index] for index in indexes)
    return md5_text, key_text.encode("ascii")


def _zero_pad(data: bytes) -> bytes:
    padded_len = ((len(data) + 15) // 16) * 16
    return data + bytes(padded_len - len(data))


def aes_ecb_encrypt(inner: bytes, key: bytes) -> bytes:
    if len(key) != 16:
        raise ProtocolError("AES-128 密钥必须为 16 字节")
    return AES.new(key, AES.MODE_ECB).encrypt(_zero_pad(inner))


def aes_ecb_decrypt(ciphertext: bytes, key: bytes) -> bytes:
    if len(key) != 16:
        raise ProtocolError("AES-128 密钥必须为 16 字节")
    if not ciphertext or len(ciphertext) % 16:
        raise ProtocolError("AES 密文长度必须是 16 的正整数倍")
    return AES.new(key, AES.MODE_ECB).decrypt(ciphertext)


def build_encrypted_frame(inner: bytes, key: bytes) -> bytes:
    ciphertext = aes_ecb_encrypt(inner, key)
    if len(ciphertext) > 0xFFFF:
        raise ProtocolError("密文不能超过 65535 字节")
    return HEADER + len(ciphertext).to_bytes(2, "big") + ciphertext + bytes((checksum(ciphertext),))


def parse_encrypted_frame(frame: bytes) -> EncryptedFrame:
    if len(frame) < 21:
        raise ProtocolError("AES 帧长度不足")
    if frame[:2] != HEADER:
        raise ProtocolError("AES 帧头必须是 EF AA")
    cipher_len = int.from_bytes(frame[2:4], "big")
    expected_len = 5 + cipher_len
    if len(frame) != expected_len:
        raise ProtocolError(f"AES 帧长度不匹配：需要 {expected_len} 字节，实际为 {len(frame)} 字节")
    if cipher_len == 0 or cipher_len % 16:
        raise ProtocolError("AES 密文长度必须是 16 的正整数倍")
    ciphertext = frame[4:-1]
    if checksum(ciphertext) != frame[-1]:
        raise ProtocolError("AES 帧 XOR 校验失败")
    return EncryptedFrame(ciphertext=ciphertext)


def validate_inner(inner: bytes) -> PlainFrame:
    if len(inner) < 3:
        raise ProtocolError("解密后的消息内文长度不足")
    payload_len = int.from_bytes(inner[1:3], "big")
    required = 3 + payload_len
    if len(inner) < required:
        raise ProtocolError("解密后的长度字段超过实际内容")
    if len(inner) != ((required + 15) // 16) * 16:
        raise ProtocolError("解密后的块数与内层长度不匹配")
    if any(inner[required:]):
        raise ProtocolError("解密后的补零区域包含非零字节，可能使用了错误的密钥")
    return PlainFrame(mid=inner[0], payload=inner[3:required])


def encrypt_plain_frame(frame: bytes, key: bytes) -> bytes:
    return build_encrypted_frame(parse_plain_frame(frame).inner, key)


def decrypt_encrypted_frame(frame: bytes, key: bytes, *, compatibility: bool = False) -> bytes:
    encrypted = parse_encrypted_frame(frame)
    ciphertext = encrypted.ciphertext
    if compatibility:
        # The firmware declares one extra block only for aligned inner messages.
        if len(ciphertext) < 32 or ciphertext[-16:] != bytes(16):
            raise ProtocolError("兼容模式要求密文末尾有一个额外的全零块")
        inner = aes_ecb_decrypt(ciphertext[:-16], key)
        plain = validate_inner(inner)
        if len(plain.inner) != len(inner):
            raise ProtocolError("兼容模式仅适用于内文长度恰好为整块的固件报文")
    else:
        plain = validate_inner(aes_ecb_decrypt(ciphertext, key))
    return build_plain_frame(plain.mid, plain.payload)
