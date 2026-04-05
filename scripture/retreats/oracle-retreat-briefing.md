# Oracle Retreat Briefing

```
══════════════════════════════════════════════════════
  RETREAT BRIEFING: ORACLE
  Prepared for Guru Bin — 2026-03-16
══════════════════════════════════════════════════════
```

---

## Engine Identity

- **Gallery code:** ORACLE
- **Accent:** Prophecy Indigo `#4B0082`
- **Mythology:** The ancient reef formation — geological time made audible. Layer upon layer of coral skeleton compressed into living stone that remembers every current that ever passed through it. Each waveform cycle is a geological epoch.
- **Aquatic placement:** Reef depth in the water column — warm enough for Oscar's patience, shallow enough for feliX's curiosity.
- **feliX-Oscar polarity:** Balanced — Oscar's geological patience in the stochastic drift; feliX's curiosity in the maqam exploration.
- **Synthesis type:** GENDY Stochastic Waveform Synthesis + Maqam Microtonal Tuning (Iannis Xenakis / CEMAMu lineage)
- **Polyphony:** Mono / Legato / Poly4 / Poly8 (8 max voices, LRU stealing with 5ms crossfade)
- **Parameter prefix:** `oracle_`

---

## Architecture Summary

### Synthesis Type
Pure stochastic waveform synthesis. No oscillator, no wavetable, no sample. 8–32 breakpoints define one waveform cycle; each cycle every breakpoint undergoes a random walk drawn from a morphable Cauchy/Logistic distribution blend. The waveform is never the same twice.

### Signal Chain
```
GENDY Breakpoints (random walk per cycle)
  -> Cubic Hermite (Catmull-Rom) Interpolation (per sample)
  -> 64-sample cycle-boundary crossfade (prevents clicks at seam)
  -> DC Blocker (5Hz first-order HPF — GENDY waveforms are inherently asymmetric)
  -> Soft Limiter (fastTanh saturation)
  -> Amp Envelope * Velocity * Voice crossfade gain
  -> Stereo Decorrelation (R channel reads at +1% phase offset — genuine width, no second oscillator)
  -> Master Level -> Output Cache (for coupling reads)
```

