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
  // U-41: the web UI can now select Explode, so the third arm of the
  // info-vs-mode rule (Validate.h:37 / validate.mjs) needs its own parity pin.
  { name: "info conflicts with explode mode (audit U-41)",
    s: { mode: "explode", info: true, inputs: [IN] } },
  // N-05 (S11): multi-input explode scatters frames (engine exits 0) — the
  // warning text must be byte-identical on both sides.
  { name: "explode with two inputs scatters frames (N-05)",
    s: { mode: "explode", inputs: [IN, IN + "2"] } },
  { name: "explode multi-input warning coexists with the info conflict",
    s: { mode: "explode", info: true, inputs: [IN, IN + "2"] } },
  { name: "explode by name still validates resize geometry",
    s: { mode: "explode", explode_by_name: true, resize_kind: "fit",
         resize_w: 0, resize_h: 0, inputs: [IN] } },
  { name: "crop 0x0 is allowed (audit U-62)",
    s: { mode: "auto", crop: true, crop_x: 2, crop_y: 2, crop_w: 0, crop_h: 0,
         inputs: [IN] },
    expect: [] },
  { name: "crop with zero height",
    s: { mode: "auto", crop: true, crop_x: 0, crop_y: 0, crop_w: 30, crop_h: 0,
         inputs: [IN] }, expect: [] },
  // threads < -1 is reachable only via a hand-written conf because both writers
  // normalize negative values away except -1 = unset.
  { name: "threads below -1 warns (audit DS-09)",
    s: { mode: "auto", threads: -7, inputs: [IN] },
    conf: `mode = auto\nthreads = -7\ninput = ${IN}\n`,
    expect: ["threads=-7: must be >= -1 (-1 = unset/default, 0 = auto, >0 = explicit thread count)"] },
  // --- S23: the multi-state sentinels and the GS-206 domains -------------
  { name: "threads unset (-1) is legal: no flag, no warning (P0-2)",
    s: { mode: "auto", threads: -1, inputs: [IN] }, expect: [] },
  { name: "loopcount -2 (play once) is legal (U-63)",
    s: { mode: "auto", loopcount: -2, inputs: [IN] }, expect: [] },
  { name: "loopcount above 65535 warns — the engine wraps it silently (P1-28)",
    s: { mode: "auto", loopcount: 65536, inputs: [IN] },
    expect: ["loopcount=65536: must be -2 (play once), -1 (unset), or 0..65535 (0 = forever); larger values wrap: the engine turns 65536 into forever and exits 0"] },
  { name: "unknown color_method warns (engine refuses it)",
    s: { mode: "auto", color_method: "kdtree", inputs: [IN] },
    expect: ["color_method=kdtree: must be diversity, blend-diversity or median-cut (the engine refuses anything else)"] },
  { name: "unknown resize_method warns (engine refuses it)",
    s: { mode: "auto", resize_method: "bicubic", inputs: [IN] },
    expect: ["resize_method=bicubic: not one of point, sample, mix, box, catrom, lanczos, lanczos2, lanczos3, mitchell, fast, good"] },
  { name: "non-numeric gamma warns; srgb/oklab/numbers do not (P1-28)",
    s: { mode: "auto", gamma_str: "banana", inputs: [IN] },
    expect: ["gamma=banana: must be srgb, oklab or a number (anything else: the engine prints a gamma error and exits 0)"] },
  { name: "named gamma oklab is accepted", s: { mode: "auto", gamma_str: "oklab", inputs: [IN] }, expect: [] },
  { name: "dither grammar is NOT enum-validated on purpose (o,4 / ro64)",
    s: { mode: "auto", dither_method: "o,4", inputs: [IN] }, expect: [] },
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
    // A parity check alone passes on two empty lists, so a fixture that expects
    // a specific warning says so explicitly. Without this, "neither side warns"
    // and "the rule is missing on both sides" are indistinguishable — which is
    // how a dead gate stayed dead for five PRs (see the R2 review rule).
    const expected = f.expect || null;
    const matchesExpected = !expected
      || (cpp.length === expected.length
        && js.length === expected.length
        && cpp.every((line, i) => line === expected[i])
        && js.every((line, i) => line === expected[i]));
    if (!same || !matchesExpected) {
      failures++;
      console.log(`FAIL ${f.name}`);
      console.log(`  C++: ${JSON.stringify(cpp)}`);
      console.log(`  JS : ${JSON.stringify(js)}`);
      if (expected) console.log(`  EXP: ${JSON.stringify(expected)}`);
    } else {
      console.log(`PASS ${f.name} (${js.length} warning${js.length === 1 ? "" : "s"})`);
    }
  }
} finally {
  rmSync(dir, { recursive: true, force: true });
}

