# XOlokun Ecosystem Landscape — 2026-03-14

**Compiled by:** Grand Surveyor Agent
**Purpose:** Unified briefing for 11 subsequent rounds of refinement work
**Sources:** All 24 seance records, 6 doctrine files, all 24 engine adapters, all standalone repos, Presets/ directory, CLAUDE.md, master specification
**Total ecosystem presets:** 1,625 (XOlokun Presets/) + additional standalone repo presets

---

## 1. Engine Completeness Matrix

This table covers all 24 integrated engines plus XOpal (concept/standalone). Status codes:
- **INTEGRATED** — Full adapter in Source/Engines/, builds, presets in XOlokun
- **INTEGRATED-THIN** — Adapter exists but delegates to standalone repo DSP; coupling stubs only
- **CONCEPT** — Design complete, no adapter yet

Seance scores are from the 24 seances held 2026-03-14.

| Engine (Gallery Code) | Source Instrument | Accent | Adapter Lines | Unique Params | Preset Count (XOlokun) | Standalone Presets | LFOs | Aftertouch | Coupling In | Coupling Out | Standalone Repo | auval | Seance Score |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| ODDFELIX (Snap) | OddfeliX | Neon Tetra Blue `#00A6D6` | 952 | 14 | 245 | 519 (XOddCouple) | 0 | No | switch-impl | 2 | XOddCouple | Yes (via XOddCouple) | ~C+ |
| ODDOSCAR (Morph) | OddOscar | Axolotl Pink `#E8839B` | 1,032 | 12 | 162 | 519 (XOddCouple) | 8 refs | No | 3 | 2 | XOddCouple | Yes (via XOddCouple) | 6.9/10 |
| OVERDUB (Dub) | XOverdub | Olive `#6B7B3A` | 1,315 | 36 | 188 | 0 | 23 refs | No | switch-impl | switch-impl | XOverdub | Yes | 7.4/10 |
| ODYSSEY (Drift) | XOdyssey | Violet `#7B2D8B` | 1,440 | 38 | 379 | 0 | 20 refs | No | switch-impl | switch-impl | XOdyssey | Yes | 7.6/10 |
| OBLONG (Bob) | XOblongBob | Amber `#E9A84A` | 1,744 | 37 | 347 | 0 | 8 refs | No | switch-impl | switch-impl | XOblongBob | Yes | 7.x/10 |
| OBESE (Fat) | XObese | Hot Pink `#FF1493` | 1,501 | 31 | 159 | 0 | 0 | No | switch-impl | switch-impl | XObese | Yes (documented) | 6.6/10 |
| ONSET | XOnset | Electric Blue `#0066FF` | 2,143 | 24 | 126 | — | 0 | No | 2 | 2 | — (native) | — | Ahead / XVC praised |
| OVERWORLD | XOverworld | Neon Green `#39FF14` | 515 (adapter only) | 1 (adapter; ~67 in standalone) | 53 | 280 | 0 | No | switch-impl | switch-impl | XOverworld | Yes | 7.6/10 |
| OPAL | XOpal | Lavender `#A78BFA` | 3,100 (incl. OpalDSP.h) | 53 | 120 | 150 (standalone) | 13 refs | No | switch-impl | switch-impl | XOpal | — | Concept reviewed |
| ORGANON | XOrganon | Bioluminescent Cyan `#00CED1` | 1,467 | 10 | 146 | — | 0 | No | switch-impl | switch-impl | — (native) | — | 8/8 PASS |
| OUROBOROS | XOuroboros | Strange Attractor Red `#FF2D2D` | 1,512 | 8 | 69 | — | 1 ref | No | switch-impl | switch-impl | — (native) | — | Production-ready |
| OBSIDIAN | XObsidian | Crystal White `#E8E0D8` | 1,581 | 25 | 1 | — | 10 refs | No | switch-impl | 3 | — (native) | — | 6.6/10 |
| ORIGAMI | XOrigami | Vermillion Fold `#E63946` | 1,919 | 24 | 1 | — | 13 refs | No | switch-impl | 2 | — (native) | — | Not scored |
| ORACLE | XOracle | Prophecy Indigo `#4B0082` | 1,798 | 23 | 1 | — | 11 refs | No | switch-impl | 4 | — (native) | — | 8.6/10 |
| OBSCURA | XObscura | Daguerreotype Silver `#8A9BA8` | 1,873 | 24 | 1 | — | 15 refs | No | switch-impl | 2 | — (native) | — | High / unanimous |
| OCEANIC | XOceanic | Phosphorescent Teal `#00B4A0` | 1,429 | 22 | 1 | 34 | 7 refs | No | switch-impl | 2 | XOceanic | Yes | 7.1/10 |
| OCELOT | XOcelot | Ocelot Tawny `#C5832B` | 3,014 (18 files) | 66 | 0 | 16 | 0 | No | custom | custom | XOcelot | — | 6.4/10 |
| OVERBITE (Bite) | XOppossum | Fang White `#F0EDE8` | 2,393 | 80 | 71 | 0 | 37 refs | Yes (1 ref) | switch-impl | switch-impl | XOppossum | Yes | Full approval |
| ORBITAL | XOrbital | Warm Red `#FF6B6B` | 1,479 | 24 | 1 | — | 0 | No | switch-impl | switch-impl | — (native) | — | APPROVED |
| OPTIC | XOptic | Phosphor Green `#00FF41` | 1,030 | 16 | 8 | — | 3 refs | No | 2 | 1 | — (native) | — | Revolutionary |
| OBLIQUE | XOblique | Prism Violet `#BF40FF` | 1,468 | 30 | 7 | — | 4 refs | No | 2 | 4 | — (native) | — | 5.9/10 |
| OSPREY | XOsprey | Abyssal Gold `#B8860B` | 2,002 | 25 | 0 | — | 4 refs (dead) | No | 3 | 2 | — (native) | — | APPROVE/CONDITIONAL |
| OSTERIA | XOsteria | (warm Mediterranean) | 1,900 | 34 | 0 | — | 4 refs | No | switch-impl | 2 | — (native) | — | Production-grade |
| OWLFISH | XOwlfish | Abyssal Gold `#B8860B` | 2,281 (13 files) | 42 | 0 | 16 | 0 | No | custom | custom | XOwlfish | — | 7.1/10 |

