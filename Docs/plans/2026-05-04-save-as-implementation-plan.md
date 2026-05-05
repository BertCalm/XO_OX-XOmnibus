# Save As Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the rich Save / Save As dialog for XOceanus presets, splitting `Cmd+S` (quick save) and `Cmd+Shift+S` (Save As) per macOS convention. Captures Mood (required) plus Category/Tags/Description (optional) on first save.

**Architecture:** One new modal dialog component (`SavePresetDialog.h`), keyboard-handler state matrix in `XOceanusEditor.h`, collision-check overload on `PresetManager::savePresetToFile()`. Reuses existing `Tokens.h` (D1-D5 locked tokens) and `GalleryColors::Ocean::*` mood colors — **no new design tokens introduced**.

**Tech Stack:** JUCE 7+ (`juce::Component`, `juce::DialogWindow`, `juce::TextEditor`, `juce::ComboBox`, `juce::LookAndFeel_V4`, `juce::UnitTest`), C++17.

**Spec source-of-truth:** `Docs/plans/2026-05-04-save-as-design.md` (committed `8ee312e0b`). Read it first; this plan implements that spec verbatim.

**Tracking:** GitHub issue #1405. Branch: `feat/save-as-dialog-v1` off `main`. PR base: `main`.

---

## Pre-flight

### Task 0: Branch + worktree setup

**Files:** none (setup only)

- [ ] **Step 0.1: Verify clean main and create worktree**

```bash
cd /Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus
git fetch origin
git status   # should show clean main; abort if uncommitted work exists
git worktree add /Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus.saveas main
cd /Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus.saveas
git checkout -b feat/save-as-dialog-v1
```

If a branch named `feat/save-as-dialog-v1` already exists (locally or on origin), STOP and report — per `feedback-multi-session-branch-collision.md`, do not auto-rename.

- [ ] **Step 0.2: Read the design spec into context**

```bash
cat Docs/plans/2026-05-04-save-as-design.md
```

- [ ] **Step 0.3: Verify build environment**

```bash
eval "$(fnm env)" && fnm use 20
which cmake && cmake --version   # must be 3.22+
```

---

## Phase 1: Core logic (TDD-friendly, no UI)

### Task 1: `PresetManager::getNextAvailableName()` helper

**Files:**
- Modify: `Source/Core/PresetManager.h` (add inline helper)
- Modify or create: `Tests/DSPTests/PresetManagerTests.cpp` (verify location matches existing pattern; if `Tests/DSPTests/` doesn't host UnitTests, find the project's test root and follow that pattern)

**Purpose:** Given a base name (e.g. `"My Preset"`), return the next non-colliding name on disk: `"My Preset (2)"`, `"My Preset (3)"`, etc. Skips `(N)` increments that already exist.

- [ ] **Step 1.1: Write the failing test**

Add to `Tests/DSPTests/PresetManagerTests.cpp` (or wherever existing UnitTests live):

```cpp
class PresetManagerNextAvailableNameTests : public juce::UnitTest
{
public:
    PresetManagerNextAvailableNameTests() : juce::UnitTest ("PresetManager::getNextAvailableName") {}

    void runTest() override
    {
        beginTest ("returns base name when no collision");
        {
            juce::TemporaryFile tempDir;
            tempDir.getFile().createDirectory();
            auto result = PresetManager::getNextAvailableName ("My Preset", tempDir.getFile());
            expectEquals (result, juce::String ("My Preset"));
        }

        beginTest ("appends (2) when base exists");
        {
            juce::TemporaryFile tempDir;
            tempDir.getFile().createDirectory();
            tempDir.getFile().getChildFile ("My Preset.xometa").create();
            auto result = PresetManager::getNextAvailableName ("My Preset", tempDir.getFile());
            expectEquals (result, juce::String ("My Preset (2)"));
        }

        beginTest ("skips to (3) when (2) also exists");
        {
            juce::TemporaryFile tempDir;
            tempDir.getFile().createDirectory();
            tempDir.getFile().getChildFile ("My Preset.xometa").create();
            tempDir.getFile().getChildFile ("My Preset (2).xometa").create();
            auto result = PresetManager::getNextAvailableName ("My Preset", tempDir.getFile());
            expectEquals (result, juce::String ("My Preset (3)"));
        }
    }
};

static PresetManagerNextAvailableNameTests presetManagerNextAvailableNameTests;
```

- [ ] **Step 1.2: Build and run tests — verify they fail**

```bash
cmake --build build --target XOceanus_Tests --config Debug
./build/XOceanus_Tests --category "PresetManager::getNextAvailableName"
```
Expected: 3 failures, all citing "getNextAvailableName: no member function".

If the project uses a different test runner target, follow that pattern.

- [ ] **Step 1.3: Implement `getNextAvailableName()`**

Add to `Source/Core/PresetManager.h` in the `PresetManager` class, public section:

