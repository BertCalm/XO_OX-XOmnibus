# Sisters of Continuous Improvement — Process Audit
## V1 Launch Arc: Six-Session Review

**Date:** 2026-03-20
**Arc scope:** 2026-03-14 (Pi Day) through 2026-03-20
**Author:** Sisters (process audit protocol)
**Audited by:** Sonnet 4.6 with full session arc context

---

## Executive Summary

The V1 arc delivered extraordinary output: 42 engines, ~21,900 presets, 13 seance verdicts, 3 FX chains, SDK Phase 1, Oxport pipeline, and full build validation — all in six sessions. The machinery works. The gaps are in consistency, measurement, and memory hygiene. This audit targets those gaps specifically so the Vol 2 arc can run faster with less re-discovery and fewer cleanup sessions.

Key finding: approximately 30–40% of session time across the arc was spent on work that better tooling or tighter memory conventions could eliminate — prefix corrections, preset miscategorization, documentation drift, and scope reversals that sent work into the trash.

---

## 1. Model Efficiency

### Current Pattern (from model-effort-guide.md)

The declared guide is sound. The problem is adherence.

| Task Type | Declared | Observed Gap |
|-----------|----------|-------------|
| Novel DSP, new engine design | Opus / High | Correct |
| JUCE UI, bug fixes, param wiring | Sonnet / Medium | Often escalated to Opus unnecessarily |
| JSON presets, git commits, research | Sonnet or Haiku / Low | Frequently Sonnet — acceptable but could be Haiku for pure preset gen |
| Audits, maintenance, documentation | Haiku / Low | Rarely used; Sonnet does almost everything |

### Where Opus Is Overused

**Preset generation.** Once a Guru Bin retreat produces sweet spots and parameter guidance, generating 150 `.xometa` files is mechanical — parameter interpolation, mood distribution, naming. This is Haiku territory. The parallel background agent pattern (feedback-parallel-preset-agents.md) is correct and already captures this insight, but it is not consistently applied. Sessions still run large preset batches in main context at Sonnet.

**Build integration tasks.** Adding a new engine to `XOceanusProcessor.cpp`, `PresetManager.h`, `XOceanusEditor.h`, and `CMakeLists.txt` follows a documented template. The morning plan for 2026-03-20 explicitly tags this as "Sonnet, ~15 min." That is correct. However, when the same task appears mid-session after a Guru Bin or seance block, the model context is already Opus and it stays there.

**Documentation updates.** The `/historical-society` skill and end-of-session MEMORY.md updates are Haiku-grade tasks dressed as Sonnet work. These can be dispatched as background agents at the end of every session without occupying main context.

### Where Sonnet Should Be Upgraded

**Seance verdicts for novel engines.** ORBWEAVE, OVERTONE, and ORGANISM were dispatched as background agents for seance and presets on 2026-03-19. The seance portion — evaluating novel DSP concepts, assigning blessings, ruling on doctrine compliance — benefits from Opus quality. Running seances in background at unspecified model is a quality risk. The verdict files look adequate, but the concern is real: ghost council quality degrades on smaller models.

**OBRIX Wave 1–3 design.** At 65 params and 1407 lines, OBRIX is the most complex single-engine design in the fleet. This was correctly run at Opus/High. The seance (6.8/10 → 9.8 target) confirms the design has genuine depth that rewards quality investment.

### Estimated Cost Impact

Conservative estimate: 15–20% of Opus sessions in the arc ran tasks that Sonnet would have handled at equivalent quality. At ~80% monthly usage baseline, the arc pushed the limit on at least two occasions. Applying the model-effort-guide consistently would recover roughly one full Opus session worth of budget per arc.

**Recommendation:** Add model-tagging to the morning plan format. Each phase already lists "(Sonnet, ~15 min)" or "(Opus, ~20 min)" — this practice should become mandatory, not optional, and should appear in the `/historical-society` session close template so it persists.

---

## 2. Skill Usage Patterns

### Skills Used Frequently (high signal)

