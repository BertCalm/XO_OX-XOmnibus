# XOlokun Battle Plan — Session-Ready Reference

> **Last updated:** 2026-03-26
> **Branch:** `feat/outshine-phase1b-rebirth`
> **Build:** PASS (0 errors) as of commit `0a362679f`
> **Repo:** `~/Documents/GitHub/XO_OX-XOmnibus`

## How to Use This Document

Pick a track or group of tracks. Each entry has enough context for a fresh Claude session to execute it. Copy the relevant section into your prompt.

**Model routing:**
- **Opus** tracks = creative design, architecture decisions, seances, vision quests
- **Sonnet** tracks = implementation, bug fixes, build, documentation, preset work
- **Haiku** tracks = read-only audits, counts, verification, git ops

**Priority tiers:**
- 🔴 **P0** — blocks other work or fixes data integrity issues
- 🟡 **P1** — high-impact, unlocks downstream tracks
- 🟢 **P2** — important but not blocking
- ⚪ **P3** — nice-to-have, V1.1 candidate

---

## Completed Tracks

| Track | Description | Commit |
|-------|-------------|--------|
| T019 | Lock AU component ID (aumu Xolk XoOx) | `04962838b` |
| T020 | XOmnibus reference cleanup | `04962838b` |
| T071-T075 | UI visual polish (6 fixes) | `1ff410239` |
| H88/T008 | Euler IIR → matched-Z in 4 FX files | `0a362679f` |
| T007 | Bipolar coupling fix (Orbital, Offering) | `0a362679f` |
| — | Dual-engine integration (Outwit/Overlap/Osmosis/Outlook adapters) | `0a362679f` |
| — | xomnibus→xolokun namespace (Okeanos/Outflow) | `0a362679f` |
| — | OcelotVoice sample-rate-aware steal fade | `0a362679f` |
| — | ParameterSmoother denormal convergence guard | `0a362679f` |
| — | fXBreath denormal flush | `0a362679f` |
| H13 | Verify no uncommitted files | Working tree clean |

---

## OPUS TRACKS (O1–O100): Creative / Architecture / Design

### UI Implementation — THE Priority

#### O1 — Build Rebirth Prototype V0.2 🟡 P1
**Skill:** `/uix-design-studio` or `/atelier`
**Prereqs:** Read `Docs/design/xolokun-spatial-architecture-v1.md` and `Docs/mockups/xolokun-v04-polished.html`
**Instructions:** Build an HTML/CSS prototype (NOT JUCE yet) implementing depth-zone dials, Audio UI knob aesthetics, OpenBridge 6.0 density, APERTURE font. 3-column layout: Column A (260pt engine rack), Column B (520pt engine detail), Column C (320pt browser/coupling/FX). Gallery Model warm white shell `#F8F6F3` framing engine accent colors. Output: a single self-contained HTML file at `Docs/mockups/xolokun-rebirth-v02.html`. Must be visually indistinguishable from a production app screenshot.

#### O2 — Build Rebirth Prototype V0.3 🟢 P2
**Prereqs:** O1 complete
**Instructions:** Extend V0.2 with full interaction: draggable knobs (mousedown+drag rotates), coupling arc click (opens inline popover), preset browse (scrollable list with mood filter pills). All state changes update visually in real-time. Still HTML — this validates interaction design before JUCE implementation.

#### O3 — Design CouplingVisualizer Center Panel 🟢 P2
**Prereqs:** Read `Source/Core/MegaCouplingMatrix.h` for 15 coupling types
**Instructions:** Design exact node positions for 4 engine slots, arc rendering spec (bezier curves, brightness = coupling energy, stroke width = amount), click targets (≥16pt), idle/no-coupling state. Produce an annotated spec with pixel dimensions. Output: `Docs/design/coupling-visualizer-spec.md`

#### O4 — Design Depth-Zone Engine Browser 🟢 P2
**Prereqs:** Read `~/.claude/projects/-Users-joshuacramblet/memory/aquatic-mythology.md` for zone assignments
**Instructions:** Design ocean cross-section overlay with 5 zones (sunlight/twilight/midnight/abyssal/hadal). 73 creature dots positioned by depth affinity. Hover = tooltip (engine name + accent color + 6-word description). Click = load into selected slot. Output: zone-to-engine assignment table + mockup spec.

#### O5 — Design Preset DNA Browser 🟢 P2
**Instructions:** Design mood tiles (15 moods as filterable pills) + hexagon cards (6-axis DNA radar per preset) + spatial navigation (drag to browse by DNA similarity). "Find Similar" sorts by Euclidean distance in 6D DNA space. Output: interaction spec + visual mockup.

#### O6 — Design Dark Cockpit Opacity Behavior 🟢 P2
**Prereqs:** Blessing B041 ratified 8-0
**Instructions:** Define 5-level opacity hierarchy (100%/80%/45%/20%/0%) derived from OpenBridge 6.0 maritime design. Triggers: PlaySurface expanded, during note play, idle 30s. Define transition timing (ms), which controls map to which level. Output: `Docs/design/dark-cockpit-spec.md`

#### O7 — Design Performance Mode Layout 🟢 P2
**Instructions:** Macros enlarged 3x in header, coupling center prominent, PlaySurface bottom 220pt full width. Column A at 45%, B at 20%, C at 20% opacity. Define exactly which controls remain interactive vs. visual-only during performance. Output: annotated layout spec.

#### O8 — VQ-UI-003: "111 Parameters, 4 Visible" 🟢 P2
**Skill:** `/visionary`
**Instructions:** Progressive disclosure architecture for engines with 60-111 params. 4 hero knobs always visible, everything else collapsed. Define: which params surface as heroes (per-engine), section collapse hierarchy, disclosure levels (1-click, 2-click, expert mode). This is the architectural pattern all custom engine panels follow.

#### O9 — VQ-UI-004: Engine Personality 🟢 P2
**Skill:** `/visionary`
**Instructions:** Each of the 73 engines should LOOK different. Define how accent color, signal flow variant, custom hero widgets, and background texture combine to give each engine visual identity. Not just color — structure. Output: personality spec for 5 reference engines (OBRIX, ONSET, OPERA, OXYTOCIN, OUROBOROS).

#### O10 — VQ-UI-005: Aquarium as Interface ⚪ P3
**Skill:** `/visionary`
**Instructions:** The mythology IS navigation. Ocean depth → engine browser. Creature identity → engine avatar. Water column position → frequency range. Design how the aquatic mythology becomes a functional navigation system, not just a theme. Output: vision doc.

### PlaySurface Design

#### O11 — Design XOuija Planchette 🟢 P2
**Prereqs:** Blessing B042 ratified 7-1
**Instructions:** Design Lissajous idle drift (0.3Hz/0.2Hz), spring lock-on behavior, bioluminescent velocity-scaled trail, warm memory hold (cursor remembers last position). The planchette is an autonomous entity with its own behavior. Output: behavior spec + motion equations.

#### O12 — Design Seaboard Continuous Keyboard 🟢 P2
**Instructions:** MPE visual language for continuous pitch/slide/pressure per note. Glissando/portamento gesture rendering. Key layout, key width, channel assignment strategy. Output: `Docs/design/seaboard-spec.md`

#### O13 — Design Tide Controller ⚪ P3
**Instructions:** Water simulation physics, ripple-to-modulation mapping. Touch creates ripples, ripple position/amplitude maps to mod destinations. Output: physics model + modulation routing spec.

#### O14 — Design Expression Panel 🟢 P2
**Instructions:** Mod wheel, pitch bend, XY pad, macro strips — all together in one panel. Define layout, sizing, MIDI CC assignments, visual feedback. Must fit in PlaySurface container alongside other surfaces.

#### O15 — Design PlaySurface Container 🟢 P2
**Instructions:** Tab switching between surfaces (MPC Pads / Seaboard / XOuija / Expression / Tide). Define tab strip design, surface selection UX, auto-expand behavior for drum engines. Output: container spec.

### Creative Features

#### O16 — Implement B026: Interval-as-Parameter 🟢 P2
**Prereqs:** Read `Source/Engines/Ouie/OuieEngine.h`
**Instructions:** Musical interval between OUIE's two voices becomes a first-class timbral parameter `ouie_interval`. Design: parameter range (unison through octave+), how interval maps to voice detuning, how it interacts with the HAMMER axis. This is a blessed feature (B026).

#### O17 — Implement B041: Dark Cockpit 🟢 P2
**Prereqs:** O6 complete (design spec)
**Instructions:** Implement the opacity hierarchy system from O6's spec. Requires `DarkCockpitController` component (see S75 for JUCE implementation). This is the design decision layer — S75 is the code.

