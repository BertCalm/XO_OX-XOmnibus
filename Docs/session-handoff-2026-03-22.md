# Session Handoff — 2026-03-22

## Session Summary

Session 2026-03-21 (continued across two context windows).

### What Was Accomplished

**OWARE Engine — Final Fix + Re-Seance → 9.2/10**
- All 3 seance-required fixes were applied by overnight agents: LFO1+LFO2 real StandardLFO objects wired to `owr_lfo1Rate/Depth/Shape` and `owr_lfo2Rate/Depth/Shape`; shimmerRate reads `owr_shimmerRate` param (4.0 Hz default); buzzAmount default 0.0 → 0.15
- This session applied the final 1-line fix: `owr_lfo1Depth` default 0.0 → 0.1 (seance finding from 6/8 ghosts)
- Re-seance confirmed: **9.2/10** with 3 Blessings awarded (B032, B033, B034)
- OWARE presets: 120 → **150** (30 new: Family×8, Submerged×7, Aether×8, Flux×7)

**ORBWEAVE — Default Trap Fixed**
- `weave_braidDepth` default: 0.5 → 0.2 (below WEAVE-I threshold; engine now decoupled on init)
- 13 new presets added (all topologies: Trefoil, Solomon, Pentagram, Cinquefoil, Figure-Eight, Torus)
- Build + auval PASS after change

**OXBOW — Guru Bin Retreat Written**
- `Docs/guru-bin-retreats/oxbow-retreat-2026-03-21.md` (854 lines)
- 10 phases, 5 recipe categories (Still Pool, Twilight Hall, Oxbow Pool, Erosion Wave, Chiasmus Showcase), 10 Awakenings, traps + CPU profile

**Test Build Fixed**
- `CMakeLists.txt`: `FamilyWaveguideTest.cpp` moved to its own `add_executable(FamilyWaveguideTest ...)` — eliminates duplicate `main()` linker error
- `XOmnibusTests` now compiles cleanly

**Fleet — 44/44 Seanced**
- All 44 engines now seanced. OWARE was the last.
- Fleet avg: ~8.7/10 (up from ~8.6)
- 5 engines at 9.0+: **OVERBITE 9.2, OWARE 9.2**, OBSCURA 9.1, OUROBOROS 9.0, OXBOW 9.0
- Blessings total: 31 → **34** (B032/B033/B034 for OWARE)

**Preset Annotation Pass**
- 1021 existing presets annotated with `(EngineName)` suffix in name field
- Format: `"name": "DESCRIPTION ... (EngineName)"`
- Improves preset browser discoverability outside engine context

**Shared DSP Migrations (prior overnight agents, confirmed committed)**
- Sprint 1 migration: 10,044 presets migrated across 44 engines
- OceanDeep migrated to StandardLFO + PitchBendUtil

### Commits This Session

```
1a381c67e  CLAUDE.md: OWARE 9.2, fleet 44/44 seanced, add B032-B034 Blessings
f33548f08  Add docs, tools, mythology, visionary journal
44d978c4e  Presets: annotate multi-engine presets with primary engine suffix
ebcd7178b  OWARE: expand to 150 presets — add 30 (Family, Submerged, Aether, Flux)
b21d91018  ORBWEAVE: add 13 new presets across 8 moods
ce868ff51  ORBWEAVE: fix Default Trap — braidDepth 0.5→0.2
9078cf598  Fix test build: move FamilyWaveguideTest to its own executable
5f10fd838  OWARE: set lfo1Depth default 0.0 → 0.1 (seance re-score 9.0 → 9.2)
```

### Build Status

**AU + auval PASS** (2026-03-21, verified after ORBWEAVE engine change)

---

## Current State

| Metric | Value |
|--------|-------|
| Engines integrated | 44 |
| Engines seanced | 44/44 ✅ |
| Fleet avg score | ~8.7/10 |
| Engines at 9.0+ | 5 (OVERBITE 9.2, OWARE 9.2, OBSCURA 9.1, OUROBOROS 9.0, OXBOW 9.0) |
| Total Blessings | 34 |
| Presets | ~17,500+ |
| OWARE presets | 150 |
| ORBWEAVE presets | 150+ (13 new this session) |
| Build | AU PASS, auval PASS |

---

## Immediate Next Actions

### Priority 1 — Sound Quality

1. **Guru Bin retreat: OWARE** — 7 Pillars parameter refinement (Material Continuum, Mallet Physics, Sympathetic Resonance, Resonator Body, Buzz Membrane, Breathing Gamelan, Thermal Drift). Retreat doc doesn't exist yet; write it.

