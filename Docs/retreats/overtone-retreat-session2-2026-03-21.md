# OVERTONE Engine — Guru Bin Retreat Session 2
*Refinement Log | 2026-03-21*
*Engine: OVERTONE (Continued Fraction Spectral Synthesis) | Accent: Spectral Ice #A8D8EA*

---

## Session Context

This is the second Guru Bin retreat for OVERTONE. Session 1 (2026-03-20) produced 8 awakening presets and documented 7 key discoveries. Session 2 deepens those discoveries, explores three new parameter interactions, and addresses the Submerged mood gap identified in session 1.

**Prior retreat state:**
- 8 awakening presets created (Velocity Bloom, Euler Convergence, Shell Resonant, Shimmer Autonomous, Hadal Shimmer, Golden Moment, Inverted Spectrum, Ratio Zero)
- Submerged mood: 1 preset (Hadal Shimmer)
- Ghost parameters: 0 (clean)
- 69% Phi concentration identified as a gap

---

## Parameter Analysis

### over_constant (0=Pi, 1=E, 2=Phi, 3=Sqrt2)
**Default: 2 (Phi)**
**Assessment:** Default is correct — Phi produces the most immediately musical output (Fibonacci partials converge toward golden ratio). However it over-represents in the library. Pi is the most dramatically alien; E is the most mathematically unique (clumping behavior); Sqrt2 is the warmest at low depth (Pell fifths).

**Refinements applied:**
- Session 2 adds 3 Pi presets, 3 E presets, 2 Sqrt2 presets, 2 Phi presets
- New guidance: match constant to mood — Pi for Submerged/dark, E for Atmosphere/Aether, Sqrt2 for Flux/Prism, Phi for Foundation/Luminous

### over_depth (0.0 – 7.0)
**Default: 2.0**
**Assessment:** Default is reasonable but the extremes (0.0, 6.5-7.0) are the most dramatically distinct. Mid-depths (3-5) are the most musically usable. Depth 7 should be used when the full convergent character of the constant is the design goal.

**Refinements applied:**
- Phi at depth 7 paired with macroDepth=1.0 to enable live Fibonacci descent gesture
- E at depths 4 and 5 used for partial-collapse voicing (new discovery)
- Sqrt2 at depth 1 confirmed as warmest low-depth setting (3/2 = perfect fifth)

### over_partial0 – over_partial7 (0.0 – 1.0)
**Defaults:** 1.0, 0.5, 0.333, 0.25, 0.2, 0.167, 0.143, 0.125 (harmonic series falloff)
**Assessment:** The default 1/(n+1) taper is correct for foundational presets but severely limits character range. Three alternative taper shapes discovered:

1. **Uniform (all 0.75):** For E constant — emphasizes the clumping effect equally across all convergents
2. **Inverted (ascending):** For Crystalline presets — emphasizes upper Sqrt2 partials (first retreat: Ice Spectrum)
3. **Bell curve (peak at 2-4, reduced at 0 and 7):** For Pi Uncanny — focuses attention on the bent harmonic region

**Refinement rule:** Choose taper shape BEFORE choosing constant and depth. The taper determines which convergents are heard; the constant and depth determine which convergents exist.

### over_velBright (0.0 – 1.0)
**Default: 0.4**
**Assessment:** Default undersells the engine. The most expressive presets use velBright at 0.8-1.0. At 1.0, the dynamic range is enormous: pianissimo passages use only partials 0-2, fortissimo passages reveal all 8.

**Refinement rule:** For lead and expressive presets, push velBright to 0.85-1.0 and reduce partials 3-7 in the base patch. Velocity then acts as an additive synthesizer controller, opening partials sequentially.

### over_filterCutoff (1000 – 20000 Hz)
**Default: 12000 Hz**
**Assessment:** Default is bright. For Submerged and Atmosphere presets, cutoff should be in the 2000-6000 Hz range. High cutoff (16000-19000 Hz) works best for Prism and Crystalline.

**Refinement rule:** Map cutoff to mood:
- Submerged: 1500-4000 Hz (pressure, no shimmer)
- Foundation/Atmosphere: 8000-13000 Hz (balanced)
- Prism/Crystalline: 15000-20000 Hz (full spectral reveal)

