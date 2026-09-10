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
inline bool is_windows_reserved_device_name(const std::string& name) {
  std::string stem;
  for (char c : name) {
    if (c == '.') break;
    stem += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  if (stem.size() == 3) {
    return stem == "con" || stem == "prn" || stem == "aux" || stem == "nul";
  }
  if (stem.size() == 4 &&
      (stem.compare(0, 3, "com") == 0 || stem.compare(0, 3, "lpt") == 0) &&
      std::isdigit(static_cast<unsigned char>(stem[3]))) {
    return true;
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