**Notes on "LFOs" column:** Value reflects number of LFO keyword occurrences in adapter `.h` file. Many engines reference LFOs in comments or coupling docs without implementing them. Zero means no LFO implementation found at all.

**Notes on preset counts:** XOlokun count = presets in `Presets/XOlokun/` that reference the engine by canonical name. Engines with count=1 typically have one legacy/stub preset.

---

## 2. Bug Severity Index

All bugs identified across the 24 seances, sorted by severity.

### P0 — Audio Broken (incorrect audio output or audio-thread safety risk)

| # | Engine | Description | Ghost Who Found It | Location |
|---|--------|-------------|-------------------|----------|
| P0-01 | OBSIDIAN | R channel filter bypass bug — one of the two filters is not applied to the right channel, creating a mono-left timbral imbalance | Dave Smith | `ObsidianEngine.h`, stereo filter processing section |
| P0-02 | OSTERIA | warmth filter (`warmthFilter`) only processes `mixL`; `mixR` is never passed through the warmth low-shelf filter — asymmetric timbre | Seance 24 | `OsteriaEngine.h` line 1227 (`mixL = warmthFilter.processSample(mixL)` only; no `mixR` equivalent) |
| P0-03 | ORIGAMI | Race condition risk — STFT uses 2048-pt FFT with 4x overlap; if `hopSampleCounter` advance and overlap-add accumulator write are not properly serialized, a glitch can occur when `maxBlockSize < kHopSize` | Seance 4 | `OrigamiEngine.h` kHopSize=512, block sizes <512 untested |
| P0-04 | OBSIDIAN | `pFormantResonance` and `pFormantIntensity` point to the same parameter ID — one parameter controls both, making formant resonance uncontrollable | Dave Smith | `ObsidianEngine.h`, parameter attachment section |

### P1 — Feature Dead (parameter exists, is in presets, does nothing)

| # | Engine | Description | Ghost Who Found It | Location |
|---|--------|-------------|-------------------|----------|
| P1-01 | ODDFELIX (Snap) | `snap_macroDepth` (M4 DEPTH) — declared, attached, cached, serialized in presets — DSP reads it once but immediately discards: `(void) macroDepth;` | Buchla, Tomita | `SnapEngine.h` line 350 |
| P1-02 | OWLFISH | `owl_morphGlide` — declared in layout, attached, cached, serialized in all 16 standalone presets — never read by any DSP path | Dave Smith | `OwlfishEngine.h` + `OwlfishParameters.h` |
| P1-03 | OBLIQUE | `oblq_percDecay` — loaded into `bounceClickDecay` at line 739 but the value is never passed to the `BounceOscillator.clickDecayCoefficient` or `updateClickDecay()`; the hardcoded default `0.99f` always wins | Dave Smith | `ObliqueEngine.h` lines 739, 229, 241 |
| P1-04 | ODYSSEY (Drift) | `crossFmDepth`, `crossFmRatio` — in `ParamSnapshot`, never wired in `Voice.h` DSP | Pearlman | `DriftEngine.h` |
| P1-05 | ODYSSEY (Drift) | `AfterTouch` and `ModWheel` mod sources allocated in ModSources struct, never populated with incoming MIDI data | Smith, Vangelis | `DriftEngine.h` renderBlock MIDI processing |
| P1-06 | OCELOT | `macro_1` through `macro_4` declared and named in layout but route to no DSP parameters — four dead knobs | Seance 3 | `OcelotEngine.h` macro implementation |
| P1-07 | OPAL | `opal_smear` — declared in concept/layout, never implemented in DSP (critical: smear is the signature scrub-time parameter) | Dave Smith | `OpalEngine.h` / `OpalDSP.h` |
| P1-08 | ODDOSCAR (Snap/Morph) | Multiple preset files reference `snap_oscTune`, `snap_attack`, `snap_sustain`, `snap_release` — none of these exist in SnapEngine's parameter layout, causing silent preset mismatches | Dave Smith | Presets/ + `SnapEngine.h` schema |
| P1-09 | OSPREY | LFO struct (`OspreyLFO`, 50+ lines, lines ~202-248) declared and fully implemented — never instantiated in the engine class; the code is dead | Klaus Schulze | `OspreyEngine.h` lines 202-248 |

### P2 — Quality Gap (missing feature, thin library, expressivity deficit)