### over_filterRes (0.0 – 0.8)
**Default: 0.3**
**Assessment:** Default is appropriate. Values above 0.6 add a resonant peak that interacts dramatically with Pi's bent harmonics — creates self-resonance near the 22/7 partial. For dramatic presets, use res=0.55-0.7 with Pi constant at depth 2-3.

### over_ampAtk (0.001 – 4.0s)
**Default: 0.02s**
**Assessment:** Default produces a near-instant onset appropriate for most uses. However the engine's Nautilus identity calls for long attacks (1.8-3.5s) in Submerged presets. The 4-second maximum is correct — attacks beyond 4 seconds feel geological and are not appropriate for a playable instrument.

**Refinement rule:** Match attack to conceptual depth:
- Pluck/lead: 0.001-0.015s
- Pad/keys: 0.02-0.35s
- Chamber/organ: 0.35-1.2s
- Submerged/deep: 1.2-3.5s

### over_lfo1Rate (0.01 – 10.0 Hz) — modulates over_depth
**Default: 0.25 Hz**
**Assessment:** LFO1 is the engine's primary movement parameter — it sweeps the convergent index, creating new partials as depth changes. The floor (0.01 Hz) should be used for geological drift. Medium rates (0.1-0.3 Hz) create organic breathing. High rates (1.5-4.0 Hz) create cascading effects.

**Critical insight:** LFO1 depth (over_lfo1Depth) above 0.5 at rates above 1.0 Hz creates the "waterfall" effect documented in Convergent Rush. This is the engine's most dramatic movement capability.

### over_lfo2Rate (0.01 – 10.0 Hz) — modulates partial rotation phase
**Default: 0.1 Hz**
**Assessment:** LFO2 provides spatial movement — rotating partial phases creates a subtle stereo widening and shimmer. At high rates (3-6 Hz) with moderate depth, LFO2 creates rapid iridescence best heard in Crystalline presets.

### over_resoMix (0.0 – 1.0)
**Default: 0.15**
**Assessment:** Default undersells the allpass resonator. Values above 0.65 transform the character fundamentally — the physical cavity becomes as important as the additive partials. At 0.9-0.95, a fast attack creates a Karplus-Strong pluck body (KS Birth). At 0.65-0.8, a slow attack creates chapel acoustics (Mesopelagic Choir, Convergent Vespers).

**Refinement rule:** Increase resoMix proportionally with attack time. Long attacks benefit from high resoMix (the resonator fills the acoustic space during the attack). Short attacks with high resoMix = pluck.

### over_macroCoupling (0.0 – 1.0) — autonomous shimmer activation
**Default: 0.0**
**Assessment:** The autonomous shimmer effect (amplitude flutter on partials 4-7 without any coupling partner) is active whenever macroCoupling > 0.5. The default of 0.0 deactivates this entirely. This is an under-used capability discovered in session 1 and deepened in session 2.

**Refinement rule:** For Flux presets, start with macroCoupling=0.7-0.85 and boost partials 4-7 to 0.8-0.95. The upper partials will animate with independent bioluminescent flutter. This is the engine's most distinctive single-parameter effect.

### over_macroSpace (0.0 – 1.0)
**Default: 0.3**
**Assessment:** Default is conservative. The Schroeder reverb is unusually clean for a spectral engine — high macroSpace (0.7-0.9) adds genuine spectral diffusion rather than muddy reverb. For Submerged presets, macroSpace=0.65-0.85 is appropriate.

---

## Submerged Mood Gap Analysis

**Before session 2:** 1 Submerged preset (Hadal Shimmer)
**After session 2:** 3 Submerged presets (+Pressure Psalm, +Mesopelagic Choir)

The engine's mythology explicitly places it in the Mesopelagic Zone (200-1000m). The Submerged mood is the correct home for OVERTONE's darkest settings. Three preset archetypes for Submerged:

1. **Pressure archetype** (Hadal Shimmer, Pressure Psalm): Pi constant, low depth, extreme low filter, very long attack, LFOs at 0.01 Hz floor. The mathematics of Pi at low depth produce bent harmonics that translate naturally to "pressure from above."

