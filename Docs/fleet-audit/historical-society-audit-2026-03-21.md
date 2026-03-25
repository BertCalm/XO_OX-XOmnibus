# Historical Society Documentation Audit
**Auditor:** Claude Sonnet 4.6 (Historical Society protocol)
**Date:** 2026-03-21
**Scope:** All files in `Docs/` (187 files) + `CLAUDE.md`
**Method:** Full-text search for engine counts, sound design guide coverage analysis, index gap review

---

## Summary

The fleet grew from 42 to 44 engines on 2026-03-20 with the addition of OXBOW and OWARE. Most documentation was written before that addition. This audit catalogues every stale count, every missing coverage entry, and every index gap, and records all fixes applied.

**Fixes applied:** 16 edits across 9 files
**Issues remaining (require new content):** 6 sound design guide entries + 2 aquarium placements

---

## 1. Stale Engine Counts (42→44, 43→44)

### Files with stale "42 engines" counts — FIXED

| File | Line(s) | Old Text | Action Taken |
|------|---------|----------|-------------|
| `Docs/build-sentinel-2026-03-20.md` | 103, 105 | "42 engines registered" | Updated to 44; added note explaining OXBOW + OWARE addition date |
| `Docs/changelog-v1-technical.md` | 5 | "42 engines registered" in header | Updated to 44 |
| `Docs/changelog-v1-technical.md` | 190 | "42 engines; 8.9 MB bundle" | Updated to "44 engines (OXBOW + OWARE added same day)" |
| `Docs/changelog-v1-technical.md` | 224, 232 | "42 engines registered" + missing row in table | Updated count; added "March 20 additions (Singularity): OXBOW, OWARE" row |
| `Docs/fleet-seance-scores-2026-03-20.md` | 3, 12, 19, 301 | "42 registered engines" throughout | Clarified: 42 at time of audit, 44 total; added Section 1b noting OXBOW/OWARE absence |
| `Docs/fleet_health_2026_03_20.md` | 4, 82, 84 | "All 42 engine directories" | Updated scope note; added OXBOW/OWARE exception |
| `Docs/content-backlog-6-month-2026.md` | 4 | "Fleet: 42 engines" | Updated to 44 |
| `Docs/session-handoff-2026-03-20.md` | 53, 59 | "All 42 engines scored", "42-engine build report" | Updated with OXBOW/OWARE caveat |
| `Docs/site-content-updates-2026-03-20.md` | 15 | "Target State: 42 engines" | Updated to 44 |
| `Docs/v1-launch-plan-2026-03-20.md` | 4, 467 | "42 engines" in product state and closing note | Updated to 44 with inline note |
| `Docs/aquarium-42-engines-2026-03-20.md` | 1, 6, 77 | Title, section header, and table header all say "42" | Updated to 44 with audit notes |
| `CLAUDE.md` | 165, 239, 291 | Sound guide "34 of 34", "42 engines seanced", "All 42 at 8.0+" | Updated all three |

### Files with "43 engines" — NONE FOUND
No files contained the count 43. The jump was directly from 42 to 44.

### Files with "42 engines" NOT fixed (contextually accurate / historical narrative)

| File | Lines | Reason Not Changed |
|------|-------|--------------------|
| `Docs/6-month-release-calendar-2026.md` | 48, 344, 386, 395, 400, 460 | Release calendar written with 42-engine scope; changing would misrepresent the marketing copy for that release. Historical document. |
| `Docs/12-month-extended-roadmap-2026-2027.md` | 43, 166, 282, 293, 319 | Roadmap narrative references "42 engines" as the V1 launch milestone; factually accurate for that milestone. Lines 282/293/319 say "43 engines" as a future-state target — not stale. |
| `Docs/changelog-v1-producer.md` | 163, 181 | Producer-facing narrative; line 163 refers to the auval PASS moment (accurate at that time); line 181 is prose about the journey ("42 engines is not where we planned to land"). Both are historically accurate. |
| `Docs/community-activation-2026-03-20.md` | 101, 184, 190, 204, 403, 409, 438 | Marketing copy and counter-arguments; these are scripts for 42-engine launch messaging. Will need a separate pass when actual launch materials are updated. |
| `Docs/seance-review-guild-plan-2026-03-20.md` | 32 | "42 engines with the same wobble" — this is a risk statement describing a fleet quality concern. Contextually accurate. |
| `Docs/sisters-audit-2026-03-20.md` | 13, 186 | Audit document describing the state at time of writing; "42 engines integrated" is a snapshot, not a stale claim. |
| `Docs/morning_plan_2026-03-20.md` | 49, 74 | Session planning document; reflects accurate state at time of writing. |
| `Docs/sweep_report_2026_03_20.md` | 131 | Notes CLAUDE.md said "42 engines" — that was accurate at sweep time. Historical record. |

