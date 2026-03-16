# OCEANIC Retreat Chapter
*Guru Bin — 2026-03-15*

---

## Engine Identity

- **Gallery code:** OCEANIC | **Accent:** Phosphorescent Teal `#00B4A0`
- **Parameter prefix:** `oceanic_`
- **Aquatic mythology:** Cephalopod school — bioluminescent, collective, emergent intelligence
- **feliX-Oscar polarity:** Deeply feliX — curious, alive, social, emergent
- **Synthesis type:** Swarm Particle Synthesis via Craig Reynolds Boid Flocking Rules
- **Polyphony:** Mono / Poly2 / Poly4 (max 4 voices, LRU stealing + 5ms crossfade)

---

## Pre-Retreat State

**0 factory presets** in XOmnibus. 34 presets in standalone XOceanic repo (not integrated).

Sound design guide described OCEANIC as "paraphonic string ensemble" with BBD chorus and Chromatophore pedalboard — parameters `oceanic_ensemble`, `oceanic_filterCutoff`, `oceanic_pedalMix`, `oceanic_abyssMix`, `oceanic_freezeMix`, `oceanic_chromRate`. **None of these are real parameters.** The guide described a different engine entirely. Corrected during Phase R7.

Seance P2-10 flagged "zero velocity response." Source examination revealed this is partially inaccurate: velocity-proportional scatter IS applied at noteOn. However, the scatter decays quickly and was invisible to the ghosts because no preset demonstrates its ceiling behavior.

B013 Blessed (Chromatophore Modulator) — praised by Buchla, Schulze, Kakehashi, Tomita. The mechanism had never been demonstrated in any preset.

---

## Phase R2: Silence

Guru Bin loads an init patch. One note — C3, velocity 64. Holds.

128 particles scatter outward from 130.81 Hz into a 3D perceptual cloud. No preset has shaped them. The boid rules pull them back, slowly. The swarm coheres. A tone emerges — not a pure tone, a *breathing* cluster. Then note off. The particles drift. The amplitude decays. The swarm dissolves.

What was that? Not a string. Not a pad. Not noise.

A school of fish, discovering they are music.

That is the first truth.

---

## Phase R3: Awakening — 6 Discoveries

### Discovery 1: The Swarm Has Two Lifetimes
Every instrument in the fleet has one envelope — the amplitude ADSR. OCEANIC has two. The Swarm ADSR (`oceanic_swarmAttack/Decay/Sustain/Release`) controls the *strength of the boid rules* — the collective intelligence of the school — independently of amplitude. At `swarmSustain=0`, the boid forces fully extinguish by the sustain phase; particles drift freely, held only by inertia, as the note continues at full amplitude. At `swarmAttack=3.0`, the school takes three seconds to learn to be a school — during those seconds it sounds scattered, raw, organic. Then it coheres into a tone. This is the most singular DSP capability in the fleet: no other engine separates the *intelligence* of the sound from the *loudness* of the sound.

### Discovery 2: The Chromatophore IS the Aftertouch
The B013 Blessing is not a metaphor. It is literal: `aftertouch → separation boost (×0.25)`. Pressing harder makes particles scatter faster from their neighbors, exactly as an octopus accelerates chromatophore cycling under stress or excitement. The mod wheel inverts it: `mod wheel → cohesion boost (×0.4)`, tightening the school. The two gestures are physical opposites — scattering and contracting — mapped to the two most expressive CCs a player has. No preset had ever taught this. Players have owned this engine without knowing their fingers control a cephalopod's nervous system.

### Discovery 3: Murmuration Is an Event, Not a State
`RhythmToBlend` coupling triggers murmuration: a cascade perturbation starting at particle 0 with influence 1.0, decaying by factor 0.97 through all 128 particles. The first particles get maximum disruption. The last particles get 0.97^128 ≈ 0.019× — barely touched. The result is a wave of reorganization that passes through the school. Send a drum hit from ONSET and 128 particles reorganize in sequence. This is not distortion. Not modulation. It is a starling murmuration triggered by percussion.

