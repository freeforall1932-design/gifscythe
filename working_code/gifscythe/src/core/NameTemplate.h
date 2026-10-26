// NameTemplate.h - turn one filename TEMPLATE into a real output name.
//
// Why this file exists (slice E2, owner decision 2026-10-26)
// ---------------------------------------------------------
// The old Output tab named things with a single-token template ("{name}_opt.gif")
// and a separate, literal "Save as…" path that applied only when exactly one
// file was queued. The shipped XnConvert 1.118.0 does it differently, and the
// owner asked for that model: ONE Filename field holding a token template, the
// FORMAT supplying the extension, and duplicates numbered instead of refused.
// Measured in the 1.118.0 binary (its own strings): the Output form carries
// `leFilename` (the template), `sbStartIndex`, `cbKeepFolder`, and the token
// menu offers {Filename}, {Filename With Ext}, {File Type}, {File Index},
// {Width}/{Height}, {DstWidth}/{DstHeight}, {Directory}, {Folder name},
// {Format}, {Current Date [...]}, plus the still-image EXIF family this app has
// no use for. The first two of its own example templates in the binary are
// `{Filename}_result` and `{Filename}_{Current Date [m_d_Y]}`; `_result` also
// exists as a standalone default-suffix string.
//
// So: default template = "{Filename}_result", extension = the chosen Format,
// and `{Filename}`, `{name}` (the pre-2026 name, kept so saved presets keep
// working) both render the source stem.
//
// Deliberate scope
// ----------------
//   * EXIF / Rating / Categories / Annotation / Color label are NOT offered:
//     they are still-image metadata and this product converts moving pictures.
//     {Frames} and {Duration} take their place - they come from GifFacts, which
//     is what a GIF/APNG/WebP user actually wants in a name.
//   * {Directory} and {Folder name} render the SOURCE folder, never a
//     destination path: a template must not be able to walk out of the
//     destination folder (`sanitize_output_name` strips separators for the same
//     reason, and this header stays a pure string function).
//
// Qt-free on purpose: the rule is unit-tested on every platform (and in this
// sandbox, which has no Qt), while the GUI only calls it.

#ifndef GIFSCYTHE_CORE_NAME_TEMPLATE_H
#define GIFSCYTHE_CORE_NAME_TEMPLATE_H

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

