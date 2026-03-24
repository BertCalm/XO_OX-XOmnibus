# OUTLOOK Guru Bin Retreat — Working Notes
*Save point for resumption — 2026-03-24*

## Status
- Full engine source read (OutlookEngine.h, 731 lines, 24 params)
- 8 existing presets identified (Foundation, Atmosphere x2, Prism, Flux, Aether, Entangled, Submerged)
- OFFERING retreat read as template (R1-R5 structure)
- Sound design guide section read (lines 3073-3124)
- Retreat document NOT YET WRITTEN
- 10 awakening presets NOT YET WRITTEN

## Branch
`claude/add-engine-skill-visionary-4pFVP`

## Engine Identity
- **Gallery code:** OUTLOOK | **Accent:** Horizon Indigo `#4169E1`
- **Parameter prefix:** `look_`
- **Creature:** The Albatross — Surface Soarer
- **Polarity:** 25% feliX / 75% Oscar — atmospheric, wide, expressive
- **Voices:** 8
- **4 DSP Systems:** PANORAMA OSCILLATOR (dual wavetable, 8 shapes, opposite-direction scanning), PARALLAX STEREO FIELD (pitch→stereo width), VISTA FILTER (dual SVF LP+HP, velocity-scaled D001), AURORA MOD (dual LFO conjunction/opposition = luminosity breathing)
- **Coupling:** Receives AmpToFilter, LFOToPitch, EnvToMorph. Sends post-filter stereo.
- **Best pairings:** OPENSKY (shimmer stacking), OMBRE (memory/vision), OPAL (granular parallax), OXBOW (entangled reverb tail)

## Phase R3: Proposed Default Refinements

| # | Parameter | Current | Proposed | Reason |
|---|-----------|---------|----------|--------|
| R1 | `look_lfo1Rate` | 0.5 Hz | **0.067 Hz** | Sutra 1 (Breathing Floor): 0.5 Hz is 2-second cycle — too fast for pad engine. 0.067 Hz = 14.9s deep breathing. Albatross thermal soaring rhythm. |
| R2 | `look_lfo2Rate` | 0.3 Hz | **0.113 Hz** | Sutra 2 (Coprime Drift): Current 5:3 ratio with LFO1 phase-locks in ~2s. 0.113 Hz gives 67:113 ratio — genuinely coprime. 8.85s period. Conjunction peaks every ~132s. |
| R3 | `look_lfo1Depth` | 0.3 | **0.35** | At slower rates, slightly deeper modulation is needed to remain audible. |
| R4 | `look_lfo2Depth` | 0.2 | **0.25** | Same reasoning — slower LFO2 needs marginally more depth for aurora luminosity to read. |
| R5 | `look_macroMovement` | 0.0 | **0.15** | D005 (Breathing): Engine must breathe by default. At 0.0, movementAmt falls to floor of 0.1 (code uses std::max(movementAmt, 0.1f)), producing only 0.1 × 0.3 = 0.03 filter mod — effectively dead. At 0.15, filter mod = 0.15 × 0.35 = 0.053 — subtle but alive. |
| R6 | `look_reverbMix` | 0.3 | **0.2** | Current reverb + macroSpace = 0.3+0.3 = 0.6 total reverb at init — muddy for first touch. 0.2+0.25 = 0.45 — cleaner default. |
| R7 | `look_macroSpace` | 0.3 | **0.25** | Works with R6 to produce cleaner init reverb level (0.45 combined). |
| R8 | `look_attack` | 0.1 | **0.3** | Pad engine — 0.1s attack is too percussive for albatross identity. 0.3s = gentle onset. |
| R9 | `look_release` | 0.8 | **1.5** | Pad voices need long release for smooth overlap. 0.8s cuts off too quickly for atmospheric character. |

## Phase R2: Signal Path Observations

### Panorama Oscillator
- Dual wavetable: 8 shapes (Sine, Triangle, Saw, Square, Pulse, Super, Noise, Formant)
- `horizonScan` sweeps both in opposite directions: scan1=horizon, scan2=1-horizon
- Osc2 detuned by 0.1% (phase2 += phaseInc * 1.001) — subtle width
- Each wave shape morphs with scan position (e.g., Sine adds odd harmonics, Triangle→Saw continuum)
- Super wave: only 3 detuned saws — could be richer (7-voice super is standard)

