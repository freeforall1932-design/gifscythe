const SEV_LABEL: Record<string, string> = {
  critical: "critical",
  major: "major",
  minor: "minor",
  advisory: "advisory",
};

const CAT_LABEL: Record<string, string> = {
  broken: "broken",
  misaligned: "misaligned",
  missing: "missing logic",
  refuted: "refuted",
  new: "new this pass",
};

export function SeverityBadge({ severity }: { severity: string }) {
  return (
    <span
      className={`sev-${severity} inline-flex rounded-full border px-2 py-0.5 text-[10px] font-semibold uppercase tracking-[0.16em]`}
    >
      {SEV_LABEL[severity] ?? severity}
    </span>
  );
}

export function CategoryBadge({ category }: { category: string }) {
  return (
    <span className={`cat-${category} mono text-[11px] uppercase tracking-[0.18em]`}>
      {CAT_LABEL[category] ?? category}
    </span>
  );
}

export function VerifyBadge({ verification }: { verification: string }) {
  const tone =
    verification === "reproduced"
      ? "text-[#7dffb3]"
      : verification === "refuted"
        ? "text-[#ffb020]"
        : verification === "partial"
          ? "text-[#9aa8c2]"
          : "text-[#c9d2e3]";
  return (
    <span className={`mono text-[11px] uppercase tracking-[0.16em] ${tone}`}>
      {verification}
    </span>
  );
}
