// test_gifsicle_command.cpp - Unit tests for the engine control layer.
// Builds commands from Settings and checks the produced argv/CLI string.
// Qt-independent; compiles with plain g++.

#include "../src/core/GifsicleCommand.h"
#include "../src/core/SettingsIO.h"
#include "../src/core/Validate.h"
#include "../src/core/version.h"
#include <cassert>
#include <sstream>
#include <cstdio>
#include <string>
#include <cmath>

using namespace gs;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { \
  std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++failures; } } while (0)

static bool has(const std::vector<std::string>& args, const std::string& a) {
  for (const auto& x : args) if (x == a) return true;
  return false;
}

static bool has_seq(const std::vector<std::string>& args, const std::string& a, const std::string& b) {
  for (size_t i = 0; i + 1 < args.size(); ++i)
    if (args[i] == a && args[i + 1] == b) return true;
  return false;
}

int main() {
  // 1. Simple merge/optimize command.
  {
    Settings s;
    s.mode = Mode::Merge;
    s.inputs = {"a.gif", "b.gif"};
    s.output = "out.gif";
    s.optimize_level = 3;
    s.loopcount = 0;
    s.delay_cs = 5;
    GifsicleCommand c(s);
    auto args = c.args();
    CHECK(has(args, "-m"));
    CHECK(has(args, "a.gif"));
    CHECK(has(args, "b.gif"));
    CHECK(has(args, "out.gif"));
    CHECK(has(args, "-O3"));
    CHECK(has(args, "--loopcount=0"));
    CHECK(has_seq(args, "-d", "5"));  // delay unit = 1/100 s, value 5 stays 5
    std::string str = c.toString();
    CHECK(str.find("-m") != std::string::npos);
    CHECK(str.find("a.gif") != std::string::npos);
    CHECK(str.find("-O3") != std::string::npos);
    CHECK(str.find("-o") != std::string::npos);
    CHECK(str.find("out.gif") != std::string::npos);
  }

  // 2. Resize-fit + colors + lossy.
  {
    Settings s;
    s.resize_kind = ResizeKind::Fit;
    s.resize_w = 320; s.resize_h = 200;
    s.color_count = 128;
    s.lossy = 60;
    s.dither = true;
    s.inputs = {"a.gif"};
    GifsicleCommand c(s);
    auto args = c.args();
    CHECK(has(args, "--resize-fit"));
    CHECK(has(args, "320x200"));
    CHECK(has(args, "-k"));
    CHECK(has(args, "128"));
    CHECK(has(args, "--lossy=60"));
    CHECK(has(args, "-f"));
    CHECK(c.toString().find("--resize-fit") != std::string::npos);
  }

  // 3. Explode mode + rotate + flip.
  {
    Settings s;
    s.mode = Mode::Explode;
    s.explode_by_name = true;
    s.rotation = Rotation::R90;
    s.flip_horizontal = true;
    s.inputs = {"a.gif"};
    GifsicleCommand c(s);
    auto args = c.args();
    CHECK(has(args, "-E"));
    CHECK(has(args, "--rotate-90"));
    CHECK(has(args, "--flip-horizontal"));
    CHECK(c.toString().find("-E") != std::string::npos);
  }

  // 4. Defaults — must include the input (NOT tautological empty||has).
  {
    Settings s;
    s.inputs = {"a.gif"};
    GifsicleCommand c(s);
    auto args = c.args();
    CHECK(!args.empty());
    CHECK(has(args, "a.gif"));
    CHECK(c.toString().find("a.gif") != std::string::npos);
  }

  // 5. Crop + transparency + background (plus-form crop syntax).
  {
    Settings s;
    s.crop = true; s.crop_x = 0; s.crop_y = 0; s.crop_w = 30; s.crop_h = 60;
    s.crop_transparency = true;
    s.background = "#ffffff";
    s.transparent = "#000000";
    s.inputs = {"a.gif"};
    GifsicleCommand c(s);
    auto args = c.args();
    CHECK(has(args, "--crop"));
    CHECK(has(args, "0,0+30x60"));
    CHECK(has(args, "--crop-transparency"));
    CHECK(has(args, "--background"));
    CHECK(has(args, "#ffffff"));
    CHECK(has(args, "--transparent"));
  }

  // 6. Settings text format is case-insensitive and ignores unknown keys.
  {
    std::istringstream input("MODE = merge\noptimize=3\nlossy=40\ninput=one.gif\nunknown=value\n");
    Settings s = load_settings(input);
    CHECK(s.mode == Mode::Merge);
    CHECK(s.optimize_level == 3);
    CHECK(s.lossy == 40);
    CHECK(s.inputs.size() == 1 && s.inputs[0] == "one.gif");
  }

  // 7. shell_quote wraps spaces; bare safe tokens stay bare.
  {
    CHECK(shell_quote("hello") == "hello");
    CHECK(shell_quote("a b").find('\'') != std::string::npos);
    Settings s;
    s.inputs = {"/tmp/my vacation/in.gif"};
    s.output = "/tmp/my vacation/out.gif";
    s.optimize_level = 2;
    GifsicleCommand c(s);
    std::string str = c.toString();
    // Display form must quote the space-containing paths.
    CHECK(str.find("'") != std::string::npos);
    // argv form must keep raw paths (no quotes baked in).
    auto args = c.args();
    CHECK(has(args, "/tmp/my vacation/in.gif"));
    CHECK(has(args, "/tmp/my vacation/out.gif"));
  }

  // 8. Malformed numeric values keep defaults (no UB) + warnings.
  {
    std::vector<LoadWarning> w;
    std::istringstream input("lossy = abc\noptimize = xyz\ncolors = 999\ninput=a.gif\n");
    Settings s = load_settings(input, &w);
    CHECK(s.lossy == -1);           // unchanged default
    CHECK(s.optimize_level == -1);  // unchanged default
    CHECK(!w.empty());              // warnings emitted
    auto vw = validate(s);
    bool colors_warned = false;
    for (const auto& x : vw) if (x.field == "colors") colors_warned = true;
    // colors=999 was accepted into the field but validate flags it
    // (set_field does accept 999 into color_count)
    CHECK(s.color_count == 999);
    CHECK(colors_warned);
  }

  // 9. save/load round-trip.
  {
    Settings s;
    s.mode = Mode::Batch;
    s.optimize_level = 3;
    s.lossy = 40;
    s.loopcount = 0;
    s.delay_cs = 5;
    s.dither = true;
    s.color_count = 128;
    s.inputs = {"one.gif", "two.gif"};
    s.output = "out.gif";
    s.careful = true;
    s.rotation = Rotation::R180;
    s.gamma_str = "srgb";
    std::ostringstream oss;
    save_settings(oss, s);
    std::istringstream iss(oss.str());
    Settings t = load_settings(iss);
    CHECK(t.mode == Mode::Batch);
    CHECK(t.optimize_level == 3);
    CHECK(t.lossy == 40);
    CHECK(t.loopcount == 0);
    CHECK(t.delay_cs == 5);
    CHECK(t.dither == true);
    CHECK(t.color_count == 128);
    CHECK(t.inputs.size() == 2);
    CHECK(t.output == "out.gif");
    CHECK(t.careful == true);
    CHECK(t.rotation == Rotation::R180);
    CHECK(t.gamma_str == "srgb");
  }

  // 10. load_settings_file missing file → nullopt (not silent defaults).
  {
    auto r = load_settings_file("/no/such/path/gifscythe_missing_test.conf");
    CHECK(!r.has_value());
  }

  // 11. Prvalue construction is safe (Settings stored by value).
  {
    auto make = []() {
      Settings s;
      s.inputs = {"prvalue.gif"};
      s.optimize_level = 1;
      return s;
    };
    GifsicleCommand c(make());  // temporary — must not dangle
    auto args = c.args();
    CHECK(has(args, "prvalue.gif"));
    CHECK(has(args, "-O") || has(args, "-O1"));
  }

  // 12. -O0 emitted for optimize_level 0 ("off").
  {
    Settings s;
    s.optimize_level = 0;
    s.inputs = {"a.gif"};
    GifsicleCommand c(s);
    CHECK(has(c.args(), "-O0"));
  }

  // 13. Named gamma + dither method.
  {
    Settings s;
    s.gamma_str = "oklab";
    s.dither_method = "ro64";
    s.inputs = {"a.gif"};
    GifsicleCommand c(s);
    auto args = c.args();
    CHECK(has(args, "--gamma=oklab"));
    CHECK(has(args, "--dither=ro64"));
  }

  // 14. Batch mode flag.
  {
    Settings s;
    s.mode = Mode::Batch;
    s.inputs = {"a.gif"};
    GifsicleCommand c(s);
    CHECK(has(c.args(), "-b"));
  }

  // 15. Bool parsing accepts yes/on consistently; rotation none.
  {
    std::istringstream input("info = yes\ncareful = on\nrotation = none\ninput=a.gif\n");
    Settings s = load_settings(input);
    CHECK(s.info == true);
    CHECK(s.careful == true);
    CHECK(s.rotation == Rotation::None);
  }

  // 16. version.h is populated.
  {
    CHECK(std::string(GS_VERSION).size() >= 3);
    CHECK(std::string(GS_VERSION).find('.') != std::string::npos);
  }

  // 17. validate catches info+mode conflict.
  {
    Settings s;
    s.info = true;
    s.mode = Mode::Merge;
    s.inputs = {"a.gif"};
    auto w = validate(s);
    bool found = false;
    for (const auto& x : w) if (x.field == "info") found = true;
    CHECK(found);
  }

  // 18. Negative crop / colors rejected by parser.
  {
    std::vector<LoadWarning> w;
    std::istringstream input("crop_w = -5\ncolors = -1\ninput=a.gif\n");
    Settings s = load_settings(input, &w);
    CHECK(s.crop_w == 0);  // rejected, stayed default
    CHECK(!w.empty());
  }

  if (failures == 0) {
    std::printf("ALL TESTS PASSED\n");
    return 0;
  }
  std::printf("%d TEST(S) FAILED\n", failures);
  return 1;
}