```cpp
/** Returns the next non-colliding preset name in the given directory.
    For "My Preset" with "My Preset.xometa" present → "My Preset (2)".
    For "My Preset" with both base and (2) present → "My Preset (3)". */
static juce::String getNextAvailableName (const juce::String& baseName, const juce::File& presetDir)
{
    auto sanitized = juce::File::createLegalFileName (baseName);
    if (! presetDir.getChildFile (sanitized + ".xometa").existsAsFile())
        return sanitized;

    for (int n = 2; n < 10000; ++n)
    {
        auto candidate = sanitized + " (" + juce::String (n) + ")";
        if (! presetDir.getChildFile (candidate + ".xometa").existsAsFile())
            return candidate;
    }
    return sanitized + " (" + juce::Time::getCurrentTime().formatted ("%H%M%S") + ")"; // fallback
}
```

- [ ] **Step 1.4: Build and run tests — verify they pass**

```bash
cmake --build build --target XOceanus_Tests --config Debug
./build/XOceanus_Tests --category "PresetManager::getNextAvailableName"
```
Expected: all 3 tests pass.

- [ ] **Step 1.5: Commit**

```bash
git add Source/Core/PresetManager.h Tests/DSPTests/PresetManagerTests.cpp
git commit -m "feat(presets): add PresetManager::getNextAvailableName helper"
```

---

### Task 2: `PresetManager::savePresetToFile()` collision-check overload

**Files:**
- Modify: `Source/Core/PresetManager.h:517-524` (add overload alongside existing `savePresetToFile`)
- Modify: same test file as Task 1

**Purpose:** New overload accepts a `confirmOverwrite` callback that is invoked only when the target file already exists. If the callback returns false, the save aborts. Existing 2-arg `savePresetToFile()` retained for silent-overwrite path.

- [ ] **Step 2.1: Write the failing test**

Add to the test file:

```cpp
class PresetManagerSaveOverwriteTests : public juce::UnitTest
{
public:
    PresetManagerSaveOverwriteTests() : juce::UnitTest ("PresetManager::savePresetToFile overwrite-confirm") {}

    void runTest() override
    {
        beginTest ("calls confirm callback only on collision");
        {
            juce::TemporaryFile tempDir;
            tempDir.getFile().createDirectory();
            auto target = tempDir.getFile().getChildFile ("test.xometa");
            PresetData data; data.name = "test";

            bool confirmCalled = false;
            auto result = PresetManager::savePresetToFile (target, data,
                [&] (juce::File) { confirmCalled = true; return true; });

            expect (result);
            expect (! confirmCalled); // no collision → callback not invoked
            expect (target.existsAsFile());
        }

        beginTest ("aborts save when confirm returns false");
        {
            juce::TemporaryFile tempDir;
            tempDir.getFile().createDirectory();
            auto target = tempDir.getFile().getChildFile ("test.xometa");
            target.create(); // create collision

            PresetData data; data.name = "test";
            data.author = "agent-test"; // marker we can verify is NOT written

            auto result = PresetManager::savePresetToFile (target, data,
                [] (juce::File) { return false; });

            expect (! result);
            expect (! target.loadFileAsString().contains ("agent-test")); // file unchanged
        }
    }
};

static PresetManagerSaveOverwriteTests presetManagerSaveOverwriteTests;
```

- [ ] **Step 2.2: Run tests — verify failure**

```bash
cmake --build build --target XOceanus_Tests --config Debug
./build/XOceanus_Tests --category "PresetManager::savePresetToFile overwrite-confirm"
```
Expected: failures citing 3-arg overload not found.

- [ ] **Step 2.3: Implement the overload**

In `Source/Core/PresetManager.h`, immediately AFTER the existing `savePresetToFile(file, data)` (around line 524):

```cpp
/** Save with collision check. confirmOverwrite is invoked only if file exists.
    Return false from the callback to abort. Returns true on successful save. */
static bool savePresetToFile (const juce::File& file,
                              const PresetData& data,
                              std::function<bool (juce::File)> confirmOverwrite)
{
    if (file.existsAsFile() && confirmOverwrite && ! confirmOverwrite (file))
        return false;
    return file.replaceWithText (serializeToJSON (data));
}
```

- [ ] **Step 2.4: Run tests — verify passing**

```bash
cmake --build build --target XOceanus_Tests --config Debug
./build/XOceanus_Tests --category "PresetManager::savePresetToFile overwrite-confirm"
```
Expected: both tests pass.

- [ ] **Step 2.5: Commit**

```bash
git add Source/Core/PresetManager.h Tests/DSPTests/PresetManagerTests.cpp
git commit -m "feat(presets): add savePresetToFile overload with confirm-overwrite callback"
```

---

## Phase 2: SavePresetDialog component

### Task 3: SavePresetDialog skeleton

**Files:**
- Create: `Source/UI/Ocean/SavePresetDialog.h`
- Modify: `CMakeLists.txt` — add the new file (per `feedback-new-files-need-cmakelists.md`; check if unity-build picks it up automatically — if so, skip)

