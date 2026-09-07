// EngineLocator.h - Find the bundled gifsicle engine regardless of CWD.
// Search order:
//   1. GS_ENGINE environment variable (exact path)
//   2. Beside the running executable (packaged layout)
//   3. ../release/<GS_VERSION>/gifsicle[.exe] relative to the executable (dev)
//   4. release/<GS_VERSION>/gifsicle[.exe] relative to CWD (dev from product dir)
//   5. PATH (bare "gifsicle")
//
// Qt-independent. Uses std::filesystem (C++17).

#ifndef GIFSCYTHE_CORE_ENGINE_LOCATOR_H
#define GIFSCYTHE_CORE_ENGINE_LOCATOR_H

#include "version.h"
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace gs {
namespace fs = std::filesystem;

inline bool path_is_executable(const fs::path& p) {
  std::error_code ec;
  if (!fs::is_regular_file(p, ec)) return false;
#ifndef _WIN32
  auto perms = fs::status(p, ec).permissions();
  if (ec) return false;
  using std::filesystem::perms;
  return (perms & (perms::owner_exec | perms::group_exec | perms::others_exec)) != perms::none;
#else
  return true;  // existence is enough on Windows
#endif
}

inline std::string engine_basename() {
#ifdef _WIN32
  return "gifsicle.exe";
#else
  return "gifsicle";
#endif
}

// exe_path: argv[0] or QCoreApplication::applicationFilePath().toStdString()
// Returns absolute path to the engine, or empty string if not found.
inline std::string locate_engine(const std::string& exe_path = {}) {
  std::vector<fs::path> candidates;

  if (const char* env = std::getenv("GS_ENGINE")) {
    if (env[0] != '\0') candidates.emplace_back(env);
  }

  fs::path exe_dir;
  if (!exe_path.empty()) {
    std::error_code ec;
    fs::path ep = fs::absolute(exe_path, ec);
    if (!ec) exe_dir = ep.parent_path();
  }
  if (exe_dir.empty()) {
    std::error_code ec;
    exe_dir = fs::current_path(ec);
  }

  const std::string base = engine_basename();
  // Packaged: engine next to the binary.
  candidates.push_back(exe_dir / base);
  // Dev layout: release/<ver>/ next to build/ or build/gui/.
  candidates.push_back(exe_dir / ".." / "release" / GS_VERSION / base);
  candidates.push_back(exe_dir / ".." / ".." / "release" / GS_VERSION / base);
  candidates.push_back(exe_dir / "release" / GS_VERSION / base);
  // CWD-relative (running from working_code/gifscythe).
  candidates.push_back(fs::path("release") / GS_VERSION / base);
  // Repo-root relative (legacy).
  candidates.push_back(fs::path("working_code") / "gifscythe" / "release" / GS_VERSION / base);

  for (const auto& c : candidates) {
    std::error_code ec;
    fs::path abs = fs::weakly_canonical(c, ec);
    if (ec) abs = fs::absolute(c, ec);
    if (ec) continue;
    if (path_is_executable(abs)) return abs.string();
  }

  // Fall back to bare name on PATH — caller may still fail at exec time.
  return base;
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_ENGINE_LOCATOR_H
