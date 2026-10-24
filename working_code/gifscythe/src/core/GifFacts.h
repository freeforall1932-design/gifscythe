// GifFacts.h - read the ANIMATION facts out of a GIF without decoding it.
//
// WHY THIS EXISTS (slice C, 2026-10-24)
//   Slice C replaces the flat queue list with the reference's thumbnail grid,
//   and the study of the shipped XnConvert (docs/planning/XN_CONVERT_PROGRAM_STUDY.md
//   §3, §11.1) found the decisive omission in its Input tab: every one of its
//   columns is a STILL-IMAGE property (Size, EXIF Date Taken, Print size,
//   Rating, ...). There is no Frames, Duration, fps or Loop anywhere in the
//   program, because XnConvert treats an animation as page 1 of a multipage
//   image and drops the rest by default.
//
//   gifscythe's unit of work is a frame with a delay, so its queue has to show
//   frames, duration, fps and loop. Those four numbers are cheap to get: the
//   GIF stream carries them in block headers, and finding them needs no LZW
//   decode, no Qt, no subprocess. This header reads them directly from the
//   bytes, which means the values can be pinned by a plain g++ test
//   (tests/test_gif_facts.cpp) instead of only being visible in a screenshot.
//
// WHY NOT `gifsicle --info`
//   The GUI already runs gifsicle, but not for this: --info on every file as it
//   enters the queue would spawn one process per drop (and the queue can be
//   hundreds of files), and it would make a *display* fact depend on the engine
//   being present — the grid would go blank when the engine is missing, which
//   is exactly when the user needs to see what they queued.
//
// WHAT IT DOES NOT DO
//   It does not validate image data, decode pixels, or judge whether the file
//   will convert. Admission stays gs::admit_input()'s job (InputAdmission.h);
//   this only reports what the container claims. A structurally broken GIF
//   returns ok=false with whatever was counted so far, and `truncated` set when
//   the file ended early, so the GUI can print "-" instead of inventing "0
//   frames".
//
// Loop semantics, pinned here because the GUI must not guess:
//   loop_count == -1  no NETSCAPE/ANIMEXTS loop block: play once
//   loop_count ==  0  loop forever (the usual "infinite" GIF)
//   loop_count ==  n  repeat n times (the block's own count)
//
// Qt-free, header-only, C++17, no allocation beyond the two small stack
// buffers: the same rule as InputAdmission.h, so the Qt-free build can test it.

#ifndef GIFSCYTHE_CORE_GIFFACTS_H
#define GIFSCYTHE_CORE_GIFFACTS_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace gs {

struct GifFacts {
  // The signature was a GIF and the block walk reached the trailer.
  bool ok = false;
  // The file ended in the middle of the block walk. `ok` is then false, but
  // width/height/frames/duration keep whatever was actually read.
  bool truncated = false;

  int width = 0;   // logical screen width, from the logical screen descriptor
  int height = 0;  // logical screen height
  int frames = 0;  // count of image descriptors (each is one frame/page)

  // Sum of the graphic-control-extension delays, in milliseconds. A GIF with
  // no GCE at all reports 0 here even though it has frames: the file does not
  // claim a duration, and inventing one at a default frame rate is the kind of
  // guess that makes a converter look wrong about its own input.
  std::int64_t duration_ms = 0;

  int loop_count = -1;  // see the header comment

  bool animated() const { return frames > 1; }

  // Frames per second implied by the container. 0 when the file either has no
  // duration to divide by (missing/zero delays) or is a single frame - the GUI
  // prints "-" for that rather than "inf".
  double fps() const {
    if (frames <= 0 || duration_ms <= 0) return 0.0;
    return static_cast<double>(frames) * 1000.0 / static_cast<double>(duration_ms);
  }

  // True when the file declares an infinite loop (the common case for a GIF
  // people actually share).
  bool loops_forever() const { return loop_count == 0; }
};

