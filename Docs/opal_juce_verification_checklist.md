# OPAL Engine — JUCE Verification Checklist

Items that could not be verified without a JUCE build environment.
Run through these on your home machine before considering OPAL production-ready.

---

## 1. Compilation

- [ ] Clean build with no warnings (`-Wall -Wextra`)
- [ ] Verify `OpalEngine.h` compiles as part of the full XOmnibus target (AU + Standalone)
- [ ] Verify iOS build (AUv3 + Standalone)
- [ ] Confirm `REGISTER_ENGINE(OpalEngine)` links correctly — "Opal" appears in `EngineRegistry::getRegisteredIds()`

## 2. Parameter Integration

- [ ] All 86 `opal_` parameters appear in the APVTS after engine load
- [ ] `ParameterGrid` auto-discovers and displays all 86 knobs when Opal is selected in a slot
- [ ] Parameter ranges match spec (e.g., `opal_grainSize` 10–800ms, `opal_filterCutoff` 20–20kHz)
- [ ] Choice parameters show correct labels (Source: Sine/Saw/Pulse/Noise/Two-Osc/Coupling, etc.)
- [ ] Saving and reloading a DAW session preserves all Opal parameter values

## 3. Audio / DSP

- [ ] Plays sound on MIDI noteOn with default parameters
- [ ] Grain density slider audibly changes grain rate (1–120 grains/sec)
- [ ] All 6 source modes produce distinct timbres (Sine, Saw, Pulse, Noise, Two-Osc, Coupling)
- [ ] Freeze button stops buffer writing — grains loop in the frozen region
- [ ] Freeze region size (`opal_freezeSize`) audibly constrains scatter when frozen
- [ ] Filter cutoff + resonance respond correctly; all 4 modes (LP/BP/HP/Notch)
- [ ] Filter key tracking shifts cutoff with note pitch
- [ ] Shimmer adds audible octave-up harmonic content
- [ ] Frost adds cold hard-limiting character
- [ ] Amp and filter envelopes shape notes correctly (ADSR)
- [ ] LFO1/LFO2 modulate targets via mod matrix
- [ ] LFO retrigger resets phase on noteOn when enabled
- [ ] Mod matrix: at least test LFO1→Filter Cutoff and Amp Env→Grain Size
- [ ] Polyphonic: 12 simultaneous notes without clicks or artifacts
- [ ] Mono mode with glide: pitch slides smoothly between notes
- [ ] Legato mode: amp envelope doesn't retrigger, filter does retrigger from current level
- [ ] Sustain pedal (CC64) holds notes, releases on pedal up
- [ ] No denormals (check CPU doesn't spike after note release + long tail)
- [ ] No clicks on voice stealing (grain pool LRU should handle gracefully)

## 4. Effects Chain

- [ ] Smear produces audible time-stretch diffusion
- [ ] Scatter Reverb: size/decay/damping respond, no runaway feedback at max settings
- [ ] Stereo Delay: time/feedback/spread all audible; feedback stays stable at 0.95
- [ ] Finish glue compressor: audible leveling at high settings
- [ ] Finish width: mono collapse at 0, wide stereo at 2.0
- [ ] Master level and pan work correctly
- [ ] FX bypass: all FX mix knobs at 0 = clean dry signal

## 5. Macros

- [ ] M1 SCATTER: sweeps grain size up + density down (inverse exponential)
- [ ] M2 DRIFT: increases position/pitch/pan scatter simultaneously
- [ ] M3 COUPLING: increases coupling level, adds freeze above 20%
- [ ] M4 SPACE: opens reverb, smear, and delay together

## 6. Coupling (Cross-Engine)

- [ ] AudioToWavetable: another engine's audio feeds Opal's grain buffer (source mode = Coupling)
- [ ] AmpToFilter: external amp modulates Opal filter cutoff
- [ ] EnvToMorph: external envelope shifts grain position
- [ ] LFOToPitch: external LFO modulates grain pitch scatter
- [ ] RhythmToBlend: external rhythm modulates grain density
- [ ] EnvToDecay: external envelope modulates freeze amount
- [ ] Opal coupling output: `getSampleForCoupling()` returns post-filter audio to other engines

## 7. Presets

- [ ] All 100 `.xometa` presets load without errors
- [ ] 6 hand-crafted presets sound musically compelling dry (no effects):
  - Glass Cloud, Frozen Bloom, Particle Storm, Shimmer Veil, Deep Drift, Scatter Glass
- [ ] Generated presets load and produce distinct sounds across mood categories
- [ ] Preset browser filters by mood correctly
- [ ] Macro knobs produce audible change in every preset (brand rule)
- [ ] Switching between presets: no clicks, parameter transitions smooth

## 8. Memory / Performance

- [ ] `OpalGrainBuffer` (~3MB static array) doesn't cause stack overflow — confirm heap allocation via `std::unique_ptr<OpalEngine>`
- [ ] `OpalStereoDelay` (~3MB) same check
- [ ] No memory allocation in `renderBlock` (profile with Instruments or sanitizer)
- [ ] CPU usage reasonable: < 5% single core at 12 voices, 32 grains, 44.1kHz
- [ ] No leaks (run with AddressSanitizer)

---

*Generated 2026-03-11 during OPAL Phase 7 QA. Branch: `claude/design-synthesis-engines-mbgCM`*
