// SettingsIO.h - Load/save Gifscythe Settings as a simple flat "key = value"
// text file. Serialization format the Qt GUI also uses to persist per-file
// settings and to round-trip state. Qt-independent.
//
// Each line is "key = value" or "key=value"; '#' starts a comment. Unknown keys
// are ignored (forward compatible). The keys map 1:1 onto GifsicleSettings.

#ifndef GIFSCYTHE_CORE_SETTINGS_IO_H
#define GIFSCYTHE_CORE_SETTINGS_IO_H

#include "GifsicleSettings.h"
#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace gs {

struct LoadWarning {
  std::string key;
  std::string value;
  std::string reason;
};

inline std::string lower(std::string s) {
  for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}
inline std::string trim(const std::string& s) {
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return "";
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}

// Safe parsers: return false on failure and leave *out unchanged.
inline bool to_long(const std::string& s, long* out) {
  if (!out) return false;
  std::istringstream i(trim(s));
  long v = 0;
  if (!(i >> v)) return false;
  // Reject trailing garbage ("40xyz").
  char extra = 0;
  if (i >> extra) return false;
  *out = v;
  return true;
}
inline bool to_double(const std::string& s, double* out) {
  if (!out) return false;
  std::istringstream i(trim(s));
  double v = 0;
  if (!(i >> v)) return false;
  char extra = 0;
  if (i >> extra) return false;
  *out = v;
  return true;
}

// Make a string value safe to write on ONE `key = value` line.
//
// Audit U-51: the serializer wrote string values verbatim, so a value
// containing a newline became a new line — and therefore a new KEY. Verified
// before the fix: a single comment "hi\nmode = merge" round-tripped into
// `mode = merge`, silently changing the run mode. CR and LF are folded to a
// space rather than escaped, which keeps the file format (and every existing
// conf, and the JS mirror) unchanged.
//
// Still a known limitation of a line-based format: leading and trailing
// whitespace inside a value is lost, because the loader trims. Documented here
// rather than papered over; fixing it properly means a quoted/escaped format,
// which is a breaking change for hand-written confs.
inline std::string encode_line_value(const std::string& v) {
  std::string out;
  out.reserve(v.size());
  for (char c : v) out += (c == '\n' || c == '\r') ? ' ' : c;
  return out;
}

// Recognised boolean spellings: 1/0, true/false, yes/no, on/off (any case).
// Returns false when `v` is not a boolean at all, leaving *out untouched.
//
// Audit U-11: the old parse_bool() mapped anything outside the true-set to
// false with no warning, so `unoptimize = maybe` and `careful = Trueish`
// silently turned those options OFF (verified: rc=0, zero warnings, no -U /
// --careful in the built command) while every numeric parser in this file
// warns. Booleans now warn exactly like the numbers do.
inline bool parse_bool_strict(const std::string& v, bool* out) {
  if (!out) return false;
  const std::string t = lower(trim(v));
  if (t == "1" || t == "true" || t == "yes" || t == "on") { *out = true; return true; }
  if (t == "0" || t == "false" || t == "no" || t == "off") { *out = false; return true; }
  return false;
}

// Lenient form, kept for callers that only need a yes/no read.
inline bool parse_bool(const std::string& v) {
  const std::string t = lower(trim(v));
  return t == "1" || t == "true" || t == "yes" || t == "on";
}

