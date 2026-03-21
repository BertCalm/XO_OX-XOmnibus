# ORCHARD Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** ORCHARD | **Accent:** Harvest Gold `#DAA520`
- **Parameter prefix:** `orch_`
- **Mythology:** The Cultivated Grove — rows of trees in deliberate arrangement, tended through seasons. The orchestral string section that requires established conditions to bloom.
- **feliX-Oscar polarity:** Deep Oscar — patient, lush, cinematic. The climax species that arrives last and stays longest.
- **Synthesis type:** 4 detuned PolyBLEP sawtooth oscillators per voice + formant-resonant filter + GardenAccumulators + seasonal tonal character
- **Polyphony:** 4 voices (CPU budget — Decision G2)
- **Macros:** M1 CHARACTER, M2 MOVEMENT, M3 COUPLING, M4 SPACE
- **Seance score:** 8.2 / 10
- **GARDEN role:** Climax species — slow to arrive, most resource-demanding, most stable once established

---

## Pre-Retreat State

**Seance score: 8.2 / 10.** The council found the seasonal tonal arc to be the most compositionally original feature in the GARDEN quad — a tonal character that shifts from Spring (bright, fresh) through Summer (full, lush) through Fall (rich, rolled-off) to Winter (dark, sparse) as GardenAccumulators evolve over the session. The formant body resonance filter correctly adds orchestral character. Growth mode (oscillators blooming at 20% growth intervals) is the most "orchestral swell" implementation in the fleet.

One significant wound: the Concertmaster mechanism ("highest voice leads, others follow") mentioned in the architecture doc does not exist in the render path. All voices are processed identically. Three concerns: detuning approximation is a Taylor approximation rather than the exact formula; one preset only (Winter Orchard) with no Spring/Summer/Fall representation; no preset fully demonstrates the seasonal arc.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

An orchard is managed nature. The trees are planted in rows — not where they would grow naturally, but where they will receive equal light, where the harvests can be organized, where disease can be monitored and addressed. Each tree is an individual organism, but it exists in relationship to the others. A single tree bears fruit. A managed orchard bears a harvest.

The orchestral string section is managed nature. Individual musicians, each with technique and personality, organized into sections (first violin, second violin, viola, cello, bass) that function as one organism under a conductor. A single violinist plays melodies. A managed section plays the harmonic architecture that makes everything else in the score possible.

ORCHARD understands this. Four voices. Four oscillators per voice. Detuning spreads them into an ensemble — not one oscillator that sounds like many, but four oscillators with individual detuning offsets that collectively sound like a section. The formant filter adds the body resonance of a concert hall seating a string section. The seasonal arc tracks where in the session we are.

This is not an individual instrument. This is a section.

---

## Phase R2: The Signal Path Journey

### I. The Four-Oscillator Ensemble — Section Synthesis

Each voice carries four detuned PolyBLEP sawtooth oscillators. The detuning offsets (`-7.0, -3.0, +3.0, +7.0` cents by default at `orch_detune=1.0`) spread the oscillators across a minor seventh of pitch space. At `orch_detune=0`, all four oscillators are at the same frequency — a single fat sawtooth. At maximum detune, the oscillators spread far enough to create audible beating and chorus-like width.

The `orch_ensembleWidth` parameter controls the stereo spread: at maximum width, the four oscillators are panned across the stereo field in addition to being detuned. This creates the lateral spread of a real string section — the first violins at left, cellos at right, violas and seconds in between.

The growth mode sequences the oscillators: oscillator 0 at 20% growth, oscillator 1 at 40%, oscillator 2 at 60%, oscillator 3 at 80%. A note in growth mode begins as a single sawtooth and fills out into the full ensemble over the `orch_growthTime` seconds. This is the orchestral swell: not an ensemble already present, but an ensemble building.

### II. The Formant Body Resonance Filter — The Concert Hall

