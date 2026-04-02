# The MPC Producer's XO_OX Field Manual
## Complete Outline — Community eBook

**Author team**: Rex (format), Kai (MPC workflows), Vibe (sound design), Scout (research)
**Version**: Outline v1.0 — 2026-03-16
**Target length**: ~60,000 words
**Format**: PDF + EPUB; visual-first layout with sidebars, callout boxes, code/XML samples
**Target audience**: MPC owners at the intermediate-to-advanced level who want to use XO_OX expansions expressively, build their own packs, and understand the format deeply. Assumes familiarity with MPC hardware but not with XPN internals.

---

## Introduction: Why This Manual Exists

~800 words

The XPN format is sparsely documented. The MPC manual covers hardware brilliantly but barely touches the expansion pack ecosystem. And XOceanus — the deepest sound-design environment on the platform — ships with no MPC integration guide at all.

This manual captures three years of R&D from XO_OX's Oxport sessions. It is the canonical reference for:

- How XPN packs are designed, built, and shipped
- How XOceanus engines translate into MPC programs
- How coupling, DNA, and velocity expressivity survive the export
- How to design your own packs using the same tools XO_OX uses

The manual is organized as a journey: from format fundamentals → workflow → expressivity → collection philosophy → tools → advanced design → visual identity → community.

**Suggested visual**: Full-width spread of the MPC Live III with XO_OX ONSET pack loaded, pads glowing.

---

## Part I: The Foundation

### Chapter 1: The XPN Format — A Deep Dive
**Estimated length**: ~8,000 words
**Primary author voice**: Rex (XPN Specialist)

#### 1.1 What XPN Actually Is
- ZIP archive structure: `manifest.json`, `.xpm` files, `/Samples/`, `/Artwork/`
- Relationship between the manifest and individual XPM program files
- How MPC hardware parses and caches XPN content
- Visual: annotated folder tree diagram of a complete XPN bundle

#### 1.2 The XPM Schema Explained
- `<Program>` root: `programVersion`, `engineType` (KeyGroup, Drum, Plugin, CV)
- `<Keygroup>` anatomy: `rootNote`, `lowNote`, `highNote`, key tracking, transpose
- `<Layer>` elements: `velStart`, `velEnd`, `sample`, `loopStart`, `loopEnd`, `loopActive`
- `<Envelope>` elements: ADSR per layer with modulation routing
- `<LFO1>`, `<LFO2>`, `<LFO3>` structure (3.0+)
- `<FX>` chain: 4 insert slots (3.0+), send/return architecture
- `<QLinks>` block: 16 assignments (3.0+), `modulationTarget`, `bipolar`, `smoothing`
- Sidebox: "The 3 Golden Rules" (KeyTrack = True, RootNote = 0, VelStart = 0)

#### 1.3 Drum Programs vs KeyGroup Programs
- When to use each: pitched instruments vs percussion
- Pad assignment: `<PadNoteMap>`, `<PadGroupMap>`
- `CycleType` values: velocity, cycle, random, random-norepeat, smart, velocity-cycle
- `CycleGroup` / `CycleKey` for round-robin grouping
- How ONSET drum voices map to drum programs
- Visual: side-by-side XML excerpts for drum vs keygroup program

#### 1.4 Velocity Layers — The Expressive Core
- Layer stack design: 4-layer vs 8-layer vs 16-layer strategies
- Velocity crossfade (`<VelocityXfade>`): hard split vs blended transitions
- How velocity should shape timbre, not just amplitude (Doctrine D001)
- Sonic DNA velocity curves and how they translate to XPM layer stacking
- Visual: velocity curve graphs showing different DNA profiles

#### 1.5 The Manifest in Depth
- All standard fields: name, vendor, version, description, tags, programs, samples
- 3.0+ additions: schemaVersion, engineRequirements, targetHardware, sampleRate, previewAudio
- AI fields: rootNoteDetected, loopPointsDetected — when to set vs let MPC infer
- XO_OX additions: genreTags, moodTags, HardwareProfile, StemsReady, PadCornerMapping
- Full annotated manifest example (Kitchen Essentials pack)

