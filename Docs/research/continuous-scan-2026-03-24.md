# Research Compendium — Continuous Scan 2026-03-24

**Scan Date:** 2026-03-24
**Scope:** 8 domains across JUCE plugin UI, audio visualization, Figma community, MPE controllers, XY pads, novel input devices, ocean/aquatic visualization, and dark design systems.
**Total finds:** 42 resources across all categories.

---

## Legend

| Priority | Meaning |
|----------|---------|
| P0 | Critical — implement or adopt immediately |
| P1 | High — near-term integration candidate |
| P2 | Medium — study and adapt patterns |
| P3 | Low — reference and inspiration only |

---

## 1. JUCE Plugin UIs — Visually Stunning

### 1.1 awesome-juce (sudara)
- **URL:** https://github.com/sudara/awesome-juce
- **What it is:** Community-curated catalog of JUCE modules, UI frameworks, plugin examples, and templates. Updated nightly with star counts. Primary discovery surface for the entire JUCE open-source UI ecosystem. 805 stars.
- **License:** MIT (catalog itself)
- **Relevance:** Master reference for sourcing JUCE UI components; replaces ad-hoc searches. Contains pointers to the specific GPU UI libs, blur modules, and inspector tools below.
- **Priority:** P0 — bookmark and re-check monthly

### 1.2 foleys_gui_magic (ffAudio)
- **URL:** https://github.com/ffAudio/foleys_gui_magic
- **What it is:** XML-driven drag-and-drop GUI builder for JUCE AudioProcessorValueTreeState. Includes live-preview editor, built-in spectrum/level/oscilloscope visualizers at zero additional code. BSD V2 3-clause (commercial-friendly since v1.4.0).
- **License:** BSD 3-Clause
- **Relevance:** Accelerates Gallery Model component development. Built-in visualizers directly applicable to engine spectral displays and coupling strip. Pattern for data-binding UI to APVTS mirrors our ParamSnapshot approach.
- **Priority:** P1 — evaluate as a template for future engine UI panels

### 1.3 melatonin_blur (sudara)
- **URL:** https://github.com/sudara/melatonin_blur
- **What it is:** Cross-platform CPU blur and shadow compositing library for JUCE. Uses Accelerate on macOS and Intel IPP on Windows. 10-30x faster than naive Stack Blur. Enables "100s of shadows" in complex vector UIs without resorting to deprecated OpenGL on macOS.
- **License:** MIT
- **Relevance:** Direct enabler for XOlokun Gallery Model's layered glow effects, engine card hover states, and the coupling arc visualizer. The aquarium site's CSS glow effects use blur heavily — this is the native JUCE equivalent.
- **Priority:** P0 — integrate for engine accent glow rendering

### 1.4 melatonin_inspector (sudara)
- **URL:** https://github.com/sudara/melatonin_inspector
- **What it is:** JUCE module for inspecting and visually editing UI component trees at runtime without recompiling. Browser-DevTools-like overlay for JUCE Component hierarchies.
- **License:** MIT
- **Relevance:** Dramatically accelerates UI iteration on the Gallery Model shell and PlaySurface. Pairs with melatonin_blur.
- **Priority:** P1 — add to dev build only

### 1.5 Pamplejuce (sudara)
- **URL:** https://github.com/sudara/pamplejuce
- **What it is:** JUCE 8 + CMake + Ninja audio plugin project template. Includes Catch2 testing, Pluginval, GitHub Actions CI, macOS notarization, Azure Trusted Signing. 686 stars. MIT licensed. Updated within the last 2 months.
- **License:** MIT
- **Relevance:** XOlokun already uses CMake + Ninja. This template codifies CI/CD best practices that our build pipeline should mirror — particularly the notarization flow and pluginval integration for AU validation.
- **Priority:** P1 — audit our GHA pipeline against Pamplejuce's workflow

### 1.6 Surge XT (surge-synthesizer)
- **URL:** https://github.com/surge-synthesizer/surge
- **What it is:** Free, open-source hybrid synthesizer, fully JUCE-based. Complete rewrite of all widgets and UI components using custom JUCE LookAndFeel. Live skin system with community skin library. Accessible UI (screen reader support). One of the largest open-source JUCE codebases.
- **License:** GPL-3.0
- **Relevance:** Best-in-class reference for large-fleet JUCE UI patterns. Their skin XML system is conceptually adjacent to our Gallery Model color-per-engine approach. Study their accessibility implementation for XOlokun V1.1+.
- **Priority:** P1 — architectural study, not direct adoption

### 1.7 Vital Synthesizer (mtytel)
- **URL:** https://github.com/mtytel/vital
- **What it is:** Spectral warping wavetable synthesizer, GPL-3.0. One of the most visually sophisticated open-source JUCE plugin UIs: animated wavetable display, XY modulation visualizer, responsive skin system. Active community skin thread.
- **License:** GPL-3.0
- **Relevance:** Direct visual reference for coupling visualizer design, animated parameter widgets, and the PlaySurface zone display. Vital's oscilloscope overlay approach informs our OpticVisualizer design. Their skin format (JSON color overrides) mirrors how we could expose engine accent theming to users.
- **Priority:** P0 — deep study for coupling visualizer and PlaySurface design

