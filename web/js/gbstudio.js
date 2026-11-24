// GB Studio font export: NAME.png + NAME.json + HOW-TO-USE.txt, as loose files or one zip.
// Matches tools/sav2font.py pixel for pixel.
import { FIRST_CODE, GLYPHS, tileFor, exportTile, gbsMapping } from "./save.js";

// GB Studio's font loader reads the green channel for shades and treats pure white and magenta as
// transparent, so the background must be GB light green, not white.
export const GB_BG = [0xe0, 0xf8, 0xcf];
export const GB_INK = [0x07, 0x18, 0x21];
export const GB_TRIM = [0xff, 0x00, 0xff];
export const SHEET_W = 128, SHEET_H = 112;

export function sheetImageData(slot) {
  const img = new ImageData(SHEET_W, SHEET_H);
  const put = (x, y, c) => {
    const i = (y * SHEET_W + x) * 4;
    img.data[i] = c[0]; img.data[i + 1] = c[1]; img.data[i + 2] = c[2]; img.data[i + 3] = 255;
  };
  for (let code = FIRST_CODE; code < FIRST_CODE + GLYPHS; code++) {
    const i = code - FIRST_CODE, tx = (i % 16) * 8, ty = Math.floor(i / 16) * 8;
    const { rows, override } = tileFor(slot, code);
    const { ink, width } = exportTile(rows, override, slot.spaceWidth);
    for (let y = 0; y < 8; y++)
      for (let x = 0; x < 8; x++) put(tx + x, ty + y, x >= width ? GB_TRIM : ink[y][x] ? GB_INK : GB_BG);
  }
  return img;
}

export async function sheetPng(slot) {
  const c = document.createElement("canvas");
  c.width = SHEET_W;
  c.height = SHEET_H;
  c.getContext("2d").putImageData(sheetImageData(slot), 0, 0);
  const blob = await new Promise((resolve) => c.toBlob(resolve, "image/png"));
  return new Uint8Array(await blob.arrayBuffer());
}

export function slug(name) {
  return name.toLowerCase().replace(/[^a-z0-9]+/g, "-").replace(/^-+|-+$/g, "") || "my-font";
}

export function howTo(name, s) {
  return [
    `${name} - a Game Boy font made in Type Designer Pocket`,
    "",
    "To use it in GB Studio (3 or newer):",
    `1. Copy ${s}.png and ${s}.json into your project's assets/fonts folder.`,
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
  ].join("\r\n");
}

export async function gbStudioZip(slot, name) {
  const s = slug(name);
  const enc = new TextEncoder();
  return makeZip([
    { name: `${s}.png`, data: await sheetPng(slot) },
    { name: `${s}.json`, data: enc.encode(JSON.stringify({ name, mapping: gbsMapping() }, null, 2) + "\n") },
    { name: "HOW-TO-USE.txt", data: enc.encode(howTo(name, s)) },
  ]);
}

// ---- minimal .zip writer (stored, no compression) ----

const CRC32 = (() => {
  const t = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    t[n] = c >>> 0;
  }
  return t;
})();

function crc32(data) {
  let c = 0xffffffff;
  for (const b of data) c = CRC32[(c ^ b) & 0xff] ^ (c >>> 8);
  return (c ^ 0xffffffff) >>> 0;
}

export function makeZip(files) {
  const enc = new TextEncoder();
  const parts = [], central = [];
  let offset = 0;
  for (const f of files) {
    const name = enc.encode(f.name), crc = crc32(f.data), size = f.data.length;
    const local = new DataView(new ArrayBuffer(30));
    local.setUint32(0, 0x04034b50, true);
    local.setUint16(4, 20, true);
    local.setUint32(14, crc, true);
    local.setUint32(18, size, true);
    local.setUint32(22, size, true);
    local.setUint16(26, name.length, true);
    parts.push(new Uint8Array(local.buffer), name, f.data);
    const cd = new DataView(new ArrayBuffer(46));
    cd.setUint32(0, 0x02014b50, true);
    cd.setUint16(4, 20, true);
    cd.setUint16(6, 20, true);
    cd.setUint32(16, crc, true);
    cd.setUint32(20, size, true);
    cd.setUint32(24, size, true);
    cd.setUint16(28, name.length, true);
    cd.setUint32(42, offset, true);
    central.push(new Uint8Array(cd.buffer), name);
    offset += 30 + name.length + size;
  }
  const cdSize = central.reduce((n, p) => n + p.length, 0);
  const end = new DataView(new ArrayBuffer(22));
  end.setUint32(0, 0x06054b50, true);
  end.setUint16(8, files.length, true);
  end.setUint16(10, files.length, true);
  end.setUint32(12, cdSize, true);
  end.setUint32(16, offset, true);
  return new Blob([...parts, ...central, new Uint8Array(end.buffer)], { type: "application/zip" });
}
