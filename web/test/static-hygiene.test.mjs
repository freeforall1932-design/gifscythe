// static-hygiene.test.mjs — regression for U-67 / NF-10 (audit F, fable 5.1 low WINNER),
// as NARROWED by measurement in S21 (see COMPILED_AUDIT §5 U-67 and §2F F-10).
//
// What was actually true of the original serveStatic():
//   TRUE      it served the entire web/ tree with no allow-list — server.mjs
//             (26 KB, disclosing the loopback-bind and GS_ENGINE handling),
//             test/transport.test.mjs (35 KB), run-paths.mjs, validate.mjs,
//             output-verify.mjs, README.md, WEB_PLAN_TEMPLATE.md, wasm/*.
//   OVERSTATED the `file.startsWith(ROOT)` prefix check is brittle hygiene, not a
//             demonstrated reachable traversal: `new URL()` normalises dot-segments
//             before serveStatic sees them, so `/../STATUS.md`, `/../../etc/hostname`
//             and `/../working_code/...` all returned 404. `/../server.mjs` returned
//             200 only because it normalises to `/server.mjs`, which is INSIDE ROOT.
//   FALSE     "HEAD returns a body" does not reproduce — Node suppresses HEAD bodies
//             itself (measured: server wrote 5000 bytes, client received 0).
//
// So this test asserts the narrowed defect (over-exposure) plus two guard rails:
// containment stays defense-in-depth, and the HEAD contract is PINNED so a future
// refactor cannot silently regress a behaviour Node currently gives us for free.
//
// The owner's S21 decision: this Node server is the supported shipped web surface
// and web/wasm/ is experimental / not shippable, so the allow-list is exactly the
// four files the UI loads (index.html, style.css, app.js, command.mjs) and
// everything else under web/ is non-routable.
//
// No engine is needed: serveStatic never touches findEngine().
//
// Run:  node web/test/static-hygiene.test.mjs

import { spawn } from "node:child_process";
import { createServer, request as httpRequest } from "node:http";
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = dirname(fileURLToPath(import.meta.url));
const SERVER = join(__dirname, "..", "server.mjs");
const WEB = join(__dirname, "..");

function freePort() {
  return new Promise((res, rej) => {
    const s = createServer();
    s.on("error", rej);
    s.listen(0, "127.0.0.1", () => {
      const { port } = s.address();
      s.close(() => res(port));
    });
  });
}

// fetch() normalises the URL before it goes on the wire, so traversal probes must
// use a raw http.request to send the literal path the server would receive.
function raw(port, method, path) {
  return new Promise((resolve) => {
    const req = httpRequest({ host: "127.0.0.1", port, method, path }, (res) => {
      const chunks = [];
      res.on("data", (c) => chunks.push(c));
      res.on("end", () => resolve({
        status: res.statusCode,
        headers: res.headers,
        body: Buffer.concat(chunks),
      }));
    });
    req.on("error", (err) => resolve({ status: null, headers: {}, body: Buffer.alloc(0), error: String(err.message) }));
    req.end();
  });
}

let failures = 0;
function check(name, ok, detail) {
  if (ok) console.log(`PASS ${name}`);
  else { failures++; console.log(`FAIL ${name}\n       ${detail}`); }
}

const port = await freePort();
const child = spawn(process.execPath, [SERVER, String(port)], {
  stdio: ["ignore", "pipe", "pipe"],
  env: { ...process.env, GS_WEB_HOST: "127.0.0.1" },
});
let serverLog = "";
child.stdout.on("data", (d) => { serverLog += d; });
child.stderr.on("data", (d) => { serverLog += d; });

