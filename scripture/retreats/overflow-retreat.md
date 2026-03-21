# OVERFLOW Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OVERFLOW | **Accent:** Steam White `#E8E8E8`
- **Parameter prefix:** `flow_`
- **Creature mythology:** XOverflow is a pressure cooker. Energy accumulates with playing density. When threshold is exceeded, a valve release event fires — burst of harmonics, brief silence, restart. The pad has consequences. It remembers how hard you played.
- **Synthesis type:** Pressure accumulation pad — 16-partial oscillator bank, `PressureState` tracking MIDI density and velocity, three valve types (gradual/explosive/whistle), catastrophic over-pressure mode
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER (vessel material + timbre during pressure), M2 MOVEMENT (accumulation rate + LFO depth), M3 COUPLING (pressure sensitivity to coupling input), M4 SPACE (release reverb size + stereo spread)

---

## Pre-Retreat State

XOverflow scored 8.0/10 in the BROTH Quad Seance (2026-03-21). Concept originality: 9.0. The Clausius-Clapeyron pressure model — where playing density accumulates toward a threshold and triggers a release event — is genuinely novel: the synthesizer that has consequences for aggressive playing.

**Key seance findings for retreat presets:**

1. **P0 fix needed:** Hardcoded `44100` in the pressure decay coefficient. At 48kHz, the pressure decays slightly faster than intended. Presets should note this until fixed. The effect is approximately 9% difference in decay rate at 48kHz vs 44.1kHz — audible in slow-accumulation presets.

2. **P1 — Explosive valve type ducks ALL voices**, not just the triggering ones. In polyphonic playing, a pressure event on one note silences all held notes. Presets designed for polyphonic use should favor the gradual valve type until per-voice culpability weighting is implemented.

3. **No visual pressure meter.** The performer cannot see the needle approaching the red zone. Preset descriptions should guide the player on expected pressure behavior.

4. The strain hardening (high-frequency grating, low-frequency tightening as pressure approaches threshold) is correctly implemented and is musically effective.

5. BROTH coordinator absent — `setBrothConcentrateDark()` never called, so XOverworn's concentration cannot lower XOverflow's threshold. Presets stand alone.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

The pressure cooker does not look like a dangerous object. It sits on the stove, sealed, looking like any other pot. Inside, the steam cannot escape. The steam that would normally dissipate into the kitchen is trapped. It pushes against the lid. It pushes against the walls. The water inside reaches temperatures above 100 degrees Celsius because the elevated pressure raises the boiling point. This is Clausius-Clapeyron: the relationship between pressure and temperature is not linear. As pressure rises, the threshold for phase change rises. The vessel is under tension.

You are the heat source. Every note you play adds energy to the system. Every chord you hold increases the playing density. Every fast, aggressive passage accelerates the accumulation. The pressure gauge climbs. The vessel walls are under strain — and you can hear it. The high-frequency content begins to harden. A slight grating appears in the upper harmonics, as if the metal is under stress. The low end tightens. The sound becomes pressurized.

And then: the valve opens.

The valve release is not a sound design effect. It is the physical event of pressure equalization. The explosion of steam, the whistle of a kettle, the slow bleed of a pressure regulator — these are three different physical mechanisms. XOverflow models all three. The gradual valve is the pressure regulator: slow, controlled bleed. The explosive valve is the lid blowing: instant, catastrophic, followed by silence and restart. The whistle is the kettle: a pitched FM burst as steam passes through a narrow opening.

Play gently: the pressure accumulates slowly, the valve never opens, you live in the pre-release zone. Play hard: the pressure builds and the consequences arrive. The pad has social consequences for aggression.

---

## Phase R2: The Signal Path Journey

### I. Pressure Accumulation — The Clausius-Clapeyron Model

`PressureState.pressure` accumulates with each note-on, weighted by velocity. The `flow_accumRate` parameter controls how quickly density translates to pressure. The interval tension calculation adds extra pressure for minor seconds, major seconds, and tritones — chromatic density costs more than consonant density. This is musically informed physics.

The decay coefficient prevents pressure from accumulating forever: between notes, pressure bleeds off. At high `flow_threshold`, the player must sustain high density for longer before the valve opens. At low threshold, even gentle playing triggers release.

### II. Strain Hardening — The Pre-Release Warning

