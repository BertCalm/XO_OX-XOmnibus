# Producer's Guild Report — OBRIX
**Date**: 2026-03-19
**Panels convened**: 25 genre specialists + full product team (Maya, Derek, Ingrid, Ops Trio, Foreseer)

---

## Executive Summary

OBRIX is architecture waiting for its catalogue. The brick-pool modular philosophy is genuinely original within XOmnibus — a meta-engine that becomes a different instrument depending on which bricks are active and which engines are coupled. But 25 genre specialists, unanimously and independently, delivered the same verdict: **zero factory presets is a session-blocking omission**. The engine architecture is sound. The empty library communicates unfinished work regardless of what the DSP does. The roadmap below transforms OBRIX from a demo instrument into a production tool in three phases.

---

## Genre Panel Highlights

| Specialist | Genre | Verdict | Top Request |
|---|---|---|---|
| Beatrice "Trap" Morales | Hip-Hop / Trap | Wavefolder + Velocity has 808-adjacent texture | Glide rate param (0–500ms) |
| Marcus "Deep" Williams | Deep House / Techno | LFO→FilterCutoff is the core sweep I need | LFO tempo sync — non-negotiable |
| Kai Suzuki | Ambient / Drone | Brick modularity aligned with process music philosophy | Looping envelope mode on ADSR |
| Sofia Chen | Jazz / Neo-Soul | Aftertouch routing is unusually expressive for this tier | Velocity curve type (Linear/Exp/Log/Fixed) |
| Jerome "Dub" Baptiste | Dub / Reggae | Delay + MOVEMENT = tape echo without external processor | Dual FX slots in series |
| Amara Okonkwo | Afrobeat / World | Ring Mod generates kora/balafon bell partials | Scale/mode lock per voice mode |
| Lars Eriksson | Cinematic / Game | Wavetable + LFO→WavetablePos is a tension cue engine | Sample-and-hold LFO shape |
| Priya Sharma | Pop / Radio | CHARACTER macro is instantly radio-useful | Unison mode (2–8 voice, ±50¢ detune) |
| Rico Valdez | Latin / Club | RingMod → metallic cowbell/clave textures | LFO tempo sync + feedback toggle |
| Emma Blackwood | Rock / Indie | Wavefolder into SVF is Moog Matriarch territory | Exposed portamento/glide time param |
| Dr. James Thornton | Classical / Orchestral | Aftertouch→Pitch is authentic string vibrato | Envelope curve shapes (exp/linear toggle) |
| Zero_1 | Experimental / Noise | RingMod→Wavefolder is already dangerous | Modulator-to-modulator routing |
| DJ Phantom | Drum & Bass | CHARACTER macro for Reese movement | Brick duplicate slot (dual Source at different pitches) |
| Yuki Tanaka | J-Pop / K-Pop | Chorus + Wavetable morph = 60% of what J-Pop needs | Arpeggiator with tempo-sync |
| Nils Frahm Jr. | Neoclassical | Sine + ADSR for prepared-piano tones | Per-voice pitch microtuning offset |
| Zara "Zed" Okafor | Grime / UK Garage | Saw + LP SVF = instant Bassline territory | Portamento time param (independent of Legato) |
| Hassan El-Amin | Middle Eastern | MOVEMENT drift is closer to oud/nay than equal temp | Pitch bend range ±0.5st for quarter-tone ornaments |
| Thandi Molefe | Amapiano / Gqom | COUPLING + ONSET = complete Amapiano rhythm section | Dedicated pitch envelope brick with decay curve |
| Viktor Kozlov | Synthwave | Pulse+Chorus+CHARACTER = authentic 80s spectrum | Tempo-synced Delay with note-value selector |
| MC Dubplate | Dancehall / Riddim | RingMod on Saw = electric piano stab | 8-step pitch sequencer brick |
| Aiko "Pulse" Nakamura | Trance / Progressive | LFO→FilterCutoff is trance's heartbeat | LFO tempo sync (non-negotiable) |
| Gabriel Santos | Lo-Fi / Chillhop | MOVEMENT macro = vinyl wow-and-flutter in a knob | Bitcrush/sample-rate-reduce effect brick |
| Ingrid "Ice" Johansson | Industrial / EBM | Wavefolder adds odd-order harmonics like analog | Hard sync + feedback loop brick |
| Chen Wei | C-Pop / Mandopop | Wavetable + Aftertouch = expressive legato phrasing | Vibrato onset delay param (0–500ms) |
| Kwame Asante | Gospel / Worship | Chorus + SPACE macro = immediate sanctuary fill | Sustain pedal CC64 + swell modulator CC2/CC11 |

