# Skill: /preset-audit-checklist

**Invoke with:** `/preset-audit-checklist`
**Status:** LIVE
**Last Updated:** 2026-03-21 | **Version:** 2.0 | **Next Review:** After next Guru Bin retreat or seance round
**Changelog:** v2.0 — 13 friction points resolved from first fleet-wide audit (16,479 presets, 44 engines). Added schema migration health, Blessing-aware exceptions, empty param detection, mood consistency checks, DNA threshold calibration.
**Purpose:** Systematic preset quality audit — part universal, part engine-specific — designed so Sonnet can audit and adjust presets on demand. Codifies Guru Bin retreat wisdom, seance scoring criteria, and the 6 Doctrines into a repeatable path to 9.0+ preset libraries. The gap between 8.0 and 9.0 is preset curation, not DSP.

---

## When to Use

- Before shipping any preset batch (new or revised)
- After `/exo-meta`, `/preset-forge`, or `/guru-bin` creates presets
- When an engine's seance score is below 9.0 and presets are the bottleneck
- As a pre-release quality gate (replaces ad-hoc preset review)
- When the Guru designs during retreats (this is his formal audit framework)
- Periodically on existing libraries to catch drift

---

## Scope Selection

Choose your audit scope before starting:

| Scope | Command | What It Covers |
|-------|---------|---------------|
| Single preset | `/preset-audit-checklist path/to/preset.xometa` | Full 7-phase audit on one file |
| Single engine | `/preset-audit-checklist engine: Opal` | All presets for that engine + coverage analysis |
| Mood folder | `/preset-audit-checklist mood: Flux` | All presets in a mood + cross-engine consistency |
| Full fleet | `/preset-audit-checklist fleet` | Everything. Produces fleet-wide health report. |
| New presets only | `/preset-audit-checklist new` | Git-diffed new `.xometa` files only |

---

## Phase 1: Context Gathering (Read Before You Judge)

Before auditing a single preset, gather engine context. Skip this for fleet-wide scans (run Phase 2-6 per-preset, Phase 7 aggregates).

### 1A. Engine Intelligence

Read these files for the target engine:

| Source | Path | What You Learn |
|--------|------|---------------|
| Sound Design Guide | `Docs/xolokun_sound_design_guides.md` → engine section | Key parameters, macro mapping, sonic thesis |
| Seance Verdict | `Docs/seances/{engine}_seance_verdict.md` | Ghost feedback, current score, P0/P1 concerns |
| Seance Cross-Reference | `Docs/seances/seance_cross_reference.md` → engine row | Score, blessings, violations, bugs |
| Engine Source | `Source/Engines/{Engine}/{Engine}Engine.h` | Actual parameter IDs, DSP architecture |
| Existing Presets | `Presets/XOlokun/` (grep for engine name) | Current library size, mood coverage |
| Retreat Notes | `scripture/retreats/{engine}_retreat.md` (if exists) | Guru refinement history, parameter wisdom |

### 1B. Engine Profile Card

Fill this out (mentally or in notes) before auditing:

```
Engine: _______________
Current Seance Score: ___/10
Preset Count: ___
Parameter Prefix: ___
Blessings: ___
Known P0/P1 Issues: ___
Macro Mapping: M1=___ M2=___ M3=___ M4=___
Architecture Type: [subtractive | FM | granular | modal | wavetable | generative | physical | spectral | hybrid]
```

---

## Phase 2: Universal Schema & Doctrine Gate

**Every preset, every engine, no exceptions.** Fail here = fail the audit.

### 2A. Schema Validation (Automated)

```bash
python3 Tools/validate_presets.py {path_to_preset_or_folder}
```

If the tool isn't available, manually verify:

**REQUIRED fields (P0 — preset non-functional without these):**
- [ ] JSON parses without error
- [ ] `schema_version` = 1
- [ ] `name` present and non-empty
- [ ] `mood` is one of: Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged
- [ ] `engines` array non-empty, engine IDs are canonical (check CLAUDE.md Engine ID table)
- [ ] `parameters.{EngineID}` keys use correct prefix (check CLAUDE.md Parameter Prefix table)
- [ ] `dna` has all 6 dimensions: `brightness`, `warmth`, `movement`, `density`, `space`, `aggression`
- [ ] All DNA values in `[0.0, 1.0]`
- [ ] `author` = `"XO_OX Designs"` for factory presets
- [ ] `version` = `"1.0.0"` for factory presets
- [ ] `macroLabels` present with 4 entries, all UPPERCASE (see **Blessed Exceptions** below for Overbite B008)
- [ ] `coupling` is an object with a `pairs` array (empty `[]` is OK for non-Entangled). `null` and missing key both fail.

