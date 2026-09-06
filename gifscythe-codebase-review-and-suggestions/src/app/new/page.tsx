import { FindingCard } from "@/components/FindingCard";
import { Nav } from "@/components/Nav";
import { listFindings } from "@/lib/queries";

export const dynamic = "force-dynamic";

export default async function NewCutsPage() {
  const rows = await listFindings({ origin: "this-pass" });

  return (
    <div className="min-h-screen">
      <Nav active="/new" />
      <main className="mx-auto max-w-7xl px-5 py-10">
        <p className="mono text-xs uppercase tracking-[0.28em] text-[#ff8ad0]">
          Beyond the prior compilation
        </p>
        <h1 className="display mt-2 max-w-3xl text-5xl text-white">
          Sixteen cuts that were not on the previous docket.
        </h1>
        <p className="mt-4 max-w-3xl text-[#b7c0d0]">
          Prior audits caught the silent-false-success class. This pass still found
          a Windows engine that cannot compile against its own config.h, a GUI
          locator that never looks where the engine is built, a mapping table that
          would 10× every delay if followed, and a GifsicleCommand type that dangles
          on temporaries. Do these in P0/P1 alongside the original criticals.
        </p>
        <div className="mt-8 grid gap-4 md:grid-cols-2">
          {rows.map((finding) => (
            <FindingCard key={finding.id} {...finding} />
          ))}
        </div>
      </main>
    </div>
  );
}
