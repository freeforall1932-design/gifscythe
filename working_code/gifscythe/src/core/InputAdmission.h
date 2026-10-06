// InputAdmission.h - ONE admission rule for GIF input, shared by every GUI
// entry point (audit GS-205 / fix-order P1-27, S37).
//
// WHY: the desktop admitted input in three different ways that disagreed:
//   * the picker offered "All files (*)" and appendInputs() validated nothing;
//   * the drop path checked `endsWith(".gif") && QFileInfo::exists(f)` — and
//     `exists()` is TRUE for a directory, so dropping a FOLDER queued the
//     folder (audit GS-205's exact words: "checks existence, not isFile()");
//   * nothing in the GUI ever looked at the bytes, so a .jpg renamed to .gif
//     was queued and only failed at run time with the engine's error.
// The core already knew how to check the GIF signature (OutputVerify.h for
// outputs, ExplodeVerify.h for frames); the GUI just never asked.
//
// The rule, decided once here:
//   1. the path exists and is a regular file (a directory is NOT input),
//   2. it is readable,
//   3. it starts with "GIF87a" or "GIF89a" — the same signature check the
//      output side uses. The EXTENSION is deliberately not consulted: a GIF
//      named `.gifsicle` is a GIF, and a `.gif` that is a JPEG is not.
//
// Qt-free and header-only on purpose: the GUI calls it, and the Linux unit
// tests (tests/test_input_admission.cpp) exercise the same rule, including the
// directory and wrong-bytes cases, without a display.
//
// Verbs: every rejection carries a machine-readable reason so the GUI can
// tell the user WHICH file was refused and WHY, instead of silently dropping
// it (a silent drop is how "the app ignored my file" bug reports are born).

#ifndef GIFSCYTHE_CORE_INPUTADMISSION_H
#define GIFSCYTHE_CORE_INPUTADMISSION_H

#include "WinUnicode.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace gs {

namespace fs = std::filesystem;

enum class AdmitIssue {
  Ok,
  Missing,     // does not exist
  NotRegular,  // a directory, socket, device, ...
  Unreadable,  // exists but cannot be opened for reading
  NotGif       // readable, but the first 6 bytes are not GIF87a/GIF89a
};

inline const char* admit_issue_name(AdmitIssue i) {
  switch (i) {
    case AdmitIssue::Ok:         return "ok";
    case AdmitIssue::Missing:    return "missing";
    case AdmitIssue::NotRegular: return "not-a-file";
    case AdmitIssue::Unreadable: return "unreadable";
    case AdmitIssue::NotGif:     return "not-gif";
  }
  return "unknown";
}

struct AdmitResult {
  bool admitted = false;
  AdmitIssue issue = AdmitIssue::Ok;
  std::string name;  // echo of the path that was judged

  explicit operator bool() const { return admitted; }
};

// `check_bytes` exists so tests can pin the signature rule on a synthetic
// buffer without a filesystem, and so the GUI can reuse the rule for bytes it
// already holds (drag-and-drop from another app on some platforms supplies
// contents, not a path).
inline bool gif_signature(const void* data, std::size_t n) {
  if (n < 6) return false;
  const char* p = static_cast<const char*>(data);
  return std::memcmp(p, "GIF87a", 6) == 0 || std::memcmp(p, "GIF89a", 6) == 0;
}

inline AdmitResult admit_input(const std::string& path) {
  AdmitResult r;
  r.name = path;
  std::error_code ec;

  const fs::path p = u8path_compat(path);
  const auto st = fs::status(p, ec);
  if (ec || !fs::exists(st)) {
    r.issue = AdmitIssue::Missing;
    return r;
  }
  if (!fs::is_regular_file(st)) {
    r.issue = AdmitIssue::NotRegular;
    return r;
  }

  std::ifstream in(p, std::ios::binary);
  if (!in) {
    r.issue = AdmitIssue::Unreadable;
    return r;
  }
  char header[6]{};
  in.read(header, 6);
  if (in.gcount() != 6 || !gif_signature(header, 6)) {
    r.issue = AdmitIssue::NotGif;
    return r;
  }

  r.admitted = true;
  return r;
}

struct AdmissionRejection {
  std::string path;
  AdmitIssue issue = AdmitIssue::Ok;
  std::string reason() const {
    return std::string(admit_issue_name(issue));
  }
};

struct AdmissionOutcome {
  std::vector<std::string> accepted;
  std::vector<AdmissionRejection> rejected;
};

// Batch form: one call per queued list, so the GUI's status line can say
// "3 added, 1 refused (not-a-file)" without a second pass.
inline AdmissionOutcome admit_inputs(const std::vector<std::string>& paths) {
  AdmissionOutcome out;
  for (const auto& p : paths) {
    const AdmitResult r = admit_input(p);
    if (r.admitted)
      out.accepted.push_back(p);
    else
      out.rejected.push_back({p, r.issue});
  }
  return out;
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_INPUTADMISSION_H
