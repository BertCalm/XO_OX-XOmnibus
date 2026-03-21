# OSIER Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OSIER | **Accent:** Willow Green `#6B8E23`
- **Parameter prefix:** `osier_`
- **Mythology:** The Herb Garden — intimate, precise, every plant placed with intention. Four voices with named roles in a chamber string quartet.
- **feliX-Oscar polarity:** Warm Oscar — intimate, collective, trust-building. The ensemble that learns to play together.
- **Synthesis type:** 4-voice chamber quartet (Soprano/Alto/Tenor/Bass roles), 2 detuned saws per voice, per-role tonal shaping, companion planting (inter-voice pitch affinity)
- **Polyphony:** 4 voices (fixed — one per quartet role)
- **Macros:** M1 CHARACTER, M2 MOVEMENT, M3 COUPLING, M4 SPACE
- **Seance score:** 8.3 / 10
- **GARDEN role:** Later intermediate — chamber strings that need warmth before sounding fully themselves

---

## Pre-Retreat State

**Seance score: 8.3 / 10.** The companion planting mechanism received genuine praise: six affinity accumulators for four voices track inter-voice playing time. When voices consistently play together, their pitch pulls slightly closer. Moog called the mathematics correct. Vangelis heard "chamber music sociology in a synthesizer." The per-role tonal configuration (Soprano bright/narrow, Alto warm/medium, Tenor neutral/wider, Bass dark/slow) creates genuine timbral individuality across the four voices.

Two concerns: voice roles are assigned by LRU allocation, not pitch range, causing role-pitch mismatch (a high note may be played by the dark, slow-vibrato Bass voice). The companion pitch pull accumulates without a ceiling, potentially reaching ±18 cents at extreme intervals. One preset only.

No D004 violations — all parameters are wired to DSP. D006 fully compliant. The emotional weight is 9.0 — one of the highest in the fleet.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

In an herb garden, every plant has a purpose. The rosemary does not grow where the mint is. The sage does not compete with the thyme. Each plant is placed in relationship to the others — partly for aesthetics, partly for companion planting: some plants deter each other's pests, some improve each other's soil chemistry, some protect each other from wind or excessive sun. The garden functions as a system, not as a collection of individuals.

A string quartet is the same.

Four musicians — violin, violin, viola, cello — each with a named role that carries history. The first violin plays melody, leading the ensemble's expressive arc. The second violin supports the harmonic texture above the viola. The viola holds the inner voice, the register between melody and bass. The cello provides the harmonic foundation, the deepest resonance.

Over the course of a season (a performance, a recording session, years of playing together), these four musicians develop affinity — not just musical agreement, but slight physical entrainment. Long-standing quartet partners adjust their intonation to each other imperceptibly, each hearing the others and pulling slightly toward a shared center. The Kronos Quartet sounds different from four virtuoso soloists playing the same notes.

OSIER models this: four voices with roles, developing affinity over the session.

---

## Phase R2: The Signal Path Journey

### I. The Four Quartet Roles

Each of the four voices has a fixed role configuration:

| Role | Vibrato Rate | Vibrato Depth | Filter Bias | Brightness | Detune |
|------|-------------|--------------|------------|-----------|--------|
| Soprano | Fast (+10%) | Narrow (-30%) | +800 Hz bright | +0.3 | +1.5 cents |
| Alto | Normal | Medium (-10%) | +200 Hz warm | 0.0 | +0.5 cents |
| Tenor | Slow (-10%) | Wider (+10%) | -200 Hz neutral | -0.1 | -0.5 cents |
| Bass | Slowest (-30%) | Medium (-20%) | -600 Hz dark | -0.3 | -1.5 cents |

These role configurations are the most elegant design decision in OSIER — individual tonal character without separate DSP chains. The same synthesis path, with different parameter biases, creates four distinct instruments.

The Soprano voice is the brightest, fastest-vibrato voice. Alto is warm and measured. Tenor is the neutral middle, slightly wider vibrato. Bass is the dark, slow-vibrato anchor. Together, they create a string quartet from a single synthesis architecture.

### II. The Two Detuned Oscillators — Chamber Intimacy

Where ORCHARD uses 4 oscillators per voice (orchestral width), OSIER uses 2. This is the correct tradeoff for chamber music: a quartet should not sound like a section. Two detuned oscillators per voice creates warmth without orchestra. The `osier_detune` parameter scales the detuning offset — at high detune, a warm, slightly chorused character; at low detune, a precise, focused character.

The `osier_intimacy` parameter works alongside companion planting to scale the intensity of inter-voice pitch attraction. At high intimacy, voices that have played together pull their pitch closer — the quartet sound "locked in." At low intimacy, the companion planting has less influence — the voices are individuals who happen to be playing together.