**RECOMMENDED fields (P2 — poor UX without these, but preset loads):**
- [ ] `description` — 1-3 sentence evocative description
- [ ] `tags` — at least 3 accurate tags
- [ ] `tempo` — numeric or `null`
- [ ] `couplingIntensity` — one of: None, Subtle, Moderate, Deep

**Flag level:** REQUIRED failures = P0 ERROR. RECOMMENDED missing = P2 WARNING.

### 2A+. Structural Integrity Checks (Added v2.0)

These checks catch issues the basic schema validation misses:

- [ ] **Empty parameter blocks**: Each engine listed in `parameters` must have ≥1 non-macro parameter key. A block containing only `macro_*` keys or 0 keys means the preset loads as a silent init patch. **Flag: P0.**
- [ ] **Mood field/folder consistency**: The `mood` value in JSON must match the parent folder name. A preset in `Presets/XOlokun/Atmosphere/` with `"mood": "Flux"` is miscategorized. **Flag: P1.**
- [ ] **No legacy DNA fields**: If `sonic_dna` or `sonicDNA` exists alongside `dna`, the preset has schema migration debt. Strip legacy fields, keep `dna`. **Flag: P1.**
- [ ] **Coupling state clarity**: Three states exist — distinguish them:
  - `coupling` key missing entirely → P1 (add `{"pairs": []}`)
  - `coupling: null` → P0 (potential runtime crash — replace with `{"pairs": []}`)
  - `coupling: {"pairs": []}` → PASS (explicit empty coupling)
- [ ] **Intra-engine param consistency**: All presets for the same engine should use the same parameter key names. If Osprey presets split between `osprey_brine` (current) and `osprey_brightness` (legacy), one generation silently fails. **Flag: P0.** Consult engine source to determine canonical params.
- [ ] **Parameter value domain consistency**: The same parameter should use the same value domain across all presets. If `obsidian_filterCutoff` is normalized [0,1] in some presets and absolute Hz (4200) in others, behavior is unpredictable. **Flag: P1.**
- [ ] **Non-standard mood folders**: Presets in folders outside the 8 standard moods (e.g., Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic) may be invisible to the preset browser if PresetManager only enumerates standard folders. **Flag: P1.** Move to standard moods or confirm PresetManager scans all subdirectories.

### Blessed Exceptions (Engine-Specific Schema Overrides)

Some engines have Blessings that override default schema rules. Check these before flagging:

| Engine | Blessing | Override |
|--------|----------|----------|
| Overbite | B008 (Five-Macro System) | `macroLabels` may have 5 entries: `BELLY/BITE/SCURRY/TRASH/PLAY DEAD`. Do NOT flag as schema error. |
| Optic | B005 (Zero-Audio Identity) | D001/D006 exemptions — Optic is a visual engine, not MIDI-driven. Do NOT flag missing velocity/expression. |
| OceanDeep | B031 (Darkness Filter Ceiling) | Brightness DNA ceiling at ~0.6 is architecturally correct. Do NOT flag as DNA gap. |
| Overtone | Spectral engine | Aggression ceiling at ~0.52 is intentional for spectral synthesis. Do NOT flag as DNA gap. |

### 2B. The 6 Doctrine Checks

| ID | Check | How to Verify | Pass Criteria |
|----|-------|--------------|---------------|
| D001 | Velocity shapes timbre | Examine velocity-sensitive parameters. Look for `velFilterAmt`, `velEnvAmt`, or similar ≥ 0.3 | Velocity 30 vs 100 must produce timbral change (brightness, harmonic content), not just volume |
| D002 | Modulation is alive | Check: 2+ LFO params non-zero, 4 macros mapped, mod wheel target present | LFO1 and LFO2 active. All 4 macros produce audible change 0→1. Mod wheel routed. |
| D003 | Physics rigor (if applicable) | Only for modal/physical engines. Check frequency ratios, decay rates, material models | Cited parameters match published data (Fletcher & Rossing, Kinsler & Frey, Chaigne) |
| D004 | No dead parameters | For 10 random params: check value ≠ default OR verify param is audibly wired | Every declared parameter must affect audio. Zero ghost params. |
| D005 | Engine breathes | Check LFO rate: at least one LFO with rate that can reach ≤ 0.01 Hz | Autonomous timbral evolution audible even with no MIDI input |
| D006 | Expression inputs assigned | Check velocity→timbre, aftertouch target, mod wheel target | Velocity + at least one CC (aftertouch/mod wheel/expression) assigned |