inline void set_field(Settings& s, const std::string& key, const std::string& val,
                      std::vector<LoadWarning>* warnings = nullptr) {
  const std::string k = lower(key);
  const std::string v = trim(val);
  auto warn = [&](const std::string& reason) {
    if (warnings) warnings->push_back(LoadWarning{key, val, reason});
  };
  auto need_long = [&](long* dest) -> bool {
    long tmp = 0;
    if (!to_long(v, &tmp)) { warn("not an integer"); return false; }
    *dest = tmp;
    return true;
  };
  auto need_bool = [&](bool* dest) -> bool {
    bool tmp = false;
    if (!parse_bool_strict(v, &tmp)) {
      warn("not a boolean (expected true/false, yes/no, on/off or 1/0); left unchanged");
      return false;
    }
    *dest = tmp;
    return true;
  };
  auto need_ulong_nonneg = [&](unsigned* dest) -> bool {
    long tmp = 0;
    if (!to_long(v, &tmp)) { warn("not an integer"); return false; }
    if (tmp < 0) { warn("negative value rejected"); return false; }
    *dest = static_cast<unsigned>(tmp);
    return true;
  };

  if (k == "mode") {
    if (v == "merge") s.mode = Mode::Merge;
    else if (v == "batch") s.mode = Mode::Batch;
    else if (v == "explode") s.mode = Mode::Explode;
    else if (v == "auto" || v.empty()) s.mode = Mode::Auto;
    else { warn("unknown mode"); s.mode = Mode::Auto; }
  }
  else if (k == "info") need_bool(&s.info);
  else if (k == "interlace") need_bool(&s.interlace);
  else if (k == "flip_horizontal") need_bool(&s.flip_horizontal);
  else if (k == "flip_vertical") need_bool(&s.flip_vertical);
  else if (k == "rotation") {
    if (v == "90") s.rotation = Rotation::R90;
    else if (v == "180") s.rotation = Rotation::R180;
    else if (v == "270") s.rotation = Rotation::R270;
    else if (v == "none" || v == "0" || v.empty()) s.rotation = Rotation::None;
    else { warn("unknown rotation"); s.rotation = Rotation::None; }
  }
  else if (k == "position_x") {
    if (need_ulong_nonneg(&s.position_x)) s.has_position = true;
  }
  else if (k == "position_y") {
    if (need_ulong_nonneg(&s.position_y)) s.has_position = true;
  }
  else if (k == "crop") need_bool(&s.crop);
  else if (k == "crop_x") need_ulong_nonneg(&s.crop_x);
  else if (k == "crop_y") need_ulong_nonneg(&s.crop_y);
  else if (k == "crop_w") need_ulong_nonneg(&s.crop_w);
  else if (k == "crop_h") need_ulong_nonneg(&s.crop_h);
  else if (k == "crop_transparency") need_bool(&s.crop_transparency);
  else if (k == "delay") {
    long tmp = 0;
    if (need_long(&tmp)) s.delay_cs = static_cast<int>(tmp);
  }
  else if (k == "disposal") {
    long tmp = 0;
    if (need_long(&tmp)) s.disposal = static_cast<int>(tmp);
  }
  else if (k == "loopcount") {
    long tmp = 0;
    if (need_long(&tmp)) s.loopcount = static_cast<int>(tmp);
  }
  else if (k == "optimize") {
    long tmp = 0;
    if (need_long(&tmp)) s.optimize_level = static_cast<int>(tmp);
  }
  else if (k == "unoptimize") need_bool(&s.unoptimize);
  else if (k == "threads") {
    long tmp = 0;
    if (need_long(&tmp)) s.threads = static_cast<int>(tmp);
  }
  else if (k == "colors") {
    long tmp = 0;
    if (need_long(&tmp)) s.color_count = static_cast<int>(tmp);
  }
  else if (k == "dither") {
    // Accept bool OR method name.
    const std::string lv = lower(v);
    if (lv == "1" || lv == "true" || lv == "yes" || lv == "on") {
      s.dither = true;
    } else if (lv == "0" || lv == "false" || lv == "no" || lv == "off" || lv == "none") {
      s.dither = false;
      s.dither_method.clear();
    } else {
      s.dither = true;
      s.dither_method = v;
    }
  }
  else if (k == "dither_method") s.dither_method = v;
  else if (k == "lossy") {
    long tmp = 0;
    if (need_long(&tmp)) s.lossy = static_cast<int>(tmp);
  }
  else if (k == "gamma") {
    // Prefer string form so srgb|oklab work; also try numeric.
    s.gamma_str = v;
    double d = 0;
    if (to_double(v, &d)) s.gamma = d;
    else s.gamma = -1.0;
  }
  else if (k == "color_method") s.color_method = v;
  else if (k == "careful") need_bool(&s.careful);
  else if (k == "resize_kind") {
    if (v == "fit") s.resize_kind = ResizeKind::Fit;
    else if (v == "touch") s.resize_kind = ResizeKind::Touch;
    else if (v == "exact") s.resize_kind = ResizeKind::Exact;
    else if (v == "scale") s.resize_kind = ResizeKind::Scale;
    else if (v == "width") s.resize_kind = ResizeKind::Width;
    else if (v == "height") s.resize_kind = ResizeKind::Height;
    else if (v == "none" || v.empty()) s.resize_kind = ResizeKind::None;
    else { warn("unknown resize_kind"); s.resize_kind = ResizeKind::None; }
  }
  else if (k == "resize_w") need_ulong_nonneg(&s.resize_w);
  else if (k == "resize_h") need_ulong_nonneg(&s.resize_h);
  else if (k == "scale_x") {
    double d = 0;
    if (to_double(v, &d)) s.scale_x = d; else warn("not a number");
  }
  else if (k == "scale_y") {
    double d = 0;
    if (to_double(v, &d)) s.scale_y = d; else warn("not a number");
  }
  else if (k == "resize_method") s.resize_method = v;
  else if (k == "background") s.background = v;
  else if (k == "transparent") s.transparent = v;
  else if (k == "remove_comments") need_bool(&s.remove_comments);
  else if (k == "remove_names") need_bool(&s.remove_names);
  else if (k == "remove_extensions") need_bool(&s.remove_extensions);
  else if (k == "comment") s.comments.push_back(v);
  else if (k == "input") s.inputs.push_back(v);
  else if (k == "output") s.output = v;
  else if (k == "explode_by_name") need_bool(&s.explode_by_name);
}

