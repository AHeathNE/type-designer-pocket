"""M3: the type test sets text exactly as GB Studio will (checked against the exporter), plus widths."""
import re
import shutil
import tempfile
import unittest
from pathlib import Path

from PIL import Image

from game import FONTS, ROM, ROOT, SANS, Game, glyph_index
import sav2font  # noqa: E402  (tools/ is on sys.path via game)
import tdpsave  # noqa: E402

LINE_PX = 144
LINES = 6
SERIF = FONTS["pocket-serif"]


def c_samples():
    """The built-in samples, read from src/typetest.c so the test can't drift from the game.

    Each sample is one or more adjacent C string literals; escapes like \\xE9 give the character with that
    code (Latin-1, or one of the symbols at 128-159).
    """
    src = (ROOT / "src" / "typetest.c").read_text()
    block = src[src.index("samples[SAMPLE_COUNT - 1] = {"):]
    block = block[: block.index("};")]
    block = re.sub(r"//[^\n]*", "", block)
    out = []
    for item in re.findall(r'((?:"(?:[^"\\]|\\.)*"\s*)+),', block):
        lits = re.findall(r'"((?:[^"\\]|\\.)*)"', item)
        out.append("".join(bytes(lit, "latin-1").decode("unicode_escape") for lit in lits))
    return out


# ---- reference: GB Studio's view of the font, built only from the exporter's code ----

def advance(slot, c):
    rows, override = sav2font.tile_for(slot, ord(c))
    return tdpsave.export_tile(rows or [[0] * 8] * 8, override, slot.space_width)


def layout(slot, s, max_lines):
    """Greedy wrap at LINE_PX: break at the last space that fits, never start a line with spaces."""
    lines, i = [], 0
    while i < len(s) and len(lines) < max_lines:
        while i < len(s) and s[i] == " ":
            i += 1
        if i >= len(s):
            break
        start, x, brk, j = i, 0, None, i
        while j < len(s):
            w = advance(slot, s[j])[1]
            if x + w > LINE_PX:
                break
            if s[j] == " ":
                brk = j
            x += w
            j += 1
        if j < len(s) and brk is not None and brk > start:
            lines.append(s[start:brk])
            i = brk + 1
        else:
            lines.append(s[start:j])
            i = j
    return lines


def render(slot, line):
    """8 rows x LINE_PX pixels of one line, as GB Studio would draw it."""
    px = [[0] * LINE_PX for _ in range(8)]
    x = 0
    for c in line:
        if x >= LINE_PX:
            break
        ink, w = advance(slot, c)
        for y in range(8):
            for dx in range(8):
                if ink[y][dx] and x + dx < LINE_PX:
                    px[y][x + dx] = 1
        x += w
    return px


