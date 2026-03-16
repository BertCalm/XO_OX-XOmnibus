# XOmnibus — Sound Design Best Practices for XPN/MPC Export

**Version:** 1.0
**Author:** Vibe (XO_OX Sound Design)
**Date:** 2026-03-16
**Status:** Living Document
**Depends on:** `xpn_sound_shape_rendering.md`, `xpn_render_spec.py`, `xo_mega_tool_xpn_export.md`

---

## Introduction

XOmnibus presets are live synthesis objects. XPN exports are frozen audio artifacts. The gap between those two things is where sound design decisions either succeed or fail.

This guide covers the thinking, the numbers, and the judgment calls that determine whether an MPC producer gets a sample that sounds like the actual engine — or a pale, truncated, lifeless echo of it.

Read the constraint sections first. The constraint is the canvas.

---

## Part 1: Pre-Export Sound Design Principles

### 1.1 The XPN Constraint Mindset

XPN captures a static audio render. Modulation is baked. LFOs are baked. Coupling is baked. Envelope trajectories are baked. The MPC producer cannot reach inside and adjust the filter — they get what was rendered.

This is not a limitation to work around. It is the design.

**The XPN constraint forces a question every sound designer must answer before rendering:**

> "What is the best single moment of this patch? What is the sound the producer needs from this engine?"

The answer is almost never "the init state at MIDI velocity 64."

**Three failure modes to avoid:**

1. **Rendering the average state** — init patch, all macros at 50%, LFOs at neutral. Produces a flat, unremarkable sample that conveys nothing of the engine's character.
2. **Rendering the safe state** — pulling back drive, reducing space, trimming reverb tail to save seconds. Produces technically clean audio that sounds nothing like the preset played live.
3. **Rendering the extreme state** — macros maxed, everything at 11. Produces an unusable sample because the interesting sonic range was never captured.

**The goal: render the characteristic moment.** The moment that answers the question: "What does this engine DO?"

---

### 1.2 Bake the Interesting Moment, Not the Average State

For each engine type, "the interesting moment" is different:

| Engine Type | The Interesting Moment |
|-------------|----------------------|
| Percussive (ONSET, OBESE plucks) | The attack transient — the snap, the crack, the slam |
| Evolving/metabolic (ORGANON) | The bloom — the first 2-4 seconds of timbral emergence |
| Physical modeling (OBSCURA, OWLFISH) | The initial excitation + early resonance build |
| Granular (OPAL) | The textural density peak — 3-5 seconds in |
| Harmonic (ODYSSEY, ORBITAL) | The filter sweep mid-travel, not the neutral position |
| Brass/wind (OTTONI, OBBLIGATO) | The attack plus the first 300ms of body resonance |
| Stochastic (ORACLE) | The settled state after the GENDY walk has developed |

**Practical method:** Load the preset. Play it live for 30 seconds. Note the moment your ear is most engaged. Render that moment.

---

### 1.3 Design for the Hit

The XPM sample is what a producer will build a track around. It will be triggered hundreds of times. It will sit in a mix. It will be pitched, chopped, layered.

"Design for the hit" means rendering at the parameter state where the sample will work best in that context, not the state where it shows off the most synthesis complexity.

**Practical guidelines:**

- **Kick drums:** Render with full drive and saturation engaged. Producers expect to drive kicks hard. Normalize to -1 dBFS.
- **Melodic pads:** Render with modulation running — a static pad is a dull pad. Capture the movement.
- **Bass patches:** Render the fundamental tone with filtering open enough to sit in a mix. The "preset maximum" filter sweep is usable only in isolation.
- **Leads:** Render with the attack characteristic clearly preserved. Do not soften attacks in service of normalization headroom.
- **Textures:** Render at the moment the texture is most useful — not the most impressive. Useful textures have dynamic range. Impressive textures clip the mix.

---

### 1.4 Velocity Layers: When to Render Multiple vs. Single

**Rule of thumb:** A velocity layer is warranted when the timbral character changes meaningfully between soft and hard playing. If only the amplitude changes, one layer is enough.

| Condition | Layer Count | Rationale |
|-----------|-------------|-----------|
| Filter opens with velocity | 3 layers (pp/mf/ff) | Timbral character shifts are perceptible |
| Noise burst adds with velocity | 3 layers | Attack character changes at velocity extremes |
| Saturation clips with hard velocity | 2 layers (soft/hard) | Distortion is not merely louder — it is different |
| Only amplitude changes | 1 layer | MPC's velocity sensitivity handles level scaling |
| Pure texture / ambient | 1 layer | Velocity response is irrelevant for atmospheric content |
| Evolving sounds (ORGANON, ORACLE) | 1 layer + 2 round-robins | Variation in time matters more than velocity |
| Drum voices | 4 layers (pp=20/mp=50/mf=80/ff=120) | Drum programming demands expressive velocity response |

The XPN pipeline uses velocity split points: **v1 (1–49), v2 (50–99), v3 (100–127)** for 3-layer patches. Render at MIDI velocities **40, 85, 120** respectively.

For 4-layer drum voices: **v1 (1–39), v2 (40–79), v3 (80–109), v4 (110–127)**. Render at **20, 50, 80, 120**.

**The velocity transition must be smooth.** Audition each layer transition at the crossover point. A sudden timbre jump between layers is a defect, not an artistic choice — unless the transition IS the point (e.g., a brushed → full strike split).

---

### 1.5 Mono vs. Stereo Render Decisions

| Voice Type | Render Format | Notes |
|------------|---------------|-------|
| Kick / sub bass | Mono | Sub energy below 80Hz is phase-incoherent in stereo; mono locks it down |
| Snare (centered) | Mono | Centered transients are cleaner in mono |
| Hi-hats / cymbals | Stereo | Stereo placement is part of the character |
| Melodic leads | Mono | Pitch-shifted playback is more stable in mono |
| Pads / evolving | Stereo | Width is load-bearing for atmospheric content |
| Textures | Stereo | Full-width renders; perform mono-sum test before export |
| Bass patches | Mono | Sub below 80Hz must be mono; mid-bass may be stereo |
| Brass/winds | Stereo | Ensemble spread is part of the identity |

