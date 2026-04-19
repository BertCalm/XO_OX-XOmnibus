# TIDEsigns — Phase 3 Design Specification
**Author:** Marina, Creative Director, TIDEsigns
**Date:** 2026-03-27
**Repo:** `~/Documents/GitHub/XO_OX-XOmnibus/`
**Reference:** `Docs/mockups/xoceanus-v05-accurate.html`
**Input:** `Docs/design/tidesigns-audit.md` — Phase 2 findings, 8 Criticals fixed, remaining items below.

---

## Scope

This spec covers the remaining work after the Phase 2 Critical fixes. Organized by the five focus areas from the brief, each entry includes Goal, Spec, and Priority.

---

## Area 1 — Signal Flow Strip: Make Interactive

### Goal
The user clicks any section label in the strip (SRC1 / SRC2 / FILTER / SHAPER / FX / OUT) and the ParameterGrid viewport immediately scrolls to that section. The clicked label highlights in the engine accent color. Hovering a non-active label shows T2. The strip is no longer a static decoration — it is a fast-nav breadcrumb.

### Spec

**File: `Source/UI/XOceanusEditor.h`**

The signal flow strip is currently rendered entirely inside `paint()` (lines 670–723) with no mouse regions and a hardcoded `activeSection = 0`. It must be refactored into a proper component or be hit-tested via `mouseDown` on the editor.

**Step 1 — Add state to XOceanusEditor.**

Add a member variable:
```cpp
int signalFlowActiveSection = 0;
```
This drives which label renders with accent color.

**Step 2 — Store per-section hit rectangles during paint.**

Replace the current paint loop with one that both draws and saves rectangles:
```cpp
std::array<juce::Rectangle<float>, 6> sfHitRects;
```
Populate `sfHitRects[i]` inside the paint loop where each section label is drawn. These are used by `mouseDown`.

**Step 3 — Add `mouseDown` override (or subregion check).**

The signal flow strip occupies `colBPanelFull.removeFromTop(kSignalFlowStripH)` in `resized()`. Store its bounds as `signalFlowStripBounds` (member, set in `resized()`). In `mouseDown`:

```cpp
if (signalFlowStripBounds.contains(e.position))
{
    for (int i = 0; i < 6; ++i)
    {
        if (sfHitRects[i].contains(e.position))
        {
            signalFlowActiveSection = i;
            scrollDetailPanelToSection(i);
            repaint(signalFlowStripBounds.toNearestInt());
            break;
        }
    }
}
```

**Step 4 — Update paint to use `signalFlowActiveSection`.**

Change line 685 from:
```cpp
const int activeSection = 0; // P0-12: default active = SRC1 (index 0)
```
to:
```cpp
const int activeSection = signalFlowActiveSection;
```

Change line 703 from `get(t1())` for active to engine accent color:
```cpp
g.setColour(active ? engineAccentForActiveSlot() : get(t3()));
```
where `engineAccentForActiveSlot()` is a helper that returns the current engine's accent colour from the engine registry. Use `get(t2())` for hover (add `mouseMove` tracking with a `signalFlowHoveredSection = -1` member following the same hit-test logic).

**Step 5 — Label names: keep SRC1/SRC2/FILTER/SHAPER/FX/OUT.**

The v05 mockup HTML (line 1616–1626) uses `SRC1 / SRC2 / FILTER / SHAPER / FX / OUT` — matching the current code. No label name change is needed. The audit note about OSC/ENV/FILTER/FX/COUPLING was a misread of an earlier draft; v05-accurate.html is the canonical reference.

**Step 6 — `scrollDetailPanelToSection(int sectionIndex)`.**

Add a method to `XOceanusEditor` that looks up the `EngineDetailPanel`'s viewport and scrolls it to the matching `ParameterGrid::Section`:

```cpp
void scrollDetailPanelToSection(int sfIndex)
{
    // Map signal flow index to ParameterGrid::Section
    // sfIndex: 0=SRC1→OSC, 1=SRC2→OSC(skip), 2=FILTER→FILTER,
    //          3=SHAPER→MOD, 4=FX→FX, 5=OUT→(top)
    static const ParameterGrid::Section kSfToSection[] = {
        ParameterGrid::Section::OSC,
        ParameterGrid::Section::OSC,
        ParameterGrid::Section::FILTER,
        ParameterGrid::Section::MOD,
        ParameterGrid::Section::FX,
        ParameterGrid::Section::OTHER
    };
    auto* detail = dynamic_cast<EngineDetailPanel*>(/* pointer */);
    if (detail)
        detail->scrollToSection(kSfToSection[sfIndex]);
}
```

Add `scrollToSection(Section s)` to `EngineDetailPanel` (file `Source/UI/Gallery/EngineDetailPanel.h`). This method iterates `ParameterGrid::sectionRuns` to find the Y offset of the target section header and calls `viewport.setViewPosition(0, sectionY)`. The `sectionRuns` vector and `parentViewport` pointer are already accessible — `sectionRuns` is private but a `scrollToSection(ParameterGrid::Section)` public method on `ParameterGrid` is the correct place:

```cpp
// In ParameterGrid.h (public section)
void scrollToSection(Section target)
{
    if (!parentViewport) return;
    int y = kPad;
    for (auto& run : sectionRuns)
    {
        if (run.sec == target)
        {
            parentViewport->setViewPosition(0, juce::jmax(0, y - 4));
            return;
        }
        y += kHeaderRowH;
        if (!collapsedSections.count(run.sec))
        {
            int cols = juce::jmax(1, getWidth() / kCellW);
            y += (run.count + cols - 1) / cols * kCellH;
        }
    }
}
```

`EngineDetailPanel::scrollToSection(ParameterGrid::Section s)` then calls:
```cpp
if (auto* grid = dynamic_cast<ParameterGrid*>(viewport.getViewedComponent()))
    grid->scrollToSection(s);
```

**Step 7 — Active section resets on engine change.**

In `EngineDetailPanel::loadEngine()` (or wherever engine change triggers a detail reload), reset `signalFlowActiveSection = 0` in the editor. Pass a callback or use a listener — the editor already observes slot selection via `selectSlot()`, so reset there.

**Color values (exact, from v05):**
- Active label: engine accent color (same as `engineAccent` used elsewhere in the strip area)
- Hover label: `var(--t2)` = `GalleryColors::get(GalleryColors::t2())`
- Inactive label: `var(--t3)` = `GalleryColors::get(GalleryColors::t3())`
- Active label padding: 3px horizontal, 3px vertical (pill background at 4% white)
- Arrow separators: always T4 (`var(--t4)`)

### Priority
MUST

---

## Area 2 — Coupling Tab Redesign

### Goal
Each collapsed route card shows source engine name, target engine name, and coupling type at a glance. No empty or "ROUTE N" placeholder text. Active routes have the engine accent on their left bar and full-opacity text. Inactive routes are clearly dimmed. The available vertical height is fully used.

### Current State
`CouplingInspectorPanel.h` already renders a collapsed summary (`slotName(source) → slotName(target) · typeShort`) at lines 330–349. The summary text uses T2 (active) or T3 (inactive). The slot names come from `slotName()` which returns generic "Slot 1"..."Slot 4" strings.

The core problem is `slotName()` returns "Slot N" instead of the actual engine name loaded in that slot.

### Spec

**File: `Source/UI/Gallery/CouplingInspectorPanel.h`**

**Fix 1 — `slotName()` should return the actual engine name.**

Current implementation (lines 483–490 approximately):
```cpp
juce::String slotName(int slotIdx) const
{
    return "Slot " + juce::String(slotIdx + 1);
}
```

