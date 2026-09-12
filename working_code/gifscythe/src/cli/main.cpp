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
#include "../core/OutputPlan.h"
#include "../core/ExplodeVerify.h"
#include "../core/WinUnicode.h"
#include "core/version.h"  // path form: resolves generated-first under CMake (U-15), src/ fallback under build.sh

#include <cstdarg>
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

void print_usage(const char* argv0, std::FILE* to) {
  std::fprintf(to, "Usage: %s <settings.conf> [--run] [--strict] [--engine <path>]\n", argv0);
  std::fprintf(to, "  Without --run: print the engine command line (live pane) on stdout.\n");
  std::fprintf(to, "  With --run:    execute it against the bundled gifsicle engine.\n");
  std::fprintf(to, "                 All commentary goes to stderr, so stdout stays a\n");
  std::fprintf(to, "                 clean byte stream when the settings have no output.\n");
  std::fprintf(to, "  --strict       refuse to continue when ANY settings warning was\n");
  std::fprintf(to, "                 printed (parse or validation); exit code 3.\n");
  std::fprintf(to, "  --engine PATH  use this gifsicle instead of the located one.\n");
  std::fprintf(to, "  --version      print the Gifscythe version and exit.\n");
  std::fprintf(to, "  Warning policy: out-of-range settings print a WARNING and the run\n");
  std::fprintf(to, "                proceeds anyway (the GUI refuses instead); pass\n");
  std::fprintf(to, "                --strict to make the CLI refuse like the GUI does.\n");
  std::fprintf(to, "                An UNSAFE output target is always refused with exit\n");
  std::fprintf(to, "                code 2. Exit codes: 0 ok, 1 engine/path/output\n");
  std::fprintf(to, "                verification failure, 2 usage or unsafe target,\n");
  std::fprintf(to, "                3 --strict refusal.\n");
  std::fprintf(to, "  GS_ENGINE env: override default engine path.\n");
  std::fprintf(to, "  Gifscythe %s\n", GS_VERSION);
}

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
  // U-07: env values are UTF-8 by contract; std::getenv would hand back ACP
  // bytes for a non-ASCII profile path on Windows.
  const std::string home_s = gs::env_utf8("HOME");
  const char* home = home_s.c_str();
#ifdef _WIN32
  const std::string profile = home_s.empty() ? gs::env_utf8("USERPROFILE") : std::string();
  if (!*home) home = profile.c_str();
#endif
  if (!home || !*home) return p;
  if (p.size() == 1) return home;
  return std::string(home) + p.substr(1);
}

// Resolve a path relative to the settings file's directory.
std::string resolve_path(const std::string& p, const fs::path& base_dir) {
  if (p.empty()) return p;
  std::string expanded = expand_home(p);
  fs::path path = gs::u8path_compat(expanded);
  if (path.is_absolute()) return gs::path_u8string(path);

  fs::path candidate = base_dir / path;
  std::error_code ec;
  if (fs::exists(candidate, ec)) return gs::path_u8string(fs::weakly_canonical(candidate, ec));
  if (fs::exists(path, ec)) return gs::path_u8string(fs::weakly_canonical(path, ec));
  // Prefer the base-anchored form even if it doesn't exist yet (for outputs).
  return gs::path_u8string(candidate);
}

fs::path exe_path_of(const char* argv0) {
  std::error_code ec;
  fs::path p = gs::u8path_compat(argv0);
  if (p.is_absolute()) return p;
  // Try PATH lookup roughly: if relative and exists from CWD, use that.
  if (fs::exists(p, ec)) return fs::absolute(p, ec);
  return p;
}

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
  // Audit U-07 (P1-4): MinGW's CRT encodes main()'s argv in the ANSI code
  // page, so any non-ASCII path was mangled before this program saw it.
  // Re-fetch the exact UTF-16 command line and split it per MSVCRT rules into
  // UTF-8 — the internal encoding contract of this codebase (WinUnicode.h).
  const std::vector<std::string> win_argv = gs::win_argv_utf8();
  std::vector<char*> win_ptrs;
  win_ptrs.reserve(win_argv.size() + 1);
  for (const auto& a : win_argv) win_ptrs.push_back(const_cast<char*>(a.c_str()));
  win_ptrs.push_back(nullptr);
  if (!win_argv.empty()) {
    argc = static_cast<int>(win_argv.size());
    argv = win_ptrs.data();
  }
