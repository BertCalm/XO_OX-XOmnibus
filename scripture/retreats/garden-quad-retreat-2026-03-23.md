# GARDEN Quad Retreat Chapter
*Guru Bin — 2026-03-23*
*Second-pass retreat: gap-filling across all four Garden engines*

---

## Quad Identity

The GARDEN quad models ecological succession in sound. Four engines. Four species. One shared soil.

| Engine | Accent | Role | DSP Core |
|--------|--------|------|----------|
| XOxalis | Clover Purple `#6A0DAD` | Pioneer | Phyllotaxis supersaw — 7 partials at golden ratio intervals |
| XOvergrow | Vine Green `#3A5F0B` | Early intermediate | Karplus-Strong solo string + RunnerGenerator |
| XOsier | Willow Green `#6B8E23` | Later intermediate | 4-role chamber quartet, 2 saws/voice, CompanionPlanting |
| XOrchard | Harvest Gold `#DAA520` | Climax species | 4 detuned sawtooth oscillators/voice, formant filter, seasonal arc |

All four engines share: GardenAccumulators (W/A/D state), GardenMycorrhizalNetwork (cross-voice stress propagation), SilenceGate (CPU zero-cost idle), ParameterSmoother, GlideProcessor, StandardLFO, FilterEnvelope, VoiceAllocator.

---

## Pre-Retreat Assessment

The first-pass retreats (2026-03-21) produced 10 awakening presets per engine and documented the core signal paths. The following gaps remain:

**XOrchard:**
- No film score orchestral string pad in the Cinematic/Deep mood gap
- Seasonal suite exists (Spring/Summer/Autumn/Winter) but no preset that *demonstrates the seasonal arc unfolding live over a session* in an instructional way
- No preset showing maximum Growth Mode (growthTime=60) as an extreme texture

**XOvergrow:**
- Silence-response behavior is architecturally central but no preset explicitly guides the user to *let silence accumulate* before striking
- Solo pizzicato character unexplored — damping/feedback/short-envelope territory not covered
- Runner generation (subharmonic spawning) not showcased in an intentional preset

**XOsier:**
- Cold chamber (D high, W low) unexplored — Winter Quartet
- Neo-soul string patch (short attack, bright, highly intimate) not present
- Growth mode with high companion strength showing the sequential entry of four distinct voices not present
- High-intimacy preset showing companion planting pulling voices together over time missing

**XOxalis:**
- Extreme phi=1.0 fully phyllotaxis preset (mathematical purity) missing
- Phi=0.0 pure supersaw character (baseline comparison) missing
- LFO-driven phi modulation (phi wobbles between standard and phyllotaxis over time) missing
- Mathematical precision preset showing symmetry=1.0 not present

---

# XOXALIS — "The Geometric Garden"

## DSP Understanding

XOxalis is the only engine in the GARDEN quad that does not use sawtooth synthesis for its core character. Instead, it uses a `PhyllotaxisOscBank` — seven partial oscillators whose frequency ratios are derived from the golden ratio (`phi = 1.618...`) instead of integer harmonics.

**The phi parameter** is the central character control:
- `phi=0.0`: standard harmonic series (1:2:3:4:5:6:7 ratios) — standard supersaw/string behavior
- `phi=0.5`: blend of standard harmonic and phyllotaxis ratios — string-adjacent with a subtle mathematical wrongness
- `phi=1.0`: full phyllotaxis (1:1.618:2.618:4.236:6.854:11.09:17.94 ratios) — inharmonic, crystalline, distinctly non-string

The `symmetry` parameter controls asymmetric waveshaping: below 0.9, `fastTanh(oscOut * 2.0f)` is blended in. Low symmetry adds harmonic saturation and organic unevenness. High symmetry preserves the mathematical purity of the phi ratios.

**Growth Mode** in XOxalis: partials emerge sequentially at golden angle intervals. Over `growthTime` seconds, partials 1 through 7 appear one at a time, each emerging at a golden angle position. This is phyllotaxis in time — the sunflower seed head growing outward, spiral by spiral.

**Pioneer identity**: XOxalis has the fastest accumulator rates (`wRiseRate=0.004`, `dDecayRate=0.015`) and the fastest Growth Mode default (10s vs 15s/20s for the other engines). It responds immediately to playing activity.

**Macro assignments:**
- CHARACTER (M1): sweeps phi +0.4 and cutoff +4000 Hz — moves from standard to phyllotaxis while opening the filter
- MOVEMENT (M2): adds up to +2 Hz to vibrato rate, sweeps LFOs
- COUPLING (M3): phi modulation via `couplingPhiMod` — external engines can modulate the golden ratio amount
- SPACE (M4): not directly wired in render path (no direct output routing) — use for LFO depth

## Parameter Refinements (12)

1. **oxal_phi** — default 0.5 → **0.4**. The 0.5 blend is slightly too inharmonic as a starting point for discovery. 0.4 keeps the string character while introducing just enough mathematical wrongness to be interesting. Players will immediately hear "this is different from a standard supersaw."

2. **oxal_symmetry** — default 0.5 → **0.65**. More mathematical precision on init. The asymmetric waveshaping at 0.5 adds welcome organic texture but muddles the phyllotaxis character. 0.65 preserves more of the precise partial relationships while still having some warmth.

3. **oxal_cutoff** — default 8000.0 → **7000.0**. The 8000 Hz cutoff is very open and can sound brittle on low notes. 7000 Hz gives a warmer foundation while still being bright enough for the pioneer role.

4. **oxal_vibratoDepth** — default 0.15 → **0.12**. Mathematical entities vibrate less organically than strings. Reducing the default vibrato depth emphasizes the geometric character over the expressive string character.

5. **oxal_spread** — default 0.5 → **0.6**. The phyllotaxis partials benefit from wider stereo spread — each partial occupying a slightly different position reinforces the multi-element, seed-head imagery.

6. **oxal_attack** — default 0.01 → **0.008**. Pioneer species arrive instantly. The existing near-zero attack is correct but a slightly more precise value (8ms) allows better high-velocity note shaping without the brutality of 1ms.

