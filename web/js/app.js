import { Pocket, resumeAudio } from "./emulator.js";
import { parseSave, NotASave, drawnCount, drawnExtCount, glyphFor, layout, TEMPLATES, LINE_PX, SAVE_SIZE, GLYPHS, UNUSED, FIRST_CODE } from "./save.js";
import { gbStudioZip, slug } from "./gbstudio.js";
import { buildOtf } from "./otf.js";

const $ = (sel, root = document) => root.querySelector(sel);
const ROM_URL = "type-designer-pocket.gb";
const STORE_KEY = "tdp.sram";
const SAMPLE = "The quick brown fox jumps over the lazy dog.";
const KEYS = {
  ArrowUp: "up", ArrowDown: "down", ArrowLeft: "left", ArrowRight: "right",
  KeyX: "a", KeyZ: "b", Enter: "start", ShiftLeft: "select", ShiftRight: "select", Backspace: "select",
};

let pocket = null;
let romBuffer = null;
let source = "game";          // where the fonts panel reads from: "game" or "file"
let fileSave = null;          // parsed save from a dropped .sav
const cardState = [];         // per slot: { name, text } the visitor typed

// ---- saved game in this browser ----

function storedSram() {
  try {
    const b64 = localStorage.getItem(STORE_KEY);
    if (!b64) return null;
    return Uint8Array.from(atob(b64), (c) => c.charCodeAt(0));
  } catch {
    return null;
  }
}

function storeSram(data) {
  try {
    let s = "";
    for (const b of data) s += String.fromCharCode(b);
    localStorage.setItem(STORE_KEY, btoa(s));
  } catch {
    /* private window or storage blocked: the game still runs, it just won't be remembered */
  }
}

// ---- the game ----

async function startGame(sram) {
  const msg = $("#screen-msg");
  try {
    if (pocket) pocket.stop();
    romBuffer ||= await (await fetch(ROM_URL)).arrayBuffer();
    pocket = await Pocket.start($("#screen"), romBuffer, sram);
    let timer = 0;
    pocket.onSramChange = (data) => {
      clearTimeout(timer);
      timer = setTimeout(() => {
        storeSram(data);
        if (source === "game") showFonts(data);
      }, 400);
    };
    msg.hidden = true;
    if (sram && source === "game") showFonts(sram);
  } catch (err) {
    msg.hidden = false;
    msg.textContent = `The game couldn't start: ${err.message}`;
  }
}

function bindControls() {
  const typing = (e) => /^(INPUT|TEXTAREA|SELECT)$/.test(e.target.tagName);
  // browsers only start sound after a key press or tap on the page
  for (const ev of ["keydown", "pointerdown"]) document.addEventListener(ev, resumeAudio, { capture: true });
  document.addEventListener("keydown", (e) => {
    if (typing(e) || !pocket || !(e.code in KEYS) || e.metaKey || e.ctrlKey || e.altKey) return;
    e.preventDefault();
    pocket.set(KEYS[e.code], true);
  });
  document.addEventListener("keyup", (e) => {
    if (pocket && e.code in KEYS) pocket.set(KEYS[e.code], false);
  });
  window.addEventListener("blur", () => pocket && pocket.releaseAll());
  for (const el of document.querySelectorAll("[data-btn]")) {
    const btn = el.dataset.btn;
    const down = (e) => { e.preventDefault(); el.setPointerCapture?.(e.pointerId); el.classList.add("down"); pocket?.set(btn, true); };
    const up = () => { el.classList.remove("down"); pocket?.set(btn, false); };
    el.addEventListener("pointerdown", down);
    el.addEventListener("pointerup", up);
    el.addEventListener("pointercancel", up);
    el.addEventListener("lostpointercapture", up);
  }

  $("#btn-download-sav").addEventListener("click", () => {
    if (pocket) download(new Blob([pocket.getSram()]), "type-designer-pocket.sav");
  });
  $("#file-load-sav").addEventListener("change", async (e) => {
    const file = e.target.files[0];
    e.target.value = "";
    if (!file) return;
    const data = new Uint8Array(await file.arrayBuffer());
    try {
      parseSave(data);
    } catch (err) {
      flash($("#play-msg"), err instanceof NotASave ? err.message : `Couldn't read ${file.name}.`);
      return;
    }
    if (!confirm("Load this save into the game? The fonts you have in the browser now will be replaced.")) return;
    const sram = new Uint8Array(SAVE_SIZE);
    sram.set(data.subarray(0, SAVE_SIZE));
    storeSram(sram);
    await startGame(sram);
    flash($("#play-msg"), `Loaded ${file.name}.`);
  });
}

function flash(el, text) {
  el.textContent = text;
  el.hidden = false;
  clearTimeout(el._t);
  el._t = setTimeout(() => (el.hidden = true), 5000);
}

function download(blob, filename) {
  const a = document.createElement("a");
  a.href = URL.createObjectURL(blob);
  a.download = filename;
  document.body.append(a);
  a.click();
  a.remove();
  setTimeout(() => URL.revokeObjectURL(a.href), 10000);
}

// ---- fonts panel ----

