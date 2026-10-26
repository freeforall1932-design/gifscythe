#!/usr/bin/env bash
# GS-204: exercise both package types in disposable fixture trees, plus real CLI bundles.
set -euo pipefail
self="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT="$(mktemp -d)"; trap 'rm -rf "$ROOT"' EXIT
PASS=0; FAIL=0
ok() { echo "  PASS: $*"; PASS=$((PASS+1)); }
bad() { echo "  FAIL: $*"; FAIL=$((FAIL+1)); }
fixture() {
  local root="$ROOT/repo"
  rm -rf "$root"
  t="$root/working_code/gifscythe"
  mkdir -p "$t/scripts" "$t/build" "$t/release/0.1.0"
  cp "$self/scripts/"package*.sh "$t/scripts/"
  printf 'Current version: 0.1.0\n' > "$t/VERSION.md"
  printf 'product\n' > "$t/README.md"
  mkdir -p "$root/legal"
  printf 'engine text\n' > "$root/legal/COPYING.gifsicle"
  printf 'first-party text\n' > "$root/legal/COPYING.ms-pl"
  printf 'qt lgpl text\n' > "$root/legal/COPYING.lgplv3"
  printf 'qt gpl companion text\n' > "$root/legal/COPYING.gplv3"
  printf 'proposed first-party notice\n' > "$root/legal/LICENSE.md-pl.proposed"
  printf '#!/bin/sh\nexit 0\n' > "$t/build/gifscythe-cli"
  cp "$t/build/gifscythe-cli" "$t/release/0.1.0/gifsicle"
  chmod +x "$t/build/gifscythe-cli" "$t/release/0.1.0/gifsicle"
  out="$t/release/0.1.0/$folder"
}
refused() {
  if "$t/scripts/package_$kind.sh" "$@" >"$ROOT/log" 2>&1; then bad "$kind $label succeeded";
  elif [[ -e "$out" ]] || compgen -G "$t/release/0.1.0/.package-*" >/dev/null; then bad "$kind $label left a package/stage";
  else ok "$kind $label fails without publishing stale/partial contents"; fi
}
echo "==> Packaging negative tests"
for kind in portable system; do
  folder=Gifscythe; [[ "$kind" == system ]] && folder=Gifscythe-system
  fixture; label="no GUI"; refused
  for missing in build/gifscythe-cli release/0.1.0/gifsicle ../../legal/COPYING.gifsicle ../../legal/COPYING.ms-pl ../../legal/COPYING.lgplv3 ../../legal/COPYING.gplv3 ../../legal/LICENSE.md-pl.proposed; do
    fixture; rm "$t/$missing"; mkdir -p "$out"; echo stale > "$out/STALE"
    label="missing $missing"; refused --engine-cli-only
  done
  fixture; : > "$t/build/gifscythe-cli"; label="empty CLI"; refused --engine-cli-only
  fixture; chmod -x "$t/build/gifscythe-cli"; label="non-executable native CLI"; refused --engine-cli-only
  fixture; label="native/Windows mix"; refused --windows --engine-cli-only
  fixture
  mkdir -p "$out"; echo stale > "$out/STALE"
  cp "$t/build/gifscythe-cli" "$t/build/gifscythe"
  if "$t/scripts/package_$kind.sh" --engine-cli-only >"$ROOT/log" 2>&1 \
     && [[ ! -e "$out/STALE" && ! -e "$out/gifscythe" && -s "$out/MANIFEST.txt" ]] \
     && [[ ! -e "$out/QT_NOTICE.txt" && -s "$out/licenses/COPYING.lgplv3" && -s "$out/licenses/COPYING.gplv3" ]] \
     && grep -q 'no GUI, by request' "$out/README.txt"; then
    ok "$kind explicit headless excludes even an available GUI and wipes stale files"
  else bad "$kind headless manifest/scope/staging"; fi
  fixture; cp "$t/build/gifscythe-cli" "$t/build/gifscythe"
  if "$t/scripts/package_$kind.sh" >"$ROOT/log" 2>&1 && [[ -x "$out/gifscythe" ]] \
     && [[ -s "$out/QT_NOTICE.txt" && -s "$out/licenses/COPYING.lgplv3" && -s "$out/licenses/COPYING.gplv3" ]]; then
    ok "$kind complete native GUI fixture stages required files"
  else bad "$kind complete GUI fixture"; fi
  fixture
  if "$t/scripts/package_$kind.sh" --bad >"$ROOT/log" 2>&1; then bad "$kind unknown option accepted";
  else [[ $? == 2 ]] && ok "$kind unknown option exits 2" || bad "$kind wrong option exit"; fi
  # Real native headless artifacts built by build.sh (also executed in CI).
  fixture
  real_version="$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' "$self/VERSION.md" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
  cp "$self/build/gifscythe-cli" "$t/build/gifscythe-cli"
  cp "$self/release/$real_version/gifsicle" "$t/release/0.1.0/gifsicle"
  if "$t/scripts/package_$kind.sh" --engine-cli-only >"$ROOT/log" 2>&1; then
    ok "$kind real engine/CLI package verified in an isolated tree"
  else bad "$kind real package: $(tail -1 "$ROOT/log")"; fi
