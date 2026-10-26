// test_ffmpeg_command.cpp - the encoder contract, pinned without an encoder.
//
// Three things are tested here, and all three are the kind that go wrong quietly:
//
//   1. the ARGV        - the muxer is pinned with -f (the staging file is named
//                        "...part-3f2a", so an extension-inferred muxer would be
//                        a coin flip), the codec matches the label, H.265 is
//                        tagged hvc1, H.264 scales to even dimensions, and a
//                        refused request builds NO command instead of half of one;
//   2. the STAGING NAME - a real extension, never doubled;
//   3. the POSTCONDITION - a file is only accepted when its bytes are the
//                        container that was promised. A still PNG under "APNG", a
//                        still WebP under "animated WebP", and an HEVC file under
//                        "H.264" are the silent no-ops the owner forbade.
//
// Every header fixture below is a real file written to a temp directory and read
// back through gs::verify_output_as(), so the verifier is exercised through the
// same code path the GUI uses.

#include "../src/core/FfmpegCommand.h"
#include "../src/core/OutputVerify.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
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

bool has(const std::vector<std::string>& v, const std::string& needle) {
  for (const auto& s : v) if (s == needle) return true;
  return false;
}

// The value that follows the FIRST occurrence of `flag`, or "<absent>".
std::string value_of(const std::vector<std::string>& v, const std::string& flag) {
  for (std::size_t i = 0; i + 1 < v.size(); ++i)
    if (v[i] == flag) return v[i + 1];
  return "<absent>";
}

std::string joined(const std::vector<std::string>& v) {
  std::string s;
  for (const auto& a : v) { s += a; s += ' '; }
  return s;
}

FfmpegEncodeRequest req(OutputFormat f) {
  FfmpegEncodeRequest r;
  r.input = "/in/go.gif";
  r.partial = "/out/go_result.webp";
  r.format = f;
  return r;
}

std::string write_bytes(const fs::path& dir, const std::string& name,
                        const std::vector<unsigned char>& bytes) {
  const fs::path p = dir / name;
  std::ofstream(p, std::ios::binary).write(reinterpret_cast<const char*>(bytes.data()),
                                           static_cast<std::streamsize>(bytes.size()));
  return p.string();
}

std::vector<unsigned char> bytes_of(const std::string& text) {
  return std::vector<unsigned char>(text.begin(), text.end());
}

// --- container headers as the encoders actually write them (trimmed) --------
std::vector<unsigned char> gif_header()      { auto b = bytes_of("GIF89a"); b.push_back(0x10); b.push_back(0x00); return b; }
std::vector<unsigned char> png_header(bool animated) {
  std::vector<unsigned char> b = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
  const std::string ihdr = std::string("\x00\x00\x00\x0d", 4) + "IHDR";
  b.insert(b.end(), ihdr.begin(), ihdr.end());
  if (animated) {
    const std::string actl = std::string("\x00\x00\x00\x08", 4) + "acTL" + "\x00\x00\x00\x02";
    b.insert(b.end(), actl.begin(), actl.end());
  }
  return b;
}
std::vector<unsigned char> webp_header(bool animated) {
  std::vector<unsigned char> b = bytes_of("RIFF");
  const std::string size = std::string("\x40\x00\x00\x00", 4);
  b.insert(b.end(), size.begin(), size.end());
  auto webp = bytes_of("WEBP");
  b.insert(b.end(), webp.begin(), webp.end());
  if (animated) {
    auto vp8x = bytes_of("VP8X");
    b.insert(b.end(), vp8x.begin(), vp8x.end());
    b.insert(b.end(), 10, 0);
    auto anim = bytes_of("ANIM");
    b.insert(b.end(), anim.begin(), anim.end());
    b.insert(b.end(), 8, 0);
    auto anmf = bytes_of("ANMF");
    b.insert(b.end(), anmf.begin(), anmf.end());
  } else {
    auto vp8l = bytes_of("VP8L");
    b.insert(b.end(), vp8l.begin(), vp8l.end());
  }
  return b;
}
std::vector<unsigned char> mp4_header(bool hevc) {
  std::vector<unsigned char> b = {0x00, 0x00, 0x00, 0x18};
  auto ftyp = bytes_of("ftyp");
  b.insert(b.end(), ftyp.begin(), ftyp.end());
  auto brand = bytes_of(hevc ? "iso5" : "isom");
  b.insert(b.end(), brand.begin(), brand.end());
  b.insert(b.end(), 4, 0);
  // Compatible brands, then a sample entry description as it appears in moov.
  auto compat = bytes_of(hevc ? "iso5iso6hvc1mp41" : "isomiso2avc1mp41");
  b.insert(b.end(), compat.begin(), compat.end());
  return b;
}
std::vector<unsigned char> webm_header(bool av1) {
  std::vector<unsigned char> b = {0x1A, 0x45, 0xDF, 0xA3};
  auto doctype = bytes_of("webm");
  b.insert(b.end(), doctype.begin(), doctype.end());
  b.insert(b.end(), 6, 0);
  auto codec = bytes_of(av1 ? "V_AV1AV01" : "V_VP9\x00\x00\x00");
  b.insert(b.end(), codec.begin(), codec.end());
  return b;
}

}  // namespace

