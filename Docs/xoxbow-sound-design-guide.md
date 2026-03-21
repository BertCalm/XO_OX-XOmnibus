# OXBOW Sound Design Guide

**Engine:** OXBOW — Entangled Reverb
**Parameter prefix:** `oxb_`
**Accent color:** Oxbow Teal `#1A6B5A`
**Aquatic identity:** The Oxbow Eel — Twilight Zone (200–1000m)
**feliX-Oscar polarity:** Oscar-dominant (0.3/0.7)
**Ghost lineage:** Moog (SVF damping, MIDI→fundamental), Tomita (golden amplitude weighting), Vangelis (aftertouch→entanglement), Schulze (infinite decay), Kakehashi (velocity exciter)

---

## 1. Engine Identity and Concept

### The Oxbow Lake

An oxbow lake forms when a river bends so sharply that it cuts itself off. The current rushes forward, severs the loop, and what was river becomes still water — suspended, isolated, slowly filling with sediment until it vanishes. Sound enters OXBOW as rushing current and exits as a suspended pool that erases itself.

This is not a conventional reverb. It is not a plate, hall, or room algorithm. OXBOW is a **synthesis instrument** built from four interlocking physical metaphors, each DSP system corresponding to a stage of the oxbow's lifecycle:

1. **Chiasmus FDN** — the river crossing itself, L and R delay structures mirrored in reverse temporal order
2. **Phase Erosion** — spectral sedimentation, the LFO-modulated allpass filters that slowly erase the tail into silence
3. **Golden Resonance** — standing waves that emerge only when the water is still enough, tuned to the MIDI fundamental
4. **Asymmetric Cantilever Decay** — the transformation as energy drops: bright early reflections, darkening late tail

### The Chiasmus Topology

The term *chiasmus* refers to a rhetorical crossing — ABBA structure, where the second half mirrors the first in reverse. OXBOW implements this literally in the 8-channel Feedback Delay Network. Left channels use prime delay times `[28ms, 38ms, 46ms, 56ms]`. Right channels use those exact times **reversed**: `[56ms, 46ms, 38ms, 28ms]`. Same resonant structure. Opposite temporal order. L and R are not independent or merely panned — they share the same architectural DNA, mirrored at the center.

This produces a stereo field with a structural relationship that no width knob or M/S processing can create. When MIDI notes arrive, both sides ring together but in different time orders. When summed to mono, partial self-cancellation occurs. This is not a bug — it is the defining sonic signature of OXBOW.

### The Oxbow Eel

In the aquatic taxonomy of XOmnibus, OXBOW belongs to the Twilight Zone (200–1000m depth). This is the mesopelagic layer where sunlight fades to nothing but life persists in darkness. The electric eel navigates this zone via electrosensory organs — not sight, but field detection. Sound in OXBOW behaves the same way: it is not heard so much as sensed, felt in the stereo field as presence rather than definition.

The Eel is Oscar-dominant. OXBOW rewards patience, long listening, and willingness to let sound dissolve. The exciter fires, the river rushes in, and then the lake takes over.

---

## 2. Signal Flow Diagram

```
MIDI NOTE-ON
     │
     ├─ midiToFreq → updateGoldenFrequencies (sets peak filter tunings)
     │
     ▼
┌─────────────────────────────────────┐
│  EXCITER (pitched impulse + noise)  │
│                                     │
│  sine osc (MIDI pitch)              │
│  + noise burst × Exciter Bright     │
│  × exciter envelope (Exciter Decay) │
│  × velocity scale (D001)            │
└──────────────────┬──────────────────┘
                   │
                   ▼
         ┌─────────────────┐
         │   PRE-DELAY     │  0–200ms buffer
         │  (0–200ms)      │
         └────────┬────────┘
                  │
         ┌────────▼────────────────────────────────────────┐
         │        8-CHANNEL CHIASMUS FDN                   │
         │                                                  │
         │  CH1–4 (LEFT branch)   CH5–8 (RIGHT branch)     │
         │  delays: 28/38/46/56ms   delays: 56/46/38/28ms  │
         │       └──────────────────────────┘              │
         │           Chiasmus mirror topology              │
         │                                                  │
         │  Householder matrix: H[i][j] = i==j ? 0.75 : -0.25  │
         │                                                  │
         │  Entanglement: cross-blend CH1–4 ↔ CH5–8        │
         │                                                  │
         │  Cantilever damping: bright early, dark late     │
         │  (CytomicSVF LP per channel, cutoff drops as    │
         │   energy falls — quadratic darkening)           │
         │                                                  │
         │  Feedback coeff from Decay Time                  │
         │  (pDecay > 29s → feedback = 1.0 = infinite)     │
         └───────────────────┬─────────────────────────────┘
                             │
                    fdnL = CH1+2+3+4 × 0.25
                    fdnR = CH5+6+7+8 × 0.25
                             │
         ┌───────────────────▼─────────────────────────────┐
         │            PHASE EROSION                         │
         │                                                  │
         │  4 allpass filters per side (8 total)            │
         │  Tuned to: 300 / 1100 / 3200 / 7500 Hz          │
         │                                                  │
         │  LFO rates: 0.03 / 0.05 / 0.07 / 0.09 Hz        │
         │  (slow, staggered sine LFOs)                     │
         │                                                  │
         │  L side: freq = base × (1 + LFO × depth)        │
         │  R side: freq = base × (1 - LFO × depth)  ◄── OPPOSITE polarity  │
         │                                                  │
         │  → Breathing spectral self-cancellation in mono  │
         └───────────────────┬─────────────────────────────┘
                             │
                  erosionL, erosionR
                             │
         ┌───────────────────▼─────────────────────────────┐
         │          GOLDEN RESONANCE                        │
         │                                                  │
         │  Mid = (L+R) × 0.5                               │
         │  Side = (L-R) × 0.5                              │
         │                                                  │
         │  convergence = midEnvelope / sideEnvelope        │
         │                                                  │
         │  if convergence > Convergence threshold:         │
         │     resonanceGain attacks toward 1.0             │
         │  else:                                           │
         │     resonanceGain decays toward 0.0              │
         │                                                  │
         │  4 Peak filters (CytomicSVF):                    │
         │  f₁ = MIDI fundamental                           │
         │  f₂ = f₁ × φ   (φ = 1.618...)                   │
         │  f₃ = f₁ × φ²                                    │
         │  f₄ = f₁ × φ³                                    │
         │                                                  │
         │  Gains: 0dB / -3dB / -6dB / -9dB (Tomita -3dB/φ) │
         │  × resonanceGain × Resonance Mix                 │
         └───────────────────┬─────────────────────────────┘
                             │
                  wetL = erosionL + goldenOutL
                  wetR = erosionR + goldenOutR
                             │
         ┌───────────────────▼─────────────────────────────┐
         │  Ring Mod coupling (if AudioToRing active)       │
         └───────────────────┬─────────────────────────────┘
                             │
                  finalL = dry × (1-DryWet) + wetL × DryWet
                  finalR = dry × (1-DryWet) + wetR × DryWet
                             │
                         OUTPUT
```

