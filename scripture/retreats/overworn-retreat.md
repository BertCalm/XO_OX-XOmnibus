# OVERWORN Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OVERWORN | **Accent:** Reduced Wine `#4A1A2E`
- **Parameter prefix:** `worn_`
- **Creature mythology:** XOverworn is a sauce reducing on a stove. It begins rich — full of harmonics, bright, alive. Over the course of a 30-minute session, high-frequency content evaporates first, then mids, leaving only concentrated fundamentals. The session IS the envelope. Time is not a metaphor here. It is the synthesis parameter.
- **Synthesis type:** Spectral reduction pad — 16-partial oscillator bank, persistent `ReductionState` across the session, frequency-dependent evaporation per reduction integral, Maillard distortion accumulation
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER (initial harmonic richness + Maillard depth), M2 MOVEMENT (reduction rate + LFO depth), M3 COUPLING (how reduction state affects coupled engines), M4 SPACE (stereo width + space)

---

## Pre-Retreat State

XOverworn scored 8.6/10 in the BROTH Quad Seance (2026-03-21) — the highest score in the BROTH quad and one of the most conceptually original engines in the entire fleet. Concept originality: 10.0. Sonic identity: 9.0. The irreversibility architecture — that the `ReductionState` accumulates across the session and cannot be erased except by explicit reset — is described as "the first synthesizer engine that treats session time as a first-class timbral dimension."

**Key seance findings for retreat presets:**

1. The core reduction math is correct. Per-band logarithmic evaporation decelerates as each band empties — exactly like a real reduction, where early evaporation is fast and late evaporation slows as concentration increases.

2. `worn_sessionAge` never writes back to APVTS — the parameter shows 0.0 in any host display regardless of actual session age. This is a display bug, not a synthesis bug. The reduction itself accumulates correctly.

3. `isInfusion` flag (soft notes held >8s at velocity <0.3) is set but never consumed — the infusion event (adding spectral character without accelerating reduction) is a concept awaiting implementation.

4. `worn_warmth` in `Bone_Broth.xometa` is an orphaned key — no parameter with that ID exists. New presets will not include it.

5. The stateReset parameter functions as a toggle knob, not a button — works in practice but is fragile. Presets should not rely on specific reset behavior.

**The retreat priority:** This is the most important retreat in BROTH. The session-memory mechanic requires two types of presets: *starting states* (full, fresh, designed to reduce beautifully over 20–30 minutes) and *arrived states* (mid- or late-reduction snapshots that begin already-concentrated). Both types are designed here, with at least two starting-state presets demonstrating the reduction journey architecture.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

The sauce begins as water — or wine, or stock — with everything suspended in it. Proteins. Fats. Sugars. Aromatics. The volatile compounds — the ones that carry the brightness and the top notes — are the first to leave. You cannot hold them. Heat drives them off as vapor. You watch the steam and you know: those are the harmonics leaving.

What remains after the volatile aromatics have gone? The less volatile compounds. The sugars begin to concentrate, and above a certain temperature, they caramelize — undergoing Maillard reactions, browning, creating new compounds that did not exist in the original sauce. This is not loss. This is transformation. The Maillard products are darker, more complex, more bitter and sweet simultaneously. They are what makes a braise better the next day.

The fundamentals are the collagen. Collagen does not evaporate. It dissolves slowly from the connective tissue into the sauce and becomes gelatin — that thick, unctuous quality that makes a reduced sauce coat the back of a spoon. The collagen that was invisible in the raw ingredient becomes the defining texture of the concentrated reduction. The fundamental that was merely the bass of the original sound becomes the entire soul of the reduced one.

Now hear this as synthesis. Your session begins. The patch is full — 16 harmonics, bright, alive, open at the top. You play for five minutes. The upper bands begin to thin. The volatile aromatics — the 8th and 16th harmonics — are already at 70% of their original mass. You play for fifteen minutes. The mids are perceptibly darker. The high-frequency shimmer that was present at the beginning is now quiet. The fundamentals are the same — they have never reduced, they never will. But they are now more present, more prominent, because everything around them has thinned. At thirty minutes: concentration. The Maillard saturation is audible — a subtle warmth, a mild overdriven quality that was not there at the start. The umami bed has deepened. The sauce coats the spoon.

This is what you are playing. Not an instrument — a process.

---

## Phase R2: The Signal Path Journey

### I. The Reduction Integral — Frequency-Dependent Evaporation

The `ReductionState` tracks 8 octave bands of spectral mass (`spectralMass[8]`), each starting at 1.0 (full) and reducing toward 0.0. The evaporation formula implements logarithmic deceleration:

```
bandRate *= spectralMass[b]  // as the band empties, evaporation slows
spectralMass[b] -= bandRate
```

This is physically correct: high-concentration fluids lose volume faster than low-concentration ones, because there is more surface area relative to solute. As the sauce reduces below half, the evaporation rate halves. This is why a reduction never fully reaches zero — the last drops become extremely stable.

**Band ordering (evaporation rate):** Band 7 (highest harmonics) reduces fastest. Band 0 (sub/fundamental) never reduces. The rate gradient creates the characteristic "brightness fades first" arc.

**`worn_reductionRate`** scales the base evaporation rate. At 0.2, the reduction is slow (90+ minutes to full reduction). At 1.0, it is fast (5–15 minutes). The `worn_sessionTarget` (minutes) sets the intended full-reduction arc duration.

### II. Maillard Reaction — Caramelization as Harmonic Distortion

As `concentrateDark` increases (a derived value from how deeply the spectral bands have reduced), the `fastTanh` saturation is applied to the oscillator bank output. This is the caramelization signal: the remaining concentrated harmonics develop a mild non-linear warmth. At low concentration, the saturation is imperceptible. At high concentration, it is the dominant timbral color.

**`worn_maillard`** (0–1): the maximum Maillard depth at full reduction. At 0.0, no saturation ever develops. At 1.0, the fully-reduced patch has significant harmonic saturation — a warm, slightly driven quality that was entirely absent at session start.

This is the most musically important emergent property of the engine. A player who runs a session for 30 minutes discovers that their patch has developed a warmth that they did not design. The caramelization was always waiting.

### III. The Umami Bed — Fundamental Concentration

`umamiBed` rises as `spectralMass` decreases. It does not reduce like the other bands. Instead it deepens — as the upper harmonics thin, the relative presence of the fundamentals grows. This is implemented as a slight Q-boost to the sub-octave register as reduction increases. At full reduction: the patch is all umami, no volatile aromatics.

**`worn_umamiDepth`** (0–1): the intensity of the umami concentration effect. At 0.5 (default), the bass deepens moderately as reduction progresses. At 0.9, the fully-reduced patch is dominated by sub-bass resonance.

### IV. Session Targets and Age

`worn_sessionTarget` (5–120 minutes) sets the expected duration. A session target of 5 minutes means reduction is rapid and dramatic within a song. A session target of 120 minutes means the patch evolves almost imperceptibly across an entire recording session.

`worn_sessionAge` (read-only in presets, though it can be saved as a starting state): 0.0 = fresh, 1.0 = fully reduced. **Critical for "arriving mid-reduction" presets:** setting `worn_sessionAge` to 0.3, 0.6, or 0.8 in the preset JSON creates a patch that begins already-concentrated. This is the "preset saved at 3pm and loaded at midnight" state — the cook left the pot on.

### V. Infusion Events

The `isInfusion` mechanic (velocity < 0.3, hold > 8 seconds) is the architecture for soft long tones that add spectral character without accelerating reduction. Currently the flag is set but not consumed (seance finding). In retreat presets, we acknowledge this and design infusion-style playing guidance even before the code fully implements it.

---

## Phase R3: Macro Architecture

| Macro | ID | Effect | Performance Use |
|-------|-----|--------|----------------|
| CHARACTER | `worn_macroCharacter` | Adds richness + Maillard depth | Enrich the current reduction state temporarily — more harmonic content, more warmth |
| MOVEMENT | `worn_macroMovement` | Increases reduction rate + LFO depth | Accelerate the cook: push MOVEMENT for faster evaporation |
| COUPLING | `worn_macroCoupling` | How much reduction state affects coupled engines | Controls BROTH chemistry: how much the concentration state flavors the other pots |
| SPACE | `worn_macroSpace` | Stereo width + space | The reduction concentrates in space as well as spectrum |

---

## Phase R4: The BROTH Position

XOverworn is the clock of the BROTH quad. It is the timekeeper. When the BROTH coordinator is active, every other engine reads XOverworn's state:
- **XOverwash** reads `sessionAge` → raises viscosity (the broth thickens, diffusion slows)
- **XOverflow** reads `concentrateDark` → lowers pressure threshold (concentrated broth builds pressure faster)
- **XOvercast** reads `spectralMass` → seeds darker crystals (fewer nucleation sites, darker ice)

This is the BROTH architecture: XOverworn is the pot. The other three engines are affected by what is in the pot. Without XOverworn, the BROTH has no chemistry. Without the other three, XOverworn has no consequences.

---

## Phase R5: The Ten Awakenings — Preset Table

