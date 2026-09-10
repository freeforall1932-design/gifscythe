// validate.test.mjs — parity check for the web validation layer (audit U-30).
//
// web/validate.mjs claims to mirror src/core/Validate.h. That claim is only
// worth anything if it is checked, so for each fixture this test:
//   1. serializes the JS settings object to a .conf with saveSettingsLines
//      (the same writer the live pane uses),
//   2. runs the real C++ `gifscythe-cli <conf>` in print mode and collects the
//      `WARNING: field=value: reason` lines it emits from gs::validate(),
//   3. calls the JS validate() on the same object,
//   4. requires the (field, value, reason) triples to match EXACTLY — same set,
//      same order, same wording.
//
// Run:  node web/test/validate.test.mjs   (after ./build.sh)

import { spawnSync } from "node:child_process";
import { mkdtempSync, writeFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { saveSettingsLines } from "../command.mjs";
import { validate } from "../validate.mjs";

const __dirname = dirname(fileURLToPath(import.meta.url));
const PRODUCT = join(__dirname, "..", "..", "working_code", "gifscythe");
const CLI = join(PRODUCT, "build", "gifscythe-cli");

const IN = "/tmp/parity/in.gif";

// Each `s` is a complete settings object. `expect` is the triple list the C++
// side is required to produce too; the test fails if either side disagrees.
const fixtures = [
  { name: "clean settings -> no warnings either side",
    s: { mode: "auto", optimize_level: 2, colors: 128, disposal: 1, lossy: 40,
         delay_cs: 5, inputs: [IN], output: "/tmp/parity/out.gif" } },
  { name: "colors out of range",
    s: { mode: "auto", color_count: 900, inputs: [IN] } },
  { name: "disposal out of range",
    s: { mode: "auto", disposal: 12, inputs: [IN] } },
  { name: "optimize out of range",
    s: { mode: "auto", optimize_level: 9, inputs: [IN] } },
  { name: "lossy out of range",
    s: { mode: "auto", lossy: 500, inputs: [IN] } },
  // delay<0 and lossy<0 are normalized to "unset" by BOTH conf writers
  // (SettingsIO.h:321,334 and command.mjs:203,214) because -1 means "unset".
  // The C++ READER accepts them from a hand-edited conf (need_long has no lower
  // bound) and validates them, so drive the C++ side from raw conf text there.
  { name: "delay below -1 (hand-written conf; writer normalizes this away)",
    s: { mode: "auto", delay_cs: -4, inputs: [IN] },
    conf: `mode = auto\ndelay = -4\ninput = ${IN}\n` },
  { name: "info conflicts with batch mode",
    s: { mode: "batch", info: true, inputs: [IN] } },
  { name: "info conflicts with merge mode",
    s: { mode: "merge", info: true, inputs: [IN] } },
  { name: "crop with zero height",
    s: { mode: "auto", crop: true, crop_x: 0, crop_y: 0, crop_w: 30, crop_h: 0,
         inputs: [IN] } },
  { name: "resize-fit 0x0 (audit U-22)",
    s: { mode: "auto", resize_kind: "fit", resize_w: 0, resize_h: 0, inputs: [IN] } },
  { name: "resize-touch 0x0",
    s: { mode: "auto", resize_kind: "touch", resize_w: 0, resize_h: 0, inputs: [IN] } },
  { name: "resize exact 0x0",
    s: { mode: "auto", resize_kind: "exact", resize_w: 0, resize_h: 0, inputs: [IN] } },
  { name: "resize-fit with only a height is FINE",
    s: { mode: "auto", resize_kind: "fit", resize_w: 0, resize_h: 200, inputs: [IN] } },
  { name: "resize-width 0",
    s: { mode: "auto", resize_kind: "width", resize_w: 0, inputs: [IN] } },
  { name: "resize-height 0",
    s: { mode: "auto", resize_kind: "height", resize_h: 0, inputs: [IN] } },
  { name: "scale 0x0",
    s: { mode: "auto", resize_kind: "scale", scale_x: 0, scale_y: 0, inputs: [IN] } },
  { name: "scale 0x1 - engine exits 0 but does nothing",
    s: { mode: "auto", resize_kind: "scale", scale_x: 0, scale_y: 1, inputs: [IN] } },
  { name: "several problems at once",
    s: { mode: "merge", info: true, optimize_level: 7, lossy: -3,
         color_count: 1, inputs: [IN] },
    conf: `mode = merge\ninfo = true\noptimize = 7\nlossy = -3\ncolors = 1\ninput = ${IN}\n` },
  { name: "no input at all",
    s: { mode: "auto", inputs: [] } },
];

let failures = 0;
const dir = mkdtempSync(join(tmpdir(), "gsvalidate-"));
try {
  for (const f of fixtures) {
    const conf = join(dir, f.name.replace(/[^A-Za-z0-9]+/g, "_") + ".conf");
    writeFileSync(conf, f.conf || saveSettingsLines(f.s));
    // spawnSync, not execFileSync: print mode exits 0, and execFileSync only
    // RETURNS stdout — piped stderr is thrown away on success, so the C++ side
    // silently looked like it emitted nothing (found by running this test).
    const r = spawnSync(CLI, [conf], { encoding: "utf8", stdio: ["ignore", "ignore", "pipe"] });
    if (r.error) throw r.error;
    const stderr = r.stderr || "";
    const cpp = [...stderr.matchAll(/^WARNING: ([^=]*)=(.*?): (.*)$/gm)]
      .map((m) => `${m[1]}=${m[2]}: ${m[3]}`);
    const js = validate(f.s).map((i) => `${i.field}=${i.value}: ${i.reason}`);

    const same = cpp.length === js.length && cpp.every((line, i) => line === js[i]);
    if (!same) {
      failures++;
      console.log(`FAIL ${f.name}`);
      console.log(`  C++: ${JSON.stringify(cpp)}`);
      console.log(`  JS : ${JSON.stringify(js)}`);
    } else {
      console.log(`PASS ${f.name} (${js.length} warning${js.length === 1 ? "" : "s"})`);
    }
  }
} finally {
  rmSync(dir, { recursive: true, force: true });
}

if (failures === 0) {
  console.log("ALL WEB VALIDATION TESTS PASSED");
  process.exit(0);
}
console.log(`${failures} WEB VALIDATION TEST(S) FAILED`);
process.exit(1);
