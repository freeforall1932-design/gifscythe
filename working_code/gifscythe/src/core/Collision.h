// Collision.h - what a run does when the destination already holds a file.
//
// The owner's answer, 2026-10-26: "Ask each time by default ... [with] skip all
// beside rename or overwrite ... it will show in tallying in error segment this
// file name at this location ... it can be changed to 3 other options ... in
// output dedicated square clickable".
//
// So the DEFAULT is to ask, the dialog offers rename/overwrite/skip plus "all",
// and the four policies are also settable up front. That mix only works if the
// decision itself is a pure function with no UI in it - otherwise the code that
// asks and the code that runs can disagree, and the disagreement shows up as a
// deleted file.
//
// POLICY SEMANTICS (each one is a different promise)
//   Ask       - the caller collects collisions and asks; this layer never guesses.
//   Rename    - never destroy: go_result.webp, go_result_1.webp, go_result_2.webp
//               (the same numbering the naming rules already use for duplicates
//               inside one run; the FIRST result keeps the plain name).
//   Overwrite - the user said replace. The run still writes through a verified
//               partial and only promotes on success, so "overwrite" never means
//               "shred the old file and then fail".
//   Skip      - leave the old file, mark the row skipped with the full path so
//               the tally can say exactly which file, where.
//
// What is deliberately NOT here: any filesystem write. `exists` is injected, so
// the numbering is provable in a unit test with no temp directory and no race
// with the real disk.

#ifndef GIFSCYTHE_CORE_COLLISION_H
#define GIFSCYTHE_CORE_COLLISION_H

#include <cstddef>
#include <functional>
#include <string>

