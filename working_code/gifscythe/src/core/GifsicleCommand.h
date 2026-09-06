// GifsicleCommand.h - Builds a gifsicle command line from GifsicleSettings.
//
// Two outputs:
//   1. args()    - argv vector (ready to exec a gifsicle subprocess)
//   2. toString()- the raw CLI string shown in the live "show me the command"
//                   pane, keeping full terminal control visible to power users.
//
// Qt-independent (STL only).

#ifndef GIFSYCYTHE_CORE_GIFSICLE_COMMAND_H
#define GIFSYCYTHE_CORE_GIFSICLE_COMMAND_H

#include "GifsicleSettings.h"
#include <sstream>

namespace gs {

class GifsicleCommand {
 public:
  explicit GifsicleCommand(const Settings& s) : settings_(s) {}

  // Build the argv vector (without the program name).
  const std::vector<std::string>& args();

  // Build a shell-friendly command line string for display.
  std::string toString();

 private:
  void build();

  const Settings& settings_;
  std::vector<std::string> args_;
  bool built_ = false;
};

// ---- impl below (header-only for easy unit testing) ----

inline void add(std::vector<std::string>& v, const std::string& s) {
  if (!s.empty()) v.push_back(s);
}

inline std::string u2s(unsigned x) { return std::to_string(x); }
inline std::string i2s(int x) { return std::to_string(x); }

inline std::string f2s(double x) {
  std::ostringstream o;
  o << x;
  return o.str();
}

inline std::string optimization_opt(int level) {
  // -O is fine without a value (=1), but we pass the explicit level attached.
  if (level == 1) return "-O";
  return "-O" + std::to_string(level);
}

inline const std::vector<std::string>& GifsicleCommand::args() {
  if (!built_) build();
  return args_;
}

inline void GifsicleCommand::build() {
  args_.clear();
  const Settings& s = settings_;

  // Mode (must come before filenames).
  switch (s.mode) {
    case Mode::Merge:   add(args_, "-m"); break;
    case Mode::Batch:   add(args_, "-b"); break;
    case Mode::Explode: add(args_, s.explode_by_name ? "-E" : "-e"); break;
    case Mode::Auto:    break;
  }

  // General
  if (s.info) add(args_, "--info");

  // Whole-GIF
  if (s.careful) add(args_, "--careful");
  if (s.color_count >= 2 && s.color_count <= 256) {
    add(args_, "-k"); add(args_, i2s(s.color_count));
  }
  if (s.dither) add(args_, "-f");
  if (s.lossy >= 0 && s.lossy <= 200) {
    // --lossy takes an OPTIONAL value; gifsicle wants it attached (=N).
    add(args_, "--lossy=" + i2s(s.lossy));
  }
  if (s.gamma >= 0) { add(args_, "--gamma"); add(args_, f2s(s.gamma)); }
  if (!s.color_method.empty()) {
    add(args_, "--color-method"); add(args_, s.color_method);
  }

  // Resize / scale
  switch (s.resize_kind) {
    case ResizeKind::Fit:
      add(args_, "--resize-fit");
      add(args_, u2s(s.resize_w) + "x" + u2s(s.resize_h));
      break;
    case ResizeKind::Touch:
      add(args_, "--resize-touch");
      add(args_, u2s(s.resize_w) + "x" + u2s(s.resize_h));
      break;
    case ResizeKind::Exact:
      add(args_, "--resize");
      add(args_, u2s(s.resize_w) + "x" + u2s(s.resize_h));
      break;
    case ResizeKind::Scale:
      add(args_, "--scale");
      add(args_, f2s(s.scale_x) + "x" + f2s(s.scale_y));
      break;
    case ResizeKind::Width:
      add(args_, "--resize-width"); add(args_, u2s(s.resize_w));
      break;
    case ResizeKind::Height:
      add(args_, "--resize-height"); add(args_, u2s(s.resize_h));
      break;
    case ResizeKind::None: break;
  }
  if (!s.resize_method.empty()) {
    add(args_, "--resize-method"); add(args_, s.resize_method);
  }

  // Frame / image options
  if (s.interlace) add(args_, "-i");
  if (s.flip_horizontal) add(args_, "--flip-horizontal");
  if (s.flip_vertical) add(args_, "--flip-vertical");
  switch (s.rotation) {
    case Rotation::R90:  add(args_, "--rotate-90"); break;
    case Rotation::R180: add(args_, "--rotate-180"); break;
    case Rotation::R270: add(args_, "--rotate-270"); break;
    case Rotation::None: break;
  }
  if (s.has_position) {
    add(args_, "-p");
    add(args_, u2s(s.position_x) + "," + u2s(s.position_y));
  }

  // Crop
  if (s.crop) {
    add(args_, "--crop");
    add(args_, u2s(s.crop_x) + "," + u2s(s.crop_y) + "+" +
               u2s(s.crop_w) + "x" + u2s(s.crop_h));
    if (s.crop_transparency) add(args_, "--crop-transparency");
  }

  // Background / transparency
  if (!s.background.empty()) { add(args_, "--background"); add(args_, s.background); }
  if (!s.transparent.empty()) { add(args_, "--transparent"); add(args_, s.transparent); }

  // Comments / names / extensions
  if (s.remove_comments) add(args_, "--no-comments");
  if (s.remove_names) add(args_, "--no-names");
  if (s.remove_extensions) add(args_, "--no-extensions");
  for (const auto& c : s.comments) { add(args_, "--comment"); add(args_, c); }

  // Animation options
  if (s.delay_cs >= 0) { add(args_, "-d"); add(args_, i2s(s.delay_cs)); }
  if (s.disposal >= 0 && s.disposal <= 7) {
    add(args_, "--disposal"); add(args_, i2s(s.disposal));
  }
  if (s.loopcount == 0) {
    // --loopcount requires attached value; 0 = loop forever.
    add(args_, "--loopcount=0");
  } else if (s.loopcount > 0) {
    add(args_, "--loopcount=" + i2s(s.loopcount));
  }
  if (s.optimize_level >= 0 && s.optimize_level <= 3) {
    // -O takes an OPTIONAL level; attach it (-O3).
    add(args_, optimization_opt(s.optimize_level));
  }
  if (s.unoptimize) add(args_, "-U");
  if (s.threads > 0) { add(args_, "-j" + i2s(s.threads)); }

  // Inputs
  for (const auto& in : s.inputs) add(args_, in);

  // Output: -o FILE
  if (!s.output.empty()) { add(args_, "-o"); add(args_, s.output); }

  built_ = true;
}

inline std::string GifsicleCommand::toString() {
  build();
  std::ostringstream o;
  for (size_t i = 0; i < args_.size(); ++i) {
    if (i) o << " ";
    o << args_[i];
  }
  return o.str();
}

}  // namespace gs

#endif  // GIFSYCYTHE_CORE_GIFSICLE_COMMAND_H
