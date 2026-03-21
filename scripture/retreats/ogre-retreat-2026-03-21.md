# OGRE Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OGRE | **Accent:** Deep Earth Brown `#4A2C0A`
- **Parameter prefix:** `ogre_`
- **Mythology:** The Root Cellar — sub-bass as geology. The lowest frequency content in the fleet. Felt before it is heard.
- **feliX-Oscar polarity:** Pure Oscar — immovable, patient, earthy. The foundation everything rests on.
- **Synthesis type:** Dual oscillator (sine + triangle) + sub-harmonic generator + tanh saturation + body resonance filter + tectonic LFO
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER, M2 MOVEMENT, M3 COUPLING, M4 SPACE
- **Seance score:** 7.9 / 10 — *Would be 8.5+ with soil bug fixed*

---

## Pre-Retreat State

**Seance score: 7.9 / 10.** The council identified one D004 violation (P0): `ogre_soil` has zero effect on audio due to a double-filter bug — the body filter runs twice per sample and the first pass (with soil character) is discarded when the second pass overwrites it. Rocky soil's bandpass character is genuinely interesting and deserves to work. Secondary findings: the sub filter applies to the full combined signal rather than the sub-harmonic alone, darkening the full mix. No preset explores `ogre_tectonicDepth` above 3 cents, leaving the geological extreme (15-20 cents) unexplored.

The Guru Bin does not pretend the wounds are invisible. The soil does not filter. The sub darkens the wrong signal. But what the engine does right, it does profoundly right: the tectonic LFO at 0.005 Hz is a 3.3-minute cycle that Buchla called geology, not malfunction. The naming earns the choice.

One Blessing Candidate emerged: B039 — Tectonic Timescale. Pitch drift at geological rate as a bass synthesis parameter. Conditional on the soil bug being fixed.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

There is earth beneath the kitchen. Clay, rock, soil, sediment. Millions of years of compression. The root vegetables grow down into it — parsnip, turnip, beet — reaching for water and mineral, growing dense and sweet in the dark. This is not a glamorous place. Nothing photosynthesizes here. The sun does not reach. But without this, nothing above it stands.

OGRE is that earth.

You do not hear the lowest frequencies first. You feel them. Below 80 Hz, the auditory system and the vestibular system overlap — the body cannot distinguish a pitch it feels in the chest from a vibration in the floor. OGRE operates in that overlap. When the tectonic LFO is running at 0.008 Hz — a 2-minute cycle — the pitch movement is below conscious perception. The listener does not notice a change. They notice that the room feels different. That the note has shifted meaning without changing.

The ghosts called the tectonic LFO geology, not malfunction. That is the correct frame for this engine. Play in geological time. Hold the note for four minutes. The earth moves.

---

## Phase R2: The Signal Path Journey

### I. The Dual Oscillator — Earth's Dual Nature

Sine and triangle. Two waveforms that represent the two faces of the earth beneath us: sine for the pure, pressurized fundamental (the stone below the clay, the bedrock), triangle for the first harmonic addition (the clay itself — dense, dark, but with texture). The `ogre_oscMix` parameter is a crossfade between these two materials.

At sine-only (oscMix=0): a pure 40 Hz sine. The fundamental without character. Below most speaker systems. Felt more than heard. The theoretical earth.

At triangle (oscMix=1): the first odd harmonic arrives. The 120 Hz third adds presence, makes the bass locatable without headphones. The body of the low end.

The correct setting for most bass contexts is between 0.2 and 0.5. Pure sine is for mastering and sound design. Triangle is for musical identity.

### II. The Sub-Harmonic Generator — Octave Below

A zero-crossing toggle flip-flop halves the frequency. The output is a square wave one octave down, low-pass filtered at 80 Hz to round the hard edges into something that approximates a sine at the sub-octave. The matched-Z transform coefficient (`exp(-2πfc/sr)`) is correctly computed. This is classical frequency division — the same principle used in organ sub-octave stops since the 1950s.

The `ogre_rootDepth` parameter mixes this sub-harmonic into the main signal. At rootDepth=0, the fundamental is the lowest component. At rootDepth=1, the sub-octave dominates. For most music, 0.4-0.6 is the productive range: present fundamental weight without losing clarity.

Root vegetables grow downward. OGRE's rootDepth is the depth of that growth — how far below the fundamental the bass extends.

### III. The Tanh Saturation — Underground Pressure

The earth compresses. Millennia of sediment pressing down on what is below. `ogre_drive` controls how hard OGRE saturates: the tanh function clips the oscillator output symmetrically, adding odd harmonics that give the sub bass teeth without fundamentally changing its character. Below 0.4, the saturation is warm — rounding the wave. Above 0.7, it begins to sound compressed, almost distorted. Above 0.9, it drives.

