> ⚠️ **RETIRED — DO NOT USE AS A LIVE PLANNING DOC (2026-04-16)**
>
> The "launch battle plan" / "path to V1" framing has been retired. XOceanus does not
> operate on a fixed-version release model. Build and refine until ready; ship when ready.
> This document is preserved for historical reference only. See `CLAUDE.md` → *Release Philosophy*.

---

# XOceanus Launch Battle Plan — Path to 10/10 (HISTORICAL)

## Top 50 Opus Tracks (Creative / Architecture / Design)

### UI Implementation (THE priority)
1. Build Rebirth Prototype V0.2 — depth-zone dials, Audio UI knob aesthetics, OpenBridge density, APERTURE font
2. Build Rebirth Prototype V0.3 — full interaction (draggable knobs, coupling arc click, preset browse)
3. Design the CouplingVisualizer center panel layout — exact node positions, arc rendering spec
4. Design the Depth-Zone Engine Browser overlay — ocean cross-section with 73 creature dots
5. Design the Preset DNA Browser — mood tiles + hexagon cards + spatial navigation
6. Design the Dark Cockpit opacity behavior — trigger conditions, transition timing
7. Design the Performance Mode layout — macros 3x, coupling center, PlaySurface bottom
8. Vision Quest VQ-UI-003: "111 parameters, 4 visible" — the progressive disclosure architecture
9. Vision Quest VQ-UI-004: Engine personality — each engine LOOKS different
10. Vision Quest VQ-UI-005: The aquarium as interface — mythology IS navigation

### PlaySurface (Make it playable)
11. Design the XOuija Planchette — Lissajous idle drift, spring lock-on, bioluminescent trail spec
12. Design the Seaboard continuous keyboard — MPE visual language, glissando/portamento gesture
13. Design the Tide Controller — water simulation physics, ripple-to-modulation mapping
14. Design the Expression Panel layout — mod wheel, pitch bend, XY pad, macro strips, all together
15. Design the PlaySurface Container — tab switching, surface selection UX

### Creative Features
16. Implement B026 (Interval-as-Parameter for OUIE) — design the `ouie_interval` parameter
17. Implement B041 (Dark Cockpit) — design the opacity hierarchy system
18. Implement B042 (Planchette autonomy) — design Lissajous + spring + warm memory behavior
19. Implement B043 (Gesture Trail as modulation) — design the DSP signal path from ring buffer
20. Design the Constellation View (25% push) — 76 engines as interactive star chart
21. Design Spatial Preset Navigation (25% push) — 19K presets as continuous 2D map
22. Design the Emotion-Responsive UI (25% push) — color temperature adapts to audio
23. Design the "Sound on First Launch" init preset — which engines, which coupling, what sound

### Seances & Creative Reviews
24. Synth Seance on the V0.2 prototype — ghosts review the actual design
25. Producers Guild review of V0.2 — does it pass the 30-second test for all 10 genres?
26. UIX Studio review of V0.2 — Ulf/Issea/Xavier/Lucy feedback loop
27. Seance the 3 Provisional blessings (B007, B033, B034) — verify or revoke
28. Kai review of the final PlaySurface MPC pad implementation