**Flag level:** P1 — any Doctrine failure blocks shipping. Fix the preset or escalate to engine-level fix.

---

## Phase 3: The 10-Point Sonic Gate (Guru-Informed)

This is the heart of the audit. Each point is scored PASS / MARGINAL / FAIL.

**A preset needs 8/10 PASS to ship. 10/10 PASS for Transcendental tier.**

### Gate 1: First-Keypress Magic

> *"The engine must sing before you understand it."* — Vangelis (Seance)

- Does the preset produce a compelling sound on first keypress with all macros at default?
- Would a producer want to keep playing after the first note?
- Is this sound unique to this engine (not achievable by another engine in the fleet)?

**FAIL indicators:** Generic sine/saw init sound. Requires macro tweaking to become interesting. Could be any synth.

### Gate 2: Macro Sweep Test

Test each macro from 0.0 → 1.0 while sustaining a note:

| Macro | Must Produce | FAIL If |
|-------|-------------|---------|
| M1 (CHARACTER) | Fundamental timbral identity change — brightness, waveform, material, personality | No audible change, or only subtle volume shift |
| M2 (MOVEMENT) | Perceptible increase in temporal evolution — LFO depth, modulation rate, animation | Static sound at both 0 and 1 |
| M3 (COUPLING) | In Entangled: coupling intensity change. In single-engine: self-modulation or nothing (acceptable) | Does something in single-engine that isn't coupling-related (mislabeled) |
| M4 (SPACE) | Reverb/delay depth increase — dry→wet spatial transition | No spatial change, or assigned to non-spatial parameter |

**FAIL indicators:** Any macro does nothing. Macro label doesn't match behavior. M1 and M2 do the same thing.

### Gate 3: Velocity Arc

Play the same note at velocity 20, 64, 100, 127:

- [ ] Timbral brightness increases with velocity (not just volume)
- [ ] The arc feels musical — each velocity step has distinct character
- [ ] Soft playing produces fundamentally rounder/darker sound (envelope depth, not just cutoff)

**FAIL indicators:** Only volume changes. Filter doesn't respond. Velocity 20 and 127 sound identical except louder.

**Guru trick (The Velocity Gate):** Map velocity to filter envelope *amount*, not just cutoff. Soft playing should produce a fundamentally different envelope shape.

### Gate 4: Expression Depth

- [ ] Aftertouch is assigned and produces audible, musical change
- [ ] Mod wheel reveals a hidden dimension (not just vibrato or volume)
- [ ] At least one expression input changes something the macros don't cover

**FAIL indicators:** Aftertouch unassigned. Mod wheel does nothing. Expression inputs duplicate macro behavior.

### Gate 5: Breathing & Evolution (D005 Deep Check)

Hold a chord for 30 seconds with no MIDI input:

- [ ] Sound evolves — filter movement, amplitude undulation, timbral drift
- [ ] Evolution rate feels organic (not mechanical stepping)
- [ ] After 30s, the sound is recognizably the same preset but has journeyed somewhere

**Guru trick (The Breathing Filter):** LFO at 0.067 Hz (ocean breathing rate, Sutra III-1) on filter cutoff at depth 0.05. Below conscious perception but creates feeling of aliveness.

**FAIL indicators:** Sound is completely static. LFO is audible but mechanical (S&H on everything). No drift.

### Gate 6: DNA Accuracy

Play the preset and score each DNA dimension by ear, then compare to `.xometa` values:

| Dimension | Listen For | Tolerance |
|-----------|-----------|-----------|
| brightness | High-frequency content, airiness, shimmer | ±0.15 of declared value |
| warmth | Saturation, roundness, analog feel | ±0.15 |
| movement | LFO activity, modulation density, temporal change | ±0.15 |
| density | Harmonic richness, layering, thickness | ±0.15 |
| space | Reverb size, stereo width, spatial depth | ±0.15 |
| aggression | Distortion, resonance bite, attack sharpness | ±0.15 |

**FAIL indicators:** All 6 values at 0.5 (placeholder). Any value off by >0.2. Brightness=0.9 but sound is dark. DNA contradicts mood assignment.

**Cross-validation rules (with MARGINAL bands):**

Thresholds have a ±0.1 MARGINAL band. Values outside MARGINAL = FAIL. Fleet audit showed 46% Atmosphere failure at strict thresholds — the MARGINAL band catches "worth reviewing" vs "definitely wrong."

