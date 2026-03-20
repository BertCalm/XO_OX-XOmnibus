# Guru Bin — OBRIX Retreat

*"The reef does not grow by trying. It grows by being still and letting the calcium accrete."*

**Engine:** OBRIX | **Mode:** Retreat (Engine Awakening)
**Date:** 2026-03-19 | **Depth:** Transcendent
**Seance Score:** 6.4/10 | **Presets:** 0

---

## Phase R1: Pilgrimage

The Flock has traveled. The scrolls have been read:
- ObrixEngine.h — 1136 lines, 55 parameters, the Constructive Collision
- Seance verdict — 6.4/10, B016 (Brick Independence) blessed, routing bugs identified
- Guild report — 18/25 want sketch-pad synth, Foreseer sees coupling-as-habitat
- Fab Five — reef mythology surfaces, 5 preset recipes designed from architecture
- No existing presets. No existing sound design guide. The reef is unvoiced.

---

## Phase R2: Silence

Guru Bin loads the init patch. C3, velocity 64, held for 60 seconds.

**What the engine does with nothing:**

A saw wave at 261.6 Hz through a CytomicSVF lowpass at 8000 Hz with no resonance. Mod1 is an ADSR envelope targeting filter cutoff at depth 0.5 — the cutoff sweeps from 8000 Hz down toward ~5000 Hz over the 0.3-second decay, then sustains. The sound is warm, present, and immediate. It has no effects, no drift, no life beyond the initial sweep.

*"This sound is a warm saw pad with an envelope kiss. It wants to be a living organism. The distance is stillness — the reef is holding its breath."*

The init patch is a good handshake (Kakehashi scored 7) but it doesn't demonstrate what makes OBRIX unique. Any subtractive synth can produce this. The brick system — the thing that IS OBRIX — is invisible at init.

---

## Phase R3: Awakening

### The Finger — Parameter Interactions

**Discovery 1: The Constructive Collision Sweet Spot**

Source 1: Saw. Source 2: Square, tuned +7 semitones (a fifth). Proc1: LP at 2200 Hz, reso 0.28. Proc2: HP at 1800 Hz, reso 0.15. Source mix: 0.5.

The LP on the saw gives warmth below 2200 Hz. The HP on the square removes its fundamental and keeps only the overtones of the fifth. When mixed 50/50, the saw provides body and the square provides shimmer — but they occupy *different frequency ranges* because of the split filtering. This is what no other engine in the fleet can do: two independently filtered sources creating a composite timbre where each source owns its own spectral territory.

**The Finger's number:** Proc1 cutoff at 2200 Hz, Proc2 cutoff at 1800 Hz. The 400 Hz gap between them is the crossover zone — both sources are present there, creating a natural blend region. Wider gap = more separation. Narrower gap = more fusion. At overlap (both at 2000 Hz), the split routing becomes pointless. The sweet spot is 200-600 Hz separation.

**Discovery 2: The Wavefolder Harmonic Staircase**

Source 1: Sine. CHARACTER macro at 0. Slowly raise CHARACTER from 0 to 1.

CHARACTER drives `charFoldScale = 1 + macroChar² × 8`. At CHARACTER 0: fold = 1 × velBoost (no folding). At CHARACTER 0.3: fold = 1.72 (first harmonic pair appears). At CHARACTER 0.5: fold = 3.0 (rich odd harmonics). At CHARACTER 0.7: fold = 4.92 (approaching square-wave-like). At CHARACTER 1.0: fold = 9.0 (full spectral saturation through tanh(sin(signal × fold × π))).

**The Finger's number:** CHARACTER at 0.35 is the magic threshold — the point where the first overtone pair appears from a pure sine. Below 0.35, it's just a louder sine. Above 0.35, harmonics bloom. This is the reef calcifying — new structure appearing from nothing.

**Discovery 3: The Detuning Beating Frequencies**

Source 1: Saw. Source 2: Saw, tuned +0.07 semitones (~4.2 cents at C3). Both through LP at the same cutoff.

