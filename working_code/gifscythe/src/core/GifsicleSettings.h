// GifsicleSettings.h - Structured settings that mirror the Gifscythe UI
// controls. Every field maps to a gifsicle command-line option. Keeping the
// "what the user set" separate from gifsicle's options lets us (a) expose a
// live raw-CLI pane (see GifsicleCommand), and (b) model the UI on XNConvert's
// ease-of-use while retaining full terminal control.
//
// This module is Qt-INDEPENDENT (STL only) so it compiles and is unit-tested
// without the GUI toolkit.
//
// Product: Gifscythe. This is the engine control layer.

#ifndef GIFSCYTHE_CORE_GIFSICLE_SETTINGS_H
#define GIFSCYTHE_CORE_GIFSICLE_SETTINGS_H

#include <string>
#include <vector>
#include <cstdint>

namespace gs {

// ---- Mode: which top-level gifsicle mode to run (at most one) ----
enum class Mode {
  Auto,     // no explicit mode; gifsicle decides (merge by default)
  Merge,    // -m
  Batch,    // -b
  Explode,  // -e
};

// ---- Resize / scale ----
enum class ResizeKind {
  None,
  Fit,        // --resize-fit WxH
  Touch,      // --resize-touch WxH
  Exact,      // --resize WxH
  Scale,      // --scale XxY
  Width,      // --resize-width W
  Height,     // --resize-height H
};

// ---- Rotate ----
enum class Rotation { None, R90, R180, R270 };

// ---- The full set of UI-driven settings ----
struct Settings {
  // Mode
  Mode mode = Mode::Auto;

  // Info / verbose (not used to produce output, but part of the app)
  bool info = false;

  // ---- Frame / image ----
  bool interlace = false;
  bool flip_horizontal = false;
  bool flip_vertical = false;
  Rotation rotation = Rotation::None;
  unsigned position_x = 0;
  unsigned position_y = 0;
  bool has_position = false;

  // Crop
  bool crop = false;
  unsigned crop_x = 0;
  unsigned crop_y = 0;
  unsigned crop_w = 0;
  unsigned crop_h = 0;
  bool crop_transparency = false;

  // ---- Animation ----
  // delay_cs is in gifsicle units: 1/100 second (NOT milliseconds).
  int delay_cs = -1;              // -d, in 1/100 sec; -1 = unchanged
  int disposal = -1;              // -D, 0..7; -1 = unchanged
  int loopcount = -1;             // -l; -1 = unchanged, 0 = forever, >0 = count
  int optimize_level = -1;        // -O; -1 = none/unchanged, 0..3 (0 = off)
  bool unoptimize = false;        // -U
  int threads = -1;               // -j; <=0 = auto

  // ---- Whole-GIF ----
  int color_count = -1;           // -k, 2..256; -1 = unchanged
  bool dither = false;            // -f (default Floyd–Steinberg when true)
  std::string dither_method;      // "" = off; "floyd-steinberg"|"ro64"|... → --dither=X
  int lossy = -1;                 // --lossy; -1 = unchanged
  // gamma_str: "" = unchanged; "srgb"|"oklab"|"2.2" etc. (named or numeric)
  std::string gamma_str;
  double gamma = -1.0;            // legacy numeric; -1 = unchanged (used if gamma_str empty)
  std::string color_method;       // --color-method
  bool careful = false;           // --careful

  // Resize
  ResizeKind resize_kind = ResizeKind::None;
  unsigned resize_w = 0;
  unsigned resize_h = 0;
  double scale_x = 1.0;
  double scale_y = 1.0;
  std::string resize_method;      // --resize-method

  // Transparency / background color
  std::string background;         // --background COL (e.g. "#ffffff")
  std::string transparent;        // --transparent COL

  // Comments / names / extensions
  bool remove_comments = false;   // --no-comments
  bool remove_names = false;      // --no-names
  bool remove_extensions = false; // --no-extensions
  std::vector<std::string> comments; // --comment TEXT (per frame insert)

  // ---- Inputs / outputs ----
  std::vector<std::string> inputs;   // GIF files or frames "#0", etc.
  std::string output;                // -o FILE, or "" for stdout
  bool explode_by_name = false;      // -E
};

// Equality for round-trip tests (inputs/output/comments compared fully).
inline bool operator==(const Settings& a, const Settings& b) {
  return a.mode == b.mode
      && a.info == b.info
      && a.interlace == b.interlace
      && a.flip_horizontal == b.flip_horizontal
      && a.flip_vertical == b.flip_vertical
      && a.rotation == b.rotation
      && a.position_x == b.position_x
      && a.position_y == b.position_y
      && a.has_position == b.has_position
      && a.crop == b.crop
      && a.crop_x == b.crop_x && a.crop_y == b.crop_y
      && a.crop_w == b.crop_w && a.crop_h == b.crop_h
      && a.crop_transparency == b.crop_transparency
      && a.delay_cs == b.delay_cs
      && a.disposal == b.disposal
      && a.loopcount == b.loopcount
      && a.optimize_level == b.optimize_level
      && a.unoptimize == b.unoptimize
      && a.threads == b.threads
      && a.color_count == b.color_count
      && a.dither == b.dither
      && a.dither_method == b.dither_method
      && a.lossy == b.lossy
      && a.gamma_str == b.gamma_str
      && a.gamma == b.gamma
      && a.color_method == b.color_method
      && a.careful == b.careful
      && a.resize_kind == b.resize_kind
      && a.resize_w == b.resize_w && a.resize_h == b.resize_h
      && a.scale_x == b.scale_x && a.scale_y == b.scale_y
      && a.resize_method == b.resize_method
      && a.background == b.background
      && a.transparent == b.transparent
      && a.remove_comments == b.remove_comments
      && a.remove_names == b.remove_names
      && a.remove_extensions == b.remove_extensions
      && a.comments == b.comments
      && a.inputs == b.inputs
      && a.output == b.output
      && a.explode_by_name == b.explode_by_name;
}

inline bool operator!=(const Settings& a, const Settings& b) { return !(a == b); }

}  // namespace gs

#endif  // GIFSCYTHE_CORE_GIFSICLE_SETTINGS_H