inline Settings load_settings(std::istream& in, std::vector<LoadWarning>* warnings = nullptr) {
  Settings s;
  std::string line;
  bool saw_position_x = false;
  bool saw_position_y = false;
  while (std::getline(in, line)) {
    std::string t = trim(line);
    if (t.empty() || t[0] == '#') continue;
    size_t eq = t.find('=');
    if (eq == std::string::npos) continue;
    std::string key = trim(t.substr(0, eq));
    std::string val = trim(t.substr(eq + 1));
    if (key.empty()) continue;
    const std::string lk = lower(key);
    if (lk == "position_x") saw_position_x = true;
    else if (lk == "position_y") saw_position_y = true;
    set_field(s, key, val, warnings);
  }
  // -p takes BOTH halves. A conf that sets only position_x (or only _y) used
  // to set has_position anyway and emit a half-specified `-p X,0`, silently
  // relocating every frame to row 0 (audit U-33). Drop the pair and say so.
  if (saw_position_x != saw_position_y) {
    if (warnings) {
      warnings->push_back(LoadWarning{
          saw_position_x ? "position_x" : "position_y", "(set without its pair)",
          "position needs both position_x and position_y; the pair was ignored"});
    }
    s.has_position = false;
    s.position_x = 0;
    s.position_y = 0;
  }
  return s;
}

// Returns nullopt if the file cannot be opened (does NOT silently return defaults).
inline std::optional<Settings> load_settings_file(const std::string& path,
                                                   std::vector<LoadWarning>* warnings = nullptr) {
  std::ifstream f(path);
  if (!f) return std::nullopt;
  return load_settings(f, warnings);
}

