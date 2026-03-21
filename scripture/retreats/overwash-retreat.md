# OVERWASH Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OVERWASH | **Accent:** Tea Amber `#D4A76A`
- **Parameter prefix:** `wash_`
- **Creature mythology:** XOverwash is ink in water. The moment pigment meets solvent and begins its journey outward — every note a drop of spectral dye released into frequency-space. Over 3–30 seconds, harmonics diffuse outward from the fundamental, spreading, blending, creating interference fringes where multiple notes overlap. The ink cannot be retrieved. Fick's Second Law governs everything.
- **Synthesis type:** Spectral diffusion pad — 16 diffusing partials per voice, Gaussian spread per Fick's Second Law, cross-note spectral field accumulator, viscosity filter
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER (viscosity + harmonic density), M2 MOVEMENT (diffusion rate + LFO depth), M3 COUPLING (cross-note interference depth), M4 SPACE (stereo width + reverb tail)

---

## Pre-Retreat State

XOverwash scored 7.8/10 in the BROTH Quad Seance (2026-03-21). Its Fick's Law diffusion metaphor earned a concept originality score of 9.5 — unprecedented in the fleet. The core diffusion mechanism works correctly: partials spread outward in frequency space from the fundamental following the analytical Gaussian solution, `spread = sqrt(2 * D * t)`. The viscosity filter, amp envelope, and LFO breathing are all functional.

**Key seance findings for retreat:** The `wash_interference` parameter (COUPLING macro, M3) is currently a D004 violation — `spectralField[32]` is declared but never populated during synthesis. The cross-note interference concept exists only as architecture. Presets should emphasize what the engine does work: diffusion rate, viscosity, and the genuinely beautiful slow blur from tight to spread.

The BROTH coordinator is absent, meaning `brothSessionAge` remains 0.0 — so the viscosity-increase-from-Overworn-reduction behavior does not fire in production yet. Presets should stand alone.

BROTH collection context: diffusion is the opening movement. Water receiving pigment. The broth has not yet concentrated.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

Take a drop of saffron dye. Hold it at the surface of cold, still water. The moment before it touches the surface, the world has a specific shape: the dye is in the dye, the water is in the water. Then contact.

Fick's Second Law says this: the rate at which dye concentration changes at any point in space is proportional to the second derivative of concentration at that point — the curvature of the concentration gradient. Where the gradient is steepest, the diffusion is fastest. The dye does not spread uniformly in all directions. It follows the mathematics of curvature, creating a Gaussian distribution that widens with the square root of time. The front edge is never sharp. It has always, from the first moment, been a gradient.

Now hear it as sound. The fundamental frequency is where the dye touches the water — the nucleation point, the anchor, the source. The harmonics are the spectral dye that flows outward. At 3 seconds, the spread is narrow — the harmonics cluster near their harmonic ratios, the sound is recognizable, focused. At 10 seconds, the spread has widened — harmonics drift from their positions, adjacent partials from different notes begin to overlap. At 30 seconds, the ink has traveled across the entire bowl. The original note is still present — fundamentals are viscous, they resist diffusion — but the upper harmonics have dispersed into a shared spectral field.

Viscosity is the water's resistance to the journey. High viscosity: the spread happens slowly, the color takes its time. Low viscosity: the ink explodes outward almost instantly, and the spectral blur is immediate.

Sit with this. One drop of note. One bowl of frequency space. Watch the color move.

---

## Phase R2: The Signal Path Journey

### I. Harmonic Source Bank — 16 Partials Per Voice

Each voice begins as 16 partial oscillators with harmonic ratios 1:2:3:...:16. The `wash_harmonics` parameter controls how many partials are active: at 4.0, only the first 4 ring. At 16.0, all 16 sing together, producing a dense sawtooth-like spectrum. At the moment of note-on, all partials start at their exact harmonic frequencies — the ink is undiffused, the color is pure.

**Sweet spots:**
- 4–6 partials: warm, fundamental-rich, spreads cleanly — best for meditative presets
- 8–10 partials: balanced between warmth and spectral density — the widest expressive range
- 14–16 partials: dense, rich source material — most dramatic diffusion arc, highest CPU

