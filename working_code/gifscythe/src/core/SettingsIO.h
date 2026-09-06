// SettingsIO.h - Load/save Gifscythe Settings as a simple flat "key = value"
// text file. This is the serialization format the Qt GUI will also use to
// persist per-file settings (and to round-trip state). Qt-independent.
//
// Each line is "key = value" or "key=value"; '#' starts a comment. Unknown keys
// are ignored (forward compatible). The keys map 1:1 onto GifsicleSettings.

#ifndef GIFSYCYTHE_CORE_SETTINGS_IO_H
#define GIFSYCYTHE_CORE_SETTINGS_IO_H

#include "GifsicleSettings.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace gs {

inline std::string lower(std::string s) {
  for (auto& c : s) c = std::tolower((unsigned char)c);
  return s;
}
inline std::string trim(const std::string& s) {
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return "";
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}
inline long to_long(const std::string& s) {
  std::istringstream i(trim(s)); long v; i >> v; return v;
}
inline double to_double(const std::string& s) {
  std::istringstream i(trim(s)); double v; i >> v; return v;
}

inline void set_field(Settings& s, const std::string& key, const std::string& val) {
  const std::string k = lower(key);
  const std::string v = trim(val);
  if (k == "mode") {
    if (v == "merge") s.mode = Mode::Merge;
    else if (v == "batch") s.mode = Mode::Batch;
    else if (v == "explode") s.mode = Mode::Explode;
    else s.mode = Mode::Auto;
  }
  else if (k == "info") s.info = (v == "1" || v == "true" || v == "yes");
  else if (k == "interlace") s.interlace = (v == "1" || v == "true");
  else if (k == "flip_horizontal") s.flip_horizontal = (v == "1" || v == "true");
  else if (k == "flip_vertical") s.flip_vertical = (v == "1" || v == "true");
  else if (k == "rotation") {
    if (v == "90") s.rotation = Rotation::R90;
    else if (v == "180") s.rotation = Rotation::R180;
    else if (v == "270") s.rotation = Rotation::R270;
    else s.rotation = Rotation::None;
  }
  else if (k == "position_x") { s.position_x = (unsigned)to_long(v); s.has_position = true; }
  else if (k == "position_y") { s.position_y = (unsigned)to_long(v); s.has_position = true; }
  else if (k == "crop") s.crop = (v == "1" || v == "true");
  else if (k == "crop_x") s.crop_x = (unsigned)to_long(v);
  else if (k == "crop_y") s.crop_y = (unsigned)to_long(v);
  else if (k == "crop_w") s.crop_w = (unsigned)to_long(v);
  else if (k == "crop_h") s.crop_h = (unsigned)to_long(v);
  else if (k == "crop_transparency") s.crop_transparency = (v == "1" || v == "true");
  else if (k == "delay") s.delay_cs = (int)to_long(v);
  else if (k == "disposal") s.disposal = (int)to_long(v);
  else if (k == "loopcount") s.loopcount = (int)to_long(v);
  else if (k == "optimize") s.optimize_level = (int)to_long(v);
  else if (k == "unoptimize") s.unoptimize = (v == "1" || v == "true");
  else if (k == "threads") s.threads = (int)to_long(v);
  else if (k == "colors") s.color_count = (int)to_long(v);
  else if (k == "dither") s.dither = (v == "1" || v == "true");
  else if (k == "lossy") s.lossy = (int)to_long(v);
  else if (k == "gamma") s.gamma = to_double(v);
  else if (k == "color_method") s.color_method = v;
  else if (k == "careful") s.careful = (v == "1" || v == "true");
  else if (k == "resize_kind") {
    if (v == "fit") s.resize_kind = ResizeKind::Fit;
    else if (v == "touch") s.resize_kind = ResizeKind::Touch;
    else if (v == "exact") s.resize_kind = ResizeKind::Exact;
    else if (v == "scale") s.resize_kind = ResizeKind::Scale;
    else if (v == "width") s.resize_kind = ResizeKind::Width;
    else if (v == "height") s.resize_kind = ResizeKind::Height;
    else s.resize_kind = ResizeKind::None;
  }
  else if (k == "resize_w") s.resize_w = (unsigned)to_long(v);
  else if (k == "resize_h") s.resize_h = (unsigned)to_long(v);
  else if (k == "scale_x") s.scale_x = to_double(v);
  else if (k == "scale_y") s.scale_y = to_double(v);
  else if (k == "resize_method") s.resize_method = v;
  else if (k == "background") s.background = v;
  else if (k == "transparent") s.transparent = v;
  else if (k == "remove_comments") s.remove_comments = (v == "1" || v == "true");
  else if (k == "remove_names") s.remove_names = (v == "1" || v == "true");
  else if (k == "remove_extensions") s.remove_extensions = (v == "1" || v == "true");
  else if (k == "comment") s.comments.push_back(v);
  else if (k == "input") s.inputs.push_back(v);
  else if (k == "output") s.output = v;
  else if (k == "explode_by_name") s.explode_by_name = (v == "1" || v == "true");
}

inline Settings load_settings(std::istream& in) {
  Settings s;
  std::string line;
  while (std::getline(in, line)) {
    std::string t = trim(line);
    if (t.empty() || t[0] == '#') continue;
    size_t eq = t.find('=');
    if (eq == std::string::npos) continue;
    std::string key = trim(t.substr(0, eq));
    std::string val = trim(t.substr(eq + 1));
    if (key.empty()) continue;
    set_field(s, key, val);
  }
  return s;
}

inline Settings load_settings_file(const std::string& path) {
  std::ifstream f(path);
  if (!f) return Settings{};
  return load_settings(f);
}

}  // namespace gs

#endif  // GIFSYCYTHE_CORE_SETTINGS_IO_H