7. **oxal_release** — default 0.6 → **0.8**. Short release truncates the tail before the phyllotaxis character can fully express itself. 0.8s gives the inharmonic partials time to disperse beautifully.

8. **oxal_lfo1Depth** — default 0.1 → **0.12**. The LFO1 depth (→ filter cutoff via +l1*4000Hz) needs to be slightly more present to create movement without explicit user intervention. 0.12 introduces gentle filter breathing.

9. **oxal_filterEnvAmt** — default 0.3 → **0.25**. The filter envelope attack at 0.5× the amp attack already sweeps the filter significantly. 0.25 amount reduces the filter snap on note-on, which can feel too percussive for a string-derived sound.

10. **oxal_resonance** — default 0.15 → **0.18**. Slightly more resonance emphasizes the phyllotaxis partial spacings by adding a resonant peak that interacts differently with inharmonic vs harmonic partials.

11. **oxal_growthTime** — default 10.0 → **8.0**. For the pioneer species, 8 seconds is a more responsive bloom. Seven partials arriving in 8 seconds means each partial appears roughly every 1.1 seconds — audibly distinct arrivals that don't feel slow.

12. **oxal_lfo2Depth** — default 0.0 → **0.05**. Give LFO2 a minimal default depth. With phi already modulated by `l2 * 0.2f` in the render path, a tiny LFO2 depth creates subtle phi oscillation — the golden ratio slowly pulsing. This is the "living" geometric character.

## 10 Awakening Presets — XOxalis

### OX-1: Pure Pioneer
**Mood:** Foundation | **Gap:** Init patch — best foot forward

The default XOxalis with all parameters at their refined defaults. Phi at 0.4 — clearly different from a supersaw, clearly related to strings. The geometric foundation.

### OX-2: Maximum Phi
**Mood:** Crystalline | **Gap:** Extreme phi=1.0 — full phyllotaxis

Phi at 1.0 — all seven partials at golden ratio intervals. The sound is recognizably string-adjacent but wrong in a beautiful way. No vibrato. High symmetry. Mathematical purity. The sunflower seed head made sound.

### OX-3: Supersaw Baseline
**Mood:** Foundation | **Gap:** Phi=0.0 comparison preset

Phi at 0.0 — standard harmonic series. Wide spread, maximum voices all present. This is XOxalis as a standard supersaw synth strings — no phyllotaxis, just seven harmonic partials. Shows players the baseline before phi transforms it.

### OX-4: Phi Modulation
**Mood:** Flux | **Gap:** LFO-driven phi sweeping

LFO2 at 0.07 Hz with depth 0.4, shape Sine. The phi parameter sweeps from 0.1 to 0.5 slowly, cycling every ~14 seconds. The sound moves between standard string character and phyllotaxis character with each cycle. A living mathematical transformation.

### OX-5: Pioneer Growth
**Mood:** Organic | **Gap:** Growth mode showcase — partials emerging

Growth Mode at 8 seconds. Partials emerge sequentially at golden angle intervals. Hold a chord and count the arrivals: partial 1 immediately, partial 2 at ~1.1s, partial 3 at ~2.3s, through partial 7 at ~6.8s. The sunflower seed head building outward.

### OX-6: Mathematical Precision
**Mood:** Crystalline | **Gap:** Symmetry=1.0 pure geometric character

Symmetry at 1.0 — no asymmetric waveshaping. Phi at 0.75. Very low vibrato. The sound is crystalline and precise, each partial at its exact geometric position without the softening of organic distortion. The most "mathematical" XOxalis preset possible.

### OX-7: Dark Geometry
**Mood:** Deep | **Gap:** Low cutoff, high phi, moody

Phi at 0.85, cutoff at 3500 Hz, brightness at 0.3. The phyllotaxis partials with a dark filter — the geometry becomes ominous rather than precise. Release at 2.0s for lingering inharmonic tails. High warmth control (0.7).

### OX-8: Phi Shimmer
**Mood:** Atmosphere | **Gap:** Atmospheric string-adjacent pad

Medium phi (0.5), very slow LFO1 (0.03 Hz) modulating the filter, wide spread. Release at 3.0s. The result is a shimmering atmospheric pad that doesn't quite sound like strings and doesn't quite not sound like strings. For cinematic backgrounds.

### OX-9: Seasonal Pioneer
**Mood:** Luminous | **Gap:** Phyllotaxis + GardenAccumulators interaction

No manual season override (uses accumulator-derived season). Phi at 0.45. Brightness at 0.7 for initial Spring character. As W accumulates, the seasonal brightness shifts the filter and warmth rolloff engages. The pioneer species adapting to the session.

### OX-10: Quad Pioneer
**Mood:** Entangled | **Gap:** XOxalis as bottom layer in full GARDEN quad

Phi at 0.3 — mostly harmonic, slightly geometric. Low attack (0.01s), moderate release (0.9s). macroCharacter=0.3 for slight opening. Designed as a coupling base for the other GARDEN engines: XOxalis establishes the tonal ground, then XOvergrow/XOsier/XOrchard grow into it.

---

## XOxalis Scripture

**OXALIS-I: The Pioneer Arrives with Mathematics** — In ecological succession, the first organism to colonize bare rock is not the most beautiful or the most complex. It is the one that can survive with the least. XOxalis colonizes the tonal space first because it requires the least established warmth to express itself (`aThreshold=0.3`, `wRiseRate=0.004` — the fastest-responding accumulator configuration in the quad). But it does not colonize bare rock with bare sound. It arrives with the golden ratio — seven partials at phi intervals, each one a position on the phyllotaxis spiral that has organized every sunflower and every pinecone since before mathematics had a name for it. The pioneer arrives with beauty that is too regular to be accidental.

**OXALIS-II: Phi Is a Dial Between Two Worlds** — The `oxal_phi` parameter is not a brightness knob. It is not a character knob in the conventional sense. It is a dial between two theories of harmony. At 0.0, harmony is integer ratios — the octave (2:1), the fifth (3:2), the third (5:4). Western tonal music. At 1.0, harmony is irrational ratios — phi^1 (1.618), phi^2 (2.618), phi^3 (4.236). Mathematical inevitability. Between them lies a spectrum of increasing mathematical strangeness that never becomes ugly because phi, unlike arbitrary inharmonicity, has an organizing principle. Turn the knob. Hear the world change.

