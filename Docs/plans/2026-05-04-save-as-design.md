# Save As / Save Dialog — Design Spec

**Date:** 2026-05-04
**Tracking:** GitHub issue #1405 (`TODO(#1354)` placeholder in `XOceanusEditor.h:1130`)
**Campaign:** V1 Ship Campaign Week 1 Lane B — Day 2 brainstorm #1
**Status:** APPROVED — ready for implementation plan (writing-plans skill)

## Problem

The current Save flow (`Cmd+S` → `juce::AlertWindow` text input) silently overwrites existing presets and has no path to capture mood, category, or tags. Issue #1405: "Save preset has no overwrite protection or mood selector." Result: user-saved presets land uncategorized and invisible in `DnaMapBrowser`'s mood-pill filters.

## Solution at a glance

Split Save into two paths matching macOS conventions:
- **`Cmd+S`** = quick save (silent overwrite of loaded preset; rich dialog only on first save of a fresh-state preset)
- **`Cmd+Shift+S`** = explicit Save As (always opens rich dialog with name + mood + category + tags + description)

## Locked decisions (5 questions resolved 2026-05-04)

| # | Decision | Rationale |
|---|---|---|
| 1 | Architecture: **Split Save / Save As** (macOS standard) | Preserves quick-save power-user loop; matches user muscle memory |
| 2 | `Cmd+S` matrix: rich dialog on fresh state, silent overwrite on modified loaded preset, no-op on unmodified | Captures rich fields at least once per preset; preserves fast iteration loop |
| 3 | Required fields: **Name + Mood**; optional: Category, Tags, Description; auto/hidden: Timbre, Author, Version, Tier | Mood is the primary `DnaMapBrowser` filter; making it required directly fixes the "uncategorized presets" problem |
| 4 | Name collision: **auto-number `(2)` at dialog open + confirm-overwrite if user manually creates collision** | Handles 99% case (sane default) without removing the explicit-overwrite path |
| 5 | Form factor: **modal dialog** styled with existing submarine tokens; mood selector = compact dropdown with color swatches | Modal matches transactional intent; reuses locked Tokens.h palette (no new tokens introduced) |

## Architecture

**One new file, two modified:**
- **NEW** `Source/UI/Ocean/SavePresetDialog.h` — modal dialog component, `juce::DialogWindow` wrapper
- **MOD** `Source/UI/XOceanusEditor.h` — `Cmd+S` / `Cmd+Shift+S` handlers + state matrix in `keyPressed()`; the existing `onSavePreset` lambda at lines 1132–1186 is replaced
- **MOD** `Source/Core/PresetManager.h` — add collision-check overload on `savePresetToFile()` (uses `juce::File::existsAsFile()` plus a confirm callback)

## Components

### SavePresetDialog
- Inherits `juce::Component`; hosted inside a `juce::DialogWindow`
- Constructor: `(PresetData initial, bool isFirstSave, std::function<void(PresetData)> onSave, std::function<void()> onCancel)`
- Internal fields:
  - `NameField` (`juce::TextEditor`) — single-line, `Save` button disabled while empty
  - `MoodDropdown` (`juce::ComboBox`) with custom `MoodDropdownLookAndFeel` for color-swatch rendering — 16 alphabetical entries from `xoceanus::GalleryColors::Ocean::*`
  - `CategoryDropdown` (`juce::ComboBox`) — 10 categories from `PresetTaxonomy.h`, optional ("— None —" first item)
  - `TagsField` (`juce::TextEditor`) — comma-separated, free-form
  - `DescriptionField` (`juce::TextEditor`) — multi-line, ~3 rows, optional
  - `SaveButton` (`juce::TextButton`)
  - `CancelButton` (`juce::TextButton`)
- Layout: vertical stack, ~480×360px content area
- Visual: depth-tone background from `Tokens.h` D1; type from D3 2-font stack; 200ms fade-in (D4); cursor changes per D5

