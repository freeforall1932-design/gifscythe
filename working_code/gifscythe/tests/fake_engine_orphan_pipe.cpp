// fake_engine_orphan_pipe.cpp - a fixture whose CHILD keeps the inherited
// stdout/stderr pipes open after the parent is killed (U-12 / P1-24, S37).
//
// WHY THIS EXISTS: U-12 says the "fully async" GUI still blocks the UI thread in
// five places, "up to 5 s per run start". Every one of those waits is
// `kill(); waitForFinished(N)` — and measuring them needed a process for which
// that wait REALLY BLOCKS, because SIGKILL itself is immediate: the harness's
// existing fixtures die instantly, so `waitForFinished` returns in ~1 ms and the
// freeze is invisible.
//
// The block is real when a killed process leaves a CHILD holding the write ends
// of the pipes: Qt must drain the channels to finish the wait, and the drain
// cannot complete while an orphan still holds them. That is the "wedged engine"
// shape a user actually hits (an engine that forked a helper, a shell wrapper, a
// crashed-but-lingering child), and it is constructible on purpose here.
//
// Behaviour:
//   * parent: sleeps GS_FAKE_ORPHAN_MS (default 30000) so it is ALIVE when the
//     harness kills it;
//   * child (forked / respawned before the parent sleeps): holds the inherited
//     stdout/stderr open for the same duration, ignoring nothing - it is not the
//     process Qt kills, so SIGKILL on the parent does not reach it.
//   * writes nothing to stdout/stderr: the pipes stay open simply by being open.
//
// It deliberately does NOT write -o: the point is to be alive and unkillable-as-a-
// family, not to produce output. Harness case T26 kills it and measures the
// event-loop gap; nothing in this file is product code.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {

int holdMs() {
  const char* env = std::getenv("GS_FAKE_ORPHAN_MS");
  if (env && *env) {
    const int v = std::atoi(env);
    if (v > 0) return v;
  }
  return 30000;
}

void sleepMs(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

}  // namespace

int main(int argc, char** argv) {
  const int hold = holdMs();

  // Child mode: hold the inherited pipes open and never touch anything else.
  if (argc > 1 && std::strcmp(argv[1], "--hold-pipes") == 0) {
    sleepMs(hold);
    return 0;
  }

  // Grandchild: inherits stdout/stderr from the parent (which inherited them
  // from QProcess), so the pipes stay open past the parent's death.
#ifdef _WIN32
  // _spawnl keeps the parent's handles inherited by default, which is exactly
  // what this fixture needs; no wait, so the child outlives the parent.
  _spawnl(_P_NOWAIT, argv[0], argv[0], "--hold-pipes", nullptr);
#else
  const pid_t pid = ::fork();
  if (pid == 0) {
    // Do not run atexit handlers or flush anything: just hold the fds.
    sleepMs(hold);
    ::_exit(0);
  }
#endif

  // Parent: alive until the harness kills it. Nothing on stdout/stderr, so a
  // reader cannot mistake this for output.
  sleepMs(hold);
  return 0;
}
