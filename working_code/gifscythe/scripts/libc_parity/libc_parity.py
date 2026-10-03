#!/usr/bin/env python3
"""libc_parity.py - does gifsicle's output depend on the libc it was built against?  (N-32 probe)

The wasm track's documented proof bar is "wasm output equals the native 1.96 oracle byte for
byte". That bar cannot pass for a build whose libc differs from the oracle's. This probe is the
measurement behind STATUS N-32, kept in the repo so it can be re-run (it was first built in a
scratch directory that does not survive a sandbox restart).

It builds the repo's own engine sources (reference_code/gifsicle, never modified; the source list
and engine version are read from scripts/build_engine.sh) with zig for

    glibc   x86_64-linux-gnu    same libc as the oracle, a different compiler (zig's clang)
    musl    x86_64-linux-musl
    wasm    wasm32-wasi         wasi-libc is musl-derived; run under Node's WASI

and runs 9 fixed gifsicle invocations on three inputs (the logo animation, logo1, and a seeded
many-colour 3-frame GIF so the quantizer has real work; the last needs Pillow). It compares the
output bytes of every build with the repo's own gcc+glibc oracle (release/<ver>/gifsicle from
./build.sh, or $ORACLE) and, where Pillow is available, the decoded pixels of the ones that differ.
With --stable it builds again with a stable qsort and a fixed random() forced into every build.

What it showed at S34 (zig 0.16.0, node 22, Pillow 12; re-run before relying on the exact counts):
    oracle == glibc (compiler irrelevant)          9/9
    glibc  == musl                                 3/9   (2 of the 6 differences decode to identical
                                                          pixels, 4 to different pixels)
    musl   == wasm                                 9/9
    --stable: glibc == musl == wasm                9/9   (qsort tie order + random() explain it all)

THE BAR (S34, the owner's N-32 decision: a same-libc native oracle).  --bar is the proof that replaces
"byte-equal to the glibc oracle": the wasm32-wasi build must equal a NATIVE build of the same sources against
the same libc family (musl) byte for byte, on every case. That isolates what a wasm port can actually break
(32-bit long and pointers, libm, alignment, stack) from the two libc behaviours no port controls. It builds
only those two engines (66 s from a cold zig cache, 9 s warm), needs zig + node + Pillow (a missing one is exit 3 - never a silently
smaller proof) and fails if a build produces no GIF (two empty outputs would otherwise "match").
CI runs it in the `portability` job. `--against glibc` is the bar the docs used to state: it is expected to
FAIL (6 of 9 differ), which is the reason it was replaced and shows the bar has teeth.
`--build-oracle DIR` writes the musl-native engine to DIR/gifsicle-musl for web/wasm/prove_wasm.mjs --oracle
(the Emscripten build, run by whoever has emcc).

Usage:  python3 working_code/gifscythe/scripts/libc_parity/libc_parity.py [--stable] [--check] [--keep DIR]
        python3 .../libc_parity.py --bar [--against musl|glibc] [--keep DIR]
        python3 .../libc_parity.py --build-oracle DIR
Needs:  zig on PATH or `pip install ziglang`; node >= 20; ./build.sh for the oracle; Pillow optional
        (required by --bar). Nothing is written into the checkout; builds go to a temp dir (or --keep DIR).
Exit:   0 measured (and, with --check, the finding still holds; with --bar, the bar passed) / 1 --check
        failed: N-32's text needs revisiting, or --bar failed / 2 a build failed / 3 SKIPPED: a tool is
        missing (printed, never silent)
"""
import argparse
import base64
import hashlib
import os
import random
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
GS = HERE.parents[1]  # working_code/gifscythe
REPO = GS.parents[1]
ENGINE = REPO / "reference_code" / "gifsicle"
CONFIG = {"native": GS / "build_support/gifsicle/config.native.h", "wasm": REPO / "web/wasm/config.wasm.h"}
BUILD_ENGINE = GS / "scripts/build_engine.sh"
SHIM = HERE / "shim.h"
WASI_RUN = HERE / "wasi_run.mjs"