### MoodDropdownLookAndFeel
- Custom `juce::LookAndFeel_V4` subclass overriding `drawComboBoxItem()`
- Renders a 12×12px filled circle (mood color from `GalleryColors.h:576–621`) followed by mood name in standard type
- Reused in any future place that needs mood selection (forward-compatible with #1428 ChainMatrix and #24 modulation viz mood filtering)

### XOceanusEditor changes
- Replace existing `Cmd+S` handler at line 1952
- Add `Cmd+Shift+S` handler
- New helper `getCurrentSaveState()` returning enum `{ NoPresetLoaded, LoadedModified, LoadedUnmodified }` based on `currentPresetData_` and `isDirty_` flags
- Decision tree implementation:
  ```
  if (modifiers.isCommandDown() && !modifiers.isShiftDown() && key == 'S') {
      switch (getCurrentSaveState()) {
          case NoPresetLoaded:    openSavePresetDialog(/*isFirstSave=*/true); break;
          case LoadedModified:    silentOverwriteCurrentPreset(); break;
          case LoadedUnmodified:  /* no-op */ break;
      }
  }
  if (modifiers.isCommandDown() && modifiers.isShiftDown() && key == 'S') {
      openSavePresetDialog(/*isFirstSave=*/false);
  }
  ```

### PresetManager additions
- New overload `savePresetToFile(juce::File targetFile, const PresetData& data, std::function<bool(juce::File)> confirmOverwrite)` — calls `confirmOverwrite(targetFile)` if file exists; if it returns false, abort the save
- Existing `savePresetToFile(file, data)` retained for the silent-overwrite path (no behavior change)
- New helper `getNextAvailableName(const juce::String& base)` — appends `(2)`, `(3)`, etc. until a non-colliding name is found

## Data flow

### Cmd+S pressed
```
keyPressed()
  → getCurrentSaveState()
    → NoPresetLoaded:    openSavePresetDialog(initial=blank, isFirstSave=true)
    → LoadedModified:    PresetManager::savePresetToFile(currentFile, currentData) [silent]
                         → toast "Saved {name}"
    → LoadedUnmodified:  return
```

### Cmd+Shift+S pressed
```
keyPressed()
  → if NoPresetLoaded:
      openSavePresetDialog(initial=blank, isFirstSave=true)
      // Fresh state: Cmd+Shift+S behaves identically to Cmd+S
  → else (LoadedModified or LoadedUnmodified):
      openSavePresetDialog(
        initial = currentPresetData with name = getNextAvailableName(currentName),
        isFirstSave = false
      )
```

In fresh state (no preset loaded), `Cmd+Shift+S` and `Cmd+S` are functionally identical — both open the rich dialog with blank defaults. The split only matters once a preset is loaded: `Cmd+S` silent-overwrites, `Cmd+Shift+S` opens the rich dialog for forking.

### SavePresetDialog Save button
```
onSaveClicked()
  → validateName() — sanitize via juce::File::createLegalFileName(); reject if empty
  → buildTargetFile(sanitizedName)
  → PresetManager::savePresetToFile(target, data, confirmOverwrite=showOverwriteDialog)
    → if collision: showOverwriteDialog() — "A preset named X exists. [Overwrite] [Cancel]"
      → user picks Overwrite → return true → save proceeds
      → user picks Cancel → return false → save aborts (dialog stays open)
    → on disk write success: close dialog, fire toast "Saved {name}"
    → on disk write failure: keep dialog open, error toast, preserve user input
```

## Defaults

| Field | Fresh-state default | Save As fork default |
|---|---|---|
| Name | empty | `getNextAvailableName(currentName)` (e.g. `"My Preset (2)"`) |
| Mood | none (user must pick) | source preset's mood |
| Category | none ("— None —") | source's category |
| Tags | empty | source's tags |
| Description | empty | source's description |
| Author | from settings (`PresetManager::getAuthorFromSettings()` — auto, hidden) | inherit |
| Version | auto: current XOceanus version (hidden) | auto: current XOceanus version |
| Tier | auto: `"awakening"` for user-created (hidden) | inherit |

## Error handling

| Failure | Behavior |
|---|---|
| Empty name | `Save` button disabled; inline "Required" message under field |
| Illegal characters in name | Silently stripped via `juce::File::createLegalFileName()` (no error UI) |
| Disk write failure | Keep dialog open; toast `"Save failed: {error}"`; user input preserved |
| Permission error (read-only filesystem) | Same as disk write failure |
| Collision detected | Confirm-overwrite dialog (Q4 D logic) |

## Visual specification

- Modal background: `Tokens::Submarine::PanelBg` (D1)
- Border: depth-ring style per D5
- Type: D3 2-font stack (heading + body)
- Animation: 200ms fade-in, 0.18 ease-out (D4)
- Mood swatches: 12×12px circle, `GalleryColors::Ocean` mood mapping
- Dialog dimensions: ~480×360px content; resizable false
- **No new tokens.** Implementing sonnet must reuse what's locked in `Source/UI/Tokens.h`.

## Testing

### Unit
- `SavePresetDialog::validateName()` — empty / illegal-chars / sanitized-result cases
- `PresetManager::getNextAvailableName()` — base / `(2)` / `(2)` already exists → `(3)` cases
- `PresetManager::savePresetToFile(file, data, confirmOverwrite)` — collision returns false aborts; collision returns true proceeds; no collision proceeds without invoking callback

### Manual smoke
1. **Fresh state Cmd+S** → rich dialog opens, all fields blank except Name; saving creates a new preset visible in `DnaMapBrowser` filtered by mood
2. **Loaded modified Cmd+S** → no dialog; file mtime updates; toast appears
3. **Loaded unmodified Cmd+S** → no dialog, no toast, no file mtime change
4. **Cmd+Shift+S on loaded preset** → rich dialog opens, name pre-filled `{parent} (2)`, mood/category/tags inherited
5. **Save As fork** → both original and forked preset coexist in browser
6. **Manual collision in Save As** → user types existing name, Save → confirm dialog → Overwrite path replaces file, Cancel path keeps dialog open

### Visual smoke
- Dialog matches submarine theme (depth-tone background, depth-ring border)
- All 16 moods listed alphabetically in dropdown with correct swatch colors
- Tab order: Name → Mood → Category → Tags → Description → Save → Cancel
- Cancel button or `Escape` key dismisses without saving
- Modal blocks clicks on background editor

## Out of scope (deferred)

- "Save as a copy" as a separate menu item (collapsed into Save As)
- Folder/collection support (`PresetData` doesn't model it yet)
- Editing favorites or play-count via dialog (external to preset JSON, owned by `PresetBrowser::toggleFavorite()`)
- Drag-to-reorder in browser
- Mood selector grid variant (deferred to v1.1 if dropdown proves too compact in user testing)

## Implementation estimate

- `SavePresetDialog.h`: ~250 lines (Component + MoodDropdownLookAndFeel)
- `XOceanusEditor.h` changes: ~50 lines (state matrix + 2 keyboard handlers; ~50 lines of `onSavePreset` lambda removed)
- `PresetManager.h` additions: ~20 lines (overload + helper)
- **Total ~320 lines added, ~50 removed.** ~3-4 hour sonnet session for Day 3.

## Downstream impact on other Lane B Day 2-7 features

- **#22 Cmd+K palette (Day 3 brainstorm)**: keyboard handler in `XOceanusEditor::keyPressed()` will need an early-out for `Cmd+K`; design should land before #22 wires its handler
- **#24 modulation viz, #25 A/B compare**: independent, no expected interaction
- **#1428 ChainMatrix (Day 4-5 brainstorm)**: may benefit from `MoodDropdownLookAndFeel` reuse if ChainMatrix needs mood-based filtering — exposed as a public reusable LookAndFeel
- **Dead Gallery panel removal #979**: independent, no interaction

## Source-of-truth references

- `Source/Core/PresetManager.h` lines 337-376 — `PresetData` struct
- `Source/Core/PresetManager.h` lines 517-589 — current `savePresetToFile` + JSON serialization
- `Source/UI/XOceanusEditor.h` lines 1129-1186 — current `onSavePreset` lambda (to be replaced)
- `Source/UI/XOceanusEditor.h` line 1952 — current `Cmd+S` handler
- `Source/Core/PresetTaxonomy.h` — category/timbre enums
- `Source/UI/GalleryColors.h` lines 576-621 — mood color mapping
- `Source/UI/Tokens.h` — locked design tokens (D1-D5, no additions allowed)
