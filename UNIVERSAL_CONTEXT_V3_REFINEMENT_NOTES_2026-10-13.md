# Universal Agent Context v3: Refinement Record

**Reviewed:** 2026-10-13  
**Scope:** Universal, stack-agnostic process guidance only. No repository-specific paths, task IDs, current runtime versions, or product decisions belong in this file.

## Findings carried forward

The mandatory ecosystem-research section is already a strength, not a missing capability. It requires searching beyond saved references, across code hosts and community sources, inspecting comparable architectures and implementation evidence, checking license/maintenance/tests/CI/packaging, and ranking by fit instead of popularity alone. Preserve it. The refinement adds a proportionality rule so small fixes are not buried under heavyweight research and makes research coverage auditable without turning the 8–15 candidate guideline into a quota.

## Gaps refined

1. **Requirement authority vs. implementation evidence:** these are separate precedence orders. The latest explicit user decision controls desired behavior; fresh source/configuration and exact-revision test/CI evidence control claims about the current implementation.
2. **Freshness and provenance:** context snapshots must state review date and, for project facts, exact branch/ref/commit. A recent commit search is not proof of branch head; a historical green run is not current CI; connector access is not proof of a clean local working tree.
3. **Verification fallback:** when one tool cannot expose workflow runs, try an available repository-host API/connector path, then inspect jobs/logs. If no available path proves the state, document the limitation rather than infer.
4. **Version-sensitive toolchain checks:** verify actual action metadata/runtime, minimum runner version, SDK and target frameworks, release notes, referenced output paths, and all migration touchpoints. Do not infer compatibility only from a major version number.
5. **Research coverage reporting:** record source categories actually searched, query themes, direct evidence for candidates actually inspected, inaccessible sources, and unresolved evidence gaps. Scale effort to the decision's consequence rather than padding a shortlist.
6. **Context-pack coherence:** one authoritative universal guide, one project-specific overlay, brief chronological handoff, consistent version references, as-of dates, and an archive verified against the files it contains.
7. **Promotion of reusable knowledge:** project-specific discoveries stay in the overlay. Only generalized methods belong in the universal guide, with project names and incidental details removed.

## Refined order

1. Separate user requirement precedence from factual-state precedence.
2. Add provenance/freshness and remote-vs-local-vs-CI distinctions.
3. Strengthen CI and toolchain verification fallbacks and reporting.
4. Make ecosystem-research coverage explicit and proportional to the decision.
5. Add context pack version/source discipline and a cross-file consistency pass.
6. Repackage only after the universal guide and project overlay are independently coherent; verify filenames and archive contents.

This refinement keeps v3 as v3 because it strengthens process without replacing its architecture. The ecosystem-research requirement remains mandatory for substantial architecture/porting/tooling decisions and remains intentionally proportionate for small maintenance work.
