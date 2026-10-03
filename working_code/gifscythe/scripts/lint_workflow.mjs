// lint_workflow.mjs - actionlint for the GitHub Actions workflow, in a sandbox that has no actionlint binary.  (S34)
//
//   node working_code/gifscythe/scripts/lint_workflow.mjs               lint .github/workflows/*.yml
//   node working_code/gifscythe/scripts/lint_workflow.mjs FILE...       lint these files
//   node working_code/gifscythe/scripts/lint_workflow.mjs --selftest    prove the linter is really running
//
// Why. A workflow is only ever run by Actions, and a structural mistake (a `needs` on a job that does not
// exist, a misspelled context property, an unknown key) costs a whole push-and-wait cycle - or, worse, a
// workflow-file error that starts no jobs at all. The actionlint binary cannot be downloaded in the agent
// sandboxes (its release assets are on a blocked host), but its WebAssembly build is on npm
// (`actionlint`, rhysd's checker compiled to wasm), which registry.npmjs.org serves. It is installed on
// first use into $GS_ACTIONLINT_DIR (default $TMPDIR/gs-actionlint; nothing enters the checkout).
//
// Known limit, filtered here and printed: the packaged build is old, so its list of GitHub-hosted runner
// labels stops at ubuntu-22.04 / windows-2022 and it calls ubuntu-24.04 "unknown". Findings of kind
// `runner-label` whose label looks like a standard hosted label are dropped; a typo'd or custom label is
// still reported. It does not run shellcheck or pyflakes on `run:` scripts - shellcheck the scripts a
// workflow calls (that is why the gate and the runners are scripts).
//
// Exit: 0 clean / 1 findings (or the self-test failed) / 3 SKIPPED: the linter could not be installed
// (printed, never silent: a skipped lint is not a clean lint).
import { execFileSync } from "node:child_process";
import { existsSync, mkdirSync, readdirSync, readFileSync } from "node:fs";
import { createRequire } from "node:module";
import { tmpdir } from "node:os";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const PIN = "actionlint@2.0.6";
const here = dirname(fileURLToPath(import.meta.url));
const repo = resolve(here, "..", "..", "..");
const dir = process.env.GS_ACTIONLINT_DIR || join(tmpdir(), "gs-actionlint");

function skip(why) {
  console.log(`SKIP: ${why} - the workflow was NOT linted`);
  process.exit(3);
}

if (!existsSync(join(dir, "node_modules", "actionlint"))) {
  console.log(`==> installing ${PIN} into ${dir} (first use)`);
  try {
    // cwd, not --prefix: `npm init --prefix` writes into the CURRENT directory (it once dropped a
    // package.json into the checkout), so the install happens inside the scratch dir itself.
    mkdirSync(dir, { recursive: true });
    execFileSync("npm", ["install", "--no-audit", "--no-fund", "--no-package-lock", PIN], { cwd: dir, stdio: "inherit" });
  } catch (e) {
    skip(`could not install ${PIN} (${e.message.split("\n")[0]})`);
  }
}
let createLinter;
try {
  ({ createLinter } = await import(pathToFileURL(createRequire(join(dir, "x.js")).resolve("actionlint")).href));
} catch (e) {
  skip(`could not load actionlint from ${dir} (${e.message.split("\n")[0]})`);
}
const lint = await createLinter();

// A label the packaged build predates, not a mistake in the workflow.
const stale = (r) =>
  r.kind === "runner-label" &&
  /label "((ubuntu|macos)-\d\d(\.\d\d)?(-arm)?|windows-20\d\d(-arm)?)" is unknown/.test(r.message);

function run(label, text) {
  const all = lint(text, label);
  const kept = all.filter((r) => !stale(r));
  return { kept, dropped: all.length - kept.length };
}

if (process.argv.includes("--selftest")) {
  // Four deliberate mistakes; a linter that reports fewer than these four kinds is not running.
  const bad = [
    "name: x", "on: push", "jobs:", "  a:", "    runs-on: ubuntu-latest", "    needs: nothere",
    "    steps:", "      - run: echo ${{ github.evnt_name }}", "      - uses: actions/checkout@v4", "        wiht: {x: 1}", "",
  ].join("\n");
  const kinds = new Set(run("selftest.yml", bad).kept.map((r) => r.kind));
  const want = ["job-needs", "expression", "syntax-check"];
  const missing = want.filter((k) => !kinds.has(k));
  if (missing.length) {
    console.log(`SELFTEST FAILED: the known-bad sample did not produce: ${missing.join(", ")} (got: ${[...kinds].join(", ") || "nothing"})`);
    process.exit(1);
  }
  const good = run("good.yml", "name: ok\non: push\njobs:\n  a:\n    runs-on: ubuntu-24.04\n    steps:\n      - run: echo hi\n");
  if (good.kept.length !== 0 || good.dropped !== 1) {
    console.log(`SELFTEST FAILED: the clean sample should have 0 findings and 1 stale label dropped, got ${good.kept.length} and ${good.dropped}`);
    process.exit(1);
  }
  console.log("SELFTEST OK: the linter reports needs/expression/syntax mistakes and passes a clean file");
  process.exit(0);
}

let files = process.argv.slice(2).filter((a) => !a.startsWith("--"));
if (files.length === 0) {
  const wf = join(repo, ".github", "workflows");
  files = readdirSync(wf).filter((f) => /\.ya?ml$/.test(f)).map((f) => join(wf, f));
}
let problems = 0;
let dropped = 0;
for (const f of files) {
  const res = run(f, readFileSync(f, "utf8"));
  dropped += res.dropped;
  for (const r of res.kept) {
    problems++;
    console.log(`${f}:${r.line}:${r.column}: ${r.message} [${r.kind}]`);
  }
}
if (dropped) console.log(`note: ${dropped} runner-label finding(s) dropped - the packaged actionlint predates those hosted labels`);
console.log(problems ? `actionlint: ${problems} problem(s) in ${files.length} file(s)` : `actionlint: clean (${files.length} file(s))`);
process.exit(problems ? 1 : 0);
