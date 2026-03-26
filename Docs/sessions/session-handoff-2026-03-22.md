# Session Handoff — 2026-03-22

## Session Summary

Session 2026-03-21 (continued across three context windows).

### What Was Accomplished

**OWARE Engine — Final Fix + Re-Seance → 9.2/10**
- All 3 seance-required fixes: LFO1+LFO2 real StandardLFO objects; shimmerRate reads `owr_shimmerRate`; buzzAmount default 0.15; lfo1Depth 0.1 default
- Re-seance confirmed: **9.2/10** with Blessings B032, B033, B034

**ORBWEAVE — Default Trap Fixed**
- `weave_braidDepth` default: 0.5 → 0.2 (below WEAVE-I threshold)
- 13 new presets (all topologies)

**Fleet — 44/44 Seanced, All Engines at 8.0+**
- 5 weakest engines fixed: OBESE 6.6→8.5, ODDOSCAR 6.9→8.5, ODDFELIX ~7.0→8.5, OCELOT 6.4→8.5, ORPHICA 8.0→8.7
- Fleet avg: ~8.8/10 (up from ~8.7)

**OPERA Engine — Engine #45 Integrated**
- Kuramoto vocal synthesis: additive partials + phase synchrony + formant breath
- Files: KuramotoField.h, OperaPartialBank.h, OperaBreathEngine.h, OperaConductor.h, ReactiveStage.h, OperaEngine.h, OperaAdapter.h/.cpp
- Registered in XOlokunProcessor.cpp + PresetManager.h
- 180 factory presets across 8 moods (macros: DRAMA/VOICE/CHORUS/STAGE)
- **Build + auval PASS** (45 engines)

**Coupling Phase D — Tutorial + Demo Presets**
- `Docs/tutorials/playing-the-space-between.md` — full producer's guide
- 8 COUPLING_* demo presets in Entangled mood
- Support docs: recipe-design-process.md, xpn_intent_schema.md, templates, render_specs

**DSP Migrations — 8 Engines Complete**
- Obbligato, Ombre → StandardLFO/ParameterSmoother
- Bite, Ohm, Ole, Onset, Orbital, Ottoni → GlideProcessor
- Remaining with inline patterns: Organon, Ottoni (done), Ouroboros, Overworld, Optic

**Guru Bin Retreats — 4 New (all V2 Theorem engines)**
- OWARE: `Docs/guru-bin-retreats/oware-retreat-2026-03-21.md` (7 Pillars)
- ORBWEAVE: `Docs/guru-bin-retreats/orbweave-retreat-2026-03-21.md`
- OVERTONE: `Docs/guru-bin-retreats/overtone-retreat-2026-03-21.md`
- ORGANISM: `Docs/guru-bin-retreats/organism-retreat-2026-03-21.md`

**Build + auval: PASS** (verified after OPERA integration)

### Commits This Session

```
3ad43d0ff  Add OCELOT preset: Kalimba and Rain (Foundation)
a838500e9  DSP migration batches 1-3: 6 engines → shared GlideProcessor/StandardLFO
5a8d09134  OPERA: 180 factory presets across 8 moods
69b42cf9d  Coupling Phase D: 8 demo presets for 'Playing the Space Between'
f7481de19  Coupling Phase D: 'Playing the Space Between' tutorial + support docs
b38eed9ab  DSP migration batch 1-2: Obbligato + Ombre → shared DSP utilities
f3d724e03  OPERA: integrate engine #45 — Kuramoto vocal synthesis (Aria Gold)
a863d4088  Add Oware to CMakeLists.txt + Opera engine scaffold
5e5377f12  Sprint 3 preset quality: mood fixes, descriptions, Submerged coverage
75aa9448e  Add 60 new Awakening presets across 6 engines
85302b0a0  Fix D002/D004/D005 across 5 weak engines: OBESE, ODDOSCAR, ODDFELIX, OCELOT, ORPHICA
```

---

## Current State