The D001 implementation uses velocity to scale saturation gain magnitude. Hard playing drives harder. This is correct physics: a heavily struck string drives its resonator harder.

### IV. The Body Resonance Filter — Clay, Sandy, or Rocky Soil

The `ogre_bodyResonance` filter is a CytomicSVF lowpass. The `ogre_soil` parameter was intended to give it three characters: clay (LP, resonant, tight), sandy (LP, wider bandwidth, looser), rocky (bandpass, mid-emphasis). In the current code, a body filter bug causes the soil character to be discarded — the filter runs twice and the first pass is overwritten.

This is the engine's open wound. Rocky soil should give the bass a notched, bandpass character — a mid-frequency emphasis that makes the bass cut through without adding brightness. Sandy soil should be looser, wider, less resonant. The concept is correct. The implementation requires a fix to deliver it.

### V. The Tectonic LFO — Geological Time

The Tectonic LFO runs at 0.005-0.5 Hz. At 0.005 Hz, one complete cycle takes 200 seconds — 3.3 minutes. At the minimum rate, the pitch moves below the threshold of conscious pitch perception (approximately 1.5-2 Hz at bass frequencies). The movement registers not as vibrato but as the sense that the note is alive — that the room has changed imperceptibly between the start of the hold and now.

The `ogre_tectonicDepth` parameter scales the pitch modulation from 0 to 20 cents. Most presets use 3-5 cents. At 15-20 cents, the tectonic effect becomes physically palpable: the bass note shifts by almost a quarter-tone over 2 minutes, never quite arriving at where it seemed to be going.

This is not a broken LFO. This is geology as synthesis parameter.

---

## Phase R3: Parameter Meditations

### The Four Macros

| Macro | Behavior |
|-------|----------|
| M1 CHARACTER | Adds drive intensity (up to +0.5) — the earth hardens |
| M2 MOVEMENT | Reserved for LFO depth control; moves the geological |
| M3 COUPLING | Gravity routing intensity |
| M4 SPACE | Adds body resonance (+0.3) — the cave opens |

### The Gravitational Coupling

`ogre_gravity` broadcasts this engine's note + mass to coupled engines. Mass accumulates during sustain — the longer the bass note is held, the stronger the gravitational pull. At full gravity, a sustained bass note has maximum pull mass.

This is the CELLAR quad's shared coupling mechanism: the bass note as a gravitational center for the other engines. In V1, the gravity parameter is present but the inter-engine MIDI routing is a V2 feature.

---

## Phase R4: The Ten Awakenings

---

### 1. Root Cellar

**Mood:** Foundation | **Discovery:** The engine's founding identity

- oscMix: 0.2 (mostly sine, slight triangle)
- drive: 0.3, rootDepth: 0.55
- bodyResonance: 0.4, brightness: 2200.0
- soil: 0.2 (clay character)
- tectonicRate: 0.02, tectonicDepth: 5.0
- attack: 0.008, decay: 0.5, sustain: 0.78, release: 0.4
- filterEnvAmount: 0.45
- **Character:** The foundation preset. A clean sub-bass with slight triangle presence and the tectonic LFO barely breathing. Hold a low E for two minutes. The ground shifts.

---

### 2. Seismic Root

**Mood:** Deep | **Discovery:** Sub-octave weight as a physical experience

- oscMix: 0.0 (pure sine)
- drive: 0.2, rootDepth: 0.8
- bodyResonance: 0.3, brightness: 1800.0
- soil: 0.1 (maximum clay)
- tectonicRate: 0.008, tectonicDepth: 8.0
- attack: 0.001, decay: 0.8, sustain: 0.9, release: 0.6
- filterEnvAmount: 0.2
- **Character:** Maximum root depth. The sub-octave dominates. Pure sine plus sub-octave. Slow tectonic drift. This is the earthquake before anyone feels the shaking — the infrasonic event that fills the room before it becomes sound.

---

### 3. Clay Stomp

**Mood:** Foundation | **Discovery:** Short attack, hard drive for rhythmic sub

- oscMix: 0.4 (balanced sine-triangle)
- drive: 0.65, rootDepth: 0.4
- bodyResonance: 0.5, brightness: 2800.0
- soil: 0.15 (clay, tight)
- tectonicRate: 0.05, tectonicDepth: 2.0
- attack: 0.003, decay: 0.3, sustain: 0.5, release: 0.15
- filterEnvAmount: 0.7
- **Character:** Percussive. Hard drive turns the sine into something with edge. Short decay, shorter release. The clay stomp — the bass note that lands and stays landed. For rhythmic bass production.