#### O18 — Implement B042: Planchette Autonomy 🟢 P2
**Prereqs:** O11 complete (planchette design)
**Instructions:** Implement Lissajous + spring + warm memory behavior per B042 blessing. Design the DSP-level behavior model. S86 handles the JUCE implementation.

#### O19 — Implement B043: Gesture Trail as Modulation 🟢 P2
**Prereqs:** Blessing B043 ratified 6-2
**Instructions:** Design the DSP signal path from the ring buffer (256 x/y/velocity/time tuples) to a modulation output. Trail becomes a replayable, freezable, coupleable signal. Two performers' trails create interference patterns. Output: signal flow diagram + parameter spec.

#### O20 — Design Constellation View ⚪ P3
**Instructions:** 73 engines as interactive star chart. Engine position determined by sonic DNA centroid. Proximity = coupling compatibility. Click to load. Zoom/pan navigation. 25% push feature — may defer to V1.1.

#### O21 — Design Spatial Preset Navigation ⚪ P3
**Instructions:** ~17K presets as continuous 2D map. Position = 6D DNA projected to 2D (t-SNE or UMAP). Browse by dragging. Cluster visualization shows mood regions. 25% push — may defer.

#### O22 — Design Emotion-Responsive UI ⚪ P3
**Instructions:** Color temperature adapts to audio output. Aggressive sounds warm the palette, ambient sounds cool it. Define the mapping from audio features (RMS, spectral centroid, crest factor) to color shift. 25% push — may defer.

#### O23 — Design "Sound on First Launch" Init Preset 🟡 P1
**Instructions:** Define which engines load in Slots 1-2, which coupling type/amount, macro positions, so the user hears something compelling in the first 3 seconds without touching anything. Candidate: OXBOW Slot 1 + OPERA Slot 2, ENTANGLE coupling at 0.35. Write the .xometa spec.

### Seances & Creative Reviews

#### O24 — Synth Seance on V0.2 Prototype 🟢 P2
**Skill:** `/synth-seance`
**Prereqs:** O1 complete (V0.2 prototype exists)
**Instructions:** Convene 8 ghost pioneers to review the actual Rebirth V0.2 design. Score 1-10 on: visual identity, interaction quality, progressive disclosure, coupling arc UX, engine personality. Demand 3 changes, bless 3 elements.

#### O25 — Producers Guild Review of V0.2 🟢 P2
**Skill:** `/producers-guild`
**Prereqs:** O1 complete
**Instructions:** Does V0.2 pass the 30-second test for all 10 genres? 25 genre specialists + PM + Market Research evaluate. Score each genre 1-10. Output: pass/fail per genre + required changes.

#### O26 — UIX Studio Review of V0.2 🟢 P2
**Skill:** `/uix-design-studio`
**Prereqs:** O1 complete
**Instructions:** Ulf (Scandinavian), Issea (Japanese), Xavier (Apple), Lucy (JUCE) review. Focus on: light+logic, ma+material, kinetics, JUCE feasibility. Output: 10 specific change requests ranked by impact.

#### O27 — Seance 3 Provisional Blessings 🟢 P2
**Skill:** `/synth-seance`
**Instructions:** B007 (Velocity Coupling Outputs, OUROBOROS), B033 (Living Tuning Grid, OWARE), B034 (Per-Mode Sympathetic Network, OWARE). Convene ghost council to verify or revoke each. Read the blessing descriptions in CLAUDE.md first.

#### O28 — Kai Review of PlaySurface MPC Pads 🟢 P2
**Skill:** `/kai`
**Prereqs:** S12 complete (MPCPadGrid built)
**Instructions:** Kai and her 7 androids review the MPC pad implementation for MPC compatibility: velocity curves, note mapping, bank switching, pad sensitivity. Score and list required changes.

### XPN / Outshine / Originate

#### O29 — Design Oxport Dashboard ⚪ P3
**Instructions:** Kai recommended a visual companion tool for the XPN export pipeline. Design: pack overview, sample browser, kit assignment grid, export progress, validation status. Output: wireframe spec.

#### O30 — Redesign Outshine UI for Rebirth ⚪ P3
**Prereqs:** O1 complete (Rebirth aesthetic established)
**Instructions:** Outshine's current UI doesn't match XOlokun's Rebirth aesthetic. Redesign to use Gallery Model warm white shell, depth-zone dials, consistent typography. Output: redesign spec.

#### O31 — Design "Send to MPC" USB Workflow ⚪ P3
**Instructions:** Define the flow: select pack → validate → copy to USB → detect MPC → confirm. Error states (no MPC found, insufficient space, format incompatible). Output: workflow diagram.

#### O32 — Design MPCe Quad-Corner Assignment Panel ⚪ P3
**Instructions:** Panel for assigning 4 engine presets to MPCe quad corners. Visual: 4 quadrants, each showing engine name + preset name + accent color. Drag to assign. Output: interaction spec.

### Brand & Launch

#### O33 — Write XOlokun Cultural Acknowledgment 🟡 P1
**Instructions:** XOlokun references Olokun, the Yoruba orisha of the deep ocean. Write a respectful cultural acknowledgment page for XO-OX.org. Research the orisha's significance, explain the naming choice, express gratitude. Get external review before publishing. Output: page copy for the About section.

#### O34 — Design V1 Launch Sequence 🟢 P2
**Skill:** `/launch-coordinator`
**Instructions:** Press kit, launch timeline, announcement sequence. Define: pre-launch (2 weeks), launch day, post-launch (2 weeks). Channels: Discord, Patreon, XO-OX.org, social. Output: timeline doc + press kit spec.

#### O35 — Design Patreon Tier Benefits 🟢 P2
**Prereqs:** Read `~/.claude/projects/-Users-joshuacramblet/memory/patreon-milestone-model.md`
**Instructions:** Kitchen Collection quads unlock at patron thresholds (10/25/50/100/250/500). Define tier benefits at each level. What does a $3/mo patron get vs $10/mo? Output: tier structure doc.

#### O36 — Write First 3 Field Guide Posts 🟢 P2
**Skill:** `/atelier` + `/field-guide-editor`
**Instructions:** Write 3 launch Field Guide posts for XO-OX.org. Suggested topics: (1) "What is XOlokun?", (2) "Your First Hour with XOlokun", (3) "The Aquatic Engine Fleet". Target: 3,000-4,000 words each. Output: 3 markdown posts.

#### O37 — Design XO-OX.org Rebrand 🟢 P2
**Skill:** `/atelier`
**Instructions:** Update XO-OX.org from XOmnibus branding to XOlokun. Homepage hero, aquarium page, Field Guide landing, about page. All copy and structural changes. Output: page-by-page content spec.

### Architecture

#### O38 — Design JUCE LookAndFeel Architecture 🟡 P1
**Instructions:** Class hierarchy: `XOlokunLookAndFeel` extends `juce::LookAndFeel_V4`. Define all override methods (drawRotarySlider for 3 knob sizes, drawButtonBackground, drawComboBox, etc. — minimum 12 overrides). Font loading strategy (Space Grotesk / Inter / JetBrains Mono via BinaryData). Color token mapping from design-tokens.css to juce::Colours. Output: class skeleton with all method signatures. Prereq for ALL JUCE UI work.

#### O39 — Design iOS AUv3 UI Adaptation 🟢 P2
**Prereqs:** Read `~/.claude/projects/-Users-joshuacramblet/memory/ios-design-spec.md`
**Instructions:** Which components need touch-target expansion (44pt min)? 3-column → 2-column on iPad portrait? Which features defer to V2? Minimum viable iPad layout at 1024x768pt. Output: 3-tier feature matrix (V1/V1.1/V2 iOS).

#### O40 — Design Standalone App UI 🟢 P2
**Instructions:** Standalone needs the custom editor, not JUCE's generic host. Define: window size, menu bar items, audio/MIDI settings panel, recent files. How does it differ from the plugin version? Output: spec doc.

#### O41 — Architecture for Mounting 93K Lines Unmounted UI ⚪ P3
**Instructions:** ~93K lines of built-but-not-mounted UI code in Source/UI/. Audit what exists, categorize by readiness (ready to mount / needs rework / deprecated), define mounting order. Output: mounting plan with dependency graph.

#### O42 — Design Preset Save/Export Workflow UX 🟢 P2
**Instructions:** User flow: tweak preset → save (overwrite vs. save-as) → export (share as .xometa, export as XPN sample). Define: save dialog, naming, mood assignment, DNA auto-calculation, export format selection. Output: UX flow diagram.

### Innovation

#### O43 — Design Living Manual ⚪ P3
**Instructions:** Glassmorphism contextual tooltips. Hover 1.2s on any control → tooltip with: param name, current value, range, plain-language description, coupling suggestion. Blurred glass background effect. Positioning rules (no window edge clipping). Write tooltip copy for top 20 params.