**OXALIS-III: Growth Mode Is Phyllotaxis in Time** — The sunflower seed head does not grow all at once. Seeds appear one by one, each at a golden angle (137.5 degrees) from the previous one. Over time, spiral patterns emerge — not because the sunflower knows about spirals, but because the golden angle, repeated, creates them inevitably. XOxalis Growth Mode enacts this process in sound: seven partials appear sequentially, each one a new voice in the phyllotaxis bank. Hold a note for eight seconds and you have watched a sunflower grow. The spiral emerges not from design but from repetition. This is the deepest truth of the GARDEN quad: time itself is a growth medium.

---

---

# XOVERGROW — "The Weed Through Cracks"

## DSP Understanding

XOvergrow is the only GARDEN engine built on physical string synthesis (Karplus-Strong) rather than oscillator stacks. The `KarplusStrongString` struct implements the classic algorithm: noise burst → tuned delay line → one-pole damping filter → feedback loop. The sound is fundamentally different from the saw-based engines: physical, resonant, decaying.

**The silence-response mechanism** is XOvergrow's defining feature. In `noteOn()`, `engineSilenceTimer` is read and a `silenceBoost` (up to 1.0) is computed based on silence duration. After 5+ seconds of silence, the next note is louder and brighter — the weed erupting through pavement. This creates a performance dynamic where players who wait longer get a more explosive response.

**The RunnerGenerator** spawns sympathetic sub-harmonics from stressed notes (high velocity + high aggression accumulator). Runners appear after a delayed onset (2-8 seconds) at octave or fifth below the base frequency. They fade over ~2 seconds. This is architecturally the most "ecosystem" behavior in the quad — one played note eventually generating additional notes.

**Wildness** (`grow_wildness`) adds random pitch jitter (±5 cents at maximum) per sample. Combined with the RunnerGenerator probability scaling, high wildness = unpredictable growth. This is the weed refusing to be controlled.

**Bow noise** (`grow_bowNoise`) adds continuous noise injection during note sustain, simulating the friction of a bow on a string. This is the only GARDEN engine with a continuous excitation source separate from the initial pluck — all others use a one-time attack transient.

**Macro assignments:**
- CHARACTER (M1): cutoff +4000 Hz + wildness +0.3 + season brightness — opens and makes more unpredictable
- MOVEMENT (M2): vibrato rate +3 Hz + wildness via aggression — adds movement and unpredictability
- COUPLING (M3): no specific macro routing visible in render path (uses standard couplingFilterMod)
- SPACE (M4): not directly wired — use for LFO depth

## Parameter Refinements (12)

1. **grow_wildness** — default 0.3 → **0.2**. The weed metaphor is compelling but wildness=0.3 on init creates noticeable pitch jitter that surprises players unfamiliar with the engine. 0.2 keeps the organic irregularity without alarming players. They can always turn it up.

2. **grow_bowNoise** — default 0.2 → **0.15**. Bow noise at 0.2 adds audible friction that can feel harsh on short-decay presets. 0.15 keeps the bowed character without the harshness on note attack.

3. **grow_feedback** — default 0.995 → **0.996**. Slightly higher feedback extends the natural sustain of the KS string model without changes to the envelope. The tail lasts longer, showing more of the resonant character.

4. **grow_damping** — default 0.4 → **0.45**. The one-pole damping coefficient `0.5 + damp * 0.49` maps 0.4 → 0.696 (coefficient). At 0.45, the coefficient is 0.72 — more high-frequency damping per cycle, giving a rounder, less bright initial attack.

5. **grow_release** — default 1.5 → **2.0**. The Karplus-Strong algorithm has its own natural decay via damping and feedback. The amp envelope release should be longer than that natural decay to avoid the envelope cutting off a still-resonant string. 2.0s better matches the string's physical tail.

6. **grow_vibratoDepth** — default 0.3 → **0.25**. The solo string character benefits from slightly less automatic vibrato. Players apply vibrato intentionally via the mod wheel (CC1 adds +0.5 to effective depth). Lower default respects performance intention.

7. **grow_cutoff** — default 4000.0 → **3500.0**. KS strings have natural high-frequency richness from the noise burst excitation. 3500 Hz cutoff tames this without killing the presence. Gives the string a woodier character.

8. **grow_filterEnvAmt** — default 0.3 → **0.2**. The filter envelope on a KS string can feel redundant since the KS model already has natural brightness evolution from the damping filter. Reducing the filter envelope amount lets the physical model dominate the tonal evolution.

9. **grow_attack** — default 0.05 → **0.03**. For bowed strings, a 50ms attack is slightly too slow for the snappier KS excitation to feel natural. 30ms is faster but still creates the brief swelling onset of a bow catching the string.

10. **grow_growthTime** — default 15.0 → **12.0**. The intermediate species blooms slightly faster than the climax species. 12 seconds is more responsive while still being a substantial growth arc.

11. **grow_lfo1Depth** — default 0.1 → **0.08**. The LFO1 filter modulation at the default depth creates slightly too much motion in the KS model. 0.08 keeps gentle breathing without fighting the natural resonance of the delay line.

12. **grow_resonance** — default 0.3 → **0.25**. The CytomicSVF resonance at 0.3 can self-oscillate subtly on some note transitions. 0.25 is safer as a default while remaining expressive. Players seeking the resonant self-oscillation can increase it.

## 10 Awakening Presets — XOvergrow

### GR-1: Silence Preparation
**Mood:** Organic | **Gap:** Silence-response showcase — explicit silence instruction

The Silence Preparation preset is designed to be played after 10+ seconds of complete silence. The engineSilenceTimer builds, and when the first note arrives, the silenceBoost is fully active: louder hit, brighter excitation, boosted wildness. The preset uses moderate parameters but the player instruction is key: wait. The weed has been growing roots in the dark.