namespace gs {

// The reference's default, minus its extension: the Format decides that.
inline constexpr const char* kDefaultNameTemplate = "{Filename}_result";

// One entry per token, shared by the renderer and the GUI's completion popup so
// the two can never drift (the popup lists exactly what renders).
struct TemplateToken {
  const char* name;     // without braces, as the user types it
  const char* summary;  // one line, for the completion popup
  const char* example;  // what it renders for the sample below
};

inline const std::vector<TemplateToken>& template_tokens() {
  static const std::vector<TemplateToken> kTokens = {
      {"Filename", "source name without its extension", "clip"},
      {"Filename With Ext", "source name as it arrived, extension included", "clip.gif"},
      {"File Type", "the source format, taken from its extension", "GIF"},
      {"File Index", "position in the queue, offset by Start index", "1"},
      {"Width", "source width in pixels", "480"},
      {"Height", "source height in pixels", "270"},
      {"DstWidth", "width after the Resize action (source width if none)", "320"},
      {"DstHeight", "height after the Resize action (source height if none)", "180"},
      {"Directory", "the source file's folder", "/clips"},
      {"Folder name", "the source file's folder name", "clips"},
      {"Format", "the output format being written", "GIF"},
      {"Frames", "frame count of the source animation", "12"},
      {"Duration", "source duration in seconds, two decimals", "2.40"},
      {"Current Date [fmt]", "the clock now, formatted inside the brackets",
       "2026-10-26_01-30-00"},
  };
  return kTokens;
}

// Everything a template may render. Unknown values are honest empties, never
// made-up numbers: frames/duration are -1 until GifFacts has read the file.
struct NameContext {
  std::string filename;    // base name, no extension ("clip")
  std::string extension;   // source extension without the dot, lower case ("gif")
  std::string directory;   // source folder, as the user sees it
  std::string folder_name;
  std::string format;      // output format label ("GIF", "WebP", "MP4 (H.264)")
  long long index = 0;     // 0-based position in the queue
  long long start_index = 1;
  unsigned width = 0;
  unsigned height = 0;
  unsigned dst_width = 0;
  unsigned dst_height = 0;
  int frames = -1;
  double duration_seconds = -1.0;
  std::tm now{};           // local time, for {Current Date [...]}
};

struct NameRender {
  std::string text;
  std::vector<std::string> warnings;  // shown, never swallowed
};

// ---- small string helpers (kept here so the header stands alone) ----------

inline std::string nt_trim(const std::string& s) {
  std::size_t b = 0;
  std::size_t e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

inline std::string nt_lower(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

inline bool nt_iequals(const std::string& a, const std::string& b) {
  return nt_lower(a) == nt_lower(b);
}

inline std::string nt_number(long long v) { return std::to_string(v); }

// The date letters a template may use, in XnView/XnConvert's vocabulary - which
// is NOT Qt's and NOT strftime's. Measured from the 1.118.0 binary's own example
// templates, all four of which are self-consistent under exactly one reading:
//   {Current Date [m_d_Y]}          -> month_day_year
//   {Current Date [d_m_Y]}          -> day_month_year
//   {Current Date [Y-m-d_H-M-S]}    -> year-month-day_hour-minute-second
//   {Current Date [Y-m-d_H-M]}
// Under Qt's letters, [Y-m-d_H-M-S] would be year-MINUTE-day_HOUR-MONTH-second,
// which nobody would ship as a default; with m = month and M = minute all four
// are ordinary timestamps. So: Y year, y 2-digit year, m month, d day, H hour,
// M minute, S second.
//
// Unlike Qt's d vs dd, a single letter is ALREADY zero-padded to its natural
// width (Y 4, everything else 2): a filename never wants the leading space an
// unpadded field would leave behind, and the reference's examples are padded.
// Repeating a letter is therefore the same as writing it once, and any other
// character is copied literally.
inline std::string format_qt_date(const std::string& fmt, const std::tm& now) {
  std::string out;
  out.reserve(fmt.size() + 8);
  auto put = [&out](int value, int width, bool zero_pad) {
    std::string digits = std::to_string(value);
    if (static_cast<int>(digits.size()) < width) {
      digits.insert(0, static_cast<std::size_t>(width) - digits.size(),
                    zero_pad ? '0' : ' ');
    }
    // A non-padded field must not carry a leading space into a filename.
    if (!zero_pad) {
      std::size_t first = digits.find_first_not_of(' ');
      digits = first == std::string::npos ? std::string("0") : digits.substr(first);
    }
    out += digits;
  };
  for (std::size_t i = 0; i < fmt.size();) {
    const char c = fmt[i];
    std::size_t run = 1;
    while (i + run < fmt.size() && fmt[i + run] == c) ++run;
    switch (c) {
      case 'Y': put(now.tm_year + 1900, 4, true); break;
      case 'y': put((now.tm_year + 1900) % 100, 2, true); break;
      case 'm': put(now.tm_mon + 1, 2, true); break;   // month, not minute
      case 'd': put(now.tm_mday, 2, true); break;
      case 'H': put(now.tm_hour, 2, true); break;
      case 'M': put(now.tm_min, 2, true); break;       // minute, not month
      case 'S': put(now.tm_sec, 2, true); break;
      default:
        // Anything else is literal text (_, -, ., spaces, ...). Repeated
        // unknown letters are emitted once per occurrence, not collapsed.
        out.append(run, c);
        break;
    }
    i += run;
  }
  return out;
}

// Extensions the FORMAT owns. A template that still ends with one is treated as
// pre-2026 input: the extension is dropped (with a warning) instead of producing
// "clip.gif.webp" the moment the Format changes.
inline bool nt_is_media_extension(const std::string& ext_no_dot) {
  static const char* kKnown[] = {"gif", "png", "apng", "webp", "jpg", "jpeg",
                                 "bmp", "mp4", "webm", "mkv", "mov", "avi",
                                 "m4v", "tif", "tiff"};
  const std::string e = nt_lower(ext_no_dot);
  for (const char* k : kKnown) {
    if (e == k) return true;
  }
  return false;
}

// True when the template asks for a per-file value, i.e. two inputs cannot
// render the same name by accident. {Format} and {Current Date} do NOT count:
// they are identical for every file of one run, which is exactly the case
// XnConvert warns about ("Output filename will be the same for all converted
// files."), and exactly what the numbering rule then handles.
inline bool template_is_constant(const std::string& tmpl) {
  static const char* kPerFile[] = {"filename", "filename with ext", "name",
                                   "file type", "file index", "width", "height",
                                   "dstwidth", "dstheight", "directory",
                                   "folder name", "frames", "duration"};
  for (std::size_t i = 0; i < tmpl.size();) {
    const std::size_t open = tmpl.find('{', i);
    if (open == std::string::npos) break;
    const std::size_t close = tmpl.find('}', open + 1);
    if (close == std::string::npos) break;
    const std::string inner = nt_lower(nt_trim(tmpl.substr(open + 1, close - open - 1)));
    for (const char* k : kPerFile) {
      if (inner == k) return false;
    }
    i = close + 1;
  }
  return true;
}

// True when the template contains {File Index}: the GUI enables Start index only
// then (a disabled spin box next to an unused token is noise).
inline bool template_uses_index(const std::string& tmpl) {
  return nt_lower(tmpl).find("{file index}") != std::string::npos;
}

// The source extension, upper-cased for {File Type} the way XnConvert shows it
// ("GIF"). Empty stays empty: an extension-less input should not invent one.
inline std::string nt_lower_key(const std::string& ext) {
  std::string up = ext;
  for (char& c : up) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return up;
}

inline NameRender render_name_template(const std::string& tmpl, const NameContext& ctx) {
  NameRender out;
  std::string t = tmpl.empty() ? std::string(kDefaultNameTemplate) : tmpl;

  // The Format owns the extension. Strip a trailing media extension that was
  // TYPED in the template - never one a token produced ({Filename With Ext} is
  // supposed to bring its own).
  {
    const std::size_t dot = t.find_last_of('.');
    if (dot != std::string::npos && dot + 1 < t.size()) {
      const std::string tail = t.substr(dot + 1);
      const bool tail_has_token = tail.find('{') != std::string::npos;
      if (!tail_has_token && nt_is_media_extension(tail)) {
        t = t.substr(0, dot);
        out.warnings.push_back(
            "The template's extension is ignored: the chosen Format supplies it.");
      }
    }
  }

  std::string result;
  result.reserve(t.size() + 16);
  for (std::size_t i = 0; i < t.size();) {
    if (t[i] != '{') {
      result += t[i++];
      continue;
    }
    const std::size_t close = t.find('}', i + 1);
    if (close == std::string::npos) {
      result += t.substr(i);
      out.warnings.push_back("The template has an unclosed '{'; it is copied as typed.");
      break;
    }
    const std::string inner = nt_trim(t.substr(i + 1, close - i - 1));
    const std::string key = nt_lower(inner);
    bool known = true;
    if (key == "filename" || key == "name") {          // {name} = pre-2026 alias
      result += ctx.filename;
    } else if (key == "filename with ext") {
      result += ctx.filename;
      if (!ctx.extension.empty()) {
        result += '.';
        result += ctx.extension;
      }
    } else if (key == "file type") {
      result += nt_lower_key(ctx.extension);
    } else if (key == "file index") {
      result += nt_number(ctx.start_index + ctx.index);
    } else if (key == "width") {
      result += nt_number(ctx.width);
    } else if (key == "height") {
      result += nt_number(ctx.height);
    } else if (key == "dstwidth") {
      result += nt_number(ctx.dst_width ? ctx.dst_width : ctx.width);
    } else if (key == "dstheight") {
      result += nt_number(ctx.dst_height ? ctx.dst_height : ctx.height);
    } else if (key == "directory") {
      result += ctx.directory;
    } else if (key == "folder name") {
      result += ctx.folder_name;
    } else if (key == "format") {
      result += ctx.format;
    } else if (key == "frames") {
      if (ctx.frames >= 0) result += nt_number(ctx.frames);
    } else if (key == "duration") {
      if (ctx.duration_seconds >= 0.0) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.2f", ctx.duration_seconds);
        result += buf;
      }
    } else if (key.rfind("current date", 0) == 0) {
      const std::size_t lb = inner.find('[');
      const std::size_t rb = inner.rfind(']');
      std::string fmt = "Y-m-d_H-M-S";
      if (lb != std::string::npos && rb != std::string::npos && rb > lb) {
        fmt = inner.substr(lb + 1, rb - lb - 1);
      }
      result += format_qt_date(fmt, ctx.now);
    } else {
      known = false;
    }
    if (!known) {
      result += t.substr(i, close - i + 1);  // copy the unknown token as typed
      out.warnings.push_back("Unknown token {" + inner + "}: copied as typed.");
    }
    i = close + 1;
  }
  out.text = result;
  if (out.text.empty()) {
    out.warnings.push_back("The template rendered an empty name; using the source name.");
    out.text = ctx.filename.empty() ? std::string("output") : ctx.filename;
  }
  return out;
}

// Duplicate rule (owner, 2026-10-26): "the first one to get processed should
// always have _result", so the FIRST file keeps the plain name and later
// collisions become _1, _2, ... before the extension. Comparison is
// case-insensitive because the shipping target (Windows) and macOS treat
// "Clip.gif" and "clip.gif" as the same file - a case-sensitive check would
// happily write both and lose one.
inline std::vector<std::string> disambiguate_names(const std::vector<std::string>& names) {
  // Two rules, and the second is why this is not a one-liner:
  //   1. the FIRST file to claim a name keeps it plainly (owner: "the first one
  //      to get processed should always have _result");
  //   2. a literal name is never taken away by a numbering step. For
  //      {a.webp, a.webp, a_1.webp} the third file OWNS a_1.webp, so the second
  //      becomes a_2.webp - not a_1.webp (which would force a_1_1.webp and make
  //      the numbering depend on queue order in a way nobody expects).
  std::vector<std::string> keys;
  keys.reserve(names.size());
  for (const std::string& n : names) keys.push_back(nt_lower(n));

  // Where each distinct name first appears; only that occurrence may keep it.
  std::vector<bool> is_first(names.size(), true);
  for (std::size_t i = 0; i < keys.size(); ++i) {
    for (std::size_t j = 0; j < i; ++j) {
      if (keys[j] == keys[i]) {
        is_first[i] = false;
        break;
      }
    }
  }

  std::vector<std::string> out;
  out.reserve(names.size());
  std::vector<std::string> assigned;
  assigned.reserve(names.size());

  // A candidate is free when nobody owns it yet AND no later file will claim it
  // literally.
  const auto is_free = [&](const std::string& candidate, std::size_t here) {
    const std::string key = nt_lower(candidate);
    if (std::find(assigned.begin(), assigned.end(), key) != assigned.end()) return false;
    for (std::size_t j = here + 1; j < keys.size(); ++j) {
      if (is_first[j] && keys[j] == key) return false;
    }
    return true;
  };

  for (std::size_t i = 0; i < names.size(); ++i) {
    const std::string& raw = names[i];
    if (is_first[i] && is_free(raw, i)) {
      out.push_back(raw);
      assigned.push_back(keys[i]);
      continue;
    }
    const std::size_t dot = raw.find_last_of('.');
    const std::size_t slash = raw.find_last_of("/\\");
    const bool has_ext =
        dot != std::string::npos && (slash == std::string::npos || dot > slash);
    const std::string stem = has_ext ? raw.substr(0, dot) : raw;
    const std::string ext = has_ext ? raw.substr(dot) : std::string();
    std::string chosen;
    for (int n = 1;; ++n) {
      std::string next = stem + "_" + std::to_string(n) + ext;
      if (is_free(next, i)) {
        chosen = next;
        break;
      }
    }
    out.push_back(chosen);
    assigned.push_back(nt_lower(chosen));
  }
  return out;
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_NAME_TEMPLATE_H
