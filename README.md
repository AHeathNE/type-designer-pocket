# Type Designer Pocket

A Game Boy game for designing pixel fonts, one 8×8 glyph at a time. Your fonts live in the cartridge's battery save, and a companion tool turns that save into a GB Studio font.

Working title. Sister project to the web [Type Making Game](https://github.com/AHeathNE/type-making-game).

## Status

**M2: templates, three fonts, editor tools.** You can keep three fonts on the cartridge and start each one from a starter template. The template can be a light-grey guide to trace over, or a copy to remix. The editor has undo, copy/paste, flips, nudge and clear.

## Playing

**Fonts screen** (at power-on). This lists the three save slots, each with its name, guide font and how many glyphs are drawn.

| Control | Does |
| --- | --- |
| Up/Down, A | Pick a font and open it |
| START | New font in that slot |
| SELECT | Credits |

**New font.**
- Pick the **guide** with Left/Right: Pocket Sans, Pocket Serif, Classic 8×8 or no guide.
- Pick how to **start**:
  - **EMPTY** shows the guide in light grey to trace over.
  - **COPY IT** fills the font with the template, ready to remix.
- The preview below shows all 95 characters. If the slot already has drawings, the game asks before erasing them.

**Character map.** Every character in a 16-wide grid, in the same order as a GB Studio font sheet. Characters you haven't drawn show the guide in light grey.

| Control | Does |
| --- | --- |
| D-pad | Pick a character |
| A | Edit it |
| B | Back to the fonts screen |

**Editor.**

| Control | Does |
| --- | --- |
| D-pad | Move the cursor (hold to repeat) |
| A | Paint. Toggles the pixel; hold A and move to keep painting |
| B | Erase (hold and move to keep erasing) |
| SELECT + ←/→ | Previous/next character |
| SELECT (tap) | Guide on/off |
| SELECT + B | Undo |
| START | Menu |

The START menu has:

| Item | Does |
| --- | --- |
| UNDO | 16 steps per character; a whole stroke is one step |
| COPY | Copy the current glyph |
| PASTE | Paste onto any character |
| FLIP H, FLIP V | Mirror the glyph across or up and down |
| NUDGE | Move the whole glyph with the d-pad; A when done |
| CLEAR | Empty the glyph |
| G: | Cycle the guide font for this font |
| DONE | Save and go back to the map |

The editor shows your glyph at actual size in the top-right corner. It also shows the width GB Studio will give it: the ink plus one blank column on each side. The ticks on the left mark the cap line, the x-height and the baseline. They use GB Studio's own metrics: capitals on rows 1–6, x-height on rows 2–6, descenders on row 7.

If a save fails its checksum, the fonts screen and the map show **SAVE CHECK!**. The game never wipes a save it recognises.

## Starter templates

| Template | Style | Source |
| --- | --- | --- |
| Pocket Sans | 5×6 capitals, GB Studio metrics | Drawn for this project (OFL) |
| Pocket Serif | Slab serifs, GB Studio metrics | Drawn for this project (OFL) |
| Classic 8×8 | Heavy, full-cell, 7-row capitals | [font8x8](https://github.com/dhepper/font8x8) by Daniel Hepper, public domain |

Each template is a text file in `fonts/` that you can edit. It's converted to C at build time, and `fonts/LICENSES.md` records where each one came from.

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
- drive the game with button presses (draw, erase, switch glyphs, toggle the guide, create fonts from templates, use every editor menu item)
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
| `src/main.c` | Main loop and screen switching |
| `src/slots.c`, `src/picker.c`, `src/credits.c` | Fonts screen, new-font screen, credits |
| `src/map.c`, `src/editor.c` | Character map, editor and its menu |
| `src/save.c` | Battery save: format, slots, checksums, write |
| `src/gfx.c` | Tiles, text, templates, glyph rendering, width rule |
| `src/input.c` | Joypad read in VBlank (taps are never lost), with d-pad auto-repeat |
| `fonts/*.txt` | Starter templates as editable text art; Pocket Sans is also the UI font |
| `tools/font2c.py` | Turns the text fonts into C at build time |
| `tools/import_font8x8.py` | Rebuilds `fonts/classic-8x8.txt` from font8x8's header |
| `tools/tdpsave.py` | Save format (read and write) |
| `tools/sav2font.py` | Save → GB Studio font |
| `tools/get-gbdk.sh` | Downloads the pinned GBDK-2020 toolchain |
| `tests/` | Headless PyBoy tests |

## Roadmap

| Stage | Contents |
| --- | --- |
| **M0** ✓ | Toolchain, hello ROM, test harness |
| **M1** ✓ | Character map and editor for the 95 ASCII glyphs, one save slot with a checksum, `.sav` to GB Studio font |
| **M2** ✓ | Templates shown as a ghost under your pixels, three slots, copy/paste/flip/nudge/undo, credits |
| **M3** | Type test screen with variable width |
| **M4** | Web import page (GB Studio zip and `.otf`), then playing in the browser |
| **M5** | Challenge mode, characters 128–255 |
| **M6** | Sound, title art, Game Boy Color palettes, real hardware |

## Licences

- **Code:** [MIT](LICENSE).
- **Fonts:** Pocket Sans and Pocket Serif are under the [SIL Open Font License 1.1](fonts/OFL.txt). Classic 8×8 is public domain. See [`fonts/LICENSES.md`](fonts/LICENSES.md).
- **Toolchain:** GBDK-2020's runtime library is GPLv2 with a linking exception, so ROMs built with it can use any licence.
