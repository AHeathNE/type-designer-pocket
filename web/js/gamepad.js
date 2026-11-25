// Game controllers (USB or Bluetooth, once paired with the computer) through the browser's Gamepad API.
// Buttons follow the Game Boy's layout by position: the right face button is A, the bottom (or left) one is B.
// Controllers labelled the other way round can swap A and B; the choice is remembered in this browser.

const SWAP_KEY = "tdp.swapAB";
const DEAD = 0.5;   // how far a stick has to move to count as a d-pad press

// standard mapping (https://w3c.github.io/gamepad/#remapping): 0 bottom, 1 right, 2 left, 3 top,
// 8 back/select, 9 start, 12-15 d-pad up, down, left, right
function standardButtons(pad, swap) {
  const on = (i) => !!(pad.buttons[i] && pad.buttons[i].pressed);
  const a = on(1) || on(3), b = on(0) || on(2);
  return {
    a: swap ? b : a,
    b: swap ? a : b,
    select: on(8),
    start: on(9),
    up: on(12),
    down: on(13),
    left: on(14),
    right: on(15),
  };
}

export class Gamepads {
  /** onChange(name or "") is called when a controller connects or disconnects. */
  constructor(onChange = () => {}) {
    this.down = {};
    this.onChange = onChange;
    try {
      this.swap = localStorage.getItem(SWAP_KEY) === "1";
    } catch {
      this.swap = false;
    }
    window.addEventListener("gamepadconnected", () => this.onChange(this.name()));
    window.addEventListener("gamepaddisconnected", () => this.onChange(this.name()));
  }

  pads() {
    return [...(navigator.getGamepads ? navigator.getGamepads() : [])].filter(Boolean);
  }

  /** The first connected controller's name, or "". */
  name() {
    const pad = this.pads()[0];
    return pad ? pad.id.replace(/\s*\(.*?Vendor.*?\)\s*/i, "").trim() || "Controller" : "";
  }

  setSwap(swap) {
    this.swap = swap;
    try {
      localStorage.setItem(SWAP_KEY, swap ? "1" : "0");
    } catch {
      /* not remembered */
    }
  }

  /** Read every controller and send changes to the game. Call once per animation frame. */
  poll(pocket) {
    const now = { up: false, down: false, left: false, right: false, a: false, b: false, start: false, select: false };
    for (const pad of this.pads()) {
      const s = standardButtons(pad, this.swap);
      for (const k in now) now[k] ||= s[k];
      const x = pad.axes[0] || 0, y = pad.axes[1] || 0;   // left stick
      now.left ||= x < -DEAD;
      now.right ||= x > DEAD;
      now.up ||= y < -DEAD;
      now.down ||= y > DEAD;
      if (pad.mapping !== "standard" && pad.axes.length >= 8) {   // many Linux drivers: hat switch on axes 6/7
        now.left ||= pad.axes[6] < -DEAD;
        now.right ||= pad.axes[6] > DEAD;
        now.up ||= pad.axes[7] < -DEAD;
        now.down ||= pad.axes[7] > DEAD;
      }
    }
    for (const k in now) {
      if (now[k] !== !!this.down[k]) {
        this.down[k] = now[k];
        pocket.set(k, now[k]);
      }
    }
  }
}