#### 1.6 XPN Evolution: 2.x vs 3.0 vs MPCe
- Format compatibility: 2.x loads in 3.0 with shim; new fields silently ignored on old hardware
- MPC 3.0 additions recap (LFO3, 4-slot FX, 16 Q-Links, programVersion)
- MPCe quad-pad architecture: `<PadCornerAssignment>` (future-watch)
- MPCe XY modulation targets: design intent even before schema is documented
- Table: Field support matrix by firmware version

#### 1.7 Common XPM Pitfalls and How to Avoid Them
- Ghost triggering: empty layer `velStart` set to non-zero
- Samples not transposing: `KeyTrack` = False
- Root note mismatch: explicit rootNote vs MPC AI inference conflict
- Samples missing from bundle: relative path errors in manifest
- Q-Link labels too long for OLED display
- Sidebox: "The Error Museum" — curated list of worst bugs and their fixes

---

### Chapter 2: XOceanus → MPC Workflow
**Estimated length**: ~6,000 words
**Primary author voice**: Kai

#### 2.1 The Export Pipeline Overview
- XOceanus engine → Oxport render → XPM generation → XPN bundle → MPC hardware
- Where each tool lives in the pipeline: 10 Python tools in `/Tools/`
- Philosophy: offline rendering is better than auto-sampling (consistency, sample rate accuracy)
- Visual: pipeline flowchart from plugin to MPC pad

#### 2.2 Rendering Samples from XOceanus Engines
- `OfflineAudioContext` rendering: why it matters and how it works
- Sample rate matching: derive from source, never hardcode 44100
- Envelope matching: offline render must use same ADSR math as live engine
- Bit depth: 24-bit WAV for XPN (not 16-bit, not 32-bit float for compatibility)
- Visual: waveform comparison showing offline vs live capture artifacts

#### 2.3 Key Range and Root Note Strategy
- Full 10-octave coverage: C-2 to G8, rendering at semitone or third intervals
- Stretch zones: how far from a rendered root can you stretch before artefacts?
- Rule of thumb: ±5 semitones for most timbres, ±3 for formant-sensitive content
- Root note annotation in filenames: `C3`, `A#4`, `Gb2` conventions
- Multi-sample reduction: when 12 roots per octave vs 4 roots per octave is appropriate

#### 2.4 Velocity Rendering — Mapping Sonic DNA to Layers
- 4-layer minimum, 8-layer recommended for expressive engines
- Sonic DNA aggression axis as velocity sensitivity guide
- How engines with velocity-shaped filter envelopes (D001 compliant) need 8+ layers to capture the sweep
- The `velocity-cycle` hybrid: rendering velocity zones AND round-robin variants simultaneously

#### 2.5 From XOceanus Presets to MPC Programs
- Loading a preset, printing the sound, mapping to keygroup zones
- Multi-articulation: sustain / staccato / release layers as `<ArticulationGroup>`
- Coupling snapshots: if two engines are coupled in XOceanus, how to capture the coupled timbre in a standalone XPN program
- Limitations: what doesn't transfer (real-time coupling modulation, live macro movement)

#### 2.6 Drum Kit Export (ONSET Programs)
- ONSET's 8 voices → 16 drum pads strategy
- Per-voice physical defaults (kick: center, snare: snappy, hat: crisp)
- `CycleGroup` assignment for round-robin within each voice
- Q-Link assignment: MACHINE/PUNCH/SPACE/MUTATE macros → pad Q-Links 1–4
- Velocity curve Vibe's musical velocity standard (reviewed in Guru Bin session)
- Visual: pad layout diagram showing voice-to-pad mapping

#### 2.7 Pack Testing on Hardware
- Test on MPC hardware, not just software (subtle differences exist)
- Live II testing for compatibility; Live III for MPCe feature validation
- Checklist: sample loading speed, loop point accuracy, velocity response, Q-Link response
- Common hardware-only failures: sample rate mismatch, long file paths, embedded cover art size

---

## Part II: Expression and Intelligence

### Chapter 3: Coupling Recipes — Recreating Live Coupling in MPC
**Estimated length**: ~5,000 words
**Primary author voice**: Kai + community contributors

