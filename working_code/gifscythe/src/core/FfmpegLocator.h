// FfmpegLocator.h - find the FFmpeg that belongs to THIS build, then fall back.
//
// Owner, 2026-10-26: "add ffmpeg directly into the project ... just make it
// portable but add fallback if ffmpeg gone it will search for ffmpeg program on
// computer program list or something that usually ffmpeg located but only called
// when ffmpeg local into the folder not found".
//
// So the order is fixed and non-negotiable:
//
//   1. OVERRIDE   GS_FFMPEG (an explicit path; the escape hatch for tests and
//                 for a user who keeps their own build somewhere odd)
//   2. SIDECAR    the copy that travels with the app - that is the "portable"
//                 part, and it wins over anything installed on the machine so a
//                 package is reproducible wherever it is unzipped:
//                   <exe dir>/ffmpeg[.exe]
//                   <exe dir>/ffmpeg/bin/ffmpeg[.exe]
//                   <exe dir>/bin/ffmpeg[.exe]
//                   <exe dir>/tools/ffmpeg/bin/ffmpeg[.exe]
//   3. SYSTEM     a real ffmpeg already on the machine: every PATH entry, then
//                 the usual install locations for the platform.
//
// Nothing here executes FFmpeg. It answers one question - "which file would we
// run?" - and says WHY it chose it, because a silent pick is how a user ends up
// with a 4-year-old build from a random PATH entry doing their encodes.
//
// Qt-free on purpose: the rule is unit-tested on every platform (and in the
// agent sandbox, which has neither Qt nor ffmpeg).