**Mono sum test for stereo renders:** Sum the stereo render to mono and confirm it does not lose more than 3 dB of perceived energy and does not produce comb filtering. If it does, the stereo image has phase issues that need to be resolved before export.

The XPN keygroup exporter produces stereo by default. Mono renders are specified in `.xometa` via `"renderMono": true`.

---

### 1.6 feliX-Oscar Export Considerations

Every XOmnibus preset sits on the feliX-Oscar polarity axis. That polarity has real acoustic consequences at render time.

**feliX patches (bright, clinical, digital: ODDFELIX, OVERWORLD, OPTIC, ORACLE, OBLIQUE)**

- Aliasing risk at high velocities, especially with hard sync and wavetable engines. Render at 48kHz minimum.
- OVERWORLD chip-register patches: NES square waves at high pitches approach Nyquist quickly. Use `--samplerate 48000` for renders above C5.
- Air frequencies (8-16kHz) are load-bearing for feliX character. Do not apply high-frequency rolloff during render. Normalize to -3 dBFS (not -6 dBFS) to preserve headroom while keeping air present.
- Anti-aliasing note: XOmnibus engines use minBLEP/PolyBLEP at the DSP level. Do not add additional anti-aliasing at the render stage — it will soften the clinical feliX character.

**Oscar patches (warm, organic, deep: OBLONG, OBESE, OVERDUB, OWLFISH, OBSCURA, OCEAN-DEEP)**

- Sub-bass content extends below 40Hz in some OBESE and OWLFISH patches. Render length must exceed the sub decay — short renders will truncate the low-frequency tail. Use render lengths 50% longer than the default for bass-dominant patches.
- High warmth patches often have mud in the 200-400Hz range. Before rendering, check the preset at C2-C3 for boxiness. A gentle 4 dB notch at 300Hz may be appropriate if the low-mids mask fundamental pitch in the sample.
- Tube saturation and ladder filter patches (OBESE: `fat_satDrive`, `fat_fltMode`) produce harmonic content that evolves with note duration. Hold time of 3+ seconds is required to capture the full saturation character.
- Guru Bin render rule (from `xpn_render_spec.py`): **Render the saturation, not around it. The driven ladder filter is OBESE's signature character. Never back off drive to normalize output level. Back off gain/volume instead. The distortion IS the sound.**

**Hybrid patches (Entangled mood, coupling presets)**

- Minimum 2 velocity layers: one capturing the cooler/digital character, one the warm/saturated response.
- The Entangled mood coupling state is the preset's identity. Do not render the decoupled state as the "clean" version — it is not the patch. Render the coupled state as the primary layer.
- If offering a dry reference (for educational comparison), mark it explicitly in the XPN metadata as a dry reference, not as an alternate velocity layer.

---

### 1.7 6D Sonic DNA Export Mapping

The `.xometa` DNA values are not just metadata — they predict specific audio characteristics that require different render decisions.

#### Brightness (0.0 – 1.0)

**High brightness (> 0.7):**
- Air frequencies are load-bearing. Normalize to -3 dBFS to preserve headroom while keeping high-frequency energy present.
- Check renders above C5 for digital harshness. A 6 dB/octave shelf above 12kHz is acceptable if the harshness is DSP artifact rather than intended character.
- Velocity layer 3 (ff, MIDI 120) is where brightness-heavy patches are most likely to clip on transients. Monitor peak output separately from velocity layers 1 and 2.

**Low brightness (< 0.3):**
- Warm, muted character. Normalizing to -6 dBFS is safe without losing character.
- Check that fundamental frequencies are present — low brightness does not mean low fundamentals. Dark pads can still need a clear root pitch.

#### Warmth (0.0 – 1.0)

**High warmth (> 0.7):**
- Check for mud in the 200-400Hz region. Play C2-C3 and listen for boxiness. This range builds up quickly in warm analog emulations (OBLONG, OBESE, OVERDUB).
- High-pass at 60Hz is appropriate for warmth-heavy patches before export. Sub content below 60Hz is rarely intentional in melodic warmth patches, and it adds size without adding character.
- Exception: OBESE bass patches with `fat_subLevel` > 0.5 should not be high-passed. The sub IS the patch.

**Low warmth (< 0.3):**
- Clinical character. Verify transient integrity — clinical patches sound wrong if the attack is rounded by normalization or limiting.
- No high-pass filter for clinical patches. Sterile character requires full-bandwidth capture.

#### Movement (0.0 – 1.0)

**High movement (> 0.7):**
- Minimum render length: 2-4 seconds to capture at least one modulation cycle.
- For LFO-modulated patches: check the preset's LFO rate parameter. Render length must exceed `1 / LFO_rate_Hz` × 2 cycles. A 0.1 Hz LFO needs 20+ seconds of render to capture meaningful modulation.
- For ORGANON metabolic presets: use `organon_render_length()` formula from `xpn_sound_shape_rendering.md`: `max(shape_default, 3.0 / metabolicRate + 2.0)`.
- Do not render at the "average" modulation state. Render at the moment the modulation is at its most expressive peak.

**Low movement (< 0.3):**
- Static patches. A single velocity layer with 2 round-robins (if the engine supports it) is enough.
- Static patches expose normalization errors more clearly — any level inconsistency between notes is audible.

#### Density (0.0 – 1.0)

**High density (> 0.7):**
- Dense patches (multiple oscillators, formant stacks, granular clouds) can benefit from rendering dry and wet variants separately (a stem approach).
- Primary export: the full dense texture as heard.
- Optional extended export for dense ensemble patches: a "stripped" variant with reverb/delay bypassed. The stripped variant is useful for producers who want to add their own spatial treatment.
- File size impact: dense textures at full render length are the largest samples in the library. Budget accordingly.

**Low density (< 0.3):**
- Sparse patches expose every noise artifact. Silence trimming must be precise.
- Round-robin variation is more audible for sparse patches — identical repetitions are more fatiguing when there is nothing else in the texture to mask them.

#### Space (0.0 – 1.0)