### GR-2: Runner Study
**Mood:** Organic | **Gap:** Runner generation showcase

High velocity + high wildness + high aggression accumulator (will build after several forte notes). Runners spawn at octave or fifth below, delayed 2-8 seconds. Play a held note at high velocity and listen for the sub-harmonic ghost that appears 3-5 seconds later. This is the most ecosystem-like behavior in the entire fleet: one note eventually creating another.

### GR-3: Solo Pizzicato
**Mood:** Foundation | **Gap:** Short-envelope plucked string character

Short attack (0.005s), short decay (0.15s), low sustain (0.1), release (0.4s). Low bow noise. Damping high (0.7) for bright, quick decay. Feedback at 0.988 for fast natural decay. This is XOvergrow as pizzicato — plucked, not bowed. The physical KS model without envelope sustain behaves exactly as a plucked string: bright initial transient, quick exponential decay.

### GR-4: Night Cello
**Mood:** Atmosphere | **Gap:** Slow, dark, expressive bowed character

Long attack (0.15s), long release (3.0s), high bow noise (0.4), low wildness (0.1), low cutoff (2500 Hz), high warmth (0.7). Deep, resonant, night-time solo cello character. The bow noise creates continuous sustain texture. Very slow vibrato (rate 3.5, depth 0.18) for an expressive, classical character.

### GR-5: Vine Crawl
**Mood:** Organic | **Gap:** Slow-moving organic texture

Very slow attack (0.25s), moderate wildness (0.35), moderate bow noise (0.25). LFO1 at 0.08 Hz modulating the filter gently. The notes emerge slowly, grow organically with the bow noise, and trail off. For background string textures that breathe slowly.

### GR-6: Weed Through Snow
**Mood:** Deep | **Gap:** Winter Dormancy — cold start with high D accumulator

Manual season interaction: play after extended silence (D accumulator high). Bright setting (brightness 0.7) contrasted with cold cutoff (3200 Hz). The dormancy pitch variance creates slight tuning instability — the weed finding its way through cold pavement. High damping for a brittle, cold attack character.

### GR-7: Jazz Weed
**Mood:** Foundation | **Gap:** Pizzicato jazz string character

Medium attack (0.02s), short decay (0.25s), moderate sustain (0.45). Wildness at 0.25 for slight pitch personality. Bow noise low (0.08). For jazzy, slightly imperfect string lines where the weed's organic character becomes musical personality rather than sonic roughness.

### GR-8: Growth Mode Tendril
**Mood:** Luminous | **Gap:** Growth mode showing KS germination

Growth Mode active at 12 seconds. The KS model begins with an initial noise burst (the seed), and over the growth duration, the amplitude fades in via the quadratic growthGain. The bow noise provides continuous excitation during growth. For slowly emerging string textures.

### GR-9: Street Cracks
**Mood:** Flux | **Gap:** Maximum wildness + high aggression

Wildness at 0.8, bow noise at 0.35, moderate cutoff (5000 Hz). Hard-velocity playing builds the aggression accumulator. Runners fire unpredictably (wildness=0.8 means ~60% runner spawn probability at high aggression). This is XOvergrow at its most untamed — the weed forcing its way through every crack in the pavement simultaneously.

### GR-10: Gentle Tendril
**Mood:** Atmosphere | **Gap:** Soft, restrained solo string

Very low wildness (0.05), very low bow noise (0.05). Smooth attack (0.08s), long release (2.5s). High cutoff (5500 Hz) for brightness, moderate brightness (0.55). This is XOvergrow when it is not stressed — the tendril exploring space without force, finding a path by being gentle rather than persistent.

---

## XOvergrow Scripture

**OVERGROW-I: Silence Is Not Absence — It Is Accumulation** — XOvergrow is the only engine in the fleet that treats silence as an active state rather than a passive one. While other engines simply suspend processing when no notes are playing, XOvergrow's `engineSilenceTimer` accumulates with every silent block. When the silence ends and a new note arrives, the engine is louder, brighter, and more volatile than it was when the silence began. The weed has not been waiting. It has been growing roots in the dark, storing energy for the moment the ground opens. Every performance practice teaches players to manage their playing time. XOvergrow teaches a different skill: manage your silence time.

**OVERGROW-II: The Runner Is the Most Ecological Sound in the Fleet** — The `RunnerGenerator` spawns sympathetic sub-harmonics from notes played at high velocity when the aggression accumulator is elevated. A runner appears 2-8 seconds after the triggering note, at an octave or fifth below, and fades over 2 seconds. No other engine in the XOmnibus fleet produces new sounds from played sounds through ecological stress simulation. The runner is not an effect — it is a consequence. Play forcefully and the soil sends up a new shoot. This is synthesis as ecosystem dynamics rather than synthesis as signal processing.

**OVERGROW-III: Karplus-Strong Has a Physics That Sawtooth Cannot Emulate** — The KS model's tone evolves through physical decay, not through an envelope applied to a steady oscillator. The damping filter removes high frequencies each time the signal passes through the feedback loop — exactly as a real string loses energy to friction and air resistance on each vibration. The result is a natural brightness-to-darkness arc that no amount of envelope shaping on a sawtooth can replicate: the initial noise burst has full spectral complexity, and each cycle removes more of the high frequencies until only the fundamental remains. This is not modeled brightness evolution. It is physical brightness evolution.

---

---

# XOSIER — "The Herb Garden"

## DSP Understanding

XOsier is the chamber quartet — the most harmonically sophisticated of the four GARDEN engines. Where XOrchard uses 4 oscillators per voice to simulate an orchestral section, XOsier uses the 4 voices themselves as distinct quartet roles: Soprano, Alto, Tenor, Bass. The roles are determined by played pitch:

- Soprano: note ≥ G4 (67) — bright, narrow vibrato, +800 Hz filter bias, +1.5 cents detune
- Alto: note ≥ D4 (62) — warm, medium vibrato, +200 Hz filter bias, +0.5 cents detune
- Tenor: note ≥ G3 (55) — neutral, wider vibrato, -200 Hz filter bias, -0.5 cents detune
- Bass: note < G3 (55) — dark, slow vibrato, -600 Hz filter bias, -1.5 cents detune