---

## 3. Every Parameter Explained

### EXCITER GROUP
The exciter is OXBOW's internal sound source — a pitched impulse generator that fires on each MIDI note-on. Unlike an oscillator, it produces a short burst that decays, feeds the FDN, and then falls silent. The reverb tail outlives the source by design.

---

**`oxb_exciterDecay` — Exciter Decay**
Range: 0.001–0.1s | Default: 0.01s | Curve: 0.5 (exponential toward short end)

How long the exciter burst lasts before the FDN takes over completely. At minimum (0.001s), the exciter fires as a near-instant click — entirely dry-side is transient energy. At maximum (0.1s), the exciter sings for 100ms, contributing a pitched "sting" before handing off to the reverb. Low values create metallic, percussive textures. Higher values create more pitched, tonal attacks.

*Musical context:* Setting exciter decay to 0.001–0.005s with high brightness produces a sharp metallic strike — useful for percussion or foley. Setting it to 0.04–0.1s with low brightness creates a soft mallet attack that blends into the reverb, useful for pad-like textures.

*Velocity scaling:* exciter length scales with velocity (Kakehashi doctrine). Soft notes give shorter, quieter attacks; hard notes give longer, brighter attacks.

---

**`oxb_exciterBright` — Exciter Bright**
Range: 0.0–1.0 | Default: 0.7

Controls the noise-to-sine ratio in the exciter. At 0.0 the exciter is a pure sine wave at the MIDI fundamental. At 1.0 the exciter is dominated by white noise with the sine barely present. The transition region (0.4–0.7) produces the most useful "plucked string" or "struck metal" character — pitched transient with spectral excitement.

*Musical context:* Low values (0.0–0.3) produce warm, round attacks that disappear quickly into reverb — useful for ambient, New Age, or Eno-style pads. High values (0.7–1.0) produce bright, aggressive metallic attacks — useful for industrial sound design, cinematic hits, or dub siren tones. Maximum brightness with minimum decay produces OXBOW's most percussive palette.

*Velocity scaling:* brightness also scales with velocity — ghost touches yield mellow attacks while hard hits cut through with air and edge.

---

### FDN GROUP

**`oxb_size` — Space Size**
Range: 0.0–1.0 | Default: 0.5

Scales the FDN delay times, controlling the perceived acoustic size of the resonant space. This parameter is applied at `prepare()` time — changes take effect when the engine reinitializes. The base delay structure ranges from approximately 28–56ms per channel at 48kHz. At size 0.0, the space collapses to a tiny plate or spring character. At size 1.0, the FDN reads like a vast architectural space.

*Musical context:* Small values (0.0–0.2) produce intimate textures — body cavity resonances (Heartbeat Close), room acoustics, close-miked spaces. Large values (0.6–1.0) produce hall, cathedral, and spatial ambience (Pedal Ocean, Velvet Crush). Mid values (0.3–0.6) cover studio rooms, small venues, and the most "classic reverb" territory.

---

**`oxb_decay` — Decay Time**
Range: 0.1–60s | Default: 4s | Curve: 0.3 (exponential toward longer times)

Controls the feedback coefficient of the FDN. At any value above 29s, feedback reaches 1.0 — the reverb sustains forever (Schulze infinite decay). The curve is heavily exponential, meaning most practical values cluster in the 0.1–10s range with the upper end reserved for ambient, infinite-sustain work.

The relationship between decay and feedback is calculated as:
`feedbackCoeff = exp(-6.9078 / (decay × sampleRate))`

This means a true RT60 relationship — the reverb loses 60dB of energy over the declared decay time.

*Musical context:* Short decay (0.1–1s): tight, close acoustic spaces, percussion, dub hits. Medium decay (2–8s): classic reverb territory, pads, vocals. Long decay (10–20s): vast ambience, ambient music, sustained pads. Infinite (>29s): the reverb never dies — combine with automation to build infinite washes.

*The Schulze mode:* Set decay above 30s and the Oxbow lake becomes a standing body of water that never drains. Every MIDI note you play adds to an accumulating pool. This is the engine's most distinctive behavior.

---

**`oxb_entangle` — Entanglement**
Range: 0.0–1.0 | Default: 0.6

Cross-blends the Left FDN channels (1–4) with the Right FDN channels (5–8). At 0.0, channels are independent — the chiasmus topology is fully preserved and stereo separation is maximum. At 1.0, channels are fully blended — L and R become identical, collapsing to mono. The useful range is 0.0–0.95, which was the sweet spot Vangelis favored for aftertouch expression.

*The math:* `entangleMix = entangle × 0.3` (subtlety cap). At parameter value 0.95, actual blend is 28.5% — substantial but not complete collapse.

*Aftertouch behavior:* Aftertouch adds up to +0.3 to entanglement, creating a maximum of 1.0. Playing notes normally keeps the field wide; pressing harder draws L and R together. This is one of the engine's primary live expression paths.

*Musical context:* Low entanglement (0.0–0.2): maximum stereo separation, M/S content will phase-cancel heavily when summed to mono — experimental, spatial, designed for stereo listening. High entanglement (0.7–0.95): the chiasmus topology loses definition, the stereo field becomes smeared and dense — useful for wall-of-sound textures. Medium entanglement (0.4–0.7): the default sweet spot, enough L/R interaction to feel alive without losing stereo.

---

**`oxb_damping` — Damping**
Range: 200–16000 Hz | Default: 6000 Hz | Curve: 0.3 (exponential toward lower frequencies)

Sets the cutoff frequency of the CytomicSVF lowpass filters applied to each FDN channel after the Householder feedback matrix. This controls the spectral character of the reverb tail in isolation from the cantilever darkening effect.

*Moog lineage:* These filters use the matched-Z SVF design specifically to avoid the "boxiness" of Euler approximation. The transition is smooth and musical even at extreme settings.

