// Run by tests/test_web.py: feeds a save through the website's JavaScript and prints JSON for Python to compare.
//   node tests/web_check.mjs SAVE.sav "sample text" ...
import { readFileSync } from "node:fs";
import { parseSave, tileFor, exportTile, layout, FIRST_CODE, GLYPHS } from "../web/js/save.js";
import { traceInk } from "../web/js/otf.js";

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
  return {
    name: slot.name,
    templateId: slot.templateId,
    spaceWidth: slot.spaceWidth,
    crcOk: slot.crcOk,
    tiles,
    areas,
    layouts: texts.map((t) => layout(slot, t)),
  };
});
console.log(JSON.stringify({ active: save.active, headerOk: save.headerOk, slots }));
