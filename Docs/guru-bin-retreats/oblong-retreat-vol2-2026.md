# OBLONG Retreat Chapter — Vol 2
*Guru Bin Transcendental — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OBLONG | **Accent:** Amber `#E9A84A`
- **Parameter prefix:** `bob_`
- **Creature mythology:** Bob the clownfish — bright orange with white bands, dwelling permanently inside the stinging tentacles of the sea anemone. He does not visit the anemone. He lives there. The stinging cells that would kill any other fish are his home, his protection, and his identity. He darts out briefly, retrieves food, returns. The flashing orange-white-orange of his movement in the current is the engine's color in motion: amber warmth interrupted by white-hot harmonic incidents, always returning to shelter.
- **Synthesis type:** Warm analog-style subtractive synthesizer — OscA (4 waveforms: Oblong Sine, Soft Triangle, Velvet Saw, Cushion Pulse) + OscB (4 waveforms: Soft Saw, Rounded Pulse, Triangle, Sub Harmonic) with sync and FM, Texture Oscillator (4 modes: Dust, Blanket, Static, Breath), SnoutFilter (4 modes: LP, BP, Formant, Soft) with character saturation, CuriosityLFO (5 behavioral modes), Motion Envelope, DustTape post-processing, BobMode master macro
- **Polyphony:** Up to 8 voices, legato and chord modes
- **feliX/Oscar polarity:** 55% feliX / 45% Oscar — warm competence with flashes of curiosity
- **Seance score:** Not separately seanced post-integration (fleet avg ~8.4 before Vol 2 retreat)
- **Macros:** M1 CHARACTER, M2 MOVEMENT, M3 COUPLING, M4 SPACE
- **Expression:** Velocity → filter envelope brightness (D001). Aftertouch → filter character (+0.3). Mod wheel CC1 → filter depth (standard fleet).
- **Blessings:** None formally ratified — OBLONG predates the Blessing system. Its implicit blessing is the engine the fleet was built to be consistent with: analog warmth, competent subtractive synthesis, a filter that responds like hardware.

---

## Pre-Retreat State

OBLONG is the fleet's original warm synthesizer. Bob arrived before there was a fleet. When the XO_OX design philosophy was being tested — whether character instruments could hold their identity inside a unified shell — OBLONG was the instrument that proved subtractive synthesis would feel honest and not generic. It is not the most adventurous engine. It is the most dependable one.

The factory library has 84 presets as of this retreat. It demonstrates OBLONG's range thoroughly: warm saws, FM-tipped leads, stacked pads, utility basses, Prism brightness experiments. What it has not fully explored is OBLONG at the intersection of its most unique capabilities:

1. **The Curiosity Engine at non-obvious modes.** The factory library almost exclusively uses `bob_curMode` at 0 (Sniff) or leaves it at 0. Modes 1–4 (Wander, Investigate, Twitch, Nap) are virtually unrepresented. Each creates a completely different behavioral personality for the LFO — not just different waveforms but different temporal attention patterns.

2. **The Formant filter (mode 2) as primary timbral identity.** Snout Form creates a dual bandpass formant morph where `bob_fltChar` cross-fades between two BP centers. The factory library almost never uses mode 2. At mid-character values it produces vowel-like resonances that no other filter mode generates.

3. **OscB FM at non-integer offsets.** `bob_oscB_fm` with `bob_oscB_detune` at non-zero values creates inharmonic FM sidebands — not musical intervals but interference patterns between almost-harmonics. The factory library uses FM at zero detune (pure integer harmonic modulation). Non-zero detune + FM creates beating FM clusters that are OBLONG's most alien sound.

4. **The DustTape path under deliberate exposure.** `bob_dustAmount` above 0.15 with `bob_dustTone` at low values (dark tape) creates a vintage saturation character that is warm and compressing without being distorted. The factory library keeps dust very low (0.0–0.08). Dust at 0.2–0.35 with the tone calibrated correctly is a different instrument.