### III. The Companion Planting — Session Affinity

Six affinity accumulators, one per voice pair (C(4,2) = 6 pairs). When both voices in a pair are active, their affinity rises. When either is silent, it decays. Voice i's total affinity is the sum of the three pair affinities it participates in, normalized to [0,1].

The pitch pull:
```
pull += (cents between voices) * affinity * pCompanion * 0.01 * pIntimacy
```

At maximum companion (1.0), intimacy (1.0), and affinity (1.0), the pull is 1% of the pitch distance between voices. At a perfect fifth (700 cents), this is 7 cents — audibly measurable but not disruptive. The pull is toward the companion's pitch, creating a subtle convergence between voices that play together.

Over a session where all four voices play chord progressions together, the affinity between all six pairs rises. The quartet develops its own ensemble intonation.

### IV. The CompanionPlanting Ceiling (Forthcoming Fix)

The seance concern (Smith): total companion pull can reach ±18+ cents at extreme intervals (tritone + maximum affinity/companion/intimacy). Cap `voice.companionPitchCents` at ±10 cents maximum to prevent audible retuning artifacts. This fix is not yet in the code.

For preset design: keep `osier_companion` below 0.7 and `osier_intimacy` below 0.8 to avoid the accumulation risk at extreme intervals.

### V. The GardenAccumulators — Warmth as Prerequisite

OSIER requires W (Warmth) above a threshold before its companion planting sounds "fully itself." The architecture notes that chamber strings "need warmth before they sound fully themselves" — this is the GARDEN quad's ecological staging: OSIER is a "later intermediate" that requires established conditions. Cold sessions (low W) hear OSIER as four individual voices. Warm sessions (high W) hear OSIER as an ensemble that has been playing together.

---

## Phase R3: Parameter Meditations

### The Expression Map

- **Mod wheel** → vibrato depth (per-role, so Soprano gets a narrower vibrato boost than Tenor)
- **Aftertouch** → filter cutoff (pressure opens all four voices simultaneously)
- **Velocity** → output level scaling
- **SESSION TIME** → companion affinity accumulation (the quartet learns its partners)