| # | Engine | Description | Ghost Who Found It | Notes |
|---|--------|-------------|-------------------|-------|
| P2-01 | OBESE (Fat) | Zero LFOs in entire engine — only Perlin drift at 0.1 Hz on pitch; no filter LFO, no amp LFO, no modulation beyond slow pitch drift | Klaus Schulze | 6.6/10 seance score |
| P2-02 | OWLFISH | Zero LFOs among 50 parameters — no temporal modulation beyond ADSR envelope | Klaus Schulze | Mixtur-Trautonium novel but static |
| P2-03 | ODDFELIX (Snap) | Zero LFOs — spectrum frozen after attack; no filter envelope; linear velocity curve (should be exp/log) | Schulze, Moog | M4 no-op compounds this |
| P2-04 | ODDOSCAR (Morph) | Zero LFOs; no aftertouch modulation source; Moog ladder filter excellent but nothing moves after attack | Klaus Schulze | 6.9/10 seance score |
| P2-05 | OBLIQUE | Zero LFOs; 6 presets only (fewest in gallery); static after initial transient | Klaus Schulze | 5.9/10 seance score — lowest scored engine |
| P2-06 | ORBITAL | Zero LFOs visible in adapter — group envelope is the crown jewel but no autonomous slow modulation | Klaus Schulze | APPROVED but breathing gap |
| P2-07 | ORGANON | Zero LFOs in adapter code — Variational Free Energy system provides internal evolution but no standard LFO | Schulze | 8/8 PASS but gap noted |
| P2-08 | ONSET | Zero LFOs — XVC cross-voice system 3-5 years ahead of industry but without even one LFO presets calcify | Schulze, Moog | XVC is blessing, LFO gap is P2 |
| P2-09 | OVERWORLD (INTEGRATED-THIN) | Overworld adapter is 515 lines; the actual Overworld DSP lives in standalone repo — adapter delegates but coupling is minimal; velocity is 1 reference (not timbral); no expression, mono output documented | Vangelis | 7.6/10 seance |
| P2-10 | OCEANIC | Zero velocity response — every note plays at identical level regardless of touch; Chromatophore praised but no expressivity | Vangelis, Kakehashi | 7.1/10 seance |
| P2-11 | OBESE (Fat) | No CC message processing at all — no mod wheel, no expression pedal | Vangelis | No MIDI CC |
| P2-12 | OVERDUB (Dub) | No MIDI CC, no mod wheel, no aftertouch — "single sine LFO is the weakest link" | Klaus Schulze | 7.4/10 seance |
| P2-13 | ODYSSEY (Drift) | Climax never demoed in any preset — the signature feature goes unheard unless user knows to push JOURNEY past threshold | Buchla, Schulze, Vangelis | V007 vision |
| P2-14 | OBLIQUE | Only 6-7 presets total — lowest preset count in the gallery | Smith | 5.9/10 |
| P2-15 | OSPREY | ShoreSystem shared with Osteria is masterwork, but dead LFO code (P1-09) means no filter evolution across coastline phrases | Schulze | Conditional approval |
| P2-16 | ORACLE | Freeze/crystallize feature for living maqam preservation not yet implemented | Tomita, Schulze | V005 vision |
| P2-17 | ORGANON | Long-term memory (the instrument that learns) not yet implemented | Schulze, Buchla | V006 vision |
| P2-18 | OCELOT | Ecosystem Matrix is the novel feature but macros don't wire into it (P1-06) — the most interesting capability is inaccessible | Seance 3 | 6.4/10 |
| P2-19 | ODDOSCAR (Morph) | `snap_oscTune`, `snap_attack`, `snap_sustain`, `snap_release` referenced in preset files but not in engine params — schema drift will cause silent load failures | Smith | Preset schema audit needed |
| P2-20 | OPTIC | Zero-audio paradigm brilliant but onboarding is a barrier — no init preset that demonstrates the concept | Schulze, Kakehashi | V002 vision |
| P2-21 | OBLONG (Bob) | CuriosityEngine is the soul but under-routed — most interesting parameter has fewest modulation paths; no aftertouch | 5 ghosts | 7.x/10 |
| P2-22 | OBSIDIAN | Velocity-to-timbre mapping absent; velocity only scales amplitude | Vangelis, Smith | D001 violation |
| P2-23 | ENGINES: Osprey, Osteria, Oceanic, Orbital, Organon, Oracle, Ouroboros, Origami, Obsidian | Zero presets in XOlokun Presets/ (count = 0 or 1 stub) — no factory presets ship for these engines | All ghosts | Critical for usability |

---

## 3. Doctrine Violation Map

### D001 — Velocity Must Shape Timbre, Not Just Amplitude

*Velocity should drive filter brightness, harmonic content, excitation character — not merely volume.*

| Violating Engine | Nature of Violation | Severity |
|-----------------|--------------------|-|
| OCEANIC | Zero velocity response — every note identical regardless of touch | P2 |
| OVERWORLD | Velocity scales amplitude only; no timbre shaping | P2 |
| OBESE | No CC processing at all; velocity path unchecked | P2 |
| OWLFISH | Velocity only triggers armor and amplitude, not timbre | P2 |
| ODYSSEY (Drift) | AfterTouch and ModWheel declared as mod sources but never populated with MIDI data | P1 |
| ODDFELIX (Snap) | Linear velocity curve when musical dynamics are logarithmic | P2 |
| OCELOT | Velocity ref count = 1 — essentially no velocity routing | P2 |
| OPAL | Velocity mapping absent from implementation | P2 |

**Compliant engines:** OVERBITE (Bite) has aftertouch; OBLONG, OBSCURA, ORIGAMI have reasonable velocity references.

---

### D002 — Modulation Depth is the Lifeblood of Expression

*At least 2 LFOs, mod wheel/aftertouch, 4 working macros, 4+ mod matrix slots.*

| Violating Engine | Nature of Violation |
|-----------------|---|
| ODDFELIX (Snap) | Zero LFOs; M4 DEPTH is a no-op (void cast) |
| ODDOSCAR (Morph) | Zero LFOs; no aftertouch source |
| OBESE (Fat) | Zero LFOs; only 0.1 Hz Perlin pitch drift |
| OWLFISH | Zero LFOs among 50 parameters |
| OBLIQUE | Zero LFOs; dead `oblq_percDecay` |
| ORBITAL | Zero LFOs in adapter |
| ORGANON | Zero LFOs; relies only on VFE internal system |
| ONSET | Zero LFOs; XVC praised but no standard LFO |
| OCELOT | 4 macro knobs declared, all route to nothing |
| OVERWORLD | Adapter has zero LFO; standalone has some but coupling is thin |

