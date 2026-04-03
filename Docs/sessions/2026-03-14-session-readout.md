# XO_OX Session Readout — 2026-03-14
## Historical Society + Producer's Guild Baseline + 3 New Deliverables

---

# PART 1: THE HISTORICAL SOCIETY (Full Run — All 5 Archivists)

## Census
- **15 repos** scanned under `~/Documents/GitHub/XO*`
- **MEMORY.md**: 124 lines (under 200-line limit, trimmed last session)
- **14 satellite memory files** checked
- **Knowledge Tree**: 5 primitives (PRIM-005 created this run), 3 contexts
- **Docs/concepts/**: 38 concept briefs — previously unindexed, now indexed

## Restorations Applied (11 stale facts corrected)

| # | File | Old | New | Source |
|---|------|-----|-----|--------|
| 1 | `memory/xodyssey-engine.md` | 34 source files | 41 source files | Archivist (verified count) |
| 2 | `memory/xodyssey-engine.md` | "Git repo is ~ (home dir), NOT dedicated repo" | Removed | Archivist (false — XOdyssey has own repo) |
| 3 | `memory/MEMORY.md` | XOdyssey "home dir" note | Removed | Archivist |
| 4 | `memory/MEMORY.md` | XOppossum gallery: BITE / `#4A7C59` | OVERBITE / Fang White `#F0EDE8` | Archivist (CTX-002 + Seance verdict) |
| 5 | `memory/MEMORY.md` | Preset total: 1,153 | 1,625 (verified .xometa scan) | Archivist |
| 6 | `memory/MEMORY.md` | XOpal: 100 factory presets | 150 (Batch 1: 100, Batch 2: 50) | Chronologist |
| 7 | `memory/MEMORY.md` | DUB/DRIFT in engine colors | OVERDUB/ODYSSEY (canonical gallery codes) | Editor |
| 8 | `XOdyssey/CLAUDE.md` | Gallery code: DRIFT | ODYSSEY (legacy: DRIFT) | Editor |
| 9 | `XOblongBob/CLAUDE.md` | Amber accent: `#f5c97a` | `#E9A84A` (all instances) | Editor (XOceanus master spec) |
| 10 | `XOverdub/CLAUDE.md` | Gallery Code: DUB / `#00FF88` | OVERDUB / Olive `#6B7B3A` | Editor + Archivist |
| 11 | `XOddCouple/CLAUDE.md` | "XO_XO Designs" | "XO_OX Designs" | Chronologist (brand typo) |

## Documents Created

| File | Contents | Created By |
|------|----------|-----------|
| `XO_OX-XOceanus/Docs/concepts/README.md` | Index of all 38 concept briefs, organized by category | Cataloguer |
| `knowledge/primitives/PRIM-005-status-field-drift.md` | New primitive: status fields drift within days of completion | Archivist |
| `knowledge/index.md` | Updated: primitive count 4 → 5 | Archivist |

## Key Findings by Archivist Domain

### Chronologist (Time & Truth)
- XOddCouple had wrong brand name ("XO_XO" instead of "XO_OX") — only repo in ecosystem
- XOblongBob, XOverdub, XOdyssey all claim preset counts in CLAUDE.md but have 0 .xometa files in standalone repos — presets live in XOceanus only
- XOhm phase status unclear ("Phase 2" but 0 presets, no synth_playbook)
- XOpal presets 100 → 150 (Batch 2 existed but wasn't counted)

### Cataloguer (Structure & Navigation)
- **38 concept briefs** in Docs/concepts/ had NO index — created README.md
- **57 files** in XOceanus/Docs/ parent folder — still needs a README (highest priority remaining)
- XOceanic, XOcelot, XOwlfish: repos exist with CLAUDE.md and source code but **zero memory satellite coverage**
- 4 newer engine repos (XOceanic, XOcelot, XOwlfish, XOhm) lack synth_playbook/ scaffold
- 12+ documents over 150 lines have no table of contents (largest: 2,127 lines)

### Editor (Clarity & Quality)
- **3 color contradictions fixed**: XOverdub (#00FF88→#6B7B3A), XOblongBob (#f5c97a→#E9A84A), both now match XOceanus master spec
- **Gallery code confusion fixed**: DRIFT→ODYSSEY, DUB→OVERDUB
- XObese parameter prefix story unclear (internal `p_` vs XOceanus `fat_`) — needs adapter clarification
- XOverworld build command split across 2 lines (inconsistent with single-line standard)
- Audio thread safety rules duplicated across 4+ CLAUDE.md files (fragmentation risk)

### Accelerator (Velocity & Utility)
- **XOblongBob knowledge stubs**: `preset_design_patterns.md` (272 bytes) and `synth_concept_patterns.md` (307 bytes) are placeholder files — empty since build completion
- **XOppossum knowledge stubs**: `architecture_patterns.md` is template header only — 5-macro system (BELLY/BITE/SCURRY/TRASH/PLAY_DEAD) undocumented
- **No adapter recipe** existed — commissioned and written this session
- **No coupling interaction cookbook** — which engines couple best remains undocumented
- **No mod matrix reusable template** — each engine reinvents the 8-slot pattern

### Archivist (Memory & Persistence)
- MEMORY.md healthy at 124 lines (under 200 limit)
- PRIM-005 created: "Status fields in MEMORY.md drift within days of completion"
- Recommended satellite files for: XOverdub, XOverworld, XOppossum/OVERBITE, XOblongBob
- XPN tools satellite file (`xpn-tools.md`) verified accurate — all tool paths exist
- XOdyssey satellite file had 2 inaccuracies (source count, repo location) — both fixed

## Documentation Health Score

| Domain | Health | Trend |
|--------|--------|-------|
| Time & Truth | GREEN | Improving — 11 stale facts corrected |
| Structure & Navigation | YELLOW | Improving — concepts/README created; Docs/ parent index still missing |
| Clarity & Quality | YELLOW | Improving — 3 color contradictions + gallery codes fixed |
| Velocity & Utility | ORANGE | Stable — knowledge stubs confirmed empty; 5 high-ROI docs still unwritten |
| Memory & Persistence | GREEN | Stable — MEMORY.md 124 lines; PRIM-005 added |

## Commissions Outstanding

| Priority | What | Status |
|----------|------|--------|
| HIGH | `how_to_write_a_xoceanus_adapter.md` | **DONE** (written this session from source) |
| HIGH | Satellite memory files for XOceanic, XOcelot, XOwlfish | Pending |
| HIGH | `how_to_design_coupling_presets.md` | Pending |
| MEDIUM | XOblongBob + XOppossum knowledge backfill | Pending |
| MEDIUM | `coupling_interaction_cookbook.md` | Pending |
| MEDIUM | Mod matrix reusable module template | Pending |
| MEDIUM | Satellite files for XOverdub, XOverworld | Pending |

---

# PART 2: NEW DELIVERABLES CREATED THIS SESSION

## 1. `how_to_write_a_xoceanus_adapter.md`
- **Path**: `~/Documents/GitHub/XO_OX-XOceanus/Docs/how_to_write_a_xoceanus_adapter.md`
- **Written from actual source code** — not generic advice
- Documents the real 10-method `SynthEngine` interface
- Corrects a misconception: `REGISTER_ENGINE` macro exists but isn't used — real registration is via `EngineRegistry::instance().registerEngine()`
- Covers the 3-method parameter pattern: `addParametersImpl()` / `addParameters()` / `createParameterLayout()`
- Documents the coupling accumulator lifecycle (P0 bug source in early engines)
- Notes the `validEngineNames` check in PresetManager that silently rejects unknown engines

## 2. `/post-engine-completion-checklist` Skill
- **Path**: `~/.claude/skills/post-engine-completion-checklist/SKILL.md`
- **Status**: LIVE in skill registry
- 5-point audit: CLAUDE.md accuracy, XOceanus sync, knowledge compendium, memory/satellite, playbook protocol
- Prevents PRIM-005 drift at the source
- Triggers on: "done", "complete", "phase N complete", "auval passes", post-Seance

## 3. `/producers-guild` Skill
- **Path**: `~/.claude/skills/producers-guild/SKILL.md`
- **Status**: LIVE in skill registry, scheduled every 24h (job `26659f74`)
- 12 named genre specialists + Product Manager (Maya) + Market Research (Derek) + Technical Architect (Ingrid) + Ops Trio (Remy/Sam/Fio) + The Foreseer
- 7-phase protocol: Intake → Genre Panels → PM Synthesis → Market Research → Architecture → Ops → Foreseer
- The Foreseer's Dominoes: traces 7 causal consequences of each major decision
- Supports `scope=full|presets|features|vision` and `quick=true` for fast reads

---

# PART 3: PRODUCER'S GUILD BASELINE REPORT — XOceanus Full Platform

## Platform Snapshot
- **24 engine modules** (7 core DSP-complete, 17 additional at varying stages)
- **1,625 factory presets** across 6 moods (Foundation/Atmosphere/Entangled/Prism/Flux/Aether)
- **15 coupling types** in MegaCouplingMatrix (per-sample, lock-free, 64 routes max)
- **4 simultaneous engine slots** with 50ms hot-swap crossfade
- **PlaySurface**: 4 zones (NoteInput, OrbitPath, PerformanceStrip, PerformancePads)
- **XPN/MPC export pipeline**: drum programs + keygroup programs + cover art + bundle builder
- **Open source, free**

## 12 Genre Specialist Reports (Summary)

### 1. Beatrice "Trap" Morales — Hip-Hop / Trap / Drill
**Strengths**: ONSET BridgedT 808 topology correct, MultibandCompressor with OTT, OVERBITE Belly-to-Bite bass macros, sidechain via coupling
**Top Gap**: No `Amp→Volume` coupling type for true sidechain ducking; no swing/groove on PlaySurface pads
**Top Presets Needed**: Tuned 808 sub, drill slide bass, trap snare stack, coupled bounce kit
**Killer Feature**: **Coupled Bounce** — one-finger trap beat via ONSET+OVERBITE Rhythm→Blend coupling. "No other synth does this. Serum cannot do this. Omnisphere cannot do this."
**Key Requests**: Per-voice pitch envelope on ONSET kick, note repeat/retrigger on pads, MIDI output from ONSET, bounce-to-audio rendering

### 2. Marcus "Deep" Williams — Deep House / Techno / Minimal
**Strengths**: SharedTransport tempo sync (stays locked over 4-hour sets), Cytomic SVF filters (sing at resonance without aliasing), CPU budget <55% at 4 engines, OrbitPath XY with physics
**Top Gap**: No per-LFO tempo sync toggle with division selection; 20ms macro smoothing too slow for filter snaps; no analog-modeled filter drive on core engines
**Top Presets Needed**: Rolling techno kick, acid bassline, Deepchord pad, atmospheric drone, dub techno chord
**Killer Feature**: **OrbitPath as tempo-synced modulation recorder** — record a filter sweep gesture, loop it on every 4-bar cycle, the coupling matrix fans that motion across all 4 engines simultaneously. "No hardware synth does this."
**Key Requests**: Movement lock (loop automation gestures), per-macro smoothing time, audio freeze/infinite hold, adjustable engine hot-swap crossfade (50ms-5000ms)

### 3. Kai Suzuki — Ambient / Drone / Experimental
**Strengths**: ODYSSEY Voyager Drift (per-voice seeded random walk, never repeats), ORGANON Shannon entropy self-mutation, OUROBOROS self-oscillating feedback with strange attractors, OPAL grain freeze, Fretless mode with Infinite Hold
**Top Gap**: No micro-tuning (no Scala import, no just intonation, no Bohlen-Pierce); release tails too short (needs 30-120 seconds); aftertouch absent on ODYSSEY/ORGANON/OUROBOROS/OPAL
**Top Presets Needed**: Infinite sustain drone, spectral freeze pad, self-evolving metallic drone, granular rain field, chaotic feedback bed
**Killer Feature**: **Three models of controlled indeterminacy coupled together** — ODYSSEY (random walk) + ORGANON (entropy) + OUROBOROS (strange attractor) via per-sample MegaCouplingMatrix. "No other synthesizer offers three distinct models of controlled indeterminacy running simultaneously with per-sample cross-modulation."
**Key Requests**: Scala/TUN file import, 120s envelope release times, self-animating Orbit Path (0.03Hz Lissajous), 0.001 Hz LFO floor (16-minute cycle), global spectral freeze

### 4. Sofia Chen — Jazz / Neo-Soul / Fusion
**Strengths**: OBLONG CuriosityEngine (5 behavioral LFO modes: Sniff/Wander/Investigate/Twitch/Nap — per-voice modulation with personality, not shape), OVERBITE 5-macro bass system, theory-aware chord mode, DustTape warmth
**Top Gap**: **No CC64 sustain pedal on most engines** ("I cannot play jazz on it. Full stop."); CC1 mod wheel stripped in adapter layer (regression); velocity only scales volume on half the fleet; chord mode stops at triads
**Top Presets Needed**: Neo-soul electric piano, Wurlitzer bark, Moog-style mono bass, Clavinet funk stab, lo-fi keys tape
**Killer Feature**: **OBLONG CuriosityEngine + OVERBITE split** — behavioral per-voice modulation that makes the sound "curious" under your fingers, plus a 5-macro bass engine that transforms from warm to aggressive with the energy of the performance. "If the velocity and sustain gaps get closed, this is it."
**Key Requests**: CC64 on ALL polyphonic engines (non-negotiable), CC1 restored, 7th chord + extended voicings, velocity curves (exp/log/S-curve), split keyboard, round-robin humanization

### 5. Jerome "Dub" Baptiste — Dub / Reggae / Bass Music
**Strengths**: OVERDUB signal path is a real dub desk (Voice→Send VCA→Drive→Tape Delay→Spring Reverb→Return→Master), tanh saturation correct for transformer emulation, Hermite interpolation on delay time changes, 6-stage allpass spring reverb with coprime delays, 4 performance pads (FIRE/XOSEND/ECHO CUT/PANIC)
**Top Gap**: No rhythmic send level modulation (envelope follower on send); no sweepable filter on delay return; no sidechain ducking on reverb; no ping-pong/multi-tap delay; no lo-fi/dubplate degradation mode
**Top Presets Needed**: Scientist self-oscillating delay, spring crash, steppers bass, Channel One organ bubble, dubwise snare throw, dub siren, pressure drop bass
**Killer Feature**: **ECHO CUT pad** — "In dub, silence is the instrument. The moment you cut the echo — the gap where the repeats were supposed to continue but don't — that is the most powerful moment in the mix." Zero-latency delay kill switch as a performance control. "This is the feature that gets passed around Kingston studio WhatsApp groups."
**Key Requests**: Send level envelope follower (auto-throw), delay time offset from grid (dub is never on the grid), sidechain on reverb return, HP filter sweep on delay return, other engines routing through OVERDUB's send/return chain

### 6. Amara Okonkwo — Afrobeat / World / Global
**Strengths**: 7 distinct engine timbres covering ensemble range (Afrobeat = 20-musician ensemble in one instrument), ONSET 8 voices + 4 algo modes for African percussion, XVC cross-voice coupling as synthesis-level ensemble interlocking, pentatonic scale quantization on PlaySurface
**Top Gap**: **No micro-tuning** (blocks non-Western music entirely — Amapiano log drums, maqam, kora tunings all require non-12-TET); no independent loop lengths per ONSET voice (polymetric rhythm is the foundation); no velocity-to-timbre switching (a djembe has 4 distinct voices, not 4 volumes)
**Top Presets Needed**: Tony Allen kit, Amapiano log drum, Afrobeat horn section, highlife guitar chop, talking drum sweep, kora pluck, balafon marimba, desert blues drone
**Killer Feature**: **XVC Cross-Voice Coupling** — "the first technology I have seen that can reproduce how polyrhythmic music works in software." The bell pattern's amplitude peaks influence the conga's timbre, the kick bends the tom's pitch — the groove emerges from interaction, not individual parts. "When I show this to my colleagues in Lagos and Accra and Dakar, they are going to hear something they recognize. Not the sounds — the behavior."
**Key Requests**: Scala file loading (critical for global music), independent loop lengths per ONSET voice (polymetric), velocity→timbre mode switching on ONSET, zone-based timbral variation within drum pads, call-and-response mode, community tuning table sharing, pattern lock on PlaySurface

### 7. Lars Eriksson — Cinematic / Film Score / Game Audio
**Strengths**: Coupling matrix for multi-engine scenes (3 elements interacting = one cinematic moment), ODYSSEY Climax system (dramatic arcs at the synthesis level), 18-stage MasterFXChain (Doppler, granular smear, frequency shifter, psychoacoustic width), CS-80-style PolyAftertouch, Touche Expression zone, ORGANON/ORACLE for sound design
**Top Gap**: No timecode-locked evolution timeline (scores to picture need exact durations); no surround output (delivers in 7.1.4 Atmos); no convolution reverb (cinematic = real acoustic spaces); no sample import for granular/spectral engines; no per-CC response curves
**Top Presets Needed**: "Slow Reveal" (30-60s evolution), "Tension Wire", "Impact Riser", "Broken Music Box", "Deep Space Transmission", "Breath Under Glass", "Drone Cathedral"
**Killer Feature**: **Cross-engine coupling with the Climax system** — 3 engines coordinated through JOURNEY macro with threshold-triggered bloom. "One performance gesture creates a 30-second arc that would take 200 automation points across 6 plugin instances. No other instrument gives me tension-to-release arcs as a synthesis parameter."
**Key Requests**: Time-locked macro automation lane (absolute seconds, not tempo), MIDI CC response curves per binding, convolution reverb with IR loading, audio file import into granular/spectral engines, surround bus output, preset morphing with timeline automation

### 8. Priya Sharma — Pop / Commercial / Radio
**Strengths**: Harmonic Exciter (adds "air" at 3-5kHz inline), OTT multiband compressor built into master chain, bus compressor with parallel blend, Spectral Tilt (one-knob brightness control), 6D Sonic DNA preset browsing with "Find Similar" / "Find Opposite", VIBE KNOB (GRIT↔SWEET)
**Top Gap**: **No arpeggiator** ("every pop synth I use has an arp — this is a hard requirement"); no unison/super width control at macro level; preset names too artistic for commercial deadlines; no limiter or loudness meter; Intuitive mode should hide engine identity entirely
**Top Presets Needed**: Radio pluck, future bass pad, pop vocal synth, lo-fi keys, trap sub, synth hook machine, ambient breakdown wash, stutter chop, crystal bell lead
**Killer Feature**: **Sonic DNA preset browser with "Find Similar"** — 6D perceptual search (brightness, warmth, movement, density, space, aggression) in 1,625 presets. "No other synth I own does this. I describe a sound in perceptual dimensions and get nearest-neighbor results. Combined with 'Find Opposite' and the free price tag, this competes directly with Serum and Massive X."
**Key Requests**: Arpeggiator (non-negotiable), unison/super width control, functional preset subtitles, final limiter stage, MIXREADY one-knob (exciter+OTT+compressor at preset-tuned ratios), chord mode on pads, FIRE pad as MasterFXSequencer trigger

### 9. Rico Valdez — Latin / Electronic / Club
**Strengths**: OBESE 13 oscillators + Mojo drift for fat reggaeton sub, OVERBITE Belly-to-Bite macro for perreo bass, ONSET Circuit-to-Algorithm blend for morphing kicks in real time, coupling for sidechain pumping (ONSET→OVERBITE AmpToFilter), DNA preset search
**Top Gap**: Tempo sync everywhere (delays, LFOs, bounce — "if not MIDI-clock-syncable, this is a studio toy, not a production tool"); no arpeggiator; no swing on ONSET (dembow is defined by swing); no `Amp→Gain` coupling; no mono compatibility monitoring
**Top Presets Needed**: Dembow kit, perreo bass, Latin brass stab, pluck lead, filtered pad riser, 808 sub drop, vocal chop texture, drop impact (4-engine hit)
**Killer Feature**: **ONSET+OVERBITE live coupling on PlaySurface** — "the drum synthesizer's kick hit physically modulates the bass synthesizer's filter in real time, and you are controlling all of this live on a touch surface. That is not a preset. That is a live performance instrument where the drums and the bass are one organism. The first producer to demo that on Instagram changes the conversation."
**Key Requests**: Tempo-synced delay/LFO divisions (non-negotiable), global swing on ONSET (0-75% MPC-style), MIDI pattern recording/playback inside instrument, `Amp→Gain` coupling type, one-shot mode on ONSET voices, per-engine output routing

### 10. Emma Blackwood — Rock / Alternative / Indie
**Strengths**: OBLONG CuriosityLFO behavioral modulation ("not a shape — a behavior"), OVERBITE Fur/Chew/Gnash/Trash character chain (a pedalboard in software), OUROBOROS leash mechanism (riding feedback like a guitarist), ODDFELIX pitch sweep (pick attack transient), Voyager Drift (real per-voice analog instability), open source (circuit-bent Casio equivalent)
**Top Gap**: **No audio input routing** ("if I could play guitar into OPAL and granulate it in real time, this becomes my live rig, not just a synth"); no guitar-style FX chain available to all engines; no latency information in spec; no MIDI learn
**Top Presets Needed**: Broken amp pad, feedback drone, lo-fi keys, shoegaze wall (OBESE 13 osc + ODDOSCAR coupled), gritty mono lead, spring reverb hit, tremolo pad, "bass that breaks" (velocity-driven distortion)
**Killer Feature**: **CuriosityLFO through velocity-driven OVERBITE saturation** — "Bob's CuriosityLFO is not a shape — it is a behavior. 'Investigate' does not cycle. It explores. When I hold a chord and the filter starts investigating, the sound evolves in a way that never repeats. Now run that through OVERBITE's five character stages with velocity — play soft and the curiosity is gentle; play hard and the curiosity becomes feral. The sound is literally curious about destroying itself. A tube amp that is also slightly sentient."
**Key Requests**: Live audio input into engine slots (transforms synth into processing rig), per-engine bypass with spillover, portamento in poly mode, MPE support (Roli Seaboard), OUROBOROS leash mapped to expression pedal, true bypass/wet-dry on OVERBITE character stages

### 11. Dr. James Thornton — Classical / Orchestral / Acoustic Simulation
**Strengths**: ORBITAL 64 sine partials with 4-group spectral envelopes (body/presence/air/shimmer — independent attack/decay per spectral band), OBSCURA mass-spring scanned synthesis (128-mass chain, Verlet integration, continuous bowing), ODDFELIX Karplus-Strong for pizzicato, ORBITAL formant filter (Peterson & Barney 1952 vowel data), PolyAftertouch (CS-80-style per-voice), coupling for sympathetic resonance
**Top Gap**: No articulation keyswitching (legato/staccato/tremolo/pizzicato within one patch); underdeveloped bow modeling (needs rosin-friction Friedlander-Keller model); no convolution reverb; no CC64 sustain pedal; no micro-tuning beyond Oracle (needs Werckmeister III, Kirnberger III, Vallotti for historical repertoire)
**Top Presets Needed**: Orchestral string section, prepared piano, bowed drone, bell tower/carillon, SATB choir pad, pizzicato ensemble, gamelan metallophone, contemporary extended technique
**Killer Feature**: **ORBITAL coupled to OBSCURA via MegaCouplingMatrix** — physical model (mass-spring string) driving additive synthesis (spectral morph position). "The resonant body of a violin shapes the harmonics of its strings, and the strings excite the body in return. Here, the instrument itself is dynamic. If bow modeling and microtuning are added, this would make it a serious tool for contemporary classical composition."
**Key Requests**: Microtuning file loading (.scl/.tun) for historical temperaments, keyswitching articulation system, convolution reverb with IR loading, OBSCURA bow excitation upgrade to rosin-friction model, ORBITAL expanded to 8 spectral groups, round-robin in XPN export

### 12. Zero_1 — Experimental / Noise / Avant-garde
**Strengths**: OUROBOROS (4 chaotic ODE systems: Lorenz/Rossler/Chua/Aizawa, RK4 solver, leash mechanism for pitch-tracked chaos), ORIGAMI (real-time STFT spectral folding, 2048-point FFT, 4 cascaded operations), ORACLE (Xenakis GENDY stochastic synthesis, 8-32 breakpoints), ORGANON (variational free energy metabolism — a synth that evolves based on what you feed it), OCEANIC (128-particle boid flocking), OVERDUB feedback > 1.0 (growing echoes)
**Top Gap**: No audio-rate feedback between coupled engines (block-delay limitation); OUROBOROS leash prevents "good catastrophes" (safety nets too conservative); no external audio input for coupling; no per-sample parameter modulation (ParamSnapshot is block-rate only); preset randomization too conservative (needs true random "destroy" button)
**Top Presets Needed**: Full divergence (Lorenz at max chaos, no safety), spectral cannibalism (ORIGAMI folding OUROBOROS), swarm collapse (OCEANIC murmuration), metabolic overload (ORGANON force-fed chaos), tuning catastrophe (ORACLE max stochastic walk), quad coupling loop (4 engines in ring)
**Killer Feature**: **OUROBOROS coupled to itself through ORIGAMI** — "Ouroboros audio feeds into Origami's spectral folding, Origami folds/freezes it, routes the spectrally-folded output back into Ouroboros via AudioToFM. The chaotic attractor is being FM-modulated by a spectrally-folded, time-frozen copy of its own previous output. This is the attractor eating a photograph of itself and hallucinating. Nothing else does this."
**Key Requests**: Audio-rate parameter modulation on chaos parameters, external audio input to coupling matrix, "remove all safety limits" mode on OUROBOROS, coupling state recording (reproduce accidents), spectral cross-synthesis between engines, 4x4 feedback coupling matrix (zero-delay)

---

## Phase 3: Product Manager Synthesis (Maya)

### Cross-Panel Feature Priority (all 12 specialists voting)

| Rank | Feature | Guild Votes | Effort | Impact | Category |
|------|---------|------------|--------|--------|----------|
| **1** | **CC64 sustain pedal on all poly engines** | 4/12 | Low | **BLOCKER** | Expression |
| **2** | **Global arpeggiator (tempo-synced, swing, patterns)** | 3/12 | Medium | **BLOCKER** | Workflow |
| **3** | **CC1 mod wheel restored in adapter layer** | 3/12 | Low | **REGRESSION** | Expression |
| **4** | **D001 velocity→timbre completion fleet-wide** | 5/12 | Medium | **BLOCKER** | Expression |
| **5** | **Amp→Volume/Gain coupling type** (13th type) | 4/12 | Medium | High | Coupling |
| **6** | **Micro-tuning / Scala file loading** | 4/12 | Medium | High | Accessibility |
| **7** | **Audio input routing into engine slots** | 4/12 | High | High | Platform |
| **8** | **OrbitPath expansion** (gesture loop, self-animate, any-param) | 5/12 | Medium | High | Differentiation |
| **9** | **Tempo sync on all time-based parameters** | 3/12 | Medium | High | Workflow |
| **10** | **Aftertouch on core engines** (ODYSSEY, ORGANON, OUROBOROS, OPAL) | 4/12 | Low | Medium | Expression |
| 11 | Swing/groove on ONSET | 3/12 | Low | Medium | Workflow |
| 12 | 7th chord + extended voicing | 2/12 | Medium | Medium | Musical |
| 13 | Independent loop lengths per ONSET voice | 2/12 | Medium | Medium | Polymetric |
| 14 | Velocity curves on PlaySurface | 3/12 | Low | Medium | Expression |
| 15 | Split keyboard mode | 2/12 | Medium | Medium | Performance |
| 16 | Per-engine mute/solo on Performance Pads | 2/12 | Low | Medium | Performance |
| 17 | Convolution reverb (IR loading) | 2/12 | High | Medium | Sound quality |
| 18 | Preset functional subtitles | 1/12 | Low | Medium | Discoverability |
| 19 | MPE support | 1/12 | High | Medium | Expression |
| 20 | Other engines through OVERDUB send/return | 2/12 | Medium | High | Architecture |

### Preset Gap Analysis (98 archetypes identified across 12 specialists)

| Category | Missing Count | Highest-Value Archetypes |
|----------|--------------|------------------------|
| Tuned Bass (808, Steppers, Perreo) | 8 | Tuned 808 sub, drill slide bass, perreo bass, steppers bass |
| World Percussion | 10 | Tony Allen kit, Amapiano log drum, talking drum, djembe, balafon |
| Electric Keys | 6 | Neo-soul EP, Wurlitzer, Clavinet, lo-fi keys |
| Dub FX Performance | 7 | Scientist delay, spring crash, dub siren, dubwise snare throw |
| Evolving Drones | 6 | Infinite sustain, spectral freeze, self-evolving metallic |
| Coupled Performance | 8 | Coupled Bounce kit, drop impact, quad coupling loop |
| Cinematic | 8 | Slow reveal, tension wire, broken music box, deep space transmission |
| Pop/Radio-Ready | 10 | Radio pluck, future bass pad, synth hook machine, crystal bell lead |
| Rock/Character | 10 | Broken amp pad, feedback drone, shoegaze wall, spring reverb hit |
| Afrobeat/World | 10 | Afrobeat horn section, highlife guitar, kora pluck, desert blues |
| Techno/Minimal | 10 | Rolling techno kick, acid bassline, Deepchord pad, minimal perc |
| Experimental/Noise | 10 | Full divergence, spectral cannibalism, swarm collapse, boid FM |
| Classical/Orchestral | 8 | String section, prepared piano, bowed drone, carillon, choir |

### XPN Export Enhancements

| Enhancement | Requested By | Impact |
|------------|-------------|--------|
| Melodic 808 keygroup export | Beatrice | High |
| Long-form rendering (30-60s for drones) | Kai, Thornton | High |
| Round-robin variations (3-4 per note) | Marcus, Sofia, Thornton, Rico | High |
| Pattern/MIDI clip inside XPN | Rico | Medium |
| Pentatonic-only note mapping | Amara | Medium |
| XVC-coupled ensemble renders | Emma | Medium |
| Velocity layers with timbral change (not just volume) | Sofia, Amara | High |
| Effect chain metadata in XPN | Jerome | Low |
| Custom cover art override | Priya | Low |
| Dub mixing kit XPN template | Jerome | Medium |

---

## Phase 4: Market Research (Derek)

### Competitive Position

| Feature | XOceanus | Serum ($189) | Pigments ($199) | Massive X ($149) | Vital (Free) |
|---------|----------|------|----------|----------|-------|
| Multi-engine coupling | **12 types, per-sample** | No | No | No | No |
| Engine count | **24 (7 core)** | 2 | 4 | 1 | 3 |
| Cross-voice drum coupling (XVC) | **Yes** | No | No | No | No |
| 6D perceptual preset search | **Yes** | No | No | No | No |
| XPN/MPC export | **Yes** | No | No | No | No |
| CC64 sustain pedal | **MISSING** | Yes | Yes | Yes | Yes |
| Arpeggiator | **MISSING** | Yes | Yes | Yes | Yes |
| Velocity→timbre | **Incomplete** | Yes | Yes | Yes | Yes |
| Price | **Free** | $189 | $199 | $149 | Free |

**Derek's verdict**: 8 genuine differentiators that no competitor matches — but 5 missing table-stakes features prevent anyone from discovering them. **Fix the floor before raising the ceiling.**

### Market Whitespace
1. Multi-engine coupling at synthesis level — **no competitor**
2. Dedicated drum synthesis with XVC — **no competitor**
3. 6D perceptual preset search — **no competitor**
4. XPN/MPC export from synth — **no competitor**
5. Open source + free + 24 engines — only Vital competes on price; Vital has no coupling

---

## Phase 5: Technical Architecture (Ingrid)

### Sequenced Roadmap

**Phase A: Expression Foundation (Low effort, DO FIRST)**
1. CC64 sustain pedal — wire to all polyphonic engines
2. CC1 mod wheel — restore in adapter layer (regression fix)
3. Aftertouch — wire `PolyAftertouch.h` to ODYSSEY, ORGANON, OUROBOROS, OPAL
4. Velocity→timbre — audit + wire filter/character to velocity on remaining engines
5. Velocity curves — add curve selection to PlaySurface NoteInputZone

**Phase B: Workflow Essentials (Medium effort)**
1. Global arpeggiator — `Source/Core/Arpeggiator.h`, synced via SharedTransport
2. Tempo sync — expose `getPhaseForDivision()` on all LFOs and delay times
3. Swing on ONSET — global shuffle percentage
4. `Amp→Volume` — 13th coupling type in MegaCouplingMatrix
5. 7th chord + extensions in ChordMachine

**Phase C: Differentiation Amplification (Medium effort)**
1. OrbitPath — any-param routing, gesture recording + loop, self-animation
2. Other engines through OVERDUB send/return
3. Split keyboard mode
4. Performance Pad remapping per preset
5. Independent loop lengths per ONSET voice
6. Pattern lock on PlaySurface

**Phase D: Platform Expansion (High effort)**
1. Micro-tuning / Scala file loading
2. Audio input routing into engine slots
3. Convolution reverb (IR loading)
4. Articulation keyswitching

**Phase E: Advanced (High effort, Specialized)**
1. MPE support
2. Surround output (7.1.4)
3. Timecode-locked macro automation
4. Audio-rate parameter modulation
5. Spectral cross-synthesis
6. MIDI output from ONSET

---

## Phase 6: Ops Review (The Trio)

**Remy**: CC64/CC1/aftertouch are wiring tasks — zero regression risk. Ship immediately.

**Sam**: Arpeggiator is highest implementation risk in Phase B. Follow SharedTransport pattern. Generate MIDI internally, don't touch the audio path.

**Fio**: Micro-tuning has hidden cost — audit every `noteToFrequency` call. Any hardcoded `440 * pow(2, (note-69)/12.0)` will ignore the Scala file.

**Consensus**: Phases A+B can ship together. Phase C is the fun stuff. Phase D needs careful architecture. Don't attempt audio-rate parameter modulation (Zero_1's request) — the block-rate ParamSnapshot pattern is a safety feature, not a limitation.

---

## Phase 7: The Foreseer

### The Unseen Issue
**Coupling makes presets non-portable.** A preset using ONSET→OVERBITE coupling sounds completely different if the user has different engines loaded. As the library grows past 2,000, "your preset sounds nothing like the demo" becomes the #1 support complaint. The coupling matrix is both the signature feature and the signature fragility.

### The Unseen Opportunity
Sofia (Jazz) and Amara (Afrobeat) independently asked for the same principle from different traditions: **synthesis that has agency** — not random, not LFO-driven, but responsive to musical context. ORGANON's metabolism already does this. Extract that principle into a platform-wide **"Responsiveness Layer"** — ambient musical awareness across all engines.

### The Foreseer's Dominoes

**Decision 1: Fix CC64/CC1/velocity (Phase A)**
```
Fix expression basics →
  1. Keyboard players can perform on XOceanus
  2. Jazz, neo-soul, classical producers adopt
  3. They generate expressive content (demos, tutorials, presets)
  4. Content reveals what XOceanus sounds like when PLAYED, not programmed
  5. Creates new position: "the free synth that feels like a real instrument"
  6. Expressive controller manufacturers (Roli, Sensel) seek integration
  7. XOceanus becomes default synthesis backend for expressive controllers
```

**Decision 2: Ship the arpeggiator (Phase B)**
```
Add arpeggiator →
  1. Pop/Latin/electronic producers can make hooks
  2. Arp patterns interact with coupling matrix (unprecedented)
  3. "Coupled arps" become a new sound design category
  4. YouTube producers make viral "coupled arp" videos
  5. Forces competitors to explain why THEIR arps don't cross engines
  6. Competitors cannot add coupling retroactively
  7. XOceanus owns "coupled arps" permanently — first and free
```

**Decision 3: Add micro-tuning (Phase D)**
```
Ship Scala file loading →
  1. Non-Western producers can use XOceanus natively
  2. Only free synth treating non-12-TET as first-class
  3. Producers in Lagos, Istanbul, Mumbai, Cairo, Jakarta discover it
  4. User base diversifies — new presets, new coupling recipes, new contexts
  5. XOceanus presets appear in Amapiano, Bollywood, Arabesk, Gamelan
  6. Western synth manufacturers realize they've ignored 80% of the world
  7. XOceanus becomes reference platform for global electronic music
```

### The Prediction (18 months)
> The conversation will not be about engine count or coupling types. It will be about **responsiveness** — the feeling that the instrument is listening to you. The CuriosityEngine, XVC, ORGANON's metabolism, the Climax system, Voyager Drift — these are all early forms of the same idea: synthesis that has agency. The producers who stay past the first week will be the ones who played a chord, heard the CuriosityEngine start investigating, felt the Voyager Drift pull voices apart, watched coupling create interactions they didn't program — and recognized that the instrument was doing something no preset told it to do. The technical term is "emergent expressivity." The market term is "it feels alive."

### Vision Statement
> XOceanus is not a synthesizer with more engines. It is the first instrument where the engines listen to each other and to the performer. The roadmap is not a feature list — it is a path from "a powerful tool that requires expertise" to "a responsive instrument that rewards expression." Fix the expression floor, then invest in the responsiveness ceiling. The result will be an instrument that feels alive — and the only one of its kind that is free.

---

## Session Metrics

| Activity | Count |
|----------|-------|
| Archivist agents dispatched | 5 |
| Genre specialist agents dispatched | 12 (6 paired panels) |
| Stale facts corrected | 11 |
| Documents created | 3 (concepts/README.md, PRIM-005, adapter recipe) |
| Skills created | 2 (`/post-engine-completion-checklist`, `/producers-guild`) |
| Skills updated | 1 (`/producers-guild` — Foreseer's Dominoes added) |
| Reports generated | 3 (Historical Society, Guild Baseline, this readout) |
| Feature requests catalogued | 20 (prioritized) |
| Preset archetypes identified | 98 |
| XPN enhancements identified | 10 |
| PlaySurface improvements identified | 11 |

---

*Report generated 2026-03-14 by the Producer's Guild + Historical Society, XO_OX Designs.*