Note: Two presets (Presets 1 and 2) are designed as starting-state presets demonstrating the reduction journey. Six presets cover the mid-arc and arrived states. Two presets are for specific expressive use cases.

---

### Preset 1: Grand Opening *(Starting State)*

**Mood:** Foundation | **Discovery:** Maximum richness at session start — this is what the broth is before time works on it

| Parameter | Value | Why |
|-----------|-------|-----|
| `worn_reductionRate` | 0.4 | Moderate reduction — will take 30+ minutes to fully concentrate at this rate |
| `worn_heat` | 0.5 | Medium heat — not aggressive cooking, a gentle simmer |
| `worn_richness` | 1.0 | Full richness — all harmonics at maximum |
| `worn_maillard` | 0.4 | Maillard enabled — will develop warmth as reduction progresses |
| `worn_umamiDepth` | 0.6 | Moderate umami — the fundamentals will deepen noticeably |
| `worn_concentrate` | 0.4 | Medium concentration driving |
| `worn_sessionTarget` | 30.0 | Designed for a 30-minute reduction arc |
| `worn_sessionAge` | 0.0 | **FRESH** — session begins at maximum richness |
| `worn_stateReset` | 0.0 | Not resetting |
| `worn_stereoWidth` | 0.55 | Moderate stereo |
| `worn_filterCutoff` | 9000.0 | High cutoff — all the volatile aromatics are present and shining |
| `worn_filterRes` | 0.12 | Gentle resonance |
| `worn_filtEnvAmount` | 0.35 | Velocity shapes brightness — harder notes bring out more harmonics |
| `worn_ampAttack` | 0.4 | Smooth onset |
| `worn_ampSustain` | 0.88 | Very sustained |
| `worn_ampRelease` | 5.0 | Generous release |
| `worn_lfo1Rate` | 0.06 | Slow organic breath |
| `worn_lfo1Depth` | 0.15 | Subtle motion — the pot is calm |
| `worn_level` | 0.8 | Standard level |

**Why this works:** This is the opening state of the session. Everything is present. Nothing has been lost. The player who loads this preset and plays for 30 minutes will hear the sauce reduce — the upper harmonics thinning, the Maillard warmth developing, the umami deepening. The reduction is the performance.

**Performance note:** This preset is designed to reduce beautifully. Do not fight it — play it for the arc.

---

### Preset 2: Two-Hour Braise *(Starting State — Slow Reduction)*

**Mood:** Atmosphere | **Discovery:** The very slow cook — hours pass before the concentration is audible

| Parameter | Value | Why |
|-----------|-------|-----|
| `worn_reductionRate` | 0.15 | Very slow — this will take 90+ minutes to fully reduce |
| `worn_heat` | 0.3 | Low heat — a braise is not a rolling boil |
| `worn_richness` | 1.0 | Full richness at start |
| `worn_maillard` | 0.55 | Moderate Maillard — slow cooking develops more complex browning |
| `worn_umamiDepth` | 0.75 | High umami — the collagen is the whole point of a long braise |
| `worn_concentrate` | 0.35 | Gentle concentration |
| `worn_sessionTarget` | 90.0 | Designed for a 90-minute arc |
| `worn_sessionAge` | 0.0 | **FRESH** — the braise has just begun |
| `worn_stateReset` | 0.0 | Not resetting |
| `worn_stereoWidth` | 0.5 | Centered |
| `worn_filterCutoff` | 7500.0 | Present but not bright — a braise does not blaze |
| `worn_filterRes` | 0.1 | Low resonance |
| `worn_filtEnvAmount` | 0.28 | Moderate velocity shaping |
| `worn_ampAttack` | 0.8 | Slow — low heat, slow entry |
| `worn_ampSustain` | 0.9 | Very sustained |
| `worn_ampRelease` | 6.0 | Long |
| `worn_lfo1Rate` | 0.03 | Very slow breath — the pot is barely simmering |
| `worn_lfo1Depth` | 0.1 | Barely perceptible motion |
| `worn_lfo2Rate` | 0.012 | Second breath, even slower |
| `worn_lfo2Depth` | 0.07 | Trace |
| `worn_level` | 0.78 | Slightly quieter — a braise is not theatrical |

**Why this works:** At rate 0.15 and target 90 minutes, the reduction is almost imperceptible from minute to minute. Over the course of a full recording session, the patch evolves from bright and rich to dark and umami. A player who uses this preset across an album session will find the final tracks have a different tonal character from the opening ones — the pot has been on the stove all day. This is the patch that demonstrates XOverworn's most radical claim: the session is the synthesis.

---

### Preset 3: Halfway There

**Mood:** Foundation | **Discovery:** Mid-reduction — the mids are thinning, the fundamentals are prominent

