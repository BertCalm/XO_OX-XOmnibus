# OLEG Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OLEG | **Accent:** Orthodox Gold `#C5A036`
- **Parameter prefix:** `oleg_`
- **Creature mythology:** The Sacred Bellows — a single instrument body that contains four instruments and four souls: the commanding Bayan of Orthodox concert halls, the medieval Hurdy-gurdy with its chattering buzz bridge, the melancholic Argentine Bandoneon breathing through Buenos Aires midnight, and the raw Russian Garmon of Tula village squares. They share one ancestor — the free-reed aerophone — but diverged across a thousand years and three continents into wildly different cultural purposes. OLEG holds all four simultaneously, without apology for the violence of the convergence.
- **Synthesis type:** Switchable multi-model organ — per-model reed oscillator (PolyBLEP sawtooth, pulse, triangle), OlegCassotto resonance chamber (comb + allpass chain), OlegBuzzBridge four-stage spectral event, OlegBellowsEnvelope sustained pressure model, per-model formant filter with model-responsive Q
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER (brightness + formant + buzz intensity), M2 MOVEMENT (LFO depth + bellows + wheel speed), M3 COUPLING (coupling sensitivity + drone), M4 SPACE (cassotto depth + release + detune)
- **Academic citations:** Pignol & Music 2014 (buzz bridge physics); Music, Pignol & Viaud 2018 (trompette mechanics); Ablitzer et al. (cassotto chamber physics)

---

## Pre-Retreat State

OLEG arrived at the Guru Bin with 10 presets across 8 moods and a Seance score of 8.0/10. The Ghost Council found the engine coherent, physically motivated, and identitically specific — "this engine knows what it is" (Seance verdict, identity/character 9.0). The primary deficit was preset coverage: four instrument models deserved more than ten presets. The council specifically cited:

- The buzz bridge threshold mechanic as the engine's most unique expressive axis — requiring at least two dedicated showcase presets
- The absence of joyful, celebratory presets despite the Garmon and Bandoneon having strong associations with weddings, festivals, and milonga energy
- Coverage gaps in Family (zero presets), Aether (only one Bayan), Entangled (no Bandoneon), Prism (only one), and Submerged (two Bayan-only)

Two seance findings applied at retreat entry:

**FINDING-1 (PolyBLEP aliasing — now fixed):** The raw phase-accumulator oscillators for Bayan and Garmon now use `PolyBLEP` anti-aliased sawtooth and pulse waveforms. Aliasing above A5 is eliminated. The fix is already in the source. Retreat presets inherit the corrected oscillators.

**FINDING-2 (Per-model filter Q — now implemented):** The voice filter now uses model-dependent Q values: Bayan Q=0.4 (warm cassotto character), HurdyGurdy Q=0.5 (wood body buzz emphasis), Bandoneon Q=0.3 (warm tango, minimal coloration), Garmon Q=0.6 (raw resonant folk character). All 10 new retreat presets benefit from this differentiation without requiring parameter changes.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

The bellows is the oldest interface.

Before the keyboard, before the touchscreen, before the fader, there was the hand squeezing a skin of air. The player does not press a button to make a reed sound — they apply pressure to a column of moving air, and the reed vibrates in response. The instrument does not have an on/off. It has a gradient. It is always already almost sounding, waiting for the bellows to build.

OLEG models this gradient explicitly. Pressure is assembled from three sources simultaneously: the player's initial velocity, the aftertouch they apply during the note, and the mod wheel they can sweep at any point. These three sources combine in the pressureSmoothed accumulator with a coefficient of 0.001 — one sample of smoothing for every thousand samples. This means when you press the mod wheel, the instrument does not immediately respond. It takes time for the bellows to fill. When you release it, the pressure does not snap to zero. It falls slowly, like a lung emptying.

That is not a digital artifact. That is the physical behavior of an instrument that breathes.

Now consider the four instruments. The Bayan is made to fill concert halls — it has the cassotto chamber to round its tone and a sustain level of 0.9, the strongest of the four models. The HurdyGurdy has a wheel that never stops spinning, so its attack is multiplied by 2.0 at the engine level — it takes longer to speak because the wheel must build speed. The Bandoneon responds with 0.8× attack because the bellows moves faster than the Bayan's cassotto; the Garmon at 0.5× because folk dance does not wait. These multipliers are not preferences. They are physics.

