import Link from "next/link";
import { Nav } from "@/components/Nav";
import { listPlan } from "@/lib/queries";

export const dynamic = "force-dynamic";

const PHASE_COPY: Record<string, { title: string; lede: string }> = {
  P0: {
    title: "P0 — Stop the silent failures",
    lede: "Before any feature work. Every critical shares one property: the tool says success while doing wrong or nothing.",
  },
  P1: {
    title: "P1 — Make the code honest",
    lede: "One truth per concept: version, engine identity, output directory, live pane, settings round-trip.",
  },
  P2: {
    title: "P2 — Prove it",
    lede: "Smoke suite in CI on both OSes, artifacts, async QProcess. Checkmarks become trustworthy again.",
  },
  P3: {
    title: "P3 — Retrofit → 1.0.0",
    lede: "Only after P0–P2 evidence. WebP/APNG stay in the bucket. Version stays 0.x until a clean Windows machine double-click works.",
  },
};

export default async function PlanPage() {
  const items = await listPlan();
  const phases = ["P0", "P1", "P2", "P3"];

  return (
    <div className="min-h-screen">
      <Nav active="/plan" />
      <main className="mx-auto max-w-5xl px-5 py-10">
        <p className="mono text-xs uppercase tracking-[0.28em] text-[#ffb020]">
          Sequenced against the project&apos;s own rules
        </p>
        <h1 className="display mt-2 text-5xl text-white">What you should do</h1>
        <p className="mt-4 max-w-2xl text-[#b7c0d0]">
          Do not rewrite the core. Do not implement the secondary audit&apos;s
          N1/N2/N14 “argument logic” fixes. Do not bump 1.0.0. Cut silent false
          success first, then make one truth per concept, then prove it in CI.
        </p>

        <div className="mt-10 space-y-12">
          {phases.map((phase) => {
            const copy = PHASE_COPY[phase] ?? {
              title: phase,
              lede: "",
            };
            const group = items.filter((item) => item.phase === phase);
            return (
              <section key={phase}>
                <h2 className="display text-3xl text-white">{copy.title}</h2>
                <p className="mt-2 text-[#b7c0d0]">{copy.lede}</p>
                <div className="mt-5 space-y-4">
                  {group.map((item) => (
                    <article key={item.id} className="panel rounded-2xl p-5">
                      <div className="flex flex-wrap items-baseline justify-between gap-2">
                        <h3 className="text-xl font-semibold text-white">
                          <span className="mono mr-2 text-sm text-[#ff2d6a]">
                            {item.id}
                          </span>
                          {item.title}
                        </h3>
                        <span className="mono text-xs text-[#8b97ad]">{item.estimate}</span>
                      </div>
                      <p className="mt-3 text-sm leading-relaxed text-[#d5dce8]">
                        {item.detail}
                      </p>
                      <p className="mt-3 text-sm text-[#9aa8c2]">
                        Closes {item.closes}
                      </p>
                      <p className="mt-2 text-sm text-[#7dffb3]">
                        Done when: {item.definitionOfDone}
                      </p>
                    </article>
                  ))}
                </div>
              </section>
            );
          })}
        </div>

        <p className="mt-12 text-sm text-[#8b97ad]">
          Full finding write-ups live on the{" "}
          <Link href="/findings" className="text-[#7dffb3]">
            docket
          </Link>
          . Context docs on{" "}
          <Link href="/docs" className="text-[#7dffb3]">
            Context
          </Link>
          .
        </p>
      </main>
    </div>
  );
}
