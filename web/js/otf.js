// Pixel font -> OpenType (.otf), built with opentype.js (window.opentype, vendored).
// Every glyph uses the same placement and width as the GB Studio export, so text set with the .otf
// matches the game's type test. One pixel is 125 units: 8 px = 1000 units per em.
import { FIRST_CODE, GLYPHS, tileFor, exportTile, charOf } from "./save.js";

const PX = 125;
const BASELINE_ROW = 7; // the baseline sits under row 6: rows 0-6 above it, row 7 descends

// Directions in screen space (y down), clockwise order: E, S, W, N.
const DIRS = [[1, 0], [0, 1], [-1, 0], [0, -1]];

/**
 * Outline the ink of an 8x8 tile as closed polygons, merging touching pixels. Edges run clockwise in
 * screen space (filled side on the right). Where two pixels touch only at a corner the tracer turns right,
 * so they stay separate shapes. Collinear points are dropped.
 */
export function traceInk(ink) {
  const on = (x, y) => x >= 0 && y >= 0 && x < 8 && y < 8 && ink[y][x] === 1;
  const edges = new Map(); // "x,y" -> list of {to, dir}
  const add = (x0, y0, x1, y1, dir) => {
    const k = `${x0},${y0}`;
    if (!edges.has(k)) edges.set(k, []);
    edges.get(k).push({ x: x1, y: y1, dir, used: false });
  };
  for (let y = 0; y < 8; y++)
    for (let x = 0; x < 8; x++) {
      if (!on(x, y)) continue;
      if (!on(x, y - 1)) add(x, y, x + 1, y, 0);
      if (!on(x + 1, y)) add(x + 1, y, x + 1, y + 1, 1);
      if (!on(x, y + 1)) add(x + 1, y + 1, x, y + 1, 2);
      if (!on(x - 1, y)) add(x, y + 1, x, y, 3);
    }
  const contours = [];
  for (const [key, list] of edges) {
    for (const first of list) {
      if (first.used) continue;
      const [sx, sy] = key.split(",").map(Number);
      const pts = [[sx, sy]];
      let e = first;
      for (;;) {
        e.used = true;
        const next = (edges.get(`${e.x},${e.y}`) || []).filter((n) => !n.used);
        if (e.x === sx && e.y === sy) break;
        pts.push([e.x, e.y]);
        if (!next.length) break;
        // prefer a right turn, then straight on, then left
        const rank = (n) => [1, 0, 3, 2].indexOf((n.dir - e.dir + 4) % 4);
        next.sort((a, b) => rank(a) - rank(b));
        e = next[0];
      }
      contours.push(dropCollinear(pts));
    }
  }
  return contours;
}

function dropCollinear(pts) {
  const out = [];
  for (let i = 0; i < pts.length; i++) {
    const a = pts[(i + pts.length - 1) % pts.length], b = pts[i], c = pts[(i + 1) % pts.length];
    if ((b[0] - a[0]) * (c[1] - b[1]) !== (b[1] - a[1]) * (c[0] - b[0])) out.push(b);
  }
  return out;
}

function glyphPath(ink) {
  const path = new opentype.Path();
  for (const pts of traceInk(ink)) {
    // screen (y down) -> font units (y up); the flip turns clockwise outlines counter-clockwise, as CFF wants
    pts.forEach(([x, y], i) => path[i ? "lineTo" : "moveTo"](x * PX, (BASELINE_ROW - y) * PX));
    path.close();
  }
  return path;
}

export function buildOtf(slot, name) {
  const glyphs = [new opentype.Glyph({ name: ".notdef", unicode: 0, advanceWidth: 4 * PX, path: new opentype.Path() })];
  for (let code = FIRST_CODE; code < FIRST_CODE + GLYPHS; code++) {
    const ch = charOf(code);
    if (!ch) continue; // 127
    const { rows, override } = tileFor(slot, code);
    // undrawn: leave it out so apps fall back instead of showing a gap (the two spaces stay, blank)
    if (!rows && code !== 32 && code !== 160) continue;
    const { ink, width } = exportTile(rows, override, slot.spaceWidth);
    const uni = ch.codePointAt(0); // ASCII and Latin-1 as they are; 128-159 are the symbols (see save.js)
    glyphs.push(new opentype.Glyph({
      name: code === 32 ? "space" : `uni${uni.toString(16).toUpperCase().padStart(4, "0")}`,
      unicode: uni,
      advanceWidth: width * PX,
      path: glyphPath(ink),
    }));
  }
  const font = new opentype.Font({
    familyName: name,
    styleName: "Regular",
    unitsPerEm: 8 * PX,
    ascender: BASELINE_ROW * PX,
    descender: -(8 - BASELINE_ROW) * PX,
    designer: "Made in Type Designer Pocket",
    glyphs,
  });
  return font.toArrayBuffer();
}
