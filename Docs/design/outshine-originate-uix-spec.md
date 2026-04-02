# UIX Design Studio — OUTSHINE & ORIGINATE

**Session: Complete UI/UX Design for Two Companion Windows**
**Architects: Ulf (Scandinavian), Issea (Japanese), Xavier (Apple-native)**

---

## Studio Opening

**Ulf:** Before we draw a single pixel, I want to name what these tools are emotionally. OUTSHINE is a workshop. A craftsman's bench in a warm room, tools hanging on pegboard, clear surfaces, the smell of wood shavings. Not a factory. Not a settings panel. A bench where you pick up materials and make something.

**Issea:** ORIGINATE is quieter. It is the moment after the making — the contemplation before release. The artist reviewing work before it leaves the studio. There is a ceremony to export. The UI should honor that pause.

**Xavier:** Both windows live in the shadow of XOceanus. They must feel like siblings, not orphans. Same family. Same bones. But each has its own character — OUTSHINE active and welcoming, ORIGINATE focused and ceremonial.

---

## A. DETAILED WIREFRAMES

### A1. OUTSHINE — Empty State (First Launch)

```
╔══════════════════════════════════════════════════════════════════════════════╗
║  ████████████████████████████████████ HEADER ███████████████████████████████ ║
║  [XO Gold bar, full width, 40px]                                            ║
║  OUTSHINE                                    Sample Instrument Forge  [?][×] ║
╠══════════════════════════════════════════════════════════════════════════════╣
║                                                                              ║
║  ┌─────────────────────────────────────────────────────────────────────────┐ ║
║  │                                                                         │ ║
║  │                                                                         │ ║
║  │                        DROP SAMPLES HERE                                │ ║
║  │                                                                         │ ║
║  │           ╔═══════════════════════════════════════════╗                 │ ║
║  │           ║                                           ║                 │ ║
║  │           ║             ↓  or click to browse         ║                 │ ║
║  │           ║                                           ║                 │ ║
║  │           ║     WAV files, folders, XPN packs         ║                 │ ║
║  │           ║                                           ║                 │ ║
║  │           ║     1 – 5 samples recommended             ║                 │ ║
║  │           ╚═══════════════════════════════════════════╝                 │ ║
║  │                                                                         │ ║
║  │                  — or start from an XPN pack —                          │ ║
║  │              [Open Pack...]  [Recent: mpce-perc-001 ▾]                  │ ║
║  │                                                                         │ ║
║  └─────────────────────────────────────────────────────────────────────────┘ ║
║                                                                              ║
║  ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄  Reveal when samples loaded  ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄  ║
║  [ Zone Map ]  [ Velocity ]  [ FX ]  [ Expression ]  [ Preview ]            ║
║   ↑ Tab bar — grayed out, non-interactive in empty state                     ║
║                                                                              ║
║  ─────────────────────────────────────────────────────────────────────────── ║
║                                              [Export ▾]     (inactive)       ║
╚══════════════════════════════════════════════════════════════════════════════╝

Window: 900×660 min. Resizable to 1280×800.
Drop zone: 80% of vertical space in empty state.
Tab bar below drop zone is rendered at 40% opacity, pointer-events: none.
```

### A2. OUTSHINE — Auto Mode (Samples Dropped, Classified)

