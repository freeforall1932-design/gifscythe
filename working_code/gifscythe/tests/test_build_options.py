#!/usr/bin/env python3
"""GS-210: invalid arguments/help must be side-effect-free, before any tools run."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

PRODUCT = pathlib.Path(__file__).resolve().parents[1]


class BuildOptions(unittest.TestCase):
    def test_entry_points(self):
        for script in ("build.sh", "scripts/build_engine.sh"):
            for args, rc in ((["--typo"], 2), (["--windows", "--typo"], 2),
                             (["--help", "--typo"], 2), (["operand"], 2),
                             (["--help"], 0), (["-h"], 0)):
                with self.subTest(script=script, args=args), tempfile.TemporaryDirectory() as tmp:
                    root = pathlib.Path(tmp)
                    dest = root / "working_code/gifscythe" / script
                    dest.parent.mkdir(parents=True)
                    shutil.copy2(PRODUCT / script, dest)
                    # No VERSION, compiler, source/config or hooks required: parsing
                    # must finish before trying to consume any of those resources.
                    before = sorted(str(p.relative_to(root)) for p in root.rglob("*"))
                    r = subprocess.run(["bash", str(dest), *args], capture_output=True, text=True)
                    self.assertEqual(r.returncode, rc, r.stdout + r.stderr)
                    self.assertIn("unknown argument" if rc else "usage:", r.stderr if rc else r.stdout)
                    self.assertEqual(before, sorted(str(p.relative_to(root)) for p in root.rglob("*")))


if __name__ == "__main__":
    unittest.main(verbosity=2)
