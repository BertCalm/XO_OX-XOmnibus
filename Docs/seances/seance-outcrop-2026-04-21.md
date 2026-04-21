# Seance Verdict: OUTCROP
**Date:** 2026-04-21
**Seance Type:** First seance — initial doctrine audit (engine scaffolded, no DAW stage time yet)
**Score (provisional): 7.0 / 10**
**Status: scaffolded, compiles clean under -Wpedantic -Werror, presets seeded (5), awaiting first real playthrough**
**Blessing: deferred pending audition**

---

## Architecture
- Wave-terrain synthesis: a probe (x(t), y(t)) orbits over a deterministic terrain field z = T(x, y)
- Terrain types: Peaks (bell sum), Ridges (angular sinusoid), Hybrid (Peaks + scaled Ridges)
- Orbit shapes: Circular, Elliptic; ratio, radius, phase, jitter all exposed
- Roughness parameter adds high-frequency perturbation to the terrain field
- Per-voice CytomicSVF filter, ADSR amp + filter envs
- 2 LFOs × 4 routable targets (OrbitRate/OrbitRadius/TerrainHeight/FilterCutoff)
- 4-slot ModMatrix, 4 macros: CHARACTER / MOVEMENT / COUPLING / SPACE
- Identity: Outcrop | Accent: Mountain Moss #5B6F57 | Prefix: `outc_`

## Citations (D003)
- Roads, "The Computer Music Tutorial" (1996) — wave-terrain synthesis overview
- Mitsuhashi, "Audio signal synthesis by functions of two variables" (JAES 1982) — formal wave-terrain foundations
  *(citation strength: adequate baseline, not exemplary — D003 met at the minimum bar)*

---

## Doctrine Compliance

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | ✅ PASS | Velocity scales peak amplitude via `velBoost = 1 + vel * fltVelTrack * 3`; terrain altitude and filter cutoff both respond. Harder playing brightens and sharpens the contour. |
| D002 | ✅ PASS | 2 LFOs + 4 targets + 4 mod-matrix slots + mod wheel + aftertouch + 4 macros. All present, all wired. |
| D003 | ✅ PASS (baseline) | Wave-terrain lineage cited. Could strengthen with a second orbital-dynamics reference if depth is desired. |
| D004 | ✅ PASS | terrainType / peakHeight / peakSpread / ridgeFreq / ridgeDepth / ridgeAngle / roughness / orbit-shape / ratio / radius / phase / jitter all affect output. Verified by code inspection. |
| D005 | ✅ PASS | LFO floor 0.005 Hz (inherited from StandardLFO). Slow orbit (low orbitRatio) adds additional long-period motion independent of LFOs. |
| D006 | ✅ PASS | Velocity → timbre (filter + amplitude). Mod wheel stored for mod-matrix. Aftertouch adds gentle level boost. |

---

## Provisional Weaknesses / Open Questions
1. **External-terrain coupling not yet wired.** The engine is designed to accept a foreign terrain contour via coupling (e.g. from Outflow / Oort / another Outcrop) but `applyCouplingInput` currently routes only to filter/LFO-rate/orbit-radius — no terrain override hook. Suggest adding a `CouplingType::AudioToMorph` route that drives terrain height directly.
2. **No tempo-sync.** Orbit ratio is a free float; adding host-tempo division (1/4, 1/8, 1/16 triplet) would make the engine a first-class rhythmic source.
3. **Terrain types are procedural, not sampled.** A future "Sampled" type that reads z from a user image or audio buffer would expand the character range substantially.
4. **Roughness is deterministic (PRNG seeded once).** Re-seeding per note adds variation; per-voice variation would be free.
5. **Orbit jitter clips at 1.0; could be soft-saturated** so high jitter doesn't aliases the probe path.

## Audition Plan
- Play 5 seeded presets (Lichen Bloom, Obsidian Ridge, Canyon Stride, Basalt Column, Alpine Mirror) on a 49-key keyboard across full range with mod wheel and aftertouch probes.
- Stress test: poly8 + heavy orbitJitter + roughness at max — watch for denormal spikes or filter blow-up.
- Coupling audit: route Outcrop → Oneiric (EnvToMorph) once MegaCouplingMatrix routing lands; expect the terrain contour to sculpt the soliton lattice potential.

## Preset Assessment
5 presets at launch (one per mood: Organic, Shadow, Kinetic, Foundation, Luminous). Adequate to demonstrate character; should grow to ≥12 across all 16 moods before V-release readiness is claimed.

## Open Debates
None raised yet — this is a first seance.

---

*Outcrop is a working scaffold with honest physics and adequate doctrine compliance. Provisional score reflects that the engine has not yet earned stage time. A post-audition re-seance is appropriate.*