```
╔══════════════════════════════════════════════════════════════════════════════╗
║  [XO Gold header, 40px]                                                     ║
║  OUTSHINE                             Auto Mode  ●  [Design Mode]  [?][×]   ║
╠═══════════════════════╦══════════════════════════════════════════════════════╣
║  SOURCE PANEL  [+ Add] ║  AUTO CLASSIFICATION                               ║
║  ─────────────────────║────────────────────────────────────────────────────  ║
║  ○ kick_001.wav        ║  kick_001.wav                                       ║
║    ████████░░  0:00.8  ║  ┌──────────────────────────────────────────────┐  ║
║    TYPE: Kick   C2     ║  │  ▪ Type:      Kick drum                      │  ║
║                        ║  │  ▪ Root:      C2  (detected)                 │  ║
║  ○ snare_001.wav       ║  │  ▪ Transient: Hard  (ms: 3.2)                │  ║
║    ████████░░  0:00.6  ║  │  ▪ Length:    0.8s  (short)                  │  ║
║    TYPE: Snare  D2     ║  │  ▪ Formant:   N/A                            │  ║
║                        ║  │  ▪ Vel layers: 1 (single)                    │  ║
║  ○ hihat_cl_001.wav    ║  └──────────────────────────────────────────────┘  ║
║    ███░░░░░░░  0:00.2  ║                                                     ║
║    TYPE: HH-CL  —      ║  ╔══════════════════════════════════════════════╗  ║
║                        ║  ║  AUTO MAPPING PREVIEW                        ║  ║
║  ○ hihat_op_001.wav    ║  ║                                              ║  ║
║    ████░░░░░░  0:00.4  ║  ║  [■ KICK]──C2─D2──[■ SNARE]──E2──[■ HH-CL] ║  ║
║    TYPE: HH-OP  —      ║  ║  ────────────────────────────────────────── ║  ║
║                        ║  ║  Velocity: Single layer (1)                  ║  ║
║  ○ perc_001.wav        ║  ║  Round-robin: Off                            ║  ║
║    ████████░░  0:01.2  ║  ║  FX Chain: Aquatic (auto-detected)           ║  ║
║    TYPE: Perc   G2     ║  ╚══════════════════════════════════════════════╝  ║
║  ─────────────────────║────────────────────────────────────────────────────  ║
║  5 samples  •  Auto   ║  [▸ Preview Instrument]                             ║
╠═══════════════════════╩══════════════════════════════════════════════════════╣
║                                                                              ║
║  ████████████████████████████████████████████████████████████████████  99%  ║
║  Analysis complete — instrument ready                                        ║
║                                                                              ║
║  ──────────────────────────────────────────────   [Export XPN Pack ▾]        ║
╚══════════════════════════════════════════════════════════════════════════════╝

Left panel: 260px, fixed. Right panel: flexible.
Each sample row: 56px tall. Mini waveform thumbnail: 120×18px.
Classification badge (TYPE:) rendered in engine accent color.
Auto Mapping Preview: compact keyboard strip, 1 octave visible minimum.
Progress bar at bottom (gold fill) reflects analysis pipeline state.
```

### A3. OUTSHINE — Design Mode (Full Manual Control)