| Metric | Value |
|--------|-------|
| Engines integrated | 45 |
| Engines seanced | 44/44 ✅ (OPERA unseanced — built this session) |
| Fleet avg score | ~8.8/10 |
| Engines at 9.0+ | 5 (OVERBITE 9.2, OWARE 9.2, OBSCURA 9.1, OUROBOROS 9.0, OXBOW 9.0) |
| Total Blessings | 34 |
| Presets | ~18,000+ |
| OPERA presets | 180 (first-pass factory library) |
| Build | AU PASS, auval PASS |

---

## Immediate Next Actions

### Priority 1 — OPERA Engine

1. **Seance OPERA** — Engine #45 needs initial seance evaluation. DSP is complex (Kuramoto synchrony + additive + breath). Target: 8.5+.

2. **OPERA Guru Bin Retreat** — No retreat yet. 7 parameters to refine: drama arc, voice quality, chorus coupling, stage depth.

### Priority 2 — Sound Quality

3. **Guru Bin retreat: OWARE** — 7 Pillars parameter refinement doc written (`oware-retreat-2026-03-21.md`). Preset expansion target: 150 (currently 150 ✅).

4. **Fab Five on OBRIX** — Style pass before launch.

### Priority 3 — Engineering

5. **Complete DSP migrations** — Remaining engines still using inline patterns:
   - Organon, Ouroboros, Overworld (batch 3 partially complete)
   - Optic (confirmed not migrated)
   - MorphEngine, SnapEngine, OrphicaEngine (modified this session — defer)

6. **OWARE V2 backlog** (from seance):
   - Asymmetric buzz nonlinearity (upper vs lower partial density)
   - Expose `thermalRetargetRate` as tweakable param (currently 0.001f hardcoded)
   - Document shimmer runs at Hz-domain rate (4Hz = musical vibrato)
   - Rename COUPLING macro → SYMPATHY in UI

### Priority 4 — Release & Content

7. **Execute Week 1 of 6-month release calendar** — `Docs/content-backlog-6-month-2026.md`

8. **Record 20 hero audio clips** — `Docs/site-sample-recordings.md`

9. **Create Patreon account** — 18 placeholder URLs in `Docs/patreon-url-sweep-2026-03-20.md`

10. **Render WAVs for Oxport** — pipeline complete, needs audio renders

---

## Technical Notes

### Git Gotchas
- **macOS case-insensitive staging**: `git diff HEAD -- "Path/File.h" | git apply --cached`
- **auval codes are case-sensitive**: Always `aumu Xolk XoOx`
- **Duplicate main() linker error**: Test files with own `main()` need separate `add_executable()`
- **New engines need CMakeLists.txt entry**: Both .h and .cpp (one-line `#include` stub) required

### OPERA Architecture Reference
Engine: `Source/Engines/Opera/OperaAdapter.h`
Prefix: `opera_`
Color: Aria Gold `#D4AF37`
Key files: KuramotoField.h (phase coupling), OperaPartialBank.h (additive), OperaBreathEngine.h (breath), OperaConductor.h (arc system), ReactiveStage.h (stage reverb)
Macros: DRAMA / VOICE / CHORUS / STAGE
Seance score: unseanced (built 2026-03-21)

### Untracked Files to Ignore
These untracked files are experiment stubs — not part of any current sprint:
- `Source/Engines/Octave/`, `Source/Engines/Oleg/` (empty/stub)
- `Source/Engines/Oto/OtoEngine.h` (Kitchen Collection concept)
- `Docs/octave-engine-architecture.md`, `Docs/oleg-engine-architecture.md`
- `Docs/cookbooks/`, `Docs/kitchen-collection-release-calendar.md` (V2 concept docs)
- `Presets/XOlokun/Deep/`, `Presets/XOlokun/Ethereal/`, `Presets/XOlokun/Kinetic/`, `Presets/XOlokun/Luminous/`, `Presets/XOlokun/Organic/` (non-standard mood dirs)
- `Docs/playing-the-space-between-tutorial.md` (duplicate of `Docs/tutorials/` version)