### XPN / Outshine / Originate
29. Design the Oxport Dashboard companion tool (Kai's recommendation)
30. Redesign the Outshine UI to match XOceanus's Rebirth aesthetic
31. Design the "Send to MPC" USB workflow
32. Design the MPCe quad-corner assignment panel

### Brand & Launch
33. Write the XOceanus cultural acknowledgment (Olokun orisha) — get external review
34. Design the V1 launch sequence — press kit, timeline, announcement
35. Design the Patreon tier benefits with the new brand
36. Write the first 3 Field Guide posts for XOceanus launch
37. Design the XO-OX.org rebrand from XOceanus to XOceanus

### Architecture
38. Design the JUCE LookAndFeel architecture for the Rebirth (Lucy consult)
39. Design the iOS AUv3 UI adaptation strategy (Xavier consult)
40. Design the standalone app UI (needs the custom editor, not generic)
41. Architecture for mounting the 93K lines of unmounted UI code
42. Design the preset save/export workflow UX

### Innovation
43. Design the Living Manual (glassmorphism contextual tooltips)
44. Design the Spectral Silhouette (engine identity as background visual DNA)
45. Theorem session — generate 3 new concepts for V1.1 features
46. Design the Coupling Gradient XPN export (4 coupling depths per pad)
47. Design the Complement Chain intelligence (Visionary Legend Feature 7)

### Community & Growth
48. Artist Collaboration framework — design the first collab workflow
49. Community strategy for launch — Barry OB's team deployment plan
50. Design the XOceanus tutorial/onboarding for first-time users

---

## Top 50 Sonnet Tracks (Implementation / Build / Documentation)

### UI Implementation
1. Implement the CouplingVisualizer as XOceanusEditor's default center panel
2. Mount the PlaySurface in XOceanusEditor (it's built, just hidden)
3. Mount the ExportDialog (it's built, no button to open it)
4. Mount the full PresetBrowser (with DNA "Find Similar", replaces stripped version)
5. Fix the 7 missing moods in the preset browser filter (hardcoded 9 → 16)
6. Fix macro params appearing twice (MacroHeroStrip AND ParameterGrid)
7. Replace the ParameterGrid with grouped collapsible sections
8. Implement 3 knob sizes (macro 80px, section 50px, detail 36px)
9. Implement the GalleryLookAndFeel knob style from Audio UI library reference
10. Implement color-coded parameter sections (OSC/FILTER/MOD/FX)

### PlaySurface Implementation
11. Wire PlaySurface MidiMessageCollector to XOceanusProcessor's MIDI input
12. Build MPCPadGrid component (16 pads, MPC note mapping, velocity curves, 4 banks)
13. Build SeaboardKeyboard component (MPE, per-note pitch/slide/pressure)
14. Build ExpressionPanel (mod wheel, pitch bend, XY pad, macro strips)
15. Build PlaySurfaceContainer (tab switching between surfaces)
16. Implement scale quantization with adjustable snap strength for fretless
17. Implement touch ripple visual feedback on pad hit
18. Wire MIDI learn to all PlaySurface controls

### DSP / Engine Polish
19. Fix the 490 presets with silently-dropped parameters (prefix migration)
20. Implement the Padé approximation utility for Fat's ZDF ladder (performance)
21. Implement the Drift tidal pulse optimization (lookup table)
22. Add ScopedNoDenormals to any remaining engines without it (fleet sweep)
23. Implement per-engine parameter grouping metadata (which params = OSC/FILTER/MOD/FX)
24. Cache OWARE's remaining per-sample coefficients with dirty flags
25. Verify all 3 Provisional blessings work correctly (B007, B033, B034)

### XPN Pipeline
26. Fix the Outshine KeyTrack toggle → locked read-only badge (Kai's H01)
27. Fix Originate default to 44.1kHz (Kai's recommendation)
28. Build the MPCe quad-corner assignment panel for Outshine
29. Wire Rebirth Mode (B040) into Outshine UI
30. Add Sonic DNA mini-radar per sample in Outshine Auto Mode
31. Build the Oxport Dashboard visual companion
32. Implement the "Send to MPC" USB copy workflow
33. Fix the Outshine/Originate aesthetic to match XOceanus dark theme

### Site & Web
34. Populate the 24 Kitchen Collection creature sprites for aquarium
35. Build the XOceanus cultural acknowledgment About page
36. Update XO-OX.org branding from XOceanus to XOceanus
37. Build the Patreon tier page with XOceanus branding
38. Implement the Field Guide template for XOceanus posts
39. Build the XPN pack download page

### Documentation
40. Write the JUCE implementation guide based on definitive UI spec Appendix D
41. Write the PlaySurface MIDI integration guide
42. Update the master specification for XOceanus
43. Write the V1 changelog from all session git logs
44. Document all 36 Blessed + 3 Provisional + 4 Candidate blessings with evidence

### Testing & QA
45. Run auval at all 5 sample rates (11025/44100/48000/96000/192000)
46. Test preset loading for all 76 engines (1 preset per engine minimum)
47. Test coupling between all 15 types with 2 engines each
48. Test PlaySurface MIDI output in GarageBand
49. Build a CI/CD pipeline for automated build + auval on push
50. Create a regression test for the coupling accumulator reset pattern

---

## Top 50 Haiku Tracks (Search / Read / Data / Git)

### Codebase Health
1. Count exact engine registration in XOceanusProcessor.cpp (verify 73)
2. Count exact preset count per engine per mood (distribution report)
3. Verify all 76 engines have entries in CLAUDE.md Engine Modules table
4. Verify all 76 engines have entries in frozenPrefixForEngine
5. Verify all 76 engines have entries in validEngineNames
6. Count total lines of code in Source/ (baseline metric)
7. Count total lines in each engine (complexity ranking)
8. List all engines still missing Guru Bin retreats
9. List all engines missing seance verdicts (should be only OSMOSIS)
10. Verify design-tokens.css has all 76 engine accent colors

### Git & History
11. Generate git stats (commits per day, lines added/removed)
12. List all commits since the XOceanus rename
13. Verify no uncommitted files on main
14. Check for any lingering merge conflict markers in source
15. List all branches and their status (stale, merged, active)
16. Verify the remote branch `claude/dual-engine-integration-uBjia` is up to date
17. Count total .xometa files in Presets/XOceanus/
18. Verify preset symlink at ~/Library/Application Support/XO_OX/XOceanus/Presets
19. Check for any files larger than 1MB that shouldn't be in git
20. Verify .gitignore covers build/, *.component, *.app

### Asset Inventory
21. Count total fonts downloaded to ~/Downloads/ (all font folders)
22. Count total Figma .fig files downloaded
23. Verify Audio UI library is complete (93 knobs, 9 sets, 8 buttons, etc.)
24. List all gradient packs and their themes
25. Verify all icon sets are catalogued in asset-registry.md
26. Check which purchased fonts have been embedded in the JUCE build
27. Count total research documents in Docs/research/
28. Count total sweep reports in Docs/
29. List all HTML prototypes in Docs/mockups/
30. Verify TOOL_REGISTRY.json has 230 entries

### Memory & Skills
31. Verify MEMORY.md is under 200 lines
32. Count total memory files
33. List all skills with their trigger counts from session logs
34. Verify all skill descriptions mention "XOceanus" (not just "XOceanus")
35. Check for any duplicate memory files
36. Verify blessing reclassification is committed
37. Check that Pixel's wishlist is up to date with latest Ui8 purchases
38. Verify Research Team skill has correct hub-spoke structure
39. Count total entries in open-source-reference-guide.md
40. Verify all sweep reports are committed to main

### Site & Presets
41. Count engines in aquarium.html (should be 73)
42. Count creatures in engine-creature-map.json (should be 73)
43. Verify design-tokens.css braces are balanced
44. Check all site HTML files for valid structure
45. Count presets in quarantine (are any recoverable?)
46. List engines with <50 presets (thin coverage)
47. Verify Oxytocin's 130 presets are in Presets/XOceanus/Oxytocin/
48. Check for any presets still referencing "XOceanus" in metadata
49. Verify coupling presets (.xocoupling) if any exist
50. Generate a preset mood distribution pie chart data (counts per mood)

---

## EXPANSION: 150 Additional Tracks (Added 2026-03-24)

### Top 50 Opus Tracks (Creative / Architecture / Design)
Numbers 51–100

#### V1 Spatial Architecture UI Design (Per Section)

51. Design the Column A Engine Rack tile spec — exact pixel layout for each 260×90pt slot tile: on/off toggle, engine name (Space Grotesk 14pt), waveform thumbnail (80×24pt), CPU% badge, swap affordance. Produce a Figma-ready annotated mockup.
52. Design the Column B Engine Detail panel for "standard" engines (≤40 params): 4 macro knobs sticky top (88pt strip), signal flow diagram (32pt), 4 collapsible param sections (OSC/FILTER/MOD/FX), waveform+ADSR sticky bottom. Define all spacing tokens.
53. Design the Column B Engine Detail panel for the 17 "custom" engines (60–111 params): jump-to-section nav bar, section collapse defaults, which params surface as hero knobs vs. secondary. Start with ONSET (111 params) as the hardest case.
54. Design the Column C Preset Browser tab: instrument-type filter pill row (DRUMS/BASS/PADS/KEYS/LEAD/FX), mood pill row, DNA radar mini-card per preset, "Find Similar" DNA proximity sort. Define scroll behavior and empty-state.
55. Design the Column C Coupling Inspector tab: source→target arc list, inline type/amount/depth per route, 3 quick-start types with plain-language descriptions (Entangle/Gravitational/Phase), "More..." expansion to full 15. Must match inline arc popover values in real-time.
56. Design the Column C FX tab: 6 FX slot list (SAT/DELAY/REVERB/MOD/COMP/SEQ), inline expand per slot showing parameter knobs, bypass toggle, slot reorder handle. Per-slot CPU cost badge.
57. Design the mini coupling graph (120×80pt) at the bottom of Column A: 4 engine nodes as dots, coupling arcs with brightness proportional to coupling energy, arc click target (≥16pt), plain-text label below active arc. Idle/no-coupling state.
58. Design the inline coupling arc popover: anchors to the arc midpoint, shows type selector (3 + "More..."), amount knob, depth knob, close affordance. Must not occlude signal flow diagram. Define animation (200ms spring, no bounce).
59. Design the Performance Mode layout: Column A/B/C dimmed at 45%/20%/20%, macros enlarged to 48pt in header, PlaySurface 220pt full width. Define exactly which controls remain interactive vs. purely visual during performance.
60. Design the CinematicMode full-width expansion: ColA + ColC collapse to 0pt with 300ms ease-out, ColB expands to 1100pt. Define collapse trigger (double-click ColB header), re-expand (same), and what ColB shows in 1100pt vs. 520pt.

#### Per-Engine Custom UI Panels (17 engines with 60–111 params)

61. Design the ONSET custom panel (111 params, `perc_` prefix): 8-voice drum grid as hero (280pt), per-voice algorithm selector, section nav DRUM/OSC/FILTER/MOD/FX. The drum grid IS the primary interaction surface. Identify the 12 hero params that surface without expanding any section.
62. Design the OBRIX custom panel (81 params, `obrix_` prefix): Brick Stack View as signal flow Variant B, modular slot-flow with drag connections, reef ecology status display (reefResident mode indicator), Wave indicator showing active wave number. Hero params: src1Type, src2Type, fxMode, reefResident.
63. Design the OBLONG (BOB) custom panel (72 params, `bob_` prefix): surface the 5 core synthesis layers as section tabs, identify the 8 hero knobs for the top macro strip, define which params collapse by default.
64. Design the OUROBOROS custom panel (`ouro_` prefix, Variant C network): circular feedback topology icon, leash mechanism slider as a distinct visual element separate from standard knobs, chaos attractor visualization (32pt strip). Hero params: topology, leash, chaos.
65. Design the ORGANON custom panel (`organon_` prefix, Variant C network): metabolic rate as a prominent biological heartbeat display, variational free energy meter, 4-metabolism mode visual indicator. Reference the B011 "publishable as paper" architecture.
66. Design the ORBWEAVE custom panel (`weave_` prefix, Variant C network): knot topology matrix as a 4×4 grid showing active Trefoil/Figure-Eight/Torus/Solomon nodes, MACRO KNOT morph arc as a gestural sweep zone. Variant C expand-to-CinematicMode routing view spec.
67. Design the OPENSKY custom panel (`sky_` prefix): RISE macro as a vertical ascension gesture strip (the single-gesture sweep of pitch env + filter + shimmer), Shepard shimmer depth display. Hero params: rise, shimmerDepth, chorusWidth.
68. Design the OUIE custom panel (`ouie_` prefix): HAMMER bipolar axis as a horizontal STRIFE↔LOVE slider (the defining interaction), dual-voice algorithm selectors (Voice A / Voice B, 8 algorithms each), interval display showing current musical interval between voices.
69. Design the OPERA custom panel (`opera_` prefix): Conductor arc shape selector (4 arc types), arc time/peak position controls, coherence meter (Kuramoto R value 0–1) as a live readout, EmotionalMemory indicator showing phase persistence state. Conductor mode toggle (Auto/Manual/Both) as a prominent 3-way switch.
70. Design the OCEANDEEP custom panel (`deep_` prefix): Hydrostatic pressure meter as depth gauge (0–100 fathoms), Darkness Filter ceiling display (50–800 Hz range emphasizing the constraint as identity), Bioluminescent Exciter visualization. Hero param: pressure, filterCeiling, bioExcite.
71. Design the OFFERING custom panel (`ofr_` prefix): City mode selector as a 5-city map icon row (NY/Detroit/LA/Toronto/Bay Area), Berlyne curiosity meter, per-drum-type display (8 voice types). B038 psychology layer indicator showing current Wundt optimal arousal zone.
72. Design the OXYTOCIN custom panel (`oxy_` prefix): Love triangle display (3-lineage visual: circuit/love/chemistry), intimacy accumulator bar (grows with note duration — B040), note duration warmth indicator. 5 legend lineage selector (RE-201/MS-20/Moog/Serge/Buchla).
73. Design the ORGANISM custom panel (`org_` prefix, Variant C): cellular automata grid display (live rule set visualization), ruleSet selector, emergence meter. CA grid should animate with audio output — cells flash on transient.
74. Design the OBESE (FAT) custom panel (`fat_` prefix): Mojo orthogonal axis as a 2D XY pad (analog warmth vs. digital precision — B015), satDrive as a VU-style analog meter. After B015 fix, Mojo must be the primary interaction surface.
75. Design the ORACLE custom panel (`oracle_` prefix): GENDY stochastic parameter display, Maqam scale grid (showing active microtonal tuning), breakpoints as an editable curve segment display. B010 reference: Buchla's 10/10.

#### Sound on First Launch, Content & Brand

76. Design "Sound on First Launch" — the exact OXBOW "First Breath" init preset spec: OXBOW in Slot 1 + OPERA in Slot 2, ENTANGLE coupling at 0.35, macro positions, what the user hears in the first 3 seconds. Write the .xometa spec and confirm the fallback hard-coded init patch parameters.
77. Design the Depth-Zone Engine Browser overlay (73 creatures as ocean cross-section): ocean zones (sunlight/twilight/midnight/abyssal/hadal), 73 creature dot positions by zone affinity, hover tooltip (engine name + accent color + 6-word description), click to load into selected slot. Produce zone-to-engine assignment for all 73.
78. Design the OBRIX Pocket standalone app: single-engine window (600×480pt), full 81 params accessible, Brick Stack View prominent, no coupling graph (single engine), preset browser sidebar. Mobile-first touch targets (44pt minimum). This is the first standalone app spec.
79. Curate the V1 "Sound on First Launch" preset chain — the 5 coupling presets that ship as the first user experience: (1) OXBOW+OPERA Entangle, (2) ONSET+OFFERING Gravity, (3) OBRIX+OUROBOROS Phase, (4) OPENSKY+ORBWEAVE Knot, (5) ORGANON+ORGANISM Adversarial. Write all 5 .xometa coupling preset specs.
80. Write Field Guide post #4: "The 15 Coupling Types Explained" — practical producer guide with audio examples, what each type sounds like, which engine pairs work best. Target: 3,500 words, 15 audio examples. This is the most-searched topic for new users.
81. Write Field Guide post #5: "Inside OBRIX — Reef Residency and the 5 Waves of a Synthesizer" — deep dive on OBRIX's 5-wave architecture. Covers Brick Independence (B016), Biophonic Synthesis, and Reef Residency. Target: 4,000 words with signal flow diagrams.
82. Write Field Guide post #6: "XOceanus for Producers — 5 Genre Starter Templates" — hip-hop (ONSET+OFFERING), ambient (OXBOW+OPERA), drum & bass (ONSET+OBRIX), cinematic (ORGANON+OPENSKY), lo-fi (OWARE+OVERDUB). Coupling presets for each template included.
83. Design the Patreon content calendar for Month 1 post-launch: Week 1 (release announcement + OBRIX deep dive), Week 2 (first patron-exclusive preset pack 50 presets), Week 3 (Kitchen Collection preview video), Week 4 (community poll: which Kitchen quad unlocks first). Include exact post copy and asset list.
84. Design the V1 press kit: one-paragraph product description, 5 key differentiators (not feature lists — differentiators), 3 quote-ready sound designer endorsements, 6 hero screenshots spec (exact scenes), video script outline (90-second demo). Write all copy.
85. Design the XO-OX.org XOceanus rebrand: homepage hero section copy, aquarium page intro text (76 engines, ocean mythology frame), Field Guide landing page restructure. Write all copy. No design tool needed — pure content.

#### Architecture, Community & Growth

86. Theorem session: generate 3 new engine concepts for V1.1 — each must address a gap in the current 73 (missing: granular chaos, physical modeling of metals, spectral morphing voice). Produce concept brief for each: name, O-word, accent color, 3-sentence mythological identity, 6 blessing candidates, coupling compatibility list.
87. Design the Artist Collaboration framework for the first external collab: identify 3 target producers (genre coverage: electronic/hip-hop/ambient), define deliverable (20 presets + 1 Field Guide post + 1 video), define compensation model (revenue share or credit). Produce the outreach email template.
88. Design the Coupling Recipe system: define what a "recipe" is (engine pair + coupling type + macro positions + intent description), format spec for a .xorecipe file, the 5 starter recipes that ship with V1. Write the format spec and all 5 recipes.
89. Design the XPN Pack product page for mpce-perc-001: pack name, tagline, 3 demo clips spec, track list (8 kits), pricing, download CTA. Write all copy for the XO-OX.org page.
90. Design the community launch strategy for Discord server: channel structure (announcements / releases / sound-design / presets / coupling / field-guide / off-topic), moderation roles, Week 1 content calendar (5 seeding posts), first community challenge brief ("make a track with OBRIX + one coupling"). Write channel descriptions and Week 1 posts.
91. Design the JUCE LookAndFeel architecture: class hierarchy (XOceanusLookAndFeel extends juce::LookAndFeel_V4), override list (drawRotarySlider, drawButtonBackground, drawComboBox, drawTabButton, drawScrollbar — minimum 12 overrides needed), font loading strategy (Space Grotesk via BinaryData), color token mapping from design-tokens.css to juce::Colours. Produce the class skeleton with all method signatures.
92. Design the iOS AUv3 UI adaptation strategy: which components require touch-target expansion (44pt minimum), which layouts collapse (3-column → 2-column on iPad portrait), which features defer to V2 (CinematicMode, Depth-Zone Browser), what the minimum viable iPad layout looks like at 1024×768pt. Produce a 3-tier feature matrix (V1 iOS / V1.1 iOS / V2 iOS).
93. Design the progressive disclosure onboarding for first-time users: 5-step contextual tooltip sequence (Step 1: "This is OBRIX — turn macro M1 CHARACTER", Step 2: "This is a coupling arc — click it", Step 3: "Browse presets by mood", Step 4: "Load a second engine", Step 5: "You're ready"). Write all tooltip copy and trigger conditions.
94. Design the V1.1 feature slate: per-knob mod rings and drag-from-source-to-target gestures (deferred from V1 per §3.2), Constellation View (25% push), Spatial Preset Navigation (25% push). For each: one-paragraph scope, 3 acceptance criteria, estimated session count.
95. Seance the V1 spatial architecture spec itself: convene the ghost council to review the 3-column layout, column proportions (260/520/320), the coupling arc popover resolution, and the PlaySurface auto-expand behavior. Produce a verdict with 3 demanded changes and 3 blessings.
96. Producers Guild review of the V1 launch preset list: do the 5 coupling presets (track #79) pass the 30-second test for hip-hop, ambient, electronic, cinematic, and experimental producers? Score each 1–10 and list required changes.
97. Design the Instrument Browser filter taxonomy: define all instrument-type values (DRUMS, BASS, PADS, KEYS, LEAD, FX, TEXTURE, GENERATOR, MODIFIER) and assign each of the 76 engines to exactly one type. Produce the mapping table. This is required for the preset browser filter pill row.
98. Design the OBRIX Flagship launch campaign: the OBRIX page on XO-OX.org (hero text, 5 feature callouts, audio player embed), the announcement post copy for all channels (Discord / Patreon / social), the 30-second promo video script. This is the V1 hero product.
99. Design the XOceanus "Living Manual" glassmorphism tooltip system: trigger conditions (hover 1.2s on any labeled control), tooltip anatomy (parameter name, current value, range, plain-language description, one-line coupling suggestion), positioning rules (avoid clipping at window edges). Write tooltip copy for the 20 most-touched parameters.
100. Retrospective design review of V1 scope: given the 326 features in the spatial architecture, identify the 10 highest-risk items that could delay V1 ship, rank by likelihood × impact, and propose mitigation for each. Produce a risk register.

---

### Top 50 Sonnet Tracks (Implementation / Build / Documentation)
Numbers 51–100

#### V1 Spatial Architecture — JUCE Implementation

51. Implement `XOceanusLookAndFeel` class in `Source/UI/XOceanusLookAndFeel.h/.cpp` — override `drawRotarySlider` for 3 knob sizes (80/50/36pt), load Space Grotesk + Inter + JetBrains Mono via BinaryData, map all design-token colors to `juce::Colour` constants. This is the prerequisite for all UI work.
52. Implement `EngineRackPanel` in `Source/UI/EngineRackPanel.h` — Column A 260pt, 4 slot tiles, each tile shows on/off toggle + engine name + waveform thumbnail + CPU% badge. Wire to `EngineRegistry` for slot load/unload events. Tile height 90pt, gap 4pt.
53. Implement the `EngineDetailPanel` base class in `Source/UI/EngineDetailPanel.h` — sticky 88pt macro strip (top), 32pt signal flow diagram, scrollable section container, sticky waveform+ADSR strip (bottom). Wire to active slot selection in `EngineRackPanel`. This is the Column B base for all engines.
54. Implement `CollapsibleParamSection` component in `Source/UI/CollapsibleParamSection.h` — section header with expand/collapse chevron, hero knobs (3 knobs @ 50pt), secondary knobs hidden until expanded, smooth 150ms height animation. Used by all standard engines for OSC/FILTER/MOD/FX sections.
55. Implement the `SectionNavBar` in `Source/UI/SectionNavBar.h` — sticky 32pt bar with [OSC][FILT][MOD][FX][★] pills, click scrolls `EngineDetailPanel` to that section, active section pill highlights with engine accent color. Required for any engine with >40 params.
56. Implement the `MiniCouplingGraph` in `Source/UI/MiniCouplingGraph.h` — 120×80pt `juce::Component`, 4 engine nodes as 8pt dots positioned by slot index, coupling arcs as bezier curves with `strokeWeight = couplingAmount * 3.0f`, arc brightness animates with coupling energy output. Click on arc triggers inline popover.
57. Implement the `CouplingArcPopover` in `Source/UI/CouplingArcPopover.h` — anchors to arc midpoint, `juce::Component` (not `CallOutBox`), shows 3-type quick-select buttons + "More..." + amount knob + depth knob. Writes directly to `MegaCouplingMatrix` state. Syncs live with `CouplingInspector` in Column C. Dismiss on outside click.
58. Implement the `SignalFlowDiagram` (Variant A — linear chain) in `Source/UI/SignalFlowDiagram.h` — 520×32pt strip, blocks auto-generated from engine param categories, clickable blocks scroll to section in `EngineDetailPanel`, active blocks glow with engine accent color, bypassed blocks dimmed. Required for all 50+ standard engines.
59. Implement the Column C tab container in `Source/UI/ColumnCPanel.h` — [PRESET][COUPLE][FX][PLAY][EXPORT][⚙] tab strip, tab switching, each tab as a separate child component. Tab strip 32pt tall, icon + label.
60. Implement the `PresetBrowserTab` in `Source/UI/PresetBrowserTab.h` — instrument-type filter pill row (DRUMS/BASS/PADS/KEYS/LEAD/FX/TEXTURE), mood pill row (15 moods), search text field, preset card list (engine name + preset name + DNA radar 24×24pt thumbnail). Wire to `PresetManager.h` for filtering. "Find Similar" button reads active preset's DNA and re-sorts list.

#### PlaySurface & MIDI Implementation

61. Implement `MPCPadGrid` component in `Source/UI/PlaySurface/MPCPadGrid.h` — 4×4 grid of 16 pads, pad size scales with container (minimum 44×44pt), velocity-sensitive color fill on hit (accent color at full velocity, 20% opacity at minimum velocity), 4 bank buttons (A/B/C/D) switching MIDI note offset. Wire to `PlaySurface` MIDI output.
62. Implement MIDI learn for all PlaySurface controls in `Source/UI/PlaySurface/MidiLearnManager.h` — right-click any knob/slider → "Assign MIDI CC" → wait for CC input → bind. Store assignments in plugin state via `getStateInformation`. Clear binding via right-click → "Remove MIDI CC". Required for live performance setup.
63. Implement `getStateInformation` and `setStateInformation` in `Source/XOceanusProcessor.cpp` — serialize active engine slot assignments, all parameter values, coupling routes (type + amount + depth), macro positions, and PlaySurface MIDI CC assignments to a `juce::MemoryBlock`. Verify round-trip with Logic Pro session save/restore.
64. Implement the A/B preset comparison system in `Source/Core/PresetManager.h` — `storeAPreset()` / `storeBPreset()` / `swapAB()` / `getActiveSlot()` methods, A/B toggle button in header, visual indicator (A or B) in header preset name display. State A and B persist across preset browser navigation until explicitly overwritten.
65. Implement `DAWAutomationExporter` in `Source/Core/DAWAutomationExporter.h` — expose the 65 automation-eligible params as named JUCE `AudioProcessorParameter` entries with proper `getName()`, `getLabel()`, `getDefaultValue()`. Params must appear correctly named in Logic Pro / Ableton automation lane dropdowns. Audit all 65 params against the master spec §6 table.
66. Implement the `EngineDetailPanel` for ONSET (111 params) — 8-voice drum grid hero (280pt), voice rows (type icon + algo selector + 6 mini knobs), section nav [DRUM][OSC][FILT][MOD][FX], sticky macros top, sticky waveform+ADSR bottom. This is the hardest custom panel — implement and verify at 960pt minimum window width.
67. Implement the `EngineDetailPanel` for OBRIX (81 params) — Brick Stack View as signal flow Variant B, reef ecology mode indicator (Off/Competitor/Symbiote/Parasite as 4-icon selector), per-source type selectors (src1Type, src2Type), Wave indicator. Wire reef resident coupling input read from `MegaCouplingMatrix`.
68. Implement the `EngineDetailPanel` for OPERA (OPERA custom panel) — Conductor arc shape selector (4 shapes as icon buttons), arc time/peak sliders, Kuramoto coherence meter (live readout of order parameter R), EmotionalMemory phase persistence indicator, Conductor mode 3-way switch. Wire to `OperaAdapter.h` state.
69. Implement the `EngineDetailPanel` for OUROBOROS — leash mechanism as a dedicated non-standard slider (continuous ring around chaos attractor), topology selector, feedback matrix display. Wire to `ouro_leash` and `ouro_topology` params.
70. Implement `PlaySurface` auto-expand for drum engines: in `EngineRackPanel`, detect when a drum engine (ONSET, OFFERING, OBRIX in drum mode) is loaded into Slot 1; if PlaySurface is collapsed, animate expansion to 220pt over 300ms. Detect via `engineType == EngineType::DRUMS` from the engine registry metadata.

#### Persistence, Export & Build

71. Implement the `XPNExportPanel` as a persistent `juce::Component` in `Source/UI/XPNExportPanel.h` — NOT a `CallOutBox`. Lives in Column C Export tab. Shows pack name input, sample rate selector, XPM format options (KeyTrack/RootNote enforced), export progress bar, open-in-Finder button. Survives focus loss, no dismiss on outside click.
72. Implement the build sentinel CI pipeline: GitHub Actions `.github/workflows/build.yml` — macOS-14 runner, install Ninja + Xcode CLT, cmake configure + cmake build, `auval -v aumu Xolk XoOx` pass/fail, post result as commit status. Trigger on push to main and all PRs. Target: green badge on README.
73. Fix `ExportDialog` memory leak (flagged in P0 list): audit `Source/UI/ExportDialog.h` for raw pointer ownership — replace any `new`/`delete` with `std::unique_ptr`, ensure `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` macro is present, verify with Instruments Leaks on a 10-export stress test.
74. Fix `OverworldEngine` build error (`ow_` prefix params): read `Source/Engines/Overworld/OverworldEngine.h`, identify the build error (likely a missing param registration or stub method), fix it, confirm `cmake --build build` passes 0 errors with OVERWORLD registered.
75. Implement `DarkCockpitController` in `Source/UI/DarkCockpitController.h` — five-level opacity hierarchy (100%/80%/45%/20%/0%), triggers: PlaySurface expanded → Column A 45%, Column B 20%, Column C 20%; during note play → non-macro controls 45%; idle 30s → secondary controls 20%. Uses `juce::Component::setAlpha()`. Wire to PlaySurface expand/collapse and MIDI note-on events.
76. Implement `CinematicModeController` in `Source/UI/CinematicModeController.h` — double-click Column B header → ColA + ColC animate to 0pt width over 300ms ease-out, ColB expands to full window width; same trigger reverses. Store pre-collapse widths, restore on exit. Keyboard shortcut: Cmd+Shift+F.
77. Implement `SingleEngineMode` in `Source/UI/SingleEngineModeController.h` — a view mode where only Slot 1 is active, Slots 2–4 hidden, Column A reduces to 80pt (just the active slot tile). Toggle via header gear menu "Single Engine Mode". Required for OBRIX Pocket companion parity.
78. Implement right-click context menus for engine slot tiles in `EngineRackPanel`: right-click slot → menu: [Load Engine...] [Copy Engine State] [Paste Engine State] [Clear Slot] [Set as Default]. Implement "Copy/Paste Engine State" as JSON to system clipboard (engine ID + all param values + preset name).
79. Implement right-click context menus for preset browser cards: right-click card → menu: [Load] [Favorite ♡] [Find Similar] [Copy DNA] [Add to Set...]. "Find Similar" re-sorts browser by DNA Euclidean distance from selected preset's 6D vector. "Copy DNA" puts JSON `{brightness:X, warmth:X, ...}` on clipboard.
80. Implement the keyboard shortcut system in `Source/UI/KeyboardShortcutHandler.h`: Cmd+Z (undo), Cmd+Shift+Z (redo), Cmd+S (save preset), Space (play/stop), Tab (cycle active slot), Cmd+B (toggle engine browser), Cmd+P (toggle preset browser), Cmd+F (search presets), Escape (dismiss popovers). Wire all to existing actions.

#### Regression Tests & QA

81. Write a regression test for `MegaCouplingMatrix` accumulator reset: instantiate 2 engines in test harness, set ENTANGLE coupling, render 512 samples, call `reset()`, verify all coupling accumulator values return to zero within 1 sample. Add to `Tests/CouplingTests.cpp`.
82. Write a regression test for `PresetManager` round-trip: load each of the 76 engines' first preset, serialize via `getStateInformation`, deserialize via `setStateInformation`, verify all param values match within `1e-5f`. Fail test if any engine has >0 mismatched params.
83. Implement per-engine CPU display in slot tiles: in `EngineRackPanel`, each tile shows `cpu_X.X%` badge, updated every 500ms via a `juce::HighResolutionTimer`. Read CPU cost from `XOceanusProcessor`'s per-engine render time tracking (add if missing). Badge turns amber at >5% CPU, red at >10%.
84. Implement `PresetPreviewThumbnail` in `Source/UI/PresetPreviewThumbnail.h`: a 24×24pt DNA radar (`juce::Path` hexagon with 6 axis points scaled by DNA values), rendered once per preset and cached as a `juce::Image`. Used in preset browser cards. Must render without blocking main thread — generate on background thread, invalidate on load.
85. Implement the MIDI learn persistence fix: ensure all CC assignments survive plugin state save/restore — serialize `MidiLearnManager` bindings inside `getStateInformation` as a JSON array `[{param_id, cc_number}]`, deserialize in `setStateInformation`. Write a unit test: bind CC 7 to `obrix_src1Type`, save state, clear bindings, restore state, verify CC 7 still maps to `obrix_src1Type`.
86. Implement scale quantization for the XOuija fretless surface in `Source/UI/PlaySurface/XOuijaPlanchette.h`: add `QuantizationMode` enum (Free / Scale / Chord), scale snap uses nearest semitone within selected scale, snap strength (0.0 = free, 1.0 = locked). Wire `ouie_interval` param (B026) as the active interval for chord mode.
87. Implement the `InstrumentTypeFilter` for the preset browser: add `"instrument_type"` field to `.xometa` schema, write a migration script `Tools/migrate_instrument_type.py` that assigns instrument type to all ~19,500+ presets based on engine ID lookup table (from Opus track #97). Wire filter pill row in `PresetBrowserTab`.
88. Implement DAW session state round-trip verification script `Tools/verify_session_roundtrip.py`: open XOceanus in Logic Pro (via AppleScript), load a test preset, save session, close, reopen, verify all parameter values via OSC or stdout log. This is the manual QA script until automated testing is feasible.
89. Implement `TouchRippleLayer` in `Source/UI/PlaySurface/TouchRippleLayer.h`: on MPCPadGrid hit, spawn a `juce::Component` overlay that expands a filled circle from pad center (engine accent color, alpha fades 0.6→0 over 120ms), `juce::Timer`-driven animation at 60fps. Multiple simultaneous ripples supported.
90. Implement the `GlassTooltip` (Living Manual) in `Source/UI/GlassTooltip.h`: `juce::Component` overlay with `glassmorphism` look (blurred background via `juce::Image` snapshot + white overlay at 12% opacity), appears after 1.2s hover on any `juce::Slider`/`juce::Button`, shows param name + current value + plain-language description. Positions itself to avoid window edge clipping. Wire to 20 most-touched params first.
91. Implement WCAG AA contrast enforcement in `XOceanusLookAndFeel`: for any text drawn on an engine accent color background, compute luminance contrast ratio (WCAG formula), if <4.5:1 override text color to white or `#1A1A1A`. Add a compile-time static assert table for all 76 accent colors — fail build if any accent color produces <4.5:1 contrast with both white and gallery-white `#F8F6F3`.
92. Implement `SessionStateManager` in `Source/Core/SessionStateManager.h`: on plugin editor close, serialize active engine configuration (slot assignments + param values + coupling routes + macro positions) to `juce::PropertiesFile` under key `"lastSession"`. On editor open, offer "Restore last session?" toast with Yes/No buttons. Restore calls `setStateInformation` on the stored blob.
93. Implement the `EngineHotSwap` 50ms crossfade in `Source/Core/EngineRegistry.h`: when `swapEngine(slot, newEngineId)` is called, start a 50ms linear crossfade between old and new engine audio output on the audio thread (read from both, blend by crossfade position). Use atomic float for crossfade position. Verify no clicks in the swap using an offline render test.
94. Fix the `frozenPrefixForEngine` missing entries: audit `Source/Core/PresetManager.h` `frozenPrefixForEngine` map — add entries for all 76 engines (OUTLOOK `look_`, OXYTOCIN `oxy_`, all Kitchen Collection engines). Run `Tools/verify_prefix_coverage.py` to confirm 76/76. This is a P0 for preset loading correctness.
95. Implement `PresetsAutoSave` in `Source/Core/PresetManager.h`: after any parameter change (debounced 2s), save current state to `juce::PropertiesFile` key `"unsavedPreset"`. On next launch, if `"unsavedPreset"` exists and differs from loaded preset, show: "You have unsaved changes from your last session. Restore?" If yes, call `setStateInformation` on stored state.
96. Implement iOS AUv3 responsive layout in `Source/UI/iOS/iOSLayoutManager.h`: at window width <768pt (iPhone / small iPad), collapse Column A to icon-only strip (40pt), Column C hidden (tab accessible via slide-in panel), Column B takes remaining width. At width ≥768pt (iPad landscape), restore 3-column layout with compressed proportions (180/360/230). All touch targets ≥44pt enforced via `setBoundsConstrained`.
97. Implement the `PresetSetManager` in `Source/Core/PresetSetManager.h`: a "Set" is a named ordered list of presets (like a playlist), stored in `.xoset` JSON files under `Presets/XOceanus/Sets/`. Right-click preset card → "Add to Set" → popover with existing sets + "New Set...". "Add to Set" also wires to the MIDI program change handler (set position = program number). V1 ships 5 factory sets (one per coupling preset chain from track #79).
98. Implement `ObrixBrickStackView` (Signal Flow Variant B) in `Source/UI/EngineDetail/ObrixBrickStackPanel.h`: visual slot-flow diagram for OBRIX showing src1→src2→filter→shaper→fx chain as connected block nodes (each block 48×32pt, connector arrows), block labels from active `obrix_src*Type` param values, clicking a block scrolls to that section in Engine Detail. Drag block to reorder (if routing allows).
99. Implement the `CouplingInspector` Column C tab in `Source/UI/CouplingInspectorTab.h`: scrollable list of all active coupling routes (source engine name → target engine name → type badge → amount slider → depth slider), "Add Route" button at bottom that opens a 3-step wizard (pick source slot → pick target slot → pick type from 15). Live-syncs with `MiniCouplingGraph` arc highlights.
100. Implement `MPC800Layout` in `Source/UI/MPC800Layout.h`: 2-column layout for 800×480pt MPC hardware viewport, Column A 260pt (engine rack), Column B 540pt (engine detail only, no Column C), tab row for Preset/FX/Export in Column B footer strip (32pt). All controls scaled to 44pt minimum touch target. This is the dedicated MPC One/Live II layout.

---

### Top 50 Haiku Tracks (Search / Read / Data / Git)
Numbers 51–100

#### Feature Coverage & Implementation Audit

51. Read `Docs/design/xoceanus-spatial-architecture-v1.md` in full and count the total number of explicitly listed features or UI components — verify it matches the "326 features" claim. List any features with no clear implementation home in `Source/UI/`.
52. Search `Source/UI/` for all existing `.h` files — list them with line counts. Identify which spatial architecture components already have stubs vs. which need to be created from scratch. This determines the true scope of Sonnet UI tracks.
53. Read `Source/Core/MegaCouplingMatrix.h` — count the number of coupling type enum values. Verify exactly 15 are defined (incl. KnotTopology + TriangularCoupling). List any that exist in the enum but have no implementation in the coupling accumulation logic.
54. Search `Source/Engines/` for all engines that do NOT have a corresponding `EngineDetailPanel` override (i.e., they fall back to the generic panel). This determines how many of the 17 custom engine panels are missing vs. already implemented.
55. Read `Source/Core/PresetManager.h` — verify `frozenPrefixForEngine` has entries for all 76 engine IDs. List any missing. Also verify `validEngineNames` contains all 73. Output a diff between expected (from CLAUDE.md) and actual.
56. Search all `.xometa` files in `Presets/XOceanus/` for any that contain `"XOceanus"` in their metadata fields — these need updating to `"XOceanus"` post-rename. Count them and list the first 10 as a sample.
57. Read `Source/XOceanusProcessor.cpp` — list all engines registered via `REGISTER_ENGINE()`. Count them. Identify any engine in CLAUDE.md's registered list that is NOT registered in the processor (unregistered engines cannot be loaded by users).
58. Search `Source/UI/` for all uses of hardcoded pixel values (e.g., `setBounds(x, y, 260, 90)`) — these should be replaced with design-token constants. Count occurrences and list the top 10 files by hardcoded-value density.
59. Read `Source/Core/EngineRegistry.h` — verify the `engineType` metadata field exists (DRUMS/BASS/PADS/etc.) for the auto-expand PlaySurface logic. If the field is missing, note which struct/class needs the new field.
60. Search `Source/DSP/` for all `ScopedNoDenormals` insertions — list which engines/DSP units have it and which are missing. Cross-reference against the list of engines with feedback/filter paths (OUROBOROS, ORBWEAVE, OVERLAP, ORGANON, OXBOW at minimum).

#### Preset & DNA Coverage Audit

61. Count `.xometa` files per engine in `Presets/XOceanus/` — produce a ranked list from most to fewest presets. Flag any engine with <10 presets. Compare against the "thin coverage engines expanded in Rounds 8–11" claim in CLAUDE.md.
62. Read 10 random `.xometa` files from `Presets/XOceanus/Foundation/` — verify each has all 6 Sonic DNA fields (brightness, warmth, movement, density, space, aggression) and all 4 macro values (M1–M4). Report any missing fields.
63. Search `Presets/XOceanus/` for all `.xometa` files where the `"instrument_type"` field is absent or empty. Count them — this is the backlog for the `Tools/migrate_instrument_type.py` script (Sonnet track #87).
64. Count total `.xometa` presets per mood across all 15 mood folders. Report the distribution. Identify which moods have <50 presets (underpopulated) and which have >3,000 (may need curation).
65. Search all `.xometa` files for any preset name exceeding 30 characters — these violate the naming rule in CLAUDE.md. Count violations and list the top 10 longest names.
66. Read `Presets/XOceanus/Coupling/` — count the `.xometa` files that define multi-engine coupling configurations. Verify each references exactly 2–4 engine IDs in the `"engines"` array and has at least 1 coupling route in `"couplingRoutes"`.
67. Search `Presets/XOceanus/` for any presets with duplicate names across all moods (same `"name"` value in two or more files). Count duplicates. CLAUDE.md claims 0 duplicates — verify.
68. Count total preset files in `Presets/XOceanus/` across all mood subdirectories. Verify the total is approximately 19,500+. Report exact count and note any discrepancy with the CLAUDE.md figure.
69. Read `Presets/XOceanus/` for the OXYTOCIN engine — count how many of the claimed 130 presets are actually present on disk. List which mood folders they appear in.
70. List all engines where the Guitar Bin retreat has NOT yet produced preset files in `Presets/XOceanus/` — specifically check for any of the 16 Kitchen Collection engines (OTO, OCTAVE, OLEG, OTIS, OVEN, OCHRE, OBELISK, OPALINE, OGRE, OLATE, OAKEN, OMEGA, ORCHARD, OVERGROW, OSIER, OXALIS) with fewer than 10 presets.

#### Coupling, Accent Color & Keyboard Shortcut Audits

71. Read `Docs/design/design-tokens.css` — extract all 76 engine accent color hex values. Compute WCAG contrast ratio against gallery-white `#F8F6F3` for each. List any below 3.0:1 (insufficient for large text) and any below 4.5:1 (insufficient for normal text). This feeds the Sonnet WCAG enforcement track (#91).
72. Search `Source/Engines/*/` headers for the `CouplingType` enum entries each engine declares it accepts (likely via a method like `getSupportedCouplingTypes()`). Verify the declared types match the documentation in `Docs/` coupling type tables. List any mismatches.
73. Read `Skills/coupling-interaction-cookbook/SKILL.md` — extract the engine pair compatibility table. Count unique engine pairs listed. Verify at least 1 pair exists for each of the 15 coupling types.
74. Search `Source/UI/` for keyboard shortcut registrations (likely `keyPressed`, `KeyPress`, or `ApplicationCommandManager` usage). List all registered shortcuts. Cross-reference against the shortcut list in Sonnet track #80 — identify any conflicts (same key mapped twice).
75. Read `Source/Core/MegaCouplingMatrix.h` and `Source/Core/EngineRegistry.h` — verify the coupling accumulator reset path exists (a method called on engine hot-swap or `prepareToPlay`). If missing, note the exact location where it needs to be added.
76. Search `Source/Engines/*/` for any engine that declares a `CouplingType::TRIANGULAR` coupling input but is NOT OXYTOCIN — TriangularCoupling is described as type #15 and OXYTOCIN-specific. Flag any accidental declarations.
77. Read `Source/DSP/Effects/` — list all FX suite headers and count the total number of distinct FX slot types available across all suites (Aquatic, Math, Boutique, Singularity). Verify there are at least 3 parameter knobs per FX type (per the "Verify all 6 FX slot types have at least 3 parameter knobs" audit goal).
78. Search `Source/Engines/*/` for any `param.value` assignment immediately following `cancelScheduledValues` — this is the click-causing pattern flagged in CLAUDE.md architecture rules. Count violations and list file + line.
79. Read `Source/Core/PresetManager.h` — verify `resolveEngineAlias()` maps all 7 legacy names (Snap→OddfeliX, Morph→OddOscar, Dub→Overdub, Drift→Odyssey, Bob→Oblong, Fat→Obese, Bite→Overbite). If any mapping is missing, note it.
80. Search `Source/Engines/*/` for any engine that does NOT override `SynthEngine::getSupportedCouplingTypes()` — these engines would silently accept all coupling types, which may cause undefined behavior. List them.

#### Git, Build & Repository Hygiene

81. Run `git log --oneline --since="2026-03-24" --until="2026-03-25"` — list all commits made today. Verify the XOceanus rename commit is present and the commit message references the B042 candidate blessing.
82. Search the entire repo for any remaining occurrences of `"XOceanus"` in non-documentation `.cpp` or `.h` source files (post-rename). List file paths and line numbers. These are code-level rename oversights.
83. Search `Source/` for any `#include` of a file path containing `XOceanus` (old include guards or paths) — list them. These would cause build failures when file names are fully updated.
84. Read `CMakeLists.txt` — verify `VERSION 1.0.0` is set (CLAUDE.md mentions this was a P0 fix). Also verify `OSX_ARCHITECTURES` appears before `project()`. List any other CMake issues found.
85. Search `Source/UI/` for any raw `new` allocations without corresponding `delete` or `std::unique_ptr` ownership — a proxy audit for memory leaks. List top 10 occurrences by file. This feeds the ExportDialog memory leak fix (Sonnet track #73).
86. Count total `.h` files in `Source/Engines/` — verify the count matches the expected structure (each of 76 engines has at least 1 `.h` file: EngineAdapter or EngineEngine). List any engines with 0 `.h` files in their subdirectory.
87. Read `Source/Engines/Overworld/OverworldEngine.h` — identify the specific build error flagged in P0 list ("stub files only, 6 headers, no processor/CMake"). Output the exact error if detectable from file content (missing method, wrong return type, etc.).
88. Search `Source/DSP/` for `exp(-2*PI*fc/sr)` vs `w/(w+1)` IIR coefficient patterns — verify all filter coefficient computations use the matched-Z formula (`exp(-2*PI*fc/sr)`) and not the Euler approximation (`w/(w+1)`). List any violations.
89. Search all `.xometa` files for any `"sampleRate": 44100` hardcoded values — these violate the "never hardcode 44100" architecture rule. Count violations. They should read from the session's actual sample rate at load time.
90. Read `Docs/MANIFEST.md` — count total documents listed. Verify `Docs/design/xoceanus-spatial-architecture-v1.md` is listed. Identify any documents in `Docs/` that are NOT in the manifest (orphaned docs).

#### Skills, Memory & Documentation Audits

91. Read all files in `~/.claude/projects/-Users-joshuacramblet/memory/` — list every memory file by name and line count. Verify MEMORY.md is under 200 lines (per Haiku track #31 from original plan). If MEMORY.md exceeds 200 lines, identify the 5 sections that could be trimmed or archived.
92. Read `Skills/README.md` — verify all 19 skill entries match actual files in `Skills/*/SKILL.md`. List any skills in the README that don't have a corresponding file on disk, and any files on disk not listed in the README.
93. Search `~/.claude/skills/` for any skill file that still mentions "XOceanus" (not "XOceanus") in its description or trigger instructions. List them — these need updating post-rename (per Haiku track #34 from original plan).
94. Read `Docs/seance_cross_reference.md` — count the number of engines with completed seance entries. Verify it matches "72 engines seanced" (73 total minus OSMOSIS). List any engine in CLAUDE.md's registered list missing from the cross-reference.
95. Search `Docs/seances/` and `scripture/seances/` — count total seance verdict documents. Verify count matches the cross-reference. List any seance files that don't match a known engine name (orphaned verdicts).
96. Read `Docs/fleet-seance-scores-2026-03-20.md` — extract the list of engines with seance score <8.0. Verify the list matches CLAUDE.md's claim that "all original 47 engines at 8.0+ target". List any below 8.0.
97. Count the number of `.xometa` files in `Presets/XOceanus/` that contain `"couplingRoutes"` with at least one entry — these are true coupling presets. Verify at least 5 exist (per the factory coupling preset chain from Opus track #79).
98. Read `Docs/v1-scope-revision-2026-03-23.md` — extract the exact list of 20–25 engines included in V1. Verify all listed engines have: (a) a seance verdict, (b) at least 10 presets, (c) an entry in `frozenPrefixForEngine`. Report any that fail one or more checks.
99. Search `Source/Export/` — list all `.h` files and their stated purposes. Verify the XPN export pipeline has handlers for the 3 critical XPM rules (KeyTrack=True, RootNote=0, empty layer VelStart=0) in at least one of these files.
100. Read `Docs/xoceanus_master_specification.md` sections 1 and 3.1 — verify the engine count in the spec matches 73. Note the date last updated. If the spec still says "XOceanus" anywhere in its title or headers (post rename), flag those lines.
