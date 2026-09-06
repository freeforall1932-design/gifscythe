import { Suspense } from "react";
import { FilterBar } from "@/components/FilterBar";
import { FindingCard } from "@/components/FindingCard";
import { Nav } from "@/components/Nav";
import { listFindings } from "@/lib/queries";

export const dynamic = "force-dynamic";

export default async function FindingsPage({
  searchParams,
}: {
  searchParams: Promise<{ category?: string; severity?: string; origin?: string; q?: string }>;
}) {
  const params = await searchParams;
  const rows = await listFindings(params);

  return (
    <div className="min-h-screen">
      <Nav active="/findings" />
      <main className="mx-auto max-w-7xl px-5 py-10">
        <p className="mono text-xs uppercase tracking-[0.28em] text-[#7dffb3]">
          Master docket
        </p>
        <h1 className="display mt-2 text-5xl text-white">Findings</h1>
        <p className="mt-3 max-w-2xl text-[#b7c0d0]">
          Prior-audit items re-verified against the clone, plus new cuts from this
          pass. Refuted items stay on the docket so nobody “fixes” correct flag
          mappings.
        </p>
        <div className="mt-6">
          <Suspense fallback={null}>
            <FilterBar />
          </Suspense>
        </div>
        <p className="mono mt-4 text-xs text-[#8b97ad]">{rows.length} filed</p>
        <div className="mt-6 grid gap-4 md:grid-cols-2">
          {rows.map((finding) => (
            <FindingCard key={finding.id} {...finding} />
          ))}
        </div>
      </main>
    </div>
  );
}
