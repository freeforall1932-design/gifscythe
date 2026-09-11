// app.js — Gifscythe web UI. Uses the SAME command builder (command.mjs) as
// the Node server and the desktop C++ control layer, so the live pane shows
// exactly what the engine runs.

import { buildArgs, shellQuote } from "./command.mjs";

const $ = (id) => document.getElementById(id);

let currentFile = null;
let afterUrl = null;
// U-52: the Before preview's object URL was created on every setFile() and never
// revoked, so repeatedly choosing large files kept every one of them reachable
// until page unload. Track it and release the previous one.
let beforeUrl = null;
// U-46: choosing a new file (or new settings) used to leave the in-flight fetch
// running; when it landed it populated After, the download link and the savings
// readout for a file that was no longer selected. A generation counter makes a
// stale completion a no-op.
let requestGen = 0;

// ---- settings model (mirrors gs::Settings) ----
function settings() {
  const resize = $("resize").value;
  const loop = $("loop").value;
  return {
    mode: "auto",
    optimize_level: Number($("optimize").value),
    lossy: Number($("lossy").value) > 0 ? Number($("lossy").value) : -1,
    color_count: $("colorsOn").checked ? Number($("colors").value) : -1,
    dither: $("dither").value !== "" && $("dither").value !== "none",
    dither_method:
      $("dither").value === "default" ? "" : $("dither").value,
    resize_kind: resize,
    resize_w: Number($("w").value),
    resize_h: Number($("h").value),
    // Audit U-42: independent X/Y scale, mirroring the desktop's scale_x /
    // scale_y controls. The single shared "Scale %" input silently forced
    // both axes to the same factor — a parity gap the desktop never had.
    scale_x: Number($("scalePctX").value) / 100,
    scale_y: Number($("scalePctY").value) / 100,
    loopcount: loop === "keep" ? -1 : loop === "forever" ? 0 : Number($("loopN").value),
    delay_cs: $("delayOn").checked ? Number($("delay").value) : -1,
  };
}

// ---- live pane ----
function refreshCommand() {
  const pre = $("command");
  if (!currentFile) {
    pre.textContent = "add a GIF to generate a command…";
    return;
  }
  const s = settings();
  s.inputs = [currentFile.name];
  s.output = currentFile.name.replace(/\.gif$/i, "") + "_opt.gif";
  pre.textContent = "gifsicle " + buildArgs(s).map(shellQuote).join(" ");
}

// ---- wiring ----
const pick = $("pick");
const file = $("file");
pick.addEventListener("click", () => file.click());
file.addEventListener("change", () => {
  if (file.files && file.files[0]) setFile(file.files[0]);
});

const drop = $("drop");
drop.addEventListener("dragover", (e) => { e.preventDefault(); drop.classList.add("over"); });
drop.addEventListener("dragleave", () => drop.classList.remove("over"));
drop.addEventListener("drop", (e) => {
  e.preventDefault();
  drop.classList.remove("over");
  const f = [...(e.dataTransfer?.files || [])].find((x) => /\.gif$/i.test(x.name) || x.type === "image/gif");
  if (f) setFile(f);
});

function setFile(f) {
  currentFile = f;
  requestGen += 1;                 // U-46: invalidate anything already in flight
  if (afterUrl) { URL.revokeObjectURL(afterUrl); afterUrl = null; }
  if (beforeUrl) { URL.revokeObjectURL(beforeUrl); beforeUrl = null; }  // U-52
  beforeUrl = URL.createObjectURL(f);
  $("before").src = beforeUrl;
  $("after").removeAttribute("src");
  $("after").alt = "—";
  $("savings").textContent = "";
  $("run").disabled = !f;
  $("download").hidden = true;
  $("status").textContent = `${f.name} — ${humanSize(f.size)} ready.`;
  refreshCommand();
}

function humanSize(bytes) {
  if (bytes >= 1048576) return (bytes / 1048576).toFixed(2) + " MB";
  if (bytes >= 1024) return (bytes / 1024).toFixed(1) + " KB";
  return bytes + " B";
}

// controls -> live pane
for (const id of ["optimize", "lossy", "colors", "colorsOn", "dither", "resize",
  "w", "h", "scalePctX", "scalePctY", "loop", "loopN", "delay", "delayOn"]) {
  $(id).addEventListener("input", refreshCommand);
  $(id).addEventListener("change", () => { updateControlState(); refreshCommand(); });
}

function updateControlState() {
  $("colors").disabled = !$("colorsOn").checked;
  $("delay").disabled = !$("delayOn").checked;
  $("loopNRow").hidden = $("loop").value !== "n";
}

// ---- optimize ----
$("run").addEventListener("click", async () => {
  if (!currentFile) return;
  const run = $("run");
  run.disabled = true;
  $("status").textContent = "Optimizing…";
  const s = settings();
  const gen = requestGen;               // U-46: which selection this run belongs to
  try {
    const url = "/optimize?settings=" + encodeURIComponent(JSON.stringify(s));
    const resp = await fetch(url, { method: "POST", body: currentFile });
    if (gen !== requestGen) return;     // a newer file was chosen; drop this one
    if (!resp.ok) {
      const err = await resp.json().catch(() => ({ error: "HTTP " + resp.status }));
      $("status").textContent =
        `Failed — exit ${err.exitCode ?? ""} ${err.stderr || err.error || ""}`.trim();
      run.disabled = false;
      return;
    }
    const blob = await resp.blob();
    if (gen !== requestGen) return;     // ...and again after the body is read
    if (afterUrl) URL.revokeObjectURL(afterUrl);
    afterUrl = URL.createObjectURL(blob);
    $("after").src = afterUrl;

    const inBytes = Number(resp.headers.get("X-Gifscythe-In-Bytes") || currentFile.size);
    const outBytes = Number(resp.headers.get("X-Gifscythe-Out-Bytes") || blob.size);
    const pct = ((outBytes - inBytes) / inBytes) * 100;
    $("savings").textContent =
      `${humanSize(inBytes)} → ${humanSize(outBytes)}  ` +
      `(${pct >= 0 ? "+" : ""}${pct.toFixed(1)}%)`;
    // Header is percent-encoded by the server so it stays ASCII-safe (U-50).
    const rawCmd = resp.headers.get("X-Gifscythe-Command");
    if (rawCmd) {
      try { $("command").textContent = decodeURIComponent(rawCmd); }
      catch { $("command").textContent = rawCmd; }
    }

    $("download").href = afterUrl;
    $("download").hidden = false;
    $("status").textContent = "Done.";
  } catch (e) {
    if (gen !== requestGen) return;     // U-46: do not report a stale failure
    $("status").textContent = "Failed — " + (e && e.message ? e.message : e);
  } finally {
    if (gen === requestGen) run.disabled = false;
  }
});

updateControlState();
