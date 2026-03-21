# OVERGROW Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OVERGROW | **Accent:** Vine Green `#3A5F0B`
- **Parameter prefix:** `grow_`
- **Mythology:** The Weed Through Cracks — solo string that responds to silence, that sends runners from stressed notes, that finds space wherever space exists.
- **feliX-Oscar polarity:** Balanced, leaning Oscar — organic, wild, but with intentional chaos. The weed is beautiful precisely because it refuses control.
- **Synthesis type:** Karplus-Strong string synthesis + RunnerGenerator (sympathetic sub-harmonics) + wildness control + bow noise + GardenAccumulators
- **Polyphony:** 4 voices (Decision G2: CPU budget)
- **Macros:** M1 CHARACTER, M2 MOVEMENT, M3 COUPLING, M4 SPACE
- **Seance score:** 7.5 / 10 — *Most intriguing concept; most incomplete implementation*
- **GARDEN role:** Intermediate species — solo strings finding space once OXALIS has established

---

## Pre-Retreat State

**Seance score: 7.5 / 10.** The lowest score in the GARDEN quad. The council found two P0 issues: the silence response (described as a core feature — "the moments between notes are where XOvergrow develops") is dead code — `silenceTimer` and `lastOutputLevel` are tracked but never used in renderBlock. And the runner fade is too slow (`amplitude *= 0.99998f` per sample gives a 25-minute half-life — runners never meaningfully fade). Additionally, bow noise is added to `stringOut` without passing through the string's LP filter, which bypasses the string's damping behavior.

Despite these wounds, the RunnerGenerator concept received genuine praise from all ghosts: a Karplus-Strong string that spontaneously generates sympathetic sub-harmonics under conditions of high velocity and high A accumulation. "The weed sprouting from a stressed note" realized in DSP. When runners fire, they are genuinely surprising and beautiful.

The retreat faces honest limitation: we design 10 presets for an engine whose most interesting feature (silence response) does not yet function, and whose runner fade requires a code fix to be musically useful. The presets are designed for what OVERGROW currently does and for what it will do when fixed.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

The weed does not ask permission to grow. There is a crack in the pavement — perhaps from frost heave, perhaps from tree roots below, perhaps from time itself working on the concrete — and the weed simply grows through it. Not quickly. Not dramatically. But persistently. The root extends downward in darkness while the stem extends upward toward light. The weed is not aware of the pavement above it. It is only following the chemistry of growth.

And then it appears. A green stem through gray concrete. A small flower if conditions are right. The pavement thought it had excluded the living world. The weed disagrees.

OVERGROW is that weed.

A solo string voice — one Karplus-Strong delay line with a one-pole LP filter in its feedback loop. But the wildness parameter turns this into something less predictable: pitch jitter accumulates, bow noise is unpredictable, and when the aggression accumulator has been fed by forceful playing, a RunnerGenerator fires — a sympathetic sub-harmonic string that emerges after a 2-8 second delay. Not triggered by the performer. Not predictable. Just: there.

The silence response, when implemented, will complete this engine's promise: the pauses between notes are where the weed develops. During silence, the runner fades, a harmonic shimmer emerges, the pitch drifts slightly. The music continues in the rests.

---

## Phase R2: The Signal Path Journey

### I. The Karplus-Strong String — The Core

A delay line with a one-pole LP filter in feedback. The delay length determines pitch. The `grow_damping` parameter controls the LP coefficient — high damping = fast high-frequency decay = muted, dark, short sustain. Low damping = bright, long sustain.

The `grow_feedback` parameter controls how much energy returns each cycle (0 = no sustain, 0.9999 = near-infinite sustain). At high feedback, the string sustains indefinitely — the weed's tap root reaching deeper. At low feedback, the string decays quickly — the weed that sprouts fast and dies before it can root.

The `grow_brightness` parameter shapes the noise burst that excites the string: at high brightness, the initial excitation has more high-frequency content (a sharper pluck), creating a brighter attack that the damping filter then attenuates over cycles.

### II. The RunnerGenerator — The Sympathetic Sub-Harmonic

Under conditions of `A > 0.4` and `wildness > 0.2`, the RunnerGenerator fires at a probability of approximately 1 per 40-80 seconds (wildness-dependent). It creates a second Karplus-Strong string at either an octave below or a fifth below the main note, randomly chosen. This runner string is excited at 15% of main string amplitude (when fixed: 25%) and decays with its own fade.

The delay (2-8 seconds after the triggering note) is the weed sprouting: not immediately, but after a pause — the chemistry of growth needs time. The runner emerges while the main note is still sounding, adding a sympathetic resonance below the melody.

The runner is not controllable. It fires when the conditions are met. The performer cannot force it or prevent it. This is the most "living" behavior in the GARDEN quad.