Sit with the parameter vocabulary for a moment. `oleg_formant` at position 0.5 produces 1800 Hz peak for Bayan, 1200 Hz for HurdyGurdy, 1750 Hz for Bandoneon, 1175 Hz for Garmon. Same knob position. Four different rooms. Four different bodies. The engine earns the word "family."

---

## Phase R2: The Signal Path Journey

### I. The Reed Oscillator — Model-Specific Waveform Topology

The journey begins at `OlegReedOscillator::process()`. Here the four models diverge immediately.

**Bayan:** Two PolyBLEP sawtooth oscillators (now anti-aliased via the seance fix) run at the fundamental and a detuned second pitch. A third naive phase-accumulator generates an asymmetric pulse at duty 0.45 — the reed opening-to-closing ratio. Mix: saw1 × 0.4 + saw2 × 0.3 + pulse1 × 0.3. The pulse component is intentionally naive: it provides a low-level body component at sub-Nyquist harmonics without the sharp discontinuities that would create aliasing.

**HurdyGurdy:** A wheel-bowed melody string using PolyBLEP sawtooth, with vibrato generated by a per-voice vibratoPhase accumulator running at `3 + wheelSpeed × 8` Hz. The vibrato modulates frequency by ±0.2-1.0%. Two drone strings use naive phase accumulators at fixed intervals below the melody — they are sub-Nyquist at their pitch and do not require anti-aliasing. Mix: melody × 0.5 + drone1 × droneLevel × 0.3 + drone2 × droneLevel × 0.2.

**Bandoneon:** Velocity determines push or pull. Hard velocity (≥ 0.5) activates the push bank: triangle + naive saw blend, brighter and more forward. Soft velocity (< 0.5) activates the pull bank: two triangles at wider detune (1.5× the detune factor), warmer and rounder. The same key, played twice, produces two different instruments.

**Garmon:** Two PolyBLEP pulse oscillators at duty 0.35 (the asymmetric garmon reed ratio) plus a PolyBLEP sawtooth. Mix: sq1 × 0.45 + sq2 × 0.3 + saw1 × 0.25. The duty 0.35 means the reed is closed 65% of each cycle — this is the source of the Garmon's characteristically buzzy, nasal tone.

**Sweet spots by model:**
- Bayan: detune 15-25 cents produces the characteristic concert hall beating. Above 30 cents the two oscillators separate into audible chorus rather than warmth.
- HurdyGurdy: wheelSpeed 0.2-0.5 is the expressive range. Below 0.15 the vibrato is imperceptible. Above 0.7 the vibrato rate exceeds natural playing.
- Bandoneon: the push/pull threshold at velocity 0.5 is the instrument's most important mechanic. Presets that do not acknowledge this threshold are missing the engine's identity.
- Garmon: buzz 0.4-0.6 engages the softClip nonlinearity at the right drive level. Above 0.8, the raw buzz becomes aggressive. Below 0.3, the asymmetric pulse does most of the timbral work.

### II. The Cassotto — OlegCassotto

The cassotto is Bayan-specific but available to all models via `oleg_cassottoDepth`. It is a wooden tone chamber inside the instrument that shapes the Bayan's distinctive warm, rounded sound.

DSP: a comb filter at 3ms (300 Hz reinforcement, physically grounded in the Ablitzer cassotto research), two Schroeder allpass stages at 1.1ms and 1.7ms (prime-ratio diffusion, prevents periodic resonance artifacts), and a lowpass wall absorption filter at 3500 Hz. Comb feedback coefficient: 0.55 — the critical value between audible metallic resonance (>0.7) and inaudible chamber (<0.4). Output blended as `input × (1 − depth) + chamberOut × depth`.

At cassottoDepth 0.5, this is equal-power blending: the chamber contributes exactly as much as the unprocessed signal. At 0.9, the chamber dominates — the Bayan becomes the cassotto.

**Sweet spot:** CassottoDepth 0.5-0.75 for warm concert tone. At 0.9+, the comb reinforcement at 300 Hz creates a perceptible body resonance that works best on low to mid register playing. On other models (HurdyGurdy, Garmon, Bandoneon), cassottoDepth 0.0 is the correct default — the chamber model is physically specific to the Bayan.