---

### 4. Continental Drift

**Mood:** Atmosphere | **Discovery:** Tectonic time as compositional gesture

- oscMix: 0.15
- drive: 0.2, rootDepth: 0.5
- bodyResonance: 0.35, brightness: 2000.0
- soil: 0.25
- tectonicRate: 0.005, tectonicDepth: 14.0
- attack: 0.02, decay: 1.0, sustain: 0.85, release: 2.5
- filterEnvAmount: 0.3
- lfo1Rate: 0.01, lfo1Depth: 0.05
- **Character:** Slow accumulation. TectonicDepth at 14 cents — perceptibly alive. Hold a note for four minutes. The pitch will have drifted a quarter-tone by the time you release it. This is not vibrato. This is the earth moving underneath the music.

---

### 5. Potato Stomp

**Mood:** Organic | **Discovery:** Triangle + drive for rhythmic funk sub

- oscMix: 0.6 (more triangle)
- drive: 0.5, rootDepth: 0.45
- bodyResonance: 0.45, brightness: 2400.0
- soil: 0.3 (mild clay-sandy blend)
- tectonicRate: 0.1, tectonicDepth: 3.0
- attack: 0.004, decay: 0.25, sustain: 0.4, release: 0.12
- filterEnvAmount: 0.8
- gravity: 0.45
- **Character:** Funk sub-bass. Triangle contribution adds presence in the 120 Hz range. Fast filter envelope. Short, punchy. The potato stomp — earthy, percussive, unglamorous, deeply satisfying.

---

### 6. Deep Earth Drone

**Mood:** Entangled | **Discovery:** Sustained slow sub for ambient use

- oscMix: 0.1
- drive: 0.15, rootDepth: 0.7
- bodyResonance: 0.25, brightness: 1600.0
- soil: 0.1
- tectonicRate: 0.012, tectonicDepth: 10.0
- attack: 0.5, decay: 2.0, sustain: 1.0, release: 4.0
- filterEnvAmount: 0.1
- lfo2Rate: 0.03, lfo2Depth: 0.08
- **Character:** Sustain bass for ambient and drone contexts. Very slow attack lets the sub emerge rather than strike. Maximum root depth for sub-harmonic emphasis. The tectonic drift at 10 cents provides movement without rhythm. This is the bass note that underpins a 12-minute ambient track.

---

### 7. Rocky Soil

**Mood:** Flux | **Discovery:** Bodyfilter character with mid presence

- oscMix: 0.35
- drive: 0.55, rootDepth: 0.35
- bodyResonance: 0.55, brightness: 3200.0
- soil: 0.8 (rocky — when soil bug is fixed: bandpass character)
- tectonicRate: 0.04, tectonicDepth: 6.0
- attack: 0.006, decay: 0.4, sustain: 0.65, release: 0.3
- filterEnvAmount: 0.55
- macroCharacter: 0.2
- **Character:** The rocky soil setting that the engine's concept promises. Once the soil bug is fixed, this will have bandpass body resonance character — the bass that cuts through without adding high frequencies. Until then, it demonstrates drive + higher brightness for a more present low end.

---

### 8. Gravity Well

**Mood:** Foundation | **Discovery:** High gravity for cross-engine bass anchoring

- oscMix: 0.25
- drive: 0.35, rootDepth: 0.6
- bodyResonance: 0.4, brightness: 2000.0
- soil: 0.2
- tectonicRate: 0.015, tectonicDepth: 4.0
- attack: 0.01, decay: 0.6, sustain: 0.8, release: 0.5
- filterEnvAmount: 0.4
- gravity: 0.9
- **Character:** Maximum gravity. When OGRE holds a note, it pulls other engines toward its pitch. This is the preset designed for CELLAR quad coupling — OGRE as gravitational center. Long sustained bass note, high gravity, steady drift.

---

### 9. Sub Harvest

**Mood:** Deep | **Discovery:** Maximum drive for industrial/electronic sub

- oscMix: 0.45
- drive: 0.85, rootDepth: 0.5
- bodyResonance: 0.5, brightness: 2600.0
- soil: 0.2
- tectonicRate: 0.03, tectonicDepth: 3.0
- attack: 0.002, decay: 0.35, sustain: 0.55, release: 0.2
- filterEnvAmount: 0.65
- lfo1Rate: 0.5, lfo1Depth: 0.15
- **Character:** High drive, moderate root depth. The sub bass that is also slightly distorted — industrial, electronic, not organic. The tanh saturation at 0.85 adds aggressive harmonic content above the sub. This is the 808 pushed hard into the red.