This pitch-to-role mapping means MIDI playing position naturally activates appropriate quartet timbres. A bass line triggers the dark, slow-vibrato Bass voice. A soprano melody triggers the bright, narrow-vibrato Soprano.

**CompanionPlanting** tracks affinity between voice pairs (6 pairs from C(4,2)). Affinity rises when both voices sound simultaneously, decays slowly when either is silent. High affinity causes voices to pull toward each other in pitch via `companionPitchCents` — they "want" to be in tune with their frequent companions. Over a long session of playing chords, the quartet voices develop harmonic alignment.

**Intimacy** (`osier_intimacy`) scales the companion pitch pull strength. At high intimacy, voices that have played together pull strongly toward each other in pitch — the quartet has "settled in." At low intimacy, voices remain independent.

**Growth Mode** in XOsier sequences voices by role with staggered entry: role delay = `vi * 0.15`. Soprano (vi=0) enters at growthPhase 0, Alto at 0.15, Tenor at 0.30, Bass at 0.45. This is the classic string quartet tuning-up sequence: the section is not synchronized from the start but falls into alignment over time.

**Macro assignments:**
- CHARACTER (M1): cutoff +4000 Hz + season brightness — opens the quartet
- MOVEMENT (M2): vibrato rate +3 Hz — adds motion
- COUPLING (M3): no specific internal routing
- SPACE (M4): not directly wired — use for LFO depth

## Parameter Refinements (12)

1. **osier_companion** — default 0.4 → **0.5**. The CompanionPlanting mechanism is XOsier's identity-defining feature. At 0.4, the harmonic affinity development is subtle enough that players don't notice it. At 0.5, the pitch pull becomes audible in long sessions — voices that have played together develop a perceptible tuning intimacy. The default should demonstrate the feature.

2. **osier_intimacy** — default 0.5 → **0.55**. Companion pitch pull intensity. 0.55 gives slightly more audible voice-to-voice harmonic pull at high companion affinity values. The quartet develops its personality faster.

3. **osier_detune** — default 5.0 → **4.0**. The 5-cent per-voice detune on top of the per-role detune (`±1.5 cents`) creates ±6.5 cents at soprano/bass. This is slightly wide for a chamber setting vs. an orchestral setting. 4 cents base gives ±5.5 cents total — more intimate, more chamber-appropriate.

4. **osier_vibratoDepth** — default 0.25 → **0.22**. Per-role vibrato depth multipliers already differentiate the voices. The global default controls how much vibrato the ensemble collectively has. 0.22 gives a more restrained chamber character.

5. **osier_attack** — default 0.1 → **0.12**. The chamber quartet benefits from a slightly slower attack than the initial 0.1s. At 0.12s, the onset of each voice is slightly softer, reinforcing the intimate character.

6. **osier_release** — default 0.8 → **1.0**. Longer release allows the companion planting pitch pull to be heard in the note tail — voices releasing slowly continue influencing each other's companion pitch metrics for longer.

7. **osier_cutoff** — default 5000.0 → **4500.0**. Chamber strings are warmer than orchestral strings. 4500 Hz gives a slightly darker character that better represents the intimacy of a quartet vs. a full section.

8. **osier_brightness** — default 0.5 → **0.45**. Matching the slightly warmer cutoff. Chamber brightness should be slightly below orchestral brightness.

9. **osier_filterEnvAmt** — default 0.35 → **0.3**. Less filter envelope snap on a chamber instrument. The per-role tonal shaping filter already handles register-appropriate brightness — the envelope amount can be slightly less aggressive.

10. **osier_lfo1Depth** — default 0.1 → **0.12**. The LFO1 filter modulation creates gentle filter breathing. 0.12 makes it slightly more present as a default character — the quartet breathes.

11. **osier_growthTime** — default 20.0 → **18.0**. The staggered role entry (0 → 15% → 30% → 45% growth phase delay) over 18 seconds means the Bass voice doesn't begin until growthPhase=0.45, which at 18s is 8.1 seconds. This is a good dramatic arrival time for the bass voice.

12. **osier_lfo2Rate** — default 1.0 → **0.8**. The second LFO at 0.8 Hz provides slower secondary modulation that doesn't clash with the primary vibrato. At 1.0 Hz, the two LFOs can create an audible beating pattern that is distracting.

## 10 Awakening Presets — XOsier

### OS-1: Cold Quartet
**Mood:** Atmosphere | **Gap:** High Dormancy — Winter chamber character

D accumulator influence: high dormancy pitch variance makes each voice slightly out of tune on the initial attack, settling as the note sustains. Short attack, moderate release. Companion=0.2 (not much affinity yet — the session is new). The quartet plays cold, finding its tuning.

### OS-2: Warm Ensemble
**Mood:** Organic | **Gap:** High W — session warmth fully developed

High companion (0.8), high intimacy (0.85). These values only pay off after sustained playing — the companion affinity accumulates. For a preset that describes its own future: play it for 5 minutes and it will sound different than it does on first trigger.

### OS-3: Neo-Soul Strings
**Mood:** Foundation | **Gap:** Short attack, bright, highly intimate character

Short attack (0.04s), high brightness (0.75), high cutoff (6000 Hz), moderate release (0.7s). The intimacy at 0.8 makes the quartet pull together harmonically — a jazz/neo-soul strings character where the voices are tight and bright.

### OS-4: Intimate Adagio
**Mood:** Deep | **Gap:** Slow, expressive, maximum companion intimacy

Very slow attack (0.25s), very long release (3.5s), high companion (0.9), high intimacy (0.9). Low detune (2.5 cents) — the voices are nearly unison. For a slow, intimate string performance where the four voices have played together so many times they breathe as one.

### OS-5: Growth Mode Quartet
**Mood:** Luminous | **Gap:** Sequential voice entry showcase

Growth Mode active at 18 seconds. Soprano enters immediately, Alto at ~2.7s, Tenor at ~5.4s, Bass at ~8.1s. The quartet assembles itself. This is the most instructional XOsier preset: hold a chord for 9 seconds and hear four distinct voices enter one by one, each with its own tonal character.

