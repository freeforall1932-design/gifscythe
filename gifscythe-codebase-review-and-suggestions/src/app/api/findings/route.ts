import { listFindings } from "@/lib/queries";
import { NextResponse } from "next/server";

export const dynamic = "force-dynamic";

export async function GET(request: Request) {
  const url = new URL(request.url);
  const rows = await listFindings({
    category: url.searchParams.get("category") ?? undefined,
    severity: url.searchParams.get("severity") ?? undefined,
    origin: url.searchParams.get("origin") ?? undefined,
    q: url.searchParams.get("q") ?? undefined,
  });
  return NextResponse.json(rows);
}
