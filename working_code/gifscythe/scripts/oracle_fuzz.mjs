#!/usr/bin/env node
// Deterministic, offline conformance sample for the JS/C++/gifsicle settings path.
// Usage: node scripts/oracle_fuzz.mjs [--quick | --full | --write]
// --quick checks the committed matrix prefix (pre-push); --full checks all rows
// (CI/verify_audit); --write deliberately regenerates the committed evidence.

import { spawnSync } from "node:child_process";
import {
  existsSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync,
} from "node:fs";
import { tmpdir } from "node:os";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { buildArgs, saveSettingsLines, toString } from "../../../web/command.mjs";
import { validate } from "../../../web/validate.mjs";

const here = dirname(fileURLToPath(import.meta.url));
const root = resolve(here, "../../..");
const product = join(root, "working_code/gifscythe");
const versionText = readFileSync(join(product, "VERSION.md"), "utf8");
const version = versionText.match(/Current version:.*?(\d+\.\d+\.\d+)/)?.[1] ?? "0.1.0";
const cli = join(product, "build/gifscythe-cli");
const engine = join(product, "release", version, "gifsicle");
const fixture = join(root, "reference_code/gifsicle/logo.gif");
const matrixFile = join(product, "tests/oracle_fuzz_matrix.json");
const seed = 0x47534631; // "GSF1", fixed so a failure is replayable.

const args = new Set(process.argv.slice(2));
const writeMode = args.has("--write");
const quick = args.has("--quick") || (!args.has("--full") && !writeMode);
if ([...args].some((x) => !["--quick", "--full", "--write"].includes(x))
    || (args.has("--quick") && args.has("--full"))
    || (writeMode && (args.has("--quick") || args.has("--full")))) {
  console.error("usage: oracle_fuzz.mjs [--quick | --full | --write]");
  process.exit(2);
}
if (!existsSync(cli) || !existsSync(engine) || !existsSync(fixture)) {
  console.error(`oracle-fuzz prerequisites missing: build CLI and engine first (${cli}, ${engine})`);
  process.exit(2);
}
const versionProbe = spawnSync(engine, ["--version"], { encoding: "utf8" });
const engineIdentity = (versionProbe.stdout || "").split(/\r?\n/)[0];
if (versionProbe.status !== 0 || !engineIdentity.startsWith("LCDF Gifsicle ")) {
  console.error(`could not identify the bundled engine: ${JSON.stringify(engineIdentity)}`);
  process.exit(2);
}

// Small, explicit boundaries guarantee coverage of the known engine/model
// seams. The seeded tail mixes valid and invalid settings without making the
// run unbounded or depending on Math.random()/external services.
const curated = [
  ["defaults", {}],
  ["colors-low-warned", { color_count: 1 }],
  ["colors-min", { color_count: 2 }],
  ["colors-max", { color_count: 256 }],
  ["colors-high-warned", { color_count: 257 }],
  ["disposal-max", { disposal: 7 }],
  ["disposal-high-warned", { disposal: 8 }],
  ["optimize-off", { optimize_level: 0 }],
  ["optimize-high-warned", { optimize_level: 4 }],
  ["lossy-max", { lossy: 200 }],
  ["lossy-high-warned", { lossy: 201 }],
  ["delay-negative-warned", { delay_cs: -2 }],
  ["loop-once", { loopcount: -2 }],
  ["loop-forever", { loopcount: 0 }],
  ["loop-wrap-warned", { loopcount: 65536 }],
  ["resize-fit-zero-refused", { resize_kind: "fit", resize_w: 0, resize_h: 0 }],
  ["scale-zero-noop-guard", { resize_kind: "scale", scale_x: 0, scale_y: 1 }],
  ["scale-asymmetric", { resize_kind: "scale", scale_x: 0.5, scale_y: 2 }],
  ["color-method-invalid", { color_method: "kdtree" }],
  ["resize-method-invalid", { resize_method: "bicubic" }],
  ["gamma-invalid-silent-engine", { gamma_str: "not-a-gamma" }],
  ["dither-parameter", { dither_method: "o,4" }],
  ["threads-auto", { threads: 0 }],
  ["crop-zero-extends", { crop: true, crop_x: 0, crop_y: 0, crop_w: 0, crop_h: 0 }],
];

