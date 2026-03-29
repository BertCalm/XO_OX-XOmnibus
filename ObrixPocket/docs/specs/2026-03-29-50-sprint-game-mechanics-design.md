# OBRIX Pocket — 50-Sprint Game Mechanics Design
## Blocks 13–22 (Sprints 61–110)
### Designed 2026-03-29 | Visionary × Feature Junkies × Phantom Circuit

## Overview

50 sprints extending OBRIX Pocket from its current state (60 sprints, ~16K lines, 61 Swift files) through three acts of game mechanics evolution. The existing 12 blocks built the nouns (specimens, wiring, catch, dive, journal). These 50 sprints build the verbs.

### Three-Act Structure
- **Act I (Blocks 13-15, Sprints 61-75): THE VOICE** — Fix the musical core, add player expression, make the Reef breathe
- **Act II (Blocks 16-19, Sprints 76-95): THE ECOLOGY** — Breeding, narrative, seasons, sound architecture
- **Act III (Blocks 20-22, Sprints 96-110): THE OMEGA REEF** — Observatory, social, endgame transcendence

### Design Principles (Phantom Circuit)
1. **The instrument IS the game** — every mechanic produces sound
2. **Progressive disclosure** — if the tutorial needs words, the design needs work
3. **Respect the player's taste** — expressive range, not forced output
4. **Engagement without addiction** — skill curves not reward schedules, no FOMO, no loot boxes
5. **Save-worthy output** — if the player makes something cool, let them keep it

---

## ACT I: THE VOICE (Blocks 13–15, Sprints 61–75)

### BLOCK 13: DIVE REDEMPTION (Sprints 61–65)
Theme: Fix the Dive from the ground up. Musical intelligence, harmonic structure, player agency.

**Sprint 61 — The Harmonic Engine**
- Build `HarmonicContext` — global system maintaining current key, scale, chord progression
- Chord progression generator: I→V→vi→IV and 8+ common progressions, weighted by specimen mood
- Each specimen assigned a musical role enum: `.bass`, `.melody`, `.harmony`, `.rhythm`, `.texture`, `.effect`
- Role determines octave range, note selection strategy, rhythmic density
- PC Pillar: Constraint — the system makes it hard to sound terrible

**Sprint 62 — Specimen Voice Identity**
- Each of 24 specimens gets a unique `VoiceProfile`: preferred scale degrees, rhythmic pattern, articulation style
- Sawfin: root + fifth, sustained, legato — anchor
- Bellcrab: arpeggiated thirds, staccato, syncopated — sparkle
- Curtain: doesn't play notes — sweeps filter in response to harmonic density
- Echovane: delays + processes other voices — spatial depth
- Voice profiles stack: 3 wired specimens = trio arrangement, not 3 random streams
- PC Pillar: Expressiveness — each specimen sounds like itself

**Sprint 63 — Arrangement Intelligence**
- `ArrangementEngine` manages density, tension, release across Dive timeline
- Intro (0-10s): one voice. Build (10-30s): voices layer. Peak (30-45s): full. Outro (45-60s): thin
- Wiring topology determines arrangement: chain = sequential build, parallel = simultaneous density
- Specimens respond to each other: melody adapts to bass voice's current chord
- PC Pillar: Flow — the Dive creates its own groove

**Sprint 64 — Player Expression in Dive**
- Touch zones: tap = accent, hold = sustain, swipe-up = intensity, swipe-down = pull back
- Tilt: filter sweep across all voices (gyroscope as expression)
- Player shapes arrangement, not notes — you're a conductor, not a pianist
- Call and response: tap a specimen zone, it plays a phrase, others respond harmonically
- PC Pillar: Expressiveness — player shapes music meaningfully

**Sprint 65 — Dive Scoring & Memory**
- Score based on: arrangement coherence, interaction timing, expression variety
- High-scoring Dives unlock "Dive Memories" — 15-sec audio clips saved to journal
- Dive Memory becomes specimen's "song" — it remembers its best performance
- Specimens with strong Dive history get subtle glow on the Reef
- PC Pillar: Consequence — every Dive leaves a mark

PC Review Gate 13: Does the Dive produce music the player would want to hear? Test with 5 different Reef configs.

---

### BLOCK 14: PLAYER VOICE (Sprints 66–70)
Theme: The player becomes a musician, not just a collector.

**Sprint 66 — The Play Surface Reimagined**
- Reef grid becomes performance surface: each wired specimen is a playable pad
- Tap = trigger with harmonic context, velocity-sensitive, hold = sustain with natural envelope

**Sprint 67 — Scale & Key Selection**
- Player chooses key (C–B) and scale (major, minor, pentatonic, blues, dorian, phrygian, mixolydian, whole tone, chromatic)
- All specimens locked to selected tonality — no wrong notes
- "Roaming key" mode: key shifts every 8 bars following chord progression
- Visual: reef water color shifts with key (warm = major, cool = minor)