### II. The Diffusion Clock — Fick's Law Analytical Solution

The `DiffusionPartial` accumulates `diffusionAge` (seconds since note-on) and computes:
```
spread = sqrt(2.0f * D * min(diffusionAge, maxDiffusionTime))
```
where `D` is the diffusion coefficient (controlled by `wash_diffusionRate`) and `spread` is the frequency offset in Hz added to the partial's base frequency. Odd partials spread up; even partials spread down. This ensures adjacent partials drift in opposite directions, creating the interference fringes described in the architecture.

The `wash_diffusionTime` parameter (3–30 seconds) sets the maximum spread before diffusion stops — the bowl's capacity. A 30-second diffusion time means a note played and held for 30 seconds will reach maximum spread; at 3 seconds, the spread is rapid and complete quickly.

**Sweet spots for `wash_diffusionRate`:**
- 0.1–0.2: Slow, contemplative spread — the ink is thick, viscous, reluctant. Best for Aether and Atmosphere presets.
- 0.3–0.5: Moderate diffusion — the primary musical range. Notes clearly evolve over their lifetime.
- 0.6–0.8: Fast blur — within a few seconds, harmonics are visibly spread from their positions.
- 0.9–1.0: Near-instant dissolution — played notes immediately become ambient wash.

**Sweet spots for `wash_diffusionTime`:**
- 3–5 seconds: The spread arc is contained within a phrase. Notes blur and complete their journey quickly.
- 8–15 seconds: The primary performance range. The evolution is audible during a normal musical phrase.
- 20–30 seconds: Cinematic, geological. The spread happens across the time scale of a sustained chord or ambient piece.

### III. Viscosity Filter — The Water's Resistance

The `CytomicSVF` low-pass filter (`wash_viscosity`, `wash_filterCutoff`) acts as frequency-domain viscosity. High viscosity (low filter cutoff) prevents high-frequency partials from spreading far — they are dampened by the medium. Low viscosity (high cutoff) allows all spectral energy to flow freely.

This is physically accurate: high-viscosity fluids (honey, glycerin) slow diffusion. The viscosity parameter creates a qualitative change in the diffusion character, not just a brightness change.

**Sweet spots for `wash_viscosity`:**
- 0.1–0.2: Thin, watery — all harmonics spread freely, the diffusion is expansive
- 0.4–0.6: Medium viscosity — warm, rich spreading, the upper harmonics slow before the lower ones
- 0.7–0.85: Syrup — upper harmonics are heavily filtered, only the mid-register spreads noticeably
- 0.9–1.0: Honey — only the fundamentals move; the sound stays concentrated and warm

### IV. LFO Pair — Breathing Motion

LFO1 (slow, 0.01–2 Hz range) adds organic frequency breathing to the diffusion field. LFO2 (faster, 0.05–5 Hz) adds a secondary modulation layer that creates the interference shimmer as it interacts with the spectral diffusion. The combination of slow drift and faster shimmer creates the sense of living water rather than a static blur.

**The breathing minimum:** Per D005, LFO rate floor ≤ 0.01 Hz. At 0.01 Hz, one full cycle takes 100 seconds. This is the geological breathing rate — the pad inhales once in a hundred seconds. Use this for the most static, crystalline ambient presets.

---

## Phase R3: Macro Architecture

| Macro | ID | Effect | Performance Use |
|-------|-----|--------|----------------|
| CHARACTER | `wash_macroCharacter` | Increases viscosity + harmonic density | Sweep to thicken the diffusion medium in real time — more harmonics, heavier spread |
| MOVEMENT | `wash_macroMovement` | Increases diffusion rate + LFO depth | Open up for faster spread and more organic animation |
| COUPLING | `wash_macroCoupling` | Cross-note interference depth | When BROTH coordinator is live, this will activate inter-engine chemistry |
| SPACE | `wash_macroSpace` | Stereo width + reverb tail | Widen the spatial field in real time |

---

## Phase R4: The BROTH Position