#### O44 — Design Spectral Silhouette ⚪ P3
**Instructions:** Engine identity as background visual DNA — a subtle watermark-like rendering derived from the engine's spectral characteristics. Each engine has a visually distinct "fingerprint" visible behind the controls.

#### O45 — Theorem Session ⚪ P3
**Skill:** `/theorem`
**Instructions:** Generate 3 new engine concepts for V1.1 addressing gaps: granular chaos, physical modeling of metals, spectral morphing voice. Each: O-word name, accent color, mythology, 6 blessing candidates, coupling compatibility.

#### O46 — Design Coupling Gradient XPN Export ⚪ P3
**Instructions:** 4 coupling depths per MPC pad (corners of quad). When pad is struck, coupling amount interpolates based on strike position. Define: XPN format extension, MPC compatibility constraints, fallback for non-position-sensitive pads.

#### O47 — Design Complement Chain Intelligence ⚪ P3
**Skill:** `/visionary`
**Instructions:** Visionary Legend Feature 7. Automatic coupling chain suggestions based on engine characteristics and DNA complementarity. "These 3 engines, in this order, with these coupling types, produce X genre."

### Community & Growth

#### O48 — Artist Collaboration Framework ⚪ P3
**Skill:** `/artist-collaboration`
**Instructions:** Design first external collab: identify 3 target producers (electronic/hip-hop/ambient), define deliverable (20 presets + 1 Field Guide post + 1 video), compensation model. Output: outreach email template + framework doc.

#### O49 — Community Strategy for Launch ⚪ P3
**Skill:** `/community`
**Instructions:** Barry OB's team deployment plan for launch. Discord channel structure, Week 1 seeding posts, first community challenge, moderation plan. Output: community launch playbook.

#### O50 — Design XOlokun Tutorial/Onboarding 🟢 P2
**Skill:** `/tutorial-studio`
**Instructions:** 5-step contextual tooltip sequence for first-time users. Step 1: "This is OBRIX", Step 2: "This is a coupling arc", Step 3: "Browse presets by mood", Step 4: "Load a second engine", Step 5: "You're ready". Write all copy and trigger conditions.

---

### O51–O60: V1 Spatial Architecture UI Design

#### O51 — Column A Engine Rack Tile Spec 🟡 P1
**Instructions:** Exact pixel layout for each 260×90pt slot tile: on/off toggle, engine name (Space Grotesk 14pt), waveform thumbnail (80×24pt), CPU% badge, swap affordance. Produce Figma-ready annotated mockup. This spec drives S52 implementation.

#### O52 — Column B Standard Engine Detail (≤40 params) 🟡 P1
**Instructions:** 4 macro knobs sticky top (88pt strip), signal flow diagram (32pt), 4 collapsible param sections (OSC/FILTER/MOD/FX), waveform+ADSR sticky bottom. All spacing tokens defined. Drives S53 implementation.

#### O53 — Column B Custom Engine Detail (60-111 params) 🟡 P1
**Instructions:** Jump-to-section nav bar, section collapse defaults, hero knob selection per engine. Start with ONSET (111 params) as hardest case. Define the pattern all 17 custom engines follow.

#### O54 — Column C Preset Browser Tab 🟢 P2
**Instructions:** Instrument-type filter pill row (DRUMS/BASS/PADS/KEYS/LEAD/FX), mood pill row (15 moods), DNA radar mini-card per preset, "Find Similar" DNA proximity sort. Scroll behavior and empty-state defined.

#### O55 — Column C Coupling Inspector Tab 🟢 P2
**Instructions:** Source→target arc list, inline type/amount/depth per route, 3 quick-start types with plain-language descriptions (Entangle/Gravitational/Phase), "More..." expansion. Real-time sync with MiniCouplingGraph.

#### O56 — Column C FX Tab 🟢 P2
**Instructions:** 6 FX slot list (SAT/DELAY/REVERB/MOD/COMP/SEQ), inline expand per slot, bypass toggle, slot reorder handle, per-slot CPU cost badge.

#### O57 — Mini Coupling Graph (120×80pt) 🟢 P2
**Instructions:** 4 engine nodes as 8pt dots, coupling arcs as bezier curves, brightness ∝ coupling energy, arc click opens popover. Idle/no-coupling state defined. Sits at bottom of Column A.

#### O58 — Inline Coupling Arc Popover 🟢 P2
**Instructions:** Anchors to arc midpoint, shows type selector (3 + "More..."), amount knob, depth knob. Must not occlude signal flow diagram. 200ms spring animation, no bounce. Close on outside click.

#### O59 — Performance Mode Layout 🟢 P2
**Instructions:** ColA/B/C dimmed at 45%/20%/20%, macros enlarged to 48pt in header, PlaySurface 220pt full width. Which controls stay interactive vs. visual-only.

#### O60 — CinematicMode Full-Width Expansion 🟢 P2
**Instructions:** ColA+ColC collapse to 0pt with 300ms ease-out, ColB expands to 1100pt. Trigger: double-click ColB header. Define what ColB shows at 1100pt vs 520pt.

### O61–O75: Per-Engine Custom UI Panels

Each of the following requires reading the corresponding engine header first.

#### O61 — ONSET Custom Panel (111 params) 🟡 P1
**Prereqs:** Read `Source/Engines/Onset/OnsetEngine.h`
**Instructions:** 8-voice drum grid as hero (280pt), per-voice algorithm selector, section nav DRUM/OSC/FILTER/MOD/FX. Identify the 12 hero params. The drum grid IS the primary interaction surface.

#### O62 — OBRIX Custom Panel (81 params) 🟡 P1
**Prereqs:** Read `Source/Engines/Obrix/ObrixEngine.h`
**Instructions:** Brick Stack View as signal flow Variant B, modular slot-flow with drag connections, reef ecology status display (reefResident mode indicator), Wave indicator. Hero params: src1Type, src2Type, fxMode, reefResident.

#### O63 — OBLONG Custom Panel (72 params) 🟢 P2
**Prereqs:** Read `Source/Engines/Oblong/OblongEngine.h`
**Instructions:** 5 core synthesis layers as section tabs, 8 hero knobs for macro strip, collapse defaults.

#### O64 — OUROBOROS Custom Panel 🟢 P2
**Prereqs:** Read `Source/Engines/Ouroboros/OuroborosEngine.h`
**Instructions:** Circular feedback topology icon, leash mechanism slider (distinct from standard knobs), chaos attractor visualization (32pt strip). Hero: topology, leash, chaos. Variant C network.

#### O65 — ORGANON Custom Panel 🟢 P2
**Instructions:** Metabolic rate as biological heartbeat display, variational free energy meter, 4-metabolism mode indicator. Reference B011.

#### O66 — ORBWEAVE Custom Panel 🟢 P2
**Instructions:** Knot topology matrix as 4×4 grid (Trefoil/Figure-Eight/Torus/Solomon), MACRO KNOT morph arc as gestural sweep zone. Variant C expand-to-CinematicMode routing.

#### O67 — OPENSKY Custom Panel 🟢 P2
**Instructions:** RISE macro as vertical ascension gesture strip (pitch env + filter + shimmer), Shepard shimmer depth display. Hero: rise, shimmerDepth, chorusWidth.

#### O68 — OUIE Custom Panel 🟢 P2
**Instructions:** HAMMER bipolar axis as horizontal STRIFE↔LOVE slider, dual-voice algorithm selectors (8 each), interval display. B025/B026/B027.

#### O69 — OPERA Custom Panel 🟢 P2
**Instructions:** Conductor arc shape selector (4 types), arc time/peak controls, coherence meter (Kuramoto R), EmotionalMemory indicator, Conductor mode 3-way switch (Auto/Manual/Both). B035/B036/B037.

#### O70 — OCEANDEEP Custom Panel 🟢 P2
**Instructions:** Hydrostatic pressure meter as depth gauge (0-100 fathoms), Darkness Filter ceiling (50-800Hz as identity), Bioluminescent Exciter visualization. B029/B030/B031.

#### O71 — OFFERING Custom Panel 🟢 P2
**Instructions:** City mode as 5-city map icon row (NY/Detroit/LA/Toronto/Bay Area), Berlyne curiosity meter, per-drum-type display (8 types). B038/B039.

#### O72 — OXYTOCIN Custom Panel 🟢 P2
**Instructions:** Love triangle display (3-lineage visual), intimacy accumulator bar (grows with note duration per B040), 5 legend lineage selector (RE-201/MS-20/Moog/Serge/Buchla).

#### O73 — ORGANISM Custom Panel 🟢 P2
**Instructions:** Cellular automata grid (live visualization), ruleSet selector, emergence meter. CA grid animates with audio output. Variant C.

