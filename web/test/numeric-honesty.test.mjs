// Numeric input contract tests for audit P1-44 (U-78/U-87).
import { buildArgs, numOrNull } from "../command.mjs";
import { validate } from "../validate.mjs";

const failures = [];
function check(name, condition, detail = "") {
  if (!condition) failures.push(`${name}${detail ? `: ${detail}` : ""}`);
}

check("empty form value is unset", numOrNull("") === null);
check("missing form value is unset", numOrNull(undefined) === null);
check("zero remains explicit", numOrNull("0") === 0);
check("finite decimal survives", numOrNull("0.5") === 0.5);
check("garbage form value is not an argv number", numOrNull("abc") === null);
check("NaN form value is not an argv number", numOrNull(NaN) === null);

const omittedResize = buildArgs({ resize_kind: "fit", resize_w: null, resize_h: 100 });
check("empty resize width omits resize flag", !omittedResize.includes("--resize-fit"),
  JSON.stringify(omittedResize));
const explicitZero = buildArgs({ resize_kind: "scale", scale_x: 0, scale_y: 1 });
check("explicit scale zero is preserved for validation", explicitZero.includes("--scale")
  && explicitZero.includes("0x1"), JSON.stringify(explicitZero));

for (const [field, value] of [
  ["color_count", "abc"], ["optimize_level", "abc"], ["lossy", "abc"],
  ["delay_cs", "abc"], ["threads", "abc"], ["loopcount", "abc"],
]) {
  const issues = validate({ [field]: value });
  check(`${field} rejects wrong type`, issues.some((i) => i.field !== "" && /finite number/.test(i.reason)),
    JSON.stringify(issues));
}

for (const field of ["color_count", "optimize_level", "lossy", "delay_cs", "threads", "loopcount"]) {
  const empty = validate({ [field]: "", inputs: ["input.gif"] });
  check(`${field} treats empty as unset`, empty.length === 0, JSON.stringify(empty));
}

// ---- N-10: the validator/builder seam (2026-09-27 external audit) ----------
// validate.mjs COERCES — Number("5") is finite, so a string validates clean and
// produces no warning. buildArgs used to require `typeof value === "number"`,
// so the same string failed that guard and the flag was silently DROPPED:
// HTTP 200, a real GIF, and a setting the caller set that was never applied.
// A numeric string must now build exactly the same argv as the number.
const SEAM = [
  ["disposal", "5", ["--disposal", "5"]],
  ["loopcount", "-2", ["--no-loopcount"]],
  ["loopcount", "0", ["--loopcount=0"]],
  ["color_count", "32", ["-k", "32"]],
  ["optimize_level", "3", ["-O3"]],
  ["lossy", "40", ["--lossy=40"]],
  ["delay_cs", "25", ["-d", "25"]],
  ["threads", "4", ["-j4"]],
  ["threads", "0", ["-j"]],
];
for (const [field, strValue, expectedFlags] of SEAM) {
  const fromString = buildArgs({ [field]: strValue });
  const fromNumber = buildArgs({ [field]: Number(strValue) });
  for (const flag of expectedFlags) {
    check(`${field}="${strValue}" emits ${flag}`, fromString.includes(flag),
      JSON.stringify(fromString));
  }
  check(`${field}: string and number build the same argv`,
    JSON.stringify(fromString) === JSON.stringify(fromNumber),
    `${JSON.stringify(fromString)} vs ${JSON.stringify(fromNumber)}`);
}

// An empty gamma satisfied `"" >= 0` and then crashed fmtDouble
// ("x.toPrecision is not a function") — an uncaught TypeError in the request
// path, not a silently wrong image. It must be treated as unset.
let gammaThrew = null;
try { buildArgs({ gamma: "" }); } catch (e) { gammaThrew = e.message; }
check("empty gamma does not throw", gammaThrew === null, gammaThrew || "");
check("empty gamma emits no --gamma flag",
  !buildArgs({ gamma: "" }).some((a) => a.startsWith("--gamma")));
check("explicit gamma 0 still emits --gamma=0", buildArgs({ gamma: 0 }).includes("--gamma=0"));
check("numeric gamma string emits the number", buildArgs({ gamma: "2.2" }).includes("--gamma=2.2"));

// Non-finite junk is still refused by the validator and never becomes an argv token.
for (const [field, value] of [["disposal", "abc"], ["loopcount", "abc"],
  ["color_count", "1e999"], ["threads", "abc"]]) {
  const argv = buildArgs({ [field]: value });
  check(`${field}="${value}" never reaches argv`,
    !argv.some((a) => String(a).includes(value)), JSON.stringify(argv));
}

if (failures.length) {
  console.error(failures.map((f) => `FAIL ${f}`).join("\n"));
  process.exit(1);
}
console.log("ALL NUMERIC HONESTY TESTS PASSED");
