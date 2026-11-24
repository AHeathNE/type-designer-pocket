#!/usr/bin/env python3
"""Build Classic 8x8 from Daniel Hepper's font8x8 (public domain), plus bold copies of Pocket Sans's symbols.

  import_font8x8.py font8x8_basic.h font8x8_ext_latin.h fonts/pocket-sans.txt fonts/classic-8x8.txt

Source: https://github.com/dhepper/font8x8 (README and headers: "License: Public Domain").
font8x8 stores the leftmost pixel in bit 0; the Game Boy (and our format) use bit 7.
  32-126   font8x8_basic.h
  128-159  font8x8 has no arrows or card suits, so these are Pocket Sans's symbols with every stroke
           doubled sideways to match the VGA weight (quotes, ellipsis, ™ and Œ/œ stay thin, as doubling
           fills them in). Being made from Pocket Sans, they are OFL 1.1.
  160-255  font8x8_ext_latin.h (Latin-1)
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from font2c import label, parse  # noqa: E402

THIN = "‘’“”…™Œœ"

HEADER = """\
# Classic 8x8: a starter template for Type Designer Pocket.
# Converted by tools/import_font8x8.py from font8x8_basic.h and font8x8_ext_latin.h by Daniel Hepper
# (https://github.com/dhepper/font8x8), released into the public domain. font8x8 is in turn based on
# Marcel Sondaar's public-domain font and IBM's public-domain VGA fonts.
# Codes 128-159 are the exception: they are bold copies of Pocket Sans's symbols, so they are under the
# SIL Open Font License 1.1 like Pocket Sans (fonts/OFL.txt).
#
# Capitals use rows 0-6 (one taller than Pocket Sans); the baseline sits under row 6, descenders on row 7.
# Format: ": <char>" then 8 rows of 8, '#' = ink, '.' = blank. Codes 32-126, then 128-255, in order
# ("space", "nbsp" and "shy" name the invisible ones; 128-159 are the symbols listed in tools/tdpsave.py).
"""


def read_h(path, count):
    rows = re.findall(r"\{\s*((?:0x[0-9A-Fa-f]{2}\s*,\s*){7}0x[0-9A-Fa-f]{2})\s*\}", Path(path).read_text())
    if len(rows) != count:
        raise SystemExit(f"expected {count} glyphs in {path}, found {len(rows)}")
    # bit 0 = left in font8x8: reverse each byte so bit 7 = left
    return [[int(f"{int(v, 16):08b}"[::-1], 2) for v in r.split(",")] for r in rows]


def main():
    basic_h, latin_h, sans_txt, out = sys.argv[1:5]
    basic, latin, sans = read_h(basic_h, 128), read_h(latin_h, 96), parse(sans_txt)
    lines = [HEADER]
    for code in list(range(32, 127)) + list(range(128, 256)):
        if code < 127:
            g = basic[code]
        elif code < 160:
            g = sans[code - 32]
            if label(code) not in THIN:
                g = [(b | (b >> 1)) & 0xFF for b in g]
        else:
            g = latin[code - 160]
        lines.append(f": {label(code)}")
        lines += [format(b, "08b").replace("0", ".").replace("1", "#") for b in g]
    Path(out).write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
