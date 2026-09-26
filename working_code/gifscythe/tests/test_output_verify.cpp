#include "core/OutputVerify.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
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

  // U-59 / P0-7: the tmp+rename guard around the engine's `-o <target>` write.
  check(gs::partial_output_path("out.gif") == "out.gif.gs-partial");
  std::vector<std::string> cmd{"gifsicle", "-O3", "-o", "out.gif", "in.gif"};
  check(gs::redirect_output_operand(cmd, "out.gif", "out.gif.gs-partial"));
  check(cmd[2] == "-o" && cmd[3] == "out.gif.gs-partial" && cmd[4] == "in.gif");
  std::vector<std::string> other{"gifsicle", "-o", "elsewhere.gif", "in.gif"};
  check(!gs::redirect_output_operand(other, "out.gif", "p"));
  check(other[2] == "elsewhere.gif");  // refused, and left untouched
  std::vector<std::string> twice{"gifsicle", "-o", "out.gif", "-o", "out.gif"};
  check(!gs::redirect_output_operand(twice, "out.gif", "p"));
  std::vector<std::string> noout{"gifsicle", "-O3", "in.gif"};
  check(!gs::redirect_output_operand(noout, "out.gif", "p"));
  const auto promoted = dir / "promote.gif";
  const auto partial = dir / "promote.gif.gs-partial";
  { std::ofstream out(promoted, std::ios::binary); out << "OLD"; }
  { std::ofstream out(partial, std::ios::binary); out << "GIF89a NEW"; }
  check(gs::promote_partial(gs::path_u8string(partial), gs::path_u8string(promoted)).empty());
  check(!fs::exists(partial));
  { std::ifstream in(promoted, std::ios::binary);
    const std::string body((std::istreambuf_iterator<char>(in)), {});
    check(body == "GIF89a NEW"); }  // promote really replaced the old bytes
  check(!gs::promote_partial(gs::path_u8string(partial), gs::path_u8string(promoted)).empty());
  { std::ofstream out(partial, std::ios::binary); out << "x"; }
  gs::discard_partial(gs::path_u8string(partial));
  check(!fs::exists(partial));
  gs::discard_partial(gs::path_u8string(partial));  // idempotent: must not throw
  check(!fs::exists(partial));

  std::cout << "Output verifier: " << tests << " assertions, " << failures << " failures\n";
  return failures ? 1 : 0;
}
