#!/usr/bin/env python3
"""S34 (N-36): scripts/fixtures.sh rebuilds the upstream test images from text - including from a CRLF checkout.

Run: python3 working_code/gifscythe/tests/test_fixtures.py
     FIXTURES_UNDER_TEST=/path/to/another/fixtures.sh python3 ...   (the RED half of a mutation test)

The repo holds no image files (the owner removed them all), so two upstream test images live as base64 text
in tests/fixtures/*.b64 with their sha256 in SHA256SUMS. The first CI run of that change failed on Windows in
exactly the two steps that call fixtures.sh: Git for Windows checks text files out with CRLF unless
.gitattributes says otherwise, GNU `base64 -d` rejects a CR ("invalid input") and `read -r` keeps the CR as
part of the file name. The Actions log is unreadable from the agent sessions, so this reproduces that checkout
offline: the fixtures are rewritten with CRLF in a scratch tree and the script must still produce
byte-identical images. It also checks the checksum guard (a corrupted b64 must fail, never be accepted).
"""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

GS = Path(__file__).resolve().parents[1]
SCRIPT = Path(os.environ.get("FIXTURES_UNDER_TEST", GS / "scripts" / "fixtures.sh")).resolve()
FIXTURES = GS / "tests" / "fixtures"
BASH = shutil.which("bash")


def expected():
    return {n: h for h, n in (l.split() for l in (FIXTURES / "SHA256SUMS").read_text().splitlines() if l.strip())}


@unittest.skipUnless(BASH and shutil.which("base64") and shutil.which("sha256sum"), "bash, base64 and sha256sum are required")
class FixturesTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="gifscythe-fixtures-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "scripts").mkdir()
        (self.root / "tests" / "fixtures").mkdir(parents=True)
        shutil.copy2(SCRIPT, self.root / "scripts" / "fixtures.sh")

    def stage(self, crlf):
        for f in FIXTURES.iterdir():
            data = f.read_bytes()
            if crlf:
                data = data.replace(b"\n", b"\r\n")
            (self.root / "tests" / "fixtures" / f.name).write_bytes(data)

    def run_script(self, dest=None):
        dest = dest or self.root / "out"
        return subprocess.run([BASH, str(self.root / "scripts" / "fixtures.sh"), str(dest)], capture_output=True, text=True), Path(dest)

    def assert_identical(self, dest):
        for name, want in expected().items():
            self.assertTrue((dest / name).exists(), f"{name} was not written")
            self.assertEqual(hashlib.sha256((dest / name).read_bytes()).hexdigest(), want, name)

    def test_lf_checkout_rebuilds_byte_identical_images(self):
        self.stage(crlf=False)
        r, dest = self.run_script()
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(r.stdout.strip(), str(dest))
        self.assert_identical(dest)

    def test_crlf_checkout_rebuilds_byte_identical_images(self):
        # what a Windows runner hands us when .gitattributes does not pin LF
        self.stage(crlf=True)
        r, dest = self.run_script()
        self.assertEqual(r.returncode, 0, f"CRLF checkout broke it: {r.stderr}")
        self.assert_identical(dest)

    def test_a_second_run_changes_nothing(self):
        self.stage(crlf=False)
        r, dest = self.run_script()
        before = {n: (dest / n).stat().st_mtime_ns for n in expected()}
        r2, _ = self.run_script(dest)
        self.assertEqual(r2.returncode, 0, r2.stderr)
        self.assertEqual(before, {n: (dest / n).stat().st_mtime_ns for n in expected()}, "an up-to-date image was rewritten")

    def test_a_corrupted_text_fixture_fails_instead_of_being_accepted(self):
        self.stage(crlf=False)
        b64 = self.root / "tests" / "fixtures" / "logo.gif.b64"
        text = b64.read_text()
        b64.write_text(text[:100] + ("A" if text[100] != "A" else "B") + text[101:])  # flip one character
        r, dest = self.run_script()
        self.assertEqual(r.returncode, 1, r.stdout + r.stderr)
        self.assertIn("wrong bytes", r.stderr)
        self.assertFalse((dest / "logo.gif").exists(), "a mismatching image was left in place")

    def test_the_repo_ships_the_attribute_that_pins_lf(self):
        attrs = (GS.parents[1] / ".gitattributes").read_text()
        self.assertIn("working_code/gifscythe/tests/fixtures/* text eol=lf", attrs)


if __name__ == "__main__":
    unittest.main(verbosity=2)