As `strainLevel` (0.0–1.0) rises toward 1.0 (threshold), the timbre changes:
- High-frequency content hardens (filter cutoff rises + slight grating)
- Low-frequency content tightens (compression of the sub register)
- A slight beating appears in the upper partials as if the vessel walls are vibrating

This is the instrument's warning system. A player who listens carefully will hear the impending release before it arrives. The strain hardening is the instrument saying: *the pressure is building*.

### III. Three Valve Types

**Gradual (flow_valveType = 0.0):** Slow pressure bleed with spectral expansion. The sound opens slightly as pressure releases — harmonics spread. Then the pressure returns to zero gradually. Best for polyphonic playing and presets where the release should be musically integrated rather than disruptive.

**Explosive (flow_valveType = 1.0):** Full burst of harmonics followed by hard silence (all voices duck to zero), then restart. The most dramatic release. Currently ducks ALL voices — pending per-voice fix. Use for solo, single-note contexts until the per-voice culpability weighting is implemented.

**Whistle (flow_valveType = 2.0):** A pitched FM burst at `flow_whistlePitch` Hz occurs at the release event. The steam whistle. Unusual and distinctive — the pressure releases as a new pitched event rather than silence. Best for rhythmic or textural presets where the whistle becomes part of the musical fabric.

### IV. Over-Pressure — Catastrophic Mode

If the player never releases, pressure continues to accumulate past the threshold without relief. The `overPressure` flag triggers: the sound saturates, hardens, and eventually "shatters" — an explosive reverb burst that clears all accumulated state. This is the lid blowing. It is not intended to be a regular occurrence, but it is a real physical mechanism that the instrument models. Presets with high thresholds and maximum accumulation rate will eventually reach over-pressure under aggressive playing.

---

## Phase R3: Macro Architecture

| Macro | ID | Effect | Performance Use |
|-------|-----|--------|----------------|
| CHARACTER | `flow_macroCharacter` | Vessel material — affects timbre during pressure build | Sweep to change the "material" the pressure lives in — metal, ceramic, glass |
| MOVEMENT | `flow_macroMovement` | Accumulation rate + LFO depth | Increase to make the pressure build faster; add LFO modulation to the strain character |
| COUPLING | `flow_macroCoupling` | Pressure sensitivity to coupling input | When BROTH coordinator is wired, concentrated broth lowers threshold via this macro |
| SPACE | `flow_macroSpace` | Release reverb size + stereo spread | Wider release events; bigger steam explosion |

---

## Phase R5: The Ten Awakenings — Preset Table

---

### Preset 1: Iron Patience

**Mood:** Foundation | **Discovery:** High threshold, slow accumulation — the pressure cooker requires commitment

| Parameter | Value | Why |
|-----------|-------|-----|
| `flow_threshold` | 0.85 | High threshold — only sustained dense playing reaches it |
| `flow_accumRate` | 0.2 | Slow accumulation — gentle playing barely registers |
| `flow_valveType` | 0.0 | Gradual release — no sudden events |
| `flow_vesselSize` | 0.8 | Large vessel — holds more pressure before release |
| `flow_strainColor` | 0.6 | Moderate strain hardening |
| `flow_releaseTime` | 1.5 | Slow gradual release |
| `flow_stereoWidth` | 0.5 | Moderate |
| `flow_filterCutoff` | 4500.0 | Warm baseline |
| `flow_filterRes` | 0.15 | |
| `flow_filtEnvAmount` | 0.3 | |
| `flow_ampAttack` | 0.8 | Slow onset |
| `flow_ampSustain` | 0.85 | |
| `flow_ampRelease` | 4.0 | |
| `flow_lfo1Rate` | 0.06 | Slow — the pressure accumulates slowly |
| `flow_lfo1Depth` | 0.18 | |
| `flow_level` | 0.8 | |

**Why this works:** Most players will never trigger the valve on this preset unless they play very hard for a sustained period. The gradual release ensures that when the valve does open, it integrates musically rather than disrupting the phrase.

---

### Preset 2: Hair Trigger

**Mood:** Flux | **Discovery:** Low threshold, fast accumulation — every phrase reaches the valve