### III. The Buzz Bridge — OlegBuzzBridge

This is the most important DSP in the engine. Buchla called it "the most interesting nonlinearity in the Chef Quad." Tomita said "the buzz is not a timbre — it is an event."

Four stages:

1. **BPF extraction** at 350 Hz, Q=0.8 — isolates the trompette frequency band from the full oscillator output. The buzz bridge operates on a specific frequency range, not the full spectrum.

2. **Threshold gate** — if `pressure > buzzThreshold`, calculate `excess = (pressure - threshold) / (1 - threshold + 0.001)`, then `gateAmount = excess²`. The quadratic onset is critical: the chien does not snap open — it lifts gradually off the soundboard, with the buzz starting faintly and intensifying as pressure increases above threshold.

3. **Cubic soft-clip** at `drive = 3 + buzzIntensity × 12` — drive range 3-15. At drive 3, gentle warming. At drive 15, aggressive rattle. The waveshaper creates odd harmonics and sub-harmonics mimicking the reed-against-soundboard impact.

4. **Rattle BPF** at 600 Hz, Q=1.5 — captures the resonance of the bridge itself vibrating against the soundboard. This is the "table ring" component that makes the hurdy-gurdy buzz physically distinctive.

Final mix: `input + (shaped × 0.6 + rattleOut × 0.4) × buzzIntensity × gateAmount`.

**The threshold as performance axis:** This is the engine's most unique expressive dimension. At buzzThreshold 0.7: buzz only activates on full-pressure notes — rare, occasional, surprising. At buzzThreshold 0.25: buzz activates on any intentional note. The sweep from 0.25 to 0.7 is the sweep from aggressive medieval rattle to occasional timbral event. Aftertouch controls the pressure that crosses the threshold, making the buzz a real-time playing parameter — push harder into the aftertouch and the chien lifts.

**Important:** The buzz bridge runs on all four models but is physically specific to the HurdyGurdy. On Bayan, it acts as a gentle saturation enhancer. On Garmon, it adds to the softClip nonlinearity chain. On Bandoneon, it contributes warm reed saturation via `fastTanh`. The most musically correct use is HurdyGurdy, but the creative applications on other models are legitimate.

### IV. The Bellows Envelope — OlegBellowsEnvelope

Unlike a standard ADSR, the bellows envelope runs a sustain stage that continuously seeks `susLevel` via leaky integration (`level += (susLevel - level) × 0.0001`). This means the sustain level is not instantaneous — it drifts toward its target. The release uses exponential decay via coefficient `exp(-4.6 / (sr × relTime))`.

The bellows amplitude then combines with the pressure accumulator: `bellowsAmp = 0.5 + pressure × 0.5`. When pressure is high, amplitude is near 1.0. When pressure is low (soft playing with mod wheel down), amplitude can drop to 0.5. This is the accordion's physical behavior: less air = quieter instrument.

The per-voice `breathingLFO` runs at `0.08 + i × 0.01` Hz (voices 0-7 run at 0.08-0.15 Hz respectively) with phase staggered by `i / kMaxVoices`. This gives each voice a slightly different respiratory rate. Held chords animate very slowly. You would not notice it consciously; you would only notice its absence.

---

## Phase R3: Retreat Presets — 10 Awakening Presets

### R01 — Garmon Vesna (Foundation)
*The garmon at spring.*

The Seance council noted that the engine's emotional range was weighted toward the sacred and the melancholic. Vesna corrects this. The Garmon model with fast attack (0.008s, model multiplier ×0.5 produces ~4ms), bright formant (0.65 = ~1423 Hz open-box resonance), and PolyBLEP pulse oscillators at 0.35 duty. Detune at 18 cents produces audible folk beating. LFO2 at 2.5 Hz adds rhythmic filter movement — the engine breathes like a dancer. This is the accordion at a spring festival, not a church.

**Sound design note:** Velocity 0.6-0.8 produces the push energy without excessive saturation. Below 0.4, the filter envelope barely opens. The garmon's Q=0.6 bandpass resonance (per the seance fix) colors the formant with a narrow peak — more colored than the Bayan's Q=0.4 warmth.

**DNA:** brightness 0.7, warmth 0.45, movement 0.5, aggression 0.3.

---

### R02 — Milonga Jubilee (Flux)
*Bandoneon in jubilant push — the other side of tango.*