#### 3.1 What Coupling Is (and Isn't in MPC)
- XOceanus coupling: real-time cross-engine parameter modulation
- MPC's equivalent: Q-Links, send/return FX chains, MIDI routing between programs
- What can be approximated: modulation depth, routing topology
- What cannot: bi-directional live feedback coupling (KNOT topology especially)

#### 3.2 The 5 Most Translatable Coupling Types
For each: description, XOceanus coupling name, MPC approximation method, Q-Link assignments, limitations

1. **Filter drive coupling** (OVERDUB → OPAL) — send level as modulation depth
2. **Oscillator lock** (SNAP → OVERWORLD) — detuned unison approximation via layer pitch offset
3. **Envelope sharing** (ONSET → OBBLIGATO) — velocity-to-send routing
4. **Spectral tilt** (OPTIC → OBSCURA) — Q-Link controlling EQ tilt across both programs
5. **Space coupling** (OPAL → DUB) — reverb send chain with shared return bus

#### 3.3 Building a "Coupling Stack" in MPC
- Track template: Program A + Program B + FX Return bus in one project
- MIDI routing: one keyboard → two programs simultaneously
- Q-Link bank assignment: Bank A = Program A controls, Bank B = Program B controls, Bank C = coupling balance
- Session template file: download and load for instant coupled workflow

#### 3.4 Recipe Cards: 10 Documented Coupling Setups
Format: Name | Engines involved | MPC track template | Q-Link map | Sound description

1. DRIFT → OPAL: wavetable shimmer into granular haze
2. OVERDUB → OHM: dub tape echo into communal drone
3. ONSET → ORACLE: percussion into generative melodic response
4. OBESE → OBBLIGATO: saturated bass into wind harmony
5. ORPHICA → OLE: microsound harp into Afro-Latin percussion
6. ORGANON → OCEANIC: biological metabolism meets phosphorescent tide
7. OUROBOROS → OVERWORLD: chaotic strange attractor into chip-era nostalgia
8. OBLIQUE → OPTIC: prismatic bounce feeding visual modulation feedback
9. OTTONI → OMBRE: brass swell into dual-narrative shadow
10. ORCA → OCTOPUS: apex predator hunting into decentralized arm intelligence

#### 3.5 MPCe Expressivity in Coupled Programs
- Assigning MPCe X/Y axes to coupling depth parameters
- Example: Y-axis controls how much OPAL granularity bleeds into OVERDUB's tape delay
- Performance notation: how to document these setups for live performance recall

---

### Chapter 4: DNA-Adaptive Velocity — Sonic DNA for Expressive Patches
**Estimated length**: ~4,500 words
**Primary author voice**: Vibe + Rex

#### 4.1 The 6D Sonic DNA Explained
- The six axes: brightness, warmth, movement, density, space, aggression
- How DNA values are set in `.xometa` presets
- What DNA is for: catalog discovery, sound design communication, velocity mapping guidance

#### 4.2 DNA → Velocity Layer Architecture
- Aggression axis as primary velocity sensitivity driver
- Brightness axis as filter envelope shape guide
- Movement axis as LFO rate influence on velocity layers
- Space axis as reverb-send velocity scaling
- Worked example: a high-aggression, high-brightness preset → steep 8-layer velocity stack with high filter envelope amount

#### 4.3 Velocity Curves Per DNA Profile
- Low aggression: gentle logarithmic curve, 4 layers, subtle filter sweep
- Medium aggression: linear with crossfade, 6 layers, moderate timbre change
- High aggression: exponential, 8 layers, dramatic filter + harmonic shift
- Maximum aggression (OBLONG, OBESE extremes): 16 layers, full character transformation
- Visual: velocity curve family showing all four profiles side by side

#### 4.4 The "Invisible Intelligence" Design Standard
- Guru Bin's demand from the XPN Seance: "the intelligence must be invisible — the player feels it, not sees it"
- What this means for velocity design: no audible velocity thresholds, no clicks between layers
- Crossfade implementation, normalization matching across layers, attack time consistency
- Sidebox: "Playing Blindfolded" — how to test whether velocity intelligence is truly invisible

#### 4.5 Case Studies: 3 Engines at Full Velocity Range
- ONSET (percussion): how 8-voice physical model velocity layers are designed
- OPAL (granular): grain density, scatter, and envelope as velocity targets
- OBESE (saturation): Mojo axis behavior across 16 velocity steps

