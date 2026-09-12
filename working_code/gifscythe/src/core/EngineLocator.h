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

#include "core/version.h"  // path form, not "version.h": must resolve through the
                            // include path so CMake builds get the generated
                            // copy (audit U-15); src/core fallback for -Isrc builds
#include "WinUnicode.h"        // U-07: wide env shim on Windows; pure logic elsewhere
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace gs {
namespace fs = std::filesystem;

// Environment reads go through this so a non-ASCII GS_ENGINE or PATH entry
// survives on Windows (std::getenv hands back ANSI-code-page bytes there —
// audit U-07). POSIX: plain getenv.
inline std::string env_utf8(const char* name) {
#ifdef _WIN32
  return win_getenv_utf8(name);
#else
  const char* v = std::getenv(name);
  return v ? std::string(v) : std::string();
#endif
}

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

// Search $PATH / %PATH% the way a shell would: split on the platform separator,
// join each entry with `name`, return the first executable absolute match.
// Returns "" when nothing on PATH matches.
//
// Audit U-05: this used to be documented as search-order step 5 but was dead
// code — locate_engine() returned the bare name "gifsicle" and every caller
// then ran path_is_executable() on it, which stats against the CWD and never
// consults PATH. Verified: an isolated CLI with gifsicle genuinely on PATH
// (`command -v gifsicle` -> a real file) printed
// "ERROR: engine not found at gifsicle" and exited 1.
inline std::string find_on_path(const std::string& name) {
  const std::string path_env = env_utf8("PATH");
  const char* env = path_env.c_str();
  if (!env || !*env) return std::string();
#ifdef _WIN32
  const char sep = ';';
#else
  const char sep = ':';
#endif
  const std::string path(env);
  size_t start = 0;
  while (start <= path.size()) {
    size_t end = path.find(sep, start);
    if (end == std::string::npos) end = path.size();
    std::string dir = path.substr(start, end - start);
    start = end + 1;
    if (dir.empty()) dir = ".";  // an empty PATH entry means the CWD
    std::error_code ec;
    fs::path cand = u8path_compat(dir) / u8path_compat(name);
    fs::path abs = fs::weakly_canonical(cand, ec);
    if (ec) abs = fs::absolute(cand, ec);
    if (!ec && path_is_executable(abs)) return path_u8string(abs);
  }
  return std::string();
}

// exe_path: argv[0] or QCoreApplication::applicationFilePath().toStdString()
// Returns an absolute path to the engine, or an EMPTY STRING if it is not
// found anywhere (callers must treat "" as "not found" — it used to return the
// bare basename, which then failed the caller's own executability probe with a
// confusing message).
inline std::string locate_engine(const std::string& exe_path = {}) {
  std::vector<fs::path> candidates;

  const std::string gs_engine = env_utf8("GS_ENGINE");
  if (!gs_engine.empty()) candidates.emplace_back(u8path_compat(gs_engine));

  fs::path exe_dir;
  if (!exe_path.empty()) {
    std::error_code ec;
    fs::path ep = fs::absolute(u8path_compat(exe_path), ec);
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
    if (path_is_executable(abs)) return path_u8string(abs);
  }

  // Step 5: PATH. Now an actual PATH search (see find_on_path above).
  return find_on_path(base);
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_ENGINE_LOCATOR_H
