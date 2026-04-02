# Filter Envelope Audit — D001 Compliance
## XOceanus Fleet | 2026-03-14

Doctrine D001: "Velocity Must Shape Timbre."
A filter envelope wired to DSP with a non-zero default and velocity scaling is the primary
mechanism for D001 compliance. This document audits the target engines, records the 4 worst
gaps fixed in this round, and lists remaining gaps for future rounds.

---

## Full Audit Table

Engines audited: Snap, Orbital, Overworld, Owlfish, Oblique, Obsidian, Morph (OddOscar),
Dub (Overdub), Osteria, Osprey, Ocelot. Additional reference engines (Bite, Opal, Dub)
included for baseline comparison.

| Engine | Filter Env Exists? | Depth Param | Default | Velocity Scales It? | D001 Status | Notes |
|--------|-------------------|-------------|---------|---------------------|-------------|-------|
| **Snap (OddfeliX)** | **NO → FIXED** | `snap_filterEnvDepth` (added) | **0.3** | Yes — `envLevel × velocity × 8000Hz` | **FIXED** | Decay envelope × velocity sweeps BPF center |
| **Morph (OddOscar)** | **NO → FIXED** | `morph_filterEnvDepth` (added) | **0.25** | Yes — `envLevel × velocity × 6000Hz` | **FIXED** | Block-rate Moog ladder cutoff sweep |
| **Oblique** | **NO → FIXED** | `oblq_filterEnvDepth` (added) | **0.3** | Yes — `envLevel × velocity × 7000Hz` | **FIXED** | Block-rate SVF cutoff per-voice |
| **Dub (Overdub)** | YES (wired) | `dub_filterEnvAmt` | **0.0 → 0.25** | Yes — `envVal × velocity × 10000Hz` | **FIXED** | DSP was correct; only default was silent |
| Bite (Overbite) | YES (full ADSR) | `poss_filterEnvAmount` | 0.3 | Yes — direct multiply | COMPLIANT | Full separate filter ADSR, full D001 |
| Opal | YES (full ADSR) | `opal_filterEnvAmt` | 0.3 | Yes — mod matrix path | COMPLIANT | Full separate filter ADSR + mod matrix |
| Orbital | No filter env | N/A | N/A | No | GAP | Filter cutoff static at 20kHz default; group envelopes are spectral, not timbral velocity response |
| Obsidian | No filter env | N/A | N/A | No | GAP | Has PD depth envelope (timbral), but not filter-based; velocity → amplitude only |
| Overworld | No filter env | N/A | N/A | No | GAP | Single static `ow_filterCutoff`; velocity → amplitude only |
| Owlfish | No filter env | N/A | N/A | No | GAP | `owl_filterCutoff` is normalized 0-1, no envelope; has amp ADSR only |
| Osteria | No filter params | N/A | N/A | No | GAP | No filter cutoff parameter at all; sound shaped by Shore system |
| Osprey | No filter env | N/A | N/A | No | CONTEXTUAL | Ambient texture engine; `osprey_filterTilt` is static; velocity → amplitude only |
| Ocelot | No filter env | N/A | N/A | No | GAP | Has amp ADSR per stratum; no spectral/filter envelope |

---

## The 4 Fixed Engines

### Fix 1: Snap (OddfeliX) — `snap_filterEnvDepth`

**Before:** The BPF center frequency was computed once per block as a static value
(`effectiveBpfCenter`) from the base cutoff parameter, macro SURFACE, coupling multiplier,
aftertouch, and LFO wobble. Every voice at every velocity got the same filter opening.
Velocity only shaped amplitude, not timbre. D001 gap.

**After:** A new parameter `snap_filterEnvDepth` (default 0.3, range 0–1) is multiplied into
a per-voice boost: `envVelBoost = filterEnvDepth × voice.velocity × voice.envelopeLevel × 8000 Hz`.
This boost is added to `effectiveBpfCenter` before `setCoefficients()`. At default 0.3 and
full velocity (1.0), a fresh hit (envelopeLevel = 1.0) gets +2400 Hz of BPF center brightness;
as the decay envelope falls, so does the filter opening. Quiet hits (velocity 0.3) get only +720 Hz.
Filter update remains block-rate (no extra CPU cost from per-sample SVF recalculation).

**D001 compliance:** Velocity shapes timbre via filter brightness on every hit.

---

### Fix 2: Morph (OddOscar) — `morph_filterEnvDepth`

**Before:** The Moog ladder filter cutoff was updated only on `sampleIndex == 0` (first sample
per block) using the static parameter value plus coupling modulation and aftertouch. No envelope
modulation of the filter. Velocity only shaped amplitude output. D001 gap for the primary pad engine.

**After:** A new parameter `morph_filterEnvDepth` (default 0.25, range 0–1) adds a velocity×envelope
boost: `filterEnvDepth × voice.velocity × voice.envelopeLevel × 6000 Hz` is added to `modulatedCutoff`
inside the existing `if (sampleIndex == 0)` block. At default 0.25 and full velocity (1.0) at
attack peak, the Moog ladder opens an extra 1500 Hz. Softer pads get less filter opening. As the
bloom attack rises and the decay follows, the filter tracks the amplitude contour.

