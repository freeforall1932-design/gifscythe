// Validate.h - Surface out-of-range / conflicting settings before they vanish.
// Qt-independent.

#ifndef GIFSCYTHE_CORE_VALIDATE_H
#define GIFSCYTHE_CORE_VALIDATE_H

#include "GifsicleSettings.h"
#include <cmath>
#include <string>
#include <vector>

namespace gs {

struct Warning {
  std::string field;
  std::string value;
  std::string reason;
};

inline std::vector<Warning> validate(const Settings& s) {
  std::vector<Warning> w;
  auto add = [&](const std::string& field, const std::string& value, const std::string& reason) {
    w.push_back(Warning{field, value, reason});
  };

  if (s.color_count != -1 && (s.color_count < 2 || s.color_count > 256)) {
    add("colors", std::to_string(s.color_count), "must be 2..256 (or unset)");
  }
  if (s.disposal != -1 && (s.disposal < 0 || s.disposal > 7)) {
    add("disposal", std::to_string(s.disposal), "must be 0..7 (or unset)");
  }
  if (s.optimize_level != -1 && (s.optimize_level < 0 || s.optimize_level > 3)) {
    add("optimize", std::to_string(s.optimize_level), "must be 0..3 (or unset); 0 = off");
  }
  if (s.lossy != -1 && (s.lossy < 0 || s.lossy > 200)) {
    add("lossy", std::to_string(s.lossy), "must be 0..200 (or unset)");
  }
  if (s.delay_cs < -1) {
    add("delay", std::to_string(s.delay_cs), "must be >= 0 (1/100 s units) or unset (-1)");
  }
  // threads (DS-06 / P0-2 + DS-09 / P1-31). The builder maps <0 to "no flag",
  // 0 to a bare -j and >0 to -jN, so ONLY -1 is a legal negative: anything
  // lower is a typo that would otherwise be silently re-read as "unset".
  if (s.threads < GS_THREADS_UNSET) {
    add("threads", std::to_string(s.threads),
        "must be >= -1 (-1 = unset/default, 0 = auto, >0 = explicit thread count)");
  }
  // loopcount (U-63 / P1-40 + GS-206 / P1-28). -2 = play once
  // (--no-loopcount), -1 = say nothing, 0..65535 = the count. The upper bound
  // is measured, not guessed: `gifsicle --loopcount=65536 logo.gif` exits 0
  // and writes "loop forever" (the Netscape field is 16-bit), i.e. the engine
  // fails SILENTLY, which is exactly what this validator exists to catch.
  if (s.loopcount != GS_LOOPCOUNT_UNSET && s.loopcount != GS_LOOPCOUNT_ONCE
      && (s.loopcount < GS_LOOPCOUNT_FOREVER || s.loopcount > GS_LOOPCOUNT_MAX)) {
    add("loopcount", std::to_string(s.loopcount),
        "must be -2 (play once), -1 (unset), or 0..65535 (0 = forever); larger values wrap: the engine turns 65536 into forever and exits 0");
  }
  // Method-name enums (GS-206 / P1-28). The engine rejects a wrong name, but
  // not uniformly: --color-method / --resize-method exit 1 with a list of
  // possibilities, while dither and gamma errors are printed and IGNORED (rc=0,
  // output written). Checked against the lists registered in gifsicle.c:1470-
  // 1508 and quantize.c — a typo costs a whole engine run otherwise.
  if (!s.color_method.empty()) {
    const std::string want = s.color_method;
    if (want != "diversity" && want != "blend-diversity" && want != "median-cut") {
      add("color_method", want,
          "must be diversity, blend-diversity or median-cut (the engine refuses anything else)");
    }
  }
  if (!s.resize_method.empty()) {
    static const char* const kResize[] = {
        "point", "sample", "mix", "box", "catrom", "lanczos", "lanczos2",
        "lanczos3", "mitchell", "fast", "good"};
    bool known = false;
    for (const char* n : kResize) if (s.resize_method == n) { known = true; break; }
    if (!known) {
      add("resize_method", s.resize_method,
          "not one of point, sample, mix, box, catrom, lanczos, lanczos2, lanczos3, mitchell, fast, good");
    }
  }
  // dither_method: NOT enum-checked on purpose. The engine's grammar carries
  // parameters (o8, "o,4", ro64x64 — quantize.c:1421 onwards), so any list we
  // hard-code would refuse legal input. A blank/whitespace name is caught: the
  // engine prints "' ' is not a valid dither" and STILL EXITS 0.
  if (!s.dither_method.empty() && s.dither_method.find_first_not_of(" \t") == std::string::npos) {
    add("dither", s.dither_method, "dither method name is blank (the engine reports it and exits 0 anyway)");
  }
  // gamma_str (GS-206): the engine accepts srgb, oklab (when built with
  // cbrtf) and any strtod-parseable number — and nothing else, but it prints
  // "--gamma not supported" with rc=0. Shape-check, no range: `--gamma=0` and
  // `--gamma=20` are accepted by the engine, so refusing them here would be
  // inventing a policy the shipped engine does not have.
  if (!s.gamma_str.empty() && s.gamma_str != "srgb" && s.gamma_str != "oklab") {
    const std::string& g = s.gamma_str;
    size_t pos = 0;
    bool numeric = true;
    double v = 0;
    try {
      v = std::stod(g, &pos);
      // isfinite on BOTH sides of the mirror: std::stod happily parses "inf"
      // and "nan", while the JS mirror's Number() returns NaN for them, and
      // web/validate.test.mjs compares the two warning lists item by item.
      numeric = (pos == g.size()) && std::isfinite(v);
    } catch (...) {
      numeric = false;                       // out_of_range, e.g. "1e999"
    }
    if (!numeric) {
      add("gamma", g, "must be srgb, oklab or a number (anything else: the engine prints a gamma error and exits 0)");
    }
  }
  if (s.info && (s.mode == Mode::Batch || s.mode == Mode::Merge || s.mode == Mode::Explode)) {
    add("info", "true", "--info cannot be combined with mode options (-m/-b/-e)");
  }
  // N-05 (S11): one shared prefix cannot carry several inputs. Verified
  // against the bundled 1.96: `gifsicle -e a.gif b.gif -o p` exits 0,
  // explodes every input BUT THE LAST as `<basename>.NNN` into the process
  // CWD, and only the last input's frames land under the prefix. Same
  // silent-data class as U-01/U-17 — surface it before any run.
  if (s.mode == Mode::Explode && s.inputs.size() > 1) {
    add("mode", "explode",
        "explode with multiple inputs scatters frames: only the LAST input honors the -o prefix, earlier inputs write <basename>.NNN into the CWD (engine exits 0) - run one file at a time");
  }
  // Crop 0x0 is legal engine syntax: width/height 0 means extend to the edge
  // (audit U-62). Negative spans are still unrepresentable here because the
  // Settings fields are unsigned; that wider model change stays separate.
  // Resize geometry (audit U-22). Verified against the bundled 1.96 engine:
  //   --resize-fit 0x0 / --resize 0x0 / --resize-touch 0x0 -> rc=1
  //       "one of W and H must be positive"   (40x0 and 0x40 are fine)
  //   --resize-width 0 / --resize-height 0    -> rc=1 "argument must be positive"
  //   --scale 0x0                             -> rc=1 "X and Y factors must be positive"
  //   --scale 0x1                             -> rc=0 but SILENTLY DOES NOTHING
  //       (output stays 60x132), which is the worst of the lot.
  switch (s.resize_kind) {
    case ResizeKind::Fit:
    case ResizeKind::Touch:
    case ResizeKind::Exact:
      if (s.resize_w == 0 && s.resize_h == 0) {
        add("resize", "0x0", "one of width and height must be > 0 (the engine refuses 0x0)");
      }
      break;
    case ResizeKind::Width:
      if (s.resize_w == 0) {
        add("resize_w", "0", "width must be > 0 (the engine refuses --resize-width 0)");
      }
      break;
    case ResizeKind::Height:
      if (s.resize_h == 0) {
        add("resize_h", "0", "height must be > 0 (the engine refuses --resize-height 0)");
      }
      break;
    case ResizeKind::Scale:
      if (!(s.scale_x > 0.0) || !(s.scale_y > 0.0)) {
        add("scale", std::to_string(s.scale_x) + "x" + std::to_string(s.scale_y),
            "both scale factors must be > 0; 0 makes the engine skip the resize silently");
      }
      break;
    case ResizeKind::None: break;
  }
  if (s.inputs.empty()) {
    add("input", "", "at least one input file is required");
  }
  return w;
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_VALIDATE_H
