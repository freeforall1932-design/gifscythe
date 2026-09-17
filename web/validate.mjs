// validate.mjs — JS mirror of src/core/Validate.h (audit U-30).
//
// The desktop app refuses a run whose settings are out of range; the web demo
// passed them straight to the engine, so the same input produced a dialog on one
// client and a raw gifsicle error on the other. Same rules, same field names,
// same messages, so the two clients cannot drift — `web/test/validate.test.mjs`
// cross-checks this against the real C++ `validate()` through `gifscythe-cli`.
//
// Values use the C++ conventions: -1 means "unset", delay is in 1/100 s.

/** @returns {{field:string,value:string,reason:string}[]} */
export function validate(s) {
  const w = [];
  const add = (field, value, reason) => w.push({ field, value: String(value), reason });

  const num = (field, v, dflt) => {
    if (v === undefined || v === null || v === "") return dflt;
    const n = Number(v);
    if (Number.isFinite(n)) return n;
    add(field, v, "must be a finite number (or unset)");
    return dflt;
  };

  const colors = num("colors", s.color_count, -1);
  if (colors !== -1 && (colors < 2 || colors > 256)) {
    add("colors", colors, "must be 2..256 (or unset)");
  }
  const disposal = num("disposal", s.disposal, -1);
  if (disposal !== -1 && (disposal < 0 || disposal > 7)) {
    add("disposal", disposal, "must be 0..7 (or unset)");
  }
  const opt = num("optimize", s.optimize_level, -1);
  if (opt !== -1 && (opt < 0 || opt > 3)) {
    add("optimize", opt, "must be 0..3 (or unset); 0 = off");
  }
  const lossy = num("lossy", s.lossy, -1);
  if (lossy !== -1 && (lossy < 0 || lossy > 200)) {
    add("lossy", lossy, "must be 0..200 (or unset)");
  }
  const delay = num("delay", s.delay_cs, -1);
  if (delay < -1) {
    add("delay", delay, "must be >= 0 (1/100 s units) or unset (-1)");
  }
  // threads (DS-06 / P0-2 + DS-09 / P1-31) — mirror of Validate.h. -1 says
  // nothing to the engine, 0 is a bare -j (auto), >0 is -jN; below -1 is a typo.
  const threads = num("threads", s.threads, -1);
  if (threads < -1) {
    add("threads", threads,
        "must be >= -1 (-1 = unset/default, 0 = auto, >0 = explicit thread count)");
  }
  // loopcount (U-63 / P1-40 + GS-206 / P1-28): -2 play once, -1 unset,
  // 0 forever, 1..65535 a count. The bound is measured: the bundled 1.96 turns
  // --loopcount=65536 into "loop forever" and exits 0.
  const loopcount = num("loopcount", s.loopcount, -1);
  if (loopcount !== -1 && loopcount !== -2 && (loopcount < 0 || loopcount > 65535)) {
    add("loopcount", loopcount,
        "must be -2 (play once), -1 (unset), or 0..65535 (0 = forever); larger values wrap: the engine turns 65536 into forever and exits 0");
  }
  // Method-name enums (GS-206 / P1-28), against the lists the engine registers
  // in gifsicle.c:1470-1508 / quantize.c:1421+.
  if (s.color_method) {
    const cm = String(s.color_method);
    if (cm !== "diversity" && cm !== "blend-diversity" && cm !== "median-cut") {
      add("color_method", cm,
          "must be diversity, blend-diversity or median-cut (the engine refuses anything else)");
    }
  }
  if (s.resize_method) {
    const kResize = ["point", "sample", "mix", "box", "catrom", "lanczos", "lanczos2",
                     "lanczos3", "mitchell", "fast", "good"];
    if (!kResize.includes(String(s.resize_method))) {
      add("resize_method", s.resize_method,
          "not one of point, sample, mix, box, catrom, lanczos, lanczos2, lanczos3, mitchell, fast, good");
    }
  }
  // dither_method is deliberately NOT enum-checked (the engine grammar carries
  // parameters: o8, "o,4", ro64x64 — quantize.c:1421 onwards), exactly as in
  // Validate.h. A blank value never reaches the settings model either: the
  // loader trims it, so a rule for it would be dead on the C++ side and would
  // break the parity this file's test enforces.
  // gamma_str: shape only — the engine accepts srgb, oklab and any finite
  // number, and refuses everything else WITHOUT failing (rc=0), so a bad name
  // is silent. Non-finite is rejected here and in Validate.h for the same
  // reason: Number("inf") is NaN while std::stod("inf") parses.
  if (s.gamma_str && s.gamma_str !== "srgb" && s.gamma_str !== "oklab") {
    const g = Number(String(s.gamma_str).trim());
    if (!Number.isFinite(g)) {
      add("gamma", s.gamma_str,
          "must be srgb, oklab or a number (anything else: the engine prints a gamma error and exits 0)");
    }
  }

  const mode = s.mode || "auto";
  if (s.info && (mode === "batch" || mode === "merge" || mode === "explode")) {
    add("info", "true", "--info cannot be combined with mode options (-m/-b/-e)");
  }
  // N-05 (S11) — mirror of Validate.h: one shared prefix cannot carry several
  // inputs; the engine explodes all but the LAST into the CWD and exits 0.
  if (mode === "explode" && s.inputs && s.inputs.length > 1) {
    add("mode", "explode", "explode with multiple inputs scatters frames: only the LAST input honors the -o prefix, earlier inputs write <basename>.NNN into the CWD (engine exits 0) - run one file at a time");
  }
  // Crop 0x0 is legal engine syntax: width/height 0 means extend to the edge
  // (audit U-62). Negative spans are still a model limitation in the shared
  // settings object rather than a validation refusal here.

  // Resize geometry — the same rules added to Validate.h for audit U-22, each
  // derived by probing the bundled gifsicle 1.96:
  //   --resize-fit 0x0 / --resize 0x0 / --resize-touch 0x0 -> rc=1
  //   --resize-width 0 / --resize-height 0                -> rc=1
  //   --scale 0x0 -> rc=1, but --scale 0x1 -> rc=0 and DOES NOTHING
  const rw = num("resize_w", s.resize_w, 0);
  const rh = num("resize_h", s.resize_h, 0);
  switch (s.resize_kind) {
    case "fit":
    case "touch":
    case "exact":
      if (rw === 0 && rh === 0) {
        add("resize", "0x0", "one of width and height must be > 0 (the engine refuses 0x0)");
      }
      break;
    case "width":
      if (rw === 0) add("resize_w", 0, "width must be > 0 (the engine refuses --resize-width 0)");
      break;
    case "height":
      if (rh === 0) add("resize_h", 0, "height must be > 0 (the engine refuses --resize-height 0)");
      break;
    case "scale": {
      const sx = num("scale_x", s.scale_x, 1);
      const sy = num("scale_y", s.scale_y, 1);
      if (!(sx > 0) || !(sy > 0)) {
        // toFixed(6) matches std::to_string(double) (%f) so the strings compare.
        add("scale", `${Number(sx).toFixed(6)}x${Number(sy).toFixed(6)}`,
            "both scale factors must be > 0; 0 makes the engine skip the resize silently");
      }
      break;
    }
    default: break;
  }

  if (!(s.inputs && s.inputs.length)) {
    add("input", "", "at least one input file is required");
  }
  return w;
}
