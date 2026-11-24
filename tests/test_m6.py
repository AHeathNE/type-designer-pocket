"""M6: design orders, START FROM in the editor, and the LEARN pages."""
import re
import shutil
import tempfile
import unittest
from pathlib import Path

from game import ROM, ROOT, SANS, TILE_CANVAS, Game, glyph_index
import tdpsave  # noqa: E402

TILE_MARK_INK, TILE_LINE = 250, 251
CELL_INK_TILE, CELL_GUIDE_TILE = TILE_CANVAS + 8, TILE_CANVAS + 4


def c_orders():
    """The 'first' strings of each design order, from src/order.c."""
    src = (ROOT / "src" / "order.c").read_text()
    block = src[src.index("order_first[ORDER_COUNT] = {"):]
    block = re.sub(r"//[^\n]*", "", block[: block.index("};")])
    return ["".join(re.findall(r'"([^"]*)"', item)) for item in block.split(",")[:-1]]


def c_pages():
    """(title, text) of each LEARN page, from src/learn.c."""
    src = (ROOT / "src" / "learn.c").read_text()
    return re.findall(r'\{"([^"]+)",.*?\n\s+"((?:[^"\\]|\\.)*)"\}', src, re.S)


def flip_h(g):
    return [int(f"{b:08b}"[::-1], 2) for b in g]


def turn(g):
    return flip_h(g)[::-1]


