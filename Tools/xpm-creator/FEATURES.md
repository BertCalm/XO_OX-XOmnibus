# XO_OX — Feature List

**Version:** 0.1.0
**Platform:** Web (Next.js 14) + Desktop (Tauri v2 / macOS)
**Stack:** React 18, TypeScript, Zustand, Tailwind CSS, Web Audio API

---

## Core Architecture

### Pad System (128 Pads)
- 8 banks x 16 pads = 128 total pads
- Standard MPC 4x4 grid layout (bottom-left to top-right numbering)
- MPC-standard MIDI note mapping per bank
- Click-to-select + click-to-audition (loaded pads play on click)
- Visual pad states: empty, loaded, active (selected), playing (flash)
- Trigger mode indicators on pad cells (oneshot, noteon, noteoff)
- Mute group badge overlay (M1-M32)
- Layer count dot indicators (color-coded, up to 8)
- Sample name preview text on loaded pads
- Drag-and-drop from sample library to pads
- OS file drag-and-drop directly onto pads (multi-file = multi-layer)
- File count indicator during drag-over
- Celebration animation on first sample load (confetti)

### Layer System (8 Layers per Pad)
- Up to 8 independent layers per pad
- Per-layer controls: volume, pan, pitch, tune coarse, tune fine
- Velocity range (velStart/velEnd) per layer
- Root note assignment per layer
- Key tracking toggle
- Randomization: pitch random, volume random, pan random
- Slice controls: start, end, loop, loop start
- Playback direction (forward/reverse)
- Sample offset
- Layer probability (0-100%) for humanized triggering
- Layer activation toggle

### Play Modes
- **Simultaneous** — all active layers play together
- **Velocity** — layers selected by incoming velocity range
- **Cycle** — round-robin through layers sequentially
- **Random** — random layer selection with anti-repeat logic

### Trigger Modes
- **One-Shot** (default) — plays to completion regardless of note off
- **Note-Off** — sustains while held, stops on release
- **Note-On** — retriggers on each press, stops previous

### Mute Groups
- 32 mute groups available
- Visual grid assignment UI
- Click-free choke with 8ms gain fade (anti-click)
- Cross-pad muting within same group

---

## Audio Engine

### Playback Engine (Web Audio API)
- Real-time pad audition with full DSP chain
- Audio graph per layer: BufferSource -> GainEnvelope -> BiquadFilter -> StereoPanner -> LayerGain -> destination
- Exponential volume envelope curves (perceptually natural decay)
- Linear filter frequency envelope curves
- Pitch via playbackRate: 2^((tuneCoarse + tuneFine/100) / 12)
- Velocity-scaled gain (0-127 mapped to 0-1)
- Parameter randomization per voice (volume, pan, pitch)
- Voice re-triggering with automatic cleanup
- Probability gate: per-layer random roll before playback

### Envelope System (AHDSR)
- Per-pad volume envelope: Attack, Hold, Decay, Sustain, Release
- Per-pad filter envelope: Attack, Hold, Decay, Sustain, Release
- Exponential curves for volume, linear for filter frequency
- Web Audio AudioParam automation scheduling
- Proper release phase with cancelScheduledValues

### Filter System
- Filter types: Off, Low-pass, Band-pass, High-pass
- Per-pad cutoff (0-1, exponentially mapped 60Hz-20kHz)
- Per-pad resonance (0-1, mapped to Q 0.5-20)
- Bipolar filter envelope amount (-1 to +1)
  - Positive: sweep up from cutoff toward 20kHz
  - Negative: sweep down from cutoff toward 60Hz

### Audio Buffer Cache
- Decoded AudioBuffer cache for zero-latency triggering
- Async decode on first access, instant return on subsequent
- Cache invalidation per sample or global clear
- Pre-decode on sample import

### Audio Context
- Singleton AudioContext at system native sample rate
- Lazy initialization on first user interaction

---

## Sample Management

