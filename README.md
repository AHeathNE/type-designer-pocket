# Type Designer Pocket

A Game Boy game for designing pixel fonts, one 8×8 glyph at a time. Your fonts live in the cartridge's battery save. The website plays the game in your browser and turns that save into a GB Studio font or an `.otf`.

Working title.

## Status

**M6: design orders and LEARN.** Draw in the order type designers use (n o H O first, a test word, or the most frequent letters), start a glyph from a related one (h from n, b from a mirrored d, é from e), and learn the parts of a letter and where fonts come from in 20 illustrated pages.

**M5: challenge mode and the full character set.** A timed challenge, like the web Type Making Game: pick the seconds per glyph and which characters, then draw each one before its timer runs out. Fonts now cover all of GB Studio's codes 32–255: ASCII, symbols (arrows, card suits, hearts, stars, curly quotes and more) and Latin-1 accents, and all three templates come with them.

**M4: the website.** Play in the browser, and export your fonts as a GB Studio zip or an `.otf`, straight from the running game or from any `.sav`.

**M3: type test and widths.** The type test sets sample text, or your own, in your font exactly as GB Studio will, with variable width and word wrap. Each glyph can have a fixed width, and the space's width sets the word spacing.

Earlier milestones: three fonts on the cartridge, each started from a template (a guide to trace over, or a copy to remix), and an editor with undo, copy/paste, flips, nudge and clear.

## Playing

**Fonts screen** (at power-on). This lists the three save slots, each with its name, guide font and how many glyphs are drawn.

| Control | Does |
| --- | --- |
| Up/Down, A | Pick a font and open it |
| START | New font in that slot |
| SELECT | LEARN |
| B | Credits |

**New font.**
- Pick the **guide** with Left/Right: Pocket Sans, Pocket Serif, Classic 8×8 or no guide.
- Pick how to **start**:
  - **EMPTY** shows the guide in light grey to trace over.
  - **COPY IT** fills the font with the template, ready to remix.
- The preview below shows the 95 ASCII characters. **COPY IT** copies all 223, symbols and accents too. If the slot already has drawings, the game asks before erasing them.

**Character map.** The character set in a 16-wide grid, in the same order as a GB Studio font sheet, one page at a time. Characters you haven't drawn show the guide in light grey.

| Page | Codes | Characters |
| --- | --- | --- |
| 1 BASIC | 32–126 | ASCII: letters, digits, punctuation |
| 2 SYMBOLS | 128–191 | `← → ↑ ↓ ▲ ▼ ◀ ▶ ♥ ♦ ♣ ♠ ★ ♪ ● ○ ‘ ’ “ ” • … – — € ™ Œ œ ✓ ✗ ■ □`, then Latin-1 punctuation (`¡ ¢ £ ¥ § © « ° ± ² ½ ¿`…) |
| 3 ACCENTS | 192–255 | Latin-1 letters: `À`–`ÿ`, plus `Æ Ð Ø Þ ß × ÷` |

| Control | Does |
| --- | --- |
| D-pad | Pick a character. Moving off the top or bottom row turns the page |
| SELECT | Next page |
| A | Edit it |
| START | Menu (below) |
| B | Back to the fonts screen |

Row 2 shows **NEXT:**, the next character you haven't drawn in the design order. The START menu has:

| Item | Does |
| --- | --- |
| TYPE TEST | See text set in your font |
| CHALLENGE | Draw against the clock (below) |
| NEXT GLYPH | Jump to the next undrawn character in the design order |
| ORDER | A cycles the design order. It's kept in the save and used by NEXT, the editor's SELECT + ←/→ and the challenge |
| LEARN | The LEARN pages, coming back here |
| FONTS | Back to the fonts screen |

