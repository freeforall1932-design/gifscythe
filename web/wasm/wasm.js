// wasm.js — Gifscythe experimental wasm UI (one screen, one file per run).
//
// The SAME command builder (../command.mjs) and validator (../validate.mjs)
// as the Node web build and the desktop C++ control layer — imported
// verbatim, so the live pane shows exactly what the engine runs. The only
// difference from the server path is transport: instead of POSTing to Node,
// this page writes the input into the Emscripten virtual FS, calls the wasm
// gifsicle module SYNCHRONOUSLY on the main thread (no Worker in v1), and
// reads the output back out of the virtual FS.
//
// Display names vs virtual-FS names: the live pane shows the real file names
// (in.gif -> stem_opt.gif, desktop parity); the actual callMain uses fixed
// virtual paths (/in.gif -> /out.gif) so user file names never become argv.

import { buildArgs, shellQuote } from "../command.mjs";
import { validate } from "../validate.mjs";

const $ = (id) => document.getElementById(id);

let currentFile = null; // single File (no queue in v1)
let beforeUrl = null;
let afterUrl = null;
let enginePromise = null; // createGifsicle() singleton (one module, many runs)

const stemOf = (name) => {
  const i = name.lastIndexOf(".");
  return i > 0 ? name.slice(0, i) : name;
};

function humanSize(bytes) {
  if (bytes >= 1048576) return (bytes / 1048576).toFixed(2) + " MB";
  if (bytes >= 1024) return (bytes / 1024).toFixed(1) + " KB";
  return bytes + " B";
}

// ---- settings model (mirrors gs::Settings; v1 subset: 6 groups) ----
function settings() {
  const resize = $("resize").value;
  const loop = $("loop").value;
  return {
    mode: "auto",
    explode_by_name: false,
    optimize_level: Number($("optimize").value),
    lossy: Number($("lossy").value) > 0 ? Number($("lossy").value) : -1,
    color_count: $("colorsOn").checked ? Number($("colors").value) : -1,
    resize_kind: resize,
    resize_w: Number($("w").value),
    resize_h: Number($("h").value),
    scale_x: Number($("scalePctX").value) / 100,
    scale_y: Number($("scalePctY").value) / 100,
    loopcount: loop === "keep" ? -1 : loop === "forever" ? 0 : Number($("loopN").value),
    delay_cs: $("delayOn").checked ? Number($("delay").value) : -1,
  };
}

// ---- live pane: what the engine will run (display names) ----
function refreshCommand() {
  const pre = $("command");
  if (!currentFile) {
    pre.textContent = "choose a GIF to generate a command…";
    return;
  }
  const s = settings();
  const a = {
    ...s,
    mode: "auto",
    inputs: [currentFile.name],
    output: stemOf(currentFile.name) + "_opt.gif",
  };
  pre.textContent = "gifsicle " + buildArgs(a).map(shellQuote).join(" ");
}

// ---- engine (lazy singleton; the dist/ script tag provides the factory) ----
// Engine stdout/stderr of the current run (reset before each callMain;
// quoted in the status line when a run fails).
let engineLog = [];

async function ensureEngine() {
  if (!enginePromise) {
    enginePromise = (async () => {
      if (typeof window.createGifsicle !== "function") {
        throw new Error(
          "engine script not loaded — run web/wasm/build_wasm.sh (needs emcc) so dist/gifsicle.js exists, then reload."
        );
      }
      return window.createGifsicle({
        print: (t) => engineLog.push(t),
        printErr: (t) => engineLog.push(t),
      });
    })();
  }
  return enginePromise;
}

function setStatus(text) {
  $("status").textContent = text;
}

function revoke(url) {
  if (url) URL.revokeObjectURL(url);
}

function onFileChanged() {
  revoke(beforeUrl);
  revoke(afterUrl);
  beforeUrl = null;
  afterUrl = null;
  const f = $("file").files && $("file").files[0];
  currentFile = f || null;
  $("fileName").textContent = f ? `${f.name} (${humanSize(f.size)})` : "no file selected (one file per run)";
  $("before").src = "";
  $("before").alt = f ? f.name : "no file selected";
  $("after").removeAttribute("src");
  $("after").alt = "—";
  $("savings").textContent = "";
  $("outputs").hidden = true;
  $("outputList").textContent = "";
  if (f) {
    beforeUrl = URL.createObjectURL(f);
    $("before").src = beforeUrl;
  }
  $("run").disabled = !f;
  refreshCommand();
  if (f) setStatus("ready.");
}

