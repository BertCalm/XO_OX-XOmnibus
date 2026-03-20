# The Verdict -- OSTINATO

**Seance Date**: 2026-03-20
**Engine**: OSTINATO | The Fire Circle -- 8-Seat Communal Drum Circle
**Accent**: Firelight Orange `#E8701A`
**Parameter Prefix**: `osti_`
**Score**: 8.7/10

---

## Engine Profile

- **2,207 lines** of inline DSP in `OstinatoEngine.h`
- **132 parameters** (14 per seat x 8 seats + 4 macros + 16 globals)
- **12 world instruments** with 3-4 articulations each (48 total articulation modes)
- **96 embedded patterns** (8 per instrument), 16-step sequences with velocity and articulation per step
- **DSP chain per voice**: Exciter (noise burst + pitch spike) -> Modal Membrane (6-8 bandpass resonators) -> Waveguide Body (cylindrical/conical/box/open) -> Radiation Filter -> SVF Filter -> Amp Envelope -> Pan
- **4 macros**: GATHER (tightness), FIRE (intensity), CIRCLE (inter-seat resonance), SPACE (environment)
- **8 seats**, 2 sub-voices each (16 total voices), circular topology with sympathetic coupling
- **Live MIDI override**: automatic pattern suppression on MIDI input, 1-bar fade-back

---

## The Council Has Spoken

