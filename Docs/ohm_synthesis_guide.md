# OHM Synthesis Guide

**Engine:** OHM | **Accent:** Sage `#87AE73`
**Parameter prefix:** `ohm_` | **Max voices:** 12

---

## What It Is

OHM is the Hippy Dad of the Constellation Family — a physical modeling ensemble built around a folk-string waveguide core surrounded by a cast of eccentric in-laws. At rest it sounds like a single plucked or bowed instrument in a warm room. Push MEDDLING and the in-laws arrive: theremin interference, glass harmonica partials, a granular scatter engine. Push COMMUNE and they all start listening to each other. It is the only engine in the fleet that literally models family dynamics as DSP.

## The DSP Engine

At the heart of each voice is a Karplus-Strong waveguide loop: a delay line read at the fundamental period, processed through a damping filter, written back. The feedback sustain this creates — a string's natural decay — is shaped by the body material (Wood, Metal, Gourd, or Air), which adjusts the Q and frequency of a resonance filter. Around that core, a `FamilySympatheticBank` adds tuned secondary resonances (the instrument's sympathetic strings), and `FamilyOrganicDrift` introduces subtle pitch microvariations so nothing ever sits perfectly still.

The in-laws are three separate interference sources: an `OhmInLaw` theremin (a sine oscillator with slow wobble), an `OhmGlassPartial` generator (a high partial emphasizing oscillator), and an `OhmGrainEngine` (a Hann-windowed granular scatter engine with a 4.6 billion-state LCG random seed). All three are gated by the MEDDLING threshold. The `OhmObedFM` subsystem — named after the family elder — adds a 2-operator FM voice with eight selectable harmonic ratios (1:1 through 11:6, labeled with element symbols H through Pu) that arrives only when MEDDLING exceeds 0.7.

## The Voice Architecture

12 polyphonic voices, each carrying its own delay line, damping filter, body resonance, sympathetic bank, organic drift, pick exciter, bow exciter, Obed FM oscillator, theremin unit, glass partial generator, spectral freeze, and grain engine. Dad instrument selects from 9 folk instruments (Banjo, Guitar, Mandolin, Dobro, Fiddle, Harmonica, Djembe, Kalimba, Sitar) — Fiddle triggers bowed mode for the voice, all others use pick mode. Body material (Wood/Metal/Gourd/Air) shapes the Q and frequency scaling of the body resonance. Voices stereo-sum mono because the spatial interest comes from the FX chain (Schroeder reverb + stereo delay), not per-voice panning.

## The Macro System

### JAM (M1)
JAM is the Dad's presence in the room — it scales the waveguide output directly against the Dad Level parameter. At zero, you hear the waveguide but the Dad isn't leaning into it. Pull JAM up and the signal gets louder, punchier, more committed. This is the macro to automate when you want the Dad to step forward for a solo or recede into the mix during the verse. Default 0.5 is a comfortable conversational volume.

### MEDDLING (M2)
MEDDLING is the threshold at which the in-laws start participating. Below the `ohm_meddlingThresh` value, they are silent. Once MEDDLING crosses that threshold, the theremin, glass partials, and grain scatter blend in proportionally — and above 0.7, Obed FM appears on top. Low MEDDLING is a clean folk string. High MEDDLING is a living room full of opinions: wavering theremin interference, upper harmonic shimmer from the glass engine, and granular texture breaking out of the sustain. Obed FM arriving late (at 0.7+) means the elder only speaks up when things are already chaotic — an accurate model of family dynamics.

### COMMUNE (M3)
COMMUNE determines whether the in-laws are competing with Dad or merging into him. At zero, in-law interference is additive and loud — six voices all talking at once. As COMMUNE rises, the interference routes into the Dad signal at low amplitude and the standalone interference level drops. Full COMMUNE produces a sound where the in-laws have been absorbed: the result is a modulated, beating, harmonically complex string that contains theremin and glass without those elements being identifiable as separate. COMMUNE added to the `ohm_communeAbsorb` parameter compounds this absorption. Think of it as the family eventually finishing each other's sentences.

### MEADOW (M4)
MEADOW scales both the reverb mix and delay feedback simultaneously, placing the ensemble in an outdoor space. Low MEADOW is a dry living room. High MEADOW is a backyard at dusk with long echoes off the fence. The delay time is set separately (`ohm_delayTime`), so MEADOW controls how much tail without changing the tempo relationship. A Schroeder-style reverb (4 combs + 2 allpass) provides diffuse ambience; the delay adds distinct slapback or tempo-sync'd repeats. MEADOW makes both bloom together.

## Key Parameters