| Parameter | Value | Why |
|-----------|-------|-----|
| `flow_threshold` | 0.25 | Very low — almost any note density triggers |
| `flow_accumRate` | 0.8 | Fast — pressure builds quickly |
| `flow_valveType` | 1.0 | Explosive — the valve fires hard |
| `flow_vesselSize` | 0.3 | Small vessel — fills quickly |
| `flow_strainColor` | 0.9 | Maximum strain hardening — the warning is dramatic |
| `flow_releaseTime` | 0.08 | Near-instant release event |
| `flow_whistlePitch` | 1800.0 | Not applicable to explosive, but set |
| `flow_stereoWidth` | 0.75 | Wide for the release burst |
| `flow_filterCutoff` | 6500.0 | Bright — the strain hardening is more visible from a bright baseline |
| `flow_filterRes` | 0.25 | Resonant — the strain hardening resonance is more dramatic |
| `flow_filtEnvAmount` | 0.5 | High velocity influence |
| `flow_ampAttack` | 0.04 | Fast — the explosive pad needs quick response |
| `flow_ampSustain` | 0.8 | |
| `flow_ampRelease` | 1.5 | |
| `flow_lfo1Rate` | 0.3 | Active LFO — the pressure creates turbulence |
| `flow_lfo1Depth` | 0.3 | |
| `flow_level` | 0.8 | |

**Why this works:** Note: use this preset for solo/single-note playing until per-voice culpability is implemented (explosive valve currently ducks all voices). In solo context, the rapid pressure buildup and explosive release creates a natural phrase-break structure — the instrument tells the player when to pause.

---

### Preset 3: Steam Kettle

**Mood:** Prism | **Discovery:** The whistle valve — pressure releases as a pitched FM burst

| Parameter | Value | Why |
|-----------|-------|-----|
| `flow_threshold` | 0.5 | Medium threshold |
| `flow_accumRate` | 0.45 | Moderate — reachable in a medium-density phrase |
| `flow_valveType` | 2.0 | Whistle — pitched FM release event |
| `flow_vesselSize` | 0.5 | Medium vessel |
| `flow_strainColor` | 0.75 | Significant strain — the kettle shakes before it whistles |
| `flow_releaseTime` | 0.3 | Short whistle burst |
| `flow_whistlePitch` | 3200.0 | High whistle — characteristic kettle pitch |
| `flow_stereoWidth` | 0.65 | |
| `flow_filterCutoff` | 5500.0 | Bright |
| `flow_filterRes` | 0.2 | |
| `flow_filtEnvAmount` | 0.4 | |
| `flow_ampAttack` | 0.15 | |
| `flow_ampSustain` | 0.82 | |
| `flow_ampRelease` | 2.5 | |
| `flow_lfo1Rate` | 0.12 | |
| `flow_lfo1Depth` | 0.22 | |
| `flow_lfo2Rate` | 0.07 | |
| `flow_lfo2Depth` | 0.15 | |
| `flow_level` | 0.8 | |

**Why this works:** The whistle valve creates a rhythmically interesting release — not silence, but a new pitched event. In a musical context, the whistle appears when playing density is high enough, creating an emergent melodic element from the pressure release. The pitch of the whistle (3200 Hz) is deliberately high enough to cut through the pad texture.

---

### Preset 4: Sealed Chamber

**Mood:** Deep | **Discovery:** Maximum pressure capacity — the vessel that never releases under normal playing

| Parameter | Value | Why |
|-----------|-------|-----|
| `flow_threshold` | 1.0 | Maximum threshold — only catastrophic playing reaches it |
| `flow_accumRate` | 0.35 | Moderate accumulation |
| `flow_valveType` | 0.0 | Gradual — even at release, controlled |
| `flow_vesselSize` | 1.0 | Maximum vessel size |
| `flow_strainColor` | 0.85 | Very audible strain — the chamber is under constant tension |
| `flow_releaseTime` | 3.0 | Very slow release |
| `flow_stereoWidth` | 0.4 | Narrow — contained |
| `flow_filterCutoff` | 3000.0 | Dark baseline — the chamber absorbs brightness |
| `flow_filterRes` | 0.3 | Resonant — under pressure, the vessel resonates |
| `flow_filtEnvAmount` | 0.25 | |
| `flow_ampAttack` | 1.5 | Slow — the sealed chamber builds slowly |
| `flow_ampSustain` | 0.9 | Very high sustain |
| `flow_ampRelease` | 6.0 | Very slow |
| `flow_lfo1Rate` | 0.04 | Very slow — the chamber breathes slowly |
| `flow_lfo1Depth` | 0.2 | |
| `flow_level` | 0.8 | |