class M6(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.rom = self.tmp / ROM.name
        self.ram = Path(str(self.rom) + ".ram")
        shutil.copy(ROM, self.rom)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def preview(self, game):
        """The editor's actual-size preview (BG tile 0): the glyph, without the guide."""
        return game.bg_tile(0)[1]

    # ---- design orders ----

    def test_order_lists(self):
        az, control, hamburg, frequent = c_orders()
        self.assertEqual(az, "")
        letters = sorted("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
        self.assertEqual(sorted(control), letters)
        self.assertEqual(control[:2] + control[26:28], "noHO")
        self.assertEqual(sorted(frequent), letters)
        self.assertEqual(hamburg, "hamburgefonstivHAMBURGEFONSTIV")
        self.assertEqual(len(set(hamburg)), 30)

    def test_order_drives_next_editor_and_challenge(self):
        game = Game(self.rom)
        game.open_slot(1)
        game.settle()
        self.assertEqual(game.text(2)[14:], "NEXT:!")                # A-Z: the first undrawn after the space
        game.press("start")
        for _ in range(3):
            game.press("down")
        game.press("a")                                          # ORDER: A-Z -> CONTROL, the menu stays open
        self.assertEqual(game.win_text(4, 16), "|>ORDER CONTROL")
        game.press("b")
        game.settle()
        self.assertEqual(game.text(2)[14:], "NEXT:n")
        game.shot("m6-map-next")
        game.map_menu(2)                                         # NEXT GLYPH
        game.settle()
        self.assertEqual(game.text(15)[6:10], "#110")

        game.open_editor()
        game.combo("select", "right")
        self.assertEqual(game.text(0)[2:6], "#111")              # n -> o
        game.combo("select", "right")
        self.assertEqual(game.text(0)[2:6], "#104")              # o -> h (straights)
        game.combo("select", "left")
        game.combo("select", "left")
        game.combo("select", "left")                             # before n: the end of the order
        self.assertEqual(game.text(0)[2:6], "#255")
        game.back_to_map()

        game.map_menu(1)                                         # CHALLENGE follows the order too
        game.wait_for(lambda: game.text(0) == "CHALLENGE", "the challenge setup")
        self.assertEqual(game.text(15), "ORDER: CONTROL")
        game.press("start")
        game.wait_for(lambda: game.text(17).startswith("1/68 "), "the first challenge glyph", limit=900)
        # a-z isn't ticked by default, so the control order's first capital, H, comes first (not A)
        self.assertEqual(game.bg_tile(game.tile(0, 0))[0], SANS[glyph_index("H")])
        game.press("start")
        game.press("down")
        game.press("down")
        game.press("a")                                          # QUIT
        game.wait_for(lambda: game.text(0) == "CHALLENGE DONE!", "the results")
        game.press("b")
        game.wait_map()
        game.power_off()
        self.assertEqual(tdpsave.parse(self.ram.read_bytes()).order, 1)   # kept in the save header

    # ---- START FROM ----

    def test_start_from(self):
        slot = tdpsave.Slot(name="FONT 1", template_id=0)
        for ch in "nod":
            slot.glyphs[glyph_index(ch)] = bytes(SANS[glyph_index(ch)])
        self.ram.write_bytes(tdpsave.build([slot]))
        n, d = SANS[glyph_index("n")], SANS[glyph_index("d")]

        game = Game(self.rom)
        game.open_slot(1)
        game.press("down")
        game.press("down")                                       # A -> a
        for _ in range(7):
            game.press("right")                                  # -> h
        game.open_editor()
        self.assertEqual(game.text(0)[2:6], "#104")
        game.press("start")
        for _ in range(8):
            game.press("down")
        self.assertEqual(game.win_text(9)[:7], "|>FROM ")
        self.assertEqual(game.bg_tile(game.win_tile(7, 9))[0], n)   # the source, shown in the menu
        game.press("a")
        self.assertEqual(self.preview(game), n)                  # h starts as a copy of n
        self.assertEqual(game.text(17), "STARTED FROM IT")
        game.combo("select", "b")                                # one undo step takes it back
        self.assertEqual(self.preview(game), [0] * 8)
        game.menu(8)

        for _ in range(13):
            game.combo("select", "right")                        # h -> u
        game.menu(8)
        self.assertEqual(self.preview(game), turn(n))            # u: n turned upside down
        for _ in range(19):
            game.combo("select", "left")                         # u -> b
        game.menu(8)
        self.assertEqual(self.preview(game), flip_h(d))          # b: d mirrored
        for _ in range(9):
            game.combo("select", "right")                        # b -> k, from l, which isn't drawn
        game.menu(8)
        self.assertEqual(game.text(17), "SOURCE NOT DRAWN")
        for _ in range(15):
            game.combo("select", "right")                        # k -> z, which has no source
        game.menu(8)
        self.assertEqual(game.text(17), "NO SOURCE FOR THIS")
        game.back_to_map()
        game.power_off()

        slot = tdpsave.parse(self.ram.read_bytes()).slots[0]
        self.assertEqual(list(slot.glyphs[glyph_index("h")]), n)
        self.assertEqual(list(slot.glyphs[glyph_index("u")]), turn(n))
        self.assertEqual(list(slot.glyphs[glyph_index("b")]), flip_h(d))

    def test_start_from_base_letter(self):
        slot = tdpsave.Slot(name="FONT 1", template_id=0)
        slot.glyphs[glyph_index("a")] = bytes(SANS[glyph_index("a")])
        self.ram.write_bytes(tdpsave.build([slot]))
        game = Game(self.rom)
        game.open_slot(1)
        game.press("select")
        game.press("select")                                     # á
        game.press("left")                                       # à
        game.open_editor()
        self.assertEqual(game.text(0)[2:6], "#224")
        game.menu(8)
        self.assertEqual(self.preview(game), SANS[glyph_index("a")])
        game.shot("m6-from-accent")
        game.back_to_map()
        game.power_off(save=False)

    # ---- LEARN ----

    def test_learn_pages(self):
        pages = c_pages()
        self.assertEqual(len(pages), 20)
        game = Game(self.rom)
        self.assertEqual(game.text(15), " SELECT:LEARN")
        self.assertEqual(game.text(16), " B:CREDITS")
        game.press("select")
        game.wait_for(lambda: game.text(0).startswith("BASELINE"), "the first LEARN page")
        self.assertEqual(game.text(0)[15:], "1/20")
        self.assertEqual(game.tile(3, 3), CELL_INK_TILE)         # H, row 1, column 1: ink
        self.assertEqual(game.tile(2, 9), TILE_LINE)             # the baseline, under row 6
        self.assertEqual(game.text(17), "<>:PAGE  B:BACK")
        game.shot("m6-learn-baseline")
        for k, (title, text) in enumerate(pages):
            game.settle()
            self.assertEqual(game.text(0)[: len(title)], title)
            self.assertEqual(game.text(0)[15:].strip(), f"{k + 1}/20")
            shown = [game.text(y) for y in range(11, 17)]
            self.assertTrue(all(len(line) <= 20 for line in shown))
            self.assertEqual(" ".join(line for line in shown if line), text, f"page {title}: all of the text shows")
            if title == "ASCENDERS":
                self.assertEqual(game.tile(3, 3), TILE_MARK_INK)    # h's ascender, pointed out
                game.shot("m6-learn-ascenders")
            if title == "BOWL, COUNTER":
                self.assertEqual(game.tile(12, 5), CELL_GUIDE_TILE)  # o's counter, shaded
                game.shot("m6-learn-bowl")
            game.press("right")
        game.settle()
        self.assertEqual(game.text(0)[15:], "1/20")              # wrapped around
        game.press("left")
        game.settle()
        self.assertEqual(game.text(0)[15:], "20/20")
        game.press("b")
        game.wait_fonts()

        game.open_slot(1)                                        # also from the map, and back to it
        game.map_menu(4)
        game.wait_for(lambda: game.text(17) == "<>:PAGE  B:BACK", "LEARN from the map")
        game.press("b")
        game.wait_map()
        game.power_off(save=False)


if __name__ == "__main__":
    unittest.main()
