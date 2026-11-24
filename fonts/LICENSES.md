# Fonts

| Font | File | Source | Licence |
| --- | --- | --- | --- |
| Pocket Sans | `pocket-sans.txt` | Drawn for Type Designer Pocket | [SIL Open Font License 1.1](OFL.txt), no Reserved Font Name |
| Pocket Serif | `pocket-serif.txt` | Drawn for Type Designer Pocket; punctuation and symbols shared with Pocket Sans | [SIL Open Font License 1.1](OFL.txt), no Reserved Font Name |
| Classic 8×8 | `classic-8x8.txt` | `font8x8_basic.h` (32–126) and `font8x8_ext_latin.h` (160–255) from [dhepper/font8x8](https://github.com/dhepper/font8x8) by Daniel Hepper, based on Marcel Sondaar's font and IBM's VGA fonts. Converted with `tools/import_font8x8.py` | Public domain (as stated in font8x8's README and headers) |
| Classic 8×8, codes 128–159 | `classic-8x8.txt` | font8x8 has no arrows or card suits, so these are Pocket Sans's symbols with each stroke doubled sideways | [SIL Open Font License 1.1](OFL.txt), as a modified version of Pocket Sans |

The code in this repository is MIT (see [`../LICENSE`](../LICENSE)). The OFL fonts can be used, embedded, modified and redistributed by anyone, including in commercial games. They can't be sold on their own, and modified versions stay under the OFL.

**Rules for new templates:** only public-domain, CC0, OFL or MIT fonts, recorded here before they're added, and never fonts taken from commercial games.

**A note on lineage:** Classic 8×8 traces back to IBM's VGA font. Its public-domain status rests on the statements of its authors (Daniel Hepper, and Marcel Sondaar before him), which is how it is widely used.

**Considered and left out:** unscii-8 by viznut. It's also declared public domain, but the same project ships recreations of the Commodore, Atari and Amiga system fonts, so it was left out to keep the set simple.