done
# Deterministic deployer fixtures: isolated PATH contains required utilities only.
kind=portable; folder=Gifscythe
fixture
for name in gifsicle gifscythe-cli gifscythe; do
  printf 'fake Windows binary\n' > "$t/build/$name.exe"
done
cp "$t/build/gifsicle.exe" "$t/release/0.1.0/gifsicle.exe"
TOOLS="$ROOT/tools"; mkdir -p "$TOOLS"
for tool in bash uname dirname grep head mkdir rm mktemp cp cat mv; do ln -s "$(command -v "$tool")" "$TOOLS/$tool"; done
label="missing Windows deployer"
old_path="$PATH"; PATH="$TOOLS"; refused --windows; PATH="$old_path"
for behavior in 'exit 1' 'exit 0'; do
  printf '#!/bin/sh\n%s\n' "$behavior" > "$TOOLS/windeployqt"; chmod +x "$TOOLS/windeployqt"
  label="failed/lying Windows deployer ($behavior)"
  PATH="$TOOLS"; refused --windows; PATH="$old_path"
done
# Successful deployment fixture proves the manifest predicate is not vacuous;
# it does NOT prove real Windows DLL completeness or runtime compatibility.
cat > "$TOOLS/windeployqt" <<'EOF'
#!/bin/sh
dir="$(dirname "$3")"
mkdir -p "$dir/platforms"
for f in Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll platforms/qwindows.dll; do
  printf 'fixture DLL\n' > "$dir/$f"
done
EOF
chmod +x "$TOOLS/windeployqt"
if PATH="$TOOLS" "$t/scripts/package_portable.sh" --windows >"$ROOT/log" 2>&1 \
   && [[ -s "$out/platforms/qwindows.dll" && ! -e "$out/gifscythe" ]]; then
  ok "portable Windows fixture stages only target binaries and asserted runtime entries"
else bad "portable successful deployer fixture"; fi
if PATH="$TOOLS" "$t/scripts/package_portable.sh" --windows --engine-cli-only >"$ROOT/log" 2>&1 \
   && [[ ! -e "$out/gifscythe.exe" && ! -e "$out/Qt6Core.dll" && ! -e "$out/QT_NOTICE.txt" ]]; then
  ok "Windows explicit headless omits GUI and runtime"
else bad "Windows headless fixture"; fi
if PATH="$TOOLS" "$t/scripts/package_system.sh" --windows >"$ROOT/log" 2>&1 \
   && [[ -s "$t/release/0.1.0/Gifscythe-system/gifscythe.exe" && ! -e "$t/release/0.1.0/Gifscythe-system/Qt6Core.dll" ]]; then
  ok "system Windows fixture deliberately does not deploy Qt"