**Compliant engines:** OVERBITE (37 LFO refs), OVERDUB (23), OPAL (13), OBSCURA (15), ORIGAMI (13), ORACLE (11), ODYSSEY (20).

---

### D003 — The Physics IS the Synthesis

*Rigor, citation, and precision in physically-motivated engines.*

| Status | Engine | Notes |
|--------|--------|-------|
| Fully compliant | OUROBOROS | Lorenz 1963, Rossler 1976, Chua 1983, Aizawa 1982 cited; empirically derived bounding boxes documented |
| Fully compliant | ORBITAL | Double-precision phase accumulation; group envelopes mathematically precise |
| Fully compliant | OBSCURA | Verlet-integrated mass-spring; scanned synthesis from physical first principles |
| Fully compliant | ORACLE | GENDY stochastic breakpoints from Xenakis; maqam intervals from musicological sources |
| Partially compliant | ORGANON | VFE system from Friston/Ashby; implementation praised but citation depth varies |
| Not applicable | Most other engines | Subtractive/FM/granular — not claiming physical modeling |

---

### D004 — Dead Parameters Are Broken Promises

*Every declared parameter must affect audio output.*

| Violating Engine | Dead Parameter | Impact |
|-----------------|---------------|--------|
| ODDFELIX (Snap) | `snap_macroDepth` (M4 DEPTH) — `(void)macroDepth;` | P1 — occupied M4 slot |
| OWLFISH | `owl_morphGlide` — never read by DSP | P1 — in all 16 presets |
| OBLIQUE | `oblq_percDecay` — read into local but value discarded | P1 — bounceClickDecay unused |
| ODYSSEY (Drift) | `crossFmDepth`, `crossFmRatio` — in snapshot, not in DSP | P1 |
| ODYSSEY (Drift) | `AfterTouch`, `ModWheel` mod sources — allocated, never populated | P1 |
| OCELOT | `macro_1`–`macro_4` — declared, route nowhere | P1 |
| OPAL | `opal_smear` — signature parameter, no DSP | P1 |
| OBSIDIAN | `pFormantResonance` & `pFormantIntensity` — same ID, one controls both | P0 |

---

### D005 — An Engine That Cannot Breathe Is a Photograph

*Every engine must have at least one LFO with rate floor ≤ 0.01 Hz.*

| Violating Engine | Gap |
|-----------------|---|
| OBESE (Fat) | Only 0.1 Hz Perlin pitch drift — no LFO |
| OWLFISH | Zero LFOs among 50 parameters |
| ODDFELIX (Snap) | Zero LFOs |
| ODDOSCAR (Morph) | Zero LFOs |
| OBLIQUE | Zero LFOs |
| ORBITAL | Zero LFOs in adapter |
| ORGANON | Zero standard LFOs (VFE only) |
| ONSET | Zero LFOs |
| OVERWORLD | Adapter has no LFO |
| OSPREY | LFO struct exists but is dead code — never instantiated |

---

### D006 — Expression Input Is Not Optional

*Velocity→timbre, at least one CC (aftertouch/mod wheel/expression), non-linear velocity curves.*

| Violating Engine | Expression Gap |
|-----------------|---|
| OCEANIC | Zero velocity response at all |
| OVERWORLD | Velocity amplitude only; no CC |
| OBESE | No CC processing whatsoever |
| OWLFISH | No aftertouch; velocity triggers only amplitude/armor |
| ODYSSEY (Drift) | AfterTouch/ModWheel sources allocated but never fed MIDI data |
| ODDFELIX (Snap) | Linear velocity curve (must be exponential) |
| OPAL | Velocity mapping absent |
| OBLONG (Bob) | No aftertouch; 5 ghosts flagged; CuriosityEngine unresponsive to touch |

---

## 4. Modulation Gap Map

### Zero LFO Engines (D005 Critical Violations)

These 10 engines have no working LFO implementation:

| Engine | LFO Status | Filter Envelope? | Notes |
|--------|-----------|-----------------|-------|
| ODDFELIX (Snap) | None | No | Spectrum frozen post-attack |
| ODDOSCAR (Morph) | None | No | Moog ladder excellent; nothing moves |
| OBESE (Fat) | None (0.1 Hz Perlin only) | No | 4-filter stereo is soul; static |
| OWLFISH | None | No | 50 params; zero temporal modulation |
| OBLIQUE | None | No | Dead percDecay compounds this |
| ORBITAL | None in adapter | Group envelope only | Crown jewel covers some gap |
| ORGANON | None standard | VFE internal | Metabolic system provides evolution |
| ONSET | None | No | XVC praised; no basic LFO |
| OVERWORLD | None in adapter | No | Standalone has some; adapter drops it |
| OSPREY | Dead code (never instantiated) | No | LFO struct exists, never used |

### Zero Aftertouch Engines

Every engine in the fleet except **OVERBITE (Bite)** has zero aftertouch implementation (`aftertouch` keyword count = 0). This is a fleet-wide D006 violation.

### Zero Mod Wheel Processing

No engine shows CC1 (mod wheel) processing in the XOlokun adapter layer. Some standalone instruments had mod wheel; the adapters strip it.

### Velocity-to-Timbre Status

| Engine | Velocity Routing | Quality |
|--------|-----------------|---------|
| OVERBITE (Bite) | 8 velocity refs; Bite/Belly macro driven by velocity | Best in fleet |
| OBLONG (Bob) | 9 refs; some timbre routing | Partial |
| OBSCURA | 16 refs; excitation velocity | Good |
| ODDOSCAR (Morph) | 6 refs; amplitude only | Weak |
| OVERDUB (Dub) | 8 refs; mostly amplitude | Weak |
| OCEANIC | 30 refs in code but no velocity response documented | Likely amplitude only |
| OVERWORLD | 1 ref | Amplitude only |
| OCELOT | 1 ref | Amplitude only |
| OWLFISH | 1 ref | Armor trigger only |

