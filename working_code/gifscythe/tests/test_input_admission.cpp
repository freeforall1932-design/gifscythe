// test_input_admission.cpp - Unit tests for gs::admit_input() (GS-205 / P1-27).
//
// Qt-independent, plain g++, no display: this is what lets the ADMISSION RULE
// be pinned on Linux CI while the GUI merely CALLS it. The GUI-level half lives
// in the offscreen harness (T8: a dropped folder / renamed JPEG is refused with
// feedback); this file pins the rule itself, including the cases the harness
// cannot easily build.

#include "../src/core/InputAdmission.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
// The suite runs on every platform CMake builds for, INCLUDING MinGW (the
// windows GUI job compiles it). Only the "unreadable file" leg needs POSIX
// permissions, so the headers and the leg are guarded rather than assumed —
// a Linux-only include here is what a `<unistd.h>`-less MinGW build trips on.
#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace gs;
namespace fs = std::filesystem;

static int failures = 0;
static int checks = 0;
#define CHECK(cond) do { ++checks; if (!(cond)) { \
  std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++failures; } } while (0)

static void write_bytes(const fs::path& p, const std::string& bytes) {
  std::ofstream out(p, std::ios::binary);
  out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

int main() {
  std::printf("== input admission (GS-205 / P1-27) ==\n");
  fs::path dir = fs::temp_directory_path() / "gs_admit_test";
  fs::remove_all(dir);
  fs::create_directories(dir);

  // ---- the signature rule, on bytes only (no filesystem) ----
  CHECK(gif_signature("GIF87a", 6));
  CHECK(gif_signature("GIF89a", 6));
  CHECK(gif_signature("GIF89a\x01\x00\x02", 9));
  CHECK(!gif_signature("GIF88a", 6));      // wrong version letter
  CHECK(!gif_signature("GIF89", 5));       // short
  CHECK(!gif_signature("", 0));
  CHECK(!gif_signature("\x89PNG\r\n\x1a\n", 8));  // a PNG
  CHECK(!gif_signature("\xff\xd8\xff\xe0", 4));   // a JPEG's SOI/APP0
  CHECK(!gif_signature("gif89a", 6));      // the magic is case-SENSITIVE

  // ---- a real GIF is admitted ----
  const fs::path good = dir / "good.gif";
  write_bytes(good, std::string("GIF89a") + std::string(64, '\0'));
  {
    const AdmitResult r = admit_input(good.string());
    CHECK(r.admitted);
    CHECK(r.issue == AdmitIssue::Ok);
  }

  // ---- a GIF with an unexpected extension is STILL admitted (bytes decide) ----
  const fs::path odd = dir / "actually-a-gif.gifsicle";
  write_bytes(odd, std::string("GIF87a") + std::string(32, '\0'));
  CHECK(admit_input(odd.string()).admitted);
  CHECK(admit_input(odd.string()).admitted);
  {
    // and a .gif suffix with non-GIF bytes is REFUSED: the old GUI queued it
    // and only failed inside the engine at run time (GS-205's complaint).
    const fs::path liar = dir / "liar.gif";
    write_bytes(liar, "this is not a GIF file at all\n");
    const AdmitResult r = admit_input(liar.string());
    CHECK(!r.admitted);
    CHECK(r.issue == AdmitIssue::NotGif);
  }

  // ---- a DIRECTORY is refused, not queued ----
  // The old drop filter was `endsWith(".gif") && QFileInfo::exists(f)`, and
  // exists() is true for directories, so a dropped folder named "x.gif" got
  // queued. is_regular_file() is the fix.
  const fs::path folder = dir / "folder.gif";
  fs::create_directories(folder);
  {
    const AdmitResult r = admit_input(folder.string());
    CHECK(!r.admitted);
    CHECK(r.issue == AdmitIssue::NotRegular);
  }

  // ---- a missing path is 'missing', not 'not-gif' ----
  {
    const AdmitResult r = admit_input((dir / "nope.gif").string());
    CHECK(!r.admitted);
    CHECK(r.issue == AdmitIssue::Missing);
  }

  // ---- an empty file is refused (it has no signature) ----
  const fs::path empty = dir / "empty.gif";
  write_bytes(empty, "");
  {
    const AdmitResult r = admit_input(empty.string());
    CHECK(!r.admitted);
    CHECK(r.issue == AdmitIssue::NotGif);
  }

  // ---- an unreadable file is 'unreadable', not 'not-gif' ----
  // POSIX-only: MinGW has no mode bits to withdraw, and root ignores them, so
  // the leg is skipped on Windows and when the open would succeed anyway. The
  // branch still exists for real users on POSIX.
#ifndef _WIN32
  if (::geteuid() != 0) {
    const fs::path locked = dir / "locked.gif";
    write_bytes(locked, std::string("GIF89a") + std::string(16, '\0'));
    ::chmod(locked.c_str(), 0000);
    const AdmitResult r = admit_input(locked.string());
    CHECK(!r.admitted);
    CHECK(r.issue == AdmitIssue::Unreadable);
    ::chmod(locked.c_str(), 0644);
  } else {
    std::printf("  (running as root: unreadable-mode case skipped)\n");
  }
#else
  std::printf("  (windows: unreadable-mode case skipped - no POSIX mode bits)\n");
#endif

  // ---- batch form separates accepted from refused and keeps the order ----
  {
    const AdmissionOutcome out =
        admit_inputs({good.string(), folder.string(), (dir / "nope.gif").string(), odd.string()});
    CHECK(out.accepted.size() == 2);
    CHECK(out.accepted[0] == good.string());
    CHECK(out.accepted[1] == odd.string());
    CHECK(out.rejected.size() == 2);
    CHECK(out.rejected[0].issue == AdmitIssue::NotRegular);
    CHECK(out.rejected[1].issue == AdmitIssue::Missing);
    // The reason strings are the GUI's vocabulary; pin them so a status line
    // reading "not-a-file" cannot silently become "unknown".
    CHECK(std::string(admit_issue_name(AdmitIssue::NotRegular)) == "not-a-file");
    CHECK(std::string(admit_issue_name(AdmitIssue::Missing)) == "missing");
    CHECK(std::string(admit_issue_name(AdmitIssue::NotGif)) == "not-gif");
    CHECK(std::string(admit_issue_name(AdmitIssue::Unreadable)) == "unreadable");
    CHECK(std::string(admit_issue_name(AdmitIssue::Ok)) == "ok");
  }

  // ---- a non-ASCII filename is admitted through the UTF-8 boundary ----
  {
    const fs::path unicode = dir / fs::u8path("ünïcode-日本語-Ω.gif");
    write_bytes(unicode, std::string("GIF89a") + std::string(16, '\0'));
    CHECK(admit_input(unicode.string()).admitted);
  }

  fs::remove_all(dir);
  std::printf("==> %d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