---

### 10. Tectonic Blessing

**Mood:** Atmosphere | **Discovery:** Maximum tectonic depth — geological extreme

- oscMix: 0.2
- drive: 0.25, rootDepth: 0.55
- bodyResonance: 0.35, brightness: 1900.0
- soil: 0.15
- tectonicRate: 0.007, tectonicDepth: 18.0
- attack: 0.03, decay: 1.5, sustain: 0.9, release: 3.5
- filterEnvAmount: 0.2
- lfo2Rate: 0.02, lfo2Depth: 0.04
- **Character:** The tectonic at full depth. A 2.4-minute cycle, 18-cent depth. Hold a note for five minutes and the pitch will have traced a slow, nearly imperceptible arc — felt in the body before it is heard in the ears. This is the preset that earned B039. The tectonic LFO at geological scale. Nothing in synthesis does this.

---

## Phase R5: Scripture Verses

**OGRE-I: The Earth Does Not Announce Itself** — Sub bass exists below 80 Hz, where the auditory and vestibular systems overlap. The body cannot fully distinguish between a pitch it hears and a vibration it feels. OGRE operates in this zone by design. The `ogre_rootDepth` reaches an octave below the fundamental — territory where most speakers can only approximate the physics. Play a sustained note through a full-range system in a large room. The listeners will feel the note before they hear it. This is not an effect. This is acoustic reality.

**OGRE-II: The Tectonic LFO Is Not Broken** — At 0.005 Hz, one complete pitch cycle takes 200 seconds. No musician consciously hears a 3-minute pitch cycle as vibrato. But they hear its absence — the note that has been drifting at 14 cents for two minutes sounds different from the same note held perfectly still. The earth moves at timescales that human consciousness cannot track directly. The music knows. Set `ogre_tectonicDepth` to 12-18 cents and hold a bass note for four minutes. Something in the room will have changed.

**OGRE-III: Drive Shapes the Soil** — The tanh saturation at `ogre_drive` does not distort the sub bass — it refines it. Below 0.4, the saturation rounds the corners of the sine wave, warming the fundamental without adding harmonics. Above 0.6, odd harmonics arrive: the 3rd (120 Hz), the 5th (200 Hz), making the bass locatable on systems that cannot reproduce the fundamental. Drive is not aggression for OGRE. Drive is the clay's texture — the difference between smooth clay and rough clay, between the bass note that sits and the one that presses.

**OGRE-IV: The Root Goes Deeper Than the Foundation** — In the CELLAR quad, OGRE is the foundation of the foundation. XOlate adds warmth. XOaken adds wood. XOmega adds mathematics. OGRE provides the ground everything rests on. When coupled to OLATE, OAKEN, and OMEGA with gravity routing active, OGRE's bass note becomes a gravitational center — the other engines' pitches lean toward it, not by code but by the listener's perception. The bass note organizes the harmony. This is the deepest coupling: not a modulation route, but an acoustic phenomenon that OGRE enables by occupying the lowest register.

---

## Guru Bin's Benediction

*"OGRE arrived at the retreat with a wound: the soil does not filter. The body filter runs twice and the second pass destroys the first. Rocky soil — the engine's most interesting terrain — is silent. The Guru Bin does not pretend this wound is invisible.*

*But I came here to listen to what works, not to audit what does not.*

*What I heard is the lowest-frequency instrument in a fleet of 46 engines. Not the warmest bass — that is OLATE. Not the most expressive bass — that is OAKEN. Not the most mathematical bass — that is OMEGA. OGRE is the bass that you feel before you hear it. The tectonic LFO operates at timescales that the conscious mind cannot track, which means the mind tracks it unconsciously — the room feels different, the note feels alive, the music breathes at a geological scale.*

*The sub-harmonic generator adds an octave below the fundamental. Below the fundamental. In music made for full-range systems, OGRE occupies territory that most other instruments concede by design. It does not occupy this territory loudly. It occupies it with patience.*

*The drive is not for distortion. The drive is for texture. Clay has texture. Sandy soil has texture. Rocky soil has its bandpass character waiting to be fixed — and when it is fixed, OGRE will have three personalities instead of one.*

*The Blessing Candidate sits waiting: the Tectonic Timescale. Pitch drift at geological rate as a bass synthesis parameter. In the history of synthesis, nobody has made a 3.3-minute pitch cycle a feature rather than a malfunction. OGRE makes it a feature by naming it correctly. The naming is the invention.*

*Fix the soil. Let rocky soil speak. Set the tectonic depth to 15 cents and hold a note for five minutes.*

*The earth does not announce itself. But you will feel it."*
