# OVERLAP Seance Prep Document

**Engine**: XOverlap (OVERLAP)
**Prepared**: 2026-03-16
**Status**: Phase 4 COMPLETE (installed 2026-03-15) | auval PASS | Seance PENDING
**Source files**: `Source/Engines/Overlap/XOverlapAdapter.h`, `Source/Engines/Overlap/DSP/`
**Architecture brief**: `~/.claude/projects/-Users-joshuacramblet/memory/xoverlap-architecture.md`

---

## 1. Engine Identity

| Field | Value |
|-------|-------|
| Gallery Code | OVERLAP |
| Creature | Lion's Mane jellyfish (*Cyanea capillata*), Glacier Bay, Alaska |
| Accent Color | Bioluminescent Cyan-Green `#00FFB4` |
| Parameter Prefix | `olap_` |
| Plugin Code | OVLP |
| Voice Count | 6 (each voice = one FDN delay line) |
| Poly / Mono / Legato | All three via `olap_voiceMode` |
| Total Parameters | 41 |
| Presets | 150 (Atmosphere 49 / Entangled 40 / Flux 25 / Aether 17 / Prism 15 / Foundation 4) |

---

## 2. DSP Architecture Summary

OVERLAP is a **6-voice Feedback Delay Network synthesizer** whose routing matrix topology is derived from mathematical knot crossing patterns. It is the only engine in the fleet — or any known commercial synthesizer — where resonance structure is geometrically determined by knot invariants.

### Signal Flow

```
MIDI → Voice Allocator (6 voices)
  → Per Voice: Bell Oscillator (sine + 3 inharmonic partials)
             → Bell-Pulse Envelope (cosine contraction, Kuramoto-coupled)
             → ADSR Amplitude Envelope
             → FDN Input Bus
  → 6-Line Feedback Delay Network
      Routing matrix determined by olap_knot:
        UNKNOT       — 6×6 identity (pure self-resonance)
        TREFOIL      — 3-crossing rotation, two triads
        FIGURE-EIGHT — 4-crossing amphichiral (symmetric M[i][j] = M[j][i])
        TORUS T(p,q) — harmonic ratio locking via delay length ratios
      Tangle depth interpolates identity → full knot matrix
      50ms crossfade on knot type change
  → Entrainment Bus (Kuramoto coupling model across 6 pulse oscillators)
  → Bioluminescence Layer (onset-triggered bandpass noise burst)
  → Stereo Spread (6 voices: -1.0 to +1.0)
  → Post FX:
      Ocean Current Chorus (allpass-modulated, 0.01–0.5 Hz)
      Den Diffusion (4-stage Schroeder allpass)
  → Output Cache (L/R per sample for coupling reads)
```

### Key DSP Concepts

- **FDN Knot Matrix**: The crossing pattern of a mathematical knot (Gauss code) is encoded directly as the N×N feedback routing matrix. Switching knot type reconfigures the entire harmonic lattice geometry. No other synth in the fleet or any commercial instrument does this.
- **Tangle Depth**: `M_effective = (1-d)*I + d*M_knot` — smooth morphing from clean ring (identity) to full topological entanglement.
- **Bell Oscillator**: Sine fundamental + 3 inharmonic partials (ratios 2.17, 3.71, 5.43). Brightness controlled by velocity (D001 compliance): `brightness = 0.3 + 0.7 * velocity`.
- **Kuramoto Entrainment**: Phase-coupling model for 6 pulse oscillators. At high `olap_entrain`, voices phase-lock (bloom pulsing in unison). At 0, voices pulse at independent natural rates.
- **Torus T(p,q)**: Configurable torus knot with p-meridian / q-longitudinal wraps. Encodes p:q delay length ratios, creating stable harmonic overtone locks alien to standard synthesis.
- **Denormal protection**: All feedback paths use `flushDenormal()` from `DSP/FastMath.h`.

### CPU Estimate

~20% single-core (6 voices × bell osc + 6-line FDN matrix multiply + Kuramoto + post FX). No oversampling required.

---

## 3. The 6 Doctrine Checks (Preliminary Assessment)

### D001 — Velocity Must Shape Timbre
**Status: PASS (DESIGNED)**

Velocity directly scales the inharmonic partial amplitudes in the bell oscillator:
```
brightness = 0.3 + 0.7 * velocity
output = sin(phase) + sin(phase*2.17)*0.3*brightness + ...
```
Velocity also scales the filter envelope depth (`olap_filterEnvAmt`), sweeping up to 4 octaves of filter cutoff on attack at high velocity. Soft strikes produce dark fundamentals; hard strikes produce bright, harmonically rich transients.

**Examiner note**: This is unusually well-designed D001 compliance — velocity shapes *both* spectral content (partials) and filter dynamics independently.

---

### D002 — Modulation is the Lifeblood
**Status: PASS (DESIGNED)**

