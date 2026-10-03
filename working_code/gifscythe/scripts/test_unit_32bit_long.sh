#!/usr/bin/env bash
#
# test_unit_32bit_long.sh - run the unit suite on a target where sizeof(long) == 4.
#
# Why this exists (N-30, S34). Windows is LLP64: `long` is 32 bits there, and Windows is the
# only platform we ship. Linux is LP64 (`long` is 64 bits) and is where everything is developed
# and where CI's fast jobs run. N-30 lived in exactly that gap for 31 CI runs: the settings
# parser classified integers with a `long` probe, so 2147483648..4294967295 were refused on
# Windows and accepted on Linux, and only the Windows unit run could see it - when that job
# could be read at all. This script builds tests/test_gifsicle_command.cpp for x86-linux-musl
# (an ILP32 target: long == 4) with zig and runs it, so the whole class can be checked on any
# Linux box without a Windows runner:   pip install ziglang
#
#   ./scripts/test_unit_32bit_long.sh                     this checkout
#   GS_ROOT=/path/to/working_code/gifscythe ./scripts/test_unit_32bit_long.sh   another tree
#                                                         (how a mutation test points it at a copy)
#
# Needs: `zig` on PATH, or python3 with the `ziglang` wheel; a kernel that can execute 32-bit
# x86 ELF binaries (the usual x86_64 Linux default). Nothing is written into the checkout.
#
# CI runs this in the `portability` job (S34, the owner's call: about two minutes, in parallel with the
# other jobs). Run it by hand after any change to src/core's parsing or to a type's width.
#
# Exit: 0 unit suite passed on the 32-bit-long target
#       1 unit tests failed (or did not compile) on it - that is the N-30 signal
#       2 the target is not what this script claims (sizeof(long) != 4) - fix the script
#       3 SKIPPED: no zig, or this kernel cannot run 32-bit binaries (printed, never silent)
#
set -uo pipefail
export LC_ALL=C

here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
root="$(cd "${GS_ROOT:-$here}" && pwd)"
TARGET="${GS_32BIT_TARGET:-x86-linux-musl}"

if command -v zig >/dev/null 2>&1; then
  ZIGCXX=(zig c++)
elif command -v python3 >/dev/null 2>&1 && python3 -c 'import ziglang' >/dev/null 2>&1; then
  ZIGCXX=(python3 -m ziglang c++)
else
  echo "SKIP: no zig (pip install ziglang) - the 32-bit-long unit run did NOT happen"
  exit 3
fi
[[ -f "$root/tests/test_gifsicle_command.cpp" ]] || { echo "ERROR: $root has no tests/test_gifsicle_command.cpp" >&2; exit 2; }

work="$(mktemp -d "${TMPDIR:-/tmp}/gs-32bit-long.XXXXXX")"
trap 'rm -rf "$work"' EXIT

# 1. The target must really be 32-bit-long, or this script would pass while proving nothing.
cat > "$work/probe.cpp" <<'EOF'
static_assert(sizeof(long) == 4, "target must be LLP64/ILP32-like: sizeof(long) == 4");
int main() { return 0; }
EOF
if ! "${ZIGCXX[@]}" -target "$TARGET" -std=c++17 "$work/probe.cpp" -o "$work/probe" 2>"$work/probe.err"; then
  echo "ERROR: $TARGET does not have a 4-byte long (or zig cannot target it):" >&2
  { grep -E 'error|static assertion' "$work/probe.err" || cat "$work/probe.err"; } | head -6 | sed 's/^/  /' >&2
  exit 2
fi
if ! "$work/probe" 2>/dev/null; then
  echo "SKIP: this kernel cannot execute 32-bit ($TARGET) binaries - the 32-bit-long unit run did NOT happen"
  exit 3
fi
echo "==> target $TARGET: sizeof(long) == 4 (static_assert), binary runs here"

# 2. The same single-translation-unit compile build.sh uses, for that target.
echo "==> building tests/test_gifsicle_command.cpp for $TARGET with ${ZIGCXX[*]}"
if ! "${ZIGCXX[@]}" -target "$TARGET" -std=c++17 -Wall -Wextra -pedantic -O2 -I"$root/src" \
      -o "$work/test_unit_32" "$root/tests/test_gifsicle_command.cpp" 2>"$work/build.err"; then
  echo "FAIL: the unit tests do not even compile for $TARGET:" >&2
  grep -E 'error' "$work/build.err" | head -10 | sed 's/^/  /' >&2
  exit 1
fi

# 3. Run it. A failure here with a green native run is the N-30 shape: platform-dependent behaviour.
echo "==> running the unit suite on the 32-bit-long target"
"$work/test_unit_32" 2>&1 | tail -12
rc=${PIPESTATUS[0]}
if [[ $rc -eq 0 ]]; then
  echo "==> PASS: unit suite green with a 4-byte long"
  exit 0
fi
echo "FAIL: unit suite red on the 32-bit-long target (rc=$rc) - compare with the native run: a difference is a width bug (N-30 class)" >&2
exit 1
