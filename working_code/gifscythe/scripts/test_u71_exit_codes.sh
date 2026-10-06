#!/usr/bin/env bash
# test_u71_exit_codes.sh — audit U-71 / fix-order P2-17.
#
# Proves the Windows exit-code classifier in src/core/ProcessRunner.h can
# never alias an NTSTATUS crash to success. The old `code & 0xff` mask made
# 0xC0000100 indistinguishable from exit 0 ("success" for a crash whose low
# byte is zero) and reported an access violation (0xC0000005) as the
# ordinary engine failure "exit 5".
#
# Legs:
#   S1  SENTINEL — refuses to pass while the old mask line exists in
#       ProcessRunner.h. On an unpatched tree THIS leg is the required
#       failing-first state.
#   S2  SEMANTIC — compiles and runs a probe: 11 raw-code table rows plus
#       a sweep of the whole 0xC0000000..0xC000FFFF band asserting that no
#       non-zero raw code delivers exit 0.
#   S3  MUTATION — the same probe recompiled with the classifier routed
#       through the OLD mask must FAIL; if the mutant passes, the table
#       has no teeth.
#
# Needs only a C++17 compiler — no Qt, no Windows: the classifier is
# deliberately platform-pure. Linux CI, macOS, the 32-bit-long portability
# target, and Windows Git Bash + MinGW all run the same script. Honours
# CXX with arguments, e.g.
#   CXX="python -m ziglang c++" ./scripts/test_u71_exit_codes.sh

set -euo pipefail
cd "$(dirname "$0")/.."
HDR=src/core/ProcessRunner.h
CXX="${CXX:-c++}"
say() { printf '%s\n' "$*"; }

if [ ! -f "$HDR" ]; then
  say "FAIL: $HDR not found — this script lives in working_code/gifscythe/scripts/"
  exit 2
fi

# --- S1 sentinel ------------------------------------------------------------
if grep -n 'static_cast<int>(code) & 0xff' "$HDR" >/dev/null 2>&1; then
  say "S1 SENTINEL FAIL: the old '& 0xff' exit mask is still present in $HDR"
  say "   (the U-71 classifier has not been applied — RED by design)"
  exit 1
fi
if ! grep -q 'classify_windows_exit_code' "$HDR"; then
  say "S1 SENTINEL FAIL: classify_windows_exit_code() not found in $HDR"
  exit 1
fi
say "S1 sentinel: old mask absent, classifier present"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/probe.cpp" <<'CPP'
// U-71 / P2-17 probe. The plain build MUST pass; the -DU71_MUTATE_OLD_MASK
// build MUST fail: it routes the classifier through the pre-fix mask on
// purpose, and a table that lets it through has no teeth.
#include <cstdio>
#include "core/ProcessRunner.h"

using gs::WinExitClass;
#ifdef U71_MUTATE_OLD_MASK
#define CLASSIFY(raw) (WinExitClass{static_cast<int>((raw)) & 0xff, false, (raw)})
#else
#define CLASSIFY(raw) (gs::classify_windows_exit_code(raw))
#endif

static int failures = 0;
static void check(bool cond, const char* what, unsigned long raw) {
  if (!cond) { ++failures; std::printf("  FAIL: %s (raw 0x%08lX)\n", what, raw); }
}

int main() {
  struct Row { unsigned long raw; int deliverable; bool abnormal; };
  // unsigned long is 4 bytes on Windows (LLP64) — every value here fits.
  const Row rows[] = {
      {0x00000000UL, 0,   false},  // success stays success
      {0x00000001UL, 1,   false},  // ordinary engine failure passes through
      {0x0000007FUL, 127, false},
      {0x000000FFUL, 255, false},  // boundary: 255 is still deliverable
      {0x00000100UL, 255, true },  // > 255: the old mask returned 0 here
      {0xC0000005UL, 255, true },  // access violation — mask returned 0x05
      {0xC0000100UL, 255, true },  // THE row: low byte 0 — mask returned 0
      {0xC0000409UL, 255, true },  // stack buffer overrun — mask returned 0x09
      {0xE0434352UL, 255, true },  // CLR exception — mask returned 0x52
      {0x80000001UL, 255, true },  // NTSTATUS warning class
      {0xFFFFFFFFUL, 255, true },
  };
  for (const Row& r : rows) {
    const WinExitClass c = CLASSIFY(r.raw);
    check(c.deliverable == r.deliverable, "deliverable mismatch", r.raw);
    check(c.abnormal == r.abnormal, "abnormal mismatch", r.raw);
  }
  // Sweep the NTSTATUS severity-ERROR band: no non-zero code may deliver 0.
  // The old mask aliases every ...00 case in this band (256 of them) to 0.
  unsigned long zero_aliases = 0;
  for (unsigned long raw = 0xC0000000UL; raw <= 0xC000FFFFUL; ++raw) {
    if (CLASSIFY(raw).deliverable == 0) ++zero_aliases;
  }
  check(zero_aliases == 0, "code in 0xC0000000..0xC000FFFF aliased to 0", zero_aliases);

  if (failures) {
    std::printf("U-71 probe: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("U-71 probe: 11/11 table rows; 0 aliases in 0xC0000000..0xC000FFFF\n");
  return 0;
}
CPP

# Word-splitting CXX is intentional (CXX="python -m ziglang c++").
# shellcheck disable=SC2086
if ! $CXX -std=c++17 -I src -o "$TMP/probe" "$TMP/probe.cpp" >"$TMP/compile.log" 2>&1; then
  say "S2 COMPILE FAIL:"; cat "$TMP/compile.log"; exit 1
fi
if ! "$TMP/probe"; then
  say "S2 SEMANTIC FAIL"; exit 1
fi
say "S2 semantic: classifier table + NTSTATUS-band sweep green"

# shellcheck disable=SC2086
if ! $CXX -std=c++17 -DU71_MUTATE_OLD_MASK=1 -I src -o "$TMP/probe_mutant" "$TMP/probe.cpp" >"$TMP/cmut.log" 2>&1; then
  say "S3 COMPILE FAIL (mutant):"; cat "$TMP/cmut.log"; exit 1
fi
if "$TMP/probe_mutant" >"$TMP/mutant.log" 2>&1; then
  say "S3 MUTATION FAIL: the probe passes even with the OLD mask reintroduced —"
  say "   the table cannot catch the regression (review_change.sh R2, executable)."
  exit 1
fi
head -n 1 "$TMP/mutant.log" | sed 's/^/   mutant leg, as expected: /'
say "S3 mutation: old-mask mutant caught"

say "test_u71_exit_codes: PASS (S1 sentinel, S2 semantic, S3 mutation)"