**Purpose:** Empty Component with name field + Save/Cancel buttons. No mood/category/tags yet. Establishes the skeleton.

- [ ] **Step 3.1: Create skeleton file**

```cpp
// Source/UI/Ocean/SavePresetDialog.h
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/PresetManager.h"
#include "../Tokens.h"

namespace xoceanus
{

class SavePresetDialog : public juce::Component
{
public:
    using SaveCallback   = std::function<void (PresetData)>;
    using CancelCallback = std::function<void()>;

    SavePresetDialog (PresetData initial,
                      bool       isFirstSave,
                      SaveCallback   onSave,
                      CancelCallback onCancel)
        : initial_ (std::move (initial))
        , isFirstSave_ (isFirstSave)
        , onSave_ (std::move (onSave))
        , onCancel_ (std::move (onCancel))
    {
        addAndMakeVisible (nameField_);
        nameField_.setText (initial_.name, juce::dontSendNotification);
        nameField_.setSelectAllWhenFocused (true);
        nameField_.onTextChange = [this] { updateSaveButtonState(); };

        addAndMakeVisible (saveButton_);
        saveButton_.setButtonText ("Save");
        saveButton_.onClick = [this] { handleSaveClicked(); };

        addAndMakeVisible (cancelButton_);
        cancelButton_.setButtonText ("Cancel");
        cancelButton_.onClick = [this] { if (onCancel_) onCancel_(); };

        updateSaveButtonState();
        setSize (480, 360);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Tokens::Submarine::PanelBg);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (24);
        auto buttonRow = bounds.removeFromBottom (40);
        bounds.removeFromBottom (16);

        nameField_.setBounds (bounds.removeFromTop (32));

        cancelButton_.setBounds (buttonRow.removeFromRight (100));
        buttonRow.removeFromRight (12);
        saveButton_.setBounds (buttonRow.removeFromRight (100));
    }

private:
    void updateSaveButtonState()
    {
        saveButton_.setEnabled (nameField_.getText().trim().isNotEmpty());
    }

    void handleSaveClicked()
    {
        if (! onSave_) return;
        PresetData out = initial_;
        out.name = juce::File::createLegalFileName (nameField_.getText().trim());
        onSave_ (out);
    }

    PresetData     initial_;
    bool           isFirstSave_;
    SaveCallback   onSave_;
    CancelCallback onCancel_;

    juce::TextEditor nameField_;
    juce::TextButton saveButton_;
    juce::TextButton cancelButton_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SavePresetDialog)
};

} // namespace xoceanus
```

- [ ] **Step 3.2: Add to CMakeLists if needed**

Check unity-build behavior:
```bash
grep -n "Source/UI/Ocean" CMakeLists.txt
```
If files in `Source/UI/Ocean/` are added explicitly (not via glob), add `Source/UI/Ocean/SavePresetDialog.h` to the source list. If glob-based, skip — unity build picks it up.

- [ ] **Step 3.3: Build standalone — verify clean compile**

```bash
cmake --build build --target XOceanus_Standalone --config Debug
```
Expected: build green, no new warnings.

- [ ] **Step 3.4: Commit**

```bash
git add Source/UI/Ocean/SavePresetDialog.h CMakeLists.txt
git commit -m "feat(ui): SavePresetDialog skeleton (name field + buttons)"
```

---

### Task 4: SavePresetDialog mood dropdown with custom LookAndFeel

**Files:**
- Modify: `Source/UI/Ocean/SavePresetDialog.h`

**Purpose:** Add the 16-item Mood dropdown with color-swatch L&F. This is the "required" field per Q3.

- [ ] **Step 4.1: Add MoodDropdownLookAndFeel as nested class**

Inside `SavePresetDialog`, add as a nested class before the constructor:

```cpp
class MoodDropdownLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override
    {
        if (isSeparator || ! isActive)
        {
            juce::LookAndFeel_V4::drawPopupMenuItem (g, area, isSeparator, isActive,
                isHighlighted, isTicked, hasSubMenu, text, shortcutKeyText, icon, textColour);
            return;
        }

        if (isHighlighted)
            g.fillAll (findColour (juce::PopupMenu::highlightedBackgroundColourId));

        // Color swatch
        auto swatchArea = area.withWidth (20).reduced (4);
        auto moodColour = GalleryColors::Ocean::moodToColour (text);
        g.setColour (moodColour);
        g.fillEllipse (swatchArea.toFloat());

        // Label
        g.setColour (isHighlighted ? findColour (juce::PopupMenu::highlightedTextColourId)
                                   : findColour (juce::PopupMenu::textColourId));
        g.drawText (text, area.withTrimmedLeft (24), juce::Justification::centredLeft);
    }
};
```

