# ORCA Retreat — Vol 3 Transcendental
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** ORCA | **Accent:** Deep Ocean `#1B2838`
- **Parameter prefix:** `orca_`
- **Source instrument:** XOrca
- **Blessing:** None yet ratified — candidate: B039 (Five Biological Subsystems as Unified DSP Architecture)
- **Synthesis type:** Apex predator synthesis — five biological subsystems (Pod Dialect wavetable+formant, Echolocation comb filter, Apex Hunt macro, Breach sub+sidechain, Countershading band-split bitcrushing) mapped directly from Orcinus orca anatomy and behavior into DSP architecture. 16-voice polyphony.

---

## Retreat Design Brief

ORCA is the only engine in the fleet whose architecture was derived from a biological system with enough specificity that each DSP module can be traced to a named anatomical or behavioral subsystem of the killer whale. The wavetable is the orca's vocal tract — metallic, formant-filtered, whale-song heavy in portamento. The echolocation is a resonant comb filter pinged by microscopic noise bursts at rates from 0.5 to 40 Hz, simulating the biosonar click trains of Orcinus orca. The Breach is a sidechain compressor triggered by the 8,000-pound body of the animal displacing water and air. Countershading is the literal pattern on the orca's body — black dorsal surface, white belly — implemented as a band-split bitcrusher that processes the frequency bands with different rules.

The factory library treats these five subsystems as individually explorable. What the Transcendental library explores is their interaction at extremes — what happens when all five biological subsystems are pushed simultaneously toward their most intense behaviors, and what the architecture reveals at the extremes of each individual parameter that no factory preset reached.

**The central Transcendental questions:**
1. What is the character of the echolocation system at its highest click rate (40 Hz) and maximum resonance (0.995), and how does this relate to the comb filter's pitch-to-delay mapping?
2. What does maximum Hunt macro combined with maximum velocity reveal — the full coordinated aggressive behavior of the apex predator DSP?
3. What is the precise acoustic signature of a full Breach event — orca_breachSub=1.0, orca_breachRatio=20.0, extreme threshold — and can this be composed rather than used as occasional emphasis?
4. What does the Countershading system do when the split frequency is very low (below 300 Hz) versus very high (3500 Hz)?
5. What is ORCA as a coupled engine — receiving signals from other engines, its echolocation and formant systems modulated by an external acoustic environment?

---

## Phase R1: Opening Meditation — The Apex Predator as DSP Architecture

The killer whale is the apex predator of every ocean on Earth. No other species hunts it. Its cognitive complexity rivals that of primates. Its communication is dialect-specific — each pod has its own set of calls, recognizable across hundreds of miles of ocean, passed from mother to calf across generations. Its echolocation range is among the highest-resolution biosonar systems in nature — able to distinguish targets at 30 Hz click rates and map acoustic space at 40 Hz with a comb filter whose Q exceeds anything achievable with standard acoustic room measurement.

ORCA takes five of these biological facts and maps them to DSP subsystems:

**Pod Dialect → Wavetable + Formant Network:** A procedural metallic wavetable scanned by LFO1, filtered through a 5-band formant network at orca vocal tract frequencies (F1=270Hz, F2=730Hz, F3=2300Hz, F4=3500Hz, F5=4500Hz × formant shift multiplier). The heavy portamento is the whale song pitch bend — glides between notes as smooth pitch curves, not linear interpolation.

**Echolocation → Resonant Comb Filter:** A comb filter pinged by single-sample white noise impulses at a rate from 0.5 to 40 Hz. The comb delay is set to `sampleRate / note_frequency` — so the comb resonates at the note pitch. At high click rates (22+ Hz), the discrete clicks become continuous excitation and the comb produces a sustained pitched metallic tone. At low rates (0.5–3 Hz), individual clicks are audible as discrete sonar pings.

**Apex Hunt → Coordinated Macro:** A single parameter (`orca_huntMacro`) simultaneously drives filter cutoff (+8000Hz at max), resonance (+0.4), formant intensity (+0.5), echolocation resonance (+0.1), bitcrush mix (+0.6), and breach sub level (+0.3). This is not a macro in the sense of four independent sliders — it is a coordinated hunting behavior where all subsystems escalate together. The apex predator does not open its filter and separately increase its formants — it coordinates all attack systems simultaneously.

**Breach → Sub-Bass Sidechain:** A pure sine sub-bass (or triangle for more presence) one octave below the lowest active voice, combined with an internal sidechain compressor (hard knee, instant attack, 150ms pump release). The compressor threshold and ratio determine how violently the main signal is displaced when the breach sub fires. At `orca_breachRatio=20.0` and `orca_breachThreshold=-6.0 dB`, the displacement is violent and physical — an 8,000-pound mass entering the water.