---

## Preset Gap Analysis

| Category | Missing Archetypes | Guild Endorsements |
|---|---|---|
| Ambient / Texture | Slow-bloom pads, spectral drones, underwater drift, wavetable fog | 14 panels |
| Cinematic | Tension risers, hybrid organic/synth pads, impact tails, void pads | 12 panels |
| Bass | Sub growl, Reese, modulated acid, lo-fi 808 emulation, riddim sub | 11 panels |
| Lead | Mono portamento lead, bright pluck, overdriven mid, icy filtered lead | 10 panels |
| Pad | Lush poly pad, detuned warm pad, choir shimmer, worship swell | 10 panels |
| Experimental | Granular-adjacent brick chaos, metallic interference, noise drone | 7 panels |
| Keys / Mallets | Rhodes-approximate, glass harmonica, bell tone, felt piano | 6 panels |
| Arp / Rhythmic | Modulated repeating motif, rhythmic gate pad, trance pluck | 5 panels |

**Mandate**: Minimum 8 presets per category = **64 presets as launch floor**. Target 120 by V1.3b.

---

## Prioritized Feature Backlog

| Priority | Feature | Guild Votes | Effort | Impact |
|---|---|---|---|---|
| **P0** | Factory preset library (120 minimum) | 25/25 | High | Critical — product unusable without it |
| **P1** | Default source: Saw not Sine | 23/25 | Trivial | High — first impression; one-line change |
| **P2** | Filter key tracking (cutoff scales with note pitch) | 21/25 | Low | High — cross-register playability; classical/jazz/cinematic unlocked |
| **P3** | LFO + Delay tempo sync (note-value divisions) | 19/25 | Medium | High — house, techno, pop, gospel, trance, synthwave all blocked without it |
| **P4** | COUPLING self-routing fallback (internal when no partner) | 17/25 | Medium | High — standalone viability for new users |
| **P5** | LFO ceiling raised to 80Hz with aliasing guard | 14/25 | Low-Medium | Medium-High — DnB/industrial/experimental; FM territory |
| **P6** | XPN export for MPC (keygroup program, brick-state metadata) | 15/25 | High | High — hip-hop, trap, house, gospel all need MPC workflow |
| **P7** | Ocean-sourced wavetable shapes (6-8 spectral tables) | 13/25 | Medium | High — reef identity; ambient/cinematic/C-Pop |
| **P8** | Tide Mode (constrained randomizer with ocean wavetable weighting) | 11/25 | Low-Medium | High — discovery + social shareability + reef mythology |
| **P9** | Second FX slot (serial routing, deferred pending CPU audit) | 10/25 | Medium-High | Medium — cinematic/dub/ambient; partially addressable via COUPLING |
| **P10** | LFO as audio-rate FM source (Brick Drop, not V1) | 8/25 | High | Medium — experimental segment |

---

## XPN Export Enhancements

1. **Brick-State Snapshot in XPN Metadata** — Embed `OBRIX_BRICK_STATE` JSON in XPN comment field. MPC ignores it; DAW-side tools can reconstruct patch for recall.
2. **Per-Note Brick Variation Export** — For Poly4/Poly8, export 3 velocity-layered keygroups with slightly randomized modulator values baked in. Organic variation on MPC without live OBRIX.
3. **Brick Drop Pack Bundling** — Every new brick drop auto-generates an XPN expansion pack (3-5 presets per new brick). XPN export is part of the Brick Drop release checklist.