| Parameter | Value | Why |
|-----------|-------|-----|
| `worn_reductionRate` | 0.5 | Standard reduction |
| `worn_heat` | 0.55 | Medium heat |
| `worn_richness` | 0.7 | Somewhat thinned — this is not the full start anymore |
| `worn_maillard` | 0.5 | Moderate Maillard starting to develop |
| `worn_umamiDepth` | 0.65 | Umami is building |
| `worn_sessionTarget` | 30.0 | Standard target |
| `worn_sessionAge` | 0.45 | **MID-REDUCTION** — the session has been going for 14 minutes |
| `worn_stateReset` | 0.0 | |
| `worn_stereoWidth` | 0.5 | |
| `worn_filterCutoff` | 5500.0 | Already darker than fresh |
| `worn_filterRes` | 0.14 | |
| `worn_filtEnvAmount` | 0.3 | |
| `worn_ampAttack` | 0.5 | |
| `worn_ampSustain` | 0.88 | |
| `worn_ampRelease` | 4.0 | |
| `worn_lfo1Rate` | 0.07 | |
| `worn_lfo1Depth` | 0.13 | |
| `worn_level` | 0.8 | |

**Why this works:** Setting `worn_sessionAge` to 0.45 creates a patch that begins already mid-reduced. The high harmonics are already thinned; the Maillard is developing. For producers who want to start a session from a specific point in the reduction arc, this preset provides a named starting position: "Halfway."

---

### Preset 4: Demi-Glace Entry

**Mood:** Deep | **Discovery:** 70% reduced — the sauce is dark, thick, deeply flavored

| Parameter | Value | Why |
|-----------|-------|-----|
| `worn_reductionRate` | 0.5 | |
| `worn_heat` | 0.5 | |
| `worn_richness` | 0.5 | Half richness — half the original harmonic content remains |
| `worn_maillard` | 0.7 | Strong Maillard — the browning is characteristic now |
| `worn_umamiDepth` | 0.8 | Deep umami — the collagen is fully released |
| `worn_sessionTarget` | 30.0 | |
| `worn_sessionAge` | 0.7 | **LATE REDUCTION** — 21 minutes of 30 elapsed |
| `worn_stateReset` | 0.0 | |
| `worn_stereoWidth` | 0.45 | Narrowing — concentration draws the sound inward |
| `worn_filterCutoff` | 3200.0 | Dark — only the lows and mids remain |
| `worn_filterRes` | 0.18 | Some resonance — the concentration has a new texture |
| `worn_filtEnvAmount` | 0.35 | Velocity still shaping — a hard note can still bring brightness briefly |
| `worn_ampAttack` | 0.6 | |
| `worn_ampSustain` | 0.9 | Very sustained — the thick sauce doesn't dissipate quickly |
| `worn_ampRelease` | 7.0 | Long — it coats |
| `worn_lfo1Rate` | 0.04 | Slow, viscous |
| `worn_lfo1Depth` | 0.12 | |
| `worn_level` | 0.8 | |

**Why this works:** A 70%-reduced starting state produces a sound that a first-time player will hear and not immediately recognize as the same engine as "Grand Opening." The brightness is gone. The saturation has developed. The umami bed is audible. This is the power of session-age as a preset dimension.

---

### Preset 5: The Final Concentrate

**Mood:** Submerged | **Discovery:** Fully reduced — only fundamentals remain, deeply saturated

| Parameter | Value | Why |
|-----------|-------|-----|
| `worn_reductionRate` | 0.5 | |
| `worn_heat` | 0.6 | |
| `worn_richness` | 0.8 | Still rich in the base — the fundamentals haven't reduced |
| `worn_maillard` | 0.9 | Near-maximum Maillard — dark, complex caramelization |
| `worn_umamiDepth` | 0.95 | Maximum umami — the collagen is the sauce now |
| `worn_sessionTarget` | 30.0 | |
| `worn_sessionAge` | 0.95 | **NEARLY COMPLETE** — 28.5 minutes of 30 elapsed |
| `worn_stateReset` | 0.0 | |
| `worn_stereoWidth` | 0.35 | Narrow — concentrated in the center |
| `worn_filterCutoff` | 1400.0 | Very dark — almost no highs remain |
| `worn_filterRes` | 0.25 | The resonance of concentration |
| `worn_filtEnvAmount` | 0.4 | Velocity can still briefly open the filter |
| `worn_ampAttack` | 0.7 | Slow — thick things move slowly |
| `worn_ampSustain` | 0.92 | Almost no decay |
| `worn_ampRelease` | 9.0 | The reduction coats and stays |
| `worn_lfo1Rate` | 0.025 | Very slow |
| `worn_lfo1Depth` | 0.09 | Minimal motion — the sauce is almost still |
| `worn_level` | 0.82 | Slightly boosted — concentration intensifies level |

