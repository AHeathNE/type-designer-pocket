// Where the game's save lives on the website:
//  - always in this browser (localStorage), so a visit picks up where you left off
//  - and, in browsers with the File System Access API (Chrome, Edge), in a real .sav file on your computer that
//    the game writes to as you play. The file's handle is kept in IndexedDB so it can be reconnected later.

const STORE_KEY = "tdp.sram";

export function storedSram() {
  try {
    const b64 = localStorage.getItem(STORE_KEY);
    if (!b64) return null;
    return Uint8Array.from(atob(b64), (c) => c.charCodeAt(0));
  } catch {
    return null;
  }
}

export function storeSram(data) {
  try {
    let s = "";
    for (const b of data) s += String.fromCharCode(b);
    localStorage.setItem(STORE_KEY, btoa(s));
  } catch {
    /* private window or storage blocked: the game still runs, it just won't be remembered */
  }
}

// ---- a save file on the computer ----

export const canUseFiles = typeof window !== "undefined" && "showSaveFilePicker" in window;
const PICKER_TYPES = [{ description: "Game Boy save", accept: { "application/octet-stream": [".sav", ".srm"] } }];
const DB = "tdp", DB_STORE = "files", DB_KEY = "save";

function db() {
  return new Promise((resolve, reject) => {
    const req = indexedDB.open(DB, 1);
    req.onupgradeneeded = () => req.result.createObjectStore(DB_STORE);
    req.onsuccess = () => resolve(req.result);
    req.onerror = () => reject(req.error);
  });
}

async function dbDo(mode, fn) {
  const d = await db();
  return new Promise((resolve, reject) => {
    const tx = d.transaction(DB_STORE, mode);
    const req = fn(tx.objectStore(DB_STORE));
    tx.oncomplete = () => resolve(req && req.result);
    tx.onerror = () => reject(tx.error);
  });
}

export class SaveFile {
  constructor(handle) {
    this.handle = handle;
    this.name = handle.name;
    this.queue = Promise.resolve();   // writes run one after another
  }

  /** Ask the visitor for an existing .sav / .srm. Throws AbortError if they cancel. */
  static async open() {
    const [handle] = await window.showOpenFilePicker({ types: PICKER_TYPES, multiple: false });
    return new SaveFile(handle);
  }

  /** Ask the visitor where to create a new .sav. Throws AbortError if they cancel. */
  static async create() {
    return new SaveFile(await window.showSaveFilePicker({ suggestedName: "type-designer-pocket.sav", types: PICKER_TYPES }));
  }

  /** The file linked on an earlier visit, or null. */
  static async remembered() {
    try {
      const handle = await dbDo("readonly", (s) => s.get(DB_KEY));
      return handle ? new SaveFile(handle) : null;
    } catch {
      return null;
    }
  }

  async remember() {
    try {
      await dbDo("readwrite", (s) => s.put(this.handle, DB_KEY));
    } catch {
      /* storage blocked: it just won't be remembered next visit */
    }
  }

  static async forget() {
    try {
      await dbDo("readwrite", (s) => s.delete(DB_KEY));
    } catch {
      /* nothing to forget */
    }
  }

  /** Read/write permission. ask=true may show the browser's prompt, so only call it from a click. */
  async permitted(ask) {
    const opts = { mode: "readwrite" };
    if (!this.handle.queryPermission) return true;   // a handle that needs no permission (OPFS)
    if ((await this.handle.queryPermission(opts)) === "granted") return true;
    return ask && (await this.handle.requestPermission(opts)) === "granted";
  }

  async read() {
    return new Uint8Array(await (await this.handle.getFile()).arrayBuffer());
  }

  /** Replace the file's contents. Writes are queued, so two quick saves can't interleave. */
  write(bytes) {
    const copy = new Uint8Array(bytes);
    this.queue = this.queue.catch(() => {}).then(async () => {
      const w = await this.handle.createWritable();
      await w.write(copy);
      await w.close();
    });
    return this.queue;
  }
}