#### 4.6 Building Responsive Patches Yourself
- Choosing velocity targets before rendering: what parameter shows the most timbral difference?
- The "velocity delta test": render at v1 and v127, compare spectrograms
- Rule: if the two extreme velocities sound more than 50% different, you have a great 8-layer candidate
- Rule: if they sound identical, you have a Q-Link problem, not a velocity problem

---

## Part III: Collections and Philosophy

### Chapter 5: Collection Philosophy — Kitchen, Travel, and Artwork Expansions
**Estimated length**: ~5,000 words
**Primary author voice**: Scout + Barry

#### 5.1 Why Collections?
- Character instruments need context — a single synth pack is a tool, a collection is a world
- The Kitchen/Travel/Artwork triad: three lenses on the same engine catalog
- How collection philosophy affects pack design decisions: what samples to render, which macros to expose

#### 5.2 Kitchen Essentials Collection
- Architecture: 6 quads × 4 engines = 24 engines + Fusion 5th slot
- Voice × FX Recipe × Wildcard (boutique synth company) design logic
- MPC Stems connection: "cooking with stems" — full-mix loops designed for ingredient separation
- Featured pack concept: "Mise en Place" — 16 drum programs, each a complete kitchen sound design
- Q-Link design standard: HEAT, SEASON, PLATE, SERVE (temperature/flavor/texture/presentation)
- Cover art direction: close-up food photography with engine accent colors as sauce/spice

#### 5.3 Travel / Water / Vessels Collection
- Architecture: 4 sets × 4 engines + Sable Island fusion = 20 engines
- Sail/Industrial/Leisure/Historical design logic
- Era-based sound design: sail era = acoustic transients, industrial = machinery textures, leisure = tropical synthesis, historical = period instruments
- MPC XL CV/Gate resonance: "Vessel Collection" for modular users using MPC XL as their ship
- Sidebox: Sable Island 5th slot — fusion engine design philosophy

#### 5.4 Artwork / Color Collection
- Architecture: 5 quads × 4 engines + Arcade fusion = 24 engines
- Voice (art styles) × FX (color science/water physics) × Wildcard (complementary color) design logic
- Color Quad A (Oxblood/Onyx/Ochre/Orchid): Erhu/Didgeridoo/Oud/Guzheng
- Color Quad B (Ottanio/Oma'oma'o/Ostrum/Ōni): Handpan/Angklung/Sitar/Shamisen
- MPCe XY axis as "2D canvas navigation": Y = brightness, X = warmth, physical metaphor for painting
- Historical and cultural content as field guide / zine opportunity
- Rich story content: blog posts, artist biography, cultural context

#### 5.5 How Collection Philosophy Changes Pack Production
- Design loops in groups of four that tell a story arc
- Name programs with collection context: "OCHRE/Dusk Study" not "Strings Pad 7"
- Cover art coherence: all 24 packs in a collection should be recognizable as a family
- Release cadence: quarterly collection drops vs random individual packs

---

## Part IV: The Tools

### Chapter 6: Oxport Tool Suite Walkthrough
**Estimated length**: ~8,000 words
**Primary author voice**: Rex + Kai

#### 6.1 Oxport Philosophy
- Why Python: portability, MPC community scripting culture, readable/forkable by community
- The 10-tool architecture: each tool does one thing well
- No hard dependencies between tools: any tool can be run in isolation

#### 6.2 Tool 1: Sample Categorizer
- Input: raw WAV folder (from XOceanus renders)
- Output: categorized folder structure with naming conventions
- DNA integration: reads `.xometa` preset to auto-tag samples
- Command-line usage, flag reference

#### 6.3 Tool 2: Drum Export (`generate_drum_program.py`)
- Input: categorized sample set + kit configuration JSON
- Output: XPM drum program file
- Kit modes: velocity, cycle, random, random-norepeat, smart
- `CycleType` selection guide
- PadNoteMap and PadGroupMap generation
- Q-Link assignment: 4 macros mapped to pads 13–16
- Configuration JSON schema with annotated example