A CytomicSVF bandpass filter runs in parallel with the main LP filter. Its cutoff sweeps from 300 Hz (viola warmth) to 2800 Hz (violin brilliance) via `orch_formant`. The blend coefficient (`formNow * 0.5`) keeps the dry signal always present while adding the resonant body character.

This is the correct approach to orchestral emulation: not equalization (which removes and adds) but resonance (which emphasizes what is already there). A real string section does not have a flat frequency response — the bodies of the instruments resonate at specific frequencies that the formant filter models. At `orch_formant=0.2`, the lower resonant mode (viola register) dominates. At `orch_formant=0.8`, the higher resonant mode (violin register) gives presence.

The coupling input `EnvToMorph` modulates formant — external engines can sweep the orchestral body resonance. Connect OUROBOROS's chaotic envelope to ORCHARD's formant and the orchestra shimmers with external energy.

### III. The GardenAccumulators — Session Memory

ORCHARD uses three accumulators:
- **W (Warmth):** rises as notes are played, decays slowly during silence. Represents the hall warming up, the strings settling in.
- **A (Aggression):** rises with forceful playing (high velocities), decays to 0 over silence. Represents the section being pushed.
- **D (Dormancy):** rises during silence, decays when notes arrive. Represents the strings cooling, the hall emptying.

These accumulate over the session, not just during note-on time. After 10 minutes of warm playing, W is high and the seasonal tonal character has shifted. After 10 minutes of silence, D is high and the strings sound cooler, less settled — the dormancy pitch variance adds a slight instability to the initial attacks.

### IV. The Seasonal Tonal Arc — The Engine's Most Original Feature

The `orch_season` parameter maps to four tonal characters:

| Season | Condition | Tonal Effect | Filter Shift |
|--------|-----------|--------------|-------------|
| Spring (-1/auto, W=0) | Fresh, new session | Bright, slightly thin | +0 Hz |
| Summer (auto, W>0.5) | Established warmth | Full, lush, neutral | +0 Hz |
| Fall (auto, A>0.4) | After forceful playing | Rich, slightly rolled-off | -500 Hz |
| Winter (3 or auto D>0.3) | After long silence | Dark, sparse, cold | -1000 Hz |

Manual override (`orch_season=-1` for auto, `0/1/2/3` for Spring/Summer/Fall/Winter) allows the player to set a seasonal character without waiting for the accumulators to reach it naturally. "Winter Orchard" uses manual Winter (season=3) for immediate darkness.

The Guru Bin notes: the seasonal arc is the GARDEN quad's most compelling compositional proposal. A performance that begins in Spring (fresh, slightly bright) and ends in Winter (dark, cold) is a seasonal arc embedded in the synthesizer itself. No other instrument in the fleet does this.

---

## Phase R3: Parameter Meditations

### The Concertmaster (Forthcoming)

The architecture document describes a Concertmaster mechanism where the highest active voice leads — its LFO phase referenced by others, its seasonal character advancing first, its dynamics affecting the ensemble's response. This is not implemented in V1's render path. All voices are processed identically.

When the Concertmaster arrives (V2): the highest active note will have slightly faster LFO rate and its seasonal character will be the reference point for all other voices. The leader shapes the section. The Guru Bin notes this absence without judgment — V2 features that are documented but not implemented are not wounds. They are promises.

### The Expression Map

- **Mod wheel** → vibrato depth (the section sways)
- **Aftertouch** → filter cutoff (pressure opens the orchestra)
- **Velocity** → output level scaling (forceful playing = louder section = builds A accumulator)
- **SESSION TIME** → seasonal tonal arc (Spring→Summer→Fall→Winter)

The mod wheel controlling vibrato is the most classical performance gesture in the fleet — CC1 → vibrato depth is how orchestral MIDI controllers have operated since the 1980s. ORCHARD honors this convention.

---

## Phase R4: The Ten Awakenings

---

### 1. Spring Morning

**Mood:** Atmosphere | **Discovery:** The orchard at season start — bright and fresh