**Countershading → Band-Split Bitcrushing:** The signal is split at `orca_crushSplitFreq` Hz into belly (low, processed through a LP filter) and dorsal (high, processed through an HP filter). Only the dorsal is bitcrushed — quantized to `orca_crushBits` bits and sample-rate reduced by `orca_crushDownsample`. The belly remains clean and smooth. The HUNT macro reduces bit depth by up to 12 bits (at max hunt, 16→4 bits for the dorsal) and increases downsample factor by 0.8.

---

## Phase R2: Diagnosis — What the Factory Library Leaves Unexplored

**Covered well:**
- Hunt macro at moderate levels (0.35–0.65) as an intensity parameter
- Echolocation at click rates 8–18 Hz for texture
- Foundation bass tones with breach sub at 0.5–0.7
- Mono legato with significant glide for whale-song portamento
- Countershading at low mix levels (0.1–0.35) as texture addition

**Underexplored:**
1. **Echolocation at 40 Hz maximum rate** — No factory preset reaches `orca_echoRate` above 22 Hz. At 30–40 Hz, the click impulses blur into continuous excitation of the comb filter — the clicks are no longer individually audible but produce a sustained pitched metallic resonance. The echolocation system transforms from a biosonar texture element into a full synthesis voice at these rates. Combined with `orca_echoReso=0.99`, the comb filter becomes a highly resonant oscillator.
2. **Hunt macro above 0.82** — Factory ceiling is approximately 0.72. Above 0.82, the HUNT macro drives: filter cutoff to ~16000 Hz (cutoff + 8000 from hunt + velocity contribution), formant intensity to near maximum (1.0), bitcrush mix to near maximum (0.6+), and breach sub to near maximum (0.8+). This is the fully activated apex predator — all five subsystems at high intensity simultaneously. The coordinated behavior at this level produces a sound unlike any individual parameter maximum.
3. **Breach at extreme ratio** — `orca_breachRatio` above 12.0 is unused in the factory library. At 20.0, the sidechain compressor's hard knee creates an instantaneous, violent displacement. Combined with `orca_breachThreshold=-6.0 dB` (very sensitive trigger), the breach fires on nearly every note and creates a rhythmic, physical sidechain pump that is unlike any factory preset's breach behavior.
4. **Countershading at low split frequency** — `orca_crushSplitFreq` below 400 Hz separates almost all of the signal energy into the "dorsal" (crushed) band — a split at 200 Hz means only the very lowest frequencies survive clean while everything above is bitcrushed. No factory preset explores this extreme split.
5. **Maximum velocity + maximum Hunt** — The ORCA velocity-to-timbre system adds up to 3000 Hz to the filter cutoff at maximum velocity at full `orca_velCutoffAmt`. Combined with Hunt at 0.85, the total filter excursion at hard velocity is approximately 19000 Hz cutoff — essentially open. This reveals the wavetable and formant network at full bandwidth, while the Hunt's coordinated escalation of all other subsystems makes this a dramatic timbral event.
6. **Pod mode at 16 voices (polyphony=3)** — The voice mode choices are: 0=Mono, 1=Mono Legato, 2=8-voice poly, 3=16-voice poly. At 16 voices, ORCA can sustain a full chord simultaneously — all voices running independent wavetables, echolocation comb filters, and amplitude envelopes. Combined with extreme glide (0.8+), playing a chord in 16-voice mode creates a massive, coordinated slide where all 16 pitches are gliding simultaneously. No factory preset uses polyphony=3 as a primary design choice.
7. **Formant shift at extremes** — `orca_formantShift` maps [0..1] → frequency multiplier [0.5..2.0]. At 0.0, all five formants are compressed to half their natural frequencies (F1=135Hz, F2=365Hz, F3=1150Hz, F4=1750Hz, F5=2250Hz) — a very different vocal tract. At 1.0, all formants are doubled (F1=540Hz, F2=1460Hz, F3=4600Hz, F4=7000Hz, F5=9000Hz) — the formants move above the mid-frequency range and the wavetable acquires a bright, presence-heavy vocal character. No factory preset uses formantShift above 0.75.

---

## Phase R3: Refinement — The 15 Transcendental Presets

### Foundation Tier (3 Presets)

