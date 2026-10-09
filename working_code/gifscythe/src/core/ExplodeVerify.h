// ExplodeVerify.h - Verify that an Explode run actually wrote frames
// (audit U-17 / fix-order P1-19). Qt-independent (STL only), so it is shared
// by the CLI and the GUI and unit-tested on every platform.
//
// WHY: rc=0 alone used to mean "complete". An engine that exits 0 without
// writing anything (the audit's test-engine repro), or a prefix that lands
// somewhere unexpected, produced "Optimization complete" over an empty
// directory. Every other mode verifies its single output; Explode was
// explicitly skipped. This header closes that hole with the rule the audit
// prescribes: snapshot the candidates BEFORE the run, enumerate AFTER, and
// require at least one NEW-or-CHANGED file that is a real GIF.
//
// Engine naming semantics (read off reference_code/gifsicle, never edited):
//   * -e  writes <prefix>.NNN  (%03d while <=1000 frames, more digits above;
//       src/support.c explode_filename())
//   * -E  writes <prefix>.<frame name> for named frames, else <prefix>.NNN
//   * with no -o, gifsicle uses the input file's BASENAME in the CWD as the
//       prefix (src/gifsicle.c input_done(): "Explode into current directory")
// so every possible frame file starts with "<prefix>." - matching on that
// covers both -e and -E without knowing frame names in advance.
//
// Known accepted edge: a re-run that rewrites a frame with IDENTICAL content
// inside the filesystem's mtime granularity would not count as changed. ext4/
// NTFS timestamps are fine-grained enough that this needs a same-instant
// byte-identical rewrite; the alternative (trusting pre-existing files) is the
// exact stale-frame lie this check exists to prevent.

#ifndef GIFSCYTHE_CORE_EXPLODE_VERIFY_H
#define GIFSCYTHE_CORE_EXPLODE_VERIFY_H

#include "GifsicleSettings.h"
#include "WinUnicode.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace gs {
namespace fs = std::filesystem;

// One candidate file's identity before/after a run.
// Every regular file named "<prefix basename>.*" in the prefix's directory.
// Shared by promotion and cleanup so the two can never disagree about what a
// frame is (N-10).
inline std::vector<fs::path> explode_frames_under(const std::string& prefix) {
  std::vector<fs::path> out;
  if (prefix.empty()) return out;
  std::error_code ec;
  const fs::path p = u8path_compat(prefix);
  fs::path dir = p.parent_path();
  if (dir.empty()) dir = fs::path(".");
  const std::string match = path_u8string(p.filename()) + ".";
  if (!fs::is_directory(dir, ec)) return out;
  for (const auto& e : fs::directory_iterator(dir, ec)) {
    if (ec) { ec.clear(); break; }
    if (!e.is_regular_file(ec)) { ec.clear(); continue; }
    const std::string name = path_u8string(e.path().filename());
    if (name.size() <= match.size()) continue;
    if (name.compare(0, match.size(), match) != 0) continue;
    out.push_back(e.path());
  }
  return out;
}

// N-10: frame-level promotion. gifsicle opens every <prefix>.NNN with
// truncating semantics (fopen(..., "wb")), so a failed re-run, a cancel or a
// SIGKILL truncates the PREVIOUS good frame set in place: the user is left with
// fewer valid frames than before and nothing telling them. The new set is
// therefore written under a partial prefix and moved over the old one only
// after verification, which is the two-phase write every other mode uses.
//
// Returns an error string on failure. A failure here leaves the destination set
// MIXED (some frames replaced, some not), so the caller must report that
// honestly rather than claim success.
inline std::string promote_explode_frames(const std::string& partial_prefix,
                                          const std::string& prefix) {
  const std::vector<fs::path> frames = explode_frames_under(partial_prefix);
  if (frames.empty()) return "no frames were produced under the partial prefix";
  const std::string match =
      path_u8string(u8path_compat(partial_prefix).filename()) + ".";
  const std::string dst_base = prefix;
  std::error_code ec;
  for (const auto& src : frames) {
    const std::string tail = path_u8string(src.filename()).substr(match.size());
    const fs::path dst = u8path_compat(dst_base + "." + tail);
    fs::rename(src, dst, ec);
    if (ec) return "could not replace " + path_u8string(dst) + ": " + ec.message();
  }
  return std::string();
}

