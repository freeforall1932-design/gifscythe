// OutputPlan.h - Plan every output of a run BEFORE any process starts, and
// refuse the runs that would silently destroy user data.
//
// Why this file exists (audit U-01, all three independent reviews, fix-order
// step 1): batch auto-naming computed each output one at a time, after the
// previous engine process had already finished, and never compared targets to
// each other or to the inputs. Two reachable consequences, both verified with
// the real engine:
//
//   * template "{name}.gif" with an empty batch folder resolves the output to
//     the INPUT ITSELF -> `gifsicle self.gif -o self.gif` exits 0 and replaces
//     the user's source file in place;
//   * two queued files that share a base name (/d1/hero.gif, /d2/hero.gif)
//     both render <batchdir>/hero_opt.gif, so the second run silently deletes
//     the first result and the UI still reports "complete".
//
// The planner is deliberately Qt-independent and template-agnostic: callers
// render whatever names they like and hand over the (input, output) pairs.
// That keeps the whole decision inside the plain-g++ unit suite and the CLI,
// so it is provable without a desktop toolkit.
//
// Policy (deliberate, and the reason each check exists):
//   * REFUSE a target that is any queued input      -> destroys a source file
//   * REFUSE two inputs mapping to one target       -> loses N-1 results
//   * REFUSE an empty target                        -> result evaporates
//   * REPORT (not refuse) targets already on disk   -> re-running an optimize
//     is normal and expected; the caller surfaces it instead of hiding it.
//
// Not implemented HERE, and why: temp-file + rename lives one layer up, in the
// run path (U-59 / P0-7), not in the planner. Both the CLI and the GUI redirect
// the engine's `-o` operand to a same-directory `<target>.gs-partial`, verify
// that partial, and only then promote it over the target. The planner therefore
// keeps returning the real, user-visible target, and the live command pane
// still shows the command the operator asked for.
//
// CORRECTED 2026-09-27. This paragraph used to argue that temp-file + rename
// was REJECTED, and cited `docs/audit/FIX_PICK_2026-09-10.md §1.4`. That file
// does not exist in this repo (docs/ holds archive/, ci/, legal/, planning/,
// release/, screenshots/ — no audit/), and the opposite of what it argued has
// since shipped as U-59. The stale wording read as settled policy in the file
// a reader opens first; it is now the truth.
//
// Known hole, tracked as N-10: Explode has NO partial guard. Frames are written
// as <prefix>.NNN and gifsicle opens each with truncating semantics
// (fopen(..., "wb")), so re-running or cancelling an Explode can leave a
// half-written frame over the previous good frame set.

#ifndef GIFSCYTHE_CORE_OUTPUT_PLAN_H
#define GIFSCYTHE_CORE_OUTPUT_PLAN_H

#include <algorithm>
#include <cctype>
#include <cstdint>
#include "WinUnicode.h"
#include <filesystem>
#include <map>
#include <string>
#include <system_error>
#include <vector>