Replace with a live lookup:
```cpp
juce::String slotName(int slotIdx) const
{
    if (slotIdx < 0 || slotIdx >= 5) return "NONE";
    auto* eng = processor.getEngine(slotIdx);
    if (!eng) return "EMPTY";
    auto id = eng->getEngineId();
    return id.isEmpty() ? ("SL" + juce::String(slotIdx + 1)) : id.toUpperCase();
}
```

The `processor` reference is already a member (`XOceanusProcessor& processor`). `getEngine(int slot)` is already called in the constructor lambda (line 38). This is a one-line logic fix.

**Fix 2 — Engine accent color on active route left bar.**

The active route's left bar (line 306–313) currently uses hardcoded mint green `0xFF52B788`. Replace with the source engine's actual accent color:
```cpp
if (active)
{
    int srcSlot = getSlotIndex(r, "source");
    auto* eng = processor.getEngine(srcSlot);
    juce::Colour barColor = eng ? eng->getAccentColour()
                                : juce::Colour(0xFF52B788); // fallback mint
    g.setColour(barColor.withAlpha(0.9f));
    g.fillRoundedRectangle(...);
}
```

**Fix 3 — Color the source/target names in the collapsed summary.**

The source name should render in the source engine's accent color, and the target name in the target engine's accent color. Arrow stays T4/gold. This matches the v05 mockup pattern (`<span style="color:#1E8B7E;">OBRIX</span> → <span style="color:#D4AF37;">OPERA</span>`).

Since `paint()` draws summary text as a single string (line 337–349), split the draw call into three separate segments: source name (source accent), " → " (T4), target name (target accent). Store the segment widths for the preceding draw call to position correctly.

```cpp
// In paint(), collapsed card section:
juce::String srcName   = slotName(getSlotIndex(r, "source"));
juce::String tgtName   = slotName(getSlotIndex(r, "target"));
juce::String typeShort = getTypeShortLabel(r);
juce::String arrow(juce::CharPointer_UTF8(" \xe2\x86\x92 "));

int srcSlot = getSlotIndex(r, "source");
int tgtSlot = getSlotIndex(r, "target");
auto* srcEng = processor.getEngine(srcSlot);
auto* tgtEng = processor.getEngine(tgtSlot);
juce::Colour srcColor = srcEng ? srcEng->getAccentColour()
                                : get(active ? t2() : t3());
juce::Colour tgtColor = tgtEng ? tgtEng->getAccentColour()
                                : get(active ? t2() : t3());

// Draw source, arrow, target as three segments
// Use g.getCurrentFont().getStringWidthFloat() to advance x
```

**Fix 4 — Height efficiency.**

The `kCardCollapsedH` is currently `kCardGap + kHeaderRowH + kCardGap + 2 = 3 + 26 + 3 + 2 = 34px`. Four cards = 136px. The action row = 34px. Header = 28px. Total = 198px. The panel height is the sidebar height minus the tab bar (32px) = ~636px. There is ~438px of unused space.

Reduce collapsed card height to a tighter 28px by adjusting:
```cpp
static constexpr int kCardGap        = 2;   // was 3
static constexpr int kHeaderRowH     = 24;  // was 26
// kCardCollapsedH = 2 + 24 + 2 + 2 = 30px
```

This saves 4px per card (16px total across 4 cards). More importantly, the extra vertical space below the 4 cards and action row should show a routing hint text block — pull the `coupling-route-hint` pattern from the v05 mockup's Column A mini graph: a single line showing the most active route summary (e.g. "OBRIX → OPERA: Entangle 0.72"). Render this in the space between the last card and the action row using T3 / 8.5px mono font.

### Priority
MUST (Fix 1 and Fix 3 are the visible gap; Fix 2 and Fix 4 are SHOULD)

---

## Area 3 — MasterFX Strip Polish

