// test_gifsicle_command.cpp - Unit tests for the engine control layer.
// Builds commands from Settings and checks the produced argv/CLI string.
// Qt-independent; compiles with plain g++.

#include "../src/core/GifsicleCommand.h"
#include "../src/core/SettingsIO.h"
#include "../src/core/ProcessRunner.h"
#include "../src/core/Validate.h"
#include "../src/core/OutputName.h"
#include "../src/core/OutputPlan.h"
#include "../src/core/ExplodeVerify.h"
#include "../src/core/WinUnicode.h"
#include "core/version.h"  // path form: generated-first under CMake (U-15)
#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <string>
#include <cmath>

using namespace gs;

static int failures = 0;
static int checks = 0;
#define CHECK(cond) do { ++checks; if (!(cond)) { \
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

// Bounds-safe plan assertions: if the planner stops reporting an issue these
// return false (a clean FAIL) instead of reading past the end of the vector.
static bool issue_is(const gs::OutputPlan& p, size_t i, gs::PlanIssueKind k) {
  return i < p.issues.size() && p.issues[i].kind == k;
}
static bool issue_other_index(const gs::OutputPlan& p, size_t i, size_t want) {
  return i < p.issues.size() && p.issues[i].other_index == want;
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

  // 19. Windows argv quoting (win_quote_arg) — MSVCRT re-split rules.
  //     Regression guard for the _spawnvp space-splitting bug found via
  //     Wine E2E on 2026-09-07 (paths with spaces were split in two).
  {
    // Plain args pass through unchanged.
    CHECK(win_quote_arg("-O3") == "-O3");
    CHECK(win_quote_arg("C:\\gs\\in\\a.gif") == "C:\\gs\\in\\a.gif");
    // Empty arg must become an explicit quoted empty string.
    CHECK(win_quote_arg("") == "\"\"");
    // Spaces force quoting (the CI/Wine bug).
    CHECK(win_quote_arg("C:\\gs\\out\\my vacation\\b_opt.gif") ==
          "\"C:\\gs\\out\\my vacation\\b_opt.gif\"");
    // Tabs also force quoting.
    CHECK(win_quote_arg("a\tb") == "\"a\tb\"");
    // Embedded quotes are escaped as \" (no space -> still must quote).
    CHECK(win_quote_arg("say \"hi\"") == "\"say \\\"hi\\\"\"");
    // Trailing backslashes before the closing quote are doubled so the
    // quote cannot be escaped by the path itself.
    CHECK(win_quote_arg("my dir\\") == "\"my dir\\\\\"");
    // Backslashes NOT before a quote stay single.
    CHECK(win_quote_arg("a\\b c") == "\"a\\b c\"");
  }

  // 20. Unknown keys are ignored WITHOUT warnings (forward compatibility).
  //     The Qt GUI persists its own state keys (batch_dir, name_template)
  //     alongside the core settings in the same file format; the CLI must
  //     tolerate them silently (S7 settings-persistence design).
  {
    std::vector<LoadWarning> w;
    std::istringstream input(
        "mode = batch\n"
        "optimize = 2\n"
        "batch_dir = /tmp/out dir\n"
        "name_template = {name}_small.gif\n"
        "input = a.gif\n");
    Settings s = load_settings(input, &w);
    CHECK(s.mode == Mode::Batch);
    CHECK(s.optimize_level == 2);
    CHECK(s.inputs.size() == 1);
    CHECK(w.empty());  // GUI keys must not raise load warnings
  }

  // 21. Threads mapping (audit U-03). "Auto" (<=0) MUST emit a bare -j.
  //     The engine's own default is thread_count = 0 = single-threaded
  //     (gifsicle.c:39, used at xform.c:1329); a bare -j selects
  //     GIFSICLE_DEFAULT_THREAD_COUNT = 8 (gifsicle.c:38, :1893). Emitting
  //     nothing therefore made the control labelled "Auto" run one thread.
  {
    Settings def;                       // threads = -1 by default
    CHECK(def.threads == -1);
    CHECK(has(GifsicleCommand(def).args(), "-j"));

    Settings auto0; auto0.threads = 0;  // the GUI spinner's "Auto"
    CHECK(has(GifsicleCommand(auto0).args(), "-j"));

    Settings four; four.threads = 4;
    auto a4 = GifsicleCommand(four).args();
    CHECK(has(a4, "-j4"));
    CHECK(!has(a4, "-j"));              // explicit N must not also add bare -j

    Settings one; one.threads = 1;      // single-threaded stays expressible
    CHECK(has(GifsicleCommand(one).args(), "-j1"));
  }

  // 22. Output planning — the data-destruction guard (audit U-01).
  {
    namespace f = std::filesystem;
    const f::path dir = f::temp_directory_path() / "gs_plan_test";
    f::remove_all(dir);
    f::create_directories(dir / "d1");
    f::create_directories(dir / "d2");
    f::create_directories(dir / "out");
    for (const char* p : {"d1/hero.gif", "d2/hero.gif", "a.gif", "b.gif", "exists.gif"})
      std::ofstream(dir / p) << "x";

    const std::string a  = (dir / "a.gif").string();
    const std::string b  = (dir / "b.gif").string();
    const std::string h1 = (dir / "d1/hero.gif").string();
    const std::string h2 = (dir / "d2/hero.gif").string();
    const std::string ex = (dir / "exists.gif").string();
    const std::string o  = (dir / "out/hero_opt.gif").string();

    // (a) The headline repro: the output IS the input.
    OutputPlan self = plan_outputs({a}, {a});
    CHECK(!self.ok);
    CHECK(self.issues.size() == 1);
    CHECK(issue_is(self, 0, PlanIssueKind::TargetsSource));
    CHECK(self.describe().find("source file") != std::string::npos);

    // (b) Two different files, same base name, one batch folder -> one target.
    OutputPlan dup = plan_outputs({h1, h2}, {o, o});
    CHECK(!dup.ok);
    CHECK(dup.issues.size() == 1);
    CHECK(issue_is(dup, 0, PlanIssueKind::DuplicateTarget));

    // (c) The same batch with distinct names is fine.
    OutputPlan good = plan_outputs({h1, h2},
                                   {(dir / "out/1.gif").string(), (dir / "out/2.gif").string()});
    CHECK(good.ok);
    CHECK(good.issues.empty());
    CHECK(good.outputs.size() == 2);

    // (d) Merge shape: N inputs, ONE output — legal, and still guarded.
    CHECK(plan_outputs({a, b}, {(dir / "out/m.gif").string()}).ok);
    OutputPlan mergeSelf = plan_outputs({a, b}, {b});   // welds a+b over b
    CHECK(!mergeSelf.ok);
    CHECK(issue_is(mergeSelf, 0, PlanIssueKind::TargetsSource));
    CHECK(issue_other_index(mergeSelf, 0, 1));        // names b, not a

    // (e) Empty target and count mismatch.
    CHECK(!plan_outputs({a}, {""}).ok);
    CHECK(issue_is(plan_outputs({a}, {""}), 0, PlanIssueKind::EmptyTarget));
    OutputPlan cnt = plan_outputs({a, b}, {(dir / "out/x.gif").string(),
                                           (dir / "out/y.gif").string(),
                                           (dir / "out/z.gif").string()});
    CHECK(!cnt.ok);
    CHECK(issue_is(cnt, 0, PlanIssueKind::CountMismatch));

    // (f) An output that already exists is REPORTED, not refused — re-running
    //     an optimize legitimately replaces the previous result.
    OutputPlan pre = plan_outputs({a}, {ex});
    CHECK(pre.ok);
    CHECK(pre.preexisting.size() == 1);
    CHECK(plan_outputs({a}, {(dir / "out/brand_new.gif").string()}).preexisting.empty());

    // (g) Path spelling must not defeat the guard: "./x", "x" and an absolute
    //     path to the same file are the same target.
    const std::string rel = "gs_plan_test/a.gif";
    const f::path saved_cwd = f::current_path();
    f::current_path(f::temp_directory_path());
    OutputPlan spelled = plan_outputs({rel}, {a});
    CHECK(!spelled.ok);
    CHECK(issue_is(spelled, 0, PlanIssueKind::TargetsSource));
    f::current_path(saved_cwd);
    f::remove_all(dir);
  }

  // 23. Resize geometry is validated (audit U-22). Verified against the engine:
  //     --resize-fit 0x0 -> rc=1, but 40x0 is legal; --scale 0x1 -> rc=0 and
  //     SILENTLY DOES NOTHING.
  {
    auto warns = [](const Settings& s, const char* field) {
      for (const auto& w : validate(s)) if (w.field == field) return true;
      return false;
    };
    Settings fit0; fit0.inputs = {"a.gif"}; fit0.resize_kind = ResizeKind::Fit;
    CHECK(warns(fit0, "resize"));                       // 0x0

    Settings fit40 = fit0; fit40.resize_w = 40;
    CHECK(!warns(fit40, "resize"));                     // 40x0 keeps the aspect

    Settings w0; w0.inputs = {"a.gif"}; w0.resize_kind = ResizeKind::Width;
    CHECK(warns(w0, "resize_w"));

    Settings h0; h0.inputs = {"a.gif"}; h0.resize_kind = ResizeKind::Height;
    CHECK(warns(h0, "resize_h"));

    Settings sc; sc.inputs = {"a.gif"}; sc.resize_kind = ResizeKind::Scale; sc.scale_x = 0.0;
    CHECK(warns(sc, "scale"));

    Settings none; none.inputs = {"a.gif"};             // ResizeKind::None
    CHECK(!warns(none, "resize"));
    CHECK(!warns(none, "scale"));
  }

  // 24. Malformed booleans warn instead of silently meaning "false"
  //     (audit U-11: `unoptimize = maybe` used to turn the option OFF, rc=0,
  //     zero warnings, while every numeric parser in the file warns).
  {
    std::vector<LoadWarning> w;
    std::istringstream in(
        "unoptimize = maybe\ncareful = Trueish\ninterlace = yes-please\n"
        "remove_comments = TRUE\ninfo = off\ninput = a.gif\n");
    Settings s = load_settings(in, &w);
    CHECK(w.size() == 3);
    CHECK(s.unoptimize == false);   // untouched, and the user was told
    CHECK(s.careful == false);
    CHECK(s.interlace == false);
    CHECK(s.remove_comments == true);   // case-insensitive true still works
    CHECK(s.info == false);             // explicit false still works
    // Every recognised spelling, both ways.
    for (const char* t : {"1", "true", "TRUE", "yes", "On"}) {
      bool v = false; CHECK(parse_bool_strict(t, &v)); CHECK(v == true);
    }
    for (const char* t : {"0", "false", "FALSE", "no", "Off"}) {
      bool v = true; CHECK(parse_bool_strict(t, &v)); CHECK(v == false);
    }
    bool v = true;
    CHECK(!parse_bool_strict("maybe", &v)); CHECK(v == true);  // left untouched
  }

  // 25. -p needs BOTH halves (audit U-33): a conf setting only position_x used
  //     to yield a half-specified `-p X,0`, silently moving frames to row 0.
  {
    std::vector<LoadWarning> w;
    std::istringstream in("position_x = 12\ninput = a.gif\n");
    Settings s = load_settings(in, &w);
    CHECK(!s.has_position);
    CHECK(!w.empty());
    CHECK(!has(GifsicleCommand(s).args(), "-p"));

    std::vector<LoadWarning> w2;
    std::istringstream in2("position_x = 12\nposition_y = 7\ninput = a.gif\n");
    Settings s2 = load_settings(in2, &w2);
    CHECK(s2.has_position);
    CHECK(has_seq(GifsicleCommand(s2).args(), "-p", "12,7"));
  }

  // 26. Settings round trip stays exact, including the toggle-dependent groups
  //     and the threads value (audit U-19 correction + U-03).
  {
    Settings s;
    s.crop = true; s.crop_x = 3; s.crop_y = 4; s.crop_w = 200; s.crop_h = 150;
    s.crop_transparency = true;
    s.has_position = true; s.position_x = 12; s.position_y = 7;
    s.resize_kind = ResizeKind::Scale; s.scale_x = 0.5; s.scale_y = 0.25;
    s.threads = 0;                       // "Auto" must survive, not collapse to -1
    std::ostringstream o; save_settings(o, s);
    std::vector<LoadWarning> w;
    std::istringstream i(o.str());
    Settings r = load_settings(i, &w);
    CHECK(w.empty());
    CHECK(r.crop && r.crop_x == 3 && r.crop_y == 4 && r.crop_w == 200 && r.crop_h == 150);
    CHECK(r.crop_transparency);
    CHECK(r.has_position && r.position_x == 12 && r.position_y == 7);
    CHECK(r.resize_kind == ResizeKind::Scale);
    CHECK(r.scale_x == 0.5 && r.scale_y == 0.25);
    CHECK(r.threads == 0);
    CHECK(has(GifsicleCommand(r).args(), "-j"));
  }

  // 27. An empty comment must not emit a bare --comment (audit U-13/U-48).
  //     add() drops empty strings, so a "" entry used to push the flag with no
  //     operand, which then swallowed the following argument as its text.
  {
    Settings s;
    s.inputs = {"a.gif"};
    s.output = "o.gif";
    s.comments = {"", "real one"};
    auto args = GifsicleCommand(s).args();
    int n = 0;
    for (const auto& a : args) if (a == "--comment") ++n;
    CHECK(n == 1);                                  // only the real comment
    CHECK(has_seq(args, "--comment", "real one"));  // and it kept its operand
    CHECK(GifsicleCommand(s).toString().find("--comment --comment") == std::string::npos);

    Settings none;
    none.inputs = {"a.gif"};
    none.comments = {""};
    CHECK(!has(GifsicleCommand(none).args(), "--comment"));
  }

  // 28. A child killed by a signal reports 128+signum, not a flat 1
  //     (audit U-32). The GUI's cancel path kills the engine on purpose, so
  //     callers need to be able to tell a crash from a kill.
#ifndef _WIN32
  {
    // run_argv() correctly reports a signalled child on stderr, but that makes a
    // GREEN run print three scary "ERROR:" lines. Silence fd 2 for these calls.
    std::fflush(stderr);
    const int saved_err = ::dup(STDERR_FILENO);
    const int devnull = ::open("/dev/null", O_WRONLY);
    if (devnull >= 0) ::dup2(devnull, STDERR_FILENO);

    // SIGTERM (15) -> 143. The child signals itself, so this needs no shell
    // pipeline and no root.
    const int rc = run_argv({"/bin/sh", "-c", "kill -TERM $$"});
    // SIGKILL (9) -> 137.
    const int rc2 = run_argv({"/bin/sh", "-c", "kill -KILL $$"});
    // A clean exit still passes straight through and is NOT in the 128+ range.
    const int rc3 = run_argv({"/bin/sh", "-c", "exit 3"});
    // A missing binary is still 127, distinct from both.
    const int rc4 = run_argv({"/definitely/not/here/gifscythe"});

    std::fflush(stderr);
    if (devnull >= 0) { ::dup2(saved_err, STDERR_FILENO); ::close(devnull); }
    ::close(saved_err);

    CHECK(rc == 128 + 15);
    CHECK(rc2 == 128 + 9);
    CHECK(rc3 == 3);
    CHECK(rc4 == 127);
  }
#endif

  // 29. Output-name sanitisation (audit U-21). The rules are a parameter, so
  //     the whole Win32 rule set is testable on a POSIX host.
  {
    using R = NameRules;
    // Directory escape is stripped under BOTH rule sets (pre-existing behaviour).
    CHECK(sanitize_output_name("../../evil", R::Posix) == "evil");
    CHECK(sanitize_output_name("../../evil", R::Windows) == "evil");
    CHECK(sanitize_output_name("..\\..\\evil.gif", R::Windows) == "evil.gif");
    CHECK(sanitize_output_name("a/b/c.gif", R::Posix) == "c.gif");

    // Characters Win32 refuses are removed on Windows only — POSIX allows them,
    // so a Linux user's file name is not silently rewritten.
    CHECK(sanitize_output_name("a<b>c.gif", R::Windows) == "abc.gif");
    CHECK(sanitize_output_name("a<b>c.gif", R::Posix) == "a<b>c.gif");
    CHECK(sanitize_output_name("q:u?o\"t|e*.gif", R::Windows) == "quote.gif");
    CHECK(sanitize_output_name("tab\there.gif", R::Windows) == "tabhere.gif");

    // Trailing dots/spaces: Win32 strips them, so we strip them first and the
    // name we display is the name that ends up on disk.
    CHECK(sanitize_output_name("trail...   ", R::Windows) == "trail");
    CHECK(sanitize_output_name("trail...   ", R::Posix) == "trail...   ");

    // Reserved device names are defused, not deleted, and matched on the stem
    // before the first dot, case-insensitively.
    CHECK(sanitize_output_name("CON", R::Windows) == "_CON");
    CHECK(sanitize_output_name("con.gif", R::Windows) == "_con.gif");
    CHECK(sanitize_output_name("LPT9.gif", R::Windows) == "_LPT9.gif");
    CHECK(sanitize_output_name("Com1.txt", R::Windows) == "_Com1.txt");
    CHECK(sanitize_output_name("NUL", R::Windows) == "_NUL");
    CHECK(sanitize_output_name("console.gif", R::Windows) == "console.gif");
    CHECK(sanitize_output_name("mycon.gif", R::Windows) == "mycon.gif");
    CHECK(sanitize_output_name("CON", R::Posix) == "CON");   // legal on POSIX

    // Nothing usable left -> "" so the caller falls back to its own default.
    CHECK(sanitize_output_name("", R::Windows).empty());
    CHECK(sanitize_output_name("...", R::Windows).empty());
    CHECK(sanitize_output_name("<>:\"|?*", R::Windows).empty());
    CHECK(sanitize_output_name("..", R::Posix).empty());
    CHECK(sanitize_output_name("/", R::Posix).empty());

    // Non-ASCII passes through untouched: every byte the rules act on is ASCII.
    CHECK(sanitize_output_name("\xe4\xbd\x9c\xe5\x93\x81.gif", R::Windows) ==
          "\xe4\xbd\x9c\xe5\x93\x81.gif");

    // The reserved-name predicate on its own.
    CHECK(is_windows_reserved_device_name("aux"));
    CHECK(is_windows_reserved_device_name("PRN.log"));
    CHECK(!is_windows_reserved_device_name("auxiliary"));
    CHECK(!is_windows_reserved_device_name("coma"));
    CHECK(!is_windows_reserved_device_name(""));

    // Host rules resolve to POSIX here (this suite runs on Linux in CI's linux
    // job; the Windows job exercises the same code through NameRules::Windows).
    CHECK(sanitize_output_name("a<b>.gif", NameRules::Host) ==
          sanitize_output_name("a<b>.gif", host_name_rules()));
  }

  // 30. A string value containing a newline must not become a new key on
  //     reload (audit U-51). Verified before the fix: one comment
  //     "hi\nmode = merge" round-tripped into `mode = merge`.
  {
    Settings s;
    s.mode = Mode::Auto;
    s.inputs = {"a.gif"};
    s.output = "o.gif";
    s.comments = {"hi\nmode = merge", "second\r\noptimize = 3"};
    s.background = "#ff\ninfo = true";
    std::ostringstream o; save_settings(o, s);
    const std::string text = o.str();
    // Exactly one line per key — no value may have introduced another.
    int mode_lines = 0, info_lines = 0, comment_lines = 0;
    std::istringstream scan(text);
    std::string line;
    while (std::getline(scan, line)) {
      if (line.rfind("mode = ", 0) == 0) ++mode_lines;
      if (line.rfind("info = ", 0) == 0) ++info_lines;
      if (line.rfind("comment = ", 0) == 0) ++comment_lines;
    }
    CHECK(mode_lines == 1);
    CHECK(info_lines == 0);
    CHECK(comment_lines == 2);

    std::istringstream i(text);
    Settings r = load_settings(i);
    CHECK(r.mode == Mode::Auto);        // not hijacked into Merge
    CHECK(r.info == false);             // not hijacked on
    CHECK(r.optimize_level == -1);      // not hijacked to 3
    CHECK(r.comments.size() == 2);
    CHECK(r.background == "#ff info = true");

    // Plain values are untouched — the fix must not perturb normal files.
    Settings plain;
    plain.comments = {"a normal comment"};
    plain.background = "#abcdef";
    plain.inputs = {"/some path/in.gif"};
    std::ostringstream o2; save_settings(o2, plain);
    CHECK(o2.str().find("comment = a normal comment\n") != std::string::npos);
    CHECK(o2.str().find("background = #abcdef\n") != std::string::npos);
    CHECK(o2.str().find("input = /some path/in.gif\n") != std::string::npos);
  }

  // 31. Unknown keys are collected for the GUI (audit U-36 / P3-7). set_field
  //     now reports whether a key is recognised, and load_settings fills an
  //     extra_keys map: lowercased key, trimmed value, last occurrence wins —
  //     the same override semantics the known keys have. The GUI's third
  //     parser (guiStateKey in MainWindow.cpp) is gone; this is the one
  //     channel for GUI-only keys like batch_dir / name_template.
  {
    Settings dummy;
    CHECK(set_field(dummy, "mode", "batch") == true);    // known core key
    CHECK(set_field(dummy, "batch_dir", "/x") == false);  // GUI key: unknown to core
    CHECK(set_field(dummy, "no_such_key", "1") == false);

    std::istringstream in(
        "# a GUI-written conf\n"
        "mode = merge\n"
        "batch_dir = /first/path\n"
        "Name_Template = {name}_x.gif\n"
        "batch_dir = /second/path\n"
        "future_key = still ignored\n");
    std::vector<LoadWarning> w;
    std::map<std::string, std::string> extra;
    Settings s = load_settings(in, &w, &extra);
    CHECK(s.mode == Mode::Merge);          // known keys still apply as before
    CHECK(w.empty());
    CHECK(extra.size() == 3);              // batch_dir, name_template, future_key
    CHECK(extra["batch_dir"] == "/second/path");   // last occurrence wins
    CHECK(extra["name_template"] == "{name}_x.gif");  // key matched case-insensitively
    CHECK(extra.count("future_key") == 1);
    CHECK(extra.count("mode") == 0);       // known keys never leak into extras

    // The file-level entry point passes the channel through...
    const std::string p =
        (std::filesystem::temp_directory_path() / "gs_extra_keys_test.conf").string();
    {
      std::ofstream out(p);
      out << "lossy = 30\nbatch_dir = /gui/dir\n";
    }
    std::map<std::string, std::string> extra2;
    auto loaded = load_settings_file(p, nullptr, &extra2);
    CHECK(loaded.has_value());
    CHECK(loaded->lossy == 30);
    CHECK(extra2["batch_dir"] == "/gui/dir");
    std::remove(p.c_str());
    // ...and a missing file is still nullopt, not silent defaults (test 10).
    std::map<std::string, std::string> extra3;
    CHECK(!load_settings_file("/no/such/path/gifscythe_missing_test.conf", nullptr, &extra3)
               .has_value());
    CHECK(extra3.empty());
  }

  // 32. Atomic save_settings_file (audit U-16 / P1-18): write tmp + fsync +
  //     rename, so a reader sees either the old file or the complete new one —
  //     never a truncated mix. The pre-fix form (ofstream Truncate + write)
  //     destroyed the previous file before the new bytes existed.
  {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "gs_atomic_save_test";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string p = (dir / "gifscythe.conf").string();

    Settings s;
    s.mode = Mode::Batch;
    s.lossy = 30;
    s.comments = {"hello"};
    CHECK(save_settings_file(p, s) == true);
    CHECK(fs::exists(p));
    CHECK(!fs::exists(p + ".tmp"));  // success leaves no stray temp file

    auto r = load_settings_file(p);
    CHECK(r.has_value());
    CHECK(r->mode == Mode::Batch && r->lossy == 30 && r->comments.size() == 1);

    // Overwriting an existing file works (rename must REPLACE: POSIX rename
    // does; Windows needs fs::rename's MoveFileEx(REPLACE_EXISTING) path).
    Settings s2;
    s2.mode = Mode::Merge;
    CHECK(save_settings_file(p, s2) == true);
    auto r2 = load_settings_file(p);
    CHECK(r2.has_value() && r2->mode == Mode::Merge);
    CHECK(!fs::exists(p + ".tmp"));

    // Fail closed, clean up after itself: the target is a DIRECTORY, so the
    // rename cannot replace it. save must return false, leave the target
    // untouched, and remove its own .tmp stray (the error branch is what
    // keeps a failed save from littering next to the user's conf).
    const std::string as_dir = (dir / "adir.conf").string();
    fs::create_directories(as_dir);
    CHECK(save_settings_file(as_dir, s) == false);
    CHECK(!fs::exists(as_dir + ".tmp"));
    CHECK(fs::is_directory(as_dir));

    // Empty path: false, no crash.
    CHECK(save_settings_file("", s) == false);

    fs::remove_all(dir);
  }

  // 33. Explode frame verification (audit U-17 / P1-19). The rule: snapshot
  //     the prefix candidates BEFORE the run, then require at least one NEW
  //     or CHANGED file that is a real GIF. rc=0 alone must never mean
  //     "frames written", and stale leftovers must never fake a success.
  {
    namespace fs = std::filesystem;

    // Prefix derivation: explicit output IS the prefix; empty output falls
    // back to the first input's basename (gifsicle explodes into the CWD),
    // cutting on either separator so the rule tests on any platform.
    Settings s;
    s.mode = Mode::Explode;
    s.output = "/tmp/frames/p";
    s.inputs = {"/tmp/in/anim.gif"};
    CHECK(explode_prefix_for(s) == "/tmp/frames/p");
    s.output = "";
    CHECK(explode_prefix_for(s) == "anim.gif");
    s.inputs = {"C:\\Users\\me\\anim.gif"};
    CHECK(explode_prefix_for(s) == "anim.gif");
    s.inputs = {};
    CHECK(explode_prefix_for(s).empty());
    // Empty prefix verifies as a failure and says why.
    ExplodeResult empty_r = verify_explode_frames("", {});
    CHECK(!empty_r.ok);
    CHECK(!empty_r.describe().empty());

    const fs::path dir = fs::temp_directory_path() / "gs_explode_verify_test";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string prefix = (dir / "p").string();

    // Nothing on disk -> not ok, and the failure text names the prefix.
    auto before = snapshot_explode_candidates(prefix);
    CHECK(before.empty());
    ExplodeResult r = verify_explode_frames(prefix, before);
    CHECK(!r.ok);
    CHECK(r.frames.empty());
    CHECK(r.describe().find(prefix) != std::string::npos);

    // A pre-existing (stale) frame the run did NOT touch is not a success —
    // this is the lie the snapshot exists to prevent.
    { std::ofstream f(prefix + ".000", std::ios::binary); f << "GIF89a-stale"; }
    before = snapshot_explode_candidates(prefix);
    CHECK(before.size() == 1);
    r = verify_explode_frames(prefix, before);
    CHECK(!r.ok);
    CHECK(r.frames.empty() && r.suspicious.empty());

    // NEW valid frames count: numeric (-e) and by-name (-E) forms alike.
    { std::ofstream f(prefix + ".001", std::ios::binary); f << "GIF89a\x01\x00"; }
    { std::ofstream f(prefix + ".myframe.gif", std::ios::binary); f << "GIF87a-old"; }
    r = verify_explode_frames(prefix, before);
    CHECK(r.ok);
    CHECK(r.frames.size() == 2);

    // NEW but empty or non-GIF files are "suspicious" and do NOT make it ok.
    fs::remove(prefix + ".001");
    fs::remove(prefix + ".myframe.gif");
    { std::ofstream f(prefix + ".002", std::ios::binary); f << ""; }
    { std::ofstream f(prefix + ".003", std::ios::binary); f << "PNG-not-a-gif"; }
    r = verify_explode_frames(prefix, before);
    CHECK(!r.ok);
    CHECK(r.suspicious.size() == 2);
    CHECK(r.describe().find("none is a valid GIF") != std::string::npos);

    // A CHANGED pre-existing file counts as written by this run.
    { std::ofstream f(prefix + ".000", std::ios::binary); f << "GIF89a-fresh-longer"; }
    r = verify_explode_frames(prefix, before);
    CHECK(r.ok);
    CHECK(r.frames.size() == 1);

    // A non-candidate next door never interferes.
    { std::ofstream f((dir / "unrelated.txt").string(), std::ios::binary); f << "x"; }
    r = verify_explode_frames(prefix, before);
    CHECK(r.ok && r.frames.size() == 1);

    fs::remove_all(dir);
  }

  // 34. Windows command-line splitter (audit U-07 / P1-4): split_win_cmdline
  //     must be the exact inverse of win_quote_arg (test 19) and follow the
  //     MSVCRT argv rules. Pure logic — runs on Linux CI; the Wine E2E proves
  //     the win32 shims around it.
  {
    auto widen = [](const std::string& s) { return std::wstring(s.begin(), s.end()); };
    auto narrow = [](const std::wstring& w) { return std::string(w.begin(), w.end()); };
    auto split1 = [&](const std::string& line) {
      std::vector<std::string> out;
      for (const auto& t : gs::split_win_cmdline(widen(line))) out.push_back(narrow(t));
      return out;
    };

    // Canonical vectors.
    auto v = split1(R"("C:\Program Files\gifsicle.exe" -e "in file.gif")");
    CHECK(v.size() == 3);
    if (v.size() == 3) {
      CHECK(v[0] == R"(C:\Program Files\gifsicle.exe)");
      CHECK(v[1] == "-e");
      CHECK(v[2] == "in file.gif");
    }
    v = split1("one two\tthree   ");          // tabs + trailing space
    CHECK(v.size() == 3 && v[2] == "three");
    v = split1(R"(a\\b)");                    // backslashes NOT before a quote
    CHECK(v.size() == 1 && v[0] == R"(a\\b)");
    v = split1(R"(a\\\"b)");                  // 2n+1 backslashes + " -> n \ + literal "
    CHECK(v.size() == 1 && v[0] == "a\\\"b");
    v = split1(R"(a\\"b")");                  // 2n backslashes + " -> n \ + toggle
    CHECK(v.size() == 1 && v[0] == R"(a\b)");
    v = split1(R"("trail\\")");               // trailing backslash run in quotes
    CHECK(v.size() == 1 && v[0] == "trail\\");
    v = split1(R"(x "" y)");                  // explicit empty argument
    CHECK(v.size() == 3 && v[1].empty());
    v = split1(R"("a""b")");                  // doubled quote inside quotes
    CHECK(v.size() == 1 && v[0] == "a\"b");

    // Round-trip: quoting any argument and splitting the line must return it
    // byte-for-byte — including UTF-8 bytes (the splitter is byte-transparent;
    // the win32 shims do the real UTF-16 conversion).
    const std::vector<std::string> tricky = {
      "plain", "with space", "quote\"inside", "trailing\\", "back\\\\slash",
      "", "  padded  ", "semi;colon", "\ttab\t", "anim\xc3\xa9.gif",
      "\xe5\x8b\x95\xe7\x94\xbb.gif",  // 動画.gif in UTF-8
    };
    for (const auto& a : tricky) {
      const std::string line = gs::win_quote_arg(a);
      auto back = split1(line);
      CHECK(back.size() == 1);
      if (back.size() == 1) CHECK(back[0] == a);
    }
    // A full argv survives join-then-split (this is exactly what
    // ProcessRunner builds and what the child CRT re-splits).
    std::string line;
    for (size_t i = 0; i < tricky.size(); ++i) {
      if (i) line += ' ';
      line += gs::win_quote_arg(tricky[i]);
    }
    auto back = split1(line);
    CHECK(back == tricky);
  }

  // 35. N-05: multi-input explode warns — the engine exits 0 but explodes
  //     every input except the LAST into the CWD (verified vs the bundled
  //     1.96: `gifsicle -e a.gif b.gif -o p` wrote a.gif.000..011 to the CWD
  //     and only b's frame under p). The GUI/CLI refuse warned runs; the
  //     warning text is mirrored byte-exact in web/validate.mjs.
  {
    Settings s;
    s.mode = Mode::Explode;
    s.inputs = {"a.gif", "b.gif"};
    bool found = false;
    for (const auto& w : validate(s)) {
      if (w.field == "mode" && w.value == "explode") {
        found = true;
        CHECK(w.reason.find("scatters frames") != std::string::npos);
      }
    }
    CHECK(found);
    s.inputs = {"a.gif"};  // single input: no warning
    found = false;
    for (const auto& w : validate(s))
      if (w.field == "mode") found = true;
    CHECK(!found);
  }

  std::printf("==> %d checks, %d failures\n", checks, failures);
  if (failures == 0) {
    std::printf("ALL TESTS PASSED\n");
    return 0;
  }
  std::printf("%d TEST(S) FAILED\n", failures);
  return 1;
}