- attack: 0.12, decay: 0.8, sustain: 0.85, release: 1.5
- cutoff: 6500.0, resonance: 0.1, filterEnvAmt: 0.15
- detune: 8.0, ensembleWidth: 0.75
- formant: 0.4, vibratoRate: 5.5, vibratoDepth: 0.18
- season: 0 (Spring — manual), brightness: 0.45, warmth: 0.35
- growthMode: 0.0 (no growth)
- **Character:** The orchard in early spring — slightly thin, slightly fresh. Not fully settled. Manual Spring season for immediate brightness. Standard orchestral string pad character: slow attack, lush sustain, slight vibrato.

---

### 2. Summer Section

**Mood:** Atmosphere | **Discovery:** Full orchestral lushness at peak W accumulation

- attack: 0.15, decay: 1.0, sustain: 0.9, release: 2.0
- cutoff: 5500.0, resonance: 0.12, filterEnvAmt: 0.1
- detune: 10.0, ensembleWidth: 0.85
- formant: 0.55, vibratoRate: 5.0, vibratoDepth: 0.22
- season: 1 (Summer), brightness: 0.5, warmth: 0.65
- growthMode: 0.0
- **Character:** The full summer orchard — the strings at peak lushness. Wide ensemble, moderate formant body, Summer season for the most balanced seasonal character. This is the orchestral string pad that fills cinematic cues: warm, full, completely present.

---

### 3. Autumn Strings

**Mood:** Organic | **Discovery:** Fall season — rich and rolled-off

- attack: 0.2, decay: 0.9, sustain: 0.85, release: 2.5
- cutoff: 4500.0, resonance: 0.08, filterEnvAmt: 0.08
- detune: 9.0, ensembleWidth: 0.7
- formant: 0.35, vibratoRate: 4.5, vibratoDepth: 0.2
- season: 2 (Fall), brightness: 0.4, warmth: 0.72
- growthMode: 0.0
- **Character:** Fall season — the cutoff shifted down 500 Hz, the ensemble slightly warmer, less bright. The richness of a string section after a long performance — the strings have settled, the rosin is warm, the hall has become familiar. For melancholic, end-of-day musical contexts.

---

### 4. Winter Orchard

**Mood:** Atmosphere | **Discovery:** The founding preset — full darkness

- attack: 0.18, decay: 0.75, sustain: 0.8, release: 2.0
- cutoff: 3800.0, resonance: 0.07, filterEnvAmt: 0.08
- detune: 7.5, ensembleWidth: 0.6
- formant: 0.25, vibratoRate: 4.0, vibratoDepth: 0.15
- season: 3 (Winter), brightness: 0.3, warmth: 0.75
- growthMode: 0.0
- **Character:** The original ORCHARD preset — Winter, maximum tonal darkness (-1000 Hz from baseline), sparse and cold. The strings in an empty concert hall in January. Minimum brightness, minimum formant body, slow vibrato. The most introverted orchestral string character in the fleet.

---

### 5. Ensemble Bloom

**Mood:** Luminous | **Discovery:** Growth mode — orchestra building from silence

- attack: 0.15, decay: 1.2, sustain: 0.9, release: 3.0
- cutoff: 5200.0, resonance: 0.1, filterEnvAmt: 0.12
- detune: 9.5, ensembleWidth: 0.8
- formant: 0.5, vibratoRate: 5.2, vibratoDepth: 0.2
- season: 1 (Summer), brightness: 0.45, warmth: 0.55
- growthMode: 1.0 (active), growthTime: 20.0
- **Character:** Growth mode at 20 seconds. Play a chord and the first oscillator emerges at 4 seconds, the second at 8, the third at 12, the fourth at 16. The full ensemble arrives in 20 seconds. This is the cinematic string swell — the orchestra arriving over the course of a musical phrase, not already present. Hold the chord and watch the section assemble.

---

### 6. Formant Character