namespace gs {

enum class CollisionPolicy { Ask, Rename, Overwrite, Skip };

inline const char* collision_policy_name(CollisionPolicy p) {
  switch (p) {
    case CollisionPolicy::Ask: return "Ask each time";
    case CollisionPolicy::Rename: return "Rename (numbered)";
    case CollisionPolicy::Overwrite: return "Overwrite";
    case CollisionPolicy::Skip: return "Skip";
  }
  return "Ask each time";
}

inline const char* collision_policy_token(CollisionPolicy p) {
  switch (p) {
    case CollisionPolicy::Ask: return "ask";
    case CollisionPolicy::Rename: return "rename";
    case CollisionPolicy::Overwrite: return "overwrite";
    case CollisionPolicy::Skip: return "skip";
  }
  return "ask";
}

// Unknown text means Ask, never Overwrite: a corrupted session file must not
// start destroying files because a word was misspelled.
inline CollisionPolicy collision_policy_from_token(const std::string& t) {
  if (t == "rename") return CollisionPolicy::Rename;
  if (t == "overwrite") return CollisionPolicy::Overwrite;
  if (t == "skip") return CollisionPolicy::Skip;
  return CollisionPolicy::Ask;
}

enum class CollisionAction { Write, Rename, Overwrite, Skip };

inline const char* collision_action_name(CollisionAction a) {
  switch (a) {
    case CollisionAction::Write: return "write";
    case CollisionAction::Rename: return "renamed";
    case CollisionAction::Overwrite: return "overwritten";
    case CollisionAction::Skip: return "skipped, file already exists";
  }
  return "write";
}

struct CollisionDecision {
  CollisionAction action = CollisionAction::Write;
  std::string final_path;  // what will actually be written (empty for Skip)
  std::string reason;      // human wording for the tally/log
};

// How many numbered candidates to try before giving up. A folder with 10k
// same-named results is a bug or a script, not a workflow; refusing loudly at
// the end beats looping forever on a filesystem that keeps saying "exists".
inline constexpr int kMaxRenameAttempts = 10000;

// Insert `_N` before the extension: "go_result.webp" -> "go_result_1.webp".
// A name with no extension gets the suffix at the end. The directory separator
// of the host is respected because this only ever touches the last dot AFTER the
// last slash, so "C:\out\go.gif" and "/a/b/go.gif" both behave.
inline std::string number_name(const std::string& path, int n) {
  const std::size_t slash = path.find_last_of("/\\");
  const std::size_t dot = path.find_last_of('.');
  // A dot only separates an extension when there is at least one NAME character
  // in front of it: ".hidden" is a name, not an extension, and ".gif" would
  // otherwise turn into "_1.gif".
  const std::size_t name_start = (slash == std::string::npos) ? 0 : slash + 1;
  const bool has_ext = dot != std::string::npos && dot > name_start && dot + 1 < path.size();
  const std::string suffix = "_" + std::to_string(n);
  if (!has_ext) return path + suffix;
  return path.substr(0, dot) + suffix + path.substr(dot);
}

// Decide what happens to ONE target.
//
// `exists` is the only filesystem dependency and it is injected. `start_index`
// lets a batch continue numbering (the second collision-free candidate for the
// whole run need not restart at 1).
inline CollisionDecision decide_collision(const std::string& target, CollisionPolicy policy,
                                          const std::function<bool(const std::string&)>& exists,
                                          int start_index = 1) {
  CollisionDecision d;
  const bool taken = exists ? exists(target) : false;
  if (!taken) {
    d.action = CollisionAction::Write;
    d.final_path = target;
    return d;
  }
  switch (policy) {
    case CollisionPolicy::Ask:
      // The caller turns this into a question. Guessing here is exactly how a
      // "just run it" batch eats a file the user forgot about.
      d.action = CollisionAction::Skip;
      d.final_path = target;
      d.reason = "already exists; waiting for the user's choice";
      return d;
    case CollisionPolicy::Overwrite:
      d.action = CollisionAction::Overwrite;
      d.final_path = target;
      d.reason = "already exists; replacing it (the old file stays until the new one verifies)";
      return d;
    case CollisionPolicy::Skip:
      d.action = CollisionAction::Skip;
      d.final_path = target;
      d.reason = "already exists; left untouched";
      return d;
    case CollisionPolicy::Rename:
      break;
  }
  const int first = start_index < 1 ? 1 : start_index;
  for (int n = first; n < first + kMaxRenameAttempts; ++n) {
    const std::string candidate = number_name(target, n);
    if (!exists || !exists(candidate)) {
      d.action = CollisionAction::Rename;
      d.final_path = candidate;
      d.reason = "already exists; writing " + candidate + " instead";
      return d;
    }
  }
  d.action = CollisionAction::Skip;
  d.final_path = target;
  d.reason = "already exists and no free numbered name was found; skipped";
  return d;
}

// ---- keep folder structure -------------------------------------------------
//
// "Keep folder structure" only means something relative to a folder the user
// added, so this returns the path of `input` under `root`, with '/' separators
// and no leading slash. Empty means "not under that root" and the caller then
// keeps the file flat - a file picked from somewhere else must never be written
// to a guessed subfolder.
inline std::string relative_under_root(const std::string& input, const std::string& root) {
  if (input.empty() || root.empty()) return {};
  std::string in = input, rt = root;
  for (char& c : in) if (c == '\\') c = '/';
  for (char& c : rt) if (c == '\\') c = '/';
  while (rt.size() > 1 && rt.back() == '/') rt.pop_back();
  if (in.size() <= rt.size()) return {};
  if (in.compare(0, rt.size(), rt) != 0) return {};
  if (in[rt.size()] != '/') return {};  // "/a/bc" is not under "/a/b"
  std::string rel = in.substr(rt.size() + 1);
  if (rel.empty()) return {};
  return rel;
}

// The directory part of a relative path ("sub/dir/x.gif" -> "sub/dir").
inline std::string relative_dir_part(const std::string& rel) {
  const std::size_t slash = rel.find_last_of('/');
  if (slash == std::string::npos || slash == 0) return {};
  return rel.substr(0, slash);
}

// Join two path pieces with the host separator, without doubling it.
inline std::string join_path(const std::string& a, const std::string& b) {
  if (a.empty()) return b;
  if (b.empty()) return a;
#ifdef _WIN32
  const char sep = '\\';
  const bool a_sep = a.back() == '\\' || a.back() == '/';
#else
  const char sep = '/';
  const bool a_sep = a.back() == '/';
#endif
  return a_sep ? a + b : a + sep + b;
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_COLLISION_H