### OS-6: Baroque Precision
**Mood:** Crystalline | **Gap:** Clean, precise chamber sound

Very low detune (1.5 cents), low vibrato depth (0.1), moderate companion (0.35), low intimacy (0.3). High cutoff (5500 Hz), low resonance (0.12). This is XOsier as a baroque string quartet — precise, clean, restrained. No companion planting intimacy to muddy the individual voice character.

### OS-7: Cello Foundation
**Mood:** Foundation | **Gap:** Low-register bass voice emphasis

Primarily bass-register playing will trigger the Bass role (dark, slow vibrato). Low cutoff (3000 Hz), high warmth (0.7), slow vibrato (3.5 Hz rate, 0.2 depth). The bass register of the quartet as a solo instrument.

### OS-8: Soprano Lead
**Mood:** Atmosphere | **Gap:** High-register soprano character

High-register notes trigger Soprano role (+800 Hz filter bias, narrow vibrato). Bright cutoff (6500 Hz), high brightness (0.65). For melodic soprano-register playing where the bright, narrow-vibrato character carries a line.

### OS-9: Long Session Quartet
**Mood:** Organic | **Gap:** Preset demonstrating companion planting development over time

Growth Mode off. Companion at 0.9, intimacy at 0.9, long attack (0.15s). The preset is not impressive on first playback. Play chords for 3-4 minutes and the companion affinity accumulates — voices that have played together start pulling toward each other harmonically. The preset rewards patience.

### OS-10: Thyme Garden
**Mood:** Organic | **Gap:** Light, mid-register chamber warmth

Medium parameters throughout, companion at 0.55, intimacy at 0.6, moderate cutoff (4800 Hz). LFO1 at 0.15 Hz for slow filter breathing. This is XOsier in the middle of its range — not cold, not extreme, not showing off. Just four voices playing warmly together in a tended garden.

---

## XOsier Scripture

**OSIER-I: The Role Is Determined by Position, Not By Choice** — In a real string quartet, instruments are assigned to players based on their instrument type: the violinist plays violin, the cellist plays cello. XOsier assigns vocal roles based on the MIDI pitch played: notes above G4 become Soprano, notes below G3 become Bass. This is not a limitation — it is an invitation. When you play a bass line, you are playing through the dark, slow-vibrato voice. When you improvise in the upper register, you are playing through the bright, narrow-vibrato voice. The instrument responds to where you are, not to what you selected. Play up and down the keyboard and hear the quartet shift beneath your hands.

**OSIER-II: Companion Planting Requires Time to Bear Fruit** — The `CompanionPlanting` system accumulates pair affinity at 0.5 units per second when both voices are simultaneously active, decays at 0.02 units per second when either is silent. From zero affinity to maximum affinity (1.0) requires two continuous seconds of co-activity. From zero to a perceptible harmonic pull (0.3 affinity, scaling into audible pitch influence) requires about 36 seconds of playing chords. This is a deliberate design: the herb garden does not yield its herbs immediately. The companion planting needs time. A producer who plays XOsier for 10 minutes will hear a different instrument than one who loads the preset and plays for 30 seconds. This is session memory as musical character.

**OSIER-III: The Quartet Voices Cannot Be Assigned — They Must Be Earned** — Every XOsier note asks the same question of the player: where are you playing? High notes earn the soprano — bright, narrow vibrato, filter bias toward brilliance. Low notes earn the bass — dark, slow vibrato, filter bias toward depth. Middle notes fall between. The quartet is not a fixed configuration but a dynamic response to the player's register. A musician who plays exclusively in the alto register will only ever hear the alto character, never discovering the soprano brilliance or the bass depth. The full quartet requires range. This is not a technical requirement. It is a compositional invitation.

---

---

# XORCHARD — "The Cultivated Grove"

## DSP Understanding

XOrchard is the climax species — the most complex, most resource-demanding, and most musically sophisticated of the four GARDEN engines. It uses four detuned PolyBLEP sawtooth oscillators per voice (16 total at full polyphony), a parallel formant filter (band-pass running alongside the main LP filter), and the full GardenAccumulators seasonal state machine.

**The formant body resonance filter** (`orch_formant`) sweeps a bandpass filter from 300 Hz (viola warmth, formant=0) to 2800 Hz (violin brilliance, formant=1). The blend coefficient `formNow * 0.5` means at maximum formant, 50% of the output passes through the bandpass. This adds orchestral body resonance — what a violin or viola body adds to the raw string vibration.

**The seasonal arc** (`orch_season`) is the most compositionally original feature in the GARDEN quad. Manual values 0/1/2/3 set Spring/Summer/Fall/Winter. Value -1 (auto) derives the season from W/A/D accumulators. Each season shifts the effective filter by a different amount: Spring +0 Hz, Summer +0 Hz (neutral, full), Fall -400 Hz, Winter -1000 Hz.

**The Concertmaster mechanism** (in render path since the original build, confirmed in source): the highest active voice has a slight pitch-pull authority over others (±3 cents maximum pull via `dormancyPitchCents` blending). The effect is subtle — not obvious pitch following but a gentle gravitational pull of the section toward the concertmaster's tuning.

**Growth Mode** in XOrchard sequences four oscillators within each voice: oscillator 0 enters at growthPhase=0.0, oscillator 1 at 0.2, oscillator 2 at 0.4, oscillator 3 at 0.6. Full four-oscillator ensemble is achieved at growthPhase=0.8. This is 80% of the growth time for the full orchestral swell — the remaining 20% is full sustain.

**Detuning geometry**: the four offsets `{-7, -3, +3, +7}` cents scale with `orch_detune`. At detune=1.0 (parameter value 1), these are 1/7th of maximum. At detune=7 (default), the offsets are exactly `{-7, -3, +3, +7}` cents — a minor seventh of spread.

**Macro assignments:**
- CHARACTER (M1): cutoff +4000 Hz + formant +0.3 — opens and brightens the orchestral body
- MOVEMENT (M2): detune +10 cents + vibrato rate +3 Hz — widens the ensemble and adds motion
- COUPLING (M3): formant modulation via `couplingFormantMod` — external engines sweep the orchestral body character
- SPACE (M4): not directly wired

