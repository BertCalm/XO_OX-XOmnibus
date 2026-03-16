# XOmnibus V2 Roadmap

*Drafted: 2026-03-14 (mid next week target: ~2026-03-18)*
*Scope rule: V1 = everything that existed by 2026-03-14 EOD. V2 = anything born after that date.*

---

## What V1 Ships

34 engines registered (24 original + 5 Constellation + OVERLAP + OUTWIT + OMBRE + ORCA + OCTOPUS), 2,451 presets, 7 moods, full doctrine compliance, all seances complete.
The Constellation (OHM/ORPHICA/OBBLIGATO/OTTONI/OLE) is integrated as stubs, swept, and description-complete.
OVERLAP and OUTWIT are Phase 3 complete in standalone form and pending XOmnibus installation.
This document is what comes next.

---

## Tier 1 — Seance Visions (Ghost-Endorsed DSP)

Specific, scoped, implementable. Each blessed by at least one ghost with a concrete proposal.

| ID | Vision | Engine | Ghost | Effort | Impact |
|----|--------|--------|-------|--------|--------|
| V011 | LAKE delay: 250ms → 2000ms (kDelMax + LAKE-scaled delTime) | OTTONI | Schulze | S | High — unlocks "mountain horn" acoustic depth |
| V009 | Per-voice spatial pan by Dad instrument (banjo L, fiddle R) | OHM | Tomita | S | Med — timbral stereo field separation |
| V010 | Aftertouch → COMMUNE routing (leaning = merging the family) | OHM | Vangelis | S | High — expressivity breakthrough for OHM |
| V003 | Chain Freeze/Unfreeze — live spectral hold performance | OBSCURA | Buchla | M | Med — live spectral theatre |
| V004 | `instantaneousFreq` as spectral compass (unused fold variable) | ORIGAMI | Vangelis | S | Low — wires already-computed value |
| V005 | Oracle crystallize — freeze a living maqam and hold it | ORACLE | Tomita, Schulze | M | Med — preserves generative moment |
| V006 | Organon long-term memory — instrument that learns the player | ORGANON | Schulze, Buchla | L | Very high — paradigm-shifting |
| V008 | OPAL as time telescope — universal audio-to-grain transformer | OPAL | All 8 | L | Very high — repositions OPAL as hub |

**Near-term picks (S = small, ship first):** V011, V009, V010, V004 — all single-file changes under 50 lines.

---

## Tier 2 — New Engines (Concept Phase → Build)

Four engines are fully designed with identity cards and aquatic species. No DSP written. V2 builds them.

### OSTINATO — The Communal Drum Circle
- **Identity**: Communal drum circle — 8 seats, each a player with instrument + personality. MACHINE macro drives the communal groove complexity.
- **Accent**: Firelight Orange `#E8701A` | **Prefix**: `osti_` | **Species**: Sea turtle colony
- **Water column**: SUNLIT SHALLOWS (warm, communal)
- **Signature mechanic**: 8-voice polyrhythmic sequencer where each voice is a physical drum model with its own personality bias (hurries, drags, accents randomly)
- **Why now**: Fills the rhythmic gap in the fleet. ONSET is electronic; OSTINATO is acoustic/communal.
- **Coupling targets**: OSTINATO → ONSET (acoustic/electronic hybrid kit), OSTINATO → OHM (Dad plays with the circle)

### OPENSKY — Pure feliX
- **Identity**: Euphoric shimmer engine — pure feliX energy, no Oscar. Wavetable + bright additive + sky-high reverb.
- **Accent**: Sunburst `#FF8C00` | **Prefix**: `sky_` | **Species**: Flying fish school (surface breakers)
- **Water column**: THE SURFACE (feliX side, pure sunlight)
- **Signature mechanic**: SHIMMER macro — stacks pitched shimmer voices above the fundamental, creating cathedral-in-open-air sound
- **Why now**: The fleet has no pure-bright euphoric lead. Every engine has Oscar-side gravity. OPENSKY is counterweight.

### OCEANDEEP — Pure Oscar
- **Identity**: Abyssal bass engine — pure Oscar, sub-frequency physical modeling, pressure + darkness.
- **Accent**: Trench Violet `#2D0A4E` | **Prefix**: `deep_` | **Species**: Giant squid
- **Water column**: THE ABYSS (deepest point, maximum pressure)
- **Signature mechanic**: PRESSURE macro — models the physical crushing effect of depth on resonant bodies; subharmonic saturation + dynamic compression increase with depth
- **Why now**: Fleet bottom-end anchor. Pairs with OPENSKY as feliX/Oscar polarity extremes.

### OUIE — The Duophonic Hammerhead
- **Identity**: Duophonic instrument — two voices on a single STRIFE↔LOVE axis. Two hammerhead shark eyes see the same scene from opposite sides.
- **Accent**: Hammerhead Steel `#708090` | **Prefix**: `ouie_` | **Species**: Hammerhead shark
- **Water column**: OPEN WATER (apex, surveying all zones)
- **Signature mechanic**: STRIFE↔LOVE axis — at STRIFE, voices fight (dissonant intervals, competing envelopes); at LOVE, voices merge (unison with phase drift, shared envelope). Continuous morphing between opposition and unity.
- **Why now**: No other engine in the fleet models the relationship between exactly two voices as its primary mechanic.

---

## Tier 3 — Theorem Engines (New Math → New Engines)

