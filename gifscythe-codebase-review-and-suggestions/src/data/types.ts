export type FindingSeed = {
  id: string;
  code: string;
  title: string;
  category: "broken" | "misaligned" | "missing" | "refuted" | "new";
  severity: "critical" | "major" | "minor" | "advisory";
  origin: "prior-audit" | "this-pass";
  verification: "reproduced" | "confirmed" | "refuted" | "partial";
  evidenceGrade: "V1" | "V2" | "V3";
  files: string;
  foundBy: string;
  symptom: string;
  rootCause: string;
  evidence: string;
  fix: string;
  phase: string;
  sortOrder: number;
};

export type DocumentSeed = {
  id: string;
  title: string;
  role: string;
  body: string;
  auditTake: string;
  sortOrder: number;
};

export type PlanSeed = {
  id: string;
  phase: string;
  title: string;
  detail: string;
  closes: string;
  estimate: string;
  definitionOfDone: string;
  sortOrder: number;
};

export type ClaimSeed = {
  id: string;
  claim: string;
  source: string;
  verdict: "holds" | "partial" | "fails";
  note: string;
  sortOrder: number;
};
