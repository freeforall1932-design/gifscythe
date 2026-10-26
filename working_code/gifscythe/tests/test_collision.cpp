// test_collision.cpp - the "already exists" rules, proven with no filesystem.
//
// The owner's answer set the default (ask), the alternatives (rename/overwrite/
// skip) and the tally wording ("skipped, file already exists" + the location).
// Every one of those is a promise about NOT destroying a file, so each is pinned
// here where the decision is a pure function and a fake `exists` map can be made
// to lie in the exact ways a real disk does (dense numbering, dotted names,
// Windows-style paths, a folder that always says "taken").

#include "../src/core/Collision.h"

#include <cstdio>
#include <set>
#include <string>

using namespace gs;

static int failures = 0;
static int checks = 0;
#define CHECK(cond) do { ++checks; if (!(cond)) { \
  std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++failures; } } while (0)
#define CHECK_MSG(cond, msg) do { ++checks; if (!(cond)) { \
  std::printf("FAIL: %s (line %d)\n  %s\n", #cond, __LINE__, std::string(msg).c_str()); ++failures; } } while (0)

namespace {

// A fake disk: exactly the paths in the set exist.
std::function<bool(const std::string&)> disk_of(const std::set<std::string>& taken) {
  return [taken](const std::string& p) { return taken.count(p) != 0; };
}

bool every_path_taken(const std::string&) { return true; }

}  // namespace

int main() {
  // ---------------------------------------------------------------- naming --
  {
    CHECK(number_name("go_result.webp", 1) == "go_result_1.webp");
    CHECK(number_name("go_result.webp", 12) == "go_result_12.webp");
    CHECK(number_name("/a/b/go.gif", 2) == "/a/b/go_2.gif");
    CHECK_MSG(number_name("C:\\out\\go.mp4", 3) == "C:\\out\\go_3.mp4",
              "a Windows path must not be split at the drive colon");
    CHECK_MSG(number_name("noext", 4) == "noext_4", "no extension: the suffix goes at the end");
    CHECK_MSG(number_name(".hidden", 1) == ".hidden_1",
              "a dotfile has no extension to insert before");
  }

  // ------------------------------------------------------------- no clash --
  {
    const auto d = decide_collision("/out/go.webp", CollisionPolicy::Ask, disk_of({}));
    CHECK(d.action == CollisionAction::Write);
    CHECK(d.final_path == "/out/go.webp");
    CHECK(d.reason.empty());
  }

  // ---------------------------------------------------------------- rename --
  {
    const auto d = decide_collision("/out/go.webp", CollisionPolicy::Rename,
                                    disk_of({"/out/go.webp"}), 1);
    CHECK(d.action == CollisionAction::Rename);
    CHECK(d.final_path == "/out/go_1.webp");
    CHECK_MSG(d.reason.find("go_1.webp") != std::string::npos, "the tally must name the new file");
  }
  {
    // Dense numbering: 1 and 2 exist, so the answer is 3 - and the search must
    // not stop at the first miss.
    const auto d = decide_collision("/out/go.webp", CollisionPolicy::Rename,
                                    disk_of({"/out/go.webp", "/out/go_1.webp", "/out/go_2.webp"}));
    CHECK_MSG(d.final_path == "/out/go_3.webp", d.final_path);
  }
  {
    // A batch continues numbering from where the previous file ended.
    const auto d = decide_collision("/out/go.webp", CollisionPolicy::Rename,
                                    disk_of({"/out/go.webp", "/out/go_1.webp"}), 2);
    CHECK(d.final_path == "/out/go_2.webp");
  }
  {
    // Padding in the source name survives: "go_007" + "_1" is not "go_8".
    const auto d = decide_collision("/out/go_007.webp", CollisionPolicy::Rename,
                                    disk_of({"/out/go_007.webp"}));
    CHECK_MSG(d.final_path == "/out/go_007_1.webp", d.final_path);
  }
  {
    // A pathologically full folder must STOP, not spin: the answer is Skip with
    // a reason, because a wrong answer that never returns is worse than a skip.
    const auto d = decide_collision("/out/go.webp", CollisionPolicy::Rename, every_path_taken);
    CHECK(d.action == CollisionAction::Skip);
    CHECK_MSG(d.reason.find("no free numbered name") != std::string::npos, d.reason);
  }

  // ------------------------------------------------------------- overwrite --
  {
    const auto d = decide_collision("/out/go.webp", CollisionPolicy::Overwrite,
                                    disk_of({"/out/go.webp"}));
    CHECK(d.action == CollisionAction::Overwrite);
    CHECK_MSG(d.final_path == "/out/go.webp", "overwrite keeps the destination");
    CHECK_MSG(d.reason.find("until the new one verifies") != std::string::npos,
              "the promise is replace-on-success, not shred-then-fail");
  }

  // ------------------------------------------------------------------ skip --
  {
    const auto d = decide_collision("/out/go.webp", CollisionPolicy::Skip,
                                    disk_of({"/out/go.webp"}));
    CHECK(d.action == CollisionAction::Skip);
    CHECK_MSG(std::string(collision_action_name(d.action)) == "skipped, file already exists",
              collision_action_name(d.action));
    CHECK_MSG(d.reason.find("left untouched") != std::string::npos, d.reason);
  }

  // ------------------------------------------------------------------- ask --
  {
    // Ask NEVER decides to destroy something on its own: the fallback is Skip,
    // and the reason says it is waiting, so a caller that forgot to ask leaves
    // the file alone rather than overwriting it.
    const auto d = decide_collision("/out/go.webp", CollisionPolicy::Ask,
                                    disk_of({"/out/go.webp"}));
    CHECK(d.action == CollisionAction::Skip);
    CHECK_MSG(d.reason.find("waiting for the user") != std::string::npos, d.reason);
  }
  {
    // ... and with nothing there, Ask does not ask. A dialog for every file in a
    // 500-file batch would be its own bug.
    const auto d = decide_collision("/out/go.webp", CollisionPolicy::Ask, disk_of({}));
    CHECK(d.action == CollisionAction::Write);
  }

  // --------------------------------------------------------------- tokens --
  {
    CHECK(std::string(collision_policy_token(CollisionPolicy::Ask)) == "ask");
    CHECK(collision_policy_from_token("rename") == CollisionPolicy::Rename);
    CHECK(collision_policy_from_token("overwrite") == CollisionPolicy::Overwrite);
    CHECK(collision_policy_from_token("skip") == CollisionPolicy::Skip);
    CHECK_MSG(collision_policy_from_token("") == CollisionPolicy::Ask, "missing key = ask");
    CHECK_MSG(collision_policy_from_token("banana") == CollisionPolicy::Ask,
              "a corrupt session must never resolve to Overwrite");
    CHECK(std::string(collision_policy_name(CollisionPolicy::Rename)) == "Rename (numbered)");
  }

  // ---------------------------------------------------- keep folder structure --
  {
    CHECK_MSG(relative_under_root("/in/sub/dir/a.gif", "/in") == "sub/dir/a.gif", "nested");
    CHECK(relative_under_root("/in/a.gif", "/in/") == "a.gif");  // trailing slash
    CHECK_MSG(relative_under_root("/in/abc/a.gif", "/in/ab").empty(),
              "\"/in/abc\" is not under \"/in/ab\" - a prefix is not a folder");
    CHECK_MSG(relative_under_root("/other/a.gif", "/in").empty(), "outside the root");
    CHECK_MSG(relative_under_root("/in", "/in").empty(), "the root itself has no relative path");
    CHECK_MSG(relative_under_root("C:\\in\\sub\\a.gif", "C:\\in") == "sub/a.gif",
              "Windows separators normalise to '/'");
    CHECK(relative_dir_part("sub/dir/a.gif") == "sub/dir");
    CHECK(relative_dir_part("a.gif").empty());
    CHECK_MSG(join_path("/out", "sub/dir") == "/out/sub/dir", "join");
    CHECK_MSG(join_path("/out/", "sub") == "/out/sub", "join without doubling the separator");
    CHECK(join_path("", "sub") == "sub");
    CHECK(join_path("/out", "") == "/out");
  }

  // ------------------------------------------------------------- summary --
  std::printf("==> %d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