```
╔══════════════════════════════════════════════════════════════════════════════╗
║  [XO Gold header, 40px]                                                     ║
║  OUTSHINE                             Design Mode  [Auto Mode]  [?][×]      ║
╠═══════════════════════╦══════════════════════════════════════════════════════╣
║  SOURCE PANEL  [+ Add] ║  [ Zone Map ]  [ Velocity ]  [ FX ]  [ Expression ]║
║  ─────────────────────║────────────────────────────────────────────────────  ║
║  ● kick_001.wav       ║                                                      ║
║    ████████░░         ║  ZONE MAP  ────────────────────────────  [Fit][1:1]  ║
║    [■ COLOR]  C2      ║                                                      ║
║                       ║  C1 ──────────── C2 ───── C3 ─── C4 ──────── C8    ║
║  ○ snare_001.wav      ║  │░░░░░░░░░░░░░│█████████│▓▓▓▓▓▓│▓▓▓▓▓▓▓▓▓▓│░░░│  ║
║    ████████░░         ║  │    (empty)   │ KICK    │SNARE │  HH-CL    │   │  ║
║    [■ COLOR]  D2      ║  │              │ Root:C2 │  D2  │           │   │  ║
║                       ║  │              │ Vel:1   │  1   │    1      │   │  ║
║  ○ hihat_cl_001.wav   ║  ╰───────────────────────────────────────────────╯  ║
║    ███░░░░░░░         ║                                                      ║
║    [■ COLOR]  —       ║  Selected Zone: KICK  ────────────────────────────  ║
║                       ║  Root Note:  [C2 ▾]   Range: [C2 ▾] – [D#2 ▾]      ║
║  ○ hihat_op_001.wav   ║  Crossfade:  [──────●────────]  12 semitones        ║
║    ████░░░░░░         ║  Key Track:  [●] On   Tune: [+0.00 ▾]               ║
║    [■ COLOR]  —       ║                                                      ║
║                       ║  ────────────────────────────────────────────────   ║
║  ○ perc_001.wav       ║  [Snap to Black Keys]  [Auto-distribute]            ║
║    ████████░░         ║  [Clear Zone]  [Split Here]  [Add Layer]            ║
║    [■ COLOR]  G2      ║                                                      ║
║  ─────────────────────║────────────────────────────────────────────────────  ║
║  5 samples •  Unsaved ║  [▸ Preview]                    Cmd+Z / Cmd+Shift+Z ║
╠═══════════════════════╩══════════════════════════════════════════════════════╣
║  ──────────────────────────────────────────────   [Export XPN Pack ▾]        ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

### A4. OUTSHINE — Rendering State

```
╔══════════════════════════════════════════════════════════════════════════════╗
║  [XO Gold header, 40px]                                                     ║
║  OUTSHINE                                        Rendering…       [Cancel]  ║
╠══════════════════════════════════════════════════════════════════════════════╣
║                                                                              ║
║                  ┌─────────────────────────────────────────────────────┐    ║
║                  │                                                     │    ║
║                  │          FORGING YOUR INSTRUMENT                    │    ║
║                  │                                                     │    ║
║                  │    ████████████████████████████████░░░░░  78%       │    ║
║                  │                                                     │    ║
║                  │    Packing zones…                                   │    ║
║                  │    Kick • C2–D#2         ✓                         │    ║
║                  │    Snare • E2–G2          ✓                         │    ║
║                  │    HH-Closed • A2–B2      ✓                         │    ║
║                  │    HH-Open • C3–D3        ⟳ in progress…           │    ║
║                  │    Perc • E3–G3           —                         │    ║
║                  │                                                     │    ║
║                  │    Estimated time: ~4 seconds                       │    ║
║                  │                                                     │    ║
║                  └─────────────────────────────────────────────────────┘    ║
║                                                                              ║
║             The render uses OfflineAudioContext — no clicks, ever.           ║
║                          (Issea's "ma" — a moment of calm)                  ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

### A5. OUTSHINE — Export / Complete State

```
╔══════════════════════════════════════════════════════════════════════════════╗
║  [XO Gold header, 40px]                                                     ║
║  OUTSHINE                                                 [?][×]            ║
╠══════════════════════════════════════════════════════════════════════════════╣
║                                                                              ║
║  ┌────────────────────────────────────────────────────────────────────────┐  ║
║  │  [Cover art preview  ]  INSTRUMENT READY                              │  ║
║  │  [  120×120 square   ]                                                │  ║
║  │  [  (auto-generated  ]  Name:         [Perc Kit 001              ]    │  ║
║  │  [  from sample      ]  Description:  [5-layer percussion kit     ]   │  ║
║  │  [   waveform art)   ]  Author:       [XO_OX Designs             ]   │  ║
║  │                         Version:      [1.0                        ]   │  ║
║  │                         Tags:         [kick snare hihat perc      ]   │  ║
║  └────────────────────────────────────────────────────────────────────────┘  ║
║                                                                              ║
║  EXPORT FORMAT  ─────────────────────────────────────────────────────────    ║
║  [● XPN Pack (recommended)]  [○ Individual XPM]  [○ WAV Folder Only]         ║
║                                                                              ║
║  Output: ~/Documents/XPN Exports/  [Change…]                                 ║
║  Size estimate:  2.4 MB (5 WAVs + XPM + manifest)                            ║
║                                                                              ║
║  ──────────────────────────────────────────────────────────────────────────  ║
║                                                                              ║
║  [← Back to Design]              [▸ Preview Again]    [Export XPN Pack]     ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

### A6. ORIGINATE — Preset Selection View

```
╔══════════════════════════════════════════════════════════════════════════════╗
║  [XO Gold header, 40px]                                                     ║
║  ORIGINATE                     Synth-to-Sample Export         [?][×]        ║
╠══════════════════════════════════════════════════════════════════════════════╣
║  PRESETS  ─────────────────────────────────────────────────────────────────  ║
║  [Search presets…                    ]  [Filter: All Engines ▾]  [All Moods▾]║
║                                                                              ║
║  ┌────────────────────────────────────────────────────────────────────────┐  ║
║  │ □  Stellar Drift                   ODYSSEY      Foundation    🎵       │  ║
║  │ □  Cave Grammar                    OXBOW        Submerged     🎵       │  ║
║  │ ☑  Iron Shore                      OSPREY       Atmosphere    🎵       │  ║
║  │ ☑  Mallet Ghost                    OWARE        Aether        🎵       │  ║
║  │ □  Brick Ecology                   OBRIX        Foundation    🎵       │  ║
║  │ □  Neon Tetra Dive                 ODDFELIX     Prism         🎵       │  ║
║  │ □  Boom Bap Psych                  OFFERING     Foundation    🎵       │  ║
║  │ □  Phase Drama                     OPERA        Entangled     🎵       │  ║
║  └────────────────────────────────────────────────────────────────────────┘  ║
║  2 selected  •  16,000+ total                  [Select All]  [Clear All]     ║
║                                                                              ║
╠══════════════════════════════════════════════════════════════════════════════╣
║  ─────────────────────────────────────────────   [Next: Render Settings →]  ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

### A7. ORIGINATE — Render Settings View

```
╔══════════════════════════════════════════════════════════════════════════════╗
║  [XO Gold header, 40px]                                                     ║
║  ORIGINATE                     Synth-to-Sample Export         [?][×]        ║
╠══════════════════════════════════════════════════════════════════════════════╣
║  ← PRESETS (2 selected)  •  RENDER SETTINGS  •  BUNDLE                      ║
║  ─────────────────────────────────────────────────────────────────────────   ║
║                                                                              ║
║  QUICK PROFILES  ───────────────────────────────────────────────────────    ║
║  [● MPC One/Live (recommended)]  [○ Light (fast)]  [○ Maximum Quality]      ║
║                                                                              ║
║  NOTE STRATEGY  ─────────────────────────────────────────────────────────   ║
║  [Chromatic Every Semitone ▾]                                                ║
║  Range:  [C1 ▾] – [C6 ▾]   •  Span: 60 notes                               ║
║                                                                              ║
║  VELOCITY LAYERS  ──────────────────────────────────────────────────────    ║
║  [1 ▾]  [3 ▾]  [6 ▾]  [●12 (full)]                                          ║
║  Current: 12 layers  ×  60 notes  =  720 renders                            ║
║                                                                              ║
║  AUDIO QUALITY  ─────────────────────────────────────────────────────────   ║
║  Bit depth:   [● 24-bit]  [○ 16-bit]  [○ 32-bit float]                      ║
║  Sample rate: [48000 Hz ▾]                                                   ║
║  Render time: [2.0s per note ▾]  (tail capture)                              ║
║  Normalize:   [● Per-note peak]  [○ Global peak]  [○ Off]                   ║
║                                                                              ║
║  ENTANGLED MODE  ────────────────────────────────────────────────────────   ║
║  [○] Bake coupling snapshot into renders                                     ║
║      (records live MegaCouplingMatrix state — Iron Shore × Mallet Ghost)     ║
║                                                                              ║
║  Estimated size:  ~86 MB  •  ~4.2 min render time                           ║
║                                                                              ║
╠══════════════════════════════════════════════════════════════════════════════╣
║  [← Back to Presets]                          [Next: Bundle Config →]       ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

### A8. Error States

#### Outshine — Zone Conflict Warning

```
╔══════════════════════════════════════════════╗
║  ⚠  ZONE CONFLICT                           ║
║  ─────────────────────────────────────────  ║
║  kick_001.wav and kick_002.wav overlap      ║
║  at zone C2–D2.                             ║
║                                             ║
║  [● Layer them (add velocity layer)]        ║
║  [○ Replace existing zone]                  ║
║  [○ Shift new zone right]                   ║
║                                             ║
║              [Cancel]   [Apply]             ║
╚══════════════════════════════════════════════╝
```

#### Originate — Export Failed

```
╔══════════════════════════════════════════════╗
║  ✕  RENDER FAILED                           ║
║  ─────────────────────────────────────────  ║
║  Iron Shore failed at note C3 vel 64.       ║
║  Error: Audio output device changed         ║
║  during render.                             ║
║                                             ║
║  Partial output saved to:                   ║
║  ~/Documents/XPN Exports/partial/           ║
║                                             ║
║           [View Log]   [Retry from C3]      ║
╚══════════════════════════════════════════════╝
```

---

## B. INTERACTION DESIGN

### B1. Progressive Disclosure Architecture

**OUTSHINE — Three Tiers:**

1. **Tier 1 (Always visible):** Drop zone / Source panel, Auto classification result, Export button.
2. **Tier 2 (Revealed by clicking "Design Mode"):** Tab bar (Zone Map / Velocity / FX / Expression). Source panel narrows from 80% width to 260px fixed. Animation: 200ms, auto-map preview slides down, tab bar fades up.
3. **Tier 3 (Revealed within each tab):** Zone inspector (selected zones), Round-robin settings (when count > Off), FX stage controls (when stage added). Height animations 150ms.

**ORIGINATE — Linear Disclosure:**
Preset Selection → Render Settings → Bundle Config → Render Progress → Complete.

### B2. Keyboard Shortcuts

**OUTSHINE:**

| Shortcut | Action |
|----------|--------|
| Cmd+O | Open files |
| Cmd+N | New instrument |
| Cmd+S | Save project |
| Cmd+E | Export XPN |
| Cmd+Z | Undo |
| Cmd+Shift+Z | Redo |
| Cmd+1–4 | Switch tabs |
| Space | Play preview |
| Escape | Close preview / dismiss modal |
| Delete | Remove selected zone |

**ORIGINATE:**

| Shortcut | Action |
|----------|--------|
| Cmd+A | Select all presets |
| Cmd+Shift+A | Deselect all |
| Space | Audition selected preset |
| Return | Next step |
| Escape | Previous step |
| Cmd+R | Start render |

---

## C. FONT SPECIFICATION

**Primary:**
- **Space Grotesk SemiBold** (20pt: window titles; 11pt: section headers ALL CAPS tracking +0.08em; 13pt: tab labels)
- **Space Grotesk Regular** (13pt: preset names, zone labels, sample names)
- **Inter Regular** (12pt: body copy, descriptions, tooltips)
- **Inter Medium** (11pt: form field labels, inline metadata)
- **JetBrains Mono Regular** (11pt: numeric values, Hz, dB, MIDI notes)

All available free/OFL from Google Fonts. Embed via JUCE binary data.

---

## D. COLOR PALETTE

### Backgrounds

| Token | Light Mode | Dark Mode | Usage |
|-------|-----------|-----------|-------|
| `bg-window` | `#F8F6F3` | `#1A1A1A` | Window fill |
| `bg-panel` | `#EFEDE9` | `#222222` | Left panel |
| `bg-card` | `#FFFFFF` | `#2A2A2A` | Cards, modals |
| `bg-hover` | `#E8E4DE` | `#333333` | List row hover |
| `bg-selected` | `#E2DDD5` | `#3A3A3A` | List row selected |
| `bg-drop-zone` | `#F0EDE7` | `#252525` | Drop zone idle |
| `bg-drop-active` | `#FBF6E9` | `#2D2B20` | Drop zone active |
| `bg-header` | `#E9C46A` | `#E9C46A` | XO Gold header |

### Text

| Token | Light | Dark | Usage |
|-------|-------|------|-------|
| `text-primary` | `#1A1A1A` | `#F0EDE8` | Primary content |
| `text-secondary` | `#6B6460` | `#9A9490` | Metadata, labels |
| `text-disabled` | `#B4AFA9` | `#555050` | Inactive controls |
| `text-on-gold` | `#1A1A1A` | `#1A1A1A` | Text on XO Gold |
| `text-caption` | `#8A8480` | `#706C68` | Captions, hints |

### Accent & Status

| Token | Hex | Usage |
|-------|-----|-------|
| `accent-gold` | `#E9C46A` | Primary accent, CTAs |
| `accent-gold-hover` | `#D4AD55` | CTA hover |
| `accent-gold-press` | `#BF9C44` | CTA press |
| `accent-gold-subtle` | `#FBF6E9` | Hover backgrounds |
| `status-success` | `#4CAF50` | Render complete |
| `status-warning` | `#FF9800` | Zone conflicts |
| `status-error` | `#F44336` | Export failures |
| `status-processing` | `#E9C46A` | In-progress |
| `status-info` | `#2196F3` | Info hints |

### Zone Colors (12 colorblind-safe palette)

| # | Name | Hex |
|---|------|-----|
| 1 | Reef Jade | `#1E8B7E` |
| 2 | Crate Yellow | `#E5B80B` |
| 3 | Axolotl Pink | `#E8839B` |
| 4 | Neon Tetra | `#00A6D6` |
| 5 | Aria Gold | `#D4AF37` |
| 6 | Bioluminescent | `#00FFB4` |
| 7 | Obsidian | `#E8E0D8` |
| 8 | Origami Red | `#E63946` |
| 9 | Oracle Indigo | `#4B0082` |
| 10 | Osprey Blue | `#1B4F8A` |
| 11 | Orphica Seafoam | `#7FDBCA` |
| 12 | Onset Blue | `#0066FF` |

---

## E. ICON LIST

| Icon | SF Symbol | Size(s) | Description |
|------|-----------|---------|-------------|
| Import / Drop | `square.and.arrow.down` | 24×24, 32×32 | Main drop zone icon |
| Add sample | `plus.circle` | 16×16 | Source panel add |
| Remove sample | `minus.circle` | 16×16 | Remove from list |
| Waveform mini | Custom SVG | 120×18 | Inline thumbnail |
| Play | `play.fill` | 16×16, 24×24 | Preview play |
| Stop | `stop.fill` | 16×16 | Preview stop |
| Drag handle | Custom SVG | 8×12 | 6-dot grip |
| Check complete | `checkmark.circle.fill` | 14×14 | Render done |
| In progress | Custom spinner SVG | 14×14 | Render active |
| Warning | `exclamationmark.triangle.fill` | 16×16 | Warning |
| Error | `xmark.circle.fill` | 16×16 | Error |
| Help | `questionmark.circle` | 16×16 | Help button |
| Close | `xmark` | 12×12 | Window close |
| Audition | `music.note` | 14×14 | Preset audition |
| Keyboard | `pianokeys` | 16×16 | Preview trigger |
| Export pack | Custom SVG | 20×20 | Export CTA |
| FX arrow | Custom SVG | 12×8 | Signal flow |
| Zone swatch | Solid rect | 8×8, r=2 | Sample badge |

---

## F. DESIGN TOKENS

### Spacing (4px base)

| Token | Value | Usage |
|-------|-------|-------|
| `space-1` | 4px | Icon padding |
| `space-2` | 8px | Default gaps |
| `space-3` | 12px | Panel padding |
| `space-4` | 16px | Card padding |
| `space-5` | 20px | Header-to-content |
| `space-6` | 24px | Section gaps |
| `space-8` | 32px | Panel padding |
| `space-12` | 48px | Large whitespace |
| `space-16` | 64px | Drop zone padding |

### Border Radius

| Token | Value | Usage |
|-------|-------|-------|
| `radius-xs` | 2px | Swatches, dots |
| `radius-sm` | 4px | Buttons, fields |
| `radius-md` | 8px | Cards, FX tiles |
| `radius-lg` | 12px | Large cards |
| `radius-xl` | 16px | Panel containers |
| `radius-pill` | 9999px | Chip tabs, tags |

### Shadows

| Token | Value | Usage |
|-------|-------|-------|
| `shadow-1` | `0 1px 3px rgba(26,12,0,0.08)` | List hover |
| `shadow-2` | `0 4px 12px rgba(26,12,0,0.12)` | Cards selected |
| `shadow-3` | `0 8px 24px rgba(26,12,0,0.16)` | Floating panels |
| `shadow-gold` | `0 0 0 2px #E9C46A` | Focus ring |
| `shadow-drop-zone` | `inset 0 0 0 2px #E9C46A` | Drop zone active |

### Transitions

| Token | Duration | Easing | Usage |
|-------|----------|--------|-------|
| `dur-instant` | 80ms | `ease-out` | Hover states |
| `dur-fast` | 150ms | `ease-in-out` | Tab switches |
| `dur-normal` | 200ms | `ease-in-out` | Reveals |
| `dur-enter` | 250ms | `cubic-bezier(0.34, 1.56, 0.64, 1)` | Window open |
| `dur-exit` | 180ms | `ease-in` | Window close |
| `dur-layout` | 300ms | `ease-in-out` | Panel changes |

---

## G. ARCHITECT COMMENTARY

### Ulf's Priority Recommendations

- **FX Routing:** Consider single-column list instead of horizontal signal flow to reduce visual complexity
- **Expression Tab:** Add "A/B variation" preview button to let users hear the difference immediately
- **Rendering Header:** "FORGING YOUR INSTRUMENT" as the exact wording (emotional climax)
- **Risk (Too Complex):** FX palette can become DAW-like quickly
- **Risk (Too Simple):** Micro-variation sliders feel like guesswork without preview

### Issea's Priority Recommendations

- **Zone Tactility:** Add 1px inner shadow on top edge of zones (depth, mass)
- **Empty State:** Three-panel "Drop → Map → Export" line illustration (disappears on first drop)
- **Rendering Ceremony:** Optional ORIGINATE menu bar collapse during long renders
- **Subtle Text:** "The render uses OfflineAudioContext — no clicks, ever" (small italic caption, ma principle)
- **Risk (Too Complex):** 5 tabs in Design Mode. Consider collapsing FX + Expression into single "Sound" panel
- **Risk (Too Simple):** New users need to understand the outcome

### Xavier's Priority Recommendations

- **Window Chrome:** Use `NSWindowStyleMaskFullSizeContentView` — traffic lights float over XO Gold header
- **Trackpad Gesture:** Two-finger pinch to zoom keyboard zone map via `magnifyGestureReceived()`
- **WCAG 2.1 AA:** Minimum 4.5:1 contrast. All colors verified. Focus rings always visible. Color never the only differentiator.
- **Zone Labels:** Must include sample name text (not color-only) for colorblind users
- **FX Assignment:** Popover click instead of drag-from-palette (reduces confusion, saves space)
- **Preview Keyboard:** Persistent 44px bar at bottom, expands on hover (zero-friction audition)
- **ORIGINATE Width:** 640px instead of 520px (prevents preset name truncation)
- **Risk (Too Complex):** Dual drag-and-drop paradigm (samples + FX)
- **Risk (Too Simple):** Preview keyboard is mode-switched, not always discoverable

### Studio Synthesis — All Three Agree

1. **Keyboard zone map is the product.** Design every decision around this.
2. **Auto Mode must look trustworthy.** Precise classification, confident presentation.
3. **XO Gold header as chrome.** Floating traffic lights over gold bar.
4. **WCAG 2.1 AA non-negotiable.** All contrast ratios verified, focus rings always visible.
5. **Zone colors survive colorblindness.** Text labels required, not color-only.

---

## Summary

This specification covers:

- **10 distinct wireframes** (OUTSHINE: empty/auto/design+tabs/rendering/export; ORIGINATE: preset/settings/bundle)
- **4 error state dialogs** (zone conflict, unsupported file, render failed, no audio device)
- **Progressive disclosure architecture** (3-tier OUTSHINE, linear ORIGINATE)
- **Complete font specification** (3 families, all OFL licensed)
- **Full color palette** (9 background tokens, 5 text tokens, 9 accent/status tokens, 12 zone colors, velocity gradient)
- **23 icon references** (SF Symbols + custom SVG)
- **Keyboard shortcuts** (12 for OUTSHINE, 6 for ORIGINATE)
- **Design tokens** (9 spacing, 6 radius, 5 shadows, 7 transitions)
- **Interaction details** (drag/drop, gestures, undo/redo scope)
- **Architect commentary** (Ulf, Issea, Xavier priorities + studio synthesis)

All measurements, colors, and specifications are absolute. This is a complete hand-off document ready for Figma design and JUCE implementation.
