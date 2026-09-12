// command.test.mjs — parity check: the JS command builder (web) must emit the
// exact same gifsicle command line as the C++ GifsicleCommand used by the
// desktop app + CLI. For each fixture we (1) serialize settings to a conf with
// saveSettingsLines, (2) run the real C++ `gifscythe-cli` in print mode with
// `--engine /opt/gifsicle`, and (3) compare its command line to the JS toString().
//
// Run:  node web/test/command.test.mjs   (after ./build.sh)

import { execFileSync } from "node:child_process";
import { mkdtempSync, writeFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import { dirname } from "node:path";
import { toString, buildArgs, saveSettingsLines } from "../command.mjs";

const __dirname = dirname(fileURLToPath(import.meta.url));
const PRODUCT = join(__dirname, "..", "..", "working_code", "gifscythe");
const CLI = join(PRODUCT, "build", "gifscythe-cli");
const ENGINE = "/opt/gifsicle"; // stable, unquoted, safe prefix in print mode

const fixtures = [
  {
    name: "empty comment is skipped (audit U-48)",
    s: {
      mode: "auto", comments: ["", "real one"],
      inputs: ["/tmp/parity/in.gif"], output: "/tmp/parity/out.gif",
    },
  },
  {
    name: "defaults+input",
    s: { mode: "auto", inputs: ["/tmp/parity/in.gif"] },
  },
  {
    name: "merge optimize lossy loop delay",
    s: {
      mode: "merge", optimize_level: 3, lossy: 40, loopcount: 0, delay_cs: 5,
      inputs: ["/tmp/parity/a.gif", "/tmp/parity/b.gif"], output: "/tmp/parity/out.gif",
    },
  },
  {
    name: "resize fit + colors + dither bare -f",
    s: {
      mode: "auto", resize_kind: "fit", resize_w: 320, resize_h: 200,
      color_count: 128, dither: true, inputs: ["/tmp/parity/in.gif"],
    },
  },
  {
    name: "named dither + color method + careful",
    s: {
      mode: "auto", dither_method: "ro64", color_method: "median-cut", careful: true,
      inputs: ["/tmp/parity/in.gif"],
    },
  },
  {
    name: "crop plus-form + transparency + bg + transparent",
    s: {
      mode: "auto", crop: true, crop_x: 1, crop_y: 2, crop_w: 30, crop_h: 40,
      crop_transparency: true, background: "#ffffff", transparent: "#000000",
      inputs: ["/tmp/parity/in.gif"],
    },
  },
  {
    name: "geometry rotate flip position interlace",
    s: {
      mode: "auto", rotation: "r90", flip_horizontal: true, flip_vertical: true,
      has_position: true, position_x: 5, position_y: 6, interlace: true,
      inputs: ["/tmp/parity/in.gif"],
    },
  },
  {
    name: "scale percent + resize method",
    s: {
      mode: "auto", resize_kind: "scale", scale_x: 0.5, scale_y: 0.5,
      resize_method: "lanczos3", inputs: ["/tmp/parity/in.gif"],
    },
  },
  {
    // Audit U-42: the web UI used to drive both axes from one "Scale %" input.
    // The builders always supported independent factors; this fixture pins the
    // asymmetric command shape so a regression to a shared factor is visible.
    name: "asymmetric scale X/Y (audit U-42)",
    s: {
      mode: "auto", resize_kind: "scale", scale_x: 0.5, scale_y: 2,
      inputs: ["/tmp/parity/in.gif"],
    },
  },
  {
    name: "gamma named + disposal + threads + unoptimize",
    s: {
      mode: "auto", gamma_str: "oklab", disposal: 2, threads: 2, unoptimize: true,
      inputs: ["/tmp/parity/in.gif"],
    },
  },
  {
    name: "loop N + -O0 + explode by name",
    s: {
      mode: "explode", explode_by_name: true, optimize_level: 0, loopcount: 7,
      inputs: ["/tmp/parity/in.gif"],
    },
  },
  {
    name: "metadata removals + comment with space",
    s: {
      mode: "auto", remove_comments: true, remove_names: true, remove_extensions: true,
      comments: ["hello world"], inputs: ["/tmp/parity/in.gif"],
    },
  },
  {
    name: "space paths quoting",
    s: {
      mode: "merge", optimize_level: 2,
      inputs: ["/tmp/my vacation/in.gif", "/tmp/my vacation/in2.gif"],
      output: "/tmp/my vacation/out.gif",
    },
  },
  {
    name: "loopcount forever + -O1 bare form",
    s: {
      mode: "auto", loopcount: 0, optimize_level: 1, inputs: ["/tmp/parity/in.gif"],
    },
  },
  {
    // Audit U-41 (P2-11): the web UI now offers every mode. The builders were
    // always mode-generic; these two fixtures pin the parity of the argv
    // shapes the old single-mode UI never emitted end-to-end. (Note: the
    // desktop BATCH runner executes one Auto command per file — this fixture
    // pins the -b BUILDER line itself, which the conf/print path shares.)
    name: "batch mode emits -b (audit U-41)",
    s: {
      mode: "batch", optimize_level: 2,
      inputs: ["/tmp/parity/a.gif", "/tmp/parity/b.gif"],
      output: "/tmp/parity/outprefix",
    },
  },
  {
    name: "explode numeric frames with prefix (audit U-41)",
    s: {
      mode: "explode", optimize_level: 2,
      inputs: ["/tmp/parity/in.gif"], output: "/tmp/parity/frames/p",
    },
  },
];

let failures = 0;

for (const fx of fixtures) {
  const dir = mkdtempSync(join(tmpdir(), "gsparity-"));
  try {
    const conf = join(dir, "case.conf");
    writeFileSync(conf, saveSettingsLines(fx.s));
    const out = execFileSync(CLI, [conf, "--engine", ENGINE], { encoding: "utf8" });
    const lines = out.split("\n");
    const cmdLine = lines[1] || "";
    const prefix = ENGINE + " ";
    if (!cmdLine.startsWith(prefix)) {
      console.log(`FAIL ${fx.name}: unexpected CLI output line: ${JSON.stringify(cmdLine)}`);
      failures++;
      continue;
    }
    const cpp = cmdLine.slice(prefix.length);
    const js = toString(fx.s);
    if (cpp === js) {
      console.log(`PASS ${fx.name}`);
    } else {
      console.log(`FAIL ${fx.name}`);
      console.log(`  cpp: ${cpp}`);
      console.log(`  js : ${js}`);
      failures++;
    }
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
}

// Direct argv sanity (no C++ needed): order + key flag forms.
const direct = {
  mode: "auto", optimize_level: 1, loopcount: 0, delay_cs: 5, crop: true,
  crop_x: 1, crop_y: 2, crop_w: 30, crop_h: 40, inputs: ["in.gif"], output: "out.gif",
};
const argv = buildArgs(direct);
const checks = [
  argv.includes("-O"), argv.includes("--loopcount=0"),
  argv.includes("--crop") && argv.includes("1,2+30x40"),
  argv.includes("-d") && argv.includes("5"),
  argv[argv.length - 2] === "-o" && argv[argv.length - 1] === "out.gif",
];
for (const c of checks) if (!c) failures++;
console.log(checks.every(Boolean) ? "PASS direct argv sanity" : "FAIL direct argv sanity");

if (failures === 0) {
  console.log("ALL WEB COMMAND TESTS PASSED");
  process.exit(0);
}
console.log(`${failures} WEB COMMAND TEST(S) FAILED`);
process.exit(1);
