// N-62 step 2: the vocabulary of OUTPUT CONTAINERS, in one Qt-free place.
//
// Three different parts of the program need to agree on this list, and they used
// to disagree by omission: the Format combo (what a user may pick), the encoder
// argv builder (what actually writes the file), and verification (what proves
// the file is real). A format that exists in only one of those three is exactly
// the "listed but silently does nothing" bug the owner forbade (OD-21).
//
// Everything here is pure data + header sniffing. No FFmpeg, no Qt, no I/O
// beyond the bytes it is handed, so the rules are testable without a 100 MB
// sidecar on the machine.
#ifndef GIFSCYTHE_CORE_MEDIA_FORMAT_H
#define GIFSCYTHE_CORE_MEDIA_FORMAT_H
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace gs {

// "Same as original" is NOT a format: it means "keep GIF". It is deliberately
// absent from this enum so that no code can treat it as an encoder target.
enum class OutputFormat {
  Gif,      // the bundled gifsicle engine; no FFmpeg needed
  Apng,     // FFmpeg
  Webp,     // FFmpeg, animated
  Mp4H264,  // FFmpeg
  Mp4H265,  // FFmpeg
  WebmVp9,  // FFmpeg
  WebmAv1,  // FFmpeg
};

inline const char* output_format_name(OutputFormat f) {
  switch (f) {
    case OutputFormat::Gif: return "GIF";
    case OutputFormat::Apng: return "APNG";
    case OutputFormat::Webp: return "WebP";
    case OutputFormat::Mp4H264: return "MP4 (H.264)";
    case OutputFormat::Mp4H265: return "MP4 (H.265/HEVC)";
    case OutputFormat::WebmVp9: return "WebM (VP9)";
    case OutputFormat::WebmAv1: return "WebM (AV1)";
  }
  return "unknown";
}

// The FORMAT owns the extension (owner's naming decision, 2026-10-26): the name
// template never carries one, and whatever the user typed cannot leak a .gif
// onto an .mp4. APNG writes ".png" - it is a PNG file; ".apng" exists but half
// the programs that matter refuse to open it, and a result the user's viewer
// will not open is not a result.
inline const char* output_format_extension(OutputFormat f) {
  switch (f) {
    case OutputFormat::Gif: return "gif";
    case OutputFormat::Apng: return "png";
    case OutputFormat::Webp: return "webp";
    case OutputFormat::Mp4H264:
    case OutputFormat::Mp4H265: return "mp4";
    case OutputFormat::WebmVp9:
    case OutputFormat::WebmAv1: return "webm";
  }
  return "gif";
}

// FFmpeg is the only encoder for everything except GIF. The GUI uses this to
// decide whether a format entry may be enabled at all: no encoder, no entry.
inline bool output_format_needs_ffmpeg(OutputFormat f) { return f != OutputFormat::Gif; }

inline bool output_format_is_video(OutputFormat f) {
  return f == OutputFormat::Mp4H264 || f == OutputFormat::Mp4H265 ||
         f == OutputFormat::WebmVp9 || f == OutputFormat::WebmAv1;
}

// Parse a stored/session string back into a format. Unknown text is GIF, never
// "whatever was selected last" - a corrupted session must not silently start
// encoding H.265.
inline OutputFormat output_format_from_token(const std::string& t) {
  if (t == "apng") return OutputFormat::Apng;
  if (t == "webp") return OutputFormat::Webp;
  if (t == "mp4-h264") return OutputFormat::Mp4H264;
  if (t == "mp4-h265") return OutputFormat::Mp4H265;
  if (t == "webm-vp9") return OutputFormat::WebmVp9;
  if (t == "webm-av1") return OutputFormat::WebmAv1;
  return OutputFormat::Gif;
}

inline const char* output_format_token(OutputFormat f) {
  switch (f) {
    case OutputFormat::Gif: return "gif";
    case OutputFormat::Apng: return "apng";
    case OutputFormat::Webp: return "webp";
    case OutputFormat::Mp4H264: return "mp4-h264";
    case OutputFormat::Mp4H265: return "mp4-h265";
    case OutputFormat::WebmVp9: return "webm-vp9";
    case OutputFormat::WebmAv1: return "webm-av1";
  }
  return "gif";
}

// ---- header sniffing -------------------------------------------------------
//
// This is a POSTCONDITION, not a decoder: it answers "did the encoder write the
// container it promised?" A run that exits 0 but leaves a 0-byte file, a GIF
// under an .mp4 name, or a still PNG where an animation was requested must fail
// loudly. It is deliberately cheap - the first slab of the file, no dependency.
struct MediaProbe {
  bool ok = false;
  std::string problem;  // empty when ok
};

inline bool probe_has(const unsigned char* data, std::size_t size, const char* needle) {
  const std::size_t n = std::strlen(needle);
  if (n == 0 || size < n) return false;
  for (std::size_t i = 0; i + n <= size; ++i) {
    if (std::memcmp(data + i, needle, n) == 0) return true;
  }
  return false;
}

inline bool probe_starts_with(const unsigned char* data, std::size_t size, const char* prefix,
                              std::size_t len) {
  return size >= len && std::memcmp(data, prefix, len) == 0;
}

// `header` must be a slab that is long enough to contain the markers checked
// below (the callers read 64 KiB); `size` is what was actually read.
inline MediaProbe probe_output_header(OutputFormat f, const std::vector<unsigned char>& header) {
  const unsigned char* d = header.data();
  const std::size_t n = header.size();
  MediaProbe r;
  if (n == 0) {
    r.problem = "engine produced an empty file";
    return r;
  }
  switch (f) {
    case OutputFormat::Gif:
      if (!probe_starts_with(d, n, "GIF87a", 6) && !probe_starts_with(d, n, "GIF89a", 6)) {
        r.problem = "expected a GIF87a/GIF89a signature";
        return r;
      }
      break;
    case OutputFormat::Apng: {
      static const char kPng[8] = {'\x89', 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
      if (!probe_starts_with(d, n, kPng, 8)) {
        r.problem = "expected a PNG signature";
        return r;
      }
      // A still PNG would satisfy the signature. The animation control chunk is
      // what makes it APNG, and it must appear before image data in practice -
      // so its absence in this slab is a real failure, not bad luck.
      if (!probe_has(d, n, "acTL")) {
        r.problem = "PNG is not animated (no acTL chunk): that is a still image, not APNG";
        return r;
      }
      break;
    }
    case OutputFormat::Webp:
      if (!probe_starts_with(d, n, "RIFF", 4) || n < 12 || std::memcmp(d + 8, "WEBP", 4) != 0) {
        r.problem = "expected a RIFF/WEBP container";
        return r;
      }
      if (!probe_has(d, n, "ANIM")) {
        r.problem = "WebP is a still image (no ANIM chunk); animation was requested";
        return r;
      }
      if (!probe_has(d, n, "ANMF")) {
        r.problem = "WebP has no animation frames (no ANMF chunk)";
        return r;
      }
      break;
    case OutputFormat::Mp4H264:
    case OutputFormat::Mp4H265: {
      if (n < 12 || std::memcmp(d + 4, "ftyp", 4) != 0) {
        r.problem = "expected an ISO base media file (no ftyp box)";
        return r;
      }
      // The compatible-brands list follows the major brand; FFmpeg writes the
      // codec tag there when asked (-tag:v hvc1), which is what we ask for.
      if (f == OutputFormat::Mp4H265) {
        if (!probe_has(d, n, "hvc1") && !probe_has(d, n, "hev1")) {
          r.problem = "MP4 carries no HEVC brand/sample entry (hvc1/hev1)";
          return r;
        }
      } else {
        if (probe_has(d, n, "hvc1") || probe_has(d, n, "hev1")) {
          r.problem = "MP4 is HEVC while H.264 was requested";
          return r;
        }
        if (!probe_has(d, n, "avc1") && !probe_has(d, n, "avc3")) {
          r.problem = "MP4 carries no H.264 sample entry (avc1/avc3)";
          return r;
        }
      }
      break;
    }
    case OutputFormat::WebmVp9:
    case OutputFormat::WebmAv1: {
      static const unsigned char kEbml[4] = {0x1A, 0x45, 0xDF, 0xA3};
      if (!probe_starts_with(d, n, reinterpret_cast<const char*>(kEbml), 4)) {
        r.problem = "expected an EBML/Matroska header";
        return r;
      }
      if (!probe_has(d, n, "webm")) {
        r.problem = "Matroska file is not the WebM doctype";
        return r;
      }
      if (f == OutputFormat::WebmVp9) {
        if (probe_has(d, n, "AV01")) {
          r.problem = "WebM is AV1 while VP9 was requested";
          return r;
        }
        if (!probe_has(d, n, "V_VP9")) {
          r.problem = "WebM carries no VP9 track (V_VP9)";
          return r;
        }
      } else {
        if (probe_has(d, n, "V_VP9")) {
          r.problem = "WebM is VP9 while AV1 was requested";
          return r;
        }
        if (!probe_has(d, n, "V_AV1") && !probe_has(d, n, "AV01")) {
          r.problem = "WebM carries no AV1 track (V_AV1/AV01)";
          return r;
        }
      }
      break;
    }
  }
  r.ok = true;
  return r;
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_MEDIA_FORMAT_H
