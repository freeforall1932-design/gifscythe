// OutputName.h - Make a rendered output name safe to use as ONE file name.
//
// Audit U-21: the GUI's name-template sanitiser stripped path separators and
// nothing else. On a product whose primary target is portable Windows that is
// not enough — `<>:"|?*`, ASCII control characters, trailing dots/spaces and
// the reserved device names (CON, PRN, AUX, NUL, COM1..9, LPT1..9) are all
// rejected by the Win32 file APIs, so a template such as `a<b>.gif` or
// `CON.gif` would render in the live pane and then fail at write time, or
// silently write somewhere the user did not expect (`CON` is the console).
//
// The rules are a PARAMETER, not a compile-time switch, so the whole Windows
// rule set is unit-testable on a POSIX host. `NameRules::Host` picks the rule
// set for the platform the binary runs on.
//
// Qt-independent. Input and output are UTF-8; every byte the rules act on is
// ASCII, so multi-byte sequences pass through untouched.

#ifndef GIFSCYTHE_CORE_OUTPUT_NAME_H
#define GIFSCYTHE_CORE_OUTPUT_NAME_H

#include <cctype>
#include <string>

namespace gs {

enum class NameRules {
  Host,      // Windows rules on Windows, POSIX rules elsewhere
  Windows,   // always apply the Win32 restrictions
  Posix,     // only strip path separators
};

inline NameRules host_name_rules() {
#ifdef _WIN32
  return NameRules::Windows;
#else
  return NameRules::Posix;
#endif
}

// True for the names Win32 reserves for devices. Compared against the part
// before the first dot, case-insensitively: "con", "CON.gif" and "Com1.txt"
// are all reserved, while "console.gif" and "mycon.gif" are not.
//
// Audit U-56 / fix-order P1-36 — the ASCII-digit form was not enough: the
// Win32 reserved-name check also folds the three legacy superscript digits, so
// `COM¹.gif` addresses a device and slips past a filter that only looks at 0-9.
// In UTF-8 those are exactly two bytes each (C2 B9 / C2 B2 / C2 B3), and the
// `tolower` loop above leaves every byte >= 0x80 alone in every locale, so the
// stem still carries them.
//
// Deliberate scope: superscripts are recognised after COM/LPT only (not after
// CON, and not as a general "any Unicode digit" rule), and full-width digits
// (U+FF11...) are NOT folded — nothing was measured to show the check accepts
// them, and a guess in this function refuses legal file names. COM0/LPT0 are
// refused even though MSDN lists COM1..COM9: the asymmetry of cost says refuse.
//
// tests/windows_reserved_names.txt is the shared table both this and the web
// admission check (web/run-paths.mjs) are tested against, so the two surfaces
// cannot drift again.
inline bool is_windows_reserved_device_name(const std::string& name) {
  std::string stem;
  for (char c : name) {
    if (c == '.') break;
    stem += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  if (stem.size() == 3) {
    return stem == "con" || stem == "prn" || stem == "aux" || stem == "nul";
  }
  const bool com_or_lpt =
      stem.size() >= 4 &&
      (stem.compare(0, 3, "com") == 0 || stem.compare(0, 3, "lpt") == 0);
  if (!com_or_lpt) return false;
  if (stem.size() == 4) {
    return std::isdigit(static_cast<unsigned char>(stem[3])) != 0;
  }
  if (stem.size() == 5 && stem[3] == static_cast<char>(0xC2)) {
    const unsigned char sup = static_cast<unsigned char>(stem[4]);
    return sup == 0xB9 || sup == 0xB2 || sup == 0xB3;   // COM¹ ² ³ / LPT…
  }
  return false;
}

// Reduce `name` to a single safe file name. Returns "" when nothing usable is
// left, so the caller falls back to its own default instead of writing a
// nameless or "." file.
inline std::string sanitize_output_name(const std::string& name,
                                        NameRules rules = NameRules::Host) {
  const NameRules effective = (rules == NameRules::Host) ? host_name_rules() : rules;
  const bool win = (effective == NameRules::Windows);

  // 1. Strip any directory part. Both separator flavours on EVERY platform —
  //    this matches the GUI's existing behaviour, and a template must never be
  //    able to escape the output folder even if the result is later copied to
  //    the other OS.
  size_t start = 0;
  for (size_t i = 0; i < name.size(); ++i) {
    if (name[i] == '/' || name[i] == '\\') start = i + 1;
  }
  std::string out = name.substr(start);

  if (win) {
    // 2. Drop the characters Win32 refuses, plus ASCII control characters.
    std::string kept;
    kept.reserve(out.size());
    for (char c : out) {
      const unsigned char u = static_cast<unsigned char>(c);
      if (u < 0x20) continue;                                  // control chars
      if (c == '<' || c == '>' || c == ':' || c == '"' || c == '|' ||
          c == '?' || c == '*') continue;
      kept += c;
    }
    out = kept;

    // 3. Trim trailing dots and spaces — Win32 strips them anyway, which would
    //    make the file we wrote unreachable by the name we displayed.
    while (!out.empty() && (out.back() == '.' || out.back() == ' ')) out.pop_back();

    // 4. Reserved device names get a leading underscore rather than being
    //    deleted, so the user can still recognise the file they asked for.
    if (is_windows_reserved_device_name(out)) out = "_" + out;
  }

  // 5. Nothing usable left: let the caller decide.
  if (out.empty() || out == "." || out == "..") return std::string();
  return out;
}

}  // namespace gs

#endif  // GIFSCYTHE_CORE_OUTPUT_NAME_H
