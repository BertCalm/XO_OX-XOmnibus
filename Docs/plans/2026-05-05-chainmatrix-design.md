# ChainMatrix Design Spec — Wave 5 C4 (Issue #1428)

**Date:** 2026-05-05  
**Author:** Lane B Day 4 session  
**Issue:** #1428  
**Branch:** feat/chainmatrix-v1

---

## 1. Purpose

ChainMatrix is a 5×5 grid editor for the cross-engine coupling matrix (`MegaCouplingMatrix`).
It lets users view all active coupling routes at a glance and add/edit/remove them by clicking
cells. Previously a no-op stub in the `PanelCoordinator`.

---

## 2. Locked Decisions

| # | Lock |
|---|------|
| Q1 Scope | 5×5 grid (slots 0–4 on both axes); click empty to add, click filled to edit |
| Q2 Form | Slide-up drawer from bottom (~50% height; ocean dims, stays visible above) |
| Q3 Add | Click empty cell → 15-type dropdown popover → instant add at depth 0.5 |
| Q4 Persist | No new persistence — coupling routes already serialize via `XOceanusProcessor.cpp` |
| Q5 Trigger | "MATRIX" button in `SubmarineHudBar` (5th button right of Chain, 3×3 grid icon) |

Diagonal cells (i==j) are visually disabled — a slot cannot couple to itself.

---

## 3. Architecture

### 3.1 New files

| File | Role |
|------|------|
| `Source/UI/Ocean/ChainMatrix.h` | Main 5×5 grid drawer component + cell hit-testing + type dropdown |
| `Source/UI/Ocean/HudIcons.h` | Added `makeGridIcon()` static method (3×3 dot grid) |

All files are header-only inline following the XOceanus UI convention.
No new CMakeLists.txt entries needed (headers are included transitively from `OceanView.h`).

### 3.2 Modified files

| File | Change |
|------|--------|
| `Source/UI/Ocean/SubmarineHudBar.h` | Add 5th MATRIX button (kRegMatrix), `onMatrixClicked` callback, `matrixActive_` state, `paintGridIcon()` helper |
| `Source/UI/Ocean/OceanView.h` | Add `chainMatrix_` member, wire MATRIX button, implement `coordinatorRequestOpen(ChainMatrix)` to open drawer, implement `coordinatorCloseCurrentPanel(ChainMatrix)` to close drawer, add `initChainMatrix()` call, add `ChainMatrix.h` include |

### 3.3 MegaCouplingMatrix usage (no changes to MCM)

All operations use existing public API:
- `getRoutes()` — read route list for painting the grid
- `addRoute(src, dst, type, depth)` — add new route
- `removeUserRoute(src, dst, type)` — remove a user-defined route
- `setRouteAmount(idx, amount)` — change depth on existing route

### 3.4 Slide-up drawer pattern

Mirrors `EnginePickerDrawer` / `SettingsDrawer`:
- `open()` / `close()` — animate drawer in/out (250 ms ease-out, 30 Hz timer)
- `isOpen()` — state query
- Drawer is a `juce::Component` + `juce::Timer` child of OceanView
- OceanView calls `addChildComponent(chainMatrix_)` then positions via `resized()`

---

## 4. Visual Spec

### 4.1 Grid

- 5×5 grid centered in drawer; row = source slot (0–4), column = dest slot (0–4)
- Labels: "0" … "4" on row header and column header
- Cell size: ~56×56 px with 4px gap
- Empty cell: faint outlined square (Ocean::plankton at 30% alpha border)
- Filled cell: accent-tinted fill, coupling type label at 9px, depth inner bar (height proportional to amount)
- Diagonal cells (i==j): cross-hatched fill (disabled, Ocean::plankton at 15%)
- Multiple routes on the same (src, dst) pair: show count badge

### 4.2 Drawer

- Drawer occupies bottom ~50% of ocean viewport height, full width
- Background: `GalleryColors::Ocean::twilight` with 95% opacity
- Header bar (36px): "COUPLING MATRIX" label + close ×button
- Drawer slide-up: 250 ms ease-out (matches existing drawers)
- Dismiss: Esc key, click outside the drawer (on dimmed ocean), or MATRIX button toggle

### 4.3 Type dropdown popover

- `juce::PopupMenu` with 15 coupling types grouped by tier:
  - **Safe:** AmpToFilter, AmpToPitch, LFOToPitch, EnvToMorph, FilterToFilter
  - **Standard:** AudioToFM, AudioToRing, AudioToWavetable, AudioToBuffer, RhythmToBlend, EnvToDecay, PitchToPitch
  - **Exotic:** KnotTopology, TriangularCoupling, AmpToChoke
- On selection: `addRoute(src, dst, type, 0.5f)` then repaint
- Duplicate/cycle guard: `addRouteChecked()` return value handled (show no-op toast if blocked)

### 4.4 Edit path (click on filled cell)

- Opens existing `CouplingConfigPopup` via `juce::CallOutBox::launchAsynchronously` pointing at the clicked cell, passing the first route on that (src, dst) pair

---

## 5. Token Usage

- Colors: `GalleryColors::Ocean::*` (twilight, shallow, surface, foam, salt, plankton)
- Accent: `XO::Tokens::Color::accent()` — teal fills for active cells
- Warning: `XO::Tokens::Color::warning()` — for exotic-tier routes
- Typography: `XO::Tokens::Type::body()`, `XO::Tokens::Type::mono()`
- Animation: `XO::Tokens::Motion::RevealMs` (250 ms), `XO::Tokens::Motion::EaseOutStep30Hz` (0.18)

No new tokens. No new design tokens. No token budget impact.

---

## 6. Persistence

No new serialization needed. `MegaCouplingMatrix` routes are serialized by
`XOceanusProcessor::getStateInformation()` / `setStateInformation()` at lines 3594–3618 and 4090–4092.
ChainMatrix is a pure editor-side view; it reads/writes MCM on the message thread.

---

## 7. Smoke Test Checklist (manual, user-executed)

- [ ] MATRIX button appears in HudBar right of Chain; click toggles drawer
- [ ] Drawer slides up from bottom (~50% height); ocean dims behind it
- [ ] Empty cell click shows 15-type popup menu (grouped by tier)
- [ ] Selecting a type adds route; cell fills with teal tint + type label
- [ ] Diagonal cells are greyed out; click does nothing
- [ ] Clicking filled cell opens `CouplingConfigPopup` in a CallOutBox
- [ ] Esc key closes drawer; MATRIX button click again closes drawer
- [ ] Routes survive plugin reload (pre-existing MCM serialization)
- [ ] auval PASS (`auval -v aumu Xocn XoOx`)
