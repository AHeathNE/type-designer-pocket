"""M5: characters 128-255 (symbols and Latin-1) everywhere, and the timed challenge mode."""
import json
import shutil
import tempfile
import unittest
from pathlib import Path

from game import FONTS, ROM, SANS, TILE_CANVAS, Game, glyph_index
from test_m3 import LINES, layout, render, vram_line
import sav2font  # noqa: E402
import tdpsave  # noqa: E402

TILE_BAR = 241
SYMBOL = 128 - 32   # glyph index of the first symbol (←)


def idx(code):
    return code - 32


class M5(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.rom = self.tmp / ROM.name
        self.ram = Path(str(self.rom) + ".ram")
        shutil.copy(ROM, self.rom)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def saved(self):
        return tdpsave.parse(self.ram.read_bytes())

    def sans_slot(self, blank=""):
        slot = tdpsave.Slot(name="FONT 1", template_id=0)
        for i, g in enumerate(SANS):
            slot.glyphs[i] = bytes(g)
        for ch in blank:
            slot.glyphs[ord(ch) - 32] = bytes(8)
        return slot

    # ---- the character set ----

    def test_templates_cover_codes_128_to_255(self):
        for name, glyphs in FONTS.items():
            self.assertEqual(len(glyphs), 224)
            self.assertEqual(glyphs[idx(127)], [0] * 8, name)
            for code in range(33, 256):
                if code in (127, 160) or (code == 173 and name == "classic-8x8"):
                    continue   # unused, no-break space, and font8x8's invisible soft hyphen
                self.assertTrue(any(glyphs[idx(code)]), f"{name}: code {code} {tdpsave.char_of(code)!r} is blank")
        # accented letters are built on the font's own letters: é carries Pocket Sans's e
        self.assertEqual(SANS[idx(0xE9)][2:7], SANS[glyph_index("e")][2:7])

    def test_case_fallback_covers_latin1(self):
        slot = tdpsave.Slot()
        slot.glyphs[idx(0xE9)] = bytes(SANS[idx(0xE9)])         # é drawn, É not
        rows, _ = sav2font.tile_for(slot, 0xC9)
        self.assertEqual(rows, slot.rows(idx(0xE9)))
        self.assertIsNone(sav2font.tile_for(slot, 0xD7)[0])      # × isn't a letter: no fallback
        self.assertEqual(tdpsave.other_case(0xFF), None)          # ÿ's capital isn't in Latin-1

    # ---- the map: three pages ----

    def test_map_pages_and_moving_between_them(self):
        game = Game(self.rom)                                   # fresh cartridge: blank, Pocket Sans guide
        game.open_slot(1)
        game.settle()
        self.assertEqual(game.text(0), "BASIC 32-126     1/3")
        self.assertEqual(game.text(16), "DRAWN 0/95  SEL:PAGE")
        self.assertEqual(game.text(17), "A:EDIT  START:MENU")
        game.press("select")
        game.settle()
        self.assertEqual(game.text(0), "SYMBOLS 128-191  2/3")
        self.assertEqual(game.text(16), "DRAWN 0/64  SEL:PAGE")
        self.assertEqual(game.bg_tile(0), (SANS[SYMBOL], [0] * 8))   # cell 0: the ← guide, light grey
        self.assertEqual(game.text(15)[6:10], "#161")              # kept the column: A (33) -> ¡ (161)
        ref = game.tile(2, 15)
        self.assertEqual(game.bg_tile(ref), (SANS[idx(161)], SANS[idx(161)]))   # reference in Pocket Sans
        game.shot("m5-map-symbols")
        game.press("select")
        game.settle()
        self.assertEqual(game.text(0), "ACCENTS 192-255  3/3")
        self.assertEqual(game.text(15)[6:10], "#225")              # á
        game.shot("m5-map-accents")
        game.press("select")
        game.settle()
        self.assertEqual(game.text(0), "BASIC 32-126     1/3")

        for _ in range(3):
            game.press("down")                                  # A (row 2) -> row 5, the last on page 1
        game.press("down")                                      # off the bottom: page 2, same column
        game.settle()
        self.assertEqual(game.text(0)[:7], "SYMBOLS")
        self.assertEqual(game.text(15)[6:10], "#129")
        game.press("up")                                        # off the top: back to page 1's last row
        game.settle()
        self.assertEqual(game.text(15)[6:10], "#113")
        for _ in range(5):
            game.press("up")
        game.press("left")
        game.settle()
        self.assertEqual(game.text(15)[6:10], "#32 ")
        game.press("left")                                      # before the space: the very last glyph
        game.settle()
        self.assertEqual(game.text(0)[:7], "ACCENTS")
        self.assertEqual(game.text(15)[6:10], "#255")
        game.press("right")
        game.settle()
        self.assertEqual(game.text(15)[6:10], "#32 ")
        game.power_off(save=False)

    def test_draw_an_accented_glyph_and_export_it(self):
        game = Game(self.rom)
        game.open_slot(1)
        game.press("select")
        game.press("select")                                    # á
        game.open_editor()
        self.assertEqual(game.text(0)[2:6], "#225")
        self.assertEqual(game.bg_tile(game.tile(0, 0))[0], SANS[idx(0xE1)])   # reference char
        game.press("a")                                         # one pixel
        game.back_to_map()
        game.settle()
        self.assertEqual(game.text(16)[:10], "DRAWN 1/64")

        game.press("select")                                    # page 1: A
        game.press("select")                                    # page 2: ¡, then up to →
        game.press("up")
        game.press("up")
        game.press("left")                                      # ←, code 128
        game.open_editor()
        self.assertEqual(game.text(0)[2:6], "#128")
        game.combo("select", "left")                            # 127 is skipped
        self.assertEqual(game.text(0)[2:6], "#126")
        game.combo("select", "right")
        self.assertEqual(game.text(0)[2:6], "#128")
        game.back_to_map()
        game.press("b")
        game.wait_fonts()
        self.assertEqual(game.text(4)[16:], "+1")                # one glyph drawn beyond ASCII
        game.power_off()

        save = self.saved()
        slot = save.slots[0]
        drawn = [i for i in range(tdpsave.GLYPHS) if slot.has_ink(i)]
        self.assertEqual(drawn, [idx(0xE1)])
        self.assertEqual(sav2font.tile_for(slot, 0xC1)[0], slot.rows(idx(0xE1)))   # Á uses á
        out = self.tmp / "out"
        sav2font.main([str(self.ram), "--out", str(out), "--name", "Accents"])
        from PIL import Image
        img = Image.open(out / "accents.png").convert("RGB")
        cell = idx(0xE1)
        tx, ty = (cell % 16) * 8, (cell // 16) * 8
        self.assertIn(sav2font.GB_INK, [img.getpixel((tx + x, ty + y)) for x in range(8) for y in range(8)])

    def test_keyboard_reaches_every_page(self):
        slot = self.sans_slot()
        self.ram.write_bytes(tdpsave.build([slot]))
        game = Game(self.rom)
        game.open_slot(1)
        game.map_menu(0)
        game.wait_typetest()
        game.press("a")
        game.wait_keyboard()
        self.assertEqual(game.text(17)[6:9], "1/3")
        for _ in range(3):
            game.press("down")                                  # A (row 2) -> the last row
        game.press("down")                                      # next page, column 1: →
        game.settle()
        self.assertEqual(game.text(17)[6:9], "2/3")
        self.assertEqual(game.bg_tile(game.tile(2, 6))[0], SANS[SYMBOL])   # the keys are drawn in Pocket Sans
        game.press("a")
        for _ in range(4):
            game.press("down")                                  # past the last row of page 2 (4 rows)
        game.settle()
        self.assertEqual(game.text(17)[6:9], "3/3")
        game.press("a")                                         # Á
        game.shot("m5-keyboard-accents")
        game.press("start")
        game.wait_typetest()
        game.settle()
        text = "\x81\xc1"
        expect = layout(slot, text, LINES)
        self.assertEqual(vram_line(game, 0), render(slot, expect[0]))
        game.power_off(save=False)

    # ---- challenge ----

    def open_challenge(self, game):
        game.open_slot(1)
        game.map_menu(1)
        game.wait_for(lambda: game.text(0) == "CHALLENGE", "the challenge setup")
        game.settle()

    def wait_glyph(self, game, n, total):
        game.wait_for(lambda: game.text(17).startswith(f"{n}/{total} "), f"challenge glyph {n}/{total}", limit=900)

    def bar(self, game):
        """Filled pixels of the timer bar (16 tiles on row 0)."""
        return sum(game.tile(2 + t, 0) - TILE_BAR for t in range(16))

    def test_challenge_setup(self):
        self.ram.write_bytes(tdpsave.build([self.sans_slot(blank="BC\xe9")]))
        game = Game(self.rom)
        self.open_challenge(game)
        self.assertEqual(game.text(2), "> TIME       < 7S  >")
        self.assertEqual(game.text(4), "   [X] A-Z")
        self.assertEqual(game.text(5), "   [ ] a-z")
        self.assertEqual(game.text(11), "   [X] ONLY UNDRAWN")
        self.assertEqual(game.text(13), "2 GLYPHS")                # B and C
        game.press("down")
        game.press("a")                                         # untick A-Z
        self.assertEqual(game.text(13), "NOTHING TO DRAW")
        game.press("start")                                     # nothing to draw: stays here
        game.settle()
        self.assertEqual(game.text(0), "CHALLENGE")
        game.press("a")
        for _ in range(5):
            game.press("down")
        game.press("a")                                         # tick ACCENTS: é
        self.assertEqual(game.text(9), ">  [X] ACCENTS")
        self.assertEqual(game.text(13), "3 GLYPHS")
        game.press("down")
        game.press("a")                                         # untick ONLY UNDRAWN: every letter, digit, mark
        self.assertEqual(game.text(13), "132 GLYPHS")            # 26 + 10 + 32 punctuation + 64 accents
        game.press("a")
        game.press("down")                                      # wraps to TIME
        for _ in range(3):
            game.press("right")
        self.assertEqual(game.text(2), "> TIME       < 20S >")
        game.shot("m5-challenge-setup")
        game.press("b")
        game.wait_map()
        game.power_off(save=False)

    def test_challenge_round_trip(self):
        self.ram.write_bytes(tdpsave.build([self.sans_slot(blank="BC\xe9")]))
        game = Game(self.rom)
        self.open_challenge(game)
        for _ in range(6):
            game.press("down")
        game.press("a")                                         # + ACCENTS: B, C, é
        for _ in range(6):
            game.press("up")                                    # TIME
        game.press("left")
        game.press("left")                                      # 3 seconds
        self.assertEqual(game.text(2), "> TIME       < 3S  >")
        game.press("start")
        game.wait_for(lambda: game.text(0) == "GET READY!", "the countdown")
        self.assertEqual(game.text(14), "3 GLYPHS, 3S EACH")
        self.assertIn(TILE_CANVAS + 8, [game.tile(x, y) for x in range(20) for y in range(4, 13)])   # a big 3
        game.shot("m5-countdown")

        # glyph 1, B: a full bar, draw a pixel, the bar runs down, then the next glyph comes up
        self.wait_glyph(game, 1, 3)
        self.assertEqual(game.text(17), "1/3         ST:PAUSE")
        self.assertEqual(game.bg_tile(game.tile(0, 0))[0], SANS[glyph_index("B")])
        self.assertGreaterEqual(self.bar(game), 124)
        self.assertEqual(game.canvas_cell(1, 1), 0)              # the guide is off in a challenge
        game.press("a")
        game.tick(90)
        self.assertTrue(40 <= self.bar(game) <= 88, self.bar(game))
        game.shot("m5-challenge-play")
        b_drawn = list(game.gb.memory[0x9000 + i * 2] for i in range(8))   # preview tile 0, low plane
        self.wait_glyph(game, 2, 3)

        # glyph 2, C: leave it blank
        self.wait_glyph(game, 3, 3)

        # glyph 3, é: the pause stops the clock
        game.press("start")
        self.assertEqual(game.win_text(0), "||PAUSED|")
        game.tick(300)
        self.assertEqual(game.text(17)[:4], "3/3 ")
        game.press("a")                                         # RESUME
        game.press("right")
        game.press("a")                                         # a pixel next to the cursor
        game.press("start")
        game.press("down")
        game.press("a")                                         # NEXT: finished
        game.wait_for(lambda: game.text(0) == "CHALLENGE DONE!", "the results")
        self.assertEqual(game.text(1), "DRAWN 2/3")
        self.assertEqual(game.text(16), "A:TRY AGAIN")
        game.shot("m5-results")

        # try C again and draw it
        game.press("right")
        game.press("a")
        self.wait_glyph(game, 1, 1)
        game.press("a")
        game.press("start")
        game.press("down")
        game.press("a")
        game.wait_for(lambda: game.text(0) == "CHALLENGE DONE!", "the results")
        self.assertEqual(game.text(1), "DRAWN 3/3")

        # try B again but draw nothing: the first drawing comes back
        game.press("left")
        game.press("a")
        self.wait_glyph(game, 1, 1)
        self.assertEqual(game.canvas_cell(1, 1), 0)              # starts blank
        game.wait_for(lambda: game.text(0) == "CHALLENGE DONE!", "the results after the time runs out")
        self.assertEqual(game.text(1), "DRAWN 3/3")
        game.press("b")
        game.wait_map()
        game.power_off()

        slot = self.saved().slots[0]
        for code in (ord("B"), ord("C"), 0xE9):
            self.assertTrue(slot.has_ink(idx(code)), chr(code))
        self.assertEqual(list(slot.glyphs[idx(ord("B"))]), b_drawn)
        self.assertEqual(bytes(slot.glyphs[idx(ord("D"))]), bytes(SANS[idx(ord("D"))]))   # untouched

    def test_challenge_quit(self):
        game = Game(self.rom)                                   # blank font: 68 glyphs by default
        self.open_challenge(game)
        self.assertEqual(game.text(13), "68 GLYPHS")
        game.press("start")
        self.wait_glyph(game, 1, 68)
        self.assertEqual(game.text(0)[2:18], "░" * 16)          # the timer bar fills row 0
        game.press("start")
        game.press("down")
        game.press("down")
        game.press("a")                                         # QUIT
        game.wait_for(lambda: game.text(0) == "CHALLENGE DONE!", "the results")
        self.assertEqual(game.text(1), "DRAWN 0/1")
        game.press("b")
        game.wait_map()
        game.power_off(save=False)


if __name__ == "__main__":
    unittest.main()