### Goal
Each FX chip has an accent-colored indicator dot that communicates whether the slot is active (lit) or bypassed (dimmed). The strip reads as 5 discrete pill-chip containers (SAT / DELAY / REVERB / MOD / COMP), not as a flat row of unlabeled knobs. The SEQ toggle integrates cleanly. ADV buttons are small and visually subordinate.

### Current State
`MasterFXSection.h` uses `kMasterFXH = 96px` (audit item C2-03 — target is 68px). Knob fill color is `textMid.withAlpha(0.7f)` (neutral grey) — not accent colored (C4-05). ADV buttons are 14px high × 28px wide; acceptable but positioned at the strip bottom rather than inline with their section. The strip uses `paint()` dividers but not pill-chip backgrounds as in the v05 mockup.

### Spec

**Fix 1 — Reduce strip height from 96px to 68px.**

**File: `Source/UI/XOceanusEditor.h` line 1266**

Change:
```cpp
static constexpr int kMasterFXH = 96;
```
to:
```cpp
static constexpr int kMasterFXH = 68;
```

Consequence: the `resized()` layout in `MasterFXSection.h` must fit within 68px. Current layout constants:
- `kHeaderH2 = 10` (section label)
- `kAdvBtnH = 14`, `kAdvBtnGap = 4`
- Knob 36px + label 10px = 46px
- Total used: 10 + 46 + 4 + 14 = 74px

To fit in 68px, reduce knob size from 36px to 28px (saves 8px):
```cpp
int kh = 28, lh = 9;
```
At 68px outer, `reduced(4,3)` inner = 62px. Stack: 10 (header) + 28 (knob) + 9 (label) + 3 (gap) + 12 (ADV) = 62px. Exact fit.

ADV button height can drop from 14px to 12px:
```cpp
static constexpr int kAdvBtnH = 12;
```

**Fix 2 — FX chip pill backgrounds per section.**

Replace the current single-fill + divider approach with per-section pill backgrounds:

In `paint()`, before drawing dividers, iterate the 6 sections and draw a rounded rect background for each:
```cpp
for (int i = 0; i < kNumSections; ++i)
{
    juce::Rectangle<float> chipRect(
        static_cast<float>(sectionX[i] + 1),
        static_cast<float>(b.getY()),
        static_cast<float>(sectionX[i+1] - sectionX[i] - 2),
        static_cast<float>(b.getHeight()));
    g.setColour(get(elevated())); // #242426
    g.fillRoundedRectangle(chipRect, 4.0f);
    g.setColour(border().withAlpha(0.10f));
    g.drawRoundedRectangle(chipRect, 4.0f, 1.0f);
}
```
Remove the existing single `fillRoundedRectangle` and `drawRoundedRectangle` calls at lines 159–162. Keep the divider paint calls (or remove them since chips now have borders).

**Fix 3 — Accent-colored indicator dot per section.**

Each chip needs a 6px dot (active = engine accent / lit; bypassed = T4 / dark). This is the `fx-slot-chip-dot` in the v05 mockup.

Add a `std::array<bool, 6> sectionActive` member (all true by default). Draw in `paint()`:
```cpp
// In each section's chip area, draw 6px dot at top-right of chip
juce::Colour dotColor = sectionActive[i] ? accentForSection(i)
                                          : get(t4());
float dotX = sectionX[i+1] - 10.0f;
float dotY = b.getY() + 4.0f;
g.setColour(dotColor);
g.fillEllipse(dotX, dotY, 6.0f, 6.0f);
if (sectionActive[i])
    g.setColour(dotColor.withAlpha(0.35f));
    // glow: draw a second, larger ellipse at reduced alpha
    g.fillEllipse(dotX - 2.0f, dotY - 2.0f, 10.0f, 10.0f);
```

For `accentForSection(i)`: SAT=gold, DELAY=T2 (neutral blue-grey), REVERB=T2, MOD=T2, COMP=T2, SEQ=gold. Alternatively use the current engine's accent color for all active sections — this ties the FX visual to the loaded engine, matching the v05 approach.

