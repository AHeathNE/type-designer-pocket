// Run by tests/test_web.py: the website's controller mapping, with a fake controller and no browser.
globalThis.window = { addEventListener() {} };
globalThis.localStorage = { store: {}, getItem(k) { return this.store[k] ?? null; }, setItem(k, v) { this.store[k] = String(v); } };
const pad = { id: "Fake (Vendor: 1 Product: 2)", mapping: "standard", axes: [0, 0, 0, 0], buttons: Array.from({ length: 17 }, () => ({ pressed: false })) };
Object.defineProperty(globalThis, "navigator", { value: { getGamepads: () => [pad, null] }, configurable: true });
const { Gamepads } = await import("../web/js/gamepad.js");

const events = [];
const pocket = { set: (b, down) => events.push(`${b}${down ? "+" : "-"}`) };
const pads = new Gamepads();
const press = (i) => { pad.buttons[i].pressed = true; pads.poll(pocket); pad.buttons[i].pressed = false; pads.poll(pocket); };
const out = {};
press(1); press(0); press(9); press(8); press(12); press(15);
out.standard = events.splice(0);
pad.axes[0] = -0.9; pads.poll(pocket); pad.axes[0] = 0.2; pads.poll(pocket);
out.stick = events.splice(0);
pads.setSwap(true);
press(1); press(0);
out.swapped = events.splice(0);
out.remembered = new Gamepads().swap;
out.name = pads.name();
console.log(JSON.stringify(out));
