// app.js — Gifscythe web UI. Uses the SAME command builder (command.mjs) as
// the Node server and the desktop C++ control layer, so the live pane shows
// exactly what the engine runs.
//
// U-41 (fix-order P2-11): all four desktop modes — Auto (single file), Batch
// (per-file Auto runs with derived <stem>_opt.gif targets), Merge (one -m run
// over every queued file) and Explode (frames as <stem>_frame.NNN, or names
// with -E) — go through the JSON multi-file endpoint POST /run. The demo
// derives output names itself; the desktop's Save-as / batch folder / name
// template controls stay desktop-only (documented in web/README.md).

import { buildArgs, shellQuote } from "./command.mjs";

const $ = (id) => document.getElementById(id);

// Queued inputs (File objects), in queue order = merge order (desktop parity).
let currentFiles = [];
// Object-URL hygiene (audit U-52): every URL this page creates is tracked and
// revoked when it is replaced. One Before URL (first queued file) + one per
// rendered result.
let beforeUrl = null;
let resultUrls = [];
// Request ownership (audit U-46): a generation counter makes a stale
// completion a no-op when the queue changed while a run was in flight.
let requestGen = 0;

// QFileInfo::completeBaseName parity with the server (<stem>_opt.gif naming).
const stemOf = (name) => {
  const i = name.lastIndexOf(".");
  return i > 0 ? name.slice(0, i) : name;
};

