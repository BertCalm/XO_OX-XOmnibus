# OTTONI Synthesis Guide

**Engine:** OTTONI | **Accent:** Patina `#5B8A72`
**Parameter prefix:** `otto_` | **Max voices:** 12

---

## What It Is

OTTONI is a brass family portrait — three generations of players sharing a waveguide stage. The toddler blows into a conch or vuvuzela, all pressure and imprecision. The tween finds the valve and starts to mean it. The teen owns the French horn and plays with actual vibrato. The GROW macro moves through these three ages in a single gesture, morphing the ensemble from loose and childlike to full and virtuosic. It is the most narrative engine in the Constellation Family: a brass section that begins in chaos, matures in real time, and arrives somewhere earned.

## The DSP Engine

All three voices share the same physical model: a `LipBuzzExciter` feeding a Karplus-Strong waveguide loop, through a `FamilyDampingFilter`, into a `FamilyBodyResonance` and `FamilySympatheticBank`. The `LipBuzzExciter` models the nonlinear buzzing of lips compressed against a mouthpiece. Its third argument is an `ageScale` parameter — derived directly from the GROW macro — that shifts the exciter's response from loose and unfocused (low age) to tight and centered (high age). The teen voice also has independent per-voice vibrato implemented as a `vibPhase` accumulator that applies pitch modulation proportional to `growTeen` — the vibrato only appears when GROW has aged the ensemble into teen territory.

The Foreign Harmonics section adds three physical deviations: **Foreign Stretch** scales the delay line length slightly (up to 10% longer — a flat partial tuning suggesting older horns or different playing customs), **Foreign Drift** adds microtonal pitch drift by modulating delay length with a slowly varying sine (up to 2 cents), and **Foreign Cold** shifts the body resonance frequency upward and increases its Q (up to 4 additional Q units) — the cold, resonant character of instruments played in a chilly performance space or built from different metals. All three are gated by the FOREIGN macro.

## The Voice Architecture

12 voices with three active layer levels simultaneously: toddler voices, tween voices, and teen voices all share the same 12-voice pool, mixed at levels determined by the GROW crossfade. At low GROW, toddler voices dominate. At GROW 0.5, tween is loudest. At GROW 1.0, only teen voices contribute. The crossfade is a linear tent function — toddler peaks at 0, tween peaks at 0.5, teen peaks at 1.0 — meaning that at intermediate GROW values you genuinely hear the blend of two age groups playing simultaneously.

The toddler instrument selection (Conch, Shofar, Didgeridoo, Alphorn, Vuvuzela, Toy Trumpet) defines the most primitive body resonance. The tween selection (Trumpet, Alto Sax, Cornet, Flugelhorn, Trombone, Baritone Sax) occupies the middle voice range. The teen selection is the widest, covering 10 instruments from French Horn to Sackbut to Bass Sax to the Ophicleide (a 19th-century keyed bugle). The teen's Bore parameter adds an additional damping reduction proportional to bore width and GROW level — wider bore + older player = darker, longer sustain.

## The Macro System

### EMBOUCHURE (M1)
EMBOUCHURE is a global mouth-pressure multiplier applied to all three voices simultaneously. Each age group's individual pressure/embouchure parameter is scaled by `(0.5 + EMBOUCHURE)`, meaning at the macro's center (0.5) the individual settings are unmodified. Below center, the ensemble plays softer and with less control — more air than tone. Above center, all three groups are pushed into denser, more focused resonance. EMBOUCHURE is the macro for recording level and sectional energy simultaneously. Push it at the crescendo of a phrase, pull back at the breath between phrases.

### GROW (M2)
GROW is the engine's signature gesture. It moves the ensemble through three developmental stages — toddler at 0, tween at 0.5, teen at 1.0 — using a triangular blend function. As GROW sweeps from 0 to 1, the toddler sound fades, the tween rises and falls, and the teen emerges. The teen's vibrato appears only as GROW approaches 1.0 (via the `growTeen` multiplier on the vibrato oscillator). The ageScale passed to the LipBuzzExciter also increases with GROW, tightening the lip model. At GROW 0 the brass section cannot stay in tune. At GROW 1 it has discipline. Automate GROW slowly across an arrangement for a developmental arc — or snap it between verses for instant character change.

### FOREIGN (M3)
FOREIGN scales the three exotic deviations: stretch, drift, and cold. These are instruments from a different tradition, a different climate, or a different era. Stretch flattens the partials by lengthening the delay line — the sound of older horns with inharmonic bore geometries. Drift adds microtonal wavering — the pitch relationship between players in a field recording from a village ceremony rather than a rehearsal room. Cold shifts the body resonance high with a sharper Q — the nasal, ringing quality of brass played in a cold church or carried on a mountaintop. Low FOREIGN is a conventional studio brass sound. High FOREIGN is something ethnographic and unresolved.

### LAKE (M4)
LAKE scales the reverb room size and delay mix simultaneously, placing the ensemble in a large outdoor space. It follows the same pattern as MEADOW in OHM: a multiplier over the base reverb and delay parameters. Low LAKE is a small room or studio tracking space. High LAKE is an alpine lake, a cathedral, a sports arena. The reverb uses a 4-comb Schroeder approximation with LP-filtered feedback for a warm, darkening tail. The delay runs ping-pong stereo (cross-fed L and R) at a fixed 250ms with 60% feedback, scaled by LAKE. At full LAKE and high GROW (teen), the teenage brass section plays into an infinite-seeming natural reverb — a genuinely imposing sound.

## Key Parameters

