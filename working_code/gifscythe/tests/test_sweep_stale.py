#!/usr/bin/env python3
"""N-07/P2-15: exercise the real sweep in an isolated, tracked Markdown corpus.

Run: python3 working_code/gifscythe/tests/test_sweep_stale.py
No mutations, git configuration changes, or generated files in the real checkout.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


SWEEP = Path(__file__).resolve().parents[1] / "scripts" / "sweep_stale.sh"


class SweepTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="gifscythe-sweep-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.env = {k: v for k, v in os.environ.items() if not k.startswith("GIT_")}
        self.script = self.root / "working_code/gifscythe/scripts/sweep_stale.sh"
        self.script.parent.mkdir(parents=True)
        shutil.copy2(SWEEP, self.script)
        subprocess.run(["git", "init", "-q", str(self.root)], env=self.env, check=True)
        self.write(".github/workflows/build.yml", "same\n")
        self.write("docs/ci/build.yml.proposed", "same\n")
        self.write("COMPILED_AUDIT.md", "# Empty fixture audit\n")
        self.counts(0)

    def write(self, name, text, tracked=True):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        if tracked:
            subprocess.run(["git", "add", "--", name], cwd=self.root,
                           env=self.env, check=True)

    def counts(self, count):
        self.write("STATUS.md", "**Counts (generated - do not edit by hand):** "
                   f"82 DONE · 5 PARTIAL · 35 OPEN · {count} UNTRIAGED · {122 + count} total\n")

    def sweep(self, fail=False, report=False, location=None, rule="S2"):
        before = {p: p.read_bytes() for p in self.root.rglob("*.md")}
        result = subprocess.run(["bash", str(self.script)] + (["--report"] if report else []),
                                cwd=self.root, env=self.env, text=True,
                                capture_output=True, timeout=10)
        self.assertEqual(result.returncode, 0 if report or not fail else 1,
                         result.stdout + result.stderr)
        self.assertIn(f"FAIL [{rule}]" if fail else f"PASS [{rule}]", result.stdout)
        if location:
            self.assertIn(location, result.stdout)
        self.assertEqual(before, {p: p.read_bytes() for p in self.root.rglob("*.md")})
        return result.stdout

    def test_matching_count(self):
        self.write("README.md", "There are 0 UNTRIAGED findings.\n")
        self.sweep()

    def test_stale_standalone_count(self):
        self.write("README.md", "# Work\n\nThere are 18 UNTRIAGED findings.\n")
        self.sweep(fail=True, location="README.md:3 quotes 18 UNTRIAGED (register says 0)")

    def test_markdown_and_wrapping(self):
        for claim in ("**18 UNTRIAGED**", "18 `UNTRIAGED`", "**18** **UNTRIAGED**",
                      "18\nUNTRIAGED", "**18**\n`UNTRIAGED`", "18\tUNTRIAGED"):
            with self.subTest(claim=claim):
                self.write("README.md", "# Work\n\n" + claim + " findings.\n")
                self.sweep(fail=True, location="README.md:3 quotes 18 UNTRIAGED")

    def test_no_cross_paragraph_or_partial_word_match(self):
        self.write("README.md", "There are 18\n\nUNTRIAGED is a state.\n\n"
                   "18 UNTRIAGEDness; ticket18 UNTRIAGED; 18UNTRIAGED.\n")
        self.sweep()

    def test_matching_nonzero_and_leading_zero(self):
        self.counts(18)
        self.write("README.md", "18 `UNTRIAGED` findings; 018 UNTRIAGED findings.\n")
        self.sweep()

    def test_reference_changes_not_hardcoded_zero(self):
        self.counts(2)
        self.write("README.md", "0 UNTRIAGED findings.\n")
        self.sweep(fail=True, location="register says 2")

    def test_multiple_claims_and_line_locations(self):
        self.write("README.md", "# Work\n\n0 UNTRIAGED; 2 UNTRIAGED.\n"
                   "3 UNTRIAGED.\n\n4\nUNTRIAGED.\n")
        output = self.sweep(fail=True)
        for line, count in ((3, 2), (4, 3), (6, 4)):
            self.assertIn(f"README.md:{line} quotes {count} UNTRIAGED", output)

    def test_count_at_start_of_wrapped_line(self):
        self.write("README.md", "# Work\n\nThese findings:\n18 UNTRIAGED.\n")
        self.sweep(fail=True, location="README.md:4 quotes 18 UNTRIAGED")

    def test_historical_corpus_exclusions(self):
        for path in ("IMPROVEMENT_LOG.md", "docs/archive/old.md", "reference_code/README.md",
                     "docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12.md"):
            self.write(path, "18 UNTRIAGED findings.\n")
        self.sweep()

    def test_date_does_not_exempt_live_count(self):
        self.write("README.md", "As of S14 (2026-09-12): 18 UNTRIAGED findings.\n")
        self.sweep(fail=True)

    def test_untracked_docs_ignored(self):
        self.write("scratch.md", "18 UNTRIAGED findings.\n", tracked=False)
        self.sweep()

    def test_full_tally_and_total_regressions(self):
        for tally in ("81 DONE · 5 PARTIAL · 35 OPEN · 0 UNTRIAGED · 122 total",
                      "82 DONE · 5 PARTIAL · 35 OPEN · 0 UNTRIAGED · 123 total",
                      "82 DONE · 5 PARTIAL ·\n35 OPEN · 18 UNTRIAGED · 140 total"):
            with self.subTest(tally=tally):
                self.write("README.md", tally + "\n")
                self.sweep(fail=True)

    def test_malformed_reference_fails_closed(self):
        self.write("STATUS.md", "**Counts:** missing\n")
        self.sweep(fail=True, location="STATUS.md has no valid UNTRIAGED count")

    def test_report_mode_reports_but_succeeds(self):
        self.write("README.md", "18 UNTRIAGED findings.\n")
        self.sweep(fail=True, report=True)

    def narrative(self, text, glyph="✅"):
        self.write("COMPILED_AUDIT.md", "## 3. Narrative\n" + text +
                   f"\n\n## 5. Register\n| **U-04** | proof | {glyph} state |\n## 6. Tasks\n")

    def test_open_narrative_fixed_register(self):
        for marker in ("⬜ **OPEN**", "OPEN", "**OPEN**"):
            with self.subTest(marker=marker):
                self.narrative(f"**Status:** {marker} — register §5 `U-04`.")
                self.sweep(fail=True, rule="S5", location="COMPILED_AUDIT.md:2: U-04 narrative OPEN")

    def test_fixed_narrative_open_register(self):
        self.narrative("**Status:** ✅ **FIXED** — register §5 `U-04`.", "⬜")
        self.sweep(fail=True, rule="S5")

    def test_historical_open_and_unrelated_ids_are_not_current(self):
        self.narrative("**Status:** ✅ **FIXED** — register §5 `U-04`. "
                       "Original report: ⬜ OPEN — U-99 was broken.")
        self.sweep(rule="S5")

    def test_matching_open_and_partial_are_allowed(self):
        for state, glyph in (("OPEN", "⬜"), ("PARTIAL", "◐")):
            self.narrative(f"**Status:** {state} — register §5 `U-04`. resolved in part.", glyph)
            self.sweep(rule="S5")

    def test_uncheckable_current_claim_fails(self):
        self.narrative("**Status:** OPEN. Original report: U-04")
        self.sweep(fail=True, rule="S5", location="uncheckable current status")

    def test_wrapped_claim_and_report_only(self):
        self.narrative("**Status:** ⬜ **OPEN** —\nregister §5 `U-04`.")
        self.sweep(fail=True, report=True, rule="S5")


if __name__ == "__main__":
    unittest.main(verbosity=2)
