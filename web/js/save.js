// Type Designer Pocket save format and GB Studio's view of a font.
// A port of tools/tdpsave.py and tools/sav2font.py: keep all three (and src/save.h, src/gfx.c) in step.

export const MAGIC = "TDPF";
export const VERSION = 1;
export const HEADER_SIZE = 16;
export const SLOT_SIZE = 2060;
export const SLOT_STRIDE = 2064;
export const SLOT_COUNT = 3;
export const GLYPHS = 224;
export const FIRST_CODE = 32;
export const SAVE_SIZE = 8192;
export const AUTO = 0xff;
export const LINE_PX = 144; // GB Studio's 18-tile text width, also the game's type test
export const TEMPLATES = { 0: "Pocket Sans", 1: "Pocket Serif", 2: "Classic 8×8", 255: "no guide" };

// CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection, no final xor
const CRC_TABLE = (() => {
  const t = new Uint16Array(256);
  for (let i = 0; i < 256; i++) {
    let c = i << 8;
    for (let b = 0; b < 8; b++) c = c & 0x8000 ? ((c << 1) ^ 0x1021) & 0xffff : (c << 1) & 0xffff;
    t[i] = c;
  }
  return t;
})();

export function crc16(bytes) {
  let crc = 0xffff;
  for (const b of bytes) crc = ((crc << 8) & 0xffff) ^ CRC_TABLE[((crc >> 8) ^ b) & 0xff];
  return crc;
}

export class NotASave extends Error {}

function u16(bytes, at) {
  return bytes[at] | (bytes[at + 1] << 8);
}

function parseSlot(raw) {
  const g0 = 266;
  const glyphs = [];
  for (let i = 0; i < GLYPHS; i++) glyphs.push(raw.slice(g0 + i * 8, g0 + i * 8 + 8));
  return {
    name: String.fromCharCode(...raw.slice(0, 12)).replace(/\s+$/, ""),
    templateId: raw[12],
    spaceWidth: raw[13],
    widths: raw.slice(42, g0),
    glyphs,
    crcOk: crc16(raw.subarray(0, SLOT_SIZE - 2)) === u16(raw, SLOT_SIZE - 2),
  };
}

/** Parse a .sav / .ram image (Uint8Array). Throws NotASave if it isn't a Type Designer Pocket save. */
export function parseSave(data) {
  if (data.length < HEADER_SIZE + SLOT_COUNT * SLOT_STRIDE)
    throw new NotASave(`This file is ${data.length} bytes, too small to be a Type Designer Pocket save.`);
  if (String.fromCharCode(...data.slice(0, 4)) !== MAGIC)
    throw new NotASave("This isn't a Type Designer Pocket save, or the game hasn't been started with it yet.");
  if (data[4] !== VERSION) throw new NotASave(`Save version ${data[4]} isn't supported (expected ${VERSION}).`);
  const count = Math.min(data[5], SLOT_COUNT);
  const slots = [];
  for (let i = 0; i < count; i++) {
    const o = HEADER_SIZE + i * SLOT_STRIDE;
    slots.push(parseSlot(data.slice(o, o + SLOT_SIZE)));
  }
  return {
    slots,
    active: data[6] < count ? data[6] : 0,
    headerOk: crc16(data.subarray(0, 14)) === u16(data, 14),
  };
}

export const hasInk = (slot, i) => slot.glyphs[i].some((b) => b);

export function rows(slot, i) {
  return Array.from(slot.glyphs[i], (b) => Array.from({ length: 8 }, (_, x) => (b >> (7 - x)) & 1));
}

export const drawnCount = (slot) => slot.glyphs.slice(0, 95).filter((g) => g.some((b) => b)).length;

/** Glyph rows for a character code: its own drawing, else the other case of the same letter. */
export function tileFor(slot, code) {
  const i = code - FIRST_CODE;
  if (i < 0 || i >= GLYPHS) return { rows: null, override: AUTO };
  if (hasInk(slot, i)) return { rows: rows(slot, i), override: slot.widths[i] };
  const ch = String.fromCharCode(code);
  if (/^[A-Za-z]$/.test(ch)) {
    const other = (ch === ch.toUpperCase() ? ch.toLowerCase() : ch.toUpperCase()).charCodeAt(0) - FIRST_CODE;
    if (hasInk(slot, other)) return { rows: rows(slot, other), override: slot.widths[other] };
  }
  return { rows: null, override: slot.widths[i] };
}

/**
 * GB Studio placement for one glyph: the ink moved to start at column 1 (column 0 if it fills all 8) and the
 * width (ink plus a blank column each side, capped at 8). An undrawn glyph is blank and spaceWidth wide. A
 * fixed width (override != AUTO) replaces the computed one.
 */
export function exportTile(glyphRows, override = AUTO, spaceWidth = 4) {
  const out = Array.from({ length: 8 }, () => new Array(8).fill(0));
  let width = spaceWidth;
  const cols = glyphRows ? [0, 1, 2, 3, 4, 5, 6, 7].filter((x) => glyphRows.some((r) => r[x])) : [];
  if (cols.length) {
    const lo = cols[0], hi = cols[cols.length - 1];
    const inkW = hi - lo + 1;
    const left = inkW >= 8 ? 0 : 1;
    width = Math.min(8, left + inkW + 1);
    for (let y = 0; y < 8; y++) for (let x = lo; x <= hi; x++) if (glyphRows[y][x]) out[y][left + x - lo] = 1;
  }
  if (override !== AUTO) width = Math.max(1, Math.min(8, override));
  return { ink: out, width };
}

export function glyphFor(slot, ch) {
  const { rows: r, override } = tileFor(slot, ch.charCodeAt(0));
  return { ...exportTile(r, override, slot.spaceWidth), drawn: !!r };
}

/** Greedy word wrap at `width` px: break at the last space that fits; never start a line with spaces. */
export function layout(slot, text, width = LINE_PX, maxLines = Infinity) {
  const lines = [];
  let i = 0;
  while (i < text.length && lines.length < maxLines) {
    while (i < text.length && text[i] === " ") i++;
    if (i >= text.length) break;
    const start = i;
    let x = 0, brk = -1, j = i;
    for (; j < text.length; j++) {
      const w = glyphFor(slot, text[j]).width;
      if (x + w > width) break;
      if (text[j] === " ") brk = j;
      x += w;
    }
    if (j < text.length && brk > start) {
      lines.push(text.slice(start, brk));
      i = brk + 1;
    } else {
      lines.push(text.slice(start, Math.max(j, start + 1)));
      i = Math.max(j, start + 1);
    }
  }
  return lines;
}
