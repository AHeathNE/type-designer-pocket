"""The website's JavaScript (web/js/save.js, otf.js) must read saves and place glyphs exactly like the Python tools.

Runs the JS under Node; skipped if Node isn't installed. The browser-only parts (the emulator, PNG and .otf
files) were checked in a browser: see README, "Website".
"""
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from game import FONTS, ROOT, glyph_index
from test_m3 import c_samples, layout
import sav2font  # noqa: E402
import tdpsave  # noqa: E402

NODE = shutil.which("node")


@unittest.skipUnless(NODE, "Node.js isn't installed")
class Web(unittest.TestCase):
    def test_js_matches_python(self):
        serif = tdpsave.Slot(name="SERIF MIX", template_id=1)
        for i, g in enumerate(FONTS["pocket-serif"]):
            serif.glyphs[i] = bytes(g)
        for ch in "abcqQ7":
            serif.glyphs[glyph_index(ch)] = bytes(8)
        serif.widths[glyph_index("A")] = 4
        serif.widths[glyph_index("7")] = 6
        serif.widths[0] = 2
        serif.space_width = 3
        classic = tdpsave.Slot(name="CHUNKY", template_id=2)
        for i, g in enumerate(FONTS["classic-8x8"]):
            classic.glyphs[i] = bytes(g)
        classic.glyphs[100] = bytes([0xFF] * 8)          # a full 8x8 block: width 8, column 0
        blank = tdpsave.Slot(name="EMPTY", template_id=0xFF)
        data = bytearray(tdpsave.build([serif, classic, blank], active=1))
        data[16 + 2 * 2064 + 5] ^= 1                     # damage slot 3

        texts = c_samples() + ["Hi! A7 7A", "x" * 60 + " y"]
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "t.sav"
            path.write_bytes(bytes(data))
            out = subprocess.run([NODE, str(ROOT / "tests" / "web_check.mjs"), str(path), *texts],
                                 capture_output=True, text=True, check=True, cwd=ROOT)
        js = json.loads(out.stdout)

        py = tdpsave.parse(bytes(data))
        self.assertEqual(js["active"], py.active)
        self.assertEqual(js["headerOk"], py.header_ok)
        for jslot, pslot in zip(js["slots"], py.slots):
            self.assertEqual(jslot["name"], pslot.name)
            self.assertEqual(jslot["templateId"], pslot.template_id)
            self.assertEqual(jslot["spaceWidth"], pslot.space_width)
            self.assertEqual(jslot["crcOk"], pslot.crc_ok)
            for k, code in enumerate(range(32, 256)):
                rows, override = sav2font.tile_for(pslot, code)
                ink, width = tdpsave.export_tile(rows or [[0] * 8] * 8, override, pslot.space_width)
                self.assertEqual(jslot["tiles"][k], {"ink": ink, "width": width}, f"{pslot.name} code {code}")
                self.assertEqual(jslot["areas"][k], sum(map(sum, ink)), f"{pslot.name} code {code}: outline area")
            for t, lines in zip(texts, jslot["layouts"]):
                self.assertEqual(lines, layout(pslot, t, 99), f"{pslot.name}: {t!r}")
        self.assertFalse(js["slots"][2]["crcOk"])


if __name__ == "__main__":
    unittest.main()