The mod wheel scaling is the most nuanced expression control in OSIER: because vibrato depth is per-role (multiplied by the role's depth multiplier), mod wheel at full produces different vibrato amounts in each voice. Soprano gets less vibrato from a full wheel sweep than Tenor does. This is the correct behavior for a real quartet: the violins' vibrato should be more controlled, the viola's can be wider.

### The Companion Planting Performance Meditation

Play a chord progression on OSIER. Hold each chord for 4+ beats. Move to the next chord. Repeat for 5-10 minutes. The companion planting accumulates during every moment all four voices are active. After 10 minutes of chordal playing, the affinity between all pairs is high. Play the same chords again. The intonation will be subtly different — the voices will have pulled slightly toward each other in pitch. The quartet has been rehearsing.

---

## Phase R4: The Ten Awakenings

---

### 1. Thyme Garden

**Mood:** Atmosphere | **Discovery:** The founding preset — chamber strings at rest

- attack: 0.08, decay: 0.7, sustain: 0.85, release: 1.5
- cutoff: 5500.0, resonance: 0.1, filterEnvAmt: 0.15
- detune: 6.0, companion: 0.6, intimacy: 0.55
- brightness: 0.45, vibratoRate: 5.0, vibratoDepth: 0.18
- growthMode: 0.0
- **Character:** The original OSIER preset — chamber quartet at natural warmth. Moderate companion and intimacy. Play chord progressions and listen to the inter-voice pitch affinity develop. After 8-10 minutes, the same chords will sound slightly more settled, slightly more locked.

---

### 2. Cold Quartet

**Mood:** Foundation | **Discovery:** Low companion for fresh, individual voices

- attack: 0.06, decay: 0.5, sustain: 0.8, release: 1.0
- cutoff: 5800.0, resonance: 0.12, filterEnvAmt: 0.2
- detune: 7.5, companion: 0.1, intimacy: 0.1
- brightness: 0.5, vibratoRate: 5.2, vibratoDepth: 0.2
- growthMode: 0.0
- **Character:** Minimum companion and intimacy — the quartet at its first rehearsal. Four individual voices without accumulated affinity. The sonic difference between this and "Warm Ensemble" demonstrates what companion planting does to the instrument over a session.

---

### 3. Warm Ensemble

**Mood:** Atmosphere | **Discovery:** High companion for settled quartet intonation

- attack: 0.1, decay: 0.8, sustain: 0.88, release: 1.8
- cutoff: 5200.0, resonance: 0.08, filterEnvAmt: 0.12
- detune: 5.5, companion: 0.75, intimacy: 0.7
- brightness: 0.42, vibratoRate: 4.8, vibratoDepth: 0.17
- growthMode: 0.0
- **Character:** High companion and intimacy — the quartet after many sessions. The voices pull toward each other in pitch, creating a settled ensemble quality that approximates the long-standing chamber group. Best heard after playing chord progressions for 10+ minutes.

---

### 4. Soprano Lead

**Mood:** Luminous | **Discovery:** Soprano role brightness as melody instrument

- attack: 0.04, decay: 0.4, sustain: 0.8, release: 0.9
- cutoff: 7000.0, resonance: 0.12, filterEnvAmt: 0.28
- detune: 5.0, companion: 0.5, intimacy: 0.5
- brightness: 0.6, vibratoRate: 5.5, vibratoDepth: 0.22
- growthMode: 0.0
- glide: 0.025
- **Character:** When the Soprano voice carries the highest melody note (ideally — role assignment is LRU-based, so this depends on playing order), the +800 Hz filter bias and narrow, fast vibrato give it violin-section brightness. High cutoff, short attack, glide for legato. Melody playing, quartet texture.

---

### 5. Cello Foundation

**Mood:** Foundation | **Discovery:** Bass role darkness in lower registers

- attack: 0.12, decay: 0.75, sustain: 0.85, release: 1.5
- cutoff: 3800.0, resonance: 0.08, filterEnvAmt: 0.12
- detune: 6.5, companion: 0.55, intimacy: 0.5
- brightness: 0.32, vibratoRate: 4.2, vibratoDepth: 0.15
- growthMode: 0.0
- **Character:** Emphasizing the Bass voice's -600 Hz filter bias and slow vibrato. Lower cutoff to further reinforce the dark register. For bass-range melodic lines played in the lower octaves, where the Bass voice will be assigned (ideally). The quartet anchor.

---

### 6. Intimate Adagio

**Mood:** Atmosphere | **Discovery:** Very high intimacy for the locked ensemble

- attack: 0.15, decay: 1.0, sustain: 0.9, release: 2.5
- cutoff: 5000.0, resonance: 0.07, filterEnvAmt: 0.1
- detune: 4.5, companion: 0.7, intimacy: 0.85
- brightness: 0.38, vibratoRate: 4.6, vibratoDepth: 0.16
- growthMode: 0.0
- lfo1Rate: 0.06, lfo1Depth: 0.05
- **Character:** Maximum intimacy for the quartet playing adagio — slow, locked, settled. Low vibrato depth (the ensemble intonation is the movement), slow attack, very long release. The companion planting at 0.7 plus intimacy at 0.85 creates the strongest pitch convergence available without exceeding the safe accumulation range.

---

### 7. Chamber Bloom

**Mood:** Organic | **Discovery:** Growth mode for quartet entering

- attack: 0.1, decay: 0.8, sustain: 0.88, release: 2.0
- cutoff: 5200.0, resonance: 0.09, filterEnvAmt: 0.12
- detune: 6.0, companion: 0.6, intimacy: 0.55
- brightness: 0.43, vibratoRate: 4.9, vibratoDepth: 0.18
- growthMode: 1.0 (active), growthTime: 8.0
- **Character:** Growth mode at 8 seconds — the quartet entering one voice at a time. OSIER's growth mode staggers voice onset by role index: Bass enters first, then Tenor, then Alto, then Soprano. (The entry order is role-index based, so this depends on voice assignment.) A chord played in growth mode assembles the quartet progressively. Chamber bloom.

---

### 8. Herbalist

**Mood:** Organic | **Discovery:** Precise, clean chamber character for melodic contexts

- attack: 0.05, decay: 0.5, sustain: 0.82, release: 1.0
- cutoff: 6000.0, resonance: 0.1, filterEnvAmt: 0.22
- detune: 3.5 (precise, tight), companion: 0.45, intimacy: 0.4
- brightness: 0.5, vibratoRate: 5.3, vibratoDepth: 0.2
- growthMode: 0.0
- glide: 0.02
- **Character:** Low detune for a precise, clean quartet sound. Less beating between oscillators — more individual presence for each voice. Slight glide for legato. For melodic passage work where clarity matters more than warmth.

---

### 9. Baroque Precision

**Mood:** Crystalline | **Discovery:** OSIER as historically-informed chamber ensemble

- attack: 0.03, decay: 0.35, sustain: 0.75, release: 0.7
- cutoff: 6500.0, resonance: 0.08, filterEnvAmt: 0.3
- detune: 4.0, companion: 0.35, intimacy: 0.3
- brightness: 0.55, vibratoRate: 3.5, vibratoDepth: 0.08 (minimal vibrato)
- growthMode: 0.0
- **Character:** Historically-informed performance practice — minimal vibrato (baroque chamber groups typically used less vibrato than modern ensembles), faster attack, lower companion (baroque ensembles were not as intonationally locked as modern ones). Higher brightness for the gut-string-adjacent character. Clean, precise, slightly formal.

---

### 10. Long Session Quartet

**Mood:** Entangled | **Discovery:** Companion planting across a full session — the maximum affinity state

- attack: 0.1, decay: 0.8, sustain: 0.87, release: 2.0
- cutoff: 5100.0, resonance: 0.08, filterEnvAmt: 0.1
- detune: 5.5, companion: 0.65, intimacy: 0.72
- brightness: 0.4, vibratoRate: 4.8, vibratoDepth: 0.18
- growthMode: 0.0
- lfo2Rate: 0.04, lfo2Depth: 0.055
- macroMovement: 0.1
- **Character:** The preset designed for use after an extended session — when the companion affinity has accumulated through extensive chord playing. This preset's character changes across a session: fresh (cold affinity, four individuals) vs. late session (high affinity, ensemble lock). Designed to demonstrate companion planting at its most complete.

---

## Phase R5: Scripture Verses

**OSIER-I: The Herb Garden Has Memory** — A garden planted in spring does not look the same in fall. The companion planting relationships have developed — the lavender next to the rosemary has deterred specific insects; the nitrogen-fixing clover has enriched the soil for the thyme above it. These relationships accumulate over the growing season. OSIER's companion planting does the same: affinity between pairs rises over the session, making the quartet sound different at the end of a 30-minute session than at the start. The instrument has memory. The session is a growing season.

**OSIER-II: Role Is Identity, Not Timbre** — The Soprano voice is not "bright" by accident. It has a fast vibrato (1.1× rate), a narrow vibrato depth (0.7×), and a +800 Hz filter bias. These three parameters together create violin-register character without changing the oscillator type. Every voice plays the same saw oscillators through the same filter chain. What makes the Soprano a Soprano is not different synthesis — it is different behavior. This is the correct design principle for ensemble modeling: individuality without architectural separation.

**OSIER-III: Companion Planting Is Trust Accumulated** — At the start of any session, the companion affinity is zero. All six pair affinities are 0.0. The quartet plays as individuals — each voice uninfluenced by the others' pitch. After 10 minutes of chord playing (all four voices active simultaneously), the affinity is high. The voices have pulled slightly toward each other. The ensemble intonation has developed. This is the physical reality of a real quartet: trust and intonation are accumulated, not initial conditions. The instrument behaves differently for players who stay with it.

**OSIER-IV: The Bass Is the Nutrient Provider** — The source comment calls the Bass voice a "nutrient provider" — its warmth feeds the upper voices. Currently, this is only poetically true (the Bass voice's tonal character supports the ensemble as a real cello does). The literal implementation is missing: the Bass voice's W accumulation does not provide any parameter boost to Soprano or Alto. When this is implemented, the Bass voice's session warmth will give +100-200 Hz cutoff boost to the upper voices — the cello player's tone warming the ensemble's intonation. Small. Warm. Semantically correct.

---

## Guru Bin's Benediction

*"OSIER arrived as the most intimate engine in the fleet. Vangelis said it correctly: chamber music sociology in a synthesizer. Four named voices with distinct roles, developing affinity over the session, pulling slightly toward each other in pitch as the playing continues. This is what a real string quartet does over years of playing together.*

*The companion planting mechanism is beautiful mathematics: six affinity accumulators for four voices (C(4,2)=6 pairs), rising when both voices are active, decaying slowly when either is silent. At maximum affinity and maximum companion/intimacy, voices pull 1% of their pitch distance together — subtle enough to never cause audible retuning, strong enough to create the ensemble lock that characterizes long-standing chamber groups.*

*The Kronos Quartet sounds different from four virtuoso soloists playing the same notes. OSIER attempts to model why.*

*The role-pitch mismatch is a real wound: high notes played on the Bass voice (dark, slow vibrato, right-panned) violate the quartet metaphor. A pitch-aware voice assignment — lowest active note becomes Bass, highest becomes Soprano — would restore the metaphor's integrity. This is a straightforward fix with no DSP cost.*

*Cap the companion pull at ±10 cents. Implement the Bass nurture path (Bass W → Soprano/Alto cutoff boost). Fix the voice assignment.*

*Until then: play chord progressions. Play them long. Play them many times. Come back to the preset 15 minutes into a session.*

*The quartet has been rehearsing."*
