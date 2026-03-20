# OSPREY Seance Verdict -- 2026-03-20

**Engine:** XOsprey | **Gallery Code:** OSPREY | **Accent:** Azulejo Blue #1B4F8A
**Creature:** The Coastal Hunter | **Param Prefix:** `osprey_`

---

## Ghost Council Score: 8.4 / 10

**Previous verdict:** APPROVE/CONDITIONAL (non-numeric)

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 Velocity->Timbre | PASS | D001 filter envelope: velocity * envelope level sweeps tilt LP cutoff upward. paramFilterEnvDepth controls the sweep range. Harder hits = brighter resonator timbre. |
| D002 Modulation Depth | PASS | Sea state LFO (OspreyLFO instance, 0.05-1.0 Hz, sine shape). 4 macros. Aftertouch + mod wheel. D005/D004 fix instantiated the LFO that was previously dead code. |
| D003 Physics Rigor | PASS | ShoreSystem (B012) implements 5-coastline cultural data with physical resonator modeling. Spring-mass shore velocity with elastic overshoot. |
| D004 No Dead Params | PASS | Fixed in D005/D004 patch: OspreyLFO was fully written but never instantiated. Now instantiated as seaStateLFO member and wired to effectiveSeaState modulation. |
| D005 Must Breathe | PASS | seaStateLFO at 0.05 Hz (20-second cycle) minimum rate. Rate scales with M2 MOVEMENT macro up to 1.0 Hz. |
| D006 Expression | PASS | Aftertouch -> shore blend position shift (pressure moves toward rougher shoreline character). Mod wheel CC1 -> sea state / turbulence intensity (+0.4). Both smoothed. |

---

## DSP Quality Assessment

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Anti-aliasing | GOOD | Resonator-based synthesis. Post-output tilt LP filter. |
| Denormal protection | GOOD | Standard denormal flushing in filter and envelope paths. |
| Sample rate independence | GOOD | sampleRateFloat used consistently for all timing. |
| Parameter smoothing | ADEQUATE | Per-block reads. Sea state LFO rate updated per block. |
| Shore system | GOOD | B012 ShoreSystem shared with Osteria. 5 coastline profiles. |

---

## Strengths

1. **B012 ShoreSystem** -- Shared cultural data system with Osteria. 5 coastline profiles provide distinct sonic character.
2. **D005/D004 recovery** -- The OspreyLFO struct was fully implemented and elegant but never instantiated. The fix was clean: add member, wire to sea state. Good engineering.
3. **Filter envelope** -- D001 properly satisfied via velocity-scaled tilt LP cutoff.
4. **Expression design** -- Aftertouch shifts shore blend (timbre), mod wheel controls turbulence (energy). Complementary axes.

## Issues Preventing Higher Score

1. **Single LFO** -- Only seaStateLFO (sine shape only). No user-controllable LFO with shape selection or multiple destinations.
2. **No mod matrix** -- LFO destination hardcoded to sea state amplitude.
3. **LFO shape fixed** -- seaStateLFO is initialized to sine shape (setShape(0)) with no user control.

---

## Path to 9.0

1. Add user-controllable LFO shape param for seaStateLFO. ~10 LOC.
2. Add a second OspreyLFO with rate/depth/shape params routed to resonator tuning or filter. ~50 LOC.

## Path to 9.5

3. Add 4-slot mod matrix. ~100 LOC.

**Estimated effort to 9.0:** 60 LOC, 45 minutes.