### Import
- Drag-and-drop audio file import (WAV, AIFF, MP3, OGG, FLAC, M4A)
- Multi-file batch import
- Audio dropzone with visual feedback
- MP4 audio extraction
- Filename parser for auto-detecting sample metadata
- Waveform peak generation on import

### Sample Library
- Scrollable sample list with search
- Sample metadata display (duration, sample rate, channels, bit depth)
- Inline sample player with playback controls
- Drag from library to pads
- Sample renaming (individual + batch with pattern)
- Quick Assign: assign all samples to sequential pads

### Batch Tools
- Batch normalize
- Batch trim silence
- Batch pitch shift
- Batch reverse

### Waveform Editor
- Interactive waveform canvas with zoom
- Click-to-select chop regions
- Manual selection via drag
- Region playback (click to preview)
- Trim handles for adjusting chop boundaries

---

## Auto-Chop System

### Transient Detection
- Energy-based transient detection with adjustable sensitivity
- Zero-crossing snap for click-free boundaries
- Configurable minimum gap between chops

### Chop Manipulation Toolbar
- **Normalize** — peak normalize selected chop region
- **Reverse** — reverse audio data
- **Fade In** — 10% duration linear fade-in
- **Fade Out** — 10% duration linear fade-out
- **Pitch Up/Down** — semitone pitch shifting
- **LP Filter** — low-pass filter at 2kHz
- **HP Filter** — high-pass filter at 300Hz
- All operations are non-destructive (create new sample)

### Chop -> Pads
- One-click: auto-chop + extract + normalize + assign to pads
- Assigns up to 16 slices to current bank's pads (layer 0)
- Optional normalization toggle
- Micro-fade on slice boundaries (32 samples, ~0.7ms)

### Extract
- Extract single selection to new sample
- Extract all chop regions as individual samples

---

## Creative Tools (13+)

### Vibe Check
- Analyzes sample characteristics (brightness, dynamics, density)
- Suggests optimal pad settings based on analysis
- One-click "Apply Vibe" to configure pad parameters

### Auto-Choke
- Intelligent mute group assignment
- Analyzes sample categories to group related sounds
- Hi-hat open/closed auto-pairing

### Tail Taming (Batch)
- Batch process sample tails
- Fade-out modes: linear, exponential, logarithmic
- Configurable tail duration
- Preview before apply

### Space Fold
- Granular-inspired audio manipulation
- Grain size control (10ms-500ms)
- Scatter amount for randomized grain positioning
- Density control
- Direction modes: forward, reverse, alternate
- Real-time preview

### Spectral Air
- Harmonic enhancer for adding "air" to samples
- Brightness control
- Shimmer effect with modulation
- High-frequency emphasis
- Subtle saturation

### Cycle Engine
- LFO-driven parameter modulation
- Multiple waveform shapes: sine, triangle, saw, square, random
- Depth and rate controls
- Target parameter selection (volume, pan, pitch, filter)
- Sync to tempo option

### Time Traveler
- Tape-style effects processor
- Tape speed variation (wow/flutter)
- Tape saturation
- Vinyl crackle layer
- Age simulation (high-cut, noise)

### Velocity Curve Selector
- Multiple velocity response curves
- Linear, logarithmic, exponential, S-curve
- Custom curve editor
- Visual curve display

### Auto-Layer Generator
- Intelligent multi-layer creation
- Velocity-split layer generation
- Round-robin layer setup
- Randomized layer variation

### Ghost Note Generator
- Generates subtle ghost note variations
- Velocity reduction with randomization
- Timing variation
- Pitch micro-variation

### Humanizer
- Applies human-like variation to pad parameters
- Tune fine randomization
- Volume randomization
- Pan randomization
- Offset randomization (timing)
- Pitch random amount

### Pad Resampling
- Renders complete pad output (all layers + envelopes + filters) to new WAV
- OfflineAudioContext rendering for accurate offline processing
- Respects play mode, velocity, and all DSP settings
- Adds result back to sample library

### Content Art Generator
- AI-inspired cover art generation for expansion packs
- Pattern-based visual generation
- Color scheme customization