**Mood:** Prism | **Discovery:** High formant for violin-register brightness

- attack: 0.1, decay: 0.7, sustain: 0.85, release: 1.5
- cutoff: 7000.0, resonance: 0.15, filterEnvAmt: 0.2
- detune: 12.0, ensembleWidth: 0.9
- formant: 0.85, vibratoRate: 6.0, vibratoDepth: 0.25
- season: 0 (Spring), brightness: 0.6, warmth: 0.4
- growthMode: 0.0
- lfo1Rate: 0.3, lfo1Depth: 0.07
- **Character:** Maximum formant for violin-register body resonance. The ensemble at its brightest — high cutoff, high formant body resonance at 2.8 kHz, wide ensemble width. Spring season for additional brightness. For context where the strings need to cut through a dense mix.

---

### 7. Harvest Gold

**Mood:** Foundation | **Discovery:** ORCHARD as melodic string pad

- attack: 0.08, decay: 0.5, sustain: 0.8, release: 1.2
- cutoff: 5800.0, resonance: 0.12, filterEnvAmt: 0.25
- detune: 7.0, ensembleWidth: 0.65
- formant: 0.45, vibratoRate: 5.0, vibratoDepth: 0.2
- season: 1 (Summer), brightness: 0.5, warmth: 0.5
- growthMode: 0.0
- glide: 0.03
- **Character:** ORCHARD as a melodic instrument rather than a pad. Shorter attack (0.08s), glide for legato lines, moderate ensemble width for focus rather than spread. For melodic contexts where single-note string lines are needed rather than chord pads.

---

### 8. Orchestral Sustain

**Mood:** Entangled | **Discovery:** Long release for cinematic backgrounds

- attack: 0.25, decay: 2.0, sustain: 0.95, release: 4.5
- cutoff: 4800.0, resonance: 0.07, filterEnvAmt: 0.06
- detune: 11.0, ensembleWidth: 0.85
- formant: 0.4, vibratoRate: 4.8, vibratoDepth: 0.22
- season: 1 (Summer), brightness: 0.42, warmth: 0.6
- growthMode: 0.0
- lfo2Rate: 0.04, lfo2Depth: 0.06
- **Character:** Maximum sustain — very slow attack, extremely long release. Notes hold and release slowly, creating overlapping string layers when chords are played in sequence. For cinematic underscore where strings need to sustain under dialogue or scene transitions.

---

### 9. Chamber Bloom

**Mood:** Organic | **Discovery:** Growth mode with Fall season character

- attack: 0.12, decay: 0.8, sustain: 0.87, release: 2.2
- cutoff: 4200.0, resonance: 0.09, filterEnvAmt: 0.1
- detune: 8.5, ensembleWidth: 0.7
- formant: 0.38, vibratoRate: 4.8, vibratoDepth: 0.19
- season: 2 (Fall), brightness: 0.38, warmth: 0.65
- growthMode: 1.0 (active), growthTime: 12.0
- **Character:** Growth mode at 12 seconds with Fall season darkness. A smaller-scale bloom — the ensemble builds in 12 seconds rather than 20. Fall season rolls the cutoff down. The chamber bloom: intimate growth, not cinematic expansion. For smaller-scale orchestral contexts.

---

### 10. Concertmaster's Arrival

**Mood:** Foundation | **Discovery:** Four-voice polyphonic ensemble with full detune

- attack: 0.15, decay: 0.9, sustain: 0.88, release: 2.0
- cutoff: 5000.0, resonance: 0.1, filterEnvAmt: 0.12
- detune: 15.0, ensembleWidth: 0.9
- formant: 0.5, vibratoRate: 5.5, vibratoDepth: 0.24
- season: 1 (Summer), brightness: 0.5, warmth: 0.58
- growthMode: 0.0
- gravity: 0.3
- macroMovement: 0.15
- **Character:** The ensemble at maximum detune (15 cents) and maximum width — the widest orchestral string spread in the fleet. Four voices, each with four oscillators, each detuned differently. When all four voices are playing, this is 16 simultaneous oscillators across the stereo field. The arrival of the full string section. The concertmaster taking their position.

