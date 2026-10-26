// N-62 step 3: the argv that turns a moving picture into another moving
// picture, for every format except GIF (gifsicle keeps that job - it is better
// at it than FFmpeg and it is already bundled).
//
// WHY THIS IS A SEPARATE, QT-FREE FILE
//   An encoder command is a contract: the arguments decide whether the result is
//   animated, whether it plays everywhere, and whether it is even the codec the
//   label promised. The GUI should not be the only place that knows it - that is
//   how a "WebP" button ends up writing a still image. So the argv is data,
//   built here, and tested (tests/test_ffmpeg_command.cpp) without Qt, without
//   FFmpeg, and without a screen.
//
// THE STAGING PATH IS THE TRAP
//   Runs write to a claimed partial (`go_1.webp.part-3f2a`) and only promote it
//   onto the real name after verification, so that a cancel cannot shred the
//   user's last good file. FFmpeg picks its muxer from the output EXTENSION -
//   and ".part-3f2a" means nothing to it (or worse, reads as something else).
//   Every command therefore pins `-f <muxer>` explicitly. That single flag is why
//   an interrupted video write is survivable.
//
// DEAD SIMPLE ON PURPOSE
//   One input, one output, no audio (a GIF has none, and an MP4 with a silent
//   AAC track is bigger for no reason), no faststart on WebM (it means nothing
//   there). Anything cleverer belongs behind a per-format settings dialog that
//   the user can actually see.
#ifndef GIFSCYTHE_CORE_FFMPEG_COMMAND_H
#define GIFSCYTHE_CORE_FFMPEG_COMMAND_H
#include "MediaFormat.h"
#include <cctype>
#include <string>
#include <vector>

