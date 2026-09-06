import Link from "next/link";
import { notFound } from "next/navigation";
import { CategoryBadge, SeverityBadge, VerifyBadge } from "@/components/Badges";
import { Nav } from "@/components/Nav";
import { NoteForm } from "@/components/NoteForm";
import { getFinding } from "@/lib/queries";

export const dynamic = "force-dynamic";

export default async function FindingDetailPage({
  params,
}: {
  params: Promise<{ id: string }>;
}) {
  const { id } = await params;
  const data = await getFinding(decodeURIComponent(id));
  if (!data) notFound();
  const { finding, notes } = data;

  return (
    <div className="min-h-screen">
      <Nav active="/findings" />
      <main className="mx-auto max-w-5xl px-5 py-10">
        <Link href="/findings" className="text-sm text-[#8b97ad]">
          ← Docket
        </Link>
        <div className="mt-4 flex flex-wrap items-center gap-2">
          <span className="mono text-[#ff2d6a]">{finding.code}</span>
          <SeverityBadge severity={finding.severity} />
          <CategoryBadge category={finding.category} />
          <VerifyBadge verification={finding.verification} />
          <span className="mono text-[11px] text-[#8b97ad]">
            {finding.evidenceGrade} · {finding.origin}
          </span>
        </div>
        <h1 className="display mt-3 text-4xl text-white md:text-5xl">{finding.title}</h1>
        <p className="mono mt-4 text-sm text-[#8b97ad]">{finding.files}</p>
        <p className="mt-1 text-sm text-[#8b97ad]">Found by {finding.foundBy}</p>

        <div className="mt-8 grid gap-4">
          {[
            ["Symptom", finding.symptom],
            ["Root cause", finding.rootCause],
            ["Evidence this pass", finding.evidence],
            ["Fix", finding.fix],
          ].map(([label, body]) => (
            <section key={label} className="panel rounded-2xl p-6">
              <h2 className="mono text-xs uppercase tracking-[0.2em] text-[#7dffb3]">
                {label}
              </h2>
              <p className="mt-3 whitespace-pre-wrap text-[15px] leading-relaxed text-[#d5dce8]">
                {body}
              </p>
            </section>
          ))}
        </div>

        <p className="mono mt-6 text-xs text-[#8b97ad]">
          Phase {finding.phase || "n/a"}
        </p>

        <section className="mt-10">
          <h2 className="display text-3xl text-white">Notes</h2>
          <NoteForm findingId={finding.id} />
          <ul className="mt-4 space-y-3">
            {notes.map((note) => (
              <li key={note.id} className="panel rounded-2xl p-4">
                <p className="mono text-xs text-[#8b97ad]">
                  {note.author} ·{" "}
                  {note.createdAt instanceof Date
                    ? note.createdAt.toISOString()
                    : String(note.createdAt)}
                </p>
                <p className="mt-2 whitespace-pre-wrap text-sm leading-relaxed text-[#d5dce8]">
                  {note.body}
                </p>
              </li>
            ))}
          </ul>
        </section>
      </main>
    </div>
  );
}