**Fix 4 — SEQ button integration.**

The SEQ section currently has a `seqToggle` button (22px high) and an ADV button below it. At 68px, the SEQ toggle must shrink proportionally. Use 18px height instead of 22px. The ADV button stays at 12px, same as other sections. Add an "off" visual state: when `seqToggle` is off, the SEQ chip background dims to `elevated().withAlpha(0.5f)` and the dot uses T4.

**File references:**
- `Source/UI/Gallery/MasterFXSection.h` — all layout and paint changes
- `Source/UI/XOceanusEditor.h` line 1266 — constant change only

### Priority
MUST (Fix 1 — height), SHOULD (Fix 2+3 — chip appearance), SHOULD (Fix 4 — SEQ)

---

## Area 4 — Engine Picker Improvements

### Goal
The search box is immediately obvious as the primary interaction. Category pills are fast and legible. The engine list rows feel polished and dense without being cramped.

### Current State
`EnginePickerPopup.h` has a 340×420 popup with:
- Search field (32px row) — correct size, already auto-focused
- 10 category pill buttons (28px row) using 3-letter abbreviations: ALL, SYN, PRC, BAS, PAD, STR, ORG, VOC, FX, UTL
- Engine list rows (kRowHeight px each)

### Spec

**Question from brief: 10 pills or dropdown?**

Keep the pills. 10 pills at 28px is the right call for this use case — the popup is 340px wide, each pill is 34px wide (340 / 10), which fits cleanly without truncation. A dropdown would add a click and hide the categories. The abbreviations (SYN, PRC, etc.) are already legible in context. No change here.

**Fix 1 — Search field visual prominence.**

The search field is already auto-focused and 11px body font. Increase its visual weight:
- Border color on focus: change `focusedOutlineColourId` from `border()` to `engineAccent` (or `xoGold` if no engine is loaded). This ties the search field to the current engine and makes the focused state obvious.
- Placeholder text: change "Search engines..." to "Search 88 engines..." — the count gives users confidence in the scope.
- Height: increase from the current reduced(0,2) within a 32px row to 26px (remove the 2px vertical reduction).

**File: `Source/UI/Gallery/EnginePickerPopup.h` lines 64–78**

```cpp
searchField.setTextToShowWhenEmpty("Search 88 engines...",
                                    GalleryColors::get(GalleryColors::t4()));
// Focused outline: accent
searchField.setColour(juce::TextEditor::focusedOutlineColourId,
                      GalleryColors::get(GalleryColors::obrixAccent())); // or accentColour passed in
```

In `resized()` at line 307:
```cpp
auto searchRow = b.removeFromTop(32);
countLabel.setBounds(searchRow.removeFromRight(52));
searchField.setBounds(searchRow.reduced(0, 3)); // was reduced(0,2) — minor tightening
```

**Fix 2 — Pill button legibility improvement.**

The pill abbreviations are correct but the visual style can improve:

- Active pill: border color changes to `xoGold` at 35% alpha + text in `xoGold`. Background `gold-dim` (`rgba(233,196,106,0.14)`). This matches v05 `.mood-pill.active` exactly.
- Inactive pill: T3 text, no fill, `border()` border. On hover: T2 text.
- Font: 9px body (current) is fine.

Verify `PillButtonLookAndFeel` (lines 25–57 approximately) renders this correctly. If active state uses the wrong color, update `stylePillButton(btn, isActive)` to set:
```cpp
btn.setColour(juce::TextButton::buttonColourId,
              isActive ? GalleryColors::get(GalleryColors::xoGold).withAlpha(0.14f)
                       : juce::Colours::transparentBlack);
btn.setColour(juce::TextButton::textColourOffId,
              isActive ? GalleryColors::get(GalleryColors::xoGold)
                       : GalleryColors::get(GalleryColors::t3()));
```