**High space (> 0.7):**
- Capture the decay tail fully. Do not truncate reverb. The `xpn_render_spec.py` space check: "render with 2-3s natural tail, let the user add MPC reverb for more."
- For OVERLAP patches (FDN reverb identity engine): render length must include the full reverb tail. OVERLAP's reverb tail can extend 6-8+ seconds. Use the `wet_large` variant strategy.
- Normalize spacious patches to -6 dBFS. MPC reverb adds significant level; headroom is needed.
- Room tone consistency: all velocity layers and all notes in a spacious patch should have the same reverb decay profile. Inconsistent room tone across velocity layers is a defect.

**Low space (< 0.3):**
- Dry patches. Render with no reverb. See `xpn_render_spec.py` space note: "render with no reverb."
- Dry patches require the most precise silence trimming — any pre-roll or post-roll is immediately audible.

#### Aggression (0.0 – 1.0)

**High aggression (> 0.7):**
- Check for clipping on transients. Peak limit at -1 dBFS with a fast-attack limiter (1ms attack, 10ms release) before export.
- Aggressive patches normalize to -1 dBFS: producers expect to drive them hard.
- Transient preservation is critical. Do not use slow-attack limiting (> 3ms) on aggressive patches — it will soften the character.
- ONSET, OBESE, and OUROBOROS at high aggression often have inter-sample peaks that exceed 0 dBFS in the render buffer. Apply true-peak limiting at -1 dBTP.

**Low aggression (< 0.3):**
- Gentle, ambient character. Normalize to -6 dBFS.
- Low aggression patches are most likely to be used in layered contexts where additional headroom is needed.

---

## Part 2: Engine-Specific Best Practices

### 2.1 Percussion Engines: ONSET, OBESE (percussive presets)

ONSET is the fleet's primary percussion engine. Its XVC cross-voice coupling (Blessing B002) is the reason ONSET exports are unlike any other engine in the fleet — the coupled state is the point.

**Recommended render lengths per ONSET voice:**

| Voice (`perc_` prefix) | Render Length | Tail | Notes |
|------------------------|---------------|------|-------|
| Kick (`perc_kick*`) | 800ms | 200ms | Capture sub ring; 1s total |
| Snare (`perc_snare*`) | 600ms | 200ms | Body + crack; 800ms total |
| Closed Hat (`perc_chat*`) | 300ms | 100ms | Very tight; 400ms total |
| Open Hat (`perc_ohat*`) | 1.2s | 300ms | Let the decay develop; 1.5s total |
| Clap (`perc_clap*`) | 500ms | 100ms | Snap + tail; 600ms total |
| Tom (`perc_tom*`) | 1.0s | 300ms | Pitch decay; 1.3s total |
| Perc / Misc (`perc_perc*`) | 400ms | 150ms | 550ms total |
| FX voice (`perc_fx*`) | 2.0s | 500ms | Longest; 2.5s total |

**ONSET XVC coupling renders:** The XVC (cross-voice coupling) system allows any voice to modulate another. When rendering coupled ONSET presets, always render the full kit with all voices active — the coupling interactions are baked into the audio. Rendering individual voices in isolation loses the cross-voice character entirely.

**OBESE percussive presets:** OBESE (`fat_` prefix) bass register focus. For OBESE pluck/percussive patches:
- The driven Moog-style ladder filter (`fat_fltMode`, `fat_satDrive`) is the character. Render with drive engaged.
- Mojo Control (`fat_mojoAnalog` / `fat_mojoDigital`) shifts the fundamental character. Render both axis extremes if creating Entangled variants.
- Bass register C1-C3 only — OBESE above C3 is musically limited.
- 4 velocity layers for OBESE percussion (the saturation responds significantly to velocity).

**Velocity layer strategy for percussion — timbral shift, not just amplitude:**

The velocity transition must involve both level and timbre. In practice:
- **v1 (pp, MIDI 20):** Filter closed, minimal noise burst, fundamental dominant
- **v2 (mp, MIDI 50):** Filter opening, noise begins to mix, character emerging
- **v3 (mf, MIDI 80):** Filter open, full noise burst, body resonance engaged
- **v4 (ff, MIDI 120):** Filter wide open or gating, saturation contributing, maximum character

If the filter does not open with velocity in a percussion preset, the velocity layers are wasted. Fix the preset: `perc_filterVelocity` > 0.3 is the minimum for meaningful timbral velocity response (D001 doctrine compliance).

**Tuning:** Pitched percussion at C3 (MIDI note 60) as root note. Un-pitched percussion (noise-dominant kicks, claps, hats) at C2 (MIDI note 48). `RootNote=0` in XPM always — let MPC auto-detect.

**Stereo placement:**
- Kick: centered, mono render
- Snare: centered-wide (stereo, mild width), or mono for clean stem work
- Hi-hats: stereo, ±20-30% offset from center
- Claps: stereo wide (claps benefit from presence on both sides)
- Toms: stereo, mild pan suggesting natural kit placement

**Transient preservation:** For sub-5ms attack times (clicks, slaps, rim shots), render at 48kHz minimum. The MPC's sample playback at 44.1kHz will interpolate, but a 48kHz source preserves the transient shape better than 44.1kHz in time-critical situations.

---

### 2.2 Harmonic and Melodic Engines

This category covers engines designed for pitched melodic content: ODYSSEY (Drift), ORBITAL, OPAL, ORGANON, ORACLE, OVERWORLD, OBSCURA, OBSIDIAN, OBLONG (Bob), ORIGAMI, OUROBOROS, OVERBITE (Bite), OHM, ORPHICA, OBBLIGATO, OTTONI, OLE, OWLFISH, OBLIQUE, OSPREY, OSTERIA, OCEANIC, OCELOT.

**Root note:** C3 (MIDI 60) for single-shot programs. C4 (MIDI 72) for looped keygroups. All XPM exports use `RootNote=0` — MPC auto-detects the root from the sample naming.

**Note sampling range:** Every minor 3rd from C1 to C6 = 21 notes. This is the standard `MINOR_3RD_NOTES` in `xpn_render_spec.py`. The MPC will pitch-shift between sample points.

Narrow the range if the engine has a natural register limit:
- OBESE, OCEANDEEP: C1-C3 only (bass identity engines)
- OVERWORLD: C1-C4 (chip-register range)
- OTTONI, OWLFISH: C1-C4 (brass/sub-harmonic character above C4 is thin)
- ODYSSEY, ORACLE, ORBITAL: Full C1-C6