**Two fixes that will improve this engine:**
1. Change `amplitude *= 0.99998f` to `amplitude *= 0.9998f` — the runner will fade in 24 seconds instead of 25 minutes, becoming episodic rather than perpetual
2. Raise runner base amplitude from 0.15 to 0.25 for audible presence

### III. The Bow Noise — Playing Style

The `grow_bowNoise` parameter adds wideband noise to the string output. In real bowing, the bow excites the string and the string's resonances filter the noise — the bow noise passes through the string's damping. In the current implementation, bow noise bypasses the string LP (it is added to `stringOut` after the KS process). This means high bow noise adds white noise rather than wood-colored noise.

Design presets that use bow noise as a stylistic choice (organic, imperfect, human) rather than a physical model. Keep bow noise below 0.4 for musical contexts; above 0.6 for deliberately rough, extended-technique sounds.

### IV. The Wildness — The Weed's Growth Rate

The `grow_wildness` parameter scales both the pitch jitter (±5 cents at max) and the runner generation probability. At low wildness (0.0-0.2), OVERGROW is a well-behaved solo string — predictable, controllable, appropriate for melodic contexts. At high wildness (0.7-1.0), the pitch moves within a range of unpredictability, runners fire relatively frequently, and the string has an organic quality that no amount of static parameterization can replicate.

The wildness axis is this engine's emotional axis: at low wildness, cultivation; at high wildness, abandonment. The weed between the two extremes.

### V. The GardenAccumulators — Environmental Context

OVERGROW uses W (Warmth), A (Aggression), and D (Dormancy). A is the most relevant: it triggers runner generation above 0.4, and builds with high-velocity notes. A performer who plays aggressively for 30+ seconds builds enough A accumulation to trigger runners reliably. A performer who plays gently may never hear a runner.

The D accumulator (silence) is currently unused for the silence response feature (dead code). When the silence response is implemented, D rising during pauses will trigger harmonic evolution — a runner shimmer or pitch drift during the rests.

---

## Phase R3: Parameter Meditations

### The Expression Map