#### O74 — OBESE Custom Panel 🟢 P2
**Instructions:** Mojo orthogonal axis as 2D XY pad (analog warmth vs digital precision per B015), satDrive as VU-style meter. Mojo IS the primary interaction.

#### O75 — ORACLE Custom Panel 🟢 P2
**Instructions:** GENDY stochastic display, Maqam scale grid (microtonal tuning), breakpoints as editable curve. B010 reference.

### O76–O85: Sound, Content & Brand

#### O76 — "Sound on First Launch" OXBOW Init Preset 🟡 P1
**Instructions:** OXBOW Slot 1 + OPERA Slot 2, ENTANGLE coupling at 0.35, macro positions defined. What the user hears in first 3 seconds. Write .xometa spec + fallback hard-coded init params.

#### O77 — Depth-Zone Engine Browser (73 creatures) 🟢 P2
**Instructions:** Assign all 73 engines to ocean zones. Hover tooltip, click to load. Produce zone-to-engine assignment table.

#### O78 — OBRIX Pocket Standalone Design 🟢 P2
**Prereqs:** Read `~/.claude/projects/-Users-joshuacramblet/memory/obrix-pocket-design.md`
**Instructions:** Single-engine window (600×480pt), 81 params, Brick Stack View, no coupling. Mobile-first 44pt touch targets. First standalone app spec.

#### O79 — V1 "Sound on First Launch" 5 Coupling Presets 🟡 P1
**Instructions:** 5 coupling presets for first user experience: (1) OXBOW+OPERA Entangle, (2) ONSET+OFFERING Gravity, (3) OBRIX+OUROBOROS Phase, (4) OPENSKY+ORBWEAVE Knot, (5) ORGANON+ORGANISM Adversarial. Write all 5 .xometa specs.

#### O80 — Field Guide #4: 15 Coupling Types 🟢 P2
**Skill:** `/atelier`
**Instructions:** 3,500 words, 15 audio examples referenced, practical producer guide. What each type sounds like, best engine pairs.

#### O81 — Field Guide #5: Inside OBRIX 🟢 P2
**Instructions:** 4,000 words. 5-wave architecture, B016, Biophonic Synthesis, Reef Residency. Signal flow diagrams.

#### O82 — Field Guide #6: 5 Genre Starter Templates 🟢 P2
**Instructions:** Hip-hop (ONSET+OFFERING), ambient (OXBOW+OPERA), DnB (ONSET+OBRIX), cinematic (ORGANON+OPENSKY), lo-fi (OWARE+OVERDUB). Coupling presets included.

#### O83 — Patreon Content Calendar Month 1 🟢 P2
**Skill:** `/patreon-content-manager`
**Instructions:** Week 1 (release + OBRIX dive), Week 2 (patron-exclusive 50 presets), Week 3 (KC preview video), Week 4 (community poll). Exact post copy + asset list.

#### O84 — V1 Press Kit 🟢 P2
**Skill:** `/launch-coordinator`
**Instructions:** 1-paragraph description, 5 differentiators, 3 endorsement quotes, 6 hero screenshot specs, 90-second video script. Write all copy.

#### O85 — XO-OX.org XOlokun Rebrand Copy 🟢 P2
**Instructions:** Homepage hero, aquarium intro, Field Guide landing restructure. Pure content, no design tools.

### O86–O100: Architecture, Community, Meta

#### O86 — Theorem: 3 New V1.1 Engine Concepts ⚪ P3
**Skill:** `/theorem`

#### O87 — Artist Collaboration Framework ⚪ P3
**Skill:** `/artist-collaboration`
**Instructions:** 3 target producers, deliverable spec, compensation model, outreach email.

#### O88 — Coupling Recipe System (.xorecipe) 🟢 P2
**Instructions:** Define format: engine pair + coupling type + macro positions + intent. 5 starter recipes for V1.

#### O89 — XPN Pack Product Page Copy 🟢 P2
**Instructions:** mpce-perc-001 page: name, tagline, 3 demo clips, 8 kits, pricing, CTA.

#### O90 — Discord Server Launch Strategy ⚪ P3
**Skill:** `/community`
**Instructions:** Channel structure, moderation, Week 1 content, first challenge.

#### O91 — JUCE LookAndFeel Class Skeleton 🟡 P1
**Instructions:** Class hierarchy, 12+ override signatures, font loading, color mapping. Prereq for all S51+ JUCE UI tracks. (Overlaps with O38 — do together.)

#### O92 — iOS AUv3 3-Tier Feature Matrix 🟢 P2
**Instructions:** V1 iOS / V1.1 iOS / V2 iOS. Touch targets, layout collapse, deferred features.

#### O93 — Progressive Disclosure Onboarding 🟢 P2
**Instructions:** 5-step tooltip sequence. All copy + trigger conditions.

#### O94 — V1.1 Feature Slate ⚪ P3
**Instructions:** Per-knob mod rings, Constellation View, Spatial Preset Nav. Scope, criteria, session estimates.

#### O95 — Seance the V1 Spatial Architecture 🟢 P2
**Skill:** `/synth-seance`
**Instructions:** Ghost council reviews 3-column layout, proportions, arc popover, PlaySurface auto-expand. 3 demands + 3 blessings.

#### O96 — Producers Guild Review of V1 Launch Presets 🟢 P2
**Skill:** `/producers-guild`
**Instructions:** Score the 5 coupling presets from O79 for 5 genres. 1-10 each + required changes.

#### O97 — Instrument Browser Filter Taxonomy 🟡 P1
**Instructions:** Assign each of 73 engines to exactly one type (DRUMS/BASS/PADS/KEYS/LEAD/FX/TEXTURE/GENERATOR/MODIFIER). Required for preset browser filter.

#### O98 — OBRIX Flagship Launch Campaign 🟢 P2
**Instructions:** OBRIX page on XO-OX.org, announcement copy, 30s promo video script.

#### O99 — Living Manual Tooltip System 🟢 P2
**Instructions:** Trigger conditions, tooltip anatomy, positioning, copy for 20 most-touched params.

#### O100 — V1 Risk Register 🟢 P2
**Instructions:** Top 10 highest-risk items by likelihood × impact. Mitigation for each.

---

## SONNET TRACKS (S1–S100): Implementation / Build / Documentation

### S1–S10: UI Implementation

#### S1 — Implement CouplingVisualizer in XOlokunEditor 🟢 P2
**Prereqs:** O3 complete (design spec)
**File:** `Source/UI/XOlokunEditor.h`
**Instructions:** Wire `CouplingVisualizer` as default center panel. Read `Source/Core/MegaCouplingMatrix.h` for coupling data. Render 4 engine nodes + coupling arcs. Build PASS required.

#### S2 — Mount PlaySurface in XOlokunEditor 🟢 P2 (PARTIAL)
**File:** `Source/UI/PlaySurface/PlaySurface.h` (181 lines expanded in last commit)
**Instructions:** PlaySurface is built but hidden. Mount it in XOlokunEditor with a toggle button. Verify MIDI output. The expansion committed in `0a362679f` added MPE support and bank switching but mounting is still incomplete.

#### S3 — Mount ExportDialog 🟢 P2
**File:** `Source/UI/ExportDialog.h`
**Instructions:** Built but no button opens it. Add export button to header bar, wire to open ExportDialog. Fix memory leak (see S73) while at it.

#### S4 — Mount Full PresetBrowser 🟢 P2
**File:** Existing stripped `PresetBrowser` needs replacement
**Instructions:** Full browser with DNA "Find Similar", replaces current stripped version. Wire to PresetManager for filtering by mood + engine + instrument type.

#### S5 — Fix 7 Missing Moods in Browser Filter 🔴 P0
**Instructions:** Preset browser filter hardcodes 9 moods but there are 16 (Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged, Coupling, Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic). Fix the hardcoded list.

#### S6 — Fix Macro Params Appearing Twice 🔴 P0
**Instructions:** Macros show in both MacroHeroStrip AND ParameterGrid. Remove duplicates from ParameterGrid when macros are displayed in the hero strip.

#### S7 — Replace ParameterGrid with Collapsible Sections 🟢 P2
**Prereqs:** S54 (CollapsibleParamSection component)
**Instructions:** Replace flat ParameterGrid with grouped collapsible sections: OSC/FILTER/MOD/FX. Each section has header + chevron + hero knobs + expandable secondary knobs.

#### S8 — Implement 3 Knob Sizes 🟢 P2
**Instructions:** Macro knobs 80px, section knobs 50px, detail knobs 36px. Implement in XOlokunLookAndFeel::drawRotarySlider with size detection.

#### S9 — Implement GalleryLookAndFeel Knob Style 🟢 P2
**Instructions:** Reference the Audio UI library for knob aesthetics. Warm white bezel, engine accent color indicator, smooth rotation. Implement in drawRotarySlider.