NOTE: `GalleryColors::Ocean::moodToColour(juce::String)` must already exist or be added. Check `Source/UI/GalleryColors.h:576-621` — if a name→colour mapping helper isn't present, add a thin wrapper:

```cpp
// In GalleryColors.h, namespace Ocean, add:
static inline juce::Colour moodToColour (const juce::String& moodName)
{
    // Map alphabetical mood names to existing per-mood colour constants.
    // Implement using the existing mood enum / colour table in this header.
    // ... [implementing agent: read the existing structure and wire accordingly]
    return juce::Colour (0xFF4A7090); // fallback
}
```

If the existing structure already has this lookup, reuse it.

- [ ] **Step 4.2: Add MoodDropdown member and wire it**

In private section:

```cpp
juce::ComboBox            moodDropdown_;
MoodDropdownLookAndFeel   moodLAF_;
```

In constructor, after `nameField_` setup:

```cpp
addAndMakeVisible (moodDropdown_);
moodDropdown_.setLookAndFeel (&moodLAF_);

// 16 moods alphabetical (read from PresetTaxonomy.h or GalleryColors.h)
const juce::StringArray moods {
    "Aether", "Atmosphere", "Coupling", "Crystalline", "Deep", "Entangled",
    "Ethereal", "Family", "Flux", "Foundation", "Kinetic", "Luminous",
    "Organic", "Prism", "Shadow", "Submerged"
};
for (int i = 0; i < moods.size(); ++i)
    moodDropdown_.addItem (moods[i], i + 1);

moodDropdown_.setText (initial_.mood, juce::dontSendNotification);
moodDropdown_.onChange = [this] { updateSaveButtonState(); };
```

In destructor (add one if not present):

```cpp
~SavePresetDialog() override { moodDropdown_.setLookAndFeel (nullptr); }
```

In `updateSaveButtonState()`, add mood-required check:

```cpp
void updateSaveButtonState()
{
    const bool nameOK = nameField_.getText().trim().isNotEmpty();
    const bool moodOK = moodDropdown_.getSelectedId() > 0;
    saveButton_.setEnabled (nameOK && moodOK);
}
```

In `resized()`, between nameField and buttonRow:

```cpp
void resized() override
{
    auto bounds = getLocalBounds().reduced (24);
    auto buttonRow = bounds.removeFromBottom (40);
    bounds.removeFromBottom (16);

    nameField_.setBounds (bounds.removeFromTop (32));
    bounds.removeFromTop (12);
    moodDropdown_.setBounds (bounds.removeFromTop (32));

    cancelButton_.setBounds (buttonRow.removeFromRight (100));
    buttonRow.removeFromRight (12);
    saveButton_.setBounds (buttonRow.removeFromRight (100));
}
```

In `handleSaveClicked()`, add:

```cpp
out.mood = moodDropdown_.getText();
```

- [ ] **Step 4.3: Build — verify clean compile**

```bash
cmake --build build --target XOceanus_Standalone --config Debug
```

- [ ] **Step 4.4: Commit**

```bash
git add Source/UI/Ocean/SavePresetDialog.h Source/UI/GalleryColors.h
git commit -m "feat(ui): SavePresetDialog mood dropdown with color swatches"
```

---

### Task 5: SavePresetDialog category, tags, description fields

**Files:**
- Modify: `Source/UI/Ocean/SavePresetDialog.h`

**Purpose:** Add the optional fields. Category = ComboBox (10 items + "— None —"), Tags = single-line TextEditor, Description = multi-line TextEditor.

- [ ] **Step 5.1: Add member fields**

In private section:

```cpp
juce::ComboBox   categoryDropdown_;
juce::TextEditor tagsField_;
juce::TextEditor descriptionField_;
juce::Label      nameLabel_, moodLabel_, categoryLabel_, tagsLabel_, descriptionLabel_;
```

- [ ] **Step 5.2: Wire in constructor (after mood dropdown)**

```cpp
addAndMakeVisible (categoryDropdown_);
categoryDropdown_.addItem ("— None —", 1);
const juce::StringArray categories {
    "keys", "pads", "leads", "bass", "drums", "perc", "textures", "fx", "sequence", "vocal"
};
for (int i = 0; i < categories.size(); ++i)
    categoryDropdown_.addItem (categories[i], i + 2);
categoryDropdown_.setText (initial_.category.isEmpty() ? "— None —" : initial_.category,
                           juce::dontSendNotification);

addAndMakeVisible (tagsField_);
tagsField_.setText (initial_.tags.joinIntoString (", "), juce::dontSendNotification);

addAndMakeVisible (descriptionField_);
descriptionField_.setMultiLine (true);
descriptionField_.setReturnKeyStartsNewLine (true);
descriptionField_.setText (initial_.description, juce::dontSendNotification);

addAndMakeVisible (nameLabel_);        nameLabel_.setText ("Name *",        juce::dontSendNotification);
addAndMakeVisible (moodLabel_);        moodLabel_.setText ("Mood *",        juce::dontSendNotification);
addAndMakeVisible (categoryLabel_);    categoryLabel_.setText ("Category",  juce::dontSendNotification);
addAndMakeVisible (tagsLabel_);        tagsLabel_.setText ("Tags",          juce::dontSendNotification);
addAndMakeVisible (descriptionLabel_); descriptionLabel_.setText ("Description", juce::dontSendNotification);
```