### DSP Complexity
- **Two independent envelopes per voice:** Amp ADSR (loudness) and Stochastic ADSR (controls how much breakpoints drift over time — the "intelligence" envelope, structurally identical to OCEANIC's Swarm ADSR)
- **Two LFOs per voice:** LFO1 modulates time step (breakpoint position drift rate); LFO2 modulates amplitude step (breakpoint height drift rate). Both default depth = 0 — completely unexplored in shipped presets.
- **Per-voice PRNG:** Xorshift64 seeded by note number — identical notes produce identical stochastic evolution across sessions. This is reproducible chaos.
- **Maqam tuning engine:** 8 canonical maqamat with quarter-tone (50-cent) intervals; `gravity` parameter blends continuously between 12-TET and full maqam intonation.
- **Denormal protection:** Applied at both DC blocker feedback path and post-envelope multiplication — clean CPU behavior during silence and release tails.

### Key Parameter Groups

| Group | Parameters |
|-------|-----------|
| GENDY Core | `oracle_breakpoints` (8–32), `oracle_timeStep`, `oracle_ampStep`, `oracle_distribution`, `oracle_barrierElasticity` |
| Maqam | `oracle_maqam` (9 choices: 12-TET + 8 maqamat), `oracle_gravity` (0–1 blend) |
| Evolution | `oracle_drift` |
| Amp Envelope | `oracle_ampAttack/Decay/Sustain/Release` |
| Stochastic Envelope | `oracle_stochEnvAttack/Decay/Sustain/Release` |
| LFO 1 | `oracle_lfo1Rate` (0.01–30 Hz), `oracle_lfo1Depth`, `oracle_lfo1Shape` (Sine/Tri/Saw/Square/S&H) |
| LFO 2 | `oracle_lfo2Rate`, `oracle_lfo2Depth`, `oracle_lfo2Shape` |
| Voice | `oracle_voiceMode`, `oracle_glide` (0–2s) |
| Macros | PROPHECY, EVOLUTION, GRAVITY, DRIFT |

### Hidden Capabilities Found in Source (Not Obvious from UI)

1. **The Stochastic Envelope is a second compositional dimension.** `oracle_stochEnvSustain=0` extinguishes breakpoint evolution completely during sustain while the amp envelope holds full amplitude. The note sounds frozen — stochastically still — then resumes evolving on release. This is the same structural insight as OCEANIC's Swarm ADSR and is completely absent from shipped presets.

2. **LFO1 and LFO2 are entirely unexplored.** Both ship at depth=0. LFO1 modulates time-step (how fast breakpoint positions drift); LFO2 modulates amplitude-step (how fast breakpoint heights drift). At 0.01 Hz — below conscious perception — they produce a slow breathing in the character of the waveform itself, not in pitch or volume. S&H shape on LFO1 at low rate creates sudden geological shifts.

3. **Breakpoint count (8–32) dramatically changes the engine's identity.** 8 breakpoints = angular, primitive, almost wavetable-like. 32 breakpoints = dense, fluid, almost bandlimited noise at high drift. The parameter defaults to 16. The extremes are untouched in presets.

4. **Barrier elasticity controls mirror reflection behavior.** At 0.0, breakpoints that hit the amplitude boundary are hard-reflected (billiard ball). At 1.0, the barrier is elastic — overshooting breakpoints are pulled back gradually. High elasticity = smoother evolution; low elasticity = more violent waveform discontinuities. No preset systematically explores this axis.

5. **Stereo decorrelation is free and inherent.** The R channel reads the same breakpoint set at a 1% phase offset — not a chorus, not a delay, but a genuinely different moment in the same waveform cycle. In Poly8 mode with 8 independent stochastic voices each slightly out of phase, the stereo field becomes a living reef.

6. **Maqam seeding via note number.** The PRNG seeds from note number, so each pitch has a deterministic stochastic DNA. Playing C4 always begins the same evolutionary path. Playing C#4 begins a different one. Melodic lines therefore have consistent timbral character per pitch — this is exploitable compositionally in maqam presets.

---

## Preset Inventory

### Total: 11 Oracle presets + 1 cross-engine preset featuring Oracle

| Mood | Count | Presets |
|------|-------|---------|
| Foundation | 1 | Oracle_Hijaz_Descent |
| Atmosphere | 5 | Oracle_Chromatic_Drift, Oracle_Maqam_Dusk, Oracle_Nahawand_Cloud, Oracle_Saba_Fog, Oracle_Sikah_Suspension |
| Entangled | 1 (+1 cross) | Oracle_Stochastic_Mesh; Origami_Oracle_Unfold (cross-engine) |
| Prism | 0 | — COVERAGE GAP — |
| Flux | 1 | Oracle_Stochastic_Flood |
| Aether | 2 | Oracle_Gravity_Veil, Oracle_Infinite_Barrier |
| Family | 0 | — COVERAGE GAP — |

### Coverage Gaps

- **Prism (0 presets):** No rhythmic, syncopated, or groove-forward Oracle presets exist. The engine's stochastic cycle evolution is inherently rhythmic — different pitches produce different cycle lengths, producing polyrhythmic interference patterns in Poly4/Poly8. This is Prism territory and is entirely absent.
- **Family (0 presets):** No ensemble-of-one or ORACLE-paired-with-sibling preset. Oracle's coupling outputs (stereo audio, envelope follower) are unused in family context.
- **Foundation (1 preset):** Only one playable, dry, no-effects Oracle preset. The reef needs more anchors.

### DNA Coverage Gaps
- No high-aggression Oracle presets (stochastic synthesis can produce genuinely brutal textures at breakpoints=8, timeStep=1.0, distribution=1.0/Cauchy). The engine is entirely positioned as ethereal/atmospheric in current library.
- No high-warmth Oracle presets. Nahawand maqam (closest to harmonic minor) + slow drift + low ampStep could anchor a warm foundation. Only explored in one preset.
- Ajam maqam (major scale) = 0 presets. Kurd maqam = 0 presets. 3 of 8 maqamat are completely unrepresented.

---

## Seance History

### B010 Blessing Summary — GENDY Stochastic Synthesis + Maqam

Oracle received **Blessing B010** — one of 15 fleet-wide Blessings. The seance quote is canonical: *"Buchla said 10 out of 10 — and there are zero presets to prove it."*

The council's full assessment: Oracle scored **8.6/10** — the second-highest score in the fleet (behind ORGANON's 8/8 PASS, ahead of all 32 other engines). The GENDY implementation was praised as historically faithful to Xenakis's original 1991 GENDY1-3 system, with the addition of the maqam tuning layer representing a genuine creative extension that Xenakis himself did not implement.