*Musical context:* Low damping (200–1500 Hz): dark, submerged, pressure-heavy reverb — submarine acoustics, cave ambience, the deep. Mid damping (3000–6000 Hz): balanced natural reverb character, present but not harsh. High damping (8000–16000 Hz): bright, metallic reverb — springs, plates, iron cathedrals. Maximum damping with high resonance mix produces the signature industrial sound of "Iron Cathedral" and "Foundry Strike."

---

**`oxb_cantilever` — Cantilever**
Range: 0.0–1.0 | Default: 0.3 (Pearlman preferred)

Controls the depth of the Asymmetric Cantilever Decay effect — the engine's most distinctive and musically unique behavior. As the FDN's energy drops from its peak, damping progressively descends, darkening the reverb tail. The rate of descent is quadratic — it accelerates as energy depletes.

*The physics:* `cantileverDamp = damping × (1 - cantilever × decayProgress²)`. At cantilever 0.0, damping is constant throughout the tail. At cantilever 1.0, the tail can darken to near-DC as energy approaches zero.

*The music:* A bright initial reflection that transforms into a dark, subterranean rumble. The reverb literally changes character as it ages. High cantilever values (0.7+) create sounds that start metallic and end like thunder — dramatically asymmetric decay arcs. This is one of OXBOW's key differentiators from conventional reverb algorithms.

*Musical context:* Low (0.0–0.2): uniform character throughout the tail — predictable, conventional. Medium (0.3–0.5): subtle transformation, the Pearlman sweet spot — enough character without excess drama. High (0.6–0.85): dramatic arcs, cinematic builds, the "Dried Riverbed" and "Storm Approach" territory where the sound weathers as it decays.

---

**`oxb_predelay` — Pre-Delay**
Range: 0–200ms | Default: 20ms | Curve: 0.5

Delays the exciter signal before it enters the FDN. The classic reverb tool for creating separation between direct sound and reverb onset, allowing transients to articulate clearly before the tail begins. OXBOW extends the usable range to 200ms — significantly longer than most reverbs.

*Musical context:* 0ms: direct, immediate — the attack and reverb begin simultaneously. 10–30ms: natural room feel — small to mid spaces. 50–100ms: cinematic or large hall territory — transient clearly separated. 100–200ms: rhythmic pre-delay territory — the gap becomes a compositional element in dub, ambient, and electronic contexts. At 100ms with 120 BPM, the pre-delay gap is exactly one 16th note.

---

### PHASE EROSION GROUP

**`oxb_erosionRate` — Erosion Rate**
Range: 0.01–0.5 Hz | Default: 0.08 Hz | Curve: 0.5

Sets the base rate of the four allpass filter LFOs that implement phase erosion. These LFOs run at staggered rates — LFO 1 is at `erosionRate`, LFO 2 at `erosionRate + 0.01`, LFO 3 at `erosionRate + 0.02`, LFO 4 at `erosionRate + 0.03`. The staggering ensures the four bands never align phase, creating a continuously shifting, never-repeating spectral texture.

*Perceptually:* Slow rates (0.01–0.1 Hz) produce barely perceptible "breathing" — the reverb tail subtly shifts character on a 10–100 second cycle. Fast rates (0.3–0.5 Hz) produce audible warble and pitch deviation — the allpass modulation becomes a musical effect rather than background texture.

*Musical context:* Slow rates: ambient, meditative, invisible movement — the tail lives and breathes without calling attention to the mechanism. Medium rates (0.1–0.2 Hz): gentle modulation compatible with tonal music. Fast rates (0.3+): VHS tracking, pitch instability, dub weirdness — the erosion becomes a feature rather than a texture.

---

**`oxb_erosionDepth` — Erosion Depth**
Range: 0.0–1.0 | Default: 0.4

Controls how far the allpass filter frequencies deviate from their center values with each LFO cycle. The effective modulation depth is `depth × 0.4`, applied as a frequency ratio. At depth 0.0, the allpass filters are fixed — no phase erosion occurs, and the stereo field is stable. At depth 1.0, the effective modulation is ±40% of the center frequency — substantial spectral shifting.

*Interaction with Erosion Rate:* Rate controls the speed of erasure, depth controls the magnitude. A slow rate with high depth creates rare but dramatic spectral events. A fast rate with low depth creates constant but subtle flutter. A slow rate with low depth is nearly inaudible. A fast rate with high depth is maximum instability.

*The mono collapse behavior:* Because L and R allpass filters are modulated with opposite polarity, increasing erosion depth increases the L/R difference. Summing to mono will partially cancel this difference, creating a more complex relationship between stereo and mono than any simple width control.

*Musical context:* Zero depth: conventional static-character reverb. Low depth (0.1–0.3): "alive" reverb that never quite repeats — recommended for ambient and long-sustain patches. Medium depth (0.4–0.6): perceptible movement, useful for dub, shoegaze, and cinematic contexts. High depth (0.7–1.0): heavy spectral shifting — experimental, lo-fi, industrial, glitch.

---

### GOLDEN RESONANCE GROUP

**`oxb_convergence` — Convergence**
Range: 1.0–20.0 | Default: 4.0 | Curve: 0.5

Sets the Mid/Side energy ratio threshold that triggers the golden resonance peak filters. Convergence is computed as `midEnvelope / sideEnvelope`. When this ratio exceeds the Convergence threshold, the four peak filters begin attacking toward full gain. When it falls below, they release.

*What this means perceptually:* When the FDN energy in the center of the stereo field exceeds the energy in the sides (relative to the threshold), the golden filters wake up. This happens naturally when the L and R channels are temporally coherent — late in the reverb tail when the Chiasmus topology is averaging out its reflections, or when entanglement is high enough to bring L and R together.

*Convergence as a sensitivity control:* Low values (1.0–3.0) make the resonance easy to trigger — even moderate mid-side balance will fire the filters. High values (10.0–20.0) make the resonance rare and precious — it only emerges when the stereo field is nearly mono. This creates a fascinating compositional behavior: the golden harmonics appear and disappear based on what notes are played, how hard, and over what duration.

*Musical context:* Low convergence: golden resonance rings frequently, adding harmonic shimmer to nearly every note — more present, more colored. High convergence: the resonance is a reward for sustained, centered energy — gates it to appear only in moments of stillness. For ambient and meditative contexts, high convergence (8.0–15.0) creates moments of golden clarity that emerge from the wash.

---

**`oxb_resonanceQ` — Resonance Focus**
Range: 0.5–20.0 | Default: 8.0 | Curve: 0.4

