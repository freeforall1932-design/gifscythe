// prove_wasm.mjs — prove the wasm engine on a real file.
//
// Usage: node web/wasm/prove_wasm.mjs [input.gif]
//   (default input: reference_code/gifsicle/logo.gif)
//
// Loads web/wasm/dist/gifsicle.js in Node (the same MODULARIZE factory the
// page uses), runs `-O3 in.gif -o out.gif` through the Emscripten virtual
// FS, and prints input/output BYTES plus the output GIF magic and --info —
// a byte-level proof, not a green build. Exits non-zero unless the output
// is a non-empty GIF the module itself produced.
//
// Native oracle (measured S19 with the repo-built 1.96, for comparison —
// NOT this script's verdict): logo.gif 8703 B -> -O3 -> 8637 B, GIF89a,
// 12 images, 60x132, loop forever.

import { readFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { createRequire } from "node:module";

const here = dirname(fileURLToPath(import.meta.url));
const repo = join(here, "..", "..");
const distJs = join(here, "dist", "gifsicle.js");
const inputPath = resolve(process.argv[2] || join(repo, "reference_code", "gifsicle", "logo.gif"));

const fail = (msg) => {
  console.error(`prove_wasm: FAIL: ${msg}`);
  process.exit(1);
};

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
console.log("prove_wasm: PASS");