#### S10 — Implement Color-Coded Parameter Sections 🟢 P2
**Instructions:** OSC = engine accent, FILTER = warm gold, MOD = cool blue, FX = muted gray. Section headers and knob bezels tinted by category.

### S11–S18: PlaySurface Implementation

#### S11 — Wire PlaySurface MIDI to Processor 🟡 P1
**File:** `Source/UI/PlaySurface/PlaySurface.h`, `Source/XOlokunProcessor.cpp`
**Instructions:** PlaySurface's MidiMessageCollector must feed into XOlokunProcessor's MIDI input. Verify notes from PlaySurface trigger voices.

#### S12 — Build MPCPadGrid (16 pads, 4 banks) 🟢 P2
**File:** `Source/UI/PlaySurface/MPCPadGrid.h` (create)
**Instructions:** 4×4 grid, pad size scales with container (min 44×44pt), velocity-sensitive color on hit, 4 bank buttons (A/B/C/D) switching MIDI note offset. Wire to PlaySurface MIDI output.

#### S13 — Build SeaboardKeyboard (MPE) 🟢 P2
**Instructions:** MPE per-note pitch/slide/pressure. Channel assignment. Visual feedback.

#### S14 — Build ExpressionPanel 🟢 P2
**Instructions:** Mod wheel + pitch bend + XY pad + macro strips. MIDI CC output.

#### S15 — Build PlaySurfaceContainer 🟢 P2
**Instructions:** Tab switching between surfaces. Auto-expand for drum engines.

#### S16 — Scale Quantization 🟢 P2
**Instructions:** Free / Scale / Chord modes. Snap strength 0.0-1.0. Wire to ouie_interval (B026).

#### S17 — Touch Ripple Visual Feedback 🟢 P2
**Instructions:** On pad hit, expanding circle from center, accent color, alpha fades 0.6→0 over 120ms, 60fps timer. Multiple simultaneous ripples.

#### S18 — Wire MIDI Learn to PlaySurface 🟢 P2
**Instructions:** Right-click → "Assign MIDI CC" → wait for input → bind. Store in plugin state. Clear via right-click.

### S19–S25: DSP / Engine Polish

#### S19 — Fix 490 Presets with Silently-Dropped Params 🔴 P0
**Prereqs:** Read `Source/Core/PresetManager.h` for `frozenPrefixForEngine`
**Instructions:** When engines were renamed, ~490 presets have parameters with old prefixes that silently don't load. Write a migration script `Tools/migrate_preset_prefixes.py` that reads each .xometa, maps old prefixes to new via the frozen prefix table, rewrites the file. Verify before/after preset param counts match. **This is data integrity — P0.**

#### S20 — Padé Approximation for Fat's ZDF Ladder ⚪ P3
**File:** `Source/Engines/Obese/ObeseEngine.h`
**Instructions:** Replace the current filter coefficient computation with Padé approximation for better accuracy at high frequencies. Performance optimization.

#### S21 — Drift Tidal Pulse Lookup Table ⚪ P3
**File:** `Source/Engines/Odyssey/OdysseyEngine.h`
**Instructions:** Replace per-sample tidal pulse sine computation with a wavetable lookup. Reduces CPU.

#### S22 — ScopedNoDenormals Fleet Sweep 🟢 P2 (PARTIAL)
**Instructions:** Search all engines for `processBlock` or `renderNextBlock` methods. Add `juce::ScopedNoDenormals noDenormals;` at the top of each if missing. Priority: engines with feedback paths (OUROBOROS, ORBWEAVE, OVERLAP, ORGANON, OXBOW). fXBreath already fixed in `0a362679f`.

#### S23 — Per-Engine Parameter Grouping Metadata 🟢 P2
**Instructions:** Add metadata to each engine declaring which params belong to OSC/FILTER/MOD/FX groups. Used by CollapsibleParamSection (S54) and color-coding (S10). Could be a static method `getParamGroups()` on each engine.

#### S24 — Cache OWARE Per-Sample Coefficients 🟢 P2
**File:** `Source/Engines/Oware/OwareEngine.h`
**Instructions:** Add dirty flags to OWARE's coefficient computation. Only recompute when params change, not every sample. Significant CPU savings.

#### S25 — Verify 3 Provisional Blessings 🟢 P2
**Instructions:** Verify B007 (velocity coupling, OUROBOROS), B033 (living tuning, OWARE), B034 (sympathetic network, OWARE) work correctly in code. Read each engine, trace the blessed feature, confirm it affects audio output. Report any broken implementations.

### S26–S33: XPN Pipeline

#### S26 — Fix Outshine KeyTrack Toggle 🟢 P2
**File:** `Source/UI/Outshine/` (find the toggle)
**Instructions:** Kai's H01: KeyTrack must be locked to True for MPC compatibility. Replace toggle with read-only badge showing "KeyTrack: ON".

#### S27 — Fix Originate Default to 44.1kHz 🟢 P2
**Instructions:** Kai recommendation. Originate should default to 44.1kHz (MPC standard), not whatever the system sample rate is.

#### S28 — Build MPCe Quad-Corner Assignment Panel 🟢 P2
**Instructions:** 4-quadrant visual panel for assigning engine presets to MPCe corners. In Outshine UI.

#### S29 — Wire Rebirth Mode (B040) into Outshine UI 🟢 P2
**Instructions:** B040 (note duration as synthesis param, OXYTOCIN). Surface this in Outshine's export options so XPN packs can leverage it.

#### S30 — Add Sonic DNA Mini-Radar per Sample 🟢 P2
**Instructions:** In Outshine Auto Mode, show a 24×24pt hexagon DNA radar per sample. Uses the 6D values from .xometa.

#### S31 — Build Oxport Dashboard 🟢 P2
**Instructions:** Visual companion for the XPN export pipeline. Pack overview, progress, validation.

#### S32 — Implement "Send to MPC" USB Copy 🟢 P2
**Instructions:** Detect MPC via USB, copy validated XPN pack, confirm success.

#### S33 — Fix Outshine/Originate Aesthetic 🟢 P2
**Instructions:** Match XOlokun dark theme. Gallery Model consistency.

### S34–S39: Site & Web

#### S34 — Populate 24 KC Creature Sprites 🟢 P2
**Skill:** `/pixel-artist`
**Instructions:** Generate pixel art sprites for all 24 Kitchen Collection engines. Add to aquarium.

#### S35 — Build XOlokun Cultural Acknowledgment Page 🟢 P2
**Prereqs:** O33 complete (copy written)
**Instructions:** Build the page on XO-OX.org. Respectful design.

#### S36 — Update XO-OX.org Branding to XOlokun 🟢 P2
**Instructions:** Find all "XOmnibus" references on XO-OX.org, replace with XOlokun. Update logos, titles, descriptions.

#### S37 — Build Patreon Tier Page 🟢 P2
**Instructions:** Implement tier page on XO-OX.org with Kitchen Collection unlock progression visual.

#### S38 — Implement Field Guide Template 🟢 P2
**Instructions:** Reusable template for Field Guide posts. Header, hero image slot, engine accent color theming, audio player embeds, related posts.

#### S39 — Build XPN Pack Download Page 🟢 P2
**Instructions:** Download page for mpce-perc-001 and future packs. Pack info, demo clips, download button.

### S40–S44: Documentation

#### S40 — Write JUCE Implementation Guide 🟢 P2
**Instructions:** Based on spatial architecture spec Appendix D. Maps design spec to JUCE components, method overrides, layout strategy.

#### S41 — Write PlaySurface MIDI Integration Guide 🟢 P2
**Instructions:** How PlaySurface MIDI flows through the system: UI → MidiMessageCollector → Processor → Engine voice allocation.

#### S42 — Update Master Spec for XOlokun 🟡 P1
**File:** `Docs/xomnibus_master_specification.md`
**Instructions:** Update all XOmnibus references to XOlokun. Verify engine count matches 73. Update any stale sections.

#### S43 — Write V1 Changelog 🟢 P2
**Skill:** `/changelog-generator`
**Instructions:** From all session git logs, produce a producer-facing changelog for V1.

#### S44 — Document All Blessings with Evidence 🟢 P2
**Instructions:** 40 blessings + 5 debates + 6 doctrines. Each blessing: description, engine, ghost votes, evidence (code location or commit). Output: `Docs/blessings-registry.md`

### S45–S50: Testing & QA

#### S45 — Run auval at 5 Sample Rates 🟡 P1
**Instructions:** `auval -v aumu Xolk XoOx` at 11025/44100/48000/96000/192000 Hz. Record pass/fail for each. Fix any failures.

#### S46 — Test Preset Loading for All 73 Engines 🟡 P1
**Instructions:** For each engine, load its first preset via PresetManager, verify no crash, verify param values populated. Script: iterate engines, load first .xometa, report failures.

