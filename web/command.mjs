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
  if (s.color_count >= 2 && s.color_count <= 256) {
    add("-k"); add(i2s(s.color_count));
  }
  // Dither: prefer explicit method string; fall back to bare -f when bool set.
  if (s.dither_method) {
    if (s.dither_method !== "none") add("--dither=" + s.dither_method);
  } else if (s.dither) {
    add("-f");
  }
  if (s.lossy >= 0 && s.lossy <= 200) {
    add("--lossy=" + i2s(s.lossy));
  }
  // Gamma: string form preferred (srgb|oklab|NUM); legacy double fallback.
  if (s.gamma_str) add("--gamma=" + s.gamma_str);
  else if (s.gamma >= 0) add("--gamma=" + fmtDouble(s.gamma));
  if (s.color_method) { add("--color-method"); add(s.color_method); }

  // Resize / scale
  switch (s.resize_kind) {
    case "fit":
      add("--resize-fit"); add(u2s(s.resize_w) + "x" + u2s(s.resize_h)); break;
    case "touch":
      add("--resize-touch"); add(u2s(s.resize_w) + "x" + u2s(s.resize_h)); break;
    case "exact":
      add("--resize"); add(u2s(s.resize_w) + "x" + u2s(s.resize_h)); break;
    case "scale":
      add("--scale"); add(fmtDouble(s.scale_x) + "x" + fmtDouble(s.scale_y)); break;
    case "width":
      add("--resize-width"); add(u2s(s.resize_w)); break;
    case "height":
      add("--resize-height"); add(u2s(s.resize_h)); break;
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
  for (const c of s.comments || []) { add("--comment"); add(c); }

  // Animation options (delay_cs is in 1/100 s, NOT milliseconds).
  if (s.delay_cs >= 0) { add("-d"); add(i2s(s.delay_cs)); }
  if (s.disposal >= 0 && s.disposal <= 7) { add("--disposal"); add(i2s(s.disposal)); }
  if (s.loopcount === 0) {
    add("--loopcount=0");           // forever
  } else if (s.loopcount > 0) {
    add("--loopcount=" + i2s(s.loopcount));
  }
  if (s.optimize_level >= 0 && s.optimize_level <= 3) {
    add(optimizationOpt(s.optimize_level));
  }
  if (s.unoptimize) add("-U");
  if (s.threads > 0) add("-j" + i2s(s.threads));

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
  if (s.loopcount >= 0) out.push("loopcount = " + s.loopcount);
  if (s.optimize_level >= 0) out.push("optimize = " + s.optimize_level);
  if (s.unoptimize) out.push("unoptimize = true");
  if (s.threads > 0) out.push("threads = " + s.threads);
  if (s.color_count >= 0) out.push("colors = " + s.color_count);
  if (s.dither_method) out.push("dither = " + s.dither_method);
  else if (s.dither) out.push("dither = true");
  if (s.lossy >= 0) out.push("lossy = " + s.lossy);
  if (s.gamma_str) out.push("gamma = " + s.gamma_str);
  else if (s.gamma >= 0) out.push("gamma = " + s.gamma);
  if (s.color_method) out.push("color_method = " + s.color_method);
  if (s.careful) out.push("careful = true");
  const resizeName = { fit: "fit", touch: "touch", exact: "exact", scale: "scale", width: "width", height: "height" };
  if (resizeName[s.resize_kind]) out.push("resize_kind = " + resizeName[s.resize_kind]);
  if (s.resize_w) out.push("resize_w = " + s.resize_w);
  if (s.resize_h) out.push("resize_h = " + s.resize_h);
  if (s.resize_kind === "scale") {
    out.push("scale_x = " + s.scale_x);
    out.push("scale_y = " + s.scale_y);
  }
  if (s.resize_method) out.push("resize_method = " + s.resize_method);
  if (s.background) out.push("background = " + s.background);
  if (s.transparent) out.push("transparent = " + s.transparent);
  if (s.remove_comments) out.push("remove_comments = true");
  if (s.remove_names) out.push("remove_names = true");
  if (s.remove_extensions) out.push("remove_extensions = true");
  for (const c of s.comments || []) out.push("comment = " + c);
  for (const input of s.inputs || []) out.push("input = " + input);
  if (s.output) out.push("output = " + s.output);
  if (s.explode_by_name) out.push("explode_by_name = true");
  return out.join("\n") + "\n";
}
