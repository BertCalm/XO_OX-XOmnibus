# Producer's Guild Fleet Review: Sub-8.0 Engines
**Date:** 2026-03-20
**Scope:** 20 engines scoring below 8.0 in ghost council seances
**Perspective:** Working producers across EDM, Film/Ambient, Hip-Hop/Trap, Experimental, Lo-Fi

---

## Executive Summary

The XOlokun fleet has 20 engines below the 8.0 threshold. The issues fall into recurring patterns:

1. **Modulation starvation** — Many engines have only a single sine LFO or a hardcoded breathing LFO. Producers need shape-selectable LFOs with sync, rate, and depth controls.
2. **Missing filter envelopes** — Several engines lack independent filter ADSR, making classic bass plucks, acid lines, and evolving pads impossible.
3. **Mono output collapse** — Engines outputting mono or summing to mono before FX waste stereo field potential.
4. **Dead or weak macros** — Macros that map to a single parameter instead of compound multi-target sweeps feel lifeless.
5. **No pitch bend handling** — Bass and lead engines without pitch bend are unusable for expressive performance.

**Estimated lift if all P0 fixes applied:** 14 of 20 engines reach 8.0+.

---

## CRITICAL TIER (Below 7.0)

---

### 1. OBLIQUE (5.9 -> ~7.2 est.)
**Source:** `Source/Engines/Oblique/ObliqueEngine.h` (1100+ lines)

**Current state:** Prismatic bounce engine with 6-facet spectral delay, phaser, and wavefolder -- architecturally rich but hamstrung by a single hardcoded 0.2 Hz sine LFO that only modulates prism color spread +/-0.15. No user-controllable LFO module exists.

**What producers want:**
- **EDM/Dance:** BPM-syncable bounce rate so ricochet clicks lock to the grid. The bounce engine is *perfect* for rhythmic builds but currently free-runs with no host tempo awareness.
- **Film/Ambient:** Slow-sweep prism color modulation with triangle/saw shapes for evolving spectral shifts. The 6-facet spectral delay is a film scorer's dream if they can automate it.
- **Experimental:** Randomized bounce patterns (S&H modulating gravity/damping per bounce) for generative percussion textures.
- **Lo-Fi:** Tape-style wow on the prism delay feedback path for warped spectral echoes.

**Specific DSP improvements:**

**P0 — Add user LFO module (blocks 8.0)**
In `ObliqueEngine`, add a proper `ObliqueUserLFO` struct with:
- `oblq_lfo1Rate` (0.01-20 Hz, with host BPM sync option via `oblq_lfo1Sync` choice: Free/1/4/1/8/1/8T/1/16/1/32)
- `oblq_lfo1Depth` (0.0-1.0)
- `oblq_lfo1Shape` (int choice: Sine=0, Triangle=1, Saw=2, Square=3, S&H=4)
- `oblq_lfo1Dest` (int choice: FilterCutoff=0, PrismColor=1, PrismFeedback=2, WavefoldAmount=3, BounceDamping=4)
- Add identical `oblq_lfo2*` parameters for a second LFO (D002 requires 2+ LFOs)

Replace the hardcoded `obliqueLfoPhase` accumulator at line ~1060 with a call to the user LFO, and route the D005 breathing behavior through LFO1 defaults (rate=0.2, depth=0.15, shape=Sine, dest=PrismColor).

**P0 — Expose prism color LFO rate to user**
Currently at line 1060-1080, the 0.2 Hz rate and +/-0.15 depth are constants. Convert `kLfoRate` and `kLfoDepth` to parameter-driven values routed through the new LFO module above.

**P1 — BPM-syncable bounce rate**
In `ObliqueBounce::Params`, add a `syncToHost` flag. When enabled, quantize `rate` (ms) to the nearest musical division of the host BPM. Compute: `syncedRateMs = (60000.0f / hostBPM) * divisionFactor` where divisionFactor comes from a `oblq_bounceSyncDiv` choice param (1/4, 1/8, 1/8T, 1/16, 1/32).

**P2 — Stereo bounce pan scatter**
In `ObliqueVoice`, add per-bounce random pan offset using the bounce index: `panOffset = (bounceIndex % 2 == 0) ? -0.3f : 0.3f` scaled by a new `oblq_bounceStereo` param (0-1). This creates the "bouncing between walls" spatial effect.

**Estimated score impact:** P0 fixes -> 8.0. P0+P1 -> 8.4. All fixes -> 8.8.

---

### 2. OCELOT (6.4)
**Source:** `Source/Engines/Ocelot/OcelotEngine.h` + `OcelotVoice.h` + `EcosystemMatrix.h`

**Current state:** 4-stratum rainforest ecosystem engine (Floor/Understory/Canopy/Emergent) with a 12-route EcosystemMatrix for cross-stratum coupling. Architecturally ambitious but the coupling stub at line 107-118 is a complete no-op (`applyCouplingInput` does nothing), and the EcosystemMatrix's audible impact has never been demonstrated.

**What producers want:**
- **Film/Ambient:** The layered ecosystem concept is exactly what ambient producers want -- 4 independent timbral layers that interact organically. But they need to *hear* the interaction, not just read about it.
- **Experimental:** Randomized biome morphing with crossfade so the engine evolves autonomously between rainforest states.
- **Hip-Hop/Trap:** At minimum, the Floor stratum needs to deliver solid sub-bass with the Canopy providing high-hat style transients. The 4-layer architecture could be a one-engine beat machine.

**Specific DSP improvements:**

**P0 — Wire applyCouplingInput (blocks 8.0)**
At line 107-118, the coupling input is completely void-cast. Implement routing:
```cpp
case CouplingType::AmpToFilter:
    // Modulate Floor stratum's filter cutoff by source amplitude
    snapshot.floorFilterMod += amount * sourceBuffer[i];
    break;
case CouplingType::AudioToFM:
    // Modulate Canopy pitch by source audio
    snapshot.canopyPitchMod += amount * sourceBuffer[i] * 12.0f; // +/-12 semitones
    break;
```
Add `floorFilterMod` and `canopyPitchMod` fields to `OcelotParamSnapshot` with per-block reset.

**P0 — Make EcosystemMatrix audible**
In `OcelotVoice::renderBlock`, the EcosystemMatrix processes cross-feed between strata but the output needs to modulate actual synthesis parameters. Currently the matrix computes modulation values that go nowhere. Each of the 12 routes should map to: Floor->Canopy pitch offset, Canopy->Floor filter cutoff, Understory->Emergent amplitude, etc. Wire `ecosystemMatrix.getModulation(route)` to actual per-sample DSP parameter offsets.

**P1 — Biome automation LFO**
Add `ocelot_biomeLFORate` (0.001-0.5 Hz) and `ocelot_biomeLFODepth` (0-1) parameters. In `renderBlock`, slowly sweep `biomeMorph.setTargetBiome()` using a triangle LFO so the ecosystem evolves between biome states over 2-60 second cycles. This would make the engine self-animating for ambient work.

**P1 — Per-stratum level controls**
Add `ocelot_floorLevel`, `ocelot_understoryLevel`, `ocelot_canopyLevel`, `ocelot_emergentLevel` (0-1 each) so producers can manually balance the 4 layers. Currently the only timbral control is `strataBalance` which is a single tilt.