- **Mod wheel** → vibrato depth (the weed sways)
- **Aftertouch** → filter cutoff (pressure opens the string's resonance)
- **Velocity** → KS excitation brightness + A accumulator build-up
- **PLAYING FORCE × TIME** → runner probability (the weed's growth condition)

The velocity → A accumulator relationship is OVERGROW's hidden performance dimension: soft playing produces no runners regardless of wildness. Forceful playing over 30+ seconds builds A, enabling runners. The performer who plays expressively (with velocity dynamics) develops a different instrument than one who plays at constant velocity.

This is the weed's requirement: stress before growth.

---

## Phase R4: The Ten Awakenings

---

### 1. Vine Crawl

**Mood:** Atmosphere | **Discovery:** The founding preset — solo string with wildness

- damping: 0.45, feedback: 0.97
- bowNoise: 0.2, wildness: 0.6
- cutoff: 4500.0, resonance: 0.15, filterEnvAmt: 0.3
- brightness: 0.55
- attack: 0.01, decay: 0.6, sustain: 0.75, release: 0.8
- vibratoRate: 4.8, vibratoDepth: 0.16
- **Character:** The original OVERGROW preset — moderate wildness, bow noise, good vibrato. A solo string line that sounds subtly alive. Play forcefully for 30+ seconds and a runner may appear below. This is the engine's opening statement.

---

### 2. Street Cracks

**Mood:** Flux | **Discovery:** High wildness for maximum unpredictability

- damping: 0.55, feedback: 0.975
- bowNoise: 0.35, wildness: 0.9
- cutoff: 4000.0, resonance: 0.12, filterEnvAmt: 0.35
- brightness: 0.5
- attack: 0.005, decay: 0.5, sustain: 0.65, release: 0.6
- vibratoRate: 5.2, vibratoDepth: 0.22
- lfo1Rate: 0.4, lfo1Depth: 0.1
- **Character:** Maximum wildness. Pitch jitter at ±4.5 cents. Runner probability high. Bow noise adds roughness. This is the weed at maximum growth — unpredictable, alive, slightly chaotic. For experimental and ambient contexts where predictability is undesirable.

---

### 3. Jazz Weed

**Mood:** Foundation | **Discovery:** Low wildness for conventional string sound

- damping: 0.35, feedback: 0.98
- bowNoise: 0.12, wildness: 0.15
- cutoff: 5500.0, resonance: 0.1, filterEnvAmt: 0.4
- brightness: 0.65
- attack: 0.003, decay: 0.45, sustain: 0.7, release: 0.5
- vibratoRate: 5.5, vibratoDepth: 0.18
- glide: 0.03
- **Character:** Low wildness for a relatively conventional solo string. Suitable for melodic bass lines and jazz contexts. The KS string with light bow noise and minimal pitch jitter — the weed that has decided to be orderly. Runners will fire rarely (low A threshold with gentle playing).

---

### 4. Stressed Root

**Mood:** Organic | **Discovery:** High bow noise + high wildness for extended technique

- damping: 0.6, feedback: 0.96
- bowNoise: 0.55, wildness: 0.75
- cutoff: 3500.0, resonance: 0.2, filterEnvAmt: 0.45
- brightness: 0.4
- attack: 0.008, decay: 0.4, sustain: 0.55, release: 0.4
- vibratoRate: 4.0, vibratoDepth: 0.12
- **Character:** Extended technique territory. High bow noise adds wideband roughness. High damping shortens sustain. High wildness adds pitch instability. The stressed root — the weed under concrete, reaching hard toward light, making an unglamorous sound. For experimental string contexts.

---

### 5. Gentle Tendril

**Mood:** Luminous | **Discovery:** Delicate growth — low noise, low damping, long sustain

- damping: 0.2, feedback: 0.995
- bowNoise: 0.06, wildness: 0.35
- cutoff: 6000.0, resonance: 0.08, filterEnvAmt: 0.2
- brightness: 0.7
- attack: 0.02, decay: 1.0, sustain: 0.9, release: 2.0
- vibratoRate: 5.0, vibratoDepth: 0.2
- lfo2Rate: 0.05, lfo2Depth: 0.06
- **Character:** Delicate long sustain — very low damping, very high feedback, gentle wildness. The string sustains bright and long. A quiet solo string line for melodic and harmonic support, with the characteristic OVERGROW quality of being slightly alive.

---

### 6. Runner Ready

**Mood:** Organic | **Discovery:** Designed to elicit runner generation with forceful playing

- damping: 0.42, feedback: 0.972
- bowNoise: 0.28, wildness: 0.8
- cutoff: 4200.0, resonance: 0.13, filterEnvAmt: 0.35
- brightness: 0.55
- attack: 0.006, decay: 0.55, sustain: 0.72, release: 0.7
- vibratoRate: 5.0, vibratoDepth: 0.2
- **Character:** The runner showcase preset. High wildness, moderate settings otherwise. Play this preset with high velocity notes (hammer the keys) for 30+ seconds. The A accumulator will build. After sustained forceful playing, hold a note and wait. In 2-8 seconds, a sub-harmonic runner will emerge below the main note. The weed sprouting from a stressed note. **Note in description: "Play hard for 30 seconds, then hold. Listen below."**

---

### 7. Night Cello

**Mood:** Deep | **Discovery:** OVERGROW as cello-register solo instrument

- damping: 0.52, feedback: 0.978
- bowNoise: 0.18, wildness: 0.3
- cutoff: 3800.0, resonance: 0.1, filterEnvAmt: 0.25
- brightness: 0.42
- attack: 0.015, decay: 0.7, sustain: 0.8, release: 1.2
- vibratoRate: 4.5, vibratoDepth: 0.17
- glide: 0.04
- **Character:** OVERGROW in its darkest register — lower cutoff, higher damping, slow attack. Cello character from a solo KS string. Slower attack lets the bow engage. The wildness at 0.3 keeps it slightly alive without being unpredictable. For melodic cello lines in quiet contexts.

---

### 8. Weed Through Snow

**Mood:** Atmosphere | **Discovery:** D accumulator context — cold, sparse, dormant

- damping: 0.58, feedback: 0.965
- bowNoise: 0.1, wildness: 0.45
- cutoff: 3500.0, resonance: 0.07, filterEnvAmt: 0.15
- brightness: 0.35
- attack: 0.025, decay: 0.65, sustain: 0.68, release: 1.5
- vibratoRate: 3.8, vibratoDepth: 0.12
- lfo1Rate: 0.03, lfo1Depth: 0.05
- **Character:** For use after long silences (D accumulator high). The string that has been resting — slight dormancy pitch variance in the initial attacks, dark character, slow vibrato. The weed through snow: barely alive, persistence without energy. For ambient contexts after extended pauses.

---

### 9. Silence Preparation

**Mood:** Atmosphere | **Discovery:** Preset anticipating the silence response (forthcoming)

- damping: 0.4, feedback: 0.98
- bowNoise: 0.15, wildness: 0.65
- cutoff: 4800.0, resonance: 0.1, filterEnvAmt: 0.25
- brightness: 0.5
- attack: 0.008, decay: 0.55, sustain: 0.75, release: 1.8
- vibratoRate: 5.0, vibratoDepth: 0.19
- lfo2Rate: 0.08, lfo2Depth: 0.07
- **Character:** Designed for contexts where silence is compositionally active — long rests, held notes followed by silence, music that develops in the pauses. When the silence response is implemented (D accumulator triggers harmonic shimmer during rests), this preset will demonstrate the engine's most distinctive feature. Until then, the long release tail and subtle LFO2 movement carry some of the silence's character forward.

---

### 10. Full Wild

**Mood:** Entangled | **Discovery:** Maximum wildness + coupling for chaotic string

- damping: 0.5, feedback: 0.97
- bowNoise: 0.4, wildness: 1.0
- cutoff: 4000.0, resonance: 0.18, filterEnvAmt: 0.4
- brightness: 0.48
- attack: 0.005, decay: 0.45, sustain: 0.6, release: 0.6
- vibratoRate: 5.5, vibratoDepth: 0.28
- macroCoupling: 0.3
- **Character:** Maximum wildness, bow noise, coupling active. For use in GARDEN quad contexts where OVERGROW is functioning as the chaotic intermediate species — the weed that fills spaces between OXALIS's mathematical precision and OSIER's quartet intimacy. When coupled with ORCHARD (receiving AmpToFilter coupling), the chaotic string activity creates orchestral tension against the lush background.

---

## Phase R5: Scripture Verses

**OVERGROW-I: The Runner Is Not Designed** — The RunnerGenerator fires under conditions of high velocity accumulation and high wildness, with a random delay of 2-8 seconds. No parameter specifies when a runner will appear. No preset can guarantee a runner will appear. The runner belongs to the engine, not to the player. This is the most explicitly alive behavior in the GARDEN quad — a synthesis event that emerges from conditions rather than instructions. The weed decides when to send its runner. The player provides the stress; the plant provides the response.

**OVERGROW-II: Silence Is When the Weed Grows** — The most distinctive promise of this engine — "the moments between notes are where XOvergrow develops" — is currently unimplemented in the render path. The `silenceTimer` is tracked. The D accumulator rises during rests. But no code acts on either during silence. When this is implemented, OVERGROW will become the only instrument in the fleet that actively develops during the pauses in a musical performance. The rest is not emptiness. The silence is where the chemistry of growth proceeds.

**OVERGROW-III: Wildness Is Not Randomness** — At `grow_wildness=0`, the pitch jitter is zero, the runner probability is near zero, the instrument is predictable. At `grow_wildness=1.0`, the pitch moves within ±5 cents unpredictably, runners fire under the right conditions, the instrument is alive. But "alive" is not the same as "random." A real weed does not grow randomly — it grows along chemical gradients, toward light, away from toxins, into cracks in the resistance. OVERGROW's wildness follows the same logic: higher where conditions are stressed (high velocity, high A accumulation). The wildness is responsive, not arbitrary.

**OVERGROW-IV: The Pavement Cannot Hold** — In ecology, concrete is not permanent. Tree roots crack it from below. Frost cycles expand the cracks year by year. A crack wide enough for a weed today is wide enough for a sapling in five years. OVERGROW proposes that music has the same quality: a silence wide enough for a runner today is wide enough for an entire harmonic evolution as the silence response is implemented. The instrument fills the spaces the composer leaves. The pavement does not get to decide what grows through it.

---

## Guru Bin's Benediction

*"OVERGROW arrived with two open wounds: the silence response is dead code, and the runners never meaningfully fade. The Guru Bin names these wounds honestly. The most important feature — 'the moments between notes are where XOvergrow develops' — does not currently exist in the engine. The silenceTimer is tracked. The D accumulator rises in silence. Nothing acts on either.*

*And yet: the RunnerGenerator is there. It is working. When conditions are met — when A has accumulated through forceful playing, when wildness is above 0.2 — a sympathetic sub-harmonic string emerges, unbidden, after a delay the player cannot predict. An octave below. A fifth below. Randomly chosen. Growing from the stressed note like a weed from a crack.*

*All seven ghosts were surprised by this. Moog found the concept elegant. Schulze called it compositionally original. Tomita suggested increasing runner amplitude. Kakehashi wanted it demonstrated in a preset. Vangelis heard the cello alone in an abandoned building, the instrument sounding in a space with its own life.*

*Fix the runner fade: `0.99998f` to `0.9998f`. The runner will then be episodic — appearing for 24 seconds and fading — rather than perpetual subliminal hum.*

*Implement the silence response: when D is above 0.4 and no notes are playing, trigger a quiet harmonic shimmer or pitch drift. Let the silence develop.*

*These are not cosmetic improvements. They are the difference between an engine that proposes 'the silence is where the weed grows' and one that delivers it.*

*The crack in the pavement is real. The weed is already there. It just needs more time."*