**1. Apex Predator Prime**
`Foundation` | Hunt macro at 0.88 — all five subsystems fully coordinated at high intensity.
*The full coordinated hunting behavior: every attack system elevated simultaneously.*
Parameters: `orca_huntMacro=0.88, orca_wtPosition=0.55, orca_formantIntensity=0.55, orca_formantShift=0.55, orca_echoRate=8.0, orca_echoReso=0.88, orca_echoMix=0.45, orca_breachSub=0.72, orca_breachThreshold=-14.0, orca_breachRatio=8.0, orca_crushBits=6.0, orca_crushDownsample=2.8, orca_crushMix=0.55, orca_crushSplitFreq=900.0, orca_filterCutoff=5000.0, orca_filterReso=0.55, orca_velCutoffAmt=0.85, orca_polyphony=1, orca_glide=0.08, orca_ampAttack=0.001, orca_ampDecay=0.3, orca_ampSustain=0.72, orca_ampRelease=0.35, orca_level=0.88`
Insight: **First ORCA preset to push huntMacro above 0.82.** At 0.88, the coordinated behavior becomes unmistakable — the filter is at approximately 14000 Hz (5000 + 8000×0.88), formant intensity is near maximum, bitcrush mix is above 0.55 (0.55 + 0.6×0.88), and breach sub is above 0.97. This is every biological system of the orca activated simultaneously. The sound is aggressive, broad-spectrum, physically imposing.

**2. Pod Choir**
`Foundation` | 16-voice polyphony + heavy glide — the pod as a harmonic structure.
*All 16 voices gliding from wherever they were. A chord that remembers its last position.*
Parameters: `orca_polyphony=3, orca_glide=0.65, orca_wtPosition=0.42, orca_wtScanRate=0.18, orca_formantIntensity=0.48, orca_formantShift=0.52, orca_echoRate=3.5, orca_echoReso=0.72, orca_echoMix=0.22, orca_huntMacro=0.25, orca_breachSub=0.5, orca_breachThreshold=-20.0, orca_breachRatio=4.0, orca_crushMix=0.0, orca_filterCutoff=6000.0, orca_filterReso=0.28, orca_velCutoffAmt=0.65, orca_ampAttack=0.05, orca_ampDecay=0.8, orca_ampSustain=0.72, orca_ampRelease=1.2, orca_level=0.82`
Insight: **First ORCA preset to use polyphony=3 (16-voice) as a primary design choice.** At 16 voices with glide=0.65, playing successive chords creates a dense, gliding harmonic mass where all active voices are simultaneously sliding toward their new pitches. The echolocation comb filter runs independently per voice — each voice's comb is tuned to its own frequency. The result is a pod of 16 independent sonar systems producing a harmonically rich, sliding texture that represents the entire pod rather than a single predator.

**3. Sub Displacement Groove**
`Foundation` | Breach ratio at maximum (20.0) + threshold at -6 dB — violent physical displacement as rhythmic element.
*Every note slams the sidechain. The breach is not a drama — it is the groove.*
Parameters: `orca_breachSub=0.88, orca_breachShape=0.0, orca_breachThreshold=-6.0, orca_breachRatio=20.0, orca_huntMacro=0.35, orca_wtPosition=0.48, orca_echoRate=1.5, orca_echoReso=0.65, orca_echoMix=0.18, orca_filterCutoff=3500.0, orca_filterReso=0.32, orca_crushMix=0.0, orca_velCutoffAmt=0.72, orca_polyphony=1, orca_glide=0.0, orca_ampAttack=0.001, orca_ampDecay=0.25, orca_ampSustain=0.65, orca_ampRelease=0.28, orca_level=0.85`
Insight: **First preset to push orca_breachRatio to 20.0 and orca_breachThreshold to −6.0 dB.** At this combination, the sidechain compressor fires instantly on every note above a very low threshold — the hard knee and 20:1 ratio produce complete displacement of the main signal for approximately 50ms (the 150ms pump release curve). Used rhythmically, this creates a groove where the timing of notes is stamped into the mix by the breach physics. The sub-bass layer at 0.88 provides the physical weight of the displacement.

---

### Atmosphere Tier (2 Presets)

**4. Echolocation Architecture**
`Atmosphere` | Echo rate at 38 Hz, resonance at 0.992 — the comb becomes a synthesis voice.
*At 38 Hz click rate, the individual pings merge into a sustained pitched metallic oscillator.*
Parameters: `orca_echoRate=38.0, orca_echoReso=0.992, orca_echoDamp=0.08, orca_echoMix=0.88, orca_wtPosition=0.22, orca_formantIntensity=0.35, orca_huntMacro=0.22, orca_breachSub=0.25, orca_breachThreshold=-28.0, orca_breachRatio=4.0, orca_crushMix=0.12, orca_crushBits=14.0, orca_filterCutoff=12000.0, orca_filterReso=0.25, orca_velCutoffAmt=0.72, orca_polyphony=2, orca_glide=0.12, orca_ampAttack=0.08, orca_ampSustain=0.65, orca_ampRelease=2.5, orca_lfo2Rate=0.12, orca_lfo2Depth=0.45, orca_level=0.85`
Insight: **First ORCA preset to push echoRate to 38 Hz.** The comb filter delay is set to `sampleRate / note_frequency` — at 38 Hz click rate, the continuous click excitation drives the comb filter at near-Nyquist of the click system, producing a sustained pitched metallic resonance. The resonance at 0.992 means the comb filter has extremely high Q — the pitch accuracy is very high and the decay of any given click impulse is very slow. The result is a pitched, sustained metallic tone whose character comes entirely from the echolocation system rather than the wavetable.