### Discovery 4: Sub-Flocks Are a Self-Composing Chord Section
`oceanic_subflocks` defaults to 1 — only the fundamental flock. At 4 sub-flocks, particles are distributed across four attractor frequencies: the MIDI note (1×), one octave up (2×), a perfect fifth up (1.5×), and one octave + fifth (3×). The boid rules operate *within each sub-flock* — particles of the same ratio cluster together. Play C3 and a self-organizing chord cluster forms at C3/C4/G4/G5. The chord is emergent; no designer placed those pitches. The flock discovered them through physics.

### Discovery 5: The Waveform Changes the Creature
128 sine particles under boid rules → the classic swarm tone: smooth, evolving, slightly glassy. 128 saw particles with PolyBLEP → 128 buzzing saws the boid force shapes into spectral swarms. 128 noise particles → the school stops being a pitched instrument; it becomes a self-organizing texture that coheres into white noise density clusters. Each waveform is a different creature made from the same flocking math.

### Discovery 6: Low Tether Is Near-Unpitched Territory
`oceanic_tether` at 0.05: the MIDI note barely influences particle behavior. High velocity-scatter sends particles far from the attractor at noteOn. The weak attractor cannot pull them fully back. The note never resolves to its intended pitch. A cloud that knows it is near C3 but cannot commit. This is the engine's most distinct timbral territory — near-unpitched, drifting, atmospheric — and the one least explored in any existing preset.

---

## Phase R4: Fellowship Trance

**The Obvious Fix:** The sound design guide was completely wrong. Every parameter in the guide was fabricated. The guide described a different, imagined engine. This is now corrected (same pattern as OUROBOROS — delay-line vs ODE chaos synthesis).

**The Hidden Trick:** `swarmSustain=0` + slow `swarmDecay` + `ampSustain=0.9` = a note that holds at full volume while its collective intelligence dissolves. The sound changes shape with no input from the player. The school dies while the volume lives.

**The Sacrifice:** The seance's P2-10 ("zero velocity response") was not fully wrong — the velocity scatter decays too quickly to be heard as timbre in most contexts. The fix is not DSP: design presets where `tether=0.05` + `scatter=0.5` makes the scatter audible because particles never return to equilibrium.

**The Revelation:** The boid rules ARE the filter. OCEANIC has no filter. Spectral content is entirely determined by the configuration of separation/alignment/cohesion/tether. High separation + low cohesion = particles spread spectrally = bright, complex texture. High cohesion = particles cluster near attractor = narrow, tone-like. This has never been communicated to a preset designer or documented anywhere.

---

## Phase R5: Awakening Presets

| Name | File | Mood | Discovery |
|------|------|------|-----------|
| Boid School | Oceanic_Boid_School.xometa | Foundation | subflocks=4, emergent chord cluster |
| Murmuration | Oceanic_Murmuration.xometa | Entangled | RhythmToBlend from Onset, cascade event |
| Chromatophore Touch | Oceanic_Chromatophore_Touch.xometa | Prism | B013 — aftertouch=scatter, mod wheel=cohesion |
| Swarm Wake | Oceanic_Swarm_Wake.xometa | Atmosphere | swarmAttack=2.5 — school coheres over 2.5 seconds |
| Dissolving School | Oceanic_Dissolving_School.xometa | Flux | swarmSustain=0 — boid rules extinguish on sustain |
| Noise Flock | Oceanic_Noise_Flock.xometa | Aether | waveform=Noise, 4 sub-flocks, self-organizing texture |
| Low Tether Drift | Oceanic_Low_Tether_Drift.xometa | Atmosphere | tether=0.05, near-unpitched drifting cloud |

---

## New Scripture Verses

Four verses to be inscribed in Book VII — Engine-Specific Verses.

**OCE-I: The Two Lifetimes** — Amp ADSR controls loudness. Swarm ADSR controls intelligence. They are independent. Design presets where the school dissolves while the note holds.

