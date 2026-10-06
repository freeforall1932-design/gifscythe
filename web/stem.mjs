// stem.mjs — ONE `stemOf`, shared by the browser (app.js) and the server
// (server.mjs). Audit U-96 / P3-18.
//
// WHY this file exists: `stemOf` used to be copy-pasted into both files, and
// its dotfile/extensionless boundary was never pinned against the desktop's
// rule (Qt's QFileInfo::completeBaseName, which MainWindow.cpp uses for
// `<stem>_opt.gif` and `<stem>_frame`). A duplicated rule with an unpinned
// boundary means the web can silently write DIFFERENT names than the desktop
// for odd upload names — the same failure class as U-56's duplicated device
// rule, which is why that one also lives in a single shared table.
//
// The semantics below are MEASURED from Qt, not guessed: the rows in
// working_code/gifscythe/tests/stem_cases.txt were produced by running
// QFileInfo::completeBaseName() over that exact name list on Qt 6.8.3, and
// tests/stem.test.mjs asserts this function matches every row — as does the
// Qt-side case in the offscreen harness (T25), so a change on either side
// makes the other go red instead of drifting. The measurement is what fixed
// the dotfile boundary: the hand-written rule this file replaced got `.gif`
// and `.hidden` wrong (Qt gives them an EMPTY stem).
//
// Run: node web/test/stem.test.mjs

/**
 * Strip the extension the way Qt's QFileInfo::completeBaseName() does:
 * everything after the LAST dot, and NOTHING when there is no dot at all.
 *
 * MEASURED, not assumed: Qt 6.8.3 treats a LEADING dot as an extension
 * separator too, so `".gif"` and `".hidden"` have an EMPTY stem (their suffix
 * is "gif"/"hidden"), and `"a."` -> `"a"`, `".."` -> `"."`. The rule that used
 * to live here (`i > 0`) got the dotfile rows wrong and would have written
 * `.gif_opt.gif` where the desktop writes `_opt.gif`; the exact values are in
 * working_code/gifscythe/tests/stem_cases.txt and both this side and Qt's side
 * are asserted against it (web/test/stem.test.mjs, harness case T25).
 *
 * @param {string} name a bare file name (no directory part; callers pass
 *   `.name` from a File or a basename)
 * @returns {string} the stem, which may be "" for a dotfile
 */
export const stemOf = (name) => {
  const s = String(name);
  const i = s.lastIndexOf(".");
  // -1 means no dot at all: the whole name is the stem. Any other index —
  // INCLUDING 0, the dotfile case — is an extension separator.
  return i >= 0 ? s.slice(0, i) : s;
};

export default stemOf;
