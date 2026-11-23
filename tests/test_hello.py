"""M0 smoke test: the ROM boots, draws a runtime glyph, and keeps a battery save across power cycles."""
import shutil
import struct
import tempfile
import unittest
from pathlib import Path

from pyboy import PyBoy

ROOT = Path(__file__).resolve().parents[1]
ROM = ROOT / "build" / "type-designer-pocket.gb"
SHOTS = ROOT / "build" / "screens"

GLYPH_TILE = 200
GLYPH_A = [0x00, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00]


def boot(rom_path, frames=150):
    """Power on, run for `frames` (PyBoy's own boot logo takes ~80), return (pyboy, screenshot). Caller stops it."""
    gb = PyBoy(str(rom_path), window="null", sound_emulated=False)
    gb.tick(frames, True)
    return gb, gb.screen.image.copy()


def screen_text(gb):
    """Read the 20x18 visible background as text. GBDK's console font puts ASCII c at tile c - 32."""
    rows = []
    for y in range(18):
        row = "".join(chr(t + 32) if 0 <= t < 96 else "#" for t in (gb.memory[0x9800 + y * 32 + x] for x in range(20)))
        rows.append(row.rstrip())
    return rows


class HelloRom(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.rom = self.tmp / ROM.name
        shutil.copy(ROM, self.rom)
        SHOTS.mkdir(parents=True, exist_ok=True)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def test_header(self):
        d = ROM.read_bytes()
        self.assertEqual(d[0x147], 0x1B, "cart type should be MBC5+RAM+BATTERY")
        self.assertEqual(d[0x149], 0x02, "save RAM should be 8 KB")

    def test_runtime_glyph(self):
        gb, shot = boot(self.rom)
        shot.save(SHOTS / "hello.png")
        # tile 200 sits at 0x8C80 in either addressing mode; 1bpp in default colours = both bitplanes equal
        vram = [gb.memory[0x8C80 + i] for i in range(16)]
        self.assertEqual(vram[0::2], GLYPH_A)
        self.assertEqual(vram[1::2], GLYPH_A)
        self.assertEqual(gb.memory[0x9800 + 8 * 32 + 17], GLYPH_TILE)
        gb.stop(save=False)

    def test_battery_save_survives_power_cycle(self):
        ram = Path(str(self.rom) + ".ram")
        for expected in (1, 2, 3):
            gb, shot = boot(self.rom)
            self.assertIn(f"boots saved: {expected}", "\n".join(screen_text(gb)))
            gb.stop(save=True)
            data = ram.read_bytes()
            self.assertEqual(len(data), 8 * 1024)
            self.assertEqual(data[0:4], b"TDP0")
            (boots,) = struct.unpack_from("<H", data, 5)
            self.assertEqual(boots, expected)
        shot.save(SHOTS / "hello-third-boot.png")


if __name__ == "__main__":
    unittest.main()
