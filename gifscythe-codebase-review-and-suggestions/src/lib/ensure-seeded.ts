import { db } from "@/db";
import { claims, documents, findings, planItems } from "@/db/schema";
import {
  allFindings,
  claims as claimSeed,
  documents as documentSeed,
  planItems as planSeed,
} from "@/data";
import { sql } from "drizzle-orm";

let seedPromise: Promise<void> | null = null;

async function seed() {
  const [row] = await db
    .select({ count: sql<number>`count(*)::int` })
    .from(findings);

  if ((row?.count ?? 0) > 0) return;

  if (documentSeed.length) {
    await db.insert(documents).values(documentSeed);
  }
  if (allFindings.length) {
    await db.insert(findings).values(allFindings);
  }
  if (planSeed.length) {
    await db.insert(planItems).values(planSeed);
  }
  if (claimSeed.length) {
    await db.insert(claims).values(claimSeed);
  }
}

export async function ensureSeeded() {
  if (!seedPromise) {
    seedPromise = seed().catch((error) => {
      seedPromise = null;
      throw error;
    });
  }
  await seedPromise;
}