async function run() {
  if (!currentFile) return;
  const s = settings();
  // The desktop refuses a run whose settings are out of range; with no
  // server in this track, the page enforces the same rules itself.
  const issues = validate({ ...s, inputs: [currentFile.name] });
  if (issues.length) {
    setStatus(
      "refusing to run: " + issues.map((w) => `${w.field} (${w.value}): ${w.reason}`).join("; ")
    );
    return;
  }
  setStatus("optimizing…");
  $("run").disabled = true;
  try {
    const M = await ensureEngine();
    const inputBytes = new Uint8Array(await currentFile.arrayBuffer());
    M.FS.writeFile("/in.gif", inputBytes);
    // Same argv the live pane shows, except the fixed virtual-FS paths.
    const args = buildArgs({ ...s, mode: "auto", inputs: ["/in.gif"], output: "/out.gif" });
    let code = 0;
    engineLog = [];
    try {
      M.callMain(args);
    } catch (e) {
      if (e && typeof e.status === "number") {
        code = e.status;
      } else {
        throw e;
      }
    }
    if (code !== 0) {
      const detail = engineLog.length ? ": " + engineLog.join(" | ") : "";
      setStatus(`engine failed (exit ${code})${detail} — no output written.`);
      return;
    }
    let outBytes;
    try {
      outBytes = M.FS.readFile("/out.gif");
    } catch (e) {
      setStatus("engine exited 0 but produced no output — refusing to claim success.");
      return;
    }
    const magic = String.fromCharCode(...outBytes.slice(0, 6));
    if (outBytes.length === 0 || (magic !== "GIF87a" && magic !== "GIF89a")) {
      setStatus("engine output is not a GIF — refusing to claim success.");
      return;
    }
    const blob = new Blob([outBytes], { type: "image/gif" });
    revoke(afterUrl);
    afterUrl = URL.createObjectURL(blob);
    $("after").src = afterUrl;
    $("after").alt = "optimized result";
    const inSize = inputBytes.length;
    const outSize = outBytes.length;
    const pct = inSize > 0 ? Math.round((1 - outSize / inSize) * 100) : 0;
    $("savings").textContent =
      `${humanSize(inSize)} → ${humanSize(outSize)} (${pct >= 0 ? "-" + pct + "%" : "+" + -pct + "%"})`;
    const outName = stemOf(currentFile.name) + "_opt.gif";
    const ul = $("outputList");
    ul.textContent = "";
    const li = document.createElement("li");
    const a = document.createElement("a");
    a.href = afterUrl;
    a.download = outName;
    a.textContent = `Download ${outName} (${humanSize(outSize)})`;
    li.appendChild(a);
    ul.appendChild(li);
    $("outputs").hidden = false;
    setStatus("done.");
  } catch (e) {
    setStatus(`failed: ${(e && e.message) || e}`);
  } finally {
    $("run").disabled = !currentFile;
  }
}

function wire(id, event, fn) {
  $(id).addEventListener(event, fn);
}

// ---- wiring ----
wire("pick", "click", () => $("file").click());
wire("file", "change", onFileChanged);
wire("run", "click", run);
for (const id of [
  "optimize",
  "lossy",
  "colorsOn",
  "colors",
  "resize",
  "w",
  "h",
  "scalePctX",
  "scalePctY",
  "loop",
  "loopN",
  "delayOn",
  "delay",
]) {
  wire(id, "change", refreshCommand);
  wire(id, "input", refreshCommand);
}
wire("colorsOn", "change", () => {
  $("colors").disabled = !$("colorsOn").checked;
});
wire("delayOn", "change", () => {
  $("delay").disabled = !$("delayOn").checked;
});
wire("loop", "change", () => {
  $("loopNRow").hidden = $("loop").value !== "n";
});

// Preload the engine at startup so the first Run is synchronous.
ensureEngine().then(
  () => {
    if (!currentFile) setStatus("engine ready — choose a GIF.");
  },
  (e) => {
    setStatus(`engine unavailable: ${(e && e.message) || e}`);
  }
);
