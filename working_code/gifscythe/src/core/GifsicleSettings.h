// GifsicleSettings.h - Structured settings that mirror the Gifscythe UI
// controls. Every field maps to a gifsicle command-line option. Keeping the
// "what the user set" separate from gifsicle's options lets us (a) expose a
// live raw-CLI pane (see GifsicleCommand), and (b) model the UI on XNConvert's
// ease-of-use while retaining full terminal control.
//
// This module is Qt-INDEPENDENT (STL only) so it compiles and is unit-tested
// without the GUI toolkit.
//
// Product: Gifscythe (v0.1.0). This is the engine control layer.

#ifndef GIFSYCYTHE_CORE_GIFSICLE_SETTINGS_H
#define GIFSYCYTHE_CORE_GIFSICLE_SETTINGS_H

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
  int delay_cs = -1;              // -d, in 1/100 sec; -1 = unchanged
  int disposal = -1;              // -D, 0..7; -1 = unchanged
  int loopcount = -1;             // -l; -1 = unchanged, 0 = forever, >0 = count
  int optimize_level = -1;        // -O; -1 = none/unchanged, 0..3
  bool unoptimize = false;        // -U
  int threads = -1;               // -j; <=0 = auto

  // ---- Whole-GIF ----
  int color_count = -1;           // -k, 2..256; -1 = unchanged
  bool dither = false;            // -f
  int lossy = -1;                 // --lossy; -1 = unchanged
  double gamma = -1.0;            // --gamma; -1 = unchanged
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

}  // namespace gs

#endif  // GIFSYCYTHE_CORE_GIFSICLE_SETTINGS_H
