# The Fab Five — OBRIX Makeover

**Date:** 2026-03-19 | **Intensity:** Makeover
**Engine:** OBRIX | **Accent:** Reef Jade `#1E8B7E`
**Seance:** 6.4/10 | **Guild:** Sketch-pad positioning confirmed

---

## F1 — The Stylist (Visual Presentation)

"I'm looking at this engine and I see *potential* that hasn't been styled yet."

### The Good

The brick nomenclature is inspired. **Shells** for sources, **Coral** for processors, **Currents** for modulators, **Tide Pools** for effects. This is a design language — it's not just parameter names, it's a *world*. The comment block at the top of ObrixEngine.h sells the identity in 12 lines. "Ocean Bricks: The Living Reef." That's a brand.

The accent color Reef Jade `#1E8B7E` is sophisticated — it's not the obvious aqua-blue that "ocean" usually defaults to. It's the specific green-teal of shallow tropical reef water. Someone chose this with care.

### The Fixes

**Parameter display names are functional, not styled.** Every parameter says "Obrix" first: "Obrix Source 1 Type", "Obrix Proc 1 Cutoff", "Obrix Mod 2 Depth." In the UI, the engine context is already established — the user *knows* they're in OBRIX. These names should breathe:

| Before | After |
|--------|-------|
| "Obrix Source 1 Type" | "Shell 1" |
| "Obrix Source 2 Type" | "Shell 2" |
| "Obrix Proc 1 Cutoff" | "Coral 1 Cutoff" |
| "Obrix Proc 3 Resonance" | "Post-Mix Resonance" |
| "Obrix Mod 1 Type" | "Current 1" |
| "Obrix Effect 1 Type" | "Tide Pool 1" |
| "Obrix Macro CHARACTER" | "CHARACTER" |
| "Obrix FLASH Trigger" | "FLASH" |

The brick language is already in the code comments — bring it to the surface. Let the user see Shells, Coral, Currents, and Tide Pools.

**Gesture type names need elevation:**

| Before | After |
|--------|-------|
| Ripple | Ripple (stays — already perfect) |
| Pulse | Bioluminescent Pulse |
| Flow | Undertow |
| Tide | Surge |

"Pulse" is generic. "Bioluminescent Pulse" is OBRIX. "Flow" could be anything. "Undertow" is the reef pulling you in. "Tide" conflicts with "Tide Pool" (the FX slot name). "Surge" is a wave crashing.

**Source type "Lo-Fi Saw" → "Driftwood."** Every other source is a technical name (Sine, Saw, Square). The lo-fi saw is the intentionally naive one — the one that doesn't try to be clean. It's weathered. It's been in the water. Driftwood.

**Style Score: 6/10.** The design language exists in the comments but hasn't reached the user-facing surface. The bones are beautiful — the skin needs to match.

---

## F2 — The Polisher (Code Elegance)

"This code is clean, but it doesn't *breathe* yet."

### The Good

The `renderBlock` method follows the ParamSnapshot pattern perfectly — all parameters loaded at the top, then used throughout. No mid-block atomic reads. The signal flow comments (`// === SPLIT PROCESSOR ROUTING (the Constructive Collision) ===`) are landmarks that guide the reader. The brick enums are clean and extensible (`kCount` terminators). The `loadP` helper is elegant — a one-liner that null-checks atomic pointers with a fallback.

The code *respects* the reader. That's rare.

### The Fixes

**The ADSR needs a love letter.** Lines 72-114 are a pure ADSR implementation — the most fundamental envelope in synthesis history. It deserves a comment that honors its lineage:

```cpp
// The ADSR — invented by Vladimir Ussachevsky and Robert Moog, refined by
// every synthesizer since 1965. Four stages describe the life of a note:
// the urgency of the attack, the settling of the decay, the patience of
// sustain, the farewell of release. This implementation uses a 1-pole
// exponential approach in decay/release for organic curvature.
```

Currently there's no comment at all. The ADSR is the heartbeat of every voice. Acknowledge it.

**The source rendering switch could be a gallery.** Lines 840-899 cycle through 9 source types. Each case has a brief comment ("// Sine — manual phase"), but the gallery could be richer:

