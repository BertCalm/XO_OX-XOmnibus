# Aquatic Theater Phase 1: Fix Broken Things

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix 5 known bugs in XOmnibusEditor.h — stale tagline, missing mood filters, broken right-click, missing engine removal, vacuous mouseUp check.

**Architecture:** All changes are in `Source/UI/XOmnibusEditor.h` (3006 lines, inline header). No new files. No DSP changes. No preset changes. Pure UI correctness fixes.

**Tech Stack:** JUCE 8, C++17, inline headers. Build: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`

**Spec:** `Docs/superpowers/specs/2026-03-21-aquatic-theater-ui-redesign-design.md` Phase 1

---

### Task 1: Fix stale header tagline (line 2813)

**Files:**
- Modify: `Source/UI/XOmnibusEditor.h:2813`

- [ ] **Step 1: Locate and read the stale string**

At line 2813:
```cpp
: "21 Engines · 12 Coupling Types · 1600+ Presets";
```

- [ ] **Step 2: Replace with dynamic counts**

Replace line 2813 with:
```cpp
: juce::String((int)EngineRegistry::instance().getRegisteredIds().size())
  + " Engines \xc2\xb7 14 Coupling Types \xc2\xb7 "
  + (processor.getPresetManager().getLibrary().empty()
     ? juce::String("18000+")
     : juce::String((int)processor.getPresetManager().getLibrary().size()) + "+")
  + " Presets";
