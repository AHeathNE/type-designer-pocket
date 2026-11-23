"""M1: character map, glyph editor, battery save slot and the .sav -> GB Studio extractor."""
import json
import shutil
import tempfile
import unittest
import zipfile
from pathlib import Path

from PIL import Image

from game import CELL_INK, CELL_OFF, ROM, SANS, TILE_CANVAS, Game, glyph_index, rows_to_bytes
import sav2font  # noqa: E402  (tools/ is on sys.path via game)
import tdpsave  # noqa: E402

A = glyph_index("A")


class M1(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.rom = self.tmp / ROM.name
        self.ram = Path(str(self.rom) + ".ram")
        shutil.copy(ROM, self.rom)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def draw_L(self, game):
        """In the editor (cursor starts at 1,1): a stem down column 1 and a foot along row 6, then erase the foot's end."""
        game.hold("a")                       # A on an empty pixel paints ink...
        for _ in range(5):
            game.press("down")               # ...and keeps painting while held
        game.release("a")                    # (1,6)
        game.press("right")                  # (2,6), not painted: A is up
        game.hold("a")
        game.press("right")
        game.press("right")                  # (4,6)
        game.release("a")
        game.press("b")                      # erase (4,6)

    # ---- cartridge ----

    def test_header(self):
        d = ROM.read_bytes()
        self.assertEqual(d[0x147], 0x1B, "cart type should be MBC5+RAM+BATTERY")
        self.assertEqual(d[0x149], 0x02, "save RAM should be 8 KB")

    def test_generated_tiles_have_pixels(self):
        """Tiles the game builds pixel by pixel at boot. GBDK 4.6.0 miscompiled this into blank tiles."""
        game = Game(self.rom)
        # ink cell, top-left corner: grid line (colour 2) along row 0 and column 0, black (3) elsewhere
        lo, hi = game.bg_tile(TILE_CANVAS + CELL_INK)
        self.assertEqual(lo, [0x00] + [0x7F] * 7)
        self.assertEqual(hi, [0xFF] * 8)
        # off cell, top-left corner: light grey (1) grid line on white
        lo, hi = game.bg_tile(TILE_CANVAS + CELL_OFF)
        self.assertEqual(lo, [0xFF] + [0x80] * 7)
        self.assertEqual(hi, [0x00] * 8)
        # cursor corner sprite (tile 0 at 0x8000): black top edge 5 px wide
        spr = [game.gb.memory[0x8000 + i] for i in range(4)]
        self.assertEqual(spr[:2], [0xF8, 0xF8])
        game.power_off(save=False)

    # ---- fresh cartridge ----

    def test_fresh_cartridge_formats_save_and_shows_guides(self):
        game = Game(self.rom)
        game.open_slot(1)
        game.shot("m1-map-fresh")
        self.assertEqual(game.text(1), "FONT 1        SANS")
        self.assertIn("DRAWN 0/95", game.text(16))
        # undrawn 'A' on the map is the Pocket Sans guide in light grey: low plane only
        lo, hi = game.bg_tile(A)
        self.assertEqual(lo, SANS[A])
        self.assertEqual(hi, [0] * 8)
        self.assertEqual(game.tile(2 + A % 16, 3 + 2 * (A // 16)), A)
        game.power_off()

        save = tdpsave.parse(self.ram.read_bytes())
        self.assertTrue(save.header_ok)
        self.assertEqual([s.name for s in save.slots], ["FONT 1", "FONT 2", "FONT 3"])
        for s in save.slots:
            self.assertTrue(s.crc_ok, "the game's CRC must match tools/tdpsave.py")
            self.assertEqual(s.template_id, 0)
            self.assertEqual(s.space_width, 4)
            self.assertEqual(s.widths, [tdpsave.AUTO] * tdpsave.GLYPHS)
            self.assertFalse(any(s.has_ink(i) for i in range(tdpsave.GLYPHS)))

    # ---- drawing, saving, exporting ----

    def test_draw_save_and_export_to_gb_studio(self):
        game = Game(self.rom)
        game.open_slot(1)
        game.open_editor()
        self.assertTrue(game.text(0).startswith("A #65"))
        self.draw_L(game)
        game.shot("m1-editor")
        self.assertIn("W5", game.text(0))          # ink 3 wide + a blank column each side
        self.assertEqual(game.canvas_cell(1, 1), CELL_INK)
        self.assertEqual(game.canvas_cell(4, 6), CELL_OFF)   # erased; Pocket Sans 'A' has no guide there
        game.back_to_map()
        game.shot("m1-map-drawn")
        self.assertIn("DRAWN 1/95", game.text(16))
        lo, hi = game.bg_tile(A)                    # drawn glyph: black ink, no guide
        self.assertEqual(lo, hi)
        game.power_off()

        save = tdpsave.parse(self.ram.read_bytes())
        slot = save.slots[0]
        self.assertTrue(slot.crc_ok)
        expect = [0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x70, 0x00]
        self.assertEqual(list(slot.glyphs[A]), expect)
        self.assertEqual(lo, expect)
        self.assertEqual(sum(slot.has_ink(i) for i in range(tdpsave.GLYPHS)), 1)

        out = self.tmp / "out"
        self.assertEqual(sav2font.main([str(self.ram), "--out", str(out), "--name", "Test Font"]), 0)
        self.assertEqual(json.loads((out / "test-font.json").read_text()), {"name": "Test Font", "mapping": {}})
        self.assertTrue((out / "HOW-TO-USE.txt").exists())
        img = Image.open(out / "test-font.png").convert("RGB")
        self.assertEqual(img.size, (128, 112))

        def tile(code):
            i = code - 32
            tx, ty = (i % 16) * 8, (i // 16) * 8
            return [[img.getpixel((tx + x, ty + y)) for x in range(8)] for y in range(8)]

        def ink(t):
            return [[1 if p == sav2font.GB_INK else 0 for p in row] for row in t]

        def width(t):
            return next((x for x in range(8) if t[0][x] == sav2font.GB_TRIM), 8)

        a = tile(ord("A"))
        self.assertEqual(rows_to_bytes(ink(a)), expect)        # ink already starts at column 1
        self.assertEqual(width(a), 5)
        self.assertEqual(tile(ord("a")), a, "lowercase falls back to the drawn capital")
        self.assertEqual(width(tile(ord("B"))), 4)              # undrawn: blank, space width
        self.assertEqual(rows_to_bytes(ink(tile(ord("B")))), [0] * 8)
        self.assertEqual(width(tile(ord(" "))), 4)
        colours = {c for _, c in img.getcolors()}
        self.assertLessEqual(colours, {sav2font.GB_BG, sav2font.GB_INK, sav2font.GB_TRIM})

        zip_path = sav2font.export(save, 0, "Test Font", self.tmp / "z", make_zip=True)[0]
        with zipfile.ZipFile(zip_path) as z:
            self.assertEqual(sorted(z.namelist()), ["HOW-TO-USE.txt", "test-font.json", "test-font.png"])

    # ---- saves written outside the game ----

    def test_save_written_by_python_loads(self):
        slot = tdpsave.Slot(name="FONT 1")
        h = glyph_index("H")
        slot.glyphs[h] = bytes(SANS[h])
        self.ram.write_bytes(tdpsave.build([slot]))
        game = Game(self.rom)
        self.assertEqual(game.text(3), "> FONT 1      1/95")
        self.assertEqual(game.text(4), "   POCKET SANS", "the game must accept Python's CRC")
        game.open_slot(1)
        self.assertIn("DRAWN 1/95", game.text(16))
        lo, hi = game.bg_tile(h)
        self.assertEqual(lo, SANS[h])
        self.assertEqual(hi, SANS[h])
        game.power_off(save=False)

    def test_damaged_save_is_flagged_not_wiped(self):
        slot = tdpsave.Slot(name="FONT 1")
        slot.glyphs[A] = bytes(SANS[A])
        data = bytearray(tdpsave.build([slot]))
        data[16 + 266 + A * 8 + 3] ^= 0x01          # flip a pixel without fixing the CRC
        self.ram.write_bytes(bytes(data))
        game = Game(self.rom)
        self.assertEqual(game.text(4), "   SAVE CHECK!")
        self.assertEqual(game.text(7), "   POCKET SANS")   # the other slots are fine
        game.open_slot(1)
        game.shot("m1-map-damaged")
        self.assertEqual(game.text(2), "SAVE CHECK!")
        game.power_off(save=True)
        self.assertEqual(self.ram.read_bytes(), bytes(data), "a damaged save must be left untouched")
        with self.assertRaises(SystemExit):
            sav2font.main([str(self.ram), "--out", str(self.tmp / "x")])
        self.assertEqual(sav2font.main([str(self.ram), "--out", str(self.tmp / "x"), "--force"]), 0)

    # ---- editor controls ----

    def test_select_switches_glyph_and_toggles_guide(self):
        game = Game(self.rom)
        game.open_slot(1)
        game.open_editor()
        game.combo("select", "right")
        self.assertTrue(game.text(0).startswith("B #66"))
        # Pocket Sans 'B' row 1 is .####... so pixel (1,1) is a guide cell
        self.assertEqual(game.canvas_cell(1, 1), 4)
        game.press("select")                             # a tap on its own toggles the guide
        self.assertEqual(game.canvas_cell(1, 1), CELL_OFF)
        game.press("select")
        self.assertEqual(game.canvas_cell(1, 1), 4)
        game.hold("select")
        game.press("left")
        game.press("left")
        game.release("select")
        self.assertTrue(game.text(0).startswith("@ #64"))
        game.power_off(save=False)


if __name__ == "__main__":
    unittest.main()
