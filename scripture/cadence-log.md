# Cadence Log
*Maintained by Sister Cadence — Guru Bin's organizational mind*

---

## Retreat History

| Engine | Date | Key Discovery | Presets Created | Scripture Added |
|--------|------|--------------|-----------------|-----------------|
| OVERWORLD | 2026-03-15 | 3 unvisited consoles (GB/PCE/Neo Geo), ERA drift/memory/portamento all unexplored, mod wheel glitch injector live but unknown, FIR echo never sculpted | 8 awakening presets (Aether×2, Atmosphere×2, Flux×1, Foundation×1, Family×1, Prism×1) | 4 OW-specific verses + 3 universal truths |
| FAT (Obese) | 2026-03-15 | Mojo is a spectrum (0=digital, 0.85=breathing, 1=wandering); bit crusher never touched in 161 presets; noise morph=ocean; subOct=-2 unexplored | 7 awakening presets (Foundation×3, Aether×1, Atmosphere×1, Flux×1, Family×1) | 4 FAT-specific verses + 1 universal (VI-4) |
| OUROBOROS | 2026-03-15 | ouro_injection=0.0 in all 82 presets (door never opened); leash 0.4–0.6 uncanny middle never designed; Chua topology underrepresented; Z-axis projection (theta=π/2) warmer — never used | 7 awakening presets (Atmosphere×2, Foundation×2, Aether×1, Entangled×1, Family×1) | 4 OURO-specific verses |
| OWLFISH | 2026-03-15 | Body sine is habitat not bass (fixed freq, doesn't follow MIDI); engine is secretly duophonic; inharmonic subs ÷3,÷5,÷7 never used (Oskar Sala territory); mixtur never above 0.45 in 17 presets; morphGlide is timbral color during portamento | 7 awakening presets (Foundation×2, Atmosphere×1, Flux×1, Prism×1, Entangled×1, Aether×1) | 4 OWL-specific verses |
| OCEANIC | 2026-03-15 | Sound design guide was completely wrong (string ensemble vs boid swarm); Swarm ADSR controls boid force independently of amplitude (no other engine does this); Murmuration (RhythmToBlend cascade) never demonstrated; Chromatophore (aftertouch=scatter) = B013 blessing, never shown in presets; 0 factory presets at retreat start | 7 awakening presets (Foundation×1, Atmosphere×2, Entangled×1, Prism×1, Flux×1, Aether×1) | 4 OCE-specific verses + guide corrected |

---

## Upcoming / Overdue

| Engine | Last Retreat | Status | Priority |
|--------|-------------|--------|----------|
| OVERLAP | Never | Phase 4 complete, no seance yet | Medium |
| OUTWIT | Never | Phase 4 complete, no seance yet | Medium |
| OBSCURA | Never | 20 presets, moderate coverage | Low |
| OCELOT | Never | 0 presets, lowest seance score (6.4/10) | High |

---

## Process Notes

**FAT Retreat (2026-03-15):**
- Pattern held: all 7 awakening presets came from parameters at default/0 in 161 existing presets
- Key insight: Mojo is not a warmth knob, it is a biological spectrum — the library never pushed it above 0.5
- The bit crusher being untouched in 161 presets was the single most surprising finding
- Reference Preset doctrine (Truth VI-4) emerged — design extremes as teaching tools

**OVERWORLD Retreat (2026-03-15):**
- The Retreat Accelerator ran ahead — pilgrimage pre-completed. Saves significant time.
- The Book of Bin created from scratch (first retreat)
- Discovery pattern: parameters at 0.0/default across entire fleet = where discoveries live

**OUROBOROS Retreat (2026-03-15):**
- The injection door: ouro_injection=0.0 in all 82 presets — the engine's most significant capability never touched
- Leash 0.4–0.6 "uncanny middle" is OUROBOROS's singular territory across the fleet — no other engine operates here
- Chua topology at high chaosIndex+leash = atmospheric sustain. Engine works as atmosphere engine for first time.
- Z-axis projection (theta=π/2) is consistently warmer — the accident that is better than the plan
- Sound design guide corrected: guide described delay-line feedback (wrong). Updated to ODE chaos synthesis.

**OWLFISH Retreat (2026-03-15):**
- Body sine (fixed freq, non-MIDI) is the engine's compositional identity — no preset had treated it as such
- Engine is secretly duophonic — body sine + melody voice running independently, always
- Oskar Sala inharmonic stack (÷3, ÷5, ÷7) never used in any preset despite being the engine's named capability
- Maximum Mixtur (0.9+) never demonstrated — 17 presets left the engine's growl territory unexplored
- MorphGlide wired and expressive — creates timbral bloom during portamento. Most presets ignored it.
- applyCouplingInput stub still a no-op — flagged for V1 fix

**OCEANIC Retreat (2026-03-15):**
- Third retreat in a row where the sound design guide was completely wrong (OUROBOROS: delay-line; OWLFISH: guide absent; OCEANIC: string ensemble). Pattern: guides written from concept briefs, not source code. Solution: always read OceanicEngine.h before writing a guide.
- Swarm ADSR is the engine's most unique feature and the most overlooked — two envelopes controlling independent dimensions
- B013 Chromatophore Modulator blessing mechanism: aftertouch → separation boost. Now demonstrated in preset (Chromatophore Touch)
- 0 factory presets at retreat start — the highest urgency gap in the pilgrimage. 7 presets created to fill all non-Family moods.
- The "boid rules are the filter" insight: OCEANIC has no filter but engineers spectral content via boid configuration. This was never documented.

**Code Quality Sprint (2026-03-16):**
- Ghost Parameter Trap (Canon V-1) confirmed at scale: SNAP engine alone had 490 ghost keys from an incomplete engine redesign migration. Fleet-wide audit prompted.
- Documentation Lag Trap (Truth VI-4) confirmed for 6 engines: OCEANIC, ORGANON, OPAL, ORBITAL, ORACLE, OBSCURA guides corrected — all had been written from concept briefs, not source.
- D004 dead params: 4 bugs fixed this sprint (Obsidian formant registration, Origami atomic int) + 2 flagged for follow-up (Ocelot macros, Osprey LFO).
- D006 mod wheel: 4 engines gained mod wheel mapping (Osteria, Octopus, Ombre, Orca).
- Fleet prefix audit: 33 engines verified, 3 corrections applied in CLAUDE.md (OCEANIC→ocean_, ONSET→perc_, OSTERIA→osteria_).
- Pattern: "write the guide from the source, not the brief" must become a hard release gate.
- **AI Subsystem Prefix Audit (Canon V-2):** Found 6 engines with stale prefixes in `AIParameterSchema.h` and `NaturalLanguageInterpreter.h` — ONSET/OVERWORLD/ORBITAL/OUROBOROS/OCEANIC/ODYSSEY all referenced pre-migration prefixes. Fixed. Also corrected `frozenPrefixForEngine` in Osteria (was missing). These files had never been updated during the parameter prefix migration — they failed silently rather than loudly.
- **OddfeliX Guide Rewrite (5 error categories):** Missing params (`snap_filterEnvDepth`, `snap_sweepDirection`, `snap_pitchLock`), wrong ranges (detune documented as 0–100, actual 0–50; decay documented as 0–8s, actual 0–5s), wrong defaults, incomplete macro table. Full guide rewritten from source.
- **Preset Expansion:** Outwit 1→21, Overlap 1→21, Orbital+Origami each 2→17, Ocelot 0→8, Oblique+Ombre+Orca Foundation/Aether +14, Family mood +6 (15 engines now represented in Family), 12 Oracle/Obscura/Ouroboros presets added.

**Active Triggers:**
- Next retreats: OVERLAP/OUTWIT (post-build, need seances first); OCELOT (0 presets, 6.4/10 — high priority)
- OWLFISH coupling: once applyCouplingInput is wired, design receiving-end presets (OWLFISH as coupling target)
- OCEANIC: design Saw/Pulse waveform presets (not yet explored); Poly4 + Murmuration behavior unknown