namespace gs {
namespace fs = std::filesystem;

// Which case rule a key uses (U-55 / P1-35, S37).
//
// WHY this is a parameter and not an #ifdef: the finding was that the Windows
// key folded ASCII only, so `Ä.gif` and `ä.gif` produced DIFFERENT keys and the
// duplicate check waved a pair through that Windows then wrote to one file —
// reopening U-01. The old code could not be tested here because the rule only
// existed inside `#ifdef _WIN32`; CI's Linux job is where the tests run, so the
// rule is now an injectable policy and the POSIX build tests the Windows rule
// with PathKeyPolicy::Windows.
//
// The fold itself is a documented PARTIAL Unicode fold (see case_fold_utf8):
// full Unicode case folding needs ICU or CompareStringOrdinal, neither of which
// this header may depend on. The ranges it covers are exactly the ones the
// finding named plus their neighbours, and the residual is stated at the
// function.
enum class PathKeyPolicy {
  Posix,    // case-sensitive; '\' is an ordinary filename character
  Windows   // case-insensitive; '\' and '/' name the same separator
};

inline PathKeyPolicy host_path_key_policy() {
#ifdef _WIN32
  return PathKeyPolicy::Windows;
#else
  return PathKeyPolicy::Posix;
#endif
}

// Unicode simple case fold for UTF-8, covering the ranges this codebase can
// verify without ICU:
//   * ASCII A-Z                      -> a-z (unchanged from the old behaviour)
//   * Latin-1 Supplement  À..Þ (minus ×) -> à..þ
//   * Latin Extended-A    0x0100..0x017F: most upper-case code points are EVEN,
//                         so an even code point folds to itself+1 (verified
//                         per range: this holds for the letters, and the two
//                         odd/odd exceptions are special-cased below)
//   * Greek               0x0391..0x03A9 -> +0x20 (final sigma 0x03C2 folds to
//                         0x03C3, the standard simple-fold choice)
//   * Cyrillic            0x0410..0x042F -> +0x20; 0x0400..0x040F -> +0x50
//   * 0x0178 Ÿ -> 0x00FF ÿ, 0x0130 İ -> 0x0069 i, 0x0131 ı -> 0x0131
// RESIDUAL (stated, not hidden): characters outside these ranges are left
// alone, so a host that distinguishes e.g. Greek final sigma in a filename
// still gets a conservative key. Closing the remainder needs ICU's full
// case-folding table or CompareStringOrdinal on Windows; both are bigger than
// this finding, and neither is testable on this host. The rule is deliberately
// CONSERVATIVE: over-folding two DISTINCT files into one key can only cause a
// spurious refusal, never a silent overwrite, which is the failure direction
// U-01/U-55 care about.
inline void fold_code_point_utf8(uint32_t& cp) {
  if (cp < 0x80) {
    if (cp >= 'A' && cp <= 'Z') cp += 0x20;
    return;
  }
  if (cp >= 0x00C0 && cp <= 0x00DE && cp != 0x00D7) { cp += 0x20; return; }
  if (cp >= 0x00C0 && cp <= 0x00DE) return;
  if (cp == 0x0178) { cp = 0x00FF; return; }
  if (cp == 0x0130) { cp = 0x0069; return; }   // İ -> i
  if (cp >= 0x0100 && cp <= 0x0137 && (cp % 2) == 0) { ++cp; return; }
  if (cp >= 0x0139 && cp <= 0x0148 && (cp % 2) == 1) { ++cp; return; }
  if (cp >= 0x014A && cp <= 0x0177 && (cp % 2) == 0) { ++cp; return; }
  if (cp >= 0x0391 && cp <= 0x03A1) { cp += 0x20; return; }
  if (cp >= 0x03A3 && cp <= 0x03AB) { cp += 0x20; return; }
  if (cp == 0x03C2) { cp = 0x03C3; return; }   // final sigma -> sigma
  if (cp >= 0x0410 && cp <= 0x042F) { cp += 0x20; return; }
  if (cp >= 0x0400 && cp <= 0x040F) { cp += 0x50; return; }
  if (cp >= 0x0460 && cp <= 0x0481 && (cp % 2) == 0) { ++cp; return; }
  if (cp >= 0x048A && cp <= 0x04BF && (cp % 2) == 0) { ++cp; return; }
}

// UTF-8 decode -> fold -> UTF-8 encode. Invalid bytes are passed through
// unchanged (one byte at a time): a key must never throw on a filename.
inline std::string case_fold_utf8(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  size_t i = 0;
  const auto put = [&out](uint32_t cp) {
    if (cp < 0x80) { out.push_back(static_cast<char>(cp)); return; }
    if (cp < 0x800) {
      out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
      out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
      return;
    }
    if (cp < 0x10000) {
      out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
      out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
      return;
    }
    out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  };
  while (i < in.size()) {
    const unsigned char b0 = static_cast<unsigned char>(in[i]);
    size_t len = 0;
    uint32_t cp = 0;
    if (b0 < 0x80) { len = 1; cp = b0; }
    else if ((b0 & 0xE0) == 0xC0) { len = 2; cp = b0 & 0x1Fu; }
    else if ((b0 & 0xF0) == 0xE0) { len = 3; cp = b0 & 0x0Fu; }
    else if ((b0 & 0xF8) == 0xF0) { len = 4; cp = b0 & 0x07u; }
    else { out.push_back(in[i++]); continue; }  // stray continuation byte
    if (i + len > in.size()) { out.push_back(in[i++]); continue; }
    bool ok = true;
    for (size_t k = 1; k < len; ++k) {
      const unsigned char bk = static_cast<unsigned char>(in[i + k]);
      if ((bk & 0xC0) != 0x80) { ok = false; break; }
      cp = (cp << 6) | (bk & 0x3Fu);
    }
    if (!ok) { out.push_back(in[i++]); continue; }
    fold_code_point_utf8(cp);
    put(cp);
    i += len;
  }
  return out;
}

// A comparison key for a path: two strings that name the same file on the host
// must produce the same key. Resolved through the filesystem so "./a.gif",
// "a.gif" and "/abs/../abs/a.gif" agree, with a purely lexical fallback when
// the OS call fails. The Windows policy additionally folds case (Unicode, not
// just ASCII — U-55) and separators, because that is how the Win32 file APIs
// treat names.
inline std::string path_key(const std::string& p, PathKeyPolicy policy) {
  if (p.empty()) return std::string();
  std::error_code ec;
  fs::path abs = fs::absolute(u8path_compat(p), ec);
  if (ec) abs = u8path_compat(p);
  fs::path norm = fs::weakly_canonical(abs, ec);
  if (ec) norm = abs;
  std::string s = path_u8string(norm.lexically_normal());
  while (s.size() > 1 && (s.back() == '/' || s.back() == '\\')) s.pop_back();
  if (policy == PathKeyPolicy::Windows) {
    s = case_fold_utf8(s);
    for (auto& c : s) if (c == '\\') c = '/';
  }
  return s;
}

// The platform's own policy — what every caller in the products uses.
inline std::string path_key(const std::string& p) {
  return path_key(p, host_path_key_policy());
}

enum class PlanIssueKind {
  CountMismatch,  // caller passed a different number of inputs and outputs
  EmptyTarget,    // an output path is empty
  TargetsSource,  // an output IS one of the queued inputs
  DuplicateTarget // two inputs would write the same file
};

inline const char* plan_issue_name(PlanIssueKind k) {
  switch (k) {
    case PlanIssueKind::CountMismatch: return "count";
    case PlanIssueKind::EmptyTarget:   return "empty";
    case PlanIssueKind::TargetsSource: return "source";
    case PlanIssueKind::DuplicateTarget: return "duplicate";
  }
  return "unknown";
}

struct PlanIssue {
  PlanIssueKind kind = PlanIssueKind::EmptyTarget;
  size_t index = 0;          // index into the inputs vector this issue belongs to
  std::string input;
  std::string output;
  std::string detail;        // the other path involved, when there is one
  size_t other_index = 0;    // its index (duplicate/source collisions)
};

struct OutputPlan {
  bool ok = true;
  std::vector<PlanIssue> issues;
  std::vector<std::string> outputs;      // valid only when ok
  // Outputs that already exist on disk. NOT a failure — re-running an optimize
  // legitimately replaces the previous result — but the caller must say so
  // rather than let it happen quietly.
  std::vector<std::string> preexisting;

