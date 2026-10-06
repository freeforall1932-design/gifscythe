// stem.test.mjs — the web half of audit U-96 / P3-18.
//
// `stemOf` decides the names the web writes (`<stem>_opt.gif`, `<stem>_frame`)
// and it used to exist TWICE (app.js and server.mjs) with its dotfile and
// extensionless boundary never pinned against the desktop's rule. The desktop's
// rule is Qt's QFileInfo::completeBaseName() (MainWindow.cpp), so the two
// surfaces could silently disagree on odd upload names.
//
// The fix has three parts, all checked here:
//   1. ONE helper (web/stem.mjs) — asserted structurally below, so a future
//      edit cannot quietly re-copy it into app.js or server.mjs;
//   2. the measured Qt semantics live in a shared table
//      (working_code/gifscythe/tests/stem_cases.txt) that BOTH this test and
//      the Qt-side offscreen case (T25) read, the same shape as
//      windows_reserved_names.txt for U-56;
//   3. every row is asserted against stemOf, including the trailing-dot rows
//      where the old hand-written rule disagreed with Qt.
//
// Run: node web/test/stem.test.mjs

import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { stemOf } from "../stem.mjs";

const HERE = dirname(fileURLToPath(import.meta.url));
const ROOT = join(HERE, "..", "..");
const TABLE = join(ROOT, "working_code", "gifscythe", "tests", "stem_cases.txt");

let pass = 0;
const failures = [];

// ---- 1. the table ----
const raw = readFileSync(TABLE, "utf8");
const provenance = raw.split("\n").filter((l) => l.startsWith("#")).join("\n");
const rows = raw.split("\n")
  .filter((l) => l && !l.startsWith("#"))
  .map((l) => {
    const i = l.indexOf("\t");
    return { name: l.slice(0, i), stem: l.slice(i + 1) };
  });

if (rows.length < 10) failures.push(`table shrank to ${rows.length} rows`);
if (!/QFileInfo::completeBaseName/.test(provenance)) {
  failures.push("table provenance no longer names the Qt rule it was measured against");
}
for (const want of ["trailing.", "a.", ".gif", "a.b.gif"]) {
  if (!rows.some((r) => r.name === want)) failures.push(`table lost the ${JSON.stringify(want)} row`);
}

// ---- 2. stemOf matches every measured row ----
for (const { name, stem } of rows) {
  const got = stemOf(name);
  if (got !== stem) {
    failures.push(`stemOf(${JSON.stringify(name)}) = ${JSON.stringify(got)}, Qt measured ${JSON.stringify(stem)}`);
  } else {
    pass++;
  }
}

// ---- 3. the rule lives in exactly one place ----
// A structural check, not a behavioural one: re-copying the helper into a
// consumer would keep every row above green while re-creating the divergence
// risk this row is about, so the consumers must IMPORT it and not define it.
for (const consumer of ["app.js", "server.mjs"]) {
  const src = readFileSync(join(ROOT, "web", consumer), "utf8");
  if (/const\s+stemOf\s*=/.test(src)) {
    failures.push(`${consumer} defines its own stemOf again (U-96 regression)`);
  }
  if (!/from\s+["']\.\/stem\.mjs["']/.test(src)) {
    failures.push(`${consumer} does not import stem.mjs`);
  } else {
    pass++;
  }
}

// ---- 4. the naming the server actually builds uses the shared helper ----
{
  const src = readFileSync(join(ROOT, "web", "server.mjs"), "utf8");
  if (!/_opt\.gif/.test(src) || !/stemOf\(/.test(src)) {
    failures.push("server.mjs no longer derives <stem>_opt.gif through stemOf");
  } else {
    pass++;
  }
}

// ---- 5. mutation legs: prove the table has TEETH ----
// The point of a pinned table is that the PRE-FIX rules fail it. Each mutant
// below is a real rule this row's history contains; if one of them satisfies
// the whole table, the table has stopped discriminating and the green above
// means nothing.
{
  const mutants = {
    // THE pre-fix rule that shipped in app.js and server.mjs (S24 node probe
    // recorded its output: ".gif" -> ".gif"): it kept dotfiles whole, and the
    // Qt measurement shows that is wrong.
    "pre-fix copy (lastIndexOf > 0: dotfiles kept whole)": (n) => {
      const i = n.lastIndexOf(".");
      return i > 0 ? n.slice(0, i) : n;
    },
    "first-dot rule (baseName, not completeBaseName)": (n) => {
      const i = n.indexOf(".");
      return i >= 0 ? n.slice(0, i) : n;
    },
    "trailing dot kept (dot only counts when followed by a character)": (n) => {
      const i = n.lastIndexOf(".");
      return i >= 0 && i < n.length - 1 ? n.slice(0, i) : n;
    },
    "no stripping at all": (n) => n,
  };
  const undetected = [];
  for (const [label, fn] of Object.entries(mutants)) {
    const caught = rows.some((r) => fn(r.name) !== r.stem);
    if (!caught) undetected.push(label);
  }
  if (undetected.length) {
    failures.push(`table no longer catches: ${undetected.join(", ")}`);
  } else {
    pass++;
  }
}

if (failures.length) {
  for (const f of failures) console.error(`FAIL: ${f}`);
  console.error(`==> ${pass} passed, ${failures.length} FAILED`);
  process.exit(1);
}
console.log(`==> stem parity: ${pass} checks, 0 failures (${rows.length} measured Qt rows)`);