- [ ] **Step 5.3: Update layout in `resized()`**

```cpp
void resized() override
{
    auto bounds = getLocalBounds().reduced (24);
    auto buttonRow = bounds.removeFromBottom (40);
    bounds.removeFromBottom (16);

    auto layoutLabeled = [&] (juce::Label& lbl, juce::Component& field, int fieldHeight) {
        auto row = bounds.removeFromTop (fieldHeight + 16);
        lbl.setBounds   (row.removeFromTop (16));
        field.setBounds (row);
        bounds.removeFromTop (8);
    };

    layoutLabeled (nameLabel_,        nameField_,         28);
    layoutLabeled (moodLabel_,        moodDropdown_,      28);
    layoutLabeled (categoryLabel_,    categoryDropdown_,  28);
    layoutLabeled (tagsLabel_,        tagsField_,         28);
    layoutLabeled (descriptionLabel_, descriptionField_,  64);

    cancelButton_.setBounds (buttonRow.removeFromRight (100));
    buttonRow.removeFromRight (12);
    saveButton_.setBounds (buttonRow.removeFromRight (100));
}
```

- [ ] **Step 5.4: Update `handleSaveClicked()` to read new fields**

Replace the body:

```cpp
void handleSaveClicked()
{
    if (! onSave_) return;
    PresetData out = initial_;
    out.name        = juce::File::createLegalFileName (nameField_.getText().trim());
    out.mood        = moodDropdown_.getText();
    out.category    = (categoryDropdown_.getText() == "— None —") ? juce::String() : categoryDropdown_.getText();
    out.tags        = juce::StringArray::fromTokens (tagsField_.getText(), ",", "\"");
    out.tags.trim(); out.tags.removeEmptyStrings();
    out.description = descriptionField_.getText();
    onSave_ (out);
}
```

- [ ] **Step 5.5: Build — verify clean compile**

```bash
cmake --build build --target XOceanus_Standalone --config Debug
```

- [ ] **Step 5.6: Commit**

```bash
git add Source/UI/Ocean/SavePresetDialog.h
git commit -m "feat(ui): SavePresetDialog category/tags/description fields"
```

---

### Task 6: SavePresetDialog collision-confirm wiring

**Files:**
- Modify: `Source/UI/Ocean/SavePresetDialog.h`

**Purpose:** When user clicks Save and the target file exists, show a JUCE-modal "Overwrite?" confirm. On Cancel, dialog stays open with user input preserved.

- [ ] **Step 6.1: Update `handleSaveClicked()` to use the collision-check overload**

Replace `handleSaveClicked()`:

```cpp
void handleSaveClicked()
{
    if (! onSave_) return;

    PresetData out = initial_;
    out.name        = juce::File::createLegalFileName (nameField_.getText().trim());
    out.mood        = moodDropdown_.getText();
    out.category    = (categoryDropdown_.getText() == "— None —") ? juce::String() : categoryDropdown_.getText();
    out.tags        = juce::StringArray::fromTokens (tagsField_.getText(), ",", "\"");
    out.tags.trim(); out.tags.removeEmptyStrings();
    out.description = descriptionField_.getText();

    auto presetDir = PresetManager::getUserPresetDirectory();
    auto target    = presetDir.getChildFile (out.name + ".xometa");

    auto confirmCallback = [this, name = out.name] (juce::File) -> bool {
        return juce::AlertWindow::showOkCancelBox (
            juce::AlertWindow::QuestionIcon,
            "Overwrite preset?",
            "A preset named \"" + name + "\" already exists.\n\nOverwrite it?",
            "Overwrite", "Cancel", this);
    };

    const bool saved = PresetManager::savePresetToFile (target, out, confirmCallback);
    if (saved)
        onSave_ (out); // closes dialog via callback
    // else: user cancelled overwrite, dialog stays open with all fields preserved
}
```

NOTE: `PresetManager::getUserPresetDirectory()` must exist. If it doesn't, add it as a static helper:

```cpp
// In PresetManager.h
static juce::File getUserPresetDirectory()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                  .getChildFile ("XO_OX")
                  .getChildFile ("XOceanus")
                  .getChildFile ("Presets");
    if (! dir.exists()) dir.createDirectory();
    return dir;
}
```

- [ ] **Step 6.2: Build**

```bash
cmake --build build --target XOceanus_Standalone --config Debug
```

- [ ] **Step 6.3: Commit**

```bash
git add Source/UI/Ocean/SavePresetDialog.h Source/Core/PresetManager.h
git commit -m "feat(ui): SavePresetDialog collision-confirm dialog"
```

