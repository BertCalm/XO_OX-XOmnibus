# Seance Verdict: OPSIN
**Date:** 2026-04-12
**Seance Type:** First seance — no prior record
**Score: 8.4 / 10**
**V1 Status: NOT READY — preset library below minimum threshold**
**Blessing: B045 "The Rewiring Instrument" awarded**

---

## Architecture
- 6-node Pulse-Coupled Oscillator (PCO) network — no traditional oscillators
- Sound emerges from mutual excitation of 6 delay-line PhotophoreNodes
- Synapse weight matrix (6×6) evolves via Hebbian plasticity during performance
- 5 topologies: Ring, Star, Mesh, Cascade, Random
- 8 materials: Membrane, Glass, Crystal, Coral, Void, Metal, Silk, Plasma (per-node CytomicSVF coloring)
- Energy Governor: per-node soft limit + global RMS ceiling
- Amp ADSR + Filter ADSR (outer VCA/VCF shaping)
- 2 LFOs (floor 0.01Hz) with 4 routable targets each
- 4-slot mod matrix (sources: velocity, mod wheel, aftertouch, LFO1/2 → destinations: filter, feedback, threshold, excitation, density, amp)
- 4 macros: BIOLUME (excitation+feedback), PLASTICITY (learning+memory), CHAIN REACTION (density+scatter), SPACE (cutoff+spread) — all unconditional
- Identity: Deep-sea anglerfish | Accent: Bioluminescent Cyan #00FFCC | Prefix: `ops_`

## Citations (D003)
- Mirollo & Strogatz (1990): pulse-coupled oscillator synchronization
- Hebb (1949): fire-together-wire-together plasticity
- David Tudor "Neural Synthesis" (1989–1995): feedback network synthesis

---

## Ghost Panel

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 7.0 | "Not a single analogue trace. I respect it but I don't recognize it." |
| Buchla | 9.5 | "This is the architecture I was reaching for. Pulse-coupled oscillators with Hebbian rewiring — Tudor did it in hardware, this does it in software. The engine teaches itself." |
| Smith | 8.0 | "Clean architecture. MIDI handling correct. Energy governor prevents runaway — correct engineering." |
| Kakehashi | 7.5 | "Twenty-one presets is thin, but the names invite exploration: Tudor Circuit, Hebbian Pad, Void Resonance." |
| Ciani | 8.5 | "Firing asymmetry creates genuine spatial divergence. The 500ms silence gate is correct — network tails need room." |
| Schulze | 9.0 | "An engine that sounds different after ten minutes than it did at the start. That is what I have been trying to compose." |
| Vangelis | 7.5 | "Velocity shapes filter brightness but not network energy. I can't make it fiercer with a harder strike." |
| Tomita | 8.0 | "Eight material types provide real timbral range. Metal and Crystal are as distinct from Membrane as strings from brass." |

---

## Doctrine Compliance

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | ⚠️ PARTIAL | `velFenvScale = 0.5 + velocity * 0.5` → filter brightness only. Network firing energy is velocity-insensitive. |
| D002 | ✅ PASS | 2 LFOs (0.01Hz floor), 4-slot mod matrix, Hebbian learning as autonomous long-term modulation. All 4 macros unconditional. |
| D003 | ✅ PASS | Three citations: Mirollo & Strogatz 1990, Hebb 1949, Tudor 1989–1995. Implementation follows cited models. |
| D004 | ✅ PASS | All parameters live and routed. `ops_excNode` audibly selects excited node. |
| D005 | ✅ PASS (exceptional) | LFOs at 0.01Hz + Hebbian rewiring creates open-ended performance evolution with no repetition. |
| D006 | ✅ PASS | Mod wheel and aftertouch both fed into modSrc and routable to any matrix destination. |

---

## Preset Assessment

**Count:** 21 presets — all in `Source/Engines/Opsin/Presets/Foundation/`

**V1 minimum:** 100 recommended | **Gap:** 79 presets

**Macro effectiveness:** All 4 macros unconditional and audible — superior to Observandum.

**Naming quality:** Outstanding. Tudor Circuit, Hebbian Pad, Void Resonance, Photophore Bells, Glass Cascade.

**Init concern:** Non-zero macro defaults (BIOLUME=0.5, PLASTICITY=0.3) are architecturally honest — a zeroed network produces silence. Init description should say "stable resting state" not "blank canvas."

---

## Coupling

**Output:**
- ch0/ch1: Post-filter stereo
- ch2: **Network firing rate 0–1** — unique signal in the fleet; encodes temporal pattern of node firings

**Input types:**
- `AudioToFM` → round-robin node injection (external audio enters network firing pattern)
- `AmpToFilter` → filter cutoff
- `EnvToMorph` → feedback amount
- `RhythmToBlend` → excitation burst trigger

**Natural partners:** Onset (rhythm → bursts), Ouroboros (chaos → feedback), Opsin ↔ Opsin (Mirollo-Strogatz synchronization demo).

---

## Blessing

**B045: "The Rewiring Instrument"**

Opsin is the only engine in the 81-engine fleet whose sound-producing structure is physically different at the end of a performance than at the start. Hebbian plasticity modifies synapse weights in real time — the engine learns the performance and changes its behavior accordingly. This is not modulation of sound parameters; it is modification of the instrument itself. No prior fleet engine implements this behavior as a primary synthesis mechanism.

---

## Recommendations

### P0 — V1 blockers
1. **Expand presets to ≥100** across Foundation, Flux, Ethereal, Shadow, Kinetic minimum. Long-form Hebbian-evolution presets require curation.

### P1 — Important
2. **Add velocity → excitation** — `velocity * 0.2f` added to effective excitation so harder playing energizes the network, not just brightens the filter.
3. **Rename Init description** — "blank canvas" is misleading; use "stable resting state."

### P2 — Nice to have
4. **Opsin ↔ Opsin Entangled preset** — mutual PCO synchronization is a Mirollo-Strogatz demonstration in realtime.
5. **Distribute presets to correct mood folders** — Tudor Circuit → Flux folder, Hebbian Pad → Ethereal folder.

---

## Path to V1

**Required:** Preset library ≥100, velocity → excitation fix.
**Estimated score at that point:** 8.7–9.0. The architecture already merits the higher score.

---

*First seance — 2026-04-12 | Ringleader RAC Session | B045 awarded*
