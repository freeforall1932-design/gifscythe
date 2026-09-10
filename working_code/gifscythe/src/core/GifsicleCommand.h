// GifsicleCommand.h - Builds a gifsicle command line from GifsicleSettings.
//
// Two outputs:
//   1. args()    - argv vector (ready to exec a gifsicle subprocess — NEVER shell)
//   2. toString()- shell-quoted CLI string for the live "show me the command" pane
//
// Qt-independent (STL only). Settings are stored BY VALUE (no dangling refs).

#ifndef GIFSCYTHE_CORE_GIFSICLE_COMMAND_H
#define GIFSCYTHE_CORE_GIFSICLE_COMMAND_H

#include "GifsicleSettings.h"
#include <sstream>
#include <cctype>

namespace gs {

// Quote a single argument for safe display / copy-paste into a POSIX shell.
// Used ONLY for display — execution always goes through argv (no shell).
inline std::string shell_quote(const std::string& a) {
  if (a.empty()) return "''";
  bool safe = true;
  for (unsigned char c : a) {
    if (!(std::isalnum(c) || c == '/' || c == '.' || c == '_' || c == '-' ||
          c == '+' || c == '=' || c == ':' || c == '@' || c == '%' || c == ',')) {
      safe = false;
      break;
    }
  }
  if (safe) return a;
  std::string out = "'";
  for (char c : a) {
    if (c == '\'') out += "'\\''";
    else out += c;
  }
  out += "'";
  return out;
}

class GifsicleCommand {
 public:
  // Store by value so temporaries (GifsicleCommand(currentSettings())) are safe.
  explicit GifsicleCommand(Settings s) : settings_(std::move(s)) {}

  // Build the argv vector (without the program name).
  const std::vector<std::string>& args();

  // Build a shell-quoted command line string for display (not for system()).
  std::string toString();

 private:
  void build();

  Settings settings_;
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
  o.precision(10);
  o << x;
  return o.str();
}

inline std::string optimization_opt(int level) {
  // -O0 is valid ("no optimization"); -O without value = 1.
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
  // Dither: prefer explicit method string; fall back to bare -f when bool set.
  if (!s.dither_method.empty()) {
    if (s.dither_method == "none") {
      // emit nothing
    } else {
      add(args_, "--dither=" + s.dither_method);
    }
  } else if (s.dither) {
    add(args_, "-f");
  }
  if (s.lossy >= 0 && s.lossy <= 200) {
    // --lossy takes an OPTIONAL value; gifsicle wants it attached (=N).
    add(args_, "--lossy=" + i2s(s.lossy));
  }
  // Gamma: prefer string form (supports srgb|oklab|NUM); legacy double as fallback.
  if (!s.gamma_str.empty()) {
    add(args_, "--gamma=" + s.gamma_str);
  } else if (s.gamma >= 0) {
    add(args_, "--gamma=" + f2s(s.gamma));
  }
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

  // Crop — gifsicle wants X,Y+WIDTHxHEIGHT (plus form), NOT comma before size.
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
  for (const auto& c : s.comments) {
    // An empty comment must not be emitted at all. add() drops empty strings,
    // so a "" entry pushed `--comment` with NO operand, which then swallowed
    // the next argument as its text (audit U-13/U-48 — verified: comments
    // {"", "real one"} produced `--comment --comment 'real one'`, losing the
    // real comment and mangling the command).
    if (c.empty()) continue;
    add(args_, "--comment");
    add(args_, c);
  }

  // Animation options
  // delay_cs is in 1/100 s (gifsicle -d units), NOT milliseconds.
  if (s.delay_cs >= 0) { add(args_, "-d"); add(args_, i2s(s.delay_cs)); }
  if (s.disposal >= 0 && s.disposal <= 7) {
    add(args_, "--disposal"); add(args_, i2s(s.disposal));
  }
  if (s.loopcount == 0) {
    // --loopcount=0 is equivalent to forever (man page confirmed).
    add(args_, "--loopcount=0");
  } else if (s.loopcount > 0) {
    add(args_, "--loopcount=" + i2s(s.loopcount));
  }
  if (s.optimize_level >= 0 && s.optimize_level <= 3) {
    // -O0 is valid ("off"); attach the level (-O3).
    add(args_, optimization_opt(s.optimize_level));
  }
  if (s.unoptimize) add(args_, "-U");
  // Threads (audit U-03). "Auto" (threads <= 0) MUST emit a bare -j, not
  // nothing: the engine's own default is single-threaded
  // (gifsicle.c:39 `int thread_count = 0;`, consumed by xform.c:1329
  // `int nthreads = thread_count;`), while a bare -j selects
  // GIFSICLE_DEFAULT_THREAD_COUNT = 8 (gifsicle.c:38, :1893). Emitting no flag
  // therefore made the control labelled "Auto" run one thread, contradicting
  // both the label and GifsicleSettings.h's "-j; <=0 = auto".
  if (s.threads > 0) add(args_, "-j" + i2s(s.threads));
  else add(args_, "-j");

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
    o << shell_quote(args_[i]);
  }
  return o.str();
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_GIFSICLE_COMMAND_H