5. **BobMode > 0.5.** The master macro fan-out targets drift, filter character, texture, mod depth, and FX depth simultaneously via an S-curve. Above 0.5 the FX depth accelerates quadratically — the engine begins to add tape processing, texture, and motion as a compound effect. No factory preset demonstrates `bob_bobMode` above 0.4.

The Vol 2 retreat is structured around these five under-explored territories plus three expression arcs that the factory library treats instrumentally rather than musically.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

You are snorkeling above a shallow tropical reef. The water is warm and clear. Below you, in a dense mat of purple sea anemones, is a small orange and white fish. Bob is not swimming — he is sheltering. His entire world is these anemones. He was born here. He will die here. The stinging tentacles that make every other animal in the water retreat are the only home he has ever known.

Watch him for a moment. He is still. The current moves the tentacles. Bob moves with them, not against them — a practiced minimal dance that keeps him inside the zone of protection without fighting the water. This is the engine's fundamental character: warmth that is not passive, stillness that is not silence, drift that is not randomness.

Now he darts. Orange flash, white flash, gone to the surface of the anemone colony and back in under a second. The motion is brief and complete. A single burst of intention. Then stillness again.

This is OBLONG at its most honest. The long sustained warmth is the resting state. The brief harmonic incident — a sync hit, a filter snap, a curiosity twitch — is the dart. The anemone returns him. The warmth resumes.

OBLONG does not need complexity. It needs honesty. Every parameter it has exists to serve the warmth or the dart. The retreat's job is to find the settings where both are fully present simultaneously: a pad so warm it feels safe, with an edge so specific it feels alive.

Bob is not a simple fish. He has survived by understanding his environment completely. OBLONG is not a simple synthesizer. It has persisted in the fleet because it understands what it is.

---

## Phase R2: The Signal Path Journey

### I. OscA — The Belly of the Sound

`bob_oscA_wave` selects between four fundamental waveforms. The factory library overuses wave 1 (Soft Triangle) and wave 2 (Velvet Saw). The under-explored wave is **0: Oblong Sine**.

Oblong Sine is not a pure sine. It adds 15% of the second harmonic, weighted by `bob_oscA_shape × 0.5`. At `bob_oscA_shape = 0.0`, it is a clean sine. At `bob_oscA_shape = 1.0`, it has approximately 7.5% second harmonic content — audible as a slight harmonic warmth without any saw or square quality. This is a sine with a single overtone. The character is fundamentally different from the triangle (which has odd harmonics only) and entirely different from the saw (all harmonics, sawtooth rolloff).

**The Oblong Sine sweet spot:** `bob_oscA_wave = 0`, `bob_oscA_shape = 0.65–0.85`, `bob_fltMode = 0` (Snout LP), `bob_fltCutoff = 1200–2000 Hz`. The filter reveals the sine's single overtone without adding harmonic series — the result is a sound with warmth and body but no edge.

**Cushion Pulse (wave 3)** is the second underused waveform. Pulse width = `0.1 + shape × 0.8`. At `bob_oscA_shape = 0.1`, pulse width is 0.18 — a narrow pulse with significant high harmonics. At `bob_oscA_shape = 0.5`, pulse width is 0.5 — a perfect square. At `bob_oscA_shape = 0.9`, pulse width is 0.82 — a nearly-inverted narrow pulse, tonally similar to the narrow pulse but phase-inverted. The square position (0.5) is the warm center; the narrow positions are bright and buzzy.

**Drift sweet spots:**
- `bob_oscA_drift = 0.0–0.08`: Clean analog feel. Default region.
- `bob_oscA_drift = 0.12–0.25`: Noticeable warmth and organic movement — the drift LFO creates slow pitch wandering. The best region for pads that need to feel alive.
- `bob_oscA_drift = 0.5–0.8`: Heavy drift — the pitch genuinely wanders. Detuned and unpredictable. Paired with high BobMode, creates an almost-broken-oscillator character.