Controls the Q (bandwidth) of the four golden ratio peak filters. At low Q values (0.5–2.0), the peak filters are broad and diffuse — adding warmth and harmonic color without distinct ringing. At high Q values (10.0–20.0), the peaks are narrow and focused — producing audible, pitched ringing at each golden harmonic.

*What you hear:* High Q with high Resonance Mix creates bell-like overtones that ring at the MIDI fundamental and its golden ratio harmonics (f, f×1.618, f×2.618, f×4.236). These are *not* musical intervals — they are inharmonic relative to the equal temperament scale, producing the "just slightly wrong" ring that gives metallic percussion its character. They are, however, mathematically elegant, which is why Tomita weighted them at -3dB per multiple.

*Musical context:* Low Q (0.5–3.0): warm resonance, more like body resonance or speaker cabinet coloration than discrete ringing. High Q (12.0–20.0): percussive metallic character — bells, gongs, struck metal, Bunraku percussion. Q values of 6.0–10.0 are the musical sweet spot for tonal applications where you want harmonic reinforcement without overt metallic character.

---

**`oxb_resonanceMix` — Resonance Mix**
Range: 0.0–1.0 | Default: 0.3

The output level of the four peak filters when they are fully triggered. The actual amplitude of each filter output is `resonanceGain × resonanceMix × goldenGains[g]` where goldenGains are the Tomita -3dB-per-harmonic weightings: `[1.0, 0.708, 0.501, 0.354]`.

*Musical context:* Low values (0.0–0.2): subtle, supportive resonance — the golden harmonics add presence without declaring themselves. Medium values (0.3–0.5): the resonance is audible and musical — the character of the engine becomes more pronounced. High values (0.6–1.0): the golden filters dominate — metallic, bell-like, industrial. Combined with high Q, maximum mix produces the sonic signature of "Welding Arc," "Iron Cathedral," and "Resonant Cage."

---

### DRY/WET AND ROUTING

**`oxb_dryWet` — Dry/Wet**
Range: 0.0–1.0 | Default: 0.5

The blend between the dry exciter signal and the wet reverb tail. At 0.0, only the pitched exciter burst is heard (essentially a direct synthesis tone with no reverb). At 1.0, only the wet FDN output is heard — the dry attack disappears entirely. The exciter counts as "dry" for this balance.

*Practical guidance:* For most musical applications, values of 0.45–0.65 produce the best balance between attack definition and spatial depth. Higher values (0.7–0.85) suit ambient and pad textures where the reverb IS the content. For coupling scenarios where OXBOW is processing another engine's output, maximum wet (1.0) with minimum exciter activity is the correct configuration.

---

## 4. Sound Design Recipes

### Recipe 1: Dub Siren Wash
*Genre: Dub, Reggae, Electronic — reference: King Tubby, Lee Perry*

The classic dub technique — a pitched siren filtered through an ocean of reverb space, shedding its melody and becoming pure texture.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.4 | Medium room — not a hall, a dub studio |
| `oxb_decay` | 4.0s | Long enough to wash but not infinite |
| `oxb_entangle` | 0.6 | Moderate stereo coupling |
| `oxb_erosionRate` | 0.15 | Audible siren-like pitch drift |
| `oxb_erosionDepth` | 0.5 | Substantial deviation for the warble effect |
| `oxb_convergence` | 4.0 | Golden resonance fires on sustained tones |
| `oxb_resonanceQ` | 8.0 | Focused but not overly sharp |
| `oxb_resonanceMix` | 0.3 | Supportive shimmer |
| `oxb_cantilever` | 0.3 | Mild asymmetric decay |
| `oxb_damping` | 4000 Hz | Warm, slightly muffled |
| `oxb_predelay` | 30ms | Classic dub headroom before echo |
| `oxb_dryWet` | 0.5 | Equal dry/wet |
| `oxb_exciterDecay` | 0.01s | Tight attack |
| `oxb_exciterBright` | 0.55 | Slightly noisy — adds air |

*Technique:* Play slow chromatic intervals. Let each note sustain for 4–8 bars before moving. The erosion will transform the pitch, the golden resonance will bloom in the sustained center, and the FDN will accumulate. Automate `oxb_decay` from 4s to 30s over 8 bars for a classic dub build.

---

### Recipe 2: Iron Cathedral
*Genre: Industrial, Cinematic — reference: Einstürzende Neubauten, Hans Zimmer*

Vast metallic reverb where every strike produces harmonics that ring like girders in a steel nave. Maximum brightness, maximum resonance.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.6 | Large but not infinite |
| `oxb_decay` | 3.0s | Long enough for drama, controlled |
| `oxb_entangle` | 0.6 | Substantial coupling |
| `oxb_erosionRate` | 0.18 | Some modulation for aliveness |
| `oxb_erosionDepth` | 0.5 | Medium deviation |
| `oxb_convergence` | 2.0 | Resonance fires easily — almost always active |
| `oxb_resonanceQ` | 18.0 | Very narrow — distinct metallic ring |
| `oxb_resonanceMix` | 0.6 | Resonance is dominant |
| `oxb_cantilever` | 0.3 | Moderate darkening |
| `oxb_damping` | 14000 Hz | Very bright — metallic |
| `oxb_predelay` | 8ms | Tight, punchy |
| `oxb_dryWet` | 0.55 | Mostly wet |
| `oxb_exciterDecay` | 0.003s | Ultra-short — a click |
| `oxb_exciterBright` | 0.95 | Maximum noise — white hot |

*Technique:* Use on low MIDI notes (C2–C3). The golden ratio harmonics at these fundamentals produce metallic partials in the 300–1500 Hz range — exactly where the human ear registers "metal." Hard strikes (velocity 100+) ignite the full harmonic cloud. Play sparse single notes with long rests and let the cathedral fill with decaying overtones.

---

### Recipe 3: Deep Ambient Pool
*Genre: Ambient, New Age — reference: Brian Eno, Harold Budd, Klaus Schulze*

The oxbow lake at maximum stillness. Infinite decay, minimal erosion, golden harmonics as rare moments of emergence.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.8 | Large spatial impression |
| `oxb_decay` | 35.0s | Above the 29s threshold — infinite feedback |
| `oxb_entangle` | 0.5 | Moderate coupling |
| `oxb_erosionRate` | 0.03 | Barely perceptible drift |
| `oxb_erosionDepth` | 0.2 | Subtle depth — "alive" not "warbling" |
| `oxb_convergence` | 8.0 | High threshold — resonance is rare |
| `oxb_resonanceQ` | 5.0 | Broad and diffuse |
| `oxb_resonanceMix` | 0.3 | Supportive, not dominant |
| `oxb_cantilever` | 0.15 | Mild asymmetry — nearly linear |
| `oxb_damping` | 7000 Hz | Moderately bright, present |
| `oxb_predelay` | 40ms | Creates space between attack and bloom |
| `oxb_dryWet` | 0.75 | Mostly wet — the reverb is the content |
| `oxb_exciterDecay` | 0.02s | Short, soft attack |
| `oxb_exciterBright` | 0.3 | Warm, round — minimal noise |

