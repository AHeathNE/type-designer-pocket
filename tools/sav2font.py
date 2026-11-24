#!/usr/bin/env python3
"""Turn a Type Designer Pocket save (.sav from an emulator, .ram from PyBoy, or a cartridge dump)
into a GB Studio font: NAME.png + NAME.json + HOW-TO-USE.txt.

  sav2font.py game.sav --list
  sav2font.py game.sav                      # active slot, into the current folder
  sav2font.py game.sav --slot 2 --name "My Font" --out fonts/ --zip

Standard library only.
"""
import argparse
import json
import re
import struct
import sys
import zipfile
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import tdpsave  # noqa: E402

# GB Studio's font loader reads the green channel for shades and treats pure white and magenta as
# transparent, so the background must be GB light green, not white.
GB_BG = (0xE0, 0xF8, 0xCF)
GB_INK = (0x07, 0x18, 0x21)
GB_TRIM = (0xFF, 0x00, 0xFF)
SHEET_W, SHEET_H = 128, 112


def tile_for(slot, code):
    """Glyph rows for a character code: its own drawing, else the other case of the same letter
    (A-Z and the Latin-1 letters: é falls back to É and the other way round)."""
    i = code - tdpsave.FIRST_CODE
    if slot.has_ink(i):
        return slot.rows(i), slot.widths[i]
    other = tdpsave.other_case(code)
    if other is not None and slot.has_ink(other - tdpsave.FIRST_CODE):
        o = other - tdpsave.FIRST_CODE
        return slot.rows(o), slot.widths[o]
    return None, slot.widths[i]


def sheet_pixels(slot):
    px = [[GB_BG] * SHEET_W for _ in range(SHEET_H)]
    for code in range(tdpsave.FIRST_CODE, tdpsave.FIRST_CODE + tdpsave.GLYPHS):
        i = code - tdpsave.FIRST_CODE
        tx, ty = (i % 16) * 8, (i // 16) * 8
        rows, override = tile_for(slot, code)
        ink, width = tdpsave.export_tile(rows or [[0] * 8] * 8, override, slot.space_width)
        for y in range(8):
            for x in range(8):
                if x >= width:
                    px[ty + y][tx + x] = GB_TRIM
                elif ink[y][x]:
                    px[ty + y][tx + x] = GB_INK
    return px


def png_bytes(px):
    h, w = len(px), len(px[0])
    raw = b"".join(b"\x00" + bytes(c for p in row for c in p) for row in px)

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def slug(name):
    s = re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-")
    return s or "my-font"


def how_to(name, s):
    return "\r\n".join([
        f"{name} - a Game Boy font made in Type Designer Pocket",
        "",
        "To use it in GB Studio (3 or newer):",
        f"1. Copy {s}.png and {s}.json into your project's assets/fonts folder.",
        "2. In GB Studio, open Settings > UI Elements and pick it as the Default Font,",
        "   or switch to it mid-game with the Set Font event.",
        "",
        "It's a variable-width font: the magenta columns in the PNG mark where each",
        "character ends. Letters you only drew in one case are used for both cases.",
        "",
        "Accented letters and the other Latin-1 characters (é ñ ü £ © ½...) work as you",
        "type them. So do the symbols (← ↑ ♥ ★ ♪ “ ” … € ™...): the .json",
        "maps them to the font's characters 128-159.",
        "",
        "Docs: https://www.gbstudio.dev/docs/settings/#ui-elements--fonts",
        "",
    ])


def export(save, slot_index, name, out_dir, make_zip=False):
    """Write the GB Studio files for one slot. Returns the paths written."""
    slot = save.slots[slot_index]
    name = name or slot.name.title() or "My Font"
    s = slug(name)
    files = {
        f"{s}.png": png_bytes(sheet_pixels(slot)),
        f"{s}.json": (json.dumps({"name": name, "mapping": tdpsave.gbs_mapping()}, indent=2, ensure_ascii=False)
                      + "\n").encode(),
        "HOW-TO-USE.txt": how_to(name, s).encode(),
    }
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    if make_zip:
        path = out_dir / f"{s}-gbstudio.zip"
        with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
            for fn, data in files.items():
                z.writestr(fn, data)
        return [path]
    paths = []
    for fn, data in files.items():
        (out_dir / fn).write_bytes(data)
        paths.append(out_dir / fn)
    return paths


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("save", help=".sav / .ram file")
    ap.add_argument("--slot", type=int, help="slot number, 1-3 (default: the slot the game last used)")
    ap.add_argument("--name", help="font name shown in GB Studio (default: the slot name)")
    ap.add_argument("--out", default=".", help="output folder (default: current folder)")
    ap.add_argument("--zip", action="store_true", help="write one -gbstudio.zip instead of loose files")
    ap.add_argument("--list", action="store_true", help="list the slots and exit")
    ap.add_argument("--force", action="store_true", help="export even if the slot's checksum fails")
    a = ap.parse_args(argv)

    try:
        save = tdpsave.parse(Path(a.save).read_bytes())
    except tdpsave.NotASave as e:
        ap.exit(1, f"{a.save}: {e}\n")
    if not save.header_ok:
        print("warning: save header checksum failed", file=sys.stderr)

    if a.list:
        for n, s in enumerate(save.slots, 1):
            drawn = sum(s.has_ink(i) for i in range(tdpsave.GLYPHS))
            flags = ("" if s.crc_ok else "  CHECKSUM FAILED") + ("  (active)" if n - 1 == save.active else "")
            guide = tdpsave.TEMPLATES.get(s.template_id, f"template {s.template_id}")
            print(f"slot {n}: {s.name:<12} {drawn:3d} glyphs drawn, guide: {guide}{flags}")
        return 0

    idx = (a.slot - 1) if a.slot else save.active
    if not 0 <= idx < len(save.slots):
        ap.exit(1, f"slot {a.slot} doesn't exist (1-{len(save.slots)})\n")
    if not save.slots[idx].crc_ok and not a.force:
        ap.exit(1, f"slot {idx + 1} failed its checksum, so the save may be damaged. Use --force to export anyway.\n")
    for p in export(save, idx, a.name, a.out, a.zip):
        print(p)
    return 0


if __name__ == "__main__":
    sys.exit(main())