### 1.8 react-juce / Blueprint (JoshMarler)
- **URL:** https://github.com/JoshMarler/react-juce
- **What it is:** Hybrid C++/JavaScript framework for building JUCE plugin UIs in React.js. Uses Duktape (JS engine), Yoga (flexbox layout), and bridges to juce::Component tree. Used in production by Creative Intent's Remnant plugin.
- **License:** MIT
- **Relevance:** High relevance for the iOS AUv3 UI work and any future web-facing XOlokun companion. Our CLAUDE.md already references a JUCE + React/Typescript architecture (WebUISynth); react-juce is the more mature path. Evaluate against the `tomduncalf/WebUISynth` approach.
- **Priority:** P2 — evaluate for iOS AUv3 UI layer

### 1.9 WebUISynth (tomduncalf)
- **URL:** https://github.com/tomduncalf/WebUISynth
- **What it is:** JUCE audio engine with React/TypeScript web UI via the `juce_browser_integration` module. Uses JUCE ValueTree on C++ side and MobX on JS side. Automatically synchronizes plugin state with browser UI.
- **License:** MIT
- **Relevance:** Alternative to react-juce for browser-based UI. More modern TypeScript/MobX stack vs. react-juce's Duktape/Yoga. Relevant for the Playable Aquarium (VQ 002) companion interface.
- **Priority:** P2 — evaluate for web companion interface

### 1.10 DISTRHO Cardinal
- **URL:** https://github.com/DISTRHO/Cardinal
- **What it is:** Open-source VCV Rack as a self-contained plugin (LV2/VST3/CLAP/AU). Built on DISTRHO Plugin Framework (DPF) — deliberately not JUCE. 2,942 stars. The most complete reference for a node-graph modular synthesizer UI in plugin form.
- **License:** GPL-3.0
- **Relevance:** Primary architectural reference for a future "Coupling Graph View" — how to render patch cables between engines visually, how to handle node placement, drag-to-connect UX. Not for direct code adoption (GPL + non-JUCE), but essential study material.
- **Priority:** P1 — study for coupling graph view design

---

## 2. Audio Visualization Libraries (Web)

### 2.1 audioMotion-analyzer (hvianna)
- **URL:** https://github.com/hvianna/audioMotion-analyzer
- **What it is:** High-resolution real-time audio spectrum analyzer built on Web Audio API + HTML5 Canvas. Zero dependencies. Configurable FFT size, gradient presets, LEDS mode, bark/mel/logarithmic scales, stereo analyzer. npm package `audiomotion-analyzer`.
- **License:** AGPL-3.0
- **Relevance:** Direct candidate for XO-OX.org aquarium sonic frequency display and the web companion to OpticVisualizer. The dark gradient presets visually align with XO_OX brand. Note: AGPL requires open-source disclosure if used server-side.
- **Priority:** P0 — prototype in aquarium site for real-time engine audio visualization

### 2.2 wavesurfer.js (katspaugh)
- **URL:** https://github.com/katspaugh/wavesurfer.js
- **What it is:** Interactive waveform rendering + audio playback library. HTML5 Canvas + Web Audio API. Spectrogram plugin (FFT). Highly configurable, plugin architecture. One of the most-starred audio visualization libraries on GitHub.
- **License:** BSD 3-Clause
- **Relevance:** XPN pack preview player — display WAV waveforms for MPC expansion previews on XO-OX.org. Also useful for the Guru Bin Transcendental tier PDF companion (interactive audio examples).
- **Priority:** P1 — implement for XPN pack preview on site

### 2.3 Peaks.js (BBC R&D)
- **URL:** https://github.com/bbc/peaks.js (part of the audiowaveform ecosystem)
- **What it is:** Modular client-side component for display and interaction with audio waveform material. Includes zoom/scroll, waveform segments, point markers. Purpose-built for editorial/cue-point workflows.
- **License:** LGPL-3.0
- **Relevance:** Useful for the Oxport/XPN pipeline's WAV preview interface if we build a web review tool. Less relevant than wavesurfer.js for our current use cases.
- **Priority:** P3 — file for later

### 2.4 willianjusten/awesome-audio-visualization
- **URL:** https://github.com/willianjusten/awesome-audio-visualization
- **What it is:** Curated list of audio visualization projects, demos, tools, and articles. Comprehensive index spanning WebGL, Three.js, D3.js, p5.js, canvas, and dedicated libraries.
- **License:** CC0
- **Relevance:** Primary discovery surface for Web audio visualization — same role as awesome-juce for plugin UIs. Check monthly for new entries.
- **Priority:** P1 — bookmark as discovery index

### 2.5 three.quarks (Alchemist0823)
- **URL:** https://github.com/Alchemist0823/three.quarks
- **What it is:** High-performance particle system and VFX engine for Three.js. GPU-accelerated. Supports emitter types, force fields (attractors, turbulence, gravity, vortex), collision, trail renderers. Designed for games and interactive experiences.
- **License:** MIT
- **Relevance:** Direct candidate for the aquarium bioluminescence particle effects and the coupling arc visualizer. Force field attractors are semantically aligned with XOlokun coupling — particles that "flow between engines" represent signal routing visually.
- **Priority:** P0 — prototype aquarium particle layer with force-field attractors as coupling metaphor

