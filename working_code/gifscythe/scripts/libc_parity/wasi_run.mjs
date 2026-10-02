// wasi_run.mjs - run a wasm32-wasi build of gifsicle under Node's WASI (libc_parity.py uses it).
//
//   node --no-warnings wasi_run.mjs <engine.wasm> <hostdir> -- <gifsicle args...>
//
// <hostdir> is preopened as /w, so the arguments name files as /w/<name>. The exit code is the
// engine's. Nothing outside <hostdir> is reachable from the module.
import { readFileSync } from "node:fs";
import { WASI } from "node:wasi";

const [wasmPath, hostDir, sep, ...args] = process.argv.slice(2);
if (!wasmPath || !hostDir || sep !== "--") {
  console.error("usage: node wasi_run.mjs <engine.wasm> <hostdir> -- <gifsicle args...>");
  process.exit(2);
}
const wasi = new WASI({
  version: "preview1",
  args: ["gifsicle", ...args],
  env: {},
  preopens: { "/w": hostDir },
  returnOnExit: true,
});
const instance = await WebAssembly.instantiate(
  await WebAssembly.compile(readFileSync(wasmPath)),
  wasi.getImportObject(),
);
process.exit(wasi.start(instance) ?? 0);