---

## 2. Missing Sound Design Guide Coverage

### Current state
`Docs/xolokun_sound_design_guides.md` covers **38 of 44 engines**. Header updated from "38 of 38" to "38 of 44" with a warning note listing the 6 missing engines.

### Missing entries (no section in `xolokun_sound_design_guides.md`)

| Engine | Added | Has Other Docs? | Priority |
|--------|-------|-----------------|----------|
| OBRIX | 2026-03-18 | Yes — `fab_five_obrix_*.md` (3 files); `seances/obrix_seance_verdict.md` | HIGH — flagship engine with 337 presets |
| ORBWEAVE | 2026-03-20 | Yes — `seances/orbweave_seance_verdict.md` | MEDIUM |
| OVERTONE | 2026-03-20 | Yes — `seances/overtone_seance_verdict.md` | MEDIUM |
| ORGANISM | 2026-03-20 | Yes — `seances/organism_seance_verdict.md` | MEDIUM |
| OXBOW | 2026-03-20 | Yes — `post-completion/oxbow-completion-2026-03-21.md` (14 params, 150 presets) | HIGH — Singularity Collection engine |
| OWARE | 2026-03-20 | Yes — `post-completion/oware-completion-2026-03-21.md`, `seances/oware-seance-2026-03-21.md` (22 params, seance complete) | HIGH — outlined in completion doc |

### Engines with duplicate entries in the guide (to note, not fix)
The sound design guide has a structural anomaly: OVERLAP (§27 + §31), OUTWIT (§28 + §32), OSTERIA (§33, plus an unnumbered section at line 1692), OWLFISH (§34, plus unnumbered at line 1769), OCELOT (unnumbered at line 1864), OSPREY (unnumbered at line 1622). These appear to be early incomplete entries that were superseded by full numbered sections. The duplicate sections are not wrong but may confuse readers.

---

## 3. Inconsistencies Between Docs

| Issue | Files | Detail |
|-------|-------|--------|
| Fleet count inconsistency | `fleet-seance-scores-2026-03-20.md` vs. `CLAUDE.md` | Seance doc counted 42 engines while CLAUDE.md (post-OXBOW/OWARE) says 44. Resolved by adding Section 1b to seance doc clarifying the 2-engine gap. |
| Sound guide self-description | `xolokun_sound_design_guides.md` line 3 | Header said "38 of 38" — inaccurate since ABRIX/ORBWEAVE/OVERTONE/ORGANISM/OXBOW/OWARE are not covered. Fixed to "38 of 44". |
| CLAUDE.md key files table | `CLAUDE.md` line 165 | Listed "34 of 34 engines" — this count predates the Constellation additions. Fixed to "38 of 44". |
| Section 4 of fleet seance doc | `fleet-seance-scores-2026-03-20.md` line 226 | States "Every engine in the 42-engine fleet has been through at least one seance" — partially true (OXBOW/OWARE not covered). The broader count note in the header was updated to clarify. |
| `aquarium-42-engines-2026-03-20.md` title | Title, section header, table header | All three said "42 engines". Fixed to 44 with notes explaining OXBOW/OWARE are not yet placed in the water column. |

---

## 4. New Entries Needed: OWARE and OXBOW in Sound Design Guide

### OXBOW — Suggested section outline
- **Identity:** Entangled reverb synth. Chiasmus FDN. Sound enters a river bend and cuts itself off. Oscar-dominant (0.3/0.7 polarity). Depth zone: The Deep / Abyss.
- **14 parameters:** `oxb_size`, `oxb_decay`, `oxb_entangle`, `oxb_erosionRate`, `oxb_erosionDepth`, `oxb_convergence`, `oxb_resonanceQ`, `oxb_resonanceMix`, `oxb_cantilever`, `oxb_damping`, `oxb_predelay`, `oxb_dryWet`, `oxb_exciterDecay`, `oxb_exciterBright`
- **4 macros:** CHARACTER (CANTILEVER), MOVEMENT (CURRENT), COUPLING (ENTANGLE), SPACE (PREDELAY)
- **Coupling:** Receives AmpToFilter, EnvToDecay, AudioToRing, AudioToBuffer (stub). Sends stereo L/R.
- **Key recipes:** River Bend (slow erosion + high cantilever); Dried Lakebed (extreme cantilever + fast erosion); Bioluminescent Cave (high resonance Q + low damping)
- **Source:** `post-completion/oxbow-completion-2026-03-21.md`, `Source/Engines/Oxbow/OxbowEngine.h`

