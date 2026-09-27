// command.mjs — JavaScript mirror of the C++ GifsicleCommand engine layer.
//
// This is the *single* command builder used by the web UI (live pane) and the
// Node server (argv execution). It is a line-by-line port of
// working_code/gifscythe/src/core/GifsicleCommand.h so the web build emits the
// exact same gifsicle argv as the desktop app. web/test/command.test.mjs
// cross-checks it against the real C++ `gifscythe-cli` output.
//
// Settings object mirrors gs::Settings field names.

export function shellQuote(a) {
  if (a === '') return "''";
  let safe = true;
  for (const ch of a) {
    const c = ch.codePointAt(0);
    const ok =
      (c >= 48 && c <= 57) ||           // 0-9
      (c >= 65 && c <= 90) ||           // A-Z
      (c >= 97 && c <= 122) ||          // a-z
      "/._-+=:@%,".includes(ch);
    if (!ok) { safe = false; break; }
  }
  if (safe) return a;
  let out = "'";
  for (const c of a) {
    if (c === "'") out += "'\\''";
    else out += c;
  }
  out += "'";
  return out;
}

function fmtDouble(x) {
  if (Number.isInteger(x)) return String(x);
  // Mirror C++ ostringstream precision(10): up to 10 significant digits,
  // then normalize (parseFloat drops trailing zeros / exponent noise).
  const s = parseFloat(x.toPrecision(10));
  return String(s);
}

const u2s = (x) => String(x);
const i2s = (x) => String(x);

// Form controls use an empty string for an unset number. Keep that distinct
// from an explicit zero, and never let a non-finite value become an argv token.
export function numOrNull(value) {
  if (value === undefined || value === null || value === "") return null;
  const n = Number(value);
  return Number.isFinite(n) ? n : null;
}

// One coercion point for every numeric setting, shared with web/validate.mjs.
//
// A JSON client may legitimately send "5" where the desktop sends the number 5:
// the conf file is text, Validate.h and validate.mjs both accept it (Number("5")
// is finite), and the settings serializer accepts it. buildArgs used to require
// `typeof value === "number"`, so such a value validated clean, was then
// silently dropped from argv, and the run still reported success — the repo's
// worst failure class (false success) reached through the validator/builder
// seam. Every numeric branch below reads its value through numArg instead, and
// emits the coerced NUMBER, never the raw JSON token.
export function numArg(value) {
  if (value === undefined || value === null || value === "") return null;
  const n = Number(value);
  return Number.isFinite(n) ? n : null;
}

function optimizationOpt(level) {
  // -O0 is valid ("no optimization"); -O without value = 1.
  if (level === 1) return "-O";
  return "-O" + String(level);
}

