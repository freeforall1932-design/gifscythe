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

  const num = (v, dflt) => (v === undefined || v === null || v === "" ? dflt : Number(v));

  const colors = num(s.color_count, -1);
  if (colors !== -1 && (colors < 2 || colors > 256)) {
    add("colors", colors, "must be 2..256 (or unset)");
  }
  const disposal = num(s.disposal, -1);
  if (disposal !== -1 && (disposal < 0 || disposal > 7)) {
    add("disposal", disposal, "must be 0..7 (or unset)");
  }
  const opt = num(s.optimize_level, -1);
  if (opt !== -1 && (opt < 0 || opt > 3)) {
    add("optimize", opt, "must be 0..3 (or unset); 0 = off");
  }
  const lossy = num(s.lossy, -1);
  if (lossy !== -1 && (lossy < 0 || lossy > 200)) {
    add("lossy", lossy, "must be 0..200 (or unset)");
  }
  const delay = num(s.delay_cs, -1);
  if (delay < -1) {
    add("delay", delay, "must be >= 0 (1/100 s units) or unset (-1)");
  }
  const mode = s.mode || "auto";
  if (s.info && (mode === "batch" || mode === "merge" || mode === "explode")) {
    add("info", "true", "--info cannot be combined with mode options (-m/-b/-e)");
  }
  if (s.crop && (!num(s.crop_w, 0) || !num(s.crop_h, 0))) {
    add("crop", "0x0", "crop width/height must be > 0");
  }

  // Resize geometry — the same rules added to Validate.h for audit U-22, each
  // derived by probing the bundled gifsicle 1.96:
  //   --resize-fit 0x0 / --resize 0x0 / --resize-touch 0x0 -> rc=1
  //   --resize-width 0 / --resize-height 0                -> rc=1
  //   --scale 0x0 -> rc=1, but --scale 0x1 -> rc=0 and DOES NOTHING
  const rw = num(s.resize_w, 0);
  const rh = num(s.resize_h, 0);
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
      const sx = num(s.scale_x, 1);
      const sy = num(s.scale_y, 1);
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
