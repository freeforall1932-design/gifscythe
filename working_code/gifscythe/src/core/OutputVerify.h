// GS-203: Qt-independent single-file postcondition. New or size/mtime-changed,
// non-empty regular file, exact GIF87a/89a signature. Not a full GIF decoder.
// Identical rewrites within filesystem timestamp granularity fail conservatively.
#ifndef GIFSCYTHE_CORE_OUTPUT_VERIFY_H
#define GIFSCYTHE_CORE_OUTPUT_VERIFY_H
#include "WinUnicode.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <string>

namespace gs {
struct OutputSnapshot {
  bool exists = false;
  std::uintmax_t size = 0;
  std::filesystem::file_time_type mtime{};
  std::string error;
};
inline OutputSnapshot snapshot_output(const std::string& name) {
  namespace fs = std::filesystem;
  OutputSnapshot result;
  const auto path = u8path_compat(name);
  std::error_code ec;
  const auto status = fs::status(path, ec);
  if (ec == std::errc::no_such_file_or_directory || status.type() == fs::file_type::not_found)
    return result;
  if (ec || !fs::is_regular_file(status)) {
    result.error = "output is not an accessible regular file";
    return result;
  }
  result.exists = true;
  result.size = fs::file_size(path, ec);
  if (!ec) result.mtime = fs::last_write_time(path, ec);
  if (ec) result.error = "cannot snapshot output metadata";
  return result;
}
inline std::string verify_output(const std::string& name, const OutputSnapshot& before) {
  if (!before.error.empty()) return before.error;
  const auto after = snapshot_output(name);
  if (!after.error.empty()) return after.error;
  if (!after.exists || after.size == 0) return "engine produced no output file (missing or empty)";
  if (before.exists && before.size == after.size && before.mtime == after.mtime)
    return "output is unchanged since the pre-run snapshot";
  std::ifstream in(u8path_compat(name), std::ios::binary);
  char header[6]{};
  in.read(header, 6);
  if (in.gcount() != 6 || (std::memcmp(header, "GIF87a", 6) != 0 && std::memcmp(header, "GIF89a", 6) != 0))
    return "invalid GIF output (expected GIF87a or GIF89a signature)";
  return {};
}
}
#endif