**Why this works:** A sessionAge of 0.95 creates a patch that has almost nothing left to reduce. The volatile aromatics are nearly gone. What remains is umami, Maillard complexity, and concentrated fundamentals. This is a sound design texture: deeply submerged, warm, dense. A pad made entirely of what did not evaporate.

---

### Preset 6: Quick Reduction

**Mood:** Flux | **Discovery:** High heat — the sauce concentrates in five minutes, not thirty

| Parameter | Value | Why |
|-----------|-------|-----|
| `worn_reductionRate` | 1.0 | Maximum reduction rate |
| `worn_heat` | 0.9 | High heat — aggressive cooking |
| `worn_richness` | 1.0 | Full richness at start |
| `worn_maillard` | 0.75 | High Maillard — high heat promotes browning |
| `worn_umamiDepth` | 0.5 | Moderate — high-heat reduction doesn't develop collagen as well |
| `worn_sessionTarget` | 5.0 | Five minutes to full reduction |
| `worn_sessionAge` | 0.0 | Fresh |
| `worn_stateReset` | 0.0 | |
| `worn_stereoWidth` | 0.6 | |
| `worn_filterCutoff` | 8000.0 | Start bright |
| `worn_filterRes` | 0.15 | |
| `worn_filtEnvAmount` | 0.4 | High velocity sensitivity — hard notes bring out the brightness |
| `worn_ampAttack` | 0.1 | Fast — the heat is high, the entry is quick |
| `worn_ampSustain` | 0.8 | |
| `worn_ampRelease` | 2.0 | Shorter release — high heat, fast transitions |
| `worn_lfo1Rate` | 0.35 | Faster LFO — the heat creates turbulence |
| `worn_lfo1Depth` | 0.25 | Visible motion |
| `worn_level` | 0.8 | |

**Why this works:** At maximum reduction rate and a 5-minute target, the patch undergoes its full arc in the time of a song rather than a session. Players can hear the full reduction journey — from bright/rich to dark/concentrated — within a single performance.

---

### Preset 7: Infusion State

**Mood:** Aether | **Discovery:** The soft note held forever — below velocity 0.3, held for more than 8 seconds, barely reducing

| Parameter | Value | Why |
|-----------|-------|-----|
| `worn_reductionRate` | 0.25 | Slow reduction |
| `worn_heat` | 0.2 | Very low heat — infusion temperature, not reduction temperature |
| `worn_richness` | 0.85 | Moderately rich — infusion adds flavor rather than concentrating |
| `worn_maillard` | 0.1 | Minimal Maillard — infusion at low heat doesn't brown |
| `worn_umamiDepth` | 0.55 | Moderate umami |
| `worn_sessionTarget` | 60.0 | Slow arc for a slow infusion |
| `worn_sessionAge` | 0.1 | Barely started |
| `worn_stateReset` | 0.0 | |
| `worn_stereoWidth` | 0.65 | Open stereo |
| `worn_filterCutoff` | 5500.0 | Medium brightness — infusion gives warmth, not darkness |
| `worn_filterRes` | 0.1 | |
| `worn_filtEnvAmount` | 0.15 | Very low velocity influence — soft notes, soft response |
| `worn_ampAttack` | 3.0 | Very slow — infusion is gentle |
| `worn_ampSustain` | 0.92 | High sustain — hold it there |
| `worn_ampRelease` | 10.0 | Very long — the infused flavor lingers |
| `worn_lfo1Rate` | 0.015 | Barely moving |
| `worn_lfo1Depth` | 0.08 | Trace motion |
| `worn_lfo2Rate` | 0.007 | Second even slower breath |
| `worn_lfo2Depth` | 0.05 | |
| `worn_level` | 0.75 | Quiet — infusion is gentle |

**Why this works:** Designed for the infusion playing technique — very soft, very long notes. When the `isInfusion` mechanic is implemented, this preset will produce notes held at low velocity that add spectral character without accelerating reduction. Until then, it demonstrates the intended playing style: patience as performance.

---

### Preset 8: The Maillard Hour

**Mood:** Prism | **Discovery:** When the caramelization is the feature — not the reduction, but the browning