### 2.6 three-nebula (creativelifeform)
- **URL:** https://github.com/creativelifeform/three-nebula
- **What it is:** WebGL particle system engine for Three.js. Supports emitters, initializers, behaviours (attraction, collision, gravity, repulsion, rotation, scale, spring). More behaviour-focused than three.quarks.
- **License:** MIT
- **Relevance:** Alternative to three.quarks for the aquarium. The Repulsion and Attraction behaviours directly model bioluminescent creature scatter/gather responses to audio input.
- **Priority:** P1 — evaluate against three.quarks for aquarium phase 0

### 2.7 jbouny/fft-ocean (Three.js FFT water)
- **URL:** https://github.com/jbouny/fft-ocean
- **What it is:** WebGL FFT-based ocean surface rendering for Three.js. GPU-computed wave simulation using the Phillips spectrum model (oceanographic). Realistically displaces a water surface mesh at high resolution.
- **License:** MIT
- **Relevance:** XO-OX.org aquarium water surface layer. Could be audio-reactive: map FFT bins to wave height parameters. The Phillips spectrum parameters (wind speed, direction) map naturally to audio energy parameters.
- **Priority:** P1 — aquarium water surface layer, audio-reactive variant

### 2.8 Three.js built-in ocean shader
- **URL:** https://threejs.org/examples/webgl_shaders_ocean.html (source: mrdoob/three.js)
- **What it is:** Built-in WebGL ocean shader in the Three.js examples. Stable, maintained, zero extra dependency. Less physics-accurate than fft-ocean but lower integration cost.
- **License:** MIT
- **Relevance:** Fastest path to a water surface in the aquarium site. Use for Phase 0, upgrade to fft-ocean for Phase 1.
- **Priority:** P0 — immediate use in aquarium Phase 0

### 2.9 ShaderParticleEngine (squarefeet)
- **URL:** https://github.com/squarefeet/ShaderParticleEngine
- **What it is:** GLSL-heavy GPU particle engine for Three.js. Runs entirely on GPU — simulation in vertex shader. Handles hundreds of thousands of particles at 60fps. Supports attractors, repellers, drag, color-over-lifetime.
- **License:** MIT
- **Relevance:** Highest-performance option for the bioluminescence particle layer in the aquarium. GPU-side simulation means we can run complex attractor math (coupling metaphor) at no CPU cost.
- **Priority:** P1 — evaluate vs. three.quarks for GPU-bound particle count requirements

### 2.10 adarkforce/3d-midi-audio-particles-threejs
- **URL:** https://github.com/adarkforce/3d-midi-audio-particles-threejs
- **What it is:** Animated particle visualizer that reacts to audio and is controllable via MIDI input. Three.js + Web Audio + WebMIDI. Demonstrates real-time parameter mapping from MIDI CC to particle system properties.
- **License:** MIT
- **Relevance:** Direct proof-of-concept for the aquarium's WebMIDI phase (VQ 002 Phase 2): MIDI CC from MPC controls particle behavior. Study the audio → particle parameter mapping pipeline.
- **Priority:** P1 — study for aquarium Phase 2 WebMIDI integration

---

## 3. Figma Audio/Music Community Files

### 3.1 SeaSynth — Synthesizer Instrument UI + Kits
- **URL:** https://www.figma.com/community/file/1120992547387371994/seasynth-synthesizer-instrument-ui-kits
- **What it is:** Full synthesizer software UI concept and component kit. Inspired by ROLI Seaboard and Vital. Includes knobs, sliders, XY pads, keyboard, waveform display, modulation routing matrix, dark skin.
- **License:** Figma Community (free)
- **Relevance:** Single most relevant Figma file for XOlokun UI work. The ROLI-inspired continuous keyboard and XY pad directly inform our PlaySurface fretless zone. The modulation matrix panel is a reference for our Coupling Strip UI redesign.
- **Priority:** P0 — duplicate and use as component reference for Gallery Model + PlaySurface

### 3.2 Synthesizer VSTi — Plugin Interface Mockup
- **URL:** https://www.figma.com/community/file/1476177446001979362/synthesizer-vsti-plugin-interface-mockup
- **What it is:** Detailed VST plugin interface mockup with oscillator, filter, envelope, effects sections. Dark theme. Realistic knob and display designs.
- **License:** Figma Community (free)
- **Relevance:** Reference for individual engine module panel layouts within the Gallery Model. The oscillator/filter section proportions inform our standard engine parameter page dimensions.
- **Priority:** P1 — use as proportional reference for engine UI panels

### 3.3 Skeuomorphic Audio Controller UI
- **URL:** https://www.figma.com/community/file/859424303965872933/skeuomorphic-audio-controller-ui
- **What it is:** Recreated skeuomorphic interface of the subtract.one web synthesizer by Julius Sohn. Rich material textures on knobs and faders. High visual fidelity.
- **License:** Figma Community (free)
- **Relevance:** Reference for OBLONG / OBRIX / physical-modelling engine UI surface materials. The material texture language of the Oware engine (wood/metal/bell/bowl) should be expressed in the UI — this file shows how.
- **Priority:** P2 — texture reference for physical modeling engine panels