// Best-effort cleanup of a partial frame set left by a failed, cancelled or
// superseded run. Never fatal: a leftover partial is untidy, not wrong.
inline void discard_explode_frames(const std::string& partial_prefix) {
  std::error_code ec;
  for (const auto& f : explode_frames_under(partial_prefix)) fs::remove(f, ec);
  ec.clear();
}

// AUD-02: claim a partial prefix this run can OWN. The old callers ran
// discard_explode_frames(<prefix>.gs-partial) first, deleting every file named
// "<prefix>.gs-partial.*" - including user files or inputs that merely share
// the name. This deletes nothing: it returns the first of
// `<prefix>.gs-partial`, `<prefix>.gs-partial.1`, ... with NO file at that
// exact path and NO file named "<candidate>.*" (so promotion and cleanup,
// which match on that pattern, only ever see frames this run wrote). Returns
// "" when nothing is free or the directory cannot be listed; refuse then.
inline std::string claim_partial_explode_prefix(const std::string& prefix) {
  if (prefix.empty()) return {};
  const std::string base = prefix + ".gs-partial";
  std::error_code ec;
  fs::path dir = u8path_compat(base).parent_path();
  if (dir.empty()) dir = fs::path(".");
  if (!fs::is_directory(dir, ec)) return {};
  for (int i = 0; i < 1000; ++i) {
    const std::string cand = i ? base + "." + std::to_string(i) : base;
    ec.clear();
    const auto st = fs::symlink_status(u8path_compat(cand), ec);
    if (ec && ec != std::errc::no_such_file_or_directory) return {};
    if (st.type() != fs::file_type::not_found) continue;
    // Any entry (not only regular files) named "<cand>.*" makes it occupied.
    bool taken = false;
    const std::string match = path_u8string(u8path_compat(cand).filename()) + ".";
    for (const auto& e : fs::directory_iterator(dir, ec)) {
      const std::string name = path_u8string(e.path().filename());
      if (name.size() > match.size() && name.compare(0, match.size(), match) == 0) {
        taken = true;
        break;
      }
    }
    if (ec) return {};
    if (!taken) return cand;
  }
  return {};
}

struct ExplodeFileState {
  std::string name;  // file NAME (not full path): comparison key
  std::uintmax_t size = 0;
  fs::file_time_type mtime{};
};

// The prefix gifsicle will explode against for these settings: the explicit
// `output` when set; otherwise the first input's basename (gifsicle explodes
// into the CWD then). Cuts on '/' AND '\' so the rule is testable on any
// platform (Windows accepts both separators).
inline std::string explode_prefix_for(const Settings& s) {
  if (!s.output.empty()) return s.output;
  if (s.inputs.empty()) return std::string();
  const std::string& in = s.inputs.front();
  const std::string::size_type cut = in.find_last_of("/\\");
  return cut == std::string::npos ? in : in.substr(cut + 1);
}

// Snapshot every existing file named "<prefix basename>.*" in the prefix's
// directory. Call BEFORE the engine runs; diff AFTER with verify_explode_frames.
inline std::vector<ExplodeFileState> snapshot_explode_candidates(
    const std::string& prefix) {
  std::vector<ExplodeFileState> out;
  if (prefix.empty()) return out;
  std::error_code ec;
  const fs::path p = u8path_compat(prefix);
  fs::path dir = p.parent_path();
  if (dir.empty()) dir = fs::path(".");
  const std::string match = path_u8string(p.filename()) + ".";
  if (!fs::is_directory(dir, ec)) return out;
  for (const auto& e : fs::directory_iterator(dir, ec)) {
    if (ec) break;
    if (!e.is_regular_file(ec)) continue;
    const std::string name = path_u8string(e.path().filename());
    if (name.size() <= match.size()) continue;           // "<stem>." alone is no frame
    if (name.compare(0, match.size(), match) != 0) continue;
    ExplodeFileState st;
    st.name = name;
    st.size = e.file_size(ec);
    if (ec) { st.size = 0; ec.clear(); }
    st.mtime = fs::last_write_time(e.path(), ec);
    if (ec) ec.clear();
    out.push_back(st);
  }
  return out;
}