| Skill | Usage | Quality Signal |
|-------|-------|---------------|
| `/synth-seance` | Every new engine — 13 verdicts | High. Blessings and debates are unique value |
| `/guru-bin` | 12 retreats complete, 13 if OSTINATO | High. Sweet spots transfer to presets |
| `/sweep` | Every major build milestone | High. Catches structural bugs before they compound |
| `/historical-society` | End of sprint sessions | Medium. Good intent, inconsistently triggered |
| `/board` | Scope decisions (V1 reversal, funding) | High. Unanimous decisions fast |
| `/post-engine-completion-checklist` | 3 engines on 2026-03-20 | High. Catches registration gaps |
| `/producers-guild` | Pack strategy, ONSET first pack | Medium. Good for market framing |

### Skills Underused

| Skill | Gap |
|-------|-----|
| `/flywheel` | Used once (2026-03-15 log exists). Should run at arc end. Did not run at end of sessions 2–6. |
| `/model-advisor` | Exists but never appears in session logs. Zero observed invocations. May be redundant with model-effort-guide.md. |
| `/audit-prefix` | Identified as needed on 2026-03-15 in the Flywheel log. Not built. Still catching prefix errors manually in session 5. |
| `/memory-sync` | Identified as needed on 2026-03-15. Not built. MEMORY.md still updated manually and drifting. |
| `/atelier` | Web work deferred across all sessions. 20 audio clips still unrecorded. Patreon URL still placeholder. |
| `/kai` | Used once for android audit (2026-03-18). XPN pack validation is Kai's domain; not invoked for TIDE TABLES or MACHINE GUN REEF preflight. |
| `/oxport` | Tool exists, orchestrator built, but dry-run preflight never invoked before session end. |

### Redundant or Overlapping Skills

**`/model-advisor` vs `model-effort-guide.md`:** Functionally identical. One is a skill, one is a memory file. The skill likely just reads the memory file. Consolidate: keep the memory file, remove the skill, add a note in session-start templates to check the guide.

**`/producers-guild` and `/exo-meta`:** Both do preset concept work. The Guild is market-facing (which presets would sell?), Exo Meta is craft-facing (how to build the preset). The distinction is real but sessions sometimes conflate them — Guild review producing preset parameter specs that Exo Meta should be authoring. Clarify the handoff in skill descriptions.

**`/historical-society` and `/memory-sync` (proposed):** Historical Society is a full documentation audit; the proposed memory-sync is a targeted counter update. These should stay distinct. Build memory-sync as a lightweight Haiku-appropriate tool and keep Historical Society for full arc reviews.

### Skills That Should Be Built

**`/audit-prefix`** — Still missing 308 days after identification. Three engines required manual prefix correction in session 4. Automate: scan `frozenPrefixForEngine()` against registered engines and flag mismatches. 30-minute build. Save this time on every future engine install.

**`/memory-sync`** — Still missing. Engine count drift (31 → 34 went stale) is a known failure mode. Automated count from `Source/Engines/` subdirectory vs MEMORY.md line 2. Haiku-compatible.

**`/session-close`** — A dedicated end-of-session checklist that: (1) runs `/memory-sync`, (2) triggers `/historical-society` for any modified files, (3) records the pending audit queue, (4) commits open work. Currently the end of sessions relies on ad-hoc morning plans written as separate docs. Make it a skill with a standard output format.

**`/preset-qa`** — Validates a batch of `.xometa` files before commit: unique names, mood category structural validity, coupling fields populated for Entangled, correct engine name casing. Catches what the 2026-03-18 normalize_engine_names.py fixed (5,818 presets with wrong casing) before they land.

---

## 3. Workflow Friction

### Scope Reversals Cost Two Sessions

The most expensive process failure in the arc was the V1 scope reversal on 2026-03-17.