// ---- settings model (mirrors gs::Settings) ----
function settings() {
  const resize = $("resize").value;
  const loop = $("loop").value;
  return {
    mode: $("mode").value,
    explode_by_name: $("explodeByName").checked,
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

function humanSize(bytes) {
  if (bytes >= 1048576) return (bytes / 1048576).toFixed(2) + " MB";
  if (bytes >= 1024) return (bytes / 1024).toFixed(1) + " KB";
  return bytes + " B";
}

// ---- live pane: one line per engine run, mirroring what /run will do ----
function refreshCommand() {
  const pre = $("command");
  if (!currentFiles.length) {
    pre.textContent = "add a GIF to generate a command…";
    return;
  }
  const s = settings();
  const names = currentFiles.map((f) => f.name);
  const lines = [];
  if (s.mode === "batch") {
    // The desktop batch runs one Auto command PER FILE (never a single -b
    // line); show exactly those commands.
    for (const name of names) {
      const one = { ...s, mode: "auto", inputs: [name], output: stemOf(name) + "_opt.gif" };
      lines.push("gifsicle " + buildArgs(one).map(shellQuote).join(" "));
    }
  } else if (s.mode === "merge") {
    const m = { ...s, inputs: names, output: "merged.gif" };
    lines.push("gifsicle " + buildArgs(m).map(shellQuote).join(" "));
  } else if (s.mode === "explode") {
    const e = { ...s, inputs: [names[0]], output: stemOf(names[0]) + "_frame" };
    lines.push("gifsicle " + buildArgs(e).map(shellQuote).join(" "));
    if (names.length > 1) lines.push("# Explode processes one file per run — remove the others or use Batch/Merge.");
  } else {
    const a = { ...s, mode: "auto", inputs: [names[0]], output: stemOf(names[0]) + "_opt.gif" };
    lines.push("gifsicle " + buildArgs(a).map(shellQuote).join(" "));
    if (names.length > 1) lines.push("# Auto processes one file per run — remove the others or use Batch/Merge.");
  }
  pre.textContent = lines.join("\n");
}

// ---- queue rendering ----
function renderFileList() {
  const ul = $("fileList");
  ul.textContent = "";
  ul.hidden = currentFiles.length === 0;
  currentFiles.forEach((f, i) => {
    const li = document.createElement("li");
    const name = document.createElement("span");
    name.className = "name";
    name.textContent = f.name;
    const size = document.createElement("span");
    size.className = "size";
    size.textContent = humanSize(f.size);
    const rm = document.createElement("button");
    rm.textContent = "×";
    rm.title = "remove from queue";
    rm.addEventListener("click", () => {
      currentFiles.splice(i, 1);
      onQueueChanged();
    });
    li.append(name, size, rm);
    ul.appendChild(li);
  });
}

function revokeResults() {
  for (const u of resultUrls) URL.revokeObjectURL(u);
  resultUrls = [];
}

function onQueueChanged() {
  requestGen += 1;               // U-46: anything in flight is now stale
  revokeResults();
  $("outputs").hidden = true;
  $("outputList").textContent = "";
  $("after").removeAttribute("src");
  $("after").alt = "—";
  $("savings").textContent = "";
  renderFileList();
  // Before preview = first queued file (U-52: revoke the previous URL).
  if (beforeUrl) { URL.revokeObjectURL(beforeUrl); beforeUrl = null; }
  if (currentFiles.length) {
    beforeUrl = URL.createObjectURL(currentFiles[0]);
    $("before").src = beforeUrl;
    $("status").textContent =
      `${currentFiles.length} file(s) queued — ${humanSize(
        currentFiles.reduce((n, f) => n + f.size, 0))} total.`;
  } else {
    $("before").removeAttribute("src");
    $("status").textContent = "";
  }
  $("run").disabled = currentFiles.length === 0;
  refreshCommand();
}

// ---- wiring ----
const pick = $("pick");
const file = $("file");
pick.addEventListener("click", () => file.click());
file.addEventListener("change", () => {
  addFiles([...(file.files || [])]);
  file.value = "";   // allow re-picking the same file after a removal
});

const drop = $("drop");
drop.addEventListener("dragover", (e) => { e.preventDefault(); drop.classList.add("over"); });
drop.addEventListener("dragleave", () => drop.classList.remove("over"));
drop.addEventListener("drop", (e) => {
  e.preventDefault();
  drop.classList.remove("over");
  // Desktop drop-filter parity (audit U-13): .gif suffix, real files only.
  addFiles([...(e.dataTransfer?.files || [])]
    .filter((x) => /\.gif$/i.test(x.name) || x.type === "image/gif"));
});

function addFiles(list) {
  let added = 0;
  for (const f of list) {
    // Dedupe by name (the web equivalent of the desktop's dedupe by path).
    if (currentFiles.some((x) => x.name === f.name)) continue;
    currentFiles.push(f);
    added += 1;
  }
  if (added) onQueueChanged();
}

// controls -> live pane
for (const id of ["mode", "explodeByName", "optimize", "lossy", "colors", "colorsOn",
  "dither", "resize", "w", "h", "scalePctX", "scalePctY", "loop", "loopN", "delay", "delayOn"]) {
  $(id).addEventListener("input", refreshCommand);
  $(id).addEventListener("change", () => { updateControlState(); refreshCommand(); });
}

function updateControlState() {
  $("colors").disabled = !$("colorsOn").checked;
  $("delay").disabled = !$("delayOn").checked;
  $("loopNRow").hidden = $("loop").value !== "n";
  $("explodeByNameRow").hidden = $("mode").value !== "explode";
}

// ---- run (POST /run: JSON in, JSON out; no header-encoding traps — U-50) ----
function bufToBase64(buf) {
  const bytes = new Uint8Array(buf);
  let bin = "";
  const CHUNK = 0x8000;
  for (let i = 0; i < bytes.length; i += CHUNK) {
    bin += String.fromCharCode.apply(null, bytes.subarray(i, i + CHUNK));
  }
  return btoa(bin);
}

function base64ToBlob(b64) {
  const bin = atob(b64);
  const bytes = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i += 1) bytes[i] = bin.charCodeAt(i);
  return new Blob([bytes], { type: "image/gif" });
}

$("run").addEventListener("click", async () => {
  if (!currentFiles.length) return;
  const run = $("run");
  run.disabled = true;
  const s = settings();
  $("status").textContent = `Running (${s.mode})…`;
  const gen = requestGen;               // U-46: which queue this run belongs to
  try {
    const payload = {
      settings: s,
      files: [],
    };
    for (const f of currentFiles) {
      payload.files.push({ name: f.name, data: bufToBase64(await f.arrayBuffer()) });
    }
    if (gen !== requestGen) return;     // queue changed while reading files
    const resp = await fetch("/run", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });
    if (gen !== requestGen) return;     // ...and again after the response
    const body = await resp.json().catch(() => null);
    if (gen !== requestGen) return;
    if (!resp.ok || !body || !body.ok) {
      const issues = body && Array.isArray(body.issues) && body.issues.length
        ? body.issues.map((i) => `${i.field}=${i.value}: ${i.reason}`).join("; ")
        : "";
      $("status").textContent =
        `Failed — ${body ? (issues || body.error || body.stderr || ("HTTP " + resp.status)) : ("HTTP " + resp.status)}`;
      run.disabled = false;
      return;
    }
    // Render every output: a download link + size per result; the After pane
    // shows the first one. All object URLs are tracked for revocation (U-52).
    revokeResults();
    const ul = $("outputList");
    ul.textContent = "";
    for (const o of body.outputs) {
      const blob = base64ToBlob(o.data);
      const u = URL.createObjectURL(blob);
      resultUrls.push(u);
      const li = document.createElement("li");
      const a = document.createElement("a");
      a.href = u;
      a.download = o.name;
      a.textContent = o.name;
      const size = document.createElement("span");
      size.className = "size";
      size.textContent = humanSize(o.bytes);
      li.append(a, size);
      ul.appendChild(li);
    }
    $("outputs").hidden = false;
    if (resultUrls.length) $("after").src = resultUrls[0];

    const pct = body.inBytes
      ? ((body.outBytes - body.inBytes) / body.inBytes) * 100 : null;
    $("savings").textContent =
      `${body.outputs.length} result(s) · ${humanSize(body.inBytes)} → ${humanSize(body.outBytes)}`
      + (pct === null ? "" : `  (${pct >= 0 ? "+" : ""}${pct.toFixed(1)}%)`);
    if (Array.isArray(body.commands) && body.commands.length) {
      $("command").textContent = body.commands.join("\n");
    }
    $("status").textContent = `Done (${body.mode}).`;
  } catch (e) {
    if (gen !== requestGen) return;     // U-46: do not report a stale failure
    $("status").textContent = "Failed — " + (e && e.message ? e.message : e);
  } finally {
    if (gen === requestGen) run.disabled = false;
  }
});

updateControlState();