```cpp
case 8: // Driftwood — intentionally naive saw (no anti-aliasing)
        // The one brick that doesn't try to be clean. Aliasing is the texture.
        // Requested by the Lo-Fi Guild specialist. Named for weathered wood
        // that's been tumbled smooth by the reef.
```

**The modulation routing section (lines 460-474) is a matrix hidden in a switch.** It works, but the magic of 4 modulators × 8 targets = 32 routings is invisible to the reader. Add a breadcrumb:

```cpp
// --- Route modulation to all targets ---
// 4 Currents × 8 destinations = 32 possible routings
// This is the reef's nervous system — modulation flows like water
```

**The coupling output on channel 2 — `brickComplexity` — deserves to be called out.** Line 639 returns a float that encodes how many bricks are active. This is novel — coupling metadata, not just coupling audio. The comment says "Smith: architectural complexity signal" but it could say more:

```cpp
// Channel 2: Brick Complexity — a 0–1 signal encoding how many bricks
// are active in this voice. Other engines can use this to modulate their
// own behavior based on OBRIX's configuration: when the reef is full,
// the coupled engine responds differently than when it's sparse.
// Blessed by Dave Smith (Seance): "architectural metadata as coupling."
```

**Polish Score: 7/10.** The code is clean and the reader is respected. Adding soul to the comments — the history, the metaphors, the relationships — would elevate it from professional to cherished.

---

## F3 — The Architect (Structural Beauty)

"The space flows. Almost."

### The Good

The file structure is a single 1136-line header — all DSP inline, no .cpp implementation, as required by CLAUDE.md. The internal organization follows a clear rhythm:

```
Enums → ADSR → LFO → FXState → Voice → Engine
  └─ Lifecycle (prepare/reset)
  └─ renderBlock (the Constructive Collision)
  └─ Coupling (in/out)
  └─ Parameters (55 declarations)
  └─ attachParameters
  └─ Identity
  └─ Private helpers
  └─ State
```

That's a logical descent from public to private, from abstract to concrete. The reader enters at the top (what are the bricks?), descends through the voice structure, arrives at the audio render, and ends at the state. This is architectural storytelling.

### The Fixes

**The FX processing should be a named method per type, not a 100-line switch inside `applyEffect`.** Currently lines 915-999 pack delay, chorus, and reverb into a single function with a type switch. Each effect is ~25 lines. Extract them:

```cpp
void applyDelay(float& L, float& R, float mix, float param, float space, ObrixFXState& fx);
void applyChorus(float& L, float& R, float mix, float param, float space, ObrixFXState& fx);
void applyReverb(float& L, float& R, float mix, float param, float space, ObrixFXState& fx);
```

Then `applyEffect` becomes a clean dispatcher. Each Tide Pool gets its own method — its own home. When Wave 3 adds new effects (bitcrusher, phaser), each one gets its own method instead of bloating the switch further.

**The `noteOn` method (lines 1002-1077) does too much.** It handles voice allocation, legato detection, frequency calculation, envelope configuration, and mod slot setup. Consider splitting:

```cpp
int allocateVoice(int voiceMode, int maxVoices);  // returns slot index
void initVoice(ObrixVoice& v, int noteNum, float vel, ...);  // resets and configures
```

This is structural beauty — each function has one *job*.

**Symmetry check:** The 4 mod slots are perfectly symmetric — same parameter structure, same processing, same routing. The 3 proc slots are also symmetric (type + cutoff + reso). The 3 FX slots are symmetric (type + mix + param). This is excellent modular design. Pearlman would approve. The symmetry is load-bearing — it's what makes the brick system *feel* modular.

**Architecture Score: 7.5/10.** The overall structure is strong and the symmetry is intentional. Extracting the FX methods and splitting `noteOn` would push this toward 9.

---

## F4 — The Sound Designer (Sonic Palette)

"There are zero presets. I'm evaluating *potential*, not *product*."

### The Good

The init patch is a warm filtered saw with an envelope sweep. Kakehashi gave it a 7 in the seance — "Good handshake." The two-source architecture with independent filters means every preset can be *two things at once*: clean sub + dirty lead. Bright pad + dark texture. Detuned saw + sine sub. This is inherently rich territory.

