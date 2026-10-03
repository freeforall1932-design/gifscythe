// prove_wasm.mjs — prove the wasm engine on a real file.
//
// Usage: node web/wasm/prove_wasm.mjs [input.gif] [--oracle <native gifsicle>] [--module <gifsicle.js>]
//   (default input:  the upstream logo.gif, rebuilt from tests/fixtures/logo.gif.b64 - the repo holds no images)
//   (default module: web/wasm/dist/gifsicle.js, the build_wasm.sh output)
//
// Loads the module in Node (the same MODULARIZE factory the page uses), runs
// `-O3 in.gif -o out.gif` through the Emscripten virtual FS, and prints
// input/output BYTES plus the output GIF magic and --info — a byte-level
// proof, not a green build.
//
// Without --oracle it exits non-zero unless the output is a non-empty GIF the
// module itself produced, and says plainly that it compared nothing.
//
// With --oracle (S34, the owner's N-32 decision) it also runs the same -O3 on
// the same input through <native gifsicle> and exits non-zero unless the two
// outputs are BYTE-EQUAL. The oracle must be built against the module's libc
// family (musl, for Emscripten): gifsicle's output depends on libc qsort tie
// order and random(), so a glibc oracle differs by construction.
//   python3 working_code/gifscythe/scripts/libc_parity/libc_parity.py --build-oracle /tmp/gs-oracle
//   node web/wasm/prove_wasm.mjs --oracle /tmp/gs-oracle/gifsicle-musl
// (That comparison, for a zig-built wasm32-wasi engine, runs in CI as
// `libc_parity.py --bar`; no Emscripten build has ever been run through THIS
// script - emcc is the part nothing here can provide.)
//
// Reference numbers (measured S19 with a glibc native 1.96 — NOT this script's
// verdict): logo.gif 8703 B -> -O3 -> 8637 B, GIF89a, 12 images, 60x132, loop
// forever.

import { mkdtempSync, readFileSync, rmSync } from "node:fs";
import { spawnSync } from "node:child_process";
import { tmpdir } from "node:os";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { createRequire } from "node:module";
import { fixturePath } from "../../working_code/gifscythe/scripts/fixtures.mjs";

const here = dirname(fileURLToPath(import.meta.url));
const repo = join(here, "..", "..");

const fail = (msg) => {
  console.error(`prove_wasm: FAIL: ${msg}`);
  process.exit(1);
};

let positional = null;
let oraclePath = null;
let modulePath = null;
{
  const argv = process.argv.slice(2);
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    const value = (flag) => {
      if (a.startsWith(`${flag}=`)) return a.slice(flag.length + 1);
      if (i + 1 >= argv.length) {
        console.error(`prove_wasm: ${flag} needs a value`);
        process.exit(2);
      }
      return argv[++i];
    };
    if (a === "--oracle" || a.startsWith("--oracle=")) oraclePath = resolve(value("--oracle"));
    else if (a === "--module" || a.startsWith("--module=")) modulePath = resolve(value("--module"));
    else if (a.startsWith("--")) {
      console.error(`prove_wasm: unknown option ${a} (usage: [input.gif] [--oracle <native gifsicle>] [--module <gifsicle.js>])`);
      process.exit(2);
    } else if (positional === null) positional = a;
    else {
      console.error(`prove_wasm: unexpected extra argument ${a}`);
      process.exit(2);
    }
  }
}
const distJs = modulePath || join(here, "dist", "gifsicle.js");
// No binary files in the repo (N-36): the default input is the upstream logo.gif rebuilt from text.
const inputPath = resolve(positional || fixturePath("logo.gif"));

let inputBytes;
try {
  inputBytes = readFileSync(inputPath);
} catch (e) {
  fail(`cannot read input ${inputPath}: ${e.message}`);
}

let createGifsicle;
try {
  createGifsicle = createRequire(import.meta.url)(distJs);
} catch (e) {
  fail(
    `cannot load ${distJs} (${e.code || e.message}) — run web/wasm/build_wasm.sh first (needs emcc).`
  );
}
if (typeof createGifsicle !== "function") {
  fail(`factory is not a function in ${distJs}`);
}

const stdout = [];
const stderr = [];
const M = await createGifsicle({
  print: (t) => stdout.push(t),
  printErr: (t) => stderr.push(t),
});

const run = (args) => {
  stdout.length = 0;
  stderr.length = 0;
  try {
    M.callMain(args);
  } catch (e) {
    // gifsicle exits non-zero via exit(): Emscripten surfaces it as an
    // ExitStatus-style exception carrying .status; anything else is a bug.
    if (e && typeof e.status === "number") return e.status;
    throw e;
  }
  return 0;
};

M.FS.writeFile("/in.gif", inputBytes);
const code = run(["-O3", "/in.gif", "-o", "/out.gif"]);
if (code !== 0) {
  fail(`gifsicle exited ${code}: ${stderr.join("\n") || stdout.join("\n") || "(no output)"}`);
}
let outBytes;
try {
  outBytes = M.FS.readFile("/out.gif");
} catch (e) {
  fail(`exit 0 but no /out.gif in the virtual FS: ${e.message}`);
}
const magic = Buffer.from(outBytes.slice(0, 6)).toString("ascii");
if (outBytes.length === 0 || (magic !== "GIF87a" && magic !== "GIF89a")) {
  fail(`output is not a GIF (bytes=${outBytes.length}, magic=${JSON.stringify(magic)})`);
}

const infoCode = run(["--info", "/out.gif"]);
const info = stdout.join("\n");

console.log(`input:  ${inputPath} (${inputBytes.length} bytes)`);
console.log(`output: /out.gif (${outBytes.length} bytes), magic ${magic}`);
console.log(`--info exit ${infoCode}:`);
console.log(info || "(no --info output)");

if (oraclePath === null) {
  console.log(
    "note: no --oracle given - this run proves the module produced a GIF, NOT that it matches native output (N-32)"
  );
  console.log("prove_wasm: PASS (non-empty GIF only)");
} else {
  // The same command, the same input, natively. Any difference is a verdict, never a warning.
  const work = mkdtempSync(join(tmpdir(), "prove-wasm-"));
  try {
    const nativeOut = join(work, "oracle.gif");
    const r = spawnSync(oraclePath, ["-O3", inputPath, "-o", nativeOut], { encoding: "utf8" });
    if (r.error) fail(`cannot run the oracle ${oraclePath}: ${r.error.message}`);
    if (r.status !== 0) {
      fail(`the oracle ${oraclePath} exited ${r.status}: ${(r.stderr || r.stdout || "(no output)").trim()}`);
    }
    let oracleBytes;
    try {
      oracleBytes = readFileSync(nativeOut);
    } catch (e) {
      fail(`the oracle exited 0 but wrote no output: ${e.message}`);
    }
    const mine = Buffer.from(outBytes);
    if (!mine.equals(oracleBytes)) {
      let at = 0;
      const n = Math.min(mine.length, oracleBytes.length);
      while (at < n && mine[at] === oracleBytes[at]) at++;
      fail(
        `module output (${mine.length} bytes) != oracle output (${oracleBytes.length} bytes), ` +
          `first difference at offset ${at}. The oracle must share the module's libc family ` +
          `(musl for Emscripten): build it with libc_parity.py --build-oracle. A glibc oracle differs by ` +
          `construction (qsort tie order, random(); STATUS N-32).`
      );
    }
    console.log(`oracle: ${oraclePath} - output byte-equal (${mine.length} bytes)`);
    console.log("prove_wasm: PASS (byte-equal to the same-libc oracle)");
  } finally {
    rmSync(work, { recursive: true, force: true });
  }
}