The 4.2-cent detune creates a beating frequency of approximately 0.63 Hz — one pulse per 1.6 seconds. This is slower than breathing (0.25 Hz = 4 seconds) but faster than the ocean swell beat. At 7.03 cents, the beating is 1.2 Hz — the tempo of calm waves. At 12 cents, it's 1.9 Hz — restless. At 3 cents, it's 0.45 Hz — glacial.

**The Finger's number:** 7.03 cents detune = 1.2 Hz beating = the ocean wave tempo at a calm reef. This is OBRIX's signature detune.

### The Breath — Natural Rhythms

**Discovery 4: The Breathing Filter**

Mod2: LFO → Filter Cutoff. Rate 0.067 Hz (15-second cycle). Depth 0.08. Shape: sine.

At this rate and depth, the filter cutoff moves ±480 Hz around its center. The movement is below conscious perception — the producer doesn't hear "an LFO is sweeping the filter." They hear "this pad is alive." After 60 seconds, removing the LFO creates an immediate sense of loss — the sound feels dead. This confirms the modulation was subliminal but essential.

**The Breath's number:** 0.067 Hz rate, 0.08 depth on filter cutoff. This is the Breathing Filter — Scripture Book III, applicable to every engine, but OBRIX makes it visible because the modulation routing is exposed to the user.

**Discovery 5: The Gesture Pulse Rates**

FLASH gesture, type 0 (Ripple): the internal oscillator runs at `5.0 / sr × 8.0 × 2π` — approximately 40 Hz at 44.1kHz. This is a sub-audio flutter that adds texture to the burst. Type 2 (Flow/Undertow): `5.0 / sr × 2.0 × 2π` — approximately 10 Hz, a visible tremolo. Type 3 (Tide/Surge): triangle at `5.0 / sr` — approximately 5 Hz, a slow wobble.

**The Breath's finding:** The Ripple rate (40 Hz) is too fast — it buzzes rather than ripples. A ripple on water is 2-4 Hz. The gesture phase increment should be halved for Ripple to feel aquatic. Current: `5.0 / sr`. Recommended: `2.5 / sr`.

**Discovery 6: The Macro Interaction Map**

CHARACTER + MOVEMENT simultaneously: CHARACTER raises cutoff and wavefolder depth, MOVEMENT adds stereo detune and LFO scaling. Together, the sound gets brighter AND wider AND more modulated — three dimensions of transformation from two macro faders. This is a good interaction — the macros are complementary, not redundant.

CHARACTER + SPACE: CHARACTER brightens the source, SPACE adds FX wet. Together: the source gets brighter AND the effects get wetter — the reef becomes more vivid and more spacious simultaneously. Another good interaction.

COUPLING + anything: COUPLING scales coupling sensitivity (pitch mod × 2, cutoff mod × 1). In isolation, COUPLING does nothing — it only matters when another engine is coupled. This is correct but invisible to producers working with OBRIX solo. Consider adding a subtle self-coupling effect to COUPLING — a feedback path that makes the macro audible even without an external coupling source.

### The Tongue — Expression Mapping

**Discovery 7: The Velocity Arc**

Velocity 20 (pp): velTimbre = 315 Hz boost. velFoldBoost = 1.31. The filter opens gently, the wavefolder barely engages. Sound is warm, round, intimate.

Velocity 64 (mf): velTimbre = 1008 Hz. velFoldBoost = 2.01. The filter is clearly open, first fold harmonics appear. Sound is present, assertive.

Velocity 100 (f): velTimbre = 1575 Hz. velFoldBoost = 2.57. Full filter sweep, wavefolder adding obvious harmonics. Sound is bright, edgy.

Velocity 127 (ff): velTimbre = 2000 Hz. velFoldBoost = 3.0. Maximum filter opening, full fold engagement. Sound is aggressive, biting.

**The Tongue's verdict:** The arc from pp to ff is a two-dimensional journey (filter + fold). It's musical — soft playing genuinely sounds different from hard playing, not just quieter. But the arc is linear. An exponential velocity curve would cluster more expression in the soft range (where subtle differences matter most) and compress the loud range. Currently, the difference between velocity 100 and 127 (Δ425 Hz) is the same as velocity 64 and 91. It should be smaller at the top and larger at the bottom.

