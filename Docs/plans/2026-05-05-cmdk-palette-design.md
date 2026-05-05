# Cmd+K Command Palette — Design Spec

**Date:** 2026-05-05
**Tracking:** Me-First Campaign enhancement #22 (originally IL-4: "Quick-action palette")
**Campaign:** V1 Ship Campaign Week 1 Lane B — Day 3 brainstorm #2
**Status:** APPROVED — ready for implementation plan (writing-plans skill)

## Problem

XOceanus has 86 implemented engines, 19,859 active factory presets, and 16 mood categories. Producers find content via `DnaMapBrowser` (mood-pill filter + 2D scatter map) and the slot bar (manual engine selection). Both work, but neither is keyboard-first. There is no single keystroke that takes a producer from "I need a warm pad" to "the warm pad is loaded into a slot." The Me-First Campaign closed with this feature STOP-gated for design — the moment was: every other workflow polish was in place, but the palette that earned the "Ableton-class workflow speed" claim hadn't been designed.

## Solution at a glance

`Cmd+K` opens a centered modal palette. The user types; the palette returns up to 10 ranked results spanning **presets + engines** (V1 scope). Pressing `Enter` loads the top result into slot 1; `Cmd+2/3/4` loads it into slots 2/3/4. The palette closes on selection, `Esc`, or click-outside. On open with empty input, the palette shows the user's 3 most-recent presets and 3 most-recent engines.

## Locked decisions (5 questions resolved 2026-05-05)

| # | Decision | Rationale |
|---|---|---|
| 1 | **Scope: presets + engines** (no params, no FX in V1) | Smallest cut that feels like a real command palette; dodges param-index complexity across 86 engines; FX/params arrive in v1.1 once usage is observable |
| 2 | **Engine load: modifier-key slot picking** (`Enter` → slot 1; `Cmd+2/3/4` → slots 2/3/4) | Earns the "Ableton-class workflow speed" claim with single-keystroke targeted load; composes with existing 1-5 slot keybinds; sane fallback for casual users |
| 3 | **Smart matching: literal fuzzy + 6D DNA dimension re-rank** (no curated synonyms) | Honors existing DNA investment; "warmth" is the most common producer adjective; ~30-50 lines of re-rank vs ~150 lines + curation pain for synonyms |
| 4 | **Form factor: centered modal overlay**, ~640×480px, full-page-darkening backdrop, single-column results list, top-10 visible | Matches Save As dialog tonal calibration (D1-D5 tokens); not a sidebar (collides with PlaySurface zones); not a dropdown (cramped for 19,859 presets) |
| 5 | **Empty state: recents** — top 3 recent presets + top 3 recent engines on first open | Concrete content beats marketing copy; recents are the cheapest useful signal; matches macOS Spotlight/VS Code Cmd+P empty-state convention |

## Architecture

**One new file, three modified:**
- **NEW** `Source/UI/Ocean/CommandPalette.h` — modal component, fuzzy search engine, result rendering, recents tracking
- **MOD** `Source/UI/XOceanusEditor.h` — `Cmd+K` handler in `keyPressed()`; instantiate and host CommandPalette as a top-level overlay child
- **MOD** `Source/Core/PresetManager.h` — three additions: (a) public accessor `getPresetLibrary()` returning the shared `std::vector<PresetData>` snapshot (promote internal access if not already exposed); (b) new convenience method `loadPresetByIndex(int presetIndex)` that resolves the index against the library and delegates to existing `loadPresetFromFile()` — note that the existing API is `loadPresetFromFile(const juce::File&)` and `loadPresetFromJSON(const juce::String&)`, neither takes an index; (c) `recordPresetLoad(int presetIndex)` + `getRecentPresetIndices()` for recents tracking
- **MOD** `Source/Core/EngineRegistry.h` — `recordEngineLoad(const std::string& id, int slot)` recents tracking (small ring buffer, 8 entries)

## Components

### CommandPalette
- Inherits `juce::Component`; rendered as an overlay child of `XOceanusEditor` (NOT a `juce::DialogWindow` — overlay gives us full control over the depth backdrop)
- Constructor: `(PresetManager& pm, XOceanusProcessor& proc, std::function<void()> onClose)`
- Internal fields:
  - `SearchField` (`juce::TextEditor`) — single-line, focused on open
  - `ResultsList` (custom `juce::Component`) — renders `std::vector<Result>` as keyboard-navigable rows
  - `FooterHint` (`juce::Component`) — static text "⏎ slot 1 · ⌘2-4 slot · esc close"
  - `selectedIndex_` — int, current keyboard cursor in results (clamped 0..results_.size()-1)
  - `searchTimer_` — `juce::Timer` debouncing input by 50ms (re-runs ranking)