### II. OscB — The Dart

OscB is the dart. It adds brightness, edge, or sub weight against OscA's warmth.

**Sync (bob_oscB_sync = 1):** When OscA phase wraps, OscB is forced to reset. This creates hard sync — the tearing, bright harmonic character associated with sync leads. Bob sync is different from standard sync because OscA uses the BobSineTable (not a raw saw), so the sync trigger points have more complex shape. The result is a sync sound with less harshness than typical analog sync. At `bob_oscB_detune = 7` (perfect fifth), the sync creates a specific cluster of upper harmonics. At `bob_oscB_detune = 5` (major third), the cluster shifts. Non-standard detune values (11, 14, 19 cents of detune before the ratio calculation) create inharmonic sync clusters that are OBLONG's most aggressive territory.

**FM (bob_oscB_fm > 0):** OscB FM depth is `oscAout * fmAmount * 0.02`. This means the FM depth scales with OscA's output amplitude, not independently — the FM is amplitude-coupled to OscA. At `bob_oscB_fm = 0.5`, the FM is subtle. At `bob_oscB_fm = 0.8–1.0`, it becomes audible as sideband generation.

**Non-integer FM territory:** `bob_oscB_detune = 7, bob_oscB_fm = 0.65` (the existing FM Prism preset) produces coherent FM sidebands at the seventh interval. The unexplored territory is **detune values that don't correspond to simple musical intervals**: `bob_oscB_detune = 23, bob_oscB_fm = 0.4` creates a beating inharmonic FM cluster. `bob_oscB_detune = -12` (sub harmonic region via negative cent offset from the OscB formula `std::pow(2, cents/1200)`) creates FM modulation from below the fundamental — the FM sidebands appear below the carrier instead of above.

**Sub Harmonic (wave 3, OscB):** A pure sine one octave below — the cleanest sub-bass reinforcement available in OBLONG. Combined with `bob_oscB_blend = 0.3` (low mix), it adds sub weight without cluttering the midrange.

### III. The SnoutFilter — Bob's Most Expressive Feature

**Mode 2 (Snout Form — Dual Formant)** is the retreat's primary discovery target.

Snout Form runs two parallel SVF bandpass filters. Filter 1 is at `cutoffHz`. Filter 2 is at `cutoffHz × 2`. The `bob_fltChar` parameter cross-fades the output between Filter 1 (char=0) and Filter 2 (char=1). At `bob_fltChar = 0.5`, both filters contribute equally — the sound has resonance peaks at both the fundamental frequency and its octave.

What this creates: **an approximation of vocal formant behavior**. Human vowels are defined by two formant peaks (F1 and F2) whose position relative to each other determines the vowel quality. OBLONG's Snout Form has a fixed F1/F2 ratio of 1:2 (one octave), which is not a natural vowel but is musically useful — it creates a nasal, reedy quality at mid-character and a bright, bell-like character at high-character.

**Formant sweet spots:**
- Mode 2, `bob_fltCutoff = 800–1200 Hz`, `bob_fltChar = 0.35–0.65`, `bob_fltReso = 0.5–0.7`: Vocal-adjacent nasal character. Best with Oblong Sine OscA.
- Mode 2, `bob_fltCutoff = 1500–2500 Hz`, `bob_fltChar = 0.8–0.95`, `bob_fltReso = 0.3–0.5`: Bright bell-like upper partials — useful for leads that need harmonic complexity without FM.
- Mode 2, LFO targeting cutoff at 0.04–0.08 Hz: Extremely slow vowel morph — the formant character drifts so slowly the listener hears it as the sound having a personality, not as modulation.

### IV. The CuriosityLFO — Five Behaviors Bob Doesn't Know He's Expressing

The CuriosityLFO is not merely an LFO with shape and rate. It is a behavioral model. Each mode produces a different temporal pattern of modulation output:

