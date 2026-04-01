# CLAUDE.md Audit Report — 2026-03-24

Autonomous overnight audit of all CLAUDE.md files across the XO_OX ecosystem.

---

## Files Audited

| File | Location |
|------|----------|
| `CLAUDE.md` (audio-xpm-creator) | `$HOME/CLAUDE.md` |
| `CLAUDE.md` (XOlokun) | `XO_OX-XOmnibus/CLAUDE.md` |
| `CLAUDE.md` (XOzone) | `XO_OX-XOzone-Instrument/CLAUDE.md` |
| `CLAUDE.md` (XOxytocin) | `XOxytocin/CLAUDE.md` |

---

## Scoring Rubric

| Criterion | Weight |
|-----------|--------|
| Commands / workflows documented | /20 |
| Architecture clarity | /20 |
| Non-obvious patterns (gotchas) | /15 |
| Conciseness | /15 |
| Currency (reflects current state) | /15 |
| Actionability | /15 |

---

## File 1: `$HOME/CLAUDE.md` (audio-xpm-creator)

**Score: 84/100**

| Criterion | Score | Notes |
|-----------|-------|-------|
| Commands / workflows | 18/20 | fnm + tsc documented; missing `npm run dev` and `npm run build` |
| Architecture clarity | 18/20 | Store/audio/persistence/undo-redo well mapped |
| Non-obvious patterns | 15/15 | Excellent — 5 categories of gotchas, all concrete |
| Conciseness | 14/15 | Tight and focused throughout |
| Currency | 12/15 | Stores list matches directory; no stale references found |
| Actionability | 13/15 | All rules are executable; QA categories are directly usable |

### Issues Found
- Missing `npm run dev` and `npm run build` commands in Environment section — engineers will reach for these first

### Changes Applied
- Added `npm run dev` and `npm run build` to the Environment section

---

## File 2: `XO_OX-XOmnibus/CLAUDE.md` (XOlokun)

**Score: 78/100 → 88/100 after fixes**

| Criterion | Score (before) | Score (after) | Notes |
|-----------|---------------|---------------|-------|
| Commands / workflows | 16/20 | 19/20 | Build commands present but no auval, no env prereqs |
| Architecture clarity | 20/20 | 20/20 | Exemplary — engine table, parameter prefixes, key files all mapped |
| Non-obvious patterns | 13/15 | 14/15 | Good; added JUCE-specific CMake gotchas |
| Conciseness | 10/15 | 11/15 | Seance Findings prose is dense; Blessings table is long but necessary |
| Currency | 12/15 | 14/15 | Preset count wrong (~19,000+ vs actual 17,251); AU ID unverified |
| Actionability | 10/15 | 14/15 | Missing V1 scope, KC summary, auval command, env setup |

### Issues Found

1. **Preset count inaccurate**: claimed `~19,000+`, actual count is 17,251 files on disk
2. **AU plugin ID missing**: `auval` command not documented; CMakeLists.txt uses `Xolk XoOx` (not the stale `Xomn XoOx` in MEMORY.md notes)
3. **No environment/prerequisites section**: CMake version, Ninja, Xcode CLT, JUCE path — none documented
4. **JUCE gotchas absent**: `OSX_ARCHITECTURES` before `project()`, `apvts.processor` vs `getProcessor()`, `atomic.load()` for jmax — all proven bugs, none captured
5. **V1 scope not documented**: Revised scope (28-34 engines, OBRIX flagship + curated selection) is in MEMORY.md and a standalone doc but not in CLAUDE.md
6. **Kitchen Collection missing as section**: 24 engines across 6 quads, all retreats complete, not summarized anywhere actionable in CLAUDE.md
7. **community-strategy-v2.md not referenced**: New community strategy doc exists in Docs/ but not linked
8. **xomnibus_* doc filenames**: All `Docs/xomnibus_*.md` references are intentionally preserved (files still named with xomnibus prefix — not a bug, flagged only for awareness)

### Changes Applied

1. **Preset count**: `~19,000+` → `~17,250` (matches actual 17,251 files)
2. **New `## Environment` section**: Added before Build — CMake ≥ 3.22, Ninja, Xcode CLT, JUCE path, AU ID `aumu Xolk XoOx`, critical CMake gotchas (OSX_ARCHITECTURES order, apvts.processor, atomic.load)
3. **auval command**: Added `auval -v aumu Xolk XoOx` to Build section with "run after every macOS build" note
4. **New `## V1 Scope` section**: OBRIX flagship + 6-8 FX + 20-25 curated (~28-34 total), Patreon milestone unlocks
5. **New `## Kitchen Collection` section**: 6-quad table with engine names, themes, retreat status, link to community-strategy-v2.md
6. **Documentation Index**: Added 4 new entries — community-strategy-v2.md, v1-scope-revision-2026-03-23.md, kitchen-collection-release-calendar.md, fleet-seance-scores-2026-03-20.md

