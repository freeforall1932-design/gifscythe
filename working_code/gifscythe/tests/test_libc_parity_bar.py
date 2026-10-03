#!/usr/bin/env python3
"""S34 (N-32): the verdict logic of `libc_parity.py --bar`, with every build and run faked.

Run: python3 working_code/gifscythe/tests/test_libc_parity_bar.py        (no zig, node or Pillow needed)

`--bar` is the wasm track's proof bar: the wasm32-wasi build must be byte-equal to a native build of the
same sources against the same libc family, on every case. A proof gate is only worth having if it can
fail, and the classic way for one to rot is to pass for the wrong reason - so this pins the four ways
`bar_main` must NOT say OK:
  - the outputs differ,
  - both builds produced nothing (two empty results are "equal"),
  - both builds produced something that is not a GIF,
  - a tool the bar needs is missing (that is a SKIP, exit 3, never a smaller proof that still passes).
Real builds are covered by CI's `portability` job, which runs `libc_parity.py --bar` for real.
"""
import contextlib
import importlib.util
import io
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest import mock

PATH = Path(__file__).resolve().parents[1] / "scripts" / "libc_parity" / "libc_parity.py"
spec = importlib.util.spec_from_file_location("libc_parity_under_test", PATH)
lp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(lp)

GIF = b"GIF89a" + bytes(range(40))


class BarTests(unittest.TestCase):
    def run_bar(self, produce, *, node=True, pillow=True, against="musl"):
        """produce(tag, index) -> bytes to write as that build's output for that case, or None for no output."""
        keep = tempfile.TemporaryDirectory(prefix="gifscythe-bar-")
        self.addCleanup(keep.cleanup)

        def fake_build(zig, work, name, target, flavour, libs, stable, objs, version):
            return Path(work) / f"engine_{name}"

        def fake_run_case(exe, tag, i, inp, args, work):
            data = produce(tag, i)
            if data is None:
                return None
            out = Path(work) / f"out_{tag}_{i}.gif"
            out.write_bytes(data)
            return out

        ns = types.SimpleNamespace(against=against, keep=keep.name)
        out = io.StringIO()
        which = (lambda name: "/usr/bin/node") if node else (lambda name: None)
        pil = {"PIL": types.ModuleType("PIL")} if pillow else {"PIL": None}  # None makes `import PIL` raise
        with mock.patch.object(lp, "build", fake_build), mock.patch.object(lp, "run_case", fake_run_case), \
                mock.patch.object(lp, "make_inputs", lambda work: True), \
                mock.patch.object(lp.shutil, "which", which), mock.patch.dict(sys.modules, pil), \
                contextlib.redirect_stdout(out):
            code = lp.bar_main(ns, ["zig"], ["objs"], "1.96")
        return code, out.getvalue()

    def test_identical_gifs_everywhere_is_ok(self):
        code, out = self.run_bar(lambda tag, i: GIF + bytes([i]))
        self.assertEqual(code, 0, out)
        self.assertIn(f"BAR OK: wasm32-wasi == musl-native, byte for byte, on {len(lp.CASES)}/{len(lp.CASES)}", out)

    def test_one_differing_case_fails_and_is_named(self):
        def produce(tag, i):
            return GIF + (b"x" if tag == "wasm" and i == 3 else b"y")

        code, out = self.run_bar(produce)
        self.assertEqual(code, 1, out)
        self.assertIn(f"BAR FAILED: wasm32-wasi != musl-native on 1 of {len(lp.CASES)}", out)
        self.assertIn(f"{lp.CASES[3][0]} (bytes differ)", out)

    def test_two_builds_that_produce_nothing_do_not_match(self):
        code, out = self.run_bar(lambda tag, i: None)
        self.assertEqual(code, 1, out)
        self.assertIn("no GIF produced", out)
        self.assertNotIn("BAR OK", out)

    def test_two_equal_non_gifs_do_not_match(self):
        code, out = self.run_bar(lambda tag, i: b"not a gif at all")
        self.assertEqual(code, 1, out)
        self.assertIn("no GIF produced", out)

    def test_only_the_wasm_side_empty_fails(self):
        code, out = self.run_bar(lambda tag, i: None if tag == "wasm" else GIF)
        self.assertEqual(code, 1, out)

    def test_missing_node_is_a_skip_not_a_pass(self):
        code, out = self.run_bar(lambda tag, i: GIF, node=False)
        self.assertEqual(code, 3, out)
        self.assertIn("SKIP: --bar needs node", out)

    def test_missing_pillow_is_a_skip_not_a_pass(self):
        code, out = self.run_bar(lambda tag, i: GIF, pillow=False)
        self.assertEqual(code, 3, out)
        self.assertIn("Pillow", out)

    def test_against_glibc_is_a_different_comparison_and_can_fail(self):
        # the bar the docs used to state: the native side is the glibc build
        def produce(tag, i):
            return GIF + (b"g" if tag == "glibc" and i % 2 else b"w")

        code, out = self.run_bar(produce, against="glibc")
        self.assertEqual(code, 1, out)
        self.assertIn("wasm32-wasi != glibc-native", out)


if __name__ == "__main__":
    unittest.main(verbosity=2)