const dimensions = [
  ["color_count", [-1, 1, 2, 16, 256, 257]],
  ["disposal", [-1, 0, 4, 7, 8]],
  ["optimize_level", [-1, 0, 1, 3, 4]],
  ["lossy", [-1, 0, 80, 200, 201]],
  ["delay_cs", [-1, 0, 5, -2]],
  ["loopcount", [-2, -1, 0, 1, 65535, 65536]],
  ["threads", [-1, 0, 1, 8, -2]],
  ["color_method", [undefined, "diversity", "median-cut", "kdtree"]],
  ["resize_method", [undefined, "lanczos3", "good", "bicubic"]],
  ["gamma_str", [undefined, "srgb", "oklab", "2.2", "not-a-gamma"]],
  ["dither_method", [undefined, "none", "ro64", "o,4"]],
  ["geometry", [
    undefined,
    { resize_kind: "fit", resize_w: 32, resize_h: 24 },
    { resize_kind: "fit", resize_w: 0, resize_h: 0 },
    { resize_kind: "width", resize_w: 0 },
    { resize_kind: "scale", scale_x: 0.5, scale_y: 2 },
    { resize_kind: "scale", scale_x: 0, scale_y: 1 },
  ]],
];

let state = seed >>> 0;
function rand() {
  state ^= state << 13; state >>>= 0;
  state ^= state >>> 17; state >>>= 0;
  state ^= state << 5; state >>>= 0;
  return state >>> 0;
}
function pick(values) { return values[rand() % values.length]; }
const cases = curated.map(([name, patch]) => ({ name, patch: { ...patch } }));
for (let i = 0; i < 40; i++) {
  const patch = {};
  const used = new Set();
  const fields = 1 + (rand() % 4);
  for (let n = 0; n < fields; n++) {
    let index = rand() % dimensions.length;
    while (used.has(index)) index = rand() % dimensions.length;
    used.add(index);
    const [key, values] = dimensions[index];
    const value = pick(values);
    if (key === "geometry") Object.assign(patch, value ?? {});
    else if (value !== undefined) patch[key] = value;
  }
  cases.push({ name: `seed-${String(i).padStart(2, "0")}`, patch });
}

const runCases = quick ? cases.slice(0, curated.length) : cases;
const tempRoot = mkdtempSync(join(tmpdir(), "gifscythe-oracle-"));
const matrix = [];
let failures = 0;
const fail = (name, reason) => { console.error(`FAIL ${name}: ${reason}`); failures++; };
const hasGif = (path) => {
  try {
    const b = readFileSync(path);
    return b.length > 6 && (b.subarray(0, 6).toString("ascii") === "GIF87a"
      || b.subarray(0, 6).toString("ascii") === "GIF89a");
  } catch { return false; }
};
const run = (file, argv, cwd, timeout = 10000) => spawnSync(file, argv, {
  cwd, encoding: "utf8", timeout, maxBuffer: 1024 * 1024,
});