**5. Whale Song Harmonic**
`Atmosphere` | Extreme formant shift at 0.95 + wavetable scan — the vocal tract at maximum extension.
*At formantShift=0.95, F5 is at 8775 Hz. The vowel is unrecognizable but the intent is clear.*
Parameters: `orca_formantShift=0.95, orca_formantIntensity=0.88, orca_wtPosition=0.35, orca_wtScanRate=0.45, orca_lfo1Rate=0.08, orca_lfo1Depth=0.55, orca_lfo1Shape=0, orca_echoRate=4.5, orca_echoReso=0.72, orca_echoMix=0.32, orca_huntMacro=0.15, orca_breachSub=0.42, orca_breachThreshold=-24.0, orca_breachRatio=5.0, orca_crushMix=0.0, orca_filterCutoff=14000.0, orca_filterReso=0.18, orca_velCutoffAmt=0.62, orca_glide=0.35, orca_ampAttack=0.12, orca_ampSustain=0.68, orca_ampRelease=3.5, orca_level=0.82`
Insight: **First preset to push orca_formantShift above 0.75.** At 0.95, the five formant frequencies are F1=513Hz, F2=1385Hz, F3=4370Hz, F4=6650Hz, F5=8550Hz. These are not orca vocal tract frequencies any more — they are shifted so far up that the "vowel" character of the formant network becomes presence-dominant rather than vowel-dominant. Combined with wavetable scanning at 0.45 scan rate and LFO1 at 0.08 Hz, the resulting tone is a slowly evolving, bright, presence-heavy sound that retains the formant network's structural character (the resonant peaks) without sounding like a recognizable vowel.

---

### Submerged Tier (3 Presets)

**6. Orca Deep Dive**
`Submerged` | Low formant shift + deep filter + breach + heavy glide — below the thermocline.
*The vocal tract compressed to its lowest setting. F1=135 Hz. The orca calls from the abyss.*
Parameters: `orca_formantShift=0.02, orca_formantIntensity=0.72, orca_wtPosition=0.65, orca_echoRate=2.2, orca_echoReso=0.75, orca_echoMix=0.38, orca_huntMacro=0.18, orca_breachSub=0.78, orca_breachShape=0.0, orca_breachThreshold=-20.0, orca_breachRatio=7.0, orca_crushMix=0.0, orca_filterCutoff=1800.0, orca_filterReso=0.42, orca_velCutoffAmt=0.78, orca_polyphony=1, orca_glide=0.45, orca_ampAttack=0.02, orca_ampDecay=0.8, orca_ampSustain=0.62, orca_ampRelease=1.8, orca_level=0.88`
Insight: At `orca_formantShift=0.02`, all formant frequencies are compressed to approximately 51% of their natural values — F1=138Hz, F2=372Hz, F3=1173Hz, F4=1785Hz, F5=2295Hz. The vocal tract is physically compressed, as if the orca is producing sound under high hydrostatic pressure. Combined with a low main filter cutoff (1800 Hz) and heavy breach sub, this produces a deep, pressure-heavy bass tone where the formant network's resonances are in the low-mid range, creating a dark, congested vocal character.

**7. Pod Ghost Hunt**
`Submerged` | 8-voice poly + near-silence + echolocation only — the ghost pod mapping the dark.
*Eight voices, echolocation high in the mix, barely any wavetable. The space is the sound.*
Parameters: `orca_echoRate=12.0, orca_echoReso=0.95, orca_echoDamp=0.06, orca_echoMix=0.82, orca_wtPosition=0.28, orca_formantIntensity=0.12, orca_huntMacro=0.08, orca_breachSub=0.22, orca_breachThreshold=-32.0, orca_breachRatio=3.0, orca_crushMix=0.0, orca_filterCutoff=5500.0, orca_filterReso=0.22, orca_velCutoffAmt=0.55, orca_polyphony=2, orca_glide=0.28, orca_ampAttack=0.15, orca_ampDecay=1.5, orca_ampSustain=0.42, orca_ampRelease=4.5, orca_level=0.72`
Insight: **First preset to use echoMix above 0.78.** At 0.82, the echolocation comb filter is the primary voice — the wavetable is reduced to a carrier for the comb's excitation, and the formant network at 0.12 intensity barely shapes the signal. Eight simultaneous voices, each with its own independent comb filter tuned to its note frequency, produce a spatial array of resonant comb tones. The result is a submerged soundscape of high-resonance pitched metallic tones at echolocation click rates — the ghost pod mapping an acoustic space in the dark.