| Mood | Dimension | PASS | MARGINAL | FAIL |
|------|-----------|------|----------|------|
| Atmosphere | aggression | < 0.4 | 0.4–0.5 | > 0.5 |
| Atmosphere | space | > 0.5 | 0.4–0.5 | < 0.4 |
| Foundation | density | > 0.4 | 0.3–0.4 | < 0.3 |
| Foundation | space | < 0.4 | 0.4–0.5 | > 0.5 |
| Submerged | brightness | < 0.4 | 0.4–0.5 | > 0.5 |
| Flux | movement | > 0.5 | 0.4–0.5 | < 0.4 |

**Note:** Some engines have Blessing-mandated DNA ceilings (see Blessed Exceptions in Phase 2A+). Check those before flagging DNA range issues.

### Gate 7: Naming Quality

- [ ] 2-3 words, ≤ 30 characters
- [ ] Evocative — references feeling, place, atmosphere, or mythology (not mechanism)
- [ ] No synth jargon ("FM pad", "wavetable sweep", "filter bass")
- [ ] No duplicates across entire `Presets/XOlokun/` tree
- [ ] Name matches the sound — play it, read the name, ask "does this fit?"

**Good:** "Twilight Entanglement", "Copper Kettle", "Abyssal Meditation", "Neon Tetra Chase"
**Bad:** "FM Pad 3", "Filter Sweep Bass", "Test Preset", "Warm Dark Slow"

**Debug/pole-fill preset detection (Added v2.0):**
Names matching these patterns are batch-generated diagnostic presets, not factory-quality:
- ALL-CAPS descriptor dumps: `BRIGHT_HOT_DENSE_VAST_AET3_4`, `5X DARK COLD KINETIC DENSE`
- Prefixed with `_ALL`: `_ALLHI_FND_01`, `_ALLLO_ATM_03`
- Numbered series with mood codes: `ICE FLUX 6`, `WARM DENSE STILL INTIMATE ATM 2`
- Single-word generic names: "Ascension", "Overcast", "Nocturne" (unless engine-specific)

These should either be renamed to evocative names or quarantined as non-factory presets. **Flag: P2.**

### Gate 8: Description & Tags