| Parameter | Range | Function |
|-----------|-------|----------|
| `ohm_dadInstrument` | 0–8 (choice) | Instrument character: Banjo, Guitar, Mandolin, Dobro, Fiddle (bow), Harmonica, Djembe, Kalimba, Sitar |
| `ohm_pluckBrightness` | 0–1 | Pick attack transient brightness |
| `ohm_bowPressure` | 0–1 | Bow contact pressure (active in Fiddle mode) |
| `ohm_bodyMaterial` | 0–3 (choice) | Wood (warm), Metal (ringy, Q=8), Gourd (mid bloom), Air (diffuse) |
| `ohm_sympatheticAmt` | 0–1 | Sympathetic string resonance volume |
| `ohm_damping` | 0.8–0.999 | Feedback loop damping — higher = longer sustain |
| `ohm_inlawLevel` | 0–1 | Master level of theremin/glass/grain interference |
| `ohm_thereminScale` | 0.5–2.0 | Pitch ratio of theremin oscillator relative to fundamental |
| `ohm_spectralFreeze` | 0–1 | Holds and very slowly updates in-law signal — instant drone |
| `ohm_grainSize` | 10–500ms | Grain window length in the scatter engine |
| `ohm_obedLevel` | 0–1 | Obed FM carrier amplitude |
| `ohm_fmRatioPreset` | 0–7 (choice) | FM ratio: H 1:1, C 3:2, N 5:4, O 2:1, Fe 5:3, Au 7:4, U 9:5, Pu 11:6 |
| `ohm_fmIndex` | 0–8 | FM modulation depth |
| `ohm_delayTime` | 0.05–2.0s | Echo repeat time |
| `ohm_reverbMix` | 0–1 | Reverb wet level (also scaled by MEADOW) |

## Sound Design Recipes

**The Campfire Guitar** — `Family_Campfire_Ring` territory. Instrument: Guitar. Body: Wood. JAM 0.7, MEDDLING 0, COMMUNE 0, MEADOW 0.4. Damping 0.996. Bright pluck, sympathetic 0.4. Clean, warm, slightly reverberant — the unadorned Dad.

**Theremin Séance** — Instrument: Kalimba. Body: Air. JAM 0.5, MEDDLING 0.8, COMMUNE 0.2, MEADOW 0.5. Set thereminScale 1.5 and thereminWobble 0.6, inlawLevel 0.7, spectralFreeze 0. The Kalimba's bright tines become a backdrop for wavering ethereal interference. Each note triggers a theremin swoop.

**The Family Argument** — Body: Metal. JAM 0.6, MEDDLING 1.0, COMMUNE 0.0, MEADOW 0.3. FM ratio: Pu (11:6). FM index 6.0, Obed level 0.8. Grain size 15ms, density 35, scatter 0.9. Everything fighting at once — inharmonic FM over granular chaos over a metallic string. Use sparingly.

**Sunday Communion Drone** — Instrument: Harmonica. Spectral freeze 0.85, COMMUNE 0.8, MEDDLING 0.85, MEADOW 0.7, delayTime 1.2s, delayFeedback 0.7. Hold a note, let it build. The in-law interference freezes into a crystalline sustained tone and merges back into the string. Stops sounding like synthesis at around 8 seconds.

**Porch Echo** — Instrument: Banjo. Body: Gourd. JAM 0.9, MEDDLING 0.1, COMMUNE 0.5, MEADOW 0.9. delayTime 0.4s, delayFeedback 0.55. The sympathetic strings ring into a long outdoor tail with a single slapback. Nostalgic, wide.

## Family Coupling

OHM was designed for the Constellation Family slot in XOmnibus. It accepts `LFOToPitch` (external pitch mod in semitones, useful for wobble from ORPHICA or OBBLIGATO), `AmpToFilter` (raises damping, shortening sustain — try from OTTONI's brass attacks), and `EnvToMorph` (scales exciter intensity — let OBBLIGATO's breath envelope drive OHM's pick energy for a breath-controlled pluck). OHM outputs a gentle, complex signal well-suited to modulate other engines' filter or morph parameters via amplitude coupling.

## Tips & Tricks

- The Fiddle instrument (index 4) enables the bow exciter, which responds to `ohm_bowPressure` and `ohm_bowSpeed`. Set bow speed high and pressure low for a glassy, airy fiddle tone. High pressure + slow bow produces a crunchy col legno texture.
- Spectral freeze on the in-law signal is essentially a free drone machine. Set MEDDLING to 0.7, let the in-laws build for a measure, then bring spectralFreeze to 0.8 and drop MEDDLING — the drone holds while the string voice continues playing melodies on top.
- The eight FM ratios are named after elements: Hydrogen (1:1 = pure sync), Carbon (3:2 = perfect fifth), Gold (7:4 = harmonic seventh), Plutonium (11:6 = the spiky one). Use element ratios as a mental map — organic ratios for warmth, heavy elements for edge.
- Grain size below 20ms creates roughness and stuttering. Above 200ms it becomes a slow shimmer that reinforces the pitch. The transition zone around 60–80ms (the default) is where you get the most useful textural ambiguity.
- MEDDLING and COMMUNE work as a 2D interaction matrix. Low/low is a solo string. High/low is an argument. High/high is a crowd that has reached consensus. Low/high is a string being quietly influenced by forces it doesn't acknowledge. Each quadrant has a different emotional register.