**8. Breach Protocol**
`Submerged` | Breach as the primary voice — the sub-bass and sidechain compression as composition.
*Maximum breach sub + shape at 0.5 (triangle) + ratio 15.0. The displacement is the music.*
Parameters: `orca_breachSub=0.95, orca_breachShape=0.55, orca_breachThreshold=-10.0, orca_breachRatio=15.0, orca_huntMacro=0.42, orca_wtPosition=0.38, orca_formantIntensity=0.38, orca_formantShift=0.48, orca_echoRate=6.0, orca_echoReso=0.78, orca_echoMix=0.25, orca_crushMix=0.22, orca_crushBits=9.0, orca_crushDownsample=1.8, orca_crushSplitFreq=650.0, orca_filterCutoff=4500.0, orca_filterReso=0.38, orca_velCutoffAmt=0.72, orca_polyphony=1, orca_ampAttack=0.001, orca_ampDecay=0.18, orca_ampSustain=0.55, orca_ampRelease=0.38, orca_level=0.85`
Insight: **First preset to use breachShape=0.55 (triangle sub).** The triangle sub (shape > 0.5) has more high-frequency presence than the pure sine — the breach event contains not just low-end weight but mid-frequency presence that adds a thump character to the displacement. Combined with breachRatio=15.0 and breachThreshold=−10 dB, the sidechain compression fires on every medium-velocity note with extreme ratio. The sub displacement at 0.95 level means the breach sub is nearly as loud as the main signal — the "sub" is a sub-bass layer of comparable weight to the primary voice.

---

### Prism Tier (2 Presets)

**9. Countershading Maximum**
`Prism` | Split at 200 Hz — almost all signal in the dorsal (crushed) band.
*Below 200 Hz: clean. Above 200 Hz: 4-bit bitcrushed + sample-rate reduced. The orca's body reversed.*
Parameters: `orca_crushSplitFreq=200.0, orca_crushBits=4.0, orca_crushDownsample=8.0, orca_crushMix=0.88, orca_huntMacro=0.52, orca_wtPosition=0.45, orca_formantIntensity=0.55, orca_formantShift=0.55, orca_echoRate=7.5, orca_echoReso=0.82, orca_echoMix=0.35, orca_breachSub=0.55, orca_breachThreshold=-18.0, orca_breachRatio=8.0, orca_filterCutoff=7000.0, orca_filterReso=0.38, orca_velCutoffAmt=0.78, orca_polyphony=1, orca_ampAttack=0.002, orca_ampDecay=0.2, orca_ampSustain=0.68, orca_ampRelease=0.35, orca_level=0.85`
Insight: **First preset to push orca_crushSplitFreq below 400 Hz.** At 200 Hz split with 4-bit / 8x downsample, only the lowest bass frequencies survive clean — everything from 200 Hz upward is quantized to 4-bit depth and sample-rate reduced by 8x. The result is an extremely lo-fi, harsh, digitally degraded sound that retains a clean sub-bass foundation below 200 Hz. At velocity peaks, the Hunt macro's +12-bit reduction on crush removes more bit depth, making the sound even more degraded at high velocity — the orca's most aggressive state is its most digital.

**10. Spectral Dorsal**
`Prism` | Split at 2500 Hz with 6-bit crush + Hunt escalating the dorsal component.
*Below 2500 Hz: the warm belly. Above: hard-edged, crystalline, fractured light.*
Parameters: `orca_crushSplitFreq=2500.0, orca_crushBits=6.0, orca_crushDownsample=2.5, orca_crushMix=0.72, orca_huntMacro=0.65, orca_wtPosition=0.58, orca_formantIntensity=0.62, orca_formantShift=0.65, orca_echoRate=14.0, orca_echoReso=0.88, orca_echoMix=0.52, orca_breachSub=0.58, orca_breachThreshold=-16.0, orca_breachRatio=9.0, orca_filterCutoff=9000.0, orca_filterReso=0.45, orca_velCutoffAmt=0.82, orca_polyphony=1, orca_lfo1Rate=0.35, orca_lfo1Depth=0.38, orca_ampAttack=0.003, orca_ampDecay=0.28, orca_ampSustain=0.72, orca_ampRelease=0.42, orca_level=0.87`
Insight: At 2500 Hz split frequency, the countershading division aligns with the natural formant split — F3 at 2300 Hz natural frequency is near the boundary. Below 2500 Hz (the clean belly band): the F1, F2, and partial F3 formant resonances are unaffected. Above 2500 Hz (the crushed dorsal): F4 and F5 presence frequencies plus the upper wavetable partials are bitcrushed to 6-bit depth. The result is a sound with a clean mid-low body and a harsh, crystalline upper register — exactly the high-contrast two-zone design of the orca's biological countershading.

