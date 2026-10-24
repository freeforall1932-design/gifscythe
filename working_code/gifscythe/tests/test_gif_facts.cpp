// test_gif_facts.cpp - Unit tests for gs::parse_gif_facts() / read_gif_facts().
//
// Qt-free, plain g++, no display - the same shape as test_input_admission.cpp,
// and for the same reason: slice C puts Frames / Duration / fps / Loop on the
// Input tab, and those numbers must be pinned by a test rather than trusted to
// a screenshot. The fixtures below are hand-built byte streams, so each case
// exercises exactly one structural fact instead of whatever a real encoder
// happened to emit.
//
// The parser deliberately does NOT decode LZW, so the image payloads here are
// plausible sub-block chains with arbitrary bytes: a decoder would reject them,
// and that is not what this test is about.

#include "../src/core/GifFacts.h"

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

namespace {

using Bytes = std::vector<unsigned char>;

Bytes ascii(const char* s) {
  Bytes v;
  for (const char* p = s; *p; ++p) v.push_back(static_cast<unsigned char>(*p));
  return v;
}

void append(Bytes& to, const Bytes& from) { to.insert(to.end(), from.begin(), from.end()); }

// A two-frame 2x2 GIF with the delays given, and an optional NETSCAPE loop
// count (-1 = write no loop block at all).
Bytes make_gif(bool gif89 = true, int delay1 = 10, int delay2 = 20, int loop = -1,
               bool global_colour_table = false, bool local_colour_table = false) {
  Bytes b = ascii(gif89 ? "GIF89a" : "GIF87a");
  const unsigned char packed = global_colour_table ? 0x81 : 0x00;  // 4 GCT entries
  const unsigned char lsd[] = {0x02, 0x00, 0x02, 0x00, packed, 0x00, 0x00};
  append(b, Bytes(lsd, lsd + sizeof lsd));
  if (global_colour_table) append(b, Bytes(12, 0x11));  // 4 * 3 bytes

  if (loop >= 0) {
    const Bytes app = ascii("NETSCAPE2.0");
    b.push_back(0x21);
    b.push_back(0xFF);
    b.push_back(static_cast<unsigned char>(app.size()));
    append(b, app);
    b.push_back(0x03);
    b.push_back(0x01);
    b.push_back(static_cast<unsigned char>(loop & 0xFF));
    b.push_back(static_cast<unsigned char>((loop >> 8) & 0xFF));
    b.push_back(0x00);  // terminate the sub-block chain
  }

  for (int d : {delay1, delay2}) {
    b.push_back(0x21);  // GCE
    b.push_back(0xF9);
    b.push_back(0x04);
    b.push_back(0x00);  // packed
    b.push_back(static_cast<unsigned char>(d & 0xFF));
    b.push_back(static_cast<unsigned char>((d >> 8) & 0xFF));
    b.push_back(0x00);  // transparent index
    b.push_back(0x00);  // block terminator

    const unsigned char desc[] = {0x2C, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02,
                                  0x00, static_cast<unsigned char>(local_colour_table ? 0x81 : 0x00)};
    append(b, Bytes(desc, desc + sizeof desc));
    if (local_colour_table) append(b, Bytes(12, 0x22));
    b.push_back(0x02);                     // LZW minimum code size
    b.push_back(0x03);                     // sub-block length
    b.push_back(0xAA); b.push_back(0xBB); b.push_back(0xCC);
    b.push_back(0x00);                     // terminate
  }

  b.push_back(0x3B);  // trailer
  return b;
}

void write_bytes(const fs::path& p, const Bytes& bytes) {
  std::ofstream out(p, std::ios::binary);
  out.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
}

GifFacts parse(const Bytes& b) { return parse_gif_facts(b.data(), b.size()); }

}  // namespace

