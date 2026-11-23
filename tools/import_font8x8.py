#!/usr/bin/env python3
"""Convert Daniel Hepper's font8x8_basic.h (public domain) into this project's text font format.

  import_font8x8.py font8x8_basic.h fonts/classic-8x8.txt

Source: https://github.com/dhepper/font8x8 (README and header: "License: Public Domain").
font8x8 stores the leftmost pixel in bit 0; the Game Boy (and our format) use bit 7.
"""
import re
import sys
from pathlib import Path

HEADER = """\
# Classic 8x8: a starter template for Type Designer Pocket.
# Converted by tools/import_font8x8.py from font8x8_basic.h by Daniel Hepper
# (https://github.com/dhepper/font8x8), released into the public domain. font8x8 is in turn based on
# Marcel Sondaar's public-domain font and IBM's public-domain VGA fonts.
#
# Capitals use rows 0-6 (one taller than Pocket Sans); the baseline sits under row 6, descenders on row 7.
# Format: ": <char>" (or ": space") then 8 rows of 8, '#' = ink, '.' = blank. Codes 32-126 in order.
"""


def main():
    src, out = sys.argv[1:3]
    rows = re.findall(r"\{\s*((?:0x[0-9A-Fa-f]{2}\s*,\s*){7}0x[0-9A-Fa-f]{2})\s*\}", Path(src).read_text())
    if len(rows) != 128:
        raise SystemExit(f"expected 128 glyphs in {src}, found {len(rows)}")
    lines = [HEADER]
    for code in range(32, 127):
        lines.append(f": {'space' if code == 32 else chr(code)}")
        for b in (int(v, 16) for v in rows[code].split(",")):
            lines.append("".join("#" if (b >> x) & 1 else "." for x in range(8)))
    Path(out).write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
