import { Nav } from "@/components/Nav";
import { listClaims, listDocuments } from "@/lib/queries";

export const dynamic = "force-dynamic";

export default async function DocsPage() {
  const [docs, claimRows] = await Promise.all([listDocuments(), listClaims()]);

  return (
    <div className="min-h-screen">
      <Nav active="/docs" />
      <main className="mx-auto max-w-5xl px-5 py-10">
        <p className="mono text-xs uppercase tracking-[0.28em] text-[#8bb4ff]">
          Read as a system
        </p>
        <h1 className="display mt-2 text-5xl text-white">Four documents + feasibility</h1>
        <p className="mt-4 max-w-2xl text-[#b7c0d0]">
          Vision, worklist, handoff, improvement log — plus the architecture review
          they all defer to. The failure mode is green-checkmark inflation: claims
          marked done without machine-checked proof.
        </p>

        <section className="panel mt-8 rounded-2xl p-5">
          <h2 className="mono text-xs uppercase tracking-[0.2em] text-[#7dffb3]">
            Claim vs code
          </h2>
          <ul className="mt-4 divide-y divide-white/10">
            {claimRows.map((claim) => (
              <li key={claim.id} className="py-3">
                <div className="flex flex-wrap items-center justify-between gap-2">
                  <p className="font-medium text-white">{claim.claim}</p>
                  <span
                    className={`mono text-[11px] uppercase tracking-[0.16em] ${
                      claim.verdict === "holds"
                        ? "text-[#7dffb3]"
                        : claim.verdict === "fails"
                          ? "text-[#ff2d6a]"
                          : "text-[#ffb020]"
                    }`}
                  >
                    {claim.verdict} · {claim.source}
                  </span>
                </div>
                <p className="mt-1 text-sm text-[#b7c0d0]">{claim.note}</p>
              </li>
            ))}
          </ul>
        </section>

        <div className="mt-8 space-y-5">
          {docs.map((doc) => (
            <article key={doc.id} className="panel rounded-2xl p-6">
              <p className="mono text-xs uppercase tracking-[0.2em] text-[#8b97ad]">
                {doc.role}
              </p>
              <h2 className="mt-1 text-2xl font-semibold text-white">{doc.title}</h2>
              <p className="mt-4 whitespace-pre-wrap text-sm leading-relaxed text-[#c9d2e3]">
                {doc.body}
              </p>
              <p className="mt-4 rounded-xl border border-[#7dffb3]/20 bg-[#7dffb3]/5 p-4 text-sm leading-relaxed text-[#d5fbe6]">
                {doc.auditTake}
              </p>
            </article>
          ))}
        </div>
      </main>
    </div>
  );
}
