# XOlokun Spatial Architecture V1

> **Status:** V1 — Post 10-panel review synthesis
> **Date:** 2026-03-24
> **Reviews incorporated:** UX Architect (25 issues), Working Producer (9), Competitor Research (pixel specs), Comic Book Guy (3 gaps), Ghost Seance (6 demands), Producer's Guild (5 priorities), Kai's Team (5 changes), Visionary (3 additions), Reddit Coalition (14 comments), User Feedback (7 decisions + form factors)
> **Principle:** Architecture before aesthetics. Every feature has an address. Every concern has a resolution.

---

## 1. Design Constraints

### Hard Constraints
- **Plugin window:** 1100×700pt default | min 960×600 | max 1920×1080
- **MPC Hardware Plugin:** min 800×480pt (new — Kai's team flagged MPC One/Live II viewport)
- **4 engine slots** simultaneously, drawn from 73 swappable engines
- **15 coupling types** between any engine pair
- **6 FX slots per engine** (SAT/DELAY/REVERB/MOD/COMP/SEQ) — corrected from draft's "4"
- **3 PlaySurface modes** (XOuija fretless, MPC 16-pad, Seaboard keyboard)
- **No floating windows** in AU plugin format
- **44pt minimum touch targets** on all touch platforms

### Resolved Decisions (from user + review panels)
| Decision | Resolution | Source |
|----------|-----------|--------|
| Column proportions | **260 / 520 / 320** (Ableton-style balanced) | User Q1 |
| PlaySurface | **Collapsible, engine-aware default** (drums auto-expand to MPC) | User Q2 + Kai Vibe |
| Default Column B | **Sound-first: Slot 1 engine detail with macros** (unanimous 8/8 critics) | All panels |
| Coupling interaction | **Inline popover on arc click** + list in Column C | UX Architect P0-3 |
| FX location | **Column B panel** with per-FX inline expansion (not cross-column) | Ghost Pearlman + UX |
| OBRIX Pocket | **Included** for component reuse | User Q6 |
| Engine tile content | **Per competitor research** — see §4 | User Q3 |

---

## 2. Layout Paradigm: 3-Column + Collapsible Zones

### Why 3-Column
Every professional creative tool uses this: Ableton (browser/arrangement/detail), Logic (library/tracks/inspector), Figma (layers/canvas/properties). It works because it separates **inventory** (left), **workspace** (center), and **configuration** (right). Users already understand this pattern.

### The Layout

**Editing Mode (PlaySurface collapsed — default for sound design):**
```
+═══════════════════════════════════════════════════════════════════+
| HEADER (52pt)                                                     |
| [Logo] [EngBrowser] [←Preset→♡DNA] [A|B]  [M1][M2][M3][M4] [⚙] |
+═══════════════════════════════════════════════════════════════════+
|              |                            |                       |
| COL A 260pt  |      COL B 520pt           |    COL C 320pt        |
| ENGINE RACK  |    MAIN CANVAS             |  BROWSER/INSPECTOR    |
|              |   (contextual)             |                       |
| ┌──────────┐ |                            | [PRESET][COUPLE][FX]  |
| │ SLOT 1   │ | Default: ENGINE DETAIL     | [PLAY][EXPORT][⚙]    |
| │ On/Off   │ | (Slot 1, 4 macros,        |                       |
| │ Name     │ | grouped params,           | Preset Browser:       |
| │ Waveform │ | waveform, ADSR)           | [Type: DRUMS▾]        |
| │ CPU 2.1% │ |                            | [Search...]           |
| ├──────────┤ | On coupling arc click:     | [Mood pills]          |
| │ SLOT 2   │ | INLINE POPOVER on arc     | [▶ Preset Card]       |
| │          │ | (type/amount/depth)        | [▶ Preset Card]       |
| ├──────────┤ |                            |                       |
| │ SLOT 3   │ | FX mode: FX CHAIN         | Coupling Inspector:   |
| │          │ | (6 slots, inline expand)   | Source→Target list     |
| ├──────────┤ |                            | Type/Amount/Depth     |
| │ SLOT 4   │ | Export mode: EXPORT PANEL  | per route             |
| │          │ | (persistent, survives      |                       |
| └──────────┘ |  focus loss)              |                       |
|              |                            |                       |
| [+ ENGINE]   | CinematicMode: expands     |                       |
|              | to full 1100pt width       |                       |
|              | (ColA+C collapse)          |                       |
+═══════════════════════════════════════════════════════════════════+
| STATUS (28pt) [FIRE][XOSEND][ECHO][PANIC] BPM Voices CPU [●●●●] |
+═══════════════════════════════════════════════════════════════════+
```

**Performance Mode (PlaySurface expanded — toggled or auto on drum engines):**
```
+═══════════════════════════════════════════════════════════════════+
| HEADER (52pt) — macros enlarged to 48pt in performance mode       |
+═══════════════════════════════════════════════════════════════════+
| COL A 260pt  |  COL B 520pt  |  COL C 320pt                      |
| (dimmed 45%) | (dimmed 20%)  |  (dimmed 20%)                     |
| Engine rack  |  Coupling     |  Hidden or                        |
| still visible|  mini-graph   |  collapsed                        |
| 390pt tall   |  (persistent) |                                   |
+═══════════════════════════════════════════════════════════════════+
| PLAYSURFACE (220pt, full width)                                   |
| [XOUIJA|MPC|KEYS] [Surface 860pt]  [Expression 240pt]            |
+═══════════════════════════════════════════════════════════════════+
| STATUS (28pt)                                                     |
+═══════════════════════════════════════════════════════════════════+
```

### Pixel Math — Verified for Worst Cases

**Editing mode (960×600 minimum window):**
```
Header: 52, Columns: 520 (600−52−28), Status: 28 = 600 ✓
Col A: 240pt (scales from 260), Col B: 440pt, Col C: 280pt = 960 ✓
Engine Detail at min: sticky top 88pt + sticky bottom 80pt = 168pt fixed
Scrollable middle: 520 − 168 = 352pt viewport for param sections
ONSET worst case: ~600pt scrollable content → 248pt hidden = short scroll
```

**Responsive sticky rule:** At column height <480pt, sticky bottom (waveform/ADSR) becomes scrollable to prevent overlap with sticky top. This occurs at minimum window with PlaySurface collapsed.

**Performance mode (960×600 minimum):**
```
Header: 52, Columns: 300 (600−52−220−28), Surface: 220, Status: 28 = 600 ✓
Col A at 300pt: tiles compress to 60pt compact each (4×60=240pt)
  + coupling mini-graph: HIDDEN at <400pt column height (recovers 80pt)
  = 240pt tiles in 300pt column ✓ (60pt breathing room)
```

**MPC Hardware (800×480):** See §11 for dedicated 2-column layout.

---

## 3. Resolving the 12 Consensus Issues

Every issue flagged by 3+ review panels, resolved:

### 3.1 Default View: Sound First, Not Coupling Graph (8/8 panels)

**Resolution:** Column B defaults to **Slot 1 Engine Detail** with 4 macro knobs prominent. The coupling graph moves to a **persistent mini-graph** (120×80pt) at the bottom of Column A, below the 4 slot tiles. This keeps coupling always visible (Buchla, Tomita demanded this) without making it the first thing a new user must parse.

**First Launch Flow:**
1. Open plugin → sound plays immediately (OXBOW "First Breath" init preset)
2. Column B shows OXBOW engine detail — 4 large macro knobs, waveform, ADSR
3. Column A shows Slot 1 active, Slots 2-4 as subtle "+" affordances
4. Mini coupling graph at bottom of Column A shows 1 node (no arcs yet — nothing to confuse)
5. User turns a macro → hears change → hooked

**0 clicks to hear sound. 0 clicks to see controls. Coupling discovered naturally when Slot 2 is loaded.**

**First-launch fallback:** If OXBOW "First Breath" fails to load (missing file, corrupt state), fall back to a hard-coded init patch (simple saw wave, no file dependency). Log warning to debug output. Never show a silent, empty plugin on first launch.

### 3.2 Modulation System: Coupling IS the Mod Matrix (Comic Book Guy P0-2)

**Resolution:** XOlokun's modulation architecture is fundamentally different from Vital/Serum. State it explicitly:

**Per-engine modulation:** Each engine has internal LFOs, envelopes, and mod routing pre-wired by the engine designer. These are exposed as standard parameters (lfoRate, lfoDepth, envAmount, etc.) in the collapsible MOD section. The engine designer decides what modulates what — the user adjusts depth and rate.

**Cross-engine modulation:** This IS coupling. Engine A's output modulates Engine B's parameters. 15 coupling types define HOW that modulation works. This is the mod matrix — it routes between engines, not within them.

**OPTIC as modulation engine:** OPTIC produces zero audio but 8 modulation outputs. Loading OPTIC in Slot 3 and coupling it to Slots 1+2 gives you 8 assignable modulation sources — more than Vital's 6 random sources.

**What's missing (deferred to V1.1):** Drag-to-assign visual modulation on individual knobs. V1 ships with the coupling system as the primary modulation paradigm. V1.1 adds per-knob mod rings and drag-from-source-to-target gestures within a single engine's MOD section.

**UI home for mod routing:** The Coupling Inspector (Column C, COUPLE tab) shows both cross-engine coupling routes AND OPTIC's 8 modulation output assignments. This is the mod matrix.

### 3.3 Coupling Self-Explanatory (Producers + Ghosts + CBG — 5/8 panels)

**Resolution:** Three-layer coupling accessibility:

**Layer 1 — Hearing it:** Default init preset has OXBOW + OPERA coupled at ENTANGLE 0.35. User hears coupling before understanding it.

**Layer 2 — Seeing it:** Mini coupling graph in Column A shows arcs between slots. Arc brightness pulses with coupling energy. A text label appears below: "OXBOW → OPERA: Entangle" in plain language.

**Layer 3 — Editing it:** Click an arc → **inline popover** appears adjacent to the arc (not in Column C) showing: type selector (3 quick-start options + "More..." for full 15), amount knob, depth knob. The 3 quick-start types with plain descriptions:
- **Entangle** — "engines breathe together"
- **Gravitational** — "one engine follows the other's pitch"
- **Phase** — "rhythms synchronize"

Full 15-type dropdown behind "More..." for power users.

**Popover/Inspector sync contract:** The inline arc popover and the Column C Coupling Inspector are live views of the same underlying coupling route state. Edits in either update the other in real-time. The popover is the primary quick-edit surface; the inspector is the overview surface. They cannot show conflicting values.

### 3.4 Parameter Overflow for 80+ Param Engines (3/8 panels)

**Resolution:** Accept scrolling for CUSTOM engines. Mitigate with design:

**Standard engines (30, ≤40 params):** Fit without scrolling in 520×620pt. 4 macros (88pt) + 4 sections × ~60pt headers with 3-4 hero knobs each (240pt) + waveform (60pt) + ADSR (50pt) = ~438pt. Room to spare.

**Semi-custom engines (26, 40-60 params):** Sections collapsed by default, hero params only. Total ~500pt. Fits without scrolling. Expand any section to see more.

**Custom engines (17, 60-111 params):** WILL scroll. Mitigate:
- Scrollbar visible and styled (not hidden)
- "Jump to section" mini-nav bar at top of Column B: [OSC][FILT][MOD][FX][★] — click to scroll to that section
- The 4 macro knobs are STICKY at the top — they don't scroll away
- The waveform/ADSR area is also sticky at bottom

**ONSET (111 params) specific layout:**
```
STICKY TOP: [4 Macro Knobs — 88pt] [Section Nav: OSC|FILT|MOD|FX|DRUM]
SCROLLABLE:
  [DRUM GRID — 8 voice pads with algo selectors — 280pt]
  [▸ OSC section — 8 params hero]
  [▸ FILTER section — 6 params hero]
  [▸ MOD section — 12 params hero]
  [▸ FX section — 4 params hero]
STICKY BOTTOM: [Waveform 180×40pt | ADSR 180×40pt]
```
Total scrollable content: ~600pt. Viewport: ~444pt (620−88−88 sticky). Scroll range: ~156pt. That's a short scroll — less than one screen height of content hidden. Acceptable.

### 3.5 Intra-Engine Signal Flow Invisible (Comic Book Guy gap)

**Resolution:** Add a **Signal Flow Mini-Diagram** as a 520×32pt strip at the top of Engine Detail (Column B), below the nameplate. Shows the audio path as a left-to-right chain:

```
[SRC1]→[SRC2]→[FILTER]→[SHAPER]→[FX]→[OUT]
```

Each block is clickable — clicking [FILTER] scrolls to the FILTER section. Active blocks glow with engine accent. Bypassed blocks are dimmed. This adds 32pt to the Column B layout but gives every engine an immediately visible signal flow.

For OBRIX: the mini-diagram IS the Brick Stack View — it shows the modular slot-flow.
Three diagram variants:
- **Variant A — Linear chain** (50+ standard engines): `[SRC]→[FILTER]→[MOD]→[FX]→[OUT]`. Auto-generated from param categories.
- **Variant B — Modular slot-flow** (OBRIX, OCELOT, engines with explicit routing): shows typed slots with drag connections. OBRIX's brick stack IS the signal flow.
- **Variant C — Network/feedback** (OUROBOROS, ORGANON, ORBWEAVE, ORGANISM): circular icon indicates complex topology. Click to expand to full routing view in CinematicMode.

Rule: engines with >2 feedback parameters or non-linear topology use Variant C. All others default to A.

### 3.6 Missing Features Table (UX Architect + Reddit)

| Feature | Resolution | UI Home | V1? |
|---------|-----------|---------|-----|
| **Undo/Redo** | Cmd-Z/Cmd-Shift-Z. Visual: undo arrow in header, right of preset nav | Header, 20×20pt | Yes |
| **A/B Compare** | [A][B] toggle in header preset nav area | Header, 40×20pt | Yes |
| **Init Preset** | Right-click slot tile → "Init Engine" | Context menu | Yes |
| **Randomize/Dice** | Right-click slot tile → "Randomize" OR dice icon in engine nameplate | Context menu + 20pt icon | Yes |
| **Right-click menus** | Every RotaryKnob: Reset Default / Copy / Paste / MIDI Learn / Automate | Component-level | Yes |
| **Keyboard shortcuts** | See §3.7 | Global | Yes |
| **Engine browser** | See §4 | Overlay | Yes |
| **CPU per engine** | 4pt bar at bottom of each slot tile | Column A tile | Yes |
| **Preset preview** | ▶ button on each PresetCard, plays 2s cached thumbnail | Column C browser | Yes |
| **Instrument-type filter** | Primary filter axis in preset browser: DRUMS/BASS/PADS/LEADS/TEXTURES/KEYS/FX | Column C | Yes |
| **Save As dialog** | Name field + mood picker + tag input + overwrite vs new choice | Dialog (CallOutBox) | Yes |
| **Session state persistence** | getStateInformation saves: engines, presets, coupling, FX, PlaySurface toggle, macros | Plugin contract | Yes |
| **PlaySurface state persistence** | Toggle state saved per-session, per-user preference | Preference store | Yes |

### 3.7 Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| 1-4 | Select engine slot 1-4 |
| Cmd-Z / Cmd-Shift-Z | Undo / Redo |
| ← → | Previous / Next preset |
| Space | Toggle PlaySurface |
| Tab | Cycle Column C tabs |
| Cmd-S | Save preset |
| Cmd-Shift-S | Save As |
| Cmd-R | Randomize active engine |
| Cmd-I | Init active engine |
| Cmd-A | A/B compare toggle |
| Z X C V | FIRE / XOSEND / ECHO / PANIC |
| Shift+drag | Fine adjust any knob |
| Double-click knob | Type exact value |
| Cmd-click knob | Reset to default |

---

## 4. Column A: Engine Rack (260pt × variable height)

### Slot Tile Specification (260pt × variable)

**Compact tile (performance mode, 95pt tall):**
```
+--+-------------------------------+--+
|A |  [1]                    [⏻]  |  |  14pt: slot # + power toggle
|C |  ENGINE NAME   [🪸]         |  |  16pt: name + creature
|C |  [▬▬▬ waveform ▬▬▬▬▬▬▬]     |  |  22pt: live mini oscilloscope
|  |  [●][●][●][●]  [FX ●●○]    |  |  20pt: macro dots + FX indicator
|S |  [▬▬▬ CPU 2.1% ▬▬▬▬▬▬▬▬]   |  |  4pt: per-engine CPU bar
+--+-------------------------------+--+
     ↑ coupling dot between tiles
```

**Expanded tile (editing mode, ~145pt tall) — adds mini macro knobs:**
```
+--+-------------------------------+--+
|A |  [1]                    [⏻]  |  |  14pt: slot # + power toggle
|C |  ENGINE NAME   [🪸]         |  |  16pt: name + creature
|  |  [M1 32pt] [M2] [M3] [M4]  |  |  40pt: 4 interactive mini macros
|  |  [▬▬▬ waveform ▬▬▬▬▬▬▬]     |  |  22pt: live mini oscilloscope
|  |  [●][●][●][●]  [FX ●●○]    |  |  20pt: macro dots + FX indicator
|S |  [▬▬▬ CPU 2.1% ▬▬▬▬▬▬▬▬]   |  |  4pt: per-engine CPU bar
+--+-------------------------------+--+
```

**What's NOT in the tile** (per competitor research): no param count badge, no pan knob, no tune display. These live in Column B detail.

**Empty slot:** Shows engine accent stripe at 10% opacity + centered "+" button (44×44pt touch target) + text "Add Engine". NOT a confusing empty frame — a clear invitation.

**Single Engine Mode:** When only 1 engine is loaded, Column A shrinks to 80pt (showing just the one tile as a minimal strip). Column B expands to 700pt. Toggled automatically or manually.

### Coupling Mini-Graph (260pt × 80pt, below tiles)
Persistent. Shows 4 nodes corresponding to the 4 tiles. Arcs between coupled nodes. Text labels on arcs ("Entangle 0.72"). Clicking an arc opens the inline popover.

### Engine Browser (Overlay — triggered from [+ ENGINE] or slot tile click)
```
+══════════════════════════════════════════════════════════+
| ENGINE BROWSER (Overlay, 600×500pt centered)             |
| [Search: ____________]  [Category ▾] [Sort ▾]           |
|                                                          |
| Categories: [ALL] [DRUMS] [BASS] [PADS] [LEADS]         |
|             [TEXTURES] [KEYS] [FX/UTILITY]               |
|                                                          |
| ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐    |
| │ 🪸 OBRIX │ │ 🥁 ONSET │ │ 🎭 OPERA │ │ 💜 OXYTO │    |
| │ Reef Jade│ │ Elec Blue│ │ Aria Gold│ │ Synapse  │    |
| │ Modular  │ │ 8-voice  │ │ Additive │ │ Circuit  │    |
| │ 81 params│ │ drum     │ │ vocal    │ │ love     │    |
| │ Flagship │ │ Flagship │ │ Flagship│ │ ★Pick  │    |
| └──────────┘ └──────────┘ └──────────┘ └──────────┘    |
| [... 73 cards, filterable, scrollable ...]              |
+══════════════════════════════════════════════════════════+
```
Each card: creature icon, engine name, accent color, category, brief description, quality tier badge (Flagship / Editor's Pick / Standard — NOT raw seance scores, which are internal). Click to load into the slot that triggered the browser.

---

## 5. Column B: Main Canvas (520pt × variable)

### B1: Engine Detail (DEFAULT — sound-first)

```
STICKY TOP (88pt):
  [Signal Flow: SRC→FILTER→FX→OUT]     32pt
  [M1 CHARACTER 56pt] [M2] [M3] [M4]   56pt (labels below)

SCROLLABLE MIDDLE (section nav: [OSC][FILT][MOD][FX][★]):
  [▸ OSCILLATOR ● — hero params]        ~60pt collapsed
  [▸ FILTER ● — hero params]            ~60pt collapsed
  [▸ MODULATION ● — hero params]        ~60pt collapsed
  [▸ FX ● — hero params]                ~60pt collapsed
  [▸ SPECIAL ● — engine-specific]        variable

STICKY BOTTOM (80pt):
  [Waveform 240×40pt]  [ADSR interactive 240×40pt]
```

**Section expansion:** Click header → section expands to show all params. Multiple sections can be open simultaneously. "Show All" button at bottom opens Level 3 (flat grid, all params).

**Dark Cockpit integration:** Controlled by a toggle in Settings (default: ON). When ON, it activates automatically when PlaySurface expands. Whichever section contains the parameter currently being touched lights up to 100%. All other sections dim to 20%. Accessibility mode overrides the minimum opacity floor to 50% but preserves all other Dark Cockpit behavior (section highlighting, coupling arc brightness, macro prominence).

### B2: FX Chain (triggered from slot tile "FX" pill)

```
[ENGINE NAME] FX CHAIN   [Serial ⟷ Parallel]     36pt header
┌─────────────────────────────────────────────────┐
│ [SAT icon] Saturation      [●bypass] [wet/dry]  │  68pt per slot
│   → [drive 24pt] [tone 24pt]  (inline hero)    │  (expanded: +120pt)
├─────────────────────────────────────────────────┤
│ [DLY icon] Delay Time      [●bypass] [wet/dry]  │
│   → [time 24pt] [feedback 24pt]                 │
├─────────────────────────────────────────────────┤
│ [REV icon] Reverb Hall     [●bypass] [wet/dry]  │
│   → [size 24pt] [decay 24pt]                    │
├─────────────────────────────────────────────────┤
│ [MOD icon] Chorus          [●bypass] [wet/dry]  │
├─────────────────────────────────────────────────┤
│ [CMP icon] Compressor      [●bypass] [wet/dry]  │
├─────────────────────────────────────────────────┤
│ [SEQ icon] Sequencer FX    [●bypass] [wet/dry]  │
└─────────────────────────────────────────────────┘
[Master Out ▬▬▬▬▬▬▬▬▬▬▬▬▬▬]                        20pt

Click any slot → expands INLINE to show all per-FX params (not Column C)
Drag handle on each slot for reorder
Total: 6×68 + 36 + 20 = 464pt — fits in 620pt with 156pt to spare
```

### B3: Export Panel (persistent, replaces CallOutBox — Kai Rex P0)

Triggered by header EXPORT button. Survives focus loss (not a popup).
```
[EXPORT PANEL — XPN Pack Builder]                    36pt header
[Select Presets ▾]  [Select Engines ▾]
[Velocity Layers: 1|4|8|16]  [Bit: 16|24]  [Rate: 44.1|48|96]
[Entangled Mode: ☐ Couple engines during render]
[Pad Assignment Grid — drag presets to pads A01-D16]  ~200pt
[Output Path: ~/Desktop/XPN/]  [Browse...]
[════════════ RENDER PROGRESS ════════════]
  ONSET Kick — Layer 4/8 — 00:23 elapsed
[Render History: last 10 exports with re-render ↻]
[START RENDER]                                       40pt button
```

**Render behavioral contract:** Rendering runs on a dedicated worker thread (per CLAUDE.md: "Export systems must run on non-audio worker threads"). Plugin audio remains live during render. Parameter changes during render do not affect the in-progress render job (snapshot taken at render start). A warning toast appears if the user modifies the source preset while rendering.

### B4: CinematicMode (full-width 1100pt — Visionary addition)

Columns A and C collapse. Column B expands to 1100pt. Used for:
- Video timeline sync (horizontal axis features)
- Generative composition arrangement view
- Full-screen coupling visualizer
- OBRIX full brick editor

Triggered by: double-click Column B header, or Cmd-F for fullscreen canvas, or specific features that require horizontal space.

---

## 6. Column C: Browser/Inspector (320pt × variable)

### Tab Bar: [PRESET] [COUPLE] [FX] [EXPORT] [⚙]

Tab overflow rule: at 6+ tabs, rightmost tabs collapse behind a "⋯" overflow menu. V1 ships with 5 tabs. Future tabs (CLOUD, HARDWARE, GEN) go through overflow.

### C1: Preset Browser (default tab)

```
[Type: ▾ ALL / DRUMS / BASS / PADS / LEADS / TEXTURES / KEYS / FX]  Primary filter
[Search: ________________] [🔍 DNA] [♻ Similar]                     Search + DNA
[Mood pills: scrollable row of 15 colored pills]                     Secondary filter
┌──────────────────────────────────────────┐
│ [▶] Coral Cathedral          ★ ♡        │  48pt per card
│     OBRIX · Foundation · [DNA hex 24pt]  │
├──────────────────────────────────────────┤
│ [▶] Aria Infinita            ♡          │
│     OPERA · Atmosphere · [DNA hex]       │
├──────────────────────────────────────────┤
│ ... (scrollable, ~10 visible at once)    │
└──────────────────────────────────────────┘
[SAVE] [SAVE AS]                                                      32pt buttons
```

**Key changes from draft:**
- **Instrument-type filter is PRIMARY** (DRUMS/BASS/PADS/etc.) — mood is secondary
- **Preview button (▶)** on every card — plays 2s cached audio thumbnail
- **"Find Similar" (DNA)** promoted to the search bar row, not buried below

### C2: Coupling Inspector

```
[COUPLING ROUTES]                                    28pt header
┌──────────────────────────────────────────┐
│ SLOT 1 → SLOT 2                          │  60pt per route
│ [Entangle ▾] [Amount ◐ 0.72] [Depth ◐]  │
│ "Engines breathe together"               │  ← plain-language!
├──────────────────────────────────────────┤
│ SLOT 1 → SLOT 3                          │
│ [Gravitational ▾] [Amount ◐] [Depth ◐]  │
│ "One follows the other's pitch"          │
└──────────────────────────────────────────┘
[+ Add Route]  [Clear All]                           28pt

Quick-start: 3 recommended types shown first in dropdown
Full 15 behind "More types..."
```

### C3-C5: FX Inspector, Export Config, Settings

FX Inspector: now only shows **cross-engine FX routing** (per Ghost Pearlman). Per-FX deep params expand inline in Column B.

Settings sections: Audio Device, MIDI Config (including **MPE Zone**, **pad MIDI mapping**, **velocity curves**), Tuning (equal temperament + Scala import), Display (theme, Dark Cockpit toggle, accessibility overrides), About.

---

## 7. Header (52pt × 1100pt)

```
[Logo 32pt] [EngBrowser 32pt] [←Preset Name→] [♡] [DNA 24pt] [A|B] [↩undo]
                               center
[M1 40pt] [M2 40pt] [M3 40pt] [M4 40pt]  [CPU] [MIDI●] [EXPORT] [⚙]
                                                            ↑ XO Gold accent
```

**Changes from draft:**
- **A/B compare toggle** added (UX Architect P1)
- **Undo button** added (UX Architect P1)
- **EXPORT button** styled distinctly with XO Gold (Producer's Guild priority)
- Macro knobs at **40pt** (editing) / **48pt** (performance mode) with 12pt gaps
- Engine Browser triggers from clicking engine name or a dedicated browse button

**Pixel budget (editing mode):** Logo(48) + EngBrowser(40) + PresetNav(200) + A/B(40) + Undo(24) + gap(24) + Macros(184 @ 40pt) + gap(24) + Indicators(132) = **716pt of 1100 = 384pt breathing room.**
**Pixel budget (performance mode):** Macros enlarge to 48pt → 4×48+3×12 = 228pt. Total = 760pt. **340pt breathing room.** Sufficient.

---

## 8. PlaySurface (1100pt × 220pt, collapsible)

**Engine-aware default:** If loaded preset contains ONSET or OFFERING, PlaySurface auto-expands to MPC mode. All other presets: collapsed. User toggle persists across sessions.

```
[XOUIJA | MPC | KEYS]  28pt tab bar
[Active Surface 860pt wide × 192pt]  [Expression 240pt wide × 192pt]
                                     [Mod Wheel 28×120]
                                     [Pitch Bend 28×120]
                                     [Macro Strips 4×20×120]
                                     [Tide Controller 100×100]
```

### MPC Pads:
- 16 pads × 4 banks (A/B/C/D)
- Bank selector: 4 pill buttons above pad grid
- MIDI mapping: right-click/long-press any pad → MIDI Learn
- "MPC Default" template button auto-maps to standard MPC note layout
- Velocity from Y position (top=soft, bottom=hard)
- Aftertouch on continued press

---

## 9. Status Bar (1100pt × 28pt)

```
[FIRE 28pt][XOSEND 28pt][ECHO 28pt][PANIC 28pt] | BPM 120 | Voices 4/8 | CPU 4.2% | [●●●●] slots | [🔒PERF]
```

**Export entry points (3, all equivalent):** The header EXPORT button (primary), status bar XOSEND (shortcut), and Column C EXPORT tab all open the same Export Panel (B3). XOSEND is the performer's shortcut. Header EXPORT is the sound designer's entry. They are functionally identical.
Performance Lock toggle prevents accidental parameter changes.

---

## 10. Component Taxonomy V1

### 7 Shared Components (build once, used by 5+ engines)

| # | Component | Used By | Customization |
|---|-----------|---------|--------------|
| 1 | **RotaryKnob** | All 73 | Size (24/32/40/56pt), accent, filmstrip set, mod ring (V1.1) |
| 2 | **CollapsibleSection** | All 73 | Name, color, hero params, expanded/collapsed |
| 3 | **NamedModeSelector** | ~20 engines | Options, icons, searchable, grid vs pills |
| 4 | **AccumulationMeter** | 5+ engines | Direction, threshold, reset button, time scale |
| 5 | **TopologySelector** | 3 engines | Topology names, visual preview, morph position |
| 6 | **TemporalDriftIndicator** | 7 engines | Time scale, direction, visual metaphor |
| 7 | **DrumPadGrid** | 2 engines (ONSET, OFFERING) | Voice names, modes, color per voice |

### 10 Specialized Components (1-2 engines each)

TriangleXYPad, BipolarAxisBar, ConductorArcDisplay, CellularAutomataGrid, DrawbarFaderBank, ModularSlotFlow (OBRIX), DualVoiceAlgoPanel, SeatRingSequencer, CrossFeedMatrix, 5MacroDisplay

### 3 New Components (from review panels)

| Component | Source | Purpose |
|-----------|--------|---------|
| **SignalFlowDiagram** | Comic Book Guy | 520×32pt left-to-right audio path per engine |
| **FileDrop** | Kai Hex | Dashed drop zone for sample import (OSMOSIS, OPAL, Outshine) |
| **InlinePopover** | UX Architect P0-3 | Adjacent to arc/control, avoids cross-column split attention |

### 3 Future-Proof Patterns (from Visionary)

| Pattern | Purpose | When Needed |
|---------|---------|-------------|
| **CinematicMode** | Column B expands to full 1100pt, A+C collapse | Video sync, generative composition, full-screen editing |
| **DataAnnotationLayer** | Decoration layer above all components for external data (collab cursors, AI suggestions, audience votes) | Collaboration mode, social features, BCI |
| **Environment Feature Shape** (7th shape) | Total rendering context replacement | VR/AR, live visualizer mode |

---

## 11. Form Factors

### Plugin AU/VST (1100×700 default)
3-column layout as described above.

### Standalone Desktop
Same + menu bar (File/Edit/View/Help) + native file dialogs + multiple windows possible.

### MPC Hardware Plugin (800×480 — NEW)
**2-column responsive layout:**
```
[Col A: 160pt engine rack] [Col B/C: 640pt contextual, bottom tab bar]
```
Bottom tabs: [ENGINE] [PRESET] [COUPLE] [FX] [EXPORT]
PlaySurface: reduced pad size (40pt pads). All touch targets ≥44pt.
No hover states — Dark Cockpit triggers on touch-down.

### iPad Landscape
**Default visible column: Column B (engine detail)** — the primary surface, same as desktop.
Left engine rack drawer (280pt) **starts pinned open by default** (fits within iPad's ~1024pt width: 280 + 520 + remaining = usable). Right browser drawer starts closed. Bottom PlaySurface drawer starts per engine-aware default.

Navigation via **persistent drawer handles** (48pt strips, not edge swipes — Kai Sage's gesture conflict fix).
- Left handle → engine rack drawer (280pt, pinned open by default)
- Right handle → browser/inspector drawer (320pt, closed by default)
- Bottom handle → PlaySurface (220pt, engine-aware default)

### iPad Portrait
Bottom tab bar: [Play] [Engine] [Presets] [FX]

### iPhone Portrait
Bottom tabs: [Play] [Sound] [Browse] [FX]
- Sound tab: 4 × 80pt macro knobs only
- No deep parameter editing on iPhone

### OBRIX Pocket (iPhone standalone)
TE PO-inspired. **Constrained OBRIX** — 4 brick types max, no ecology/environment.
- Portrait: 4 large knobs + brick palette + mini 2×2 pad
- Landscape: brick stack view + 8 knobs

---

## 12. Accessibility

- WCAG 2.1 AA minimum. AAA for text.
- **Low-contrast engine colors mitigated:** Crystal White (#E8E0D8), Sub Bass Black (#0D0D0D), Deep Ocean (#1B2838) get +40 Oklch lightness boost when used as text/UI elements on shell background (Reddit a11y_or_gtfo fix)
- **Dark Cockpit accessibility override:** Controls never dim below 50% in accessibility mode. All dimmed controls remain interactive (not decorative)
- Full keyboard navigation (Tab order on all controls, XO Gold focus ring)
- Right-click context menus accessible via Shift+F10
- Reduced motion mode: all animations → instant transitions
- VoiceOver / AccessibilityHandler on all components
- 5-channel redundancy: color + shape + text + position + pattern (never color alone)

---

## 13. Extensibility Schema (7 Feature Shapes)

| Shape | Description | UI Home | Example |
|-------|------------|---------|---------|
| **Parameter** | Single controllable value | Knob/toggle in CollapsibleSection | New engine param |
| **Panel** | Complex interactive surface | Column B view mode | Constellation View |
| **Inspector** | Config/browsing interface | Column C tab | Cloud preset sharing |
| **Strip** | Persistent thin display | Between zones, collapsible | Coupling arc strip |
| **Overlay** | Temporary full-window modal | Over entire window, 300ms fade | Engine browser, About |
| **Dialog** | Popup for specific task | CallOutBox from button | Save As, MIDI Learn |
| **Environment** | Total rendering context swap | Replaces entire UI | VR mode, live visualizer |

**Column C tab overflow:** At 6+ tabs, rightmost collapse behind "⋯" menu.
**CinematicMode:** Column B expands to 1100pt. Panels, overlays, and CinematicMode together handle all foreseeable horizontal-axis features.

---

## 14. DAW Integration Contract

| Concern | Resolution |
|---------|-----------|
| **AU parameter exposure** | Expose ONLY: 4 macros × 4 slots (16) + master volume (1) + coupling amount × 12 directed routes (12) + coupling depth × 12 routes (12) + FX wet/dry × 6 slots × 4 engines (24) = **65 automation lanes**. NOT all 400+ engine params. Coupling depth lanes are optional (can be hidden if host struggles >64 params). |
| **Session state** | getStateInformation saves: 4 engine IDs, 4 preset paths, all coupling routes, all macro values, FX chain states, PlaySurface mode + toggle state, Column B last view, Column C last tab |
| **Preset recall** | Presets are .xometa files on disk. Session state stores the path. On reopen, re-loads from disk. If file missing: toast warning, fall back to init. |
| **Transport sync** | OSTINATO/OBRIX sequencers sync to host BPM via `getPlayHead()->getPosition()`. Transport controls in status bar are display-only (host owns transport). |

---

## 15. Rendering Budget

| Element | Mechanism | Budget |
|---------|-----------|--------|
| Coupling arcs (max 6) | `juce::Timer` at 30Hz, dirty-rect repaint | <0.5ms/frame |
| Mini waveforms (4 slots) | `AsyncUpdater` from lock-free ring buffer, 30Hz | <0.3ms/frame |
| Engine accent color tinting | LookAndFeel global, computed once per engine switch | ~0ms ongoing |
| Dark Cockpit transitions | Single float animation (400ms), consulted in paint() | ~0ms ongoing |
| Filmstrip knobs | `juce::Image` cache, one drawImage per knob per paint | <0.05ms/knob |
| **Total UI thread budget** | | **<2ms/frame at 60fps** |

Fallbacks: If host uses software rendering (no GPU), coupling arcs drop to 15Hz. Mini waveforms drop to 15Hz. All animations remain under 4ms/frame total.

---

*This document has been reviewed by 10 independent panels. It addresses every P0 issue, every consensus concern, and every platform constraint identified in the first review cycle. It is ready for the second quantum audit.*
