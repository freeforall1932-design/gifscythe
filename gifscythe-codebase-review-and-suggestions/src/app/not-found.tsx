import Link from "next/link";
import { Nav } from "@/components/Nav";

export default function NotFound() {
  return (
    <div className="min-h-screen">
      <Nav active="" />
      <main className="mx-auto max-w-3xl px-5 py-20">
        <p className="mono text-xs uppercase tracking-[0.28em] text-[#ff2d6a]">
          missing frame
        </p>
        <h1 className="display mt-3 text-5xl text-white">That cut is not on the docket.</h1>
        <Link href="/findings" className="mt-6 inline-block text-[#7dffb3]">
          Back to findings
        </Link>
      </main>
    </div>
  );
}