int main() {
  const fs::path dir = fs::temp_directory_path() / "gs_ffmpeg_command_test";
  fs::remove_all(dir);
  fs::create_directories(dir);

  // ------------------------------------------------------------------ argv --
  {
    // GIF is gifsicle's job. Handing it to FFmpeg would quietly lose every
    // gifsicle option the Actions tab exposes.
    const auto r = req(OutputFormat::Gif);
    CHECK(!ffmpeg_request_refusal(r).empty());
    CHECK(ffmpeg_encode_args(r).empty());
  }
  {
    const auto args = ffmpeg_encode_args(req(OutputFormat::Apng));
    CHECK(has(args, "-f"));
    CHECK_MSG(value_of(args, "-f") == "apng", joined(args));
    CHECK_MSG(value_of(args, "-c:v") == "apng", joined(args));
    CHECK_MSG(value_of(args, "-plays") == "0", "APNG must loop forever by default");
    CHECK_MSG(value_of(args, "-pix_fmt") == "rgba", "APNG needs alpha or the edges go black");
    CHECK(args.back() == "/out/go_result.webp");  // staging path, not a name it invents
    CHECK(!has(args, "-an"));                     // not a video container; -an is noise
  }
  {
    const auto args = ffmpeg_encode_args(req(OutputFormat::Webp));
    // libwebp (still) instead of libwebp_anim (moving) is THE classic bug: it
    // succeeds, writes one frame, and every later check still passes.
    CHECK_MSG(value_of(args, "-c:v") == "libwebp_anim", joined(args));
    CHECK_MSG(has(args, "-loop"), "animated WebP needs an explicit loop count");
    CHECK_MSG(value_of(args, "-f") == "webp", joined(args));
    CHECK_MSG(value_of(args, "-pix_fmt") == "yuva420p", joined(args));
  }
  {
    const auto args = ffmpeg_encode_args(req(OutputFormat::Mp4H264));
    CHECK_MSG(value_of(args, "-c:v") == "libx264", joined(args));
    CHECK_MSG(value_of(args, "-f") == "mp4", joined(args));
    CHECK_MSG(value_of(args, "-movflags") == "+faststart", "a shareable MP4 plays before it downloads");
    CHECK_MSG(has(args, "-an"), "no audio track: the source was a GIF");
    bool even = false;
    for (const auto& a : args) if (a.find("trunc(iw/2)*2") != std::string::npos) even = true;
    CHECK_MSG(even, "odd GIF widths make libx264 fail; the scale filter prevents that");
    CHECK(!has(args, "-tag:v"));  // hvc1 is HEVC-only
  }
  {
    const auto args = ffmpeg_encode_args(req(OutputFormat::Mp4H265));
    CHECK_MSG(value_of(args, "-c:v") == "libx265", joined(args));
    CHECK_MSG(value_of(args, "-tag:v") == "hvc1", "hev1 does not play on Apple devices");
    CHECK_MSG(value_of(args, "-f") == "mp4", joined(args));
  }
  {
    const auto vp9 = ffmpeg_encode_args(req(OutputFormat::WebmVp9));
    CHECK_MSG(value_of(vp9, "-c:v") == "libvpx-vp9", joined(vp9));
    CHECK_MSG(value_of(vp9, "-f") == "webm", joined(vp9));
    CHECK_MSG(value_of(vp9, "-b:v") == "0", "CRF mode needs -b:v 0 or VP9 refuses the quality");
    const auto av1 = ffmpeg_encode_args(req(OutputFormat::WebmAv1));
    CHECK_MSG(value_of(av1, "-c:v") == "libaom-av1", joined(av1));
    CHECK_MSG(value_of(av1, "-cpu-used") == "6", "AV1 at default speed looks like a hang");
  }
  {
    // Quality and fps are only emitted where asked; the default request must not
    // smuggle a frame rate in and re-time the animation.
    const auto args = ffmpeg_encode_args(req(OutputFormat::Mp4H264));
    CHECK(!has(args, "-r"));
    FfmpegEncodeRequest r = req(OutputFormat::Mp4H264);
    r.fps = 12;
    CHECK_MSG(value_of(ffmpeg_encode_args(r), "-r") == "12", "an explicit fps must reach ffmpeg");
    FfmpegEncodeRequest bad = req(OutputFormat::Mp4H264);
    bad.fps = 0;
    CHECK(!ffmpeg_request_refusal(bad).empty());
    bad.fps = -1;
    bad.quality = 101;
    CHECK(!ffmpeg_request_refusal(bad).empty());
  }
  {
    // Every FFmpeg command must end with the staging path, and the staging path
    // must be the LAST argument: an option appended after it is ignored by
    // FFmpeg, which is how "-an" silently stops applying.
    for (OutputFormat f : {OutputFormat::Apng, OutputFormat::Webp, OutputFormat::Mp4H264,
                           OutputFormat::Mp4H265, OutputFormat::WebmVp9, OutputFormat::WebmAv1}) {
      const auto args = ffmpeg_encode_args(req(f));
      CHECK_MSG(!args.empty(), output_format_name(f));
      CHECK_MSG(args.back() == "/out/go_result.webp", output_format_name(f));
      CHECK_MSG(args[args.size() - 2] == ffmpeg_muxer(f), output_format_name(f));
    }
  }

  // ------------------------------------------------------------ staging name --
  {
    CHECK_MSG(ffmpeg_staging_name("go_result.gif", OutputFormat::Webp) == "go_result.webp",
              "the staging name is where the encoder writes; it needs a real extension");
    CHECK_MSG(ffmpeg_staging_name("go_result", OutputFormat::Mp4H264) == "go_result.mp4", "");
    CHECK_MSG(ffmpeg_staging_name("/a/b/go.gif.part-7", OutputFormat::Apng) ==
                  "/a/b/go.gif.part-7.png",
              "a staging marker is not a media extension: stripping it would collide two runs");
    CHECK_MSG(ffmpeg_staging_name("/a/b/go.v2.gif", OutputFormat::Apng) == "/a/b/go.v2.png",
              "dotted stems survive");
    CHECK_MSG(ffmpeg_staging_name("C:\\out\\go.gif", OutputFormat::Mp4H265) ==
                  "C:\\out\\go.mp4",
              "a Windows path must not be treated as a dotted filename");
    CHECK_MSG(ffmpeg_staging_name("go.gif", OutputFormat::Gif) == "go.gif",
              "GIF keeps the caller's path");
  }

  // ----------------------------------------------------------- postcondition --
  {
    // Happy paths: each advertised container is accepted.
    struct Case { OutputFormat f; std::vector<unsigned char> bytes; const char* file; };
    const Case cases[] = {
        {OutputFormat::Gif, gif_header(), "ok.gif"},
        {OutputFormat::Apng, png_header(true), "ok.png"},
        {OutputFormat::Webp, webp_header(true), "ok.webp"},
        {OutputFormat::Mp4H264, mp4_header(false), "ok264.mp4"},
        {OutputFormat::Mp4H265, mp4_header(true), "ok265.mp4"},
        {OutputFormat::WebmVp9, webm_header(false), "okvp9.webm"},
        {OutputFormat::WebmAv1, webm_header(true), "okav1.webm"},
    };
    for (const Case& c : cases) {
      // Snapshot BEFORE the write, exactly as a run does: the verifier's job is
      // to prove something NEW appeared, so a pre-existing copy cannot pass.
      const std::string path = (dir / c.file).string();
      const auto before = snapshot_output(path);
      write_bytes(dir, c.file, c.bytes);
      CHECK_MSG(verify_output_as(path, before, c.f).empty(),
                std::string(c.file) + ": " + verify_output_as(path, before, c.f));
    }
  }
  {
    // The silent no-ops. Each one used to pass a "file exists and is non-empty"
    // check, which is exactly why the format-aware verifier exists.
    auto wrote = [&](const std::string& name, const std::vector<unsigned char>& bytes) {
      const std::string path = (dir / name).string();
      write_bytes(dir, name, bytes);
      return path;
    };
    const std::string still_png = wrote("still.png", png_header(false));
    const std::string still_webp = wrote("still.webp", webp_header(false));
    const std::string hevc = wrote("hevc.mp4", mp4_header(true));
    const std::string h264 = wrote("h264.mp4", mp4_header(false));
    const std::string moving_gif = wrote("mislabeled.mp4", gif_header());
    const std::string vp9 = wrote("vp9.webm", webm_header(false));
    const std::string empty_file = wrote("empty.webp", {});

    CHECK_MSG(!verify_output_as(still_png, OutputSnapshot{}, OutputFormat::Apng).empty(),
              "a still PNG under APNG must be refused");
    CHECK_MSG(!verify_output_as(still_webp, OutputSnapshot{}, OutputFormat::Webp).empty(),
              "a still WebP under animated WebP must be refused");
    CHECK_MSG(!verify_output_as(hevc, OutputSnapshot{}, OutputFormat::Mp4H264).empty(),
              "HEVC under an H.264 label must be refused");
    CHECK_MSG(verify_output_as(hevc, OutputSnapshot{}, OutputFormat::Mp4H265).empty(),
              "H.265 is what the file actually is");
    CHECK_MSG(verify_output_as(h264, OutputSnapshot{}, OutputFormat::Mp4H264).empty(),
              "H.264 is what the file actually is");
    CHECK_MSG(!verify_output_as(moving_gif, OutputSnapshot{}, OutputFormat::Mp4H264).empty(),
              "a GIF renamed .mp4 must be refused");
    CHECK_MSG(!verify_output_as(vp9, OutputSnapshot{}, OutputFormat::WebmAv1).empty(),
              "VP9 under an AV1 label must be refused");
    CHECK_MSG(!verify_output_as(h264, OutputSnapshot{}, OutputFormat::WebmVp9).empty(),
              "an MP4 under a WebM label must be refused");
    CHECK_MSG(verify_output_as(dir.string() + "/does-not-exist.webp",
                               OutputSnapshot{}, OutputFormat::Webp).find("missing or empty") !=
                  std::string::npos,
              "a missing output must name the real problem");
    // A zero-byte file is caught before any probe: same message as before.
    CHECK_MSG(!verify_output_as(empty_file, OutputSnapshot{}, OutputFormat::Webp).empty(),
              "an empty result is never a success");
    // And the old GIF-only entry point still behaves (CLI compatibility).
    CHECK(verify_output(moving_gif, OutputSnapshot{}).empty());

    // "Unchanged since the pre-run snapshot" must still fire: re-running onto the
    // same bytes is not proof that this run produced anything.
    const auto same = snapshot_output(h264);
    CHECK_MSG(!verify_output_as(h264, same, OutputFormat::Mp4H264).empty(),
              "an untouched pre-existing file must not pass as this run's result");
  }

  // ---------------------------------------------------------------- summary --
  fs::remove_all(dir);
  std::printf("==> %d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