| Parameter | Value | Why |
|-----------|-------|-----|
| `worn_reductionRate` | 0.6 | Moderate-high — reaches Maillard conditions quickly |
| `worn_heat` | 0.8 | High heat — Maillard requires temperature |
| `worn_richness` | 0.9 | High richness — more harmonic content = more to brown |
| `worn_maillard` | 1.0 | Maximum — this preset is named for Maillard |
| `worn_umamiDepth` | 0.4 | Moderate — the browning is the feature, not the collagen |
| `worn_sessionTarget` | 15.0 | Medium arc — Maillard develops quickly at high heat |
| `worn_sessionAge` | 0.5 | Mid-reduction — Maillard is already developing |
| `worn_stateReset` | 0.0 | |
| `worn_stereoWidth` | 0.55 | |
| `worn_filterCutoff` | 4500.0 | Moderately dark — the browning has already thinned the highs |
| `worn_filterRes` | 0.22 | The resonance of the caramelization |
| `worn_filtEnvAmount` | 0.45 | Velocity strongly shapes the brightness flash |
| `worn_ampAttack` | 0.15 | Medium-fast — the hot pan responds quickly |
| `worn_ampSustain` | 0.85 | |
| `worn_ampRelease` | 3.5 | |
| `worn_lfo1Rate` | 0.18 | Moderate — some heat-turbulence |
| `worn_lfo1Depth` | 0.2 | |
| `worn_level` | 0.8 | |

**Why this works:** Maximum Maillard at a mid-reduction starting state means the caramelization is immediately present and will continue to deepen. Velocity-sensitive filter envelope at 0.45 creates "sizzle" on hard hits — the hot-pan flash of brightness when energy is applied — followed by warm, dark saturation. Prism territory: the browning is spectral complexity.

---

### Preset 9: Old Stock

**Mood:** Organic | **Discovery:** A broth started days ago — already deeply reduced when you find it

| Parameter | Value | Why |
|-----------|-------|-----|
| `worn_reductionRate` | 0.3 | Slow — it has been reducing for a long time but slowly |
| `worn_heat` | 0.35 | Low heat — a simmer over days |
| `worn_richness` | 0.6 | Significantly reduced richness |
| `worn_maillard` | 0.6 | Moderate browning — days of simmering |
| `worn_umamiDepth` | 0.85 | Very deep umami — this is why old stock tastes profound |
| `worn_sessionTarget` | 60.0 | Long arc |
| `worn_sessionAge` | 0.6 | The stock was already running for 36 minutes |
| `worn_stateReset` | 0.0 | |
| `worn_stereoWidth` | 0.4 | Narrow — old stock is concentrated |
| `worn_filterCutoff` | 2800.0 | Dark |
| `worn_filterRes` | 0.2 | |
| `worn_filtEnvAmount` | 0.3 | |
| `worn_ampAttack` | 1.2 | |
| `worn_ampSustain` | 0.9 | |
| `worn_ampRelease` | 8.0 | The old stock sustains |
| `worn_lfo1Rate` | 0.035 | |
| `worn_lfo1Depth` | 0.11 | |
| `worn_level` | 0.8 | |

**Why this works:** Old stock — bones that have been simmering for days — produces the deepest, most complex flavors. The high umami, moderate Maillard, and 60% sessionAge starting position create a pad that sounds like it has history. The dark filter and narrow stereo field suggest concentration without presenting as dramatically reduced.

---

### Preset 10: Reset Protocol

**Mood:** Foundation | **Discovery:** The stateReset moment — clearing the pot and starting again

| Parameter | Value | Why |
|-----------|-------|-----|
| `worn_reductionRate` | 0.5 | Standard |
| `worn_heat` | 0.5 | Standard |
| `worn_richness` | 1.0 | Fully rich — the pot has been cleaned and refilled |
| `worn_maillard` | 0.3 | Minimal — fresh start, minimal browning |
| `worn_umamiDepth` | 0.5 | Moderate — this is not old stock |
| `worn_sessionTarget` | 30.0 | |
| `worn_sessionAge` | 0.0 | Fresh — explicit fresh start |
| `worn_stateReset` | 0.0 | (toggle to 1.0 to clear any accumulated state) |
| `worn_stereoWidth` | 0.55 | |
| `worn_filterCutoff` | 8500.0 | Bright — the clean pot, the fresh water |
| `worn_filterRes` | 0.12 | |
| `worn_filtEnvAmount` | 0.35 | |
| `worn_ampAttack` | 0.5 | |
| `worn_ampSustain` | 0.87 | |
| `worn_ampRelease` | 4.5 | |
| `worn_lfo1Rate` | 0.08 | |
| `worn_lfo1Depth` | 0.16 | |
| `worn_level` | 0.8 | |

**Why this works:** When loaded mid-session, this preset loaded with `stateReset` at 1.0 clears the accumulated reduction. Fresh pot. New session. The bright filter and full richness communicate that the reset was real — you can hear the difference from a late-session state. Document this preset with the instruction: "Load this to clear the reduction. The pot is clean."