**Estimated score impact:** P0 fixes -> 7.8. P0+P1 -> 8.4.

---

### 3. OBESE (6.6)
**Source:** `Source/Engines/Fat/FatEngine.h` (1200+ lines)

**Current state:** 13-oscillator-per-voice multi-osc synth with ZDF ladder filters, arpeggiator, saturation, and bitcrusher. Mojo drift system for analog character. Has only a single `BreathingLFO` (line 692-705) -- a sine-only 0.07 Hz filter wobble with no user controls. The Mojo axis (analog drift amount) is static -- nothing modulates it over time.

**What producers want:**
- **EDM/Dance:** This is a supersaw engine begging for a proper unison detune spread, BPM-synced arpeggiator with host tempo lock, and sidechain-responsive filter ducking.
- **Hip-Hop/Trap:** 13 oscillators should deliver *massive* bass weight. Need the ZDF ladder filter exposed with full ADSR so it can do 808-style filter plucks. Sub oscillator needs an independent level control.
- **Lo-Fi:** The bitcrusher + Mojo drift is the right recipe but needs an LFO modulating Mojo amount so analog character breathes rather than sitting at a static value.

**Specific DSP improvements:**

**P0 — Add 2 user-controllable LFOs (blocks 8.0)**
Replace the `BreathingLFO` struct with a full `FatUserLFO`:
- `fat_lfo1Rate` (0.01-20 Hz) / `fat_lfo1Sync` (Free/1/4/1/8/1/16/1/32)
- `fat_lfo1Depth` (0-1) / `fat_lfo1Shape` (Sine/Tri/Saw/Square/S&H)
- `fat_lfo1Dest` (FilterCutoff/MojoAmount/Morph/SubLevel/SatDrive/CrushRate)
- Duplicate for `fat_lfo2*`
In `renderBlock` at the per-sample voice loop (~line 1035), replace `voice.breathingLFO.process(0.07f)` with the user LFO output routed to the selected destination.

**P0 — Expose sub oscillator level as independent parameter**
The sub osc at `FatVoice::subOsc` is always mixed at a fixed ratio relative to the 12 main oscs. Add `fat_subLevel` (0-1, default 0.5) so producers can boost sub independently for hip-hop bass.

**P1 — Mojo modulation target**
Add `fat_mojoLFO` (0-1) that routes LFO1 output to the Mojo drift amount parameter. This makes the analog character breathe -- at LFO peaks you get maximum drift, at troughs it cleans up. The Mojo axis (Blessing B015) is the engine's unique feature but it's static.

**P1 — Arpeggiator host BPM sync**
`FatArpeggiator::processBlock` at line 541 takes a `bpm` parameter but this appears to be an internal fixed value. Wire it to the host transport BPM via `juce::AudioPlayHead::getPosition()`. Add `fat_arpSync` bool param to toggle free-running vs host-synced.

**P2 — Unison detune spread control**
The 13 oscillators are fixed at root/-12/+12 per group. Add `fat_unisonDetune` (0-50 cents) that spreads each group's 3 oscillators by +/- detune cents relative to the group center, creating the classic supersaw spread effect.

**Estimated score impact:** P0 fixes -> 8.0. P0+P1 -> 8.5. All -> 8.8.

---

### 4. OBSIDIAN (6.6 -> ~8.2 est.)
**Source:** `Source/Engines/Obsidian/ObsidianEngine.h` (800+ lines)

**Current state:** Phase distortion synthesis engine with 2D PD lookup table (density x tilt), Euler-Bernoulli stiffness partials, 4-formant resonance network, and 2 per-voice LFOs (5 shapes). Post-recovery estimated at 8.2 after Round 9A fixes (R-channel filter bypass P0, formant param ID collision P0, velocity->PD depth added).

**What producers want:**
- **Film/Ambient:** The formant network is excellent for evolving vowel-like textures. Need formant frequency table morphing over time (not just static 4-band).
- **Experimental:** The 2D PD space (density x tilt) is genuinely unique. Producers want XY pad-style macro control that sweeps both axes simultaneously.
- **EDM/Dance:** PD synthesis produces punchy, bright timbres perfect for leads. Need faster LFO rates (up to 30+ Hz) for audio-rate FM-like effects.

**Specific DSP improvements:**

**P1 — Formant morph automation**
Add `obsidian_formantMorph` (0-1) that crossfades the 4 formant filter frequencies between two preset vowel tables (e.g., "ah" at 0.0 and "ee" at 1.0). In `ObsidianVoice`, interpolate the 4 bandpass center frequencies: `freq[i] = lerp(vowelA[i], vowelB[i], morphAmount)`. Route one of the existing LFOs to this target.

**P1 — PD XY macro**
Add `obsidian_macroXY` that simultaneously sweeps `obsidian_density` and `obsidian_tilt` along a diagonal path through the 2D PD space. Map it as: `density = macroXY`, `tilt = 1.0 - macroXY` for the default diagonal, with a `obsidian_xyAngle` (0-360 degrees) for custom sweep directions.

**P2 — Stiffness partial count control**
Currently 16 partials are computed. Add `obsidian_partialCount` (1-16) so producers can thin the spectrum for cleaner tones or max it for dense metallic textures. Partials above the count should have their amplitude zeroed.

**Estimated score impact:** Already at ~8.2 post-recovery. P1 fixes -> 8.6. All -> 8.9.
**Priority note:** OBSIDIAN is already above 8.0 estimated. Focus effort elsewhere first.

---

### 5. ODDOSCAR (6.9)
**Source:** `Source/Engines/Morph/MorphEngine.h` (1200+ lines)

**Current state:** Wavetable morphing pad engine with Moog ladder filter -- the "Oscar" half of the XO_OX duality. Excellent filter warmth (4-pole nonlinear ladder) but the only LFO is a hardcoded 0.3 Hz coupling sine (line 568-577). No user-accessible modulation. The morph oscillator (sine->saw->square->noise) is beautiful but static without modulation.

**What producers want:**
- **Film/Ambient:** This is supposed to be THE pad engine. Ambient producers need slow morph sweeps (LFO on morph position), filter movement, and long attack/release times. Currently the morph position only changes if you move the knob.
- **Lo-Fi:** The Moog ladder + morph osc could produce perfect vintage pad textures if there was tape-style modulation (wow/flutter on pitch, gentle filter wobble).
- **EDM/Dance:** The morph sweep from sine to noise is perfect for risers/buildups if an LFO or envelope can automate it.

**Specific DSP improvements:**