try {
  // wait for listen
  const deadline = Date.now() + 10_000;
  for (;;) {
    try { const r = await raw(port, "GET", "/"); if (r.status) break; } catch { /* not up */ }
    if (Date.now() > deadline) throw new Error("server did not come up");
    await new Promise((r) => setTimeout(r, 100));
  }

  // ---- 1. the shipped UI still loads, byte-exact, with correct MIME ----
  const UI = [
    ["/", "index.html", "text/html"],
    ["/index.html", "index.html", "text/html"],
    ["/style.css", "style.css", "text/css"],
    ["/app.js", "app.js", "text/javascript"],
    ["/command.mjs", "command.mjs", "text/javascript"],
    ["/request-guard.mjs", "request-guard.mjs", "text/javascript"],
  ];
  for (const [path, file, mime] of UI) {
    const r = await raw(port, "GET", path);
    const want = readFileSync(join(WEB, file));
    check(`U-67 GET ${path} serves ${file} byte-exact`,
      r.status === 200 && r.body.equals(want),
      `status ${r.status}, ${r.body.length} bytes vs ${want.length} on disk`);
    check(`U-67 GET ${path} content-type is ${mime}`,
      String(r.headers["content-type"] || "").startsWith(mime),
      `content-type ${r.headers["content-type"]}`);
  }

  // the UI's own references must resolve, or the allow-list broke the page
  const indexHtml = (await raw(port, "GET", "/index.html")).body.toString("utf8");
  for (const ref of ['href="style.css"', 'src="app.js"']) {
    check(`U-67 index.html still references ${ref}`, indexHtml.includes(ref), "reference missing");
  }
  const appJs = (await raw(port, "GET", "/app.js")).body.toString("utf8");
  check("U-67 app.js still imports ./command.mjs", appJs.includes('from "./command.mjs"'), "import missing");

  // The allow-list is only correct if it covers what the UI actually imports.
  // Without this, adding a module to app.js and forgetting the route produces a
  // page that 404s on module load — a "hygiene fix" silently breaking the
  // product, which is the class of thing S21's measure-first rule exists to catch.
  const imports = [...appJs.matchAll(/from\s+"(\.\/[^"]+)"/g)].map((m) => m[1]);
  check(`U-67 app.js imports are all routable (${imports.map((i) => i.slice(2)).join(", ")})`,
    imports.length > 0 && imports.every((rel) => UI.some(([url, file]) => url === "/" + rel.slice(2) && file === rel.slice(2))),
    `imports ${imports.join(", ")} vs the UI list ${UI.map(([u]) => u).join(", ")}`);
  for (const rel of imports) {
    const r = await raw(port, "GET", "/" + rel.slice(2));
    check(`U-67 ${"/" + rel.slice(2)} loads in the browser (not a 404)`,
      r.status === 200, `status ${r.status}`);
  }


  // ---- 2. the narrowed defect: internal files must NOT be routable ----
  const MUST_NOT_SERVE = [
    "/server.mjs",
    "/run-paths.mjs",
    "/validate.mjs",
    "/output-verify.mjs",
    "/test/transport.test.mjs",
    "/test/command.test.mjs",
    "/test/validate.test.mjs",
    "/test/body-limit.test.mjs",
    "/README.md",
    "/WEB_PLAN_TEMPLATE.md",
    "/wasm/wasm.js",
    "/wasm/index.html",
    "/wasm/build_wasm.sh",
    "/wasm/config.wasm.h",
  ];
  for (const path of MUST_NOT_SERVE) {
    const r = await raw(port, "GET", path);
    check(`U-67 GET ${path} is not routable`,
      r.status === 404,
      `status ${r.status} with ${r.body.length} bytes (expected 404)`);
  }

  // server.mjs specifically: its bytes must not be disclosed
  const leaked = await raw(port, "GET", "/server.mjs");
  check("U-67 server.mjs source is not disclosed",
    !leaked.body.includes("GS_WEB_HOST") && !leaked.body.includes("findEngine"),
    `${leaked.body.length} bytes of server source returned`);

  // ---- 3. containment stays defense-in-depth (raw, un-normalised paths) ----
  for (const path of ["/../server.mjs", "/../STATUS.md", "/../../etc/hostname",
                      "/%2e%2e/server.mjs", "/..%2fserver.mjs",
                      "/../working_code/gifscythe/VERSION.md"]) {
    const r = await raw(port, "GET", path);
    check(`U-67 traversal ${path} never yields a body`,
      r.status === 404 || r.status === 403,
      `status ${r.status} with ${r.body.length} bytes`);
  }

  // ---- 4. HEAD contract PINNED (Node gives this free; do not let it regress) ----
  for (const path of ["/", "/app.js", "/style.css"]) {
    const r = await raw(port, "HEAD", path);
    check(`U-67 HEAD ${path} -> 200 with an empty body`,
      r.status === 200 && r.body.length === 0,
      `status ${r.status}, body ${r.body.length} bytes`);
    check(`U-67 HEAD ${path} still sends content-type`,
      Boolean(r.headers["content-type"]),
      "no content-type header on HEAD");
  }
  const headMissing = await raw(port, "HEAD", "/server.mjs");
  check("U-67 HEAD /server.mjs is not routable either",
    headMissing.status === 404 && headMissing.body.length === 0,
    `status ${headMissing.status}, body ${headMissing.body.length}`);

  // ---- 5. the API endpoints are untouched by a static-serving change ----
  const postRun = await new Promise((resolve) => {
    const req = httpRequest({ host: "127.0.0.1", port, method: "POST", path: "/run" }, (res) => {
      const c = []; res.on("data", (d) => c.push(d));
      res.on("end", () => resolve({ status: res.statusCode, body: Buffer.concat(c).toString("utf8") }));
    });
    req.on("error", (e) => resolve({ status: null, body: String(e.message) }));
    req.end(JSON.stringify({ settings: {}, files: [] }));
  });
  check("U-67 POST /run still reaches its handler (400 usage, not 404)",
    postRun.status === 400 && /no files/i.test(postRun.body),
    `status ${postRun.status} body ${postRun.body.slice(0, 80)}`);

  const method = await raw(port, "PUT", "/");
  check("U-67 PUT / still 405", method.status === 405, `status ${method.status}`);
} catch (err) {
  failures++;
  console.log(`FAIL harness error: ${err && err.stack ? err.stack : err}`);
} finally {
  child.kill("SIGKILL");
}

if (failures) {
  console.log(`\n--- server log ---\n${serverLog.trim()}\n`);
  console.log(`${failures} STATIC-HYGIENE TEST(S) FAILED`);
  process.exit(1);
}
console.log("static-hygiene: all cases passed");
process.exit(0);
