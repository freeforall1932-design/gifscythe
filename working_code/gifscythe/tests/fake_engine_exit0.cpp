// fake_engine_exit0.cpp - a LYING GIF engine, used as a test fixture
// (audit U-17 / fix-order P1-19).
//
// It exits 0 and writes NOTHING: the exact scenario in which Explode used to
// report "Optimization complete." over an empty directory. T7 of the offscreen
// harness points GS_ENGINE at this binary and requires the GUI to refuse the
// false success; smoke_cli.sh drives the same scenario through the CLI with a
// shell equivalent. Deliberately trivial and dependency-free so it builds on
// every CI platform (linux g++, windows MinGW -static).

int main() {
  return 0;
}