- [ ] Description is 1-3 sentences, evocative, no jargon
- [ ] Description doesn't just repeat the name
- [ ] Description tells a producer what this sound *does* (not how it's made)
- [ ] At least 3 tags, accurate to the sound
- [ ] Tags include at least one from: instrument type (pad, bass, lead, pluck, percussion, texture, drone) + one mood-adjacent descriptor

### Gate 9: Output Level & Headroom

- [ ] Peak output between -6dB and -1dB (leaves headroom for coupling + master FX)
- [ ] Amp envelope sustain + reverb tail doesn't clip
- [ ] No preset louder than -1dB peak

**Guru trick (The Tape Ceiling):** Saturation at 0.03-0.08 rolls off harsh transients without audible distortion. The sound of tape compression without the tape.

### Gate 10: Polyphony & CPU Appropriateness

- [ ] Polyphony matches the sound type:

| Sound Type | Expected Voices | Rationale |
|------------|----------------|-----------|
| Pads | 4-6 | Rarely use more than 4 simultaneously |
| Bass | 1-2 | Monophonic by nature |
| Leads | 1-2 | Single-line melodies |
| Plucks/Bells | 6-8 | Need natural resonance overlap |
| Percussion | 4-8 | Decay tail overlap |
| Drones | 2-4 | Sustained, less voice cycling |

**Guru trick (The CPU Free Lunch):** Reducing polyphony by 2 voices typically saves 10-15% CPU with zero audible impact on pads. Sacrifice voices, keep soul. (Scripture: Parsimonia Canon)

---

## Phase 4: Engine-Specific Depth Check

**This is what makes a 9.0 library, not just 9.0 presets.** Each engine architecture type has additional criteria that generic checks miss.

### Architecture Type: Subtractive / Analog-Modeling
*Engines: OddfeliX, OddOscar, Obese, Overbite, Oblique*

- [ ] Filter type variety: presets demonstrate LP, HP, BP, and notch (if available)
- [ ] Resonance used expressively (not always 0 or always max)
- [ ] Oscillator waveform variety: presets cover all available waveforms
- [ ] Unison/detune presets exist showing width progression
- [ ] Drive/saturation used intentionally (Guru trick: 0.03-0.08 for subtle warmth)

### Architecture Type: FM / Phase Modulation
*Engines: Ouie, Orbital*

- [ ] Algorithm variety: presets demonstrate multiple FM algorithms (not all the same)
- [ ] Modulation index range explored: from clean sine to metallic mayhem
- [ ] Velocity→index mapping present (harder hit = richer harmonics)
- [ ] Bell/chime presets demonstrate FM's unique territory (inharmonic partials)
- [ ] For OUIE specifically: STRIFE↔LOVE axis explored across presets

### Architecture Type: Granular / Spectral
*Engines: Opal, Overtone, Obscura*

- [ ] Grain size variety: from micro-granular glitch to macro-grain texture
- [ ] Source position/scan demonstrated across presets
- [ ] Freeze/sustain behavior shown in at least one preset
- [ ] Spectral characteristics unique to this engine (not generic granular)
- [ ] For OVERTONE: continued fraction convergents (π, e, φ, √2) each represented

### Architecture Type: Physical Modeling / Modal
*Engines: Ostinato, Oware, Obsidian, Osprey, Osteria*

- [ ] Material variety: presets cover all available materials/body types
- [ ] Excitation type variety: different strike/bow/pluck methods represented
- [ ] Decay curves physically plausible (wood decays faster than metal)
- [ ] Resonance behavior: sympathetic resonance demonstrated if available
- [ ] For OSTINATO: all 12 instruments represented, autonomous + hand-drum modes shown
- [ ] For OWARE: Material Continuum (wood↔metal↔bell↔bowl) traversed across presets

### Architecture Type: Wavetable / Morphing
*Engines: Odyssey, Orca, Oblong*

- [ ] Wavetable position explored across presets (not all at same position)
- [ ] Morphing/journey demonstrated (M1 should traverse the wavetable)
- [ ] Both static and animated wavetable presets exist
- [ ] Modulation of wavetable position by LFO/envelope shown

### Architecture Type: Generative / Algorithmic
*Engines: Organism, Ouroboros, Oracle, Orbweave*

- [ ] Determinism vs. randomness balance shown across presets
- [ ] Self-generating behavior demonstrated (D005 deep — sound evolves autonomously)
- [ ] Leash/constraint mechanism shown (chaos with control)
- [ ] Multiple rule sets or algorithm modes covered
- [ ] For ORBWEAVE: topological knot types (Trefoil/Figure-Eight/Torus/Solomon) each represented

### Architecture Type: Effects-Driven / Textural
*Engines: Overlap, Outwit, Oxbow, OceanDeep*

- [ ] Wet/dry spectrum explored across presets
- [ ] FX parameter extremes shown (not all moderate reverb)
- [ ] Spatial characteristics demonstrated (narrow→wide progression)
- [ ] Engine-specific FX identity clear (not generic reverb sounds)
- [ ] For OXBOW: Chiasmus chirality (L/R mirror decay) demonstrated
- [ ] For OCEANDEEP: all 3 waveguide bodies (Open Water/Cave/Wreck) represented

### Architecture Type: Chip / Retro
*Engines: Overworld*

- [ ] Era variety: presets span NES, Genesis, SNES eras
- [ ] Authentic limitations respected (limited polyphony, appropriate waveforms)
- [ ] Modern touches applied tastefully (reverb, filter movement)
- [ ] ERA triangle explored across presets

### Architecture Type: Visual / Non-Audio
*Engines: Optic*

- [ ] Visual responsiveness to audio input demonstrated
- [ ] Multiple visualization modes covered
- [ ] Coupling inputs from other engines shown
- [ ] (D001/D006 exemptions documented — Optic is intentionally non-MIDI)

---

## Phase 5: Coverage & Gap Analysis

Run this at engine or fleet scope. Single-preset audits skip this phase.

### 5A. Mood Distribution

Count presets per mood for the engine:

| Mood | Count | Target | Status |
|------|-------|--------|--------|
| Foundation | ___ | ≥ 3 | |
| Atmosphere | ___ | ≥ 3 | |
| Entangled | ___ | ≥ 2 | |
| Prism | ___ | ≥ 3 | |
| Flux | ___ | ≥ 2 | |
| Aether | ___ | ≥ 2 | |
| Family | ___ | (only if OHM/ORPHICA/OBBLIGATO/OTTONI/OLE) | |
| Submerged | ___ | ≥ 2 | |

**FAIL**: Any mood at 0 presets. **WARNING**: Any mood below target.

Historically thin moods: Flux, Aether, Submerged. Prioritize these.

### 5B. DNA Coverage Map

For each DNA dimension, check min and max values across the engine's presets:

```
brightness:  min=___ max=___  [gap if max < 0.7 or min > 0.3]
warmth:      min=___ max=___
movement:    min=___ max=___
density:     min=___ max=___
space:       min=___ max=___
aggression:  min=___ max=___
```

**FAIL**: Any dimension with range < 0.4 (e.g., all presets between 0.4 and 0.6 — no extremes).
**Target**: Each dimension should span at least 0.0-0.3 to 0.7-1.0 across the library.

**Blessing-aware exceptions:** Some engines have intentional DNA ceilings/floors that are features, not bugs:
- OceanDeep brightness ≤ 0.6 (B031 Darkness Filter Ceiling)
- Overtone aggression ≤ 0.52 (spectral engine — aggression ceiling is architectural)
- Check `Blessed Exceptions` table in Phase 2A+ before flagging DNA range issues.

### 5C. Duplicate Detection

```bash
python3 Tools/validate_presets.py --check-duplicates
```

Or manually: no two presets share the same name across the entire `Presets/XOlokun/` tree.

### 5D. Preset Count Health

| Count | Status | Action |
|-------|--------|--------|
| 0 | CRISIS | Engine cannot ship. Create minimum 8 (1 per mood). |
| 1-7 | CRITICAL | Below minimum. Backfill missing moods first. |
| 8-15 | MINIMAL | Functional but thin. Expand extreme DNA values. |
| 16-30 | HEALTHY | Good coverage. Focus on quality over quantity. |
| 31-50 | STRONG | Can begin Transcendental tier design. |
| 51+ | MATURE | Focus on curation — remove weakest, elevate best. |

### 5E. Entangled Preset Validation

For every Entangled mood preset:

- [ ] At least 2 engines loaded in `engines` array
- [ ] At least 1 coupling route with amount > 0.1
- [ ] Route uses a non-STUB CouplingType
- [ ] M3 (COUPLING) audibly changes coupling intensity
- [ ] Preset name indicates the coupling pair (e.g., "OXBOW+OPAL: Tidal Resonance")

### 5F. Fleet-Level Entangled Integrity (Engine/Fleet Scope Only)

At engine or fleet scope, compute the **Entangled Integrity Rate**:

```
Entangled Integrity = (Entangled presets WITH coupling pairs) / (Total Entangled presets) × 100%
```

| Rate | Status | Action |
|------|--------|--------|
| 90-100% | HEALTHY | Entangled mood is trustworthy |
| 50-89% | WARNING | Significant number of solo presets misfiled as Entangled |
| < 50% | CRITICAL | Entangled mood is structurally fraudulent — mass re-mooding required |

**Fleet baseline (2026-03-21):** 85-94% of Entangled presets fleet-wide had NO coupling pairs. This was the single largest structural issue discovered. When auditing, always report this metric prominently.

An Entangled preset with 1 engine and no coupling pairs should be re-mooded to Foundation, Atmosphere, or Prism based on its DNA profile. This is not a creative judgment — it's a categorization fix.

---

## Phase 6: The Guru's Refinement Arsenal

**Apply 2-3 of these per preset where relevant.** These are the techniques that separate 8.5 from 9.0+.

### Universal Tricks (Apply to Any Engine)

| Trick | Parameter Guidance | When to Apply | Scripture |
|-------|-------------------|--------------|-----------|
| **The Breathing Filter** | LFO at 0.067 Hz on filter cutoff, depth 0.05 | Pads, drones, atmospheres — any sustained sound | Sutra III-1 |
| **The Tape Ceiling** | Saturation/drive at 0.03-0.08 | Any preset with harsh transients | — |
| **The Drift Anchor** | Per-voice drift depth 0.15, rate 0.02 | Pads, textures — creates width without detune | — |
| **The Velocity Gate** | Velocity → envelope depth (not just cutoff) | Every preset (D001 done right) | — |
| **The Formant Whisper** | Resonance 0.28-0.32 on LP, cutoff 2200-2800 Hz | Vocal-adjacent sounds, talking basses | — |
| **The Ghost Harmonic** | Sub oscillator at -18 to -22dB, 0.3-0.7 cents detune | Bass, foundation sounds needing weight | — |
| **The Release Cathedral** | Release time 1.618 seconds (golden ratio) | Atmospheric, spatial sounds | Truth VI-1 |
| **The Coupling Whisper** | Coupling amount 0.08-0.15 | Entangled presets — subliminal > obvious | — |
| **The CPU Free Lunch** | Reduce polyphony by 2 voices | Pads, drones (save 10-15% CPU) | Parsimonia Canon |

### Application Decision Tree

```
Is this a sustained sound (pad/drone/atmosphere)?
  → Apply: Breathing Filter + Drift Anchor + Release Cathedral

Is this a bass or foundation sound?
  → Apply: Ghost Harmonic + Velocity Gate + Tape Ceiling

Is this an Entangled preset?
  → Apply: Coupling Whisper (amount 0.08-0.15, not 0.5)

Is this a lead or pluck?
  → Apply: Velocity Gate + Formant Whisper (if vocal quality desired)

Is the CPU budget tight?
  → Apply: CPU Free Lunch (reduce polyphony by 2)
```

---

## Phase 7: Report & Prioritization

### Report Template

```
══════════════════════════════════════════════════
  PRESET AUDIT REPORT
  Engine: {name} | Scope: {preset/engine/mood/fleet}
  Date: {date} | Auditor: {Claude/Guru Bin}
  Current Seance Score: {X.X}/10
  Presets Scanned: {N}
══════════════════════════════════════════════════

SCHEMA ERRORS ({N}):
  P0 ❌ {file} — {issue}

DOCTRINE VIOLATIONS ({N}):
  P1 ❌ {file} — {D00X}: {specific failure}

SONIC GATE RESULTS:
  Gate 1 (First-Keypress):    {N} PASS / {N} MARGINAL / {N} FAIL
  Gate 2 (Macro Sweep):       {N} PASS / {N} MARGINAL / {N} FAIL
  Gate 3 (Velocity Arc):      {N} PASS / {N} MARGINAL / {N} FAIL
  Gate 4 (Expression Depth):  {N} PASS / {N} MARGINAL / {N} FAIL
  Gate 5 (Breathing):         {N} PASS / {N} MARGINAL / {N} FAIL
  Gate 6 (DNA Accuracy):      {N} PASS / {N} MARGINAL / {N} FAIL
  Gate 7 (Naming):            {N} PASS / {N} MARGINAL / {N} FAIL
  Gate 8 (Description/Tags):  {N} PASS / {N} MARGINAL / {N} FAIL
  Gate 9 (Output Level):      {N} PASS / {N} MARGINAL / {N} FAIL
  Gate 10 (Polyphony/CPU):    {N} PASS / {N} MARGINAL / {N} FAIL

ENGINE-SPECIFIC FINDINGS ({N}):
  ⚠️  {finding}

COVERAGE GAPS:
  Missing moods: {list}
  DNA gaps: {dimension}: {range missing}
  Preset count: {N} ({status})

GURU TRICKS APPLIED:
  ✨ {preset} — applied {trick}: {what changed}

ESTIMATED SEANCE IMPACT:
  Current: {X.X}/10 → Projected after fixes: {X.X}/10

FIX PRIORITY:
  P0 (blocks shipping):  {list}
  P1 (doctrine failure): {list}
  P2 (sonic quality):    {list}
  P3 (nice to have):     {list}
══════════════════════════════════════════════════
```

### Priority Definitions

| Priority | Meaning | Fix Timing |
|----------|---------|-----------|
| P0 | Schema error — preset won't load | Immediate. Fix before any other work. |
| P1 | Doctrine violation — preset ships broken promises | Before next commit. May require engine-level fix. |
| P2 | Sonic quality — preset works but doesn't reach 9.0 standard | Before release. Use Guru tricks. |
| P3 | Polish — naming, tags, description, DNA fine-tuning | Batch with next preset pass. |

---

## Phase 8: Schema Migration Health (Fleet Scope Only)

The fleet contains 3 generations of preset schemas. This phase identifies migration debt.

### Schema Generations

| Generation | Characteristics | How to Identify |
|------------|----------------|-----------------|
| Gen 1 (early) | `sonic_dna` field, `coupling: null`, `version: "1.0"`, mixed-case macroLabels, no description/tempo/couplingIntensity | Has `sonic_dna` key |
| Gen 2 (mid) | `sonicDNA` field, partial fields, generic macro names (`MACRO1`/`M1`), some coupling | Has `sonicDNA` key |
| Gen 3 (current) | `dna` only, full fields, engine-specific macros, `coupling: {"pairs": []}`, descriptions | Has `dna` without legacy variants |

### Migration Health Metrics

For each engine, compute:

```
Gen 3 Rate = (Gen 3 presets) / (Total presets) × 100%
```

| Rate | Status | Action |
|------|--------|--------|
| 90-100% | HEALTHY | Schema is current |
| 50-89% | NEEDS MIGRATION | Run `preset_migration_sprint1.py` |
| < 50% | LEGACY HEAVY | Prioritize migration before quality work — fixes are cheaper at Gen 3 |

### Migration Script

```bash
# Dry run — see what would change
python3 Tools/preset_migration_sprint1.py

# Apply fixes
python3 Tools/preset_migration_sprint1.py --apply
```

The migration script handles: coupling null→{pairs:[]}, wrong param prefixes, legacy DNA field stripping, version string normalization, missing field addition. Run this BEFORE the audit — it eliminates hundreds of false positives.

---

## Sonnet Execution Guide

**When running this as an autonomous Sonnet task**, follow this exact workflow:

### Step 0: Run migration script first (fleet scope only)
```bash
python3 Tools/preset_migration_sprint1.py --apply
```
This eliminates hundreds of false positives from legacy schema debt. Skip for single-preset audits.

### Step 1: Read the engine's sound design guide section
```
Read: Docs/xolokun_sound_design_guides.md → {engine} section
Read: Source/Engines/{Engine}/{Engine}Engine.h → parameter list
```

### Step 2: Gather all presets
```
Glob: Presets/XOlokun/**/*{engine}*.xometa
Glob: Presets/XOlokun/**/*.xometa → grep for engine name in "engines" array
```

### Step 3: Run schema validation + structural integrity
```bash
python3 Tools/validate_presets.py Presets/XOlokun/
```
Then manually check Phase 2A+ items: empty param blocks, mood/folder consistency, legacy DNA fields, coupling state, intra-engine param consistency. These are the highest-value checks that the validator misses.

### Step 4: Check Blessed Exceptions
Before flagging any schema or DNA issue, check the Blessed Exceptions table in Phase 2A+. Overbite 5-macro, Optic D001/D006 exemptions, OceanDeep brightness ceiling, and Overtone aggression ceiling are all intentional.

### Step 5: For each preset, check Phase 2 (Doctrine) and Phase 3 (Sonic Gate)
- Read the `.xometa` file
- Check parameter values against the 10-Point Sonic Gate
- Use MARGINAL bands for DNA cross-validation (not strict thresholds)
- Flag issues with priority level

### Step 6: Run Phase 5 (Coverage) at engine scope
- Count presets per mood
- Compute DNA min/max per dimension (with Blessing-aware exceptions)
- Check for duplicates
- Compute **Entangled Integrity Rate** (Phase 5F)
- Flag non-standard mood folder presets

### Step 7: Apply Phase 6 (Guru Tricks) to any preset scoring MARGINAL
- Use the Decision Tree to select appropriate tricks
- Edit the `.xometa` file with refined values
- Document what was changed and why

### Step 8: Generate Phase 7 report + Schema Migration Health (Phase 8)

**Sonnet can do all of this autonomously.** The only step requiring human judgment is Gate 1 (First-Keypress Magic) — flag these for human play-testing and move on.

### Fleet Audit Tip: Batch by Engine Groups
For fleet-wide audits, dispatch parallel agents covering 7-8 engines each. Save per-batch reports to `Docs/fleet-audit/batch{N}-{engines}.md`, then aggregate into a master report at `Docs/fleet-audit/FLEET-BASELINE-{date}.md`. This approach audited 16,479 presets across 44 engines in ~8 minutes using 6 parallel Sonnet agents.

---

## Integration Points

| Trigger | What This Skill Does |
|---------|---------------------|
| After `/exo-meta` or `/preset-forge` | Run on new presets — full Phase 2-3, skip Phase 5 |
| After `/guru-bin` retreat | Run on retreat presets — full Phase 2-6 with Guru tricks |
| Before release | Run fleet-wide — full Phase 2-7 |
| After `/synth-seance` | Cross-reference seance feedback with audit findings |
| Before updating MEMORY.md counts | Fleet scan for accurate preset counts |
| As input to `/producers-guild` | Provide coverage data for product roadmap |

---

## Related Skills

| Need | Use |
|------|-----|
| Create new presets from scratch | `/preset-architect` |
| Automated schema + duplicate checks | `/preset-qa` |
| Existing preset quality gate (lighter) | `/preset-auditor` |
| DNA assignment | `/dna-designer` |
| Coupling preset design | `/coupling-preset-designer` |
| Engine doctrine compliance | `/engine-health-check` |
| Guru Bin refinement retreat | `/guru-bin` |
| Convert retreat tables to .xometa | `/preset-forge` |
| Fleet-wide health dashboard | `/fleet-inspector` |

---

## Relationship to Existing Preset Skills

This skill is the **comprehensive audit framework** that supersedes ad-hoc quality checks:

- `/preset-qa` → Automated schema validation (Phase 2A of this skill). Still useful standalone for quick checks.
- `/preset-auditor` → Lighter quality gate (Phase 2-3 partial). Use when you need a quick pass, not a full audit.
- `/preset-audit-checklist` (this skill) → Full 7-phase audit with Guru wisdom. Use when targeting 9.0+.

**When in doubt, use this skill.** It includes everything the other two check, plus the Guru-informed sonic depth that separates good from transcendent.