namespace detail {

// Bounded, defensive walk. Both back ends expose exactly this: read n bytes,
// skip n bytes (without reading them - colour tables and LZW payloads can be
// most of the file), and the total size so a skip past the end is DETECTED.
// std::istream::seekg() past EOF succeeds silently, which is why the position
// is tracked by hand instead of trusted to the stream.

class MemorySource {
 public:
  MemorySource(const unsigned char* data, std::size_t size)
      : data_(data), size_(size) {}

  bool read(unsigned char* dst, std::size_t n) {
    if (!data_ || n > size_ - pos_) return false;
    std::memcpy(dst, data_ + pos_, n);
    pos_ += n;
    return true;
  }
  bool skip(std::size_t n) {
    if (n > size_ - pos_) return false;
    pos_ += n;
    return true;
  }
  std::size_t size() const { return size_; }
  std::size_t pos() const { return pos_; }

 private:
  const unsigned char* data_ = nullptr;
  std::size_t size_ = 0;
  std::size_t pos_ = 0;
};

class FileSource {
 public:
  explicit FileSource(const std::string& path) : in_(path, std::ios::binary) {
    std::error_code ec;
    const std::uintmax_t n = std::filesystem::file_size(path, ec);
    size_ = ec ? 0 : static_cast<std::size_t>(n);
  }

  bool good() const { return static_cast<bool>(in_); }

  bool read(unsigned char* dst, std::size_t n) {
    if (n > size_ - pos_) return false;
    if (n == 0) return true;
    in_.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(n));
    if (!in_) return false;
    pos_ += n;
    return true;
  }
  bool skip(std::size_t n) {
    if (n > size_ - pos_) return false;
    pos_ += n;
    in_.seekg(static_cast<std::streamoff>(n), std::ios::cur);
    return static_cast<bool>(in_);
  }
  std::size_t size() const { return size_; }
  std::size_t pos() const { return pos_; }

 private:
  std::ifstream in_;
  std::size_t size_ = 0;
  std::size_t pos_ = 0;
};

// A sub-block chain is [len][len bytes]... terminated by a zero length. Every
// unknown extension and every image's LZW payload has this shape, so one
// skipper covers them all. `first` optionally receives the first block's bytes
// (used for the application identifier, which is itself the first block).
template <class Source>
bool skip_sub_blocks(Source& src) {
  for (int guard = 0; guard < 1 << 20; ++guard) {
    unsigned char len = 0;
    if (!src.read(&len, 1)) return false;
    if (len == 0) return true;
    if (!src.skip(len)) return false;
  }
  return false;
}

// Cap the walk: a malformed file must not spin the GUI thread. 1e6 blocks is
// far above any real GIF (a 10,000-frame animation with a GCE each is ~20k).
constexpr int kMaxBlocks = 1000000;