### 3.4 Synthwave UI Kit
- **URL:** https://www.figma.com/community/file/1114174373561311418/synthwave-ui-kit
- **What it is:** Synthwave-aesthetic UI component kit. CRT effects, scan lines, neon glows, dark mode first, grid displays, retro oscilloscope elements.
- **License:** Figma Community (free)
- **Relevance:** The OPTIC visual engine's OpticVisualizer and the OVERWORLD chip-synth engine should reference this visual language. CRT scan-line elements directly applicable to the Winamp-style visualizer panel.
- **Priority:** P2 — visual language reference for OPTIC and OVERWORLD panels

### 3.5 Data Visualization Charts — Dark Mode
- **URL:** https://www.figma.com/community/file/1060951969983411754/data-visualization-charts-dark-mode
- **What it is:** Dark-mode chart components: line charts, bar charts, area charts, scatter plots, donut charts, data tables. Consistent dark color palette.
- **License:** Figma Community (free)
- **Relevance:** Coupling performance analytics display, preset DNA radar charts, and the session-handoff statistical summaries on XO-OX.org could use these patterns for a consistent dark data visualization language.
- **Priority:** P2 — reference for analytics/DNA display components

### 3.6 Music GUI/UI Kit (Ableton-inspired)
- **URL:** https://www.figma.com/community/file/1362930821190233244/music-gui-ui-kit
- **What it is:** Highly detailed DAW-style GUI kit based on Ableton's UI conventions. Includes clip launcher grid, mixer channel strips, automation lanes, transport controls.
- **License:** Figma Community (free)
- **Relevance:** Reference for the Coupling Performance System's "live set" view (Phase B PerformanceViewPanel). The clip-launcher grid metaphor could extend to coupling preset launching from within XOlokun.
- **Priority:** P2 — reference for Performance View panel

---

## 4. Seaboard/MPE Controllers — Open Source Implementations

### 4.1 WeAreROLI/mpejs (ROLI official)
- **URL:** https://github.com/WeAreROLI/mpejs
- **What it is:** Official ROLI JavaScript MPE library. "Next generation MIDI for the web." Accepts MPE messages and returns a normalized representation of current touch state as sorted array of `activeNote` objects with named properties (channel, pitch, slide, pressure, pitchbend). WebMIDI-ready.
- **License:** MIT
- **Relevance:** Direct implementation library for the aquarium's WebMIDI phase (VQ 002 Phase 2). When a user plays the MPC X in MPE mode, mpejs normalizes the 5D per-note data (strike/lift/slide/glide/press) into the aquarium's creature control parameters.
- **Priority:** P0 — adopt for aquarium Phase 2 WebMIDI layer