- **LFO 1**: `olap_lfo1Rate` 0.01–20 Hz, 5 shapes (Sine/Triangle/Saw/Square/S&H), 6 destinations (Tangle, Dampening, Pulse Rate, Delay Base, Filter Cutoff, Spread)
- **LFO 2**: Same range and shapes, same 6 destinations
- **Mod Wheel (CC1)**: 4 destinations (Tangle Depth / Entrainment / Bioluminescence / Filter Cutoff)
- **Aftertouch**: 4 destinations (Tangle Depth / Entrainment / Brightness / Pulse Rate)
- **4 Macros**: KNOT (topology), PULSE (movement), ENTRAIN (coupling), BLOOM (density/space)

The LFO destination list is architecturally elegant: modulating Tangle Depth with an LFO makes the knot topology breathe in and out of entanglement over time — a uniquely topological form of movement.

**Examiner note**: One potential concern — verify adapter wires LFO destinations to DSP. The Osprey seance found an LFO struct "written but never called." Confirm `lfo1Phase` and `lfo2Phase` advance per block and apply to all 6 declared destinations.

---

### D003 — The Physics IS the Synthesis
**Status: PASS (APPLICABLE AND RIGOROUS)**

OVERLAP claims mathematical knot topology as its synthesis paradigm. This doctrine is applicable (unlike for pure VA synths). Assessment:

- **Knot matrices**: Derived from actual Gauss code crossing patterns, not arbitrary routing.
- **Energy preservation**: Column sums ≤ 1.0 at full tangle depth — mathematically correct for stability.
- **Amphichiral property**: Figure-Eight matrix explicitly stated as symmetric (M[i][j] = M[j][i]) — correct topological property.
- **Torus T(p,q)**: Delay ratios derived from meridian/longitudinal wrap formula — mathematically sound.
- **Kuramoto model**: Standard phase-coupling framework, cites the model by name. Used as documented in oscillator synchronization literature.

**Examiner note**: The fleet has never had a topology engine before. Ghost council may debate whether the matrix derivation is "mathematically honest" or whether simplifications (the trefoil uses a cyclic approximation, not the full knot group) compromise D003. This is a legitimate seance debate point.

---

### D004 — Dead Parameters Are Broken Promises
**Status: PASS (DESIGNED) — NEEDS VERIFICATION**

All 41 parameters are declared with corresponding DSP targets in the architecture spec. Key parameters to verify in adapter:

| Parameter | Declared Target | Risk |
|-----------|----------------|------|
| `olap_torusP` / `olap_torusQ` | Torus T(p,q) delay ratio computation | These only matter when `olap_knot = 3 (Torus)` — are they silently ignored for other knot types? |
| `olap_glide` | Portamento (mono/legato only) | Dead in poly mode by design — acceptable, but verify it works when `olap_voiceMode != 0` |
| `olap_currentRate` | Ocean current LFO rate | Is this wired separately from `olap_lfo1Rate`/`olap_lfo2Rate`? |

**Examiner note**: `olap_torusP` and `olap_torusQ` are contextually active (only meaningful when Torus knot selected) — this is acceptable, similar to how FM ratio params only affect FM voices. Not a D004 violation but worth clarifying in the seance.

---

### D005 — An Engine That Cannot Breathe Is a Photograph
**Status: PASS (DESIGNED)**

- Both LFOs: rate floor 0.01 Hz (explicitly documented as D005 compliance)
- Ocean Current Chorus: `olap_chorusRate` 0.01–0.5 Hz — an additional slow modulation source
- Bell-Pulse Envelope (`olap_pulseRate`): floor 0.01 Hz — voices can contract as slowly as once per 100 seconds

Three independent slow-modulation mechanisms. OVERLAP has exceptional D005 compliance on paper — the entrainment system means voice phases themselves drift slowly toward and away from lock, creating emergent breathing that doesn't require an explicit LFO at all.

**Examiner note**: Verify `lfo1Phase` and `lfo2Phase` advance per block in adapter code (not just in the standalone engine). Several past engines had LFO state not advancing in the XOceanus context.

---

### D006 — Expression Input Is Not Optional
**Status: PASS (DESIGNED)**

- **Velocity → timbre**: Bell oscillator brightness + filter envelope depth (strong, dual-path)
- **Mod Wheel (CC1)**: Routable to 4 destinations via `olap_modWheelDest`; depth via `olap_modWheelDepth`
- **Aftertouch**: Routable to 4 destinations via `olap_atPressureDest`; depth via `olap_atPressureDepth`

**Examiner note**: The spec declares mod wheel and aftertouch destinations. Verify the adapter reads `modWheelValue` and `aftertouchValue` and applies them per block. Past seances (ODDOSCAR, ODYSSEY) found expression routing declared but never consumed.

