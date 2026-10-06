"""Optional cross-check against the repository's actual C++ AES source."""
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from protocol import aes_ecb_encrypt, build_plain_frame


class CppCrossCheck(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ unavailable")
    def test_repository_aes(self):
        root = Path(__file__).resolve().parents[3]
        source = r'''
#include "aes.h"
#include <cstdio>
int main() {
    const unsigned char key[] = "5da6aadc280a1dda";
    AES_ctx ctx;
    AES_init_ctx(&ctx, key);
    unsigned char block[16] = {};
    while (std::fread(block, 1, 16, stdin) == 16) {
        AES_ECB_encrypt(&ctx, block);
        for (unsigned char b : block) std::printf("%02x", b);
    }
}
'''
        # Hex input is embedded to avoid Windows text-mode stdin translations.
        inner = build_plain_frame(0x12, bytes(range(29)))[2:-1]
        padded = inner + bytes((-len(inner)) % 16)
        source = source.replace(
            'while (std::fread(block, 1, 16, stdin) == 16) {',
            'const unsigned char data[] = {' + ','.join(map(str, padded)) + '};\n'
            'for (unsigned int i = 0; i < sizeof(data); i += 16) {\n'
            'for (int j = 0; j < 16; ++j) block[j] = data[i+j];'
        )
        with tempfile.TemporaryDirectory() as folder:
            harness = Path(folder) / "crosscheck.cpp"
            exe = Path(folder) / "crosscheck.exe"
            harness.write_text(source, encoding="utf-8")
            compiled = subprocess.run([
                shutil.which("g++"), "-DUSE_SOFT_AES_ENCRPT", str(harness),
                str(root / "application/communication/src/aes.cpp"),
                "-I" + str(root / "application/communication/include"),
                "-o", str(exe),
            ], capture_output=True, timeout=60)
            self.assertEqual(compiled.returncode, 0, compiled.stderr.decode(errors="replace"))
            actual = subprocess.check_output([str(exe)], timeout=10).decode()
        self.assertEqual(actual, aes_ecb_encrypt(inner, b"5da6aadc280a1dda").hex())