Wider than 2 semitones of pitch-shifting introduces audible artifacts in synthesis samples (unlike acoustic samples). The minor-3rd sampling interval keeps pitch-shifting to ±1.5 semitones maximum — comfortable for most synthesis engines.

**Release tail:** 500ms minimum for melodic patches. 2+ seconds for evolving patches. Truncated release is the most common defect in melodic XPN exports.

**Velocity layers — 3 layers minimum for melodic content:**
- v1 (pp, MIDI 40): soft playing character — filter slightly closed, gentle dynamics
- v2 (mf, MIDI 85): nominal playing character — full filter open, standard dynamics
- v3 (ff, MIDI 120): hard playing character — filter wide, possible saturation/distortion

For keygroup programs: `ZonePlay=1` (Velocity). Split points: v1 (1-49), v2 (50-99), v3 (100-127).

**Pitch stability test:** Render a sustained note at C3 for 3 seconds. Import the WAV and check the pitch using any pitch detection tool. The fundamental must stay within ±5 cents of C3 for the duration. Vibrato from LFO pitch modulation is acceptable if it was intentional — but drift is not.

**Transpose range:** -24 to +24 semitones is the comfortable pitch-shifting range for most synthesis-rendered samples. Beyond ±24 semitones, pitch artifacts become significant and additional sample points should be rendered. The minor-3rd spacing keeps the MPC's stretch range well within ±2 semitones, so the full -24/+24 range is available without quality issues.

**Modulation wheel state:** For engines with mod wheel response (all engines under D006 doctrine), render three variants if the mod wheel creates a dramatically different character:
- MW=0 (dry state, `<VelStart>1</VelStart>` layer): the base character
- MW=64 (mid state): partial modulation engaged
- MW=127 (full state): fully expressive state

These become separate XPN programs ("PresetName Dry," "PresetName Full") rather than velocity layers within the same program.

---

### 2.3 Engine-Specific Melodic Notes

**ODYSSEY (Drift, `drift_` prefix):** Full range C1-C6. ODYSSEY's wavetable morphing (`drift_oscA_mode`, `drift_wavetablePos`) is the core sound. Render at a stable wavetable position (not mid-morph). 4 velocity layers. Render length 3s minimum for drifting/phasing presets.

**ORBITAL (`orb_` prefix):** ORBITAL's Group Envelope System (Blessing B001) means the envelope behavior is more complex than standard ADSR. Hold time of 3+ seconds per note is required to capture envelope interactions. 4 velocity layers. `orb_brightness` is the primary velocity target.

**ORGANON (`organon_` prefix):** Metabolic synthesis. The bloom time varies by preset (formula in `xpn_sound_shape_rendering.md` Section 6.1). Two round-robin renders of the same preset produce naturally different timbral journeys due to VFE non-determinism — this is a feature. Render length must exceed bloom time + 2s sustain. Do not render ORGANON presets at their init state — the organism must be running for the character to emerge.

**ORACLE (`oracle_` prefix):** GENDY stochastic synthesis + maqam scales. The stochastic walk never repeats — use 3+ second hold times and accept that no two renders are identical. This non-determinism is Blessing B010: "the unpredictability is the point." 2 round-robins leverage the natural variation.

**OPAL (`opal_` prefix):** Granular engine. Two export modes:
1. **Pitched keygroup:** Sample C2-C4 every minor third (9 notes), single layer, 6-10 second renders. Best for granular pads and tuned textures.
2. **WAV stem:** 30-second stereo recording of the granular texture playing freely. Best for ambient textures. `xpn_render_spec.py` uses `"program_type": "stem"` for OPAL presets.

For granular engines, render at multiple grain sizes if the grain size parameter is the defining character of the preset — small grains (< 50ms), medium (100-200ms), and large (300ms+) produce very different textures.

**OVERWORLD (`ow_` prefix):** Chip register engines (NES 2A03, Genesis YM2612, SNES SPC700). ERA triangle (`ow_era`) is the 2D timbral crossfade (Blessing B009). Render at each ERA corner position if creating a "Complete" expansion: Pure NES, Pure Genesis, Pure SNES, and Blend center. C1-C4 range; narrow range is appropriate (chip synthesis loses character above C4). 48kHz render sample rate for NES square waves above C4.

**OBLONG (Bob, `bob_` prefix):** Warm analog character. 4 velocity layers, filter velocity sensitivity is load-bearing. Render length 3s. High-pass at 60Hz appropriate for warmth-heavy Bob presets that are not explicitly bass patches.

**OVERBITE (Bite, `poss_` prefix):** Bass-forward character with Five-Macro System (Blessing B008). BELLY/BITE/SCURRY/TRASH/PLAY DEAD macros produce drastically different characters. If exporting as variants, render each macro extreme: `poss_belly` at 0.0 and 1.0 are different programs.

**OHM (`ohm_` prefix):** Sage commune engine. The MEDDLING/COMMUNE axis (`ohm_macroMeddling`) shifts character from restrained solo to full harmonic commune. Two variants: calm (MEDDLING=0.0, COMMUNE=0.0) and full commune (MEDDLING=0.7, COMMUNE=1.0). Capture the hum sustain, not just the attack. Hold time 4+ seconds.

**ORPHICA (`orph_` prefix):** Siphonophore microsound harp. Pluck transient is the identity. Render dry — the harp tail decays naturally. 3 second hold minimum. 4 velocity layers. Full range C1-C6 because the harp character reads across the full keyboard.

**OBBLIGATO (`obbl_` prefix):** Dual wind texture. BOND macro (`obbl_bond`) blends between breath A and breath B. Three variants: bond_a (BOND=0.0), bond_blend (BOND=0.5), bond_b (BOND=1.0). Each is a distinct harmonic blend.

**OTTONI (`otto_` prefix):** Triple brass choir. GROW macro (`otto_macroGrow`) controls ensemble size from intimate solo to full swell. Two variants: grow_solo (minimum) and grow_ensemble (maximum). Capture the attack transient — brass character is defined by the first 50ms of the attack slam.

**OLE (`ole_` prefix):** Afro-Latin trio. DRAMA macro (`ole_macroDrama`) is the primary export axis. Rhythm is load-bearing: capture the attack within the first 50ms. Two variants: calm (DRAMA=0.0) and drama (DRAMA=1.0).