// Build the argv vector (without the program name). Mirrors
// GifsicleCommand::build() exactly.
export function buildArgs(s) {
  const args = [];
  const add = (x) => { if (x !== "" && x != null) args.push(x); };

  // Mode (must come before filenames).
  switch (s.mode) {
    case "merge": add("-m"); break;
    case "batch": add("-b"); break;
    case "explode": add(s.explode_by_name ? "-E" : "-e"); break;
    case "auto": default: break;
  }

  // General
  if (s.info) add("--info");

  // Whole-GIF
  if (s.careful) add("--careful");
  const colorCount = numArg(s.color_count);
  if (colorCount !== null && colorCount >= 2 && colorCount <= 256) {
    add("-k"); add(i2s(colorCount));
  }
  // Dither: prefer explicit method string; fall back to bare -f when bool set.
  if (s.dither_method) {
    if (s.dither_method !== "none") add("--dither=" + s.dither_method);
  } else if (s.dither) {
    add("-f");
  }
  const lossy = numArg(s.lossy);
  if (lossy !== null && lossy >= 0 && lossy <= 200) {
    add("--lossy=" + i2s(lossy));
  }
  // Gamma: string form preferred (srgb|oklab|NUM); legacy double fallback.
  // numArg, not a bare `>= 0`: an EMPTY gamma satisfied `"" >= 0` and then
  // crashed fmtDouble ("x.toPrecision is not a function") — an uncaught
  // TypeError in the request path, not a silently wrong image.
  if (s.gamma_str) add("--gamma=" + s.gamma_str);
  else {
    const gamma = numArg(s.gamma);
    if (gamma !== null && gamma >= 0) add("--gamma=" + fmtDouble(gamma));
  }
  if (s.color_method) { add("--color-method"); add(s.color_method); }

  // Resize / scale — every dimension read through numArg (same seam as above).
  const rw = numArg(s.resize_w);
  const rh = numArg(s.resize_h);
  const sx = numArg(s.scale_x);
  const sy = numArg(s.scale_y);
  switch (s.resize_kind) {
    case "fit":
      if (rw !== null && rh !== null) {
        add("--resize-fit"); add(u2s(rw) + "x" + u2s(rh));
      }
      break;
    case "touch":
      if (rw !== null && rh !== null) {
        add("--resize-touch"); add(u2s(rw) + "x" + u2s(rh));
      }
      break;
    case "exact":
      if (rw !== null && rh !== null) {
        add("--resize"); add(u2s(rw) + "x" + u2s(rh));
      }
      break;
    case "scale":
      if (sx !== null && sy !== null) {
        add("--scale"); add(fmtDouble(sx) + "x" + fmtDouble(sy));
      }
      break;
    case "width":
      if (rw !== null) { add("--resize-width"); add(u2s(rw)); }
      break;
    case "height":
      if (rh !== null) { add("--resize-height"); add(u2s(rh)); }
      break;
    case "none": default: break;
  }
  if (s.resize_method) { add("--resize-method"); add(s.resize_method); }

  // Frame / image options
  if (s.interlace) add("-i");
  if (s.flip_horizontal) add("--flip-horizontal");
  if (s.flip_vertical) add("--flip-vertical");
  switch (s.rotation) {
    case "r90": add("--rotate-90"); break;
    case "r180": add("--rotate-180"); break;
    case "r270": add("--rotate-270"); break;
    case "none": default: break;
  }
  if (s.has_position) {
    add("-p"); add(u2s(s.position_x) + "," + u2s(s.position_y));
  }

  // Crop — gifsicle wants X,Y+WIDTHxHEIGHT (plus form).
  if (s.crop) {
    add("--crop");
    add(u2s(s.crop_x) + "," + u2s(s.crop_y) + "+" + u2s(s.crop_w) + "x" + u2s(s.crop_h));
    if (s.crop_transparency) add("--crop-transparency");
  }

  // Background / transparency
  if (s.background) { add("--background"); add(s.background); }
  if (s.transparent) { add("--transparent"); add(s.transparent); }

  // Comments / names / extensions
  if (s.remove_comments) add("--no-comments");
  if (s.remove_names) add("--no-names");
  if (s.remove_extensions) add("--no-extensions");
  // Mirror GifsicleCommand.h: an empty comment would emit `--comment` with no
  // operand and swallow the next argument.
  for (const c of s.comments || []) {
    if (!c) continue;
    add("--comment"); add(c);
  }

  // Animation options (delay_cs is in 1/100 s, NOT milliseconds).
  const delayCs = numArg(s.delay_cs);
  if (delayCs !== null && delayCs >= 0) { add("-d"); add(i2s(delayCs)); }
  const disposal = numArg(s.disposal);
  if (disposal !== null && disposal >= 0 && disposal <= 7) { add("--disposal"); add(i2s(disposal)); }
  // Must mirror GifsicleCommand.h (the parity test enforces it). Four states:
  // -2 play once (--no-loopcount), -1 unchanged, 0 forever, >0 a count.
  // numArg, not `===`: a JSON "-2" is a string, so BOTH strict comparisons used
  // to miss and a "play once" request emitted no loop flag at all.
  const loopcount = numArg(s.loopcount);
  if (loopcount === -2) {
    add("--no-loopcount");          // play once = the loop extension absent
  } else if (loopcount === 0) {
    add("--loopcount=0");           // forever
  } else if (loopcount !== null && loopcount > 0) {
    add("--loopcount=" + i2s(loopcount));
  }
  const optimizeLevel = numArg(s.optimize_level);
  if (optimizeLevel !== null && optimizeLevel >= 0 && optimizeLevel <= 3) {
    add(optimizationOpt(optimizeLevel));
  }
  if (s.unoptimize) add("-U");
  // Must mirror GifsicleCommand.h exactly (the parity test enforces it).
  // Tri-state (DS-06 / P0-2): -1 (or anything below) says NOTHING, which is the
  // engine's single-threaded default; 0 is a bare -j = "auto"
  // (GIFSICLE_DEFAULT_THREAD_COUNT = 8); >0 is -jN. Emitting bare -j for -1
  // used to make the "unset" sentinel mean 8 threads.
  // numArg, not `=== 0`: a JSON "0" is a string, so the auto-threads state
  // (-j with no count) used to be silently dropped. -1/unset still says nothing.
  const threads = numArg(s.threads);
  if (threads !== null && threads > 0) add("-j" + i2s(threads));
  else if (threads === 0) add("-j");

  // Inputs
  for (const input of s.inputs || []) add(input);

  // Output: -o FILE
  if (s.output) { add("-o"); add(s.output); }

  return args;
}

// Shell-quoted command line string for display (mirrors toString()).
export function toString(s) {
  return buildArgs(s).map(shellQuote).join(" ");
}