| Order | Draws first | Why |
| --- | --- | --- |
| A-Z | Code order: punctuation, digits, A–Z, a–z, then symbols and accents | The GB Studio sheet's order |
| CONTROL | `n o`, then straights (`h m u r l i`), rounds (`c e d b p q`), diagonals (`v w x y k z`), the rest; then `H O`, `E F L T I`, `C G D Q`, `B P R U J`, `A V W X Y K M N Z S` | n and o set the stems, spacing and curves, and every family borrows from them |
| HAMBURG | `hamburgefonstiv`, then `HAMBURGEFONSTIV` | A classic type designer's test word that shows a font's style fast |
| FREQUENT | `e t a o i n s h r d l c u m w f g y p b v k j x q z`, then the capitals the same way | Real English words read soonest |

Everything an order doesn't list follows in code order.

**Type test.** Sets text in your font exactly as GB Studio will. It uses the same rules as the exporter:
- variable width
- ink moved to start at column 1
- letters drawn in one case used for both
- word wrap at 144 px

Left/Right cycles through seven samples: a pangram, an all-caps pangram, digits and punctuation, a shop-dialogue line, accented Latin-1, symbols, and **your own text**. The screen lists any characters the sample needs that you haven't drawn yet.

| Control | Does |
| --- | --- |
| Left/Right | Change sample |
| A | Write your own text on the keyboard screen (up to 40 characters): A adds, B deletes, SELECT adds a space, START shows it. The keyboard has the same three pages as the map; move off the top or bottom row to turn the page |
| B | Back to the map |

**Challenge.** From the map's START menu. The same idea as the web Type Making Game: a countdown, then one glyph after another, each with its own timer.

| Setting | Choices |
| --- | --- |
| TIME | 3, 5, 7, 10, 15, 20, 30 or 60 seconds per glyph (7 to start) |
| Characters | A–Z, a–z, 0–9, PUNCTUATION, SYMBOLS, ACCENTS (A–Z, 0–9 and punctuation to start) |
| ONLY UNDRAWN | On: only the characters this font is missing. Off: every character in the chosen sets |

- START begins the challenge with a 3-2-1-GO countdown. B cancels it.
- Each glyph starts blank, with the guide off (SELECT shows it). A bar along the top runs down.
- When the time runs out, the next glyph comes up. START pauses the clock: **RESUME**, **NEXT** (done with this one) or **QUIT**.
- If you leave a glyph empty, its old drawing (if it had one) comes back. The game saves after every glyph.
- The results screen shows every glyph from the round, with the ones you missed in light grey. **A** tries one again, timed. **B** goes back to the map.

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
| WIDTH | Set a fixed GB Studio width with Left/Right; B goes back to automatic. Dashed lines show where the glyph starts and where the next one begins. On the space, this sets the word spacing. A fixed width shows as `W5*` |
| CLEAR | Empty the glyph |
| FROM | Start from a related glyph, shown next to it: h m r from n, u from n turned over, b from d mirrored, accented letters from their base letter, `)` from `(`, and so on. One undo step takes it back. `-` means there's no source |
| G: | Cycle the guide font for this font |
| DONE | Save and go back to the map |