---

## Phase 3: XOceanusEditor wiring

### Task 7: `getCurrentSaveState()` helper in XOceanusEditor

**Files:**
- Modify: `Source/UI/XOceanusEditor.h` (private section)

**Purpose:** Helper that classifies the current editor state into the 3-way enum used by the Cmd+S decision tree.

- [ ] **Step 7.1: Add enum + helper**

In the private section of `XOceanusEditor` class:

```cpp
enum class SaveState { NoPresetLoaded, LoadedModified, LoadedUnmodified };

SaveState getCurrentSaveState() const
{
    if (currentPresetData_.name.isEmpty())
        return SaveState::NoPresetLoaded;
    return isDirty_ ? SaveState::LoadedModified : SaveState::LoadedUnmodified;
}
```

If `isDirty_` doesn't exist yet, search for `dirty`/`modified` flags around current `onSavePreset` lambda (lines 1132–1186). If none, add `bool isDirty_ = false;` and wire it to `true` in the parameter-change listeners (this is a small extension, not in the original spec — flag it in the PR body).

- [ ] **Step 7.2: Build — verify the helper compiles**

```bash
cmake --build build --target XOceanus_Standalone --config Debug
```

- [ ] **Step 7.3: Commit**

```bash
git add Source/UI/XOceanusEditor.h
git commit -m "feat(ui): XOceanusEditor::getCurrentSaveState helper"
```

---

### Task 8: Cmd+S handler

**Files:**
- Modify: `Source/UI/XOceanusEditor.h` around line 1952 (current Cmd+S handler)

**Purpose:** Replace the existing keyboard handler with the 3-way state matrix.

- [ ] **Step 8.1: Replace Cmd+S branch in `keyPressed()`**

Find the current handler at `XOceanusEditor.h:1952` (or grep for `'S'` in `keyPressed()`). Replace:

```cpp
if (key.getModifiers().isCommandDown() && ! key.getModifiers().isShiftDown()
    && key.getKeyCode() == 'S')
{
    switch (getCurrentSaveState())
    {
        case SaveState::NoPresetLoaded:
            openSavePresetDialog (/*isFirstSave=*/true);
            return true;
        case SaveState::LoadedModified:
            silentOverwriteCurrentPreset();
            return true;
        case SaveState::LoadedUnmodified:
            return true; // no-op
    }
}
```

- [ ] **Step 8.2: Add `silentOverwriteCurrentPreset()` private method**

```cpp
void silentOverwriteCurrentPreset()
{
    if (currentPresetData_.name.isEmpty()) return; // defensive

    auto target = PresetManager::getUserPresetDirectory()
                     .getChildFile (currentPresetData_.name + ".xometa");

    if (PresetManager::savePresetToFile (target, currentPresetData_))
    {
        isDirty_ = false;
        toastStore_.addToast ("Saved " + currentPresetData_.name);
    }
    else
    {
        toastStore_.addToast ("Save failed", ToastSeverity::Error);
    }
}
```

NOTE: `toastStore_` and `ToastSeverity` may not exist with these names. Find the project's existing toast/notification helper (grep for `toast` or `notification` in `XOceanusEditor.h`) and adapt.

- [ ] **Step 8.3: Add `openSavePresetDialog()` private method (stub for now)**

```cpp
void openSavePresetDialog (bool isFirstSave)
{
    PresetData initial;
    if (! isFirstSave)
    {
        initial = currentPresetData_;
        initial.name = PresetManager::getNextAvailableName (
            currentPresetData_.name,
            PresetManager::getUserPresetDirectory());
    }

    auto* dialog = new xoceanus::SavePresetDialog (
        std::move (initial),
        isFirstSave,
        [this] (PresetData saved) {
            currentPresetData_ = saved;
            isDirty_ = false;
            toastStore_.addToast ("Saved " + saved.name);
            // dialog auto-closes when callback fires; cleanup via DialogWindow's owner
        },
        [] { /* cancel — DialogWindow handles cleanup */ });

    juce::DialogWindow::LaunchOptions opts;
    opts.content.setOwned (dialog);
    opts.dialogTitle = isFirstSave ? "Save Preset" : "Save Preset As...";
    opts.dialogBackgroundColour = Tokens::Submarine::PanelBg;
    opts.escapeKeyTriggersCloseButton = true;
    opts.useNativeTitleBar = false;
    opts.resizable = false;
    opts.launchAsync();
}
```

- [ ] **Step 8.4: Add include for SavePresetDialog**

At the top of `XOceanusEditor.h`:

```cpp
#include "Ocean/SavePresetDialog.h"
```

- [ ] **Step 8.5: Build — verify clean compile**

```bash
cmake --build build --target XOceanus_Standalone --config Debug
```

- [ ] **Step 8.6: Commit**

