// main.cpp - Gifscythe CLI driver.
//
// Two modes:
//   gifscythe-cli settings.conf              build + PRINT the gifsicle CLI line
//   gifscythe-cli settings.conf --run        build + RUN it against the bundled
//                                           gifsicle engine
//
// Proves the engine control layer end-to-end without the GUI toolkit. The printed
// command line is exactly what the GUI's "show me the command" pane displays.
//
// Execution NEVER goes through a shell (argv exec). Exit codes are honest.

#include "../core/SettingsIO.h"
#include "../core/GifsicleCommand.h"
#include "../core/EngineLocator.h"
#include "../core/ProcessRunner.h"
#include "../core/Validate.h"
#include "../core/version.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

namespace {

bool read_file(const fs::path& path, std::string* out) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  std::ostringstream ss;
  ss << f.rdbuf();
  *out = ss.str();
  return true;
}

// Expand a leading ~ to $HOME / USERPROFILE. Reject bare ~ alone as a path.
std::string expand_home(const std::string& p) {
  if (p.empty() || p[0] != '~') return p;
  if (p.size() > 1 && p[1] != '/' && p[1] != '\\') return p;  // ~user not expanded
  const char* home = std::getenv("HOME");
#ifdef _WIN32
  if (!home) home = std::getenv("USERPROFILE");
#endif
  if (!home) return p;
  if (p.size() == 1) return home;
  return std::string(home) + p.substr(1);
}

// Resolve a path relative to the settings file's directory.
std::string resolve_path(const std::string& p, const fs::path& base_dir) {
  if (p.empty()) return p;
  std::string expanded = expand_home(p);
  fs::path path(expanded);
  if (path.is_absolute()) return path.string();

  fs::path candidate = base_dir / path;
  std::error_code ec;
  if (fs::exists(candidate, ec)) return fs::weakly_canonical(candidate, ec).string();
  if (fs::exists(path, ec)) return fs::weakly_canonical(path, ec).string();
  // Prefer the base-anchored form even if it doesn't exist yet (for outputs).
  return candidate.string();
}

fs::path exe_path_of(const char* argv0) {
  std::error_code ec;
  fs::path p(argv0);
  if (p.is_absolute()) return p;
  // Try PATH lookup roughly: if relative and exists from CWD, use that.
  if (fs::exists(p, ec)) return fs::absolute(p, ec);
  return p;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::printf("Usage: %s <settings.conf> [--run] [--engine <path>]\n", argv[0]);
    std::printf("  Without --run: print the gifsicle command line (live pane).\n");
    std::printf("  With --run:    execute it against the bundled gifsicle engine.\n");
    std::printf("  GS_ENGINE env: override default engine path.\n");
    std::printf("  Gifscythe %s\n", GS_VERSION);
    return 2;
  }

  std::string settings_arg = argv[1];
  bool do_run = false;
  std::string engine_override;

  for (int i = 2; i < argc; ++i) {
    if (std::strcmp(argv[i], "--run") == 0) do_run = true;
    else if (std::strcmp(argv[i], "--engine") == 0 && i + 1 < argc) {
      engine_override = argv[++i];
    }
  }

  // Absolutize the settings path so everything is CWD-independent.
  std::error_code ec;
  fs::path settings_path = fs::absolute(expand_home(settings_arg), ec);
  if (ec) {
    std::fprintf(stderr, "ERROR: cannot resolve settings path '%s'\n", settings_arg.c_str());
    return 1;
  }
  fs::path base_dir = settings_path.parent_path();
  if (base_dir.empty()) base_dir = fs::current_path(ec);

  std::string content;
  if (!read_file(settings_path, &content)) {
    std::fprintf(stderr, "ERROR: cannot read settings file '%s'\n", settings_path.string().c_str());
    return 1;
  }

  std::vector<gs::LoadWarning> load_warnings;
  std::istringstream iss(content);
  gs::Settings s = gs::load_settings(iss, &load_warnings);
  for (const auto& w : load_warnings) {
    std::fprintf(stderr, "WARNING: settings key '%s' value '%s': %s\n",
                 w.key.c_str(), w.value.c_str(), w.reason.c_str());
  }

  // Resolve input/output relative paths against the settings file dir.
  for (auto& in : s.inputs) in = resolve_path(in, base_dir);
  if (!s.output.empty()) s.output = resolve_path(s.output, base_dir);

  auto warnings = gs::validate(s);
  for (const auto& w : warnings) {
    std::fprintf(stderr, "WARNING: %s=%s: %s\n",
                 w.field.c_str(), w.value.c_str(), w.reason.c_str());
  }

  // Locate engine.
  std::string engine_path;
  if (!engine_override.empty()) {
    engine_path = resolve_path(engine_override, fs::current_path(ec));
  } else {
    engine_path = gs::locate_engine(exe_path_of(argv[0]).string());
  }

  gs::GifsicleCommand cmd(s);
  std::string cli = cmd.toString();

  // The live CLI pane (shell-quoted for safe copy-paste).
  std::printf("# Gifscythe %s command (live CLI pane)\n", GS_VERSION);
  std::printf("%s %s\n", gs::shell_quote(engine_path).c_str(), cli.c_str());

  if (!do_run) {
    std::printf("# (use --run to execute)\n");
    return 0;
  }

  // Pre-flight: engine must exist.
  if (!gs::path_is_executable(engine_path)) {
    std::fprintf(stderr, "ERROR: engine not found at %s\n", engine_path.c_str());
    std::fprintf(stderr, "       Build it with ./scripts/build_gifsicle.sh\n");
    std::fprintf(stderr, "       Or set GS_ENGINE / pass --engine <path>\n");
    return 1;
  }

  // Build argv: engine + command args. NEVER concatenate into a shell string.
  std::vector<std::string> full_argv;
  full_argv.push_back(engine_path);
  const auto& args = cmd.args();
  full_argv.insert(full_argv.end(), args.begin(), args.end());

  std::printf("# -> running (argv exec, no shell)\n");
  int rc = gs::run_argv(full_argv);
  std::printf("# -> exit code %d\n", rc);
  return rc;  // honest 0..255
}