The critique embedded in the quote is the mandate for this retreat: the engine is exceptional; the preset library does not prove it.

### Engine Health at Seance
- **D001 (velocity → timbre):** Velocity scales amplitude envelope directly. Minimal timbre impact — velocity does not affect stochastic depth or distribution morph.
- **D002 (modulation is lifeblood):** 2 LFOs present but shipped at depth=0. Macros functional. Aftertouch and mod wheel wired (D006 compliant).
- **D003 (physics rigor):** Fully compliant. GENDY mathematics are correctly implemented per Xenakis's formulation. Maqam cent offsets are historically accurate.
- **D004 (no dead params):** All parameters wired to DSP. Fleet-wide resolution confirmed.
- **D005 (engines must breathe):** LFOs present with 0.01 Hz floor. Compliant.
- **D006 (expression required):** Aftertouch → drift (+0.15 max chaos). Mod wheel (CC1) → gravity (+0.4 maqam pull). Both implemented and scaling confirmed in source.

### Unresolved Concerns from Seance
- No P0 bugs. No critical DSP issues.
- The one standing tension: **velocity only affects amplitude, not stochastic depth or distribution.** D001 is technically met (velocity scales the envelope which does modulate timbre indirectly as breakpoints evolve differently at different amplitude levels), but a direct velocity→stochastic depth path would make Oracle far more expressive. This is a retreat discovery opportunity, not a bug.

---

## Applicable Scripture

### Directly Applicable Verses

**Canon V-1: The Ghost Parameter Trap** — Not applicable (no architecture changes; Oracle has no ghost parameters). But the spirit applies: audit every shipped preset's `oracle_lfo1Depth` and `oracle_lfo2Depth` — they are almost certainly at default (0.0). That is not a design choice; it is an absence of thought.

**Truth VI-3: The Default Trap** — *The most applicable verse for this retreat.* Oracle ships with `oracle_lfo1Depth=0.0`, `oracle_lfo2Depth=0.0`, `oracle_gravity=0.0` (in most presets), `oracle_stochEnvSustain=0.7` everywhere. These are non-choices. The retreat must state a position on every parameter.

**Truth VI-1: The Golden Ratio Release** — Apply to Amp Release and Stochastic Release defaults. 1.618 seconds for Aether and Foundation pad releases. The reef decay should feel resolved.

**Truth VI-2: The Mod Wheel Contract (D006)** — Confirmed: mod wheel increases maqam gravity. Every Oracle preset should be designed such that pulling the wheel from 0 to 1 audibly increases the pull toward the maqam. Presets with `oracle_gravity=0.0` and no maqam selected will not demonstrate this gesture. Design presets where the wheel's range is musically meaningful.

**Sutra III-1: The Breathing Rate** — 0.067 Hz is the tempo of the ocean. LFO1 at 0.067 Hz modulating time-step will make the waveform evolution breathe below conscious perception. LFO2 at 0.067 Hz on amplitude-step makes the harmonic density shift like tide. Start slow drift presets here.

**Truth VI-4: The Reference Preset** — Oracle needs one preset at parameter extremes: breakpoints=8, timeStep=1.0, ampStep=1.0, distribution=1.0 (full Cauchy), barrierElasticity=0.0. Not necessarily beautiful — a teaching instrument that makes every other Oracle preset feel more nuanced by contrast.

**Canon V-2: The Integration Layer Drift** — Not directly applicable (no prefix changes), but the seance quote is scripture: *"Buchla said 10 out of 10 — and there are zero presets to prove it."* This is the Retreat's mandate.

---

## Retreat Priorities (Recommended)

### Priority 1: Activate the Two Hidden Dimensions
**The Stochastic Envelope and the LFOs are Oracle's unexplored interior.** Every existing preset ships with LFO depths at 0 and stochastic envelope treated as a secondary ADSR clone. The retreat should produce at least 8 presets that make these dimensions audible:
- Stochastic envelope with very different settings from the amp envelope (stochEnvSustain=0 while ampSustain=0.8 = note holds at full volume but waveform freezes)
- LFO1 at 0.067 Hz (ocean breathing) modulating time-step
- LFO2 with S&H shape at low rate (sudden geological shifts in amplitude breakpoints)