try {
  for (let i = 0; i < runCases.length; i++) {
    const { name, patch } = runCases[i];
    const dir = join(tempRoot, String(i).padStart(3, "0"));
    mkdirSync(dir);
    const out = join(dir, "out.gif");
    const settings = { mode: "auto", ...patch, inputs: [fixture], output: out };
    const warnings = validate(settings);
    const accepted = warnings.length === 0;
    const argv = buildArgs(settings);
    const normalizedArgv = argv.map((a) => a === fixture ? "<fixture>" : a === out ? "<output>" : a);
    const conf = join(dir, "settings.conf");
    // Two malformed negative sentinels are intentionally not emitted by the
    // normal form writer (it normalizes them to "unset"). Feed them as raw
    // conf entries so the native parser/validator sees the same candidate that
    // the JS validator just judged, matching the existing parity fixture rule.
    let confText = saveSettingsLines(settings);
    if (Number.isFinite(settings.delay_cs) && settings.delay_cs < -1) {
      confText += `delay = ${settings.delay_cs}\n`;
    }
    if (Number.isFinite(settings.threads) && settings.threads < -1) {
      confText += `threads = ${settings.threads}\n`;
    }
    writeFileSync(conf, confText);

    // Compare the actual C++-parsed conf command with the JS argv builder.
    const print = run(cli, [conf, "--engine", engine], dir);
    const lines = (print.stdout || "").split(/\r?\n/);
    const prefix = `${engine} `;
    const cppLine = lines[1] ?? "";
    const parity = print.status === 0 && cppLine.startsWith(prefix)
      && cppLine.slice(prefix.length) === toString(settings);
    if (!parity) fail(name, `JS/C++ argv mismatch (CLI exit ${print.status}; C++=${JSON.stringify(cppLine)}; JS=${JSON.stringify(toString(settings))})`);

    // The bundled engine is the oracle, not the mirrors. Every candidate runs
    // in its own disposable directory so refusal/no-output cannot reuse a stale
    // product. Successful output must be a non-empty GIF with real GIF magic.
    const direct = run(engine, argv, dir);
    const engineGif = direct.status === 0 && hasGif(out);
    const engineRefused = direct.status !== 0 || !engineGif;
    if (accepted && engineRefused) {
      fail(name, `validator accepted, engine did not produce a verified GIF (exit ${direct.status})`);
    }
    if (engineRefused && !warnings.length) {
      fail(name, `engine refused (exit ${direct.status}) but product accepted without warning`);
    }

    // Strict CLI is the product refusal/acceptance contract for the same conf.
    rmSync(out, { force: true });
    const cliRun = run(cli, [conf, "--strict", "--run", "--engine", engine], dir);
    const cliGif = hasGif(out);
    const cliExpected = accepted ? 0 : 3;
    if (cliRun.status !== cliExpected) {
      fail(name, `--strict CLI exit ${cliRun.status}; expected ${cliExpected} for ${warnings.length} warning(s)`);
    }
    if (accepted && (cliRun.status !== 0 || !cliGif)) {
      fail(name, `product accepted but --run did not leave a verified GIF (exit ${cliRun.status})`);
    }
    if (!accepted && cliGif) {
      fail(name, "--strict refused settings but unexpectedly left output");
    }

    matrix.push({
      name,
      settings: patch,
      argv: normalizedArgv,
      validatorWarnings: warnings,
      jsCppArgvParity: parity,
      directEngineExit: direct.status ?? -1,
      directEngineGif: engineGif,
      strictCliExit: cliRun.status ?? -1,
      strictCliGif: cliGif,
    });
    console.log(`${accepted ? "ACCEPT" : "WARN"} ${name}: engine rc=${direct.status}, CLI rc=${cliRun.status}`);
  }
} finally {
  rmSync(tempRoot, { recursive: true, force: true });
}

if (!matrix.some((row) => row.validatorWarnings.length === 0 && row.directEngineGif)) {
  fail("coverage", "sample has no product-accepted, engine-verified case");
}
if (!matrix.some((row) => row.directEngineExit !== 0 || !row.directEngineGif)) {
  fail("coverage", "sample has no engine-refused case");
}
if (!matrix.some((row) => row.validatorWarnings.length > 0 && row.directEngineExit === 0)) {
  fail("coverage", "sample has no warned case the engine itself accepts (silent/no-op guard coverage)");
}

const expected = {
  schema: 1,
  seed: `0x${seed.toString(16)}`,
  engine: engineIdentity,
  cases: matrix,
};
if (failures) {
  console.error(`oracle-fuzz: ${failures} failure(s) across ${matrix.length} cases (seed ${expected.seed})`);
  process.exit(1);
}
const rendered = JSON.stringify(expected, null, 2) + "\n";
if (writeMode) {
  mkdirSync(dirname(matrixFile), { recursive: true });
  writeFileSync(matrixFile, rendered);
  console.log(`WROTE ${matrixFile} (${matrix.length} cases)`);
} else if (!existsSync(matrixFile)) {
  fail("matrix", `missing committed matrix ${matrixFile}; run --write deliberately after review`);
} else {
  const committed = JSON.parse(readFileSync(matrixFile, "utf8"));
  const want = quick ? { ...expected, cases: expected.cases.slice(0, matrix.length) } : expected;
  const have = quick ? { ...committed, cases: committed.cases.slice(0, matrix.length) } : committed;
  if (JSON.stringify(have) !== JSON.stringify(want)) {
    fail("matrix", `${quick ? "quick prefix" : "full matrix"} differs from committed artifact; inspect the diff and regenerate only after review`);
  }
}

if (failures) {
  console.error(`oracle-fuzz: ${failures} failure(s) across ${matrix.length} cases (seed ${expected.seed})`);
  process.exit(1);
}
console.log(`oracle-fuzz: ${matrix.length} deterministic cases passed (seed ${expected.seed}, ${quick ? "quick" : "full"})`);
