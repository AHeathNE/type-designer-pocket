"""Shared helpers: a Type Designer Pocket cartridge running headless in PyBoy, driven by buttons."""
import os
import sys
from pathlib import Path

from pyboy import PyBoy

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import font2c  # noqa: E402

ROM = ROOT / "build" / "type-designer-pocket.gb"
NOI = ROOT / "build" / "type-designer-pocket.noi"   # symbols, from the -Wl-j link flag
SHOTS = ROOT / "build" / "screens"
FONTS = {name: font2c.parse(ROOT / "fonts" / f"{name}.txt") for name in ("pocket-sans", "pocket-serif", "classic-8x8")}
SANS = FONTS["pocket-sans"]

TILE_UI = 128          # UI text: code c at 128 + c - 32
TILE_CANVAS = 224      # editor cells: state * 4 + corner (0 off, 1 guide, 2 ink)
CELL_OFF, CELL_GUIDE, CELL_INK = 0, 4, 8   # TILE_CANVAS offsets of each state's top-left corner


def bg_tile_addr(n):
    """VRAM address of BG tile n in the 0x8800 addressing mode GBDK uses."""
    return 0x9000 + n * 16 if n < 128 else 0x8800 + (n - 128) * 16


def _symbols():
    syms = {}
    for line in NOI.read_text().splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[0] == "DEF":
            syms[parts[1]] = int(parts[2], 16)
    return syms


SYMBOLS = _symbols()
# GBDK's vsync() is where the game waits when it has nothing to do: from _vsync to the next symbol
_VSYNC = SYMBOLS["_vsync"]
_VSYNC_END = min(a for a in SYMBOLS.values() if a > _VSYNC)


def glyph_index(ch):
    return ord(ch) - 32


class Game:
    """A powered-on cartridge, past the title screen."""

    def __init__(self, rom, title=False, cgb=None, sound=False):
        """Boots past the title screen to the fonts screen (title=True: stop at the title screen).

        cgb: True for a Game Boy Color, False for the original. By default the ROM's header decides (it's
        Color-compatible, so a Color), unless TDP_DMG=1 is set to run the whole suite on the original.
        """
        if cgb is None and os.environ.get("TDP_DMG") == "1":
            cgb = False
        # sound=True emulates the APU, so its registers can be read back (slower; off by default)
        self.gb = PyBoy(str(rom), window="null", sound_emulated=sound, cgb=cgb)
        self.wait_title()
        if not title:
            self.press("start")
            self.wait_fonts()

    # ---- time and buttons ----

    def tick(self, n=1):
        self.gb.tick(n, True)

    def wait_for(self, cond, what, limit=600):
        for _ in range(limit):
            self.tick()
            if self.gb.memory[0xFF40] & 0x80 and cond():
                self.tick(2)
                return
        raise AssertionError(f"timed out waiting for {what}; screen:\n" + "\n".join(self.screen()))

    def idle(self):
        """True when the CPU is waiting in vsync(): the game has finished this frame's work."""
        return _VSYNC <= self.gb.register_file.PC < _VSYNC_END

    def settle(self, quiet=3, limit=900):
        """Wait until the game has caught up with every button press: idle for `quiet` frames in a row."""
        still = 0
        for _ in range(limit):
            self.tick()
            still = still + 1 if self.idle() else 0
            if still >= quiet:
                return
        raise AssertionError("the game never went idle")

    def press(self, button, hold=3, after=5):
        self.gb.button_press(button)
        self.tick(hold)
        self.gb.button_release(button)
        self.tick(after)

    def hold(self, button):
        self.gb.button_press(button)
        self.tick(3)

    def release(self, button):
        self.gb.button_release(button)
        self.tick(3)

    def combo(self, held, button):
        self.hold(held)
        self.press(button)
        self.release(held)

    # ---- screens ----

    def wait_title(self):
        self.wait_for(lambda: self.text(12) == "    PRESS START", "the title screen")

    def wait_fonts(self):
        self.wait_for(lambda: self.text(13) == " A:OPEN", "the fonts screen")

    def wait_map(self):
        self.wait_for(lambda: self.text(17).startswith("A:EDIT"), "the character map")

    def wait_editor(self):
        self.wait_for(lambda: self.tile(19, 1) < 95 and self.text(0)[2:3] == "#", "the editor")

    def wait_typetest(self):
        self.wait_for(lambda: self.text(0).startswith("TYPE TEST"), "the type test")

    def wait_keyboard(self):
        self.wait_for(lambda: self.text(0) == "A:ADD B:DEL ST:DONE", "the keyboard")

    def oam(self, sprite):
        """(OAM x, OAM y) of a sprite."""
        a = 0xFE00 + sprite * 4
        return self.gb.memory[a + 1], self.gb.memory[a]

    def wait_picker(self):
        self.wait_for(lambda: self.text(0).startswith("NEW FONT IN SLOT"), "the new-font screen")

    def open_slot(self, n=1):
        """From the fonts screen, open slot n (1-3)."""
        for _ in range(3):
            self.press("up")
        for _ in range(n - 1):
            self.press("down")
        self.press("a")
        self.wait_map()

    def open_editor(self):
        self.press("a")
        self.wait_editor()

    def back_to_map(self):
        """From the editor: START opens the menu, DONE is the last item."""
        self.press("start")
        self.press("up")      # wraps to DONE
        self.press("a")
        self.wait_map()

    def map_menu(self, item):
        """From the character map, START's menu: 0 TYPE TEST, 1 CHALLENGE, 2 FONTS."""
        self.press("start")
        for _ in range(item):
            self.press("down")
        self.press("a")

    def menu(self, item):
        """Run an editor menu item by its position:
        0 UNDO, 1 COPY, 2 PASTE, 3 FLIP H, 4 FLIP V, 5 NUDGE, 6 WIDTH, 7 CLEAR, 8 FROM, 9 G:, 10 DONE."""
        self.press("start")
        for _ in range(item):
            self.press("down")
        self.press("a")

    # ---- reading the screen ----

    def tile(self, x, y):
        return self.gb.memory[0x9800 + y * 32 + x]

    def win_tile(self, x, y):
        return self.gb.memory[0x9C00 + y * 32 + x]

    def text(self, y):
        return self.screen()[y]

    def screen(self):
        """Visible background as text: UI tiles as their characters, font tiles as a block."""
        rows = []
        for y in range(18):
            row = ""
            for x in range(20):
                t = self.tile(x, y)
                row += chr(t - TILE_UI + 32) if TILE_UI <= t < TILE_UI + 95 else ("█" if t < 95 else "░")
            rows.append(row.rstrip())
        return rows

    def win_text(self, y, width=9):
        return "".join(chr(t - TILE_UI + 32) if TILE_UI <= (t := self.win_tile(x, y)) < TILE_UI + 95 else "|"
                       for x in range(width)).rstrip()

    def bg_tile(self, n):
        a = bg_tile_addr(n)
        data = [self.gb.memory[a + i] for i in range(16)]
        return data[0::2], data[1::2]   # (low plane, high plane) per row

    def canvas_cell(self, x, y):
        """Editor canvas state of pixel (x, y): CELL_OFF, CELL_GUIDE or CELL_INK."""
        return self.tile(2 + 2 * x, 1 + 2 * y) - TILE_CANVAS

    def shot(self, name):
        SHOTS.mkdir(parents=True, exist_ok=True)
        self.gb.screen.image.save(SHOTS / f"{name}.png")

    def power_off(self, save=True):
        self.gb.stop(save=save)


def rows_to_bytes(rows):
    return [int("".join(map(str, r)), 2) for r in rows]