*Technique:* Use sustain pedal philosophy — trigger notes and release. The infinite feedback means the lake accumulates every note you've played. Sparse, widely-spaced notes (one every 8–16 bars at 60 BPM) allow the pool to develop richly without overloading. The golden resonance will emerge in the prolonged silences. Automate `oxb_dryWet` from 0.75 down to 0.0 to gradually submerge the dry signal into pure reverb.

---

### Recipe 4: Shoegaze Wall
*Genre: Shoegaze, Dream Pop — reference: My Bloody Valentine, Slowdive, Cocteau Twins*

Dense, velvet reverb with heavy entanglement. Guitars become clouds. Vocals become ghosts. The dry signal is still present but barely visible through the wash.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.85 | Vast — cathedral-sized |
| `oxb_decay` | 18.0s | Very long — sustains through entire phrases |
| `oxb_entangle` | 0.7 | Heavy coupling — stereo field smears |
| `oxb_erosionRate` | 0.08 | Slow, invisible movement |
| `oxb_erosionDepth` | 0.3 | Moderate depth |
| `oxb_convergence` | 3.0 | Easy trigger — the wall is always harmonically active |
| `oxb_resonanceQ` | 3.0 | Broad and diffuse — adds density not ringing |
| `oxb_resonanceMix` | 0.12 | Low mix — supportive, subliminal |
| `oxb_cantilever` | 0.2 | Mild — the wall should be consistent |
| `oxb_damping` | 5500 Hz | Warm-bright balance |
| `oxb_predelay` | 15ms | Just enough separation for attack clarity |
| `oxb_dryWet` | 0.7 | The reverb IS the sound |
| `oxb_exciterDecay` | 0.01s | Tight — don't compete with the wash |
| `oxb_exciterBright` | 0.45 | Moderate — neutral not aggressive |

*Technique:* Stack multiple notes (chord voicings) rather than single lines. The Chiasmus topology handles complex inputs beautifully — each new note adds a fresh burst of energy to the accumulated pool. High entanglement with moderate erosion creates the distinctive "crushed stereo" quality where L and R sound like a single wide entity rather than two discrete channels.

---

### Recipe 5: Phase Ghost (Experimental Erasing)
*Genre: Experimental, Glitch, Sound Design*

Maximum phase erosion causes the reverb tail to partially erase itself — fleeting moments of golden resonance emerge and vanish from a wash of self-cancelling echoes. Check in stereo, then collapse to mono for a completely different experience.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.55 | Medium-large |
| `oxb_decay` | 5.5s | Long tail gives erasure time to operate |
| `oxb_entangle` | 0.92 | Near-maximum — L and R nearly unified |
| `oxb_erosionRate` | 0.35 | Fast modulation — dramatic spectral shifting |
| `oxb_erosionDepth` | 0.95 | Near-maximum deviation — heavy cancellation |
| `oxb_convergence` | 4.0 | Moderate threshold |
| `oxb_resonanceQ` | 8.0 | Focused peaks for emergence moments |
| `oxb_resonanceMix` | 0.35 | Noticeable when triggered |
| `oxb_cantilever` | 0.2 | Low — keep character consistent for observation |
| `oxb_damping` | 6000 Hz | Neutral |
| `oxb_predelay` | 15ms | Standard |
| `oxb_dryWet` | 0.6 | Mostly wet — the effect is the point |
| `oxb_exciterDecay` | 0.02s | Short |
| `oxb_exciterBright` | 0.5 | Neutral |

*Technique:* Play single notes, record the stereo output, and compare against a mono fold. The stereo version will have a complex, shifting wash where fragments of the original pitch appear and vanish. The mono version will have different moments of presence and absence — where stereo has content, mono may have silence, and vice versa. This is the Chiasmus topology made audible as composition.

---

### Recipe 6: Intimate Heartbeat Space
*Genre: Film scoring, ASMR, lo-fi, Organic — reference: Jon Hopkins, Nils Frahm*

The smallest possible reverb — the acoustic space of a single chest cavity, intimate and internal. All the engineering is hidden in service of warmth.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.08 | Tiny — a body, not a room |
| `oxb_decay` | 0.9s | Short — the body absorbs quickly |
| `oxb_entangle` | 0.12 | Low — wide but intimate |
| `oxb_erosionRate` | 0.06 | Barely moving |
| `oxb_erosionDepth` | 0.1 | Almost no deviation |
| `oxb_convergence` | 2.0 | Easy trigger at this small size |
| `oxb_resonanceQ` | 3.0 | Broad — body resonance not metallic |
| `oxb_resonanceMix` | 0.1 | Subliminal warmth |
| `oxb_cantilever` | 0.2 | Mild asymmetry |
| `oxb_damping` | 800 Hz | Very dark — internal, muffled |
| `oxb_predelay` | 0ms | No delay — immediate |
| `oxb_dryWet` | 0.22 | Mostly dry — the sound should be the point |
| `oxb_exciterDecay` | 0.042s | Medium length — a soft breath |
| `oxb_exciterBright` | 0.12 | Very dark — almost no noise |

*Technique:* Play on middle C (C4) and nearby notes. The very low damping cutoff at 800Hz means almost all mid and high frequencies are absorbed — what survives is only the sub-bass resonance of the FDN, which has a distinctly internal, somatic quality. Layer with a conventional instrument and set dry/wet around 0.2 — the room becomes the body of the instrument rather than the space surrounding it.

---

### Recipe 7: Dried Riverbed (Cantilever Drama)
*Genre: Cinematic, Sound Design, Electronic*