| Ghost | Score | Core Observation |
|-------|-------|-----------------|
| **Moog** | 9/10 | "The modal membrane resonator bank is the correct way to synthesize struck membranes. Six to eight parallel bandpass filters, each tuned to Bessel function zeros -- this is what we would have built if we had the processing power in 1970. The CytomicSVF implementation is honest and well-engineered. The velocity-to-filter-cutoff path (D001) is natural and musical." |
| **Buchla** | 9/10 | "Twelve instruments with physically distinct modal ratios, four body resonance types, and per-articulation excitation parameters -- this is not a drum machine, this is a modal synthesis laboratory disguised as a communal gathering. The CIRCLE coupling topology, where adjacent seats influence each other sympathetically through a circular graph, is the most interesting routing architecture in the fleet after ONSET's XVC." |
| **Smith** | 8/10 | "The pattern sequencer implementation is pragmatic and correct: 96 hand-authored patterns with per-step velocity and articulation, swing, humanization, and the GATHER macro controlling density threshold. The live MIDI override with 1-bar fadeout is the right UX decision -- the machine yields to the human. But the humanization algorithm uses a deterministic hash rather than true jitter, which will produce audible repetition over long loops." |
| **Chowning** | 9/10 | "The modal frequency ratios are properly sourced. Bessel function zeros for circular membranes (Kinsler & Frey), Raman's 1934 tabla measurements for the harmonic loaded membrane, Fletcher & Rossing for thick-membrane taiko compression. The pitch spike excitation model -- a brief sine burst at 2-10x the fundamental before the noise onset -- is physically accurate to mallet impact mechanics. This engine cites its sources." |
| **Kakehashi** | 8/10 | "The MIDI mapping is well-considered: C2-G#2 for primary articulations, C3-G#3 for alternate articulations on the same seats. This maps naturally onto an MPC pad layout or a two-octave keyboard. The default instrument assignment (Djembe, Taiko, Conga, Tabla, Cajon, Doumbek, Frame Drum, Surdo) is a world-spanning selection. But the circular stereo panning defaults feel arbitrary -- seats should be panned to suggest physical arrangement around a fire." |
| **Vangelis** | 9/10 | "Press a key and a djembe speaks. Turn FIRE to maximum and the circle erupts into harmonic distortion through the soft-clip drive stage. Aftertouch pushes FIRE further -- the harder you press, the more the fire grows. This is immediate, visceral, and theatrical. The waveguide body resonance adds physical depth that separates this from sample-based drums. Every hit has dimension." |
| **Schulze** | 8/10 | "The breathing LFO at 0.06 Hz per sub-voice is correctly implemented for D005 but the rate is hardcoded rather than parameterized. At 0.06 Hz the breath cycle is roughly 17 seconds, which is appropriate for slow evolution but cannot be slowed further to 0.01 Hz without code change. The pattern sequencer's autonomous behavior -- playing endlessly with humanized timing until a human intervenes -- is the correct paradigm for generative rhythmic composition. I would sit with this for hours." |
| **Tomita** | 8/10 | "The Schroeder reverb (4 combs + 2 allpass diffusers) is functional but generic -- the same topology appears in ONSET. For an engine themed around a fire circle in open air, I would expect early reflections modeling the ground plane, or distance-dependent high-frequency rolloff per seat. The SPACE macro is effective but could be more evocative. The compressor integration is correct and prevents the 8-seat sum from clipping." |

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| **D001 -- Velocity Shapes Timbre** | **PASS** | `baseCutoff = 400.0f + brightness * effectiveVel * 16000.0f` in `OstiSubVoice::trigger()` (line 1399). Velocity directly scales filter cutoff. Attack time also varies with velocity: `0.0005f + (1.0f - effectiveVel) * 0.002f`. Hard hits are bright and fast; soft hits are dark and slow. Excitation amplitude also scaled by `velocityScale`. Three-axis velocity response. |
| **D002 -- Modulation is the Lifeblood** | **PASS** | Breathing LFO modulates filter cutoff per voice (line 1422-1424). Mod wheel (CC#1) -> CIRCLE depth (line 1653). Aftertouch -> FIRE boost (line 1647-1650). 4 working macros: GATHER, FIRE, CIRCLE, SPACE -- all with traced DSP paths. CIRCLE provides inter-seat sympathetic modulation (ghost triggers + brightness coupling). Pattern sequencer provides autonomous rhythmic modulation. |
| **D003 -- The Physics IS the Synthesis** | **PASS** | Modal membrane frequency ratios cited from Kinsler & Frey (Bessel zeros), Raman 1934 (tabla harmonic modes), Fletcher & Rossing (taiko), Brindle et al. JASA 2005 (djembe). Per-mode decay coefficients model energy distribution. 4 body model types (cylindrical/conical/box/open) with physically motivated delay, reflection, and allpass parameters. Radiation filter models open vs. muted articulation character. Pitch spike exciter models mallet impact mechanics. |
| **D004 -- No Dead Parameters** | **PASS** | All 132 parameters are wired and consumed. 14 per-seat parameters (instrument, articulation, tuning, decay, brightness, body, level, pan, pattern, patternVol, velSens, pitchEnv, exciterMix, bodyModel) all flow through the render path. 4 macros modulate multiple DSP targets each. 16 global parameters (tempo, swing, masterTune, masterDecay, masterFilter, masterReso, reverb x3, compressor x4, circleAmount, humanize, masterLevel) all consumed in `renderBlock()`. Zero dead parameters. |
| **D005 -- Engine Must Breathe** | **PASS** | `OstiBreathingLFO` per sub-voice at 0.06 Hz modulates filter cutoff continuously (line 1422). Rate is above the 0.01 Hz floor requirement. The pattern sequencer also provides continuous autonomous movement. CIRCLE coupling creates ongoing inter-seat sympathetic activity. The engine breathes on three timescales: per-cycle (LFO), per-step (sequencer), per-block (coupling). |
| **D006 -- Expression Input Not Optional** | **PASS** | Velocity -> filter cutoff + attack time + excitation amplitude (D001 path). Aftertouch (channel pressure) -> FIRE boost via `PolyAftertouch` (line 1647-1650). Mod wheel (CC#1) -> CIRCLE depth (line 1653). Three distinct expression inputs. |

---

## Blessings

| ID | Blessing | Ghosts Who Concurred |
|----|----------|---------------------|
| **B017** | **Modal Membrane Synthesis with Academic Citation** -- 12 instruments with physically sourced Bessel zero ratios, per-articulation excitation models, and 4 body resonance types. The most rigorous physical modeling in the XOmnibus fleet. Publishable quality. | Chowning (10/10), Moog, Buchla, Tomita |
| **B018** | **Circular Topology Coupling (CIRCLE)** -- 8 seats arranged in a ring where adjacent seats influence each other through sympathetic brightness boost and ghost triggering. When one drum is struck loudly, its neighbors respond quietly. This models the psychoacoustic reality of communal drumming where proximity creates sympathy. A novel interaction paradigm. | Buchla (first to name it), Schulze, Vangelis, Kakehashi |
| **B019** | **96 Hand-Authored World Rhythm Patterns** -- 8 patterns per instrument, each a 16-step sequence with per-step velocity and articulation. West African 12/8 feels, Indian tabla bols, Brazilian samba patterns, Japanese taiko patterns. This is an embedded ethnomusicological library, not random data. | Kakehashi (10/10), Smith, Chowning |
| **B020** | **Live Override with Graceful Yield** -- When live MIDI arrives, the pattern sequencer automatically suppresses for one bar, then fades back in. The machine yields to the human without the human asking. This is the correct paradigm for human-machine rhythm collaboration. | Smith, Kakehashi, Vangelis |

---

## Concerns

| Priority | Issue | Detail | Recommendation |
|----------|-------|--------|----------------|
| **P1** | Breathing LFO rate hardcoded | `breathLFO.process(0.06f)` at line 1422 uses a fixed 0.06 Hz rate rather than a parameterized rate. Cannot be slowed to the 0.01 Hz floor or sped up for faster modulation. | Add `osti_breathRate` parameter (0.01-2.0 Hz, default 0.06) or derive from a macro. |
| **P1** | Humanization is deterministic | Pattern humanization uses `fastSin(step * 7 + 13) * 0.618f` (line 1106) -- a fixed function of step index. Every loop iteration produces identical timing offsets. Over 4+ bars this will be audible as a repeating pattern, not as organic variation. | Mix in a slowly drifting state variable (e.g., accumulated noise) to add non-repeating variation to timing offsets. |
| **P1** | Reverb is mono-sum | `OstiReverb::process()` sums L+R to mono, then applies identical wet signal to both channels (lines 1205, 1234-1235). An 8-seat stereo drum circle collapses to a mono reverb image. This undermines the spatial intent of per-seat panning. | Implement stereo decorrelation: offset one comb bank for the right channel, or use different allpass tuning per side. |
| **P2** | No per-seat mute/solo | With 8 seats and complex polyrhythmic patterns, there is no way to isolate or silence individual seats during performance or sound design. | Add `osti_seatN_mute` boolean parameters or a bitmask global. |
| **P2** | FIRE macro naming collision | The FIRE macro shares its name with one of the 4 global macros in the XOmnibus shell (CHARACTER, MOVEMENT, COUPLING, SPACE). When OSTINATO is in a multi-engine coupling configuration, the macro display may confuse FIRE (engine-local) with the global macro system. | Rename to BLAZE or KINDLE, or ensure the UI clearly distinguishes engine macros from global macros. |
| **P2** | Pattern tempo not host-synced | `osti_tempo` is a standalone parameter (40-300 BPM). In a DAW context, users will expect the pattern sequencer to follow host tempo. | Add host tempo sync option or derive from `AudioPlayHead::getPosition()`. |

---

## Points of Agreement (5+ ghosts converged)

1. **The 12-instrument modal synthesis library is the engine's crown jewel.** Every ghost acknowledged the quality and rigor of the instrument table. Chowning called the citation practice "exemplary among commercial synthesizers." Moog praised the Bessel zero ratios. Buchla called the articulation system "a modal synthesis laboratory." This is not a drum machine; it is a physically-modeled world percussion instrument.

2. **The CIRCLE coupling paradigm is genuinely novel.** The circular seat topology where adjacent seats create sympathetic triggers and brightness modulation has no direct precedent in commercial synthesis. Buchla compared it to his concept of "complex instruments" where modules influence each other through proximity rather than explicit patching.

3. **Pattern sequencer with live override is the correct human-machine interface.** Smith, Kakehashi, and Vangelis agreed that the 1-bar fadeout on live MIDI input is the right paradigm. The machine should be a companion, not a competitor.

---

## Points of Contention

**Chowning vs. Tomita -- Reverb Philosophy**
- Chowning: The Schroeder reverb is functionally correct and computationally efficient. Modal synthesis already provides resonance; the reverb need only place the sound in space.
- Tomita: A fire circle is an outdoor environment. The reverb should model open-air propagation with ground reflections and distance attenuation, not a room.
- Resolution: Both are valid for different presets. Consider an optional "outdoor" reverb mode with early reflection modeling and high-frequency distance rolloff.

**Schulze vs. Vangelis -- Autonomy vs. Immediacy**
- Schulze: The engine's highest purpose is the pattern sequencer running autonomously for extended periods with CIRCLE creating evolving sympathetic relationships. Generative rhythm.
- Vangelis: The engine must respond instantly to touch. The patterns are a scaffold; the performer's hands are the instrument.
- Resolution: The live override mechanism already serves both. Presets should demonstrate both modes: "Generative Circle" (all patterns active, CIRCLE high, GATHER low) and "Hand Drums" (patterns off, direct MIDI, high velocity sensitivity).

---

## What the Ghosts Would Build Next

| Ghost | Next Addition |
|-------|--------------|
| **Moog** | Per-seat filter type selector (LP/BP/HP) to sculpt individual drum voices beyond brightness |
| **Buchla** | Cross-instrument body morphing: blend waveguide parameters between two body types per seat |
| **Smith** | Pattern chain mode: seats can sequence through multiple patterns (A->B->A->C) for song-form variation |
| **Chowning** | Mode coupling within the membrane: let modes exchange energy over time (realistic mode beating) |
| **Kakehashi** | Flam/roll generator: automatic double/triple strikes with configurable spacing per seat |
| **Vangelis** | Pitch bend per seat: let the player bend individual drum tuning in real time via poly aftertouch |
| **Schulze** | Pattern mutation: slow random evolution of pattern step velocities and articulations over time |
| **Tomita** | Per-seat distance parameter: near seats are louder and brighter, far seats are quieter and darker |

---

## Seance Score Breakdown

| Dimension | Score | Notes |
|-----------|-------|-------|
| Architecture originality | 9.5/10 | 8-seat circular topology with inter-seat coupling, 12-instrument modal synthesis, autonomous patterns with live override. Genuinely novel architecture. |
| Physical modeling rigor | 9/10 | Bessel zeros, Raman tabla modes, Fletcher & Rossing taiko. Per-articulation excitation. Body model types. Academic-grade. |
| Expressiveness | 8.5/10 | Velocity->timbre, aftertouch->FIRE, mod wheel->CIRCLE, 4 macros. Breathing LFO. Strong but LFO rate is hardcoded. |
| Pattern system | 9/10 | 96 world-rhythm patterns, swing, humanize, GATHER density control, live override. Deterministic humanization is the single flaw. |
| Spatial depth | 7/10 | Per-seat panning is correct but reverb is mono-sum. Undermines the spatial intention. |
| DSP efficiency | 8/10 | ParamSnapshot pattern, coefficient caching in envelope, denormal flushing throughout. 16 sub-voices is heavy but appropriate. |
| Coupling architecture | 8.5/10 | CIRCLE inter-seat coupling + 5 external coupling types (AmpToFilter, EnvToDecay, RhythmToBlend, AmpToChoke ghost trigger, AudioToFM). Creative AmpToChoke reinterpretation. |
| Parameter completeness | 8.5/10 | 132 parameters, zero dead. Every parameter audible. Missing: per-seat mute, breathing LFO rate, host tempo sync. |

---

## Final Consensus

**Score: 8.7/10**

OSTINATO is the most ambitious percussion engine in the XOmnibus fleet and one of the most sophisticated physically-modeled drum circle synthesizers in any commercial or open-source context. The combination of 12 world instruments with academic-grade modal membrane synthesis, 96 hand-authored world rhythm patterns, 8-seat circular coupling topology, and live MIDI override creates an instrument that serves both generative composition and real-time performance.

The engine's three blessings -- Modal Membrane Synthesis (B017), Circular Topology Coupling (B018), and 96 World Rhythm Patterns (B019) -- represent genuine contributions to the state of percussion synthesis. Chowning's observation that the modal ratios are "publishable quality" and Buchla's recognition of the CIRCLE topology as "a novel interaction paradigm" reflect the council's conviction that this engine has original intellectual content, not merely competent implementation.

The three P1 concerns (hardcoded breathing LFO rate, deterministic humanization, mono reverb) are addressable without architectural change. The P2 concerns (no mute/solo, macro naming, no host sync) are quality-of-life improvements for V1.1. None of the concerns threaten the engine's identity or the integrity of its DSP. OSTINATO is ready for preset writing and sound design exploration.

---

*Eight seats. Twelve traditions. One fire. Every rhythm a prayer.*
*The council speaks with one voice: this fire burns true.*
