# XPN Collection: First Five Paid Packs — Build Spec
**Date:** 2026-03-16
**Project:** XO_OX Patreon — Current Tier Vault
**Status:** Design spec — ready for sound design and export

---

## Purpose

These five packs are the vault that must exist before Patreon launches. A Deep Water subscriber who joins on day one should be able to make a full record immediately — drums handled by the free TIDE TABLES pack, everything else covered here. Each pack targets a distinct production role so there is no overlap and no gap.

**Collective coverage:** bass/synthesis, granular atmosphere, lo-fi character, electronic beat tools, flagship coupling showcase.

**None of these packs duplicate TIDE TABLES** (ONSET drum kits, perc-mode only).

---

## Pack 1: DRIFT SAMPLES — Bass + Synthesis

**Primary engine:** ODYSSEY (`drift_` prefix)
**Accent:** Violet `#7B2D8B`
**Target presets:** 60 presets across 4 programs

### Why ODYSSEY for bass

ODYSSEY is a drift synth — slow analog oscillator drift, detuning instability, and a filter that breathes. These qualities make it ideal for bass work that refuses to sit perfectly in place: sub tones that swell, distorted mid-bass that shifts in character, modular-adjacent sequences with filter automation doing the legwork. The `drift_oscA_mode` and `drift_oscB_detune` parameters produce the kind of unstable unison bass that sounds expensive. OVERDUB's spring reverb can couple in for low-frequency splash, but DRIFT SAMPLES is single-engine by design — every pad is immediately playable, no coupling required.

### Programs (4)

| Program | Concept | Preset count | Mood target |
|---------|---------|-------------|-------------|
| Sub Architecture | Pure sine/triangle sub foundations, 1-3 octave spread, clean filter sweeps | 15 | Foundation / Flux |
| Analog Pressure | Mid-bass punch with oscillator character: saw stacks, aggressive filter envelopes, velocity → brightness | 15 | Foundation / Prism |
| Drift Sequence | Arpeggiated and sequenced bass tones — slow LFO filter movement, rhythmic interest, loop-friendly | 15 | Flux / Entangled |
| Character Bass | Distorted, textured, or harmonically complex bass — talks to OVERDUB's drive section conceptually | 15 | Atmosphere / Aether |

### Mood distribution
Foundation 30%, Flux 25%, Prism 20%, Entangled 15%, Atmosphere 10%

### Must-subscribe value
ODYSSEY's bass register sits differently from every other engine in the fleet. Producers who are skeptical about synthesis packs respond to bass because it is functional, not decorative. DRIFT SAMPLES leads with utility and earns the deeper Patreon commitment.

### Tools required
- `xpn_keygroup_export.py` — multi-zone keygroup XPN files per program
- `xpn_render_spec.py` — render spec per preset (sample rate, length, root note)
- `xpn_drum_export.py` — N/A (this is keygroup, not drum mode)
- `xpn_packager.py` — final ZIP assembly with manifest
- `xpn_manifest_generator.py` — program + preset metadata
- `xpn_cover_art_generator_v2.py` — pack cover at Violet `#7B2D8B`

---

## Pack 2: PHOSPHOR — Atmosphere + Pads

**Primary engine:** OPAL (`opal_` prefix)
**Accent:** Lavender `#A78BFA`
**Target presets:** 60 presets across 4 programs

### Why OPAL for atmosphere

OPAL is the granular engine — it reads source material as a cloud of overlapping grains that can be frozen, stretched, scattered, or slowly drifted. The `opal_grainSize`, `opal_grainDensity`, and `opal_scatter` parameters produce sustained textures that evolve on their own without explicit LFO assignment. The result is pads that do not sound like conventional synthesis: spectral shimmer, frozen-moment reverb textures, slow-building harmonic clusters. For sample-based MPC producers who have never touched a granular synth, PHOSPHOR is the bridge.

### Programs (4)