Three engines approved by Theorem on Pi Day 2026. Each has a mathematical foundation the fleet currently lacks.

### OVERTONE — The Nautilus
- **Identity**: Continued fractions spectral engine — pitch expressed as infinite series rather than fixed ratio
- **Accent**: `#A8D8EA` | **Prefix**: `over_` | **Species**: Nautilus
- **Core DSP**: Continued fraction oscillator — each harmonic partial's ratio is computed from a truncated continued fraction expansion. Adding depth terms = adding harmonic complexity.
- **Signature**: DEPTH macro unfolds the continued fraction, transforming a pure sine into an ever-more-complex spectral shape

### KNOT — The Kelp Knot
- **Identity**: Topological bidirectional coupling mode — not a sound engine but a new coupling type
- **Accent**: `#8E4585` | **Prefix**: `knot_`
- **Core concept**: Engines in a KNOT configuration exchange modulation in both directions simultaneously, with the coupling strength between any pair determined by their "knot distance" — a topological metric
- **Note**: This is a coupling architecture addition, not a standalone engine. May ship as a MegaCouplingMatrix extension rather than a gallery engine.

### ORGANISM — The Coral Colony
- **Identity**: Cellular automata generative engine — each voice is a cell; reproduction, death, and state transitions generate melody and rhythm
- **Accent**: `#C6E377` | **Prefix**: `org_`
- **Core DSP**: Conway-adjacent rule set where each voice's amplitude and pitch are determined by neighbor state. Rules are selecteable (survive/born conditions).
- **Signature**: COLONY macro controls the rule set density — sparse rules produce sparse melody; dense rules produce polyrhythmic noise

---

## Tier 4 — Platform Improvements

Non-engine features that make V1 better for everyone.

### AudioToBuffer Phase 3
- `IAudioBufferSink` interface, DFS cycle detection, FREEZE state machine
- Fully spec'd in `Docs/audio_to_buffer_phase3_spec.md` — just needs implementation
- **Impact**: Enables real-time audio routing between any engine pair without latency artifacts

### Master Spec Rewrite — Section 3
- "The Seven Engines" table is stale (claims 7, ships 31)
- Needs a 31-engine canonical table with voice counts, CPU estimates, coupling roles
- Low priority but fixes the doc for new contributors

### Constellation Knowledge Compendium
- 5 engines missing `synth_playbook/agent_knowledge/` files (architecture, DSP, presets, coupling)
- Directories created; content deferred from today's session
- Needed before a second Guru Bin retreat on any Constellation engine

### OTTONI GROW Default Presets
- Default changed to 0.35f (Tween range) but existing presets were authored with 0.0f default in mind
- A listening pass on OTTONI Foundation presets is warranted to check init patch character

---

## Tier 5 — Deferred Doctrine Improvements

Improvements to existing engines identified in seances but not urgent enough for V1 patch.

| Engine | Finding | Effort | Notes |
|--------|---------|--------|-------|
| OHM | Mono voice summing (Dad voices not stereo-spread by instrument) | S | V009 addresses this |
| OHM | SIDES LFO rate control (SIDES has no LFO — only manual sweep) | S | B019 known limitation |
| ORPHICA | kBufSize 186ms — too short for slow attack granular textures | S | Extend to 512ms or 1s |
| OBBLIGATO | FX chain routing audit — misrouted at 7.8 seance | M | Needs read + targeted fix |
| OTTONI | LAKE macro only scales reverb/delay mix, not delay time | S | V011 addresses this |
| Fleet | Master spec "Seven Engines" section rewrite | M | Doc work, not DSP |

---

## V2 Priority Stack (recommended order)

```
Week 1 (mid next week):
  1. V011 — OTTONI LAKE delay time extension (50 lines, Schulze's mountain)
  2. V010 — OHM aftertouch → COMMUNE (Vangelis's expressivity unlock)
  3. V009 — OHM per-voice instrument pan (Tomita's stereo field)
  4. Constellation knowledge compendium (4 files × 4 engines = 16 files)

Week 2–3:
  5. OSTINATO scaffold (FamilyWaveguide-compatible, communal drum)
  6. OPENSKY + OCEANDEEP as paired build (feliX/Oscar polarity engines)
  7. AudioToBuffer Phase 3 implementation

Month 2:
  8. OUIE — duophonic STRIFE↔LOVE (most novel, highest complexity)
  9. OVERTONE — continued fractions oscillator
 10. ORGANISM — cellular automata engine

Ongoing:
  - Deferred doctrine improvements as capacity allows
  - KNOT coupling architecture (requires deep MegaCouplingMatrix work)
  - Guru Bin retreats on Constellation engines using completed knowledge compendium
```

---

## What V2 Is NOT

- A rebuild of V1. The 29 registered engines are complete (plus OVERLAP and OUTWIT pending installation).
- A feature-flag system. New engines ship when they're done.
- A V1 preset expansion. Preset expansion is continuous; it doesn't define version boundaries.
- A JUCE version upgrade. Stick with 8.0.x for the V2 cycle.

---

*See also:*
- `Docs/audio_to_buffer_phase3_spec.md` — AudioToBuffer implementation spec
- `~/.claude/skills/synth-seance/knowledge/visions/` — Full ghost vision files
- `~/.claude/projects/-Users-joshuacramblet/memory/theorem-2026-03-14.md` — Theorem results detail
- `Docs/xomnibus_engine_roadmap.md` — Updated engine integration status