| Parameter | Range | Function |
|-----------|-------|----------|
| `otto_toddlerInst` | 0–5 (choice) | Conch, Shofar, Didgeridoo, Alphorn, Vuvuzela, Toy Trumpet |
| `otto_toddlerPressure` | 0–1 | Toddler lip pressure |
| `otto_tweenInst` | 0–5 (choice) | Trumpet, Alto Sax, Cornet, Flugelhorn, Trombone, Baritone Sax |
| `otto_tweenEmbouchure` | 0–1 | Tween embouchure quality |
| `otto_tweenValve` | 0–1 | Valve modulation — subtle pitch wobble on tween voice |
| `otto_teenInst` | 0–9 (choice) | French Horn, Trombone, Tuba, Euphonium, Tenor Sax, Dungchen, Serpent, Ophicleide, Sackbut, Bass Sax |
| `otto_teenEmbouchure` | 0–1 | Teen embouchure quality |
| `otto_teenBore` | 0–1 | Bore width — wider bore lowers teen damping and darkens tone |
| `otto_teenVibratoRate` | 3–8 Hz | Teen vibrato speed |
| `otto_teenVibratoDepth` | 0–1 | Teen vibrato depth (applies only at high GROW) |
| `otto_foreignStretch` | 0–1 | Partial detuning via delay line stretch |
| `otto_foreignDrift` | 0–1 | Microtonal pitch drift from sinusoidal delay modulation |
| `otto_foreignCold` | 0–1 | Body resonance shift — higher frequency, sharper Q |
| `otto_driveAmount` | 0–1 | Soft-clip saturation (tanh) on summed output |
| `otto_sympatheticAmt` | 0–1 | Sympathetic resonance amplitude |
| `otto_damping` | 0.8–0.999 | Feedback loop damping |

## Sound Design Recipes

**Coming of Age** — Full GROW automation, 0→1 over 8 bars. Toddler: Toy Trumpet, pressure 0.3. Tween: Cornet, embouchure 0.5. Teen: French Horn, embouchure 0.8, bore 0.4. EMBOUCHURE 0.6, FOREIGN 0, LAKE 0.5. The ensemble grows up in front of you — imprecise → capable → virtuosic.

**Distant Ceremony** — GROW 0.5 (tween+teen blend). Toddler: Didgeridoo. Teen: Dungchen. FOREIGN 0.8 (all three effects at high scale). LAKE 0.9. Foreign cold pushes the body resonance into a nasal shimmer. Drift makes the pitch relationship feel like a field recording. The result is a distant ritual sound from no particular culture.

**Young Brass Section** — GROW 0.5. Tween: Flugelhorn. Teen: Euphonium, bore 0.7. Chorus rate 2.0 (slight widening). Drive 0.15. LAKE 0.4. A functional brass pad — warm, slightly unfocused, capable of sustaining chord clusters. The flugelhorn's body ratio (1.0× at Q=4, a centered soft response) blends naturally with the euphonium's low, warm resonance.

**The Toddler King** — GROW 0. Toddler: Vuvuzela. Pressure 0.6. Sympathetic 0.5. Drive 0.25. EMBOUCHURE 0.7. FOREIGN 0.3. The vuvuzela body resonance has a single narrow peak. With drive adding soft saturation and drift wobbling the pitch, it becomes a bizarre harmonic statement. Works in a track that has no other pitched content.

**Old Horn Cold Day** — GROW 0.9 (teen dominant). Teen: Serpent (the historical instrument — a wooden bass brass instrument). Teen bore 0.8. FOREIGN 1.0. LAKE 0.6. Vibrato rate 3.5 Hz, depth 0.5. The Serpent's body ratio (arbitrary from the instrument's position in the array) plus Foreign Cold at max produces a very high Q body resonance with a strained, nasal character. The slow vibrato and long sustain (high bore, high damping) suggest an old player on an old instrument.

## Family Coupling

OTTONI accepts `LFOToPitch` for external pitch wobble, `AmpToFilter` to vary sustain damping, and `EnvToMorph` to scale exciter intensity. Because the teen vibrato appears only at high GROW and is pitch-modulating, coupling OHM's sympathetic output into OTTONI's `LFOToPitch` slot creates a situation where the Dad's folk string vibrato transfers into the teenage brass player's embouchure — a cross-instrument expressive transfer that has no analog in conventional synthesis. OTTONI's output — a sum of potentially three age groups with drive saturation — is a harmonically dense source well-suited to coupling into OPAL's granular cloud as an audio input.

## Tips & Tricks

- GROW is the macro for automation, not just preset differentiation. Write a 16-bar automation lane that starts at 0 (toddler), rises through 0.5 (tween at bar 8), and arrives at 1.0 (teen at bar 16). You have built a development arc that most arrangers spend an entire arrangement trying to achieve through other means.
- The Foreign Stretch parameter adds up to 10% extra length to the delay line. At moderate values (0.3–0.5) it produces the slightly inharmonic partial structure of older hand-made instruments. At maximum it sounds deliberately out of tune in a way that is musically interesting on sustained chords.
- Drive adds soft-clip saturation after the reverb and delay. It is most useful on the toddler voice at high pressure — the vuvuzela and toy trumpet benefit from a gentle clip that prevents their basic waveform from sounding too synthetic. Keep drive below 0.3 for subtle warmth, above 0.5 for a character effect.
- Teen vibrato is tied to GROW via `growTeen`. At GROW 0.5 you get half-amplitude vibrato. At GROW 0 you get none. This means you cannot have a toddler with a vibrato — the maturity of the vibrato is physically linked to the maturity of the player. If you want vibrato at low GROW, automate GROW up momentarily.
- Sympathetic amount is shared across all three age groups. High sympathetic with high GROW creates a resonant haze around the teen voice — the kind of hall resonance you hear when a brass section plays in a live acoustic space. Pull sympathetic down to 0 to hear the instrument in an anechoic chamber. The difference is striking.
