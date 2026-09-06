"use client";

import { useRouter } from "next/navigation";
import { useState, type FormEvent } from "react";

export function NoteForm({ findingId }: { findingId: string }) {
  const router = useRouter();
  const [author, setAuthor] = useState("investigator");
  const [body, setBody] = useState("");
  const [status, setStatus] = useState<"idle" | "saving" | "error">("idle");

  async function onSubmit(event: FormEvent) {
    event.preventDefault();
    if (!body.trim()) return;
    setStatus("saving");
    const response = await fetch("/api/notes", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ findingId, author, body }),
    });
    if (!response.ok) {
      setStatus("error");
      return;
    }
    setBody("");
    setStatus("idle");
    router.refresh();
  }

  return (
    <form onSubmit={onSubmit} className="panel mt-4 rounded-2xl p-4">
      <div className="flex flex-wrap gap-3">
        <input
          value={author}
          onChange={(event) => setAuthor(event.target.value)}
          className="w-40 rounded-lg border border-white/10 bg-black/40 px-3 py-2 text-sm outline-none focus:border-[#7dffb3]"
          placeholder="author"
        />
        <button
          type="submit"
          disabled={status === "saving"}
          className="rounded-lg bg-[#7dffb3] px-4 py-2 text-sm font-semibold text-[#072012] disabled:opacity-60"
        >
          {status === "saving" ? "Filing…" : "File note"}
        </button>
      </div>
      <textarea
        value={body}
        onChange={(event) => setBody(event.target.value)}
        rows={4}
        placeholder="Add a verification note, reproduction, or disagreement…"
        className="mt-3 w-full rounded-lg border border-white/10 bg-black/40 px-3 py-2 text-sm leading-relaxed outline-none focus:border-[#7dffb3]"
      />
      {status === "error" ? (
        <p className="mt-2 text-sm text-[#ff6b8a]">Could not file the note.</p>
      ) : null}
    </form>
  );
}