- **2026-03-14 (Pi Day):** Decision: "Everything today is V1" — 24 working engines + 5 Constellation + 4 concept engines (OSTINATO, OPENSKY, OCEANDEEP, OUIE) all V1 scope.
- **2026-03-15:** Decision confirmed: all 4 concept engines are V1 scope.
- **2026-03-17:** Decision reversed by Board: 4 concept engines deferred to post-V1. Timeline risk outweighs brand argument.
- **2026-03-18:** All 4 concept engines built (DSP complete). Now they are back in scope.

The reversal generated: a Board session, two memory file updates (one marking the reversal, one marking the superseded decision), confusion in at least one subsequent session about what was in scope, and documentation that still references "V1 concept engines" vs "post-V1" inconsistently.

**Root cause:** The Pi Day decision was made without a launch timeline in hand. When the timeline surfaced (Week 6 target), the reality of "zero DSP" for 4 engines collided with the deadline. The reversal was correct but expensive.

**Fix for Vol 2:** Before any scope decision that adds new engines or features, run a quick Board brief: "Does this fit a realistic timeline?" The Board format works for this (3 minutes, returns a yes/no with conditions). Do not let Pi Day-style creative expansions land in memory as hard commitments without timeline validation.

### Handoffs Between Sessions Lose Context

**Pattern observed:** Each session ends with a "morning plan" document. The morning plan format is good, but it is written at the end of sessions (when the author is tired and the context window is full) and read at the start of the next session (when context is cold). The plans are generally accurate but miss the subtle "why this order" reasoning.

Example from 2026-03-20 morning plan: Phase 3 (ORBWEAVE seance + presets) is marked "(Opus, ~20 min)" but Phase 4 (4 missing seances in parallel) is not model-tagged. An operator starting fresh would not know whether to upgrade model for Phase 4.

**Fix:** Standardize the morning plan format to include: model per phase, blockers vs optional, and a one-line "why now" for each phase. The `/session-close` skill (proposed above) should produce this format automatically.

### Re-discovery Cost: Prefix Errors and Documentation Rot

The 2026-03-16 code quality sprint found 3 wrong prefixes in CLAUDE.md and 6 engines with documentation that did not match their actual DSP. The Canon V-2 scripture entry ("Integration Layer Drift") documents the root cause correctly: identity drifts from code reality when there is no automated check.

**Evidence of re-discovery:** The flywheel log from 2026-03-15 identifies the prefix audit as a needed tool. Four sessions later, the same class of error appears (OCEANIC `oceanic_` vs `ocean_`, ONSET `onset_` vs `perc_`, OSTERIA `ost_` vs `osteria_`). The insight was captured but the fix was not built.

**Pattern:** Insight → memory file → never-built tool → same error reappears. This is the most consistent workflow failure in the arc.

**Fix:** Any flywheel finding that proposes a new tool must be added to a "tools to build" backlog with an assigned session. The Historical Society should check this backlog at the start of each arc and confirm each tool was built or deliberately deferred.

### WAV Recording Blocker Has Been Deferred Across All 6 Sessions

The hero audio clips (20 clips for XO-OX.org) and the WAV stems for TIDE TABLES and MACHINE GUN REEF require a manual recording session on hardware. This blocker appeared on 2026-03-14 ("tomorrow"), 2026-03-15, 2026-03-17, 2026-03-18, and 2026-03-20. It has never been scheduled as a concrete calendar block.

**Impact:** TIDE TABLES spec complete. MACHINE GUN REEF spec complete. Neither can ship. Site conversion rate is depressed without audio clips. Patreon URL is still a placeholder (`www.patreon.com/cw/XO_OX`).

**Fix:** These are not AI tasks. They are owner tasks requiring specific hardware time. The session-close template should track them as "owner blockers" with a date field. If no date is set, the Historical Society flags them at arc start.

---

## 4. Pipeline Efficiency

### What Is Already Correctly Parallelized

- Background agent dispatch for preset generation (5 parallel agents per the feedback file). This is the right pattern.
- ORBWEAVE / OVERTONE / ORGANISM were dispatched as 3 parallel agents on 2026-03-19. Correct.
- The 4 seances (OSTINATO, OPENSKY, OCEANDEEP, OUIE) dispatched in parallel on 2026-03-20. Correct.
- The 12-round Prism Sweep ran parallel detective agents. Correct.

