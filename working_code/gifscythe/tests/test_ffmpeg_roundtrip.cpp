// test_ffmpeg_roundtrip.cpp - prove the encoder commands by RUNNING them.
//
// test_ffmpeg_command.cpp proves what the argv SAYS. This proves it WORKS: it
// finds a real ffmpeg (the same locator the app uses, so GS_FFMPEG/PATH/the
// bundled copy are all honoured), asks it for one GIF, then encodes that GIF into
// every advertised format with exactly the arguments the GUI will pass, and
// checks each result with the same postcondition the run path uses.
//
// WHY IT SKIPS INSTEAD OF FAILING
//   A checkout with no FFmpeg must still build and test (GIF-only is a supported
//   state, and that is the state CI runs in). "SKIP" here is an explicit,
//   printed fact - not a silent pass - and it exits 0 so the build stays green.
//   Run it on a machine with FFmpeg (or set GS_FFMPEG) to get the real proof.
//
// POSIX ONLY: it shells out with std::system, and the quoting rules for that are
// the shell's. Windows CI skips the file with a printed reason rather than
// pretending to have tested it.
#include "../src/core/FfmpegCommand.h"
#include "../src/core/FfmpegLocator.h"
#include "../src/core/OutputVerify.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

using namespace gs;
namespace fs = std::filesystem;

static int failures = 0;
static int checks = 0;
#define CHECK(cond) do { ++checks; if (!(cond)) { \
  std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++failures; } } while (0)
#define CHECK_MSG(cond, msg) do { ++checks; if (!(cond)) { \
  std::printf("FAIL: %s (line %d)\n  %s\n", #cond, __LINE__, std::string(msg).c_str()); ++failures; } } while (0)

namespace {

std::string quoted(const std::string& s) {
  std::string out = "'";
  for (char c : s) {
    if (c == '\'') out += "'\\''";
    else out += c;
  }
  out += "'";
  return out;
}

// Run `program args...`, returning true only for a clean exit.
bool run(const std::string& program, const std::vector<std::string>& args,
         const std::string& logPath) {
  std::string cmd = quoted(program);
  for (const auto& a : args) cmd += " " + quoted(a);
  cmd += " > " + quoted(logPath) + " 2>&1";
  const int rc = std::system(cmd.c_str());
  return rc == 0;
}

std::string tail_of(const std::string& path, std::size_t maxBytes) {
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return "(no log)";
  std::fseek(f, 0, SEEK_END);
  const long size = std::ftell(f);
  const long start = size > static_cast<long>(maxBytes) ? size - static_cast<long>(maxBytes) : 0;
  std::fseek(f, start, SEEK_SET);
  std::string out;
  out.resize(static_cast<std::size_t>(size - start));
  const std::size_t got = std::fread(&out[0], 1, out.size(), f);
  out.resize(got);
  std::fclose(f);
  return out;
}

}  // namespace

int main() {
#ifdef _WIN32
  std::printf("==> SKIP: this round-trip test shells out with POSIX quoting\n");
  std::printf("==> 0 checks, 0 failures\n");
  return 0;
#else
  const FfmpegLocation loc = locate_ffmpeg("");
  if (!loc.found()) {
    std::printf("==> SKIP: no FFmpeg found (%s)\n",
                ffmpeg_location_summary(loc).c_str());
    std::printf("==> GIF-only build: the other formats are disabled, which is honest, so nothing to prove here\n");
    std::printf("==> 0 checks, 0 failures\n");
    return 0;
  }
  std::printf("==> FFmpeg under test: %s\n", ffmpeg_location_summary(loc).c_str());

  const fs::path dir = fs::temp_directory_path() / "gs_ffmpeg_roundtrip";
  fs::remove_all(dir);
  fs::create_directories(dir);

  // A deliberately ODD-width source: 65x49 is what makes libx264 fail without
  // the even-dimension scale filter, so the filter is exercised, not assumed.
  const std::string source = (dir / "source.gif").string();
  const bool made = run(loc.path,
                        {"-hide_banner", "-loglevel", "error", "-f", "lavfi", "-i",
                         "testsrc=size=65x49:rate=10", "-t", "0.6", "-y", source},
                        (dir / "make-source.log").string());
  CHECK_MSG(made, "could not synthesise a test GIF: " + tail_of((dir / "make-source.log").string(), 800));
  if (!made) {
    std::printf("==> 2 checks, %d failures\n", failures);
    return 1;
  }
  CHECK_MSG(verify_output_as(source, OutputSnapshot{}, OutputFormat::Gif).empty(),
            "the synthesised source is not a GIF: " +
                verify_output_as(source, OutputSnapshot{}, OutputFormat::Gif));

  struct Case {
    OutputFormat format;
    const char* file;
  };
  const Case cases[] = {
      {OutputFormat::Apng, "out.apng.png"},
      {OutputFormat::Webp, "out.webp"},
      {OutputFormat::Mp4H264, "out.h264.mp4"},
      {OutputFormat::Mp4H265, "out.h265.mp4"},
      {OutputFormat::WebmVp9, "out.vp9.webm"},
      {OutputFormat::WebmAv1, "out.av1.webm"},
  };

  for (const Case& c : cases) {
    const std::string out = (dir / c.file).string();
    // Snapshot BEFORE the encode, exactly like a run: "something new appeared"
    // is part of the postcondition.
    const OutputSnapshot before = snapshot_output(out);
    FfmpegEncodeRequest request;
    request.input = source;
    request.partial = out;
    request.format = c.format;
    const std::vector<std::string> args = ffmpeg_encode_args(request);
    CHECK_MSG(!args.empty(), std::string("no argv for ") + output_format_name(c.format));
    const std::string log = out + ".log";
    const bool ok = run(loc.path, args, log);
    const std::string verdict = verify_output_as(out, before, c.format);
    // Both halves matter: a clean exit with a bad file is the silent no-op, and
    // a non-zero exit with a good file still means something went wrong.
    CHECK_MSG(ok && verdict.empty(),
              std::string(output_format_name(c.format)) + ": exit " + (ok ? "0" : "non-zero") +
                  (verdict.empty() ? "" : ", verification: " + verdict) + "\n  " +
                  tail_of(log, 600));
    if (ok && verdict.empty()) {
      std::printf("    ok  %-16s %s (%lld bytes)\n", output_format_name(c.format), c.file,
                  static_cast<long long>(fs::file_size(out)));
    }
  }

  // A negative control: the verifier must reject a file that is the WRONG
  // container for the requested format. Reuse the source GIF under an mp4 name.
  const std::string liar = (dir / "liar.mp4").string();
  fs::copy_file(source, liar, fs::copy_options::overwrite_existing);
  CHECK_MSG(!verify_output_as(liar, OutputSnapshot{}, OutputFormat::Mp4H264).empty(),
            "the verifier accepted a GIF renamed .mp4");

  fs::remove_all(dir);
  std::printf("==> %d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
#endif
}
