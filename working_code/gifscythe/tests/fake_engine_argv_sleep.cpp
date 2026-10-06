// fake_engine_argv_sleep.cpp - a SLOW, argv-logging GIF engine fixture.
//
// The offscreen GUI harness needs a "engine" that (a) makes each run's exact
// argv observable on disk, (b) stays alive long enough that a control can be
// mutated or a Cancel pressed while it is genuinely running, and (c) still
// produces output the GUI's own U-59 promotion path accepts (non-empty file
// with a GIF87a/GIF89a signature - see src/core/OutputVerify.h). The real
// gifsicle can do none of those on demand, and the two existing fixtures are
// one-shot (exit0 writes nothing; partial_failure writes corrupt bytes).
//
// Used by these harness groups:
//   T22  U-72 / P1-42  cancel-latch honesty: the sleep is the window in which
//                      Cancel is pressed while the process is really running.
//   T24  U-58 / P1-38  mid-batch settings mutation: one argv log line per job
//                      proves what the continuation actually ran.
//
// Environment contract (all values are ASCII in every caller - the harness
// puts the log in a QTemporaryDir - so std::getenv is safe here; this file
// stays dependency-free like the other fixtures, deliberately):
//   GS_FAKE_ARGV_LOG   append-extended argv log: ONE line per run, the argv
//                      tokens joined by TAB, no program name. Unset = no log.
//   GS_FAKE_SLEEP_MS   how long the process stays alive before exiting
//                      (default 800; the harness's mutation/cancel window).
//   GS_FAKE_EXIT       explicit exit code. Unset = 0 when output was written,
//                      64 when it was not (the other fixtures' convention).
//
// Input/output discovery understands the shape GifsicleCommand builds for a
// default single-file job (src/core/GifsicleCommand.h): flags, then the input
// path, then "-o <path>". The input is the LAST token before "-o" that does
// not start with '-' and names an existing file; if none does, nothing is
// written and the run exits non-zero, so a mis-parse surfaces as a visible
// harness failure instead of a silent pass.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

std::string envOr(const char* name, const std::string& fallback) {
  const char* v = std::getenv(name);
  return (v && *v) ? std::string(v) : fallback;
}

long envLong(const char* name, long fallback) {
  const char* v = std::getenv(name);
  if (!v || !*v) return fallback;
  char* end = nullptr;
  const long parsed = std::strtol(v, &end, 10);
  return (end && *end == '\0') ? parsed : fallback;
}

void appendArgvLog(const std::string& path, const std::vector<std::string>& args) {
  if (path.empty()) return;
  std::ofstream log(path, std::ios::binary | std::ios::app);
  if (!log) return;
  for (size_t i = 0; i < args.size(); ++i) {
    if (i) log << '\t';
    log << args[i];
  }
  log << '\n';
  // Closed here, BEFORE the sleep: the harness polls this file to know a run
  // has started, so the line must be durable, not buffered until exit.
}

}  // namespace

int main(int argc, char** argv) {
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
  appendArgvLog(envOr("GS_FAKE_ARGV_LOG", ""), args);

  // "-o <path>" is how every GUI run directs output (GifsicleCommand.h).
  std::string output;
  size_t outputAt = args.size();
  for (size_t i = 0; i + 1 < args.size(); ++i) {
    if (args[i] == "-o") {
      output = args[i + 1];
      outputAt = i;
      break;
    }
  }

  // The input is the last pre"-o" token that is not a flag and exists on disk.
  std::string input;
  for (size_t i = outputAt; i-- > 0;) {
    if (!args[i].empty() && args[i][0] != '-' && std::ifstream(args[i]).good()) {
      input = args[i];
      break;
    }
  }

  bool wroteOutput = false;
  if (!output.empty() && !input.empty()) {
    std::ifstream in(input, std::ios::binary);
    std::ofstream out(output, std::ios::binary | std::ios::trunc);
    if (in && out) {
      out << in.rdbuf();
      out.close();
      wroteOutput = out.good();
    }
  }

  const long sleepMs = envLong("GS_FAKE_SLEEP_MS", 800);
  if (sleepMs > 0) std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));

  const long exitCode = envLong("GS_FAKE_EXIT", wroteOutput ? 0 : 64);
  if (!wroteOutput && exitCode == 0) {
    std::cerr << "fake_engine_argv_sleep: no usable -o/input pair to copy\n";
  }
  return static_cast<int>(exitCode);
}
