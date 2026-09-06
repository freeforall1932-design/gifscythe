import {
  integer,
  pgTable,
  serial,
  text,
  timestamp,
} from "drizzle-orm/pg-core";

export const documents = pgTable("documents", {
  id: text("id").primaryKey(),
  title: text("title").notNull(),
  role: text("role").notNull(),
  body: text("body").notNull(),
  auditTake: text("audit_take").notNull(),
  sortOrder: integer("sort_order").notNull(),
});

export const findings = pgTable("findings", {
  id: text("id").primaryKey(),
  code: text("code").notNull(),
  title: text("title").notNull(),
  category: text("category").notNull(),
  severity: text("severity").notNull(),
  origin: text("origin").notNull(),
  verification: text("verification").notNull(),
  evidenceGrade: text("evidence_grade").notNull(),
  files: text("files").notNull(),
  foundBy: text("found_by").notNull(),
  symptom: text("symptom").notNull(),
  rootCause: text("root_cause").notNull(),
  evidence: text("evidence").notNull(),
  fix: text("fix").notNull(),
  phase: text("phase").notNull(),
  sortOrder: integer("sort_order").notNull(),
});

export const planItems = pgTable("plan_items", {
  id: text("id").primaryKey(),
  phase: text("phase").notNull(),
  title: text("title").notNull(),
  detail: text("detail").notNull(),
  closes: text("closes").notNull(),
  estimate: text("estimate").notNull(),
  definitionOfDone: text("definition_of_done").notNull(),
  sortOrder: integer("sort_order").notNull(),
});

export const notes = pgTable("notes", {
  id: serial("id").primaryKey(),
  findingId: text("finding_id")
    .notNull()
    .references(() => findings.id, { onDelete: "cascade" }),
  author: text("author").notNull(),
  body: text("body").notNull(),
  createdAt: timestamp("created_at", { withTimezone: true })
    .defaultNow()
    .notNull(),
});

export const claims = pgTable("claims", {
  id: text("id").primaryKey(),
  claim: text("claim").notNull(),
  source: text("source").notNull(),
  verdict: text("verdict").notNull(),
  note: text("note").notNull(),
  sortOrder: integer("sort_order").notNull(),
});