#### 6.4 Tool 3: Keygroup Export (`generate_keygroup_program.py`)
- Input: multi-sample root note folder + configuration JSON
- Output: XPM keygroup program file
- Auto-zone calculation from filename root notes
- Layer stack generation (4/8/16 layer strategies)
- `ArticulationGroup` support for multi-articulation exports
- LFO stub generation (LFO1/2/3)
- 4-slot FX chain stub generation

#### 6.5 Tool 4: Kit Expander (`expand_kit.py`)
- Input: seed kit + variation parameters
- Output: N variant kits (e.g., 8 flavors of a snare kit)
- Variation axes: pitch, decay, velocity curve, layer shuffle
- Use case: generating collection variants for a Karma Seance or full-collection release

#### 6.6 Tool 5: Bundle Builder (`build_xpn.py`)
- Input: folder of XPM files + samples + manifest.json
- Output: `.xpn` ZIP archive
- Manifest validation: schema check before bundling
- Sample integrity check: all referenced samples exist
- Cover art embedding vs path reference decision logic
- Preview audio inclusion (3.0+ `previewAudio` field)

#### 6.7 Tool 6: Cover Art Generator
- Input: engine accent color + pack name + optional background image
- Output: 1200×1200 master PNG + 1000×1000 XPN embed version
- Color field design: accent color as primary, warm white `#F8F6F3` as secondary
- Typography: Space Grotesk for display text
- Template variations: solid field, gradient, layered transparency
- Sidebox: "Cover Art as Identity" — how XO_OX covers are recognizable at thumbnail scale

#### 6.8 Tool 7: Packager (`package_release.py`)
- Input: validated XPN bundle + release metadata
- Output: release ZIP with readme, changelog, install instructions
- Version tagging: semver for pack versions (`1.0.0`, not dates)
- Checksum generation for integrity verification

#### 6.9 Tool 8: Render Spec Generator
- Input: XOceanus engine name + preset name
- Output: render specification JSON (sample rate, note range, velocity steps, loop requirements)
- Integration with Oxport pipeline as the front-end specification step

#### 6.10 Tools 9 and 10: XPN Manifest Validator + Batch Tester
- Manifest validator: schema compliance, field completeness, hardware profile consistency
- Batch tester: load XPN into MPC Beats (desktop) programmatically, export pass/fail report
- CI integration: how to run these in a GitHub Actions workflow

#### 6.11 Running the Full Pipeline End-to-End
- Step-by-step walkthrough: ONSET → ONSET_Pack_v1.xpn in 12 steps
- Where human judgment is required vs automated
- Estimated time per pack type: drum (2 hours), keygroup melodic (4 hours), full collection pack (1 day)

---

## Part V: Advanced Design

### Chapter 7: Advanced Keygroup Design
**Estimated length**: ~6,000 words
**Primary author voice**: Rex + Vibe

#### 7.1 Multi-Articulation Keygroups
- What multi-articulation means: sustain, staccato, pizzicato, tremolo, harmonics — on one key
- The `<ArticulationGroup>` schema (3.0+)
- Hardware trigger options: pad velocity zone, note-off, mod wheel CC
- Design workflow: render each articulation separately, combine in Oxport

#### 7.2 Round-Robin Design
- Why round-robin matters: the "machine gun" problem in repetitive note triggering
- 4-cycle vs 8-cycle round-robin: minimum for realism, optimal for naturalness
- `CycleGroup` + `CycleKey` assignment for per-voice grouping
- `velocity-cycle` hybrid: velocity layers × round-robin cycles simultaneously
- Storage calculation: 8 velocity × 8 round-robin × 88 notes = 5,632 samples — when to stop

#### 7.3 Release Layers
- Mute-group triggered release samples: the technique that separates great instrument libraries from mediocre ones
- XPM mute group assignment for release triggering
- Designing acoustic release transients for XOceanus engines (string noise, key click, mechanical resonance)
- Automation: Oxport's release layer generator utility

#### 7.4 Loop Point Design
- Finding clean loop points: zero-crossing, spectral matching, silence bridging
- Loop crossfade: the invisible join
- AI loop detection (MPC 3.0 `loopDetect` flag) vs manually authored loop points
- When not to loop: transient-only content, rhythmic samples
- Sidebox: "The Perfect Loop" — the geometry of a sustained tone that breathes