namespace gs {

struct FfmpegEncodeRequest {
  std::string input;   // the queued moving picture (GIF today)
  std::string partial; // staging path: created by the run, promoted on success
  OutputFormat format = OutputFormat::Gif;
  int quality = -1;  // -1 = builder default; 0..100 where the codec has a scale
  int fps = -1;      // -1 = keep the source timing
  int loop = 0;      // 0 = animate forever (APNG/WebP)
};

// The muxer name passed to `-f`. Empty for GIF (gifsicle owns it).
inline const char* ffmpeg_muxer(OutputFormat f) {
  switch (f) {
    case OutputFormat::Gif: return "";
    case OutputFormat::Apng: return "apng";
    case OutputFormat::Webp: return "webp";
    case OutputFormat::Mp4H264:
    case OutputFormat::Mp4H265: return "mp4";
    case OutputFormat::WebmVp9:
    case OutputFormat::WebmAv1: return "webm";
  }
  return "";
}

// The video codec. libwebp_anim (not libwebp) is the one that writes a MOVING
// WebP; asking for libwebp gives a still image and no error, which is exactly
// the silent no-op this project refuses.
inline const char* ffmpeg_video_codec(OutputFormat f) {
  switch (f) {
    case OutputFormat::Gif: return "";
    case OutputFormat::Apng: return "apng";
    case OutputFormat::Webp: return "libwebp_anim";
    case OutputFormat::Mp4H264: return "libx264";
    case OutputFormat::Mp4H265: return "libx265";
    case OutputFormat::WebmVp9: return "libvpx-vp9";
    case OutputFormat::WebmAv1: return "libaom-av1";
  }
  return "";
}

// A name to show in the run log, e.g. "APNG via libaom-av1".
inline std::string ffmpeg_encode_label(OutputFormat f) {
  return std::string(output_format_name(f)) + " via " + ffmpeg_video_codec(f);
}

// Empty when the request can be encoded; otherwise the reason, worded for the
// person reading the dialog.
inline std::string ffmpeg_request_refusal(const FfmpegEncodeRequest& r) {
  if (r.input.empty()) return "no input file was selected";
  if (r.partial.empty()) return "no staging path was claimed for the result";
  if (r.format == OutputFormat::Gif)
    return "GIF is written by the bundled gifsicle engine, not by FFmpeg";
  if (r.quality != -1 && (r.quality < 0 || r.quality > 100))
    return "quality must be between 0 and 100";
  if (r.fps != -1 && (r.fps < 1 || r.fps > 120))
    return "frame rate must be between 1 and 120";
  return {};
}

// The full argument list (without argv[0]). Returns empty on a refused request,
// so a caller that ignores the refusal above produces no command at all rather
// than a half-formed one.
inline std::vector<std::string> ffmpeg_encode_args(const FfmpegEncodeRequest& r) {
  std::vector<std::string> a;
  if (!ffmpeg_request_refusal(r).empty()) return a;

  a.push_back("-hide_banner");
  a.push_back("-nostdin");   // never wait for a tty; we run it with a pipe
  a.push_back("-y");         // the staging path is ours; overwriting it is the point
  a.push_back("-i");
  a.push_back(r.input);

  // Frame rate first, so the filters see the timing the rest of the command
  // assumes. GIF sources have a per-frame delay; FFmpeg's default 25 fps would
  // speed the animation up, so the caller may pin it.
  if (r.fps != -1) {
    a.push_back("-r");
    a.push_back(std::to_string(r.fps));
  }

  switch (r.format) {
    case OutputFormat::Gif:
      return {};
    case OutputFormat::Apng:
      a.push_back("-c:v");
      a.push_back("apng");
      a.push_back("-pix_fmt");
      a.push_back("rgba");
      a.push_back("-plays");
      a.push_back(std::to_string(r.loop));
      break;
    case OutputFormat::Webp:
      a.push_back("-c:v");
      a.push_back("libwebp_anim");
      a.push_back("-pix_fmt");
      a.push_back("yuva420p");
      a.push_back("-loop");
      a.push_back(std::to_string(r.loop));
      a.push_back("-q:v");
      a.push_back(std::to_string(r.quality < 0 ? 75 : r.quality));
      break;
    case OutputFormat::Mp4H264:
      // yuv420p + even dimensions: everything plays it. An odd width from a
      // cropped GIF makes libx264 fail outright, and "failed for some inputs" is
      // a bug report nobody can reproduce.
      a.push_back("-vf");
      a.push_back("scale=trunc(iw/2)*2:trunc(ih/2)*2,format=yuv420p");
      a.push_back("-c:v");
      a.push_back("libx264");
      a.push_back("-preset");
      a.push_back("medium");
      a.push_back("-crf");
      a.push_back(std::to_string(r.quality < 0 ? 20 : (1 + (100 - r.quality) * 50 / 100)));
      a.push_back("-movflags");
      a.push_back("+faststart");
      break;
    case OutputFormat::Mp4H265:
      a.push_back("-vf");
      a.push_back("scale=trunc(iw/2)*2:trunc(ih/2)*2,format=yuv420p");
      a.push_back("-c:v");
      a.push_back("libx265");
      a.push_back("-preset");
      a.push_back("medium");
      a.push_back("-crf");
      a.push_back(std::to_string(r.quality < 0 ? 24 : (1 + (100 - r.quality) * 50 / 100)));
      // hvc1, not hev1: QuickTime/iOS refuse hev1, and "plays in VLC but not on
      // the phone" is not a result.
      a.push_back("-tag:v");
      a.push_back("hvc1");
      a.push_back("-movflags");
      a.push_back("+faststart");
      break;
    case OutputFormat::WebmVp9:
      a.push_back("-c:v");
      a.push_back("libvpx-vp9");
      a.push_back("-b:v");
      a.push_back("0");
      a.push_back("-crf");
      a.push_back(std::to_string(r.quality < 0 ? 32 : (10 + (100 - r.quality) * 40 / 100)));
      a.push_back("-row-mt");
      a.push_back("1");
      break;
    case OutputFormat::WebmAv1:
      a.push_back("-c:v");
      a.push_back("libaom-av1");
      a.push_back("-b:v");
      a.push_back("0");
      a.push_back("-crf");
      a.push_back(std::to_string(r.quality < 0 ? 32 : (10 + (100 - r.quality) * 40 / 100)));
      a.push_back("-cpu-used");
      a.push_back("6");  // AV1 encoding is slow enough to look hung at default speed
      break;
  }

  // A GIF carries no audio; if a future input does, a stray AAC track is weight
  // nobody asked for.
  if (output_format_is_video(r.format)) {
    a.push_back("-an");
  }
  // The explicit muxer: the staging path's extension is not a format hint.
  a.push_back("-f");
  a.push_back(ffmpeg_muxer(r.format));
  a.push_back(r.partial);
  return a;
}

// The staging name an encoder needs. GIF keeps whatever the caller supplied (the
// gifsicle engine writes to the path it is given); every FFmpeg format stages
// under its real extension so a human looking at the temp folder can also tell
// what is being written. Claiming is still the caller's job.
inline std::string ffmpeg_staging_name(const std::string& final_name, OutputFormat f) {
  if (f == OutputFormat::Gif) return final_name;
  const std::string want = std::string(".") + output_format_extension(f);
  std::string stem = final_name;
  const std::size_t dot = stem.find_last_of('.');
  const std::size_t slash = stem.find_last_of("/\\");
  if (dot != std::string::npos && (slash == std::string::npos || dot > slash)) {
    // Only a trailing MEDIA extension is replaced. A staging marker such as
    // ".part-7" is not one, so it is kept: stripping it would hand two
    // concurrent runs the same temp file, and the loser would silently win.
    std::string tail = stem.substr(dot + 1);
    for (char& c : tail) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    static const char* kMedia[] = {"gif", "png", "webp", "mp4", "webm", "apng"};
    for (const char* m : kMedia) {
      if (tail == m) { stem = stem.substr(0, dot); break; }
    }
  }
  return stem + want;
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_FFMPEG_COMMAND_H
