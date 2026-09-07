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
  if (s.crop && (s.crop_w == 0 || s.crop_h == 0)) {
    add("crop", "0x0", "crop width/height must be > 0");
  }
  if (s.inputs.empty()) {
    add("input", "", "at least one input file is required");
  }
  return w;
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_VALIDATE_H
