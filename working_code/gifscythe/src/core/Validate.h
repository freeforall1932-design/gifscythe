// Validate.h - Surface out-of-range / conflicting settings before they vanish.
// Qt-independent.

#ifndef GIFSCYTHE_CORE_VALIDATE_H
#define GIFSCYTHE_CORE_VALIDATE_H

#include "GifsicleSettings.h"
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
  if (s.crop && (s.crop_w == 0 || s.crop_h == 0)) {
    add("crop", "0x0", "crop width/height must be > 0");
  }
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
