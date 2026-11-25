// The Play page: the game full size, game controllers, and the save kept in a file on the computer.
import { Pocket, resumeAudio, audioRunning } from "./emulator.js";
import { parseSave, NotASave, SAVE_SIZE } from "./save.js";
import { storedSram, storeSram, SaveFile, canUseFiles } from "./store.js";
import { Gamepads } from "./gamepad.js";

const $ = (sel) => document.querySelector(sel);
const ROM_URL = "type-designer-pocket.gb";
const KEYS = {
  ArrowUp: "up", ArrowDown: "down", ArrowLeft: "left", ArrowRight: "right",
  KeyX: "a", KeyZ: "b", Enter: "start", ShiftLeft: "select", ShiftRight: "select", Backspace: "select",
};

let pocket = null;
let romBuffer = null;
let file = null;        // the linked SaveFile, or null: saving in this browser only
let pending = null;     // the latest save not yet written, while the game is mid-save
let timer = 0;
const pads = new Gamepads(() => showPad());

// ---- the game ----

async function startGame(sram) {
  const msg = $("#screen-msg");
  try {
    if (pocket) pocket.stop();
    romBuffer ||= await (await fetch(ROM_URL)).arrayBuffer();
    pocket = await Pocket.start($("#screen"), romBuffer, sram);
    pocket.onFrame = () => pads.poll(pocket);
    pocket.onSramChange = (data) => {
      // the game writes its save in several pieces: wait until it has finished
      pending = data;
      clearTimeout(timer);
      timer = setTimeout(flush, 400);
    };
    msg.hidden = true;
  } catch (err) {
    msg.hidden = false;
    msg.textContent = `The game couldn't start: ${err.message}`;
  }
}

async function flush() {
  clearTimeout(timer);
  if (!pending) return;
  const data = pending;
  pending = null;
  storeSram(data);
  if (!file) return;
  try {
    await file.write(data);
    showSave(`Last saved ${new Date().toLocaleTimeString()}.`);
  } catch (err) {
    showSave(`Couldn't write ${file.name}: ${err.message}. Your fonts are still kept in this browser.`, true);
  }
}

const toSram = (bytes) => {
  const sram = new Uint8Array(SAVE_SIZE);
  sram.set(bytes.subarray(0, SAVE_SIZE));
  return sram;
};

// ---- the save file ----

function showSave(extra = "", warn = false) {
  const status = $("#save-status");
  status.textContent = (file ? `Saving to ${file.name} on your computer.` : "Saving in this browser only.") + (extra ? ` ${extra}` : "");
  status.classList.toggle("warn", warn);
  $("#btn-unlink").hidden = !file;
  $("#btn-reconnect").hidden = true;
}

// Start using a file the visitor has granted. An empty file gets the game in progress; a save is loaded and
// the game restarts with it.
async function linkFile(f, { load }) {
  const bytes = await f.read();
  if (bytes.length && load) {
    let save;
    try {
      save = parseSave(bytes);
    } catch (err) {
      showSave(err instanceof NotASave ? `${f.name}: ${err.message}` : `Couldn't read ${f.name}.`, true);
      return false;
    }
    file = f;
    const sram = toSram(bytes);
    storeSram(sram);
    await startGame(sram);
    showSave(save.slots.some((s) => !s.crcOk) ? "One of its fonts failed its checksum, so check it." : "Loaded it.");
  } else {
    file = f;
    pending = pocket ? pocket.getSram() : storedSram() || new Uint8Array(SAVE_SIZE);
    await flush();
    showSave("Wrote the game in progress to it.");
  }
  await f.remember();
  return true;
}