---

## Playable Surface Recommendations

1. **BRICK PAD (MPCe Quad-Corner)** — CHARACTER / MOVEMENT / COUPLING / SPACE → four pad corners. Pressure on each corner morphs its macro in real-time. OBRIX becomes a live performance instrument, not a preset recall device.
2. **COUPLING LOCK Button** — Dedicated pad/button freezes COUPLING macro state mid-performance. Prevents drift from a beautiful coupling moment. Toggle on/off.
3. **BRICK MUTE Surface** — Each active brick exposed as a mutable pad slot. Live mute individual Processors/Modulators without changing the patch. Mixer-style mute grid for the brick chain.

---

## Technical Roadmap (Phased)

### Phase 1 — Foundation (V1.3b, 3-4 weeks)
*Goal: Make OBRIX standalone-viable and first-impression-correct*

- Default source: Saw (one-line change — ship this week)
- Filter key tracking: pitch-to-Hz mod bus input on SVF cutoff; `obrix_filterKeyTrack` param (0–200%)
- LFO ceiling: raise to 80Hz with log-curve rescaling + aliasing guard at >20Hz
- COUPLING self-routing fallback: CHARACTER macro output as internal coupling source when no partner detected. Add `obrix_couplingSource` enum (PARTNER / SELF / OFF)
- Preset library: 64 minimum across 8 categories
- Tempo sync infrastructure: AudioPlayHead BPM query; LFO division enum; Delay dual-mode (ms / BPM-division)

### Phase 2 — Expansion (Brick Drop cadence, weeks 5-10)
*Goal: Open advanced territory without breaking Phase 1 users*

