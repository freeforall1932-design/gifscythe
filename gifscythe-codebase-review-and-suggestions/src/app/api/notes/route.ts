import { db } from "@/db";
import { findings, notes } from "@/db/schema";
import { ensureSeeded } from "@/lib/ensure-seeded";
import { eq } from "drizzle-orm";
import { NextResponse } from "next/server";

export const dynamic = "force-dynamic";

export async function GET() {
  await ensureSeeded();
  const rows = await db.select().from(notes);
  return NextResponse.json(rows);
}

export async function POST(request: Request) {
  await ensureSeeded();
  const payload = (await request.json()) as {
    findingId?: string;
    author?: string;
    body?: string;
  };
  const findingId = payload.findingId?.trim();
  const body = payload.body?.trim();
  const author = payload.author?.trim() || "investigator";
  if (!findingId || !body) {
    return NextResponse.json({ error: "findingId and body required" }, { status: 400 });
  }
  const [exists] = await db
    .select({ id: findings.id })
    .from(findings)
    .where(eq(findings.id, findingId))
    .limit(1);
  if (!exists) {
    return NextResponse.json({ error: "finding not found" }, { status: 404 });
  }
  const [created] = await db
    .insert(notes)
    .values({ findingId, author, body })
    .returning();
  return NextResponse.json(created, { status: 201 });
}