### 4.2 mpe.js (standalone)
- **URL:** https://mpe.js.org
- **What it is:** Independent JavaScript MPE library (separate from ROLI's mpejs). mpeInstrument instance accepts MPE MIDI messages and returns standardized active note objects. Well-documented API.
- **License:** MIT
- **Relevance:** Alternative to WeAreROLI/mpejs if ROLI's library has stale dependencies. Same use case: normalizing MPE input for the aquarium WebMIDI interface.
- **Priority:** P1 — evaluate alongside mpejs, choose the more actively maintained

### 4.3 euwbah/microtonal-seaboard
- **URL:** https://github.com/euwbah/microtonal-seaboard
- **What it is:** Maps microtonal split-key tunings onto ROLI Seaboard RISE/Block. Reads CC74 (Slide dimension) to distinguish vertical key sections. Outputs MPE-style MIDI to virtual port.
- **License:** MIT
- **Relevance:** Implementation reference for our PlaySurface fretless zone, where vertical position on a zone maps to microtonal pitch offset. The CC74 → pitch subdivision mapping is directly applicable to XOlokun PlaySurface Zone 2 (Fretless).
- **Priority:** P2 — implementation reference for fretless zone pitch math

### 4.4 pianosnake/isomorphic-keyboards
- **URL:** https://github.com/pianosnake/isomorphic-keyboards
- **What it is:** Collection of isomorphic keyboard layouts implemented in-browser. Multiple layouts: Harmonic Table (Tonnetz), Wicki-Hayden, Janko, Gerhard. Includes a note-recognition quiz. Web-based, keyboard/mouse input.
- **License:** MIT
- **Relevance:** Reference for future PlaySurface "grid layout" mode. The Tonnetz/Harmonic Table layout (adjacent notes = consonant intervals) is a natural companion to the OUIE engine's interval-as-parameter (B026) and the ORBWEAVE knot topology.
- **Priority:** P2 — reference for PlaySurface grid layout mode

### 4.5 hexatone (000masa000)
- **URL:** https://github.com/000masa000/hexatone
- **What it is:** Browser-based MIDI isomorphic keyboard with hexagonal layout. WebMIDI output. Touch-enabled.
- **License:** MIT
- **Relevance:** Hex grid controller directly relevant to the XOlokun PlaySurface's isomorphic mode. The visual hex tessellation pattern is also aesthetically aligned with ORBWEAVE's knot topology diagrams.
- **Priority:** P2 — reference implementation for hex grid input

---

## 5. XY Pad / 2D Controller Implementations

### 5.1 touchmidi (benc-uk)
- **URL:** https://github.com/benc-uk/touchmidi
- **What it is:** Flexible HTML5 touch-based control surface for MIDI. XY pads, sliders, buttons, knobs, all configurable via JSON layout files. WebMIDI output. Live demo at code.benco.io/touchmidi/. XY pads map each axis to independent CC values.
- **License:** MIT
- **Relevance:** Direct reference for the aquarium's web controller interface. The JSON layout config maps cleanly to our APVTS parameter scheme. Could serve as a browser-based performance controller for XOlokun standalone until a native iOS app is built.
- **Priority:** P1 — adapt for web companion controller for XOlokun

### 5.2 Open Stage Control (jean-emmanuel)
- **URL:** https://framagit.org/open-stage-control/open-stage-control (mirrored: https://github.com/jean-emmanuel/open-stage-control)
- **What it is:** Libre modular OSC/MIDI controller. Node/Electron server + Chrome client. Includes XY pads, sliders, matrices, canvas widgets with custom JS. Bi-directional OSC. Used by live performers and installation artists worldwide.
- **License:** GPL-3.0
- **Relevance:** Study its XY canvas widget implementation for our coupling XY performance interface. The bi-directional OSC pattern is relevant if we add OSC support to XOlokun standalone. Note: now hosted on Framagit, not GitHub.
- **Priority:** P2 — study canvas widget XY implementation

### 5.3 Korg NTS-3 Kaoss Pad logueSDK
- **URL:** https://github.com/korginc/logue-sdk (loguSDK)
- **What it is:** Open SDK for developing effects for Korg NTS-3 kaoss pad. The NTS-3 XY pad drives Mute, FX Depth, and tap BPM. Community can write custom XY-mapped effects in C++.
- **License:** MIT
- **Relevance:** Hardware XY pad SDK reference. Our COUPLING_SPACE and COUPLING_MOTION performance parameters (Coupling Phase A) could be mapped to XY input. Study how Korg exposes the XY surface as two-axis parameter input to the DSP chain.
- **Priority:** P3 — hardware reference only

---

## 6. Novel Music Input Devices (Open Source)

### 6.1 IACHM — Real-time Movement-Based Music Player (matthieu-cervera)
- **URL:** https://github.com/matthieu-cervera/IACHM
- **What it is:** Real-time music player using phone accelerometric and gyroscopic data. Max/MSP core + Python + deep learning gesture classifier. Maps physical movement to music playback parameters.
- **License:** MIT
- **Relevance:** Motion-to-music pipeline reference. Our iOS AUv3 build could expose accelerometer/gyroscope data as macro sources (CHARACTER, MOVEMENT, COUPLING, SPACE), turning phone tilt into live performance gestures within XOlokun.
- **Priority:** P2 — architecture reference for iOS gesture-as-macro

### 6.2 Apple Silicon Accelerometer (olvvier)
- **URL:** https://github.com/olvvier/apple-silicon-accelerometer
- **What it is:** Reads the undocumented MEMS accelerometer + gyroscope on Apple Silicon MacBooks via IOKit HID. Maps motion data to arbitrary parameters (demonstrated with keyboard sounds).
- **License:** MIT
- **Relevance:** XOlokun standalone (macOS) could expose MacBook accelerometer as a live performance gesture source. Tilting the laptop modifies COUPLING or SPACE macro. Niche but memorable as a demo/feature.
- **Priority:** P3 — novelty feature, low priority

### 6.3 tonaljs/tonal
- **URL:** https://github.com/tonaljs/tonal
- **What it is:** Comprehensive music theory library in TypeScript. Functions for notes, intervals, chords, scales, modes, keys, tunings. 6,000+ stars. Active maintenance. npm packages: `@tonaljs/tonal`, `@tonaljs/chord`, etc.
- **License:** MIT
- **Relevance:** Foundation for the aquarium's "pitch-aware creature response" in Phase 2. When the user plays a chord, tonal.js identifies the chord quality, and creatures respond to harmonic tension (dissonant chords → agitated particles; consonant chords → smooth flows). Also relevant to the Chord Machine design.
- **Priority:** P0 — adopt for aquarium WebAudio synthesis + chord-aware visualization

### 6.4 MIDIano (Bewelge)
- **URL:** https://github.com/Bewelge/MIDIano
- **What it is:** JavaScript MIDI player and piano-learning webapp. WebMIDI input. Animated note falls, piano roll visualization. Chrome/Edge.
- **License:** MIT
- **Relevance:** Reference for animated MIDI playback visualization in the aquarium. The note-to-creature mapping in our aquarium Phase 1 could use the same MIDI roll animation pattern as a "schools of fish following the melody."
- **Priority:** P3 — visual reference only

---

## 7. Ocean / Aquatic Visualization (WebGL, GLSL, CSS)

### 7.1 three.js built-in ocean shader (mrdoob)
- **URL:** https://threejs.org/examples/webgl_shaders_ocean.html
- **What it is:** Built-in FFT water surface shader in Three.js examples (`examples/jsm/objects/Water.js`). Renders real-time animated water surface with reflection, refraction, sun position, wave parameters. Maintained as part of core Three.js.
- **License:** MIT
- **Relevance:** Fastest path to a water surface in the XO-OX.org aquarium. Phase 0 baseline. No extra dependency beyond Three.js (already in the web stack).
- **Priority:** P0 — immediate use for aquarium water surface

### 7.2 jbouny/fft-ocean
- **URL:** https://github.com/jbouny/fft-ocean
- **What it is:** Dedicated WebGL FFT ocean rendering library for Three.js. Physics-based Phillips spectrum model. Configurable wind speed/direction, wave height, foam. More accurate and customizable than the built-in Three.js water.
- **License:** MIT
- **Relevance:** Upgrade path for the aquarium water surface in Phase 1. Wind parameters could be audio-reactive: amplitude envelope drives wind speed, spectral centroid drives wave direction.
- **Priority:** P1 — aquarium Phase 1 upgrade to physics-based water

### 7.3 jbouny/ocean
- **URL:** https://github.com/jbouny/ocean
- **What it is:** Standalone realistic water shader for Three.js (separate repo, older). Caustics, depth fade, foam. Good reference for underwater caustics rendering.
- **License:** MIT
- **Relevance:** The underwater caustics effect directly serves the aquarium's "look up from beneath" camera angle. Caustics flicker that responds to audio amplitude = bioluminescent pulse effect.
- **Priority:** P2 — caustics reference for underwater camera angle

### 7.4 ywang170/three.js-glsl-simple-underwater-shader
- **URL:** https://github.com/ywang170/three.js-glsl-simple-underwater-shader
- **What it is:** Minimal GLSL underwater effect shader for Three.js. Chromatic aberration, color desaturation with depth, god-ray effect. Simple and educational.
- **License:** MIT
- **Relevance:** Quick win for the aquarium's underwater camera mode. The god-ray effect (light shafts from surface) is directly achievable with this shader and contributes substantially to visual depth.
- **Priority:** P1 — implement for aquarium underwater camera mode

### 7.5 anikoh/Bioluminescence
- **URL:** https://github.com/anikoh/Bioluminescence
- **What it is:** Game project inspired by deep-sea bioluminescent creatures (memory-card/minesweeper hybrid). Includes actual bioluminescence glow particle assets and visual logic.
- **License:** MIT
- **Relevance:** Source of bioluminescence glow art assets and color palette reference (teal/cyan/violet/gold). Our ORGANISM engine's "Emergence Lime" and OCEANDEEP's bioluminescent exciter have direct visual counterparts in this palette.
- **Priority:** P2 — asset and color reference

### 7.6 kiritoInd/sea-shaders
- **URL:** https://github.com/kiritoInd/sea-shaders
- **What it is:** Sea shader using GLSL noise algorithms (Simplex noise, Perlin noise, FBM). Generates organic wave-like surfaces procedurally. Educational and well-commented.
- **License:** MIT
- **Relevance:** GLSL noise patterns directly applicable to: (a) the aquarium background procedural texture, (b) the ORGANISM cellular automata engine's visual companion shader, (c) the OCEANDEEP bioluminescent exciter visualization.
- **Priority:** P2 — GLSL noise patterns for aquarium and engine viz

### 7.7 ShaderParticleEngine — deep sea bioluminescence
- **URL:** https://github.com/squarefeet/ShaderParticleEngine
- (See also Section 2.9 above — dual relevance here)
- **Specific use:** The "colour over lifetime" and "size over lifetime" features reproduce the fade-in/fade-out pulse of bioluminescent organisms. GPU simulation frees CPU for audio DSP.
- **Priority:** P1 — bioluminescence creature effect

### 7.8 Nugget8/Three.js-Ocean-Scene
- **URL:** https://github.com/Nugget8/Three.js-Ocean-Scene
- **What it is:** Procedural skybox + water shader optimized for mobile in Three.js. Lightweight, mobile-first. Includes atmospheric scattering (sky gradient from deep blue to cyan at horizon).
- **License:** MIT
- **Relevance:** The aquarium site needs to work on mobile (fans browsing on phones at shows). This is the mobile-optimized water alternative to jbouny/fft-ocean.
- **Priority:** P2 — mobile fallback for aquarium water surface

### 7.9 CSS underwater animation (tdoughty — CodePen)
- **URL:** https://codepen.io/tdoughty/pen/ZZqgQq
- **What it is:** Pure CSS underwater animation — no WebGL dependency. Bubble particles, color depth fade, light shaft simulation using CSS gradients and keyframe animations.
- **License:** CodePen public (free to adapt)
- **Relevance:** Fallback for browsers without WebGL support. The CSS-only aquarium can serve as progressive enhancement: if Three.js fails to initialize (old hardware), the CSS fallback still communicates the aquatic identity.
- **Priority:** P2 — CSS fallback layer for aquarium

---

## 8. Design System Component Libraries — Dark Mode / Data Viz

### 8.1 shadcn/ui
- **URL:** https://ui.shadcn.com / https://github.com/shadcn-ui/ui
- **What it is:** Copy-and-paste React component library built on Radix UI + Tailwind CSS. Not a traditional npm package — you own the components. 70,000+ GitHub stars. Dark mode first-class (CSS variables, one-click switch). Accessibility via Radix primitives. Actively maintained with Config 2025 updates.
- **License:** MIT
- **Relevance:** The XO-OX.org site and the audio-xpm-creator app both use Next.js + Tailwind. shadcn/ui is the most natural component foundation — its dark mode token system and component ownership model align with our "Gallery Model" philosophy (own the shell, customize the accent). Replace ad-hoc UI components with shadcn/ui primitives.
- **Priority:** P0 — adopt as component foundation for XO-OX.org and xpm-creator

### 8.2 Tremor (Vercel / tremorlabs)
- **URL:** https://github.com/tremorlabs/tremor / https://www.tremor.so
- **What it is:** React component library purpose-built for analytics dashboards and charts. 35+ components: KPI cards, line/bar/area/donut charts, data tables, filter controls. Built on Recharts + Radix UI + Tailwind. Now fully free and open source (acquired by Vercel). 16,000+ stars. Dark mode support.
- **License:** Apache-2.0
- **Relevance:** Direct candidate for the XO-OX.org fleet analytics dashboard, preset DNA radar charts, and seance score displays. "Show the data, hide the chrome" design philosophy aligns with our technical precision brand. Pairs with shadcn/ui.
- **Priority:** P1 — use for fleet analytics and preset DNA visualization on site

### 8.3 Dark Mode Designsystem (Figma Community)
- **URL:** https://www.figma.com/community/file/1003274936198410603/dark-mode-designsystem
- **What it is:** Full design system built for dark/light mode with 200+ adaptable components. Updated April 2025. Desktop/tablet optimized.
- **License:** Figma Community (free)
- **Relevance:** Token reference for XOlokun dark mode toggle. Our Gallery Model is light-mode-first, but the dark mode variant needs a systematic color token approach — this file provides the reference architecture.
- **Priority:** P2 — dark mode token architecture reference

### 8.4 Untitled UI (Figma + React)
- **URL:** https://www.untitledui.com
- **What it is:** Premium Figma UI kit + React component library. Color variable system for dark mode. Supports Figma Config 2025 features. One-click dark mode across entire component library.
- **License:** Commercial (free tier available for Figma)
- **Relevance:** Highest quality dark mode component reference. The one-click dark mode architecture (CSS variable override per theme) is the exact approach we should take for XOlokun dark mode toggle.
- **Priority:** P2 — dark mode architecture reference (study free tier)

### 8.5 VCV Rack source (VCVRack/Rack)
- **URL:** https://github.com/VCVRack/Rack
- **What it is:** Open-source virtual Eurorack synthesizer. Uses NanoVG (antialiased 2D vector rendering on OpenGL) for all UI. Every module is a self-contained component with its own panel SVG. The patch cable routing is achieved via a drag-from-port system on a canvas overlay.
- **License:** GPL-3.0
- **Relevance:** While GPL prevents direct code adoption, VCV Rack is the definitive architectural reference for a node-graph synthesizer UI at scale. Study: (a) how cable routing prevents Z-order conflicts, (b) how module panels maintain independent state while sharing the global theme, (c) NanoVG as a lighter-weight alternative to full JUCE Component trees for a future coupling graph view.
- **Priority:** P1 — architectural study for coupling graph view

---

## Summary Table

| # | Name | Category | Priority | License | URL |
|---|------|----------|----------|---------|-----|
| 1 | awesome-juce | JUCE UI | P0 | MIT | https://github.com/sudara/awesome-juce |
| 2 | foleys_gui_magic | JUCE UI | P1 | BSD-3 | https://github.com/ffAudio/foleys_gui_magic |
| 3 | melatonin_blur | JUCE UI | P0 | MIT | https://github.com/sudara/melatonin_blur |
| 4 | melatonin_inspector | JUCE UI | P1 | MIT | https://github.com/sudara/melatonin_inspector |
| 5 | Pamplejuce | JUCE CI | P1 | MIT | https://github.com/sudara/pamplejuce |
| 6 | Surge XT | JUCE UI | P1 | GPL-3 | https://github.com/surge-synthesizer/surge |
| 7 | Vital synth | JUCE UI | P0 | GPL-3 | https://github.com/mtytel/vital |
| 8 | react-juce | JUCE+JS | P2 | MIT | https://github.com/JoshMarler/react-juce |
| 9 | WebUISynth | JUCE+JS | P2 | MIT | https://github.com/tomduncalf/WebUISynth |
| 10 | DISTRHO Cardinal | Modular UI | P1 | GPL-3 | https://github.com/DISTRHO/Cardinal |
| 11 | audioMotion-analyzer | Web Viz | P0 | AGPL-3 | https://github.com/hvianna/audioMotion-analyzer |
| 12 | wavesurfer.js | Web Viz | P1 | BSD-3 | https://github.com/katspaugh/wavesurfer.js |
| 13 | Peaks.js | Web Viz | P3 | LGPL-3 | https://github.com/bbc/peaks.js |
| 14 | awesome-audio-visualization | Index | P1 | CC0 | https://github.com/willianjusten/awesome-audio-visualization |
| 15 | three.quarks | Particles | P0 | MIT | https://github.com/Alchemist0823/three.quarks |
| 16 | three-nebula | Particles | P1 | MIT | https://github.com/creativelifeform/three-nebula |
| 17 | jbouny/fft-ocean | Ocean | P1 | MIT | https://github.com/jbouny/fft-ocean |
| 18 | Three.js ocean shader | Ocean | P0 | MIT | https://threejs.org/examples/webgl_shaders_ocean.html |
| 19 | ShaderParticleEngine | GPU Particles | P1 | MIT | https://github.com/squarefeet/ShaderParticleEngine |
| 20 | 3D MIDI audio particles | Viz | P1 | MIT | https://github.com/adarkforce/3d-midi-audio-particles-threejs |
| 21 | SeaSynth Figma | Figma UI | P0 | Figma | https://www.figma.com/community/file/1120992547387371994 |
| 22 | VST Plugin Mockup Figma | Figma UI | P1 | Figma | https://www.figma.com/community/file/1476177446001979362 |
| 23 | Skeuomorphic Audio UI | Figma UI | P2 | Figma | https://www.figma.com/community/file/859424303965872933 |
| 24 | Synthwave UI Kit | Figma UI | P2 | Figma | https://www.figma.com/community/file/1114174373561311418 |
| 25 | Data Viz Dark Mode | Figma UI | P2 | Figma | https://www.figma.com/community/file/1060951969983411754 |
| 26 | Music GUI/UI Kit | Figma UI | P2 | Figma | https://www.figma.com/community/file/1362930821190233244 |
| 27 | WeAreROLI/mpejs | MPE | P0 | MIT | https://github.com/WeAreROLI/mpejs |
| 28 | mpe.js | MPE | P1 | MIT | https://mpe.js.org |
| 29 | microtonal-seaboard | MPE | P2 | MIT | https://github.com/euwbah/microtonal-seaboard |
| 30 | isomorphic-keyboards | MPE/Grid | P2 | MIT | https://github.com/pianosnake/isomorphic-keyboards |
| 31 | hexatone | MPE/Grid | P2 | MIT | https://github.com/000masa000/hexatone |
| 32 | touchmidi | XY Pad | P1 | MIT | https://github.com/benc-uk/touchmidi |
| 33 | Open Stage Control | XY Pad | P2 | GPL-3 | https://framagit.org/open-stage-control/open-stage-control |
| 34 | IACHM motion music | Novel Input | P2 | MIT | https://github.com/matthieu-cervera/IACHM |
| 35 | Apple Silicon accel | Novel Input | P3 | MIT | https://github.com/olvvier/apple-silicon-accelerometer |
| 36 | tonaljs/tonal | Music Theory | P0 | MIT | https://github.com/tonaljs/tonal |
| 37 | jbouny/ocean (caustics) | Ocean | P2 | MIT | https://github.com/jbouny/ocean |
| 38 | underwater shader | Ocean | P1 | MIT | https://github.com/ywang170/three.js-glsl-simple-underwater-shader |
| 39 | sea-shaders GLSL noise | Ocean | P2 | MIT | https://github.com/kiritoInd/sea-shaders |
| 40 | shadcn/ui | Design System | P0 | MIT | https://ui.shadcn.com |
| 41 | Tremor | Design System | P1 | Apache-2 | https://github.com/tremorlabs/tremor |
| 42 | VCV Rack source | Modular UI | P1 | GPL-3 | https://github.com/VCVRack/Rack |

---

## P0 Action Items

These are the highest-priority items that should drive immediate work:

1. **melatonin_blur** — Integrate into XOlokun JUCE build for engine glow rendering. Direct 10-30x blur speedup enables the accent-color glow halos on engine cards.

2. **Vital synthesizer UI** — Deep-study session: how they handle animated wavetable display and coupling/mod visualizer. Extract patterns for our coupling arc in the Gallery Model.

3. **three.quarks + Three.js ocean shader** — Begin aquarium Phase 0 prototype. Ocean surface (built-in shader, immediate) + particle layer (three.quarks with force-field attractors = coupling metaphor).

4. **audioMotion-analyzer** — Prototype real-time spectrum display in the aquarium site, driven by Web Audio API from XPN preview audio. Note AGPL license — ensure disclosure if bundled server-side.

5. **SeaSynth Figma file** — Duplicate into XO-OX Figma workspace. Extract: (a) XY pad component, (b) modulation matrix panel, (c) continuous keyboard surface. These three components are the immediate missing pieces in our Gallery Model spec.

6. **WeAreROLI/mpejs** — Add to aquarium Phase 2 tech spec. The normalized activeNote object model maps directly to our 5D PlaySurface gesture input (strike/lift/slide/glide/press).

7. **tonaljs/tonal** — Add to package.json for aquarium. Chord quality detection drives harmonic tension → creature behavior mapping.

8. **shadcn/ui** — Evaluate against current XO-OX.org + xpm-creator component setup. Migrate where appropriate.

---

*Next scan scheduled: 2026-04-14 (3-week cadence). Focus areas for next scan: CLAP plugin ecosystem, iOS AUv3 UI toolkits, WebAssembly audio DSP (AudioWorklet + Emscripten), generative AI music interface research.*