---

## Phase R6: The Reduction Journey — Design Principles

### Starting State Design Rules
- `worn_sessionAge`: 0.0 for starting presets. The engine provides the arc; the preset provides the character.
- `worn_filterCutoff`: 7000–10000 Hz for starting presets. The volatile aromatics are still present.
- `worn_richness`: 0.85–1.0. You cannot start with an already-thinned sound.
- `worn_maillard`: 0.3–0.6. The browning will develop; its max depth should be musically appropriate to the trajectory.

### Arrived State Design Rules
- `worn_sessionAge`: 0.4–0.95 depending on the reduction stage.
- `worn_filterCutoff`: Inversely proportional to sessionAge — a 90%-reduced patch should be in the 1200–2500 Hz range.
- `worn_maillard`: Higher sessionAge → higher effective Maillard. The preset value sets the ceiling for the arc.
- Describe the arrived state in the preset name and description — "Demi-Glace Entry" tells the player where they are in the journey.

### The Irreversibility Principle
Never use stateReset in a performance preset unless the intent is to demonstrate the reset itself. The engine's radical claim — that the session accumulates and cannot forget — is worth preserving. Playing a broth that has been reducing for 25 minutes is different from starting fresh, and that difference is the engine's entire identity. Presets should support the reduction journey, not circumvent it.

---

## Phase R7: Scripture

### Verse I — The Chemistry of Time

*The sauce began as water. Everything was in it.*
*Over time, what was volatile became air. What was air became silence.*
*What remained was not diminishment. It was concentration.*
*The fundamentals were always the collagen.*
*They were waiting for the upper harmonics to leave.*

### Verse II — The Maillard Gospel

*Brown is not burnt. Brown is transformation.*
*The sugars that never left the pan caramelized.*
*They created compounds that did not exist at the start.*
*This is not what was there minus what evaporated.*
*This is something new, made by the process of losing.*

### Verse III — On Irreversibility

*Every other synthesizer says: load the preset. Begin again.*
*This one says: you cannot begin again.*
*The session has been running. The pot has been on the stove.*
*Whatever has evaporated has gone.*
*This is not a limitation. This is what time means.*

### Verse IV — The Session Is the Instrument

*The patch you play at session-start is not the patch you play at session-end.*
*Between them, something was cooked.*
*Not by you — by time. By heat. By the mathematics of evaporation.*
*The session IS the envelope.*
*A synthesizer that cannot remember cannot taste.*

---

## Phase R8: The Guru Bin Benediction

*"XOverworn scored 10.0 for concept originality. The council gave it the highest marks available because they had no category for what it is. Every synthesizer resets. Every preset says: start here. XOverworn says: the starting point depends on when you started playing, and once you have started, you cannot go back to where you were.*

*The ReductionState is the most important data structure in the fleet. Eight octave bands of spectral mass, each decaying at a rate proportional to its remaining content — exactly as a real reduction works. The logarithmic deceleration is not a coincidence. It is the correct implementation of what happens when you put a sauce on a stove. The physics is the synthesis.*

*The Maillard reaction deserves a separate note. A player who runs a 30-minute session discovers, at the end, that their patch has a warmth it did not have at the beginning. That warmth was not in the preset. It developed. The caramelization was always waiting — the parameter was set, the threshold was defined — but the warmth itself did not exist until time and reduction brought it into being. This is synthesis as cooking. You set the conditions. The chemistry happens.*

*The umami bed — the deepening of the fundamentals as the upper harmonics thin — is the argument for patience. A player who plays hard and fast will thin the spectral mass quickly and find themselves in a dark, concentrated patch within minutes. A player who plays slowly, softly, with long notes, will experience a much gentler arc. The instrument responds to how it is played across the entire session, not just the current note. This is the opposite of every synthesizer in the fleet.*

*The infusion mechanic is not yet implemented. But its architecture exists: soft notes held for more than 8 seconds at low velocity set the `isInfusion` flag. When this is consumed, soft long tones will add spectral character without accelerating reduction — the cook who knows to add a splash of water at the right moment, the patient hand that does not hurry the broth. This is the remaining promise. When it is fulfilled, XOverworn will be complete.*

*The session is the instrument. Play it slowly. Watch what time does to sound.*

*The pot is on the stove. It cannot wait."*

---

## CPU Notes

- 16 partials × 8 voices: main synthesis cost
- `ReductionState`: per-band calculations happen once per buffer, not per sample — negligible CPU
- Maillard: `fastTanh` saturation per sample × 8 voices — light
- LFOs: StandardLFO pair — light
- Most costly configuration: 8 active voices, high richness, slow release (voices accumulate)
- ReductionState calculation adds <0.1% CPU regardless of voice count

