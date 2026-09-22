// device-names.test.mjs — the web half of audit U-56 / fix-order P1-36.
//
// The C++ sanitizer and the web admission check implement the SAME Win32
// reserved-device rule on two hosts, and the finding was that the desktop copy
// missed the superscript aliases `COM¹`..`COM³` that the C: parser folds like
// digits. A rule duplicated in two languages needs one table, not two opinions:
// both this test and the C++ unit suite read
// working_code/gifscythe/tests/windows_reserved_names.txt, so a row that only
// one surface honours goes red on whichever side drifted.
//
// Run: node web/test/device-names.test.mjs

import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { uploadNameError } from "../run-paths.mjs";

const HERE = dirname(fileURLToPath(import.meta.url));
const TABLE = join(HERE, "..", "..", "working_code", "gifscythe", "tests",
                   "windows_reserved_names.txt");

let pass = 0;
const failures = [];
const rows = readFileSync(TABLE, "utf8").split("\n")
  .filter((l) => l && !l.startsWith("#"))
  .map((l) => { const i = l.indexOf("\t"); return { name: l.slice(0, i), want: l.slice(i + 1) }; })
  .filter((r) => r.want === "reserved" || r.want === "ok");

if (rows.length < 20) failures.push(`table shrank to ${rows.length} rows`);

for (const { name, want } of rows) {
  const err = uploadNameError(name);
  const rejected = err !== null;
  const problems = [];
  if (want === "reserved" && !rejected) problems.push("admitted a reserved device name");
  if (want === "reserved" && rejected && !/reserved Windows device/.test(err)) {
    problems.push(`rejected for the wrong reason: ${err}`);
  }
  if (want === "ok" && rejected) problems.push(`refused a legal name: ${err}`);
  if (problems.length) {
    failures.push(`${JSON.stringify(name)} [${want}]: ${problems.join("; ")}`);
  } else {
    pass += 1;
  }
}

for (const f of failures) console.log(`FAIL ${f}`);
if (failures.length) {
  console.log(`device-names: ${failures.length} FAILED of ${pass + failures.length}`);
  process.exit(1);
}
console.log(`device-names: all cases passed (${pass} rows vs the shared table)`);
