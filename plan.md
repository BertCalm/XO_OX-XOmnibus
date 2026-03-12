# XPN Export System — Enhancement Plan

## Current State Assessment

The XPN export pipeline consists of:
- **XPNExporter.h** (434 lines) — rendering pipeline + XPM generation + validation
- **XPNCoverArt.h** (591 lines) — procedural cover art (9 styles, 20 engines)
- **Python tools** — xpn_bundle_builder.py, xpn_drum_export.py, xpn_cover_art.py
- **Specs** — xo_mega_tool_xpn_export.md, xpn_sound_shape_rendering.md

The C++ exporter is functional but has a **placeholder WAV renderer** (generates silence) and several gaps vs. the spec.

---

## 1. Design Alignment (spec gaps)

### 1a. Sound Shape Classification System
The spec (`xpn_sound_shape_rendering.md`) defines 6 Sound Shapes (Transient, Sustained, Evolving, Bass, Texture, Rhythmic) with per-shape render settings — but XPNExporter has no awareness of this. Every preset gets the same render duration/strategy.

**Enhancement:** Add `SoundShapeClassifier` that analyzes preset DNA + engines to select optimal render settings:
- Transient → short render (1s hold, 0.5s tail), more velocity layers
- Evolving → long render (6s hold, 3s tail), fewer notes
- Bass → OctavesOnly strategy, 24-bit mandatory
- etc.

### 1b. Missing `prism_fractal` Cover Art Style
XPNCoverArt assigns `"prism_fractal"` to OBLIQUE but never implements it — falls through to `styleFreqBands`. Need a unique prismatic/refractive visual style.

### 1c. Drum Kit Export (C++ parity)
Python `xpn_drum_export.py` handles ONSET drum kit export, but no C++ equivalent exists. Users can't export drum kits from within the plugin.

---

## 2. Performance Improvements

### 2a. Parallel Note Rendering
Currently renders notes sequentially in a single loop. Notes within a preset are independent and can be rendered in parallel using a thread pool. For a typical export (25 notes × 3 velocity layers = 75 WAVs per preset), this could be 4-8x faster on modern hardware.

**Implementation:** Use `juce::ThreadPool` with worker count = `std::thread::hardware_concurrency() - 1`. Each job renders one note/velocity combination. Synchronize progress updates with atomic counters.

### 2b. Export Size Estimation
No way to predict output size before starting. Add `estimateExportSize()` that calculates:
`notes × velLayers × presets × (renderSeconds + tailSeconds) × sampleRate × channels × (bitDepth/8)`

### 2c. Cover Art Render Optimization
`styleWaveMorph` iterates every 3rd pixel across 2000×2000 canvas for 5 layers — ~2.2M fill operations. Could use larger tile sizes or render at 500×500 and upscale with the procedural pass, then composite.

### 2d. Normalization Pass
`normCeiling` (-0.3 dBFS) is defined in `RenderSettings` but never applied. After rendering, a peak-scan + gain pass should normalize to the target ceiling.

---

## 3. UI/UX Enhancements

### 3a. Export Progress Dialog
No export UI exists in Source/UI/. Need:
- Preset selection list (filter by mood/engine/tag)
- Render settings panel (strategy, velocity layers, bit depth, sample rate)
- Bundle config (name, description, cover engine picker)
- Real-time progress bar with per-note granularity
- Size estimate display (updates as settings change)
- Cancel button with clean abort
- Completion summary (files created, total size, time elapsed)

### 3b. Pre-Export Validation Panel
`validateBatch()` exists but isn't surfaced to UI. Show warnings/errors before export starts with fix-it suggestions (e.g., "2 presets have names > 30 chars").

### 3c. Cover Art Preview
Allow previewing the procedural cover art before export. Show a 200×200 thumbnail that regenerates when engine/seed changes. Add seed randomize button.

### 3d. Export Presets (Quick Profiles)
Common export configurations as one-click profiles:
- "MPC Standard" (EveryMinor3rd, 3 vel layers, 24-bit, 48kHz)
- "Lightweight" (OctavesOnly, 1 vel layer, 16-bit, 44.1kHz)
- "Maximum Quality" (Chromatic, 3 vel layers, 24-bit, 48kHz)

---

## 4. Accessibility