- Layout: vertical stack inside a 640×480px content rect, centered. Search field 48px tall at top, results list fills middle, footer 32px at bottom.
- Visual: depth-tone background from `Tokens.h` D1; type from D3 2-font stack; 200ms fade-in (D4); cursor changes per D5; backdrop `Tokens::Submarine::Backdrop50` (50% black overlay)

### Result
```cpp
enum class ResultKind { Preset, Engine };

struct Result {
    ResultKind kind;
    juce::String displayName;       // e.g. "Warm Bass" or "Obsidian"
    juce::String secondaryLabel;    // mood for presets, engine kind for engines
    juce::Colour accent;            // mood color or engine color
    int presetIndex = -1;           // valid when kind == Preset
    std::string engineId;           // valid when kind == Engine
    float score = 0.0f;             // for ranking; higher = better
};
```

### Search & ranking — `CommandPalette::rerank(const juce::String& query)`

1. **Tokenize:** lowercase the query, split on whitespace into `tokens`.
2. **Score each preset:**
   - **Literal score:** for each token, +3.0 per substring hit in `name`, +2.0 per hit in `tags`, +1.0 per hit in `description`, +1.0 per hit in `mood` label.
   - **DNA score:** if any token matches a DNA dimension keyword (`bright`/`brightness`, `warm`/`warmth`, `mov`/`moving`/`movement`, `dense`/`density`, `space`/`spacious`, `aggress`/`aggressive`), add `dna.<dim> * 2.0`.
   - **Mood score:** if any token matches one of the 16 mood labels (case-insensitive), add `+2.0`.