- LFO as audio-rate FM operator (per-sample processing when rate >20Hz)
- Wavetable ocean shapes: 8 spectral shapes from underwater recordings (Brick Drop #1)
- Tide Mode: constrained randomizer with ocean wavetable weighting + sea-creature preset naming (Brick Drop #2)
- XPN export pipeline: keygroup program, velocity layers, brick-state metadata
- Modulation matrix display: visual routing diagram showing which modulator goes where at what depth

### Phase 3 — Maturity (Brick Drop #3-5, weeks 11-20)
*Goal: OBRIX as platform*

- New Processor brick: Comb filter / Karplus-Strong pluck
- New Modulator brick: Step sequencer (8 steps, tempo-synced)
- New Source brick: Noise-color (White/Pink/Brown/Crackle)
- Second FX slot: **Gate behind CPU profiling** — only ships after measurement on 8-voice poly at 48kHz on minimum-spec hardware
- MPCe quad-corner macro mapping (platform-layer work)

---

## Ops Notes (The Trio)

1. **LFO 80Hz is not a range slider change** — above 20Hz, per-block processing aliases. Requires processing-rate decision per modulator per voice. Aliasing guard is non-negotiable before shipping.
2. **COUPLING self-routing needs a source enum** — auto-detecting partner presence and silently changing behavior breaks existing presets. Add `obrix_couplingSource` (PARTNER/SELF/OFF). Let the producer choose.
3. **Default source change serialization** — verify preset format explicitly encodes waveform type on save. Loading an old Sine-default preset must not silently upgrade to Saw.
4. **Tide Mode randomizer needs a constraint grammar** — pure random produces silence (modulators unrouted), DC offset (wavefolder without following filter), amplitude spikes. Signal flow validity rules required: Source → Processor → Effect always present; modulator targets always resolved.
5. **XPN Poly8 export has MPC memory ceiling risk** — 8 velocity layers × 4+ octaves exceeds per-program sample budget on MPC hardware. Define spec: 3 velocity layers × 5 octaves max, tail truncation at -60dB. Test on hardware, not just software.

---

## The Foreseer's Vision

### Unseen Issue
**Nobody flagged the modulation matrix legibility problem — it will be the single most common support request.**

As COUPLING self-routing, audio-rate LFO, key tracking, and tempo sync all ship, the number of active modulation connections in a complex preset will exceed what any producer can mentally track. Without a visual modulation routing display (which modulator goes where, at what depth, with what polarity), OBRIX becomes a black box. Producers will distrust presets they didn't make, will never edit shipped presets, and will stop buying Brick Drops because they can't understand what the bricks are doing. **A modulation matrix display is Phase 1 scope, not a UI nicety.**

### Unseen Opportunity
**Wavetable ocean shapes + Tide Mode = OBRIX's identity landmark.**

Individually modest. Combined: Tide Mode draws exclusively from ocean-sourced wavetables when randomizing Source bricks, generates architecturally valid patches, names results after sea creatures from the XO_OX mythology taxonomy. Every press is a new creature from the reef. This is not just a randomizer — it is a generative mythology machine. The most-shared OBRIX feature on social media. Promotes the reef mythology without requiring producers to read documentation. Makes the wavetable content discoverable. Makes the randomizer safe through constraint grammar.

### The Dominoes
**Decision: Ship filter key tracking + LFO 80Hz + COUPLING self-routing as Phase 1**

```
Phase 1 ships →
  1. OBRIX becomes standalone-viable — new segment: producers who don't yet own XOmnibus
  2. COUPLING self-routing + CHARACTER feedback creates unintentional self-oscillating patches
     — community discovers this, posts about it, OBRIX gets credited for something it didn't design
  3. Audio-rate LFO triggers "why can't I route it as FM operator?" requests within 90 days
     — validates Phase 2 FM brick planning before it's built
  4. Filter key tracking opens cinematic/classical/jazz producers who previously dismissed OBRIX
     — expands addressable genre pool, revenue from unreachable segment
  5. COUPLING self-routing architecture becomes SDK reference implementation
     — third-party developers learn the self-coupling pattern from OBRIX source
  6. Standalone viability changes Brick Drop marketing from "XOmnibus users" to "OBRIX owners"
     — wider audience per drop, same development cost
  7. Phase 1 completeness re-scores the Seance from 7.2 → 8.8+
     — the score is referenced by purchase decisions; better score = better conversion
```

### The Prediction
In 18 months, producers will want **OBRIX to learn their creative preferences across sessions** — not AI presets, but preference-weighted Tide Mode that notices "this producer always engages Wavefolder, rarely uses Chorus, gravitates toward Aftertouch modulation" and weights the randomizer accordingly. The infrastructure is modest: 12-integer brick preference array, session log. The experience feels magical. No competitor is building producer-instrument relationship over time. OBRIX is the only engine with a small enough brick pool to make this tractable.

### Vision Statement
OBRIX is not a synthesizer that happens to have bricks — it is a reef that happens to make sound, and the bricks are living organisms that grow, combine, and respond to the producer's hands. Where every other modular tool sells infinite possibility and delivers infinite confusion, OBRIX sells a curated ocean: deep enough to spend a career in, constrained enough to learn in an afternoon, alive enough to surprise you every time you reach in. The Brick Drop cadence is not a release strategy — it is the tide coming in. Something new washes up every two weeks. The producer is not configuring a machine; they are building a reef, one organism at a time, and the reef remembers what grew there.

---

## Next Session Starting Points

1. **Ship default Saw source** — one-line change in `ObrixEngine.h` param defaults. Ship this week.
2. **Commit 64 starter presets** — across 8 categories, 8 each. Minimum before any demo. Run through Guru Bin for quality pass.
3. **Prototype Tide Mode** — constrained randomizer with signal-flow validity grammar. The Foreseer's highest-leverage single feature for social traction and mythology reinforcement.

---

*Guild convened 2026-03-19. 25 genre specialists + Maya (PM) + Derek (Market) + Ingrid (Architect) + Remy/Sam/Fio (Ops) + The Foreseer.*