**OBSCURA (`obscura_` prefix):** Physical modeling strings. The stiffness parameter (`obscura_stiffness`) and body resonance take time to develop. Hold 3+ seconds per note. Render dry — OBSCURA's acoustic model does not benefit from added reverb at render time.

**OWLFISH (`owl_` prefix):** Mixtur-Trautonium oscillator (Blessing B014). Subharmonic series extends below 20Hz in some patches. Abyssal register focus: C1-C4 only. Hold time 4+ seconds to capture subharmonic series development. Normalize to -6 dBFS — Owlfish low-end is dense.

**OUROBOROS (`ouro_` prefix):** Strange attractor chaos engine (Blessing B003). The leash mechanism controls chaos depth. Two variants: leash_tight (stable, predictable) and leash_loose (chaotic, evolving). Hold 3+ seconds for feedback to develop. The Velocity Coupling Outputs (Blessing B007) mean velocity-sensitive renders capture genuinely different timbral states.

**OBSIDIAN (`obsidian_` prefix):** Crystal resonance. Glassy harmonics need clean captures — render dry, no added reverb. Full range C1-C6. The depth parameter (`obsidian_depth`) controls harmonic complexity; render at musical depth (0.5-0.7 range) rather than maximum for usable samples.

**ORIGAMI (`origami_` prefix):** Fold synthesis. Fold point extremes produce qualitatively different sounds: low fold (smooth, near-sine) vs. high fold (complex harmonics). Two variants: fold_low, fold_high.

**OCEANIC (`ocean_` prefix):** Chromatophore modulator (Blessing B013). Two separation states: separated vs. converged. Hold 4+ seconds for the tidal evolution. 2 velocity layers (the separation response is more important than velocity for this engine).

**OBLIQUE (`oblq_` prefix):** Prismatic bounce. Full range C1-C6. Render dry — the prism reflections need clean captures to be useful as samples. 4 velocity layers, 48kHz recommended (bright character, aliasing risk at high velocities).

**OSPREY (`osprey_` prefix):** ShoreSystem coastline (Blessing B012, shared with OSTERIA). Shore blend controls cultural timbral character. Two variants: shore_near and shore_far. 4 velocity layers.

**OSTERIA (`osteria_` prefix):** Porto wine warmth. Natural sustain — hold 2+ seconds per note. 4 velocity layers. High-pass at 60Hz appropriate for warmth-heavy Osteria presets.

---

### 2.4 Texture and Ambient Engines

**OPAL, OVERWORLD (atmospheric), ORGANON (texture archetype), OBSCURA, OCEANIC (ambient), OVERLAP**

**Render duration:** 4-16 seconds depending on texture evolution rate. The minimum is: render until the texture stops obviously changing. For slow evolving textures (high space, low movement DNA), this may be 10-15 seconds. For rhythmically dense textures, 4-6 seconds captures the character.

**Loop-ability:** Texture engines benefit from seamless loops if the texture is relatively stationary. The XPN pipeline uses one-shot by default (`LoopOnOff=False` on all layers). If the preset is tagged `"sustainLoop": true` in `.xometa`, the pipeline will attempt automatic loop-point detection (see `xpn_sound_shape_rendering.md` Section 5.3).

**Crossfade loop method for manually created loops:**
1. Find a stable amplitude region in the sustain body (avoid attack and decay)
2. Find zero-crossings within that region
3. Set loop start and end to zero-crossings
4. Apply 50ms crossfade (`LoopCrossfade=50` in XPM)
5. Verify: the loop plays at least 5 times in sequence without audible click or timbral reset

**Stereo width:** Render texture engines at full stereo width. Apply mono-sum test: summing to mono should not lose more than 3 dB of perceived level and should not produce obvious comb filtering. If it does, check for phase issues in the source patch.

**Granular engines (OPAL):** Render at multiple grain sizes if grain size is the primary character parameter (`opal_grainSize`). Small grains (15-30ms) produce smooth tonal blending. Large grains (200-500ms) produce pitched stutters. These are different sounds; they should be different XPN programs.

**Movement baking for textures:** Capture the "most interesting" modulation state, not the init state. For a slowly evolving pad, this may mean starting the render after a 5-second pre-roll where the patch has already been playing. The MPC producer should not experience a slow buildup every time they trigger the sample — unless the buildup IS the sound design intent.

---

### 2.5 Coupling Presets (Entangled Mood)

Entangled mood presets define the XOmnibus difference. They must be rendered in their coupling state — the coupling IS the preset.

**Always render both engines at their coupling state.** Do not render each engine solo and combine in the XPM. The coupled output is not the sum of two solos — it is a new sound that only exists when the coupling is active.

**Dry reference renders:** Also render engine A solo and engine B solo as separate XPN programs for educational comparison, not as layers in the same XPN program. Label them: "PresetsName A Solo" and "PresetName B Solo."

**Coupling type affects render strategy:**

| Coupling Type | Render Strategy |
|---------------|-----------------|
| `AudioToBuffer` | Capture the output of engine B (the buffer-receiving engine). Engine A feeds engine B's sample buffer. The XPN sample should be engine B's output. |
| `FrequencyModulation` | Render with FM depth at the preset's designed depth (not min, not max). 50% is not always the interesting state — render at the depth where the FM character is clearest. |
| `AmplitudeModulation` | Capture the pumping/AM effect at its natural depth. Render long enough to capture the AM rate (minimum 2 AM cycles at the LFO rate). |
| `PitchTracking` | Render the target engine being driven (engine B). Engine A's pitch modulation is baked into engine B's pitch response. |
| `FilterCutoffDriving` | Render engine B (the driven engine) with filter sweep at the coupling's characteristic sweep depth. |
| `ReverbSend` | Render with reverb at the designed wet/dry ratio. Do not strip reverb. |
| `ChaoticModulation` | Accept non-determinism. Render 2-3 times and use the most musically useful render. |

**Coupling preset render length:** Coupling interactions often take 1-3 seconds to develop. Use a 2-second pre-roll before the render window begins. The rendered sample should start when the coupling has already settled into its characteristic state.

---

## Part 3: Kit Curation Principles