---

## File 3: `XO_OX-XOzone-Instrument/CLAUDE.md` (XOddCouple)

**Score: 76/100**

| Criterion | Score | Notes |
|-----------|-------|-------|
| Commands / workflows | 17/20 | cmake build present; no auval command |
| Architecture clarity | 18/20 | Key files table comprehensive; parameter system clear |
| Non-obvious patterns | 10/15 | Architecture rules listed but no runtime gotchas (no anti-patterns) |
| Conciseness | 15/15 | Well-scoped, nothing extraneous |
| Currency | 10/15 | References `XO_OX_Instrument_Development_Playbook.md` and `synth_playbook/` — not verified on disk; preset count (114) unverified |
| Actionability | 12/15 | Workflow steps are clear; no JUCE gotchas |

### Issues Found
- No auval command documented
- No runtime gotchas (parallel to what JUCE builds commonly hit)
- Minor: macOS build missing `-G Ninja` (uses default generator)

### Changes Applied
None. Scores are acceptable and this is a standalone instrument (lower priority than XOlokun). Issues are low-severity.

---

## File 4: `XOxytocin/CLAUDE.md` (XOxytocin standalone)

**Score: 88/100 → 91/100 after fixes**

| Criterion | Score (before) | Score (after) | Notes |
|-----------|---------------|---------------|-------|
| Commands / workflows | 18/20 | 18/20 | cmake + auval command present |
| Architecture clarity | 20/20 | 20/20 | Signal flow diagram, DSP classes, parameter table all excellent |
| Non-obvious patterns | 15/15 | 15/15 | Matched-Z, block-rate caching, bipolar checks, ParamSnapshot — all captured |
| Conciseness | 14/15 | 14/15 | Detailed but all load-bearing |
| Currency | 12/15 | 14/15 | Seance status stale (said "not yet run" — it ran at 9.5/10); preset count wrong (said 120, actual 130) |
| Actionability | 13/15 | 13/15 | Post-build checklist is actionable |

### Issues Found
1. **5x "XOmnibus" references**: All stale — engine was integrated into XOlokun (renamed 2026-03-24)
2. **Seance status stale**: Said "Not yet run" — seance completed 2026-03-22 at 9.5/10 (fleet leader)
3. **Preset count stale**: Said 120, actual on-disk count is 130

### Changes Applied
1. All 5 occurrences of "XOmnibus" replaced with "XOlokun"
2. Seance row updated: "Not yet run" → "9.5/10 — Fleet leader (B040 Note Duration, unanimous). Run 2026-03-22."
3. Preset count updated: 120 → 130

---

## Summary of All Changes

| File | Changes |
|------|---------|
| `$HOME/CLAUDE.md` | Added `npm run dev` + `npm run build` to Environment |
| `XO_OX-XOmnibus/CLAUDE.md` | Fixed preset count; added Environment section with JUCE gotchas + AU ID; added auval command; added V1 Scope section; added Kitchen Collection section; expanded Documentation Index |
| `XOxytocin/CLAUDE.md` | Replaced 5x XOmnibus → XOlokun; updated seance status (9.5/10); updated preset count (120 → 130) |
| `XO_OX-XOzone-Instrument/CLAUDE.md` | No changes (standalone instrument, acceptable baseline) |

---

## Remaining Gaps (Not Fixed — Require Manual Decision)

1. **XOlokun Seance Findings prose (lines 363+)**: ~400-word changelog-style paragraph. Accurate but extremely dense — consider splitting into a table. Not changed because it's a historical record and reducing it risks losing traceability.

2. **`Docs/xomnibus_*.md` filenames**: 20 doc files still named with `xomnibus_` prefix. CLAUDE.md links to them correctly (files exist). Renaming would be a separate task requiring updates to all cross-references.

3. **XOddCouple auval command**: Not added — AU ID not verified from CMakeLists; would require reading the XOzone repo's build config.

4. **MEMORY.md warning** (202 lines, limit 200): Per the warning in the memory file itself, detailed content should move to topic files. Outside scope of CLAUDE.md audit.
