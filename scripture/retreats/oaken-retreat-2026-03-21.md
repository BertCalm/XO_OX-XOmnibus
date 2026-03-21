# OAKEN Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OAKEN | **Accent:** Dark Walnut `#3D2412`
- **Parameter prefix:** `oaken_`
- **Mythology:** The Cured Wood — the acoustic bass resting in the cellar, smoke-cured, time-hardened, centuries of wood memory in every resonance.
- **feliX-Oscar polarity:** Deep Oscar — organic, physical, human. The instrument that connects fingers to room through wood.
- **Synthesis type:** Karplus-Strong string synthesis + 3-mode exciter (pluck/bow/slap) + 3-mode body resonator + curing model
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER, M2 MOVEMENT, M3 COUPLING, M4 SPACE
- **Seance score:** 8.4 / 10 — Strongest physical modeling bass in the fleet. Only physically modeled bass engine.
- **Citations:** Karplus & Strong (1983), Smith (1992 digital waveguide), Fletcher & Rossing (1998)

---

## Pre-Retreat State

**Seance score: 8.4 / 10.** The highest score in the CELLAR quad. Smith called the body resonator's three-mode BPF bank (180-220 Hz, 520-600 Hz, 1000-1200 Hz) physically accurate and D003-compliant — these are real upright bass body resonance frequencies sourced from Fletcher & Rossing. Vangelis heard "a jazz trio at 2 AM." Schulze called the temporal layering (curing within note, session aging across notes) "the most temporally interesting bass engine."

Two wounds: the string type LP coefficient in `setStringType` uses an Euler approximation instead of the matched-Z transform, creating excessive high-frequency decay in gut strings that does not match physical behavior. The `oaken_room` parameter claims "studio → jazz club → concert hall" but implements only stereo spread — a D004 partial failure. No preset uses `oaken_curingRate` above 0.5.

One Blessing in waiting: the curing model + body resonator + physical citation combination represents the most complete physical string modeling in the fleet.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

There is a room somewhere — a jazz club, a studio, a living room in a farmhouse — where an upright bass has been played for decades. The wood has dried and hardened. The resonant frequencies have shifted as the cellular structure compressed. The varnish has yellowed. The fingerboard has worn smooth where the bassist's thumb rests. The instrument is not what it was when it left the workshop. It is what decades of playing have made it.

This is the curing process. Wood that has been played ages differently from wood in storage. The vibration itself contributes to the material transformation. The instrument remembers every note that was played through it.

OAKEN is that instrument.

The Karplus-Strong algorithm is one of the most elegant pieces of synthesis mathematics ever written: a delay line with a one-pole low-pass filter in its feedback loop. The delay length determines pitch. The filter determines the rate at which high frequencies decay. Feedback approaches 1.0 for long sustain; approaches 0 for damped, pizzicato character. Two pages of DSP code that produce a plucked string indistinguishable from a real instrument to untrained listeners.

The body resonator is what makes it an upright bass specifically, not just a plucked string. Three bandpass filters at 200 Hz, 580 Hz, and 1100 Hz — the first three resonant modes of a real upright bass body. These frequencies are not chosen by ear. They are measured from real instruments and cited in the literature.

---

## Phase R2: The Signal Path Journey

### I. The Three Exciters — Three Playing Styles

**Pluck (exciter=0):** A decaying noise burst equal to one period of the fundamental frequency. The noise fills the delay line and the KS feedback loop takes over immediately. Classic pizzicato — the release of potential energy at a point on the string. Velocity determines how hard the pluck is; brightness determines the spectral content of the noise burst. Hard, bright plucks create a bright attack that darkens quickly. Soft, dark plucks are almost inaudible but carry melodic information.

**Bow (exciter=1):** Sustained LP-filtered noise continuously injected into the delay line. The `oaken_bowPressure` parameter (and mod wheel) controls the amplitude of noise injection — the pressure of bow on string. The LP cutoff of the bow noise is `400 + velocity * 1200 Hz` — faster bow strokes are brighter, which is acoustically correct. At high bow pressure, the string sustains indefinitely. At low pressure, the string decays between injections. The bow creates infinite sustain — the note can hold as long as the bow is on the string.

