# Type Designer Pocket

A Game Boy game for designing pixel fonts, one 8×8 glyph at a time. Your fonts live in the cartridge's battery save, and a companion tool turns that save into a GB Studio font.

Working title. Sister project to the web [Type Making Game](https://github.com/AHeathNE/type-making-game).

## Status

**M1: character map, editor, save, extractor.** You can draw all 95 ASCII characters on the Game Boy. Your font saves to the cartridge with a checksum, and `tools/sav2font.py` turns the save file into a GB Studio font.

## Playing

The game opens on the **character map**: every character in a 16-wide grid, in the same order as a GB Studio font sheet. Characters you haven't drawn yet show the Pocket Sans guide in light grey.

| Screen | Control | Does |
| --- | --- | --- |
| Map | D-pad | Pick a character |
| Map | A | Edit it |
| Editor | D-pad | Move the cursor (hold to repeat) |
| Editor | A | Paint. Toggles the pixel; hold A and move to keep painting |
| Editor | B | Erase (hold and move to keep erasing) |
| Editor | SELECT + ←/→ | Previous/next character |
| Editor | SELECT (tap) | Guide on/off |
| Editor | START | Save and go back to the map |

The editor shows your glyph at actual size in the top-right corner. It also shows the width GB Studio will give it: the ink plus one blank column on each side. The ticks on the left mark the cap line, the x-height and the baseline. They use GB Studio's own metrics: capitals on rows 1–6, x-height on rows 2–6, descenders on row 7.

If a save fails its checksum, the map shows **SAVE CHECK!**. The game never wipes a save it recognises.

## Exporting to GB Studio

Emulators keep the cartridge save next to the ROM, as `type-designer-pocket.sav` (mGBA, SameBoy) or `.gb.ram` (PyBoy). A cartridge dumped with a flash cart or cart reader gives you the same bytes.

```bash
python3 tools/sav2font.py type-designer-pocket.sav --list
```

```bash
python3 tools/sav2font.py type-designer-pocket.sav --name "My Font" --out fonts-out/
```

This writes `my-font.png`, `my-font.json` and `HOW-TO-USE.txt`. Add `--zip` to get a single `my-font-gbstudio.zip` instead. Copy the PNG and JSON into your GB Studio project's `assets/fonts` folder. The PNG follows GB Studio's font format:
- a 128×112 sheet covering characters 32–255
- a GB light green background
- magenta columns marking each character's width

Letters you drew in only one case are used for both. Characters you haven't drawn come out blank, 4 px wide. The script uses only the Python standard library.

## Build

Needs `make`, `curl`, `tar` and `python3`. The first build downloads the pinned GBDK-2020 release into `vendor/gbdk` and checks its SHA-256.

```bash
make
```

The ROM lands in `build/type-designer-pocket.gb`. Open it in any Game Boy emulator, for example mGBA (`sudo apt install mgba-qt`).

## Test

```bash
make test
```

This creates `.venv`, installs [PyBoy](https://github.com/Baekalfen/PyBoy), and runs `tests/` headless. The tests:
- drive the game with button presses (draw, erase, switch glyphs, toggle the guide)
- power-cycle the cartridge
- check the save the game writes against `tools/tdpsave.py`
- export it to a GB Studio sheet
- load saves written by Python, including a damaged one

Screenshots go to `build/screens/`. Ubuntu needs `python3-venv` installed first.

## Cartridge and save format

| Setting | Value |
| --- | --- |
| MBC | MBC5 + RAM + battery (type `0x1B`) |
| Save RAM | 8 KB, one bank |
| Target | Original Game Boy (DMG) first |

The save holds a 16-byte header and three font slots of 2060 bytes each. Each slot has:
- a name and the space width
- a width setting for each glyph
- 224 glyphs, 8 bytes each (one byte per row)
- a CRC-16

[`tools/tdpsave.py`](tools/tdpsave.py) documents every byte. [`src/save.h`](src/save.h) must stay in step with it.

## Toolchain notes

The project is pinned to **GBDK-2020 4.5.0**. 4.6.0 (SDCC 4.6.2) miscompiled code that indexes a pointer by `y * 2`: it dropped the `add hl, hl`, so tiles built pixel by pixel came out blank. `tests/test_m1.py` checks those tiles' pixels so a toolchain upgrade can't break them silently.

**Save RAM rule.** Read and write save RAM only by copying whole structs with `memcpy` into and out of normal RAM. SDCC (in 4.5.0 and 4.6.0) can compile `SAVE->field++` followed by a read of `SAVE->field` through a fixed pointer into a read of the wrong address.

## Project layout

| Path | What it is |
| --- | --- |
| `src/main.c` | Main loop: map ↔ editor |
| `src/map.c`, `src/editor.c` | The two screens |
| `src/save.c` | Battery save: format, load, checksum, write |
| `src/gfx.c` | Tiles, text, glyph rendering, width rule |
| `src/input.c` | Joypad with d-pad auto-repeat |
| `fonts/pocket-sans.txt` | Pocket Sans, the reference/UI font, as editable text art |
| `tools/font2c.py` | Turns a text font into C at build time |
| `tools/tdpsave.py` | Save format (read and write) |
| `tools/sav2font.py` | Save → GB Studio font |
| `tools/get-gbdk.sh` | Downloads the pinned GBDK-2020 toolchain |
| `tests/` | Headless PyBoy tests |

## Roadmap

| Stage | Contents |
| --- | --- |
| **M0** ✓ | Toolchain, hello ROM, test harness |
| **M1** ✓ | Character map and editor for the 95 ASCII glyphs, one save slot with a checksum, `.sav` to GB Studio font |
| **M2** | Templates shown as a ghost under your pixels, three slots, copy/paste/flip/nudge/undo, credits |
| **M3** | Type test screen with variable width |
| **M4** | Web import page (GB Studio zip and `.otf`), then playing in the browser |
| **M5** | Challenge mode, characters 128–255 |
| **M6** | Sound, title art, Game Boy Color palettes, real hardware |

## Licences

GBDK-2020's runtime library is GPLv2 with a linking exception, so ROMs built with it can use any licence. Pocket Sans was drawn for this project; see [`fonts/LICENSES.md`](fonts/LICENSES.md).