### Sequential Tasks That Could Be Parallelized

**Guru Bin retreats.** Retreats are currently run one engine at a time in main context. Retreats for engines with complete seance verdicts and known parameter schemas are largely independent. A retreat for OPENSKY does not depend on the OCEANDEEP retreat. These can be parallelized: dispatch one background agent per engine, each receiving the seance verdict, parameter schema, and sweet-spot discovery protocol. Reconvene to review findings.

**Post-completion checklists.** The 2026-03-20 checklist ran OVERTONE, ORGANISM, ORBWEAVE in sequence in one document. These are independent read-only audits. Dispatch 3 parallel agents, merge results.

**Sound design guide sections.** New engine guide sections (Sections 31–36 in the sprint report) were written sequentially. Each section requires only the engine's parameter schema and identity card. Parallelizable.

### Parallel Tasks With Hidden Dependencies

**Preset generation before seance.** The sprint report shows presets landing for OSTINATO/OPENSKY/OCEANDEEP/OUIE before seances were complete (150 presets existed at seance time per the engine status table). This means presets were generated without seance-discovered sweet spots and Gospel corrections. The presets are valid but may not express the engine's peak capability. The correct sequence is: seance → Guru Bin retreat → preset generation. Violating this order produces "factory adequate" presets, not "Guru Bin" presets.

**Engine registration before preset normalization.** The 2026-03-18 sprint shows 5,818 presets with wrong engine name casing were silently dropped on load — discovered after registration was complete. The normalization tool (normalize_engine_names.py) ran after the fact. The correct sequence: normalize_engine_names → register engine → generate presets. Add this to the new engine integration checklist.

**DSP fixes after preset generation.** OSTINATO received a DSP fix (OstiDecayEnv upgraded from decay to full ADSR) and ORCA received an AudioToRing dead-path fix after their preset libraries were complete. This means presets authored with the broken DSP may not sound as designed. Not a catastrophe, but a quality gap. The fix: doctrine D004 (dead parameters audit) should run before preset generation, not after.

### Where Agents Duplicate Work

**Seance cross-reference document.** `Docs/seance_cross_reference.md` exists and is maintained. But the fleet health dashboard (`fleet_health_2026_03_20.md`) was generated separately and shows different seance counts (13 vs "30 seances complete" in CLAUDE.md). Three sources now track seance status. Any agent reading a different source gets a different answer. Consolidate to one authoritative source: the fleet health dashboard, generated by a script, not manually maintained.

**Preset counts.** MEMORY.md says ~15,200 presets. The sweep report says 15,199. The sprint report says 13,550. The fleet health dashboard says 21,918. These cannot all be correct simultaneously — they reflect different points in time and different counting methods (some count files, some count presets-per-engine, some count across mood directories). A unified count script run at session start/end would resolve this.

---

## 5. Documentation Debt

### MEMORY.md: Anatomy of the Bloat

MEMORY.md hit 206 lines against a 200-line limit. The growth pattern reveals the problem:

**Inline detail that belongs in satellite files.** The MEMORY.md still contains full build commands, JUCE gotcha lists, param prefix gotchas, status tables for individual standalone repos, and multi-line V1 scope histories. Each of these belongs in a dedicated satellite file with a single pointer in MEMORY.md. The refactored version (current MEMORY.md as of 2026-03-20) is already much cleaner — but the pattern will re-bloat within 2–3 sessions if not enforced.

**Status fields that should be structured data.** "42 engines integrated" is a count that changes. "All deficits resolved" is a boolean that changes. "Seances complete: 12 retreats" is a count that changes. These are all tracked in prose in MEMORY.md. They should be auto-generated by the fleet health script and referenced, not embedded.

**Historical decisions not archived.** The V1 scope reversal now has three files: the original decision, the reversal, and the "CURRENT DECISION" update to the reversal. Old decisions should be archived (moved to `Docs/decisions/archive/`) and MEMORY.md should point only to the current decision file. Otherwise the memory footprint grows with every scope change.