---

## 5. Coupling Readiness Matrix

All 24 engines implement `applyCouplingInput` and `getSampleForCoupling`. Quality varies significantly.

| Engine | `applyCouplingInput` Depth | `getSampleForCoupling` Quality | Coupling Types Claimed | Actual Implementation |
|--------|--------------------------|-------------------------------|----------------------|----------------------|
| OVERBITE (Bite) | switch-case with real routing | Post-FX stereo + envelope follower | Multiple types | Substantive — 5-macro driven coupling |
| ODDOSCAR (Morph) | 3 switch cases; filterCutoffModulation, morphModulation written | Real data | AmpToFilter, EnvToMorph, LFOToPitch | Well-implemented |
| OBLIQUE | switch-case with real routing | Post-FX stereo + envelope follower | 4 types | Good — coupling docs thorough |
| OSPREY | Reads real `excitationMod` + `pitchMod` | Real output | 3 types + ShoreSystem | ShoreSystem masterwork |
| ORACLE | switch-case with 4 output types | Real data | 4 types | Good |
| OUROBOROS | switch-case | Real chaotic oscillator data | Velocity outputs; Leash | Blessed by Smith |
| OPAL | switch-case | Grain cloud output | Multiple | Good |
| ORIGAMI | switch-case | Spectral output | 2 types | Good |
| OVERWORLD (adapter) | switch-case | ERA state | AmpToFilter, EnvToMorph, AudioToFM | Thin — delegates to standalone DSP |
| OCELOT | Custom (18-file structure) | Custom | Custom | Non-standard; may not work with MegaCouplingMatrix |
| OWLFISH | Custom (13-file structure) | Custom | Custom | Non-standard; may not work with MegaCouplingMatrix |
| Remaining engines | switch-case | Post-render stereo | Varies | Standard pattern; quality varies |

**Systemic Concern:** OCELOT and OWLFISH use non-standard coupling implementations due to their multi-file structure. Verify they are correctly registered with `MegaCouplingMatrix`.

---

## 6. Preset Library Health

Targets are approximate based on gallery size and engine role.

| Engine | XOlokun Presets | Standalone Presets | Target | Gap | Status |
|--------|-----------------|-------------------|--------|-----|--------|
| ODDFELIX (Snap) | 245 | 519 (XOddCouple) | 200 | Met | Healthy |
| ODDOSCAR (Morph) | 162 | 519 (XOddCouple) | 200 | -38 | Slightly thin |
| OVERDUB (Dub) | 188 | 0 | 150 | Met | Healthy |
| ODYSSEY (Drift) | 379 | 0 | 200 | Met | Richest single engine |
| OBLONG (Bob) | 347 | 0 | 200 | Met | Very healthy |
| OBESE (Fat) | 159 | 0 | 150 | Met | Acceptable |
| ONSET | 126 | — | 120 | Met | Healthy |
| OVERWORLD | 53 | 280 | 120 | -67 | Thin in XOlokun |
| OPAL | 120 | 150 | 150 | Met | Healthy |
| OVERBITE (Bite) | 71 | 0 | 150 | -79 | Thin |
| ORGANON | 146 | — | 120 | Met | Healthy |
| OUROBOROS | 69 | — | 100 | -31 | Thin |
| OPTIC | 8 | — | 60 | -52 | Very thin |
| OBLIQUE | 7 | — | 60 | -53 | Critically thin (5.9/10 engine) |
| OBSIDIAN | 1 | — | 100 | -99 | No presets (1 stub) |
| ORIGAMI | 1 | — | 100 | -99 | No presets (1 stub) |
| ORACLE | 1 | — | 100 | -99 | No presets (1 stub) |
| OBSCURA | 1 | — | 100 | -99 | No presets (1 stub) |
| OCEANIC | 1 | 34 | 100 | -99 | No XOlokun presets |
| OCELOT | 0 | 16 | 80 | -80 | Zero presets |
| OSPREY | 0 | — | 80 | -80 | Zero presets |
| OSTERIA | 0 | — | 80 | -80 | Zero presets |
| ORBITAL | 1 | — | 100 | -99 | No presets (1 stub) |
| OWLFISH | 0 | 16 | 80 | -80 | Zero presets |

**Total XOlokun presets:** 1,625
**By mood:** Foundation=286, Atmosphere=281, Entangled=346, Prism=321, Flux=217, Aether=174
**Thinnest mood:** Aether (174) and Flux (217) are significantly behind Entangled (346). Consider targeted Aether and Flux preset generation passes.

**Critical observation:** 11 engines have 0 or 1 preset. The gallery is not usable for engines: OBSIDIAN, ORIGAMI, ORACLE, OBSCURA, OCEANIC, OCELOT, OSPREY, OSTERIA, ORBITAL, OWLFISH, and OBLIQUE (7 presets).

---

## 7. Documentation Health