#### 7.5 Polyphonic Aftertouch Preparation (MPCe Future-Proofing)
- MPCe poly AT is coming: stub `<AftertouchTarget>` per layer now
- Recommended targets: filter resonance, vibrato depth, harmonic tilt
- Design philosophy: AT should reveal a secondary character, not just increase volume

#### 7.6 Advanced Q-Link Strategy
- 16 Q-Links: how to use all of them thoughtfully
- Four banks: Filter controls (Q1–4), Envelope controls (Q5–8), FX controls (Q9–12), Macro controls (Q13–16)
- Bipolar vs unipolar assignment: when each is appropriate
- `smoothing` values: 0ms (switches), 20ms (filters), 40ms (pitch), 80ms (reverb tails)
- OLED naming conventions: 8-character limit, abbreviation standard
- Performance ergonomics: most-used controls on Q1–4 (leftmost), least-used on Q13–16

---

### Chapter 8: Cover Art and Visual Identity
**Estimated length**: ~4,000 words
**Primary author voice**: HT Ammell / Fab Five design team voice

#### 8.1 Why Cover Art Is a First-Class Feature
- In MPC's pack browser, cover art is the first (and sometimes only) impression
- Inconsistent art = lost sales at the thumbnail stage
- XO_OX's gallery model: warm white shell + engine accent color = instant brand recognition

#### 8.2 The Gallery Model for Pack Art
- Design system: `#F8F6F3` warm white as background, engine accent as primary color
- Typography: Space Grotesk display, weight 700, short names only
- Hierarchy: Engine name (large) > Pack name (medium) > XO_OX logo (small, always present)
- Visual: template showing all three hierarchy levels

#### 8.3 Cover Art Specifications (Current and Future-Proofed)
- Master: 1200×1200 PNG, sRGB, no ICC profile embedded
- XPN embed: 1000×1000 PNG, downscaled from master with Lanczos filter
- Thumbnail: 200×200 auto-generated from master (never hand-crafted separately)
- MPCe-ready: 1200×1200 master is sufficient for current and next-gen hardware
- Sidebar: "Why not 2000×2000" — file size vs display benefit analysis

#### 8.4 Collection Cover Art — Variant Systems
- All packs in a collection must be recognizable as a family at 200×200 thumbnail scale
- Technique: consistent background treatment, varying accent color and icon
- Kitchen collection: close-up texture photography as background
- Travel collection: water surface photography as background
- Artwork collection: painted surface / canvas texture as background
- Visual: 4-pack thumbnail comparison showing family coherence

#### 8.5 The XO_OX Brand Constants in Pack Art
- XO Gold `#E9C46A` as secondary accent (never as primary — that belongs to the engine)
- The XO_OX logomark: placement, minimum size, clear space rules
- Font licensing: Space Grotesk is OFL — free for commercial use
- Sidebox: "Do Not Violate" — list of design decisions that are non-negotiable

#### 8.6 Automated Cover Art Generation
- Oxport's cover art tool: inputs, outputs, template system
- When to use automation vs manual design: automated for standard library packs, manual for hero/featured packs
- Batch generation for 24-pack collections: time estimate, quality gate review process

---

## Part VI: Community and Future

### Chapter 9: Community Contributions — Building Your Own Packs
**Estimated length**: ~5,000 words
**Primary author voice**: Barry OB / Scout

#### 9.1 Who This Chapter Is For
- Intermediate MPC producers who want to create commercial-quality packs
- Sound designers who already have sample libraries and want to package them
- Community members who have developed original XOceanus presets and want to export them

#### 9.2 The XO_OX Community Contribution Model
- How community packs differ from official XO_OX packs
- Review process: Community Curator (quality gate) + Guru Bin (soul review) + Barry's distribution check
- Credit and attribution: community creator name in manifest, XO_OX co-branding guidelines
- Revenue sharing model (if applicable): Patreon / Gumroad split

#### 9.3 Your First Pack — A 12-Step Project
1. Choose an engine and a concept
2. Select 4–8 representative presets
3. Define render spec (note range, velocity steps, articulations)
4. Run Oxport render spec generator
5. Render samples (offline or via auto-sampling)
6. Run sample categorizer
7. Configure and run drum or keygroup export tool
8. Generate manifest
9. Add Q-Link assignments and review naming
10. Generate cover art
11. Run bundle builder
12. Validate with XPN manifest validator + MPC Beats test load

