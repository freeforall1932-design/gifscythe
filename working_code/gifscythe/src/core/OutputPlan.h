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
// Not implemented here, and why: temp-file + rename. It would only protect a
// PREVIOUS output from a crashed engine, and it perturbs the live command pane
// contract (the pane must show the command that actually runs). The two
// vectors that actually destroy data are closed by planning. See
// docs/audit/FIX_PICK_2026-09-10.md §1.4.

#ifndef GIFSCYTHE_CORE_OUTPUT_PLAN_H
#define GIFSCYTHE_CORE_OUTPUT_PLAN_H

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <string>
#include <system_error>
#include <vector>

namespace gs {
namespace fs = std::filesystem;

// A comparison key for a path: two strings that name the same file on the host
// must produce the same key. Resolved through the filesystem so "./a.gif",
// "a.gif" and "/abs/../abs/a.gif" agree, with a purely lexical fallback when
// the OS call fails. Windows additionally folds case and separators, because
// that is how the Win32 file APIs treat names.
inline std::string path_key(const std::string& p) {
  if (p.empty()) return std::string();
  std::error_code ec;
  fs::path abs = fs::absolute(p, ec);
  if (ec) abs = fs::path(p);
  fs::path norm = fs::weakly_canonical(abs, ec);
  if (ec) norm = abs;
  std::string s = norm.lexically_normal().string();
  while (s.size() > 1 && (s.back() == '/' || s.back() == '\\')) s.pop_back();
#ifdef _WIN32
  for (auto& c : s) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (c == '\\') c = '/';
  }
#else
  (void)0;  // POSIX: case-sensitive, '\' is an ordinary filename character
#endif
  return s;
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
inline OutputPlan plan_outputs(const std::vector<std::string>& inputs,
                               const std::vector<std::string>& outputs) {
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
    const std::string k = path_key(inputs[i]);
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

    const std::string k = path_key(outputs[i]);

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
    if (fs::exists(fs::path(outputs[i]), ec) && !ec) plan.preexisting.push_back(outputs[i]);
  }

  plan.outputs = outputs;
  return plan;
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_OUTPUT_PLAN_H