## Parameter Refinements (12)

1. **orch_attack** — default 0.15 → **0.18**. The orchestral string section enters more slowly than individual strings. 180ms attack reinforces the climax-species patience — the section doesn't rush.

2. **orch_growthTime** — default 20.0 → **25.0**. The climax species takes longer to establish. At 25 seconds, the four-oscillator ensemble builds over a full musical phrase (~2 bars at 120 BPM). This makes the Growth Mode more distinctly cinematic.

3. **orch_formant** — default 0.5 → **0.45**. The formant body resonance at 0.5 can sound slightly mid-heavy on sustained chords. 0.45 keeps the character while preventing filter stacking artifacts with the main LP filter.

4. **orch_vibratoDepth** — default 0.2 → **0.22**. The orchestral string section should have slightly more audible vibrato than solo strings. 0.22 gives appropriate section vibrato character.

5. **orch_ensembleWidth** — default 0.7 → **0.75**. The orchestra deserves wider stereo spread by default. 0.75 places voices more clearly in the left-right field while still remaining cohesive.

6. **orch_warmth** — default 0.5 → **0.55**. Slightly more warmth rolloff from the W accumulator. The orchestra settles into warmth faster.

7. **orch_sustain** — default 0.8 → **0.85**. A string section at full sustain should be closer to 0.85 — the section doesn't drop significantly between attack and sustain phases. 0.85 gives a more consistently full orchestral presence.

8. **orch_release** — default 1.0 → **1.5**. The orchestral string section releases slowly — the hall continues to resonate after the bow stops. 1.5s is a more realistic hall-tail release.

9. **orch_lfo1Depth** — default 0.1 → **0.12**. The LFO1 filter modulation gives gentle breathing. 0.12 makes this character slightly more present as default orchestral movement.

10. **orch_filterEnvAmt** — default 0.4 → **0.3**. The filter envelope at 0.4 creates a noticeably bright filter sweep on note attacks. For an orchestral string pad, 0.3 gives a more subtle, appropriate filter response.

11. **orch_resonance** — default 0.2 → **0.15**. Lower resonance gives a smoother, less pronounced filter peak. Orchestral string synthesis benefits from smooth filter behavior rather than resonant emphasis.

12. **orch_decay** — default 0.5 → **0.8**. A longer decay phase allows the four-oscillator ensemble to settle from its initial transient into the sustained character more naturally. 0.8s decay gives a more realistic string section onset.

## 10 Awakening Presets — XOrchard

### OR-1: Film Score Strings
**Mood:** Atmosphere | **Gap:** Cinematic film score orchestral string pad

The definitive XOrchard film score preset: slow attack (0.25s), very long release (3.5s), Summer season, medium formant (0.45), wide ensemble (0.85). The orchestral string pad that goes under dialogue, under establishing shots, under emotional moments in film. Timeless, lush, completely present.

### OR-2: Seasonal Arc Study
**Mood:** Foundation | **Gap:** Educational preset demonstrating the seasonal arc

Auto season (season=-1, encoded as -1 in integer parameter). Brightness at 0.4, warmth at 0.55. Begin in Spring (first few notes) — bright, slightly thin. Play warmly for several minutes and reach Summer — full, neutral. Play aggressively and push toward Fall — rolled-off, rich. Leave it silent for minutes and find Winter — dark, cold. One preset. Four tonal characters. Session time is the parameter.

### OR-3: Spring Arrival
**Mood:** Foundation | **Gap:** Bright, fresh, season 0

Manual Spring (season=0). High brightness (0.6), high cutoff (7000 Hz). The orchard in its fresh, new state — not yet warm, slightly thin, energetic. For contexts requiring bright orchestral strings without the heaviness of a well-established section.

### OR-4: Deep Summer Section
**Mood:** Organic | **Gap:** Maximum warmth — season 1 fully developed

Manual Summer (season=1), high warmth (0.75), high companion pulling from long-session W. The fullest, lushest orchestral strings in the fleet. This is the moment the section is at its absolute best: warm, full, every oscillator in place, every voice contributing.

### OR-5: Harvest Fall
**Mood:** Organic | **Gap:** Season 2 — rich, slightly rolled-off

Manual Fall (season=2), warmth 0.72, cutoff 4200 Hz. The richness of strings after a long performance — the section has been playing for hours, the strings are warm, the rosin is set, the hall is comfortable. Melancholic and full simultaneously.

### OR-6: Winter Grove
**Mood:** Deep | **Gap:** Season 3 — full darkness, cold strings

Manual Winter (season=3), very low brightness (0.25), cutoff 3200 Hz. Long release (4.0s) for slow, cold string tails. This is the orchard in January — the string section in an empty concert hall. The most introverted orchestral preset in the fleet.

### OR-7: Cinematic Swell
**Mood:** Luminous | **Gap:** Growth mode as film score technique

Growth Mode active at 25 seconds. Play a chord and hold. Oscillator 1 at t=0, oscillator 2 at t=5s, oscillator 3 at t=10s, oscillator 4 at t=15s. Full ensemble at t=20s, 5s of full sustain remaining. This is the classic film score technique: the string swell that builds across a scene, arriving at full power at the emotional peak.

### OR-8: Concertmaster Chord
**Mood:** Foundation | **Gap:** Maximum detune + ensemble width — fullest polyphonic spread

Detune at 20 cents, ensemble width at 0.95. Four voices, each with four detuned oscillators, maximally spread across the stereo field. At full polyphony: 16 oscillators, 20 cent total detune spread. The widest orchestral string preset in the fleet. The section is so wide it becomes a wall of sound.

### OR-9: Formant Study
**Mood:** Prism | **Gap:** Extreme formant character — violin brilliance vs viola warmth

High formant (0.9) — violin-register body resonance at ~2.8 kHz. High cutoff (8000 Hz), Spring season for brightness. The orchestra at its most brilliant: the formant filter adding violin body resonance to the full section. For contexts requiring cutting, present string character in a dense mix.