---

## XPM Program Export

### Drum Program Builder
- Full MPC-standard XPM 2.1 XML generation
- Per-instrument volume + filter envelope export (AHDSR)
- Filter type, cutoff, resonance, envelope amount export
- Trigger mode mapping (oneshot=0, noteoff=1, noteon=2)
- Zone play mode mapping (simultaneous=1, velocity=2, cycle=3, random=4)
- 128-pad MIDI note mapping
- Mute group mapping (pad group map)
- Layer probability export
- All layer parameters: volume, pan, pitch, tune, velocity range, slice settings, etc.

### Keygroup Program Builder
- Multi-sample keygroup program generation
- Note range splitting across instruments
- Pitch bend range and wheel-to-LFO settings

### Modulation System
- Modulation routing presets
- Sources: Velocity, Aftertouch, ModWheel, LFO1, PitchEnv, FilterEnv, AmpEnv
- Destinations: FilterCutoff, FilterResonance, Volume, Pan, Pitch, LFO1Rate, LFO1Depth
- LFO settings: rate, shape (sine/triangle/saw/square/random), sync

---

## XPN Expansion Pack System

### XPN Packager
- Create MPC expansion packs (.xpn = renamed .zip)
- Bundle multiple programs + samples + metadata
- Expansion XML manifest generation
- Cover art embedding
- Progress tracking during packaging

### XPN/XPM Ingestion (Kit Upcycler)
- Import existing .xpn expansion packs
- Import existing .xpm program files
- Parse and extract all instruments, layers, and settings
- Reconstruct pad assignments from imported data
- Sample extraction and library integration

---

## MIDI Controller Support

### Web MIDI Input
- Browser-native Web MIDI API integration
- Device enumeration and selection
- Hot-plug device detection
- MPC-standard note mapping per bank (A + B)
- Sequential fallback mapping for higher banks (C-H)
- Velocity-sensitive pad triggering
- Note-off support for noteoff trigger mode
- Activity indicator (flash on note events)
- Bank sync with UI bank selector
- Ghost trigger prevention (only mapped notes fire)

---

## Cloud Storage Integration

### Providers
- Google Drive (OAuth2)
- Microsoft OneDrive (OAuth2)
- Dropbox (OAuth2)

### Cloud Browser
- Browse remote folders
- Upload/download samples and programs
- Create folders
- File metadata display

### Cloud Export
- Direct export of XPM/XPN to cloud storage
- Provider selection
- Folder navigation
- Progress tracking

---

## Desktop App (Tauri v2)

### Platform
- macOS native app via Tauri v2
- Minimum macOS 10.15 (Catalina)
- Bundle targets: .dmg and .app

### Window Configuration
- Default size: 1400x900
- Minimum size: 1024x700
- Centered on launch
- Resizable with decorations

### Native Capabilities
- File system read/write (dialog-based)
- Native file open/save dialogs
- Shell integration
- Debug logging (development builds)

### Progressive Enhancement
- `isTauri()` detection for runtime environment
- Dynamic Tauri API imports (no bundler errors in web mode)
- Graceful fallback to web APIs when not in Tauri

---

## UI/UX System

### Theme System
- Multiple built-in themes
- Dynamic CSS variable system
- Texture overlay support
- Theme switcher component
- Theme persistence

### Layout
- Responsive workspace layout
- Header with app branding and navigation
- Sidebar with section navigation
- Tabbed content areas (Samples, Pads, Program, Export)

### UI Components
- Button (variants: primary, secondary, accent, ghost)
- Card containers
- Modal dialogs
- Progress bars
- Sliders with labels
- Tabs
- Tooltips with rich content
- Info popovers with contextual help

### Command Palette
- Keyboard-activated command search (Cmd+K)
- Fuzzy search across all actions
- Quick access to tools, exports, settings

### Keyboard Shortcuts
- Comprehensive shortcut system
- Visual shortcut reference panel
- Customizable bindings