**Fix 3 — Engine list row refinements.**

Each row shows: 8px accent dot + engine name + category badge. The current list is functional. Improvements:

- Row height: verify `kRowHeight` is 28px. If 32px, reduce to 28px for density.
- Section header rows (e.g. "SYNTH (12)") should use T4 text at 8px mono with left padding matching the dot column. Currently they may not indent consistently.
- Selected row: left border 2px accent color + `rgba(255,255,255,0.045)` background — matches v05 `.preset-card.active`.
- Engine name: 11px body, T1 color. Category badge: 8px mono, T4 (dimmed), right-aligned in the row.

No structural changes — these are paint adjustments in `paintListBoxItem()`.

### Priority
SHOULD (Fix 1 — search prominence is the most visible gap), NICE (Fix 2+3 — incremental polish)

---

## Area 5 — Overall Polish Sweep

### Goal
No duplicate labels. No stale references. Consistent separator and border styling. All dead parking replaced with `setVisible(false)`.

### Remaining Items from Audit (not yet addressed by Phase 2 Critical fixes)

**P5-01 — Fix GalleryLookAndFeel reference comment (C1-05 / C3-07)**

**File: `Source/UI/Gallery/GalleryLookAndFeel.h` line 9**

Change:
```cpp
// Matched to xoceanus-v04-polished.html prototype exactly.
```
to:
```cpp
// Matched to xoceanus-v05-accurate.html prototype exactly.
```

Priority: MUST (prevents future contributors from matching wrong prototype)

---

**P5-02 — Fix tab bar height from 38px to 32px (C1-03)**

**File: `Source/UI/Gallery/SidebarPanel.h` line 428**

Change:
```cpp
static constexpr int kTabBarH = 38;
```
to:
```cpp
static constexpr int kTabBarH = 32;
```

Also update the file header comment (line 7) from `38pt` to `32pt` if it exists.

Priority: MUST

---

**P5-03 — Fix tab bar active underline comment (C1-04)**

**File: `Source/UI/Gallery/SidebarPanel.h` line 9**

The file header says "Active: full-opacity text + 2px XO Gold underline" but the code uses engine accent color (`engineAccent`). The code is correct per v05. Fix the comment:

Change header comment to:
```
// Active: full-opacity text + 2px engine accent color underline
```

Priority: NICE

---

**P5-04 — Remove dead `kMacroKnobsRowH` constant (C3-06)**

**File: `Source/UI/XOceanusEditor.h` line 1268**

Remove:
```cpp
static constexpr int kMacroKnobsRowH = 64; // P0-13: macro knobs row placeholder
```

Priority: SHOULD

---

**P5-05 — Fix `CouplingInspectorPanel` miniViz redundant parking (C3-03)**

**File: `Source/UI/Gallery/CouplingInspectorPanel.h` line 417**

Change:
```cpp
miniViz.setBounds (0, -200, 0, 0);
miniViz.setVisible(false);
```
to:
```cpp
miniViz.setBounds (0, 0, 0, 0);
miniViz.setVisible(false);
```

Priority: SHOULD

---

**P5-06 — Fix `OutshineSidebarPanel` parking coordinate (C3-04)**

**File: `Source/UI/Gallery/SidebarPanel.h` line 399**

Change:
```cpp
outshineSidebar->setBounds(-4000, -4000, 320, 180);
```
to:
```cpp
outshineSidebar->setBounds(0, 0, 0, 0);
outshineSidebar->setVisible(false);
```

Priority: SHOULD

---

**P5-07 — Remove `CreatureState` scaffold struct (C3-05)**

**File: `Source/UI/Gallery/CompactEngineTile.h` lines 17–23**

The `CreatureState` struct (four float members) has no active usage after creature/porthole renderer removal. Remove the struct and its member variable. If creature sprites are planned for v2, document them in `Docs/specs/` instead of leaving dead code in the tile.