#### S47 — Test All 15 Coupling Types 🟢 P2
**Instructions:** For each coupling type in MegaCouplingMatrix, pair 2 engines, set coupling amount to 0.5, render 512 samples, verify output differs from uncoupled. Report any types that produce no audible difference.

#### S48 — Test PlaySurface MIDI in GarageBand 🟢 P2
**Instructions:** Manual test: open XOlokun in GarageBand, play PlaySurface pads, verify MIDI reaches engines. Note any latency or dropped notes.

#### S49 — Build CI/CD Pipeline 🟢 P2
**File:** `.github/workflows/build.yml` (create)
**Instructions:** macOS-14 runner, Ninja, cmake configure+build, auval pass/fail, commit status. Trigger on push to main + PRs.

#### S50 — Regression Test: Coupling Accumulator Reset 🟢 P2
**File:** `Tests/CouplingTests.cpp` (create)
**Instructions:** Instantiate 2 engines, set ENTANGLE, render 512 samples, call reset(), verify accumulators return to zero within 1 sample.

### S51–S60: V1 Spatial Architecture JUCE Implementation

#### S51 — XOlokunLookAndFeel Class 🟡 P1
**Prereqs:** O38/O91 design spec complete
**File:** `Source/UI/XOlokunLookAndFeel.h` (create)
**Instructions:** Override drawRotarySlider (3 knob sizes), load fonts via BinaryData, map design-token colors. **Prereq for all UI work.**

#### S52 — EngineRackPanel (Column A) 🟡 P1
**File:** `Source/UI/EngineRackPanel.h` (create)
**Instructions:** 260pt wide, 4 slot tiles at 90pt each. Wire to EngineRegistry for load/unload. Per O51 spec.

#### S53 — EngineDetailPanel Base Class (Column B) 🟡 P1
**File:** `Source/UI/EngineDetailPanel.h` (create)
**Instructions:** Sticky 88pt macro strip, 32pt signal flow, scrollable sections, sticky waveform+ADSR bottom. Wired to active slot selection.

#### S54 — CollapsibleParamSection 🟡 P1
**File:** `Source/UI/CollapsibleParamSection.h` (create)
**Instructions:** Header + chevron, hero knobs (3 @ 50pt), hidden secondary, 150ms height animation.

#### S55 — SectionNavBar 🟢 P2
**File:** `Source/UI/SectionNavBar.h` (create)
**Instructions:** 32pt bar with [OSC][FILT][MOD][FX][★] pills, click scrolls to section, accent color highlight.

#### S56 — MiniCouplingGraph 🟢 P2
**File:** `Source/UI/MiniCouplingGraph.h` (create)
**Instructions:** 120×80pt, 4 nodes as 8pt dots, bezier arcs, brightness ∝ energy, click opens popover.

#### S57 — CouplingArcPopover 🟢 P2
**File:** `Source/UI/CouplingArcPopover.h` (create)
**Instructions:** Anchored to arc midpoint, 3-type quick-select + "More..." + amount + depth knobs. Writes to MegaCouplingMatrix. Dismiss on outside click.

#### S58 — SignalFlowDiagram (Variant A) 🟢 P2
**File:** `Source/UI/SignalFlowDiagram.h` (create)
**Instructions:** 520×32pt strip, auto-generated blocks from param categories, clickable, accent color glow.

#### S59 — Column C Tab Container 🟢 P2
**File:** `Source/UI/ColumnCPanel.h` (create)
**Instructions:** [PRESET][COUPLE][FX][PLAY][EXPORT][⚙] tabs, 32pt tab strip, icon+label.

#### S60 — PresetBrowserTab 🟢 P2
**File:** `Source/UI/PresetBrowserTab.h` (create)
**Instructions:** Type filter pills, mood pills, search, preset cards with DNA radar, "Find Similar". Wire to PresetManager.

### S61–S70: PlaySurface & MIDI

#### S61 — MPCPadGrid Component 🟢 P2
See S12 (same track, different numbering).

#### S62 — MIDI Learn for PlaySurface 🟢 P2
See S18.

#### S63 — getStateInformation / setStateInformation 🔴 P0
**File:** `Source/XOlokunProcessor.cpp`
**Instructions:** Serialize: engine slot assignments, all param values, coupling routes (type/amount/depth), macro positions, PlaySurface MIDI CC assignments. Must round-trip with Logic Pro save/restore. **Critical for DAW compatibility.**

#### S64 — A/B Preset Comparison 🟢 P2
**File:** `Source/Core/PresetManager.h`
**Instructions:** storeA/storeB/swapAB/getActive. A/B toggle in header. Persists across browser navigation.

#### S65 — DAWAutomationExporter 🟢 P2
**File:** `Source/Core/DAWAutomationExporter.h` (create)
**Instructions:** Expose 65 automation-eligible params with proper getName/getLabel/getDefaultValue. Must appear correctly in Logic/Ableton automation lanes.

#### S66 — ONSET EngineDetailPanel (111 params) 🟡 P1
**Prereqs:** O61 design spec + S53 base class + S54 collapsible sections
**Instructions:** Hardest custom panel. 8-voice drum grid hero, voice rows, section nav. Verify at 960pt min width.

#### S67 — OBRIX EngineDetailPanel (81 params) 🟡 P1
**Prereqs:** O62 design spec + S53 base class
**Instructions:** Brick Stack View (Variant B), reef ecology indicator, Wave indicator.

#### S68 — OPERA EngineDetailPanel 🟢 P2
**Prereqs:** O69 design spec
**Instructions:** Conductor arc shapes, coherence meter, EmotionalMemory indicator, mode 3-way switch.

#### S69 — OUROBOROS EngineDetailPanel 🟢 P2
**Prereqs:** O64 design spec
**Instructions:** Leash ring slider, topology selector, feedback matrix display.

#### S70 — PlaySurface Auto-Expand for Drum Engines 🟢 P2
**Instructions:** Detect drum engine (ONSET/OFFERING/OBRIX drum mode) in Slot 1 → animate PlaySurface expansion to 220pt over 300ms.

### S71–S80: Persistence, Export & Build

#### S71 — XPNExportPanel (Column C Export Tab) 🟢 P2
**File:** `Source/UI/XPNExportPanel.h` (create)
**Instructions:** Persistent Component (NOT CallOutBox). Pack name, sample rate, format options, progress bar, open-in-Finder. Survives focus loss.

#### S72 — Build Sentinel CI Pipeline 🟢 P2
**File:** `.github/workflows/build.yml` (create)
**Instructions:** GitHub Actions: macOS-14, Ninja, cmake, auval, commit status badge.

#### S73 — Fix ExportDialog Memory Leak 🔴 P0
**File:** `Source/UI/ExportDialog.h`
**Instructions:** Audit for raw pointer ownership. Replace new/delete with unique_ptr. Add JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR. Verify with 10-export stress test.

#### S74 — Fix OverworldEngine Build Error 🔴 P0
**File:** `Source/Engines/Overworld/OverworldEngine.h`
**Instructions:** Overworld has stub files only (6 headers, no processor/CMake). Identify and fix the build error. Confirm cmake --build passes with OVERWORLD registered.

#### S75 — DarkCockpitController 🟢 P2
**Prereqs:** O6/O17 design spec
**File:** `Source/UI/DarkCockpitController.h` (create)
**Instructions:** 5-level opacity via setAlpha(). Triggers: PlaySurface expand, note play, idle 30s.

#### S76 — CinematicModeController 🟢 P2
**Prereqs:** O60 design spec
**File:** `Source/UI/CinematicModeController.h` (create)
**Instructions:** Double-click ColB header → ColA+ColC collapse to 0pt over 300ms, ColB expands. Cmd+Shift+F shortcut.

#### S77 — SingleEngineMode 🟢 P2
**File:** `Source/UI/SingleEngineModeController.h` (create)
**Instructions:** Only Slot 1 active, ColA reduces to 80pt. Toggle via gear menu. OBRIX Pocket parity.

#### S78 — Right-Click Context Menus for Slot Tiles 🟢 P2
**Instructions:** [Load Engine...][Copy State][Paste State][Clear Slot][Set Default]. Copy/Paste as JSON to clipboard.

#### S79 — Right-Click Context Menus for Preset Cards 🟢 P2
**Instructions:** [Load][Favorite ♡][Find Similar][Copy DNA][Add to Set...]. DNA as JSON clipboard.

#### S80 — Keyboard Shortcut System 🟢 P2
**File:** `Source/UI/KeyboardShortcutHandler.h` (create)
**Instructions:** Cmd+Z/Shift+Z (undo/redo), Cmd+S (save), Space (play/stop), Tab (cycle slot), Cmd+B/P/F (browsers), Escape (dismiss).

### S81–S100: Tests, QA & Advanced Features

