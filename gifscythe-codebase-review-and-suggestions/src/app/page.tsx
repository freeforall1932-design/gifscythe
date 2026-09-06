import Image from "next/image";
import Link from "next/link";
import { HEART_FACTS } from "@/data";
import { Nav } from "@/components/Nav";
import { getStats, listClaims, listPlan, recentNotes } from "@/lib/queries";

export const dynamic = "force-dynamic";

export default async function HomePage() {
  const [stats, claimRows, plan, notes] = await Promise.all([
    getStats(),
    listClaims(),
    listPlan(),
    recentNotes(5),
  ]);
  const p0 = plan.filter((item) => item.phase === "P0");

  return (
    <div className="min-h-screen bg-[#07080c] text-[#e8edf7]">
      <Nav active="/" />
      <main>
        <section className="relative overflow-hidden">
          <Image
            src="/images/hero.png"
            alt="Cutting-room light table with GIF frames and a scythe"
            width={1600}
            height={900}
            priority
            className="h-[520px] w-full object-cover opacity-70"
          />
          <div className="absolute inset-0 bg-gradient-to-t from-[#07080c] via-[#07080c]/55 to-transparent" />
          <div className="absolute inset-0 scanlines" />
          <div className="absolute bottom-0 left-0 right-0 mx-auto max-w-7xl px-5 pb-12">
            <p className="mono text-xs uppercase tracking-[0.28em] text-[#7dffb3]">
              freeforall1932-design/gifscythe · main · v0.1.0
            </p>
            <h1 className="display mt-3 max-w-4xl text-5xl leading-[0.95] text-white md:text-7xl">
              The product reports success while doing the wrong thing.
            </h1>
            <p className="mt-5 max-w-2xl text-lg text-[#c9d2e3]">
              Independent re-read of the four context docs, live reproduction of
              the flagged criticals, and sixteen new cuts the prior audits missed.
              Architecture is sound. Silent false success is not.
            </p>
          </div>
        </section>

        <section className="mx-auto grid max-w-7xl gap-4 px-5 py-10 md:grid-cols-4">
          {[
            ["Broken at runtime", stats.broken, "wrong result / false green"],
            ["Misaligned", stats.misaligned, "code vs its own labels"],
            ["Missing logic", stats.missing, "promised, never written"],
            ["New this pass", stats.newCuts, "not in the prior compilation"],
          ].map(([label, value, hint]) => (
            <div key={String(label)} className="panel rounded-2xl p-5">
              <p className="mono text-[11px] uppercase tracking-[0.2em] text-[#8b97ad]">
                {label}
              </p>
              <p className="display mt-2 text-5xl text-white">{value}</p>
              <p className="mt-2 text-sm text-[#8b97ad]">{hint}</p>
            </div>
          ))}
        </section>

        <section className="mx-auto grid max-w-7xl gap-8 px-5 pb-16 lg:grid-cols-[1.4fr_0.8fr]">
          <div>
            <div className="mb-4 flex items-end justify-between">
              <h2 className="display text-3xl text-white">Five facts at the heart</h2>
              <Link href="/findings" className="text-sm text-[#7dffb3]">
                All findings →
              </Link>
            </div>
            <div className="grid gap-3">
              {HEART_FACTS.map((fact, index) => (
                <Link
                  key={fact.id}
                  href={`/findings/${fact.id}`}
                  className="panel flex gap-4 rounded-2xl p-4 hover:border-[#7dffb3]/40"
                >
                  <span className="display text-2xl text-[#ff2d6a]">0{index + 1}</span>
                  <span>
                    <span className="mono text-xs text-[#ff2d6a]">{fact.id}</span>
                    <span className="mt-1 block font-semibold text-white">
                      {fact.title}
                    </span>
                    <span className="mt-1 block text-sm text-[#b7c0d0]">
                      {fact.detail}
                    </span>
                  </span>
                </Link>
              ))}
            </div>
          </div>

          <aside className="space-y-4">
            <div className="panel overflow-hidden rounded-2xl">
              <Image
                src="/images/false-success.png"
                alt="Cracked green checkmark over a dissolving GIF"
                width={800}
                height={800}
                className="h-48 w-full object-cover"
              />
              <div className="p-5">
                <p className="mono text-[11px] uppercase tracking-[0.2em] text-[#ff2d6a]">
                  Verdict
                </p>
                <p className="mt-2 text-lg font-semibold text-white">
                  Keep the architecture. Stop shipping green checkmarks.
                </p>
                <p className="mt-2 text-sm leading-relaxed text-[#b7c0d0]">
                  {stats.reproduced} findings reproduced or confirmed against the
                  clone this pass. {stats.refuted} secondary-audit claims must not
                  be “fixed” — they would break correct gifsicle mappings.
                </p>
              </div>
            </div>
            <div className="panel rounded-2xl p-5">
              <p className="mono text-[11px] uppercase tracking-[0.2em] text-[#8b97ad]">
                Doc claims
              </p>
              <ul className="mt-3 space-y-2">
                {claimRows.map((claim) => (
                  <li key={claim.id} className="flex items-start justify-between gap-3">
                    <span className="text-sm text-[#c9d2e3]">{claim.claim}</span>
                    <span
                      className={`mono shrink-0 text-[10px] uppercase tracking-[0.14em] ${
                        claim.verdict === "holds"
                          ? "text-[#7dffb3]"
                          : claim.verdict === "fails"
                            ? "text-[#ff2d6a]"
                            : "text-[#ffb020]"
                      }`}
                    >
                      {claim.verdict}
                    </span>
                  </li>
                ))}
              </ul>
            </div>
          </aside>
        </section>

        <section className="border-y border-white/10 bg-[#0c1018]">
          <div className="mx-auto max-w-7xl px-5 py-14">
            <p className="mono text-xs uppercase tracking-[0.28em] text-[#ffb020]">
              What you should do
            </p>
            <h2 className="display mt-2 max-w-3xl text-4xl text-white">
              P0 before any feature work. Version stays 0.x. WebP stays in the bucket.
            </h2>
            <div className="mt-8 grid gap-4 md:grid-cols-2">
              {p0.map((item) => (
                <article key={item.id} className="panel rounded-2xl p-5">
                  <p className="mono text-xs text-[#ff2d6a]">{item.id}</p>
                  <h3 className="mt-2 text-xl font-semibold text-white">{item.title}</h3>
                  <p className="mt-2 text-sm leading-relaxed text-[#b7c0d0]">
                    {item.detail}
                  </p>
                  <p className="mono mt-3 text-[11px] text-[#8b97ad]">
                    Closes {item.closes} · {item.estimate}
                  </p>
                </article>
              ))}
            </div>
            <Link
              href="/plan"
              className="mt-8 inline-flex rounded-full bg-[#7dffb3] px-5 py-2 text-sm font-semibold text-[#072012]"
            >
              Full sequenced plan
            </Link>
          </div>
        </section>

        <section className="mx-auto max-w-7xl px-5 py-14">
          <div className="flex items-end justify-between">
            <h2 className="display text-3xl text-white">Investigator notes</h2>
            <Link href="/findings" className="text-sm text-[#8b97ad]">
              File against a finding
            </Link>
          </div>
          {notes.length === 0 ? (
            <p className="mt-4 text-[#8b97ad]">
              No notes yet. Open a finding and file a reproduction or disagreement.
            </p>
          ) : (
            <ul className="mt-6 grid gap-3 md:grid-cols-2">
              {notes.map((note) => (
                <li key={note.id} className="panel rounded-2xl p-4">
                  <p className="mono text-xs text-[#ff2d6a]">
                    {note.code} · {note.author}
                  </p>
                  <p className="mt-1 font-medium text-white">{note.title}</p>
                  <p className="mt-2 text-sm text-[#b7c0d0]">{note.body}</p>
                </li>
              ))}
            </ul>
          )}
        </section>
      </main>
    </div>
  );
}