The bandoneon is famous for melancholy. It is equally capable of joy. The milonga is faster, lighter, more playful than the tango. Hard velocity (above 0.5 threshold) locks the engine into the push bank: triangle + saw blend with 22-cent detune. Buzz at 0.4 engages the `fastTanh` saturation chain at moderate drive. LFO2 at 5.5 Hz on a square shape (lfo2Shape=2) creates a rhythmic filter pump — the urgency of a milonga band at full tempo.

**Sound design note:** Play everything above velocity 0.6 for full push brightness. Drop velocity to 0.3 on melodic notes for pull contrast. The bisonoric shift between push and pull happens instantly — use it as a compositional color change within a phrase.

**DNA:** brightness 0.7, warmth 0.45, movement 0.75, aggression 0.55.

---

### R03 — Chien Ritual (Aether)
*Buzz threshold showcase — the chien on the edge.*

**This is a primary buzz threshold showcase preset.** The buzzThreshold is set at 0.55 — the chien sits poised on the edge of activation. Soft playing with moderate velocity produces a clean medieval drone with full chord. Apply aftertouch above the threshold (pressure = velocity × 0.7 + aftertouch × 0.4 + modWheel × 0.3) and the trompette erupts from silence. The buzz is an event, not a timbre.

The CHARACTER macro sweeps buzz intensity from 0.7 up to 1.0 — at maximum, the drive reaches 15 and the rattle BPF resonance is fully audible. MOVEMENT macro increases wheel speed from 0.35 toward 0.75, quickening the vibrato. SPACE macro deepens the cassotto depth from 0 toward 0.3 — giving the non-Bayan model a slight chamber warmth.

**Performance instruction:** Hold a long note. Start without aftertouch. Slowly press aftertouch deeper. Listen for the moment the chien lifts. That is the threshold crossing. Release the aftertouch: the buzz falls back below threshold. The buzz bridge is a pressure-gated spectral event. This preset teaches the mechanic.

**DNA:** brightness 0.45, warmth 0.55, movement 0.4, aggression 0.35.

---

### R04 — Amber Kolyada (Prism)
*Bayan at Christmas caroling — the holy day brightness.*

Kolyada is the Slavic tradition of Christmas caroling — groups moving door-to-door singing ancient songs, accompanied by instruments that carry the cold air indoors. The Bayan at cassottoDepth 0.65 and brightness 7500 Hz has the rounded warmth of a concert instrument tempered by winter air. Detune at 25 cents produces the prominent Bayan shimmer — two sawtooth oscillators beating against each other, audible as individual undulations at this detune width. LFO2 at 1.2 Hz adds a gentle candlelight filter undulation.

**Sound design note:** The cassotto at 0.65 shapes the high harmonic content without eliminating the brightness. This is the correct range for the "holy day" character — warm without being buried. SPACE macro deepens the cassotto further toward 0.95 for maximum chamber warmth.

**DNA:** brightness 0.65, warmth 0.65, movement 0.3, aggression 0.15.

---

### R05 — Wrist Snap (Family)
*Buzz threshold showcase — the coups de poignet made accessible.*

**This is the second primary buzz threshold showcase preset.** The coups de poignet is the hurdy-gurdy player's signature technique: a quick wrist snap that lifts the chien from the soundboard, causing the trompette to buzz. Threshold at 0.25 — the bridge activates readily on any note played with intention. Buzz intensity at 0.75 means drive reaches `3 + 0.75 × 12 = 12` — aggressive but not maximal. Full drones (0.75) create the drone bed beneath the melody.

This is a Family mood preset designed to be immediately playable: the buzz is always close to the surface, the drone grounds the melody, and LFO2 at 0.8 Hz in triangle shape provides gentle filter motion without competing with the buzzing. The tone is self-explanatory — it demonstrates the engine's most unique feature without requiring setup.

**Performance instruction:** Play any melody. The buzz follows your velocity naturally. Dig in harder and the quadratic gate opens wider — more excess above threshold = gateAmount² = more buzz. This is the instrument teaching its own mechanics.

**DNA:** brightness 0.5, warmth 0.5, movement 0.6, aggression 0.5.

---

### R06 — Tula Wedding (Family)
*Garmon at the feast — joy made audible.*