#ifndef GIFSCYTHE_CORE_FFMPEG_LOCATOR_H
#define GIFSCYTHE_CORE_FFMPEG_LOCATOR_H

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace gs {

namespace fs = std::filesystem;

enum class FfmpegSource {
  None = 0,   // nothing found: the formats stay listed-but-disabled, with a reason
  Override,   // GS_FFMPEG
  Sidecar,    // ships with the app
  Path,       // found by walking PATH
  WellKnown,  // found in a standard install location
};

inline const char* ffmpeg_source_name(FfmpegSource s) {
  switch (s) {
    case FfmpegSource::Override: return "GS_FFMPEG";
    case FfmpegSource::Sidecar: return "bundled";
    case FfmpegSource::Path: return "PATH";
    case FfmpegSource::WellKnown: return "installed";
    default: return "not found";
  }
}

struct FfmpegLocation {
  std::string path;                        // empty when source == None
  FfmpegSource source = FfmpegSource::None;
  std::vector<std::string> searched;       // what we looked at, in order
  bool found() const { return source != FfmpegSource::None && !path.empty(); }
};

inline std::string ffmpeg_exe_name() {
#ifdef _WIN32
  return "ffmpeg.exe";
#else
  return "ffmpeg";
#endif
}

inline bool ffmpeg_is_usable_file(const fs::path& p) {
  std::error_code ec;
  if (!fs::is_regular_file(p, ec)) return false;
  if (fs::file_size(p, ec) == 0 || ec) return false;   // a 0-byte placeholder is not a program
#ifndef _WIN32
  const auto perms = fs::status(p, ec).permissions();
  if (ec) return false;
  const bool any_exec = (perms & (fs::perms::owner_exec | fs::perms::group_exec |
                                  fs::perms::others_exec)) != fs::perms::none;
  if (!any_exec) return false;
#endif
  return true;
}

// PATH split, with the platform's separator. Empty entries are skipped: an
// empty PATH element means "current directory" on POSIX, and silently running
// whatever ffmpeg happens to sit in the CWD is how a build gets hijacked.
inline std::vector<std::string> ffmpeg_split_path(const std::string& value) {
#ifdef _WIN32
  const char sep = ';';
#else
  const char sep = ':';
#endif
  std::vector<std::string> out;
  std::string cur;
  for (char c : value) {
    if (c == sep) {
      if (!cur.empty()) out.push_back(cur);
      cur.clear();
    } else {
      cur += c;
    }
  }
  if (!cur.empty()) out.push_back(cur);
  return out;
}

// The standard places a package manager or an installer would have put it.
inline std::vector<std::string> ffmpeg_well_known_dirs() {
  std::vector<std::string> dirs;
#ifdef _WIN32
  if (const char* pf = std::getenv("ProgramFiles")) {
    dirs.push_back(std::string(pf) + "\\ffmpeg\\bin");
  }
  if (const char* pf86 = std::getenv("ProgramFiles(x86)")) {
    dirs.push_back(std::string(pf86) + "\\ffmpeg\\bin");
  }
  if (const char* lad = std::getenv("LOCALAPPDATA")) {
    dirs.push_back(std::string(lad) + "\\ffmpeg\\bin");
    // winget / scoop / chocolatey land here on many machines
    dirs.push_back(std::string(lad) + "\\Microsoft\\WinGet\\Links");
  }
  if (const char* up = std::getenv("USERPROFILE")) {
    dirs.push_back(std::string(up) + "\\scoop\\shims");
  }
  dirs.push_back("C:\\ffmpeg\\bin");
  dirs.push_back("C:\\ProgramData\\chocolatey\\bin");
#elif defined(__APPLE__)
  dirs.push_back("/opt/homebrew/bin");   // Apple silicon Homebrew
  dirs.push_back("/usr/local/bin");      // Intel Homebrew / manual installs
  dirs.push_back("/opt/local/bin");      // MacPorts
  dirs.push_back("/usr/bin");
#else
  dirs.push_back("/usr/bin");
  dirs.push_back("/usr/local/bin");
  dirs.push_back("/bin");
  dirs.push_back("/snap/bin");           // snap
  dirs.push_back("/var/lib/flatpak/exports/bin");
  if (const char* home = std::getenv("HOME")) {
    dirs.push_back(std::string(home) + "/.local/bin");
    dirs.push_back(std::string(home) + "/.local/state/nix/profiles/profile/bin");
  }
  dirs.push_back("/nix/var/nix/profiles/default/bin");
#endif
  return dirs;
}

// Candidate sidecar paths relative to the executable's own folder.
inline std::vector<std::string> ffmpeg_sidecar_relative_paths() {
  const std::string exe = ffmpeg_exe_name();
#ifdef _WIN32
  const char sep = '\\';
#else
  const char sep = '/';
#endif
  const std::string s(1, sep);
  return {exe, "ffmpeg" + s + "bin" + s + exe, "bin" + s + exe,
          "tools" + s + "ffmpeg" + s + "bin" + s + exe};
}

// The whole search. `app_dir` is the folder holding the running executable;
// `env_path` and `override` are parameters (not read from the environment here)
// so the precedence is unit-testable instead of being a guess about the host.
inline FfmpegLocation locate_ffmpeg(const std::string& app_dir,
                                    const std::string& env_path,
                                    const std::string& override_path,
                                    const std::vector<std::string>& well_known_dirs) {
  FfmpegLocation out;

  const auto try_file = [&out](const fs::path& p, FfmpegSource src) {
    out.searched.push_back(p.string());
    if (ffmpeg_is_usable_file(p)) {
      out.path = p.string();
      out.source = src;
      return true;
    }
    return false;
  };

  // 1. explicit override
  if (!override_path.empty()) {
    if (try_file(fs::path(override_path), FfmpegSource::Override)) return out;
    // An override that does not work is NOT quietly replaced by something else:
    // the caller shows "GS_FFMPEG points at <path>, which is not a usable
    // ffmpeg" so a typo cannot turn into a silent downgrade.
    out.path = override_path;
    out.source = FfmpegSource::None;
    return out;
  }

  // 2. the sidecar next to the executable
  //
  // An empty app_dir means "here": skipping the step entirely (the first version
  // did) made the MOST important case - the copy shipped in the folder - vanish
  // for any caller that did not have an absolute path to hand, and the test below
  // then passed for the wrong reason.
  {
    const fs::path root = app_dir.empty() ? fs::path(".") : fs::path(app_dir);
    for (const std::string& rel : ffmpeg_sidecar_relative_paths()) {
      if (try_file(root / fs::path(rel), FfmpegSource::Sidecar)) return out;
    }
  }

  // 3. PATH
  for (const std::string& dir : ffmpeg_split_path(env_path)) {
    if (try_file(fs::path(dir) / fs::path(ffmpeg_exe_name()), FfmpegSource::Path)) return out;
  }

  // 4. the usual install locations
  for (const std::string& dir : well_known_dirs) {
    if (try_file(fs::path(dir) / fs::path(ffmpeg_exe_name()), FfmpegSource::WellKnown)) return out;
  }

  out.source = FfmpegSource::None;
  out.path.clear();
  return out;
}

// Convenience overload for callers that want the process environment.
inline FfmpegLocation locate_ffmpeg(const std::string& app_dir) {
  const char* p = std::getenv("PATH");
  const char* o = std::getenv("GS_FFMPEG");
  return locate_ffmpeg(app_dir, p ? p : "", o ? o : "", ffmpeg_well_known_dirs());
}

// One line a tooltip or a log can print, stating the choice AND its reason.
inline std::string ffmpeg_location_summary(const FfmpegLocation& loc) {
  if (loc.found()) {
    return std::string(ffmpeg_source_name(loc.source)) + ": " + loc.path;
  }
  if (!loc.path.empty()) {
    return "GS_FFMPEG points at " + loc.path + ", which is not a usable ffmpeg";
  }
  return "no ffmpeg found (looked next to the program, on PATH, and in the usual install folders)";
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_FFMPEG_LOCATOR_H