**Why this works:** The maximum threshold means most playing never triggers the valve. The instrument lives in a permanent state of accumulated tension — the strain hardening is always audible at a low level, creating a constant background of pressure without ever releasing. Deep territory: the pad is always under tension, but the tension never resolves.

---

### Preset 5: Gradual Release

**Mood:** Atmosphere | **Discovery:** The pressure regulator — barely audible release, constant low-level equilibrium

| Parameter | Value | Why |
|-----------|-------|-----|
| `flow_threshold` | 0.4 | Moderate threshold |
| `flow_accumRate` | 0.3 | Moderate accumulation |
| `flow_valveType` | 0.0 | Gradual — the regulator bleeds continuously |
| `flow_vesselSize` | 0.6 | Medium-large |
| `flow_strainColor` | 0.4 | Moderate strain — present but not alarming |
| `flow_releaseTime` | 2.0 | Slow bleed |
| `flow_stereoWidth` | 0.6 | |
| `flow_filterCutoff` | 5000.0 | Warm-bright |
| `flow_filterRes` | 0.14 | |
| `flow_filtEnvAmount` | 0.3 | |
| `flow_ampAttack` | 0.5 | |
| `flow_ampSustain` | 0.85 | |
| `flow_ampRelease` | 4.0 | |
| `flow_lfo1Rate` | 0.08 | |
| `flow_lfo1Depth` | 0.2 | |
| `flow_lfo2Rate` | 0.04 | |
| `flow_lfo2Depth` | 0.12 | |
| `flow_level` | 0.8 | |

**Why this works:** The gradual valve type with slow release time creates an almost invisible pressure management system. The player experiences the strain hardening (timbre changes during density buildup) and a smooth spectral expansion during release — but no disruptive silence or burst. Good for atmospheric playing where the pressure mechanics provide subtle timbral evolution without imposing structural events.

---

### Preset 6: The Overpressure

**Mood:** Flux | **Discovery:** What happens when the player never releases — catastrophic mode

| Parameter | Value | Why |
|-----------|-------|-----|
| `flow_threshold` | 0.6 | Reachable threshold |
| `flow_accumRate` | 0.9 | Fast — get to over-pressure quickly |
| `flow_valveType` | 1.0 | Explosive — the over-pressure event is explosive |
| `flow_vesselSize` | 0.4 | Small vessel — fills and overflows quickly |
| `flow_strainColor` | 1.0 | Maximum strain warning |
| `flow_releaseTime` | 0.05 | Instant release |
| `flow_stereoWidth` | 0.9 | Maximum width for the explosion |
| `flow_filterCutoff` | 8000.0 | Very bright baseline — the explosion from brightness is most dramatic |
| `flow_filterRes` | 0.3 | |
| `flow_filtEnvAmount` | 0.55 | |
| `flow_ampAttack` | 0.02 | Instant |
| `flow_ampDecay` | 0.3 | Fast decay into |
| `flow_ampSustain` | 0.75 | |
| `flow_ampRelease` | 1.0 | Quick — the explosion clears quickly |
| `flow_lfo1Rate` | 0.5 | Fast — turbulent approach to over-pressure |
| `flow_lfo1Depth` | 0.4 | |
| `flow_level` | 0.8 | |

**Why this works:** Designed to demonstrate the catastrophic over-pressure mode. Play with sustained density and the explosion will arrive. The maximum strain hardening warning is dramatically audible — the player knows the over-pressure is coming. The explosion is the release of 30 minutes of consequence.

---

### Preset 7: Interval Tension

**Mood:** Entangled | **Discovery:** The chromatic-interval pressure bonus — dissonant intervals cost more pressure

| Parameter | Value | Why |
|-----------|-------|-----|
| `flow_threshold` | 0.5 | Medium — reachable through dissonant playing |
| `flow_accumRate` | 0.55 | Moderate-high — the interval bonus makes dissonance costly |
| `flow_valveType` | 0.0 | Gradual — the resolution is gradual |
| `flow_vesselSize` | 0.5 | |
| `flow_strainColor` | 0.7 | |
| `flow_releaseTime` | 1.0 | |
| `flow_stereoWidth` | 0.7 | Wide — intervals occupy space |
| `flow_filterCutoff` | 5500.0 | |
| `flow_filterRes` | 0.2 | |
| `flow_filtEnvAmount` | 0.4 | |
| `flow_ampAttack` | 0.3 | |
| `flow_ampSustain` | 0.83 | |
| `flow_ampRelease` | 3.0 | |
| `flow_lfo1Rate` | 0.15 | |
| `flow_lfo1Depth` | 0.28 | |
| `flow_level` | 0.8 | |