**D001 compliance:** Velocity and envelope contour now jointly shape Morph's timbral brightness.

---

### Fix 3: Oblique — `oblq_filterEnvDepth`

**Before:** The Cytomic SVF voice filter was set once per block from `filterCutoff + couplingFilterMod`
(the block-rate `modulatedCutoff`). Every active voice got an identical cutoff regardless of its
envelope position or velocity. Filter was fully static within a block. D001 gap.

**After:** A new parameter `oblq_filterEnvDepth` (default 0.3, range 0–1) enables per-voice
filter opening: `envVelBoost = filterEnvDepth × voice.velocity × voice.envelopeLevel × 7000 Hz`.
Each voice gets `voiceCutoff = clamp(baseCutoff + envVelBoost, 20, 20000)`. Newly triggered
voices (high envelopeLevel, full velocity) get a bright filter; aging/decaying voices get a
progressively darker tone. Multiple simultaneous notes each track their own envelope independently.

**D001 compliance:** Per-voice velocity × envelope drives spectral brightness independently.

---

### Fix 4: Dub (Overdub) — `dub_filterEnvAmt` default 0.0 → 0.25

**Before:** The `dub_filterEnvAmt` parameter existed and was fully wired to DSP. The computation
`envOffset = filterEnv × envVal × voice.velocity × 10000 Hz` was correct and already D001-compliant.
However, the default was 0.0, which meant the feature was completely silent on any new patch or
init preset. The parameter was a "broken promise" in practice.

**After:** Default raised from 0.0 to 0.25. No DSP changes required. At default 0.25 and full
velocity, the amp envelope sweeps the filter open by up to 2500 Hz on the attack transient, then
decays with the note. The `-1.0 to +1.0` bipolar range allows both positive (filter opens on attack)
and negative (filter closes on attack, opens on sustain/release) modulation.

**Note on preset compatibility:** Existing `.xometa` presets that explicitly store
`"dub_filterEnvAmt": 0.0` will continue to load at 0.0. Only truly new patches (no stored value)
will pick up the 0.25 default. This is safe.

**D001 compliance:** Filter envelope is now active by default on Dub.

---

## Remaining Gaps (Future Rounds)

### Priority 1 — High expressiveness value, engine architecture supports it

| Engine | Recommended Fix | Effort |
|--------|----------------|--------|
| **Orbital** | Add `orb_filterEnvDepth` to apply per-voice cutoff sweep using group envelope level. The existing `externalFilterMod` path shows the plumbing exists. | Medium |
| **Obsidian** | The PD depth envelope already tracks velocity; add `obsidian_filterEnvDepth` to wire the existing `pPdEnvAttack/Decay/Sustain/Release` envelope level into the filter cutoff offset. No new envelope needed, just a depth scalar. | Low |
| **Owlfish** | Add `owl_filterEnvDepth` parameter. Wire amp envelope output (already computed for output scaling) into the normalized cutoff parameter. Requires understanding the 0-1 normalized cutoff mapping. | Low |

### Priority 2 — Architecturally unusual, needs design review

| Engine | Notes |
|--------|-------|
| **Overworld** | ERA-crossfade chip engine; the "filter" is actually a physical SVF on the mixed output. A filter env depth param would make sense but needs to be wired into the chip-layer mix before the SVF. Medium complexity. |
| **Ocelot** | Multi-strata ecosystem engine; filter is per-strata via `canopySpectralFilter`. Adding velocity response to canopy filter or overall strata amplitude-to-filter would require per-stratum envelope tracking. |
| **Osteria** | No filter parameter at all beyond smoke/warmth (LPF tilt). Adding a filter envelope requires first adding a primary `osteria_filterCutoff` parameter and SVF into the signal chain. Larger scope change. |
| **Osprey** | Ambient texture engine; velocity response is contextually appropriate to leave low-priority. The resonator brightness parameter already scales somewhat with voice character. |

### Priority 3 — Engines not in audit scope (for completeness)

Engines with verified D001 compliance (Bite, Opal, Oracle, Organon, Oceanic, Ouroboros) are
not listed above. Engines not audited in this round (Obscura, Origami, Optic, Ostinato, OpenSky,
OceanDeep, Ouie, Onset) should be included in a future filter envelope audit pass.

---

## Summary

| Round | Engines Fixed | Parameters Added | Defaults Changed |
|-------|--------------|-----------------|-----------------|
| This round | 4 | 3 new (`snap_filterEnvDepth`, `morph_filterEnvDepth`, `oblq_filterEnvDepth`) | 1 (`dub_filterEnvAmt` 0.0→0.25) |
| Remaining gaps | 7 high-priority | Pending | Pending |

The 3 new parameters use a consistent pattern: `depth × voice.velocity × voice.envelopeLevel × kMaxSweepHz`.
This ensures velocity and envelope contour jointly determine the filter opening at all times,
fully satisfying D001 across Snap, Morph, Oblique, and Dub.