function bindSave() {
  if (!canUseFiles) {
    for (const id of ["#btn-open-save", "#btn-new-save"]) $(id).hidden = true;
    $("#save-note").textContent =
      "This browser can't keep a save file on your computer (Chrome and Edge can). Your fonts stay in this browser; Download .sav keeps a copy.";
  }
  $("#btn-open-save").addEventListener("click", async () => {
    try {
      const f = await SaveFile.open();
      if (!(await f.permitted(true))) return showSave(`No permission to write ${f.name}.`, true);
      if (storedSram() && !confirm(`Load the fonts in ${f.name}? The game restarts with that save, in place of the one in this browser.`)) return;
      await linkFile(f, { load: true });
    } catch (err) {
      if (err.name !== "AbortError") showSave(`Couldn't open the file: ${err.message}`, true);
    }
  });
  $("#btn-new-save").addEventListener("click", async () => {
    try {
      await linkFile(await SaveFile.create(), { load: false });
    } catch (err) {
      if (err.name !== "AbortError") showSave(`Couldn't create the file: ${err.message}`, true);
    }
  });
  $("#btn-unlink").addEventListener("click", async () => {
    await flush();
    file = null;
    await SaveFile.forget();
    showSave("The file keeps the last save.");
  });
  $("#btn-download-sav").addEventListener("click", () => {
    if (!pocket) return;
    const a = document.createElement("a");
    a.href = URL.createObjectURL(new Blob([pocket.getSram()]));
    a.download = file ? file.name : "type-designer-pocket.sav";
    a.click();
    setTimeout(() => URL.revokeObjectURL(a.href), 10000);
  });
  // don't lose the last save when the tab closes or hides
  document.addEventListener("visibilitychange", () => document.visibilityState === "hidden" && flush());
  window.addEventListener("pagehide", flush);
}

// A file linked on an earlier visit. The browser only lets the page write to it again after a click.
async function reconnectOrStart() {
  const f = canUseFiles ? await SaveFile.remembered() : null;
  if (!f) return startGame(storedSram());
  if (await f.permitted(false)) {
    if (await linkFile(f, { load: true })) return;
    return startGame(storedSram());
  }
  const msg = $("#screen-msg");
  msg.hidden = false;
  msg.replaceChildren();
  const again = Object.assign(document.createElement("button"), { className: "btn primary", textContent: `Use ${f.name}` });
  const skip = Object.assign(document.createElement("button"), { className: "btn", textContent: "Play from this browser" });
  msg.append(again, skip);
  $("#save-status").textContent = `Last time the game saved to ${f.name}. Reconnect it to keep saving there.`;
  const reconnect = $("#btn-reconnect");
  reconnect.textContent = `Reconnect ${f.name}`;
  reconnect.hidden = false;
  const go = async () => {
    if (!(await f.permitted(true))) return showSave(`No permission to write ${f.name}.`, true);
    if (!(await linkFile(f, { load: true }))) await startGame(storedSram());
  };
  again.onclick = reconnect.onclick = go;
  skip.onclick = async () => {
    await startGame(storedSram());
    showSave();
    reconnect.hidden = false;
  };
}

// ---- controls ----

function showPad() {
  const name = pads.name();
  $("#pad-status").textContent = name
    ? `Connected: ${name}.`
    : "No controller yet. Pair it with your computer, then press any button on it.";
}

function bindControls() {
  const typing = (e) => /^(INPUT|TEXTAREA|SELECT)$/.test(e.target.tagName);
  for (const ev of ["keydown", "pointerdown"]) document.addEventListener(ev, resumeAudio, { capture: true });
  document.addEventListener("keydown", (e) => {
    if (typing(e) || e.metaKey || e.ctrlKey || e.altKey) return;
    if (e.code === "KeyF") return toggleFull();
    if (!pocket || !(e.code in KEYS)) return;
    e.preventDefault();
    pocket.set(KEYS[e.code], true);
  });
  document.addEventListener("keyup", (e) => {
    if (pocket && e.code in KEYS) pocket.set(KEYS[e.code], false);
  });
  window.addEventListener("blur", () => pocket && pocket.releaseAll());
  $("#swap-ab").checked = pads.swap;
  $("#swap-ab").addEventListener("change", (e) => pads.setSwap(e.target.checked));
  $("#btn-full").addEventListener("click", toggleFull);
  // browsers only allow sound after a click or key press (a controller doesn't count)
  setInterval(() => ($("#sound-hint").hidden = audioRunning() || !pocket), 500);
}

function toggleFull() {
  if (document.fullscreenElement) document.exitFullscreen();
  else $("#stage").requestFullscreen?.();
}

// ---- go ----

// For checks in the browser console: window.tdp.pocket is the running game.
window.tdp = { get pocket() { return pocket; }, get file() { return file; }, linkFile, flush, pads };

bindSave();
bindControls();
showSave();
showPad();
reconnectOrStart();