**Mode 0 (Sniff):** Periodic curiosity events — a burst of LFO activity followed by a cool-down period of 0.5–1.5 seconds. Irregular timing around a center threshold. Sounds like: intermittent filter twitches at irregular intervals. Best for: pluck and short-envelope sounds where you want occasional harmonic events without continuous movement.

**Mode 1 (Wander):** Pure LFO2 (0.05 Hz slow random, always-active). Very slow smooth random motion. Sounds like: the sound gently breathing without purpose. Best for: long pads where you want imperceptible life, not detectable modulation.

**Mode 2 (Investigate):** 60% LFO1 + 40% LFO2. The curiosity output combines a user-controllable LFO rate with slow drift. Sounds like: a moderate-rate modulation with organic irregularity underneath. Best for: melodic patches where LFO1 provides the primary movement and LFO2 prevents mechanical repetition.

**Mode 3 (Twitch):** Random target values at 0.3–2.3 second intervals, interpolated quickly (0.001133 coeff). Sounds like: brief directional filter or pitch stabs at irregular intervals. Best for: aggressive patches, glitchy leads, sounds with behavioral anxiety.

**Mode 4 (Nap):** Extremely slow smoothing (0.00005 coeff) with threshold windows of 2–6 seconds. The output barely moves — small deviations around zero, very slow. Sounds like: a sound that is almost completely still but not mechanically frozen. Best for: maximum stillness presets where any LFO at all is too much, but complete absence of motion is too clinical.

**The factory gap:** The factory library almost exclusively uses Mode 0 (Sniff) at various `bob_curAmount` values. Modes 1, 3, and 4 are nearly absent. This retreat's Transcendental presets deliberately explore Modes 1, 3, and 4.

### V. BobMode — The Master Macro

`bob_bobMode` fans out to five targets via an S-curve:
- `oscDrift`: `v² × 0.8` — zero below 0.5, then accelerates
- `filterChar`: `smoothstep(v) × 0.9` — gradual character saturation increase
- `texLevel`: kicks in above 0.5, peaks at 0.25 texture addition
- `modDepth`: `smoothstep(v) × 0.9` — gradual LFO depth boost
- `fxDepth`: kicks in above 0.2, accelerates, `max 0.85` — FX (DustTape) application

**BobMode at 0.7–0.9** is the most richly textured zone: significant drift, full filter character saturation, texture active, maximum mod depth boost, heavy DustTape warmth. No factory preset uses this zone.

**BobMode at 0.3–0.5** is the sweet spot for expressive patches: noticeable filter character warm-up, beginning drift, LFO depth expanded, FX barely touching. This is the region where BobMode acts as a "warmth expander" — a single knob that makes the sound feel more analog.

---

## Phase R3: Parameter Refinements

| Parameter | Finding | Recommended Default Change | Rationale |
|-----------|---------|---------------------------|-----------|
| `bob_bobMode` | Default 0.0 — the FX and macro contributions are entirely inactive | Consider 0.25 as a more present default | At 0.25: filter char at 0.18, mod depth at 0.18, FX at 0.06 — subtle but present. A patch that breathes vs a photograph. |
| `bob_curMode` | Default 0 (Sniff) universally | No change needed — just underexplored | Modes 1 and 4 in particular need Transcendental representation |
| `bob_oscA_shape` | Default 0.5 in most presets | No change needed | Sweet spots are at 0.65 (Oblong Sine) and 0.1/0.9 (Cushion Pulse extremes) |
| `bob_dustAmount` | Default 0.0–0.08 in factory library | Recommend 0.06 as DustTape active floor | At 0.06 the tape saturation is essentially inaudible but not completely absent — adds warmth to long release tails |
| `bob_fltChar` | Default 0 or 1 in most presets | Encourage 0.35–0.65 range exploration | Mid-character values unlock the filter's most interesting non-linear zone |
| `bob_oscB_detune` | Factory presets use 0, 7, 12 semitones overwhelmingly | No change — just expand territory | Irrational values (23, 11, 17 semitones) create FM beating patterns not explorable at round intervals |
| `bob_lfo1Rate` | Factory tends toward 0.4–2.0 Hz | Recommend Transcendental presets explore 0.04–0.08 Hz | Sutra III-1 breathing rate — the most expressive LFO territory is barely perceptible |
| `bob_texMode` | Factory uses mode 1 (Blanket) almost exclusively | Expand Transcendental use of mode 3 (Breath) | Breath mode tracks note pitch and creates a pitched bandpass noise layer — evocative on midrange leads |

