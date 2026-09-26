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
#include <system_error>
#include <vector>

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

// ---- U-59 / P0-7: a run must never write straight onto the user's file ----
//
// The engine writes direct to `-o <target>`, so a cancel (the engine is killed
// mid-write) or any failed run left a TRUNCATED file over the last good result:
// re-optimising an existing `<name>_opt.gif` and pressing Cancel destroyed it.
// The target is now written as a partial beside it and only promoted onto the
// target once the run is over (and, for verified modes, once the postcondition
// above has passed), so the previous bytes survive every failure path.
// Same tmp+rename class as U-01 (batch auto-naming) and U-16 (settings).

// The partial's name: the target plus a suffix, so it lands on the same
// filesystem (a rename across filesystems would not be atomic) and cannot
// collide with a planned target (OutputPlan.h plans `<name>_opt.gif` shapes,
// never a `.gs-partial` one).
inline std::string partial_output_path(const std::string& target) {
  return target + ".gs-partial";
}

// Redirect the single `-o <target>` operand of a full argv vector (argv[0] is
// the engine path) to `-o <partial>`. Returns false unless the operand occurs
// EXACTLY once with exactly that value: an ambiguous command line is refused by
// the caller rather than run with an unguarded write.
inline bool redirect_output_operand(std::vector<std::string>& argv,
                                    const std::string& target,
                                    const std::string& partial) {
  std::size_t hits = 0;
  std::size_t at = 0;
  for (std::size_t i = 1; i + 1 < argv.size(); ++i) {
    if (argv[i] == "-o" && argv[i + 1] == target) {
      ++hits;
      at = i + 1;
    }
  }
  if (hits != 1) return false;
  argv[at] = partial;
  return true;
}

// Replace the target with the verified partial. std::filesystem::rename has
// replace semantics on both POSIX and Windows, so the swap is one step and the
// target is never absent mid-run. Returns an error string, empty on success.
inline std::string promote_partial(const std::string& partial,
                                   const std::string& target) {
  std::error_code ec;
  std::filesystem::rename(u8path_compat(partial), u8path_compat(target), ec);
  if (ec) return "could not replace the output file (" + ec.message() + ")";
  return {};
}

// Best-effort removal of a partial left by a cancelled, refused or failed run.
// Deliberately silent: the caller has already reported the real failure, and a
// cleanup error must not mask it.
inline void discard_partial(const std::string& partial) {
  std::error_code ec;
  std::filesystem::remove(u8path_compat(partial), ec);
}
}
#endif