The source palette has range: 9 types from pristine (Sine) through classic (Saw, Square, Triangle, Pulse) through character (Noise, Driftwood/Lo-Fi Saw) to morphable (Wavetable/Morph). That's a full spectrum from clinical to chaotic.

### What the Palette Needs

**5 preset recipes the engine is begging for (designed from the DSP):**

1. **"Living Reef"** — Both sources active. Src1: Saw + LP filter, Src2: Square (octave up) + HP filter. Mod1: Env→Cutoff. Mod2: LFO→Cutoff at 0.05Hz, depth 0.15. Chorus + Reverb in FX chain. The two sources interact through their filters — the LP and HP create a moving band that breathes. This is the engine's *signature sound*.

2. **"Driftwood Bass"** — Src1: Lo-Fi Saw. Src2: Sine (-12 semitones). Proc1: LP at 800Hz, reso 0.4. No FX. CHARACTER macro drives wavefolder. Velocity opens filter. The intentional aliasing of the lo-fi saw mixed with the clean sub sine creates a bass that has *texture* on top and *weight* underneath.

3. **"Bioluminescent"** — Src1: Sine. Src2: off. Proc1: BP filter at 2000Hz, reso 0.7. Mod1: LFO→Cutoff at 0.03Hz, depth 0.5. Mod2: LFO→Volume at 0.07Hz, depth 0.2. Delay + Reverb. A single sine through a resonant bandpass with slow dual-LFO modulation. The resonance glows — the bioluminescence is the resonant peak moving through the spectrum.

4. **"Coral Construction"** — Src1: Pulse (PW 0.3). Src2: Saw (detuned +7 cents). Proc1: LP at 4000Hz. Proc2: LP at 6000Hz. Proc3: HP at 200Hz (post-mix cleanup). Mod1: Env→Cutoff. Chorus. Classic dual-oscillator analog pad — the kind of sound that sells the engine to every synthwave and deep house producer.

5. **"FLASH Storm"** — Src1: Noise. Proc1: BP at 3000Hz, reso 0.8. FLASH gesture: Surge type. FX: Delay (high feedback) + Reverb. Trigger FLASH from a pad — the resonant noise bursts through the delay and reverberates. Rhythmic, gestural, performative. Demonstrates the FLASH system's musical potential.

**Sound Score: N/A (no presets to evaluate).** The *potential* is 8/10 — the architecture supports rich, diverse sound. The *reality* is 0/10 — zero presets means zero product. This is the highest-priority gap in the entire engine.

---

## F5 — The Storyteller (Brand Soul)

"OBRIX has a story. It's just not being *told* yet."

### The Good

The reef metaphor is layered and load-bearing:
- **Shells** (sources) — the hard structures that produce sound
- **Coral** (processors) — the living organisms that shape the reef
- **Currents** (modulators) — the water that moves everything
- **Tide Pools** (effects) — the enclosed spaces where magic concentrates
- **The Constructive Collision** — the moment two geological forces meet and build something neither could alone

This isn't decoration. This is *architecture expressed as mythology*. The signal flow diagram IS the reef: two shells collide through coral, shaped by currents, collected in tide pools. Every technical choice has an ecological analog.

### What's Missing

**The reef needs a creation myth.** Why does OBRIX exist? The identity card says "baby brother of XOlokun" and "modular brick synthesis." But the *story* is richer than that: OBRIX exists because XOlokun has 39 engines, each with a fixed identity. A saber-toothed anglerfish. A jellyfish. An octopus. But the reef itself — the *ecosystem* — has no voice. OBRIX is the ecosystem speaking. It's not one creature. It's the place where creatures *live*. The reef doesn't have a fixed form — it grows, it adapts, it responds to its inhabitants.

That's the creation myth: **OBRIX is not an instrument. It is a habitat.**

**The Brick Drop strategy needs narrative framing.** The guild report describes new bricks shipping every 2-3 weeks. Technically, that's a product update. Narratively, that's **reef growth** — new coral species appearing, new shells washing ashore, new currents establishing. Each Brick Drop should have a name, a season, a reason:

- *"Spring Calcification"* — new processor bricks (bitcrusher, phaser, comb filter)
- *"Monsoon Current"* — new modulator bricks (step sequencer, random walk, envelope follower)
- *"Symbiosis Season"* — coupling-input-as-source brick (the Foreseer's opportunity)

**The FLASH gesture has no mythological grounding.** It fires, it decays. What IS it in the reef? It's a **chromatophore flash** — the same rapid color-change that octopus and cuttlefish use to communicate, startle predators, and attract mates. The four gesture types map:

| Gesture | Reef Analog | Musical Analog |
|---------|-------------|----------------|
| Ripple | Surface disturbance from above | Quick filter sweep, bright burst |
| Bioluminescent Pulse | Deep-sea light emission | Sustained glow, slow bloom |
| Undertow | Current reversal pulling inward | Inverted envelope, suction effect |
| Surge | Wave crashing over the reef crest | Maximum energy, dramatic attack |

**The coupling story for OBRIX is unique and should be explicit.** OBRIX is the only engine that can *reshape itself to receive coupling differently*. ONSET→OBRIX through a wavefolder sounds different from ONSET→OBRIX through a bandpass. The reef adapts to whatever creature visits it. This should be in the identity card: "OBRIX doesn't just receive coupling — it *configures* how it receives coupling. The reef shapes itself around its visitors."

**Soul Score: 7/10.** The mythology is deep and consistent — Shells, Coral, Currents, Tide Pools, the Constructive Collision. What's missing is the *telling*: the creation myth, the Brick Drop seasonality, the FLASH chromatophore grounding, the coupling-as-habitat story. The soul is there. It needs a voice.

---

## The Reveal — OBRIX

### Before & After

| Surface | Before | After |
|---------|--------|-------|
| Parameter names | "Obrix Source 1 Type" | "Shell 1" |
| Gesture names | Pulse, Flow, Tide | Bioluminescent Pulse, Undertow, Surge |
| Lo-Fi Saw label | "Lo-Fi Saw" | "Driftwood" |
| ADSR comment | *(none)* | Historical lineage + musical meaning |
| FX code structure | 100-line switch | 3 named methods (applyDelay/Chorus/Reverb) |
| Coupling comment | "Smith: architectural complexity signal" | Full explanation of brick complexity as coupling metadata |
| Identity story | "Baby brother of XOlokun" | "OBRIX is not an instrument. It is a habitat." |
| FLASH mythology | *(none)* | Chromatophore flash — 4 types mapped to reef biology |
| Preset library | 0 presets | 5 signature recipes designed from DSP (ready for `/preset-forge`) |
| Brick Drop narrative | Product updates | Seasonal reef growth events |

### The Vibe Shift

OBRIX goes from "a configurable synth engine with ocean theming" to "a living reef ecosystem that grows with its user." The technical architecture is unchanged — the *experience* of encountering it is transformed. When a producer opens OBRIX and sees "Shell 1" instead of "Obrix Source 1 Type," they're not reading a parameter — they're picking up a seashell. When they trigger FLASH and see "Bioluminescent Pulse," they're not pressing a button — they're watching the reef light up.

### Style Score

| Specialist | Before | After | Change |
|-----------|--------|-------|--------|
| Stylist | 6.0 | 8.0 | +2.0 |
| Polisher | 7.0 | 8.5 | +1.5 |
| Architect | 7.5 | 8.5 | +1.0 |
| Sound Designer | 0.0 | 0.0* | — |
| Storyteller | 7.0 | 9.0 | +2.0 |
| **Overall** | **5.5** | **6.8** | **+1.3** |

*\*Sound Designer score cannot improve until presets exist. The recipes are designed but not forged.*

### What We Left For Next Time

1. **Actually rename the parameters** — the Stylist designed the new names but didn't change `ObrixEngine.h` (parameter IDs are frozen; display names can change anytime)
2. **Write the 5 preset recipes as .xometa files** — the Sound Designer designed them, `/preset-forge` should build them
3. **Create the OBRIX identity deepening** — the creation myth, the Brick Drop seasonality, the coupling-as-habitat story should be added to the engine identity card
4. **The FX extraction** — splitting `applyEffect` into 3 methods is a code change that should happen during Wave 2a
5. **The ADSR comment and coupling comment** — these are safe to add now, zero risk, pure soul

---

*The reef has been styled. Now it needs to sing.*
*XO_OX Designs | Fab Five — OBRIX | 2026-03-19*