Priority: SHOULD

---

**P5-08 — Subtitle font size 8px → 10px (C1-07)**

**File: `Source/UI/XOceanusEditor.h` line 599**

Change:
```cpp
GalleryFonts::body(8.0f)
```
to:
```cpp
GalleryFonts::body(10.0f)
```

Priority: NICE

---

**P5-09 — PLAY button tooltip and SETTINGS note (C4-02)**

The PLAY tab is hidden in the sidebar tab bar (`SidebarPanel.h` line 73–75). Users looking for PlaySurface will not find it. Two mitigations:

1. **File: `Source/UI/XOceanusEditor.h`** — add a tooltip to the PLAY header button (wherever `playBtn` is constructed):
   ```cpp
   playBtn.setTooltip("Open PlaySurface — 4-zone performance interface (PS)");
   ```

2. **File: `Source/UI/Gallery/SettingsPanel.h`** — add a row in the settings panel near the bottom:
   ```
   PlaySurface        [OPEN PS]
   ```
   The button calls `openPlaySurface()` on the processor or fires the same callback as the header PLAY button.

Priority: SHOULD

---

**P5-10 — Tile hover tooltip copy (C4-01)**

**File: `Source/UI/Gallery/CompactEngineTile.h` line 47 (approximately)**

The current tooltip is: "Click to select engine, right-click for options"

Change to: "Click to open engine detail — right-click for options"

"Open engine detail" tells users what will happen. "Select" is ambiguous.

Priority: SHOULD

---

**P5-11 — Remove dead `macroHero.loadEngine()` call (C2-02)**

**File: `Source/UI/Gallery/EngineDetailPanel.h` line 138 (approximately)**

The `macroHero` strip is zeroed out and invisible. The `loadEngine()` call on it still runs on every engine change. Remove it:

```cpp
// Remove this line:
macroHero.loadEngine(engineId, enginePrefix);
```

Also remove the `macroHero.setBounds(0, 0, 0, 0)` and `macroHero.setVisible(false)` calls and the `addChildComponent(macroHero)` call if the strip is confirmed dead for v1. If there is any chance it returns, leave the member but guard the `loadEngine` call with `if (macroHero.isVisible())`.

Priority: SHOULD

---

**P5-12 — `kMasterFXH` constant alignment (C2-03)**

See Area 3, Fix 1. This is the same item.

---

**P5-13 — Resolve dark mode default comment contradiction (C1-08)**

**File: `Source/UI/XOceanusEditor.h` line 90**

Change comment:
```cpp
// Light mode is the default (brand rule)
```
to:
```cpp
// Dark mode is the default. Light mode toggles via DK button. (see getBoolValue("darkMode", true))
```

Priority: NICE

---

## Summary Table