### Parallax Stereo
- noteNorm = (note - 36) / 60 → C2=0, C7=1
- spread = noteNorm × parallaxAmount → low notes narrow, high notes wide
- Equal-power panning with LFO2 micro-drift on pan angle (±0.1)
- Smart design: natural depth without reverb dependency

### Vista Filter
- LP + HP in series (LP first, then HP for low mud removal)
- LP cutoff = 200 + vistaLine×18000 + coupling + filterEnv×velFilterMod + LFO1×movement
- HP cutoff = 20 + (1-vistaLine)×300 — dark vista = more HP clearing
- Filter envelope shares amp ADSR timing (scaled: A×0.5, D×0.8, S=0, R×0.5)
- NO independent filter ADSR — design limitation (noted, not retreat scope)

### Aurora Mod
- conjunction = (lfo1 + lfo2) × 0.5 — when both positive, brightness peaks
- auroraLuminosity = 0.5 + conjunction × 0.5 → range [0.25, 0.75]
- Luminosity modulates amplitude: monoOut × (1 + (luminosity-0.5) × lfo2Dep × 0.4)
- At lfo2Dep=0.2: amplitude varies by ±4% — very subtle. At 0.25: ±5%.

### FX
- Ping-pong delay: 375ms L / 250ms R at 48kHz, 0.35 feedback (cross-feed)
- 4-tap allpass diffusion reverb: prime tap offsets [1117, 1543, 2371, 3079], feedback 0.45
- Space = reverb, not delay by default (delayMix default 0.0)

### Expression (D006)
- Mod wheel → movementAmt (scales LFO1→filter depth). Additive with macroMovement.
- Aftertouch → +4000Hz filter cutoff scaled by aftertouchDep. Opens vista.
- Velocity → filter brightness (D001): velFilterMod = vel×0.6 + 0.4 → soft vel=0.4 scales cutoff to 40%, hard vel=1.0 scales to 100%.

## 10 Awakening Preset Plan

| # | Name | Mood | Concept | Waves |
|---|------|------|---------|-------|
| 1 | Horizon Rest | Foundation | Clean init pad, albatross at rest | Sine + Tri |
| 2 | Aurora Rise | Atmosphere | Slow breathing, wide parallax, dusk | Saw + Sine |
| 3 | Albatross Soar | Atmosphere | Super wave, max parallax, long tail | Super + Sine |
| 4 | Indigo Shelf | Prism | Formant + saw scanning, resonant edge | Saw + Formant |
| 5 | Thermal Column | Flux | Fast movement, kinetic pad | Tri + Square |
| 6 | Vista Drone | Aether | Extremely slow, near-drone, minimal | Sine + Sine |
| 7 | Coastal Lead | Foundation | Faster attack, monophonic lead | Saw + Pulse |
| 8 | Shimmer Stack | Entangled | OUTLOOK + OPENSKY AmpToFilter coupling | Super + Saw |
| 9 | Memory Horizon | Entangled | OUTLOOK + OMBRE EnvToMorph coupling | Sine + Formant |
| 10 | Storm Front | Flux | Aggressive, dark, high movement | Square + Noise |

## Resume Instructions

To complete this retreat:
1. `git checkout claude/add-engine-skill-visionary-4pFVP`
2. Write full R1-R5 retreat document at `Docs/guru-bin-retreats/outlook-retreat-2026-03-24.md`
   - R1: Opening Meditation (Albatross mythology)
   - R2: Signal Path Journey (use observations above)
   - R3: Parameter Refinements (use table above)
   - R4: Two-Second Audition Test
   - R5: 10 Awakening Presets (use plan above, write full .xometa files)
3. Write 10 .xometa preset files to `Presets/XOlokun/{mood}/` directories
4. Commit and push to branch

Template: Follow `Docs/guru-bin-retreats/offering-retreat-2026-03-21.md` R1-R5 structure.
Existing presets at `Presets/XOlokun/{mood}/OUTLOOK_*.xometa` for format reference.
