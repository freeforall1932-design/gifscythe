// Glue harness: executes web/wasm/wasm.js in Node with a stub DOM + a fake
// engine module that shells out to the REAL native gifsicle.
// Proves: settings()->buildArgs argv is engine-valid, validate() integration,
// FS write/callMain/read flow, GIF magic check, savings + download rendering,
// and the invalid-settings refusal path.
// This tests the JS glue, NOT the wasm binary: no emcc output is involved,
// so it runs anywhere node does. Requirements: node >= 18, a built engine
// (run working_code/gifscythe/build.sh first), the logo.gif upstream fixture.
// Exit 0 + GLUE-HARNESS: PASS on success, 1 on failure, 2 on missing engine.
import { readFileSync, writeFileSync, mkdtempSync, existsSync } from "node:fs";
import { execFileSync } from "node:child_process";
import os from "node:os";
import path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ENGINE = path.resolve(HERE, "../../working_code/gifscythe/release/0.1.0/gifsicle");
const LOGO = path.resolve(HERE, "../../reference_code/gifsicle/logo.gif");

if (!existsSync(ENGINE)) {
  console.error(`glue_harness: engine not found at ${ENGINE}`);
  console.error("glue_harness: run working_code/gifscythe/build.sh first.");
  process.exit(2);
}
function el(id, init = {}) {
  return { id, listeners: {}, value: "", checked: false, disabled: false,
    hidden: false, textContent: "", children: [],
    addEventListener(t, fn) { (this.listeners[t] ||= []).push(fn); },
    appendChild(c) { this.children.push(c); return c; },
    append(...cs) { this.children.push(...cs); },
    removeAttribute() {}, click() {}, ...init };
}
const defaults = { optimize: "3", lossy: "0", colorsOn: false, colors: "128",
  resize: "none", w: "320", h: "240", scalePctX: "100", scalePctY: "100",
  loop: "keep", loopN: "3", delayOn: false, delay: "10" };
const els = {};
globalThis.document = {
  getElementById: (id) => (els[id] ||= el(id,
    id in defaults ? (typeof defaults[id] === "boolean" ? { checked: defaults[id] } : { value: defaults[id] }) : {})),
  createElement: (tag) => el(tag),
};
const vfs = {};
globalThis.window = {
  createGifsicle: async () => ({
    FS: { writeFile: (p, b) => (vfs[p] = Buffer.from(b)),
          readFile: (p) => { if (!vfs[p]) throw new Error("ENOENT " + p); return vfs[p]; } },
    callMain: (args) => {
      const dir = mkdtempSync(path.join(os.tmpdir(), "glue-"));
      const inp = path.join(dir, "in.gif"), out = path.join(dir, "out.gif");
      writeFileSync(inp, vfs["/in.gif"]);
      const mapped = args.map((a) => (a === "/in.gif" ? inp : a === "/out.gif" ? out : a));
      console.log("callMain argv: gifsicle " + mapped.join(" "));
      try { execFileSync(ENGINE, mapped, { stdio: ["ignore", "pipe", "pipe"] }); }
      catch (e) { const err = new Error("exit"); err.status = e.status ?? 1; throw err; }
      vfs["/out.gif"] = readFileSync(out);
    },
  }),
};
globalThis.URL.createObjectURL = () => "blob:stub";
globalThis.URL.revokeObjectURL = () => {};
globalThis.Blob = class Blob { constructor(parts) { this.parts = parts; } };
const logoBytes = readFileSync(LOGO);
const fakeFile = { name: "logo.gif", size: logoBytes.length,
  arrayBuffer: async () => logoBytes.buffer.slice(logoBytes.byteOffset, logoBytes.byteOffset + logoBytes.byteLength) };
await import(pathToFileURL(path.join(HERE, "wasm.js")).href);
const $ = (id) => els[id];
// Eager wiring only: pick/file/run are wired at import; command and the rest
// are touched lazily by handlers, so checking them here is a false failure.
if (!$("pick") || !$("file") || !$("run")) throw new Error("wiring missed elements");
// default settings, single file
$("file").files = [fakeFile];
for (const fn of ($("file").listeners.change || [])) fn();
if (!$("command")) throw new Error("change handler did not render the command pane");
console.log("live pane: " + $("command").textContent);
console.log("run disabled after select: " + $("run").disabled);
for (const fn of ($("run").listeners.click || [])) await fn();
const doneOk = $("status").textContent === "done."; // capture: the refuse run below overwrites status
console.log("status: " + $("status").textContent);
console.log("savings: " + $("savings").textContent);
console.log("result links: " + $("outputList").children.length);
// out-of-range settings must refuse (validate.mjs integration)
$("lossy").value = "999";
for (const fn of ($("run").listeners.click || [])) await fn();
console.log("refuse status: " + $("status").textContent);
const ok = doneOk
  && $("outputList").children.length === 1
  && /\(\d+ B\)|KB/.test($("savings").textContent)
  && $("status").textContent.startsWith("refusing to run:");
console.log(ok ? "GLUE-HARNESS: PASS" : "GLUE-HARNESS: FAIL");
process.exit(ok ? 0 : 1);
