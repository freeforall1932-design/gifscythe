"use client";

import { usePathname, useRouter, useSearchParams } from "next/navigation";

const CATEGORIES = [
  { value: "all", label: "All" },
  { value: "broken", label: "Broken" },
  { value: "misaligned", label: "Misaligned" },
  { value: "missing", label: "Missing" },
  { value: "new", label: "New this pass" },
  { value: "refuted", label: "Refuted" },
];

const SEVERITIES = ["all", "critical", "major", "minor", "advisory"];

export function FilterBar() {
  const router = useRouter();
  const pathname = usePathname();
  const params = useSearchParams();

  function update(key: string, value: string) {
    const next = new URLSearchParams(params.toString());
    if (!value || value === "all") next.delete(key);
    else next.set(key, value);
    const query = next.toString();
    router.push(query ? `${pathname}?${query}` : pathname);
  }

  return (
    <div className="flex flex-wrap items-center gap-3">
      <div className="flex flex-wrap gap-1">
        {CATEGORIES.map((category) => {
          const current = params.get("category") ?? "all";
          const active = current === category.value;
          return (
            <button
              key={category.value}
              type="button"
              onClick={() => update("category", category.value)}
              className={`rounded-full px-3 py-1.5 text-sm ${
                active ? "bg-white text-black" : "bg-white/10 text-[#c9d2e3] hover:bg-white/15"
              }`}
            >
              {category.label}
            </button>
          );
        })}
      </div>
      <select
        value={params.get("severity") ?? "all"}
        onChange={(event) => update("severity", event.target.value)}
        className="rounded-full border border-white/10 bg-black/50 px-3 py-1.5 text-sm"
      >
        {SEVERITIES.map((severity) => (
          <option key={severity} value={severity}>
            {severity}
          </option>
        ))}
      </select>
      <form
        className="ml-auto"
        onSubmit={(event) => {
          event.preventDefault();
          const form = event.currentTarget;
          const value = String(new FormData(form).get("q") ?? "");
          update("q", value);
        }}
      >
        <input
          name="q"
          defaultValue={params.get("q") ?? ""}
          placeholder="Search code, file, symptom…"
          className="w-64 rounded-full border border-white/10 bg-black/50 px-4 py-1.5 text-sm outline-none focus:border-[#7dffb3]"
        />
      </form>
    </div>
  );
}