2. **Deep choir archetype** (Mesopelagic Choir): E constant at mid-depth, partial weighting toward mid-register, long ADSR, high resonator. The Euler clumping creates a narrow-band "choir" that sounds like voices compressed by water pressure.

3. **Abyssal drone archetype** (to be developed in session 3): E at depth 7, all partials uniform, extremely low filter (1500 Hz), resonator at maximum, release > 7 seconds. The mathematical clumping creates a drone that is harmonically static yet internally alive with beating.

---

## Constant Coverage Summary (after session 2)

| Constant | Session 1 Presets | Session 2 Presets | Total Awakening |
|----------|-------------------|-------------------|-----------------|
| Pi (0) | Hadal Shimmer | Pressure Psalm, Pi Uncanny | 3 |
| E (1) | Euler Convergence | Euler Clump, Mesopelagic Choir, Convergent Vespers | 4 |
| Phi (2) | Ratio Zero, Velocity Bloom, Golden Moment | Bioluminescent Pulse, Phi Singularity, KS Birth | 6 |
| Sqrt2 (3) | (none) | Sqrt2 Dawn, Pell Cascade | 2 |

Phi still leads but is no longer dominant. E and Pi are now properly represented.

---

## Mood Coverage Summary (after session 2)

| Mood | Before | Added Session 2 | After |
|------|--------|-----------------|-------|
| Foundation | Ratio Zero, Velocity Bloom | Euler Clump, KS Birth | 4 |
| Atmosphere | Euler Convergence, Inverted Spectrum | Sqrt2 Dawn, Convergent Vespers | 4 |
| Submerged | Hadal Shimmer | Pressure Psalm, Mesopelagic Choir | 3 |
| Flux | Shimmer Autonomous | Bioluminescent Pulse, Pell Cascade | 3 |
| Aether | Continued Silence, Golden Moment | Phi Singularity | 3 |
| Prism | Shell Resonant | Pi Uncanny | 2 |

---

## New Parameter Interaction Map

Three parameter interactions discovered in session 2 that are not documented elsewhere:

1. **E constant + depth 4-6 + uniform partial amplitudes = internal chord voicing**
   The Euler convergents at mid-depth have enough spread to form distinct intervals but enough convergence to sound unified. Setting all 8 partials to equal amplitudes (0.7-0.8) lets the mathematics decide the voicing.

2. **macroDepth=1.0 + Phi constant + depth=7 = Fibonacci descent gesture**
   With macroDepth at maximum, the DEPTH macro knob sweeps from depth 7 (34/21 ≈ φ) all the way to depth 0 (pure fundamental). This is a one-gesture harmonic descent through the entire Fibonacci sequence. No other constant offers as musically coherent a macro-sweep.

3. **resoMix > 0.85 + ampAtk < 0.005s = Karplus-Strong body emerges**
   Below attack time of 5ms, the allpass resonator's impulse response becomes the primary attack character. The additive partials arrive later as a harmonic overlay. This inverts the engine's normal balance: the physical model leads, the spectral model follows.

---

## Recommendations for Session 3

1. **Abyssal Drone** (Submerged): E constant depth 7, all partials 0.72, filterCutoff=1500, resoMix=0.92, ampRel=7.8. The deepest, most static version of the engine.

2. **Pi Metal Bell** (Prism): Pi constant depth 6-7, inverted partial taper (0 reduced, 7 boosted), fast attack 0.001s, short decay. Pi's alien ratios sound most dramatic as a struck bell. The large numerators of Pi's convergents (103993, 33102) create metallic inharmonicity.

3. **Seance OVERTONE** — the engine now has 18 awakening presets and 3 distinct mood archetypes. Seance conditions are met for the first formal quality assessment. Predicted score: 8.2-8.6.

4. **Sound design guide entry** — write the OVERTONE entry for `Docs/xomnibus_sound_design_guides.md` (currently missing; flagged in CLAUDE.md).

---

*Retreat log sealed 2026-03-21.*
*10 new awakening presets created. 4 scripture verses recorded.*
*Scripture: `scripture/retreats/overtone-retreat.md` (Second Session section)*