### 3.1 Velocity Layer Curation

**Audition in context.** Every velocity layer should be auditioned in a beat at 90-130 BPM, not in isolation. The layer that sounds "correct" when playing solo may be dull in a groove, or the one that sounds thin in isolation may cut perfectly in a mix.

**The velocity transition must be smooth at the crossover velocity.** Test by programming a MIDI sequence that sweeps from velocity 49 to 50 (the v1/v2 boundary) and velocity 99 to 100 (the v2/v3 boundary). The transition must not produce an audible timbre jump.

Exception: **intentional timbre breaks are valid design choices.** A brushed snare that transitions to a full strike at velocity 80 is a legitimate artistic choice. Document these breaks in the `.xometa` metadata: `"velocityBreak": {"velocity": 80, "description": "Brush to full strike"}`.

**Test at fast repetition rates.** Program a sequence of 16th notes at 140 BPM. Repetition artifacts — phase coherence between samples, audible click from retrigger — reveal themselves at fast repetition rates. Address these before export.

---

### 3.2 Pad Assignment Philosophy

**Standard MPC 4×4 pad layout for XO_OX drum kits:**

| Row (Top to Bottom) | Pads | Content |
|---------------------|------|---------|
| Row 4 (top) | 13-16 | Texture / wildcard — tonal accents, FX elements, atonal hits |
| Row 3 | 9-12 | Melodic accents — short pitched stabs, perc tones, bells |
| Row 2 | 5-8 | Rhythmic core — hats, snare variants, claps, rimshots |
| Row 1 (bottom) | 1-4 | Foundation — sub/kick variants, bass accent, low tom |

This layout places the most frequently triggered elements (kick, snare, hats) on the lower rows, which are the most ergonomically natural for left-hand playing.

**Tension-release pairing:** Place complementary elements adjacent. Pad 1 (tension source) adjacent to pad 2 (release). Examples:
- Pad 5: closed hat (tension/pulse) + Pad 6: open hat (release/space)
- Pad 1: tight kick (punch) + Pad 2: sustained 808 sub (bloom)
- Pad 9: short pluck accent + Pad 10: long resonant bell

**Frequency coverage test:** Program all 16 pads to trigger simultaneously. The composite sound should cover the frequency spectrum from sub through air without obvious holes or masking clusters. Use this test to identify:
- Frequency masking: two pads with the same dominant frequency (one is redundant)
- Frequency holes: a gap in the mid-range that leaves the kit sounding thin
- Sub accumulation: multiple sub-heavy elements on simultaneous pads (clashes)

---

### 3.3 Round-Robin Curation

**3 round-robins minimum** for organic hi-hat and percussion feel.

**6 round-robins for "human" drum textures.** Research on drum machine humanization consistently shows that 4-6 round-robins produce indistinguishability from real performance for most listeners. Beyond 6, the returns diminish.

**Round-robin variation must be subtle.** Audible variation = too much. The variations should create micro-texture — slight pitch drift, slightly different noise component, slightly different envelope attack — not different sounds. If listening carefully, a round-robin cycle should be almost imperceptible. If the differences are immediately obvious, they are too large.

**The ghost round-robin:** Include one round-robin variation that is slightly quieter (2-3 dB) with a slightly rolled-off high end. This variation, triggered at random intervals by the MPC, adds ghost-note character without the producer having to program ghost notes explicitly.

**ONSET XVC round-robins:** The XVC cross-voice coupling means round-robin variations of a coupled ONSET kit are naturally non-identical — the cross-voice modulation state differs with each retrigger. This is a design advantage. Render 3 full kit passes for 3 distinct coupled round-robin states.

---

### 3.4 DNA-Matching Within Kits

A kit should have internal DNA coherence. The 16 pads of a kit should feel like they belong in the same sonic world.

**Coherent kit DNA:** Kick and sub share high aggression + high weight. Hats share similar brightness. Melodic accents share similar warmth. The kit's Sonic DNA reads as a single musical personality.

**Testing for coherence:** Play all 16 pads sequentially in a simple groove. If any pad "sticks out" as coming from a different world, it breaks the kit's identity. Common culprits:
- A bright feliX-character melody accent in an otherwise warm Oscar-character kit
- A highly reverberant pad in an otherwise dry kit
- A highly compressed, punchy element in an otherwise dynamic kit

**Contrast kits are valid.** A kit intentionally spanning the full DNA range (e.g., a "Contrast" kit that uses OddfeliX digital elements alongside OBESE analog warmth) should be explicitly designed for that tension. The contrast is the identity. Label these kits accordingly.

---

## Part 4: MPC Performance Optimization

### 4.1 File Size vs. Quality

**The sweet spot for MPC standalone: 48kHz / 24-bit WAV.**

| Format | Use Case | Notes |
|--------|----------|-------|
| 44.1kHz / 24-bit | Standard melodic content | Acceptable for most synthesis |
| 48kHz / 24-bit | Transients, feliX-bright content | Preserves attack integrity |
| 96kHz / 24-bit | Content with genuine >20kHz information | Rarely necessary for synthesis |
| 44.1kHz / 16-bit | One-shots only, never melodic loops | Dither required; avoid for sustained content |

The MPC's internal sample rate is 44.1kHz. Samples at 48kHz are resampled on import. This is not a problem — the MPC's resampling is clean. The reason to render at 48kHz is to preserve the source transient shape before the resampler handles it.

96kHz renders are only warranted if the synthesis engine generates genuine energy above 20kHz. Analog-modeled and most synthesis engines do not. The exceptions: some physical modeling engines produce partials above 20kHz through physical processes. In practice, for XOmnibus exports, use 48kHz/24-bit as the ceiling.

16-bit is acceptable for one-shots where the lowest-level signal is -60 dBFS or louder. For melodic content, loop points, and any sustained sound below -60 dBFS, 16-bit dither artifacts are audible in quiet passages.

**File size targets:**

| Content Type | Target Size |
|--------------|-------------|
| One-shot (kick, snare, hat) | < 2 MB |
| Melodic single-shot sample | < 8 MB |
| Evolving melodic sample | < 15 MB |
| Texture/ambient sample | < 20 MB |
| Rhythmic loop sample | < 12 MB |