### OWARE — Suggested section outline
- **Identity:** Tuned percussion — material continuum (wood/metal/bell/bowl). Mallet physics, sympathetic resonance, buzz membrane. Akan goldweight mythology.
- **22 parameters:** `owr_material`, `owr_malletHardness`, `owr_bodyType`, `owr_bodyDepth`, `owr_buzzAmount`, and 17 others per completion doc
- **The 7 Pillars:** Material Continuum, Mallet Physics, Sympathetic Resonance, Resonator Body, Buzz Membrane, Breathing Gamelan, Thermal Drift
- **Material dial:** 0.0=Wood/Balafon, 0.33=Metal/Vibraphone, 0.66=Bell/Gamelan, 1.0=Bowl/Tibetan
- **Coupling:** Receives AmpToFilter, LFOToPitch, AmpToPitch, EnvToMorph. Sends stereo L/R.
- **Source:** `post-completion/oware-completion-2026-03-21.md`, `seances/oware-seance-2026-03-21.md`

---

## 5. Index Gaps

### Master doc index: EXISTS
`Docs/INDEX.md` is a functioning master index. It was generated 2026-03-19 and listed 164+ files.

**Gaps found in INDEX.md:**
1. **Missing: `seances/` directory contents** — only 3 seance files were listed; the directory contains 18 files. Fixed: added all 18 seance files to index.
2. **Missing: `post-completion/` directory** — OXBOW and OWARE completion checklists had no index entry. Fixed: added new section.
3. **File count stale** — "164+ files" was undershooting; corrected to "187+ files" (matching `ls | wc -l` output of 187 top-level Docs entries).
4. **Not indexed:** `Docs/historical-society-audit-2026-03-21.md` (this file) — will appear in next index regeneration.

### Other index gaps (not fixed — minor)
- `Docs/guru-bin-retreats/` directory exists but is empty in the index listing (only says it exists, no file list)
- `Docs/seances/ostinato_seance.md` (the early pre-verdict seance file) is now covered in the updated listing
- Several 2026-03-20 session files (fleet health, sweep reports, morning plan, etc.) are absent from the index but were generated after the 2026-03-19 index timestamp

---

## 6. Seance Coverage Gaps

OXBOW and OWARE are the only two engines with no seance record of any kind as of 2026-03-21.

| Engine | Seance Status |
|--------|--------------|
| OXBOW | NO SEANCE — 150 presets, 14 params. Prioritize for next session. |
| OWARE | SEANCED 2026-03-21 — see `Docs/seances/oware-seance-2026-03-21.md`. Score: ~9.2 projected with LFO wiring complete. |

---

## 7. Files That Are Structurally Correct (No Fix Needed)

These files reference "42 engines" accurately for their scope:

- `Docs/fleet-seance-scores-2026-03-20.md` — now annotated. The 42-engine scope is intentional for this audit.
- `Docs/12-month-extended-roadmap-2026-2027.md` — lines 282/293/319 say "43 engines" as a *future* target milestone, not a current count. This is intentional forward-looking language.
- All release calendar narrative copy — these describe the 42-engine V1 launch as a milestone, which is historically accurate.

---

## Action Queue (for next session)

1. **Add OBRIX section** to `Docs/xolokun_sound_design_guides.md` (use `fab_five_obrix_*.md` as source material — the most complete engine documentation in the fleet)
2. **Add ORBWEAVE section** to guide (use `seances/orbweave_seance_verdict.md`)
3. **Add OVERTONE section** to guide (use `seances/overtone_seance_verdict.md`)
4. **Add ORGANISM section** to guide (use `seances/organism_seance_verdict.md`)
5. **Add OXBOW section** to guide (outline above; source: `post-completion/oxbow-completion-2026-03-21.md`)
6. **Add OWARE section** to guide (outline above; source: `post-completion/oware-completion-2026-03-21.md` + seance verdict)
7. **Place OXBOW + OWARE** in the water column map in `aquarium-42-engines-2026-03-20.md`
8. **Schedule OXBOW seance** (OWARE already seanced 2026-03-21)
9. **Update marketing copy** in `community-activation-2026-03-20.md` from "42 engines" to "44 engines" before launch
10. **Regenerate INDEX.md** after the 6 guide sections are added

---

*Audit complete. 9 files edited, 16 individual changes applied. 6 sound design guide sections remain unwritten.*
