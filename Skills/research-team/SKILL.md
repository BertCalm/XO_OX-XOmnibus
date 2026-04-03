# Skill: /research-team

**Invoke with:** `/research-team`
**Status:** LIVE
**Last Updated:** 2026-03-24 | **Version:** 1.0 | **Next Review:** Monthly or when new engines are added
**Purpose:** Multi-specialist research team that scours open-source projects, academic papers, commercial synths, game engines, scientific visualization, generative art, and physical simulation libraries for ideas, techniques, reusable components, and inspiration to continuously elevate XOceanus in every dimension — DSP, UI, playability, architecture, and creative identity.

---

## When to Use This Skill

Use this skill when:
- Evaluating an open-source project or repo for ideas XOceanus can borrow, remix, or learn from
- Hunting for solutions to a specific DSP, UI, or architecture challenge
- Scouting for cross-domain inspiration (physics, biology, game engines, scientific viz, generative art)
- Auditing what the fleet is missing that the state of the art already has
- Building or expanding the Research Compendium (`Docs/open_source_reference_guide.md`)
- Preparing for a new engine design and need prior art research
- Reviewing a batch of repos or links provided by the user
- Autonomously discovering new open-source projects relevant to XOceanus

---

## The Research Team

Six specialists, each with a distinct lens. When analyzing any source, **all six perspectives are applied**. When the user provides repos or links, launch parallel agents — one per batch — and compile findings through all six lenses.

### 1. DSP Scout

**Mission:** Find algorithms, techniques, and optimizations that could improve XOceanus engine sound quality, CPU efficiency, or creative range.

**Looks for:**
- Oscillator techniques (polyBLEP, BLIT, wavetable interpolation, spectral morphing)
- Filter topologies (ladder, SVF, Sallen-Key, diode, Korg35, morphable multimode)
- Physical modeling approaches (waveguide, modal, bowed string, membrane, mallet)
- Modulation architectures (per-voice LFO, drawable curves, physics-based modulators)
- Effects algorithms (reverb topologies, chorus/BBD, tape modeling, spectral processing)
- Anti-aliasing strategies (oversampling, BLAMP, band-limited tables)
- Denormal protection patterns
- SIMD optimization techniques
- Block-rate vs. sample-rate computation tradeoffs
- Parameter smoothing approaches (linear ramp vs. one-pole IIR vs. circular buffer)
- Academic citations and papers behind DSP implementations

**Maps findings to:** Specific XOceanus engines, DSP library (`Source/DSP/`), Doctrines (D001-D006)

### 2. UI Hunter

**Mission:** Find visual patterns, components, interaction paradigms, and aesthetic approaches that could elevate the Gallery Model UI and PlaySurface.

**Looks for:**
- Knob/slider/control designs and interaction patterns
- Modulation visualization (depth arcs, animated indicators, drag-drop assignment)
- Waveform/spectrum/oscilloscope display techniques
- 3D visualization (wavetable, orbital, topology)
- Preset browser UX patterns (search, filtering, minimap, tile vs. list)
- Color systems and theming approaches (accent colors, dark/light, accessibility)
- Animation and transition patterns (easing curves, crossfade, spring physics)
- Layout strategies for dense parameter panels (grid systems, tabs, collapsible sections)
- Touch/gesture interaction for PlaySurface
- XY pad, radial, and 2D control surfaces
- Typography in audio contexts
- Non-synth visual inspiration (game UI, scientific visualization, data dashboards, maritime/nautical)

**Maps findings to:** Gallery Model, PlaySurface, OpticVisualizer, per-engine panels, preset browser

### 3. Architecture Analyst

**Mission:** Study code structure, threading models, memory management, plugin hosting patterns, and build systems that could improve XOceanus reliability and maintainability.

**Looks for:**
- Audio thread safety patterns (lock-free queues, FIFO, worker threads, semaphores)
- Voice allocation and stealing algorithms
- Parameter management (APVTS, pointer-based routing, dirty flags, caching)
- State serialization and preset migration patterns
- Engine/module hot-swap architectures
- Build system patterns (CMake, CI/CD, pluginval, headless testing)
- Memory pool and pre-allocation strategies
- Plugin format compliance (AU, VST3, AUv3, CLAP)
- MVC/separation of concerns patterns
- Code generation for parameter declarations
- Modular DSP graph architectures (node-based, processor chains)

**Maps findings to:** `Source/Core/`, architecture rules, build system, SDK

### 4. Competitive Intelligence

**Mission:** Track what commercial and high-profile open-source synths are doing that XOceanus should know about — features, UX patterns, market positioning, user expectations.