| ID | Area | Priority | File | Change |
|----|------|----------|------|--------|
| A1-01 | Signal Flow — state | MUST | `XOceanusEditor.h` | Add `signalFlowActiveSection` member, `sfHitRects` array |
| A1-02 | Signal Flow — mouse | MUST | `XOceanusEditor.h` | `mouseDown` hit-test on strip bounds |
| A1-03 | Signal Flow — paint | MUST | `XOceanusEditor.h` | Use `signalFlowActiveSection`; accent color for active; T2 for hover |
| A1-04 | Signal Flow — scroll | MUST | `ParameterGrid.h` | Add `scrollToSection(Section)` public method |
| A1-05 | Signal Flow — scroll wiring | MUST | `EngineDetailPanel.h` | Add `scrollToSection(Section)` that delegates to grid |
| A1-06 | Signal Flow — reset | SHOULD | `XOceanusEditor.h` | Reset `signalFlowActiveSection=0` in `selectSlot()` |
| A2-01 | Coupling — engine names | MUST | `CouplingInspectorPanel.h` | `slotName()` uses `processor.getEngine(slot)->getEngineId()` |
| A2-02 | Coupling — accent bar | SHOULD | `CouplingInspectorPanel.h` | Active route left bar uses source engine accent color |
| A2-03 | Coupling — colored summary | MUST | `CouplingInspectorPanel.h` | Src/tgt names drawn in respective engine accent colors |
| A2-04 | Coupling — height use | SHOULD | `CouplingInspectorPanel.h` | Tighten collapsed card height; add routing hint text |
| A3-01 | MasterFX — height | MUST | `XOceanusEditor.h:1266` | `kMasterFXH` 96→68 |
| A3-02 | MasterFX — knob resize | MUST | `MasterFXSection.h` | Knob 36→28px, ADV 14→12px to fit 68px strip |
| A3-03 | MasterFX — chip pills | SHOULD | `MasterFXSection.h` | Per-section pill backgrounds replacing single fill |
| A3-04 | MasterFX — indicator dots | SHOULD | `MasterFXSection.h` | 6px accent dot per section (active/bypassed states) |
| A3-05 | MasterFX — SEQ toggle | SHOULD | `MasterFXSection.h` | Shrink SEQ toggle to 18px; dim chip when off |
| A4-01 | Picker — search prominence | SHOULD | `EnginePickerPopup.h` | Focused border = accent; placeholder = "Search 88 engines..." |
| A4-02 | Picker — pill active color | SHOULD | `EnginePickerPopup.h` | Active pill: gold text + gold-dim bg; hover: T2 |
| A4-03 | Picker — row refinements | NICE | `EnginePickerPopup.h` | Row 28px; left border on selected; category badge T4 |
| P5-01 | Polish — LnF comment | MUST | `GalleryLookAndFeel.h:9` | Update v04 → v05 reference |
| P5-02 | Polish — tab bar height | MUST | `SidebarPanel.h:428` | `kTabBarH` 38 → 32 |
| P5-03 | Polish — tab comment | NICE | `SidebarPanel.h:9` | Fix "XO Gold" comment to "engine accent" |
| P5-04 | Polish — dead constant | SHOULD | `XOceanusEditor.h:1268` | Remove `kMacroKnobsRowH` |
| P5-05 | Polish — miniViz parking | SHOULD | `CouplingInspectorPanel.h:417` | Remove redundant `setBounds(0,-200,…)` |
| P5-06 | Polish — Outshine parking | SHOULD | `SidebarPanel.h:399` | Replace `(-4000,-4000)` with `setVisible(false)` |
| P5-07 | Polish — CreatureState | SHOULD | `CompactEngineTile.h:17-23` | Remove dead struct |
| P5-08 | Polish — subtitle font | NICE | `XOceanusEditor.h:599` | 8px → 10px |
| P5-09 | Polish — PLAY discovery | SHOULD | `XOceanusEditor.h`, `SettingsPanel.h` | Add tooltip + settings row for PlaySurface |
| P5-10 | Polish — tile tooltip | SHOULD | `CompactEngineTile.h:47` | "Click to open engine detail" |
| P5-11 | Polish — dead loadEngine | SHOULD | `EngineDetailPanel.h:138` | Remove `macroHero.loadEngine()` call |
| P5-13 | Polish — dark mode comment | NICE | `XOceanusEditor.h:90` | Fix contradiction in comment |

---

## Build Validation

After each batch of changes:

1. `cmake --build build` — confirm zero new errors/warnings
2. `auval -v aumu Xocn XoOx` — confirm AU validation pass
3. Manual check: click each signal flow label → ParameterGrid scrolls to correct section
4. Manual check: COUPLE tab → collapsed route cards show real engine names in accent colors
5. Manual check: MasterFX strip fits within 68px with no overlapping knob labels
6. Manual check: Engine picker search field shows "Search 88 engines..." placeholder and focused accent border

---

*TIDEsigns Phase 3 Spec — Marina, Creative Director — 2026-03-27*