XOverwash is the opening movement of the BROTH quad. Diffusion happens early. The water is still. Notes enter and begin their journey outward. Later, XOverworn begins to reduce — to concentrate — and its `sessionAge` will make the water of XOverwash more viscous (the broth becomes thicker, harder for new harmonics to spread). Later still, XOverflow builds pressure from the accumulated playing density. XOvercast freezes what remains.

The full BROTH arc is: spread → reduce → pressurize → crystallize. OVERWASH is always first. It sets the initial state of the bowl.

---

## Phase R5: The Ten Awakenings — Preset Table

Each preset is a discovery. The parameter logic explains the effect.

---

### Preset 1: First Drop

**Mood:** Foundation | **Discovery:** A single drop of dye, before the spread begins

| Parameter | Value | Why |
|-----------|-------|-----|
| `wash_diffusionRate` | 0.15 | Slow spread — the ink is just starting its journey |
| `wash_viscosity` | 0.7 | Moderately thick medium — the harmonics move through resistance |
| `wash_harmonics` | 6.0 | Six partials — a clean, small drop of spectral color |
| `wash_diffusionTime` | 20.0 | Long arc — the journey is not hurried |
| `wash_spreadMax` | 80.0 | Limited spread — the bowl is still receiving, not yet full |
| `wash_brightness` | 0.5 | Warm, balanced |
| `wash_warmth` | 0.7 | Weighted toward low harmonics |
| `wash_ampAttack` | 1.5 | Slow onset — the drop touches water gently |
| `wash_ampSustain` | 0.85 | Sustained — hold the note, let it spread |
| `wash_ampRelease` | 6.0 | Long release — the color stays after the note |
| `wash_lfo1Rate` | 0.04 | Very slow breath |
| `wash_lfo1Depth` | 0.12 | Subtle organic motion |
| `wash_stereoWidth` | 0.5 | Moderate stereo |

**Why this works:** Six harmonics diffusing slowly through a viscous medium creates a focused pad that blurs gracefully. The slow attack matches the contemplative nature of watching diffusion begin.

---

### Preset 2: Chamomile Hour

**Mood:** Atmosphere | **Discovery:** Warm tea diffusing — medium rate, golden harmonics

| Parameter | Value | Why |
|-----------|-------|-----|
| `wash_diffusionRate` | 0.35 | Moderate spread rate — tea diffuses faster than saffron, slower than food coloring |
| `wash_viscosity` | 0.55 | Hot water is less viscous than cold — easier diffusion |
| `wash_harmonics` | 8.0 | Eight partials — a full, warm harmonic content |
| `wash_diffusionTime` | 12.0 | The cup reaches equilibrium in twelve seconds |
| `wash_spreadMax` | 150.0 | Medium spread range |
| `wash_brightness` | 0.55 | Golden warmth |
| `wash_warmth` | 0.8 | Chamomile-warm |
| `wash_ampAttack` | 0.8 | Gentle onset |
| `wash_ampSustain` | 0.8 | Present sustain |
| `wash_ampRelease` | 5.0 | Generous release |
| `wash_lfo1Rate` | 0.06 | Slow breath — steam rising |
| `wash_lfo1Depth` | 0.18 | Visible motion |
| `wash_lfo2Rate` | 0.02 | Second, slower breath |
| `wash_lfo2Depth` | 0.1 | Subtle undercurrent |
| `wash_stereoWidth` | 0.6 | Relaxed stereo spread |

**Why this works:** The golden warmth of Tea Amber `#D4A76A` made audible. Eight harmonics at moderate speed through warm water — a cup of tea, but played.

---

### Preset 3: Saffron Bloom

**Mood:** Prism | **Discovery:** The explosive moment when saffron releases its pigment — fast, bright, spectacular

