"""M7: ROM banks, title screen and settings, sound, Game Boy Color themes."""
import shutil
import tempfile
import unittest
from pathlib import Path

from game import ROM, Game
import tdpsave  # noqa: E402

NR22, NR52 = 0xFF17, 0xFF26
GREEN_PAPER = (224, 248, 208)


def close(a, b, tol=12):
    return all(abs(x - y) <= tol for x, y in zip(a, b))


class M7(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.rom = self.tmp / ROM.name
        self.ram = Path(str(self.rom) + ".ram")
        shutil.copy(ROM, self.rom)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def test_cartridge_header(self):
        data = ROM.read_bytes()
        self.assertEqual(len(data), 64 * 1024)
        self.assertEqual(data[0x143], 0x80)      # works on the original and the Game Boy Color
        self.assertEqual(data[0x147], 0x1B)      # MBC5 + RAM + battery
        self.assertEqual(data[0x148], 0x01)      # 64 KB: four banks
        self.assertEqual(data[0x149], 0x02)      # 8 KB save RAM
        self.assertEqual(sum(data[0x134:0x14D]) + 0x19 + data[0x14D] & 0xFF, 0, "header checksum")

    def test_title_sound_and_colours(self):
        game = Game(self.rom, title=True, cgb=True, sound=True)
        self.assertEqual(game.text(10), "TYPE DESIGNER POCKET")
        self.assertEqual(game.text(14), "> SOUND   ON")
        self.assertEqual(game.text(15), "  COLORS  GREY")
        self.assertTrue(game.gb.memory[NR52] & 0x80, "sound is on")
        game.shot("m7-title")
        game.press("a")                                          # sound off
        self.assertEqual(game.text(14), "> SOUND   OFF")
        self.assertFalse(game.gb.memory[NR52] & 0x80)
        game.press("down")
        game.press("right")                                      # GREY -> GREEN
        game.settle()
        self.assertEqual(game.text(15), "> COLORS  GREEN")
        self.assertTrue(close(game.gb.screen.image.convert("RGB").getpixel((1, 1)), GREEN_PAPER))
        game.shot("m7-title-green")
        game.press("start")
        game.wait_fonts()
        game.power_off()
        save = tdpsave.parse(self.ram.read_bytes())
        self.assertEqual((save.mute, save.theme), (1, 1))        # both kept in the save header

        game = Game(self.rom, title=True, cgb=True, sound=True)              # and they come back at power-on
        self.assertEqual(game.text(14), "> SOUND   OFF")
        self.assertEqual(game.text(15), "  COLORS  GREEN")
        self.assertFalse(game.gb.memory[NR52] & 0x80)
        game.power_off(save=False)

    def test_original_game_boy_has_no_colour_setting(self):
        game = Game(self.rom, title=True, cgb=False)
        self.assertEqual(game.text(14), "> SOUND   ON")
        self.assertEqual(game.text(15), "")
        game.press("down")                                       # nothing below SOUND
        game.press("a")
        self.assertEqual(game.text(14), "> SOUND   OFF")
        game.power_off(save=False)

    def test_sound_effects_play(self):
        game = Game(self.rom, sound=True)
        game.open_slot(1)
        game.press("right")                                      # a cursor move: a short blip on channel 2
        self.assertEqual(game.gb.memory[NR22], 0x51)
        game.open_editor()
        game.press("a")                                          # painting a pixel
        self.assertEqual(game.gb.memory[NR22], 0x31)
        game.power_off(save=False)

    def test_rename_a_font(self):
        game = Game(self.rom)
        game.open_slot(2)
        game.map_menu(4)                                         # RENAME
        game.wait_for(lambda: game.text(0) == "A:ADD B:DEL ST:SAVE", "the rename keyboard")
        self.assertEqual(game.text(2), "NAME: FONT 2_")
        self.assertEqual(game.text(17)[:4], "6/12")
        for _ in range(6):
            game.press("b")                                      # clear it
        self.assertEqual(game.text(2), "NAME: _")
        game.press("start")                                      # an empty name isn't allowed
        game.settle()
        self.assertEqual(game.text(0), "A:ADD B:DEL ST:SAVE")
        for _ in range(3):
            game.press("down")                                   # the keyboard stays on page 1: no page turn
        self.assertEqual(game.text(17)[6:9].strip(), "")         # and shows no page number
        for _ in range(3):
            game.press("up")                                     # back to A
        game.press("a")                                          # A
        game.press("right")
        game.press("a")                                          # B
        game.press("a")                                          # B
        game.press("left")
        game.press("a")                                          # A
        game.press("select")                                     # a space
        self.assertEqual(game.text(2), "NAME: ABBA _")
        game.press("start")
        game.wait_map()
        self.assertEqual(game.text(1)[:12].rstrip(), "ABBA")
        game.press("b")
        game.wait_fonts()
        self.assertEqual(game.text(6)[2:14].rstrip(), "ABBA")       # the fonts screen, slot 2
        game.power_off()
        self.assertEqual(tdpsave.parse(self.ram.read_bytes()).slots[1].name, "ABBA")   # the space is trimmed


if __name__ == "__main__":
    unittest.main()
