import Link from "next/link";
import { CategoryBadge, SeverityBadge, VerifyBadge } from "@/components/Badges";

type FindingCardProps = {
  id: string;
  code: string;
  title: string;
  category: string;
  severity: string;
  origin: string;
  verification: string;
  files: string;
  symptom: string;
};

export function FindingCard(finding: FindingCardProps) {
  return (
    <Link
      href={`/findings/${encodeURIComponent(finding.id)}`}
      className="panel group block rounded-2xl p-5 transition hover:border-[#ff2d6a]/40 hover:bg-white/[0.04]"
    >
      <div className="flex flex-wrap items-center gap-2">
        <span className="mono text-xs text-[#ff2d6a]">{finding.code}</span>
        <SeverityBadge severity={finding.severity} />
        <CategoryBadge category={finding.category} />
        <VerifyBadge verification={finding.verification} />
        {finding.origin === "this-pass" ? (
          <span className="rounded-full bg-[#ff2d6a]/15 px-2 py-0.5 text-[10px] uppercase tracking-[0.16em] text-[#ff8ad0]">
            new cut
          </span>
        ) : null}
      </div>
      <h3 className="mt-3 text-lg font-semibold tracking-tight text-white group-hover:text-[#7dffb3]">
        {finding.title}
      </h3>
      <p className="mt-2 line-clamp-3 text-sm leading-relaxed text-[#b7c0d0]">
        {finding.symptom}
      </p>
      <p className="mono mt-3 truncate text-[11px] text-[#8b97ad]">{finding.files}</p>
    </Link>
  );
}