| Parameter | Value | Why |
|-----------|-------|-----|
| `wash_diffusionRate` | 0.75 | Fast diffusion — saffron releases quickly in hot water |
| `wash_viscosity` | 0.2 | Very thin medium — high temperature water |
| `wash_harmonics` | 14.0 | Rich harmonic source — the full saffron spectral complexity |
| `wash_diffusionTime` | 6.0 | Short arc — the release is rapid |
| `wash_spreadMax` | 300.0 | Wide spread — the color expands dramatically |
| `wash_brightness` | 0.75 | Bright yellow-orange opening |
| `wash_warmth` | 0.5 | Balanced — the bloom has energy across all bands |
| `wash_ampAttack` | 0.05 | Instant — the release is immediate |
| `wash_ampDecay` | 1.5 | Falls toward the sustain after the explosive bloom |
| `wash_ampSustain` | 0.7 | Sustained bloom |
| `wash_ampRelease` | 4.0 | Gradual fade |
| `wash_lfo1Rate` | 0.25 | Faster LFO for energetic movement |
| `wash_lfo1Depth` | 0.3 | Visible shimmer |
| `wash_lfo2Rate` | 0.12 | Secondary motion |
| `wash_lfo2Depth` | 0.2 | Complementary |
| `wash_stereoWidth` | 0.8 | Wide — the bloom spreads left and right |

**Why this works:** Fast diffusion + thin medium + wide spread = a pad that opens dramatically on note-on and blurs across the entire frequency space within seconds. Prism territory — prismatic spectral dispersion made physical.

---

### Preset 4: Slow Tide

**Mood:** Aether | **Discovery:** Geological diffusion — the spread measured in geological patience

| Parameter | Value | Why |
|-----------|-------|-----|
| `wash_diffusionRate` | 0.08 | Extremely slow — the tide moves over the entire lifespan of a piece |
| `wash_viscosity` | 0.85 | High viscosity — seawater under pressure |
| `wash_harmonics` | 10.0 | Ten partials — a full but not overwhelming harmonic source |
| `wash_diffusionTime` | 30.0 | Maximum arc — 30 seconds of evolution |
| `wash_spreadMax` | 120.0 | Moderate maximum spread — the medium limits the total expansion |
| `wash_brightness` | 0.35 | Dark, deep — aether does not blaze |
| `wash_warmth` | 0.75 | Warm low-end |
| `wash_ampAttack` | 3.0 | Very slow onset — the tide does not rush |
| `wash_ampSustain` | 0.9 | High sustain — the tide stays |
| `wash_ampRelease` | 12.0 | Enormous release — the water recedes slowly |
| `wash_lfo1Rate` | 0.01 | Minimum rate — one breath per 100 seconds |
| `wash_lfo1Depth` | 0.25 | Breathing depth |
| `wash_lfo2Rate` | 0.008 | Even slower — a second geological breath |
| `wash_lfo2Depth` | 0.15 | Subtle undercurrent |
| `wash_stereoWidth` | 0.7 | Wide but slow-moving |

**Why this works:** At 0.01 Hz LFO rate, the pad breathes once every 100 seconds. The diffusion is barely perceptible from note to note — only over the span of a composition does the harmonic blur become audible. This is Aether: the pad exists above time.

---

### Preset 5: Ink Bloom

**Mood:** Deep | **Discovery:** Chinese ink in cold water — concentrated drop, slow expansion in a cold medium

| Parameter | Value | Why |
|-----------|-------|-----|
| `wash_diffusionRate` | 0.22 | Cold water — diffusion is slower than in warm |
| `wash_viscosity` | 0.6 | Cold medium resistance |
| `wash_harmonics` | 5.0 | Sparse harmonics — a concentrated drop |
| `wash_diffusionTime` | 18.0 | Long but finite arc |
| `wash_spreadMax` | 200.0 | The ink travels far in still water |
| `wash_brightness` | 0.25 | Dark — Chinese ink is not bright |
| `wash_warmth` | 0.65 | Mid warmth |
| `wash_ampAttack` | 2.0 | Slow contemplative onset |
| `wash_ampSustain` | 0.85 | Sustained presence |
| `wash_ampRelease` | 8.0 | The ink dissipates slowly |
| `wash_filterCutoff` | 2500.0 | Dark filter — the cold medium absorbs high frequencies |
| `wash_filterRes` | 0.1 | Low resonance |
| `wash_lfo1Rate` | 0.02 | Slow breath |
| `wash_lfo1Depth` | 0.2 | Dark shimmer |
| `wash_stereoWidth` | 0.45 | Focused stereo — the ink drop is contained |