Maximum cantilever combined with aggressive erosion — the reverb transforms from crystalline to barren over its lifetime. Play a note and listen as the tail weathers from bright to dark.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.45 | Medium size |
| `oxb_decay` | 5.0s | Long enough for the transformation to complete |
| `oxb_entangle` | 0.6 | Moderate |
| `oxb_erosionRate` | 0.25 | Fast — audible weathering |
| `oxb_erosionDepth` | 0.8 | High — dramatic spectral shifts |
| `oxb_convergence` | 4.0 | Standard |
| `oxb_resonanceQ` | 7.0 | Moderate ring |
| `oxb_resonanceMix` | 0.25 | Subtle gold in the opening moments |
| `oxb_cantilever` | 0.85 | Near-maximum — severe darkening |
| `oxb_damping` | 3000 Hz | Already somewhat dark at start |
| `oxb_predelay` | 15ms | Standard |
| `oxb_dryWet` | 0.55 | Mostly wet |
| `oxb_exciterDecay` | 0.015s | Short — the strike should be quick |
| `oxb_exciterBright` | 0.5 | Neutral attack |

*Technique:* Play a single note on a high MIDI note (C5–C6) and let it sustain without additional notes. At first, the reverb is bright — high energy means damping cutoff is near the configured value. As energy depletes quadratically, the cantilever drops the cutoff progressively lower. By 4–5 seconds, the tail is dark and rumbling — a completely different acoustic character from the opening moment. This temporal transformation is the signature OXBOW behavior and cannot be achieved with a static reverb.

---

### Recipe 8: Golden Bell (Tuned Resonance)
*Genre: Ambient, Gamelan, Meditation, Cinematic*

High Q golden resonance with low convergence threshold — every note produces a cloud of inharmonic bell-like overtones tuned to the golden ratio series.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.4 | Medium — bell-like, not cathedral |
| `oxb_decay` | 3.0s | Bell-like RT60 |
| `oxb_entangle` | 0.5 | Moderate |
| `oxb_erosionRate` | 0.1 | Slow — the bell should ring clearly |
| `oxb_erosionDepth` | 0.25 | Subtle movement |
| `oxb_convergence` | 2.0 | Low — resonance fires immediately |
| `oxb_resonanceQ` | 15.0 | Very narrow — distinct pitched rings |
| `oxb_resonanceMix` | 0.5 | Resonance is prominent |
| `oxb_cantilever` | 0.2 | Low — the bell should maintain character |
| `oxb_damping` | 9000 Hz | Bright — bells are bright |
| `oxb_predelay` | 5ms | Tight |
| `oxb_dryWet` | 0.6 | The resonance is the instrument |
| `oxb_exciterDecay` | 0.005s | Short click exciter |
| `oxb_exciterBright` | 0.8 | Bright — ignites the harmonics |

*Technique:* The golden ratio harmonics are at f, f×1.618, f×2.618, f×4.236. These are not octaves, fifths, or thirds — they are genuinely inharmonic. Play melodic lines and notice that each note generates a unique cloud of overtones that do not relate harmonically to adjacent notes. This creates a shimmering, Gamelan-like quality where melodic intervals produce unpredictable spectral intersections.

---

### Recipe 9: Cinematic Pre-Delay Tension
*Genre: Cinematic Scoring, Trailer music, Dramatic underscoring*

Long pre-delay separating the attack from the reverb onset — creates the distinctive "impact + bloom" structure of cinematic reverbs.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.75 | Large spatial impression |
| `oxb_decay` | 10.0s | Long — the bloom builds slowly |
| `oxb_entangle` | 0.55 | Moderate |
| `oxb_erosionRate` | 0.18 | Active modulation for cinematic motion |
| `oxb_erosionDepth` | 0.5 | Medium depth |
| `oxb_convergence` | 6.0 | Higher — resonance is a dramatic accent |
| `oxb_resonanceQ` | 5.0 | Broad resonance — atmospheric, not metallic |
| `oxb_resonanceMix` | 0.2 | Supportive |
| `oxb_cantilever` | 0.72 | High — the bloom darkens dramatically |
| `oxb_damping` | 3500 Hz | Darker — ominous quality |
| `oxb_predelay` | 80ms | Long gap — hear the attack clearly first |
| `oxb_dryWet` | 0.55 | Balance for impact |
| `oxb_exciterDecay` | 0.02s | Defined attack |
| `oxb_exciterBright` | 0.28 | Dark, controlled |

*Technique:* At 80ms pre-delay, you hear the exciter strike clearly, then 80ms of silence, then the reverb bloom emerges as a separate event. This gap creates dramatic tension — it sounds like a strike heard across a vast space. With the cantilever at 0.72 and damping at 3500Hz, the bloom starts slightly bright and grows progressively darker over the 10-second tail. Ideal for hits, accents, and tension markers in film scores.

---

### Recipe 10: Schulze Infinite Wash
*Genre: Kosmische Musik, Minimal Techno, Generative Ambient*

Decay set above 29s activates infinite feedback. Each note permanently adds to the resonating pool. The Oxbow lake never drains.

| Parameter | Value | Why |
|-----------|-------|-----|
| `oxb_size` | 0.65 | Large but defined |
| `oxb_decay` | 45.0s | Well above 29s threshold — fully infinite |
| `oxb_entangle` | 0.7 | Heavy coupling for wash density |
| `oxb_erosionRate` | 0.05 | Very slow drift |
| `oxb_erosionDepth` | 0.3 | Moderate — prevents exact repetition |
| `oxb_convergence` | 5.0 | Moderate — resonance emerges in sustained moments |
| `oxb_resonanceQ` | 4.0 | Broad — harmonics blend into the wash |
| `oxb_resonanceMix` | 0.25 | Subtle shimmer |
| `oxb_cantilever` | 0.2 | Low — consistent tail character |
| `oxb_damping` | 6000 Hz | Neutral-bright |
| `oxb_predelay` | 20ms | Standard |
| `oxb_dryWet` | 0.65 | Mostly wet — the accumulating pool is the content |
| `oxb_exciterDecay` | 0.012s | Short, punctual |
| `oxb_exciterBright` | 0.4 | Moderate |

*Technique:* This patch should be used as a compositional structure over many minutes. Each note played adds permanently to the FDN resonance. Begin with single low notes (C2–C3) separated by long rests. Progressively add higher notes and tighter intervals as the session develops. After 10–15 minutes of sparse playing, the engine will contain a complex, evolving harmonic structure assembled note by note. Use this as a generative ambient environment or as a slow-build within a set.

**Critical warning:** In infinite mode, DO NOT play fast passages or dense chords — the FDN will accumulate energy until it saturates. This is the correct behavior for slow, meditative performance. For safety, automate `oxb_decay` back below 29s to let the lake begin draining before the next section.

---

## 5. Coupling Recommendations

OXBOW supports three incoming coupling types: `AmpToFilter`, `EnvToDecay`, and `AudioToRing`.

