"""Type Designer Pocket save format (battery RAM / emulator .sav), version 1.

Layout, little-endian, starting at the beginning of the 8 KB save RAM (0xA000 on the cartridge):

  Header (16 bytes)
    0   magic        "TDPF"
    4   version      1
    5   slot_count   3
    6   active       slot shown at boot
    7   reserved     7 bytes, zero
    14  crc16        CRC-16/CCITT-FALSE of bytes 0-13

  Slot i at 16 + i * 2064 (2060 bytes used, 4 spare)
    0     name          12 bytes, ASCII, space padded
    12    template_id   0 = blank
    13    space_width   pixels GB Studio gives an undrawn glyph (default 4)
    14    used          28 bytes, bit (i & 7) of byte i >> 3 set when glyph i has ink
    42    widths        224 bytes, 0xFF = auto, else a fixed GB Studio width 1-8
    266   glyphs        224 x 8 bytes, one per row, bit 7 = leftmost pixel; glyph i is code 32 + i
    2058  crc16         CRC-16/CCITT-FALSE of bytes 0-2057

Must match src/save.h.
"""
import struct
from dataclasses import dataclass, field

MAGIC = b"TDPF"
VERSION = 1
HEADER_SIZE = 16
SLOT_SIZE = 2060
SLOT_STRIDE = 2064
SLOT_COUNT = 3
GLYPHS = 224
FIRST_CODE = 32
SAVE_SIZE = 8192
AUTO = 0xFF


def crc16(data, crc=0xFFFF):
    """CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection, no final xor."""
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


@dataclass
class Slot:
    name: str = "FONT"
    template_id: int = 0
    space_width: int = 4
    widths: list = field(default_factory=lambda: [AUTO] * GLYPHS)
    glyphs: list = field(default_factory=lambda: [bytes(8) for _ in range(GLYPHS)])
    crc_ok: bool = True

    def rows(self, i):
        """Glyph i as 8 rows of 8 ints (0/1)."""
        return [[(b >> (7 - x)) & 1 for x in range(8)] for b in self.glyphs[i]]

    def has_ink(self, i):
        return any(self.glyphs[i])

    def pack(self):
        used = bytearray(GLYPHS // 8)
        for i in range(GLYPHS):
            if self.has_ink(i):
                used[i >> 3] |= 1 << (i & 7)
        name = self.name.encode("ascii", "replace")[:12].ljust(12, b" ")
        body = name + bytes([self.template_id, self.space_width]) + bytes(used) + bytes(self.widths)
        body += b"".join(bytes(g) for g in self.glyphs)
        assert len(body) == SLOT_SIZE - 2
        return body + struct.pack("<H", crc16(body))

    @classmethod
    def unpack(cls, raw):
        body, (crc,) = raw[: SLOT_SIZE - 2], struct.unpack_from("<H", raw, SLOT_SIZE - 2)
        g0 = 266
        return cls(
            name=raw[0:12].decode("ascii", "replace").rstrip(),
            template_id=raw[12],
            space_width=raw[13],
            widths=list(raw[42:g0]),
            glyphs=[bytes(raw[g0 + i * 8 : g0 + i * 8 + 8]) for i in range(GLYPHS)],
            crc_ok=crc16(body) == crc,
        )


@dataclass
class Save:
    slots: list
    active: int = 0
    header_ok: bool = True


class NotASave(ValueError):
    pass


def parse(data):
    """Parse a .sav / .ram image. Raises NotASave if it isn't a Type Designer Pocket save."""
    if len(data) < HEADER_SIZE + SLOT_COUNT * SLOT_STRIDE:
        raise NotASave(f"file is {len(data)} bytes, too small for a Type Designer Pocket save")
    if data[0:4] != MAGIC:
        raise NotASave("no TDPF header: not a Type Designer Pocket save, or the game has never been started")
    version, count, active = data[4], data[5], data[6]
    if version != VERSION:
        raise NotASave(f"save version {version} is not supported (expected {VERSION})")
    (hcrc,) = struct.unpack_from("<H", data, 14)
    count = min(count, SLOT_COUNT)
    slots = [Slot.unpack(data[HEADER_SIZE + i * SLOT_STRIDE : HEADER_SIZE + i * SLOT_STRIDE + SLOT_SIZE]) for i in range(count)]
    return Save(slots=slots, active=active if active < count else 0, header_ok=crc16(data[:14]) == hcrc)


def build(slots, active=0):
    """Build a full 8 KB save image from up to SLOT_COUNT slots (missing slots are blank)."""
    slots = list(slots) + [Slot(name=f"FONT {i + 1}") for i in range(len(slots), SLOT_COUNT)]
    head = MAGIC + bytes([VERSION, SLOT_COUNT, active]) + bytes(7)
    out = bytearray(SAVE_SIZE)
    out[0:16] = head + struct.pack("<H", crc16(head))
    for i, s in enumerate(slots[:SLOT_COUNT]):
        o = HEADER_SIZE + i * SLOT_STRIDE
        out[o : o + SLOT_SIZE] = s.pack()
    return bytes(out)


def export_tile(rows, override=AUTO, space_width=4):
    """GB Studio placement for one glyph: (ink rows shifted into place, width).

    Matches the web Type Making Game: ink moves to start at column 1 (column 0 if it fills all 8),
    and the width is the ink plus one blank column on each side, capped at 8. An undrawn glyph is blank
    and `space_width` wide. A fixed width (override != 0xFF) replaces the computed one.
    """
    cols = [x for x in range(8) if any(r[x] for r in rows)]
    if not cols:
        out, width = [[0] * 8 for _ in range(8)], space_width
    else:
        lo, hi = cols[0], cols[-1]
        ink_w = hi - lo + 1
        left = 0 if ink_w >= 8 else 1
        width = min(8, left + ink_w + 1)
        out = [[0] * 8 for _ in range(8)]
        for y in range(8):
            for x in range(lo, hi + 1):
                if rows[y][x]:
                    out[y][left + x - lo] = 1
    if override != AUTO:
        width = max(1, min(8, override))
    return out, width