#### S81 — Regression Test: Coupling Reset 🟢 P2
See S50.

#### S82 — Regression Test: PresetManager Round-Trip 🟡 P1
**File:** `Tests/PresetTests.cpp` (create)
**Instructions:** For each of 73 engines: load first preset, serialize, deserialize, verify all params match ±1e-5f. Fail on any mismatch.

#### S83 — Per-Engine CPU Display 🟢 P2
**Instructions:** CPU% badge in slot tiles, updated 500ms. Amber >5%, red >10%.

#### S84 — PresetPreviewThumbnail (DNA Radar) 🟢 P2
**Instructions:** 24×24pt hexagon with 6 axes scaled by DNA. Cached as juce::Image. Background thread generation.

#### S85 — MIDI Learn Persistence Fix 🟢 P2
**Instructions:** Serialize CC bindings in getStateInformation as JSON array. Unit test: bind CC 7 → obrix_src1Type, save, clear, restore, verify.

#### S86 — Scale Quantization for XOuija 🟢 P2
**Instructions:** Free/Scale/Chord modes. Snap strength 0.0-1.0. Wire ouie_interval (B026).

#### S87 — InstrumentTypeFilter + Migration Script 🟡 P1
**Prereqs:** O97 (taxonomy assignment table)
**Instructions:** Add `"instrument_type"` to .xometa schema. Write `Tools/migrate_instrument_type.py` to assign types to all ~17K presets. Wire filter in PresetBrowserTab.

#### S88 — DAW Session Round-Trip Verify Script ⚪ P3
**Instructions:** `Tools/verify_session_roundtrip.py` — open in Logic via AppleScript, load preset, save session, close, reopen, verify params.

#### S89 — TouchRippleLayer 🟢 P2
See S17.

#### S90 — GlassTooltip (Living Manual) 🟢 P2
**Prereqs:** O43/O99 design spec
**Instructions:** Glassmorphism: blurred background + white overlay. 1.2s hover trigger. Param name, value, range, description. Edge clipping avoidance.

#### S91 — WCAG AA Contrast Enforcement 🟢 P2
**Prereqs:** H71 contrast audit complete
**Instructions:** In XOlokunLookAndFeel: compute luminance contrast for text on accent backgrounds. Override text color if <4.5:1. Static assert table for all 73 accents.

#### S92 — SessionStateManager 🟢 P2
**Instructions:** On editor close: serialize to PropertiesFile. On open: "Restore last session?" toast.

#### S93 — EngineHotSwap 50ms Crossfade 🟡 P1
**File:** `Source/Core/EngineRegistry.h`
**Instructions:** swapEngine() starts 50ms linear crossfade on audio thread. Atomic float for position. Verify no clicks with offline render.

#### S94 — Fix frozenPrefixForEngine Missing Entries 🔴 P0
**File:** `Source/Core/PresetManager.h`
**Instructions:** Audit frozenPrefixForEngine map. Add entries for ALL 73 engines (OUTLOOK `look_`, OXYTOCIN `oxy_`, all Kitchen Collection). Run `Tools/verify_prefix_coverage.py`. **P0 for preset loading.**

#### S95 — PresetsAutoSave 🟢 P2
**Instructions:** After param change (debounced 2s), save to PropertiesFile. On next launch, offer restore if differs.

#### S96 — iOS AUv3 Responsive Layout 🟢 P2
**Instructions:** <768pt: ColA icon-only (40pt), ColC hidden (slide-in), ColB fills. ≥768pt: 3-column (180/360/230). All touch targets ≥44pt.

#### S97 — PresetSetManager (.xoset) 🟢 P2
**Instructions:** Named ordered preset lists stored as .xoset JSON. Right-click → "Add to Set". Program change = set position. 5 factory sets.

#### S98 — ObrixBrickStackView (Signal Flow Variant B) 🟢 P2
**Prereqs:** O62 design spec
**Instructions:** Connected block nodes (48×32pt each), labels from active src*Type, click scrolls to section, drag reorder if routing allows.

#### S99 — CouplingInspector (Column C Tab) 🟢 P2
**Instructions:** Scrollable list of active routes (source→target→type→amount→depth). "Add Route" 3-step wizard. Live-syncs with MiniCouplingGraph.

#### S100 — MPC800Layout 🟢 P2
**Instructions:** 2-column for 800×480pt MPC viewport. ColA 260pt, ColB 540pt, no ColC. Tab row in ColB footer. All controls ≥44pt.

---

## HAIKU TRACKS (H1–H100): Search / Read / Data / Git

All Haiku tracks are read-only audits. Run them as verification passes. Most can be parallelized.

### H1–H10: Codebase Health

#### H1 — Count Engine Registrations 🔴 P0
**Command:** Search `Source/XOlokunProcessor.cpp` for `REGISTER_ENGINE` or equivalent. Count. Must equal 73.

#### H2 — Preset Count per Engine per Mood 🟢 P2
**Command:** Count .xometa files in `Presets/XOlokun/{mood}/{engine}/` for all combinations. Output distribution table.

#### H3 — Verify 73 Engines in CLAUDE.md Table 🔴 P0
**Command:** Count rows in Engine Modules table in `CLAUDE.md`. Must be 73. List any missing.

#### H4 — Verify 73 in frozenPrefixForEngine 🔴 P0
**File:** `Source/Core/PresetManager.h`
**Command:** Count entries in frozenPrefixForEngine. Diff against CLAUDE.md engine list.

#### H5 — Verify 73 in validEngineNames 🔴 P0
**File:** `Source/Core/PresetManager.h`
**Command:** Count entries in validEngineNames. Must be 73.

#### H6 — Count Total Lines in Source/ 🟢 P2
**Command:** `find Source/ -name "*.h" -o -name "*.cpp" | xargs wc -l | tail -1`