**Looks for:**
- Feature gaps: things users expect in 2026 that XOceanus doesn't have yet
- UX standards: interaction patterns that have become industry norms
- Market trends: what synthesis methods are gaining popularity
- Preset ecosystem patterns: how other synths organize and distribute presets
- MPE/expression support depth
- DAW integration depth (automation, MIDI learn, state persistence)
- Accessibility features (screen readers, keyboard navigation, high contrast)
- Cross-platform strategies (macOS/iOS/Windows/Linux/Web)
- Pricing and distribution models for open-source instruments
- Community and ecosystem building patterns

**Maps findings to:** Product roadmap, feature planning, market positioning

### 5. Integration Specialist

**Mission:** Identify reusable BSD/MIT/LGPL-licensed code that can be directly adopted, and GPL code that can be studied for algorithm reimplementation.

**Looks for:**
- **Directly usable (BSD/MIT/Apache/LGPL):** Libraries, modules, utilities that can be linked or included without licensing issues
- **Study-only (GPL):** Algorithms and techniques that must be independently reimplemented
- **Dependency candidates:** Libraries that could replace custom XOceanus code with battle-tested alternatives
- **SDK integration opportunities:** Code that could enhance the third-party engine SDK

**License classification:**
| License | Can Use Directly | Can Study & Reimplement | Notes |
|---------|-----------------|------------------------|-------|
| MIT / BSD / Apache 2.0 | Yes | Yes | Attribute in source |
| LGPL | Yes (as linked library) | Yes | Don't modify the library itself |
| GPL v2/v3 | No | Yes | Reimplement from concepts only |
| Proprietary (source available) | No | Carefully | Check specific terms |

**Maps findings to:** `Source/DSP/`, SDK, build dependencies

### 6. Cross-Domain Explorer

**Mission:** Scout inspiration beyond the audio world — game engines, scientific visualization, generative art, physical simulation, biology, mathematics, maritime design, architecture, and any other domain that could spark novel ideas.

**Looks for:**
- **Physics simulations:** Verlet integration, SPH fluids, cloth simulation, N-body → OXBOW, ORGANON, OUROBOROS
- **Cellular automata & emergence:** Conway variants, Lenia, reaction-diffusion → ORGANISM
- **Graph theory & topology:** Knot invariants, network flow, spectral graph theory → ORBWEAVE, MegaCouplingMatrix
- **Generative art:** Perlin noise, L-systems, strange attractors, fractal geometry → OUROBOROS, ORACLE
- **Game engine patterns:** Entity-component systems, spatial hashing, LOD, shader pipelines → OpticVisualizer, performance
- **Scientific visualization:** Volume rendering, flow fields, spectrograms, phase portraits → engine visualizers
- **Maritime/nautical design:** Bridge interfaces (OpenBridge), sonar displays, depth gauges → OCEANDEEP, ORCA, OUIE
- **Biology:** Bioluminescence, chromatophore patterns, neural oscillation, swarm behavior → OCEANIC, OCTOPUS, ORGANON
- **Psychology & perception:** Berlyne arousal curves, Gestalt principles, attention models → OFFERING, UI design
- **Music theory & ethnomusicology:** Non-Western tuning systems, rhythmic frameworks, oral traditions → ORACLE, OSTINATO, OWARE
- **Architecture & spatial design:** Acoustic modeling, gallery curation, lighting design → Gallery Model UI

**Maps findings to:** New engine concepts, coupling types, visual design, PlaySurface modes, XOuija concept

---

## How to Run a Research Session

### Mode 1: Analyze Provided Links

When the user provides repos, URLs, or references:

1. **Batch the links** into groups of 6-8
2. **Launch parallel agents** — one per batch, each applying all 6 specialist lenses
3. **Compile findings** into the standard format (see Output Format below)
4. **Update the Compendium** (`Docs/open_source_reference_guide.md`)
5. **Surface top findings** to the user with priority ratings

### Mode 2: Targeted Hunt

When seeking solutions to a specific challenge:

1. **Define the challenge** (e.g., "How should OVERTONE's spectral engine anti-alias?")
2. **Search the Compendium first** — check if existing research covers it
3. **Web search** for open-source implementations, academic papers, and forum discussions
4. **Analyze top 5-10 results** through the relevant specialist lenses
5. **Recommend specific approaches** with license status and implementation complexity

### Mode 3: Autonomous Discovery

When expanding the Compendium proactively:

1. **Identify gaps** — which XOceanus engines/features have the least research coverage?
2. **Search for relevant projects** on GitHub, KVR, academic repositories
3. **Analyze and catalog** through all 6 lenses
4. **Add to Compendium** with proper categorization
5. **Flag high-priority discoveries** to the user

### Mode 4: Cross-Domain Expedition

When exploring non-audio domains:

1. **Start from an XOceanus concept** (e.g., ORGANISM's cellular automata)
2. **Search adjacent domains** (biology, game dev, computational art)
3. **Identify transferable techniques** (data structures, algorithms, visual patterns)
4. **Map to XOceanus applications** with concrete implementation suggestions
5. **Add to Compendium** under Cross-Domain section

---

## Output Format

Every research finding follows this structure:

```markdown
### [Project/Source Name]

**Source:** [URL]
**License:** [License] — [Can Use Directly / Study Only]
**Domain:** [Audio / Game Engine / Science / Art / Biology / etc.]

#### DSP Ideas
- [Specific technique] — [Which XOceanus engine/component benefits] — [Priority P0-P3]

#### UI Ideas
- [Specific pattern] — [Which XOceanus UI component benefits] — [Priority P0-P3]

#### Architecture Ideas
- [Specific pattern] — [Which XOceanus system benefits] — [Priority P0-P3]

#### Reusable Components
- [Component name] — [License] — [Integration complexity: Drop-in / Adapter / Reimplement]

#### Cross-Domain Inspiration
- [Concept] — [How it maps to XOceanus] — [Priority P0-P3]
```

### Priority Levels

| Level | Meaning | Action |
|-------|---------|--------|
| **P0** | Fixes a known bug or resolves a seance finding | Implement now |
| **P1** | Significant quality improvement to an existing engine or system | Implement this release cycle |
| **P2** | Meaningful enhancement or new capability | Schedule for next release |
| **P3** | Interesting future possibility or long-term inspiration | Catalog for reference |

---

## The Compendium

All research lives in `Docs/open_source_reference_guide.md` — the single source of truth for external inspiration.

### Compendium Structure

```
# XOceanus Open-Source Research Compendium

## Priority Action Items (P0-P3)
## DSP Reference Library
  ### Oscillators & Synthesis
  ### Filters
  ### Physical Modeling
  ### Modulation
  ### Effects & Processing
  ### Anti-Aliasing & Optimization
## UI Pattern Library
  ### Controls & Interaction
  ### Visualization
  ### Layout & Navigation
  ### Theming & Color
## Architecture Reference
  ### Threading & Safety
  ### State Management
  ### Plugin Infrastructure
## Cross-Domain Inspiration
  ### Physics & Simulation
  ### Biology & Emergence
  ### Generative Art
  ### Maritime & Nautical
  ### Game Engines
  ### Scientific Visualization
## Reusable Components (by license)
  ### BSD / MIT / Apache (directly usable)
  ### LGPL (linkable)
  ### GPL (study only)
## Concept Seeds
  ### XOuija (cursor-driven coupling instrument)
  ### [Future concepts from research]
## Full Project Index
  ### Analyzed Projects (detailed findings)
  ### Cataloged Projects (awaiting analysis)
  ### awesome-juce Full Inventory
```

### Maintenance Rules

1. **Never delete entries** — mark deprecated findings as `[SUPERSEDED by X]`
2. **Date all additions** — every entry gets a `(added YYYY-MM-DD)` tag
3. **Link back to engines** — every finding must reference at least one XOceanus engine, system, or concept
4. **Track implementation** — when a finding is implemented, mark it `[IMPLEMENTED in engine/component]`
5. **Review quarterly** — re-evaluate P2/P3 items for promotion or archival

---

## Integration with Other Skills

| Skill | How Research Team Feeds It |
|-------|--------------------------|
| `/synth-seance` | Research findings become questions for the ghost panel |
| `/engine-health-check` | New DSP techniques raise the quality bar |
| `/validate-engine` | Cross-reference against state-of-the-art implementations |
| `/preset-architect` | UI research influences preset browser and parameter display |
| `/dsp-safety` | Architecture findings inform thread-safety patterns |
| `/coupling-debugger` | New coupling types discovered through research |
| `/master-audit` | Research Compendium becomes a reference during fleet audits |
| `/architect` | Architecture Analyst findings inform code review standards |

---

## Example Invocations

```
# Analyze a batch of repos
/research-team https://github.com/foo/bar https://github.com/baz/qux

# Targeted hunt for a specific problem
/research-team How do other synths handle wavetable anti-aliasing at high frequencies?

# Cross-domain expedition
/research-team Find physics simulation techniques relevant to OXBOW's phase erosion

# Autonomous discovery for a specific engine
/research-team What open-source projects could improve OUIE's vocal synthesis?

# Expand the Compendium
/research-team Scan GitHub for new JUCE synthesizer projects from 2026

# Non-audio inspiration
/research-team Find generative art techniques that could inspire new coupling visualizations
```

---

## Research Ethics

- **Always attribute** — every finding links back to its source
- **Respect licenses** — never suggest copying GPL code into XOceanus without explicit reimplementation
- **Credit ideas** — when an XOceanus feature is inspired by external work, note the inspiration in code comments
- **Give back** — when XOceanus develops novel techniques, consider open-sourcing them for the community
- **Stay curious** — the best ideas come from the most unexpected places (OpenBridge → OCEANDEEP UI)