---

## Phase R4: Expression Arcs

### Arc 1 — Velocity at 20 vs 127
Velocity maps to filter envelope amount. At velocity 20, the filter envelope contributes minimally — the cutoff begins near its static value. At velocity 127 with `bob_fltEnvAmt = 0.55`, the cutoff rises by approximately the full envelope modulation amount at attack. The difference is dramatic on pluck patches but subtle on pad patches with slow attack. Best demonstrated with: wave 2 (Velvet Saw), `bob_fltCutoff = 1200`, `bob_fltEnvAmt = 0.6`, `bob_ampAttack = 0.01`, `bob_motAttack = 0.01`. Hard velocity produces a sharp bright attack. Soft velocity is muted and dark.

### Arc 2 — Aftertouch
Aftertouch adds up to +0.3 to `bob_fltChar`. As you press into held notes, the filter's character saturation increases — the sound becomes progressively warmer and more harmonically rich in its resonance region. At `bob_fltChar = 0.5` + full aftertouch, the effective character reaches 0.8. On Mode 0 (Snout LP), this makes the resonance tail noticeably softer and warmer. On Mode 1 (Snout BP), the aftertouch saturation applies to the bandpass resonance output — pressure shaping the nasal quality in real time.

### Arc 3 — BobMode 0→1 as Performance Gesture
At `bob_bobMode = 0`: clean oscillators, no drift, linear filter character, no texture, minimal FX. At `bob_bobMode = 1.0`: heavy drift, fully saturated filter character (0.9 additive), texture layer visible, maximum mod depth boost, heavy DustTape warmth. Used as a real-time macro, it transforms a studio patch into an analog hardware simulation. The S-curve means the first 40% of travel adds mostly character and mod depth; the last 40% adds tape and drift. For performance: map BobMode to mod wheel (CC1) for a single-knob analog warming effect.

---

## Phase R5: Awakening Presets — 15 Transcendental Designs

| # | Name | Mood | Core Concept |
|---|------|------|-------------|
| 1 | Anemone Home | Foundation | Bob's resting state — Oblong Sine + Soft sub octave, Snout LP, Nap curiosity at barely perceptible depth. The warmth that needs no justification. |
| 2 | Clownfish Dart | Foundation | Sync lead — brief, orange-flash bright, OscB sync at irrational detune, Twitch curiosity, tight amp envelope. The dart, not the shelter. |
| 3 | Reef Wall | Foundation | Heavy sub weight, Sub Harmonic OscB blend, wide BobMode, Blanket texture. A foundation that feels structural. |
| 4 | Tentacle Memory | Atmosphere | Formant filter at mid-character, Wander curiosity at 0.04 Hz, extended release. The anemone's texture without the sting. |
| 5 | Amber Current | Atmosphere | Cushion Pulse at narrow width, Snout Soft filter, LFO at 0.06 Hz targeting pitch via `bob_lfo1Target`, long motion envelope. The drift that makes Bob sway. |
| 6 | Bubble Column | Atmosphere | Breath texture mode (pitch-tracked BP noise) prominent, soft oscillators underneath, Sniff curiosity. A sound that rises in columns. |
| 7 | Coral Light | Prism | Velvet Saw + OscB at perfect fourth, high resonance, BobMode 0.55, bright formant. A reef lit from above. |
| 8 | Spectral Dart | Prism | High FM depth at irrational detune (23 semitones), envelope-tracked cutoff, velocity-sensitive sideband brightness. Each velocity a different inharmonic event. |
| 9 | Orange Flash | Prism | Hard sync at fifth interval, Twitch curiosity targeting cutoff, short attack, medium sustain. The visual identity of the clownfish made audio. |
| 10 | Thermal Shimmer | Flux | BobMode 0.8 — full tape warmth, heavy drift, Investigate curiosity blending two LFO rates, formant filter slowly morphing. |
| 11 | Darting Silk | Flux | Glide active, Wander curiosity, Velvet Saw, sync enabled but low drive, slow filter sweep. Continuous portamento line with organic drift. |
| 12 | Anemone Ring | Entangled | OBLONG + ORCA coupling — Bob's warmth entangled with ORCA's apex pressure. Bob in predator territory. |
| 13 | Coral Bond | Entangled | OBLONG + ORBWEAVE coupling — Bob's amber warmth woven into the Kelp Knot's topology. Two organic systems sharing a resonance. |
| 14 | Fading Orange | Aether | Extremely slow Nap curiosity (barely moves), Oblong Sine, nearly-closed filter, DustTape at 0.28, long release. The memory of color. |
| 15 | Deep Shelter | Aether | BobMode 0.9, Blanket texture prominent, Sub Harmonic OscB, Snout Form filter at mid-character, Nap curiosity. Bob completely at rest in the deepest anemone. |