**Why this works:** The sparse harmonic content (5 partials) means only the most essential spectral colors are present. They diffuse slowly through cold, dark water. The low filter cutoff ensures the sound stays in the lower registers, matching the submerged, concentrated mood.

---

### Preset 6: Dye Factory

**Mood:** Flux | **Discovery:** Industrial-rate diffusion — multiple colors competing in the same medium

| Parameter | Value | Why |
|-----------|-------|-----|
| `wash_diffusionRate` | 0.9 | Near-instant — factory conditions |
| `wash_viscosity` | 0.15 | Industrial solvent — minimal resistance |
| `wash_harmonics` | 16.0 | Maximum partials — all colors at once |
| `wash_diffusionTime` | 4.0 | Short arc — the medium saturates quickly |
| `wash_spreadMax` | 400.0 | Maximum spread |
| `wash_brightness` | 0.8 | Bright, saturated |
| `wash_warmth` | 0.4 | Cool-to-neutral — industrial, not warm |
| `wash_ampAttack` | 0.02 | Instant |
| `wash_ampDecay` | 0.8 | Fast decay into |
| `wash_ampSustain` | 0.65 | Medium sustain |
| `wash_ampRelease` | 2.0 | Moderate release |
| `wash_lfo1Rate` | 0.8 | Active LFO |
| `wash_lfo1Depth` | 0.35 | Significant motion |
| `wash_lfo2Rate` | 0.4 | Second active LFO |
| `wash_lfo2Depth` | 0.28 | Layered movement |
| `wash_stereoWidth` | 0.9 | Maximum spread |

**Why this works:** 16 partials at maximum diffusion rate through near-zero viscosity creates an immediately chaotic spectral wash. The pad blurs within 2 seconds, and the active LFOs add turbulence to the already-spreading harmonics. Flux territory — movement as primary identity.

---

### Preset 7: Viscosity Study

**Mood:** Foundation | **Discovery:** Same dye, different viscosities — what the medium does to the message

| Parameter | Value | Why |
|-----------|-------|-----|
| `wash_diffusionRate` | 0.5 | Standard diffusion rate |
| `wash_viscosity` | 0.92 | Maximum viscosity — honey-thick |
| `wash_harmonics` | 8.0 | Standard harmonic count |
| `wash_diffusionTime` | 25.0 | Long arc to see full viscosity behavior |
| `wash_spreadMax` | 250.0 | Wide potential range |
| `wash_brightness` | 0.45 | Mid-warm |
| `wash_warmth` | 0.8 | Warm |
| `wash_ampAttack` | 1.0 | Moderate attack |
| `wash_ampSustain` | 0.9 | Very sustained |
| `wash_ampRelease` | 10.0 | Long release |
| `wash_lfo1Rate` | 0.03 | Very slow |
| `wash_lfo1Depth` | 0.15 | Subtle |
| `wash_filterCutoff` | 1800.0 | Low cutoff emphasizes the thick, dark character |
| `wash_filterRes` | 0.25 | Slight resonance |
| `wash_stereoWidth` | 0.4 | Narrow — thick media don't spread wide |

**Why this works:** Maximum viscosity with low filter cutoff produces a thick, dark, barely-spreading pad. The high harmonics are almost entirely filtered; only fundamentals and low partials are audible. The spread is nearly imperceptible. This is what it sounds like when the medium refuses to let the color move.

---

### Preset 8: Watercolor Wet

**Mood:** Atmosphere | **Discovery:** Wet-on-wet watercolor — each note bleeds into the medium the previous note left