| Program | Concept | Preset count | Mood target |
|---------|---------|-------------|-------------|
| Frozen Moment | Long-tail frozen pads — grain freeze, maximum density, slow amplitude modulation | 15 | Atmosphere / Aether |
| Scatter Field | Active grain scatter — textural motion, rhythmic density fluctuation, unpredictable shimmer | 15 | Flux / Entangled |
| Harmonic Drift | Pitch-shifted grain clusters tuned to harmonic intervals — playable pad chords with drift | 15 | Atmosphere / Foundation |
| Signal Decay | Granular processing of decaying transients — gate-reverb textures, compressed-grain punch-pad hybrids | 15 | Prism / Flux |

### Mood distribution
Atmosphere 35%, Flux 25%, Aether 20%, Entangled 10%, Foundation 10%

### Must-subscribe value
There is no loop-based equivalent to a well-designed granular pad. PHOSPHOR occupies a space on the MPC that sample packs cannot — these are living textures, not recordings. The pack demonstrates that XOceanus is a sound design instrument, not a sample playback machine.

### Tools required
- `xpn_keygroup_export.py` — full chromatic keygroup mapping (OPAL is melodic)
- `xpn_render_spec.py` — longer render lengths for evolving pads (4–8 bar captures)
- `xpn_packager.py` — final ZIP
- `xpn_manifest_generator.py`
- `xpn_cover_art_generator_v2.py` — Lavender `#A78BFA`
- `xpn_sample_loop_optimizer.py` — check loop points for sustained pad samples

---

## Pack 3: CARBON PAPER — Lo-Fi + Character

**Primary engines:** OVERDUB (`dub_` prefix) + OBESE (`fat_` prefix)
**Accents:** Olive `#6B7B3A` / Hot Pink `#FF1493` — pack identity uses Olive as lead
**Target presets:** 60 presets across 4 programs

### Why this engine pairing

OVERDUB is a dub machine: voice oscillator, send VCA, drive, tape delay, spring reverb. Its `dub_tapeSpeed` and `dub_springDecay` parameters are the lo-fi color source. OBESE is a saturation/compression engine with `fat_satDrive` and `fat_mojo` — the Mojo Control is an orthogonal analog/digital axis that shifts between warm tape saturation and aggressive digital clip. Together they cover the two flavors of lo-fi: the organic warmth of tape degradation (OVERDUB) and the grittier, more abrasive lo-fi aesthetic of crushed dynamics and intentional distortion (OBESE).

### How combining two engines works in a pack

Each program is built around a dominant engine, with the secondary engine used as a single-layer color. CARBON PAPER is not a coupling showcase — the coupling is subtle and preset-embedded. Programs 1 and 3 are OVERDUB-dominant with OBESE providing saturation drive on top. Programs 2 and 4 are OBESE-dominant with OVERDUB's tape delay providing the trailing verb. Producers experience two engines as one cohesive sonic identity without needing to understand the routing.

### Programs (4)

| Program | Dominant engine | Concept | Preset count | Mood target |
|---------|----------------|---------|-------------|-------------|
| Tape Ghost | OVERDUB | Spring reverb + tape delay saturated bass tones, dub organ, sustained melodic leads | 15 | Atmosphere / Foundation |
| Warm Crush | OBESE | Bitcrushed and saturated one-shots, compressed pads, purposefully damaged timbres | 15 | Flux / Prism |
| Dub Space | OVERDUB | Long tape delay textures, ping-pong dub echoes, melodic fragments in space | 15 | Aether / Atmosphere |
| Character Drive | OBESE | Maximum Mojo aggressive leads and stabs — lo-fi with an edge, not lo-fi as aesthetic only | 15 | Foundation / Prism |

### Mood distribution
Foundation 25%, Atmosphere 25%, Flux 20%, Prism 15%, Aether 15%

### Must-subscribe value
Lo-fi is one of the most searched MPC pack categories. CARBON PAPER answers that demand with synthesis rather than recordings, meaning every preset is unique to XOceanus. The OVERDUB spring reverb (Seance B004, praised by Vangelis + Tomita) is the sonic proof point — it sounds unlike any convolution reverb.