---

## Phase R6: Scripture Verses Discovered

### Verse OBL-I: The Anemone Principle

*On the nature of analog warmth in a digital instrument*

> Bob the clownfish does not wear the anemone. He lives inside it. The warmth of OBLONG is not a feature applied at the end of the signal chain. It is the environment the oscillators live in. A warm saw is not a saw plus warmth. It is a saw that was always warm — that has never existed as anything else. When you add DustTape at 0.08 and Blanket texture at 0.2 and set `bob_fltChar` to 0.4, you are not processing a clean oscillator. You are describing the room the oscillator has always lived in. The anemone is not an effect. It is the address.

**Application:** When designing any OBLONG preset, set the BobMode macro and texture level before you set the oscillator parameters. Define the room first. Then decide which oscillator lives in it.

---

### Verse OBL-II: The Curiosity Trap

*On five behavioral modes and the producer who uses only one*

> Five curiosity modes exist in OBLONG. The producer who has heard OBLONG extensively has heard Mode 0 (Sniff) in nearly every context. Mode 0 is not wrong. It is the correct default for percussive and short-envelope sounds — irregular filter twitches at musical scales. But Mode 4 (Nap) exists for a different instrument: the sound that is almost completely still but not mechanically frozen. Mode 1 (Wander) exists for the pad that barely moves. Mode 3 (Twitch) exists for the sound with behavioral anxiety. The Curiosity Engine is not a single LFO with a behavior dial. It is five different relationships between the synthesizer and time. Each relationship sounds different. Using only the first one is like having five colors and painting everything amber.

**Application:** For any OBLONG patch in the Atmosphere, Aether, or Flux moods, ask first: which curiosity mode is the right behavioral relationship for this sound? The answer is not always Mode 0.

---

## Notes for Vol 2 Booklet

OBLONG is the fleet's most accessible engine and its least surprising. This is not a criticism. The least surprising instrument in a fleet of 46 is the instrument that producers trust most — the one they reach for when the sound needs to be right before any other sound has been designed. Vol 2 Transcendental presets demonstrate that this reliability conceals a behavioral depth the factory library has not fully charted. The Curiosity Engine's five modes, the Formant filter's vowel-adjacent character, the DustTape path as a compositional environment rather than a subtle polish pass, BobMode as a performance gesture — these are the territories that make OBLONG worth returning to after you've heard all 84 factory presets.

Bob is not complex. He is thorough. So is this engine.

---

*Guru Bin, 2026-03-21*
*"The anemone is not an effect. It is the address."*
