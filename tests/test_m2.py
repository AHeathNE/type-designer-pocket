"""M2: fonts screen with three slots, new fonts from templates, editor menu tools, credits."""
import shutil
import tempfile
import unittest
from pathlib import Path

from game import CELL_GUIDE, CELL_INK, CELL_OFF, FONTS, ROM, SANS, Game, glyph_index
import tdpsave  # noqa: E402

A = glyph_index("A")
SERIF = FONTS["pocket-serif"]
CLASSIC = FONTS["classic-8x8"]


class M2(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.rom = self.tmp / ROM.name
        self.ram = Path(str(self.rom) + ".ram")
        shutil.copy(ROM, self.rom)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def saved(self):
        return tdpsave.parse(self.ram.read_bytes())

    def glyph(self, game):
        """The glyph being edited, read back from the editor's canvas tiles."""
        return [int("".join("1" if game.canvas_cell(x, y) == CELL_INK else "0" for x in range(8)), 2) for y in range(8)]

    # ---- fonts screen and credits ----

    def test_fonts_screen_lists_three_slots(self):
        game = Game(self.rom)
        game.shot("m2-fonts")
        self.assertEqual(game.text(0), "TYPE DESIGNER POCKET")
        self.assertEqual(game.text(3), "> FONT 1      0/95")
        self.assertEqual(game.text(6), "  FONT 2      0/95")
        self.assertEqual(game.text(9), "  FONT 3      0/95")
        self.assertEqual(game.text(10), "   POCKET SANS")
        game.press("down")
        self.assertEqual(game.text(6)[0], ">")
        game.power_off(save=False)

    def test_credits(self):
        game = Game(self.rom)
        game.press("select")
        game.wait_for(lambda: game.text(0) == "CREDITS", "the credits")
        game.shot("m2-credits")
        lines = "\n".join(game.screen())
        for needle in ("AHeathNE", "(MIT)", "(OFL 1.1)", "font8x8 by Daniel", "Public domain", "GBDK-2020"):
            self.assertIn(needle, lines)
        game.press("b")
        game.wait_fonts()
        game.power_off(save=False)

    def test_slots_are_independent(self):
        game = Game(self.rom)
        game.open_slot(2)
        self.assertTrue(game.text(1).startswith("FONT 2"))
        game.open_editor()
        game.press("a")                     # one pixel in slot 2's 'A'
        game.back_to_map()
        game.press("b")
        game.wait_fonts()
        self.assertEqual(game.text(3), "  FONT 1      0/95")
        self.assertEqual(game.text(6), "> FONT 2      1/95")
        game.power_off()
        save = self.saved()
        self.assertEqual(save.active, 1)
        self.assertFalse(save.slots[0].has_ink(A))
        self.assertTrue(save.slots[1].has_ink(A))
        self.assertTrue(all(s.crc_ok for s in save.slots))
        self.assertTrue(save.header_ok)

    # ---- new font from a template ----

    def test_new_font_copies_serif_template(self):
        game = Game(self.rom)
        game.press("down")                  # slot 2
        game.press("start")
        game.wait_picker()
        self.assertEqual(game.text(0), "NEW FONT IN SLOT 2")
        self.assertEqual(game.text(2), "> GUIDE POCKET SANS")
        self.assertEqual(game.text(3), "  START EMPTY")
        game.press("right")
        self.assertEqual(game.text(2), "> GUIDE POCKET SERIF")
        lo, hi = game.bg_tile(A)            # preview: serif guide in light grey
        self.assertEqual((lo, hi), (SERIF[A], [0] * 8))
        game.press("down")
        game.press("right")
        self.assertEqual(game.text(3), "> START COPY IT")
        lo, hi = game.bg_tile(A)            # preview: copied, so black
        self.assertEqual((lo, hi), (SERIF[A], SERIF[A]))
        game.shot("m2-picker")
        game.press("a")
        game.wait_map()
        self.assertEqual(game.text(1), "FONT 2        SERIF")
        self.assertEqual(game.text(16), "DRAWN 94/95 SEL:PAGE")   # every glyph but the space, clear of the hint
        game.power_off()
        save = self.saved()
        slot = save.slots[1]
        self.assertEqual(save.active, 1)
        self.assertEqual(slot.template_id, 1)
        self.assertEqual([list(g) for g in slot.glyphs], SERIF)   # all 224, accents and symbols too
        self.assertTrue(slot.crc_ok)

    def test_new_font_guide_only_and_no_guide(self):
        game = Game(self.rom)
        game.press("start")
        game.wait_picker()
        for _ in range(2):
            game.press("right")             # classic 8x8, empty
        self.assertEqual(game.text(2), "> GUIDE CLASSIC 8X8")
        game.press("a")
        game.wait_map()
        self.assertEqual(game.text(1), "FONT 1        8X8")
        lo, hi = game.bg_tile(A)
        self.assertEqual((lo, hi), (CLASSIC[A], [0] * 8))
        game.open_editor()
        self.assertEqual(game.canvas_cell(2, 0), CELL_GUIDE)   # classic 'A' row 0 is ..##....
        game.back_to_map()
        game.press("b")
        game.wait_fonts()
        game.press("start")
        game.wait_picker()
        game.press("left")                  # sans -> (wraps back) no guide
        self.assertEqual(game.text(2), "> GUIDE NO GUIDE")
        game.press("down")
        game.press("right")                 # can't copy nothing
        self.assertEqual(game.text(3), "> START EMPTY")
        game.press("a")
        game.wait_map()
        self.assertEqual(game.text(1), "FONT 1        -")
        self.assertEqual(game.bg_tile(A), ([0] * 8, [0] * 8))
        game.power_off()
        self.assertEqual(self.saved().slots[0].template_id, 0xFF)

    def test_new_font_asks_before_erasing(self):
        slot = tdpsave.Slot(name="FONT 1")
        slot.glyphs[A] = bytes(SANS[A])
        slot.glyphs[A + 1] = bytes(SANS[A + 1])
        self.ram.write_bytes(tdpsave.build([slot]))
        game = Game(self.rom)
        game.press("start")
        game.wait_picker()
        game.press("a")
        self.assertEqual(game.text(17), "ERASE 2? A:YES B:NO")
        game.press("b")                     # changed my mind
        self.assertEqual(game.text(17), "A:CREATE  B:CANCEL")
        game.press("b")
        game.wait_fonts()
        self.assertEqual(game.text(3), "> FONT 1      2/95")
        game.press("start")
        game.wait_picker()
        game.press("a")
        game.press("a")                     # yes
        game.wait_map()
        self.assertIn("DRAWN 0/95", game.text(16))
        game.power_off()
        self.assertFalse(any(self.saved().slots[0].has_ink(i) for i in range(95)))

    # ---- editor tools ----

    def test_menu_tools(self):
        game = Game(self.rom)
        game.open_slot(1)
        game.open_editor()
        game.press("a")                         # ink at (1,1)
        game.press("start")
        game.shot("m2-menu")
        self.assertEqual(game.win_text(1), "|>UNDO")
        self.assertEqual(game.win_text(9), "| G:SANS")
        self.assertEqual(game.win_text(11), "|B:CLOSE")
        game.press("b")                         # close without doing anything

        game.menu(1)                            # COPY
        self.assertEqual(game.text(17), "COPIED")
        game.menu(4)                            # FLIP V: (1,1) -> (1,6)
        self.assertEqual(self.glyph(game), [0, 0, 0, 0, 0, 0, 0x40, 0])
        game.menu(3)                            # FLIP H: (1,6) -> (6,6)
        self.assertEqual(self.glyph(game), [0, 0, 0, 0, 0, 0, 0x02, 0])
        game.menu(5)                            # NUDGE
        self.assertEqual(game.text(17), "NUDGE: D-PAD  A:OK")
        game.press("left")
        game.press("up")
        game.press("a")
        self.assertEqual(self.glyph(game), [0, 0, 0, 0, 0, 0x04, 0, 0])
        game.menu(0)                            # UNDO the nudge (one step for the whole nudge)
        self.assertEqual(self.glyph(game), [0, 0, 0, 0, 0, 0, 0x02, 0])
        game.menu(7)                            # CLEAR
        self.assertEqual(self.glyph(game), [0] * 8)
        game.menu(2)                            # PASTE the copied (1,1)
        self.assertEqual(self.glyph(game), [0, 0x40, 0, 0, 0, 0, 0, 0])

        game.combo("select", "right")           # paste works across glyphs: B gets it too
        game.menu(2)
        self.assertEqual(self.glyph(game), [0, 0x40, 0, 0, 0, 0, 0, 0])
        game.back_to_map()
        self.assertIn("DRAWN 2/95", game.text(16))
        game.power_off()
        slot = self.saved().slots[0]
        self.assertEqual(list(slot.glyphs[A]), [0, 0x40, 0, 0, 0, 0, 0, 0])
        self.assertEqual(list(slot.glyphs[A + 1]), [0, 0x40, 0, 0, 0, 0, 0, 0])

    def test_undo_shortcut_and_history(self):
        game = Game(self.rom)
        game.open_slot(1)
        game.open_editor()
        game.press("a")                         # stroke 1: (1,1)
        game.press("right")
        game.hold("a")                          # stroke 2: (2,1) (3,1)
        game.press("right")
        game.release("a")
        self.assertEqual(self.glyph(game)[1], 0x70)
        game.combo("select", "b")               # undo stroke 2 in one step
        self.assertEqual(self.glyph(game)[1], 0x40)
        self.assertEqual(game.text(17), "UNDONE")
        game.combo("select", "b")               # undo stroke 1
        self.assertEqual(self.glyph(game)[1], 0x00)
        game.combo("select", "b")
        self.assertEqual(game.text(17), "NOTHING TO UNDO")
        game.press("b")                         # erasing white changes nothing, so no undo step
        game.combo("select", "b")
        self.assertEqual(game.text(17), "NOTHING TO UNDO")
        game.power_off(save=False)

    def test_menu_guide_cycles_and_saves(self):
        game = Game(self.rom)
        game.open_slot(1)
        game.open_editor()
        self.assertEqual(game.canvas_cell(1, 2), CELL_GUIDE)   # sans 'A' row 2: .#...#..
        game.press("start")
        for _ in range(8):
            game.press("down")
        game.press("a")                                       # G:SANS -> G:SERIF, menu stays open
        self.assertEqual(game.win_text(9), "|>G:SERIF")
        game.press("a")
        self.assertEqual(game.win_text(9), "|>G:8X8")
        game.press("a")
        self.assertEqual(game.win_text(9), "|>G:OFF")
        game.press("b")
        self.assertEqual(game.canvas_cell(1, 2), CELL_OFF)
        game.menu(8)                                          # back to SANS
        game.press("b")
        self.assertEqual(game.canvas_cell(1, 2), CELL_GUIDE)
        game.menu(8)                                          # SERIF, and keep it
        game.press("b")
        game.back_to_map()
        self.assertEqual(game.text(1), "FONT 1        SERIF")
        game.power_off()
        self.assertEqual(self.saved().slots[0].template_id, 1)


if __name__ == "__main__":
    unittest.main()