### Specific Staleness Issues Found

| File | Staleness Issue |
|------|----------------|
| `v1-scope-decision-march-2026.md` | Still references "30 engines" and "Week 6: repo public, 30 engines" — actual count is 42 |
| `skill-ecosystem.md` | Says "16 total" skills, dated 2026-03-14 — actual count is 38+. Overtone/Organism/Orbweave engines listed as "post-V1" in old decisions |
| `four-engine-campaign-2026-03-18.md` | Phases 2–8 partially complete — status column is stale against current open-engines-status.md |
| `CLAUDE.md (repo)` | Duplicate OVERLAP and OUTWIT rows in engine table (confirmed by sweep report) |
| `guru-bin-campaign.md` | Lists OCEANIC retreat #8 but the current MEMORY says 12 retreats complete, including OBRIX/OSTINATO which are not in the tracker |

### Documentation That Is Missing

**Per-engine decision log.** Why does OVERBITE use plain param IDs (`filter_cutoff`) instead of prefixed ones (`poss_filter_cutoff`)? The answer is "342 presets already use plain names" — but this lives only in MEMORY.md as a one-line note. An XOceanus adapter author discovering this inconsistency has no way to find the rationale without full context search. Each engine with a non-standard convention should have a `decisions.md` in its engine directory.

**Oxport one-click orchestrator spec.** `Tools/oxport.py` exists (8-stage pipeline). The P1 roadmap item "Oxport One-Click" is listed as "(M effort)" in the oxport-tool-suite.md memory file but no spec exists. The sprint report shows the orchestrator was built, but the dry-run preflight is not invoked in any session.

**Seance completion protocol.** The `/post-engine-completion-checklist` skill is documented. But the sequence "seance → Guru Bin → preset gen" is only implied by the skill ecosystem diagram in skill-ecosystem.md. It is not written as a protocol. Three engines (ORBWEAVE at launch: 0 presets, OVERTONE: 306, ORGANISM: 333) show inconsistent application of this sequence.

**Vol 2 arc plan.** The pending audit queue lists tasks for next session but there is no arc-level plan for Vol 2 equivalent to the V1 launch decisions. What is Vol 2? When does it start? What are the success criteria? This should exist before the session that starts it.

---

## 6. Recommendations

### Top 5 Process Improvements for the Vol 2 Arc

**Recommendation 1: Build `/audit-prefix` and `/memory-sync` before the first Vol 2 engine install.**

These two tools were identified on 2026-03-15, never built, and caused manual correction work in sessions 3 and 4. Time cost to build: one focused 30-minute Sonnet session. Time saved: 15–30 minutes per engine install × every future engine. At 3+ engines planned for Vol 2 (KNOT coupling, plus any Theorem results), this pays back immediately.

**Recommendation 2: Enforce the seance → Guru Bin → preset generation sequence as a hard gate.**

The post-engine-completion-checklist already checks for a seance verdict. Add a Guru Bin retreat entry as a required checkbox before any preset generation begins. ORBWEAVE shipped with 0 presets and no retreat because the retreat was never scheduled post-seance. OVERTONE and ORGANISM have presets but no retreat notes — those presets are unrefined.

The gate: no `/exo-meta` or preset generator for an engine unless that engine's seance verdict and Guru Bin retreat are both marked complete. Record this in the fleet health dashboard.

**Recommendation 3: Replace the morning plan document with a `/session-close` skill.**

The morning plan is valuable but inconsistently authored and easily loses model-tagging, blocker vs optional distinction, and the "why this order" rationale. A skill produces a standardized output. Make `/session-close` mandatory at session end. It should: run memory-sync, produce the next session's task list with model tags, flag owner blockers (audio recording, Patreon URL), and append to `Docs/session-log.md`.

**Recommendation 4: Consolidate preset count and seance count to one authoritative script.**