**Sprint 68 — Macro Coupling Controls**
- 4 macro knobs: Density, Brightness, Space, Chaos (remappable)
- Long-press to remap to any coupling parameter
- Macros save per-Reef-configuration

**Sprint 69 — Specimen Sound Tweaking**
- Microscope edit mode: adjust parameters ±20% from default
- Tweaks persist as "mutation" marker on specimen card
- Mutated specimens sound unique in Dives

**Sprint 70 — Performance Recording**
- Record tap events, macro movements, tilt gestures as MIDI-like event stream
- Export as: M4A audio, MIDI file, .xodive replay
- 15-sec social clip or full recording

PC Review Gate 14: Can two players with the same Reef produce different music?

---

### BLOCK 15: THE LIVING REEF (Sprints 71–75)
Theme: The Reef becomes a breathing, evolving ecosystem.

**Sprint 71 — Reef Heartbeat**
- Always-on ambient generative loop based on population
- Density affects tempo, coupling chains create harmonic relationships
- Sounds different every app open

**Sprint 72 — Specimen Aging & Maturation**
- 4 life stages: Spawn (1-3d), Juvenile (3-14d), Adult (14-60d), Elder (60+d)
- Each stage shifts voice: Spawn = simple/pure, Elder = complex/harmonically rich
- Elders teach younger specimens through wiring

**Sprint 73 — Reef Chemistry**
- Affinity and tension relationships beyond 8 coupling pairs
- Grid placement matters: closer = stronger interaction
- Tension ≠ bad — it's dissonance. Player chooses consonance vs dissonance.

**Sprint 74 — Weather System**
- 24-hour real-time cycle: morning bright/major, evening warm/subdued, night deep/minor
- Rare events: Bioluminescent Storm, Deep Current, Solar Tide
- Events NOT announced — discovered by noticing sound changes

**Sprint 75 — Reef Snapshot & Compare**
- Named snapshots: "My Jazz Reef", "Ambient Morning"
- A/B compare mode
- Includes: layout, coupling, mutations, macros, scale/key

PC Review Gate 15: Session respect test — 2 min and 2 hours both feel complete.

---

## ACT II: THE ECOLOGY (Blocks 16–19, Sprints 76–95)

### BLOCK 16: BREEDING & GENETICS (Sprints 76–80)
Theme: Coupling creates new life.

**Sprint 76 — Coupling Crucible**
- High-affinity specimens wired 7+ days can breed
- Offspring inherits blended traits from both parents
- Limited: one breeding per pair per 14 days

**Sprint 77 — Trait Inheritance System**
- 12 inheritable sonic traits with dominant/recessive expression (Mendelian genetics for synths)
- Traits: octave range, waveform affinity, rhythmic density, harmonic preference, envelope shape, filter tendency, modulation depth, vibrato speed, stereo width, reverb affinity, attack character, sustain behavior

**Sprint 78 — Generational Memory**
- Gen-1 (caught), Gen-2 (bred), Gen-3+
- Higher generations: wider mutation windows (20% → 30% → 40%)
- Gen-5+ specimens profoundly unique

**Sprint 79 — Breeding Predictions & Discovery**
- Probabilistic predictions before breeding
- 5% rare mutation events
- Discovery log tracks unique trait combinations

**Sprint 80 — The Nursery**
- 24-hour formation period in separate zone
- Player influences offspring by playing music near Nursery
- After formation, moves to main Reef

PC Review Gate 16: Are breeding waits meaningful (nursery interaction) or arbitrary gates?

---

### BLOCK 17: THE DEEP CURRENT — NARRATIVE (Sprints 81–85)
Theme: Story emerges from the deep.

**Sprint 81 — The Tide Log**
- Auto-generated narrative journal in water-column mythology voice
- Milestone entries at achievement points

**Sprint 82 — Deep Specimens: Expedition Mode**
- 8 Deep Specimens require multi-Dive expedition challenges (3 Dives, escalating)
- Each requires specific Reef composition + conditions to attract
- Earned through mastery, not luck

**Sprint 83 — The Reef Memory Wall**
- Permanent display of greatest MOMENTS (not trophies) — with audio clips
- Syncs to desktop via Reef Link

**Sprint 84 — Curiosity Pings**
- Subtle in-Reef anomalies that reward investigation
- Not notifications — discovered by looking at the Reef

**Sprint 85 — The Deep Lore**
- 24 mythology fragments across Deep Specimens + events
- Story told through sound and brief poetic fragments — never exposition

PC Review Gate 17: Day 1 vs Day 30 — are they discovering different things?

---

### BLOCK 18: TIDES & SEASONS (Sprints 86–90)
Theme: Time as design material.

**Sprint 86 — Tidal Cycles**
- 4-hour cycle: Low Tide → Rising → High Tide → Falling
- Low Tide reveals hidden things, High Tide = peak performance

**Sprint 87 — Seasonal Migration**
- 28-day cycle (7 per season): Spring (fertile), Summer (peak), Autumn (rich), Winter (rare/contemplative)

