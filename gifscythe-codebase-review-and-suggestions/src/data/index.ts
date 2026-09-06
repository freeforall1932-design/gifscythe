import { brokenFindings } from "./findings-broken";
import { misalignedFindings } from "./findings-misaligned";
import { missingFindings } from "./findings-missing";
import { refutedFindings } from "./findings-refuted";
import { newFindings } from "./findings-new";
import { documents } from "./docs";
import { claims, planItems } from "./plan";
import type { FindingSeed } from "./types";

export { documents, claims, planItems };
export type { FindingSeed } from "./types";

export const allFindings: FindingSeed[] = [
  ...brokenFindings,
  ...misalignedFindings,
  ...missingFindings,
  ...refutedFindings,
  ...newFindings,
];

export const HEART_FACTS = [
  {
    id: "U-B1",
    title: "CLI --run exits 0 when the engine does not exist",
    detail:
      "system() wait-status 32512 = 127<<8 wraps to 0 through C's 8-bit exit truncation. Reproduced this pass: print says 32512, $? is 0.",
  },
  {
    id: "U-B2",
    title: "Engine is only found from one working directory",
    detail:
      "README usage from working_code/gifscythe fails. Absolute path from repo root writes a 9,458-byte GIF. Same process, two CWDs, opposite outcomes.",
  },
  {
    id: "U-B3",
    title: "GUI welds the queue into one GIF",
    detail:
      "Mode::Merge is hardcoded. logo.gif (12) + logo1.gif (1) → 13-image animation. Users asked to optimize two files.",
  },
  {
    id: "U-B4",
    title: "Empty output vaporizes the result, then reports success",
    detail:
      "gifsicle writes 821 bytes to stdout. MainWindow never reads stdout. Status: 'Optimization complete.'",
  },
  {
    id: "U-B7",
    title: "CMake cannot configure at all",
    detail:
      "STATIC library with only a header: no link language. build.sh hides it with >/dev/null 2>&1. Linux CI can be green without a GUI.",
  },
] as const;
