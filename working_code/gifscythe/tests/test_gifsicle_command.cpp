// test_gifsicle_command.cpp - Unit tests for the engine control layer.
// Builds commands from Settings and checks the produced argv/CLI string.
// Qt-independent; compiles with plain g++.

#include "../src/core/GifsicleCommand.h"
#include "../src/core/SettingsIO.h"
#include <cassert>
#include <sstream>
#include <cstdio>
#include <string>

using namespace gs;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { \
  std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++failures; } } while (0)

static bool has(const std::vector<std::string>& args, const std::string& a) {
  for (const auto& x : args) if (x == a) return true;
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
    CHECK(has(args, "-d"));
    CHECK(has(args, "5"));
    // toString should contain -m and a.gif
    std::string str = c.toString();
    CHECK(str.find("-m") != std::string::npos);
    CHECK(str.find("a.gif") != std::string::npos);
    CHECK(str.find("-O3") != std::string::npos);
    CHECK(str.find("-o out.gif") != std::string::npos);
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
    CHECK(c.toString().find("--resize-fit 320x200") != std::string::npos);
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

  // 4. Defaults (empty settings) should produce minimal command (no crash).
  {
    Settings s;
    s.inputs = {"a.gif"};
    GifsicleCommand c(s);
    auto args = c.args();
    CHECK(args.empty() || has(args, "a.gif"));
    CHECK(c.toString().find("a.gif") != std::string::npos);
  }

  // 5. Crop + transparency + background.
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

  if (failures == 0) {
    std::printf("ALL TESTS PASSED\n");
    return 0;
  }
  std::printf("%d TEST(S) FAILED\n", failures);
  return 1;
}
