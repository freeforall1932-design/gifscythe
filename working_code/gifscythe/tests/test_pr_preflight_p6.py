#!/usr/bin/env python3
"""S34: exercise P6 of pr_preflight.sh (the "Docs synced through" check) in an isolated temp repo.

Run: python3 working_code/gifscythe/tests/test_pr_preflight_p6.py
     PREFLIGHT_UNDER_TEST=/path/to/another/pr_preflight.sh python3 .../test_pr_preflight_p6.py
     (the second form is how the RED half of a mutation test is run against an older copy)

Why P6 has this shape. The owner merges from the GitHub UI and continues, so nothing may be
left to edit after the merge. A PR therefore *pre-syncs itself*: right after `gh pr create` it
moves the handoff line to its own number. The number is true from the merge on, and the PR
already carries the write-up. P6 must accept that claim for the OPEN PR of the branch being
checked and for nothing else: any other number that has not merged is still a false claim,
and a merged PR that the line does not name still means the docs are behind.

No real gh, no network, and nothing is written into the real checkout.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

DEFAULT = Path(__file__).resolve().parents[1] / "scripts" / "pr_preflight.sh"
PREFLIGHT = Path(os.environ.get("PREFLIGHT_UNDER_TEST", DEFAULT)).resolve()
BRANCH = "arena/01a0f7ed-gifscythe"

# The preflight only ever asks gh a handful of questions. Each answer comes from the environment
# so a scenario is data, not code. `pr view` fails (exit 1) when FAKE_PR_VIEW is empty, exactly like
# gh does for a number that does not exist.
FAKE_GH = r"""#!/usr/bin/env bash
case "$*" in
  "auth status"*) exit 0 ;;
  "repo view"*) echo "owner/repo" ;;
  "api "*) echo "deadbee" ;;
  "run list"*) echo "1 success" ;;
  "pr list --head"*) echo "#10 OPEN - fake" ;;
  "pr list --state merged"*) echo "${FAKE_MERGED}" ;;
  "pr view "*) if [[ -n "${FAKE_PR_VIEW:-}" ]]; then echo "${FAKE_PR_VIEW}"; else echo "no such PR" >&2; exit 1; fi ;;
  *) echo "fake gh: unhandled call: $*" >&2; exit 2 ;;
esac
"""

STUB = "#!/usr/bin/env bash\necho '==> Done. stub'\nexit 0\n"


class P6Tests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="gifscythe-p6-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        scripts = self.root / "working_code/gifscythe/scripts"
        scripts.mkdir(parents=True)
        shutil.copy2(PREFLIGHT, scripts / "pr_preflight.sh")
        for name in ("check_docs.sh", "sweep_stale.sh"):  # P1/P2 are not under test here
            (scripts / name).write_text(STUB, encoding="utf-8")
            (scripts / name).chmod(0o755)
        bindir = self.root / "bin"
        bindir.mkdir()
        (bindir / "gh").write_text(FAKE_GH, encoding="utf-8")
        (bindir / "gh").chmod(0o755)
        self.env = {k: v for k, v in os.environ.items() if not k.startswith("GIT_")}
        self.env["PATH"] = f"{bindir}{os.pathsep}{self.env['PATH']}"
        git = lambda *a: subprocess.run(["git", "-c", "user.name=t", "-c", "user.email=t@t", *a],
                                        cwd=self.root, env=self.env, check=True,
                                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        git("init", "-q", "-b", BRANCH)
        git("commit", "-q", "--allow-empty", "-m", "init")

    def p6(self, synced, merged, view=""):
        """Return (the P6 verdict line, the whole report) for a handoff synced through PR `synced`,
        a newest merged PR `merged`, and `gh pr view <synced>` answering `view`."""
        (self.root / "SESSION_HANDOFF.md").write_text(
            f"# Session Handoff\n\n**Docs synced through:** PR #{synced} · branch `{BRANCH}` · merged as `abc1234`\n",
            encoding="utf-8")
        env = dict(self.env,
                   FAKE_MERGED=f"{merged} arena/other-branch abc1234",
                   FAKE_PR_VIEW=view)
        out = subprocess.run(["bash", "working_code/gifscythe/scripts/pr_preflight.sh", "--online"],
                             cwd=self.root, env=env, capture_output=True, text=True).stdout
        lines = [l for l in out.splitlines() if "[P6]" in l]
        self.assertEqual(len(lines), 1, f"expected exactly one P6 verdict line, got {lines!r}\n{out}")
        return lines[0].strip(), out

    # ---- the behaviour that existed before S34 and must not move --------------------------------
    def test_synced_through_the_newest_merged_pr_passes(self):
        verdict, _ = self.p6(synced=9, merged=9)
        self.assertTrue(verdict.startswith("PASS [P6] handoff is synced through PR #9"), verdict)

    def test_a_merge_the_line_does_not_name_means_docs_are_behind(self):
        verdict, _ = self.p6(synced=8, merged=9)
        self.assertTrue(verdict.startswith("FAIL [P6] docs are BEHIND"), verdict)

    # ---- the pre-sync: allowed for the branch's own open PR, and only for it ----------------------
    def test_own_open_pr_may_be_claimed_before_it_merges(self):
        verdict, _ = self.p6(synced=10, merged=9, view=f"OPEN {BRANCH}")
        self.assertTrue(verdict.startswith("PASS [P6] handoff is pre-synced through PR #10"), verdict)

    def test_the_same_claim_passes_the_normal_way_once_merged(self):
        verdict, _ = self.p6(synced=10, merged=10, view=f"MERGED {BRANCH}")
        self.assertTrue(verdict.startswith("PASS [P6] handoff is synced through PR #10"), verdict)

    def test_someone_elses_open_pr_may_not_be_claimed(self):
        verdict, _ = self.p6(synced=10, merged=9, view="OPEN arena/another-session")
        self.assertTrue(verdict.startswith("FAIL [P6] handoff claims to be synced through PR #10"), verdict)

    def test_a_closed_unmerged_pr_may_not_be_claimed(self):
        verdict, _ = self.p6(synced=10, merged=9, view=f"CLOSED {BRANCH}")
        self.assertTrue(verdict.startswith("FAIL [P6] handoff claims to be synced through PR #10"), verdict)

    def test_a_pr_that_does_not_exist_may_not_be_claimed(self):
        verdict, _ = self.p6(synced=10, merged=9, view="")
        self.assertTrue(verdict.startswith("FAIL [P6] handoff claims to be synced through PR #10"), verdict)

    def test_pre_sync_never_hides_a_merge_the_docs_do_not_describe(self):
        # PR #10 is this branch's own open PR, but PR #11 merged meanwhile and the docs say nothing of it.
        verdict, _ = self.p6(synced=10, merged=11, view=f"OPEN {BRANCH}")
        self.assertTrue(verdict.startswith("FAIL [P6] docs are BEHIND"), verdict)


if __name__ == "__main__":
    unittest.main(verbosity=2)