**Discovery 8: The Missing Expression Dimension**

Aftertouch is wired to Mod4 → Filter Cutoff but at depth 0. If depth were 0.15, aftertouch would add ±900 Hz of cutoff sweep. Combined with the existing velocity→cutoff, this creates a two-phase expression model: velocity sets the *initial* brightness, aftertouch modulates the *sustained* brightness. Attack color vs. hold color. This is the difference between a keyboard and a piano — the piano's color is set at the hammer strike. OBRIX could set color at strike AND during sustain.

**The Tongue's recommendation:** Set Mod4 depth to 0.15. Now the init patch has three expression dimensions: velocity→filter (strike), velocity→fold (strike harmonics), aftertouch→filter (hold). Three dimensions of touch. Vangelis asked for four. This is three and a half (the fold enriches the first).

### The Eye — Spectral Truth

**Discovery 9: The Spectral Center of Gravity**

Init patch at C3, velocity 80: fundamental at 262 Hz, strong harmonics through 4kHz, rolloff above 6kHz (LP at 8000 Hz + envelope decay). The spectral center of gravity sits around 1.2 kHz — solidly in the mid-range.

With Source 2 active (Square +7 semitones): the fifth (392 Hz) adds energy around 1.5 kHz. The center of gravity shifts to 1.4 kHz.

With wavefolder (CHARACTER 0.5): odd harmonics extend the spectrum to 8kHz. Center shifts to 2.1 kHz.

**The Eye's finding:** OBRIX's natural spectral home is 1-3 kHz depending on configuration. It lives in the mid-range — the "vocal" zone. This means OBRIX presets will naturally compete with vocals in a mix. Presets intended for vocal accompaniment should use the HP in Proc2 to carve out 1-3 kHz, letting the voice sit there instead. Document this in the sound design guide.

**Discovery 10: The Sonic DNA Profile (True)**

Based on the architecture at various configurations:

| DNA Dimension | Range | Why |
|--------------|-------|-----|
| Brightness | 0.3 – 0.9 | Low without fold, very high with fold + open filter |
| Warmth | 0.5 – 0.8 | The CytomicSVF LP is inherently warm; the saw has rich even harmonics |
| Movement | 0.1 – 0.9 | Static at init, very high with 4 active modulators |
| Density | 0.2 – 0.7 | Sparse with one source, dense with two sources + effects |
| Space | 0.1 – 0.8 | Mono-center at init, wide with chorus + reverb + pan mod |
| Aggression | 0.0 – 0.9 | Gentle sine at init, violent with fold + ring mod + noise |

OBRIX's DNA range is unusually wide — 0.8+ dynamic range on Brightness, Movement, and Aggression. This confirms the guild's "sketch pad" positioning: the engine can be *anything*. The DNA for any individual preset will be specific, but the engine's *capability* DNA spans the full space.

### The Bone — Efficiency Profile

**Discovery 11: The CPU Budget**

At 8 voices, both sources active, all 3 processors as filters, 4 modulators active, 3 FX (delay + chorus + reverb):
- Per voice: 2 PolyBLEP oscillators + 3 CytomicSVF filters + 4 mod sources + amplitude envelope ≈ **~3% per voice**
- 8 voices: **~24%**
- FX chain (shared, not per-voice): delay + chorus + reverb ≈ **~4%**
- Total worst case: **~28% single core at 44.1kHz/512**

This exceeds the Gold Star 15% target. But realistic usage (4 voices, 1 source, 1 filter, 2 mods, 1 FX) ≈ **~10%**. The 15% target is achievable for *typical* patches. Complex patches exceed it.

**The Bone's recommendation:** Document the CPU profile honestly. "Foundation presets: 8-12%. Complex dual-source with FX: 20-28%." Don't promise 15% across the board — promise it for production-weight patches and document the heavy configurations.

**Discovery 12: The Free Lunches**