### Quick Start Guide
- First-launch onboarding
- Step-by-step workflow guidance

### Toast Notifications
- Success, error, warning, info variants
- Auto-dismiss with configurable duration
- Stack-based display

### Error Handling
- Global error boundary (React)
- Error log viewer with filtering
- Categorized error tracking
- Error store with persistence

---

## State Management (Zustand)

| Store | Purpose |
|-------|---------|
| `audioStore` | Sample library, import/export |
| `padStore` | 128 pads, layers, assignments, play/trigger modes |
| `envelopeStore` | Per-pad AHDSR volume + filter envelopes |
| `playbackStore` | Active voices, playing state, cycle/random counters |
| `projectStore` | Project metadata, save/load |
| `exportStore` | Export configuration and progress |
| `themeStore` | Theme selection and customization |
| `historyStore` | Undo/redo state tracking |
| `toastStore` | Notification queue |
| `errorStore` | Error log and categorization |
| `cloudStore` | Cloud provider auth and file operations |

---

## Data Persistence

### IndexedDB (via idb)
- Project save/load
- Sample buffer storage
- Offline-capable

### Project Manager
- Create, open, save, delete projects
- Project metadata (name, date, sample count)
- Auto-save capability

---

## Audio Processing Pipeline

| Processor | Location | Purpose |
|-----------|----------|---------|
| `audioSlicer` | Chop system | Transient detection, slice, normalize, zero-crossing snap, micro-fade |
| `chopProcessors` | Chop toolbar | Fade in/out, reverse, filter (offline BiquadFilter render) |
| `pitchShifter` | Chop + tools | Semitone/cent pitch shifting |
| `pitchDetector` | Auto-detect | Autocorrelation-based pitch detection |
| `humanizer` | Pad tools | Parameter randomization for natural feel |
| `padPlaybackEngine` | Real-time | Full DSP chain with envelope scheduling |
| `padResampler` | Resample tool | OfflineAudioContext full pad render |
| `wavEncoder` | Export | 16/24-bit WAV encoding |
| `sculptingProcessors` | Creative tools | Advanced audio sculpting effects |
| `velocityCurves` | Velocity tool | Custom velocity response mapping |
| `autoChoke` | Mute groups | Intelligent auto-choke assignment |
| `vibeCheck` | Analysis | Sample characteristic analysis |
| `tailTaming` | Batch tool | Tail fade processing |
| `spaceFold` | Creative | Granular manipulation |
| `spectralAir` | Creative | Harmonic enhancement |
| `cycleEngine` | Creative | LFO modulation |
| `timeTraveler` | Creative | Tape/vinyl effects |
| `ghostNoteGenerator` | Creative | Ghost note variations |
| `autoLayerGenerator` | Creative | Intelligent layer creation |
| `sampleCategorizer` | Organization | AI-like sample type detection |
| `filenameParser` | Import | Metadata extraction from filenames |
| `uiSounds` | UX | Interface audio feedback |

---

## File Format Support

### Input
- WAV (PCM, all bit depths)
- AIFF / AIFF-C
- MP3
- OGG Vorbis
- FLAC
- M4A / AAC (via MP4 extraction)
- XPM (MPC program import)
- XPN (MPC expansion pack import)

### Output
- WAV (16-bit, 24-bit)
- XPM (MPC 2.1 drum/keygroup programs)
- XPN (MPC expansion packs)

---

## Easter Eggs
- First-sample celebration animation
- Branded greetings system
- UI sound effects for interactions

---

## Technical Specifications

- **159 TypeScript/TSX source files**
- **16 Zustand state stores**
- **34 audio processing modules**
- **System native sample rate**
- **128 pads, 8 layers each = 1024 possible layer slots**
- **32 mute groups**
- **127 MIDI velocity levels**
- **128 MIDI note range**
- **XPM 2.1 XML format compatibility**
- **Static export for Tauri (Next.js `output: 'export'`)**

---

*XO_OX (pronounced "zoe ox") — Professional MPC XPM/XPN program creator*
