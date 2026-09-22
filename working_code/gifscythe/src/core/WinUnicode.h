// WinUnicode.h - Windows Unicode support for the core layer (audit U-07 /
// fix-order P1-4). Qt-independent.
//
// WHY: the Windows side of this codebase used to be ANSI-only in four places:
//   1. MinGW's CRT hands main() an argv encoded in the ANSI code page (ACP),
//      so a path with characters outside the ACP was ALREADY mangled before
//      the CLI ever saw it;
//   2. std::getenv() returns ACP bytes (GS_ENGINE, PATH);
//   3. ProcessRunner called CreateProcessA, which reinterpreted our UTF-8
//      command-line bytes through the ACP again on the way to the engine;
//   4. std::filesystem narrow-string conversions are NOT UTF-8 on every
//      MinGW libstdc++ (verified under Wine with gcc 12-win32: path(string)
//      widens BYTEWISE while path::string() encodes UTF-8 - an asymmetric
//      mangling of every non-ASCII path), and narrow ifstream/ofstream/fopen
//      go through the ACP as well.
// This header removes all four: the exact UTF-16 command line is re-fetched
// with GetCommandLineW and split here per the MSVCRT argv rules (the same
// rules win_quote_arg in ProcessRunner.h inverts), environment variables are
// read with GetEnvironmentVariableW, string<->path boundaries go through the
// u8path_compat()/path_u8string() helpers below, and everything is converted
// to UTF-8 - this codebase's internal encoding contract.
//
// The splitter and the helpers' POSIX branches are PURE LOGIC (no Windows
// API), so they are unit-tested on Linux CI; the API shims compile only under
// _WIN32. Residual, documented: the ENGINE's own argv parsing belongs to
// upstream gifsicle's CRT (ACP-encoded; reference_code is read-only), so a
// character the system ACP cannot represent still cannot reach the engine's
// file APIs - on Windows 10 1903+ the system-wide UTF-8 ACP option closes
// that gap; Gifscythe's own chain is lossless regardless (proven under Wine:
// the child's UTF-16 command line carries CJK intact).

#ifndef GIFSCYTHE_CORE_WINUNICODE_H
#define GIFSCYTHE_CORE_WINUNICODE_H

#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>  // global scope, ALWAYS: mingw headers included after this
                      // one rely on the global include guard being consumed here
#endif

namespace gs {

// ---- string(UTF-8) <-> fs::path boundary helpers --------------------------
// EVERY place a std::string becomes a path (or a path becomes a std::string)
// must go through these on Windows: implicit conversion uses the toolchain's
// narrow encoding, which is not UTF-8 on all MinGW libstdc++ builds (see
// header comment). On POSIX, narrow strings ARE the native filename bytes, so
// these are the identity.
inline std::filesystem::path u8path_compat(const std::string& utf8) {
#ifdef _WIN32
  return std::filesystem::u8path(utf8);
#else
  return std::filesystem::path(utf8);
#endif
}

inline std::string path_u8string(const std::filesystem::path& p) {
#ifdef _WIN32
  return p.u8string();
#else
  return p.string();
#endif
}

// Split a Windows command line into argv tokens using the MSVCRT rules:
//   * 2n backslashes + "     -> n literal backslashes, quote TOGGLES state
//   * 2n+1 backslashes + "   -> n literal backslashes + a literal quote
//   * n backslashes (no " after) -> n literal backslashes
//   * inside quotes, "" is a literal quote (and stays inside quotes)
//   * an explicitly quoted empty string "" is a real (empty) argument
// These are exactly the rules win_quote_arg() (ProcessRunner.h) produces
// arguments for; unit test 34 pins the round-trip.
inline std::vector<std::wstring> split_win_cmdline(const std::wstring& cmd) {
  std::vector<std::wstring> out;
  std::wstring cur;
  bool in_quotes = false;
  bool has_token = false;  // distinguishes "" (a real empty arg) from nothing
  size_t i = 0;
  while (i < cmd.size()) {
    const wchar_t c = cmd[i];
    if (c == L'\\') {
      size_t n = 0;
      while (i + n < cmd.size() && cmd[i + n] == L'\\') ++n;
      if (i + n < cmd.size() && cmd[i + n] == L'"') {
        cur.append(n / 2, L'\\');
        if (n % 2 == 1) {
          cur += L'"';          // odd run: the quote is literal, state kept
          has_token = true;
        } else {
          in_quotes = !in_quotes;  // even run: the quote toggles quoting
          has_token = true;
        }
        i += n + 1;
      } else {
        cur.append(n, L'\\');
        has_token = true;
        i += n;
      }
      continue;
    }
    if (c == L'"') {
      if (in_quotes && i + 1 < cmd.size() && cmd[i + 1] == L'"') {
        cur += L'"';            // "" inside quotes -> one literal quote
        has_token = true;
        i += 2;
        continue;
      }
      in_quotes = !in_quotes;
      has_token = true;
      ++i;
      continue;
    }
    if (!in_quotes && (c == L' ' || c == L'\t')) {
      if (has_token) {
        out.push_back(cur);
        cur.clear();
        has_token = false;
      }
      ++i;
      continue;
    }
    cur += c;
    has_token = true;
    ++i;
  }
  if (has_token) out.push_back(cur);
  return out;
}

#ifdef _WIN32

// UTF-16 -> UTF-8. Empty in, empty out.
inline std::string wide_to_utf8(const std::wstring& w) {
  if (w.empty()) return std::string();
  const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(),
                                    static_cast<int>(w.size()),
                                    nullptr, 0, nullptr, nullptr);
  if (n <= 0) return std::string();
  std::string s(static_cast<size_t>(n), '\0');
  WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()),
                      &s[0], n, nullptr, nullptr);
  return s;
}

// GetEnvironmentVariableW wrapped into UTF-8: std::getenv() would hand back
// ACP bytes and lose every character the ACP cannot represent.
inline std::string win_getenv_utf8(const char* name) {
  std::wstring wname(name, name + std::char_traits<char>::length(name));
  const DWORD len = GetEnvironmentVariableW(wname.c_str(), nullptr, 0);
  if (len == 0) return std::string();  // unset (or error): "not set" either way
  std::wstring buf(static_cast<size_t>(len), L'\0');
  const DWORD got = GetEnvironmentVariableW(wname.c_str(), &buf[0], len);
  if (got == 0 || got >= len) return std::string();
  buf.resize(got);
  return wide_to_utf8(buf);
}

// The process's argv as UTF-8, re-fetched from the exact UTF-16 command line
// (GetCommandLineW) instead of trusting the CRT's ACP conversion. Token 0 is
// the program path, exactly like argv[0].
inline std::vector<std::string> win_argv_utf8() {
  const std::wstring cmd(GetCommandLineW());
  const std::vector<std::wstring> parts = split_win_cmdline(cmd);
  std::vector<std::string> out;
  out.reserve(parts.size());
  for (const auto& p : parts) out.push_back(wide_to_utf8(p));
  return out;
}

#endif  // _WIN32

}  // namespace gs

#endif  // GIFSCYTHE_CORE_WINUNICODE_H