struct ExplodeResult {
  bool ok = false;                       // >=1 new-or-changed VALID GIF frame
  std::vector<std::string> frames;       // new-or-changed, GIF87a/GIF89a magic
  std::vector<std::string> suspicious;   // new-or-changed but empty / not a GIF
  std::string prefix;                    // exactly what was searched
  std::string directory;

  // Honest failure text: names the prefix and directory searched and what the
  // rule is, so the user can see WHERE the frames were expected (audit A-17:
  // "surface the exact prefix searched on failure").
  std::string describe() const {
    std::string d =
        "Explode verification failed: no frame file was written.\n"
        "  prefix searched: " + prefix + "\n"
        "  directory:       " + directory + "\n"
        "  rule: after exit 0 at least one NEW or CHANGED file named\n"
        "        <prefix>.NNN (-e) or <prefix>.<frame name> (-E) must exist\n"
        "        and start with GIF87a/GIF89a.";
    if (!suspicious.empty()) {
      d += "\n  found " + std::to_string(suspicious.size()) +
           " new/changed file(s) under the prefix, but none is a valid GIF:";
      for (const auto& s : suspicious) d += "\n    " + s;
    }
    return d;
  }
};

inline bool explode_file_is_gif(const fs::path& f) {
  std::ifstream in(f, std::ios::binary);
  char hdr[6] = {0, 0, 0, 0, 0, 0};
  in.read(hdr, 6);
  return in.gcount() == 6 && std::memcmp(hdr, "GIF8", 4) == 0 &&
         (hdr[4] == '7' || hdr[4] == '9') && hdr[5] == 'a';
}

// Diff the post-run directory against the pre-run snapshot. A file counts as
// written-by-this-run when it is NEW or its (size, mtime) changed; untouched
// leftovers from an earlier run must not fake a success.
inline ExplodeResult verify_explode_frames(
    const std::string& prefix, const std::vector<ExplodeFileState>& before) {
  ExplodeResult r;
  r.prefix = prefix;
  if (prefix.empty()) {
    r.directory = "(none - no output prefix and no input to derive one from)";
    return r;
  }
  std::error_code ec;
  const fs::path p = u8path_compat(prefix);
  fs::path dir = p.parent_path();
  if (dir.empty()) dir = fs::path(".");
  r.directory = path_u8string(dir);
  const std::string match = path_u8string(p.filename()) + ".";
  if (!fs::is_directory(dir, ec)) return r;

  for (const auto& e : fs::directory_iterator(dir, ec)) {
    if (ec) break;
    if (!e.is_regular_file(ec)) continue;
    const std::string name = path_u8string(e.path().filename());
    if (name.size() <= match.size()) continue;
    if (name.compare(0, match.size(), match) != 0) continue;

    std::uintmax_t size = e.file_size(ec);
    if (ec) { size = 0; ec.clear(); }
    fs::file_time_type mtime = fs::last_write_time(e.path(), ec);
    if (ec) ec.clear();

    bool existed = false;
    for (const auto& b : before) {
      if (b.name == name) {
        existed = true;
        if (b.size == size && b.mtime == mtime) break;  // untouched -> stale
        // size or mtime changed -> this run rewrote it; fall through
        if (explode_file_is_gif(e.path())) r.frames.push_back(name);
        else r.suspicious.push_back(name);
        break;
      }
    }
    if (existed) continue;
    if (explode_file_is_gif(e.path())) r.frames.push_back(name);
    else r.suspicious.push_back(name);
  }
  r.ok = !r.frames.empty();
  return r;
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_EXPLODE_VERIFY_H