# (name, zig target, config flavour, libs)
VARIANTS = (
    ("glibc", "x86_64-linux-gnu.2.28", "native", ["-lm", "-lpthread"]),
    ("musl", "x86_64-linux-musl", "native", ["-lm"]),
    ("wasm", "wasm32-wasi", "wasm", ["-lm"]),
)
# (label, input file, arguments)
CASES = (
    ("logo -O3", "logo.gif", ["-O3"]),
    ("logo -O3 resize-fit 30x66", "logo.gif", ["-O3", "--resize-fit", "30x66"]),
    ("logo colors16", "logo.gif", ["--colors", "16"]),
    ("logo colors4 dither", "logo.gif", ["--colors", "4", "--dither"]),
    ("synth colors16", "synth.gif", ["--colors", "16"]),
    ("synth colors16 dither", "synth.gif", ["--colors", "16", "--dither"]),
    ("synth colors64 -O2", "synth.gif", ["--colors", "64", "-O2"]),
    ("synth scale0.5 colors32", "synth.gif", ["--scale", "0.5", "--colors", "32"]),
    ("logo1 -O3 lossy60", "logo1.gif", ["-O3", "--lossy=60"]),
)


def zig_command():
    if shutil.which("zig"):
        return ["zig"]
    try:
        import ziglang  # noqa: F401
    except ImportError:
        return None
    return [sys.executable, "-m", "ziglang"]


def engine_facts():
    text = BUILD_ENGINE.read_text(encoding="utf-8")
    objs = re.search(r'^OBJS="([^"]+)"', text, re.M).group(1).split()
    version = re.search(r'^ENGINE_VERSION="([^"]+)"', text, re.M).group(1)
    return objs, version


def build(zig, work, name, target, flavour, libs, stable, objs, version):
    stage = work / f"cfg_{flavour}"
    if not stage.exists():
        stage.mkdir()
        shutil.copy2(CONFIG[flavour], stage / "config.h")
    out = work / f"engine_{name}{'_stable' if stable else ''}{'.wasm' if name == 'wasm' else ''}"
    cmd = [*zig, "cc", "-O2", "-target", target, "-DHAVE_CONFIG_H", f'-DVERSION="{version}"',
           "-include", str(SHIM)]
    if stable:
        cmd.append("-DGS_STABLE_LIBC")
    cmd += [f"-I{stage}", "-Iinclude", "-Isrc", *[f"src/{o}.c" for o in objs], *libs, "-o", str(out)]
    r = subprocess.run(cmd, cwd=ENGINE, capture_output=True, text=True)
    if r.returncode != 0:
        print(f"ERROR: building {out.name} failed:\n{r.stderr[-1500:]}", file=sys.stderr)
        sys.exit(2)
    return out


