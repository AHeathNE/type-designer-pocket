// Run by tests/test_web.py: feeds a save through the website's JavaScript and prints JSON for Python to compare.
//   node tests/web_check.mjs SAVE.sav "sample text" ...
// Sample texts are Unicode, as a visitor would type them on the page.
import { readFileSync } from "node:fs";
import { createRequire } from "node:module";
import { parseSave, tileFor, exportTile, layout, charOf, codeOf, gbsMapping, FIRST_CODE, GLYPHS } from "../web/js/save.js";
import { traceInk, buildOtf } from "../web/js/otf.js";
import { howTo } from "../web/js/gbstudio.js";

globalThis.opentype = createRequire(import.meta.url)("../web/vendor/opentype.min.js");

const [savePath, ...texts] = process.argv.slice(2);
const save = parseSave(new Uint8Array(readFileSync(savePath)));

const slots = save.slots.map((slot) => {
  const tiles = [];
  for (let code = FIRST_CODE; code < FIRST_CODE + GLYPHS; code++) {
    const { rows, override } = tileFor(slot, code);
    tiles.push(exportTile(rows, override, slot.spaceWidth));
  }
  // outline area (shoelace) per glyph: must equal the number of ink pixels
  const areas = tiles.map(({ ink }) =>
    traceInk(ink).reduce((sum, pts) => {
      let a = 0;
      for (let i = 0; i < pts.length; i++) {
        const [x0, y0] = pts[i], [x1, y1] = pts[(i + 1) % pts.length];
        a += x0 * y1 - x1 * y0;
      }
      return sum + a / 2;
    }, 0));
  // the .otf, read back: Unicode code point -> advance width (in pixels)
  const font = opentype.parse(buildOtf(slot, slot.name || "Font"));
  const cmap = {};
  for (let g = 0; g < font.glyphs.length; g++) {
    const glyph = font.glyphs.get(g);
    for (const u of glyph.unicodes || []) if (u) cmap[u] = glyph.advanceWidth / 125;
  }
  return {
    name: slot.name,
    templateId: slot.templateId,
    spaceWidth: slot.spaceWidth,
    crcOk: slot.crcOk,
    tiles,
    areas,
    cmap,
    layouts: texts.map((t) => layout(slot, t)),
  };
});
const chars = [];
for (let code = FIRST_CODE; code < FIRST_CODE + GLYPHS; code++) {
  const ch = charOf(code);
  chars.push(ch);
  if (ch !== null && codeOf(ch) !== code) throw new Error(`codeOf(charOf(${code})) is ${codeOf(ch)}`);
}
console.log(JSON.stringify({
  active: save.active, headerOk: save.headerOk, slots, chars, mapping: gbsMapping(), howTo: howTo("My Font", "my-font"),
}));