Three different counts for presets appear across MEMORY.md, the fleet health dashboard, and session sprint reports. Run one script at session start and end: count `.xometa` files in `Presets/XOceanus/`, count seance verdict files in `Docs/seances/`, compare both against registered engine count. Output a 4-line health summary. Paste it into MEMORY.md at session close. All other counts are stale by definition.

**Recommendation 5: Implement Vol 2 scope discipline from day one.**

The V1 arc lost two sessions to a scope reversal caused by a creative commitment made before timeline validation. Vol 2 should start with a Board brief: What is in scope? What is the ship target? What is the criteria for "done"? Any new engine, feature, or collection proposed mid-arc goes through a 3-minute Board brief before landing in MEMORY.md as a commitment. The /board skill already supports this. Use it before writing memory, not after.

### New Skills or Automations That Eliminate Friction

| Tool | Type | Priority | Estimated Build Time |
|------|------|----------|---------------------|
| `/audit-prefix` | Skill | P0 — build now | 30 min (Sonnet) |
| `/memory-sync` | Skill | P0 — build now | 20 min (Haiku-compatible) |
| `/session-close` | Skill | P1 | 45 min (Sonnet) |
| `/preset-qa` | Skill | P1 | 45 min (Sonnet) |
| `Tools/count_presets.sh` | Script | P1 | 10 min |
| `/vol-2-kickoff` | Skill | P2 (before Vol 2) | 30 min |
| Per-engine `decisions.md` template | Docs | P2 | 15 min template + 1 hr backfill |

### Model and Effort Downgrades That Save Cost Without Quality Loss

| Task | Current | Recommended | Rationale |
|------|---------|-------------|-----------|
| Preset generation (post-retreat) | Sonnet (main context) | Haiku (background agents) | Mechanical interpolation once sweet spots are defined |
| Sound design guide sections | Sonnet (main) | Sonnet (background) | Independent per-engine, parallelizable |
| Documentation updates (MEMORY, CLAUDE.md) | Sonnet (main) | Haiku (background, `/memory-sync`) | Counter updates and link corrections |
| Post-completion checklist | Sonnet (main) | Sonnet (background, parallel) | Read-only audit, no synthesis required |
| Guru Bin retreats (mature engines with verdicts) | Opus (main) | Sonnet (background, parallel) | Pattern is established; sweet-spot discovery is templated |
| Novel seance evaluation | Sonnet (background) | Opus (main) | Ghost council verdicts benefit from full reasoning; don't background new engines |
| Novel engine DSP design | Opus (main) | No change | Correct |
| Build validation, cmake wiring | Sonnet (main) | No change | Correct |

---

## Appendix: Fleet Health Discrepancy Summary

The audit found four different preset counts in active documentation:

| Source | Count | Date | Method |
|--------|-------|------|--------|
| `fleet_health_2026_03_20.md` | 21,918 | 2026-03-20 | File count per engine directory |
| `sweep_report_2026_03_20.md` | 15,199 | 2026-03-20 | `.xometa` files under `Presets/XOceanus/` |
| `sprint_report_2026-03-18.md` | 13,550 | 2026-03-18 | `.xometa` files at that point in time |
| `MEMORY.md` | ~15,200 | 2026-03-20 | Manual estimate |

The 21,918 figure (fleet health) likely double-counts or includes JSON presets in engine subdirectories. The 15,199 sweep figure is the most methodologically rigorous (single directory, single extension). **15,199 is the figure to use.** The fleet health script should be corrected to count only `Presets/XOceanus/**/*.xometa`.

Seance count discrepancy: CLAUDE.md says "30 seances complete (2026-03-19)" but the fleet health dashboard shows 13 engines with seance verdicts. The 30 figure likely refers to the pre-March-14 Prism Sweep scope of 24 original engines + 5 Constellation + OBRIX (total 30 — the count before the 12 new engines were added). The current fleet health dashboard is the authoritative source: 13 engines have verdict files in `Docs/seances/`. The other 29 are pending.

---

*Audit complete. Sisters recommend scheduling `/audit-prefix` and `/memory-sync` builds as the first task of the next session before any engine work begins.*