### 4a. Screen Reader Support for Export Dialog
All export UI controls need JUCE accessibility attributes:
- `setAccessible(true)`, `setTitle()`, `setDescription()` on every interactive element
- Focus ring drawing (already in `A11y` namespace in XOmnibusEditor.h)
- Keyboard tab order for all controls
- Announce progress updates to screen readers via `AccessibilityHandler::postAnnouncement()`

### 4b. Keyboard-Only Export Flow
Ensure entire export workflow is navigable without mouse:
- Tab between sections, Enter to start export, Escape to cancel
- Arrow keys to navigate preset selection list
- Space to toggle preset inclusion

### 4c. High Contrast Mode for Progress
Progress indicators should work in both light/dark mode with WCAG AA contrast. Use engine accent colors for the progress bar fill with sufficient contrast against backgrounds.

### 4d. Export Completion Notification
Post system notification on export completion (especially for long exports). Use `juce::SystemTrayIconComponent` or native notification API.

---

## 5. QA Enhancements

### 5a. Export Test Suite
**Zero export tests exist.** Need a new `Tests/ExportTests/XPNExportTests.cpp`:

- **XPM Rule Enforcement**: Verify every generated XPM has `KeyTrack="True"`, `RootNote="0"`, empty layers have `VelStart="0"`
- **WAV Format Validation**: Correct sample rate, bit depth, channel count, non-zero length
- **Filename Sanitization**: Special chars, Unicode, max length, collisions
- **Note Strategy Coverage**: Verify correct MIDI note sets for each strategy
- **Velocity Layer Ranges**: No gaps, no overlaps, cover 0-127
- **Batch Validation**: Duplicate names detected, DNA range enforcement
- **Bundle Structure**: Required files present (Manifest.xml, Preview.png, Keygroups/)
- **Cancel Mid-Export**: Verify clean abort with no corrupt files
- **Empty Preset List**: Graceful error return
- **Large Batch Stress Test**: 100+ presets don't OOM

### 5b. Cover Art Regression Tests
- All 20 engine IDs resolve to correct accent/style/label
- Legacy aliases (SNAP, MORPH, etc.) resolve correctly
- Output files exist and are valid PNGs
- 1000×1000 and 2000×2000 dimensions correct

### 5c. Round-Trip XPM Validation
Parse generated XPM XML back and verify:
- Zone key ranges cover 0-127 without gaps
- All sample file references exist
- Velocity ranges within each zone are contiguous and complete

---

## 6. Error Handling & Robustness

### 6a. File I/O Error Handling
Currently `createDirectory()`, `createOutputStream()` failures are silently ignored. Add proper error propagation:
- Check directory creation success
- Check file write success
- Handle disk full, permissions, path too long
- Return specific error messages in `ExportResult`

### 6b. Cancellation Granularity
Currently only checks cancellation between presets. For large presets (Chromatic = 73 notes × 3 layers = 219 WAVs), need per-note cancellation check.

### 6c. Atomic Export (temp → rename)
Write to a temp directory, then rename to final path on success. If export fails or is cancelled, no partial bundle remains to confuse the user.

---

## Implementation Priority

| Priority | Item | Impact | Effort |
|----------|------|--------|--------|
| **P0** | 5a. Export test suite | QA foundation | Medium |
| **P0** | 6a. File I/O error handling | Reliability | Low |
| **P1** | 1a. Sound Shape classifier | Spec alignment | Medium |
| **P1** | 2a. Parallel note rendering | Performance | Medium |
| **P1** | 2d. Normalization pass | Audio quality | Low |
| **P1** | 6b. Cancellation granularity | UX | Low |
| **P2** | 3a. Export progress dialog | UI/UX | High |
| **P2** | 1b. prism_fractal style | Completeness | Low |
| **P2** | 2b. Size estimation | UX | Low |
| **P2** | 3b. Pre-export validation UI | UX | Medium |
| **P2** | 6c. Atomic export | Robustness | Low |
| **P3** | 4a-d. Accessibility | A11y | Medium |
| **P3** | 1c. C++ drum export | Feature parity | High |
| **P3** | 3c. Cover art preview | UX | Low |
| **P3** | 3d. Export presets | UX | Low |
| **P3** | 5b-c. Cover art + XPM tests | QA depth | Medium |
