<!-- rnd: warmth-density-balance -->

# XPN Warmth × Density Balance — R&D Spec

**Date**: 2026-03-16
**Context**: Fleet DNA diversity audit revealed two co-correlated dimensions compressed into midrange, suppressing overall diversity score.

---

## 1. Current State

The fleet DNA diversity report returned a composite score of **0.294** — well below the target threshold of 0.35+. Two dimensions account for the largest share of the compression:

- **Warmth**: 69.7% of presets land in the 0.3–0.7 midrange band
- **Density**: 68.4% of presets land in the 0.3–0.7 midrange band

Because warmth and density are co-correlated (warm sounds tend toward richer, denser harmonic stacks; cold sounds trend sparse and clinical), midrange clustering in one dimension reinforces clustering in the other. The two must be addressed together.

**Engines driving midrange warmth clustering**: OHM, OBBLIGATO, OLE, OTTONI, OBLONG, OVERBITE — all character instruments with strong fundamental presence and organic timbres that naturally settle around 0.4–0.6 warmth. OPAL and OVERLAP contribute through their default reverb wash.

**feliX–Oscar axis**: Oscar-character engines (low-numbered, bass-forward, organic) produce high warmth. feliX-character engines (high-registered, digital, spectral) produce low warmth. Most of the fleet sits between these poles rather than at them — the extremes are underrepresented.

---

## 2. Warmth Extremes Strategy

### Ultra-Warm (0.8–1.0)
Target timbres: tape saturation, analog grit, bowed strings, wooden resonators, breath noise, close-mic'd acoustics.

**Engines that should own this space**: OVERDUB (tape delay is core identity), OVERBITE (bass-forward analog character), OBLONG (Apple Liquid Glass warmth, woody body), ONSET (kick/tom physical mass).

**Naming vocabulary**: Amber, Hearth, Embers, Molasses, Humid, Barrel, Tallow, Woolen, Char, Sunken, Beeswax, Flannel, Kiln, Dusk Settle, Mahogany, Umber.

### Ultra-Cold (0.0–0.2)
Target timbres: additive/spectral synthesis, hard-clipped digital oscillators, clinical sine stacks, granular frozen transients, ring modulation.

**Engines that should own this space**: ODYSSEY (wavetable/spectral), OVERWORLD (chip synthesis — NES/YM2612 binary waveforms), OUTWIT (Wolfram CA cellular automata), OVERTONE (continued fractions spectral).

**Naming vocabulary**: Arctic, Clinical, Steel, Void, Stark, Sterile, Lattice, Tungsten, Glass Spine, Fluorescent, Zero-K, Inert, Slab, Freeze Dried, Cathode, Null Field.

---

## 3. Density Extremes Strategy

### Ultra-Dense (0.8–1.0)
Target timbres: full unison stacks, saturated chords, granular clouds, spectral smear, layered percussion, thick pads with complex sidebands.

**Best engines**: OPAL (granular layering is native), OVERLAP (FDN reverb builds density organically), OBBLIGATO (dual-wind unison), OTTONI (triple brass stack), OHM (rich drone unison).

**Naming vocabulary**: Bramble, Sediment, Crush, Confluence, Thicket, Slurry, Resin, Dense Pack, Magma Floor, Swell Mass, Cordage, Tidal Press, Overgrown, Canopy.

### Ultra-Sparse (0.0–0.2)
Target timbres: single oscillator with no unison, minimal release, dry signal path, staccato attacks, skeletal melodic lines.

**Best engines**: ODYSSEY (single-voice mode), OVERWORLD (1-voice NES pulse), ORGANISM (cellular automata single-cell seed), OUIE (duophonic at minimum).

**Naming vocabulary**: Filament, Hairline, Solitary, One Wire, Bare Nerve, Skeleton Key, Signal Alone, Pluck Dry, Single Spoke, Ghost Note, Vacant Lot, Threadbare, Minus One.

---

## 4. Warmth × Density Quadrants

These four extreme-corner quadrant types would most directly pull the fleet distribution away from midrange clustering.

| Quadrant | Warmth | Density | Character |
|----------|--------|---------|-----------|
| **Hot Dense** | 0.8–1.0 | 0.8–1.0 | Thick analog bass, saturated pad, driven organ |
| **Hot Sparse** | 0.8–1.0 | 0.0–0.2 | Solo cello, intimate acoustic, close-mic pluck |
| **Cold Dense** | 0.0–0.2 | 0.8–1.0 | Polyphonic digital cluster, frozen spectral organ |
| **Cold Sparse** | 0.0–0.2 | 0.0–0.2 | Single crystal sine, clinical pluck, one-note chip |

---

## 5. Per-Engine Assignments

**Hot Dense** — OVERDUB, OVERBITE, OHM
- OVERDUB: tape drive cranked, long reverb tail, dense harmonic smear
- OVERBITE: bass stack with filter resonance, saturated fundamental
- OHM: full drone unison, slow LFO modulation depth

**Hot Sparse** — OBLONG, ONSET (solo voice), OBBLIGATO (single wind)
- OBLONG: single-voice mode, slow attack, minimal release, no FX
- ONSET: snare brush or single tom, low reverb, intimate room
- OBBLIGATO: solo flute voice, breath noise prominent, no unison

**Cold Dense** — OPAL, OVERLAP, ODYSSEY
- OPAL: dense granular cloud, freeze mode, high grain count
- OVERLAP: FDN at maximum diffusion, cold tonic cluster input
- ODYSSEY: polyphonic wavetable stack, spectral register, full voices

**Cold Sparse** — OVERWORLD, OUTWIT, ORGANISM
- OVERWORLD: 1-voice NES pulse, no vibrato, 8-bit duty 50%
- OUTWIT: single CA arm, minimal rule activation, dry output
- ORGANISM: seed state = single live cell, no reproduction cascade

---

## 6. Batch Generation Spec

Current diversity score: **0.294**. Target: **0.35+** (delta: +0.056).

Modeling assumption: each extreme-corner preset carries ~3–4× the diversity weight of a midrange preset because it pulls the mean of its dimension toward the edge. To shift the fleet average by +0.056 across two correlated dimensions requires approximately **80–120 new extreme-corner presets** distributed across the four quadrants.

**Recommended allocation** (total: 100 presets):

| Quadrant | Count | Rationale |
|----------|-------|-----------|
| Hot Dense | 28 | Largest gap; bass/pad users expect this |
| Cold Sparse | 28 | Largest gap; digital/experimental users |
| Hot Sparse | 22 | Acoustic solo voice — underrepresented |
| Cold Dense | 22 | Frozen cluster — highest novelty signal |

**Distribution across engines**: 10–12 presets per assigned engine per quadrant. Enforce that each preset is tagged with explicit Sonic DNA warmth and density values at generation time (not inferred post-hoc) so the diversity score recalculation reflects true placement.

**Validation gate**: After batch generation, rerun fleet DNA diversity report. If score does not reach 0.35, identify which quadrant still clusters toward center and generate an additional 20-preset correction batch for that quadrant only. Repeat until threshold is met.

---

*See also*: `xpn_sonic_dna_automation_rnd.md`, `xpn_sonic_identity_system_rnd.md`, `engine_sound_design_philosophy_rnd.md`
