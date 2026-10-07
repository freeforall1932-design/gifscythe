#!/usr/bin/env python3
"""GS-210: invalid arguments/help must be side-effect-free, before any tools run."""
import pathlib
import re
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


def read_version_md(text):
    """The product version, as build.sh itself extracts it from VERSION.md."""
    m = re.search(r"Current version:.*?(\d+\.\d+\.\d+)", text)
    return m.group(1) if m else None


def read_pro_version(text):
    """The VERSION qmake would use, as written in gifscythe.pro."""
    m = re.search(r"^VERSION\s*=\s*(\S+)\s*$", text, re.MULTILINE)
    return m.group(1) if m else None


def version_pair_agrees(version_md_text, pro_text):
    """GS-210: the .pro's VERSION must equal VERSION.md.

    Kept as a pure function of the two texts so the guard can be SHOWN to bite:
    the case below feeds it a deliberately drifted pair as well as the real one.
    """
    return read_version_md(version_md_text) == read_pro_version(pro_text)


class GuiBuilderDispatch(unittest.TestCase):
    """GS-210: CMake is the preferred GUI path; qmake is the fallback.

    Behavioural, not a grep for the line: build.sh is SOURCED (its execution
    guard makes a source inert), the two builder functions are replaced by
    recorders, and the real build_gui() dispatcher is exercised. A grep could be
    satisfied by moving code around; this cannot.
    """

    def _dispatch(self, cmake_ok, qmake_ok):
        driver = (
            'source "%s/build.sh"\n'
            'log=()\n'
            'build_gui_cmake() { log+=("cmake"); return %d; }\n'
            'build_gui_qmake() { log+=("qmake"); return %d; }\n'
            'rc=0; build_gui || rc=$?\n'
            'echo "order=${log[*]} rc=$rc"\n'
        ) % (PRODUCT, 0 if cmake_ok else 1, 0 if qmake_ok else 1)
        r = subprocess.run(["bash", "-c", driver], capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        return r.stdout.strip()

    def test_cmake_is_tried_first_and_short_circuits(self):
        # Both available: CMake must win and qmake must never run. This is the
        # exact behaviour the old dispatch got wrong (it ran qmake first).
        self.assertEqual(self._dispatch(True, True), "order=cmake rc=0")

    def test_qmake_is_the_fallback(self):
        self.assertEqual(self._dispatch(False, True), "order=cmake qmake rc=0")

    def test_both_failing_reports_failure(self):
        self.assertEqual(self._dispatch(False, False), "order=cmake qmake rc=1")

    def test_sourcing_build_sh_has_no_side_effects(self):
        # The test seam is only safe if sourcing really is inert: no build/ dir,
        # no version.h write, no hook bootstrap.
        with tempfile.TemporaryDirectory() as tmp:
            before = sorted(p.name for p in PRODUCT.iterdir())
            r = subprocess.run(["bash", "-c", 'source "%s/build.sh"' % PRODUCT],
                               capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            self.assertEqual(before, sorted(p.name for p in PRODUCT.iterdir()))


class WindowsGuiSubsystem(unittest.TestCase):
    """The desktop app and its qmake fallback must not create a console window."""

    def test_gui_targets_are_not_console_subsystem(self):
        cmake = (PRODUCT / "CMakeLists.txt").read_text()
        pro = (PRODUCT / "gifscythe.pro").read_text()
        main_window = (PRODUCT / "src/qtui/MainWindow.cpp").read_text()
        self.assertIn("qt_add_executable(gifscythe WIN32", cmake)
        self.assertIn("win32:CONFIG += windows", pro)
        self.assertIn("CREATE_NO_WINDOW", main_window)
        self.assertIn("STARTF_USESHOWWINDOW", main_window)
        self.assertIn("SW_HIDE", main_window)


class QmakeProjectVersion(unittest.TestCase):
    """GS-210: the .pro's hardcoded VERSION is guarded against VERSION.md drift.

    The .pro cannot be validated by running qmake here (no qmake in this sandbox,
    and the .pro is explicitly the secondary path), so the honest closure is the
    parity guard: duplication that cannot drift silently. The mutation leg below
    is what proves the guard has teeth.
    """

    def test_real_files_agree(self):
        self.assertTrue(version_pair_agrees(
            (PRODUCT / "VERSION.md").read_text(),
            (PRODUCT / "gifscythe.pro").read_text()),
            "gifscythe.pro VERSION does not match VERSION.md")

    def test_guard_bites_on_drift(self):
        real_md = (PRODUCT / "VERSION.md").read_text()
        real_pro = (PRODUCT / "gifscythe.pro").read_text()
        # Same .pro, bumped version doc -> must be seen as disagreement.
        drifted_md = re.sub(r"Current version:\*\*\s*\d+\.\d+\.\d+",
                            "Current version:** 9.9.9", real_md)
        self.assertNotEqual(drifted_md, real_md, "mutation did not change VERSION.md")
        self.assertFalse(version_pair_agrees(drifted_md, real_pro))
        # Same doc, bumped .pro -> likewise.
        drifted_pro = re.sub(r"(?m)^VERSION\s*=\s*\S+", "VERSION = 9.9.9", real_pro)
        self.assertNotEqual(drifted_pro, real_pro, "mutation did not change gifscythe.pro")
        self.assertFalse(version_pair_agrees(real_md, drifted_pro))


if __name__ == "__main__":
    unittest.main(verbosity=2)