### AmpToFilter (Source Amplitude → OXBOW Damping)
The source engine's amplitude is mapped to OXBOW's damping cutoff, shifting it by up to ±4000 Hz. This creates a dynamic where louder source material opens the reverb's frequency response and quieter material darkens it.

**Best source engines:**
- **ONSET** — percussive amplitude spikes from drum hits create bright, metallic reverb flash on each strike, darkening between hits
- **OBRIX** — bricks with distinct amplitude envelopes create rhythmically synced brightness modulation in the reverb
- **ORBITAL** — sustained pad amplitude creates a slow, breathing brightness arc that mirrors the pad's own volume contour
- **OPENSKY** — shimmer source amplitude can drive very bright damping values (up to 16000 Hz), creating a combined shimmer + metallic resonance character

### EnvToDecay (Source Envelope → OXBOW Decay Time)
The source engine's envelope scales OXBOW's decay time by up to ±10 seconds. Notes with long envelopes push the reverb toward longer decay; shorter envelopes tighten it.

**Best source engines:**
- **OVERBITE** (OVERBITE's Bite Depth envelope → OXBOW decay): envelope-reactive reverb length that responds to the attack articulation
- **ODYSSEY** — the dual-oscillator envelope creates rhythmic decay variation that animates the reverb over time
- **ORGANISM** — cellular automata patterns create irregular, generative decay variations — the reverb's length is determined by emergence

### AudioToRing (Source Audio × OXBOW Wet Output)
This is OXBOW's most transformative incoming coupling — the source engine's raw audio signal is multiplied against OXBOW's wet output, creating ring modulation between the incoming signal and the reverb tail.

**Best source engines:**
- **OVERLAP** → OXBOW AudioToRing: the Lion's Mane jellyfish's bioluminescent shimmer ring-modulates against the reverb tail, creating sideband ghost harmonics in the FDN
- **OBRIX** — brick synthesis waveforms ring-modulate the reverb, adding non-linear overtones to the golden resonance
- **OUROBOROS** — the chaotic strange attractor signal ring-modulates the FDN for unpredictable, evolving texture

### OXBOW as Source
OXBOW can feed other engines as a source for AmpToFilter, EnvToDecay, and AudioToRing.

**Recommended outgoing routes:**
- **OXBOW → OVERLAP (AmpToFilter):** OXBOW's reverb amplitude opens or closes OVERLAP's resonant space — the oxbow lake controlling the jellyfish's bioluminescence
- **OXBOW → ORGANON (AmpToFilter):** OXBOW's energy drives the metabolic rate of ORGANON's cellular processes — quiet reverb yields slow metabolism, energetic reverb accelerates it
- **OXBOW → OWARE (AudioToRing):** OXBOW's reverb tail ring-modulates OWARE's tuned percussion, adding spectral content to the mallet strikes from the accumulated lake resonance

### KnotTopology (Bidirectional)
OXBOW's Chiasmus FDN geometry makes it a natural partner for KnotTopology coupling with OVERLAP or ORBWEAVE. The irreducible mutual entanglement of KnotTopology aligns with OXBOW's own Chiasmus philosophy — both systems are built on structural mirroring. A OXBOW ↔ OVERLAP KnotTopology coupling creates a reverb system where the FDN and the jellyfish's bioluminescence co-evolve, neither engine being the master.

---

## 6. Genre Applications

### Ambient and Drone
OXBOW was designed with ambient music in mind. The Schulze infinite decay mode, the barely-perceptible erosion at low rates, and the golden resonance as a rare emergence event all point toward long-form, patient listening. Set large size, very long decay (>29s), low erosion depth, and low convergence threshold. Let single notes accumulate over many minutes. The Oxbow lake fills slowly.

**Key presets:** Pedal Ocean (Ethereal), Velvet Crush (Ethereal), Phase Ghost (Entangled), Chiasmus (Entangled)

**Workflow:** Use OXBOW as the final element in a signal chain — other engines generate material, OXBOW holds and transforms it. Set pre-delay to 60–100ms to maintain articulation in the lead element. The reverb should be inaudible as "reverb" and audible only as "space."

---

### Dub and Roots
Dub is one of OXBOW's most natural genre homes. The dub tradition treats reverb as a compositional element, not a finishing tool — echoes are mixed in and out, reverb is automated, spaces become instruments. OXBOW's erosion rate adds a distinctly analog wobble that suits the genre.

**Key presets:** Dub Siren Wash (Flux), Version Excursion (Flux), VHS Tracking (Flux)

**Workflow:** Automate `oxb_decay` between phrases — longer on drops, shorter on cuts. Use `oxb_predelay` at 30–80ms to create rhythm in the echo structure. High erosion rate (0.15–0.25) creates the characteristic dub pitch drift that makes reverb feel like a physical object being manipulated. Automate `oxb_entangle` for spatial effects — pulling it high smears the field, releasing it sharpens up.

---

### Cinematic and Trailer
OXBOW's cantilever system is uniquely suited to cinematic application. The asymmetric decay arc — bright opening, darkening tail — mirrors the classic cinematic sound design structure of impact followed by dissolution. Combined with long pre-delay, it produces a spatially complex event that contains both the strike and the bloom as distinct, sequenced sonic gestures.

**Key presets:** Storm Approach (Flux), Iron Cathedral (Flux), Dried Riverbed (Flux), Cantilever Collapse (Flux), Girder Flex (Flux)

**Workflow:** For impact hits, use short exciter decay (0.002–0.005s), maximum brightness, high Q resonance, and pre-delay of 10–25ms. The strike sounds physically massive because the high-Q golden filters ignite immediately on the transient. For dissolves, use high cantilever (0.7+), long decay (8–15s), and low damping (3000–5000 Hz). The reverb will be bright on the strike and dark by the end — the exact arc of cinematic emotion.

---

### Experimental and Glitch
OXBOW's Chiasmus topology creates mono/stereo relationships that don't exist in conventional reverb algorithms. Maximum phase erosion with high entanglement produces sounds that are unpredictable, unstable, and musically interesting in ways that cannot be designed by intent — only discovered by exploration. The stereo/mono relationship is compositionally exploitable.

**Key presets:** Phase Ghost (Entangled), Twilight Negation (Entangled), Eel Static (Flux), Welding Arc (Flux), Phantom Meander (Entangled)

**Workflow:** Record stereo output, then fold to mono and compare. Use the phase relationships as a compositional tool — certain notes and velocities will produce more mono survival (useful for bass, kick) while others will disappear in mono (useful for decorative elements that should exist only in stereo). Extreme erosion depth at fast rates (0.35–0.5 Hz, 0.8+ depth) creates crackling, unstable textures that respond differently to each new note — an effectively unique sound on every trigger.

---

## 7. Tips and Tricks

### The Mono Collapse Test
OXBOW's defining behavior emerges in the difference between stereo and mono. After designing a patch, always perform the mono collapse test: sum the stereo output to mono and listen to what survives. If the patch sounds identical in mono, you're not using the Chiasmus topology. If the mono version has unexpected silences, blooms, or a completely different character — you're using the engine correctly. The goal is not to avoid mono collapse but to make it interesting.

### Velocity as Timbre
Every MIDI note maps velocity to two exciter dimensions: decay length and brightness. Soft notes (velocity 1–40) produce short, dark attacks. Hard notes (velocity 100–127) produce longer, brighter attacks with more noise energy. This means the same OXBOW patch produces meaningfully different timbres across the velocity range. Expressive playing on a velocity-sensitive controller exploits the full character range. For programmed MIDI, use velocity automation rather than fixed velocity.

### Aftertouch for Live Expression
The engine maps aftertouch (both poly and channel pressure) to entanglement — applying up to +0.3 to the current `oxb_entangle` value. This is a live performance tool, not a static parameter. Starting a performance with low entanglement and gradually pressing harder draws the stereo field together over time — a gesture available to no conventional reverb unit. Use a keyboard with aftertouch capability to access this dimension.

### Convergence as a Gate
The Convergence parameter is essentially a gate on the golden resonance — when the Mid/Side ratio exceeds the threshold, the peak filters activate; when it drops below, they release. Rather than thinking of it as a filter quality setting, think of it as a sensitivity control for a very particular kind of musical event. High convergence values (12+) mean the golden resonance only appears in specific conditions: sustained notes, very high entanglement, or specific MIDI notes where the FDN naturally centers. These moments become compositionally precious and should be heard as features, not accidents.

### MIDI Note = Resonance Pitch
Every MIDI note-on recalculates the four golden ratio peak filter frequencies from the new MIDI fundamental. Playing C3 tunes the resonance to C3, f×φ, f×φ², f×φ³. Playing C4 retunes the entire resonance cloud an octave higher. This makes melody a direct controller of the golden resonance character. Melodic passages produce sweeping spectral transformations in the reverb tail — the FDN and the resonance are playing the same melody in overlapping time.

### Size Is an Init Parameter
`oxb_size` currently takes effect at engine initialization — real-time size changes require reinitializing the FDN delay lines. Use `oxb_size` for preset-level spatial character decisions rather than real-time automation. The other parameters — decay, entangle, erosion, convergence, resonance, cantilever, damping — are all real-time controllable and are the appropriate targets for performance modulation.

### Designing for the Tail, Not the Attack
Most synthesis designs optimize for the first 100ms — the attack and initial character. OXBOW rewards attention to the tail. Open the resonance mix high, set convergence low, use a long decay, and just listen to what happens after the first second. The golden harmonics emerge. The cantilever transforms the color. The erosion drifts the phase relationships. The most distinctive OXBOW patches are the ones where the first second is ordinary and the next five seconds are extraordinary.

### Coupling as Sound Design
`AmpToFilter` coupling from a percussive source creates rhythmic brightness modulation in the reverb — effectively a tremolo on the spectral character of the FDN rather than the amplitude. This is fundamentally different from any effect achievable within a single engine and worth exploring as a primary sound design approach rather than an enhancement. Pair ONSET (drums) → OXBOW with `AmpToFilter` at moderate intensity (0.2–0.4) for percussion that has a reverberant shadow that opens and closes with every hit.

### Surgical Use of Pre-Delay
Pre-delay is a time offset, not a blend. Setting pre-delay to 0ms blends attack and reverb onset together — appropriate for pads and ambient. Setting pre-delay to 80–120ms creates a 2-event structure: strike, then bloom. At 100ms pre-delay and 120 BPM, the delay gap is exactly one 16th note — rhythmically synchronized reverb onset. At 166ms (one dotted 8th at 120 BPM), pre-delay participates in the groove. These are not accidents — they are compositional choices.

### Infinite Mode: Use With Intention
When `oxb_decay` exceeds 29 seconds, feedback coefficient reaches exactly 1.0 and the reverb never decays. This is a powerful creative tool and a genuine engineering commitment. The energy in the FDN is conserved (minus the damping filters), meaning the lake will accumulate for the entire session. Budget sessions in infinite mode carefully — they build toward a sonic state, not a static effect. The reward for patience is a reverb that has months of history in it. The risk is that you accumulate too much energy too fast and saturate. Start sparse, go slow, and embrace the accumulated pool as the composition.

---

## Appendix: Default Values Quick Reference

| Parameter | ID | Default | Range |
|-----------|-----|---------|-------|
| Space Size | `oxb_size` | 0.5 | 0.0–1.0 |
| Decay Time | `oxb_decay` | 4.0s | 0.1–60s |
| Entanglement | `oxb_entangle` | 0.6 | 0.0–1.0 |
| Erosion Rate | `oxb_erosionRate` | 0.08 Hz | 0.01–0.5 Hz |
| Erosion Depth | `oxb_erosionDepth` | 0.4 | 0.0–1.0 |
| Convergence | `oxb_convergence` | 4.0 | 1.0–20.0 |
| Resonance Focus | `oxb_resonanceQ` | 8.0 | 0.5–20.0 |
| Resonance Mix | `oxb_resonanceMix` | 0.3 | 0.0–1.0 |
| Cantilever | `oxb_cantilever` | 0.3 | 0.0–1.0 |
| Damping | `oxb_damping` | 6000 Hz | 200–16000 Hz |
| Pre-Delay | `oxb_predelay` | 20ms | 0–200ms |
| Dry/Wet | `oxb_dryWet` | 0.5 | 0.0–1.0 |
| Exciter Decay | `oxb_exciterDecay` | 0.01s | 0.001–0.1s |
| Exciter Bright | `oxb_exciterBright` | 0.7 | 0.0–1.0 |

**Polyphony:** 1 voice (monophonic reverb instrument)
**Incoming coupling:** AmpToFilter, EnvToDecay, AudioToRing, AudioToBuffer
**Aftertouch target:** Entanglement (+0.3 max)
**Infinite decay threshold:** `oxb_decay` > 29s

---

*XO_OX Designs — OXBOW Sound Design Guide*
*Engine added: 2026-03-20*