int main() {
  std::printf("== gif facts (slice C: animation columns) ==\n");

  // ---- the happy path: 2 frames, 300 ms, loop forever ----
  {
    const Bytes g = make_gif(true, 10, 20, 0);
    const GifFacts f = parse(g);
    CHECK(f.ok);
    CHECK(!f.truncated);
    CHECK(f.width == 2);
    CHECK(f.height == 2);
    CHECK(f.frames == 2);
    CHECK(f.duration_ms == 300);  // (10 + 20)/100 s
    CHECK(f.loop_count == 0);
    CHECK(f.animated());
    CHECK(f.loops_forever());
    const double fps = f.fps();
    CHECK(fps > 6.6 && fps < 6.7);  // 2 frames / 0.3 s
  }

  // ---- no loop block at all: play once, and say so with -1 (not 0) ----
  {
    const GifFacts f = parse(make_gif(true, 5, 5, -1));
    CHECK(f.ok);
    CHECK(f.loop_count == -1);
    CHECK(!f.loops_forever());
    CHECK(!f.animated() == false);
    CHECK(f.duration_ms == 100);
  }

  // ---- a finite repeat count survives the byte order ----
  {
    const GifFacts f = parse(make_gif(true, 1, 1, 0x0102));  // 258
    CHECK(f.ok);
    CHECK(f.loop_count == 258);
  }

  // ---- GIF87a is a GIF too ----
  {
    const GifFacts f = parse(make_gif(false, 10, 20, -1));
    CHECK(f.ok);
    CHECK(f.frames == 2);
  }

  // ---- colour tables are skipped, not mis-read as blocks ----
  {
    const GifFacts gct = parse(make_gif(true, 10, 20, 0, /*gct=*/true, false));
    CHECK(gct.ok);
    CHECK(gct.frames == 2);
    const GifFacts lct = parse(make_gif(true, 10, 20, -1, false, /*lct=*/true));
    CHECK(lct.ok);
    CHECK(lct.frames == 2);
  }

  // ---- a single-frame GIF is not animated, and has no invented fps ----
  {
    Bytes one = ascii("GIF89a");
    const unsigned char lsd[] = {0x10, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00};
    append(one, Bytes(lsd, lsd + sizeof lsd));
    const unsigned char desc[] = {0x2C, 0, 0, 0, 0, 0x10, 0, 0x10, 0, 0x00};
    append(one, Bytes(desc, desc + sizeof desc));
    one.push_back(0x02);
    one.push_back(0x02); one.push_back(0xAA); one.push_back(0xBB);
    one.push_back(0x00);
    one.push_back(0x3B);
    const GifFacts f = parse(one);
    CHECK(f.ok);
    CHECK(f.frames == 1);
    CHECK(!f.animated());
    CHECK(f.duration_ms == 0);
    CHECK(f.fps() == 0.0);
  }

  // ---- truncated: the numbers read so far survive, but it is not "ok" ----
  {
    // Losing only the trailer: both frames were complete and counted.
    Bytes g = make_gif(true, 10, 20, 0);
    g.resize(g.size() - 1);
    const GifFacts f = parse(g);
    CHECK(!f.ok);
    CHECK(f.truncated);
    CHECK(f.width == 2);
    CHECK(f.frames == 2);
    CHECK(f.duration_ms == 300);
  }
  {
    // Losing bytes inside the SECOND image descriptor: the first frame is
    // real, the second never completed, and the count must say 1 - a grid
    // claiming 2 frames for a file that holds 1 would be a lie about input.
    Bytes g = make_gif(true, 10, 20, 0);
    g.resize(g.size() - 10);
    const GifFacts f = parse(g);
    CHECK(!f.ok);
    CHECK(f.truncated);
    CHECK(f.frames == 1);
    CHECK(f.duration_ms == 300);  // both GCE delays were read before the cut
  }

  // ---- not a GIF at all: no signature, and NOT reported as truncated ----
  {
    const Bytes png = ascii("\x89PNG\r\n\x1a\n");
    const GifFacts f = parse(png);
    CHECK(!f.ok);
    CHECK(!f.truncated);
    CHECK(f.frames == 0);
    CHECK(f.width == 0);
  }
  {
    const GifFacts f = parse(Bytes{});
    CHECK(!f.ok);
    CHECK(!f.truncated);
  }
  {
    const Bytes short_sig = ascii("GIF8");
    CHECK(!parse(short_sig).ok);
  }

  // ---- garbage where a block should be: stop, do not loop forever ----
  {
    Bytes g = ascii("GIF89a");
    const unsigned char lsd[] = {2, 0, 2, 0, 0, 0, 0};
    append(g, Bytes(lsd, lsd + sizeof lsd));
    g.push_back(0x99);  // not a block a GIF may contain
    const GifFacts f = parse(g);
    CHECK(!f.ok);
    CHECK(!f.truncated);
    CHECK(f.frames == 0);
  }

  // ---- the file back end streams the same answer from disk ----
  {
    const fs::path dir = fs::temp_directory_path() / "gs_gif_facts_test";
    fs::remove_all(dir);
    fs::create_directories(dir);

    const fs::path p = dir / "two_frames.gif";
    write_bytes(p, make_gif(true, 10, 20, 3));
    const GifFacts f = read_gif_facts(p.string());
    CHECK(f.ok);
    CHECK(f.frames == 2);
    CHECK(f.duration_ms == 300);
    CHECK(f.loop_count == 3);

    // A missing file is a clean failure, not an exception or a partial read.
    const GifFacts missing = read_gif_facts((dir / "nope.gif").string());
    CHECK(!missing.ok);
    CHECK(!missing.truncated);
    CHECK(missing.frames == 0);

    fs::remove_all(dir);
  }

  std::printf("==> %d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