SELECT + ←/→ steps through all 223 characters (127 isn't used), in the design order chosen on the map. The editor shows your glyph at actual size in the top-right corner. It also shows the width GB Studio will give it: the ink plus one blank column on each side. The ticks on the left mark the cap line, the x-height and the baseline. They use GB Studio's own metrics: capitals on rows 1–6, x-height on rows 2–6, descenders on row 7.

If a save fails its checksum, the fonts screen and the map show **SAVE CHECK!**. The game never wipes a save it recognises.

**LEARN.** Twenty short pages, each with one or two letters drawn big, the part being explained in dark grey (or a space like a counter in light grey) and guide lines for the baseline, x-height and cap height. Left/Right turns the page; B goes back.

| Pages | Topics |
| --- | --- |
| 1–10 | The parts of a letter: baseline, cap height, x-height, ascenders and descenders, stems, bowls and counters, crossbars, apex and vertex, serifs, accents |
| 11–15 | Designing: spacing, control letters, letter families, test words, letter frequency |
| 16–20 | Where fonts come from: movable type, roman and sans serif type, bitmap fonts, Game Boy tiles, proportional and monospace |

## Starter templates

| Template | Style | Source |
| --- | --- | --- |
| Pocket Sans | 5×6 capitals, GB Studio metrics | Drawn for this project (OFL) |
| Pocket Serif | Slab serifs, GB Studio metrics | Drawn for this project (OFL) |
| Classic 8×8 | Heavy, full-cell, 7-row capitals | [font8x8](https://github.com/dhepper/font8x8) by Daniel Hepper, public domain (its symbols are bold copies of Pocket Sans's, OFL) |

Each template is a text file in `fonts/` that you can edit. It's converted to C at build time, and `fonts/LICENSES.md` records where each one came from. Every template covers all 223 characters. In Pocket Sans and Pocket Serif, the accented letters start from the font's own A–Z and a–z, with accents on rows 0–1; accented capitals drop one row to fit under them, as in IBM's VGA font.

**The character set.** Codes 32–126 are ASCII and 160–255 are Latin-1, which is what GB Studio uses for any character below 256, so those work in GB Studio dialogue as you type them. Codes 128–159 hold the symbols, and the exported `.json` maps each one (`"♥": 136`), so they work as typed too. Code 127 isn't used. The table is `EXTRA` in [`tools/tdpsave.py`](tools/tdpsave.py), mirrored in `web/js/save.js`.

## Website

`web/` is a static site with no build step. `make web` copies it into `build/web` together with a freshly built ROM; serve that folder with any static server, for example `python3 -m http.server --directory build/web`.

- **Play.** The ROM runs in [binjgb](https://github.com/binji/binjgb), vendored in `web/vendor/binjgb`. You can use the keyboard (arrows, X = A, Z = B, Enter = START, Shift = SELECT) or the on-screen buttons. The save is kept in the browser's local storage. **Download .sav** saves a copy, and **Load a .sav** restarts the game with one.
- **Your fonts.** Each slot shows its glyphs, a type-test box and a font name. The fonts come from the running game, refreshed whenever it saves, or from a `.sav` you drop in. Each slot exports as:
  - **GB Studio (.zip)**: the same PNG, JSON and how-to as `tools/sav2font.py`.
  - **.otf**: one pixel is 125 units on a 1000-unit em, with the baseline under row 6. Each character sits at its real Unicode code point, so the symbols at 128–159 come out as ← ♥ ★ and so on. Glyph placement and widths match the GB Studio export. Pixels are traced into merged outlines, and undrawn characters are left out so apps fall back to another font.
- **Checks.**
  - `tests/test_web.py` runs `web/js/save.js` and `otf.js` under Node and compares every glyph, width, outline area and line break with the Python tools.
  - In a browser, I checked that:
    - the zip's PNG matches `sav2font.py` byte for byte
    - the `.otf` passes the browser's font sanitizer
    - text set with the `.otf` matches the type test pixel for pixel
- **Publishing.** `.github/workflows/website.yml` runs the tests, builds the site and deploys it to GitHub Pages on every push to `main`. To enable it, go to Settings › Pages › Source and choose "GitHub Actions".

## Exporting with the command line

Emulators keep the cartridge save next to the ROM, as `type-designer-pocket.sav` (mGBA, SameBoy) or `.gb.ram` (PyBoy). A cartridge dumped with a flash cart or cart reader gives you the same bytes.

```bash
python3 tools/sav2font.py type-designer-pocket.sav --list
```

```bash
python3 tools/sav2font.py type-designer-pocket.sav --name "My Font" --out fonts-out/
```

This writes `my-font.png`, `my-font.json` (with the symbol mapping) and `HOW-TO-USE.txt`. Add `--zip` to get a single `my-font-gbstudio.zip` instead. Copy the PNG and JSON into your GB Studio project's `assets/fonts` folder. The PNG follows GB Studio's font format:
- a 128×112 sheet covering characters 32–255
- a GB light green background
- magenta columns marking each character's width

Letters you drew in only one case are used for both, accented ones too (`é` ↔ `É`). Characters you haven't drawn come out blank, 4 px wide. The script uses only the Python standard library.

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
- drive the game with button presses (draw, erase, switch glyphs, toggle the guide, create fonts from templates, use every editor menu item, type on the keyboard, turn the map's pages, play a whole challenge with pause, retry and quit)
- compare the type test's pixels, line by line, with the same text laid out by the exporter's code
- power-cycle the cartridge
- check the save the game writes against `tools/tdpsave.py`
- export it to a GB Studio sheet
- load saves written by Python, including a damaged one
- check that every template covers codes 32–255
- check every LEARN page shows all of its text within the screen

Screenshots go to `build/screens/`. Ubuntu needs `python3-venv` installed first.

## Cartridge and save format

| Setting | Value |
| --- | --- |
| MBC | MBC5 + RAM + battery (type `0x1B`) |
| Save RAM | 8 KB, one bank |
| ROM | 32 KB: bank 0 plus bank 1, which stays mapped because the game never switches ROM banks. The templates, challenge, type test, orders and LEARN live in bank 1 (`#pragma bank 1`) |
| Target | Original Game Boy (DMG) first |

The save holds a 16-byte header (which also keeps the design order) and three font slots of 2060 bytes each. Each slot has:
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
| `src/typetest.c` | Type test and keyboard |
| `src/challenge.c` | Challenge setup, countdown and results (the timer runs in the editor) |
| `src/order.c` | Design orders and the START FROM sources |
| `src/learn.c` | The LEARN pages: text and diagrams |
| `src/save.c` | Battery save: format, slots, checksums, write |
| `src/gfx.c` | Tiles, text, templates, glyph rendering, width rule, variable-width text and word wrap |
| `src/input.c` | Joypad read in VBlank: every press is counted and handed out one per frame, so taps are never lost or merged |
| `fonts/*.txt` | Starter templates as editable text art; Pocket Sans is also the UI font |
| `tools/font2c.py` | Turns the text fonts into C at build time |
| `tools/import_font8x8.py` | Rebuilds `fonts/classic-8x8.txt` from font8x8's headers and Pocket Sans's symbols |
| `tools/tdpsave.py` | Save format (read and write) |
| `tools/sav2font.py` | Save → GB Studio font |
| `tools/get-gbdk.sh` | Downloads the pinned GBDK-2020 toolchain |
| `tests/` | Headless PyBoy tests, plus a Node check of the website's JavaScript |
| `web/` | The website: player, font previews, GB Studio and `.otf` export |
| `web/vendor/` | binjgb (emulator, MIT) and opentype.js (MIT), with their licences |
| `.github/workflows/website.yml` | Test, build and publish the website to GitHub Pages |

## Roadmap

| Stage | Contents |
| --- | --- |
| **M0** ✓ | Toolchain, hello ROM, test harness |
| **M1** ✓ | Character map and editor for the 95 ASCII glyphs, one save slot with a checksum, `.sav` to GB Studio font |
| **M2** ✓ | Templates shown as a ghost under your pixels, three slots, copy/paste/flip/nudge/undo, credits |
| **M3** ✓ | Type test screen with variable width, per-glyph and space widths |
| **M4** ✓ | Website: play in the browser, export fonts from the live game or a `.sav` (GB Studio zip and `.otf`) |
| **M5** ✓ | Challenge mode, characters 128–255 (symbols and Latin-1) in the game, templates and exports |
| **M6** ✓ | Design orders, START FROM, LEARN pages |
| **M7** | Sound, title art, Game Boy Color palettes, real hardware. ROM bank 1 is nearly full, so this starts with real bank switching |

## Licences

- **Code:** [MIT](LICENSE).
- **Fonts:** Pocket Sans and Pocket Serif are under the [SIL Open Font License 1.1](fonts/OFL.txt). Classic 8×8 is public domain, apart from its symbols (codes 128–159), which are bold copies of Pocket Sans's and so OFL. See [`fonts/LICENSES.md`](fonts/LICENSES.md).
- **Toolchain:** GBDK-2020's runtime library is GPLv2 with a linking exception, so ROMs built with it can use any licence.
- **Website:** binjgb by Ben Smith and opentype.js by Frederik De Bleser, both MIT; their licences are in `web/vendor/`.
