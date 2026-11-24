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

// The character set. Codes 32-126 are ASCII and 160-255 are Latin-1, so GB Studio needs no mapping for them.
// 128-159 hold symbols, which GB Studio reaches through the font's "mapping". 127 isn't used.
export const EXTRA = "←→↑↓▲▼◀▶♥♦♣♠★♪●○‘’“”•…–—€™Œœ✓✗■□";
export const UNUSED = 127;

/** The Unicode character a code stands for, or null for 127. */
export function charOf(code) {
  if (code === UNUSED) return null;
  return code >= 128 && code < 160 ? EXTRA[code - 128] : String.fromCharCode(code);
}

/** The code for a character typed on the web page, or -1 if it isn't in the set. */
export function codeOf(ch) {
  const k = EXTRA.indexOf(ch);
  if (k >= 0) return 128 + k;
  const c = ch.charCodeAt(0);
  return (c >= 32 && c < 127) || (c >= 160 && c <= 255) ? c : -1;
}

/** GB Studio font JSON "mapping" for the symbols: {"←": 128, ...}. */
export const gbsMapping = () => Object.fromEntries([...EXTRA].map((ch, k) => [ch, 128 + k]));

/** The same letter in the other case, if it has one in this set (ASCII and Latin-1), else -1. */
export function otherCase(c) {
  if ((c >= 65 && c <= 90) || (c >= 192 && c <= 222 && c !== 215)) return c + 32;
  if ((c >= 97 && c <= 122) || (c >= 224 && c <= 254 && c !== 247)) return c - 32;
  return -1;
}

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

const countInk = (slot, from, to) => slot.glyphs.slice(from, to).filter((g) => g.some((b) => b)).length;
export const drawnCount = (slot) => countInk(slot, 0, 95);             // codes 32-126
export const drawnExtCount = (slot) => countInk(slot, 96, GLYPHS);    // codes 128-255

/** Glyph rows for a character code: its own drawing, else the other case of the same letter
 * (A-Z and the Latin-1 letters: é falls back to É and the other way round). */
export function tileFor(slot, code) {
  const i = code - FIRST_CODE;
  if (i < 0 || i >= GLYPHS) return { rows: null, override: AUTO };
  if (hasInk(slot, i)) return { rows: rows(slot, i), override: slot.widths[i] };
  const other = otherCase(code) - FIRST_CODE;
  if (other >= 0 && hasInk(slot, other)) return { rows: rows(slot, other), override: slot.widths[other] };
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

/** A character as GB Studio sets it. Characters outside the set get the space width, like undrawn ones. */
export function glyphFor(slot, ch) {
  const code = codeOf(ch);
  if (code < 0) return { ...exportTile(null, AUTO, slot.spaceWidth), drawn: false };
  const { rows: r, override } = tileFor(slot, code);
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
