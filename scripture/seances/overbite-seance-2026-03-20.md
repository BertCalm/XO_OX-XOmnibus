# OVERBITE Seance Verdict -- 2026-03-20

**Engine:** XOverbite (XOppossum) | **Gallery Code:** OVERBITE | **Accent:** Fang White #F0EDE8
**Creature:** The Opossum | **Param Prefix:** `poss_`

---

## Ghost Council Score: 9.2 / 10

**Previous verdict:** Full approval (non-numeric)

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 Velocity->Timbre | PASS | Velocity scales filter envelope depth (filtEnvAmt * filtEnvVal * velocity * 10000 Hz). Amp velocity sensitivity param. Harder hits open filter = brighter. |
| D002 Modulation Depth | PASS | 3 LFOs (7 shapes each: Sine/Tri/Saw/Square/S&H/Random/Stepped). 3 ADSR envelopes (amp/filter/mod). Mod matrix routing ("Off/LFO1/LFO2/LFO3/AmpEnv/FilterEnv/ModEnv"). Aftertouch + mod wheel. 5 macros (BELLY/BITE/SCURRY/TRASH/PLAY DEAD). |
| D003 Physics Rigor | N/A | Not a physical model. |
| D004 No Dead Params | PASS | All params wired to DSP. 5-macro system all produce audible change. |
| D005 Must Breathe | PASS | LFO rates go down to sub-0.01 Hz range. Drift LFO at 0.37 Hz provides autonomous motion. |
| D006 Expression | PASS | Aftertouch -> BITE macro intensity (+0.3). Mod wheel CC1 -> BITE macro depth (+0.4). Both smoothed via PolyAftertouch. |

---

## DSP Quality Assessment

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Anti-aliasing | GOOD | Standard subtractive synthesis with SVF filters. |
| Denormal protection | GOOD | Standard flushDenormal patterns. |
| Sample rate independence | GOOD | All rates derived from sampleRate. |
| Parameter smoothing | GOOD | Per-block reads + smoothing where needed. |
| Modulation architecture | EXCELLENT | 3 LFOs + 3 envelopes + mod routing = deepest modulation system in fleet. |

---

## Strengths

1. **B008 Five-Macro System** -- BELLY/BITE/SCURRY/TRASH/PLAY DEAD. All 8 ghosts praised this. The best expression system in the fleet.
2. **3 LFOs + 3 envelopes** -- Most modulation sources of any engine. 7 LFO shapes each.
3. **Mod matrix routing** -- Each destination can select from 7 modulation sources.
4. **D002 exemplar** -- If D002 compliance were a competition, OVERBITE wins. 3 LFOs, 3 envelopes, 5 macros, mod wheel, aftertouch.
5. **342 presets** -- Largest preset library demonstrates the engine's versatility.

## Issues Preventing Higher Score

1. **No formal mod matrix with depth control** -- Routing is per-destination (select source), but there is no bipolar depth knob per routing slot. A proper mod matrix would add depth, polarity, and multiple destinations per source.
2. **Drift LFO at 0.37 Hz** -- This is good but higher than the ultra-slow breathing of some engines. The user LFOs can go slower, so this is minor.

---

## Path to 9.5

1. Add bipolar depth control to each mod routing slot (currently source selection only). ~60 LOC.
2. Add a second destination per LFO for more complex modulation patches. ~40 LOC.

## Path to 10.0

3. Add a dedicated 8-slot mod matrix with source/destination/depth/polarity. ~150 LOC.

**Estimated effort to 9.5:** 100 LOC, 1.5 hours.