**Sprint 88 — Migratory Specimens**
- 4 season-specific specimens visit 7 days each
- Catching converts migratory→resident (different voice — trade-off)

**Sprint 89 — Event Tides**
- 1-2 per season, 48 hours each: Deep Current, Bioluminescent Bloom, The Convergence
- NOT announced — discovered through Reef observation

**Sprint 90 — Seasonal Archive**
- Auto-generated Season Cards: stats + 30-sec audio compilation
- Collectible timeline of musical evolution

PC Review Gate 18: 7 consecutive days each feel different without player effort.

---

### BLOCK 19: SOUND ARCHITECTURE (Sprints 91–95)
Theme: From conductor to architect.

**Sprint 91 — The Sequencer Grid**
- 16-step sequencer, each row a specimen
- Steps inherit harmonic context — always the right note

**Sprint 92 — Pattern Memory**
- Save patterns as "Reef Patterns", chain into song structure

**Sprint 93 — Arrangement View**
- Timeline view for full song structure
- Dive becomes any length (30s to 10min) based on arrangement

**Sprint 94 — Mix Console**
- Per-specimen: volume, pan, send. Master: compressor, limiter, width.

**Sprint 95 — Composition Export**
- WAV/M4A, MIDI, .xoreef with patterns
- Stems mode: per-specimen audio export

PC Review Gate 19: Is the GAME still present or has it become a DAW? Catching/breeding/wiring must remain primary path to sound design.

---

## ACT III: THE OMEGA REEF (Blocks 20–22, Sprints 96–110)

### BLOCK 20: THE OBSERVATORY (Sprints 96–100)
Theme: See and understand your musical self.

**Sprint 96 — Spectral Reef View**: Frequency spectrum visualization showing gaps and overlaps
**Sprint 97 — Musical DNA Profile**: Player history analysis — genre, scale, coupling preferences
**Sprint 98 — Theory Tutor (Implicit)**: Theory surfaces through experience, not instruction
**Sprint 99 — Listening History Integration**: Opt-in Apple Music/Spotify (on-device only) taste learning
**Sprint 100 — The Centennial Dive**: Special 5-min Dive using ALL specimens ever caught — "Your Opus"

PC Review Gate 20: Day 100+ Dives still surprising? Novelty audit.

---

### BLOCK 21: CROSS-REEF SOCIAL (Sprints 101–105)
Theme: Reefs connect through music, not metrics.

**Sprint 101 — Reef Postcards**: 15-sec audio + visual, "Message in a Bottle" public feed
**Sprint 102 — Specimen Gifting**: Gift bred specimens with full journal — generosity, not economy
**Sprint 103 — Collaborative Dive**: Two players merge Reefs for real-time jam session
**Sprint 104 — Reef Radio**: Opt-in ambient broadcast — infinite generative radio of player Reefs
**Sprint 105 — The Chorus**: Monthly community composition from all active Reefs

PC Review Gate 21: No follower counts, likes, or leaderboards. Social = creation, not competition.

---

### BLOCK 22: THE OMEGA REEF (Sprints 106–110)
Theme: The endgame beyond the endgame.

**Sprint 106 — Reef Ecology Simulation**: Population dynamics, resource competition, ecology score
**Sprint 107 — Specimen Autonomy**: Specimens develop autonomous behavior, self-wiring, self-composing
**Sprint 108 — Reef Legacy**: Seed new Reefs from old ones, Legacy mode preserves musical eras
**Sprint 109 — The Abyss**: Hidden deep zone for Gen-4+ specimens, sub-bass/spectral content
**Sprint 110 — The Omega Dive**: Unlimited Dive using ALL Reefs + Abyss. No score. No timer. Musical autobiography.

PC Review Gate 22 (Final): Full-game audit across 5 pillars — Expressiveness, Consequence, Constraint, Discovery, Flow.

---

## Implementation Priority

### Phase 1 (Immediate): Sprints 61-75, Blocks 13-15
The musical core. Fix the Dive, add player expression, make the Reef breathe.

### Phase 2 (After Phase 1 stable): Sprints 76-95, Blocks 16-19
Ecology and depth. Depends on Voice Profile system from Sprint 62.

### Phase 3 (Mature player base): Sprints 96-110, Blocks 20-22
Social, observatory, endgame. Needs network infrastructure and months of player history.

### Critical Path
Sprint 61 (Harmonic Engine) → Sprint 62 (Voice Identity) → Sprint 63 (Arrangement Intelligence) — this is the load-bearing wall. If the Harmonic Engine doesn't produce good music, everything downstream collapses.

---

## Unverified Assumptions
- Apple Music/Spotify API allows on-device-only listening history (Sprint 99)
- Local network collaborative Dive latency acceptable for real-time jam (Sprint 103)
- Generative arrangement intelligence produces non-repetitive output at Day 100+ (Sprint 100)

## Reviewed By
- Phantom Circuit Interactive — 5-pillar game design review at each block boundary
- The Visionary — escalation cascade, Omega Point architecture
- Feature Junkies — playability, customization, sound design, utility, cross-platform audit