### Tools required
- `xpn_keygroup_export.py` — melodic programs use keygroup format
- `xpn_drum_export.py` — for Warm Crush one-shots (velocity/cycle kit modes)
- `xpn_kit_expander.py` — expand velocity layers for character one-shots
- `xpn_packager.py`
- `xpn_manifest_generator.py`
- `xpn_cover_art_generator_v2.py` — Olive `#6B7B3A`
- `xpn_sample_categorizer.py` — sort renders into melodic vs. drum-mode bins

---

## Pack 4: SIGNAL CHAIN — Electronic + Beat Tools

**Primary engines:** ONSET (`perc_` prefix, tonal/melodic mode) + OVERWORLD (`ow_` prefix)
**Accents:** Electric Blue `#0066FF` / Neon Green `#39FF14` — Electric Blue leads
**Target presets:** 60 presets across 4 programs

### Why this pairing, and not just drums

TIDE TABLES already ships ONSET drum kits. SIGNAL CHAIN exploits ONSET's lesser-known tonal side: the `perc_noiseLevel` parameter ranges from pure pitched tone to pure noise — at low noise values, ONSET produces chip-like melodic percussion and electronic stabs that no drum-mode kit exposes. OVERWORLD's ERA triangle (NES 2A03 / Genesis YM2612 / SNES SPC700) provides chip synthesis that sits directly alongside ONSET's tonal percussion register. The combination produces the full electronic toolkit: not just kicks and hats but the synth hits, melodic perc, and tonal textures that complete a beat.

### Programs (4)

| Program | Dominant engine | Concept | Preset count | Mood target |
|---------|----------------|---------|-------------|-------------|
| Tonal Perc | ONSET | Melodic percussion — tuned transients, mallet-like attacks, electronic woodblock and marimba analogs | 15 | Foundation / Prism |
| ERA Synthesis | OVERWORLD | Chip leads and pads from the ERA triangle — 8-bit leads, 16-bit pads, retro-future crossfades | 15 | Flux / Prism |
| Hybrid Stabs | ONSET + OVERWORLD | Short attack, fast decay stabs combining tonal perc body with chip oscillator edge | 15 | Foundation / Flux |
| Electronic Texture | OVERWORLD | Longer ERA chord textures, atmospheric chip pads, ambient game-music material | 15 | Atmosphere / Aether |

### Mood distribution
Foundation 30%, Flux 25%, Prism 20%, Atmosphere 15%, Aether 10%

### Must-subscribe value
This pack unlocks ONSET's non-drum potential — something TIDE TABLES deliberately avoids demonstrating. It also introduces OVERWORLD, which is likely unknown to most MPC producers and delivers a chip-synthesis voice unavailable anywhere else in the MPC ecosystem.

### Tools required
- `xpn_keygroup_export.py` — ERA Synthesis and Electronic Texture use keygroup format
- `xpn_drum_export.py` — Tonal Perc and Hybrid Stabs use drum-kit XPN format
- `xpn_kit_expander.py` — velocity layers for stab programs
- `xpn_adaptive_velocity.py` — ONSET tonal velocity curves (musical response, not linear)
- `xpn_packager.py`
- `xpn_manifest_generator.py`
- `xpn_cover_art_generator_v2.py` — Electric Blue `#0066FF`

---

## Pack 5: WATER COLUMN — Flagship Showcase

**Primary engines:** OPAL + ODYSSEY + OVERDUB + OVERWORLD + ONSET (5 engines)
**Accents:** Ocean-depth gradient — no single engine color owns the cover
**Target presets:** 80 presets across 6 programs

### Concept

WATER COLUMN is the argument for Deep Water. Every other pack demonstrates one engine or a tight pairing. This pack demonstrates what XOceanus uniquely enables: coupling presets where two or three engines modulate each other, all 7 moods represented, maximum variety from a single pack. A producer who loads this and explores for an hour understands what the platform is. It should be the last free sample that convinces them to subscribe.

