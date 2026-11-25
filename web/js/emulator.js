// The Type Designer Pocket ROM running in the browser, on binjgb (MIT, vendored in vendor/binjgb).
// Based on binjgb's own docs/simple.js player, cut down: no rewind. The ROM is Game Boy Color compatible, so
// binjgb runs it as a Color: the colours come from the game's own COLORS setting.

const CPU_TICKS_PER_SECOND = 4194304;
const MAX_STEP_SEC = 5 / 60;
const EVENT_NEW_FRAME = 1;
const EVENT_AUDIO_BUFFER_FULL = 2;
const EVENT_UNTIL_TICKS = 4;
const AUDIO_FRAMES = 4096;
const AUDIO_LATENCY_SEC = 0.1;
const VOLUME = 0.5;
const CGB_COLOR_CURVE = 2;   // binjgb's approximation of a real Game Boy Color screen

// One audio context for the page. Browsers keep it suspended until the visitor presses a key or taps:
// call resumeAudio() from those handlers.
let audioCtx = null;
function audio() {
  audioCtx ||= new (window.AudioContext || window.webkitAudioContext)();
  return audioCtx;
}
export function resumeAudio() {
  if (audioCtx && audioCtx.state !== "running") audioCtx.resume();
}
export const audioRunning = () => !!audioCtx && audioCtx.state === "running";
const BUTTONS = ["up", "down", "left", "right", "a", "b", "start", "select"];
const SETTERS = {
  up: "_set_joyp_up", down: "_set_joyp_down", left: "_set_joyp_left", right: "_set_joyp_right",
  a: "_set_joyp_A", b: "_set_joyp_B", start: "_set_joyp_start", select: "_set_joyp_select",
};

let modulePromise = null;

export class Pocket {
  /** Start the ROM. `sram` (Uint8Array, optional) restores a previous save. */
  static async start(canvas, romBuffer, sram) {
    modulePromise ||= Binjgb();
    return new Pocket(await modulePromise, canvas, romBuffer, sram);
  }

  constructor(module, canvas, romBuffer, sram) {
    this.m = module;
    const size = (romBuffer.byteLength + 0x7fff) & ~0x7fff;
    this.romPtr = module._malloc(size);
    this.heap(this.romPtr, size).fill(0).set(new Uint8Array(romBuffer));
    this.e = module._emulator_new_simple(this.romPtr, size, audio().sampleRate, AUDIO_FRAMES, CGB_COLOR_CURVE);
    if (!this.e) throw new Error("binjgb couldn't load the ROM");
    this.joypad = module._joypad_new();
    module._emulator_set_default_joypad_callback(this.e, this.joypad);
    if (sram) this.loadSram(sram);

    this.ctx = canvas.getContext("2d");
    this.image = this.ctx.createImageData(160, 144);
    this.frame = this.heap(module._get_frame_buffer_ptr(this.e), module._get_frame_buffer_size(this.e));
    this.held = new Set();
    this.audioBuf = this.heap(module._get_audio_buffer_ptr(this.e), module._get_audio_buffer_capacity(this.e));
    this.audioStart = 0;
    this.onSramChange = null;
    this.onFrame = null;   // called before each step, e.g. to read game controllers
    this.lastSec = 0;
    this.raf = requestAnimationFrame((t) => this.step(t));
  }

  heap(ptr, size) {
    return new Uint8Array(this.m.HEAP8.buffer, ptr, size);
  }

  stop() {
    cancelAnimationFrame(this.raf);
    this.m._joypad_delete(this.joypad);
    this.m._emulator_delete(this.e);
    this.m._free(this.romPtr);
  }

  set(button, down) {
    if (!BUTTONS.includes(button)) return;
    if (down) this.held.add(button);
    else this.held.delete(button);
    this.m[SETTERS[button]](this.e, down ? 1 : 0);
  }

  releaseAll() {
    for (const b of [...this.held]) this.set(b, false);
  }

  step(ms) {
    this.raf = requestAnimationFrame((t) => this.step(t));
    if (this.onFrame) this.onFrame();
    const sec = ms / 1000;
    const delta = Math.min(Math.max(sec - (this.lastSec || sec), 0), MAX_STEP_SEC);
    this.lastSec = sec;
    const until = this.m._emulator_get_ticks_f64(this.e) + delta * CPU_TICKS_PER_SECOND;
    for (;;) {
      const event = this.m._emulator_run_until_f64(this.e, until);
      if (event & EVENT_NEW_FRAME) {
        // the wasm heap can grow and move; re-take the view if it went stale
        if (!this.frame.byteLength)
          this.frame = this.heap(this.m._get_frame_buffer_ptr(this.e), this.m._get_frame_buffer_size(this.e));
        this.image.data.set(this.frame);
        this.ctx.putImageData(this.image, 0, 0);
      }
      if (event & EVENT_AUDIO_BUFFER_FULL) this.pushAudio();
      if (event & EVENT_UNTIL_TICKS) break;
    }
    if (this.m._emulator_was_ext_ram_updated(this.e) && this.onSramChange) this.onSramChange(this.getSram());
  }

  // Queue binjgb's buffer of stereo 8-bit samples to play right after the last one (as binjgb's simple.js).
  pushAudio() {
    const ctx = audio();
    if (ctx.state !== "running") return;
    if (!this.audioBuf.byteLength)
      this.audioBuf = this.heap(this.m._get_audio_buffer_ptr(this.e), this.m._get_audio_buffer_capacity(this.e));
    const now = ctx.currentTime;
    this.audioStart = Math.max(this.audioStart, now + AUDIO_LATENCY_SEC);
    if (this.audioStart > now + 4 * AUDIO_LATENCY_SEC) this.audioStart = now + AUDIO_LATENCY_SEC;   // fell behind
    const buffer = ctx.createBuffer(2, AUDIO_FRAMES, ctx.sampleRate);
    const left = buffer.getChannelData(0), right = buffer.getChannelData(1);
    for (let i = 0; i < AUDIO_FRAMES; i++) {
      left[i] = (this.audioBuf[2 * i] * VOLUME) / 255;
      right[i] = (this.audioBuf[2 * i + 1] * VOLUME) / 255;
    }
    const src = ctx.createBufferSource();
    src.buffer = buffer;
    src.connect(ctx.destination);
    src.start(this.audioStart);
    this.audioStart += AUDIO_FRAMES / ctx.sampleRate;
  }

  withExtRam(cb) {
    const fd = this.m._ext_ram_file_data_new(this.e);
    const buf = this.heap(this.m._get_file_data_ptr(fd), this.m._get_file_data_size(fd));
    const result = cb(fd, buf);
    this.m._file_data_delete(fd);
    this.m._free(fd);
    return result;
  }

  /** The cartridge's battery RAM: the same bytes an emulator writes to a .sav file. */
  getSram() {
    return this.withExtRam((fd, buf) => {
      this.m._emulator_write_ext_ram(this.e, fd);
      return new Uint8Array(buf);
    });
  }

  loadSram(data) {
    this.withExtRam((fd, buf) => {
      buf.fill(0).set(data.subarray(0, buf.length));
      this.m._emulator_read_ext_ram(this.e, fd);
    });
  }
}