These targets are for individual WAV files, not whole programs. A 21-note keygroup program with 3 velocity layers has 63 WAV files; each should be < 8 MB.

---

### 4.2 Sample Start Trim

Trim silence at the sample start to < 5ms. The MPC's `SampleStart` parameter has latency compensation, but excessive pre-roll adds perceptible delay on live triggering.

**For attack-critical sounds (transients, clicks, snares, plucks):** 0-1ms pre-roll. Any pre-roll on a transient source is audible as a soft attack or a brief moment of silence before the hit.

**For pads and textures:** 2-5ms of silence is acceptable and avoids click artifacts from absolute zero-start.

**Silence detection threshold:** Trim to the point where the RMS level exceeds -60 dBFS. Anything below -60 dBFS at the start of a sample is functionally silence on the MPC and wastes file size.

**Post-roll (tail trim):** Similarly, trim tails after the signal drops below -60 dBFS and stays there for > 100ms. Tails that end abruptly (truncated attack sounds) are an exception — do not trim tails from sounds that have clearly been cut off. If the sound naturally decays below -60 dBFS, trim there. If it was clearly truncated, extend the render time and re-render.

---

### 4.3 Normalization Strategy

Different content types require different normalization targets, and the difference is not subtle.

| Content Type | Normalization Target | Rationale |
|--------------|----------------------|-----------|
| One-shots (general) | -3 dBFS | Leave headroom for MPC's internal EQ and FX chain |
| Loops and textures | -6 dBFS | MPC reverb can add significant level; headroom essential |
| Kick drums | -1 dBFS | Producers expect to drive kicks hard; they will apply compression |
| Transient-dominant attacks | -1 dBFS | True-peak normalized to -1 dBTP |
| Atmospheric textures | -6 dBFS | Layer-able; headroom needed for multiple simultaneous layers |
| Melodic keygroup samples | -3 dBFS | Standard headroom for pitched playback |

**CRITICAL: Do not normalize velocity layers to the same peak.** Louder velocity layers should be louder. The MPC does not apply additional level scaling within a velocity range — the sample level IS the velocity level. If you normalize all three velocity layers to -3 dBFS and rely on the `VelStart`/`VelEnd` range to control dynamics, the dynamic response will be entirely determined by the MPC's linear velocity scaling, which does not capture the timbral/dynamic relationship of the original synthesis.

The ratio between velocity layer peak levels should match the actual dynamic ratio of the synthesis engine at those velocities. If the engine produces -12 dBFS at MIDI 40 and -3 dBFS at MIDI 120, the exported layers should reflect that 9 dB difference, not be normalized to the same level.

**True-peak vs. sample-peak normalization:** Use true-peak normalization (`-1 dBTP`) for transient-dominant content. Sample-peak normalization can miss inter-sample peaks that occur during D/A conversion. On the MPC, inter-sample peaks above 0 dBFS cause audible clipping even if the WAV header shows a sample peak below 0 dBFS.

---

### 4.4 XPM XML Field Reference

Three critical XPM rules — enforced by the XPN pipeline, never bypassed manually:

```xml
<KeyTrack>True</KeyTrack>    <!-- samples transpose across zones -->
<RootNote>0</RootNote>        <!-- MPC auto-detect convention -->
<!-- Empty layer MUST have VelStart=0 to prevent ghost triggering -->
<Layer number="3">
  <SampleName></SampleName>
  <VelStart>0</VelStart>
  <VelEnd>0</VelEnd>
</Layer>
```

**Velocity layer split point reference:**

| Layer Count | v1 Range | v2 Range | v3 Range |
|-------------|----------|----------|----------|
| 1 layer | 1–127 | — | — |
| 2 layers | 1–79 (soft) | 80–127 (hard) | — |
| 3 layers | 1–49 (pp) | 50–99 (mf) | 100–127 (ff) |

For drum kits using 4 layers:

| Layer | VelStart | VelEnd | Render Velocity |
|-------|----------|--------|-----------------|
| v1 (pp) | 1 | 39 | MIDI 20 |
| v2 (mp) | 40 | 79 | MIDI 50 |
| v3 (mf) | 80 | 109 | MIDI 80 |
| v4 (ff) | 110 | 127 | MIDI 120 |

**ZonePlay mode reference:**

| Value | Mode | When to Use |
|-------|------|-------------|
| `0` | Cycle (round robin) | Evolving patches, textures, anything where variation matters more than velocity |
| `1` | Velocity | Transients, sustains, bass, anything with expressive dynamic range |
| `2` | Random | Textures where cycle-order predictability is undesirable |

ZonePlay is per-keygroup. You cannot combine velocity switching and round robin within a single keygroup. Use the overlapping keygroup technique from `xpn_sound_shape_rendering.md` Section 4.2 if both are needed: separate keygroups per velocity band, each with its own Cycle-mode round robins.

**AmpRelease settings by sound type:**

```xml
<!-- Transient: very short release -->
<AmpRelease>0.1</AmpRelease>

<!-- Sustained melodic -->
<AmpRelease>0.3</AmpRelease>

<!-- Evolving / pad -->
<AmpRelease>0.5</AmpRelease>

<!-- Texture / ambient: longest -->
<AmpRelease>1.0</AmpRelease>
```

---

### 4.5 WAV File Naming Convention

The XPN pipeline uses this naming convention (from `xpn_sound_shape_rendering.md` Section 7.3):

| Sound Shape | Pattern | Example |
|-------------|---------|---------|
| Transient (velocity) | `{NAME}__{NOTE}__v{1-3}.WAV` | `Hard_Pluck__C2__v3.WAV` |
| Sustained (velocity) | `{NAME}__{NOTE}__v{1-2}.WAV` | `Warm_Pad__C2__v2.WAV` |
| Evolving (round-robin) | `{NAME}__{NOTE}__rr{1-2}.WAV` | `Cellular_Bloom__C2__rr1.WAV` |
| Bass (velocity) | `{NAME}__{NOTE}__v{1-2}.WAV` | `Sub_Pressure__C1__v1.WAV` |
| Texture (single) | `{NAME}__{NOTE}.WAV` | `Deep_Fog__C2.WAV` |
| Rhythmic (single) | `{NAME}__{NOTE}.WAV` | `Pulse_Grid__C2.WAV` |
| Drum voices | `{SLUG}_{VOICE}_{VEL}.WAV` | `Crush_Kit_kick_v3.WAV` |
| Variant patches | `{NAME}_{VARIANT}__{NOTE}__v{N}.WAV` | `Obbligato_bond_blend__C3__v2.WAV` |