The pack name references the XO_OX water column mythology — engines mapped across depth zones from surface to trench — and provides the narrative spine for the cover art and liner notes.

### Programs (6)

| Program | Engines | Coupling type | Concept | Preset count | Mood target |
|---------|---------|---------------|---------|-------------|-------------|
| Surface Light | OPAL + ODDFELIX | Spectral coupling | Granular shimmer + neon tetra brightness — sparkling surface textures | 12 | Atmosphere / Aether |
| Thermocline | ODYSSEY + OBESE | Envelope coupling | Drifting bass tones processed through Mojo saturation — pressure and warmth | 12 | Foundation / Flux |
| Bioluminescent | OVERWORLD + OPAL | LFO coupling | ERA chip synthesis grain-processed — chip textures dissolving into granular clouds | 14 | Prism / Entangled |
| Mid-Water | OVERDUB + ODYSSEY | Amplitude coupling | Dub tape delay on drifting bass — deep lo-fi atmospheres with physical space | 14 | Atmosphere / Foundation |
| Hadal Zone | ONSET + OVERWORLD | Pitch coupling | Tonal ONSET perc pitched by OVERWORLD ERA oscillators — alien depth percussion | 14 | Entangled / Aether |
| Full Column | 4-engine routing | Multi-coupling | Flagship coupling presets — OPAL+ODYSSEY+OVERDUB+OVERWORLD simultaneously | 14 | Family / Entangled |

### Mood distribution
All 7 moods represented. Family mood appears exclusively in Full Column program.
Atmosphere 20%, Entangled 20%, Foundation 15%, Aether 15%, Flux 15%, Prism 10%, Family 5%

### Must-subscribe value
WATER COLUMN cannot be replicated by any other MPC pack or plugin. The coupling presets in Full Column are not available anywhere else in the pack catalog — they require Deep Water back-catalog access to receive. The pack doubles as the primary marketing artifact: audio clips from Surface Light and Bioluminescent are the announcement content for Patreon launch.

### Tools required
- `xpn_keygroup_export.py` — melodic and pad programs
- `xpn_drum_export.py` — Hadal Zone percussion kits
- `xpn_generate_coupling_presets.py` — automated coupling preset generation for multi-engine programs
- `xpn_coupling_recipes.py` — defines valid coupling routes per program
- `xpn_coupling_preset_auditor.py` — validate coupling chain correctness before export
- `xpn_kit_expander.py` — velocity layers for Full Column presets
- `xpn_packager.py`
- `xpn_manifest_generator.py`
- `xpn_liner_notes.py` — generate per-program liner notes (mythology context)
- `xpn_cover_art_generator_v2.py` — water column depth gradient, custom multi-accent treatment

---

## Vault Summary

| Pack | Engine(s) | Programs | Presets | Primary role |
|------|-----------|----------|---------|-------------|
| DRIFT SAMPLES | ODYSSEY | 4 | 60 | Bass / synthesis |
| PHOSPHOR | OPAL | 4 | 60 | Atmosphere / granular pads |
| CARBON PAPER | OVERDUB + OBESE | 4 | 60 | Lo-fi / character |
| SIGNAL CHAIN | ONSET + OVERWORLD | 4 | 60 | Electronic / beat tools |
| WATER COLUMN | 5 engines | 6 | 80 | Flagship / coupling showcase |
| **TOTAL** | **8 engines** | **22** | **320** | |

With TIDE TABLES (drums) + these 5 packs, a Deep Water subscriber on day one has drums, bass, atmosphere, lo-fi, electronic textures, and a full coupling showcase. A full record is buildable from the vault alone.

---

## Build Order

1. DRIFT SAMPLES — simplest export path, single engine, bass preset design is fastest
2. PHOSPHOR — OPAL export requires longer render times; start renders early
3. CARBON PAPER — two-engine programs, moderate complexity
4. SIGNAL CHAIN — dual-mode export (drum + keygroup in same pack)
5. WATER COLUMN — highest complexity; coupling validation required before export; build last

---

*Spec complete. Sound design and render passes follow.*