**P0 — Add 2 user LFOs (blocks 8.0)**
Add `MorphUserLFO` with shape selection (Sine/Tri/Saw/Square/S&H):
- `morph_lfo1Rate` (0.01-20 Hz) / `morph_lfo1Depth` (0-1) / `morph_lfo1Shape` (5 choices) / `morph_lfo1Dest` (FilterCutoff/MorphPosition/Pitch/Amplitude/Resonance)
- Duplicate for `morph_lfo2*`
In the per-sample loop at ~line 568, route LFO output to the selected destination. The default should be LFO1->FilterCutoff (Oscar's signature is warm filter movement).

**P0 — Filter envelope depth control**
The filter currently has no independent envelope. Add `morph_filterEnvDepth` (-1 to +1, bipolar) that scales an existing ADSR (or add a dedicated filter ADSR: `morph_fltAttack`, `morph_fltDecay`, `morph_fltSustain`, `morph_fltRelease`). Apply as: `effectiveCutoff = baseCutoff + envLevel * filterEnvDepth * 8000.0f` (Hz range).

**P1 — Morph position modulation**
Add `morph_morphLFO` (0-1) that routes LFO2 output to the morph oscillator's position. This turns static wavetable scanning into the animated, evolving pad sound that defines great wavetable synths like the PPG Wave.

**P1 — Aftertouch -> filter cutoff sensitivity**
The aftertouch handler currently just sets `v.vel = jmax(v.vel, atPressure)` which only affects amplitude. Add aftertouch -> filter cutoff modulation: `aftertouch * sensitivity * 4000.0f` added to cutoff. Add `morph_atFilterSens` (0-1, default 0.3).

**Estimated score impact:** P0 fixes -> 8.0. P0+P1 -> 8.5.

---

### 6. OLE (7.0)
**Source:** `Source/Engines/Ole/OleEngine.h` (460+ lines)

**Current state:** 3-aunt Latin string engine (Tres Cubano/Berimbau/Charango) with husband instruments (Oud/Bouzouki/Pin) and an Alliance shifting system. Uses FamilyWaveguide primitives. The `isHusband` regression post-SP7.5 may cause husband voices to never sound. 4 dead parameters were reported.

**What producers want:**
- **Film/Ambient:** The 3-aunt concept with sympathetic strings is perfect for world music film scores. Need longer sustain tails and adjustable sympathetic resonance density.
- **Lo-Fi:** The waveguide string models with drift could produce beautiful lo-fi guitar textures if the tremolo and drift parameters were more accessible.
- **Experimental:** The Alliance shifting system (2-vs-1 configurations) is creative but needs automation -- producers want to sweep through alliance configurations via an LFO.

**Specific DSP improvements:**

**P0 — Verify and fix isHusband regression (blocks 8.0)**
At line 146, husbands are gated by `if(v.isHusband && pDr < 0.7f) continue;` -- DRAMA must be above 0.7 for husbands to sound. Verify that husband voice allocation at line 57-61 correctly sets `isHusband=true` and that the DRAMA parameter (`mDrama`) is being read correctly. If the regression is that `mDrama` pointer is null or always returns 0, husbands are permanently silent. Fix: add null-check fallback `float pDr = mDrama ? mDrama->load() : 0.8f;` (default above threshold).

**P0 — Wire dead parameters**
Audit all 4 reported dead params. From the source: check `allianceCfg`, `allianceBlend`, `hOudLvl`, `hBouzLvl`, `hPinLvl` -- if any of these atomic pointers are never attached in `attachParameters()`, they'll read default values forever. Each must be wired to its corresponding APVTS parameter ID.

**P1 — Add proper LFO for Alliance sweep**
Add `ole_allianceLFORate` (0.01-2 Hz) and `ole_allianceLFODepth` (0-1). In renderBlock, modulate `alliancePos` (line 115) with a triangle LFO: `alliancePos += lfoOutput * lfoDepth * 3.0f` so the alliance configuration rotates continuously. This makes the 3-aunt system come alive.

**P1 — Aunt tremolo exposed as LFO**
Aunt 3 (Charango) has per-voice tremolo phase (line 18) but it appears to be tied to a fixed rate via `pA3Tr`. Convert this to a proper LFO with shape selection and expose it as the second LFO (D002 compliance). Rate: `ole_lfo2Rate` (0.01-20 Hz), Depth: `ole_lfo2Depth` (0-1), Shape: Sine/Tri.

**Estimated score impact:** P0 fixes -> 7.8. P0+P1 -> 8.3.

---

## NEEDS WORK TIER (7.0-7.9)

---

### 7. ODDFELIX (~7.0 est.)
**Source:** `Source/Engines/Snap/SnapEngine.h` (1066 lines)

**Current state:** Percussive transient engine with 3 osc modes (Sine+Noise, FM, Karplus-Strong), pitch-snap sweep, waveshaper, and HPF->BPF cascade. Has only a single hardcoded 0.15 Hz breathing LFO (line 432-434) on BPF center (+/-8%). The `snap_macroDepth` void-cast was fixed but the engine still feels one-dimensional.

**What producers want:**
- **EDM/Dance:** This is THE drum/percussion engine for electronic producers. It needs velocity-to-decay mapping, BPM-synced retrigger, and a proper modulation source for pitch-snap sweep rate.
- **Hip-Hop/Trap:** The FM mode + pitch snap is perfect for 808-style kicks if producers can control the pitch envelope independently. Need `snap_pitchEnvDecay` exposed.
- **Experimental:** Karplus-Strong mode should have damping and feedback controls exposed for metallic percussion textures.

**Specific DSP improvements:**

**P0 — Add 2 user LFOs with shape selection (blocks 8.0)**
- `snap_lfo1Rate` (0.01-30 Hz, with sync option) / `snap_lfo1Depth` (0-1) / `snap_lfo1Shape` (Sine/Tri/Saw/Square/S&H) / `snap_lfo1Dest` (FilterCutoff/PitchSnapRate/FMIndex/KSFeedback/NoiseLevel)
- `snap_lfo2Rate/Depth/Shape/Dest` (identical)
Replace the breathing LFO at line 432 with the user LFO system, defaulting LFO1 to Rate=0.15, Depth=0.08, Shape=Sine, Dest=FilterCutoff.

**P1 — Velocity-to-timbre mapping**
At note-on, velocity should modulate more than just amplitude. Add: velocity -> BPF center frequency (brighter hits at higher velocity), velocity -> pitch snap depth (harder hits have deeper pitch sweep), velocity -> FM index (harder = more metallic). Scale: `effectiveParam = baseParam + velocity * sensitivity * range`.

**P1 — Karplus-Strong damping exposure**
The KS oscillator has `damping` at line 89 but check if it's wired to a user parameter. If not, add `snap_ksDamping` (0-1, default 0.5) so producers can control the decay character from bright (low damping) to dull (high damping).

**Estimated score impact:** P0 -> 8.0. P0+P1 -> 8.4.

---

### 8. OCEANIC (7.1)
**Source:** `Source/Engines/Oceanic/OceanicEngine.h` (1468 lines)

**Current state:** 128-particle-per-voice boid flocking swarm synthesis with 4 sub-flocks, PolyBLEP particles, 2 LFOs, and DC blocker. Velocity response to timbre was resolved in Round 9E. The BBD chorus (Blessing B013) is praised but nothing creates dynamic movement beyond the internal boid rules.

**What producers want:**
- **Film/Ambient:** The swarm concept is a sound designer's dream. Need macro control over flock behavior: a single knob that sweeps from "tight school" (coherent unison) to "scattered chaos" (wide particle spread) with continuous morphing.
- **Experimental:** Murmuration trigger should be accessible via MIDI CC or macro, not just coupling input. Producers want to trigger cascade reorganizations manually.
- **EDM/Dance:** The 4 sub-flocks at frequency ratios (1x, 2x, 1.5x, 3x) are fixed. Allow user-tunable ratios for creating specific harmonic textures.

**Specific DSP improvements:**

**P0 — Dynamic movement macro (blocks 8.0)**
The 2 LFOs exist but the engine needs a compound MOVEMENT macro that simultaneously modulates:
- Boid `separation` strength (particles spread/cluster)
- Boid `cohesion` strength (how strongly they're pulled to attractor)
- Sub-flock ratios (slight detune between sub-flocks)
Add `ocean_macroSwarm` (0-1) that sweeps: at 0 = tight school (high cohesion, low separation), at 1 = scattered shoal (low cohesion, high separation, wider sub-flock detune).

**P1 — User-triggerable murmuration**
Add `ocean_murmuration` as a trigger parameter (button/momentary) or wire it to MIDI CC#64 (sustain pedal). When triggered, randomize all 128 particle positions and velocities within bounded ranges, then let boid rules re-converge. This creates dramatic textural shifts on demand.

**P1 — Sub-flock ratio tuning**
Replace the fixed `kSubFlockRatios[4] = {1.0, 2.0, 1.5, 3.0}` with user parameters: `ocean_flockRatio2` (0.5-4.0, default 2.0), `ocean_flockRatio3` (0.5-4.0, default 1.5), `ocean_flockRatio4` (0.5-4.0, default 3.0). This lets producers tune the spectral structure of the swarm.

**Estimated score impact:** P0 -> 7.8. P0+P1 -> 8.4.

---

### 9. OWLFISH (7.1)
**Source:** `Source/Engines/Owlfish/OwlfishEngine.h` + `OwlfishVoice.h` (2800+ lines across files)

**Current state:** Monophonic Mixtur-Trautonium engine (Blessing B014) with organ-pipe chain: Abyss Habitat -> Owl Optics -> Diet -> Sacrificial Armor -> Amp Envelope -> Abyss Reverb. Novel synthesis concept but monophonic-only, and the `morphGlide` and LFO issues were fixed post-seance.

**What producers want:**
- **Film/Ambient:** The Mixtur-Trautonium concept produces unique drone/organ textures. Need polyphony (at least 4 voices) so chords are possible.
- **Experimental:** The organ-pipe chain is perfect for experimental music. Need direct control over the subharmonic oscillator ratios (they're currently preset-driven).
- **Lo-Fi:** The Sacrificial Armor stage (saturation/compression) should have independent drive and mix controls for warmth shaping.

**Specific DSP improvements:**

**P0 — Verify LFO fixes are wired (blocks 8.0)**
Post-seance fixes added LFOs but verify in `OwlfishParamSnapshot.h` (332 lines) that `owl_lfo1Rate`, `owl_lfo1Depth`, `owl_lfo1Shape` parameters exist and are routed to destinations in `OwlfishVoice::renderBlock`. If they exist only as parameters but don't affect DSP, they're D004 violations.

**P1 — Add 4-voice polyphony option**
`OwlfishEngine` wraps a single `OwlfishVoice`. Add a voice pool of 4 voices with `owl_voiceMode` (Mono=0, Poly4=1). In Poly4 mode, allocate voices round-robin with LRU stealing. This doubles the engine's musical range.

**P1 — Subharmonic ratio control**
In `SubharmonicOsc.h` (147 lines), expose the subharmonic frequency ratio as `owl_subRatio` (choice: 1/2, 1/3, 1/4, 1/5, 1/6, 1/7, 1/8) so producers can select the undertone series position. The Mixtur-Trautonium's whole identity is subharmonic division -- making it selectable is essential.

**P2 — MorphGlide rate automation**
Add `owl_morphGlideRate` as an LFO-modulatable parameter (not just static) so the spectral morph between organ configurations sweeps automatically.

**Estimated score impact:** P0 -> 7.6. P0+P1 -> 8.3.

---

### 10. OTTONI (7.2)
**Source:** `Source/Engines/Ottoni/OttoniEngine.h` (460 lines)

**Current state:** 3-age brass waveguide engine (Toddler/Tween/Teen) with GROW macro that crossfades between age stages, lip buzz exciter, and sympathetic resonances. Instrument choice params were fixed. Per-voice vibrato exists for Teen section. Compact but effective.

**What producers want:**
- **Film/Ambient:** The 3-age concept (child->adult brass) is evocative for cinematic scores. Need longer reverb tails and better body resonance per instrument.
- **Hip-Hop/Trap:** Brass stabs are a staple. Need tighter attack, controllable release, and the ability to layer Toddler+Teen simultaneously (not just GROW crossfade).
- **EDM/Dance:** Need BPM-synced tremolo for rhythmic brass textures.

**Specific DSP improvements:**

**P0 — Verify D001 velocity->brightness (blocks 8.0)**
At line 58, velocity is passed as `msg.getVelocity()/127.f` but in the voice processing loop, check that velocity modulates `effTodPres` / `effTwEmb` / `effTnEmb` (embouchure = brightness). If velocity only affects amplitude (via `ampEnv=v`), add: `voiceBright *= (0.5f + vel * 0.5f)` -- harder playing = brighter timbre.

**P1 — Layer mode (simultaneous age stages)**
Currently GROW crossfades between ages. Add `otto_layerMode` (bool) that, when enabled, renders all 3 age stages simultaneously at their individual level params rather than crossfading. This lets producers create full brass sections with children, teens, and adults playing together.

**P1 — Reverb stereo spread**
The reverb at line 38-41 uses `revState[4]` which appears to be a simple mono allpass chain. Replace with a stereo Schroeder or FDN reverb using different allpass lengths for L/R channels. Add `otto_revWidth` (0-1) controlling the decorrelation amount.

**Estimated score impact:** P0 -> 7.8. P0+P1 -> 8.3.

---

### 11. OBRIX (7.2 -> 9.4 roadmap)
**Source:** `Source/Engines/Obrix/ObrixEngine.h` (1424 lines)

**Current state:** Modular "ocean bricks" synthesis engine with 2 sources, 3 processors, 4 modulators, 3 effects, 65 params across 3 waves of development. Wave 3 added Drift Bus, Journey Mode, and per-brick spatial. Has proper ADSR envelopes, LFOs with 5 shapes, and FM between sources. The 7.2 score was primarily because of zero factory presets at seance time.

**What producers want:**
- **All genres:** OBRIX is architecturally the most flexible engine in the fleet. The score is depressed by *lack of demonstration*, not DSP quality. 150 presets are in progress (Wave 4) and will showcase the engine properly.
- **EDM/Dance:** The FM source-to-source modulation + filter feedback + wavetable banks make this a complete production synth. Producers need BPM-synced delay in the FX chain.
- **Experimental:** Journey Mode (suppress note-off for indefinite evolution) is a killer feature for experimental music. Needs more visibility in presets.

**Specific DSP improvements:**

**P0 — Ship 150 factory presets (blocks 8.0)**
This is the #1 priority. The engine DSP is strong (65 params, 3 waves of development). What's missing is demonstration. Target:
- 22 Lesson presets (tutorial: one brick at a time)
- 128 genre presets (Techno/Lo-Fi/Experimental/Synthwave/EDM + place presets)
Wave 4 is in progress per memory.

**P1 — LFO rate ceiling increase**
Current LFO ceiling is 30 Hz (flagged in seance). Raise `ObrixLFO` max rate to 100 Hz for audio-rate territory. In `setRate`, change the clamping: `phaseInc = clamp(hz, 0.001f, 100.0f) / sr`. Audio-rate LFO->pitch creates FM-like effects, and LFO->filter creates aggressive resonant sweeps.

**P1 — BPM-synced delay FX**
In the delay effect (ObrixEffectType::Delay), add `obrix_delaySyncDiv` (choice: Free/1/4/1/8/1/8T/1/16) and wire to host BPM. When synced: `delayMs = (60000.0f / hostBPM) * divFactor`.

**P2 — Default init patch improvement**
The default Sine source gives the filter nothing to sculpt. Change default `obrix_src1Type` to Saw and `obrix_proc1Type` to LPFilter with cutoff at 2000 Hz. This makes the engine sound interesting immediately on load.

**Estimated score impact:** P0 (presets) -> 8.5+. P0+P1 -> 9.0+. The DSP is already there.

---

### 12. OVERDUB (7.4)
**Source:** `Source/Engines/Dub/DubEngine.h` (1357 lines)

**Current state:** Dub synth with voice->send VCA->drive->tape delay->spring reverb->master. Has analog drift, pitch envelope, and a single `DubLFO` (sine-only, line 198) with 3-way routing (pitch/filter/amp). Spring reverb earned Blessing B004. The single sine-only LFO is the weakest modulation in the fleet.

**What producers want:**
- **Hip-Hop/Trap:** Dub bass is a trap staple. The tape delay + spring reverb chain is excellent. Need proper bass weight: expose the sub oscillator as a dedicated parameter, and add a filter envelope for bass plucks.
- **Lo-Fi:** Perfect engine for lo-fi but needs the analog drift amount modulatable (not static), and tape flutter modulation should be more pronounced with a dedicated control.
- **EDM/Dance:** BPM-synced tape delay is essential. Also need the LFO to support triangle/saw/S&H shapes for rhythmic modulation.

**Specific DSP improvements:**

**P0 — Multi-shape LFO (blocks 8.0)**
`DubLFO` at line 198 is sine-only. Add shape selection:
- `dub_lfoShape` (int choice: Sine=0, Triangle=1, Saw=2, Square=3, S&H=4)
In `DubLFO::process()`, switch on shape (currently hardcoded `std::sin`). Add: tri = `4.0f * std::fabs(phase - 0.5f) - 1.0f`, saw = `2.0f * phase - 1.0f`, square = `(phase < 0.5f) ? 1.0f : -1.0f`, S&H = hold random value per cycle.

**P0 — Add second LFO (D002)**
Add `dub_lfo2Rate` / `dub_lfo2Depth` / `dub_lfo2Shape` / `dub_lfo2Dest` with the same routing options. Two LFOs targeting different destinations (e.g., LFO1->filter, LFO2->tape delay time) creates the evolving dub textures producers expect.

**P1 — Tape delay BPM sync**
Add `dub_delaySyncDiv` (choice: Free/1/4/1/8/1/8T/1/16/dotted-1/8). When not Free, override delay time with `60000.0f / hostBPM * divFactor`. Dotted-eighth is the classic dub delay rhythm.

**P1 — Tape flutter depth control**
The `DubAnalogDrift` controls general pitch drift, but tape flutter (periodic speed variation) needs its own parameter: `dub_flutterRate` (0.5-8 Hz) and `dub_flutterDepth` (0-1). Apply as pitch modulation in the delay feedback path, not the oscillator. This is what makes dub delays sound like actual tape.

**Estimated score impact:** P0 -> 8.0. P0+P1 -> 8.5.

---

### 13. OHM (7.6)
**Source:** `Source/Engines/Ohm/OhmEngine.h` (587 lines)

**Current state:** Family waveguide engine with Dad (physical modeling), In-Laws (theremin/glass/spectral freeze/grain), and Obed (2-op FM). MEDDLING + COMMUNE dual-axis macros. Mono voice summing is the main issue -- all voices sum to mono before output. D001 half-honored (intensity but not brightness).

**What producers want:**
- **Film/Ambient:** The theremin In-Law component + spectral freeze is excellent for cinematic eeriness. Need stereo spread per voice section (Dad left-center, In-Laws right-center, Obed spread).
- **Experimental:** The glass harmonica partials + granular scatter are unique. Need grain density and scatter amount exposed as macro-modulatable.
- **Hip-Hop/Trap:** Obed's 2-op FM with 8 ratio presets can produce punchy bass. Need the FM envelope attack/decay exposed and velocity-responsive.

**Specific DSP improvements:**

**P0 — Stereo voice spread (blocks 8.0)**
In the voice rendering loop, instead of summing all voices to a mono mix and then stereo-izing, assign per-voice pan positions:
- Dad voices: slight random pan offset per voice (+/-0.2 from center) using voice index
- In-Law voices: wider spread (+/-0.4) for spatial eeriness
- Obed voices: center (bass weight)
Apply equal-power pan law: `gainL = cos(pan * PI/2)`, `gainR = sin(pan * PI/2)`.

**P0 — D001: velocity -> brightness**
Currently velocity only maps to intensity (amplitude). Add velocity -> Dad waveguide damping filter cutoff: `dampingBright = baseDamp + velocity * 0.3f`. Harder playing = brighter, more harmonically rich pluck. For Obed: velocity -> FM index: `effIndex = baseIndex * (0.5f + velocity * 0.5f)`.

**P1 — SIDES macro as spatial LFO**
The SIDES macro currently has no LFO. Add `ohm_sidesLFO` (0-1) that auto-pans the In-Law section between L/R at a rate derived from the existing drift LFO. This creates the spatial movement that the engine's family metaphor implies -- in-laws "moving around the room."

**P1 — Grain density macro routing**
Wire the COMMUNE macro to also modulate grain density and scatter in `OhmGrainEngine` (line 84). Currently COMMUNE likely maps to a single parameter -- make it a compound macro that increases granular complexity.

**Estimated score impact:** P0 -> 8.1. P0+P1 -> 8.5.

---

### 14. ODYSSEY (7.6)
**Source:** `Source/Engines/Drift/DriftEngine.h` (1841 lines)

**Current state:** Drift/journey synth (from XOdyssey) with VoyagerDrift per-voice random walk, PolyBLEP oscillators, Cytomic SVF filter, and Climax system. Dead `crossFmDepth` was fixed. Aftertouch and ModWheel were wired. The Climax system (a crescendo/evolution destination) was never demonstrated in presets.

**What producers want:**
- **Film/Ambient:** The VoyagerDrift is the engine's identity -- organic, alive pads. Need the Climax system demonstrated: presets where a long sustained note evolves from quiet/dark to loud/bright over 30-60 seconds.
- **EDM/Dance:** The dual oscillator with cross-FM is good for leads. Need the filter envelope to have positive AND negative depth for acid-style resonant squelches.
- **Lo-Fi:** VoyagerDrift + gentle filter movement = perfect lo-fi pads if combined with tape saturation.

**Specific DSP improvements:**

**P0 — Climax system presets (blocks 8.0)**
The Climax system exists in DSP but was never demonstrated. Create 10-15 presets specifically showcasing it: long pad that evolves from whisper-quiet filtered drone to full-spectrum crescendo over 30 seconds. This is the engine's unique selling point.

**P1 — Bipolar filter envelope depth**
Check `drift_filterEnvDepth` -- if it's 0-1 only (unipolar), extend to -1 to +1 range. Negative depth sweeps the filter DOWN from the cutoff on note-on, creating inverted pluck effects essential for experimental and ambient work. In the envelope application: `cutoffMod = envLevel * filterEnvDepth * 8000.0f` where filterEnvDepth is now bipolar.

**P1 — Cross-FM depth modulation**
Now that `crossFmDepth` is fixed, add it as an LFO destination so the FM amount evolves over time. Static FM is usable but modulated FM creates the evolving metallic textures that make FM synthesis expressive.

**P2 — Drift-to-filter routing**
VoyagerDrift currently modulates pitch. Add `drift_driftToFilter` (0-1) that routes the same random walk to filter cutoff. This creates correlated pitch+filter movement that sounds organic.

**Estimated score impact:** P0 -> 8.0. P0+P1 -> 8.4.

---

### 15. OVERWORLD (7.6)
**Source:** `Source/Engines/Overworld/OverworldEngine.h` (672 lines)

**Current state:** Chip synthesis engine wrapping 6 retro console emulations (NES/FM Genesis/SNES/Game Boy/PC Engine/Neo Geo) with ERA triangle pad for 3-way blending. Signal chain: VoicePool -> SVFilter -> BitCrusher -> GlitchEngine -> FIREcho. Expression and mono output issues were resolved post-sweep.

**What producers want:**
- **Lo-Fi:** This IS the lo-fi engine. The BitCrusher + chip emulations are perfect. Need the crush amount automatable via LFO for evolving lo-fi textures.
- **EDM/Dance:** Chiptune leads over modern beats -- need proper portamento/glide for chip lead lines, and BPM-synced arpeggiator.
- **Experimental:** The GlitchEngine is powerful but needs finer control. What types of glitches? Can producers select between buffer repeat, stutter, reverse, pitch shift?

**Specific DSP improvements:**

**P0 — ERA triangle LFO modulation (blocks 8.0)**
The ERA macros (`ow_macroEra`, `ow_macroCrush`, etc.) exist but need an LFO to sweep them. Add `ow_eraLFORate` (0.01-5 Hz) and `ow_eraLFODepth` (0-1) that modulates the ERA X position over time, morphing between chip engines automatically. This transforms a static chip selector into a dynamic timbral evolution tool.

**P1 — Glitch type selection**
If `GlitchEngine` supports multiple glitch types, expose `ow_glitchType` (choice: BufferRepeat=0, Stutter=1, Reverse=2, PitchShift=3, Bitshift=4). If it currently only does one type, add at least buffer-repeat and stutter modes.

**P1 — Chip-accurate portamento**
Add `ow_portamento` (0-500 ms) with pitch slide quantized to semitones (chip-style discrete steps rather than smooth analog glide). This is how chip music sounds -- note transitions are stepped, not smooth.

**P2 — Stereo output width**
Post-sweep fix resolved mono but verify that the output uses proper stereo spread. Chip engines should have slight per-voice pan scatter based on the ERA vertex (NES-leaning = slightly left, Genesis-leaning = center, SNES-leaning = slightly right).

**Estimated score impact:** P0 -> 8.0. P0+P1 -> 8.4.

---

### 16. OVERTONE (7.6 -- re-seance)
**Source:** `Source/Engines/Overtone/OvertoneEngine.h` (901 lines)

**Current state:** Continued fraction spectral additive synthesis (Pi/E/Phi/Sqrt2 convergent tables) with 8 partials. Pi table spectral collapse at low depth was patched 2026-03-20. No anti-aliasing near Nyquist was patched. Declared 8-voice but implements 1.

**What producers want:**
- **Film/Ambient:** The mathematical basis creates unique metallic/bell tones impossible with standard synthesis. Need the 1-voice limitation fixed for chord work.
- **Experimental:** Sweeping between Pi/E/Phi/Sqrt2 convergent tables is a genuinely novel modulation axis. Need this exposed as a macro-controllable parameter.
- **EDM/Dance:** The metallic partial ratios could produce IDM-style bell leads. Need faster attack times and sharper transients.

**Specific DSP improvements:**

**P0 — Implement true 8-voice polyphony (blocks 8.0)**
The engine declares 8 voices but only implements 1. Add a voice pool array of 8 `OvertoneVoice` structs, each with independent phase accumulators for all 8 partials, amp envelope, and filter state. Implement LRU voice stealing with 5ms crossfade. This is the single most impactful fix.

**P0 — Verify Pi table patch correctness**
Post-patch, verify that entries 0-5 of the Pi convergent table now have meaningful spectral spread (they were all clustered near 1.0 pre-patch). The table should progress from near-integer ratios (clean harmonics) to irrational ratios (metallic inharmonicity) as depth increases.

**P1 — Constant type sweep**
Add `over_constantType` as a continuous 0-3 parameter (not discrete choice) so it crossfades between convergent tables: Pi(0) -> E(1) -> Phi(2) -> Sqrt2(3). This creates a unique modulation axis that no other synth offers. Wire to a macro and/or LFO destination.

**P1 — Partial brightness macro enhancement**
`over_macroColor` should not just boost upper partials uniformly. Implement spectral tilt: at Color=0, weight is `1/(n+1)` (natural harmonic rolloff). At Color=1, weight is inverted: `n/(kNumPartials)` (upper partials dominant). This creates a dramatic bright/dark sweep.

**Estimated score impact:** P0 -> 8.5 (polyphony is massive). P0+P1 -> 9.0.

---

### 17. OCEANDEEP (7.8)
**Source:** `Source/Engines/OceanDeep/OceandeepEngine.h` (883 lines)

**Current state:** Abyssal bass synth with 3-sine sub stack (fundamental/-1oct/-2oct), hydrostatic compressor, waveguide body (comb filter), bioluminescent exciter (bandpass noise), darkness filter (50-800 Hz LP). No filter ADSR and no pitch bend are the primary gaps -- critical for a bass engine.

**What producers want:**
- **Hip-Hop/Trap:** A bass synth without filter ADSR is fundamentally incomplete. The classic 808 pluck, acid bass, and every modern bass sound requires filter envelope. This is the single most critical gap.
- **EDM/Dance:** Sub bass needs pitch bend for the portamento bass drops that define modern bass music. Also need sidechain-responsive compression (external trigger on the hydrostatic compressor).
- **Film/Ambient:** The bioluminescent exciter (noise bursts) is perfect for abyssal atmospherics. Need rate and density controls exposed, not just level.

**Specific DSP improvements:**

**P0 — Add filter ADSR (blocks 8.0)**
Add 4 parameters: `deep_fltAttack` (0.001-2s), `deep_fltDecay` (0.001-5s), `deep_fltSustain` (0-1), `deep_fltRelease` (0.001-5s). Add `deep_fltEnvDepth` (-1 to +1, bipolar).
Create a `DeepFilterADSR` struct (identical to the amp ADSR pattern already in the file). In the per-sample render loop, compute: `effectiveCutoff = baseDarkness + filterEnv.process() * fltEnvDepth * 750.0f` (Hz range, scaled to the 50-800 Hz darkness filter range). Trigger on noteOn, release on noteOff.

**P0 — Add pitch bend handler**
In `renderBlock`'s MIDI processing loop, add:
```cpp
if (msg.isPitchWheel()) {
    pitchBendSemitones = (msg.getPitchWheelValue() - 8192) / 8192.0f * pitchBendRange;
}
```
Add `deep_pitchBendRange` (0-24 semitones, default 2). Apply to oscillator frequencies: `freq *= fastPow2(pitchBendSemitones / 12.0f)`.

**P1 — Bioluminescent exciter controls**
Expose `deep_bioRate` (0.1-10 Hz, rate of noise bursts), `deep_bioDensity` (0-1, burst length), `deep_bioBandwidth` (50-2000 Hz, bandpass width). These turn the exciter from a simple noise layer into a controllable alien atmosphere generator.

**P1 — Sidechain input for hydrostatic compressor**
In `applyCouplingInput`, map `CouplingType::AmpToFilter` to modulate the compressor's peak detector externally: `peak = max(peak, externalLevel * sidechainAmount)`. Add `deep_sidechainAmount` (0-1). This lets kick drums pump the bass -- the #1 most requested production feature for bass engines.

**Estimated score impact:** P0 -> 8.5. P0+P1 -> 8.9.

---

### 18. OMBRE (7.8 -> ~8.0 est.)
**Source:** `Source/Engines/Ombre/OmbreEngine.h` (941 lines)

**Current state:** Dual-narrative engine (Memory/Forgetting + Perception) with `OmbreMemoryBuffer` -- a circular delay-line with decay-on-read and 4 granular read heads. Had only 1 LFO (D002 requires 2); post-SP7.5 added LFO2 to reach ~8.0.

**What producers want:**
- **Film/Ambient:** The memory/forgetting concept is cinematically powerful. The granular read heads need more control: head count, spread, and drift rate as independent parameters.
- **Experimental:** The decay-on-read buffer is unique. Need the decay rate modulatable by LFO/envelope for evolving memory dissolution.
- **Lo-Fi:** The memory degradation concept IS lo-fi. Need a saturation stage in the feedback path so memories get warmer/muddier with each pass.

**Specific DSP improvements:**

**P1 — Grain head control parameters**
Add `ombre_grainHeads` (1-8, default 4), `ombre_grainSpread` (0-1, head temporal spacing multiplier), `ombre_grainDrift` (0-1, per-head drift rate). In `readGrains`, use `grainHeads` instead of hardcoded `kNumHeads = 4`, and scale the head offsets by `grainSpread`.

**P1 — Decay rate as LFO destination**
The memory decay rate is computed from parameters but not modulatable. Add decay rate as an LFO2 destination choice. When modulated: fast LFO = memories dissolve rhythmically (like a strobe revealing fragments). Slow LFO = breathing between remembering and forgetting.

**P1 — Feedback saturation**
In `OmbreMemoryBuffer::writeSample`, add optional `fastTanh` saturation before writing: `buffer[writePos] = fastTanh(sample * (1.0f + satDrive * 3.0f))` controlled by `ombre_memorySaturation` (0-1). Each replay through the buffer accumulates more warmth.

**Estimated score impact:** Already ~8.0. P1 fixes -> 8.5.

---

### 19. OBBLIGATO (7.8)
**Source:** `Source/Engines/Obbligato/ObbligatoEngine.h` (504 lines)

**Current state:** Dual-brother woodwind waveguide engine (Brother A: air-jet flute/clarinet; Brother B: reed oboe/bassoon) with 5 voice routing modes (Alternate/Split/Layer/RoundRobin/Velocity). FX chain routing is misrouted (V2 backlog). D001 intensity-not-brightness pattern.

**What producers want:**
- **Film/Ambient:** Woodwind duets are bread-and-butter for film scoring. The Layer mode (both brothers simultaneously) needs proper stereo imaging -- A left, B right.
- **Lo-Fi:** The waveguide woodwinds with drift could produce beautiful imperfect flute/clarinet textures. Need breath noise layer exposed.
- **Hip-Hop/Trap:** Woodwind stabs and runs are increasingly popular in modern hip-hop. Need tighter attacks and a velocity-to-brightness curve.

**Specific DSP improvements:**

**P0 — D001: velocity -> brightness, not just intensity (blocks 8.0)**
At voice render time, velocity currently only scales amplitude via `ampEnv=v`. Add velocity->waveguide damping: for Brother A (air-jet), `effFlutter *= (0.5f + vel * 0.5f)` and damping filter cutoff `dampBright = baseDamp + vel * 0.2f`. For Brother B (reed), `effBite *= (0.3f + vel * 0.7f)`. Harder playing = brighter, more harmonically rich timbre.

**P0 — Fix FX chain routing**
The FX chain was reported as misrouted. Audit the output path: if Brother A and B are mixed before FX, ensure the mix uses the routing mode's balance. If in Layer mode, verify both brothers reach the FX chain at proper levels. The fix is likely a missing gain staging correction.

**P1 — Layer mode stereo imaging**
In Layer mode (routing==2), pan Brother A to 0.3 (left-of-center) and Brother B to 0.7 (right-of-center) using equal-power pan law. Add `obbl_layerWidth` (0-1) controlling the spread. This creates a natural woodwind duet spatial image.

**P1 — Breath noise layer**
Add `obbl_breathNoise` (0-1) that mixes filtered white noise (1-pole LP at 3000 Hz) into the air-jet exciter. This adds realism -- real wind instruments always have some breath noise. Scale by velocity for dynamic breath expression.

**Estimated score impact:** P0 -> 8.2. P0+P1 -> 8.6.

---

### 20. OUTWIT (7.9 -- latest re-seance)
**Source:** `Source/Engines/Outwit/XOutwitAdapter.h` + external DSP in `XOutwit/Source/DSP/`

**Current state:** 8-arm Wolfram cellular automaton synthesizer with GA-driven SOLVE macro, SYNAPSE/CHROMATOPHORE/DEN macros, 2 LFOs, ink cloud FX, Den reverb. Score was revised to 8.7 in the re-seance but the latest score reference shows 7.9. Key gaps: pitch wheel unhandled, mono Den reverb collapses stereo, step rate ceiling 40 Hz.

**What producers want:**
- **Experimental:** This is THE experimental engine. The CA rules (0-255) produce unpredictable timbres. Need the step rate ceiling raised to 200+ Hz for audio-rate CA synthesis territory -- this would create entirely new sound categories.
- **EDM/Dance:** The 8-arm architecture could create rhythmic patterns if step rate syncs to host BPM. Add a quantized step mode where CA steps align to musical divisions.
- **Film/Ambient:** The Den reverb needs to be stereo (different arm panning + stereo reverb tails) for immersive sound design.

**Specific DSP improvements:**

**P0 — Pitch wheel handler (blocks 8.0 from 7.9)**
In `renderBlock`'s MIDI loop, add pitch bend processing:
```cpp
if (msg.isPitchWheel()) {
    float bend = (msg.getPitchWheelValue() - 8192) / 8192.0f;
    pitchBendSemitones = bend * snap.pitchBendRange; // new param owit_pitchBendRange (0-24, default 2)
}
```
Apply to the base frequency of all 8 arms in the per-sample loop.

**P1 — Raise step rate ceiling to 200 Hz**
In the step rate parameter, change the max from 40 Hz to 200 Hz. The CA at audio rates (>20 Hz) starts producing pitched tones whose timbre is determined by the Wolfram rule number -- this is genuinely novel synthesis territory that no commercial synth offers. Above 100 Hz, add anti-aliasing: apply a gentle 1-pole lowpass on the CA output to prevent harsh digital artifacts.

**P1 — Stereo Den reverb**
Replace the mono Den reverb with a stereo variant. Use different allpass delay lengths for L/R channels (e.g., L: 1087, 1213 samples; R: 1153, 1327 samples -- coprime numbers prevent frequency correlation). Each of the 8 arms should have a pan position (arm 0 = hard left, arm 7 = hard right, linearly interpolated) feeding into the stereo reverb.

**P1 — BPM-synced step rate**
Add `owit_stepSync` (choice: Free/1/1/1/2/1/4/1/8/1/16) that quantizes the CA step rate to musical divisions of the host BPM. At 120 BPM, 1/16 = 32 Hz (rhythmic), 1/1 = 2 Hz (slow evolution). This bridges experimental and rhythmic music.

**Estimated score impact:** P0 -> 8.2. P0+P1 -> 8.8.

---

## Priority Matrix Summary

### Must-fix for V1 (P0 -- blocks 8.0)

| Engine | Fix | Effort | Impact |
|--------|-----|--------|--------|
| OBRIX | Ship 150 factory presets | Large (content) | 7.2 -> 8.5+ |
| OVERTONE | Implement 8-voice polyphony | Medium | 7.6 -> 8.5 |
| OCEANDEEP | Add filter ADSR + pitch bend | Small | 7.8 -> 8.5 |
| ODDOSCAR | Add 2 user LFOs + filter envelope | Medium | 6.9 -> 8.0 |
| OBESE | Add 2 user LFOs + sub level control | Medium | 6.6 -> 8.0 |
| OBLIQUE | Add 2 user LFOs with shape/dest | Medium | 7.2 -> 8.0 |
| ODDFELIX | Add 2 user LFOs with shape/dest | Medium | 7.0 -> 8.0 |
| OVERDUB | Multi-shape LFO + add second LFO | Small | 7.4 -> 8.0 |
| OLE | Fix isHusband regression + dead params | Small | 7.0 -> 7.8 |
| OHM | Stereo voice spread + vel->brightness | Small | 7.6 -> 8.1 |
| OBBLIGATO | Vel->brightness + FX chain fix | Small | 7.8 -> 8.2 |
| OUTWIT | Pitch wheel handler | Tiny | 7.9 -> 8.2 |

### High-value enhancements (P1 -- gets to 8.5)

| Engine | Fix | Effort | Impact |
|--------|-----|--------|--------|
| OBLIQUE | BPM-sync bounce rate | Medium | 8.0 -> 8.4 |
| OCELOT | Wire coupling + make EcosystemMatrix audible | Large | 6.4 -> 7.8 |
| OBESE | Mojo modulation + arp BPM sync | Medium | 8.0 -> 8.5 |
| OCEANIC | Dynamic swarm macro + murmuration trigger | Medium | 7.1 -> 8.4 |
| OWLFISH | 4-voice polyphony + subharmonic ratio control | Medium | 7.1 -> 8.3 |
| OTTONI | Layer mode + stereo reverb | Medium | 7.2 -> 8.3 |
| ODYSSEY | Climax presets + bipolar filter env | Medium | 7.6 -> 8.4 |
| OVERWORLD | ERA LFO modulation + glitch types | Medium | 7.6 -> 8.4 |
| OVERTONE | Constant type sweep + brightness tilt | Small | 8.5 -> 9.0 |
| OCEANDEEP | Bio exciter controls + sidechain | Medium | 8.5 -> 8.9 |
| OUTWIT | Step rate 200 Hz + stereo reverb + BPM sync | Medium | 8.2 -> 8.8 |

### Polish (P2 -- gets to 9.0)

| Engine | Fix |
|--------|-----|
| OBLIQUE | Stereo bounce pan scatter |
| OBESE | Unison detune spread |
| OBSIDIAN | Stiffness partial count control |
| OVERWORLD | Stereo output width per chip engine |
| OMBRE | Grain head count + feedback saturation |

---

## Fleet-Wide Recurring Fixes

These patterns recur across multiple engines and could be addressed with shared infrastructure:

### 1. Standard LFO Module (affects 8+ engines)
Create `Source/DSP/StandardLFO.h` with: 5 shapes (Sine/Tri/Saw/Square/S&H), rate 0.001-200 Hz, BPM sync option, per-engine destination routing. Every engine that currently has a breathing-only LFO should swap to this shared module. Engines: OBLIQUE, OBESE, ODDOSCAR, ODDFELIX, OVERDUB.

### 2. Standard Filter ADSR (affects 3+ engines)
Create a reusable filter envelope module that can be dropped into any engine: attack/decay/sustain/release + bipolar depth + velocity scaling. Engines needing it: OCEANDEEP, ODDOSCAR, ODYSSEY.

### 3. Pitch Bend Handler (affects 3+ engines)
Add a shared pitch bend helper to `SynthEngine.h` base class: `float getPitchBendSemitones()` that processes pitch wheel MIDI and returns the offset. Engines: OCEANDEEP, OUTWIT, OVERWORLD.

### 4. Stereo Voice Spread Utility
Add `Source/DSP/StereoSpread.h` with: per-voice pan position calculation, equal-power pan law, configurable width parameter. Engines: OHM, OBBLIGATO, OTTONI, OVERWORLD.

---

## Genre Coverage Gaps (Fleet-Wide)

| Genre Need | Currently Served By | Gap |
|-----------|-------------------|-----|
| 808-style bass pluck | OCEANDEEP (almost) | Missing filter ADSR |
| Supersaw leads | OBESE (potential) | No unison detune spread control |
| Acid bass (303-style) | None | Need bipolar filter env + resonance boost + glide |
| Dub delay rhythms | OVERDUB (almost) | No BPM sync on tape delay |
| Chiptune leads | OVERWORLD | No chip-style portamento |
| Generative ambient | OCELOT (potential) | EcosystemMatrix not audible |
| Woodwind ensemble | OBBLIGATO | Layer mode needs stereo imaging |
| Brass section | OTTONI | No simultaneous age layers |

---

*Report generated by the Producer's Guild review panel. Each recommendation includes specific parameter names, value ranges, and code locations to enable direct implementation.*