---

### Flux Tier (2 Presets)

**11. Hunt Escalation**
`Flux` | Hunt macro sweeping from 0.0 to 1.0 via macroCharacter — the approach and strike.
*At CHARACTER=0: quiet survey mode. At CHARACTER=1: full apex predator activation.*
Parameters: `orca_huntMacro=0.0, orca_macroCharacter=0.0, orca_macroMovement=0.0, orca_macroCoupling=0.0, orca_macroSpace=0.0, orca_wtPosition=0.42, orca_wtScanRate=0.52, orca_formantIntensity=0.45, orca_formantShift=0.52, orca_echoRate=8.0, orca_echoReso=0.85, orca_echoMix=0.45, orca_breachSub=0.55, orca_breachThreshold=-18.0, orca_breachRatio=8.0, orca_crushBits=12.0, orca_crushMix=0.15, orca_crushSplitFreq=900.0, orca_filterCutoff=6000.0, orca_filterReso=0.42, orca_velCutoffAmt=0.82, orca_polyphony=1, orca_glide=0.05, orca_ampAttack=0.002, orca_ampSustain=0.72, orca_ampRelease=0.38, orca_level=0.88`
Insight: The macroCharacter controls huntMacro amount (via `huntAmount = clamp(pHuntMacro + macroChar * 0.3, 0, 1)`). This preset is designed to be performed live with the CHARACTER macro — at 0, the orca is in survey mode (calm, low hunt escalation, mild countershading). At CHARACTER=1, the full 0.3 of additional hunt is added, escalating the coordinated predator behavior across all five subsystems. The preset documents the full journey from survey to strike as a macro performance sweep.

**12. Pod Dialect Scan**
`Flux` | LFO1 scanning wavetable position at 0.55 Hz with high formant intensity — the vocal evolving.
*The orca's dialect is not fixed. It evolves within the scan range — same pod, different phrase.*
Parameters: `orca_lfo1Rate=0.55, orca_lfo1Depth=0.78, orca_lfo1Shape=0, orca_wtScanRate=0.88, orca_wtPosition=0.5, orca_formantIntensity=0.82, orca_formantShift=0.6, orca_echoRate=5.5, orca_echoReso=0.72, orca_echoMix=0.28, orca_huntMacro=0.25, orca_breachSub=0.42, orca_breachThreshold=-22.0, orca_breachRatio=5.0, orca_crushMix=0.0, orca_filterCutoff=8500.0, orca_filterReso=0.28, orca_velCutoffAmt=0.68, orca_polyphony=1, orca_glide=0.22, orca_ampAttack=0.01, orca_ampSustain=0.72, orca_ampRelease=0.8, orca_level=0.85`
Insight: LFO1 in ORCA modulates the wavetable position scanner — it sweeps the wavetable position over time, which the Pod Dialect mod envelope also partially controls. At lfo1Rate=0.55 Hz and lfo1Depth=0.78, the wavetable position sweeps over 78% of its range twice per second. Combined with wtScanRate=0.88 and formantIntensity=0.82, the formant network is continuously responding to a rapidly changing source signal — the formant resonances interact with different spectral profiles of the wavetable at different moments. The result is a rapidly evolving vocal character that sounds like the orca cycling through multiple phrases in quick succession.

---

### Entangled Tier (3 Presets)