  // One human-readable line per issue, ready for a dialog or stderr.
  std::string describe() const {
    std::string out;
    for (const auto& is : issues) {
      if (!out.empty()) out += "\n";
      switch (is.kind) {
        case PlanIssueKind::CountMismatch:
          out += "Internal error: " + std::to_string(is.index) +
                 " input(s) but " + std::to_string(is.other_index) + " output(s).";
          break;
        case PlanIssueKind::EmptyTarget:
          out += "No output file for \"" + is.input + "\".";
          break;
        case PlanIssueKind::TargetsSource:
          out += "Refusing to overwrite a source file: \"" + is.output +
                 "\" is input #" + std::to_string(is.other_index + 1) +
                 " of this run. Choose a different name template or output folder.";
          break;
        case PlanIssueKind::DuplicateTarget:
          out += "Two inputs would write the same file \"" + is.output +
                 "\" (\"" + is.input + "\" and \"" + is.detail +
                 "\"). Add {name} to the template, or give the files distinct names.";
          break;
      }
    }
    return out;
  }
};

// Plan a whole run. inputs[i] is written to outputs[i].
// Returns ok=false with one issue per problem; nothing is executed by this
// function and no file is touched except to stat() existing targets.
// `policy` exists so tests can plan under the WINDOWS case rule on Linux
// (U-55 / P1-35): products always take the default (the host's own rule).
inline OutputPlan plan_outputs(const std::vector<std::string>& inputs,
                               const std::vector<std::string>& outputs,
                               PathKeyPolicy policy = host_path_key_policy()) {
  OutputPlan plan;
  // Two legal shapes:
  //   batch  — one output per input           (inputs.size() == outputs.size())
  //   merge  — every input welded into ONE    (outputs.size() == 1)
  // Anything else is a caller bug and nothing below would be meaningful.
  const bool one_target = (outputs.size() == 1 && !inputs.empty());
  if (!one_target && inputs.size() != outputs.size()) {
    plan.ok = false;
    PlanIssue is;
    is.kind = PlanIssueKind::CountMismatch;
    is.index = inputs.size();
    is.other_index = outputs.size();
    plan.issues.push_back(is);
    return plan;
  }

  // Every queued input, by key: an output that matches ANY of them destroys a
  // source, not just the one it was paired with (merge writes N inputs into 1
  // output, and a batch template can render another file's name).
  std::map<std::string, size_t> input_keys;
  for (size_t i = 0; i < inputs.size(); ++i) {
    const std::string k = path_key(inputs[i], policy);
    if (!k.empty() && input_keys.find(k) == input_keys.end()) input_keys[k] = i;
  }

  std::map<std::string, size_t> seen;
  for (size_t i = 0; i < outputs.size(); ++i) {
    PlanIssue is;
    is.index = i;
    // In the merge shape there is one target and N sources, so there is no
    // single paired input to name; the offending one is filled in below.
    is.input = i < inputs.size() ? inputs[i] : std::string();
    is.output = outputs[i];

    if (outputs[i].empty()) {
      is.kind = PlanIssueKind::EmptyTarget;
      if (is.input.empty() && !inputs.empty()) is.input = inputs.front();
      plan.ok = false;
      plan.issues.push_back(is);
      continue;
    }

    const std::string k = path_key(outputs[i], policy);

    const auto src = input_keys.find(k);
    if (src != input_keys.end()) {
      is.kind = PlanIssueKind::TargetsSource;
      is.other_index = src->second;
      is.input = inputs[src->second];
      plan.ok = false;
      plan.issues.push_back(is);
      continue;  // one issue per output is enough
    }

    const auto dup = seen.find(k);
    if (dup != seen.end()) {
      is.kind = PlanIssueKind::DuplicateTarget;
      is.detail = dup->second < inputs.size() ? inputs[dup->second] : std::string();
      is.other_index = dup->second;
      plan.ok = false;
      plan.issues.push_back(is);
      continue;
    }
    seen[k] = i;

    std::error_code ec;
    if (fs::exists(u8path_compat(outputs[i]), ec) && !ec) plan.preexisting.push_back(outputs[i]);
  }

  plan.outputs = outputs;
  return plan;
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_OUTPUT_PLAN_H