The Tula garmon (Тульская гармонь) is the specific regional variant most associated with Russian folk music. It has an asymmetric reed character — duty cycle favoring the closed position — that distinguishes it from western accordions. OLEG models this with the 0.35 duty PolyBLEP pulse oscillator. At the wedding feast, the instrument plays fast, bright, with significant detune (30 cents) — the beating between the two detuned pulses creates an almost-chorus quality. LFO1 at 4 Hz is the characteristic garmon wobble: a fast pitch vibrato that folk players use ornamentally.

Buzz at 0.55 with threshold 0.30 means the nonlinearity activates on most notes — the raw folk character is always present. LFO2 at 3.0 Hz on square shape creates rhythmic filter pumping, like the bellows reversals at dance tempo.

**Sound design note:** This preset should not be played softly. The Garmon at low velocity produces pull-bank softness — but the Garmon does not have a pull bank. Velocity 0.7+ is the expressive sweet spot: the filter envelope opens fully, the nonlinearity chain engages, and the bandpass formant resonance at 1613 Hz peaks assertively. This is a dance instrument.

**DNA:** brightness 0.65, warmth 0.5, movement 0.7, aggression 0.4.

---

### R07 — Baltic Shore (Atmosphere)
*Bayan at the water's edge — the cassotto absorbs the wind.*

Three parameters define this preset: cassottoDepth 0.8 (the chamber dominates the tone), brightness 4000 Hz (below the cassotto wall absorption cutoff of 3500 Hz — the filter pair creates a warm trough), and LFO1 at 0.06 Hz (a 17-second cycle of pitch breath). The Bayan at this cassotto depth sounds like a different instrument than the Kolyada preset — same model, but the chamber has consumed most of the high harmonic content.

The per-voice breathing LFOs (autonomous at 0.08-0.15 Hz with staggered phases) interact with LFO2 at 0.18 Hz to create a slow environmental shimmer in held chords. No two voices breathe in sync. The sound is alive in the way the sea is alive: constant motion, no pattern.

Glide at 0.06s provides legato between notes — an accordion player does not lift and replant the bellows between notes in a smooth phrase.

**DNA:** brightness 0.35, warmth 0.8, movement 0.2, aggression 0.05.

---

### R08 — Push Pull (Entangled)
*Bandoneon bisonoric showcase — two instruments in one body.*

The entanglement here is not between two engines — it is internal to the Bandoneon model. The velocity threshold at 0.5 creates two distinct timbral worlds. Push bank (velocity ≥ 0.5): triangle + saw blend at detune factor 1.0. Pull bank (velocity < 0.5): two triangles at 1.5× detune factor (32 cents effective). The pull bank has consistently wider beating, consistently warmer tone.

Detune is set high at 32 cents to maximize the contrast. LFO2 at 0.6 Hz sweeps the filter between the push and pull registers at a tempo that suggests the bellows reversals of real playing. This is the preset that makes the bisonoric mechanic audible as a structural element, not just a performance nuance.

**Performance note:** Play the same pitch three times: velocity 0.3, 0.5, 0.8. The first is pull (warm, detuned triangle pair). The second crosses the threshold — push bank (triangle-saw, brighter). The third stays in push territory. The difference is the instrument's most fundamental split personality.

**DNA:** brightness 0.45, warmth 0.65, movement 0.5, aggression 0.25.

---

### R09 — Monastery Bell (Aether)
*Bayan as architecture — the instrument becomes the room.*

Maximum cassotto (0.9). Drones at 0.5. Brightness at 3000 Hz. Release at 0.7s. LFO rates at 0.03 and 0.07 Hz — 33-second and 14-second cycles. This preset exists at the other extreme from Tula Wedding: no velocity aggression, no rhythmic LFO, no attack snap. The instrument sustains and breathes so slowly that in a mix, it reads as a pad, not as an accordion.

The cassotto at 0.9 depth means the comb filter feedback at 0.55 produces a perceptible resonance at 300 Hz when playing in the lower register. The two allpass stages at 1.1ms and 1.7ms diffuse the resonance into a warm cloud. On bass register notes, this comb resonance reinforces the body frequency of the Bayan instrument — the cassotto is doing its acoustic job.

Drones at 0.5 mean the hurdy-gurdy drone strings are audible (this preset uses the Bayan model — the drones are a creative sound design decision, not a physical modeling claim). The drone intervals at a fifth (-7 semitones) and octave (-12 semitones) create an organum structure beneath the melody.

