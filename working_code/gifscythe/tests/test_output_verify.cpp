#include "core/OutputVerify.h"
#include <iostream>
#include <chrono>
int main(int argc, char** argv) {
  if (argc != 2) return 2;
  namespace fs = std::filesystem;
  const auto dir = gs::u8path_compat(argv[1]);
  const auto file = dir / "out.gif";
  const auto name = gs::path_u8string(file);
  int tests = 0, failures = 0;
  auto check = [&](bool good) { ++tests; if (!good) { ++failures; std::cerr << "failure " << tests << '\n'; } };
  auto write = [&](const char* bytes) { std::ofstream out(file, std::ios::binary); out << bytes; };
  auto absent = gs::snapshot_output(name);
  check(!absent.exists && absent.error.empty());
  check(!gs::verify_output(name, absent).empty());
  write(""); check(!gs::verify_output(name, absent).empty());
  write("GIF89"); check(!gs::verify_output(name, absent).empty());
  write("PNG garbage"); check(!gs::verify_output(name, absent).empty());
  write("GIF87a fixture"); check(gs::verify_output(name, absent).empty());
  auto before = gs::snapshot_output(name);
  check(!gs::verify_output(name, before).empty());
  fs::last_write_time(file, before.mtime + std::chrono::seconds(2));
  check(gs::verify_output(name, before).empty());
  before = gs::snapshot_output(name);
  write("GIF89a longer fixture"); fs::last_write_time(file, before.mtime);
  check(gs::verify_output(name, before).empty());
  check(!gs::snapshot_output(gs::path_u8string(dir)).error.empty());
  auto bad = before; bad.error = "snapshot failed";
  check(gs::verify_output(name, bad) == bad.error);
  fs::remove(file); check(!gs::verify_output(name, before).empty());
  std::cout << "Output verifier: " << tests << " assertions, " << failures << " failures\n";
  return failures ? 1 : 0;
}