---

## 4. Potential Blessing Candidates

### Primary Candidate: **New Blessing — Knot-Topology FDN**

No fleet engine has a precedent for this. The FDN routing matrix derived from mathematical knot crossing patterns is a genuinely novel synthesis paradigm. If the ghost council unanimously recognizes this, OVERLAP would earn a new blessing (B016 or similar):

**Candidate Blessing**: *Knot-Topology Resonance* — the only synthesizer that derives harmonic structure from mathematical knot invariants. Switching knot type reorganizes the overtone lattice geometrically. Torus T(p,q) locks overtone ratios to p:q via delay length math.

### Secondary Candidate: **B001-adjacent — Entrainment Architecture**

The Kuramoto synchronization model as a synthesis voice-interaction mechanism is novel within the fleet. Not quite B001 (Group Envelope System) but analogous in spirit — multiple voices interacting through a mathematical coupling model. Could be recognized as a sub-blessing or component of the knot-topology blessing.

### Possible Connection to Existing Blessings

- **B009 (ERA Triangle)**: OVERLAP's KNOT macro sweeps through 4 discrete knot types with tangle depth — structurally similar to the 2D timbral crossfade concept of ERA, but in a 1D topology-morphing form. Not equivalent enough to claim B009, but the council may draw the comparison.

---

## 5. Open Questions for the Full Seance

1. **Knot matrix mathematical rigor**: The trefoil uses a cyclic approximation (not the full knot group / Alexander polynomial). Does this violate D003 in the eyes of the council, or is the approximation mathematically justified as an engineering encoding of the topological property?

2. **LFO wiring in adapter**: Are both `lfo1Phase` and `lfo2Phase` advancing per block, and are all 6 declared destinations actually applied? (Risk of Osprey-class silent failure.)

3. **Coupling integration**: Does `OUROBOROS → OVERLAP (AudioToFM)` produce audible warping? Is the `FilterToFilter` coupling type correctly consuming source filter output and routing it to OVERLAP's resonance chain?

4. **Bioluminescence DSP**: The onset detector triggers "bandpass noise burst at the knot crossing frequencies." Are these crossing frequencies computed correctly from the current knot matrix, or are they hardcoded to a fixed approximation?

5. **Torus T(p,q) preset coverage**: How many of the 150 presets demonstrate the Torus knot variants? The Torus is the mathematically richest mode (p:q harmonic locking) but also the most complex to design for. Preset depth here will determine how well the engine's signature feature is represented.

6. **Legato mode FDN continuity**: In legato mode, the spec says "FDN state preserved across notes." Does pitch change correctly retune the delay-line resonant frequencies without audible discontinuity? This is a subtle edge case.

7. **Preset quality depth**: 150 presets is strong. Are the 49 Atmosphere presets meaningfully differentiated, or do many share the same knot type / similar parameter ranges? The seance historically calls out thin preset design even when count targets are met.

8. **OVERLAP + OUROBOROS coupling preset**: The "crown jewel" pairing (bidirectional topology meets chaos) — does a preset exist demonstrating it?

---

## 6. Recommended Ghost Council Members

OVERLAP's synthesis paradigm spans physical modeling (FDN, knot topology), modular synthesis philosophy (routing matrices), and ambient/academic sound design. Recommended council:

| Ghost | Rationale |
|-------|-----------|
| **Don Buchla** | Topological thinking is deeply aligned with Buchla's non-linear, network-based approach. The FDN as signal graph has clear Buchla lineage. He will likely champion the knot paradigm — or challenge its mathematical rigor. |
| **Pauline Oliveros** | Deep listening and emergent slow process are core OVERLAP aesthetics. The entrainment system (voices slowly locking together over time) is fundamentally a deep listening experience. She will evaluate whether the engine creates genuine meditative depth. |
| **Brian Eno** | Ambient music pioneer. OVERLAP's glacial, slowly evolving textures are exactly his domain. He will evaluate whether the engine can produce genuinely useful ambient material versus intellectually interesting but sonically limited topology exercises. |
| **Jean-Claude Risset** | Computer music pioneer who worked extensively with FDN and resonant synthesis. He will evaluate the mathematical correctness of the knot matrices and whether the inharmonic partial ratios (2.17, 3.71, 5.43) are the right choices for bell-like character. |
| **Suzanne Ciani** | Buchla specialist, ambient synthesist. Will evaluate whether OVERLAP's topological control vocabulary is accessible to a performing musician or only to a theorist. |

**Optional addition**: A mathematician ghost (Gauss, Poincaré, or a fictional "Knot Theorist" persona) could be convened to specifically evaluate the topological claims. This would be a first for the seance format — precedent set by OUROBOROS where Lorenz and Rossler were acknowledged as implicit validators.

---

*Prep completed 2026-03-16. Ready for full ghost council seance.*