**13. Predator Into Knot** (ORCA + OVERLAP)
`Entangled` | ORCA as the predator; OVERLAP as the topological space it hunts through.
*The hunt macro drives OVERLAP's entrain — the more aggressive the hunt, the more locked the space becomes.*
Coupling: `ORCA → OVERLAP, type: AmpToFilter, amount: 0.78`
Parameters (ORCA): `orca_huntMacro=0.58, orca_wtPosition=0.48, orca_formantIntensity=0.52, orca_echoRate=18.0, orca_echoReso=0.88, orca_echoMix=0.62, orca_breachSub=0.65, orca_breachThreshold=-14.0, orca_breachRatio=10.0, orca_crushMix=0.28, orca_crushBits=8.0, orca_filterCutoff=7500.0, orca_filterReso=0.45, orca_velCutoffAmt=0.78, orca_polyphony=1, orca_glide=0.08, orca_ampAttack=0.002, orca_ampSustain=0.72, orca_ampRelease=0.35, orca_level=0.88`
Parameters (OVERLAP): `olap_knot=1, olap_tangleDepth=0.78, olap_feedback=0.82, olap_dampening=0.32, olap_delayBase=12.0, olap_bioluminescence=0.45, olap_entrain=0.55, olap_spread=0.78, olap_filterCutoff=8000.0, olap_filterRes=0.2, olap_release=8.0`
Insight: AmpToFilter coupling carries ORCA's amplitude into OVERLAP's filter modulation path. When ORCA is at high amplitude (hard velocity, active hunt), OVERLAP's filter opens, revealing more of the Trefoil FDN's high-frequency spectral content. The ORCA signal is the predator whose intensity changes the topology of the acoustic space it occupies. At Hunt=0.58, ORCA is actively hunting — the echolocation at 18 Hz is almost continuous, the breach sub is active, and the countershading is partially engaged. All of this activity drives OVERLAP's filter higher, creating a spatial expansion that tracks the predator's intensity.

**14. Breach and Bloom** (ORCA + OXBOW)
`Entangled` | ORCA's breach event triggers OXBOW's entanglement reverb — the displacement as a spatial event.
*The 8,000-pound impact. Then the acoustic room responds for 45 seconds.*
Coupling: `ORCA → OXBOW, type: AmpToChoke, amount: 0.82`
Parameters (ORCA): `orca_breachSub=0.88, orca_breachShape=0.0, orca_breachThreshold=-8.0, orca_breachRatio=16.0, orca_huntMacro=0.45, orca_wtPosition=0.38, orca_formantIntensity=0.42, orca_echoRate=6.0, orca_echoReso=0.75, orca_echoMix=0.22, orca_crushMix=0.0, orca_filterCutoff=5000.0, orca_filterReso=0.38, orca_velCutoffAmt=0.72, orca_polyphony=1, orca_ampAttack=0.001, orca_ampDecay=0.15, orca_ampSustain=0.0, orca_ampRelease=0.25, orca_level=0.85`
Parameters (OXBOW): `oxb_entangle=0.88, oxb_decay=45.0, oxb_diffusion=0.82, oxb_feedback=0.82, oxb_dampening=0.2, oxb_bioluminescence=0.55, oxb_spread=0.92`
Insight: **The most physically cinematic preset in the fleet.** ORCA's breach event (breachRatio=16, threshold=−8dB, hard knee) fires a violent sidechain displacement on every note. The AmpToChoke coupling carries this displacement trigger into OXBOW's entangled reverb — triggering a room response that continues for 45 seconds. The ORCA attack is instantaneous and violent (the physical breach); the OXBOW tail is long, structural, evolving (the acoustic aftermath in the space). Two physical timescales — the impact and the room — coupled through the displacement trigger.

**15. Echo-Location Bloom** (ORCA + OPAL)
`Entangled` | ORCA echolocation click train fires into OPAL's granular engine.
*OPAL samples the acoustic echo of ORCA's sonar — the space remembers where the orca searched.*
Coupling: `ORCA → OPAL, type: EnvToMorph, amount: 0.65`
Parameters (ORCA): `orca_echoRate=28.0, orca_echoReso=0.95, orca_echoDamp=0.05, orca_echoMix=0.78, orca_wtPosition=0.25, orca_formantIntensity=0.22, orca_huntMacro=0.18, orca_breachSub=0.18, orca_breachThreshold=-30.0, orca_breachRatio=4.0, orca_crushMix=0.0, orca_filterCutoff=11000.0, orca_filterReso=0.18, orca_velCutoffAmt=0.62, orca_polyphony=2, orca_glide=0.18, orca_ampAttack=0.05, orca_ampSustain=0.55, orca_ampRelease=2.5, orca_level=0.75`
Parameters (OPAL): `opal_grainSize=0.08, opal_grainDensity=0.72, opal_scatter=0.55, opal_freeze=0.0, opal_pitch=0.0, opal_feedback=0.35, opal_mix=0.72`
Insight: At echoRate=28 Hz with orca_echoReso=0.95, the ORCA echolocation system is in near-continuous excitation mode — the comb filter is essentially a pitched metallic oscillator driven at 28 impulses per second. The EnvToMorph coupling carries ORCA's modulation envelope into OPAL's pitch parameter, causing OPAL's grain playback pitch to follow ORCA's modulation arc. The visual metaphor: ORCA's sonar locates acoustic space, OPAL samples what the sonar found and holds it in granular suspension. Every ORCA note search generates a OPAL memory.

