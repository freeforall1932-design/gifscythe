// GS-203: mirrors core/OutputVerify.h. Signature + metadata, not full decoding.
import { stat, readFile } from "node:fs/promises";
export const hasGifMagic = (buf) => buf.length >= 6
  && (buf.subarray(0, 6).equals(Buffer.from("GIF87a"))
      || buf.subarray(0, 6).equals(Buffer.from("GIF89a")));
export async function snapshotOutput(file) {
  try {
    const st = await stat(file, { bigint: true });
    return st.isFile() ? { exists: true, size: st.size, mtime: st.mtimeNs, error: null }
      : { error: "output is not an accessible regular file" };
  } catch (err) {
    if (err.code === "ENOENT") return { exists: false, error: null };
    return { error: "cannot snapshot output metadata" };
  }
}
export async function verifyOutput(file, before) {
  if (before.error) return { error: before.error };
  const after = await snapshotOutput(file);
  if (after.error) return { error: after.error };
  if (!after.exists || after.size === 0n)
    return { error: "the engine exited 0 but produced no output file" };
  if (before.exists && before.size === after.size && before.mtime === after.mtime)
    return { error: "output is unchanged since the pre-run snapshot" };
  let data;
  try { data = await readFile(file); } catch { return { error: "cannot read output file" }; }
  if (!hasGifMagic(data))
    return { error: "the engine exited 0 but produced invalid GIF output (expected GIF87a or GIF89a signature)" };
  return { data, error: null };
}