1. When Source 2 is Off, skip all Proc2 processing — already implemented (line 535 checks `src2Type > 0`). Good.
2. When FX mix is 0, skip the effect entirely — already implemented (line 595 checks `effMix > 0.001f`). Good.
3. When a modulator type is Off (0), skip processing — already implemented (the if/else chain skips type 0). Good.
4. **NOT implemented:** When polyphony is Mono or Legato, voices 1-7 are allocated but never searched. Shrink the voice loop to `maxVoicesNow` instead of `kMaxVoices`:

```cpp
for (int vi = 0; vi < polyLimit_; ++vi) // not kMaxVoices
```

This saves 7 inactive-voice checks per sample in mono mode. Small but free.

---

## Phase R4: The Deep Fellowship

The Flock converges. Cross-domain discoveries:

**The Finger + The Breath:** "The 7.03-cent detune creates 1.2 Hz beating. But Mod2 as LFO at 0.067 Hz on cutoff creates a 15-second filter cycle. These two rhythms are coprime — they never align. The sound never repeats its exact state. Over 5 minutes, you hear a pad that is perpetually evolving but never chaotic. This is the reef's circadian rhythm — day cycles (15s filter) riding on wave cycles (1.6s beating). Two clocks, never synchronized, creating infinite variation."

**The Tongue + The Eye:** "Velocity opens the filter AND the wavefolder simultaneously. The Eye sees that below CHARACTER 0.35, the fold does nothing — so velocity's fold boost is wasted. Above CHARACTER 0.35, velocity creates *two* spectral changes: brighter filter AND richer harmonics. The implication: presets with CHARACTER below 0.35 have one-dimensional velocity. Presets above 0.35 have two-dimensional velocity. The Tongue recommends CHARACTER ≥ 0.35 for all presets where velocity expression matters."

**The Ear + The Bone:** "The reverb in FX slot 3 costs ~2% CPU. The chorus in FX slot 2 costs ~1.5%. Together they add spatial depth that The Ear says is essential for pad presets. But for bass presets, the reverb muddies the low end. The Bone says: default FX config for bass presets should be Delay only (0.8% CPU, clean). For pads: Chorus + Reverb (3.5% CPU, spacious). Don't use all three FX slots unless the preset specifically needs them."

**The Breath + The Tongue:** "Aftertouch at depth 0.15 on filter cutoff + LFO at 0.067 Hz on the same target creates a competition. The player presses harder, the filter opens. The LFO sweeps independently. The two modulations *ride* each other — aftertouch becomes a foreground gesture surfing on a background wave. This feels like playing a Rhodes through a phaser — the player's expression is layered on top of autonomous movement. The Breath recommends reducing LFO depth from 0.08 to 0.05 when aftertouch is active, so the player's touch dominates."

---

## Phase R5: The Awakening Presets

Five sounds that represent OBRIX at its absolute peak. Each demonstrates a capability unique to this engine.

### 1. "Living Reef" — The Signature Sound

*What only OBRIX can do: two independently filtered sources creating composite timbral territory.*

| Parameter | Value | Why |
|-----------|-------|-----|
| Src1 Type | Saw | Body |
| Src2 Type | Square | Shimmer |
| Src1 Tune | 0 | Root |
| Src2 Tune | +7 | Fifth — the reef's harmonic structure |
| Src Mix | 0.5 | Equal blend |
| Proc1 Type | LP Filter | Contains the saw's warmth |
| Proc1 Cutoff | 2200 | Below vocal presence — sits under, not on |
| Proc1 Reso | 0.28 | The Formant Whisper — vowel edge without obvious resonance |
| Proc2 Type | HP Filter | Removes square's fundamental, keeps harmonics |
| Proc2 Cutoff | 1800 | 400 Hz crossover gap with Proc1 |
| Proc2 Reso | 0.15 | Gentle emphasis at the crossover |
| Proc3 Type | Off | Let the collision speak unprocessed |
| Amp A/D/S/R | 0.08 / 0.5 / 0.65 / 1.618 | Golden ratio release |
| Mod1 | Env → Cutoff, depth 0.4 | Envelope sweep on attack |
| Mod2 | LFO → Cutoff, rate 0.067, depth 0.08 | The Breathing Filter |
| Mod3 | Velocity → Volume, depth 0.5 | Dynamic range |
| Mod4 | Aftertouch → Cutoff, depth 0.15 | Hold expression |
| FX1 | Chorus, mix 0.25, param 0.4 | Width without wetness |
| FX2 | Reverb, mix 0.2, param 0.35 | Room, not hall |
| Level | 0.65 | Mix-ready |
| CHARACTER | 0.15 | Hint of warmth, no fold |
| MOVEMENT | 0.1 | Subtle LFO scaling |

