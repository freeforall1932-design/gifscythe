import { db } from "@/db";
import { claims, documents, findings, notes, planItems } from "@/db/schema";
import { ensureSeeded } from "@/lib/ensure-seeded";
import { and, desc, eq, ilike, or, sql } from "drizzle-orm";

export async function getStats() {
  await ensureSeeded();
  const [row] = await db
    .select({
      total: sql<number>`count(*)::int`,
      broken: sql<number>`count(*) filter (where ${findings.category} = 'broken')::int`,
      misaligned: sql<number>`count(*) filter (where ${findings.category} = 'misaligned')::int`,
      missing: sql<number>`count(*) filter (where ${findings.category} = 'missing')::int`,
      refuted: sql<number>`count(*) filter (where ${findings.category} = 'refuted')::int`,
      newCuts: sql<number>`count(*) filter (where ${findings.origin} = 'this-pass')::int`,
      critical: sql<number>`count(*) filter (where ${findings.severity} = 'critical')::int`,
      reproduced: sql<number>`count(*) filter (where ${findings.verification} = 'reproduced')::int`,
    })
    .from(findings);

  return (
    row ?? {
      total: 0,
      broken: 0,
      misaligned: 0,
      missing: 0,
      refuted: 0,
      newCuts: 0,
      critical: 0,
      reproduced: 0,
    }
  );
}

export async function listFindings(opts: {
  category?: string;
  severity?: string;
  origin?: string;
  q?: string;
}) {
  await ensureSeeded();
  const filters = [];
  if (opts.category && opts.category !== "all") {
    if (opts.category === "new") {
      filters.push(eq(findings.origin, "this-pass"));
    } else {
      filters.push(eq(findings.category, opts.category));
    }
  }
  if (opts.severity && opts.severity !== "all") {
    filters.push(eq(findings.severity, opts.severity));
  }
  if (opts.origin && opts.origin !== "all") {
    filters.push(eq(findings.origin, opts.origin));
  }
  if (opts.q) {
    const needle = `%${opts.q}%`;
    filters.push(
      or(
        ilike(findings.title, needle),
        ilike(findings.code, needle),
        ilike(findings.files, needle),
        ilike(findings.symptom, needle),
      ),
    );
  }

  return db
    .select()
    .from(findings)
    .where(filters.length ? and(...filters) : undefined)
    .orderBy(findings.sortOrder);
}

export async function getFinding(id: string) {
  await ensureSeeded();
  const [finding] = await db
    .select()
    .from(findings)
    .where(eq(findings.id, id))
    .limit(1);
  if (!finding) return null;
  const findingNotes = await db
    .select()
    .from(notes)
    .where(eq(notes.findingId, id))
    .orderBy(desc(notes.createdAt));
  return { finding, notes: findingNotes };
}

export async function listDocuments() {
  await ensureSeeded();
  return db.select().from(documents).orderBy(documents.sortOrder);
}

export async function listPlan() {
  await ensureSeeded();
  return db.select().from(planItems).orderBy(planItems.sortOrder);
}

export async function listClaims() {
  await ensureSeeded();
  return db.select().from(claims).orderBy(claims.sortOrder);
}

export async function recentNotes(limit = 8) {
  await ensureSeeded();
  return db
    .select({
      id: notes.id,
      findingId: notes.findingId,
      author: notes.author,
      body: notes.body,
      createdAt: notes.createdAt,
      title: findings.title,
      code: findings.code,
    })
    .from(notes)
    .innerJoin(findings, eq(notes.findingId, findings.id))
    .orderBy(desc(notes.createdAt))
    .limit(limit);
}