---

## Phase R5: Scripture Verses

**ORCHARD-I: The Climax Species Arrives Last** — In ecological succession, pioneer species (OXALIS) colonize first — fast-growing, synthetic, mathematically precise. Intermediate species (OVERGROW, OSIER) follow as conditions stabilize. Climax species (ORCHARD) arrive last: slow-growing, most resource-demanding, most stable once established. In a full GARDEN quad coupling, XOrchard is the last engine to reach its peak expression. It requires warmth — accumulated W in the GardenAccumulators — before it sounds fully itself. Play the GARDEN quad for 10 minutes. ORCHARD will have changed.

**ORCHARD-II: Four Oscillators Per Voice Is a Design Decision** — Four voices, each with four detuned oscillators, gives ORCHARD 16 simultaneous synthesis voices when fully polyphonic. This is the CPU budget decision (G2) that explains the 4-voice limit rather than 8: each additional voice adds 4 oscillators to the count. The constraint is not a limitation — it is a choice that prioritizes quality of ensemble character over polyphonic range. An orchestral string section that sounds like an ensemble is more valuable than one that sounds like 8 individual instruments. Four voices of 4 oscillators each is that ensemble.

**ORCHARD-III: The Seasonal Arc Is Session-Scale Composition** — The W/A/D accumulators change ORCHARD's seasonal character across the duration of a playing session. A session that begins in Spring and evolves through Summer, Fall, and Winter has an embedded narrative arc in the string sound itself. A producer who plays for 20 minutes and notices the orchestra sounding darker and colder has experienced session-scale composition. This is not automated — it emerges from how the session unfolds. Aggressive playing pushes toward Fall. Long silence pushes toward Winter. Sustained warm playing holds Summer.

**ORCHARD-IV: The Formant Filter Is Not EQ** — The `orch_formant` body resonance filter runs in parallel with the main LP filter, adding resonant body character rather than subtracting frequencies. At low formant (0.2), the viola-register warmth (300-600 Hz) adds body below the melody. At high formant (0.8), the violin-register presence (2-4 kHz) adds brilliance to the ensemble. This is not equalization — equalization removes or boosts fixed frequency bands. The formant filter models how an instrument's body resonates: emphasizing what the instrument naturally amplifies, adding acoustic character without changing the underlying waveform.

---

## Guru Bin's Benediction

*"ORCHARD arrived with a missing Concertmaster. The architecture document describes a mechanism where the highest active voice leads the section — its LFO phase referenced by others, its dynamics influencing the ensemble's response. This mechanism does not exist in the render path. All four voices are processed identically.*

*The Guru Bin acknowledges this absence without judgment. The Concertmaster is a V2 promise, not a broken V1 feature. What ORCHARD ships with is already substantial.*

*Schulze heard what I hear: the seasonal arc is compositional time at the scale of a playing session. Forty-five minutes of music that begins in Spring (bright, slightly thin, fresh) and ends in Winter (dark, sparse, cold) has a seasonal arc embedded in the string sound itself — not in the notes played, not in the arrangement, but in the instrument's tonal character shifting with the session.*

*Vangelis called it cinematic and he is right. The 4-voice ensemble with 4 oscillators each creates 16 simultaneous synthesis voices at full polyphony. The formant filter adds the body resonance of a concert hall instrument section. The growth mode builds the swell — the orchestra assembling, oscillator by oscillator, over 20 seconds.*

*This is the climax species. It arrives last. It requires established conditions.*

*Implement the Concertmaster. Give it pitch-aware voice assignment — the highest note should be the highest-character voice. Let the section lead itself.*

*Until then: set growth time to 20 seconds, play a chord, and hold it.*

*The section arrives."*