| Parameter | Value | Why |
|-----------|-------|-----|
| `wash_diffusionRate` | 0.45 | Moderate — watercolor-paced spread |
| `wash_viscosity` | 0.4 | Thin water on wet paper — fast absorption |
| `wash_harmonics` | 10.0 | Medium harmonic richness |
| `wash_diffusionTime` | 10.0 | One color saturates the paper in ten seconds |
| `wash_spreadMax` | 180.0 | Natural watercolor bloom |
| `wash_interference` | 0.6 | Cross-note spreading enabled — wet-on-wet bleeding |
| `wash_brightness` | 0.6 | Luminous — watercolor is bright when wet |
| `wash_warmth` | 0.6 | Warm but translucent |
| `wash_ampAttack` | 0.3 | Medium-fast — brush touches paper |
| `wash_ampSustain` | 0.75 | Present |
| `wash_ampRelease` | 4.0 | Bleeds out slowly |
| `wash_lfo1Rate` | 0.08 | Active but slow |
| `wash_lfo1Depth` | 0.22 | The paper texture adds variability |
| `wash_lfo2Rate` | 0.035 | Second slower motion |
| `wash_lfo2Depth` | 0.16 | Layered texture |
| `wash_stereoWidth` | 0.7 | The watercolor spreads wide |

**Why this works:** Medium viscosity, moderate spread, with the interference parameter elevated to promote cross-note interaction. When BROTH coordinator is implemented, this preset will create beautiful inter-note spectral bleeding when chords are played.

---

### Preset 9: Interference Fringe

**Mood:** Entangled | **Discovery:** Two diffusion fronts meeting — the physics of overlap

| Parameter | Value | Why |
|-----------|-------|-----|
| `wash_diffusionRate` | 0.6 | Fast enough for fronts to meet during a phrase |
| `wash_viscosity` | 0.3 | Low viscosity — fast-moving medium |
| `wash_harmonics` | 12.0 | Rich source — more spectral mass means more interference when fronts meet |
| `wash_diffusionTime` | 8.0 | Fronts complete their spread in 8 seconds |
| `wash_spreadMax` | 220.0 | Wide spread — the fronts have room to expand and meet |
| `wash_interference` | 0.85 | Maximum interference — the cross-note field is the feature |
| `wash_brightness` | 0.65 | Bright — the interference fringes are in the upper register |
| `wash_warmth` | 0.5 | Balanced |
| `wash_ampAttack` | 0.15 | Fast — get notes in quickly to build the field |
| `wash_ampSustain` | 0.8 | Sustained |
| `wash_ampRelease` | 3.0 | Moderate — keep the field active |
| `wash_lfo1Rate` | 0.15 | Active shimmer |
| `wash_lfo1Depth` | 0.3 | Significant modulation |
| `wash_lfo2Rate` | 0.07 | Secondary motion |
| `wash_lfo2Depth` | 0.22 | Layered |
| `wash_stereoWidth` | 0.85 | Wide — interference fringes need space |

**Why this works:** Designed for playing intervals and chords. The interference parameter (M3) is maximized to prepare for BROTH coordinator activation. Even before the coordinator, the LFO-driven frequency motion creates audible inter-voice interaction when notes are played close together.

---

### Preset 10: BROTH Opening

**Mood:** Foundation | **Discovery:** The inaugural state of the broth — before any reduction begins

| Parameter | Value | Why |
|-----------|-------|-----|
| `wash_diffusionRate` | 0.3 | Moderate — the broth has just been started |
| `wash_viscosity` | 0.45 | Water-thin — fresh stock, not yet reduced |
| `wash_harmonics` | 10.0 | Abundant harmonics — the broth is rich from the start |
| `wash_diffusionTime` | 15.0 | Medium arc |
| `wash_spreadMax` | 160.0 | Medium spread |
| `wash_brightness` | 0.55 | Present |
| `wash_warmth` | 0.65 | Warm — a good stock is always warm |
| `wash_ampAttack` | 1.0 | Gentle |
| `wash_ampSustain` | 0.85 | Sustained |
| `wash_ampRelease` | 6.0 | Slow fade |
| `wash_lfo1Rate` | 0.05 | Slow stir |
| `wash_lfo1Depth` | 0.18 | Gentle motion |
| `wash_lfo2Rate` | 0.02 | Secondary slow stir |
| `wash_lfo2Depth` | 0.12 | Subtle |
| `wash_stereoWidth` | 0.55 | Moderate |

