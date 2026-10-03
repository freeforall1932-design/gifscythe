#!/usr/bin/env python3
"""S34 (N-32): exercise `web/wasm/prove_wasm.mjs --oracle` without emcc.

Run: python3 working_code/gifscythe/tests/test_prove_wasm_oracle.py
     GS_TEST_ENGINE=/path/to/a/native/gifsicle python3 .../test_prove_wasm_oracle.py
     PROVE_UNDER_TEST=/path/to/an/older/prove_wasm.mjs python3 ...   (the RED half of a mutation test)

Why this exists. The owner's N-32 decision replaced the unpassable bar "wasm output is byte-equal to the
glibc oracle" with "byte-equal to a NATIVE build against the same libc family". prove_wasm.mjs is where
that bar is enforced for the Emscripten module - and no Emscripten module can be built in any sandbox
that has existed so far, so the comparison logic is exercised here with a fake module instead: a tiny
CommonJS factory with the same shape as emcc's MODULARIZE output (FS.writeFile/readFile, callMain, print)
that shells out to a real native gifsicle, exactly as web/wasm/glue_harness.mjs does for the page glue.
The fake proves the SCRIPT, never the wasm binary.

Needs node and a native gifsicle (./build.sh, or GS_TEST_ENGINE). Without either, every test is skipped
with that reason printed - a skip is not a pass. CI sets GS_REQUIRE_PROOF=1, which turns that skip into a
FAILURE: the first CI run of this test was green in 0 s with no way to tell from outside whether ten tests
had run or ten had been skipped, which is exactly how a proof rots.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

GS = Path(__file__).resolve().parents[1]
REPO = GS.parents[1]
PROVE = Path(os.environ.get("PROVE_UNDER_TEST", REPO / "web" / "wasm" / "prove_wasm.mjs")).resolve()
NODE = shutil.which("node")
REQUIRED = os.environ.get("GS_REQUIRE_PROOF") == "1"  # CI sets it: a skipped proof must never read as a passed one


def find_engine():
    if os.environ.get("GS_TEST_ENGINE"):
        p = Path(os.environ["GS_TEST_ENGINE"])
        return p if p.exists() else None
    found = sorted((GS / "release").glob("*/gifsicle"))
    return found[0] if found else None


ENGINE = find_engine()

# The factory shape emcc's -sMODULARIZE=1 build exports (module.exports = createGifsicle). callMain maps
# the virtual /in.gif and /out.gif onto a temp dir and runs the real engine there.
FAKE_MODULE = r"""
const { execFileSync } = require("node:child_process");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const ENGINE = process.env.FAKE_ENGINE;
const CORRUPT = process.env.FAKE_CORRUPT === "1";
module.exports = async function createGifsicle(opts) {
  const vfs = {};
  return {
    FS: {
      writeFile: (p, b) => { vfs[p] = Buffer.from(b); },
      readFile: (p) => { if (!vfs[p]) throw new Error("ENOENT " + p); return vfs[p]; },
    },
    callMain(args) {
      const dir = fs.mkdtempSync(path.join(os.tmpdir(), "fake-wasm-"));
      const real = (a) => (a === "/in.gif" ? path.join(dir, "in.gif") : a === "/out.gif" ? path.join(dir, "out.gif") : a);
      for (const v of ["/in.gif", "/out.gif"]) if (vfs[v]) fs.writeFileSync(real(v), vfs[v]);
      let res;
      try {
        res = execFileSync(ENGINE, args.map(real), { stdio: ["ignore", "pipe", "pipe"], encoding: "utf8" });
      } catch (e) {
        const err = new Error("exit"); err.status = e.status ?? 1; throw err;
      }
      for (const line of String(res).split("\n")) if (line) opts.print(line);
      if (fs.existsSync(real("/out.gif"))) {
        let out = fs.readFileSync(real("/out.gif"));
        if (CORRUPT && args.includes("-O3")) out = Buffer.concat([out, Buffer.from([0])]);  // one extra byte
        vfs["/out.gif"] = out;
      }
    },
  };
};
"""


class ProveWasmOracleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        missing = [what for what, ok in (("node", NODE), ("a native gifsicle (run ./build.sh or set GS_TEST_ENGINE)", ENGINE))
                   if not ok]
        if missing:
            msg = f"{' and '.join(missing)} not found - prove_wasm.mjs cannot run, nothing was tested"
            if REQUIRED:
                raise AssertionError(msg + " (GS_REQUIRE_PROOF=1 makes that a failure, not a skip)")
            raise unittest.SkipTest(msg)
        cls.temp = tempfile.TemporaryDirectory(prefix="gifscythe-prove-")
        cls.fake = Path(cls.temp.name) / "fake_gifsicle.js"
        cls.fake.write_text(FAKE_MODULE, encoding="utf-8")

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def prove(self, *args, corrupt=False):
        env = dict(os.environ, FAKE_ENGINE=str(ENGINE), FAKE_CORRUPT="1" if corrupt else "0")
        return subprocess.run([NODE, str(PROVE), *args], capture_output=True, text=True, env=env, timeout=120)

    def with_fake(self, *args, **kw):
        return self.prove("--module", str(self.fake), *args, **kw)

    def test_without_an_oracle_it_passes_and_says_it_compared_nothing(self):
        r = self.with_fake()
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("prove_wasm: PASS (non-empty GIF only)", r.stdout)
        self.assertIn("NOT that it matches native output", r.stdout)

    def test_output_equal_to_the_oracle_passes(self):
        r = self.with_fake("--oracle", str(ENGINE))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("output byte-equal", r.stdout)
        self.assertIn("prove_wasm: PASS (byte-equal to the same-libc oracle)", r.stdout)

    def test_oracle_equals_form_works_too(self):
        r = self.with_fake(f"--oracle={ENGINE}")
        self.assertEqual(r.returncode, 0, r.stderr)

    def test_one_extra_byte_fails_and_names_the_offset(self):
        r = self.with_fake("--oracle", str(ENGINE), corrupt=True)
        self.assertEqual(r.returncode, 1, r.stdout)
        self.assertIn("prove_wasm: FAIL: module output", r.stderr)
        self.assertIn("first difference at offset", r.stderr)
        self.assertIn("libc_parity.py --build-oracle", r.stderr)  # says how to get a valid oracle
        self.assertNotIn("PASS", r.stdout)

    def test_a_missing_oracle_fails_cleanly(self):
        r = self.with_fake("--oracle", "/nonexistent/gifsicle")
        self.assertEqual(r.returncode, 1, r.stdout)
        self.assertIn("cannot run the oracle", r.stderr)

    def test_an_oracle_that_exits_nonzero_fails(self):
        false = shutil.which("false")
        self.assertTrue(false, "no `false` binary")
        r = self.with_fake("--oracle", false)
        self.assertEqual(r.returncode, 1, r.stdout)
        self.assertIn("exited 1", r.stderr)

    def test_an_oracle_that_writes_nothing_is_not_an_oracle(self):
        true = shutil.which("true")
        self.assertTrue(true, "no `true` binary")
        r = self.with_fake("--oracle", true)
        self.assertEqual(r.returncode, 1, r.stdout)
        self.assertIn("wrote no output", r.stderr)

    def test_an_unknown_option_is_a_usage_error_not_an_input_path(self):
        r = self.with_fake("--nope")
        self.assertEqual(r.returncode, 2, r.stderr)
        self.assertIn("unknown option --nope", r.stderr)

    def test_oracle_without_a_value_is_a_usage_error(self):
        r = self.prove("--module", str(self.fake), "--oracle")
        self.assertEqual(r.returncode, 2, r.stderr)
        self.assertIn("--oracle needs a value", r.stderr)

    def test_a_missing_module_still_points_at_the_build_script(self):
        r = self.prove("--module", "/nonexistent/gifsicle.js")
        self.assertEqual(r.returncode, 1, r.stdout)
        self.assertIn("cannot load", r.stderr)
        self.assertIn("build_wasm.sh", r.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=2)