| Engine | Standalone CLAUDE.md | Concept Brief | Spec Doc | Seance Doc | Notes |
|--------|---------------------|---------------|----------|------------|-------|
| ODDFELIX (Snap) | Via XOddCouple | xo_odd... | Thorough | 24 seances | Sound design guide covers it |
| ODDOSCAR (Morph) | Via XOddCouple | — | Thorough | 24 seances | |
| OVERDUB (Dub) | Yes (XOverdub) | xoverdub_concept_brief.md | xoverdub in CLAUDE.md | Seance 7 | |
| ODYSSEY (Drift) | Yes (XOdyssey) | xodyssey_concept_brief.md | docs/XOdyssey_Master_Spec_v1.0.md | Seance 11 | |
| OBLONG (Bob) | Yes (XOblongBob) | xoblongbob_concept_brief.md | SPEC.md in repo | Seance 1 | |
| OBESE (Fat) | Yes (XObese) | xobese_concept_brief.md | Partial | Seance 15 | |
| ONSET | Native | xonset_concept_brief.md | xonset_architecture_blueprint.md | Seance 8 | Best-documented native engine |
| OVERWORLD | Yes (XOverworld) | xoverworld_concept_brief.md | In CLAUDE.md | Seance 12 | |
| OPAL | Yes (XOpal) | xopal_concept_brief.md | xopal_phase1_architecture.md | Seance 16 | |
| ORGANON | Native | xorganon_concept_brief.md | xo_organon_phase1_architecture.md | Seance 22 | |
| OUROBOROS | Native | xouroboros_concept_brief.md | xo_ouroboros_phase1_architecture.md | Seance 9 | |
| OBSIDIAN | Native | xobsidian_design_spec.md | xolokun_engine_roadmap_v3.md | Seance 20 | |
| ORIGAMI | Native | xorigami_design_spec.md | xorigami_design_spec.md | Seance 4 | |
| ORACLE | Native | xoracle_design_spec.md | xoracle_design_spec.md | Seance 21 | |
| OBSCURA | Native | xobscura_design_spec.md | xobscura_design_spec.md | Seance 2 | |
| OCEANIC | Yes (XOceanic) | xoceanic_concept_brief.md | xoceanic_master_spec.md | Seance 10 | Best spec of Vol.3 |
| OCELOT | Yes (XOcelot) | xocelot_concept_brief.md | Partial | Seance 3 | Macro deadness not documented |
| OVERBITE (Bite) | Yes (XOppossum) | xoverbite_concept_brief.md | docs/xolokun_integration_spec.md | Seance 13 | Note: params use plain names, NOT poss_ prefix |
| ORBITAL | Native | xorbital_concept_brief.md | xorbital_architecture_blueprint.md | Seance 5 | |
| OPTIC | Native | xoptic_concept_brief.md | Partial | Seance 6 | |
| OBLIQUE | Native | xoblique_concept_brief.md | Partial | Seance 19 | Lowest score — docs reflect thin state |
| OSPREY | Native | xosprey_concept_brief.md | xosprey_architecture_blueprint.md | Seance 23 | Dead LFO not flagged in spec |
| OSTERIA | Native | xosteria_concept_brief.md | xosteria_architecture_blueprint.md | Seance 24 | Warmth L-only bug not documented |
| OWLFISH | Yes (XOwlfish) | xowlfish_concept_brief.md | Partial | Seance 14 | morphGlide dead param not documented |