---

## Phase R4: Scripture — The Verses of the Apex

### ORC-I: Five Systems, One Intention
*"The killer whale does not coordinate its filter cutoff with its bitcrusher depth. It does not decide to escalate its formant network at the same moment it increases its echolocation click rate. All five systems escalate together because they are expressions of one biological intention: the identification, approach, and capture of prey. The HUNT macro is not a convenient single control. It is the recognition that these five systems are not separate features — they are manifestations of a single coordinated behavior. In the apex predator, the architecture is the biology."*

### ORC-II: The Click Train Becomes the Tone
*"Below 3 Hz: individual sonar pings, widely spaced, mapping distant space. At 18 Hz: rapid scanning, the prey is identified and close. At 38 Hz: the clicks have merged — the sonar is no longer mapping space but creating a continuous acoustic presence, a sustained metallic tone at the frequency of the target. This is not a synthesis trick. This is the physics of the echolocation system — the same physical behavior that real orca use to distinguish fish species by their swim bladder resonance at high click rates. At 38 Hz, the biosonar has become a musical instrument."*

### ORC-III: The Breach Is Not a Special Effect
*"A breach event — a killer whale launching its full body clear of the water surface — displaces approximately 30 cubic meters of water. The acoustic consequence is not a splash. It is a pressure wave that propagates through the water column for kilometers. The BREACH subsystem models this not as a reverb send but as a sidechain compressor — an instantaneous, violent displacement of the acoustic environment by a massive physical object. When the breach ratio is 20:1 and the threshold is −6 dB, every note above a whisper triggers the physics of 8,000 pounds of animal mass entering the water column. This is not a special effect. It is the physics of weight."*

### ORC-IV: The Countershading Knows What It Is For
*"The orca's coloration pattern — pure white belly, solid black dorsal — is not decorative. The countershading disrupts depth perception from predators above (who see the white belly against the bright surface) and from below (who see the black dorsal against the dark deep). It is a visual processing exploit. The DSP countershading is the same principle: the clean belly band deceives the listener into hearing weight and warmth; the crushed dorsal band deceives them into hearing aggression and edge. Two processing regimes designed so that each exploits a different perceptual system. The architecture knows what it is for."*

---

## Summary

**15 Transcendental presets delivered:**
| Name | Mood | Key Parameter Space Explored |
|------|------|------------------------------|
| Apex Predator Prime | Foundation | orca_huntMacro=0.88, all five subsystems fully coordinated |
| Pod Choir | Foundation | polyphony=3 (16-voice), glide=0.65, pod as harmonic structure |
| Sub Displacement Groove | Foundation | breachRatio=20.0, breachThreshold=−6dB, breach as rhythm |
| Echolocation Architecture | Atmosphere | echoRate=38 Hz, echoReso=0.992, comb becomes oscillator |
| Whale Song Harmonic | Atmosphere | formantShift=0.95, presence-dominant vocal tract |
| Orca Deep Dive | Submerged | formantShift=0.02, compressed vocal tract, deep filter |
| Pod Ghost Hunt | Submerged | echoMix=0.82, echolocation as primary voice, ghost pod |
| Breach Protocol | Submerged | breachSub=0.95, breachShape=0.55 triangle, breach as voice |
| Countershading Maximum | Prism | crushSplitFreq=200 Hz, 4-bit dorsal, full lo-fi contrast |
| Spectral Dorsal | Prism | splitFreq=2500 Hz, formant-aligned band split |
| Hunt Escalation | Flux | macroCharacter sweep, survey-to-strike live performance |
| Pod Dialect Scan | Flux | lfo1Rate=0.55 Hz, lfo1Depth=0.78, rapid vocal evolution |
| Predator Into Knot | Entangled | ORCA + OVERLAP, AmpToFilter, hunt drives topology |
| Breach and Bloom | Entangled | ORCA + OXBOW, AmpToChoke, breach triggers 45s reverb |
| Echo-Location Bloom | Entangled | ORCA + OPAL, EnvToMorph, sonar search as granular memory |

**4 Scripture verses:** ORC-I through ORC-IV

**Key insight:** The factory library explores each of ORCA's five biological subsystems individually, treating them as separate features of a well-equipped synthesizer. The Transcendental library explores their coordination — the fact that these five systems are not independent parameters but expressions of a single biological intention. At Hunt=0.88, at echoRate=38 Hz, at breachRatio=20:1, the architecture reveals itself not as a collection of features but as a unified behavioral system. The apex predator is not built from parts. It is one animal.