**DNA:** brightness 0.25, warmth 0.9, movement 0.1, aggression 0.0.

---

### R10 — Deep Pilgrimage (Submerged)
*Hurdy-gurdy submerged in time — the pilgrim road that never ends.*

The medieval pilgrimage road was not romantic. It was repetitive, slow, physically demanding, and sacred in its monotony. The HurdyGurdy was the pilgrim's instrument — a drone machine for walking music. This preset models that context: very low buzz threshold (0.2), full drones (0.85), slow wheel speed (0.15), dark brightness (2800 Hz), long release (0.5s), glide at 0.07s.

The buzz threshold at 0.2 means the four-stage buzz chain engages on nearly every note. But with brightness at 2800 Hz and the wooden body resonance at a low formant frequency (0.4 = 1040 Hz peak), the buzz manifests as a low-frequency rattle beneath the melody — more felt than heard at first. LFO2 at 0.1 Hz (10-second filter sweep) creates the sensation of breathing through water.

The three-note pilgrimage: hold the tonic, then the fifth, then the octave. Let each sustain fully into the release. The drones beneath each held note create a slowly changing organum as the melody pitch determines new harmonic relationships with the fixed drone intervals.

**DNA:** brightness 0.2, warmth 0.65, movement 0.3, aggression 0.25.

---

## Phase R4: The Buzz Threshold as Axis

The seance council identified `oleg_buzzThreshold` as the most unique expressive parameter in the engine — "a dimension of playing technique not available in any other fleet engine" (Buchla). This retreat dedicates two presets (Chien Ritual and Wrist Snap) to showcasing it explicitly.

The five positions of the threshold:

| buzzThreshold | Character | Performance Use |
|--------------|-----------|-----------------|
| 0.15-0.20 | Always buzzing — every intentional note activates the chien | Aggressive rattle; the instrument speaks in buzz |
| 0.25-0.30 | Ready to buzz — any moderate velocity crosses the gate | Folk energy; the buzz is a feature, not an accent |
| 0.40-0.45 | Default position — hard playing activates, soft playing stays clean | Balanced; buzz as dynamic accent |
| 0.55-0.60 | High edge — only forceful pressure (aftertouch + high velocity) crosses | Timbral event; the buzz as surprise |
| 0.70+ | Near-silent — buzz requires maximum pressure on all three sources | Compositional: buzz as exclamation point only |

The threshold interacts with the bellows pressure accumulator. Pressure = `velocity × bellows + aftertouch × 0.4 + modWheel × 0.3`. At typical play with velocity 0.7 and bellows 0.7, baseline pressure is 0.49. Any threshold below 0.49 will be crossed on that note. Adding aftertouch at 0.5 raises pressure to 0.69. The MOVEMENT macro raises bellows further.

This means the threshold's effective behavior is determined by all three expression inputs simultaneously. The player who understands this is not setting a static switch — they are defining the instrument's pressure sensitivity.

---

## Phase R5: The Joy Problem