#endif
  if (argc < 2) {
    print_usage(argv[0], stdout);
    return 2;
  }

  std::string settings_arg = argv[1];
  bool do_run = false;
  bool strict = false;
  std::string engine_override;

  // Strict argument parsing (audit U-23). Previously the loop below had no
  // else-branch, so a typo like --rnu was silently dropped and the run went
  // ahead with the wrong semantics (verified: rc=0, stderr empty, no message),
  // and `--engine` with no value was ignored just as quietly. A driver whose
  // whole job is honesty about what it ran cannot swallow its own arguments.
  if (std::strcmp(argv[1], "-h") == 0 || std::strcmp(argv[1], "--help") == 0) {
    print_usage(argv[0], stdout);
    return 0;
  }
  if (std::strcmp(argv[1], "--version") == 0) {
    std::printf("Gifscythe %s\n", GS_VERSION);
    return 0;
  }

  for (int i = 2; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--run") {
      do_run = true;
    } else if (a == "--strict") {
      strict = true;
    } else if (a == "--engine") {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "ERROR: --engine requires a path\n");
        print_usage(argv[0], stderr);
        return 2;
      }
      engine_override = argv[++i];
    } else if (a == "-h" || a == "--help") {
      print_usage(argv[0], stdout);
      return 0;
    } else if (a == "--version") {
      std::printf("Gifscythe %s\n", GS_VERSION);
      return 0;
    } else {
      std::fprintf(stderr, "ERROR: unknown argument '%s'\n", a.c_str());
      print_usage(argv[0], stderr);
      return 2;
    }
  }

  // Absolutize the settings path so everything is CWD-independent.
  std::error_code ec;
  fs::path settings_path = fs::absolute(gs::u8path_compat(expand_home(settings_arg)), ec);
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

  // ---- --strict (audit U-40 / fix-order P3-5) ----
  // Documented policy: the CLI prints warnings and proceeds, while the GUI
  // refuses. That asymmetry is deliberate for interactive use but wrong for
  // scripted use, where a warning nobody reads is a silent behavior change.
  // --strict closes the gap: any parse or validation warning becomes a
  // refusal with its own exit code (3), BEFORE anything is printed or run,
  // so a strict invocation is all-or-nothing.
  if (strict && (!load_warnings.empty() || !warnings.empty())) {
    std::fprintf(stderr,
                 "ERROR: --strict: %zu parse + %zu validation warning(s) above — refusing to continue\n",
                 load_warnings.size(), warnings.size());
    return 3;
  }

  // Locate engine.
  std::string engine_path;
  if (!engine_override.empty()) {
    engine_path = resolve_path(engine_override, fs::current_path(ec));
  } else {
    engine_path = gs::locate_engine(gs::path_u8string(exe_path_of(argv[0])));
  }

  // ---- stdout purity (audit U-04) ----
  // With --run and no `output` key, gifsicle writes GIF BYTES to fd 1. The
  // commentary below used to be printf'd into the same libc-buffered stdout,
  // so `gifscythe-cli conf --run > out.gif` produced a file that started with
  // GIF89a and ended with "# -> exit code 0" (verified: 8887 bytes, gifsicle
  // then reports "trailing garbage after GIF ignored"). In --run mode every
  // note goes to stderr and stdout belongs to the engine alone. Print mode
  // keeps the command line on stdout — that IS its documented output.
  auto note = [do_run](const char* fmt, ...) {
    std::FILE* to = do_run ? stderr : stdout;
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(to, fmt, ap);
    va_end(ap);
  };

  gs::GifsicleCommand cmd(s);
  std::string cli = cmd.toString();

  // The live CLI pane (shell-quoted for safe copy-paste).
  note("# Gifscythe %s command (live CLI pane)\n", GS_VERSION);
  note("%s %s\n", gs::shell_quote(engine_path.empty() ? gs::engine_basename() : engine_path).c_str(),
       cli.c_str());

  if (!do_run) {
    note("# (use --run to execute)\n");
    return 0;
  }

  // ---- Plan the output before anything runs (audit U-01) ----
  // The desktop GUI refuses runs whose target is a queued source or whose
  // targets collide; the CLI has to hold the same line, or `--run` becomes the
  // easy way around the guard. Explode is exempt: its `output` is a PREFIX and
  // the engine appends .000/.001, so it cannot land on an input.
  if (!s.output.empty() && s.mode != gs::Mode::Explode) {
    const gs::OutputPlan plan = gs::plan_outputs(s.inputs, {s.output});
    if (!plan.ok) {
      std::fprintf(stderr, "ERROR: refusing to run — the planned output is not safe:\n");
      std::fprintf(stderr, "%s\n", plan.describe().c_str());
      return 2;
    }
    for (const auto& p : plan.preexisting) {
      std::fprintf(stderr, "NOTE: output already exists and will be replaced: %s\n", p.c_str());
    }
  }

  // Pre-flight: engine must exist.
  if (!gs::path_is_executable(gs::u8path_compat(engine_path))) {
    std::fprintf(stderr, "ERROR: engine not found%s%s\n",
                 engine_path.empty() ? "" : " at ", engine_path.c_str());
    std::fprintf(stderr, "       Searched GS_ENGINE, the folders next to this executable,\n");
    std::fprintf(stderr, "       the release/ trees, and PATH.\n");
    std::fprintf(stderr, "       Build it with ./scripts/build_engine.sh\n");
    std::fprintf(stderr, "       Or set GS_ENGINE / pass --engine <path>\n");
    return 1;
  }

  // Build argv: engine + command args. NEVER concatenate into a shell string.
  std::vector<std::string> full_argv;
  full_argv.push_back(engine_path);
  const auto& args = cmd.args();
  full_argv.insert(full_argv.end(), args.begin(), args.end());

  // ---- Explode frame verification (audit U-17 / P1-19) ----
  // rc=0 alone used to mean success even when NOT A SINGLE frame was written.
  // Snapshot the prefix candidates BEFORE the run so leftovers from an earlier
  // run cannot fake it, then require at least one new/changed real GIF after.
  std::vector<gs::ExplodeFileState> explode_before;
  std::string explode_prefix;
  if (s.mode == gs::Mode::Explode) {
    explode_prefix = gs::explode_prefix_for(s);
    explode_before = gs::snapshot_explode_candidates(explode_prefix);
  }

  note("# -> running (argv exec, no shell)\n");
  int rc = gs::run_argv(full_argv);
  if (rc == 0 && s.mode == gs::Mode::Explode) {
    const gs::ExplodeResult vr =
        gs::verify_explode_frames(explode_prefix, explode_before);
    if (vr.ok) {
      note("# -> explode wrote %zu frame(s) under %s\n", vr.frames.size(),
           vr.prefix.c_str());
      if (!vr.suspicious.empty()) {
        note("# -> NOTE: %zu new/changed file(s) under the prefix are not GIFs\n",
             vr.suspicious.size());
      }
    } else {
      std::fprintf(stderr, "ERROR: engine exited 0 but wrote no frames — %s\n",
                   vr.describe().c_str());
      rc = 1;  // honest: the run did NOT produce what explode promises
    }
  }
  note("# -> exit code %d\n", rc);
  return rc;  // honest 0..255
}