function bindSource() {
  for (const r of document.querySelectorAll('input[name="source"]')) {
    r.addEventListener("change", () => {
      source = r.value;
      $("#drop").hidden = source !== "file";
      if (source === "game") showFonts(pocket ? pocket.getSram() : storedSram());
      else if (fileSave) renderSlots(fileSave);
      else renderSlots(null, "Choose or drop a .sav file from an emulator or a cartridge dump.");
    });
  }
  const drop = $("#drop");
  const take = async (file) => {
    if (!file) return;
    try {
      fileSave = parseSave(new Uint8Array(await file.arrayBuffer()));
      renderSlots(fileSave);
    } catch (err) {
      fileSave = null;
      renderSlots(null, err instanceof NotASave ? err.message : `Couldn't read ${file.name}.`);
    }
  };
  $("#file-open-sav").addEventListener("change", (e) => take(e.target.files[0]));
  drop.addEventListener("dragover", (e) => { e.preventDefault(); drop.classList.add("over"); });
  drop.addEventListener("dragleave", () => drop.classList.remove("over"));
  drop.addEventListener("drop", (e) => {
    e.preventDefault();
    drop.classList.remove("over");
    take(e.dataTransfer.files[0]);
  });
}

function showFonts(sram) {
  if (!sram) return renderSlots(null, "Play the game above and your fonts will show up here.");
  try {
    renderSlots(parseSave(sram));
  } catch (err) {
    renderSlots(null, err instanceof NotASave ? "Play the game above and your fonts will show up here." : err.message);
  }
}

function renderSlots(save, message = "") {
  const list = $("#slots");
  $("#fonts-msg").textContent = message;
  $("#fonts-msg").hidden = !message;
  if (!save) {
    list.replaceChildren();
    return;
  }
  save.slots.forEach((slot, i) => {
    let card = list.children[i];
    if (!card) {
      card = $("#slot-card").content.firstElementChild.cloneNode(true);
      list.append(card);
      bindCard(card, i);
    }
    card._slot = slot;
    const st = (cardState[i] ||= { name: "", text: SAMPLE });
    const fallbackName = slot.name.replace(/\b\w+/g, (w) => w[0] + w.slice(1).toLowerCase());
    $(".slot-title", card).textContent = slot.name || `Font ${i + 1}`;
    const ext = drawnExtCount(slot);
    $(".slot-meta", card).textContent =
      `${drawnCount(slot)}/95 drawn${ext ? ` + ${ext} symbols and accents` : ""} · guide: ` +
      `${TEMPLATES[slot.templateId] ?? "?"}${i === save.active ? " · last opened" : ""}`;
    $(".slot-bad", card).hidden = slot.crcOk;
    $(".name", card).placeholder = fallbackName;
    $(".name", card).value = st.name;
    $(".sample", card).value = st.text;
    drawGrid($(".grid", card), slot);
    drawText($(".test", card), slot, st.text);
  });
  while (list.children.length > save.slots.length) list.lastElementChild.remove();
}

function bindCard(card, i) {
  const fontName = () => $(".name", card).value.trim() || $(".name", card).placeholder || `Font ${i + 1}`;
  $(".name", card).addEventListener("input", (e) => (cardState[i].name = e.target.value));
  $(".sample", card).addEventListener("input", (e) => {
    cardState[i].text = e.target.value;
    drawText($(".test", card), card._slot, e.target.value);
  });
  $(".export-gbs", card).addEventListener("click", async () => {
    const name = fontName();
    download(await gbStudioZip(card._slot, name), `${slug(name)}-gbstudio.zip`);
  });
  $(".export-otf", card).addEventListener("click", () => {
    const name = fontName();
    download(new Blob([buildOtf(card._slot, name)], { type: "font/otf" }), `${slug(name)}.otf`);
  });
}

const colour = (name) => getComputedStyle(document.documentElement).getPropertyValue(name).trim();

// glyph grid: all 224 cells in GB Studio's sheet order (codes 32-255, 16 to a row), ink only
function drawGrid(canvas, slot) {
  const S = 3, ctx = canvas.getContext("2d");
  canvas.width = 16 * 10 * S;
  canvas.height = (GLYPHS / 16) * 10 * S;
  ctx.fillStyle = colour("--paper");
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  const ink = colour("--ink");
  const empty = colour("--faint");
  for (let i = 0; i < GLYPHS; i++) {
    if (i === UNUSED - FIRST_CODE) continue;
    const ox = ((i % 16) * 10 + 1) * S, oy = (Math.floor(i / 16) * 10 + 1) * S;
    const g = slot.glyphs[i];
    if (!g.some((b) => b)) {
      ctx.fillStyle = empty;
      ctx.fillRect(ox + 3 * S, oy + 7 * S, 2 * S, S);   // a small dash marks an undrawn character
      continue;
    }
    ctx.fillStyle = ink;
    for (let y = 0; y < 8; y++) for (let x = 0; x < 8; x++) if ((g[y] >> (7 - x)) & 1) ctx.fillRect(ox + x * S, oy + y * S, S, S);
  }
}

// type test: the same layout and widths as the game and the GB Studio export
function drawText(canvas, slot, text) {
  const S = 3, LINE_H = 10;
  const lines = layout(slot, text, LINE_PX, 8);
  canvas.width = LINE_PX * S;
  canvas.height = Math.max(1, lines.length) * LINE_H * S;
  const ctx = canvas.getContext("2d");
  ctx.fillStyle = colour("--paper");
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  ctx.fillStyle = colour("--ink");
  lines.forEach((line, k) => {
    let x = 0;
    for (const ch of line) {
      const { ink, width } = glyphFor(slot, ch);
      for (let y = 0; y < 8; y++)
        for (let dx = 0; dx < 8; dx++)
          if (ink[y][dx] && x + dx < LINE_PX) ctx.fillRect((x + dx) * S, (k * LINE_H + 1 + y) * S, S, S);
      x += width;
    }
  });
}

// ---- go ----

// For debugging in the browser console (and the site's own checks): window.tdp.pocket is the running game.
window.tdp = { get pocket() { return pocket; }, parseSave };

bindControls();
bindSource();
showFonts(storedSram());
startGame(storedSram());