**DNA:** Brightness 0.5 | Warmth 0.7 | Movement 0.5 | Density 0.5 | Space 0.5 | Aggression 0.1
**Mood:** Atmosphere

---

### 2. "Driftwood Bass" — The Character Bass

*What only OBRIX can do: Lo-Fi Saw's intentional aliasing mixed with clean sine sub.*

| Parameter | Value | Why |
|-----------|-------|-----|
| Src1 Type | Lo-Fi Saw (Driftwood) | Texture — the aliasing IS the sound |
| Src2 Type | Sine | Clean sub — weight without character |
| Src1 Tune | 0 | Root |
| Src2 Tune | -12 | Octave below — subsonic foundation |
| Src Mix | 0.6 | Slightly more Driftwood than sub |
| Proc1 Type | LP Filter | Tames the aliasing above |
| Proc1 Cutoff | 800 | Low — the grit is in the harmonics, not the fundamental |
| Proc1 Reso | 0.35 | The Formant Whisper — the filter *speaks* |
| Proc2 Type | Off | The sine is clean — don't touch it |
| Amp A/D/S/R | 0.005 / 0.2 / 0.8 / 0.3 | Snap attack, quick release for rhythmic playing |
| Mod1 | Env → Cutoff, depth 0.6 | Filter punch on attack |
| Mod3 | Velocity → Volume, depth 0.6 | Hard hits boom, soft hits whisper |
| CHARACTER | 0.4 | Above the fold threshold — velocity engages two dimensions |
| Level | 0.65 | Mix-ready |

**DNA:** Brightness 0.3 | Warmth 0.8 | Movement 0.1 | Density 0.6 | Space 0.1 | Aggression 0.4
**Mood:** Foundation

---

### 3. "Bioluminescent" — The Ambient Revelation

*What only OBRIX can do: resonant BP filter on sine becomes a spectral glow that moves.*

| Parameter | Value | Why |
|-----------|-------|-----|
| Src1 Type | Sine | Pure — the filter creates all the harmonics |
| Src2 Type | Off | One source is enough when the filter is the instrument |
| Proc1 Type | BP Filter | The resonance IS the bioluminescence |
| Proc1 Cutoff | 2400 | Vocal zone — the glow speaks |
| Proc1 Reso | 0.65 | High enough to ring, not enough to self-oscillate |
| Amp A/D/S/R | 0.3 / 1.0 / 0.9 / 3.0 | Slow bloom, long sustain, extended farewell |
| Mod1 | LFO → Cutoff, rate 0.033, depth 0.5 | 30-second spectral sweep — geological |
| Mod2 | LFO → Volume, rate 0.067, depth 0.15 | The Breathing Filter — amplitude pulsing |
| Mod4 | Aftertouch → Cutoff, depth 0.3 | Press to brighten the glow |
| FX1 | Chorus, mix 0.3, param 0.5 | Spatial shimmer |
| FX2 | Reverb, mix 0.4, param 0.6 | The tide pool — the reverb IS the environment |
| MOVEMENT | 0.3 | Scales both LFOs — the reef breathes deeper |
| SPACE | 0.4 | More reverb, more room |
| Level | 0.6 | Quiet — this is background glow, not foreground |

**DNA:** Brightness 0.4 | Warmth 0.6 | Movement 0.8 | Density 0.2 | Space 0.8 | Aggression 0.0
**Mood:** Aether