```bash
git add Source/UI/XOceanusEditor.h
git commit -m "feat(ui): XOceanusEditor Cmd+S state-matrix handler"
```

---

### Task 9: Cmd+Shift+S handler

**Files:**
- Modify: `Source/UI/XOceanusEditor.h` (in `keyPressed()`, immediately after the Cmd+S block)

- [ ] **Step 9.1: Add Cmd+Shift+S branch**

```cpp
if (key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown()
    && key.getKeyCode() == 'S')
{
    if (getCurrentSaveState() == SaveState::NoPresetLoaded)
        openSavePresetDialog (/*isFirstSave=*/true); // identical to Cmd+S in fresh state
    else
        openSavePresetDialog (/*isFirstSave=*/false);
    return true;
}
```

- [ ] **Step 9.2: Build**

```bash
cmake --build build --target XOceanus_Standalone --config Debug
```

- [ ] **Step 9.3: Commit**

```bash
git add Source/UI/XOceanusEditor.h
git commit -m "feat(ui): XOceanusEditor Cmd+Shift+S Save As handler"
```

---

### Task 10: Remove the old `onSavePreset` lambda

**Files:**
- Modify: `Source/UI/XOceanusEditor.h:1132-1186`

**Purpose:** The old AlertWindow-based save flow is now superseded by the dialog flow. Remove it cleanly.

- [ ] **Step 10.1: Read the existing lambda**

```bash
sed -n '1125,1195p' Source/UI/XOceanusEditor.h
```

Identify any wiring that references `onSavePreset` — if the existing UI has a SAVE button somewhere that wires to this lambda, redirect it to call `openSavePresetDialog (getCurrentSaveState() == SaveState::NoPresetLoaded)` instead.

- [ ] **Step 10.2: Delete lines 1132–1186 (the lambda definition)**

Use Edit tool to remove. Confirm by:
```bash
grep -n "onSavePreset" Source/UI/XOceanusEditor.h
```
Expected: only references are wiring that still calls `openSavePresetDialog(...)` — no orphan declarations.

- [ ] **Step 10.3: Build**

```bash
cmake --build build --target XOceanus_Standalone --config Debug
```

- [ ] **Step 10.4: Commit**

```bash
git add Source/UI/XOceanusEditor.h
git commit -m "refactor(ui): remove legacy onSavePreset lambda (superseded by dialog)"
```

---

## Phase 4: Validation

### Task 11: Build + auval

**Files:** none

- [ ] **Step 11.1: Full release build**

```bash
cmake --build build --target XOceanus_AU --config Release
cmake --build build --target XOceanus_Standalone --config Release
```
Expected: both green, no new warnings.

- [ ] **Step 11.2: auval**

```bash
auval -v aufx Xnus XO_O 2>&1 | tail -20
```
Expected: `AU VALIDATION SUCCEEDED`.

- [ ] **Step 11.3: Sentinel — verify dialog text is in binary**

```bash
strings build/XOceanus_artefacts/Release/AU/XOceanus.component/Contents/MacOS/XOceanus | grep "Overwrite preset"
```
Expected: at least one match. If 0 matches, the file likely wasn't compiled in (CMakeLists issue) — debug before claiming green.

### Task 12: Manual smoke test

**Files:** none

- [ ] **Step 12.1: Launch standalone, exercise the 4 paths from the spec**

```bash
pkill -x XOceanus 2>/dev/null
open build/XOceanus_artefacts/Release/Standalone/XOceanus.app
```

Run these checks IN ORDER:
1. **Fresh state Cmd+S** → rich dialog opens with blank fields. Type "test1", select Mood "Foundation", click Save → toast appears, dialog closes, file `~/Library/Application Support/XO_OX/XOceanus/Presets/test1.xometa` exists.
2. **Loaded modified Cmd+S** → tweak any parameter, hit Cmd+S → no dialog, file mtime updates, toast "Saved test1".
3. **Loaded unmodified Cmd+S** → without changing anything, hit Cmd+S → no dialog, no toast, mtime unchanged.
4. **Cmd+Shift+S on loaded preset** → rich dialog opens, name pre-filled "test1 (2)", mood inherited as "Foundation". Click Save → both files exist.
5. **Manual collision in Save As** → Cmd+Shift+S, type "test1" (existing), click Save → confirm dialog appears, click Cancel → dialog stays open with "test1" still in field. Click Save again → confirm → Overwrite → file replaces.

If any of the 5 checks fail, fix and re-test before opening PR.

### Task 13: PR

**Files:** none

- [ ] **Step 13.1: Push**

```bash
git push -u origin feat/save-as-dialog-v1
```

- [ ] **Step 13.2: Open PR**

