#!/usr/bin/env python3
"""S34: the decision table of scripts/ci_gate.sh, with a fake `gh` (no network, nothing written to the checkout).

Run: python3 working_code/gifscythe/tests/test_ci_gate.py
     GATE_UNDER_TEST=/path/to/another/ci_gate.sh python3 .../test_ci_gate.py     (the RED half of a mutation test)

The gate decides whether a `push` run may skip itself because the pull request's own run covers the
commit. Its two failure directions are not equally bad, and the table pins both:
  - a duplicate run (skip=false when a PR run exists) costs minutes;
  - a MISSING run (skip=true when no PR run will exist) costs the proof. That is the case for a PR with
    merge conflicts - GitHub starts no pull_request workflow for it - so the gate must run the push.
Every doubt (API error, gh missing, mergeable still null) therefore resolves to "run".
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

DEFAULT = Path(__file__).resolve().parents[1] / "scripts" / "ci_gate.sh"
GATE = Path(os.environ.get("GATE_UNDER_TEST", DEFAULT)).resolve()
BRANCH = "arena/01a0f7ed-gifscythe"
OWNER = "freeforall1932-design"
REPO = f"{OWNER}/gifscythe"

# `pulls?...head=` answers FAKE_PRS (one number per line; the word FAIL makes the call fail like a 404/403 does).
# `pulls/<n>` answers the next word of FAKE_MERGEABLE (space separated; the last word repeats), counted in a file.
FAKE_GH = r"""#!/usr/bin/env bash
echo "$*" >> "$FAKE_LOG"
case "$*" in
  "api repos/"*"/pulls?state=open&head="*)
    if [[ "${FAKE_PRS:-}" == "FAIL" ]]; then echo "HTTP 403" >&2; exit 1; fi
    [[ -n "${FAKE_PRS:-}" ]] && printf '%s\n' "$FAKE_PRS"
    exit 0 ;;
  "api repos/"*"/pulls/"*)
    n=$(cat "$FAKE_COUNT" 2>/dev/null || echo 0); n=$((n+1)); echo "$n" > "$FAKE_COUNT"
    read -r -a seq <<< "${FAKE_MERGEABLE:-null}"
    i=$((n-1)); [[ $i -ge ${#seq[@]} ]] && i=$((${#seq[@]}-1))
    if [[ "${seq[$i]}" == "ERROR" ]]; then exit 1; fi
    echo "${seq[$i]}"; exit 0 ;;
  *) echo "fake gh: unhandled call: $*" >&2; exit 2 ;;
esac
"""


class GateTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="gifscythe-gate-")
        self.addCleanup(self.temp.cleanup)
        self.dir = Path(self.temp.name)
        (self.dir / "bin").mkdir()
        gh = self.dir / "bin" / "gh"
        gh.write_text(FAKE_GH, encoding="utf-8")
        gh.chmod(0o755)

    def gate(self, *, event="push", ref=f"refs/heads/{BRANCH}", branch=BRANCH, prs="", mergeable="null",
             path_without_gh=False):
        out = self.dir / "github_output"
        out.write_text("", encoding="utf-8")
        log = self.dir / "gh_calls"
        log.write_text("", encoding="utf-8")
        count = self.dir / "mergeable_polls"
        count.unlink(missing_ok=True)
        base = "/usr/bin:/bin"
        empty = self.dir / "empty"  # a PATH with nothing on it: `command -v gh` must find nothing
        empty.mkdir(exist_ok=True)
        env = {
            "PATH": str(empty) if path_without_gh else f"{self.dir / 'bin'}:{base}",
            "EVENT": event, "REF": ref, "BRANCH": branch, "REPO": REPO, "OWNER": OWNER,
            "GS_GATE_SLEEP": "0", "GITHUB_OUTPUT": str(out),
            "FAKE_PRS": prs, "FAKE_MERGEABLE": mergeable, "FAKE_LOG": str(log), "FAKE_COUNT": str(count),
        }
        r = subprocess.run([shutil.which("bash"), str(GATE)], capture_output=True, text=True, env=env, timeout=60)
        self.assertEqual(r.returncode, 0, f"the gate must never fail: {r.stderr}")
        written = [line for line in out.read_text(encoding="utf-8").splitlines() if line]
        self.assertEqual(len(written), 1, f"exactly one output line expected, got {written}")
        self.assertIn(written[0], ("skip=true", "skip=false"))
        polls = int(count.read_text()) if count.exists() else 0
        return written[0], r.stdout, log.read_text(encoding="utf-8"), polls

    # --- always run -----------------------------------------------------------------------------
    def test_a_pull_request_event_always_runs_and_asks_nothing(self):
        verdict, _, calls, _ = self.gate(event="pull_request", ref="refs/pull/10/merge", prs="10", mergeable="true")
        self.assertEqual(verdict, "skip=false")
        self.assertEqual(calls, "")

    def test_a_push_to_main_always_runs(self):
        verdict, _, calls, _ = self.gate(ref="refs/heads/main", branch="main", prs="10", mergeable="true")
        self.assertEqual(verdict, "skip=false")
        self.assertEqual(calls, "")

    def test_a_tag_push_always_runs(self):
        verdict, _, _, _ = self.gate(ref="refs/tags/v1.0.0", branch="v1.0.0", prs="10", mergeable="true")
        self.assertEqual(verdict, "skip=false")

    def test_a_branch_with_no_open_pr_runs(self):
        verdict, out, _, polls = self.gate(prs="")
        self.assertEqual(verdict, "skip=false")
        self.assertIn("no open pull request", out)
        self.assertEqual(polls, 0)

    # --- skip: the PR run covers the commit ------------------------------------------------------
    def test_an_open_mergeable_pr_makes_the_push_run_skip(self):
        verdict, out, _, _ = self.gate(prs="10", mergeable="true")
        self.assertEqual(verdict, "skip=true")
        self.assertIn("PR #10 is open and mergeable", out)

    def test_the_query_names_the_owner_and_the_whole_branch_including_its_slash(self):
        _, _, calls, _ = self.gate(prs="10", mergeable="true")
        self.assertIn(f"head={OWNER}:{BRANCH}", calls)

    def test_mergeable_null_is_polled_until_github_has_computed_it(self):
        verdict, _, _, polls = self.gate(prs="10", mergeable="null null true")
        self.assertEqual(verdict, "skip=true")
        self.assertEqual(polls, 3)

    def test_with_two_open_prs_one_mergeable_pr_is_enough(self):
        verdict, _, _, _ = self.gate(prs="10\n11", mergeable="false true")
        self.assertEqual(verdict, "skip=true")

    # --- run: the PR run may not exist, or the gate cannot tell ---------------------------------------
    def test_a_conflicting_pr_has_no_pull_request_run_so_the_push_must_run(self):
        verdict, out, _, _ = self.gate(prs="10", mergeable="false")
        self.assertEqual(verdict, "skip=false")
        self.assertIn("mergeable=false", out)

    def test_mergeable_that_never_settles_runs(self):
        verdict, _, _, polls = self.gate(prs="10", mergeable="null")
        self.assertEqual(verdict, "skip=false")
        self.assertEqual(polls, 8)

    def test_an_api_error_listing_prs_runs(self):
        verdict, out, _, _ = self.gate(prs="FAIL")
        self.assertEqual(verdict, "skip=false")
        self.assertIn("could not list pull requests", out)

    def test_an_api_error_reading_the_pr_runs(self):
        verdict, _, _, _ = self.gate(prs="10", mergeable="ERROR")
        self.assertEqual(verdict, "skip=false")

    def test_gh_missing_runs(self):
        verdict, out, _, _ = self.gate(prs="10", mergeable="true", path_without_gh=True)
        self.assertEqual(verdict, "skip=false")
        self.assertIn("gh is not available", out)

    def test_it_also_works_without_a_github_output_file(self):
        env = {"PATH": f"{self.dir / 'bin'}:/usr/bin:/bin", "EVENT": "push", "REF": f"refs/heads/{BRANCH}",
               "BRANCH": BRANCH, "REPO": REPO, "OWNER": OWNER, "FAKE_PRS": "", "FAKE_LOG": str(self.dir / "l"),
               "FAKE_COUNT": str(self.dir / "c")}
        r = subprocess.run(["bash", str(GATE)], capture_output=True, text=True, env=env, timeout=60)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("skip=false", r.stdout)


if __name__ == "__main__":
    unittest.main(verbosity=2)