**OCE-II: The Boid Rules Are the Filter** — OCEANIC has no filter. The spectral content is entirely determined by separation/cohesion/tether configuration. High separation = bright. High cohesion = narrow. The physics is the EQ.

**OCE-III: The Chromatophore** — Aftertouch scatters. Mod wheel contracts. These are the two primary expressive gestures. Design every OCEANIC preset around these opposites.

**OCE-IV: Murmuration Is an Event** — RhythmToBlend coupling triggers a cascade reorganization that passes through all 128 particles in sequence. It is not modulation. It is a flock reorganizing. Design Entangled presets that demonstrate what happens when ONSET sends one hit.

---

## CPU Notes

- 128 particles × control-rate decimation (~2kHz) = moderate-to-heavy DSP cost
- Mono: safe for any configuration
- Poly2: moderate; caution with high-scatter configurations
- Poly4: respect required; recommend for sustained, slow presets only
- Murmuration is single-block cost — no sustained CPU overhead from coupling trigger

---

## Unexplored After Retreat

- Saw/Pulse waveforms with boid rules: PolyBLEP 128-oscillator clusters with collective cohesion — not yet designed
- High scatter + high tether (velocity drives initial chaos but MIDI note dominates recovery): the ceiling of velocity expressiveness
- Coprime LFO rates on separation/cohesion simultaneously: two forces in perpetual non-phase — never aligned
- Sub-flock count as macro: CHARACTER mapping to sub-flock count (1→4 as CHARACTER increases) — self-composing harmonic complexity via expression
- Poly4 with Murmuration: cascade reorganization across multiple simultaneous voices — does the murmuration propagate through all 4 voices simultaneously?

---

## Documentation Correction Notes

The following was incorrect in `Docs/xomnibus_sound_design_guides.md` before this retreat:
- Engine type: "paraphonic string ensemble" → corrected to "swarm particle synthesis"
- Voice count: "1 (paraphonic — all 128 MIDI notes)" → corrected to "Mono/Poly2/Poly4"
- Parameters: six fabricated parameters → replaced with actual `oceanic_separation`, `oceanic_alignment`, `oceanic_cohesion`, `oceanic_tether`, `oceanic_scatter`, `oceanic_subflocks`, `oceanic_damping`, `oceanic_waveform`, `oceanic_swarmAttack/Decay/Sustain/Release`, `oceanic_lfo1/2Rate/Depth/Shape`, `oceanic_voiceMode`, `oceanic_glide`, macros
- Coupling: "AudioToWavetable, EnvToMorph, LFOToPitch" → corrected to "AudioToFM, AmpToFilter, RhythmToBlend"

---

## Guru Bin's Benediction

*"OCEANIC was described as a string ensemble. It is not a string ensemble. The designers of the documentation heard something warm and evolving and reached for the nearest familiar name — strings. But strings are composed. This instrument is emergent.*

*128 particles are not programmed to make a sound. They are given rules to follow each other, and the sound that results is what happens when 128 oscillators try to be a school of fish. The timbre is the byproduct of social behavior.*

*The Chromatophore Modulator was blessed because four synthesizer pioneers heard something they could not immediately explain — a physical relationship between touch and collective behavior that had no precedent. Pressing harder makes the school scatter. Releasing makes it cohere. It responds to pressure the way a cephalopod responds to stress.*

*No preset had ever shown this. The engine's highest capability — the one the ghosts praised unanimously — was never heard by a producer, because no one wrote the preset that demonstrates it.*

*The sound of OCEANIC is not a preset. It is a trajectory. Every note traces a path from scatter to cohere to drift. The producer is not choosing a timbre — they are choosing the physics of a collective organism's lifetime.*

*Play Chromatophore Touch with one hand. With the other, press down slowly on aftertouch. Feel the school panic. Release. Feel it reconvene. That is what the ghosts heard. That is what they praised. That is what this retreat returns to the world."*