#### 9.4 Quality Standards for Community Packs
- Minimum sample count per program (no shortcuts)
- Velocity layer requirements by Sonic DNA profile
- Required Q-Link assignments (at least 4 meaningful targets)
- Cover art requirements (must use XO_OX community template)
- Documentation requirement: 200-word pack description + Q-Link map table

#### 9.5 Community Samples and Field Recordings
- Field recording guidelines: 24-bit/96kHz minimum, clean gain staging, no room noise
- Cultural and contextual sourcing: Travel and Artwork collections need authentic regional material
- Credits and rights: how to handle samples from community members
- The XO_OX sample commons: shared repository for approved community samples

#### 9.6 Publishing and Distribution
- XO_OX Patreon tier structure: free tier vs Patron-exclusive packs
- Gumroad for one-time purchase packs
- Future: MPC hardware in-device store (speculative but architectural planning)
- Social proof workflow: preview audio → cover art → community Discord post → release

#### 9.7 The Ongoing Conversation
- Discord community channels for pack feedback
- Field Guide blog series: community members can pitch articles
- "Pack of the Month" showcase
- Seance sessions for community-built engines (open invitation)

---

## Appendices

### Appendix A: Complete XPM Field Reference
**Format**: Table with columns: Field Name | Parent Element | Type | Required | Default | Description | Firmware Version Added

Full coverage of every XPM field documented in this manual and the extended 3.0 schema. ~4,000 words equivalent in table form.

Sections:
- Program root attributes
- Keygroup fields
- Layer fields
- Envelope fields
- LFO fields (1, 2, 3)
- FX chain fields (inserts 1–4, sends)
- Q-Link assignment fields
- HardwareProfile fields (XO_OX extension)
- Drum program fields
- PadNoteMap / PadGroupMap fields
- Mute group fields

---

### Appendix B: Golden Rules Checklist
One-page quick reference — suitable for printing and posting above your studio monitor.

```
XPN PRODUCTION — GOLDEN RULES

FORMAT
□ programVersion = "3.0"
□ KeyTrack = "True" on all keygroup programs
□ RootNote = "0" (MPC auto-detect convention)
□ Empty layer VelStart = "0"
□ All referenced samples exist in /Samples/
□ Cover art is exactly 1000×1000 PNG

VELOCITY
□ Minimum 4 layers per program
□ Velocity thresholds produce audible timbral change (not just volume)
□ Crossfade enabled on multi-layer keygroups
□ Layer normalization: consistent peak level across all velocities

Q-LINKS
□ At least 4 Q-Link assignments per program
□ All Q-Link names ≤ 8 characters
□ Bipolar set correctly (filter = unipolar, pitch = bipolar)
□ Smoothing: 20ms default, 40ms for pitch, 0ms for switches

MANIFEST
□ schemaVersion = "3.0"
□ targetHardware includes at least one value
□ genreTags and moodTags populated
□ previewAudio path exists if file is included

COVER ART
□ Master at 1200×1200
□ XPN embed at 1000×1000
□ XO_OX logo present
□ Engine accent color is primary color
□ Warm white #F8F6F3 as background

TESTING
□ Loaded and played on MPC hardware (not just desktop)
□ All Q-Links tested for response
□ Velocity extreme test (v1 and v127 sound different)
□ No clicks or pops at any velocity
□ Loop points confirmed seamless (if looped)
```

---

### Appendix C: Sonic DNA → Velocity Layer Quick Reference Table

| DNA Profile | Aggression | Layers | Curve | Filter Env Amount | Crossfade |
|-------------|-----------|--------|-------|------------------|-----------|
| Foundation  | 1–2       | 4      | Log   | Low              | Hard split |
| Atmosphere  | 1–3       | 4      | Log   | Low              | Soft blend |
| Prism       | 3–5       | 6      | Lin   | Medium           | Soft blend |
| Flux        | 4–6       | 8      | Lin   | Medium-High      | Crossfade |
| Aether      | 2–4       | 4      | Log   | Low              | Soft blend |
| Entangled   | 4–7       | 8      | Exp   | High             | Crossfade |
| Family      | 1–8       | Varies | Custom | Varies          | Varies |