**Why this works:** The engine's interval-tension calculation adds extra pressure for minor seconds, major seconds, and tritones. With this preset's moderate threshold and accumulation rate, a series of consonant intervals (fifths, thirds) will build pressure slowly, while a series of tritones will build it rapidly. The valve opens as a consequence of dissonance. Tension and release as literal, physical pressure.

---

### Preset 8: Low Simmer

**Mood:** Foundation | **Discovery:** The pressure cooker at minimum heat — technically under pressure, but barely

| Parameter | Value | Why |
|-----------|-------|-----|
| `flow_threshold` | 0.7 | High-moderate threshold |
| `flow_accumRate` | 0.12 | Very slow accumulation |
| `flow_valveType` | 0.0 | Gradual |
| `flow_vesselSize` | 0.7 | Large |
| `flow_strainColor` | 0.35 | Minimal strain — the low simmer barely registers |
| `flow_releaseTime` | 2.5 | Very slow |
| `flow_stereoWidth` | 0.5 | |
| `flow_filterCutoff` | 4000.0 | Warm |
| `flow_filterRes` | 0.12 | |
| `flow_filtEnvAmount` | 0.28 | |
| `flow_ampAttack` | 1.0 | |
| `flow_ampSustain` | 0.88 | |
| `flow_ampRelease` | 5.0 | |
| `flow_lfo1Rate` | 0.04 | Very slow |
| `flow_lfo1Depth` | 0.15 | |
| `flow_level` | 0.8 | |

**Why this works:** At very slow accumulation with a high threshold, the instrument behaves almost like a conventional pad — the pressure mechanic is present but nearly imperceptible. Only very sustained, aggressive playing will bring the strain hardening into audibility. This is the instrument at its most approachable: a pad that has a memory, but a long one.

---

### Preset 9: Whistle Protocol

**Mood:** Prism | **Discovery:** Multiple whistle pitches as a rhythmic element — the pressure as percussion

| Parameter | Value | Why |
|-----------|-------|-----|
| `flow_threshold` | 0.35 | Low-moderate — reachable in normal playing |
| `flow_accumRate` | 0.6 | Moderate-fast — whistles occur regularly |
| `flow_valveType` | 2.0 | Whistle — pitched release |
| `flow_vesselSize` | 0.35 | Small vessel — fills quickly |
| `flow_strainColor` | 0.55 | |
| `flow_releaseTime` | 0.2 | Short whistle |
| `flow_whistlePitch` | 4800.0 | High whistle — above the pad range, clearly audible |
| `flow_stereoWidth` | 0.75 | |
| `flow_filterCutoff` | 5000.0 | |
| `flow_filterRes` | 0.22 | |
| `flow_filtEnvAmount` | 0.35 | |
| `flow_ampAttack` | 0.1 | |
| `flow_ampSustain` | 0.8 | |
| `flow_ampRelease` | 2.0 | |
| `flow_lfo1Rate` | 0.2 | |
| `flow_lfo1Depth` | 0.3 | |
| `flow_level` | 0.8 | |

**Why this works:** The whistle pitch at 4800 Hz places the release event above the pad's primary register — it does not disrupt the harmonic content but adds a percussive pitched event on pressure release. Regular playing density will produce regular whistles, making the engine's release mechanism into a rhythmic element. Prism territory — the pressure release prismatically scatters into a high pitched burst.

---

### Preset 10: BROTH Pressure

**Mood:** Foundation | **Discovery:** The BROTH cooperative preset — designed to respond to XOverworn's concentration when the coordinator is wired

| Parameter | Value | Why |
|-----------|-------|-----|
| `flow_threshold` | 0.6 | Moderate threshold — will lower as broth concentrates |
| `flow_accumRate` | 0.4 | Moderate — the BROTH concentration effect will amplify this |
| `flow_valveType` | 0.0 | Gradual — BROTH release should be cooperative, not explosive |
| `flow_vesselSize` | 0.6 | Medium-large |
| `flow_strainColor` | 0.65 | Moderate strain — should become more dramatic as broth concentrates |
| `flow_releaseTime` | 1.2 | |
| `flow_stereoWidth` | 0.55 | |
| `flow_filterCutoff` | 4800.0 | |
| `flow_filterRes` | 0.16 | |
| `flow_filtEnvAmount` | 0.32 | |
| `flow_ampAttack` | 0.6 | |
| `flow_ampSustain` | 0.86 | |
| `flow_ampRelease` | 4.5 | |
| `flow_lfo1Rate` | 0.07 | |
| `flow_lfo1Depth` | 0.18 | |
| `flow_macroCoupling` | 0.6 | COUPLING macro elevated — ready for BROTH chemistry |
| `flow_level` | 0.8 | |