else bad "system Windows fixture"; fi
echo "==> FFmpeg sidecar (N-62)"
# The owner wants FFmpeg IN the package. It is optional only because a plain
# checkout has no 156 MB sidecar; a release build says --require-ffmpeg and then
# absence is fatal. What must NEVER happen is a package that silently lacks the
# encoder while the app still advertises the formats.
ff_platform=""
case "$(uname -s)" in
  Linux*)  ff_platform="linux64" ;;
  Darwin*) ff_platform="macos-$( [[ "$(uname -m)" == arm64 ]] && echo arm64 || echo x64 )" ;;
  *)       ff_platform="win64" ;;
esac
sidecar() { # stage a plausible ffmpeg where the locator/package expect it
  mkdir -p "$t/vendor/ffmpeg/$ff_platform/bin"
  printf '#!/bin/sh\necho ffmpeg version fixture\n' > "$t/vendor/ffmpeg/$ff_platform/bin/ffmpeg"
  chmod +x "$t/vendor/ffmpeg/$ff_platform/bin/ffmpeg"
}
fixture; sidecar
if "$t/scripts/package_portable.sh" --engine-cli-only >"$ROOT/log" 2>&1 \
   && [[ -x "$out/ffmpeg" && -s "$out/FFMPEG_NOTICE.txt" && -s "$out/MANIFEST.txt" ]] \
   && grep -q '^ffmpeg$' "$out/MANIFEST.txt" \
   && grep -q 'FFmpeg is bundled' "$out/README.txt" \
   && grep -q 'GNU General Public License v3' "$out/FFMPEG_NOTICE.txt"; then
  ok "bundled sidecar lands next to the app with a notice and an honest README"
else bad "bundled sidecar staging"; fi
fixture
if "$t/scripts/package_portable.sh" --engine-cli-only >"$ROOT/log" 2>&1 \
   && [[ ! -e "$out/ffmpeg" && ! -e "$out/FFMPEG_NOTICE.txt" ]] \
   && grep -q 'FFmpeg is NOT bundled' "$out/README.txt";
then ok "a package without the sidecar says so instead of implying the formats work"
else bad "sidecar absence not disclosed"; fi
fixture; mkdir -p "$out"; echo stale > "$out/STALE"
label="require-ffmpeg without sidecar"; kind=portable; folder=Gifscythe
refused --engine-cli-only --require-ffmpeg
fixture; sidecar; : > "$t/vendor/ffmpeg/$ff_platform/bin/ffmpeg"
chmod +x "$t/vendor/ffmpeg/$ff_platform/bin/ffmpeg"
# A truncated/empty ffmpeg is worse than none: the app would pick it, fail to
# encode, and the failure would point at the format instead of the file. It must
# read as "absent" - and a release build must then refuse outright.
if "$t/scripts/package_portable.sh" --engine-cli-only >"$ROOT/log" 2>&1 \
   && [[ ! -e "$out/ffmpeg" && ! -e "$out/FFMPEG_NOTICE.txt" ]] \
   && grep -q 'FFmpeg is NOT bundled' "$out/README.txt"; then
  ok "0-byte sidecar is treated as absent, not bundled"
else bad "0-byte sidecar mishandled"; fi
if "$t/scripts/package_portable.sh" --engine-cli-only --require-ffmpeg >"$ROOT/log" 2>&1; then
  bad "0-byte sidecar satisfied --require-ffmpeg"
else ok "0-byte sidecar cannot satisfy --require-ffmpeg"; fi
fixture
if "$t/scripts/package_portable.sh" --engine-cli-only >"$ROOT/log" 2>&1 && grep -q 'FFmpeg is NOT bundled' "$out/README.txt"; then
  ok "no sidecar is still a legal dev package (GIF needs no FFmpeg)"
else bad "sidecar-free dev package"; fi

echo "==> Done. $PASS passed, $FAIL failed."
[[ "$FAIL" == 0 ]]