// Exact inverse of load_settings — same keys, enums → strings.
inline void save_settings(std::ostream& out, const Settings& s) {
  out << "# Gifscythe settings\n";

  switch (s.mode) {
    case Mode::Merge:   out << "mode = merge\n"; break;
    case Mode::Batch:   out << "mode = batch\n"; break;
    case Mode::Explode: out << "mode = explode\n"; break;
    case Mode::Auto:    out << "mode = auto\n"; break;
  }
  if (s.info) out << "info = true\n";
  if (s.interlace) out << "interlace = true\n";
  if (s.flip_horizontal) out << "flip_horizontal = true\n";
  if (s.flip_vertical) out << "flip_vertical = true\n";
  switch (s.rotation) {
    case Rotation::R90:  out << "rotation = 90\n"; break;
    case Rotation::R180: out << "rotation = 180\n"; break;
    case Rotation::R270: out << "rotation = 270\n"; break;
    case Rotation::None: break;
  }
  if (s.has_position) {
    out << "position_x = " << s.position_x << "\n";
    out << "position_y = " << s.position_y << "\n";
  }
  if (s.crop) {
    out << "crop = true\n";
    out << "crop_x = " << s.crop_x << "\n";
    out << "crop_y = " << s.crop_y << "\n";
    out << "crop_w = " << s.crop_w << "\n";
    out << "crop_h = " << s.crop_h << "\n";
    if (s.crop_transparency) out << "crop_transparency = true\n";
  }
  if (s.delay_cs >= 0) out << "delay = " << s.delay_cs << "\n";
  if (s.disposal >= 0) out << "disposal = " << s.disposal << "\n";
  if (s.loopcount >= 0) out << "loopcount = " << s.loopcount << "\n";
  if (s.optimize_level >= 0) out << "optimize = " << s.optimize_level << "\n";
  if (s.unoptimize) out << "unoptimize = true\n";
  // threads is written for every value >= 0 so that 0 ("Auto") survives a
  // save/load round trip instead of collapsing back to the -1 default. Both
  // now emit a bare -j (see GifsicleCommand.h), so the behaviour is identical;
  // this only keeps the serialised file an exact inverse of the control.
  if (s.threads >= 0) out << "threads = " << s.threads << "\n";
  if (s.color_count >= 0) out << "colors = " << s.color_count << "\n";
  if (!s.dither_method.empty()) out << "dither = " << encode_line_value(s.dither_method) << "\n";
  else if (s.dither) out << "dither = true\n";
  if (s.lossy >= 0) out << "lossy = " << s.lossy << "\n";
  if (!s.gamma_str.empty()) out << "gamma = " << encode_line_value(s.gamma_str) << "\n";
  else if (s.gamma >= 0) out << "gamma = " << s.gamma << "\n";
  if (!s.color_method.empty()) out << "color_method = " << encode_line_value(s.color_method) << "\n";
  if (s.careful) out << "careful = true\n";
  switch (s.resize_kind) {
    case ResizeKind::Fit:    out << "resize_kind = fit\n"; break;
    case ResizeKind::Touch:  out << "resize_kind = touch\n"; break;
    case ResizeKind::Exact:  out << "resize_kind = exact\n"; break;
    case ResizeKind::Scale:  out << "resize_kind = scale\n"; break;
    case ResizeKind::Width:  out << "resize_kind = width\n"; break;
    case ResizeKind::Height: out << "resize_kind = height\n"; break;
    case ResizeKind::None: break;
  }
  if (s.resize_w) out << "resize_w = " << s.resize_w << "\n";
  if (s.resize_h) out << "resize_h = " << s.resize_h << "\n";
  if (s.resize_kind == ResizeKind::Scale) {
    out << "scale_x = " << s.scale_x << "\n";
    out << "scale_y = " << s.scale_y << "\n";
  }
  if (!s.resize_method.empty()) out << "resize_method = " << encode_line_value(s.resize_method) << "\n";
  if (!s.background.empty()) out << "background = " << encode_line_value(s.background) << "\n";
  if (!s.transparent.empty()) out << "transparent = " << encode_line_value(s.transparent) << "\n";
  if (s.remove_comments) out << "remove_comments = true\n";
  if (s.remove_names) out << "remove_names = true\n";
  if (s.remove_extensions) out << "remove_extensions = true\n";
  for (const auto& c : s.comments) out << "comment = " << encode_line_value(c) << "\n";
  for (const auto& in : s.inputs) out << "input = " << encode_line_value(in) << "\n";
  if (!s.output.empty()) out << "output = " << encode_line_value(s.output) << "\n";
  if (s.explode_by_name) out << "explode_by_name = true\n";
}

inline bool save_settings_file(const std::string& path, const Settings& s) {
  std::ofstream f(path);
  if (!f) return false;
  save_settings(f, s);
  return static_cast<bool>(f);
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_SETTINGS_IO_H
