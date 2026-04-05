# Synth Seance Verdict: OXIDIZE (XOxidize)

**Engine #77** | **Seance Date:** 2026-04-05 | **Score: 8.7/10 (pre-fix) → 9.2+ post-fix**

## Concept
Temporal degradation synthesis — the Second Law of Thermodynamics as a performance interface. Per-voice age accumulator drives 6-stage corrosion cascade. Accent: Verdigris #4A9E8E.

## Ghost Verdicts Summary

| Ghost | Core Observation | Blessing | Warning |
|-------|-----------------|----------|---------|
| **Moog** | Corrosion→erosion chain is "causally entangled" — not standard subtractive | Sediment FDN outlives its notes | Aftertouch→age resistance must be <5ms latency |
| **Buchla** | 7 independent LUT curves = "differential equation with character" | Independent LUT curves = genuine architecture | Valve/tanh is beneath this engine |
| **Smith** | Per-voice independent aging is the key design insight | Shared sediment + independent voice aging | One Luminous preset — own that decision |
| **Kakehashi** | Tape Memory justifies the engine — democratic lo-fi instrument | Tape Memory = lo-fi tool for the world | Fix macro defaults; add tempo-sync dropout |
| **Pearlman** | Sediment FDN = best DSP engineering in engine | FDN engineering — coprime, Hadamard, correct | Voice-steal crossfade missing; M3/M4 dropout stacking |
| **Tomita** | ageCurve = choosing the shape of duration (unprecedented) | Three-dimensional timbral sculpture | No Luminous OXIDIZE preset |
| **Vangelis** | "The instrument weeps. It earns the name." | Mod wheel controlling time = cinematic dramaturgy | Don't over-engineer the macros |
| **Schulze** | Engine thinks in correct time units — narrative arc | Thinks in correct time units | Label aftertouch "preservation" |

## Points of Agreement
1. **Sediment FDN is exceptional** (5 ghosts) — coprime delays, Hadamard feedback, 300s T60
2. **Aftertouch "fight entropy" is philosophically profound** (5 ghosts)
3. **Independent voice aging is key innovation** (4 ghosts)
4. **Needs Luminous coverage** (3 ghosts)
5. **Can score film** (3 ghosts)

## Points of Contention
- **Valve/tanh**: Buchla says lazy, Moog says earned if drive is right — OPEN
- **Aftertouch labeling**: Schulze wants "preservation", Vangelis says directness IS the soul — OPEN
- **Sediment tail default**: Pearlman says 0.5 too aggressive, Schulze says show the edge — lean Pearlman

## Issues

### P0 (Must Fix)
- **P0-01**: Macro defaults 0.5 in snapshot vs 0.0 in Init Pristine — blank patches differ from init
- **P0-02**: Voice-steal crossfade likely missing — resetState() before noteOn may click

### P1 (Should Fix)
- **P1-01**: Valve/tanh mode is generic (Buchla)
- **P1-02**: No Luminous OXIDIZE preset (Tomita, Smith, Kakehashi)
- **P1-03**: M3/M4 both stack dropoutRate — give M4 dropoutSmear instead (Pearlman)

### P2 (Consider)
- **P2-01**: Dropout gate lacks tempo-sync option (Kakehashi)
- **P2-02**: sedimentTail default 0.5 may be aggressive (Pearlman)
- **P2-03**: FDN character changes at 96kHz (Pearlman)
- **P2-04**: Add 1-2 mainstream-friendly presets (Kakehashi)

## Blessings Earned
- **B044 (proposed)**: Age Accumulator with Independent LUT Curves — 7 independent mathematical curves (S-curve, exponential, logarithmic, quadratic, sqrt, linear) indexed by a single time axis create emergent compositional counterpoint between degradation stages. Proposed by Buchla, confirmed by Schulze, Tomita.
- **B040 (shared with OXYTOCIN)**: Note Duration as Synthesis Parameter — OXIDIZE is the entropy side of B040's coin. OXYTOCIN accumulates love; OXIDIZE accumulates degradation. Same mechanism, opposite meaning.

## Debates Touched
- **DB005** (Autonomy vs Agency): The aftertouch "fight entropy" is a new data point — the performer physically resists the engine's autonomous process
- **DB002** (Silence as paradigm): OXIDIZE's dropout gate creates silence as a structural element, not an absence

## Score Justification
- Concept novelty: 9.5 — time as primary synthesis parameter with independent temporal curves is unprecedented
- DSP quality: 9.0 — CytomicSVF, Hadamard FDN, coprime delays, denormal-safe, zero audio-thread allocation
- Parameter design: 8.5 — 50 params well-organized; macro defaults mismatch (P0-01) and M3/M4 stacking (P1-03) lower score
- Expression: 9.0 — aftertouch-fights-entropy (Vangelis), velocity-as-temporal-birth (Tomita), mod-wheel-as-time
- Preset coverage: 8.0 — 56 presets good but Luminous gap and no mainstream-friendly patches
- Coupling: 8.5 — 4 coupling types semantically sound; AM→age is physically correct
- **Overall: 8.7/10** → **9.2+ after P0 fixes + Luminous presets**
