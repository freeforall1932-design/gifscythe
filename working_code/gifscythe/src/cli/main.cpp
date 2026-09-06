// main.cpp - Gifscythe CLI driver.
//
// Two modes:
//   gifscythe-cli settings.conf              build + PRINT the gifsicle CLI line
//   gifscythe-cli settings.conf --run        build + RUN it against the bundled
//                                           gifsicle engine
//
// This proves the engine control layer end-to-end without needing the GUI
// toolkit. It is also the power-user terminal interface: the printed command
// line is exactly what the GUI's "show me the command" pane will display.
//
// DEPENDENCIES: gifsicle engine built at release/<version>/gifsicle (for --run).

#include "../core/SettingsIO.h"
#include "../core/GifsicleCommand.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <unistd.h>

namespace fs_probe {
bool exists(const std::string& p) {
  FILE* f = std::fopen(p.c_str(), "rb");
  if (f) { std::fclose(f); return true; }
  return false;
}
}  // namespace fs_probe

namespace {
bool read_file(const std::string& path, std::string* out) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  char buf[4096]; size_t n;
  while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) out->append(buf, n);
  std::fclose(f);
  return true;
}

// Resolve a path relative to the settings file's directory. Absolute paths and
// relative-to-CWD paths that exist are passed through unchanged.
std::string resolve_path(const std::string& p, const std::string& base_dir) {
  if (p.empty() || p[0] == '/' || p[0] == '~') return p;
  std::string base = base_dir == "." ? "" : base_dir + "/";
  std::string candidate = base + p;
  // Prefer the base_dir-anchored path if it exists, else the plain relative one.
  if (fs_probe::exists(candidate)) return candidate;
  if (fs_probe::exists(p)) return p;
  return candidate;
}

// --- minimal command execution (no Qt; system() is fine for a dev tool) ---
int run_with_system(const std::string& cmd) {
  return std::system(cmd.c_str());
}
}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::printf("Usage: %s <settings.conf> [--run] [--engine <path>]\n", argv[0]);
    std::printf("  Without --run: print the gifsicle command line (live pane).\n");
    std::printf("  With --run:    execute it against the bundled gifsicle engine.\n");
    return 2;
  }

  std::string settings_path = argv[1];
  bool do_run = false;
  // Default engine path, relative to the repo root (resolved to absolute below).
  std::string engine_path = "working_code/gifscythe/release/0.1.0/gifsicle";

  std::string base_dir = ".";
  {
    size_t slash = settings_path.find_last_of('/');
    if (slash != std::string::npos) base_dir = settings_path.substr(0, slash);
  }
  // Make base_dir absolute so it works regardless of CWD.
  if (base_dir == ".") {
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd))) base_dir = cwd;
  }

  for (int i = 2; i < argc; ++i) {
    if (std::strcmp(argv[i], "--run") == 0) do_run = true;
    else if (std::strcmp(argv[i], "--engine") == 0 && i + 1 < argc) {
      engine_path = argv[++i];
    }
  }

  // The default engine path is relative to the REPO ROOT, not the settings dir.
  // Find the repo root: settings file lives at <root>/working_code/gifscythe/examples/.
  std::string repo_root;
  if (!settings_path.empty() && settings_path[0] == '/') {
    repo_root = settings_path;
    // Strip <root>/working_code/gifscythe/examples/<file>
    size_t idx = repo_root.find("/working_code/gifscythe");
    if (idx != std::string::npos) repo_root = repo_root.substr(0, idx);
    else {
      // fallback: file's parent's parent's parent
      repo_root = settings_path.substr(0, settings_path.find_last_of('/'));
      repo_root = repo_root.substr(0, repo_root.find_last_of('/'));
      repo_root = repo_root.substr(0, repo_root.find_last_of('/'));
    }
  } else {
    repo_root = base_dir;
  }

  // Resolve engine path: if relative, anchor to repo root (search locations).
  if (!engine_path.empty() && engine_path[0] != '/') {
    std::string cand = repo_root + "/" + engine_path;
    if (fs_probe::exists(cand)) engine_path = cand;
    else engine_path = resolve_path(engine_path, base_dir);
  }

  std::string content;
  if (!read_file(settings_path, &content)) {
    std::fprintf(stderr, "ERROR: cannot read settings file '%s'\n", settings_path.c_str());
    return 1;
  }
  std::istringstream iss(content);
  gs::Settings s = gs::load_settings(iss);

  // Resolve input/output relative paths against the settings file dir.
  for (auto& in : s.inputs) in = resolve_path(in, base_dir);
  if (!s.output.empty()) s.output = resolve_path(s.output, base_dir);

  gs::GifsicleCommand cmd(s);
  std::string cli = cmd.toString();

  // The live CLI pane.
  std::printf("# Gifscythe command (live CLI pane)\n");
  std::printf("%s %s\n", engine_path.c_str(), cli.c_str());

  if (!do_run) {
    std::printf("# (use --run to execute)\n");
    return 0;
  }

  // Build the actual invocation.
  std::string full = engine_path + " " + cli;
  std::printf("# -> running\n");
  int rc = run_with_system(full);
  std::printf("# -> exit code %d\n", rc);
  return rc;
}
