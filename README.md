# Type Designer Pocket

A Game Boy game for designing pixel fonts, one 8×8 glyph at a time. Your fonts live in the cartridge's battery save, and a companion tool turns that save into a GB Studio font or an `.otf`.

Working title. Sister project to the web [Type Making Game](https://github.com/AHeathNE/type-making-game).

## Status

**M0: toolchain.** The hello ROM boots, builds a glyph tile from 1bpp data at runtime, and keeps a boot counter in battery save RAM. Headless tests cover all three.

## Build

Needs `make`, `curl` and `tar`. The first build downloads the pinned GBDK-2020 release into `vendor/gbdk` and checks its SHA-256.

```bash
make
```

The ROM lands in `build/type-designer-pocket.gb`. Open it in any Game Boy emulator, for example mGBA (`sudo apt install mgba-qt`). The emulator writes the save next to the ROM as a `.sav` file.

## Test

```bash
make test
```

This creates `.venv`, installs [PyBoy](https://github.com/Baekalfen/PyBoy), and runs `tests/` headless. The tests boot the ROM, inspect video memory, and power cycle it to confirm the save survives. Screenshots go to `build/screens/`.

## Cartridge

| Setting | Value |
| --- | --- |
| MBC | MBC5 + RAM + battery (type `0x1B`) |
| Save RAM | 8 KB, one bank, at `0xA000` |
| Target | Original Game Boy (DMG) first |

### Save RAM rule

Read and write save RAM only by copying whole structs with `memcpy` into and out of a work copy in normal RAM. SDCC (GBDK 4.5.0 and 4.6.0) can compile `SAVE->field++` followed by a read of `SAVE->field` through a fixed pointer into a read of the wrong address. The M0 test caught this as a boot counter that showed 0 on screen while the save held the right value.

## Project layout

| Path | What it is |
| --- | --- |
| `src/` | Game source (C, GBDK-2020) |
| `tests/` | Headless PyBoy tests |
| `tools/get-gbdk.sh` | Downloads the pinned GBDK-2020 toolchain |
| `vendor/` | Downloaded toolchain (not committed) |
| `build/` | ROM and test screenshots (not committed) |

## Roadmap

| Stage | Contents |
| --- | --- |
| **M0** | Toolchain, hello ROM, test harness |
| **M1** | Character map and editor for the 95 ASCII glyphs, one save slot with a checksum, `.sav` to GB Studio font |
| **M2** | Templates shown as a ghost under your pixels, three slots, copy/paste/flip/nudge/undo, credits |
| **M3** | Type test screen with variable width |
| **M4** | Web import page (GB Studio zip and `.otf`), then playing in the browser |
| **M5** | Challenge mode, characters 128–255 |
| **M6** | Sound, title art, Game Boy Color palettes, real hardware |

## Licences

GBDK-2020's runtime library is GPLv2 with a linking exception, so ROMs built with it can use any licence. Starter fonts are added in M2, each with its licence recorded in `fonts/LICENSES.md`.
