// ProcessRunner.h - Run argv without a shell. Honest exit codes.
// Qt-independent. POSIX: fork+execvp; Windows: CreateProcessA with a
// correctly quoted command line.
//
// Windows note (fixed 2026-09-07, found via Wine E2E): MinGW's _spawnvp
// joins arguments with spaces WITHOUT quoting, so any argument containing
// a space (e.g. C:\Users\John Smith\in.gif) was split into two arguments
// and the engine failed on the fragment. We now build the command line
// ourselves with MSVCRT argv-parsing-compatible quoting and call
// CreateProcessA directly — still never a shell (no cmd.exe).

#ifndef GIFSCYTHE_CORE_PROCESS_RUNNER_H
#define GIFSCYTHE_CORE_PROCESS_RUNNER_H

#include <string>
#include <vector>
#include <cstdio>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <unistd.h>
  #include <cerrno>
  #include <cstring>
#endif

namespace gs {

// Quote one argument per MSVCRT argv-parsing rules (the rules gifsicle's
// C runtime uses to re-split the command line). Pure string logic — kept
// platform-independent so it is unit-tested on Linux CI too:
//   * no space/tab/quote -> pass through unchanged
//   * otherwise          -> wrap in double quotes; every run of
//     backslashes immediately before a literal '"' is doubled and the
//     quote escaped as \" ; a trailing run of backslashes before the
//     closing quote is doubled so it cannot escape the quote itself.
inline std::string win_quote_arg(const std::string& a) {
  if (!a.empty() && a.find_first_of(" \t\"") == std::string::npos) return a;
  std::string out = "\"";
  size_t pending_backslashes = 0;
  for (char c : a) {
    if (c == '\\') { ++pending_backslashes; continue; }
    if (c == '"') {
      out.append(pending_backslashes * 2, '\\');
      out += "\\\"";
    } else {
      out.append(pending_backslashes, '\\');
      out += c;
    }
    pending_backslashes = 0;
  }
  out.append(pending_backslashes * 2, '\\');
  out += '"';
  return out;
}

// Run program + args (args[0] should be the program path). Returns the child's
// exit code 0..255. If the child was killed by a signal the shell convention
// 128+signum is returned (audit U-32). 127 means the binary could not be
// started; 1 means a fork/wait failure. NEVER goes through a shell — no /bin/sh
// and no cmd.exe are ever involved (verify_audit.sh E3 greps src/ for those
// tokens and exempts lines containing "NEVER"/"no shell").
inline int run_argv(const std::vector<std::string>& args) {
  if (args.empty()) return 1;

#ifdef _WIN32
  std::string cmdline;
  for (size_t i = 0; i < args.size(); ++i) {
    if (i) cmdline += ' ';
    cmdline += win_quote_arg(args[i]);
  }
  std::vector<char> cmdbuf(cmdline.begin(), cmdline.end());
  cmdbuf.push_back('\0');  // CreateProcess may write into the buffer

  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  if (!CreateProcessA(nullptr, cmdbuf.data(), nullptr, nullptr,
                      TRUE /*inherit std handles*/, 0, nullptr, nullptr,
                      &si, &pi)) {
    std::fprintf(stderr, "ERROR: failed to start '%s' (Win32 error %lu)\n",
                 args[0].c_str(), static_cast<unsigned long>(GetLastError()));
    return 127;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD code = 0;
  const BOOL ok = GetExitCodeProcess(pi.hProcess, &code);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  if (!ok) return 1;
  return static_cast<int>(code) & 0xff;
#else
  std::vector<char*> argv;
  argv.reserve(args.size() + 1);
  std::vector<std::string> storage = args;
  for (auto& s : storage) argv.push_back(s.data());
  argv.push_back(nullptr);

  const pid_t pid = fork();
  if (pid < 0) {
    std::fprintf(stderr, "ERROR: fork failed: %s\n", std::strerror(errno));
    return 1;
  }
  if (pid == 0) {
    execvp(argv[0], argv.data());
    std::fprintf(stderr, "ERROR: exec failed for '%s': %s\n",
                 args[0].c_str(), std::strerror(errno));
    _exit(127);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    std::fprintf(stderr, "ERROR: waitpid failed: %s\n", std::strerror(errno));
    return 1;
  }
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  if (WIFSIGNALED(status)) {
    // Audit U-32: report the shell convention 128+signum, not a flat 1. Callers
    // could not previously tell a crash from a kill — and the GUI's cancel path
    // kills the engine on purpose, so the distinction is not academic.
    std::fprintf(stderr, "ERROR: process killed by signal %d\n", WTERMSIG(status));
    return 128 + WTERMSIG(status);
  }
  return 1;
#endif
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_PROCESS_RUNNER_H
