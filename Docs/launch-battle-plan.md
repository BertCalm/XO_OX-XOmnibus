# XOlokun Launch Battle Plan — Path to 10/10

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
20. Design the Constellation View (25% push) — 73 engines as interactive star chart
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
30. Redesign the Outshine UI to match XOlokun's Rebirth aesthetic
31. Design the "Send to MPC" USB workflow
32. Design the MPCe quad-corner assignment panel

### Brand & Launch
33. Write the XOlokun cultural acknowledgment (Olokun orisha) — get external review
34. Design the V1 launch sequence — press kit, timeline, announcement
35. Design the Patreon tier benefits with the new brand
36. Write the first 3 Field Guide posts for XOlokun launch
37. Design the XO-OX.org rebrand from XOmnibus to XOlokun

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
50. Design the XOlokun tutorial/onboarding for first-time users

---

## Top 50 Sonnet Tracks (Implementation / Build / Documentation)

### UI Implementation
1. Implement the CouplingVisualizer as XOlokunEditor's default center panel
2. Mount the PlaySurface in XOlokunEditor (it's built, just hidden)
3. Mount the ExportDialog (it's built, no button to open it)
4. Mount the full PresetBrowser (with DNA "Find Similar", replaces stripped version)
5. Fix the 7 missing moods in the preset browser filter (hardcoded 9 → 16)
6. Fix macro params appearing twice (MacroHeroStrip AND ParameterGrid)
7. Replace the ParameterGrid with grouped collapsible sections
8. Implement 3 knob sizes (macro 80px, section 50px, detail 36px)
9. Implement the GalleryLookAndFeel knob style from Audio UI library reference
10. Implement color-coded parameter sections (OSC/FILTER/MOD/FX)

### PlaySurface Implementation
11. Wire PlaySurface MidiMessageCollector to XOlokunProcessor's MIDI input
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
33. Fix the Outshine/Originate aesthetic to match XOlokun dark theme

### Site & Web
34. Populate the 24 Kitchen Collection creature sprites for aquarium
35. Build the XOlokun cultural acknowledgment About page
36. Update XO-OX.org branding from XOmnibus to XOlokun
37. Build the Patreon tier page with XOlokun branding
38. Implement the Field Guide template for XOlokun posts
39. Build the XPN pack download page

### Documentation
40. Write the JUCE implementation guide based on definitive UI spec Appendix D
41. Write the PlaySurface MIDI integration guide
42. Update the master specification for XOlokun
43. Write the V1 changelog from all session git logs
44. Document all 36 Blessed + 3 Provisional + 4 Candidate blessings with evidence

### Testing & QA
45. Run auval at all 5 sample rates (11025/44100/48000/96000/192000)
46. Test preset loading for all 73 engines (1 preset per engine minimum)
47. Test coupling between all 15 types with 2 engines each
48. Test PlaySurface MIDI output in GarageBand
49. Build a CI/CD pipeline for automated build + auval on push
50. Create a regression test for the coupling accumulator reset pattern

---

## Top 50 Haiku Tracks (Search / Read / Data / Git)

### Codebase Health
1. Count exact engine registration in XOlokunProcessor.cpp (verify 73)
2. Count exact preset count per engine per mood (distribution report)
3. Verify all 73 engines have entries in CLAUDE.md Engine Modules table
4. Verify all 73 engines have entries in frozenPrefixForEngine
5. Verify all 73 engines have entries in validEngineNames
6. Count total lines of code in Source/ (baseline metric)
7. Count total lines in each engine (complexity ranking)
8. List all engines still missing Guru Bin retreats
9. List all engines missing seance verdicts (should be only OSMOSIS)
10. Verify design-tokens.css has all 73 engine accent colors

### Git & History
11. Generate git stats (commits per day, lines added/removed)
12. List all commits since the XOlokun rename
13. Verify no uncommitted files on main
14. Check for any lingering merge conflict markers in source
15. List all branches and their status (stale, merged, active)
16. Verify the remote branch `claude/dual-engine-integration-uBjia` is up to date
17. Count total .xometa files in Presets/XOlokun/
18. Verify preset symlink at ~/Library/Application Support/XO_OX/XOlokun/Presets
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
34. Verify all skill descriptions mention "XOlokun" (not just "XOmnibus")
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
47. Verify Oxytocin's 130 presets are in Presets/XOlokun/Oxytocin/
48. Check for any presets still referencing "XOmnibus" in metadata
49. Verify coupling presets (.xocoupling) if any exist
50. Generate a preset mood distribution pie chart data (counts per mood)
