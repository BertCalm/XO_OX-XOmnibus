# XOceanus Documentation Governance

**Created:** 2026-03-20
**Purpose:** Policy document — when to update which file, authority levels, SLAs, and maintenance ownership.

---

## Document Authority Hierarchy

| Level | Documents | Description |
|-------|-----------|-------------|
| **L1 — Canonical** | `CLAUDE.md`, `Docs/xoceanus_master_specification.md` | Single source of truth. Must be updated within 24h of any material change. |
| **L2 — Living Index** | `Docs/seances/seance_cross_reference.md`, `Docs/MANIFEST.md`, `Skills/README.md` | Accurate indexes. Must stay in sync with L1. |
| **L3 — Reference** | `Docs/xoceanus_sound_design_guides.md`, `Skills/*/SKILL.md`, `Docs/fleet_health_*.md` | Deep reference. Updated per engine/feature cycle. |
| **L4 — Historical** | `Docs/prism_sweep_*.md`, `Docs/seances/*.md`, `Docs/guild/*.md` | Immutable once committed. Archival value. |
| **L5 — Archive** | `Docs/plans/`, `Docs/specs/archive/`, superseded docs | Completed or obsolete. Add status headers, do not delete. |

---

## When to Update Which Document

### Scenario: New Engine Added

| Document | Update Required | Timing | Notes |
|----------|----------------|--------|-------|
| `CLAUDE.md` | ✅ Required | Within 24h | Add to engine list, engine table, parameter prefix table |
| `Docs/xoceanus_master_specification.md` | ✅ Required | Within 24h | Section 3.1 engine table, section 3 coupling if new type added |
| `Source/XOceanusProcessor.cpp` | ✅ Required | On integration | Register engine, add to addParameters, SilencePeriod if needed |
| `Source/Core/PresetManager.h` | ✅ Required | On integration | Add to validEngineNames, frozenPrefixForEngine |
| `Docs/seances/seance_cross_reference.md` | ✅ Required | After seance | Add seance row with score, blessings, D-violations, quote |
| `Docs/xoceanus_sound_design_guides.md` | ✅ Required | Within 1 week | Add per-engine guide section |
| `Skills/README.md` | ⚠️ If skill changes | On skill creation | Only if a new skill was created for the engine |
| `CMakeLists.txt` | ✅ Required | On integration | Add .h and .cpp to source list |

### Scenario: New Coupling Type Added

| Document | Update Required | Timing |
|----------|----------------|--------|
| `Source/Core/SynthEngine.h` (CouplingType enum) | ✅ Required | On implementation |
| `CLAUDE.md` line "14 coupling types" | ✅ Required | Within 24h |
| `Docs/xoceanus_master_specification.md` section 4.1 | ✅ Required | Within 24h |
| `Skills/coupling-interaction-cookbook/SKILL.md` | ✅ Required | Within 1 week |
| `Docs/xoceanus_new_engine_process.md` appendix | ⚠️ If notable | Within 1 week |

### Scenario: New Blessing Confirmed (Ghost Council)

| Document | Update Required | Timing |
|----------|----------------|--------|
| `CLAUDE.md` blessings table | ✅ Required | Within 24h — assign next global B-ID |
| `Docs/seances/seance_cross_reference.md` engine row | ✅ Required | With seance commit |
| Seance verdict file | ✅ Required | Same commit as seance |

**Blessing ID assignment rules:**
- IDs are globally unique and sequential (B001, B002, ... B031, B032...)
- Do NOT reuse local per-seance "B017" numbering as global IDs — translate to next available global ID
- Proposed blessings (not confirmed by full council) noted as "Proposed" in status column

### Scenario: Doctrine Violation Resolved Fleet-Wide

| Document | Update Required | Timing |
|----------|----------------|--------|
| `CLAUDE.md` Critical Fleet-Wide Findings | ✅ Required | Same commit as fix |
| `Docs/seances/seance_cross_reference.md` doctrine violation table | ✅ Required | Within 24h |
| `Docs/prism_sweep_final_report.md` | ✅ Required | At round completion |

### Scenario: New Preset Batch Committed

| Document | Update Required | Timing |
|----------|----------------|--------|
| `CLAUDE.md` preset count | ✅ Required | Within 24h if count changes by > 100 |
| `Docs/fleet_health_{date}.md` | ✅ Required | Monthly or at major milestones |

### Scenario: Seance Debate Resolved

| Document | Update Required | Timing |
|----------|----------------|--------|
| `CLAUDE.md` debates table | ✅ Required | Within 24h — change status from UNRESOLVED |
| `Docs/seances/seance_cross_reference.md` | ✅ Required | Add note in relevant engine row |
| Seance verdict file | ✅ Required | Already committed |

---

## Update SLAs

| Document Tier | Change Type | SLA |
|---------------|------------|-----|
| L1 Canonical | Any material change | 24 hours |
| L2 Living Index | New engine/seance/skill | 48 hours |
| L3 Reference | Engine/feature update | 1 week |
| L4 Historical | N/A (immutable) | — |
| L5 Archive | Status header only | 1 week |

---

## What NOT to Change

### Frozen Fields (Never Modify)

- **Parameter IDs** in `Source/Core/PresetManager.h:frozenPrefixForEngine()` — these are API contracts
- **Engine IDs returned by `getEngineId()`** once first preset is shipped
- **Seance verdict files** once committed — they are L4 historical record
- **B001–B016 blessing definitions** — they are canonical as awarded by ghost council

### Deprecated (Don't Update, Add Archive Header Instead)

- `Docs/xo_mega_tool_preset_system.md` — superseded by `Docs/xoceanus_preset_spec_for_builder.md`
- `Docs/xo_mega_tool_visual_identity.md` — superseded by `Docs/xoceanus_technical_design_system.md`
- Any `Docs/plans/` file with `STATUS: COMPLETED`

---

## Naming Conventions

### Files
- Engine headers: `{EngineName}Engine.h` (e.g., `OceandeepEngine.h`) — lowercase-first after O-prefix
- Seance verdicts: `{lowercase_engine}_seance_verdict.md` (e.g., `orbweave_seance_verdict.md`)
- Fleet health: `fleet_health_{YYYY-MM-DD}.md` — one per major milestone
- Guild reviews: `{engine}_{review_type}_{YYYY-MM-DD}.md`

### Engine IDs (getEngineId() return values)
Must match `PresetManager.h:validEngineNames`. Current canonical list:
- Use exact capitalization as in CLAUDE.md engine table (e.g., "OceanDeep" not "Oceandeep", "OpenSky" not "Opensky")
- Never add X prefix to returned ID (e.g., "Overlap" not "XOverlap")

---

## Common Mistakes to Avoid

1. **Forgetting to update CLAUDE.md engine count** after adding an engine
2. **Using local seance B-IDs as global IDs** — translate to next available global B-ID
3. **Registering engine with wrong getEngineId()** — must match validEngineNames exactly
4. **Updating only one of the two engine tables** (CLAUDE.md and master spec are both L1)
5. **Creating spec files without a status header** — add STATUS: ACTIVE/DRAFT/ARCHIVE
6. **Putting session transcripts in Docs/specs/** — those go in Docs/sessions/ (future)
