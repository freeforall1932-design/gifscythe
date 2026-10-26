// test_ffmpeg_locator.cpp - Unit tests for gs::locate_ffmpeg().
//
// Qt-free, plain g++, no display. The claims worth pinning are the ones the
// owner asked for in one sentence ("make it portable ... but add fallback ...
// only called when ffmpeg local into the folder not found"), plus the two ways
// a naive locator goes wrong:
//
//   * the BUNDLED copy must win over anything installed on the machine, or a
//     package is not reproducible;
//   * a broken GS_FFMPEG must NOT fall through to some random PATH ffmpeg - the
//     user said where it is, so a typo has to be visible, not silently repaired.
//
// Fixtures are real temporary files; on POSIX they must also be executable, so
// the permission rule is exercised rather than assumed.

#include "../src/core/FfmpegLocator.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace gs;
namespace fs = std::filesystem;

static int failures = 0;
static int checks = 0;
#define CHECK(cond) do { ++checks; if (!(cond)) { \
  std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++failures; } } while (0)
#define CHECK_MSG(cond, msg) do { ++checks; if (!(cond)) { \
  std::printf("FAIL: %s (line %d)\n  %s\n", #cond, __LINE__, msg); ++failures; } } while (0)

namespace {

fs::path make_exe(const fs::path& dir, const std::string& rel, const std::string& body) {
  const fs::path p = dir / fs::path(rel);
  fs::create_directories(p.parent_path());
  std::ofstream(p, std::ios::binary) << body;
#ifndef _WIN32
  fs::permissions(p, fs::perms::owner_exec | fs::perms::owner_read |
                        fs::perms::owner_write, fs::perm_options::add);
#endif
  return p;
}

bool same(const std::string& a, const fs::path& b) {
  std::error_code ec;
  return fs::equivalent(fs::path(a), b, ec) && !ec;
}

}  // namespace

int main() {
  const fs::path root = fs::temp_directory_path() / "gs_ffmpeg_locator_test";
  std::error_code ec;
  fs::remove_all(root, ec);
  fs::create_directories(root);

  const fs::path app = root / "app";
  const fs::path sysA = root / "sysA";
  const fs::path sysB = root / "sysB";
  fs::create_directories(app);
  fs::create_directories(sysA);
  fs::create_directories(sysB);

  const std::string exe = ffmpeg_exe_name();
#ifdef _WIN32
  const std::string pathSep = ";";
  const std::string dirSep = "\\";
#else
  const std::string pathSep = ":";
  const std::string dirSep = "/";
#endif

  // ---- the sidecar wins over PATH, which is the whole point of "portable" ----
  {
    const fs::path sidecar = make_exe(app, exe, "bundled ffmpeg");
    const fs::path onPath = make_exe(sysA, exe, "system ffmpeg");
    const FfmpegLocation loc =
        locate_ffmpeg(app.string(), sysA.string(), "", {sysB.string()});
    CHECK_MSG(loc.found(), ffmpeg_location_summary(loc).c_str());
    CHECK(loc.source == FfmpegSource::Sidecar);
    CHECK(same(loc.path, sidecar));
    CHECK_MSG(!same(loc.path, onPath), "the installed copy must not win");
  }

  // ---- the nested layout <app>/ffmpeg/bin/ffmpeg also counts as a sidecar ----
  {
    fs::remove(app / exe, ec);
    const fs::path nested = make_exe(app, std::string("ffmpeg") + dirSep + "bin" + dirSep + exe,
                                     "nested bundled ffmpeg");
    const FfmpegLocation loc = locate_ffmpeg(app.string(), sysA.string(), "", {});
    CHECK(loc.source == FfmpegSource::Sidecar);
    CHECK(same(loc.path, nested));
    fs::remove_all(app / "ffmpeg", ec);
  }

  // ---- no sidecar: the first usable PATH entry is used, and it is named ----
  {
    const fs::path onPath = make_exe(sysA, exe, "system ffmpeg");
    const FfmpegLocation loc =
        locate_ffmpeg(app.string(), sysA.string() + pathSep + sysB.string(), "", {});
    CHECK(loc.found());
    CHECK(loc.source == FfmpegSource::Path);
    CHECK(same(loc.path, onPath));
    CHECK_MSG(std::string(ffmpeg_source_name(loc.source)) == "PATH", "the source is reported");
  }

  // ---- PATH entries that hold nothing are skipped, not treated as found ----
  // (sysA deliberately still has its ffmpeg from the block above, so this uses
  // an EMPTY directory first: that is the "holds nothing" case being tested.)
  {
    const fs::path emptyDir = root / "emptyDir";
    fs::create_directories(emptyDir);
    const fs::path inB = make_exe(sysB, exe, "second ffmpeg");
    const FfmpegLocation loc = locate_ffmpeg(app.string(),
                                             emptyDir.string() + pathSep + "/nonexistent" +
                                                 pathSep + sysB.string(),
                                             "", {});
    CHECK(loc.found());
    CHECK(same(loc.path, inB));
  }

  // ---- an empty PATH element means "the current directory": refused ----
  {
    const FfmpegLocation loc = locate_ffmpeg(app.string(), pathSep + pathSep, "", {});
    CHECK(!loc.found());
    CHECK_MSG(loc.source == FfmpegSource::None, "an empty PATH element must not run the CWD");
  }

  // ---- last resort: the well-known install folders ----
  {
    const fs::path installed = make_exe(sysB, exe, "installed ffmpeg");
    const FfmpegLocation loc = locate_ffmpeg(app.string(), "", "", {sysB.string()});
    CHECK(loc.found());
    CHECK(loc.source == FfmpegSource::WellKnown);
    CHECK(same(loc.path, installed));
  }

  // ---- a broken GS_FFMPEG is reported, never silently replaced ----
  {
    const FfmpegLocation loc =
        locate_ffmpeg(app.string(), sysA.string(), (root / "typo" / exe).string(),
                      {sysB.string()});
    CHECK_MSG(!loc.found(), "an override that does not exist must not fall through");
    CHECK(loc.source == FfmpegSource::None);
    CHECK_MSG(ffmpeg_location_summary(loc).find("GS_FFMPEG") != std::string::npos,
              "the summary must name the override and say it failed");
  }

  // ---- a WORKING override wins over the sidecar (tests and power users) ----
  {
    const fs::path sidecar = make_exe(app, exe, "bundled ffmpeg");
    const fs::path chosen = make_exe(sysA, std::string("custom-") + exe, "override ffmpeg");
    const FfmpegLocation loc = locate_ffmpeg(app.string(), "", chosen.string(), {});
    CHECK(loc.source == FfmpegSource::Override);
    CHECK(same(loc.path, chosen));
    CHECK(!same(loc.path, sidecar));
  }

  // ---- a 0-byte placeholder is not a program ----
  {
    const fs::path empty = app / exe;
    { std::ofstream(empty, std::ios::binary).flush(); }
    const FfmpegLocation loc = locate_ffmpeg(app.string(), "", "", {});
    CHECK_MSG(!loc.found(), "an empty file next to the app is not an ffmpeg");
    fs::remove(empty, ec);
  }

#ifndef _WIN32
  // ---- a non-executable file is not a program either (POSIX) ----
  {
    const fs::path notExec = app / exe;
    std::ofstream(notExec, std::ios::binary) << "not executable";
    const FfmpegLocation loc = locate_ffmpeg(app.string(), "", "", {});
    CHECK_MSG(!loc.found(), "mode 0644 is not an ffmpeg binary");
    CHECK_MSG(ffmpeg_location_summary(loc).find("no ffmpeg found") != std::string::npos,
              "the empty result says so in words");
    fs::remove(notExec, ec);
  }
#endif

  // ---- PATH splitting honours the platform separator and skips empties ----
  {
    const std::vector<std::string> parts =
        ffmpeg_split_path(std::string("a") + pathSep + pathSep + "b");
    CHECK(parts.size() == 2);
    CHECK(parts[0] == "a");
    CHECK(parts[1] == "b");
    CHECK(ffmpeg_split_path("").empty());
  }

  // ---- the search list is kept, so a failure can be explained ----
  {
    const FfmpegLocation loc = locate_ffmpeg(app.string(), sysA.string(), "", {sysB.string()});
    CHECK_MSG(!loc.searched.empty(), "the locator records what it looked at");
  }

  fs::remove_all(root, ec);
  std::printf("==> %d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
