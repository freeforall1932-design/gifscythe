// test_name_template.cpp - Unit tests for gs::render_name_template() and the
// duplicate-numbering rule (slice E2).
//
// Qt-free, plain g++, no display - the same shape as test_gif_facts.cpp. The
// Output tab's Filename field is now a token template (XnConvert's model), and
// the three claims worth pinning are the ones a screenshot cannot prove:
//
//   * the DEFAULT names a file "{source}_result" and the extension comes from
//     the Format, never from the template;
//   * every token renders the thing it promises, and an unknown token is copied
//     as typed WITH a warning instead of silently vanishing;
//   * duplicates are numbered _1, _2 with the FIRST file keeping the plain name
//     (owner's rule, 2026-10-26) - including the case-only collision that would
//     silently overwrite a file on Windows/macOS.

#include "../src/core/NameTemplate.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace gs;

static int failures = 0;
static int checks = 0;
#define CHECK(cond) do { ++checks; if (!(cond)) { \
  std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++failures; } } while (0)
#define CHECK_EQ(a, b) do { ++checks; const std::string _a = (a); const std::string _b = (b); \
  if (_a != _b) { std::printf("FAIL: %s == %s (line %d)\n  got: \"%s\"\n  want:\"%s\"\n", \
  #a, #b, __LINE__, _a.c_str(), _b.c_str()); ++failures; } } while (0)

namespace {

NameContext sample() {
  NameContext c;
  c.filename = "clip";
  c.extension = "gif";
  c.directory = "/clips/today";
  c.folder_name = "today";
  c.format = "GIF";
  c.index = 0;
  c.start_index = 1;
  c.width = 480;
  c.height = 270;
  c.dst_width = 320;
  c.dst_height = 180;
  c.frames = 12;
  c.duration_seconds = 2.4;
  c.now.tm_year = 126;  // 2026
  c.now.tm_mon = 9;     // October (0-based)
  c.now.tm_mday = 26;
  c.now.tm_hour = 1;
  c.now.tm_min = 30;
  c.now.tm_sec = 0;
  return c;
}

bool has_warning(const NameRender& r, const char* needle) {
  for (const std::string& w : r.warnings) {
    if (w.find(needle) != std::string::npos) return true;
  }
  return false;
}

}  // namespace

int main() {
  // ---- the default template is the reference's: name + _result ----
  {
    NameRender r = render_name_template("", sample());
    CHECK_EQ(r.text, "clip_result");
    CHECK(r.warnings.empty());
    CHECK_EQ(std::string(kDefaultNameTemplate), "{Filename}_result");
  }

  // ---- every token renders what it promises ----
  {
    NameContext c = sample();
    CHECK_EQ(render_name_template("{Filename}", c).text, "clip");
    CHECK_EQ(render_name_template("{Filename With Ext}", c).text, "clip.gif");
    CHECK_EQ(render_name_template("{File Type}", c).text, "GIF");
    CHECK_EQ(render_name_template("{Format}", c).text, "GIF");
    CHECK_EQ(render_name_template("{Width}x{Height}", c).text, "480x270");
    CHECK_EQ(render_name_template("{DstWidth}x{DstHeight}", c).text, "320x180");
    CHECK_EQ(render_name_template("{Directory}", c).text, "/clips/today");
    CHECK_EQ(render_name_template("{Folder name}", c).text, "today");
    CHECK_EQ(render_name_template("{Frames}f", c).text, "12f");
    CHECK_EQ(render_name_template("{Duration}s", c).text, "2.40s");
  }

  // ---- {File Index} is offset by Start index, and 1-based by default ----
  {
    NameContext c = sample();
    CHECK_EQ(render_name_template("{File Index}", c).text, "1");
    c.index = 4;
    CHECK_EQ(render_name_template("{File Index}", c).text, "5");
    c.start_index = 100;
    CHECK_EQ(render_name_template("{File Index}", c).text, "104");
    CHECK(template_uses_index("{Filename}_{File Index}"));
    CHECK(!template_uses_index("{Filename}_result"));
  }

  // ---- {name} still works: saved presets must not break ----
  {
    CHECK_EQ(render_name_template("{name}_opt", sample()).text, "clip_opt");
    CHECK(!template_is_constant("{name}_opt"));
  }

  // ---- the Format owns the extension ----
  {
    // A template that still carries one loses it, and says so once.
    NameRender r = render_name_template("{Filename}_opt.gif", sample());
    CHECK_EQ(r.text, "clip_opt");
    CHECK(has_warning(r, "extension is ignored"));
    // ...but {Filename With Ext} keeps the source extension on purpose.
    r = render_name_template("{Filename With Ext}", sample());
    CHECK_EQ(r.text, "clip.gif");
    CHECK(r.warnings.empty());
    // A dot that is not a media extension is just text.
    CHECK_EQ(render_name_template("{Filename}_v1.2", sample()).text, "clip_v1.2");
    CHECK(render_name_template("{Filename}_v1.2", sample()).warnings.empty());
  }

  // ---- unknown tokens are visible, never silent ----
  {
    NameRender r = render_name_template("{Filename}_{Nope}", sample());
    CHECK_EQ(r.text, "clip_{Nope}");
    CHECK(has_warning(r, "Unknown token"));
    // An unclosed brace is copied, not dropped.
    r = render_name_template("{Filename}_{Width", sample());
    CHECK_EQ(r.text, "clip_{Width");
    CHECK(has_warning(r, "unclosed"));
  }

  // ---- Qt's date letters are NOT strftime's ----
  // The reference's own example is {Current Date [m_d_Y]}: m is the MONTH here.
  {
    NameContext c = sample();
    CHECK_EQ(render_name_template("{Current Date [Y-m-d]}", c).text, "2026-10-26");
    CHECK_EQ(render_name_template("{Current Date [m_d_Y]}", c).text,
             "10_26_2026");  // month, day, year - not minute/day/year
    CHECK_EQ(render_name_template("{Current Date [H-M-S]}", c).text, "01-30-00");
    CHECK_EQ(render_name_template("{Current Date [d.m.Y_H-M]}", c).text,
             "26.10.2026_01-30");
    // A bare {Current Date} still renders something useful.
    CHECK_EQ(render_name_template("{Current Date}", c).text, "2026-10-26_01-30-00");
    // Unknown letters stay literal instead of producing strftime noise.
    CHECK_EQ(render_name_template("{Current Date [Y!]}", c).text, "2026!");
  }

  // ---- unknown facts are honest empties, not invented numbers ----
  {
    NameContext c = sample();
    c.frames = -1;
    c.duration_seconds = -1.0;
    CHECK_EQ(render_name_template("a{Frames}b{Duration}c", c).text, "abc");
    // A rendered-empty name falls back to the source name, with a warning.
    NameRender r = render_name_template("{Frames}", c);
    CHECK_EQ(r.text, "clip");
    CHECK(has_warning(r, "empty name"));
  }

  // ---- constant templates are detectable (the GUI warns with the reference's
  //      sentence) but are NOT refused any more: numbering handles them ----
  {
    CHECK(template_is_constant("{Current Date [Y-m-d]}"));
    CHECK(template_is_constant("static_name"));
    CHECK(template_is_constant("{Format}"));
    CHECK(!template_is_constant("{Filename}"));
    CHECK(!template_is_constant("{File Index}"));
    CHECK(!template_is_constant("{Width}-{Height}"));
  }

  // ---- the duplicate rule: first keeps the plain name ----
  {
    const std::vector<std::string> out = disambiguate_names(
        {"go_result.webp", "go_result.webp", "go_result.webp", "other.webp"});
    CHECK_EQ(out[0], "go_result.webp");
    CHECK_EQ(out[1], "go_result_1.webp");
    CHECK_EQ(out[2], "go_result_2.webp");
    CHECK_EQ(out[3], "other.webp");
  }
  {
    // Case-only collisions are collisions: Windows and macOS would lose one.
    const std::vector<std::string> out =
        disambiguate_names({"Go_Result.webp", "go_result.webp"});
    CHECK_EQ(out[0], "Go_Result.webp");
    CHECK_EQ(out[1], "go_result_1.webp");
  }
  {
    // A number that is already taken is skipped, not reused.
    const std::vector<std::string> out = disambiguate_names(
        {"a.webp", "a.webp", "a_1.webp"});
    CHECK_EQ(out[0], "a.webp");
    CHECK_EQ(out[1], "a_2.webp");
    CHECK_EQ(out[2], "a_1.webp");
  }
  {
    // No extension: the number still lands at a sane place.
    const std::vector<std::string> out = disambiguate_names({"name", "name"});
    CHECK_EQ(out[0], "name");
    CHECK_EQ(out[1], "name_1");
    // A dot in the FOLDER is not an extension.
    const std::vector<std::string> dirs =
        disambiguate_names({"out.d/x", "out.d/x"});
    CHECK_EQ(dirs[0], "out.d/x");
    CHECK_EQ(dirs[1], "out.d/x_1");
    // Nothing to do when every name differs.
    CHECK(disambiguate_names({"a.gif", "b.gif"}) ==
          std::vector<std::string>({"a.gif", "b.gif"}));
  }

  // ---- the whole Output-tab story in one template ----
  {
    NameContext c = sample();
    c.index = 2;
    CHECK_EQ(render_name_template("{Filename}_{DstWidth}x{DstHeight}_{File Index}",
                                  c).text,
             "clip_320x180_3");
  }

  std::printf("==> %d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