def vram_line(game, tile0):
    px = [[0] * LINE_PX for _ in range(8)]
    for t in range(LINE_PX // 8):
        lo, hi = game.bg_tile(tile0 + t)
        for y in range(8):
            assert lo[y] == hi[y], "text is drawn in black only"
            for dx in range(8):
                px[y][t * 8 + dx] = (lo[y] >> (7 - dx)) & 1
    return px


class M3(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.rom = self.tmp / ROM.name
        self.ram = Path(str(self.rom) + ".ram")
        shutil.copy(ROM, self.rom)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def serif_slot(self):
        """Pocket Serif copied in, with some glyphs missing and some fixed widths, to exercise every rule."""
        slot = tdpsave.Slot(name="FONT 1", template_id=1)
        for i, g in enumerate(SERIF):
            slot.glyphs[i] = bytes(g)
        for ch in "qxzQ7?":
            slot.glyphs[glyph_index(ch)] = bytes(8)          # missing; x/z fall back to X/Z, q/Q/7/? stay blank
        for ch in "abcdefghij":
            slot.glyphs[glyph_index(ch)] = bytes(8)          # these use the capitals
        slot.widths[glyph_index("i")] = 3                     # override on a missing glyph: ignored (I is used)
        slot.widths[glyph_index("I")] = 3
        slot.widths[glyph_index("7")] = 6                     # override on a blank glyph: a 6-wide gap
        slot.widths[glyph_index(" ")] = 3
        return slot

    def assert_matches_exporter(self, game, slot, text):
        expect = layout(slot, text, LINES)
        for k in range(LINES):
            want = render(slot, expect[k]) if k < len(expect) else [[0] * LINE_PX for _ in range(8)]
            self.assertEqual(vram_line(game, k * 18), want, f"line {k + 1} of {text!r}")
            for c in range(18):
                self.assertEqual(game.tile(1 + c, 2 + 2 * k), k * 18 + c)
        return expect

    def open_type_test(self, game):
        game.open_slot(1)
        game.map_menu(0)                        # TYPE TEST
        game.wait_typetest()
        game.settle()

    # ---- type test ----

    def test_type_test_matches_the_exporter(self):
        slot = self.serif_slot()
        self.ram.write_bytes(tdpsave.build([slot]))
        game = Game(self.rom)
        self.open_type_test(game)
        samples = c_samples()
        self.assertEqual(len(samples), 6)
        self.assertTrue(any(ord(c) > 191 for c in samples[4]), "sample 5 has accented letters")
        self.assertTrue(all(32 <= ord(c) < 160 for c in samples[5]) and any(ord(c) >= 128 for c in samples[5]))
        for n, text in enumerate(samples):
            self.assertEqual(game.text(0), f"TYPE TEST      <{n + 1}/7>")
            lines = self.assert_matches_exporter(game, slot, text)
            self.assertGreater(len(lines), 0)
            missing = sorted({c for c in text if c not in " \xa0" and advance(slot, c)[0] == [[0] * 8] * 8
                              and sav2font.tile_for(slot, ord(c))[0] is None}, key=text.index)
            if missing:
                self.assertEqual(game.text(14)[:11].rstrip(), f"MISSING {len(missing)}".ljust(11).rstrip())
                for k, c in enumerate(missing[:8]):   # each shown in Pocket Sans
                    t = game.tile(12 + k, 14)
                    self.assertEqual(game.bg_tile(t)[0], SANS[ord(c) - 32], f"missing {c!r}")
            else:
                self.assertEqual(game.text(14), "ALL GLYPHS DRAWN")
            game.shot(f"m3-typetest-{n + 1}")
            game.press("right")
            game.settle()
        self.assertEqual(game.text(2), " (A: WRITE YOUR OWN)")   # sample 7 is yours, still empty
        game.press("right")
        game.settle()
        self.assertEqual(game.text(0), "TYPE TEST      <1/7>")
        game.press("left")
        game.press("left")
        game.settle()
        self.assertEqual(game.text(0), "TYPE TEST      <6/7>")
        game.press("b")
        game.wait_map()
        game.power_off(save=False)

    def test_wrapping_and_all_drawn(self):
        slot = tdpsave.Slot(name="FONT 1")
        for i, g in enumerate(SANS):
            slot.glyphs[i] = bytes(g)
        self.ram.write_bytes(tdpsave.build([slot]))
        game = Game(self.rom)
        self.open_type_test(game)
        lines = self.assert_matches_exporter(game, slot, c_samples()[0])
        self.assertEqual(game.text(14), "ALL GLYPHS DRAWN")
        self.assertGreaterEqual(len(lines), 2, "the pangram should wrap")
        game.power_off(save=False)

    # ---- your own text ----

    def type_text(self, game, s):
        key = glyph_index("A")                  # where the keyboard starts
        for ch in s:
            if ch == " ":
                game.press("select")
                continue
            want = glyph_index(ch)
            while key % 16 != want % 16:
                game.press("right" if want % 16 > key % 16 else "left")
                key += 1 if want % 16 > key % 16 else -1
            while key // 16 != want // 16:
                game.press("down" if want > key else "up")
                key += 16 if want > key else -16
            game.press("a")

    def test_keyboard_writes_your_own_sample(self):
        slot = self.serif_slot()
        self.ram.write_bytes(tdpsave.build([slot]))
        game = Game(self.rom)
        self.open_type_test(game)
        game.press("a")
        game.wait_keyboard()
        self.type_text(game, "Pixel Fonts!x")
        game.settle()
        self.assertEqual(game.text(17)[:5], "13/40")
        game.press("b")                         # delete the x
        game.settle()
        self.assertEqual(game.text(17)[:5], "12/40")
        game.shot("m3-keyboard")
        game.press("start")
        game.wait_typetest()
        game.settle()
        self.assertEqual(game.text(0), "TYPE TEST      <7/7>")
        self.assert_matches_exporter(game, slot, "Pixel Fonts!")
        game.power_off(save=False)

    # ---- widths ----

    def test_width_setting_reaches_the_export(self):
        slot = tdpsave.Slot(name="FONT 1")
        for i, g in enumerate(SANS):
            slot.glyphs[i] = bytes(g)
        self.ram.write_bytes(tdpsave.build([slot]))
        a = glyph_index("A")
        auto = tdpsave.export_tile(slot.rows(a))[1]       # Pocket Sans A: ink 5 wide -> 7
        self.assertEqual(auto, 7)

        game = Game(self.rom)
        game.open_slot(1)
        game.open_editor()
        self.assertEqual(game.text(0)[7:9], "W7")
        game.menu(6)                                      # WIDTH
        self.assertEqual(game.text(17), "<>:WIDTH B:AUTO A:OK")
        # markers: ink starts at column 1 and moves to column 1, so the glyph starts at the cell edge
        self.assertEqual(game.oam(4)[0], 16 + 0 * 16 - 1 + 8)
        self.assertEqual(game.oam(12)[0], 16 + 7 * 16 - 1 + 8)
        game.press("left")
        game.press("left")
        self.assertEqual(game.text(0)[7:10], "W5*")
        self.assertEqual(game.oam(12)[0], 16 + 5 * 16 - 1 + 8)
        game.shot("m3-width")
        game.press("a")
        self.assertEqual(game.text(17), "A:INK B:RUB ST:MENU")

        game.combo("select", "right")                     # B: fixed width 8, then back to auto
        game.menu(6)
        for _ in range(3):
            game.press("right")
        game.press("b")
        game.press("a")
        self.assertEqual(game.text(0)[7:10], "W7 ")

        for _ in range(34):                               # back to the space (index 0)
            game.combo("select", "left")
        self.assertTrue(game.text(0).startswith("  #32"))
        game.menu(6)
        game.press("left")
        game.press("left")                                # space: 4 -> 2
        game.press("a")
        game.back_to_map()
        game.power_off()

        slot = tdpsave.parse(self.ram.read_bytes()).slots[0]
        self.assertEqual(slot.widths[a], 5)
        self.assertEqual(slot.widths[a + 1], tdpsave.AUTO)
        self.assertEqual(slot.widths[0], 2)
        out = self.tmp / "out"
        sav2font.main([str(self.ram), "--out", str(out), "--name", "W"])
        img = Image.open(out / "w.png").convert("RGB")

        def width(code):
            i = code - 32
            tx, ty = (i % 16) * 8, (i // 16) * 8
            return next((x for x in range(8) if img.getpixel((tx + x, ty)) == sav2font.GB_TRIM), 8)

        self.assertEqual(width(ord("A")), 5)
        self.assertEqual(width(ord("B")), 7)
        self.assertEqual(width(ord(" ")), 2)


if __name__ == "__main__":
    unittest.main()
