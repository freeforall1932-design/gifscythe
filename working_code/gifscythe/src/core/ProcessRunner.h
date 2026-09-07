// ProcessRunner.h - Run argv without a shell. Honest exit codes.
// Qt-independent. POSIX: fork+execvp; Windows: _spawnvp.

#ifndef GIFSCYTHE_CORE_PROCESS_RUNNER_H
#define GIFSCYTHE_CORE_PROCESS_RUNNER_H

#include <string>
#include <vector>
#include <cstdio>

#ifdef _WIN32
  #include <process.h>
  #include <io.h>
#else
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <unistd.h>
  #include <cerrno>
  #include <cstring>
#endif

namespace gs {

// Run program + args (args[0] should be the program path). Returns the process
// exit code 0..255, or 127 if the binary could not be started, or 1 on other
// failures. NEVER goes through /bin/sh.
inline int run_argv(const std::vector<std::string>& args) {
  if (args.empty()) return 1;

#ifdef _WIN32
  std::vector<char*> argv;
  argv.reserve(args.size() + 1);
  std::vector<std::string> storage = args;  // mutable copies
  for (auto& s : storage) argv.push_back(s.data());
  argv.push_back(nullptr);
  const intptr_t rc = _spawnvp(_P_WAIT, argv[0], argv.data());
  if (rc < 0) {
    std::fprintf(stderr, "ERROR: failed to start '%s'\n", args[0].c_str());
    return 127;
  }
  return static_cast<int>(rc) & 0xff;
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
    std::fprintf(stderr, "ERROR: process killed by signal %d\n", WTERMSIG(status));
    return 1;
  }
  return 1;
#endif
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_PROCESS_RUNNER_H
