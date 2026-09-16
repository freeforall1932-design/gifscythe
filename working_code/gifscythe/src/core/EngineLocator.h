// EngineLocator.h - Find the bundled gifsicle engine regardless of CWD.
// Search order:
//   1. Non-empty GS_ENGINE (exact path; invalid overrides STOP, never fall back)
//   2. Beside the running executable (packaged layout) — the executable path
//      comes from the OS, not argv[0], so a symlink install still finds its
//      sibling (U-65 / P1-41)
//   3. release/current/, then release/<GS_VERSION>/ — relative to the executable,
//      to its parent (the dev layout), to the CWD, and to working_code/ (U-66)
//   4. PATH (bare "gifsicle")
//
// Why `current` (audit U-66 / fix-order P1-41): the desktop pinned the engine to
// `release/<GS_VERSION>/` while the web server picked the newest numeric
// directory, so bumping VERSION.md moved one surface and not the other — the
// GUI would report "engine not found" while http://localhost:8000 kept working.
// `release/current` is the shared answer: a packager (or a developer) points it
// at exactly one engine build, every surface follows, and the numeric-newest
// rule remains the fallback so nothing that works today stops working.
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
#ifndef _WIN32
#include <unistd.h>
#endif

namespace gs {
namespace fs = std::filesystem;

// The version-independent release/ subdirectory every surface checks first (U-66).
// Declared here so the web server's mirror (web/server.mjs) and this file quote
// the same name rather than two literals that can drift.
inline constexpr const char* GS_ENGINE_CURRENT = "current";

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
  return ::access(p.c_str(), X_OK) == 0;
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

enum class EngineSource { None, Environment, Bundled, Path };

inline const char* engine_source_name(EngineSource source) {
  switch (source) {
    case EngineSource::Environment: return "GS_ENGINE";
    case EngineSource::Bundled: return "bundled/release";
    case EngineSource::Path: return "PATH";
    default: return "none";
  }
}

// An invalid explicit override is distinct from failed automatic discovery.
// The latter may still be printed as a prospective command by CLI print mode.
struct EngineResolution {
  std::string path;
  EngineSource source = EngineSource::None;
  std::string error;  // non-empty only for an invalid GS_ENGINE override
};

// exe_path: argv[0] or QCoreApplication::applicationFilePath().toStdString().
// Non-empty GS_ENGINE is an exact path relative to CWD, NOT a PATH search.
// Empty/unset preserves automatic discovery. File format/architecture is left
// to process launch; a spawn failure never triggers a second engine selection.
inline EngineResolution resolve_engine(const std::string& exe_path = {}) {
  const std::string gs_engine = env_utf8("GS_ENGINE");
  if (!gs_engine.empty()) {
    std::error_code ec;
    fs::path abs = fs::absolute(u8path_compat(gs_engine), ec);
    if (!ec && path_is_executable(abs))
      return {path_u8string(abs), EngineSource::Environment, {}};
    return {{}, EngineSource::Environment,
            "GS_ENGINE override is not an executable regular file: " + gs_engine
              + "; refusing automatic fallback"};
  }
  std::vector<fs::path> candidates;

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
  // release/<pin-or-version>/ in each of the four roots a caller might be in.
  // `current` first, so one symlink re-points every surface at once.
  const char* const roots[] = {"", "..", "../..", "working_code/gifscythe"};
  const char* const pins[] = {GS_ENGINE_CURRENT, GS_VERSION};
  for (const char* root : roots) {
    for (const char* pin : pins) {
      const fs::path rel = (root[0] == '\0')
          ? fs::path("release") / pin / base
          : fs::path(root) / "release" / pin / base;
      candidates.push_back(exe_dir / rel);
      candidates.push_back(rel);  // CWD-relative (running from product dir)
    }
  }

  for (const auto& c : candidates) {
    std::error_code ec;
    fs::path abs = fs::weakly_canonical(c, ec);
    if (ec) abs = fs::absolute(c, ec);
    if (ec) continue;
    if (path_is_executable(abs)) return {path_u8string(abs), EngineSource::Bundled, {}};
  }

  // Step 5: PATH. Now an actual PATH search (see find_on_path above).
  const auto from_path = find_on_path(base);
  return {from_path, from_path.empty() ? EngineSource::None : EngineSource::Path, {}};
}

// Compatibility for the GUI: failure is still an empty path. In particular,
// an invalid GS_ENGINE must not cause the GUI to select a different engine.
inline std::string locate_engine(const std::string& exe_path = {}) {
  return resolve_engine(exe_path).path;
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_ENGINE_LOCATOR_H