```

Note: `\xc2\xb7` is the UTF-8 encoding of the middle dot `·`. If the existing code uses a literal `·`, keep that instead.

- [ ] **Step 3: Build and verify**

Run: `cmake --build build 2>&1 | grep -E "error:|FAILED:"`
Expected: 0 errors. The tagline should now read "46 Engines · 14 Coupling Types · 18000+ Presets" (or the actual library count once scanned).

- [ ] **Step 4: Commit**

```bash
git add Source/UI/XOmnibusEditor.h
git commit -m "fix: dynamic engine/preset counts in header tagline (was hardcoded 21/1600)"
```

---

### Task 2: Fix mouseUp button check bug (line 883)

**Files:**
- Modify: `Source/UI/XOmnibusEditor.h:881-884`

- [ ] **Step 1: Read the current mouseUp handler**

At line 881-894:
```cpp
void mouseUp(const juce::MouseEvent& e) override
{
    if (!e.mods.isLeftButtonDown() || e.mouseWasDraggedSinceMouseDown())
        return;

    if (hasEngine)
    {
        onSelect(slot);
    }
    else
    {
        showLoadMenu();
    }
}
```

The `!e.mods.isLeftButtonDown()` check is vacuous — on mouseUp, the button has already been released, so `isLeftButtonDown()` always returns false. This means the check never gates anything.

- [ ] **Step 2: Fix the guard**

Replace the mouseUp method (lines 881-894) with:
```cpp
void mouseUp(const juce::MouseEvent& e) override
{
    if (e.mouseWasDraggedSinceMouseDown())
        return;

    if (e.mods.isRightButtonDown())
        return;  // handled by mouseDown or separate right-click handler

    if (hasEngine)
    {
        if (onSelect) onSelect(slot);
    }
    else
    {
        showLoadMenu();
    }
}
```

Changes: removed dead `isLeftButtonDown()` check, added right-button guard (prep for Task 4), added null check on `onSelect`.

- [ ] **Step 3: Build and verify**

Run: `cmake --build build 2>&1 | grep -E "error:|FAILED:"`
Expected: 0 errors.

- [ ] **Step 4: Commit**

```bash
git add Source/UI/XOmnibusEditor.h
git commit -m "fix: remove vacuous isLeftButtonDown check in CompactEngineTile mouseUp"
```

---

### Task 3: Add missing mood filter tabs — Family and Submerged (lines 1422, 1487, 1575, 1606)

**Files:**
- Modify: `Source/UI/XOmnibusEditor.h` at 4 locations

- [ ] **Step 1: Update kNumMoods (line 1606)**

Change:
```cpp
static constexpr int kNumMoods = 7;
```
To:
```cpp
static constexpr int kNumMoods = 9;
```

- [ ] **Step 2: Update moodLabels array (line 1422-1424)**

Change:
```cpp
static const char* moodLabels[] = {
    "ALL", "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether"
};
```
To:
```cpp
static const char* moodLabels[] = {
    "ALL", "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family", "Submerged"
};
```

- [ ] **Step 3: Update moodNames in updateFilter (line 1575-1577)**

Change:
```cpp
static const char* moodNames[] = {
    "", "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether"
};
```
To:
```cpp
static const char* moodNames[] = {
    "", "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family", "Submerged"
};
```

- [ ] **Step 4: Update moodColors in paintListBoxItem (line 1487-1494)**

Change the array to include colors for Family and Submerged:
```cpp
static const juce::Colour moodColors[] = {
    juce::Colour(0xFF00A6D6), // Foundation → Neon Tetra Blue
    juce::Colour(0xFFE8839B), // Atmosphere → Axolotl Gill Pink
    juce::Colour(0xFF7B2D8B), // Entangled  → Violet
    juce::Colour(0xFF0066FF), // Prism      → Electric Blue
    juce::Colour(0xFFE9A84A), // Flux       → Amber
    juce::Colour(0xFFA78BFA), // Aether     → Lavender
    juce::Colour(0xFFFF8A7A), // Family     → Rascal Coral (Obbligato accent)
    juce::Colour(0xFF2D0A4E), // Submerged  → Trench Violet (OceanDeep accent)
};
```

- [ ] **Step 5: Check for moodIds array near line 1495-1497 and update if it exists**

If there is a parallel `moodIds[]` array, add `"Family"` and `"Submerged"` entries in the same positions.

- [ ] **Step 6: Build and verify**

Run: `cmake --build build 2>&1 | grep -E "error:|FAILED:"`
Expected: 0 errors. The preset browser should now show 9 mood tabs.

- [ ] **Step 7: Commit**

```bash
git add Source/UI/XOmnibusEditor.h
git commit -m "fix: add Family and Submerged mood tabs to preset browser (was missing 2 of 8)"
```

---

### Task 4: Implement right-click context menu + engine removal (lines 881, 899)

**Files:**
- Modify: `Source/UI/XOmnibusEditor.h` CompactEngineTile class

- [ ] **Step 1: Add mouseDown handler for right-click**

Add this method to the CompactEngineTile class (before `mouseUp`):

```cpp
void mouseDown(const juce::MouseEvent& e) override
{
    if (e.mods.isPopupMenu() && hasEngine)
    {
        juce::PopupMenu menu;
        menu.addSectionHeader("SLOT " + juce::String(slot + 1) + ": " + engineName.toUpperCase());
        menu.addSeparator();
        menu.addItem(100, "Change Engine...");
        menu.addItem(101, "Remove Engine");
        menu.addSeparator();

        // Move to slot submenu
        juce::PopupMenu moveMenu;
        for (int i = 0; i < 4; ++i)
        {
            if (i != slot)
                moveMenu.addItem(200 + i, "Move to Slot " + juce::String(i + 1));
        }
        menu.addSubMenu("Move to Slot", moveMenu);

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [this](int result)
            {
                if (result == 100)
                    showLoadMenu();
                else if (result == 101)
                {
                    processor.unloadEngine(slot);
                    if (onEngineRemoved) onEngineRemoved(slot);
                }
                else if (result >= 200 && result < 204)
                {
                    int targetSlot = result - 200;
                    // Swap: load this engine in target, unload from current
                    processor.loadEngine(targetSlot, currentEngineId.toStdString());
                    processor.unloadEngine(slot);
                    if (onEngineRemoved) onEngineRemoved(slot);
                }
            });
    }
}
```

- [ ] **Step 2: Add member variables and callback for the new functionality**

Add to CompactEngineTile's member section:
```cpp
std::function<void(int)> onEngineRemoved;
juce::String currentEngineId;  // set when engine loads, used for move/swap
```

Update the `refresh()` method (or wherever `engineName` is set) to also store `currentEngineId` — look for where `hasEngine` is set to `true` and the engine ID is available.

- [ ] **Step 3: Add "Remove Engine" to showLoadMenu as well**

In `showLoadMenu()` (line 899), after the engine list, add:
```cpp
if (hasEngine)
{
    menu.addSeparator();
    menu.addItem(9999, "Remove Engine from Slot " + juce::String(slot + 1));
}
```

And in the async callback, handle it:
```cpp
if (result == 9999)
{
    processor.unloadEngine(slot);
    if (onEngineRemoved) onEngineRemoved(slot);
    return;
}
```

- [ ] **Step 4: Wire onEngineRemoved in XOmnibusEditor**

In the main editor where CompactEngineTile instances are created, add:
```cpp
tiles[i]->onEngineRemoved = [this](int s) {
    // Return to overview when engine is removed
    showOverview();
    for (auto* t : tiles) t->refresh();
};
```

- [ ] **Step 5: Update tooltip to match new behavior**

Find the tooltip string that says "right-click to swap" and update it to:
```cpp
setTooltip("Click to edit parameters. Right-click for options.");
```

- [ ] **Step 6: Build and verify**

Run: `cmake --build build 2>&1 | grep -E "error:|FAILED:"`
Expected: 0 errors. Right-clicking a loaded engine slot should show: Change Engine, Remove Engine, Move to Slot.

- [ ] **Step 7: Commit**

```bash
git add Source/UI/XOmnibusEditor.h
git commit -m "feat: right-click context menu on engine slots (change/remove/move)"
```

---

### Task 5: Final build verification + open standalone

- [ ] **Step 1: Clean rebuild**

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
```
Expected: 0 errors, plugin installs to Components.

- [ ] **Step 2: Open standalone and verify**

```bash
open build/XOmnibus_artefacts/Release/Standalone/XOmnibus.app
```

Verify:
- Header shows "46 Engines · 14 Coupling Types · 18000+ Presets"
- Preset browser shows 9 mood tabs (ALL + 8 moods)
- Left-click on empty slot opens engine load menu
- Right-click on loaded slot shows context menu (Change/Remove/Move)
- Clicking on loaded slot navigates to parameter detail

- [ ] **Step 3: Commit any final adjustments**

```bash
git add Source/UI/XOmnibusEditor.h
git commit -m "phase 1 complete: fix 5 broken UI elements in XOmnibusEditor"
```
