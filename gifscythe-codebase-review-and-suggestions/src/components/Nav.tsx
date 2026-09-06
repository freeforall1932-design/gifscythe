import Image from "next/image";
import Link from "next/link";

const LINKS = [
  { href: "/", label: "Overview" },
  { href: "/findings", label: "Findings" },
  { href: "/new", label: "New cuts" },
  { href: "/plan", label: "Do this next" },
  { href: "/docs", label: "Context" },
];

export function Nav({ active }: { active: string }) {
  return (
    <header className="sticky top-0 z-40 border-b border-white/10 bg-[#07080c]/80 backdrop-blur-xl">
      <div className="mx-auto flex max-w-7xl items-center gap-6 px-5 py-3">
        <Link href="/" className="flex items-center gap-3">
          <Image
            src="/images/mark.png"
            alt="Gifscythe audit mark"
            width={36}
            height={36}
            className="size-9 rounded-md border border-white/10 object-cover"
          />
          <span className="leading-tight">
            <span className="block font-semibold tracking-tight">CUT ROOM</span>
            <span className="block text-[11px] uppercase tracking-[0.22em] text-[#8b97ad]">
              gifscythe audit
            </span>
          </span>
        </Link>
        <nav className="ml-auto flex flex-wrap items-center gap-1 text-sm">
          {LINKS.map((link) => {
            const isActive =
              link.href === "/" ? active === "/" : active.startsWith(link.href);
            return (
              <Link
                key={link.href}
                href={link.href}
                className={`rounded-full px-3 py-1.5 transition ${
                  isActive
                    ? "bg-white text-[#07080c]"
                    : "text-[#c9d2e3] hover:bg-white/10"
                }`}
              >
                {link.label}
              </Link>
            );
          })}
        </nav>
      </div>
    </header>
  );
}