// Same keys the C++ save_settings() writes; used by tests to hand settings
// from JS to the C++ CLI for the parity check.
// Mirror SettingsIO.h encode_line_value(): a value containing a newline would
// otherwise be written as a second line, and therefore parsed as a new KEY on
// reload (audit U-51 — a single comment "hi\nmode = merge" used to change the
// run mode). CR/LF fold to a space so the file format stays unchanged.
// DS-12 / P1-13 mirror of encode_line_value(): fold newlines (U-51) and quote
// ONLY when the value would otherwise be lossy — leading/trailing whitespace, or
// a leading quote. web/command.mjs is the writer the parity test feeds to the
// real C++ CLI, so the two encoders must agree byte for byte.
const lineValue = (v) => {
  const flat = String(v).replace(/[\r\n]+/g, " ");
  const needs = flat.length > 0
    && (flat.startsWith('"') || /^\s/.test(flat) || /\s$/.test(flat));
  if (!needs) return flat;
  return '"' + flat.replace(/["\\]/g, "\\$&") + '"';
};

export function saveSettingsLines(s) {
  const out = [];
  const b = (v) => (v ? "true" : "false");
  const modeName = { merge: "merge", batch: "batch", explode: "explode", auto: "auto" };
  out.push("mode = " + (modeName[s.mode] || "auto"));
  if (s.info) out.push("info = true");
  if (s.interlace) out.push("interlace = true");
  if (s.flip_horizontal) out.push("flip_horizontal = true");
  if (s.flip_vertical) out.push("flip_vertical = true");
  const rotName = { r90: "90", r180: "180", r270: "270" };
  if (rotName[s.rotation]) out.push("rotation = " + rotName[s.rotation]);
  if (s.has_position) {
    out.push("position_x = " + s.position_x);
    out.push("position_y = " + s.position_y);
  }
  if (s.crop) {
    out.push("crop = true");
    out.push("crop_x = " + s.crop_x);
    out.push("crop_y = " + s.crop_y);
    out.push("crop_w = " + s.crop_w);
    out.push("crop_h = " + s.crop_h);
    if (s.crop_transparency) out.push("crop_transparency = true");
  }
  if (s.delay_cs >= 0) out.push("delay = " + s.delay_cs);
  if (s.disposal >= 0) out.push("disposal = " + s.disposal);
  // -2 (play once) must survive a round trip, so the guard is != unset
  // (SettingsIO.h does the same).
  if (s.loopcount !== -1 && s.loopcount !== undefined && s.loopcount !== null) out.push("loopcount = " + s.loopcount);
  if (s.optimize_level >= 0) out.push("optimize = " + s.optimize_level);
  if (s.unoptimize) out.push("unoptimize = true");
  // 0 ("Auto") is serialised explicitly so it survives a round trip, matching
  // SettingsIO.h — and since P0-2 it is NOT the same as leaving the key out:
  // 0 means bare -j (8 threads), absence means the engine's own default.
  if (s.threads >= 0) out.push("threads = " + s.threads);
  if (s.color_count >= 0) out.push("colors = " + s.color_count);
  if (s.dither_method) out.push("dither = " + lineValue(s.dither_method));
  else if (s.dither) out.push("dither = true");
  if (s.lossy >= 0) out.push("lossy = " + s.lossy);
  if (s.gamma_str) out.push("gamma = " + lineValue(s.gamma_str));
  else {
    const gammaOut = numArg(s.gamma);
    if (gammaOut !== null && gammaOut >= 0) out.push("gamma = " + gammaOut);
  }
  if (s.color_method) out.push("color_method = " + lineValue(s.color_method));
  if (s.careful) out.push("careful = true");
  const resizeName = { fit: "fit", touch: "touch", exact: "exact", scale: "scale", width: "width", height: "height" };
  if (resizeName[s.resize_kind]) out.push("resize_kind = " + resizeName[s.resize_kind]);
  if (s.resize_w) out.push("resize_w = " + s.resize_w);
  if (s.resize_h) out.push("resize_h = " + s.resize_h);
  if (s.resize_kind === "scale") {
    out.push("scale_x = " + s.scale_x);
    out.push("scale_y = " + s.scale_y);
  }
  if (s.resize_method) out.push("resize_method = " + lineValue(s.resize_method));
  if (s.background) out.push("background = " + lineValue(s.background));
  if (s.transparent) out.push("transparent = " + lineValue(s.transparent));
  if (s.remove_comments) out.push("remove_comments = true");
  if (s.remove_names) out.push("remove_names = true");
  if (s.remove_extensions) out.push("remove_extensions = true");
  for (const c of s.comments || []) out.push("comment = " + lineValue(c));
  for (const input of s.inputs || []) out.push("input = " + lineValue(input));
  if (s.output) out.push("output = " + lineValue(s.output));
  if (s.explode_by_name) out.push("explode_by_name = true");
  return out.join("\n") + "\n";
}
