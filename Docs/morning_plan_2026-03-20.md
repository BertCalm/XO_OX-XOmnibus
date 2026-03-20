# Morning Plan — 2026-03-20

**Context:** Session on 2026-03-19 shipped OBRIX Waves 1-3 (65 params), 750 presets for 5 deficit engines, OVERLAP+OUTWIT seance fixes, and dispatched 3 agents for OVERTONE/ORGANISM/ORBWEAVE. This plan covers what remains.

---

## Phase 1: Land Last Night's Work (10 min)
> **Trigger:** `just commit and push what the agents built last night`

The 3 background agents (ORBWEAVE DSP, OVERTONE seance+presets, ORGANISM seance+presets) should be complete. Commit and push their output.

---

## Phase 2: ORBWEAVE Integration (Sonnet, ~15 min)
> **Trigger:** `register ORBWEAVE in XOmnibus — add to XOmnibusProcessor.cpp, PresetManager.h, XOmnibusEditor.h, CMakeLists.txt, and CLAUDE.md`

The ORBWEAVE DSP was built by the agent but needs wiring:
- `Source/XOmnibusProcessor.cpp` — add `#include` + `registerEngine("Orbweave", ...)`
- `Source/Core/PresetManager.h` — add to engine list + alias map (`"Orbweave", "weave"`)
- `Source/UI/XOmnibusEditor.h` — add accent color `0xFF8E4585`
- `CMakeLists.txt` — add `Source/Engines/Orbweave/OrbweaveEngine.h` + `.cpp`
- `CLAUDE.md` — update engine count 39→42, add ORBWEAVE/OVERTONE/ORGANISM rows to table

---

## Phase 3: ORBWEAVE Seance + Presets (Opus, ~20 min)
> **Trigger:** `run seance on ORBWEAVE then generate 150 presets`

- `/synth-seance` on OrbweaveEngine.h
- Write `Tools/orbweave_preset_gen.py` (same pattern as obrix_preset_gen.py)
- Generate 150 presets across 7 moods

---

## Phase 4: 4 Missing Seances (Opus, ~20 min background)
> **Trigger:** `run seances on OSTINATO, OPENSKY, OCEANDEEP, OUIE — all 4 in parallel`

These engines shipped with DSP on 2026-03-18 but never got ghost verdicts:
- OSTINATO — Firelight Orange, Fire Circle drums
- OPENSKY — Sunburst, euphoric shimmer
- OCEANDEEP — Trench Violet, abyssal synth
- OUIE — Hammerhead Steel, duophonic

---

## Phase 5: Build Validation (Sonnet, ~10 min)
> **Trigger:** `/build-sentinel` or `cmake --build build && auval -v aumu Xomn XoOx`

Verify all 42 engines compile and auval passes with the 3 new additions.

---

## Phase 6: Post-Completion Checklists (Sonnet, ~15 min)
> **Trigger:** `/post-engine-completion-checklist OVERTONE, ORGANISM, ORBWEAVE`

Locks in documentation for the 3 new engines before facts drift.

---

## Phase 7: Full Audit Sweep (Sonnet, ~20 min)
> **Trigger:** `/sweep` or `run the full audit`

Dispatches parallel agents across code, docs, presets, brand, UI. This is the quality gate before declaring the session done. Covers:
- Code sweep (new engines, seance fixes)
- Preset QA (new 150×3 = 450 presets)
- Documentation accuracy (CLAUDE.md, master spec, preset gap analysis)
- Memory staleness

---

## Phase 8: Fleet Inspector (Sonnet, ~5 min)
> **Trigger:** `/fleet-inspector`

Final dashboard: all 42 engines, preset counts, doctrine compliance, seance coverage. This produces the "state of the fleet" snapshot.

---

## Phase 9: Memory + Docs Cleanup (Sonnet, ~10 min)
> **Trigger:** `/historical-society`

Updates MEMORY.md, satellite files, CLAUDE.md engine counts, preset gap analysis. Removes stale entries (like "seances pending" for engines that now have verdicts).

---

## Optional / Lower Priority

| Task | Trigger | Notes |
|------|---------|-------|
| **Aquarium update** | `/mythology-keeper OVERTONE, ORGANISM, ORBWEAVE` | Water column placement for 3 new engines |
| **Field Guide posts** | `/field-guide-editor` | 3 new engines = 3 new guide entries |
| **Guru Bin retreats** | `/guru-bin OVERTONE` then ORGANISM, ORBWEAVE | Find sweet spots, write Scripture |
| **XPN packs** | `/oxport` | XObese .xpn bundle still pending |
| **Hero audio clips** | Manual recording session | 20 clips needed for site (see site-sample-recordings.md) |
| **Patreon URL** | Manual | Still placeholder `patreon.com/xoox` |
| **OSTINATO/OPENSKY/OCEANDEEP/OUIE Guru Bin** | `/guru-bin [engine]` | Sweet spots + Scripture for the 4 new engines |

---

## Quick Copy-Paste Morning Sequence

```
# Phase 1-2 (Sonnet)
just commit and push what the agents built last night
register ORBWEAVE in XOmnibus — add to XOmnibusProcessor.cpp, PresetManager.h, XOmnibusEditor.h, CMakeLists.txt, and CLAUDE.md

# Phase 3 (switch to Opus)
run seance on ORBWEAVE then generate 150 presets

# Phase 4 (Opus, parallel agents)
run seances on OSTINATO, OPENSKY, OCEANDEEP, OUIE — all 4 in parallel

# Phase 5-9 (switch back to Sonnet)
/build-sentinel
/post-engine-completion-checklist OVERTONE, ORGANISM, ORBWEAVE
/sweep
/fleet-inspector
/historical-society
```

---

## Session Totals (2026-03-19)

| Metric | Before | After |
|--------|--------|-------|
| Engines with DSP | 39 | 42 (+OVERTONE, ORGANISM, ORBWEAVE) |
| Presets | ~10,500 | ~14,800 (+750 deficit fill) |
| Seance verdicts | 30 | 34 (+OVERLAP, OUTWIT, OVERTONE, ORGANISM) |
| Preset deficit engines | 5 | 0 (RESOLVED) |
| OBRIX params | 60 | 65 (Wave 3 shipped) |
| DSP fixes applied | — | 7 (OVERLAP 3 + OUTWIT 4) |