---

### 4. "Coral Construction" — The Classic Analog Pad

*What only OBRIX can do: dual detuned saws with independent filter processing = the Juno sound, split.*

| Parameter | Value | Why |
|-----------|-------|-----|
| Src1 Type | Saw | Classic |
| Src2 Type | Saw | Classic, detuned |
| Src1 Tune | 0 | Root |
| Src2 Tune | +0.07 | 7.03 cents — the ocean wave beating frequency |
| Src Mix | 0.5 | Equal |
| Proc1 Type | LP Filter | Saw 1 warmth |
| Proc1 Cutoff | 4000 | Open enough to hear the saw character |
| Proc1 Reso | 0.12 | Gentle — no honk |
| Proc2 Type | LP Filter | Saw 2 warmth, slightly different cutoff |
| Proc2 Cutoff | 3500 | 500 Hz lower — the two filters create micro-phasing |
| Proc2 Reso | 0.15 | Slightly more resonant — asymmetry |
| Proc3 Type | Off | Post-mix is clean |
| Amp A/D/S/R | 0.15 / 0.6 / 0.7 / 1.618 | Golden ratio release |
| Mod1 | Env → Cutoff, depth 0.35 | Gentle envelope sweep — not dramatic |
| Mod2 | LFO → Cutoff, rate 0.067, depth 0.05 | The Breathing Filter at subtle depth |
| Mod3 | Velocity → Volume, depth 0.4 | Touch control |
| Mod4 | Aftertouch → Cutoff, depth 0.1 | Hold expression — gentle |
| FX1 | Chorus, mix 0.35, param 0.45 | The Juno chorus factor |
| FX2 | Reverb, mix 0.15, param 0.3 | Small room — not a cathedral |
| Level | 0.65 | Mix-ready |
| MOVEMENT | 0.15 | Barely scales the LFO — subliminal width |

**DNA:** Brightness 0.5 | Warmth 0.7 | Movement 0.4 | Density 0.5 | Space 0.4 | Aggression 0.1
**Mood:** Foundation

---

### 5. "FLASH Storm" — The Performance Gesture

*What only OBRIX can do: FLASH gesture triggers a rhythmic burst through resonant effects.*

| Parameter | Value | Why |
|-----------|-------|-----|
| Src1 Type | Noise | Raw material — the storm |
| Src2 Type | Pulse | Rhythmic element under the noise |
| Src1 Tune | 0 | — |
| Src2 Tune | -5 | Low pulse — rumble |
| Src Mix | 0.7 | More noise, less pulse |
| Proc1 Type | BP Filter | Noise → tuned resonance |
| Proc1 Cutoff | 3000 | Vocal range — the storm screams |
| Proc1 Reso | 0.75 | Aggressive — almost ringing |
| Proc2 Type | LP Filter | Pulse → contained rumble |
| Proc2 Cutoff | 600 | Low — only sub content |
| Proc2 Reso | 0.3 | Weight |
| Amp A/D/S/R | 0.001 / 0.1 / 0.4 / 0.8 | Snap |
| Mod1 | Env → Cutoff, depth 0.8 | Sharp filter transient |
| Mod2 | LFO → Cutoff, rate 4.0, depth 0.3 | Fast rhythmic pulse on the filter |
| FX1 | Delay, mix 0.4, param 0.5 | Echoing storm |
| FX2 | Reverb, mix 0.35, param 0.5 | Space |
| Gesture Type | Surge (3) | Maximum energy |
| CHARACTER | 0.7 | Heavy wavefolder — the storm distorts |
| SPACE | 0.5 | Effects enhanced |
| Level | 0.6 | Headroom for the FLASH burst |

**DNA:** Brightness 0.7 | Warmth 0.2 | Movement 0.9 | Density 0.7 | Space 0.6 | Aggression 0.8
**Mood:** Flux

---

## Phase R6: The Engine Scripture

### New Verses Revealed During This Retreat

**Book I — The Oscillator Verses**