#### H7 — Lines per Engine (Complexity Ranking) 🟢 P2
**Command:** Count lines per engine directory in Source/Engines/*/. Rank by LOC.

#### H8 — Engines Missing Guru Bin Retreats 🟢 P2
**Command:** Check `scripture/retreats/` for each engine. List engines without retreat files.

#### H9 — Engines Missing Seance Verdicts 🟢 P2
**Command:** Check `Docs/seances/` + `scripture/seances/`. Should be only OSMOSIS missing.

#### H10 — Verify design-tokens.css Has 73 Colors 🟢 P2
**Command:** Count engine accent color entries in design-tokens.css. Must be 73.

### H11–H20: Git & History

#### H11 — Git Stats 🟢 P2
**Command:** `git log --since="2026-01-01" --pretty=format:"%ai" | cut -d ' ' -f1 | sort | uniq -c | sort -rn | head -20`

#### H12 — Commits Since XOlokun Rename 🟢 P2
**Command:** `git log --oneline --since="2026-03-24"` — list all post-rename commits.

#### H13 — Verify No Uncommitted Files ✅ DONE

#### H14 — Check for Merge Conflict Markers 🔴 P0
**Command:** `grep -rn "<<<<<<\|======\|>>>>>>" Source/` — must be 0 results.

#### H15 — List All Branches and Status 🟢 P2
**Command:** `git branch -a --format='%(refname:short) %(upstream:track)'`

#### H16 — Verify Remote Branch Up to Date 🟢 P2
**Command:** `git fetch origin && git log origin/main..HEAD --oneline`

#### H17 — Count Total .xometa Files 🟢 P2
**Command:** `find Presets/XOlokun/ -name "*.xometa" | wc -l` — should be ~17,250.

#### H18 — Verify Preset Symlink 🟢 P2
**Command:** `ls -la ~/Library/Application\ Support/XO_OX/XOlokun/Presets`

#### H19 — Check for Files >1MB in Git 🟢 P2
**Command:** `git ls-files | while read f; do s=$(wc -c < "$f" 2>/dev/null); [ "$s" -gt 1048576 ] && echo "$s $f"; done`

#### H20 — Verify .gitignore Coverage 🟢 P2
**Command:** Read `.gitignore`, verify `build/`, `*.component`, `*.app`, `*.vst3` present.

### H21–H30: Asset Inventory

#### H21 — Count Fonts in Downloads 🟢 P2
#### H22 — Count Figma Files 🟢 P2
#### H23 — Verify Audio UI Library Completeness 🟢 P2
#### H24 — List Gradient Packs 🟢 P2
#### H25 — Verify Icon Set Catalog 🟢 P2
#### H26 — Check Embedded Fonts in JUCE Build 🟢 P2
#### H27 — Count Research Documents 🟢 P2
#### H28 — Count Sweep Reports 🟢 P2
#### H29 — List HTML Prototypes 🟢 P2
#### H30 — Verify TOOL_REGISTRY.json (230 entries) 🟢 P2

*(Each is a simple count/list command — self-explanatory from the track name.)*

### H31–H40: Memory & Skills

#### H31 — Verify MEMORY.md Under 200 Lines 🟢 P2
**Command:** `wc -l ~/.claude/projects/-Users-joshuacramblet/memory/MEMORY.md`

#### H32 — Count Total Memory Files 🟢 P2
**Command:** `ls ~/.claude/projects/-Users-joshuacramblet/memory/ | wc -l`

#### H33 — List Skills with Trigger Counts 🟢 P2
#### H34 — Verify Skills Say "XOlokun" Not "XOmnibus" 🟡 P1
#### H35 — Check for Duplicate Memory Files 🟢 P2
#### H36 — Verify Blessing Reclassification Committed 🟢 P2
#### H37 — Check Pixel Wishlist vs Ui8 Purchases 🟢 P2
#### H38 — Verify Research Team Skill Structure 🟢 P2
#### H39 — Count Open-Source Reference Guide Entries 🟢 P2
#### H40 — Verify All Sweep Reports Committed 🟢 P2

### H41–H50: Site & Presets

#### H41 — Count Engines in aquarium.html (should be 73) 🟢 P2
#### H42 — Count Creatures in engine-creature-map.json (should be 73) 🟢 P2
#### H43 — Verify design-tokens.css Braces Balanced 🟢 P2
#### H44 — Check All Site HTML Valid Structure 🟢 P2
#### H45 — Count Presets in Quarantine 🟢 P2
#### H46 — List Engines with <50 Presets 🟡 P1
#### H47 — Verify OXYTOCIN's 130 Presets on Disk 🟢 P2
#### H48 — Check for "XOmnibus" in Preset Metadata 🟡 P1
#### H49 — Verify Coupling Presets (.xocoupling) 🟢 P2
#### H50 — Preset Mood Distribution Data 🟢 P2

### H51–H60: Feature Coverage & Implementation Audit

#### H51 — Count Features in Spatial Architecture Spec 🟢 P2
**File:** `Docs/design/xolokun-spatial-architecture-v1.md`
**Instructions:** Count explicitly listed features. Verify "326 features" claim. List any without implementation home.

#### H52 — Existing UI File Inventory with Line Counts 🟡 P1
**Command:** `find Source/UI/ -name "*.h" -exec wc -l {} \; | sort -rn`
**Instructions:** Identify which spatial arch components are stubs vs. need creation.

#### H53 — Count Coupling Type Enum Values (verify 15) 🟢 P2
**File:** `Source/Core/MegaCouplingMatrix.h`

#### H54 — Engines Missing Custom EngineDetailPanel 🟢 P2
**Command:** Search Source/Engines/ for engines without EngineDetailPanel overrides.

#### H55 — frozenPrefixForEngine Completeness Diff 🔴 P0
See H4 (same audit, numbered differently in expansion).

#### H56 — Presets Containing "XOmnibus" in Metadata 🟡 P1
**Command:** `grep -rl "XOmnibus" Presets/XOlokun/` — count and list first 10.

#### H57 — Registered Engines in XOlokunProcessor.cpp 🔴 P0
See H1.

#### H58 — Hardcoded Pixel Values in Source/UI/ 🟢 P2
**Command:** Search for `setBounds(` with literal numbers. Count, list top 10 files.

#### H59 — Check engineType Metadata Field Exists 🟢 P2
**File:** `Source/Core/EngineRegistry.h`

#### H60 — ScopedNoDenormals Coverage Audit 🟢 P2
**Command:** Search for ScopedNoDenormals in Source/DSP/ and Source/Engines/. List present/missing.

### H61–H70: Preset & DNA Coverage Audit

#### H61 — Preset Count Ranked by Engine 🟡 P1
**Command:** Count .xometa per engine. Flag any <10 presets.

#### H62 — Verify DNA Fields in 10 Random Presets 🟢 P2
#### H63 — Count Presets Missing instrument_type 🟢 P2
#### H64 — Preset Distribution by Mood 🟢 P2
#### H65 — Preset Names Exceeding 30 Characters 🟢 P2
#### H66 — Coupling Preset Validation 🟢 P2
#### H67 — Duplicate Preset Name Check 🟢 P2
#### H68 — Total Preset Count Verification (~17,250) 🟡 P1
#### H69 — OXYTOCIN 130 Preset Verification 🟢 P2
#### H70 — Kitchen Collection Preset Count per Engine 🟢 P2

### H71–H80: Coupling, Color & Shortcut Audits

#### H71 — WCAG Contrast Ratio for 73 Accent Colors 🟡 P1
**File:** `Docs/design/design-tokens.css`
**Instructions:** Compute contrast vs #F8F6F3. List any <3.0:1 or <4.5:1. Feeds S91.

#### H72 — Coupling Type Declaration Audit 🟢 P2
#### H73 — Coupling Cookbook Engine Pair Count 🟢 P2
#### H74 — Keyboard Shortcut Conflict Audit 🟢 P2
#### H75 — Coupling Accumulator Reset Path Exists 🟢 P2
#### H76 — TriangularCoupling Only in OXYTOCIN 🟢 P2
#### H77 — FX Suite Type Count (≥3 knobs each) 🟢 P2
#### H78 — cancelScheduledValues + param.value Click Pattern 🔴 P0
**Command:** Search for `cancelScheduledValues` near `param.value` assignment. Count violations.

#### H79 — resolveEngineAlias Coverage (7 legacy names) 🟢 P2
#### H80 — Engines Missing getSupportedCouplingTypes 🟢 P2

### H81–H90: Git, Build & Repo Hygiene

#### H81 — Commits Since 2026-03-24 🟢 P2
#### H82 — "XOmnibus" in Source .cpp/.h Files 🟡 P1
**Command:** `grep -rn "XOmnibus" Source/ --include="*.cpp" --include="*.h"` — should be 0.

#### H83 — XOmnibus in #include Paths 🟢 P2
#### H84 — CMakeLists.txt Version + Architecture Check 🟢 P2
#### H85 — Raw `new` Without unique_ptr in Source/UI/ 🟡 P1
#### H86 — Count .h Files per Engine (verify 73 dirs) 🟢 P2
#### H87 — OverworldEngine Build Error Identification 🔴 P0
**File:** `Source/Engines/Overworld/OverworldEngine.h`

#### H88 — Euler IIR Audit ✅ DONE (committed in `0a362679f`)

#### H89 — Hardcoded 44100 in .xometa Files 🟢 P2
#### H90 — MANIFEST.md Completeness 🟢 P2

### H91–H100: Skills, Memory & Documentation

#### H91 — Memory File Inventory 🟢 P2
#### H92 — Skills README vs Disk Consistency 🟢 P2
#### H93 — Skills Still Saying "XOmnibus" 🟡 P1
#### H94 — Seance Cross-Reference Count (verify 72) 🟢 P2
#### H95 — Seance Verdict Document Count 🟢 P2
#### H96 — Engines with Seance Score <8.0 🟢 P2
#### H97 — True Coupling Preset Count (≥5) 🟢 P2
#### H98 — V1 Engine List Verification 🟡 P1
#### H99 — XPN Export Pipeline Rule Audit 🟢 P2
#### H100 — Master Spec Engine Count + Staleness 🟡 P1

---

## Priority Summary

| Priority | Count | Description |
|----------|-------|-------------|
| 🔴 P0 | ~18 | Blocks other work, data integrity, build errors |
| 🟡 P1 | ~30 | High-impact, unlocks downstream, prereqs for UI |
| 🟢 P2 | ~200 | Important V1 work |
| ⚪ P3 | ~48 | Nice-to-have, V1.1 candidates |

### Recommended Session Starters

**"I want to fix bugs"** → S19 (490 preset migration), S73 (ExportDialog leak), S74 (Overworld build error), S94 (frozen prefix gaps), S5/S6 (UI duplicates)

**"I want to audit the codebase"** → H1-H10 as parallel Haiku agents, then H51-H60

**"I want to build UI"** → O38/O91 (LookAndFeel design) → S51 (implement) → S52/S53/S54 (Column A/B/sections). O1 (V0.2 prototype) runs in parallel.

**"I want to ship presets"** → H61 (count gaps) → `/exo-meta` for thin engines → `/preset-audit-checklist` → `/guru-bin`

**"I want to work on the site"** → O37 (rebrand spec) → S36 (implement) → O36 (Field Guide posts) → S38 (template)

**"I want to prepare for launch"** → O34 (launch sequence) → O84 (press kit) → O83 (Patreon calendar) → O98 (OBRIX campaign)

**"Run a seance"** → O27 (3 Provisional blessings) or O95 (spatial architecture review)

**"Quick wins"** → H14 (conflict markers), H82 (XOmnibus in source), S5/S6 (browser fixes), H78 (click pattern audit)