Note the double underscore (`__`) separating name, note, and velocity suffix. Single underscores are used within name slugs and variant labels.

---

## Part 5: Quality Checks Before Export

### 5.1 Pre-Export Checklist

Run through this list for every preset before exporting:

- [ ] **Listen dry:** The preset sounds good without effects stripped, at nominal level
- [ ] **Velocity test:** The timbre audibly shifts between v1 and v3 (not just the volume)
- [ ] **Pitch test:** Sustained note at C3 stays in pitch for the full render length (±5 cents max drift)
- [ ] **Frequency coverage:** The sample is not missing sub or air content that belongs to the preset
- [ ] **Tail check:** The decay is not truncated. Full tail is captured.
- [ ] **Peak check:** No clipping. True-peak < 0 dBFS
- [ ] **Velocity layer levels:** v3 peak > v2 peak > v1 peak (louder layers ARE louder)
- [ ] **Mono sum:** Stereo renders pass mono-sum test (< 3 dB level loss when summed)
- [ ] **Transition audition:** No audible timbre jump at velocity crossover points (v49→v50, v99→v100)
- [ ] **Round-robin check:** If round-robins are used, each variation is meaningfully (but subtly) different

### 5.2 Post-Export XPM Validation

The `xpn_validator.py` tool runs automated checks. Manual checks that the validator does not catch:

- **Ghost triggering:** Load the XPM on MPC and check that no sound plays on pads that should be silent. Ghost triggering is caused by empty layers without `VelStart=0`.
- **Pitch drift across keyboard:** Play C1, C3, C5 in sequence on the MPC. The pitch should track evenly. Pitch drift indicates a `RootNote` mismatch.
- **Level consistency:** Play the same note at velocity 64 five times. Each trigger should be the same level (for Velocity mode) or slightly varied (for Cycle mode).
- **Velocity range dead zones:** Program velocities 1, 50, 100, 127 and confirm each triggers the expected layer. Dead zones (a velocity that triggers no layer) indicate velocity range gaps in the XPM.

---

## Appendix A: Engine Sound Shape Quick Reference

| Engine | Default Sound Shape | Default ZonePlay | Recommended Render Length | Variants |
|--------|---------------------|------------------|--------------------------|---------|
| OddfeliX (Snap) | Transient | Velocity | 3s | — |
| OddOscar (Morph) | Evolving | Cycle | 8s | morph_a, morph_mid, morph_b |
| Overdub (Dub) | Sustained | Velocity | 5s | dry, wet |
| Odyssey (Drift) | Evolving | Cycle | 6s | — |
| Oblong (Bob) | Sustained | Velocity | 4s | — |
| Obese (Fat) | Bass | Velocity | 4s | — |
| Onset (Perc) | Transient | Velocity | Voice-specific | XVC coupled state |
| Overworld | Transient/Sustained | Velocity | 3s | ERA corners |
| Opal | Texture | Random | 10-30s stem | grain sizes |
| Orbital | Sustained/Evolving | Velocity | 4s | — |
| Organon | Evolving | Cycle | bloom_time + 4s | low_metabolism, high_metabolism |
| Ouroboros | Evolving | Cycle | 5s | leash_tight, leash_loose |
| Obsidian | Sustained | Velocity | 4s | — |
| Overbite (Bite) | Bass | Velocity | 4s | macro extremes |
| Origami | Sustained | Velocity | 4s | fold_low, fold_high |
| Oracle | Evolving | Cycle | 6s | — |
| Obscura | Sustained | Velocity | 5s | — |
| Oceanic | Texture | Cycle | 8s | separated, converged |
| Ocelot | Sustained | Velocity | 4s | biome_a, biome_b |
| Optic | Stem | — | 30s (audio-producing only) | — |
| Oblique | Transient | Velocity | 3s | — |
| Osprey | Sustained | Velocity | 4s | shore_near, shore_far |
| Osteria | Sustained | Velocity | 4s | — |
| Owlfish | Bass | Velocity | 6s | — |
| Ohm | Sustained | Velocity | 5s | calm, full commune |
| Orphica | Transient | Velocity | 4s | — |
| Obbligato | Sustained | Velocity | 4s | bond_a, bond_blend, bond_b |
| Ottoni | Transient | Velocity | 4s | grow_solo, grow_ensemble |
| Ole | Transient | Velocity | 4s | calm, drama |
| Overlap | Texture | Cycle | 8s | dry, wet_small, wet_large |
| Outwit | Evolving | Cycle | 6s | rule_simple, rule_complex |
| Ombre | Evolving | Cycle | 6s | memory, forgetting |
| Orca | Sustained/Evolving | Velocity | 5s | calm_hunt, active_hunt, breach |
| Octopus | Evolving | Cycle | 6s | shallow_arms, deep_arms |

---

## Appendix B: Sound Shape Render Settings Summary

Full specification in `xpn_sound_shape_rendering.md` Section 3. Quick reference:

| Setting | Transient | Sustained | Evolving | Bass | Texture | Rhythmic |
|---------|-----------|-----------|----------|------|---------|----------|
| Render length | 2s | 4s | 8s | 3s | 10s | 8 bars |
| Tail capture | 1s | 2s | 3s | 1.5s | 4s | 2s |
| Velocity layers | 3 | 2 | 0 (use RR) | 2 | 1 | 1 |
| Round-robins | 1 | 1 | 2 | 1 | 1 | 1 |
| ZonePlay | 1 | 1 | 0 | 1 | 2 | 1 |
| Normalization | -3 dB | -3 dB | -1 dB | -3 dB | -6 dB | -3 dB |
| Note range | C2-C5 m3 | C1-C6 m3 | C2-C5 m3 | C0-C3 m3 | C2-C4 m3 | C2-C4 m3 |

---

*CONFIDENTIAL — XO_OX Internal Sound Design Document*
*Living document: update when new engines ship or when MPC firmware changes affect export pipeline.*