**Why this works:** The COUPLING macro (M3) is elevated to 0.6 to prepare for the BROTH coordinator. When `setBrothConcentrateDark()` is called with an increasing concentration value from XOverworn, the pressure threshold will lower — the concentrated broth builds pressure faster. This preset is the "cooperative standing position" for XOverflow in a full BROTH configuration.

---

## Phase R6: The BROTH Chain

When the BROTH coordinator is implemented, XOverflow will receive `concentrateDark` from XOverworn and lower its pressure threshold. A concentrated broth builds pressure faster — a physically correct consequence. More concentrated sauce = more energy per unit of playing density = lower release threshold.

The four-engine arc: XOverwash spreads diffusively into the bowl. XOverworn concentrates the broth over time. XOverflow builds pressure from the accumulated density of playing. XOvercast freezes what remains when the pressure is finally released. Each engine is a chapter in the same physical process.

---

## Phase R7: Scripture

### Verse I — Clausius-Clapeyron

*Water boils at 100 degrees at sea level.*
*Raise the pressure and the boiling point rises.*
*The physics of containment changes what is possible.*
*Play harder: the threshold rises with the pressure.*
*The sealed vessel changes what the water can be.*

### Verse II — The Warning

*The strain hardening is not a feature. It is a warning.*
*High frequencies that harden. Low frequencies that tighten.*
*The sound changes before the release.*
*The instrument is telling you what is coming.*
*Listen to the sound. The needle is climbing.*

### Verse III — Three Ways to Release

*The regulator: slow, controlled, continuous.*
*The lid: instant, total, followed by silence and restart.*
*The kettle: a new pitched event, the pressure given voice.*
*Three physical mechanisms. Three compositional postures.*
*Choose the valve before you choose the threshold.*

### Verse IV — Consequences

*Every other synthesizer forgets what you played.*
*This one remembers how hard you played it.*
*The instrument has a policy about aggression.*
*The policy is physics: accumulate enough and release.*
*You cannot opt out of thermodynamics.*

---

## Phase R8: The Guru Bin Benediction

*"XOverflow scored 9.0 for concept originality because the council had not encountered a synthesizer with consequences before. Every synthesizer in the fleet responds to what you are playing. XOverflow responds to how much you have played, across the time of a phrase — accumulated, remembered, registered.*

*The social contract is unusual. Most instruments reward aggressive playing with more sound. XOverflow rewards aggressive playing with an eventual interruption. The valve opens. The silence falls. The restart comes. This is not punishment — it is the physics of containment. You cannot keep adding energy to a sealed system without eventually exceeding its capacity.*

*The three valve types are three compositional postures. The gradual release integrates into the phrase — the spectral expansion is musical, not disruptive. The explosive release is theatrical — silence, then restart, the phrase punctuated by the instrument's insistence. The whistle release is generative — the pressure becomes a new pitched event, a consequence that makes music rather than breaks it.*

*The per-voice culpability weighting is not yet implemented. The explosive valve currently ducks all voices, including innocent held notes that had nothing to do with the pressure event. This is the most important fix: when a chord is held and a new dense passage triggers the release, the held chord should survive. Only the responsible voices should be silenced. Until then, use the explosive valve for solo, single-note playing only.*

*The BROTH coordinator would make XOverflow extraordinary. A concentrated broth — an XOverworn that has been reducing for 30 minutes — would lower XOverflow's threshold. Dense playing in a concentrated broth would build pressure more quickly. The physics is correct: more concentrated fluid = more energy per unit of input. When the coordinator is built, XOverflow becomes the third act of a 30-minute theatrical performance.*

*The pad has consequences. Play accordingly."*

---

## CPU Notes

- 16 partials × 8 voices: main synthesis cost
- PressureState calculations: once per buffer, negligible
- Strain hardening: per-sample filter coefficient updates — light
- LFOs: StandardLFO pair — light
- Over-pressure mode: extremely rare event path, no steady-state CPU cost