def make_inputs(work):
    """Copy the two real inputs; generate the seeded many-colour one when Pillow is available."""
    # The repo holds no binary files (N-36): the upstream test images are decoded from tests/fixtures/*.b64
    # and checked against SHA256SUMS, so every case runs on the byte-identical upstream input.
    sums = {n: h for h, n in (l.split() for l in (GS / "tests/fixtures/SHA256SUMS").read_text().splitlines() if l.strip())}
    for name in ("logo.gif", "logo1.gif"):
        data = base64.b64decode((GS / "tests/fixtures" / f"{name}.b64").read_text())
        if hashlib.sha256(data).hexdigest() != sums[name]:
            print(f"ERROR: tests/fixtures/{name}.b64 decodes to the wrong bytes", file=sys.stderr)
            sys.exit(2)
        (work / name).write_bytes(data)
    try:
        from PIL import Image
    except ImportError:
        return False
    rnd = random.Random(0x47534631)
    frames = []
    for f in range(3):
        im = Image.new("RGB", (96, 72))
        px = im.load()
        for y in range(72):
            for x in range(96):
                px[x, y] = ((x * 255 // 95 + f * 40 + rnd.randint(-18, 18)) % 256,
                            (y * 255 // 71 + rnd.randint(-18, 18)) % 256,
                            ((x * y) // 27 + f * 70 + rnd.randint(-18, 18)) % 256)
        frames.append(im.convert("P", palette=Image.ADAPTIVE, colors=256))
    frames[0].save(work / "synth.gif", save_all=True, append_images=frames[1:],
                   duration=[80, 80, 80], loop=0)
    return True


def run_case(exe, tag, i, inp, args, work):
    out = work / f"out_{tag}_{i}.gif"
    out.unlink(missing_ok=True)
    if exe.suffix == ".wasm":
        cmd = ["node", "--no-warnings", str(WASI_RUN), str(exe), str(work), "--",
               *args, f"/w/{inp}", "-o", f"/w/{out.name}"]
    else:
        cmd = [str(exe), *args, str(work / inp), "-o", str(out)]
    subprocess.run(cmd, capture_output=True)
    return out if out.exists() else None


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()[:12] if path else "(none)"


def decoded_frames(path):
    """RGBA of every frame. Alpha-0 pixels carry arbitrary RGB, so they are normalised first."""
    from PIL import Image
    im, seq = Image.open(path), []
    try:
        while True:
            fr = im.convert("RGBA")
            getter = getattr(fr, "get_flattened_data", None) or fr.getdata  # getdata is deprecated in Pillow 12
            seq.append((fr.size, [(0, 0, 0, 0) if p[3] == 0 else p for p in getter()]))
            im.seek(im.tell() + 1)
    except EOFError:
        pass
    return seq


def find_oracle():
    if os.environ.get("ORACLE"):
        path = Path(os.environ["ORACLE"])
        return path if path.exists() else None
    found = sorted((GS / "release").glob("*/gifsicle"))
    return found[0] if found else None


def is_gif(path):
    return bool(path) and path.read_bytes()[:4] == b"GIF8"


def bar_main(a, zig, objs, version):
    """N-32's chosen bar: wasm32-wasi == a same-libc native build, byte for byte, on every case."""
    missing = []
    if not shutil.which("node"):
        missing.append("node")
    try:
        import PIL  # noqa: F401
    except ImportError:
        missing.append("Pillow (pip install pillow)")
    if missing:
        print(f"SKIP: --bar needs {' and '.join(missing)} - the wasm proof bar did NOT run")
        return 3
    against = a.against
    work = Path(a.keep) if a.keep else Path(tempfile.mkdtemp(prefix="gs-wasm-bar-"))
    work.mkdir(parents=True, exist_ok=True)
    try:
        make_inputs(work)
        variants = [v for v in VARIANTS if v[0] in (against, "wasm")]
        builds = {}
        for name, target, flavour, libs in variants:
            print(f"==> building {name} ({target}) ...", flush=True)
            builds[name] = build(zig, work, name, target, flavour, libs, False, objs, version)
        outs = {tag: [run_case(exe, tag, i, c[1], c[2], work) for i, c in enumerate(CASES)]
                for tag, exe in builds.items()}
        print(f"\n{'case':28} {against:13} {'wasm':13} same")
        failures = []
        for i, c in enumerate(CASES):
            x, y = outs[against][i], outs["wasm"][i]
            same = is_gif(x) and is_gif(y) and x.read_bytes() == y.read_bytes()
            print(f"{c[0]:28} {digest(x):13} {digest(y):13} {'yes' if same else 'NO'}")
            if not same:
                why = "no GIF produced" if not (is_gif(x) and is_gif(y)) else "bytes differ"
                failures.append(f"{c[0]} ({why})")
        n = len(CASES)
        if failures:
            print(f"\nBAR FAILED: wasm32-wasi != {against}-native on {len(failures)} of {n} invocations:\n  "
                  + "\n  ".join(failures))
            return 1
        print(f"\nBAR OK: wasm32-wasi == {against}-native, byte for byte, on {n}/{n} invocations")
        return 0
    finally:
        if not a.keep:
            shutil.rmtree(work, ignore_errors=True)


def build_oracle_main(a, zig, objs, version):
    """Write the same-libc (musl) native engine to DIR/gifsicle-musl for prove_wasm.mjs --oracle."""
    out_dir = Path(a.build_oracle)
    out_dir.mkdir(parents=True, exist_ok=True)
    name, target, flavour, libs = next(v for v in VARIANTS if v[0] == "musl")
    scratch = Path(tempfile.mkdtemp(prefix="gs-oracle-"))
    try:
        print(f"==> building the same-libc oracle ({target}) ...", flush=True)
        built = build(zig, scratch, name, target, flavour, libs, False, objs, version)
        dest = out_dir / "gifsicle-musl"
        shutil.copy2(built, dest)
        dest.chmod(0o755)
        print(f"oracle: {dest}")
        return 0
    finally:
        shutil.rmtree(scratch, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--stable", action="store_true", help="also build with a stable qsort + fixed random()")
    ap.add_argument("--check", action="store_true", help="assert the N-32 finding still holds (implies --stable)")
    ap.add_argument("--keep", metavar="DIR", help="build into DIR and keep it (default: a temp dir, removed)")
    ap.add_argument("--bar", action="store_true",
                    help="enforce the wasm proof bar (N-32): wasm32-wasi == a same-libc native build, byte for byte")
    ap.add_argument("--against", choices=("musl", "glibc"), default="musl",
                    help="with --bar: the native build the wasm build is held to (default musl; glibc is expected to FAIL)")
    ap.add_argument("--build-oracle", metavar="DIR",
                    help="write the musl-native engine to DIR/gifsicle-musl (the oracle for prove_wasm.mjs --oracle) and exit")
    a = ap.parse_args()
    a.stable = a.stable or a.check

    zig = zig_command()
    if zig is None:
        print("SKIP: no zig (pip install ziglang) - the libc-parity measurement did NOT happen")
        return 3
    objs, version = engine_facts()
    if a.build_oracle:
        return build_oracle_main(a, zig, objs, version)
    if a.bar:
        return bar_main(a, zig, objs, version)
    have_node = shutil.which("node") is not None
    if not have_node:
        print("note: no node - the wasm column is skipped")

    work = Path(a.keep) if a.keep else Path(tempfile.mkdtemp(prefix="gs-libc-parity-"))
    work.mkdir(parents=True, exist_ok=True)
    try:
        have_synth = make_inputs(work)
        if not have_synth:
            print("note: no Pillow - the 'synth' cases and the pixel comparison are skipped (pip install pillow)")
        cases = [c for c in CASES if have_synth or c[1] != "synth.gif"]

        builds = {}  # tag -> executable
        oracle = find_oracle()
        if oracle:
            builds["oracle"] = oracle
        else:
            print("note: no oracle (run ./build.sh, or set ORACLE) - the oracle column is skipped")
        for stable in ((False, True) if a.stable else (False,)):
            for name, target, flavour, libs in VARIANTS:
                if name == "wasm" and not have_node:
                    continue
                tag = name + ("+stable" if stable else "")
                print(f"==> building {tag} ({target}) ...", flush=True)
                builds[tag] = build(zig, work, name, target, flavour, libs, stable, objs, version)

        results = {tag: [digest(run_case(exe, tag.replace("+", "_"), i, c[1], c[2], work))
                         for i, c in enumerate(cases)] for tag, exe in builds.items()}

        order = [t for t in ("oracle", "glibc", "musl", "wasm") if t in results]
        print(f"\n{'case':28} " + " ".join(f"{t:13}" for t in order))
        for i, c in enumerate(cases):
            print(f"{c[0]:28} " + " ".join(f"{results[t][i]:13}" for t in order))

        n = len(cases)

        def agree(x, y):
            return sum(results[x][i] == results[y][i] for i in range(n))

        print(f"\nbyte-identical output across builds of the same sources (n = {n}):")
        claims = {}
        if "oracle" in results:
            claims["compiler"] = agree("oracle", "glibc")
            print(f"  oracle (gcc+glibc) == glibc (zig clang+glibc)   {claims['compiler']}/{n}   the compiler does not matter")
        claims["libc"] = agree("glibc", "musl")
        print(f"  glibc == musl                                    {claims['libc']}/{n}   the libc does")
        if "wasm" in results:
            claims["wasm"] = agree("musl", "wasm")
            print(f"  musl == wasm (wasi-libc, Node WASI)              {claims['wasm']}/{n}")
        if have_synth:  # Pillow is importable
            differing = [i for i in range(n) if results["glibc"][i] != results["musl"][i]]
            same_px = sum(decoded_frames(work / f"out_glibc_{i}.gif") == decoded_frames(work / f"out_musl_{i}.gif")
                          for i in differing)
            print(f"  of the {len(differing)} glibc/musl differences: {same_px} decode to identical pixels, "
                  f"{len(differing) - same_px} to different pixels")
        if a.stable:
            tags = [t for t in ("glibc+stable", "musl+stable", "wasm+stable") if t in results]
            claims["stable"] = sum(len({results[t][i] for t in tags}) == 1 for i in range(n))
            print("with a stable qsort + a fixed random() forced into every build:")
            print(f"  {' == '.join(t.split('+')[0] for t in tags)}   {claims['stable']}/{n}")

        if a.check:
            bad = []
            if "compiler" in claims and claims["compiler"] != n:
                bad.append(f"oracle != zig clang+glibc ({claims['compiler']}/{n}) - the compiler now matters")
            if claims["libc"] == n:
                bad.append("glibc == musl in every case - the libc no longer matters; N-32 may be moot")
            if "wasm" in claims and claims["wasm"] != n:
                bad.append(f"musl != wasm ({claims['wasm']}/{n}) - wasm is no longer musl-equivalent")
            if claims["stable"] != n:
                bad.append(f"a stable libc does not make the builds agree ({claims['stable']}/{n}) - a third cause exists")
            if bad:
                print("\nCHECK FAILED - N-32's text needs revisiting:\n  " + "\n  ".join(bad))
                return 1
            print("\nCHECK OK - the N-32 finding still holds")
        return 0
    finally:
        if not a.keep:
            shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