2. **Guru Bin retreats: ORBWEAVE, OVERTONE, ORGANISM** — no retreats yet. These three are the remaining un-retreated V2 engines.

3. **Push 6 weakest engines to 8.0+**: OBLIQUE, OCELOT, OBESE, ODDOSCAR, ODDFELIX, ORPHICA. All scored below 8.5 last seance.

### Priority 2 — Documentation

4. **Sound design guide**: Add entries for OBRIX, ORBWEAVE, OVERTONE, ORGANISM, OXBOW, OWARE to `Docs/xomnibus_sound_design_guides.md` (6 engines missing)

5. **Update fleet seance scores doc**: `Docs/fleet-seance-scores-2026-03-20.md` — add OWARE 9.2 post-fix row

### Priority 3 — Release & Content

6. **Execute Week 1 of 6-month release calendar** — see `Docs/content-backlog-6-month-2026.md`

7. **Record 20 hero audio clips** — see `Docs/site-sample-recordings.md`

8. **Create Patreon account** — 18 placeholder URLs in `Docs/patreon-url-sweep-2026-03-20.md`

9. **Render WAVs for Oxport** — pipeline complete, needs audio renders

### Priority 4 — Engineering

10. **Migrate remaining engines to shared DSP utilities** — Sonnet/medium work. Most engines still use inline LFO/envelope patterns instead of StandardLFO/FilterEnvelope from `Source/DSP/`

11. **OWARE V2 backlog** (from seance):
    - Asymmetric buzz nonlinearity (upper partials vs. lower partials different density)
    - Expose `thermalRetargetRate` as a tweakable param (currently hardcoded 0.001f)
    - Document that shimmer runs at Hz-domain rate (4Hz ≈ musical vibrato, not 0.3Hz breath)
    - Consider renaming COUPLING macro to SYMPATHY in the UI

12. **Fab Five on OBRIX** — style pass before launch

---

## Key Files Modified This Session

| File | Change |
|------|--------|
| `Source/Engines/Oware/OwareEngine.h` | lfo1Depth default 0.0→0.1 |
| `Source/Engines/Orbweave/OrbweaveEngine.h` | braidDepth default 0.5→0.2 |
| `CMakeLists.txt` | FamilyWaveguideTest separated into own executable |
| `CLAUDE.md` | 44/44 seanced, OWARE 9.2, B032-B034, 34 total blessings |
| `Docs/guru-bin-retreats/oxbow-retreat-2026-03-21.md` | NEW — OXBOW retreat (854 lines) |
| `Presets/XOmnibus/*/Oware_*.xometa` | 30 new presets |
| `Presets/XOmnibus/*/ORBWEAVE_*.xometa` | 13 new presets |
| 1021 existing presets | (EngineName) suffix added to name field |

---

## Technical Notes

### git Gotchas Learned This Session

- **macOS case-insensitive staging**: If git tracks `OceanDeepEngine.h` (uppercase D) but file is `OceandeepEngine.h` (lowercase), `git add` silently fails. Fix: `git diff HEAD -- "Path/To/File.h" | git apply --cached`
- **auval codes are case-sensitive**: Always `aumu Xomn XoOx` (read from Info.plist). NOT `XOMn Xa_X` or any variant.
- **Duplicate main() linker error**: If a test file has its own `main()`, it must be its own `add_executable()` target, not included in `XOmnibusTests`.

### OWARE Architecture Reference

Engine: `Source/Engines/Oware/OwareEngine.h`
Prefix: `owr_`
Color: Akan Goldweight `#B5883E`
Voices: 8 | Modes: 8
Seance score: 9.2/10 (2026-03-21)

**7 Pillars:**
1. Material Continuum (wood↔metal↔bell↔bowl) — `owr_material`
2. Mallet Physics (Chaigne 1997) — `owr_malletHardness`, `owr_malletMass`
3. Sympathetic Resonance Network — `owr_sympathy` (SYMPATHY macro M3)
4. Resonator Body — `owr_bodyResonance`, `owr_bodyDecay`
5. Buzz Membrane (balafon mirliton) — `owr_buzzAmount` (default 0.15)
6. Breathing Gamelan (shimmer) — `owr_shimmerRate` (default 4.0 Hz)
7. Thermal Drift — `owr_thermalDrift`

**Blessings:** B032 (Mallet Articulation Stack), B033 (Living Tuning Grid), B034 (Per-Mode Sympathetic Network)
