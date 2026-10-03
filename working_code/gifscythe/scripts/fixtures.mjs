// fixtures.mjs - materialise the test images from their TEXT form (the JS twin of fixtures.sh).  (S34, N-36)
//
// The repo holds no binary files, so the two upstream test images (logo.gif, logo1.gif) live as base64 text in
// tests/fixtures/*.b64 with their sha256 in tests/fixtures/SHA256SUMS. fixturePath("logo.gif") decodes on first
// use into <product>/build/fixtures (gitignored), verifies the checksum, and returns the path - so a test
// reads the byte-identical upstream file. Throws on a mismatch rather than testing against wrong bytes.
import { createHash } from "node:crypto";
import { existsSync, mkdirSync, readFileSync, renameSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const product = join(here, "..");                 // working_code/gifscythe
const src = join(product, "tests", "fixtures");
export const FIXTURE_DIR = process.env.GS_FIXTURE_DIR || join(product, "build", "fixtures");

const sha256 = (buf) => createHash("sha256").update(buf).digest("hex");

function expectedSums() {
  const sums = new Map();
  for (const line of readFileSync(join(src, "SHA256SUMS"), "utf8").split("\n")) {
    const m = line.trim().match(/^([0-9a-f]{64})\s+\*?(\S+)$/);
    if (m) sums.set(m[2], m[1]);
  }
  return sums;
}

export function fixturePath(name) {
  const want = expectedSums().get(name);
  if (!want) throw new Error(`fixtures.mjs: no fixture named ${name} in ${join(src, "SHA256SUMS")}`);
  const out = join(FIXTURE_DIR, name);
  if (existsSync(out) && sha256(readFileSync(out)) === want) return out;
  mkdirSync(FIXTURE_DIR, { recursive: true });
  const bytes = Buffer.from(readFileSync(join(src, `${name}.b64`), "utf8"), "base64");
  if (sha256(bytes) !== want) throw new Error(`fixtures.mjs: ${name} decodes to the wrong bytes (sha256 mismatch)`);
  const tmp = `${out}.tmp`;
  writeFileSync(tmp, bytes);
  renameSync(tmp, out);
  return out;
}