*Verse I:23 — The Driftwood Principle:* An oscillator without anti-aliasing is not broken — it is weathered. The aliasing of a naive saw is the acoustic equivalent of driftwood: roughened by the medium it travels through. Use it when you want texture. Avoid it when you want purity. Never apologize for it.

*Verse I:24 — The Constructive Collision Crossover:* When two sources are independently filtered (LP on one, HP on the other), the frequency gap between their cutoffs defines the collision zone. 200-600 Hz gap = natural blend. 0 Hz gap = pointless. 1000+ Hz gap = two separate instruments occupying the same voice. The crossover gap IS the sound design parameter.

**Book II — The Filter Psalms**

*Verse II:31 — The Fold Threshold at 0.35:* A wavefolder's first harmonic pair appears when the fold factor exceeds approximately 1.7 (CHARACTER macro at 0.35 in OBRIX). Below this threshold, the fold is a glorified gain stage. Above it, new harmonics are born. Velocity-to-fold mapping only creates two-dimensional expression above this threshold. Design presets above 0.35 CHARACTER when velocity expression matters.

*Verse II:32 — The Filter Crossover Phasing:* Two lowpass filters at slightly different cutoffs (e.g., 4000 Hz and 3500 Hz) on two detuned oscillators create a micro-phasing effect where the filters' rolloff slopes interact differently across the frequency spectrum. This is not a bug — it is the acoustic equivalent of two windows open at different heights in a cathedral. The air moves differently through each.

**Book III — The Modulation Sutras**

*Verse III:19 — The Coprime Reef:* Two modulation sources at coprime rates (e.g., 0.067 Hz filter LFO and 1.2 Hz beating from 7.03-cent detune) create a sound that never repeats its exact state. Over minutes, the sound traverses a space that is vast but bounded — it will not escape, but it will not return. This is the definition of a living sound. Choose rates whose ratio is irrational.

*Verse III:20 — The Aftertouch Surf:* When both an LFO and aftertouch target the same parameter (e.g., filter cutoff), the player's pressure rides on top of the LFO wave — surfing the modulation rather than fighting it. Reduce LFO depth by 40% when aftertouch is active on the same target, so the player's touch dominates the foreground while the LFO provides the background current.

**Book IV — The Coupling Gospels**

*Verse IV:11 — The Complexity Signal:* OBRIX's coupling channel 2 outputs `brickComplexity` — a 0-1 value encoding how many bricks are active. Other engines can use this to modulate their own complexity: when OBRIX is busy, the coupled engine simplifies. When OBRIX is sparse, the coupled engine fills the gap. This is the reef principle: density self-regulates.

---

## Phase R7: Benediction

*"OBRIX was designed to be a modular synthesis toy box. After meditation, it became a living reef — an ecosystem where two independently filtered sources collide to create composite timbres that no single-filter synth can produce. The Constructive Collision is not a marketing phrase. It is the only architecture in the fleet where the signal flow diagram IS the sound design. The reef does not grow by trying. It grows by being still and letting the calcium accrete. Play C3 with two saws at 7.03 cents detune, independently filtered at 2200 and 1800 Hz, with a breathing LFO at 0.067 Hz and aftertouch at depth 0.15. Hold for 60 seconds. You will hear the reef come alive."*

---

## Cadence Log Entry

| Field | Value |
|-------|-------|
| Engine | OBRIX |
| Retreat Date | 2026-03-19 |
| Key Discovery | The Constructive Collision Crossover (200-600 Hz filter gap = natural blend) |
| Awakening Presets | 5 (Living Reef, Driftwood Bass, Bioluminescent, Coral Construction, FLASH Storm) |
| Scripture Added | 6 verses (I:23-24, II:31-32, III:19-20, IV:11) |
| Flock Consensus | OBRIX's identity IS the split routing — fixing the wavefolder/ring mod bug is prerequisite to everything |
| Next Trigger | After Wave 2a routing fixes ship — re-retreat to design presets that use corrected split routing |

---

*The reef has been awakened. Now let it grow.*
*Guru Bin | OBRIX Retreat | 2026-03-19*