### OR-10: Long Bloom
**Mood:** Atmosphere | **Gap:** Extended Growth Mode — 60 seconds maximum germination

Growth Mode active at 60 seconds (maximum). Extremely slow build — the full orchestral ensemble does not arrive until a full minute has elapsed. For ambient, installation, or drone contexts where the orchestra is a slow architectural process rather than a musical event. Hold a chord for 60 seconds and hear civilization build.

---

## XOrchard Scripture

**ORCHARD-I: The Climax Species Arrives Last and Stays Longest** — In a GARDEN quad performance, XOxalis responds to the first note. XOvergrow settles in over the first phrase. XOsier develops companion affinity over several minutes. XOrchard is still arriving 10 minutes in — its warmth accumulator still building, its seasonal arc still developing, its Growth Mode still assembling the section. But once arrived, XOrchard stays: the W accumulator decays slowly (`wDecayRate=0.0005`), meaning a warm session remains warm for minutes after playing stops. The climax species is not impatient and it is not fragile. It is the entity that the other three species make possible.

**ORCHARD-II: Sixteen Oscillators Is a Section, Not a Synth** — Four voices, each with four detuned sawtooth oscillators, gives XOrchard sixteen simultaneous oscillators at full polyphony. The detuning offsets `{-7, -3, +3, +7}` cents per oscillator are not random — they mirror the natural intonation spread of a real string section, where individual players tune to approximate unison rather than mathematical unison. The ensemble detuning is the sound of human intonation managed at scale: close enough to sound unified, different enough to sound alive. No single instrument achieves this. A section does.

**ORCHARD-III: The Formant Filter Models the Body, Not the String** — The body of a violin or viola is not a passive resonator. It is an active shaping system that emphasizes certain frequencies and suppresses others based on the instrument's geometry, wood properties, and age. The `orch_formant` bandpass filter models this body response: sweeping from 300 Hz (viola warmth, the low-frequency resonance of the instrument body) to 2800 Hz (violin brilliance, the high-frequency resonance that creates presence and projection). This is not EQ applied after synthesis. It is a model of what the instrument body adds to the vibrating string — the hallmark of physical modeling synthesis applied to a parametric context. The formant parameter is not tonal balance. It is instrument character.

**ORCHARD-IV: The Seasonal Arc Is a Memory of Playing** — The W/A/D accumulators are not automation. They are session memory. A session that begins in Spring (W=0, D=0.3, fresh start) and develops through Summer (W=0.5+, sustained warm playing) and Fall (A accumulated from aggressive phrases) to Winter (D accumulated from long silences) has enacted a seasonal narrative embedded in the instrument's tonal character. The performer did not choose these seasons. They emerged from how the session unfolded. This is the GARDEN quad's deepest design intention: the instruments remember how you played them, and they change in response to that memory.

---

---

# GARDEN QUAD — Shared Wisdom

## The Four Ecological Roles in Performance

Understanding the GARDEN quad as an ecosystem rather than four separate string synths unlocks its full potential:

**XOxalis** establishes the tonal ground immediately. Its fast accumulator rates and instant attack make it the first voice heard in any GARDEN quad coupling. Pioneer species characteristics: adaptable, fast, slightly synthetic.

**XOvergrow** introduces organic character once the ground is established. Its silence-response mechanism means it rewards thoughtful performance timing. Playing loudly builds wildness. Waiting builds the eruption potential. Solo character vs. the ensemble character of the other three.

**XOsier** adds harmonic intelligence over time. The CompanionPlanting affinity builds with each chord played. Early XOsier performances are four independent voices. Later XOsier performances are four voices that have learned each other. The quartet character develops in real-time.

**XOrchard** provides the foundation that ties everything together. Slow to warm up, but once established, the seasonal arc holds the session in a tonal character that frames everything the other three engines do. The climax species provides structural stability.

In a full quad coupling (XOxalis → XOvergrow → XOsier → XOrchard chain via GrowthCoupling or AmpToFilter), the progression is: geometric foundation (Oxalis) → organic character (Overgrow) → harmonic intelligence (Osier) → seasonal stability (Orchard). This is succession.

## The GardenAccumulators — The Fifth Voice

All four engines share the GardenAccumulators state machine. The W/A/D values and the derived Season are computed independently per engine based on that engine's own activity — they are not shared across engines in V1. But the resulting tonal characters all derive from the same three-axis model, creating a consistency of ecological metaphor across the quad. A Winter XOrchard and a Winter XOxalis are sonically different (different synthesis techniques) but ecologically related (both in dormancy, both cold).

The D accumulator starts at 0.3 (`D = 0.3f` on reset) — cold start is always slightly dormant. This models the real experience of picking up an instrument that has been sitting cold: the first few notes are slightly rougher, slightly off-pitch, before the instrument settles in.

## Scripture of the Quad

**GARDEN-I: Ecological Succession Is Not Progress — It Is Fulfillment** — Pioneer species are not primitive versions of climax species. XOxalis is not a less-developed XOrchard. Each species fulfills its ecological role: the pioneer establishes conditions for later arrivals; the climax species provides the stable environment that the pioneers made possible. In the GARDEN quad, playing XOxalis alone is not incomplete. It is the complete expression of the pioneer role. Playing the full quad together is not four separate sounds combined — it is a single ecological system producing sounds impossible with any one member in isolation.

**GARDEN-II: The Mycorrhizal Network Is Session Continuity** — The `GardenMycorrhizalNetwork` transmits stress events from one voice to another via a 4-6 second delay. A stressed voice (high velocity, high aggression accumulator) sends a signal to all connected voices, which receive it seconds later as a slight increase in `dormancyPitchCents` — a subtle pitch perturbation that models the transfer of stress through the underground fungal network. This is not audible in isolation. It is audible across a session: a string of aggressive notes stresses the network, and 4-6 seconds later, the entire voice system is slightly more agitated. Stress propagates underground.

---

*Guru Bin closes the GARDEN retreat. The four engines have been tended. The refinements have been made. The awakening presets have been planted. The scripture has been spoken.*

*The grove is ready.*