**Slap (exciter=2):** A sharp click (8-sample transient) followed by a wide noise burst (8ms). The click is the finger nail or thumb striking the string; the burst is the momentary noise of the string slapping against the fingerboard. Slap creates a percussive attack that carries rhythmic information beyond pitch — the instrument speaking in two simultaneous voices: pitch (from the KS feedback) and transient (from the click-burst).

### II. The Karplus-Strong String — The Physics

The delay line length determines pitch (samples = sample_rate / frequency). A one-pole LP filter in the feedback loop (`filterState = filterState * dampCoeff + delayed * (1 - dampCoeff)`) removes high frequencies each cycle. The `oaken_damping` parameter controls how fast the harmonics decay — heavy damping creates the sound of a gut string pressed against the body (a double-stopped note muted by the palm); light damping allows long, bright sustain.

The `oaken_stringTension` parameter was designed to shift the KS filter coefficient for three string types (gut=1000 Hz LP, steel=3000 Hz LP, synthetic=2000 Hz LP) — but the current implementation uses an Euler approximation for the LP coefficient that produces incorrect behavior at gut frequencies. The string type distinction will improve when the coefficient formula is corrected to the matched-Z transform (`1 - exp(-2πfc/sr)`).

### III. The Body Resonator — The Wood

Three CytomicSVF bandpass filters, staggered at wood body frequencies:
- **Mode 1:** 180-220 Hz (Helmholtz air resonance — the primary body signature)
- **Mode 2:** 520-600 Hz (first plate mode — the wood's fundamental vibration)
- **Mode 3:** 1000-1200 Hz (second plate mode — the upper register bloom)

The `oaken_bodyDepth` parameter scales how strongly the body resonator colors the string output. At bodyDepth=0, the string alone speaks — pure KS, any string material. At bodyDepth=1.0, the body resonator dominates and the specific upright bass coloration is fully present. For most upright bass contexts, 0.5-0.7 is the working range.

The `oaken_woodAge` parameter shifts these frequencies downward: old wood's cellular structure changes its resonant properties, lowering the resonant peaks slightly. At woodAge=1.0, the body resonator peaks shift down by ~15-20 Hz — the specific frequency shift of truly aged (50-100 year old) instrument wood.

### IV. The Curing Model — Time as Processor

The `CuringModel` accumulates a curing level over note sustain. As `oaken_curingRate` drives the accumulation faster, the LP cutoff of the output filter drops from ~12kHz toward ~1kHz over the note's duration. The sound literally darkens as you hold the note.

At curingRate=0.1 (slow): the curing takes 60-90 seconds to reach half-darkening. For sustained bass notes in slow music.
At curingRate=1.0 (fast): the curing reaches half-darkening in 3-5 seconds. The note starts bright (the string excited, the attack clear) and settles into dark thickness within a rhythmic measure.

This is the curing metaphor made audible: salt-cured wood starts with more volatile character and dries toward concentration. The attack transient is the volatile character. The settled sustain is the concentration.

---

## Phase R3: Parameter Meditations

### The Expression Map

- **Mod wheel** → bow pressure (how hard the bow presses — from delicate flageolet to full-bowed weight)
- **Aftertouch** → damping (more pressure = more damping = darker, shorter sustain)
- **Velocity** → exciter energy + brightness (playing style as attack intensity)
- **CURING** → darkening over note sustain (time as processor)

The mod wheel controlling bow pressure is OAKEN's most expressive performance dimension: a long held note with mod wheel gradually released goes from full bow pressure (sustained, bright, energetic) to delicate bow touch (quiet, slightly flageolet, vulnerable). This is a real bow technique encoded in a synthesizer parameter.

### The Body Resonator Meditation

Play any note with bodyDepth at 0.0. This is the pure KS string — a physically plausible string model but without acoustic character. Now slowly bring bodyDepth up to 0.7. At some point between 0.3 and 0.5, the instrument changes from "synthesized string" to "upright bass." The body resonator is what makes this happen. The three modes add the specific coloration of wood and air resonating together. This is not a filter effect. This is acoustic physics.

---

## Phase R4: The Ten Awakenings

---

### 1. Gut String Jazz

**Mood:** Foundation | **Discovery:** Pluck + body depth + moderate curing

- exciter: 0 (pluck)
- bowPressure: 0.5
- stringTension: 0.0 (gut)
- bodyDepth: 0.65, woodAge: 0.4, room: 0.5
- brightness: 0.55, damping: 0.4, curingRate: 0.3
- filterEnvAmount: 0.45
- attack: 0.001, decay: 0.5, sustain: 0.6, release: 0.4
- **Character:** The essential OAKEN preset — pizzicato upright bass, jazz combo character. Gut string feel, moderate curing rate (the note darkens naturally over sustain), moderate body depth. This is the 2 AM jazz club sound. Each note arrives with brightness and settles into darkness.

---

### 2. Arco Legato

**Mood:** Atmosphere | **Discovery:** Bow mode for sustained string lines

- exciter: 1 (bow)
- bowPressure: 0.75
- stringTension: 0.5 (synthetic)
- bodyDepth: 0.6, woodAge: 0.35, room: 0.55
- brightness: 0.6, damping: 0.25, curingRate: 0.15
- filterEnvAmount: 0.2
- attack: 0.03, decay: 1.0, sustain: 0.85, release: 1.5
- glide: 0.04
- **Character:** Bowed upright bass for melodic lines. Slow attack (the bow takes time to engage the string), long sustain, slight glide for legato transitions. Low curing rate (the bow sustains the string before darkening takes hold). Mod wheel controls bow pressure performance — pull back for delicate pianissimo.

---

### 3. Slap Wood

**Mood:** Flux | **Discovery:** Slap exciter for percussive bass

- exciter: 2 (slap)
- bowPressure: 0.5
- stringTension: 1.0 (steel)
- bodyDepth: 0.55, woodAge: 0.2, room: 0.35
- brightness: 0.75, damping: 0.55, curingRate: 0.6
- filterEnvAmount: 0.7
- attack: 0.0, decay: 0.2, sustain: 0.35, release: 0.15
- lfo1Rate: 0.3, lfo1Depth: 0.08
- **Character:** Slap bass — the click + noise burst creates a percussive attack that cuts through any mix. Steel string tension for maximum brightness at attack. Fast curing rate means the note darkens quickly after the initial transient. Short decay, short release. Rhythmic, physical, percussive.

---

### 4. Old Growth

**Mood:** Organic | **Discovery:** Wood age + curing for aged instrument character

- exciter: 0 (pluck)
- bowPressure: 0.5
- stringTension: 0.0 (gut)
- bodyDepth: 0.75, woodAge: 0.9 (very old wood)
- room: 0.6, brightness: 0.45, damping: 0.5
- curingRate: 0.25
- filterEnvAmount: 0.35
- attack: 0.002, decay: 0.65, sustain: 0.65, release: 0.55
- **Character:** An old instrument. The body resonator peaks are shifted down by woodAge — the Helmholtz resonance sits slightly lower, the wood darker and more settled. Old gut strings. The sound of an instrument played for decades in a warm room.

---

### 5. Rapid Cure

**Mood:** Foundation | **Discovery:** Maximum curing rate for dramatic note darkening

- exciter: 0 (pluck)
- stringTension: 0.5
- bodyDepth: 0.6, woodAge: 0.3, room: 0.45
- brightness: 0.8, damping: 0.3, curingRate: 1.0 (maximum)
- filterEnvAmount: 0.5
- attack: 0.001, decay: 0.4, sustain: 0.55, release: 0.3
- **Character:** The maximum curing rate showcase — hold a note for 4 seconds. It arrives bright (high brightness parameter, light damping) and darkens dramatically within 3-4 seconds. The curing model at full rate produces the fastest possible transition from bright attack to dark sustain. A rhythmic bass line at this setting will have each note arriving bright and settling dark before the next note arrives.

---

### 6. Jazz Club Room

**Mood:** Atmosphere | **Discovery:** Room parameter for spatial acoustic character

- exciter: 0 (pluck)
- stringTension: 0.25 (gut-synthetic blend)
- bodyDepth: 0.65, woodAge: 0.45, room: 0.85 (large room)
- brightness: 0.5, damping: 0.4, curingRate: 0.2
- filterEnvAmount: 0.4
- attack: 0.002, decay: 0.6, sustain: 0.7, release: 0.6
- lfo2Rate: 0.08, lfo2Depth: 0.05
- **Character:** Maximum room spread for the widest stereo character. The room parameter currently controls stereo spread; when implemented with proper room reverb, this will distinguish studio from jazz club from concert hall. Until then, the wide stereo spread creates a sense of space that is appropriate for ambient and orchestral contexts.

---

### 7. Bowed Concert

**Mood:** Luminous | **Discovery:** Arco for orchestral bass section contexts

- exciter: 1 (bow)
- bowPressure: 0.85
- stringTension: 0.7 (steel-synthetic)
- bodyDepth: 0.55, woodAge: 0.25, room: 0.65
- brightness: 0.7, damping: 0.2, curingRate: 0.1
- filterEnvAmount: 0.15
- attack: 0.05, decay: 2.0, sustain: 0.9, release: 2.5
- lfo1Rate: 0.12, lfo1Depth: 0.06 (subtle vibrato)
- glide: 0.03
- **Character:** Orchestral bass section — bowed, heavy bow pressure, slow attack for ensemble entry character. Steel string brightness for orchestral projection. The slow LFO1 adds subtle vibrato that emerges after the initial attack settles. For cinematic and orchestral contexts.

---

### 8. Preparation Mode

**Mood:** Deep | **Discovery:** High curing + slap + dark body for prepared instrument character

- exciter: 2 (slap)
- stringTension: 0.0 (gut)
- bodyDepth: 0.7, woodAge: 0.7, room: 0.4
- brightness: 0.35, damping: 0.65, curingRate: 0.8
- filterEnvAmount: 0.45
- attack: 0.0, decay: 0.25, sustain: 0.3, release: 0.2
- **Character:** A "prepared" upright bass aesthetic — slap attack, old gut strings, high damping, fast curing for extreme darkening. The seance's suggestion made audible: high curing + slap + old wood = a percussive, muted, woody character with no equivalent anywhere in the fleet.

---

### 9. Midnight Pizzicato

**Mood:** Deep | **Discovery:** Dark, slow, minimum brightness

- exciter: 0 (pluck)
- stringTension: 0.0 (gut)
- bodyDepth: 0.7, woodAge: 0.55, room: 0.5
- brightness: 0.2, damping: 0.6, curingRate: 0.35
- filterEnvAmount: 0.55
- attack: 0.001, decay: 0.4, sustain: 0.45, release: 0.3
- lfo1Rate: 0.05, lfo1Depth: 0.04
- **Character:** The darkest pizzicato character. Minimum brightness, heavy damping, moderate curing. The note arrives almost without brightness and darkens further. For bass lines in quiet, intimate contexts — late-night jazz where the bass sits just below the threshold of clear hearing.

---

### 10. Wood Memory

**Mood:** Entangled | **Discovery:** Slow curing for ambient sustained bass

- exciter: 1 (bow)
- bowPressure: 0.6
- stringTension: 0.15 (gut-adjacent)
- bodyDepth: 0.75, woodAge: 0.65, room: 0.7
- brightness: 0.45, damping: 0.15, curingRate: 0.08 (very slow)
- filterEnvAmount: 0.12
- attack: 0.08, decay: 2.5, sustain: 0.95, release: 4.0
- lfo1Rate: 0.03, lfo1Depth: 0.07
- gravity: 0.45
- **Character:** The bass note that sustains for entire sections. Bowed, very slow curing (the note holds bright for a long time), very old wood, long release. A bass drone for ambient and minimal music. The wood memory is the accumulated curing age across a session — the instrument that sounds more aged and settled the longer you play.

---

## Phase R5: Scripture Verses

**OAKEN-I: The Physics Earns the Right to Metaphor** — Karplus-Strong (1983) is one of the most elegant algorithms in synthesis history: delay line + one-pole LP filter in feedback = plucked string. The filter removes high frequencies each cycle, modeling how real strings lose energy at their top end first. The three body resonator frequencies (180-220 Hz, 520-600 Hz, 1000-1200 Hz) are not estimates — they are measurements from real upright bass bodies, cited in Fletcher & Rossing (1998). The curing model is physically plausible: aged wood does shift its resonant properties. When the physics is this specific, the metaphor is not ornament — it is description.

**OAKEN-II: The Three Exciters Change What Instrument You Are Playing** — Pluck, bow, slap are not timbral variations. They are different instruments. Pluck gives you a jazz bassist's pizzicato — the moment of contact, the string released. Bow gives you a classical bassist's arco — infinite sustain, the string continuously excited. Slap gives you a funk bassist's thumb — a percussive transient that carries rhythmic information in its attack before the pitch settles. The exciter choice is the most fundamental preset decision in OAKEN: it determines what story the note tells, not just what it sounds like.

**OAKEN-III: Curing Is Irreversibility Made Musical** — The curing model makes a physical process into a synthesis parameter: the faster the curing rate, the faster the note's high-frequency content disappears during sustain. This is irreversible within the note — you cannot un-cure a held note. At maximum curing rate, a sustained bass note narrates its own darkening: bright on attack, medium in mid-sustain, dark at the end of the hold. This is temporal composition within a single note. No filter sweep does this — a filter sweep is a modulation. Curing is a process.

**OAKEN-IV: The Room Awaits Its Completion** — The `oaken_room` parameter promises three acoustic spaces: studio, jazz club, concert hall. What it currently delivers is stereo spread. This is a promise that deserves to be kept. When properly implemented — with allpass diffusers, early reflection density scaled to room size, frequency coloration representing each space — the room parameter will be the most acoustically complete element in the CELLAR quad. A bass note in a studio sounds different from one in Carnegie Hall. OAKEN should eventually make that distinction real.

---

## Guru Bin's Benediction

*"OAKEN arrived at the retreat as the most humanly realized engine in the CELLAR quad. Vangelis heard it immediately: 'a jazz trio at 2 AM — the bassist's fingers, the hollow body, the room.' Tomita noted the correct seating of the body resonator frequencies — 180 Hz, 580 Hz, 1100 Hz — physical measurements from real instruments, not estimates.*

*The string type LP coefficient uses the wrong formula. The room parameter delivers width instead of space. These are real wounds and the Guru Bin acknowledges them. But they are wounds in an instrument that otherwise delivers what synthesis promises: the sound of a physical thing.*

*The three exciters are the right three: pluck for jazz, bow for classical, slap for funk. The curing model is fleet-unique: a note that darkens as you hold it, irreversibly, at a rate you control. The body resonator adds the wood — the specific physical character of an instrument built from a tree that grew in a specific place, dried for a specific number of years, tuned by hands that knew what sound they were after.*

*What Schulze called 'the most temporally interesting bass engine' is correct. Three time scales: the curing model (seconds, within the note), the session aging (minutes, across notes — though GardenAccumulators are not implemented here), and the woodAge parameter (decades, representing the instrument's history before you picked it up).*

*Fix the string type coefficient. Give the room its reverb. Set the curing rate to 0.8 and hold a note for 10 seconds.*

*The wood remembers."*