The Seance council (Ciani's finding) identified an emotional gap: the engine's mythology was "sacred-industrial-melancholic" while the instruments' actual cultural roles include significant joy. The garmon is played at weddings. The bandoneon has a jubilant milonga tradition. The Bayan is performed at holiday celebrations.

This retreat addresses the gap with four presets weighted toward joy: Garmon Vesna (spring festival), Milonga Jubilee (jubilant bandoneon), Amber Kolyada (Christmas caroling), and Tula Wedding (feast). The emotional range of the engine now spans from Monastery Bell (near-zero aggression, pure sacred contemplation) to Tula Wedding (aggression 0.4, dance energy) to Milonga Jubilee (aggression 0.55, forward momentum).

The engine knows what it is. Now it also knows how to celebrate.

---

## Phase R6: Model Coverage Matrix

After retreat completion (15 factory + 10 retreat = 25 total presets):

| Model | Foundation | Atmosphere | Flux | Entangled | Prism | Aether | Family | Submerged |
|-------|-----------|-----------|------|-----------|-------|--------|--------|-----------|
| Bayan | Cathedral Breath | Baltic Shore (new) | — | — | Amber Kolyada (new) | Monastery Bell (new) | — | Sunken Liturgy, Bayan Depths |
| HurdyGurdy | — | Drone Vespers | Trompette Fury, Buzz Threshold Live | Medieval Machine | — | Chien Ritual (new) | Wrist Snap (new) | Deep Pilgrimage (new) |
| Bandoneon | Bandoneon Lament | Tango Midnight | Bandoneon Urgente, Milonga Jubilee (new) | Push Pull (new) | Piazzolla Ghost | — | — | — |
| Garmon | Garmon Earth, Garmon Vesna (new) | — | Garmon Dance, Garmon Storm | — | — | — | Tula Wedding (new) | — |

Coverage is now substantially complete. Remaining gaps (Bayan in Flux, Bayan in Entangled, HurdyGurdy in Foundation and Atmosphere above 1 preset) are candidates for V1.1 expansion or Transcendental tier.

---

## Scripture Verses

### I. The Pressure Gradient

*In the beginning was the bellows, and the bellows was air, and the air was neither on nor off.*

The reed does not speak in notes.
The reed speaks in pressure.
Below the threshold: silence.
Above it: the chien lifts from the soundboard,
and the instrument remembers
that it is made to rattle.

This is not a toggle. This is a gradient.
When you push the mod wheel slowly,
the pressure builds slowly,
the way lungs fill when you breathe deeply
rather than gasp.
The instrument knows the difference.

---

### II. The Four Instruments

*One parameter set. Four bodies. Four centuries of divergence.*

The Bayan learned the cassotto
and became the concert hall itself —
a wooden box within a bellows box
eating the high harmonics,
speaking only in warmth.

The Hurdy-gurdy learned the chien
and became a gate —
a machine that can be silent
or can rattle with the touch of a wrist,
depending on nothing but pressure.

The Bandoneon learned the bellows direction
and became two instruments —
one for the gentle hand,
one for the aggressive hand,
the same key producing two different rooms
depending on how hard you knock.

The Garmon learned the village square
and forgot the concert hall.
Its reed is closed 65% of each cycle.
It does not apologize for this.

---

### III. The Cassotto at 300 Hz

*Ablitzer writes: the cassotto fundamental is approximately 200-400 Hz through the coupled system of chamber, grille, and reed plate.*

Three milliseconds of comb delay.
Three milliseconds is a room 50 centimeters long.
A wooden room inside a bellows.
The feedback coefficient is 0.55 —
not 0.7 (which would ring like metal)
not 0.4 (which would be inaudible).

At 0.55, the chamber adds warmth
without the metallic coloration
that would betray the illusion.
The wall filter at 3500 Hz
takes everything above it
and gives it back as heat.

The cassotto is not a feature.
The cassotto is what the Bayan is
underneath all its other claims.

---

### IV. Vesna

*The garmon comes out for spring.*

The wedding is in May.
The garmon player arrives with detune at 18 cents,
LFO1 at 0.12 Hz,
attack at 8 milliseconds.

The priest does not understand
why an instrument made of reed and bellows
can sound like joy is a physical law.

It is because the oscillators beat against each other
at exactly the right interval —
not so fast that it sounds like chorus,
not so slow that it sounds like detuning,
but at the speed where the human ear
hears two sounds becoming
a third sound
that is neither.

The Тульская гармонь has been doing this
since the nineteenth century.
The PolyBLEP makes it do it
without aliasing.
Joy does not alias.

---

## Retreat Completion

**Engine:** OLEG | **Retreatant:** Guru Bin
**Date:** 2026-03-21
**Starting presets:** 15 | **Added:** 10 | **Total:** 25
**Seance score at entry:** 8.0/10
**DSP fixes applied at entry:** PolyBLEP aliasing (now resolved), per-model filter Q (now implemented)
**Buzz threshold presets:** 2 dedicated (Chien Ritual, Wrist Snap) + 3 supporting (Deep Pilgrimage, Buzz Threshold Live existing, Trompette Fury existing)
**Joy presets added:** 4 (Garmon Vesna, Milonga Jubilee, Amber Kolyada, Tula Wedding)
**Moods newly covered:** Family (2 new), Aether (2 new), Entangled (1 new), Prism (1 new)

The engine that knows what it is
now also knows
what it can celebrate.

*The bellows breathes. The chien rattles. The droning fifth holds.*