### Priority 2: Fill the Maqam Library
**5 of 8 maqamat are absent or underrepresented.** The current preset library explores Hijaz (1), Saba (1), Nahawand (1), Sikah (1), and general "chromatic drift" (gravity=0). Missing: Rast (the foundation maqam — most common in Arabic music), Bayati (contemplative, devotional), Kurd (dark, Phrygian-like), Ajam (bright, major scale). Each maqam is a distinct emotional world. Guru Bin should produce at minimum one committed preset per maqam, treating each selection + gravity=0.8+ as a different instrument.

### Priority 3: Cover Prism and Foundation with Structural Presets
**Prism = 0 Oracle presets.** The stochastic cycle creates natural rhythmic interference in Poly8 mode when different voices have slightly different pitches — each MIDI note triggers a different breakpoint count cycle that repeats at a different rate. A Prism preset holds a cluster chord, sets Poly8, and lets the interference create polyrhythmic textures. Foundation needs 3–4 more anchors: dry, relatively stable (low drift), single maqam, high gravity — the reef at geological rest.

---

## What Guru Bin Will Find Fresh

These parameter interactions appear in no current preset and no scripture verse — they are unexplored territory:

1. **Breakpoint count as a timbral axis within a single preset.** Macro EVOLUTION currently adds chaos speed. If a preset is designed where EVOLUTION macro also sweeps `oracle_breakpoints` (via coupling or parallel macro assignment), the waveform changes fundamental character — from 8-point angular to 32-point fluid — under performance control. No shipped preset does this. This requires coupling design, not just preset writing, but the blueprint is already in the source.

2. **Legato mode + glide + maqam.** Legato mode retains the stochastic state of the current voice — the breakpoints continue their evolution rather than resetting to a sine shape on each note. Combined with glide (0.3–0.8s), the pitch slides while the waveform continues its geological drift. In Bayati or Saba maqam at gravity=0.85, this produces a genuinely microtonal glide with quarter-tone intervals as waypoints. This is the maqam oud simulation territory. Nothing in the preset library approaches it.

3. **Stochastic envelope decay = 0 (instant collapse).** If `oracle_stochEnvAttack=0`, `oracle_stochEnvDecay=0`, `oracle_stochEnvSustain=0`, `oracle_stochEnvRelease=0` — the stochastic envelope fires for exactly one cycle and collapses. The waveform evolves once on note-on and then freezes. Combined with slow amp release, you hear a waveform that changes shape once and then holds that shape for the duration of the note. Each note is a different frozen geological moment. Play a melody: each note has a unique, stable, unrepeatable character. This is Oracle as a strange wavetable instrument — but the wavetable generates itself once per note.

4. **Aftertouch + high base drift.** Aftertouch sensitivity is 0.15 (conservative). If a preset ships with `oracle_drift=0.85`, aftertouch pushes drift to 1.0 — full chaos. Starting at 0.85 rather than the default 0.3 means the performer's finger pressure navigates the space between almost-chaotic and fully-chaotic, rather than between stable and barely-moving. Design presets where aftertouch is the drama switch, not a subtle color.

5. **Distribution morph as a timbral switch.** `oracle_distribution=0.0` = Logistic distribution = breakpoints drift smoothly, steps follow a bell curve. `oracle_distribution=1.0` = Cauchy distribution = heavy-tailed, occasionally makes enormous jumps. The transition between these is a timbral shift from "organic" to "erratic." No preset systematically positions a sound at 0.0 or 1.0 — everything sits near 0.5. The extremes are different instruments.

---

```
══════════════════════════════════════════════════════
  READY FOR PHASE R2: SILENCE

  The reef is ancient. Buchla gave it 10 out of 10.
  Now prove it.
══════════════════════════════════════════════════════
```

---

*Briefing prepared by the Retreat Accelerator — 2026-03-16*
*Source files: `Source/Engines/Oracle/OracleEngine.h` | `Presets/XOceanus/*/Oracle_*.xometa` | `Docs/seance_cross_reference.md` | `scripture/the-scripture.md`*