---

### Appendix D: Engine MPCe X/Y Modulation Targets Quick Reference

| Engine | Pad X Recommended | Pad Y Recommended | Performance Note |
|--------|------------------|------------------|-----------------|
| OVERDUB | Send level | Tape drive | Increase Y for warm saturation entering reverb |
| OPAL | Grain scatter | Grain density | Diagonal movement sweeps from sparse to dense cloud |
| ONSET | Pan position | Decay time | X = stereo placement, Y = snap vs sustain |
| OHM | Commune depth | Filter cutoff | Y = overall brightness, X = communal harmonic spread |
| ORPHICA | Pluck brightness | Resonator decay | X = pick position, Y = resonance ring length |
| OBLONG | Harmonic tilt | Aggression | Classic tone sweep diagonal |
| OVERWORLD | ERA crossfade | Filter cutoff | Y = era position (NES → SPC), X = brightness |
| ORACLE | GENDY density | Maqam scale | Live microtonal navigation on X |
| OUROBOROS | Chaos amount | Leash tension | Y = tighten/loosen chaos, X = orbital radius |
| ORGANON | Metabolic rate | Entropy | Diagonal from stable to runaway organism |

---

### Appendix E: Oxport Tool Quick Reference

| Tool | Input | Output | Key Flags |
|------|-------|--------|-----------|
| `sample_categorizer.py` | Raw WAV folder | Categorized folder | `--dna-file`, `--engine` |
| `generate_drum_program.py` | Config JSON + samples | `.xpm` drum | `--cycle-type`, `--layers` |
| `generate_keygroup_program.py` | Config JSON + samples | `.xpm` keygroup | `--articulation`, `--loop` |
| `expand_kit.py` | Seed kit + params | N variant kits | `--variations`, `--axis` |
| `build_xpn.py` | XPM files + manifest | `.xpn` bundle | `--validate`, `--preview-audio` |
| `cover_art_generator.py` | Color + name | PNG cover art | `--accent`, `--resolution` |
| `package_release.py` | XPN + metadata | Release ZIP | `--version`, `--checksum` |
| `render_spec_generator.py` | Engine + preset | Spec JSON | `--engine`, `--note-range` |
| `validate_manifest.py` | `manifest.json` | Pass/fail report | `--strict`, `--firmware` |
| `batch_tester.py` | XPN folder | Test report | `--mpc-beats-path` |

---

## Production Notes

**Estimated total word count**: 58,000–65,000 words

**Suggested visual asset count**: ~45 diagrams and screenshots
- 6 XPM/XML annotated code examples
- 8 workflow flowcharts
- 10 velocity curve graphs
- 6 pad layout diagrams
- 5 cover art templates and comparisons
- 4 spectrogram comparisons
- 6 full-spread photography spreads (for collection chapters)

**Suggested sidebox count**: ~22 sidebars
- "The 3 Golden Rules" (Ch. 1)
- "The Error Museum" (Ch. 1)
- "Playing Blindfolded" (Ch. 4)
- "Cover Art as Identity" (Ch. 6)
- "Do Not Violate" (Ch. 8)
- "The Perfect Loop" (Ch. 7)
- + 16 engine-specific technical notes throughout

**Target audience tiers**:
1. **Primary**: MPC Live II/III owners using XO_OX packs, wanting to understand them more deeply
2. **Secondary**: Sound designers and expansion pack creators wanting to build for MPC
3. **Tertiary**: XOceanus users wanting to bridge their plugin workflow to hardware

**Tone**: Professional but warm — "written by producers, for producers." No corporate-speak. Code examples are practical and copy-paste ready. Historical context is included where it adds soul (a word that comes up a lot in XO_OX culture).

**Publication format**: PDF primary, EPUB secondary. Cover should be hero-quality art. Interior: 2-column layout on wide pages, with sidebars, callout boxes, and code blocks with syntax highlighting.

---

*Outline compiled by Scout — 2026-03-16*
*"The map is not the territory, but a good map makes the territory navigable."*
