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

if (failures.length) {
  console.error(failures.map((f) => `FAIL ${f}`).join("\n"));
  process.exit(1);
}
console.log("ALL NUMERIC HONESTY TESTS PASSED");