**Sound Design Guide coverage:** 20 of 24 engines covered. Missing: OCELOT, OSTINATO, OPENSKY, OCEANDEEP (per guide's own header). OUIE also absent.

**Documentation quality summary:** Standalone repos (XOblongBob, XOdyssey, XOverdub, XOppossum, XOverworld, XOceanic, XOwlfish, XObese) have the most complete documentation. Native-only engines (Organon, Ouroboros, Orbital, Optic, etc.) have architecture blueprints but no operational CLAUDE.md equivalents. No engine's documentation has been updated to reference seance findings — all seance knowledge lives only in the seance knowledge tree.

---

## 8. Priority Work Queue

Ranked by impact: P0 audio bugs first, then systemic issues, then high-visibility missing features, then polish.

### Tier 1 — P0 Audio Bugs (Fix Before Any Release)

| # | Item | Engine | Impact |
|---|------|--------|--------|
| 1 | Fix Obsidian R channel filter bypass — right channel missing one filter pass | OBSIDIAN | Broken stereo imaging |
| 2 | Fix Osteria warmth filter — add `mixR = warmthFilter.processSample(mixR)` after line 1227 | OSTERIA | Asymmetric warmth |
| 3 | Fix Obsidian `pFormantResonance`/`pFormantIntensity` same-ID collision | OBSIDIAN | Control is broken |
| 4 | Audit Origami STFT race condition — verify thread safety when `maxBlockSize < 512` | ORIGAMI | Potential audio glitch |

### Tier 2 — Systemic Issues (Affect Multiple Engines)

| # | Item | Engines Affected | Impact |
|---|------|-----------------|--------|
| 5 | Fleet-wide aftertouch pass — add channel pressure handling to every engine adapter's MIDI loop | All 23 engines (only Bite has it) | D006 violation fleet-wide |
| 6 | Fleet-wide velocity-to-timbre audit — ensure velocity drives at least one non-amplitude parameter per engine | OCEANIC, OVERWORLD, OBESE, OWLFISH, OCELOT, OPAL (worst cases) | D001 violation |
| 7 | Preset schema audit — reconcile preset files referencing non-existent params (`snap_oscTune`, `snap_attack`, `snap_sustain`, `snap_release`) | ODDFELIX (Snap) presets | Silent preset load failures |
| 8 | Verify OCELOT and OWLFISH coupling correctness — multi-file non-standard implementations may not interoperate with MegaCouplingMatrix | OCELOT, OWLFISH | Coupling broken silently |
| 9 | LFO minimum standard pass — add at least one LFO (rate 0.005–10 Hz, sine/triangle minimum) to each zero-LFO engine | ODDFELIX, ODDOSCAR, OBESE, OWLFISH, OBLIQUE, ORBITAL, ORGANON, ONSET, OVERWORLD, OSPREY (fix dead code) | D005 violation x10 |

### Tier 3 — High-Visibility Dead Parameters

| # | Item | Engine | Impact |
|---|------|--------|--------|
| 10 | Implement `snap_macroDepth` (M4 DEPTH) — wire to reverb send or FX depth | ODDFELIX (Snap) | D004 — M4 is a no-op |
| 11 | Implement `owl_morphGlide` — wire to SubharmonicOsc ratio portamento | OWLFISH | D004 — in all 16 presets |
| 12 | Fix `oblq_percDecay` — pass `bounceClickDecay` to `BounceOscillator.updateClickDecay()` | OBLIQUE | D004 — one-line fix |
| 13 | Wire Odyssey `crossFmDepth` and `crossFmRatio` into Voice.h DSP | ODYSSEY (Drift) | D004 — FM cross-mod never fires |
| 14 | Populate Odyssey `AfterTouch` and `ModWheel` mod sources from MIDI events | ODYSSEY (Drift) | D004 + D006 |
| 15 | Implement Ocelot macros 1-4 — wire to EcosystemMatrix biome parameters | OCELOT | D004 — 4 dead knobs |
| 16 | Implement `opal_smear` — wire to grain source buffer read-head scrub position | OPAL | D004 — signature parameter |
| 17 | Fix Osprey dead LFO — instantiate `OspreyLFO` member; wire to resonator tuning | OSPREY | D004 + D005 |

### Tier 4 — Preset Library Critical Gaps

| # | Item | Gap |
|---|------|-----|
| 18 | Build Obsidian preset library — target 60 presets (currently 1 stub) | -59 |
| 19 | Build Oracle preset library — target 60 presets (8.6/10 engine with no presets) | -59 |
| 20 | Build Orbital preset library — target 60 presets (APPROVED engine with no presets) | -59 |
| 21 | Build Origami preset library — target 40 presets | -39 |
| 22 | Build Obscura preset library — target 40 presets | -39 |
| 23 | Build Osprey + Osteria preset libraries — 40 presets each (ShoreSystem deserves showcase) | -80 total |
| 24 | Build Oblique preset library — target 40 presets (currently 7; also has worst seance score) | -33 |

### Tier 5 — Vision Implementation (High Creative Value)

| # | Item | Engine | Vision |
|---|------|--------|--------|
| 25 | Implement Odyssey Climax presets — create 10+ hero presets that pass JOURNEY threshold | ODYSSEY (Drift) | V007 — signature feature never demoed |
| 26 | Build Optic onboarding preset — one starter patch that demonstrates zero-audio paradigm | OPTIC | V002 — zero-audio needs discovery aid |
| 27 | Implement Oracle freeze/crystallize — freeze current GENDY breakpoints as a static wavetable | ORACLE | V005 |
| 28 | Implement Opal time-telescope coupling — Opal as universal input transformer for other engines | OPAL | V008 |

### Tier 6 — Quality Polish

| # | Item | Engine |
|---|------|--------|
| 29 | Add mod wheel (CC1) processing to top 6 engines (Bob, Drift, Dub, Bite, Snap, Morph) | Multi-engine |
| 30 | Exponential velocity curve for ODDFELIX (Snap) — replace linear `velocity * amplitude` with `velocity^2 * amplitude` | ODDFELIX |

---

## 9. Round 2–12 Recommendations

Given what the Grand Survey found, here is recommended allocation for 11 subsequent refinement rounds:

### Round 2 — P0 Audio Bug Fix Sprint (1 round)
**Focus:** Fix the 4 P0 bugs before anything else.
- Obsidian R channel filter bypass
- Osteria warmth L-only
- Obsidian formant parameter ID collision
- Origami STFT thread safety audit
- Deliverable: All P0 bugs resolved; Obsidian and Osteria sound correctly

### Round 3 — Dead Parameter Sweep (1 round)
**Focus:** Eliminate all D004 violations. One-line to medium fixes.
- Snap M4 DEPTH implementation
- Owlfish morphGlide wiring
- Oblique percDecay wiring (one-line fix)
- Ocelot macro routing (biggest effort in this round)
- Opal opal_smear implementation
- Osprey LFO instantiation
- Odyssey crossFmDepth + aftertouch/modwheel population
- Deliverable: Zero dead parameters across the fleet

### Round 4 — LFO Minimum Standard Pass (1 round)
**Focus:** Eliminate D005 violations across 10 zero-LFO engines.
- Add 1-2 LFOs to: Snap, Morph, Obese, Owlfish, Oblique, Orbital, Organon, Onset, Overworld adapter, Osprey (fix dead code)
- Minimum bar: sine LFO with rate 0.005–10 Hz, routing to filter cutoff or signature parameter
- Deliverable: Every engine breathes

### Round 5 — Expression & Velocity Pass (1 round)
**Focus:** D001 and D006 violations.
- Fleet-wide aftertouch (channel pressure) into MIDI processing loop
- Velocity-to-timbre for: Oceanic, Overworld, Obese, Owlfish
- Exponential velocity curve for Snap
- Mod wheel (CC1) basic routing for top 6 engines
- Deliverable: Every engine responds to player gesture

### Round 6 — Preset Library: New Engines Wave 1 (1 round)
**Focus:** Engines with zero presets that scored well.
- Oracle: 60 presets (8.6/10 — urgent)
- Orbital: 60 presets (APPROVED — urgent)
- Organon: Supplement to 200 (already 146, healthy but can grow)
- Deliverable: 120+ new presets for highest-quality engines

### Round 7 — Preset Library: New Engines Wave 2 (1 round)
**Focus:** Remaining zero-preset engines.
- Obsidian: 60 presets
- Origami: 40 presets
- Obscura: 40 presets
- Osprey: 40 presets
- Osteria: 40 presets
- Deliverable: All engines have ≥ 40 presets

### Round 8 — Oblique + Optic Quality Rescue (1 round)
**Focus:** The two lowest-scored surviving engines need concentrated attention.
- Oblique: 40 new presets; fix percDecay (done in Round 3); add LFO (done in Round 4)
- Optic: Onboarding preset for zero-audio paradigm; 30+ new presets
- Deliverable: Oblique reaches 50+ presets; Optic has onboarding story

### Round 9 — Ocelot + Owlfish Deep Fix (1 round)
**Focus:** Both are non-standard multi-file engines needing deeper work.
- Ocelot: Wire macros into EcosystemMatrix; validate coupling with MegaCouplingMatrix
- Owlfish: Validate coupling integration; implement morphGlide (done Round 3); add LFO (done Round 4)
- Deliverable: Both engines fully functional with macros working

### Round 10 — Vision Implementation Sprint (1 round)
**Focus:** Implement the highest-value seance visions.
- Odyssey Climax presets (V007)
- Oracle freeze/crystallize (V005)
- Opal time-telescope coupling (V008)
- Deliverable: Three flagship visions ship

### Round 11 — Preset Schema Audit + Migration (1 round)
**Focus:** Clean the preset database.
- Audit all 1,625 presets for non-existent parameter references
- Fix `snap_oscTune`, `snap_attack`, `snap_sustain`, `snap_release` schema drift
- Verify all preset `engines` arrays use canonical engine names (not legacy aliases)
- Verify 6-mood coverage for all integrated engines
- Deliverable: Clean preset database, no schema drift

### Round 12 — Final Integration QA + Mood Balance (1 round)
**Focus:** System-level quality before v1.0 call.
- Aether and Flux mood preset generation (currently thinnest at 174 and 217)
- auval validation for all engines
- Sound Design Guide update for 4 missing engines (OCELOT, OSTINATO, OPENSKY, OCEANDEEP)
- Seance findings incorporated into per-engine CLAUDE.md files
- Final coupling matrix smoke test (all 24 engines × 13 coupling types)
- Deliverable: XOlokun v1.0 feature-complete

---

## Appendix A — Standalone Repo Summary

| Repo | Source Lines | Presets | CMakeLists | CLAUDE.md | auval Documented |
|------|------------|---------|-----------|----------|-----------------|
| XOblongBob | 15,066 | 0 (in XOlokun: 347) | Yes | Yes | Yes (per MEMORY.md) |
| XOdyssey | 6,835 | 0 (in XOlokun: 379) | Yes | Yes | Yes (per MEMORY.md) |
| XOverdub | 3,563 | 0 (in XOlokun: 188) | Yes | Yes | Yes (per MEMORY.md) |
| XOppossum (OVERBITE) | 1,004,823* | 0 (in XOlokun: 71) | Yes | Yes | Yes (per MEMORY.md) |
| XOverworld | 9,764 | 280 | Yes | Yes | Yes (per MEMORY.md) |
| XOceanic | 2,032 | 34 | Yes | Yes | — |
| XOwlfish | 2,615 | 16 | Yes | Yes | — |
| XObese | 10,332 | 0 (in XOlokun: 159) | Yes | Yes | Yes (documented in CLAUDE) |
| XOddCouple (ODDFELIX+ODDOSCAR) | 76 files | 519 | Yes | Yes | — |
| XOcelot | 23 files | 16 | Yes | Yes | — |
| XOpal | 10 files | 150 | Yes | Yes | — |

*XOppossum line count appears inflated — likely includes binary or generated files in the `find` path. Actual source: MEMORY.md documents 53 source files, ~12,775 lines.

---

## Appendix B — Seance Score Summary

| Rank | Engine | Score | Key Strength | Key Gap |
|------|--------|-------|-------------|---------|
| 1 | Organon | 8/8 PASS | VFE publishable as academic paper | No standard LFO |
| 2 | Oracle | 8.6/10 | GENDY genuine; Buchla 10/10 | No presets; no freeze |
| 3 | Optic | Revolutionary | Zero-audio paradigm | Onboarding; only 8 presets |
| 4 | Ouroboros | Production-ready | Most scientifically rigorous | Very thin presets (69) |
| 4 | Osprey/Osteria | Approved / Production-grade | ShoreSystem masterwork | Dead LFO; warmth bug; no presets |
| 5 | Overbite (Bite) | Full approval | Best macro system in fleet | Thin presets (71) |
| 6 | Orbital | APPROVED | Group envelopes crown jewel | No LFO; 1 preset |
| 7 | Onset | Ahead of industry | XVC cross-voice novel | No LFO; no aftertouch |
| 8 | Odyssey (Drift) | 7.6/10 | Climax paradigm signature | Climax never demoed; dead params |
| 8 | Overworld | 7.6/10 | ERA triangle original | Thin adapter; no expression |
| 9 | Overdub (Dub) | 7.4/10 | Spring reverb blessed | Single sine LFO only |
| 10 | Oblong (Bob) | 7.x/10 | CuriosityEngine soul | Under-routed; no aftertouch |
| 11 | Oceanic | 7.1/10 | Triple-BBD chorus soul | Zero velocity response |
| 11 | Owlfish | 7.1/10 | Mixtur-Trautonium novel | Zero LFOs; dead morphGlide; no presets |
| 12 | Obese (Fat) | 6.6/10 | 4-filter stereo exceptional | Zero LFOs; no CC |
| 12 | Obsidian | 6.6/10 | Phase distortion correct | R channel bug; 1 preset |
| 13 | Ocelot | 6.4/10 | EcosystemMatrix novel | Dead macros; 0 presets in XOlokun |
| 14 | Morph (OddOscar) | 6.9/10 | Moog ladder excellent | Zero LFOs; no aftertouch |
| 15 | Obscura | High | Physics IS synthesis (unanimous) | 1 preset |
| 16 | Snap (OddfeliX) | ~C+ | Transient precision | M4 no-op; zero LFOs |
| 17 | Oblique | 5.9/10 (lowest) | Prismatic bounce concept | Dead percDecay; 7 presets; zero LFOs |

---

*Document generated: 2026-03-14 by Grand Surveyor Agent. All data sourced from live filesystem reads and seance knowledge tree. Update this document after each refinement round.*