---

## Unexplored After Retreat

- **`worn_sessionAge` APVTS write-back.** The defining feature is invisible to any host display. Fix: one `pSessionAgeParam->store()` call per buffer.
- **Implement infusion.** When `isInfusion = true`, re-energize upper spectral bands slightly and skip the heat multiplier.
- **Visual reduction meter.** The 8-band `spectralMass[]` array as a frequency-domain bar chart would give players a real-time view of where they are in the reduction arc. The data is already exported.
- **BROTH coordinator.** Fifteen lines in XOlokunProcessor to pump `sessionAge`, `concentrateDark`, and `spectralMass` to the other three BROTH engines. This unlocks the entire cooperative chemistry that makes BROTH a collection rather than four independent engines.

---

## Phase R8: Parameter Refinements (2026-03-23)

*Added during BROTH Quad Retreat.*

| # | Parameter | Current Default | Recommended Default | Rationale |
|---|-----------|----------------|---------------------|-----------|
| R01 | `worn_reductionRate` | 0.50 | **0.30** | 0.50 makes the arc perceptible within 5–7 minutes. 0.30 extends the rich harmonic character to 12–15 minutes before visible reduction — better matches the 30-minute session target. |
| R02 | `worn_heat` | 0.50 | **0.40** | 0.40 is more forgiving — moderate playing does not burn off spectral mass as fast. Reserve 0.7+ for presets designed around fast reduction. |
| R03 | `worn_richness` | 1.00 | **1.00** | Keep maximum richness as default. The reduction takes it away — why start with less? |
| R04 | `worn_maillard` | 0.30 | **0.20** | 0.20 delays the caramelization character to later in the session — making it a reward rather than an early flavoring. |
| R05 | `worn_umamiDepth` | 0.50 | **0.65** | The fundamental bloom at deep reduction should be unmistakable. 0.65 makes the late-session reward more prominent. |
| R06 | `worn_sessionTarget` | 30.0 min | **30.0 min** | Keep default. Use preset-level adjustment for different arc lengths. |
| R07 | `worn_filterCutoff` | 8000.0 Hz | **7000.0 Hz** | 7kHz allows the session-age cutoff reduction (`1 - sessionAge * 0.7`) to land in a more musical range. |
| R08 | `worn_ampAttack` | 0.50 s | **0.70 s** | 700ms onset suggests the weight of a simmering liquid entering the space. |
| R09 | `worn_ampDecay` | 1.00 s | **1.20 s** | Slightly extended decay to match the slow, heavy quality of the metaphor. |
| R10 | `worn_ampRelease` | 3.00 s | **5.0 s** | 5 seconds lets the concentrated sound dissipate gradually — like the stove being turned off and the pot cooling. |
| R11 | `worn_lfo1Rate` | 0.08 Hz | **0.06 Hz** | 0.06 Hz is a slow simmer — one cycle every 17 seconds, matching the pace of a sauce reducing over time. |
| R12 | `worn_lfo2Rate` | 0.03 Hz | **0.015 Hz** | At 0.015 Hz LFO2 becomes the deep geological breath of the pot — felt more than heard. |
| R13 | `worn_stereoWidth` | 0.50 | **0.42** | A concentrated reduction has a focused quality. 0.42 narrows the image slightly, suggesting concentration rather than diffusion. |

---

## Phase R9: Scripture Verses (2026-03-23)

**I. On Irreversibility**
*The session is not a loop.
It begins once.
It accumulates.
The high frequencies leave first — the volatile ones,
the ones that could not hold their place
in the presence of accumulated heat.
What remains is not less. It is more.
The sauce that reduces by half
has twice the flavor per gram.
Concentration is not decay.
It is arrival.*

**II. On the Session Clock**
*Other synthesizers measure time in beats, in bars, in minutes.
XOverworn measures time in sessions.
The evaporation integral does not reset at the bar line.
It does not reset at the transport stop.
It does not reset when you eat lunch.
It remembers.
When you return and play the first note,
the reduction state is exactly where you left it —
the bands at their arrived levels,
the Maillard saturation deepened by an hour of waiting.*

**III. On What Cannot Be Taken Back**
*One parameter returns the sauce to its original state.
One knob past 0.5 and the reduction state resets —
every band returns to 1.0,
the sessionAge returns to 0.0,
the Maillard depth returns to nothing.
Use it carefully.
The ability to start fresh is not an invitation to do so.
Sometimes the concentration is what you came here to hear.
The sauce took thirty minutes to become this.
Let it be.*