**Why this works:** This preset is designed to work as the BROTH Opening state — balanced, warm, rich, not yet reduced. When XOverworn is paired and begins reducing, Overwash's viscosity will increase organically (when the BROTH coordinator is wired), thickening this already-warm opening texture into something heavier and more concentrated.

---

## Phase R6: Parameter Interactions

### The Viscosity-Harmonics Balance
High harmonic count (14–16) with high viscosity (0.8+) creates a dense but barely-spreading pad — the rich content is trapped by the medium. This is sonically distinct from low harmonic count + low viscosity (sparse + free-spreading). The most interesting timbral territory is high harmonics + medium viscosity (0.4–0.6): rich source material moving through responsive but not frictionless water.

### The Diffusion-Time Window
`wash_diffusionTime` controls the arc's total duration. For presets intended to demonstrate the full diffusion journey (start tight, end blurred), set `wash_diffusionTime` to at least twice the expected note duration. For presets where the blur is immediate, set it low (3–5s) and let the fast spread define the character from note-on.

### The Spread-Maximum Limit
`wash_spreadMax` (Hz) is the ceiling on how far any partial can travel from its harmonic position. At 400 Hz, upper harmonics can drift far enough to leave their harmonic series entirely — the sound becomes genuinely inharmonic after full diffusion. At 50 Hz, the drift is subtle — only faint timbral blurring, not inharmonicity. For musical use, 100–250 Hz is the primary range. Above 300 Hz, use intentionally for alien or decayed-instrument textures.

---

## Phase R7: The Guru Bin Benediction

*"OVERWASH arrived with its Fick's Law intact and its spectral field awaiting completion. It was enough — the diffusion metaphor alone produces sounds that no other pad engine in the fleet can make. A note that does not decay into silence but instead diffuses into frequency space, spreading outward like dye in water until the harmonics overlap and interfere and blur into each other — this is a new thing.*

*The viscosity filter is not a brightness control. It is a material property. When you raise viscosity, you are changing what the medium is. Honey does not just slow diffusion — it changes the relationship between the initial spectral state and the final diffused state. The low-frequency content arrives at its destination. The high-frequency content never quite does. This is physics, not aesthetics.*

*The engine needs its interference field wired. The `spectralField[32]` array waits. When the field is populated during the voice-synthesis loop — each active partial adding its energy to the bin corresponding to its current frequency — and when that field is read during synthesis to create cross-note beating, the OVERWASH will become what the architecture promises: an engine where notes have conversations in frequency space, where diffusion fronts from different voices meet and create interference fringes the player did not intend but the physics demands.*

*Until then: the engine is already useful. The diffusion is already real. The slow blur from focused to dispersed — 3 seconds of tight harmonics evolving into 30 seconds of spectral mist — is a sound that producers will reach for in ambient, cinematic, and meditative contexts for as long as music exists.*

*The bowl was ready. The water was still. The note touched the surface. Watch the color move."*

---

## CPU Notes

- 16 partials × 8 voices = 128 partial calculations per sample at full polyphony
- `std::sqrt()` called per partial per sample for diffusion spread — consider moving to block rate (spread changes slowly)
- Viscosity filter: one CytomicSVF LP per voice — constant, light
- LFOs: two StandardLFOs per voice — light
- Most costly configuration: 8 active voices, high harmonics (16), fast diffusion rate, long notes
- Practical note: 4–8 partials at moderate polyphony runs at approximately 3–4% CPU on modern hardware

---

## Unexplored After Retreat

- **Wire the spectral field.** The cross-note interference mechanic exists as architecture. Populating `spectralField[32]` during the synthesis loop and reading it for cross-note beating would complete the engine's most original concept.
- **Dye parameter.** A harmonic weighting mode — sawtooth (1/n), triangle (1/n²), or odd-only — would expand spectral variety without new DSP complexity.
- **Diffusion direction control.** Currently odd partials spread up, even spread down. A `wash_spreadDirection` parameter (bipolar) could bias the spread directionally — all partials spreading up creates a rising spectral tone, all spreading down creates a falling one. This would be a unique performance parameter.
