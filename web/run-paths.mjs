// GS-202 / P0-6: the /run API accepts file NAMES, never client paths.
// Apply the same portable policy on every host; reject rather than silently
// basename/sanitize (which could turn distinct uploads into one output).
import path from "node:path";

export function uploadNameError(name) {
  if (typeof name !== "string" || !name.trim()) return "a non-empty filename is required";
  if (name === "." || name === "..") return "dot path components are not filenames";
  if (/[\uD800-\uDFFF]/u.test(name)) return "unpaired Unicode surrogates are not allowed";
  if (/[<>:"/\\|?*\x00-\x1f\x7f]/u.test(name)) {
    return "paths, separators, control characters and Windows-special characters are not allowed";
  }
  if (/[. ]$/u.test(name)) return "trailing dots or spaces are not allowed";
  // Win32 device aliases apply even with an extension; superscript digits are
  // recognized as COM/LPT aliases by Windows as well.
  if (/^(con|prn|aux|nul|com[0-9¹²³]|lpt[0-9¹²³])(?:\.|$)/iu.test(name)) {
    return "reserved Windows device names are not allowed";
  }
  return null;
}

// Conservative, platform-independent collision policy, not a promise to emulate
// every filesystem's Unicode collation. Names keep their original spelling.
export const outputNameKey = (name) => name.normalize("NFC").toLowerCase();

// Separate from name admission: verify the FINAL resolved target, not a raw
// string prefix ("/tmp/gsweb-other" is not inside "/tmp/gsweb"). Parameterized
// so POSIX and Windows drive/UNC semantics can both be tested on Linux.
// Lexical containment assumes our private mkdtemp and a trusted engine; it is
// not a sandbox for a malicious engine or a hostile local symlink writer.
export function assertContainedPath(dir, target, paths = path) {
  const root = paths.resolve(dir);
  const resolved = paths.resolve(target);
  const rel = paths.relative(root, resolved);
  if (!rel || rel === ".." || rel.startsWith(`..${paths.sep}`) || paths.isAbsolute(rel)) {
    throw new Error("refusing to run: output path is outside the request temporary directory");
  }
  return resolved;
}

export function requestPath(dir, name) {
  const problem = uploadNameError(name);
  if (problem) throw new Error(`refusing unsafe output name: ${problem}`);
  return assertContainedPath(dir, path.resolve(dir, name));
}