template <class Source>
GifFacts parse_gif(Source& src) {
  GifFacts f;

  unsigned char sig[6] = {0};
  if (!src.read(sig, 6)) return f;  // empty/short: not a GIF, not "truncated"
  const bool sig_ok =
      std::memcmp(sig, "GIF87a", 6) == 0 || std::memcmp(sig, "GIF89a", 6) == 0;
  if (!sig_ok) return f;

  unsigned char lsd[7] = {0};  // width, height, packed, bg, aspect
  if (!src.read(lsd, 7)) {
    f.truncated = true;
    return f;
  }
  f.width = static_cast<int>(lsd[0]) | (static_cast<int>(lsd[1]) << 8);
  f.height = static_cast<int>(lsd[2]) | (static_cast<int>(lsd[3]) << 8);
  const unsigned char packed = lsd[4];
  if ((packed & 0x80) != 0) {  // global colour table present
    const std::size_t entries = static_cast<std::size_t>(1) << ((packed & 0x07) + 1);
    if (!src.skip(entries * 3)) {
      f.truncated = true;
      return f;
    }
  }

  for (int guard = 0; guard < kMaxBlocks; ++guard) {
    unsigned char block = 0;
    if (!src.read(&block, 1)) {
      f.truncated = true;
      return f;
    }

    if (block == 0x3B) {  // trailer
      f.ok = true;
      return f;
    }

    if (block == 0x21) {  // extension introducer
      unsigned char label = 0;
      if (!src.read(&label, 1)) {
        f.truncated = true;
        return f;
      }
      unsigned char header_len = 0;
      if (!src.read(&header_len, 1)) {
        f.truncated = true;
        return f;
      }
      unsigned char header[256] = {0};
      const std::size_t n = header_len < 255 ? header_len : 255;
      if (!src.read(header, n)) {
        f.truncated = true;
        return f;
      }
      if (header_len > n && !src.skip(header_len - n)) {  // cannot happen: u8 <= 255
        f.truncated = true;
        return f;
      }

      if (label == 0xF9 && header_len >= 4) {
        // Graphic control extension: packed, delay (1/100 s, little endian),
        // transparent index. A zero delay is legal and means "as fast as
        // possible" - it contributes 0 ms, which is why duration can stay 0
        // on a multi-frame file and fps() reports 0.
        const std::int64_t centis =
            static_cast<std::int64_t>(header[1]) |
            (static_cast<std::int64_t>(header[2]) << 8);
        f.duration_ms += centis * 10;
      }

      if (label == 0xFF && header_len >= 11 &&
          (std::memcmp(header, "NETSCAPE2.0", 11) == 0 ||
           std::memcmp(header, "ANIMEXTS1.0", 11) == 0)) {
        // Loop block: sub-block 3 bytes [0x01][lo][hi]. The FIRST one wins;
        // a file that repeats the block is malformed but harmless.
        for (int g2 = 0; g2 < 64; ++g2) {
          unsigned char len = 0;
          if (!src.read(&len, 1)) {
            f.truncated = true;
            return f;
          }
          if (len == 0) break;
          unsigned char sub[256] = {0};
          const std::size_t sn = len < 255 ? len : 255;
          if (!src.read(sub, sn)) {
            f.truncated = true;
            return f;
          }
          if (len > sn && !src.skip(len - sn)) {
            f.truncated = true;
            return f;
          }
          if (len >= 3 && sub[0] == 0x01 && f.loop_count < 0) {
            f.loop_count = static_cast<int>(sub[1]) | (static_cast<int>(sub[2]) << 8);
          }
        }
        continue;  // the loop ends at the chain terminator already
      }

      // Every other extension (comment, plain text, application) continues
      // with a sub-block chain; the header above was its first block for
      // 0x01/0xFF and the whole payload for a GCE.
      if (!skip_sub_blocks(src)) {
        f.truncated = true;
        return f;
      }
      continue;
    }

    if (block == 0x2C) {  // image descriptor -> one frame
      unsigned char desc[9] = {0};  // left, top, width, height, packed
      if (!src.read(desc, 9)) {
        f.truncated = true;
        return f;
      }
      ++f.frames;
      const unsigned char img_packed = desc[8];
      if ((img_packed & 0x80) != 0) {  // local colour table
        const std::size_t entries =
            static_cast<std::size_t>(1) << ((img_packed & 0x07) + 1);
        if (!src.skip(entries * 3)) {
          f.truncated = true;
          return f;
        }
      }
      unsigned char lzw_min = 0;
      if (!src.read(&lzw_min, 1)) {
        f.truncated = true;
        return f;
      }
      if (!skip_sub_blocks(src)) {
        f.truncated = true;
        return f;
      }
      continue;
    }

    // Anything else is not a block a GIF may contain: stop, and do not claim
    // the file is complete.
    return f;
  }

  f.truncated = true;
  return f;
}

}  // namespace detail

// Parse from bytes the caller already holds (tests, and any future drag payload).
inline GifFacts parse_gif_facts(const unsigned char* data, std::size_t size) {
  detail::MemorySource src(data, size);
  return detail::parse_gif(src);
}

// Parse straight from disk, streaming: large animations are not read into
// memory, because colour tables and LZW payloads are SKIPPED, not loaded.
inline GifFacts read_gif_facts(const std::string& path) {
  detail::FileSource src(path);
  if (!src.good()) return GifFacts{};
  return detail::parse_gif(src);
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_GIFFACTS_H