// ---------------------------------------------------------------------------
// U-78 (P1-44): the wrong-TYPE class, pinned against the real CLI.
//
// A non-numeric value has no Validate.h counterpart, so it cannot be a parity
// row in the loop above: the C++ side catches it in the PARSER (SettingsIO.h
// need_int/need_long -> "not an integer", counted in the `parse=N` half of
// WARNING-SUMMARY and refused by --strict with rc=3), while the web has no
// parse layer at all — JSON hands validate() the raw type, so validate()'s
// finite-number gate IS the mirror. The two wordings differ by design; what
// must not differ is the outcome: garbage in -> a named refusal on both
// surfaces, never a silent success. Before P1-44 the web answered 200 ok:true
// with the setting quietly missing from argv (U-78's executed S24 probe).
// Every expectation below was measured against the built CLI first: parse
// warning present, `parse=1`, `--strict` rc=3, for all seven integer keys.
// The HTTP half of this contract (422 + named issue) is pinned end-to-end in
// transport.test.mjs; the empty-vs-zero JS contract is in numeric-honesty.
// ---------------------------------------------------------------------------
const typeGate = [
  // [conf key = the field name both surfaces report, settings-object key]
  ["colors", "color_count"],
  ["optimize", "optimize_level"],
  ["lossy", "lossy"],
  ["delay", "delay_cs"],
  ["threads", "threads"],
  ["loopcount", "loopcount"],
  ["disposal", "disposal"],
];
const typeDir = mkdtempSync(join(tmpdir(), "gstypegate-"));
try {
  for (const [confKey, settingKey] of typeGate) {
    const conf = join(typeDir, confKey + ".conf");
    writeFileSync(conf, `mode = auto\n${confKey} = abc\ninput = ${IN}\n`);
    const printed = spawnSync(CLI, [conf],
      { encoding: "utf8", stdio: ["ignore", "ignore", "pipe"] });
    if (printed.error) throw printed.error;
    const stderr = printed.stderr || "";
    const strict = spawnSync(CLI, [conf, "--strict"],
      { encoding: "utf8", stdio: ["ignore", "ignore", "pipe"] });
    const issues = validate({ mode: "auto", [settingKey]: "abc", inputs: [IN] });

    const problems = [];
    if (!stderr.includes(`WARNING: settings key '${confKey}' value 'abc': not an integer`)) {
      problems.push(`C++ parser did not name the wrong-type value (stderr: ${JSON.stringify(stderr)})`);
    }
    if (!/WARNING-SUMMARY: parse=1\b/.test(stderr)) {
      problems.push("C++ WARNING-SUMMARY does not report parse=1");
    }
    if (strict.status !== 3) {
      problems.push(`C++ --strict exited ${strict.status}, expected 3 (refuse on any warning)`);
    }
    if (!issues.some((i) => i.field === confKey && /finite number/.test(i.reason))) {
      problems.push(`JS validate() did not refuse it (got ${JSON.stringify(issues)})`);
    }
    if (issues.length !== 1) {
      problems.push(`JS validate() returned ${issues.length} issues, expected exactly 1`);
    }
    if (problems.length) {
      failures++;
      console.log(`FAIL U-78 wrong-type ${confKey}\n       ${problems.join("\n       ")}`);
    } else {
      console.log(`PASS U-78 wrong-type ${confKey} refused on both surfaces (C++ parse+strict rc=3, JS 422 issue)`);
    }
  }
} finally {
  rmSync(typeDir, { recursive: true, force: true });
}

if (failures === 0) {
  console.log("ALL WEB VALIDATION TESTS PASSED");
  process.exit(0);
}
console.log(`${failures} WEB VALIDATION TEST(S) FAILED`);
process.exit(1);