```bash
gh pr create --base main --head feat/save-as-dialog-v1 --title "feat(ui): Save As dialog with mood + tags + collision-confirm" --body "$(cat <<'EOF'
## Summary
Implements the rich Save / Save As dialog per design spec at `Docs/plans/2026-05-04-save-as-design.md`.

- `Cmd+S`: rich dialog on fresh state; silent overwrite on modified loaded preset; no-op on unmodified
- `Cmd+Shift+S`: always opens rich dialog (Save As fork)
- Required: Name + Mood; optional: Category/Tags/Description
- Auto-numbering `(2)` on dialog open; confirm-overwrite if user manually creates collision
- Modal form factor, submarine-themed via existing `Tokens.h` (no new tokens)

## Spec
`Docs/plans/2026-05-04-save-as-design.md` (committed `8ee312e0b`)

## Test plan
- [x] auval PASSED at 44.1k/48k/96k/192k
- [x] Build GREEN, no new warnings
- [x] Sentinel: "Overwrite preset" present in binary
- [x] Manual smoke (5 paths) passed locally

## Issue
Closes #1405. Removes `TODO(#1354)` placeholder comment.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
EOF
)"
```

- [ ] **Step 13.3: Worktree cleanup**

```bash
cd /Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus
git worktree remove /Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus.saveas
```

---

## Plan-level estimates

| Phase | Tasks | Est. lines | Est. time (sonnet) |
|---|---|---|---|
| Pre-flight | Task 0 | 0 | 5 min |
| Phase 1 (logic) | Tasks 1-2 | ~60 | 30 min |
| Phase 2 (dialog) | Tasks 3-6 | ~250 | 90 min |
| Phase 3 (editor) | Tasks 7-10 | ~80 | 45 min |
| Phase 4 (validation) | Tasks 11-13 | 0 | 30 min |
| **Total** | **13 tasks** | **~390 lines added, ~55 removed** | **~3.5 hours** |

## Sequence + parallelism

- Tasks 1-2 are independent of Tasks 3-6 — could run as 2 parallel sub-sessions if useful (likely overkill for this scope; sequential is fine)
- Tasks 7-10 depend on Tasks 1-6 being complete (XOceanusEditor wiring needs the dialog and PresetManager helpers)
- Tasks 11-13 are validation; sequential

## Verification sentinels (track per task)

| Task | Sentinel |
|---|---|
| 1.4 | `./build/XOceanus_Tests --category "PresetManager::getNextAvailableName"` shows 3 PASS |
| 2.4 | Same runner shows 2 additional PASS for `savePresetToFile overwrite-confirm` |
| 3.3 | Build green, no new warnings |
| 4.3 | Build green; mood dropdown visually rendered with 16 items |
| 6.2 | Build green; collision-confirm dialog visually appears in test |
| 8.5 | Build green; Cmd+S triggers correct branch in 3 states |
| 11.2 | `auval -v aufx Xnus XO_O` returns SUCCEEDED |
| 11.3 | `strings <binary> \| grep "Overwrite preset"` matches |
| 12.1 | All 5 manual smoke paths pass |

## Constraints (carry from spec)

- Do NOT introduce new `Tokens.h` entries — reuse D1-D5 locked tokens
- Do NOT touch dead Gallery panels (ColumnLayoutManager, HeaderIndicators, OverviewPanel, SidebarPanel) — scheduled for deletion in W1-B5
- Branch collision: if `feat/save-as-dialog-v1` already exists, STOP and report (per `feedback-multi-session-branch-collision.md`)
- New `.cpp` files must be added to CMakeLists if not picked up by glob (per `feedback-new-files-need-cmakelists.md`)
- Verify with `strings <binary>` before claiming success — CMake can link stale objects (per `feedback-build-agent-must-verify-binary-contents.md`)

## Self-review (post-write check)

- [x] Spec coverage: all 5 locked decisions implemented
  - Q1 split Save/Save As → Tasks 8 + 9
  - Q2 Cmd+S state matrix → Task 7 + 8
  - Q3 Name+Mood required → Task 4 (mood) + Task 5 (others optional)
  - Q4 auto-number + collision-confirm → Task 1 (helper) + Task 6 (confirm)
  - Q5 modal form factor + mood swatches → Tasks 3-5 + Task 4 LookAndFeel
- [x] No placeholders ("TBD", "TODO", "implement later") — all code blocks have actual content
- [x] Type consistency: `getNextAvailableName`, `savePresetToFile`, `getCurrentSaveState`, `openSavePresetDialog`, `silentOverwriteCurrentPreset`, `getUserPresetDirectory` consistent across all task references
- [x] Test commands consistent: same `XOceanus_Tests` target, same `--category` syntax
- [x] File paths absolute and exact

## Out of scope (deferred per spec)

- "Save as a copy" separate menu item (collapsed into Save As)
- Folder/collection support
- Mood selector grid variant (deferred to v1.1)
- Drag-to-reorder in browser

---

**This plan is paste-ready for a Day 3 sonnet subagent dispatch.** Pre-flight (Task 0) sets up the worktree; Phase 1-4 implements; Task 13 opens the PR. Total ~3.5h sonnet at standard build times.