3. **Score each engine:**
   - **Literal score:** for each token, +3.0 per substring hit in engine ID, +2.0 per hit in engine display name, +1.0 per hit in engine kind tag (e.g. "bass", "pad", "string").
   - **No DNA score** (engines aren't DNA-tagged at the engine level — only their presets are).
4. **Filter:** keep only items with `score > 0`.
5. **Sort:** descending by `score`. Tie-break: presets above engines (presets are denser content surface), then alpha.
6. **Top 10:** truncate.

For an **empty query**, skip ranking entirely. Return `recents_` directly: 3 recent presets followed by 3 recent engines.

### Engine load action — `CommandPalette::activateResult(int index, int targetSlot)`

```cpp
void activateResult(int index, int targetSlot) {
    auto& r = results_[index];
    if (r.kind == ResultKind::Preset) {
        presetManager_.loadPresetByIndex(r.presetIndex);
        presetManager_.recordPresetLoad(r.presetIndex);
    } else {
        processor_.loadEngine(targetSlot, r.engineId);
        EngineRegistry::instance().recordEngineLoad(r.engineId, targetSlot);
    }
    onClose_();
}
```

Note: presets ignore `targetSlot` — the preset's stored slot configuration is authoritative. This is correct: a preset is a multi-slot configuration; "load it into slot 2" doesn't make sense. The footer hint reflects this dynamically: when the cursor is on a preset row, footer reads `⏎ load · esc close`; when cursor is on an engine row, footer reads `⏎ slot 1 · ⌘2-4 slot · esc close`.

### Recents tracking
- `PresetManager` adds `std::deque<int> recentPresetIndices_` (max size 8, push-front on load, dedupe). The empty-state UI shows the top 3.
- `EngineRegistry` adds `std::deque<std::string> recentEngineIds_` (max size 8, push-front on load, dedupe). The empty-state UI shows the top 3.
- Both are cleared on plugin instance destruction. **Not persisted across DAW sessions in V1** — persistence is a v1.1 enhancement (would need a settings-file write path, which the Save As work introduced; reuse opportunity but not required).
- **Recording site:** record only on palette-triggered loads (inside `CommandPalette::activateResult`), NOT on every `presetLoaded` / `loadEngine` callback. This keeps recents semantically clean — it reflects "what you searched for and chose," not "every preset that ever loaded including factory init." The implementation is one line per branch in `activateResult`: call `presetManager_.recordPresetLoad(...)` or `EngineRegistry::instance().recordEngineLoad(...)` immediately after the load.
- The recents writes happen on the message thread (palette interactions are message-thread-only). No locking needed.

## Data flow

### Cmd+K pressed
```
XOceanusEditor::keyPressed(Cmd+K)
  → if commandPalette_ == nullptr:
      commandPalette_ = std::make_unique<CommandPalette>(presetManager, processor, [this]{ closeCommandPalette(); });
      addAndMakeVisible(*commandPalette_);
      commandPalette_->setBounds(getLocalBounds());
      commandPalette_->startEntryAnimation();
      commandPalette_->grabSearchFocus();
  → else (already open): no-op (or close — pick one; recommend no-op for now)
```

### User types in search field
```
SearchField::textChanged()
  → palette.searchTimer_.restart(50ms debounce)
    → on timer fire: palette.rerank(searchField.getText())
      → palette.results_ = ranked top-10
      → palette.selectedIndex_ = 0
      → palette.resultsList.repaint()
```

### User presses Enter / Cmd+2/3/4
```
CommandPalette::keyPressed(key)
  → if key == Return: activateResult(selectedIndex_, /*slot=*/0)
  → if key == Cmd+2:  activateResult(selectedIndex_, /*slot=*/1)
  → if key == Cmd+3:  activateResult(selectedIndex_, /*slot=*/2)
  → if key == Cmd+4:  activateResult(selectedIndex_, /*slot=*/3)
  → if key == Escape: onClose_()
  → if key == ArrowDown: selectedIndex_ = min(selectedIndex_+1, results_.size()-1); repaint
  → if key == ArrowUp:   selectedIndex_ = max(selectedIndex_-1, 0); repaint
  → footer label updates per current selectedIndex_'s ResultKind
```

### Click outside
```
XOceanusEditor::mouseDown(e)
  → if commandPalette_ open and click is outside palette content rect:
      closeCommandPalette()
```

## Defaults and corner cases

| Case | Behavior |
|---|---|
| Open with no recents (fresh install) | Empty state shows "Type to search presets and engines · ⌘K closes" |
| Query returns 0 results | Show "No matches" state — single line, dimmed |
| User presses `Cmd+5` (no slot 5) | No-op (only slots 0-3 exist) |
| User presses Cmd+2/3/4 with cursor on a preset row | Ignored (presets don't take a slot arg) — footer hint already reflects this |
| Already-open palette receives `Cmd+K` again | No-op (don't toggle — explicit `Esc` close avoids accidental dismissal mid-type) |
| Disk-load failure (corrupt preset file) | Toast "Failed to load {name}"; palette closes; recents NOT updated |
| Engine factory failure (`createEngine()` returns nullptr) | Toast "Engine {id} unavailable"; palette closes; recents NOT updated |
| Backdrop click-outside | Same as Esc |
| Resizing the editor while palette open | Palette resizes to track full editor bounds; content rect stays centered |

## Visual specification

- Modal backdrop: `Tokens::Submarine::Backdrop50` (50% black overlay over editor; intercepts clicks)
- Content rect: 640×480px, centered, depth-tone background (`Tokens::Submarine::PanelBg` — D1)
- Border: depth-ring style per D5
- Type: D3 2-font stack (heading + body)
- Animation: 200ms fade-in, 0.18 ease-out (D4); content rect scales 95% → 100% to mirror fade
- Result row height: 56px. Layout per row:
  - Left: 8px engine/mood color swatch (full-height bar)
  - Middle: `displayName` (D3 heading-ish, 16pt) + `secondaryLabel` (D3 body, 12pt, 60% opacity)
  - Right: kind badge — "PRESET" or "ENGINE", 10pt uppercase, 50% opacity
- Selected row: background brightens to `Tokens::Submarine::PanelBgHover`
- Search field: 48px tall, no border, monospace placeholder "Search presets and engines…"
- Footer: 32px tall, top border depth-ring, monospace 11pt 60% opacity
- **No new tokens.** Implementing sonnet must reuse what's locked in `Source/UI/Tokens.h`. If `Backdrop50` or `PanelBgHover` don't exist as named tokens yet, derive them inline from existing primaries (e.g., `PanelBg.brighter(0.15f)`) and flag in the PR for v1.1 token consolidation.

## Testing

### Unit
- `CommandPalette::rerank()` — empty query returns recents; "warm" matches presets with literal "warm" in name/tag AND high `warmth` DNA; "obsidian" matches the engine; "bass" matches both bass-tagged presets and the OGRE/OLATE/OAKEN engines.
- `CommandPalette::activateResult()` — preset path calls `loadPresetByIndex`; engine path calls `processor.loadEngine` with correct slot.
- `EngineRegistry::recordEngineLoad()` / `getRecentEngineIds()` — push-front + dedupe + max-size-8 invariants.
- `PresetManager::recordPresetLoad()` / `getRecentPresetIndices()` — same invariants.

### Manual smoke
1. **Cmd+K opens palette** — search field focused, empty state shows "Type to search…" or recents if any exist.
2. **Type "warm"** → top results include presets with high warmth DNA *and* presets with literal "warm" in name. Mixing is correct.
3. **Type "obsidian"** → engine result appears; pressing Enter loads Obsidian into slot 1; Cmd+3 loads it into slot 3.
4. **Type "bass"** → results mix presets (tagged "bass") and engines (OGRE, OLATE, OAKEN, OMEGA). Footer hint changes when cursor moves between a preset row and an engine row.
5. **Esc closes palette** without side effect; `Cmd+K` re-opens; recents now show what was just loaded.
6. **Click outside palette** closes it.
7. **Open palette → don't type → press Enter** loads the most-recent preset.
8. **Resize editor while palette open** — palette tracks full bounds; content rect stays centered.

### Visual smoke
- Modal matches submarine theme (depth-tone background, depth-ring border)
- Backdrop dims editor content but doesn't fully obscure
- All 16 moods render swatches with correct color (matches `GalleryColors::Ocean`)
- Engine results use the engine's accent color (matches `Docs/reference/engine-color-table.md`)
- Result row hover/selected states match Tokens.h motion (200ms / 0.18 ease)
- No paint or layout regression in the editor when palette is closed

## Out of scope (deferred to v1.1)

- Parameter search ("type 'cutoff' → jumps to OddOscar filter cutoff")
- FX chain search (Otrium / Oblate / Oligo / etc.)
- Curated synonym dictionary ("fat" → high warmth+aggression)
- Recents persistence across DAW sessions (settings-file write path)
- Action verbs ("save", "export", "show DnaMap" — non-load actions)
- Cmd+K toggle (currently treats re-press as no-op while open)
- Result preview (hover a preset → audition it)

## Implementation estimate

- `CommandPalette.h`: ~450 lines (Component + ResultsList + ranking + recents wiring)
- `XOceanusEditor.h` changes: ~30 lines (Cmd+K handler + palette lifecycle + click-outside hit-testing)
- `PresetManager.h` additions: ~20 lines (`getPresetLibrary`, `recordPresetLoad`, `getRecentPresetIndices`)
- `EngineRegistry.h` additions: ~20 lines (`recordEngineLoad`, `getRecentEngineIds`)
- **Total ~520 lines added.** ~4-5 hour sonnet session for Day 3 / Day 4.

## Downstream impact on other Lane B Day 4-7 features

- **#1428 ChainMatrix (Day 4-5 brainstorm)**: independent. Palette doesn't search FX chains in V1.
- **#24 modulation viz (Day 4-5)**: independent. Palette doesn't expose modulation routes.
- **#25 A/B compare diff (Day 6-7)**: independent. Palette is a load action; A/B is a state-comparison overlay.
- **Save As (just shipped, PR #1522)**: keyboard handler in `XOceanusEditor::keyPressed()` already handles Cmd+S/Cmd+Shift+S; Cmd+K adds another modifier-key branch — no collision but the keyPressed() function is getting denser. Consider extracting a `KeyboardCommandRouter` helper in v1.1 if a fourth shortcut shows up.
- **Dead Gallery panel removal #979**: independent.

## Source-of-truth references

- `Source/Core/PresetManager.h` lines 337-376 — `PresetData` struct
- `Source/Core/PresetManager.h` line 706 — `allPresets_` shared library
- `Source/UI/Ocean/DnaMapBrowser.h` lines 41-56 — `PresetDot` (DNA fields and shape)
- `Source/Core/EngineRegistry.h` lines 32-78 — `getRegisteredIds()`, `createEngine()`, factories map
- `Source/XOceanusProcessor.cpp` line 3023 — `loadEngine(int slot, const std::string& id)`
- `Source/UI/XOceanusEditor.h` line 1884 — `keyPressed()` (current shortcut routing)
- `Source/UI/Tokens.h` — locked design tokens (D1-D5; no additions allowed for V1)
- `Docs/plans/2026-05-04-save-as-design.md` — design tonal calibration (modal patterns, token usage)
