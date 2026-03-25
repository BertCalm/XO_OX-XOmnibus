# Outshine Phase 1A-Beta: UI Build

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task.

**Goal:** Build the Outshine JUCE UI — 10 components in `Source/UI/Outshine/` that wire to the hardened XOutshine backend, producing a daily-driver sample instrument forge.

**Architecture:** DocumentWindow (not CallOutBox), responsive layout (Mode A/B at 700px), Gallery Model design tokens. Launched from the EXPORT sidebar tab. All components follow `ExportDialog.h` patterns: GalleryColors, A11y::setup(), buildXXX() section builders.

**Tech Stack:** JUCE 8, C++17, GalleryColors.h, GalleryFonts, A11y

**Spec:** `docs/superpowers/specs/2026-03-25-outshine-phase1a-design.md` (Revision 2)
**Depends on:** Phase 1A-alpha complete (analyzeGrains/exportPearl API, multi-zone keygroups, YIN pitch detection)

---

## Prerequisites Checklist

Before any task in this plan begins, verify all Phase 1A-alpha items are complete:

- [ ] `XOutshine::analyzeGrains()` is a public method
- [ ] `XOutshine::exportPearl()` is a public method
- [ ] `XOutshine::getAnalyzedSamples()` returns `const std::vector<AnalyzedSample>&`
- [ ] `ExportFormat` enum exists (`XPNPack`, `WAVFolder`, `XPMOnly`)
- [ ] `MPEExpressionRoutes` struct exists
- [ ] YIN pitch detection implemented (not raw autocorrelation)
- [ ] Multi-zone midpoint rule implemented
- [ ] `KeyTrack=False` written for drum programs
- [ ] `SampleCategory` includes `Loop`, `Woodwind`, `Brass`, `Vocal`
- [ ] `XOriginate.h` circular include resolved

---

## File Structure

```
Source/UI/Outshine/    (NEW directory — all files)
├── OutshineDocumentWindow.h
├── OutshineMainComponent.h
├── OutshineSidebarPanel.h
├── OutshineShellState.h
├── OutshineInputPanel.h
├── OutshineFolderBrowser.h
├── OutshineGrainStrip.h
├── OutshineAutoMode.h
├── OutshineZoneMap.h
├── OutshineMPEPanel.h
├── OutshineExportBar.h
└── OutshinePreviewPlayer.h

Source/UI/XOlokunEditor.h  (MODIFY — wire sidebar panel)
```

---

## Design Token Reference

Use only these tokens from `Source/UI/GalleryColors.h`. Never hardcode hex values except where explicitly noted.

| Role | Token | Notes |
|---|---|---|
| Background | `GalleryColors::get(GalleryColors::shellWhite())` | Theme-aware |
| Header fill | `GalleryColors::get(GalleryColors::xoGold)` | Brand constant `#E9C46A` |
| Primary text | `GalleryColors::get(GalleryColors::textDark())` | Theme-aware |
| Secondary text | `GalleryColors::get(GalleryColors::textMid())` | Theme-aware |
| Disabled text | `juce::Colour(0xFF666666)` | Direct hex — documented exception |
| Borders | `GalleryColors::get(GalleryColors::borderGray())` | Theme-aware |
| Active accent | `GalleryColors::get(GalleryColors::xoGold)` | Gold accent |
| Error state | `juce::Colour(0xFFE05252)` | Direct hex — documented exception |
| Slot background | `GalleryColors::get(GalleryColors::slotBg())` | Theme-aware |
| Gold text (on white) | `GalleryColors::get(GalleryColors::xoGoldText())` | WCAG AA on shellWhite |

Typography tokens — all from `GalleryFonts` (declared in `GalleryLookAndFeel.h`):

| Role | Call |
|---|---|
| Window title | `GalleryFonts::display(14.0f)` |
| Section labels | `GalleryFonts::heading(9.0f)` |
| Body / labels | `GalleryFonts::body(12.0f)` |
| Data values, MIDI note names, durations | `GalleryFonts::value(11.0f)` (monospace) |

Spacing: 4px base unit, 16px outer window padding, 8px inner spacing, 4px chip gap.
Border radii: 4px (chips/badges), 6px (panels/inputs), 8px (main sections).

---

## Accessibility Pattern

Every interactive component must:

1. Call `A11y::setup(*this, "Accessible Name", "Description")` in its constructor
2. Override `paint()` to call `drawFocusRing(g, bounds)` when `hasKeyboardFocus(false)` is true
3. Set minimum touch/click target 32×32px for all interactive controls
4. Ensure all text passes WCAG AA contrast (4.5:1 normal text, 3:1 large text)

---

## ExportDialog Pattern Reference

All components follow the patterns established in `Source/UI/ExportDialog/ExportDialog.h`:

```cpp
class MyComponent : public juce::Component
{
public:
    MyComponent(/* dependencies */)
    {
        setWantsKeyboardFocus(true);
        A11y::setup(*this, "Name", "Description");

        buildSectionA();
        buildSectionB();

        setSize(width, height);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));

        // Header bar
        auto headerArea = getLocalBounds().removeFromTop(kHeaderH);
        g.setColour(GalleryColors::get(GalleryColors::xoGold));
        g.fillRect(headerArea);
        g.setColour(juce::Colour(GalleryColors::Light::textDark));
        g.setFont(GalleryFonts::display(14.0f));
        g.drawText("SECTION NAME", headerArea.reduced(12, 0), juce::Justification::centredLeft);

        // Section dividers
        g.setColour(GalleryColors::get(GalleryColors::borderGray()));
        // ... section divider lines
    }

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromTop(kHeaderH);
        // ... setBounds calls
    }

private:
    void buildSectionA() { /* addAndMakeVisible + configure child components */ }

    static constexpr int kHeaderH = 36;
    // ... child components
};
```

---

## Tasks

### Task 1: OutshineDocumentWindow

**File:** `Source/UI/Outshine/OutshineDocumentWindow.h`
**Depends on:** Task 2 (OutshineMainComponent)

**Purpose:** Top-level JUCE DocumentWindow. Free-floating, resizable, custom XO Gold title bar. Launched from the EXPORT sidebar tab. Hosts `OutshineMainComponent` as its content component.

**Implementation requirements:**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "OutshineMainComponent.h"

namespace xolokun {

class OutshineDocumentWindow : public juce::DocumentWindow
{
public:
    explicit OutshineDocumentWindow(XOlokunProcessor& processorRef)
        : juce::DocumentWindow("OUTSHINE",
                               juce::Colour(GalleryColors::Light::shellWhite),
                               juce::DocumentWindow::closeButton)
        , mainComponent(std::make_unique<OutshineMainComponent>(processorRef))
    {
        setUsingNativeTitleBar(false);           // Custom XO Gold title bar
        setResizable(true, true);
        setResizeLimits(700, 500, 1600, 1000);
        setContentOwned(mainComponent.release(), true);
        centreWithSize(900, 660);
    }

    void closeButtonPressed() override
    {
        if (auto* mc = dynamic_cast<OutshineMainComponent*>(getContentComponent()))
            mc->cancelPipeline();               // cancel any running pipeline
        setVisible(false);
        delete this;                            // owner (SafePointer in sidebar) nulls on delete
    }

    void paint(juce::Graphics& g) override
    {
        // Custom XO Gold title bar — no native chrome
        auto titleBar = getLocalBounds().removeFromTop(getTitleBarHeight());
        g.setColour(GalleryColors::get(GalleryColors::xoGold));
        g.fillRect(titleBar);
        g.setColour(juce::Colour(GalleryColors::Light::textDark));
        g.setFont(GalleryFonts::display(13.0f));
        g.drawText("OUTSHINE", titleBar.reduced(12, 0), juce::Justification::centredLeft);

        // Body background
        auto body = getLocalBounds().withTrimmedTop(getTitleBarHeight());
        g.setColour(GalleryColors::get(GalleryColors::shellWhite()));
        g.fillRect(body);
    }

private:
    std::unique_ptr<OutshineMainComponent> mainComponent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineDocumentWindow)
};

} // namespace xolokun
```

**Key constraints:**
- `setUsingNativeTitleBar(false)` — custom paint replaces native chrome
- `setResizable(true, true)` — both user-resize and resize via setSize() enabled
- `setResizeLimits(700, 500, 1600, 1000)` — spec minimum/maximum
- `centreWithSize(900, 660)` — default launch position
- Close: cancel pipeline, then `delete this` — the `SafePointer` in `OutshineSidebarPanel` nulls automatically on deletion
- Do NOT anchor to any button (DocumentWindow is free-floating)

**Keyboard:** Escape on the window closes it. Add `keyPressed` override in OutshineMainComponent (Task 2) to propagate Escape upward.

**Commit message:** `feat(outshine): add OutshineDocumentWindow — custom gold title bar, 900×660 default, 700×500 min`

---

### Task 2: OutshineMainComponent

**File:** `Source/UI/Outshine/OutshineMainComponent.h`
**Depends on:** Tasks 3–11 (all child panels)

**Purpose:** Content component inside the DocumentWindow. Owns state machine, backend reference, and responsive layout. Acts as the message hub between all child components.

**State machine:**

```cpp
enum class OutshineState { Shell, Input, Preview, Exporting };
```

| State | Condition | Layout |
|---|---|---|
| `Shell` | No grains loaded | Full width: ShellState centered |
| `Input` | Grains loading / analysis running | Full width: InputPanel |
| `Preview` | analyzeGrains() complete | Split: InputPanel (left 50%) + AutoMode (right 50%) |
| `Exporting` | exportPearl() running | Full width: ExportBar progress replaces left panel |

**Implementation requirements:**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../../Core/XOlokunProcessor.h"   // processor reference
#include "../../Export/XOutshine.h"
#include "OutshineShellState.h"
#include "OutshineInputPanel.h"
#include "OutshineGrainStrip.h"
#include "OutshineAutoMode.h"
#include "OutshineExportBar.h"

namespace xolokun {

class OutshineMainComponent : public juce::Component
{
public:
    explicit OutshineMainComponent(XOlokunProcessor& processorRef);
    ~OutshineMainComponent() override;

    // Called by OutshineDocumentWindow before delete
    void cancelPipeline();

    // Called by grain strip when grains change
    void onGrainsChanged(const juce::StringArray& grainPaths);

    // Called by export bar
    void onExportClicked(const juce::String& pearlName,
                         ExportFormat format,
                         const juce::File& outputPath);

    // Child access for progress callback
    OutshineExportBar& getExportBar() { return *exportBar; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    void transitionToState(OutshineState newState);
    void runAnalysis(const juce::StringArray& grainPaths);
    void runPipeline(const juce::String& pearlName,
                     ExportFormat format,
                     const juce::File& outputPath);

    XOlokunProcessor& processor;
    XOutshine outshine;
    OutshineSettings currentSettings;
    OutshineState uiState { OutshineState::Shell };

    std::unique_ptr<OutshineShellState>   shellState;
    std::unique_ptr<OutshineInputPanel>   inputPanel;
    std::unique_ptr<OutshineGrainStrip>   grainStrip;
    std::unique_ptr<OutshineAutoMode>     autoMode;
    std::unique_ptr<OutshineExportBar>    exportBar;

    juce::StringArray currentGrains;
    std::atomic<bool> cancelFlag { false };

    static constexpr int kGrainStripH  = 40;
    static constexpr int kExportBarH   = 60;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineMainComponent)
};

} // namespace xolokun
```

**`resized()` — Mode A/B responsive layout:**

```cpp
void OutshineMainComponent::resized()
{
    auto area = getLocalBounds();

    // Grain strip always pinned above export bar
    auto exportBarArea = area.removeFromBottom(kExportBarH);
    auto grainArea     = area.removeFromBottom(kGrainStripH);
    exportBar->setBounds(exportBarArea);
    grainStrip->setBounds(grainArea);

    // Remaining area split by state
    if (uiState == OutshineState::Shell)
    {
        shellState->setBounds(area);
        inputPanel->setBounds({});
        autoMode->setBounds({});
    }
    else if (uiState == OutshineState::Input)
    {
        inputPanel->setBounds(area);
        shellState->setBounds({});
        autoMode->setBounds({});
    }
    else  // Preview or Exporting
    {
        auto left  = area.removeFromLeft(area.getWidth() / 2);
        inputPanel->setBounds(left);
        autoMode->setBounds(area);
        shellState->setBounds({});
    }
}
```

**Progress callback wiring:**

```cpp
// In runAnalysis() — background thread safety via SafePointer + callAsync
auto callback = [safeThis = juce::Component::SafePointer<OutshineMainComponent>(this)]
    (OutshineProgress& p)
{
    if (p.cancelled) return;
    float prog  = p.overallProgress;
    juce::String stg = p.stage;
    juce::MessageManager::callAsync([safeThis, prog, stg]() {
        if (auto* w = safeThis.getComponent())
            w->getExportBar().setProgress(prog, stg);
    });
};
```

**Key behaviors:**
- `onGrainsChanged()` cancels any running analysis (`cancelFlag = true`), resets `cancelFlag = false`, launches new `juce::Thread`-based `analyzeGrains()` call
- After `analyzeGrains()` returns, post to message thread: call `autoMode->populate(outshine.getAnalyzedSamples())`, transition to `Preview` state, call `resized()`
- `cancelPipeline()` sets `cancelFlag = true` and joins the background thread before returning
- `keyPressed()`: Escape → find parent DocumentWindow and close

**Commit message:** `feat(outshine): add OutshineMainComponent — state machine, responsive layout, backend wiring`

---

### Task 3: OutshineShellState

**File:** `Source/UI/Outshine/OutshineShellState.h`
**Depends on:** GalleryColors.h only

**Purpose:** The Oyster empty state. Shown when no grains are loaded. Accepts drag-and-drop and notifies parent when files are dropped.

**Implementation requirements:**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"

namespace xolokun {

class OutshineShellState : public juce::Component,
                           public juce::FileDragAndDropTarget
{
public:
    // Callback: invoked when files are dropped onto the shell state
    std::function<void(const juce::StringArray&)> onFilesDropped;

    OutshineShellState()
    {
        setWantsKeyboardFocus(false);
        A11y::setup(*this, "Outshine Shell", "Drop WAV files, folders, or XPN archives to begin");
    }

    void paint(juce::Graphics& g) override;
    void resized() override {}

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }
    void fileDragEnter(const juce::StringArray&, int, int) override;
    void fileDragExit(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray& files, int, int) override;

private:
    bool dragActive { false };
    float borderOpacity { 0.0f };    // 0 = no glow, 1 = full gold glow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineShellState)
};

} // namespace xolokun
```

**`paint()` implementation:**

```cpp
void OutshineShellState::paint(juce::Graphics& g)
{
    g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));

    auto bounds = getLocalBounds().reduced(24);

    // Gold border glow on drag-enter
    if (dragActive)
    {
        g.setColour(GalleryColors::get(GalleryColors::xoGold).withAlpha(0.7f));
        g.drawRoundedRectangle(getLocalBounds().reduced(4).toFloat(), 8.0f, 2.0f);
    }
    else
    {
        g.setColour(GalleryColors::get(GalleryColors::borderGray()));
        g.drawRoundedRectangle(getLocalBounds().reduced(4).toFloat(), 8.0f, 1.0f);
    }

    // Oyster icon placeholder (64x64 centered)
    auto center = getLocalBounds().getCentre();
    juce::Rectangle<float> iconArea(center.x - 32.0f,
                                    center.y - 60.0f,
                                    64.0f, 64.0f);
    g.setColour(GalleryColors::get(GalleryColors::borderGray()));
    g.fillEllipse(iconArea);  // Placeholder until oyster asset is provided
    g.setColour(GalleryColors::get(GalleryColors::textMid()));
    g.setFont(GalleryFonts::value(22.0f));
    g.drawText(juce::String(juce::CharPointer_UTF8("\xf0\x9f\xaa\xb8")),
               iconArea.toNearestInt(), juce::Justification::centred);
    // TODO: replace with BinaryData::oyster_svg when asset is provided

    // Text labels
    g.setColour(GalleryColors::get(GalleryColors::textDark()));
    g.setFont(GalleryFonts::display(14.0f));
    g.drawText("The shell is open.",
               getLocalBounds().withY(center.y + 16).withHeight(22),
               juce::Justification::centred);

    g.setColour(GalleryColors::get(GalleryColors::textMid()));
    g.setFont(GalleryFonts::body(13.0f));
    g.drawText("Drop a grain to begin.",
               getLocalBounds().withY(center.y + 40).withHeight(18),
               juce::Justification::centred);

    g.setColour(juce::Colour(0xFF666666));
    g.setFont(GalleryFonts::body(11.0f));
    g.drawText("WAV files, folders, or XPN archives.",
               getLocalBounds().withY(center.y + 60).withHeight(16),
               juce::Justification::centred);
}
```

**Drag state transitions:**

```cpp
void OutshineShellState::fileDragEnter(const juce::StringArray&, int, int)
{
    dragActive = true;
    repaint();

    // Reduced motion check: skip animation if accessibility flag is set
    bool reducedMotion = juce::Desktop::getInstance().isScreenReaderEnabled();
    if (!reducedMotion)
    {
        // Icon pulse: scale 1.0 → 1.1 → 1.0 over 300ms
        // Implemented via juce::Animator or manual Timer in full code
    }
}

void OutshineShellState::fileDragExit(const juce::StringArray&)
{
    dragActive = false;
    repaint();
}

void OutshineShellState::filesDropped(const juce::StringArray& files, int, int)
{
    dragActive = false;
    repaint();
    if (onFilesDropped)
        onFilesDropped(files);
}
```

**Reduced motion:** Check `juce::Desktop::getInstance().isScreenReaderEnabled()` and skip all animations (pulse, glow fade). Instant transitions only.

**Commit message:** `feat(outshine): add OutshineShellState — oyster empty state, drag-drop, gold glow on enter`

---

### Task 4: OutshineInputPanel

**File:** `Source/UI/Outshine/OutshineInputPanel.h`
**Depends on:** Task 5 (OutshineFolderBrowser), Task 6 (OutshineGrainStrip)

**Purpose:** Responsive container for the input area. Mode A (>=700px): vertical split — drop zone left, folder browser right. Mode B (<700px): tabbed — Drop Zone / Browse Files / Recent Grains. The grain strip is NOT owned here — it is owned by OutshineMainComponent and sits below this panel.

**Implementation requirements:**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "OutshineFolderBrowser.h"

namespace xolokun {

class OutshineInputPanel : public juce::Component,
                           public juce::FileDragAndDropTarget,
                           private juce::TabbedComponent  // Mode B only — constructed lazily
{
public:
    // Callback: files ready to be added to grain strip
    std::function<void(const juce::StringArray&)> onFilesSelected;

    explicit OutshineInputPanel();
    ~OutshineInputPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // FileDragAndDropTarget (Mode A drop zone left side)
    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }
    void fileDragEnter(const juce::StringArray&, int, int) override;
    void fileDragExit(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray& files, int, int) override;

    // Public for OutshineMainComponent to populate Recent Grains tab
    void setRecentGrains(const juce::StringArray& recents);

private:
    void buildModeA();
    void buildModeB();
    void switchToMode(bool modeA);

    bool isModeA { true };
    bool dropZoneDragActive { false };

    // Mode A: separate left/right layout
    std::unique_ptr<OutshineFolderBrowser>   folderBrowser;
    juce::TextButton                          selectAllBtn { "Select All WAVs" };
    juce::TextButton                          addSelectedBtn { "Add Selected \xe2\x86\x92" };   // →

    // Mode B: tabbed layout
    std::unique_ptr<juce::TabbedComponent>   tabComponent;
    std::unique_ptr<OutshineFolderBrowser>   tabFolderBrowser;

    // Recent grains list (Mode A: dropdown, Mode B: tab)
    juce::StringArray recentGrains;

    static constexpr int kButtonH    = 28;
    static constexpr int kButtonBarH = 36;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineInputPanel)
};

} // namespace xolokun
```

**`resized()` — Mode A/B breakpoint:**

```cpp
void OutshineInputPanel::resized()
{
    const bool shouldBeModeA = (getWidth() >= 700);

    if (shouldBeModeA != isModeA)
        switchToMode(shouldBeModeA);

    if (isModeA)
    {
        // Left half: drop zone (painted directly)
        // Right half: folder browser + buttons
        auto area = getLocalBounds().reduced(8);
        auto left  = area.removeFromLeft(area.getWidth() / 2).reduced(4);
        auto right = area.reduced(4);

        // Drop zone is painted in paint() using dropZoneArea
        dropZoneArea = left;

        // Button bar at bottom of right panel
        auto btnBar = right.removeFromBottom(kButtonBarH);
        int btnW = btnBar.getWidth() / 2 - 4;
        selectAllBtn.setBounds(btnBar.removeFromLeft(btnW));
        btnBar.removeFromLeft(8);
        addSelectedBtn.setBounds(btnBar.removeFromLeft(btnW));

        folderBrowser->setBounds(right);
    }
    else
    {
        // Mode B: tabbed fills entire area
        tabComponent->setBounds(getLocalBounds());
    }
}
```

**Mode A drop zone paint:**

In `paint()`, draw the drop zone on the left half:
- Dashed border: 1px, `borderGray()`, 8px radius — use `g.drawDashedLine()` or a Path with dash pattern
- On `dropZoneDragActive`: border glow gold (`xoGold`), 2px
- Center text: "Drop grains here" in `GalleryFonts::body()` `textDark()`, then "WAV • Folders • XPN" in `GalleryFonts::body()` `textMid()`
- Store `dropZoneArea` as a member `juce::Rectangle<int>` set in `resized()`; use it in `filesDropped()` and drag enter/exit

**Mode B tab construction:**

```cpp
void OutshineInputPanel::buildModeB()
{
    tabComponent = std::make_unique<juce::TabbedComponent>(
        juce::TabbedButtonBar::TabsAtTop);
    tabComponent->setColour(juce::TabbedComponent::backgroundColourId,
                            GalleryColors::get(GalleryColors::shellWhite()));
    tabComponent->addTab("Drop Zone",    GalleryColors::get(GalleryColors::shellWhite()), nullptr, false);
    tabComponent->addTab("Browse Files", GalleryColors::get(GalleryColors::shellWhite()), tabFolderBrowser.get(), false);
    tabComponent->addTab("Recent Grains",GalleryColors::get(GalleryColors::shellWhite()), nullptr, false);
    addAndMakeVisible(*tabComponent);
}
```

**"Add Selected →" button handler:**

```cpp
addSelectedBtn.onClick = [this]() {
    juce::StringArray selected = folderBrowser->getSelectedFilePaths();
    if (selected.isEmpty()) return;
    if (onFilesSelected)
        onFilesSelected(selected);
};
```

**"Select All WAVs" button handler:**

```cpp
selectAllBtn.onClick = [this]() {
    folderBrowser->selectAllWavFiles();
};
```

**A11y:**
- `A11y::setup(selectAllBtn, "Select All WAVs", "Select all WAV files in the current folder")`
- `A11y::setup(addSelectedBtn, "Add Selected", "Add selected files to the grain strip")`
- Drop zone region: mark as `juce::AccessibilityRole::button` with description "Drop zone for WAV files, folders, or XPN archives"

**Commit message:** `feat(outshine): add OutshineInputPanel — Mode A/B responsive layout, drop zone, folder browser split`

---

### Task 5: OutshineFolderBrowser

**File:** `Source/UI/Outshine/OutshineFolderBrowser.h`
**Depends on:** GalleryColors.h, JUCE audio formats

**Purpose:** File system browser with path bar, multi-select, WAV duration display, and "Select All WAVs" / "Add Selected" actions. This is the highest-effort component.

**Implementation requirements:**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../GalleryColors.h"

namespace xolokun {

struct FolderEntry
{
    juce::File   file;
    bool         isDirectory { false };
    juce::String durationStr;   // e.g., "0:03.2" — populated lazily for WAV files
    bool         selected { false };
};

class OutshineFolderBrowser : public juce::Component,
                              private juce::ListBoxModel
{
public:
    // Called when user double-clicks a WAV or uses "Add Selected →"
    std::function<void(const juce::StringArray&)> onFilesConfirmed;

    explicit OutshineFolderBrowser();
    ~OutshineFolderBrowser() override;

    void setRootPath(const juce::File& directory);
    juce::StringArray getSelectedFilePaths() const;
    void selectAllWavFiles();
    void clearSelection();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // ListBoxModel
    int  getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool rowSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    juce::String getTooltipForRow(int row) override;

    void navigateTo(const juce::File& directory);
    void loadDirectory(const juce::File& directory);
    juce::String getFormattedDuration(const juce::File& wavFile);

    juce::File                     currentPath;
    std::vector<FolderEntry>       entries;
    juce::ListBox                  fileList;
    juce::Label                    pathLabel;
    juce::TextButton               backButton { "<" };
    juce::AudioFormatManager       formatManager;
    int                            lastClickedRow { -1 };  // for Shift+range select

    static constexpr int kPathBarH = 32;
    static constexpr int kRowH     = 28;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineFolderBrowser)
};

} // namespace xolokun
```

**Constructor:**

```cpp
OutshineFolderBrowser::OutshineFolderBrowser()
{
    formatManager.registerBasicFormats();

    setWantsKeyboardFocus(true);
    A11y::setup(*this, "File Browser", "Navigate folders and select WAV files");

    // Path bar
    pathLabel.setFont(GalleryFonts::value(11.0f));
    pathLabel.setColour(juce::Label::textColourId,
                        GalleryColors::get(GalleryColors::textDark()));
    addAndMakeVisible(pathLabel);

    backButton.setButtonText(juce::String(juce::CharPointer_UTF8("\xe2\x86\x90")));  // ←
    A11y::setup(backButton, "Back", "Navigate to parent directory");
    backButton.onClick = [this]() {
        if (currentPath.getParentDirectory() != currentPath)
            navigateTo(currentPath.getParentDirectory());
    };
    addAndMakeVisible(backButton);

    // File list
    fileList.setModel(this);
    fileList.setRowHeight(kRowH);
    fileList.setMultipleSelectionEnabled(true);
    fileList.setColour(juce::ListBox::backgroundColourId,
                       GalleryColors::get(GalleryColors::slotBg()));
    A11y::setup(fileList, "Files", "List of files and folders");
    addAndMakeVisible(fileList);

    // Start in user home directory
    navigateTo(juce::File::getSpecialLocation(juce::File::userHomeDirectory));
}
```

**`resized()`:**

```cpp
void OutshineFolderBrowser::resized()
{
    auto area = getLocalBounds();
    auto pathBar = area.removeFromTop(kPathBarH);
    backButton.setBounds(pathBar.removeFromLeft(kPathBarH).reduced(4));
    pathLabel.setBounds(pathBar.reduced(4, 4));
    fileList.setBounds(area);
}
```

**`loadDirectory()`:**

```cpp
void OutshineFolderBrowser::loadDirectory(const juce::File& directory)
{
    entries.clear();

    // Add parent entry if not root
    if (directory.getParentDirectory() != directory)
        entries.push_back({ directory.getParentDirectory(), true, "..", false });

    // Subdirectories first, then WAV files
    juce::RangedDirectoryIterator dirIt(directory, false, "*", juce::File::findDirectories);
    for (auto& entry : dirIt)
        entries.push_back({ entry.getFile(), true, {}, false });

    juce::RangedDirectoryIterator wavIt(directory, false, "*.wav;*.WAV", juce::File::findFiles);
    for (auto& entry : wavIt)
    {
        FolderEntry fe { entry.getFile(), false, {}, false };
        // Duration loaded lazily in paintListBoxItem to avoid blocking on large dirs
        entries.push_back(std::move(fe));
    }

    pathLabel.setText(directory.getFullPathName(), juce::dontSendNotification);
    fileList.deselectAllRows();
    fileList.updateContent();
}
```

**`paintListBoxItem()`:**

```cpp
void OutshineFolderBrowser::paintListBoxItem(int row, juce::Graphics& g,
                                              int w, int h, bool rowSelected)
{
    if (row < 0 || row >= (int)entries.size()) return;
    const auto& entry = entries[(size_t)row];

    // Row background
    if (rowSelected)
        g.fillAll(GalleryColors::get(GalleryColors::xoGold).withAlpha(0.18f));
    else
        g.fillAll(GalleryColors::get(GalleryColors::slotBg()));

    // Icon
    juce::String icon = entry.isDirectory ? juce::String(juce::CharPointer_UTF8("\xf0\x9f\x93\x81"))
                                          : juce::String(juce::CharPointer_UTF8("\xf0\x9f\x8e\xb5"));
    g.setFont(GalleryFonts::body(12.0f));
    g.setColour(GalleryColors::get(GalleryColors::textMid()));
    g.drawText(icon, 4, 0, 24, h, juce::Justification::centredLeft);

    // Name
    g.setColour(GalleryColors::get(GalleryColors::textDark()));
    g.setFont(GalleryFonts::body(12.0f));
    g.drawText(entry.file.getFileName(), 32, 0, w - 80, h,
               juce::Justification::centredLeft, true);

    // Duration (right-aligned, WAV files only)
    if (!entry.isDirectory)
    {
        // Load duration lazily if not yet cached
        if (entries[(size_t)row].durationStr.isEmpty())
            entries[(size_t)row].durationStr = getFormattedDuration(entry.file);

        g.setColour(GalleryColors::get(GalleryColors::textMid()));
        g.setFont(GalleryFonts::value(10.0f));
        g.drawText(entries[(size_t)row].durationStr,
                   w - 72, 0, 68, h, juce::Justification::centredRight);
    }

    // Focus ring
    if (fileList.isRowSelected(row) && fileList.hasKeyboardFocus(true))
    {
        g.setColour(GalleryColors::get(GalleryColors::xoGold));
        g.drawRect(0, 0, w, h, 1);
    }
}
```

**Multi-select:**

```cpp
void OutshineFolderBrowser::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (row < 0 || row >= (int)entries.size()) return;
    const auto& entry = entries[(size_t)row];

    if (entry.isDirectory) return;  // directories navigate on double-click only

    if (e.mods.isShiftDown() && lastClickedRow >= 0)
    {
        // Range select: Shift+click
        int lo = juce::jmin(row, lastClickedRow);
        int hi = juce::jmax(row, lastClickedRow);
        fileList.deselectAllRows();
        for (int i = lo; i <= hi; ++i)
            if (!entries[(size_t)i].isDirectory)
                fileList.selectRow(i, true, false);
    }
    else if (e.mods.isCommandDown())
    {
        // Individual toggle: Cmd+click
        if (fileList.isRowSelected(row))
            fileList.deselectRow(row);
        else
            fileList.selectRow(row, true, false);
    }
    else
    {
        // Single select
        fileList.selectRow(row, false, true);
    }
    lastClickedRow = row;
}

void OutshineFolderBrowser::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row < 0 || row >= (int)entries.size()) return;
    const auto& entry = entries[(size_t)row];

    if (entry.isDirectory)
        navigateTo(entry.file);
    else if (onFilesConfirmed)
        onFilesConfirmed({ entry.file.getFullPathName() });
}
```

**`getSelectedFilePaths()`:**

```cpp
juce::StringArray OutshineFolderBrowser::getSelectedFilePaths() const
{
    juce::StringArray result;
    auto selected = fileList.getSelectedRows();
    for (int i = 0; i < selected.size(); ++i)
    {
        int row = selected[i];
        if (row >= 0 && row < (int)entries.size() && !entries[(size_t)row].isDirectory)
            result.add(entries[(size_t)row].file.getFullPathName());
    }
    return result;
}
```

**`selectAllWavFiles()`:**

```cpp
void OutshineFolderBrowser::selectAllWavFiles()
{
    fileList.deselectAllRows();
    for (int i = 0; i < (int)entries.size(); ++i)
        if (!entries[(size_t)i].isDirectory)
            fileList.selectRow(i, true, false);
}
```

**`getFormattedDuration()`:**

```cpp
juce::String OutshineFolderBrowser::getFormattedDuration(const juce::File& wavFile)
{
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(wavFile));
    if (!reader) return "--:--";

    double durationS = (double)reader->lengthInSamples / reader->sampleRate;
    int mins = (int)(durationS / 60.0);
    double secs = durationS - mins * 60.0;
    return juce::String(mins) + ":" + juce::String(secs, 1).paddedLeft('0', 4);
}
```

**Keyboard navigation:**
- Arrow keys in `fileList`: handled by JUCE ListBox natively
- Space: toggle selection of focused row
- Enter: navigate into directory, or confirm single WAV selection

**A11y:**
- `A11y::setup(fileList, "File list", "List of WAV files and subdirectories")`
- Each row: set accessible description to filename + duration in `getAccessibleName()`

**Commit message:** `feat(outshine): add OutshineFolderBrowser — path bar, multi-select, WAV duration, Shift/Cmd click`

---

### Task 6: OutshineGrainStrip

**File:** `Source/UI/Outshine/OutshineGrainStrip.h`
**Depends on:** GalleryColors.h

**Purpose:** Horizontal strip of removable "chip" components representing loaded grains. Spans full window width, always visible. Notifies parent when grain list changes so re-analysis can be triggered.

**Implementation requirements:**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"

namespace xolokun {

class OutshineGrainStrip : public juce::Component
{
public:
    // Invoked whenever the grain list changes (add or remove)
    std::function<void(const juce::StringArray&)> onGrainsChanged;

    OutshineGrainStrip()
    {
        setWantsKeyboardFocus(true);
        A11y::setup(*this, "Grain Strip", "List of loaded audio grains");
        addAndMakeVisible(scrollContainer);
        addAndMakeVisible(countLabel);
        countLabel.setFont(GalleryFonts::value(11.0f));
        countLabel.setColour(juce::Label::textColourId,
                             GalleryColors::get(GalleryColors::textMid()));
        countLabel.setJustificationType(juce::Justification::centredRight);
    }

    void addGrains(const juce::StringArray& paths);
    void removeGrain(int index);
    void clear();
    juce::StringArray getGrainPaths() const { return grainPaths; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    void rebuildChips();
    void notifyChanged();
    int  getFocusedChipIndex() const { return focusedChipIndex; }

    juce::StringArray grainPaths;
    juce::Viewport    scrollContainer;
    juce::Component   chipHolder;          // inside the viewport
    juce::Label       countLabel;
    int               focusedChipIndex { -1 };

    static constexpr int kChipH      = 28;
    static constexpr int kChipGap    = 4;
    static constexpr int kChipPadX   = 8;
    static constexpr int kCountW     = 72;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineGrainStrip)
};

} // namespace xolokun
```

**`rebuildChips()`:**

```cpp
void OutshineGrainStrip::rebuildChips()
{
    chipHolder.removeAllChildren();

    int x = kChipGap;
    for (int i = 0; i < grainPaths.size(); ++i)
    {
        juce::String shortName = juce::File(grainPaths[i]).getFileNameWithoutExtension();
        if (shortName.length() > 20)
            shortName = shortName.substring(0, 18) + "…";

        // Chip background button
        auto* chip = new juce::TextButton(shortName + "  \xc3\x97");  // × (UTF-8)
        chip->setSize(GalleryFonts::body(12.0f).getStringWidth(shortName) + 48, kChipH);
        chip->setTopLeftPosition(x, (getHeight() - kChipH) / 2);
        chip->setColour(juce::TextButton::buttonColourId,
                        GalleryColors::get(GalleryColors::borderGray()));
        chip->setColour(juce::TextButton::textColourOffId,
                        GalleryColors::get(GalleryColors::textDark()));

        int capturedIndex = i;
        chip->onClick = [this, capturedIndex]() {
            removeGrain(capturedIndex);
        };

        A11y::setup(*chip,
                    "Remove " + juce::File(grainPaths[i]).getFileName(),
                    "Remove this grain from the strip");

        chipHolder.addAndMakeVisible(chip);
        x += chip->getWidth() + kChipGap;
    }

    chipHolder.setSize(juce::jmax(x, scrollContainer.getWidth()), getHeight());
    scrollContainer.setViewedComponent(&chipHolder, false);

    // Update count label
    countLabel.setText(juce::String(grainPaths.size()) + " grain" +
                       (grainPaths.size() == 1 ? "" : "s"),
                       juce::dontSendNotification);

    repaint();
}
```

**`addGrains()`:** De-duplicate by path before appending; call `rebuildChips()`, then `notifyChanged()`.

**`removeGrain()`:** Remove from `grainPaths`, call `rebuildChips()`, then `notifyChanged()`. If `grainPaths` becomes empty, parent transitions to Shell state via `onGrainsChanged({})`.

**`notifyChanged()`:**

```cpp
void OutshineGrainStrip::notifyChanged()
{
    if (onGrainsChanged)
        onGrainsChanged(grainPaths);
}
```

**`resized()`:**

```cpp
void OutshineGrainStrip::resized()
{
    auto area = getLocalBounds().reduced(4, 4);
    countLabel.setBounds(area.removeFromRight(kCountW));
    scrollContainer.setBounds(area);
    rebuildChips();
}
```

**`keyPressed()`:** Arrow keys move `focusedChipIndex`; Delete/Backspace removes focused chip.

**Paint:** Draw a top border line in `borderGray()` to visually separate from the panels above. Background: `shellWhite()`.

**State:** `grainPaths` persists across Mode A/B resize — chips are never lost on layout change.

**Commit message:** `feat(outshine): add OutshineGrainStrip — chip strip, horizontal scroll, remove on click, keyboard nav`

---

### Task 7: OutshineAutoMode

**File:** `Source/UI/Outshine/OutshineAutoMode.h`
**Depends on:** Task 8 (OutshineZoneMap), Task 9 (OutshineMPEPanel)

**Purpose:** Right panel container. Shows classification summary, zone map, MPE panel, and Rebirth Mode teaser. Populated from `analyzeGrains()` results after analysis completes.

**Implementation requirements:**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "OutshineZoneMap.h"
#include "OutshineMPEPanel.h"
#include "../../Export/XOutshine.h"   // AnalyzedSample, UpgradedProgram

namespace xolokun {

class OutshineAutoMode : public juce::Component
{
public:
    OutshineAutoMode();

    // Called by OutshineMainComponent after analyzeGrains() completes
    void populate(const std::vector<AnalyzedSample>& samples);

    // Child access for export bar
    OutshineZoneMap&   getZoneMap()  { return *zoneMap; }
    OutshineMPEPanel&  getMPEPanel() { return *mpePanel; }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void buildSummaryGrid();
    void buildRebirthTeaser();

    // Classification summary labels
    juce::Label typeLabel,   typeValue;
    juce::Label zonesLabel,  zonesValue;
    juce::Label velLabel,    velValue;
    juce::Label rrLabel,     rrValue;

    std::unique_ptr<OutshineZoneMap>   zoneMap;
    std::unique_ptr<OutshineMPEPanel>  mpePanel;
    juce::Label                        rebirthTeaser;

    static constexpr int kSummaryH  = 60;
    static constexpr int kZoneMapH  = 64;
    static constexpr int kMPEH      = 120;
    static constexpr int kTeaserH   = 60;
    static constexpr int kPad       = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineAutoMode)
};

} // namespace xolokun
```

**`buildSummaryGrid()`:** 4 pairs of labels (left-aligned label, right-aligned value). Font: `GalleryFonts::body(12.0f)` for labels, `GalleryFonts::value(12.0f)` for values.

**`populate()`:**

```cpp
void OutshineAutoMode::populate(const std::vector<AnalyzedSample>& samples)
{
    if (samples.empty()) return;

    // Determine program type from first sample category
    bool isDrum = (samples[0].category == SampleCategory::Kick ||
                   samples[0].category == SampleCategory::Snare  ||
                   /* ... other drum categories ... */);

    typeValue.setText(isDrum ? "Drum" : "Keygroup", juce::dontSendNotification);
    zonesValue.setText(juce::String((int)samples.size()), juce::dontSendNotification);
    velValue.setText("4", juce::dontSendNotification);    // from OutshineSettings
    rrValue.setText("2", juce::dontSendNotification);     // from OutshineSettings

    // Populate zone map and MPE panel
    // ZoneMap receives the analyzed samples directly; builds zone boundaries from root notes
    zoneMap->setSamples(samples);

    // MPE panel: category-aware defaults
    mpePanel->setCategoryDefaults(isDrum ? MPECategory::Drum : MPECategory::Melodic);
}
```

**Rebirth Mode teaser:**

```cpp
void OutshineAutoMode::buildRebirthTeaser()
{
    rebirthTeaser.setText("Rebirth Mode \xe2\x80\x94 Phase 1B\n"
                          "Route grains through OPERA \xc2\xb7 OBRIX \xc2\xb7 ONSET",
                          juce::dontSendNotification);
    rebirthTeaser.setFont(GalleryFonts::body(11.0f));
    rebirthTeaser.setColour(juce::Label::textColourId,
                            GalleryColors::get(GalleryColors::textMid()));
    rebirthTeaser.setAlpha(0.35f);    // Spec: 35% opacity, not interactive
    addAndMakeVisible(rebirthTeaser);

    // Click on teaser → toast "Rebirth Mode arrives in Phase 1B"
    // Implemented via MouseListener on the teaser component
}
```

**`resized()`:**

```cpp
void OutshineAutoMode::resized()
{
    auto area = getLocalBounds().reduced(kPad);

    auto summaryArea = area.removeFromTop(kSummaryH);
    // 4 label/value pairs in a 2-column grid
    int rowH = summaryArea.getHeight() / 4;
    int col1W = summaryArea.getWidth() / 2;
    // ... setBounds for each label pair

    area.removeFromTop(8);
    zoneMap->setBounds(area.removeFromTop(kZoneMapH));
    area.removeFromTop(8);
    mpePanel->setBounds(area.removeFromTop(kMPEH));
    area.removeFromTop(8);
    rebirthTeaser.setBounds(area.removeFromTop(kTeaserH));
}
```

**"Browse Files" placeholder:** A dashed-border rectangle at 20% opacity below zone map. Drawn in `paint()`. Label: "Spectrum View — Phase 1B". Not interactive.

**Commit message:** `feat(outshine): add OutshineAutoMode — classification summary, zone/MPE containers, Rebirth teaser`

---

### Task 8: OutshineZoneMap

**File:** `Source/UI/Outshine/OutshineZoneMap.h`
**Depends on:** GalleryColors.h, XOutshine.h (AnalyzedSample)

**Purpose:** Proportional keyboard visualization (C0–C8 = MIDI 0–108). Shows zone shading, root note labels, and amber warning icons for low-confidence pitch detections. Click on a zone to scroll the grain strip to the source sample.

**Implementation requirements:**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../../Export/XOutshine.h"   // AnalyzedSample

namespace xolokun {

class OutshineZoneMap : public juce::Component,
                        public juce::TooltipClient
{
public:
    // Callback: zone clicked → grain strip should scroll to this source index
    std::function<void(int grainIndex)> onZoneClicked;

    OutshineZoneMap()
    {
        setWantsKeyboardFocus(false);
        A11y::setup(*this, "Zone Map", "Keyboard zone visualization showing sample mapping");
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void setSamples(const std::vector<AnalyzedSample>& samples);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    juce::String getTooltip() override;

private:
    struct ZoneEntry
    {
        int    rootMidi;
        int    lowMidi;
        int    highMidi;
        int    grainIndex;
        bool   pitchUnverified;   // pitchConfidence < 0.15
        juce::String noteName;
    };

    float midiToX(int midiNote) const;
    static juce::String midiToNoteName(int midi);
    int   xToMidi(float x) const;
    int   hitZone(float x) const;

    std::vector<ZoneEntry> zones;
    int                    hoveredZone { -1 };

    static constexpr int   kMidiLow  = 0;    // C-2 (MIDI 0)
    static constexpr int   kMidiHigh = 108;  // C8 (MIDI 108)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineZoneMap)
};

} // namespace xolokun
```

**`setSamples()`:** Compute zone boundaries using the midpoint rule (see spec Section 6, Stage 6). Sort samples by `detectedMidiNote`. For each sample: `lowMidi = midpoint(root[i-1], root[i])` rounded down; `highMidi = midpoint(root[i], root[i+1]) - 1`. Edge zones: low boundary of first zone is `kMidiLow`, high boundary of last zone is `kMidiHigh`.

**`paint()`:**

```cpp
void OutshineZoneMap::paint(juce::Graphics& g)
{
    g.fillAll(GalleryColors::get(GalleryColors::slotBg()));

    auto bounds = getLocalBounds().toFloat();

    // Draw zone fills
    for (int i = 0; i < (int)zones.size(); ++i)
    {
        const auto& z = zones[(size_t)i];
        float x1 = midiToX(z.lowMidi);
        float x2 = midiToX(z.highMidi + 1);
        juce::Rectangle<float> zoneRect(x1, 0, x2 - x1, bounds.getHeight() - 18.0f);

        // Zone fill: xoGold at 30% opacity
        g.setColour(GalleryColors::get(GalleryColors::xoGold).withAlpha(0.30f));
        g.fillRect(zoneRect);

        // Zone border
        g.setColour(GalleryColors::get(GalleryColors::borderGray()));
        g.drawVerticalLine((int)x1, 0.0f, bounds.getHeight() - 18.0f);

        // Root note label centered in zone
        g.setColour(GalleryColors::get(GalleryColors::textDark()));
        g.setFont(GalleryFonts::value(9.0f));
        g.drawText(z.noteName,
                   (int)((x1 + x2) / 2.0f - 16), (int)(bounds.getHeight() - 16), 32, 14,
                   juce::Justification::centred);

        // Amber warning icon for unverified pitch
        if (z.pitchUnverified)
        {
            g.setColour(juce::Colour(0xFFE9A84A));  // amber
            g.setFont(GalleryFonts::body(10.0f));
            g.drawText("!", (int)x1 + 2, 2, 12, 14, juce::Justification::centredLeft);
        }
    }

    // White/black key pattern (subtle, 5% opacity overlay)
    g.setColour(GalleryColors::get(GalleryColors::textDark()).withAlpha(0.05f));
    for (int midi = kMidiLow; midi <= kMidiHigh; ++midi)
    {
        bool isBlack = (midi % 12 == 1 || midi % 12 == 3 || midi % 12 == 6 ||
                        midi % 12 == 8 || midi % 12 == 10);
        if (isBlack)
        {
            float x = midiToX(midi);
            float w = midiToX(midi + 1) - x;
            g.fillRect(x, 0.0f, w, (bounds.getHeight() - 18.0f) * 0.6f);
        }
    }

    // Focus ring
    if (hasKeyboardFocus(false))
    {
        g.setColour(GalleryColors::get(GalleryColors::xoGold));
        g.drawRect(getLocalBounds(), 1);
    }
}
```

**`midiToX()`:** Linear mapping: `(midiNote - kMidiLow) / (kMidiHigh - kMidiLow + 1) * getWidth()`.

**`mouseDown()`:** Identify zone via `hitZone(e.x)`. If valid, call `onZoneClicked(zones[i].grainIndex)`.

**`getTooltip()`:** Return zone range and root note for the zone under the cursor, e.g., `"A#2 – D#3 · Root: C3 · 4 layers"`.

**`midiToNoteName()`:** Return note names like "C3", "F#4" using standard MIDI convention (middle C = C3 = MIDI 60 in this display).

**Single-sample edge case:** If `zones.size() == 1`, the single zone spans the full keyboard width. Show a centered label with the root note.

**Commit message:** `feat(outshine): add OutshineZoneMap — keyboard zone visualization, root labels, unverified amber badges`

---

### Task 9: OutshineMPEPanel

**File:** `Source/UI/Outshine/OutshineMPEPanel.h`
**Depends on:** GalleryColors.h

**Purpose:** 4 expression route rows showing MPE expression mapping with mini progress bars. CC74 row annotated with "(manual config)" because it cannot be embedded in XPM. Rows animate during MIDI input (hooks only in Phase 1A).

**Implementation requirements:**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"

namespace xolokun {

enum class MPECategory { Melodic, Drum };

struct MPERoute
{
    juce::String dimensionName;    // "Slide (CC74)", "Pressure", "Pitch Bend", "Velocity", "ModWheel"
    juce::String destination;      // "Filter Cutoff", "Filter Resonance", "±24 semitones", etc.
    float        amount { 0.0f };  // 0.0–1.0 — displayed as mini progress bar
    bool         active { true };
    bool         manualConfigOnly { false };   // true for CC74 — not embeddable in XPM
    float        liveValue { 0.0f };           // 0.0–1.0 — updated by MIDI callback in Phase 1B
};

class OutshineMPEPanel : public juce::Component
{
public:
    OutshineMPEPanel()
    {
        setWantsKeyboardFocus(false);
        A11y::setup(*this, "MPE Expression Panel", "MPE expression route configuration");
    }

    void setCategoryDefaults(MPECategory category);

    // Phase 1B hooks — called from MIDI callback on message thread
    void setLiveValue(int routeIndex, float value);

    void paint(juce::Graphics& g) override;
    void resized() override {}

private:
    void paintRoute(juce::Graphics& g, const MPERoute& route,
                    juce::Rectangle<int> rowBounds) const;

    std::vector<MPERoute> routes;

    static constexpr int kRowH   = 24;
    static constexpr int kPad    = 8;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineMPEPanel)
};

} // namespace xolokun
```

**Melodic defaults:**

```cpp
void OutshineMPEPanel::setCategoryDefaults(MPECategory category)
{
    routes.clear();
    if (category == MPECategory::Melodic)
    {
        routes.push_back({ "Slide (CC74)", "Filter Cutoff",      0.80f, true,  true  });
        routes.push_back({ "Pressure",     "Filter Resonance",   0.40f, true,  false });
        routes.push_back({ "Pitch Bend",   "\xc2\xb124 semitones", 1.0f, true,  false });
        routes.push_back({ "Velocity",     "Amplitude + Bright", 1.0f,  true,  false });
        routes.push_back({ "ModWheel",     "Filter Cutoff",      0.50f, true,  false });
    }
    else  // Drum
    {
        routes.push_back({ "Slide (CC74)", "Pitch \xc2\xb12 semi", 0.0f, true, true  });
        routes.push_back({ "Pressure",     "Choke Speed",        0.40f, true,  false });
        routes.push_back({ "Pitch Bend",   "(omitted)",          0.0f,  false, false });
        routes.push_back({ "Velocity",     "Amplitude + Drive",  1.0f,  true,  false });
        routes.push_back({ "ModWheel",     "(omitted)",          0.0f,  false, false });
    }
    repaint();
}
```

**`paintRoute()`:**

```cpp
void OutshineMPEPanel::paintRoute(juce::Graphics& g, const MPERoute& route,
                                   juce::Rectangle<int> rowBounds) const
{
    if (!route.active)
    {
        g.setColour(GalleryColors::get(GalleryColors::textMid()).withAlpha(0.4f));
        // Draw dimmed row
    }

    // Gold left border for active routes
    if (route.active && route.amount > 0.0f)
    {
        g.setColour(GalleryColors::get(GalleryColors::xoGold));
        g.fillRect(rowBounds.removeFromLeft(2));
    }
    else
    {
        rowBounds.removeFromLeft(2);   // padding only
    }

    rowBounds.reduce(4, 2);

    // Dimension name
    g.setColour(route.active ? GalleryColors::get(GalleryColors::textDark())
                             : GalleryColors::get(GalleryColors::textMid()).withAlpha(0.4f));
    g.setFont(GalleryFonts::body(11.0f));
    int nameW = 110;
    g.drawText(route.dimensionName, rowBounds.removeFromLeft(nameW),
               juce::Justification::centredLeft);

    // Arrow
    g.setColour(GalleryColors::get(GalleryColors::textMid()));
    g.drawText("\xe2\x86\x92", rowBounds.removeFromLeft(16),  // →
               juce::Justification::centred);

    // Destination label
    int destW = 120;
    g.setColour(route.active ? GalleryColors::get(GalleryColors::textDark())
                             : GalleryColors::get(GalleryColors::textMid()).withAlpha(0.4f));
    g.drawText(route.destination, rowBounds.removeFromLeft(destW),
               juce::Justification::centredLeft);

    // Mini progress bar
    if (route.active && route.amount > 0.0f)
    {
        auto barBounds = rowBounds.removeFromLeft(64).reduced(0, 6);
        g.setColour(GalleryColors::get(GalleryColors::borderGray()));
        g.fillRoundedRectangle(barBounds.toFloat(), 3.0f);
        g.setColour(GalleryColors::get(GalleryColors::xoGold));
        g.fillRoundedRectangle(barBounds.toFloat().withWidth(barBounds.getWidth() * route.amount), 3.0f);
    }

    // "(manual config)" annotation — amber, only for CC74
    if (route.manualConfigOnly)
    {
        g.setColour(juce::Colour(0xFFE9A84A));  // amber
        g.setFont(GalleryFonts::body(9.0f));
        g.drawText("(manual config)", rowBounds, juce::Justification::centredLeft);
    }
}
```

**`paint()`:** Call `paintRoute()` for each route in sequence, each row is `kRowH` pixels tall with a 1px `borderGray()` separator between rows.

**Phase 1B live value hooks:** `setLiveValue(int routeIndex, float value)` updates `routes[routeIndex].liveValue` and calls `repaint()`. During preview playback, the Pressure bar pulses and ModWheel bar responds. In Phase 1A, these hooks exist but are never called.

**Commit message:** `feat(outshine): add OutshineMPEPanel — 5 expression rows, gold borders, manual-config annotation`

---

### Task 10: OutshineExportBar

**File:** `Source/UI/Outshine/OutshineExportBar.h`
**Depends on:** GalleryColors.h, XOutshine.h (ExportFormat)

**Purpose:** Pearl name field, export format selector, "Pearl & Export" gold CTA button, 3px progress bar, dual-line progress strings (metaphor + technical), time estimate, cancel button. Pinned at the bottom of the window, full width.

**Implementation requirements:**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../../Export/XOutshine.h"   // ExportFormat enum

namespace xolokun {

class OutshineExportBar : public juce::Component
{
public:
    // Callbacks
    std::function<void(const juce::String&, ExportFormat, const juce::File&)> onExportClicked;
    std::function<void()> onCancelClicked;

    OutshineExportBar();
    ~OutshineExportBar() override = default;

    // Called from OutshineMainComponent on message thread
    void setProgress(float progress, const juce::String& stageString);
    void setReadyToExport(bool ready);
    void setUnverifiedCount(int count);      // badge on export button
    void setExporting(bool exporting);
    void setGrainCount(int n);               // for time estimate

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void buildControls();
    void onExportButtonClicked();
    juce::String formatTimeEstimate(int grainCount) const;

    juce::TextEditor  pearlNameField;
    juce::ComboBox    formatSelector;
    juce::TextButton  exportBtn { "Pearl & Export" };
    juce::TextButton  cancelBtn { "Cancel" };

    juce::Label       progressMetaphorLabel;   // "Pearl forming..."
    juce::Label       progressTechLabel;       // "Stage 6/8: Building XPM programs"
    juce::Label       unverifiedBadge;

    float             currentProgress { 0.0f };
    bool              isExporting { false };
    bool              readyToExport { false };
    int               grainCount { 0 };
    int               unverifiedCount { 0 };

    static constexpr int kBarH        = 3;
    static constexpr int kButtonH     = 32;
    static constexpr int kNameFieldW  = 200;
    static constexpr int kFormatW     = 140;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineExportBar)
};

} // namespace xolokun
```

**Constructor:**

```cpp
OutshineExportBar::OutshineExportBar()
{
    setWantsKeyboardFocus(false);
    A11y::setup(*this, "Export Bar", "Pearl name, export format, and export controls");
    buildControls();
    setSize(900, 60);
}
```

**`buildControls()`:**

```cpp
void OutshineExportBar::buildControls()
{
    // Pearl name field
    pearlNameField.setFont(GalleryFonts::body(12.0f));
    pearlNameField.setTextToShowWhenEmpty("Pearl Name", GalleryColors::get(GalleryColors::textMid()));
    pearlNameField.setInputRestrictions(64, {});
    A11y::setup(pearlNameField, "Pearl Name", "Name for the exported instrument");
    addAndMakeVisible(pearlNameField);

    // Format selector
    formatSelector.addItem("XPN Pack",    1);
    formatSelector.addItem("WAV Folder",  2);
    formatSelector.addItem("XPM Only",    3);
    formatSelector.setSelectedId(1, juce::dontSendNotification);
    A11y::setup(formatSelector, "Export Format", "Choose XPN Pack, WAV Folder, or XPM Only");
    addAndMakeVisible(formatSelector);

    // Export button — XO Gold background
    exportBtn.setColour(juce::TextButton::buttonColourId,
                        GalleryColors::get(GalleryColors::xoGold));
    exportBtn.setColour(juce::TextButton::textColourOffId,
                        juce::Colour(GalleryColors::Light::textDark));
    exportBtn.onClick = [this]() { onExportButtonClicked(); };
    A11y::setup(exportBtn, "Pearl and Export", "Run the full pipeline and export the instrument");
    addAndMakeVisible(exportBtn);

    // Cancel button — hidden until exporting
    cancelBtn.setVisible(false);
    cancelBtn.onClick = [this]() { if (onCancelClicked) onCancelClicked(); };
    A11y::setup(cancelBtn, "Cancel", "Cancel the export pipeline");
    addAndMakeVisible(cancelBtn);

    // Progress labels
    progressMetaphorLabel.setFont(GalleryFonts::body(11.0f));
    progressMetaphorLabel.setColour(juce::Label::textColourId,
                                    GalleryColors::get(GalleryColors::textDark()));
    addAndMakeVisible(progressMetaphorLabel);

    progressTechLabel.setFont(GalleryFonts::value(10.0f));
    progressTechLabel.setColour(juce::Label::textColourId,
                                GalleryColors::get(GalleryColors::textMid()));
    addAndMakeVisible(progressTechLabel);

    // Unverified badge
    unverifiedBadge.setFont(GalleryFonts::value(9.0f));
    unverifiedBadge.setColour(juce::Label::textColourId, juce::Colour(0xFFE9A84A));
    unverifiedBadge.setVisible(false);
    addAndMakeVisible(unverifiedBadge);
}
```

**`setProgress()`:**

```cpp
void OutshineExportBar::setProgress(float progress, const juce::String& stageString)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    currentProgress = progress;

    // Metaphor headline: derive from progress range
    juce::String metaphor;
    if (progress < 0.15f)      metaphor = "Opening the grain...";
    else if (progress < 0.35f) metaphor = "Layering nacre...";
    else if (progress < 0.75f) metaphor = "Pearl forming...";
    else if (progress < 0.95f) metaphor = "Packaging...";
    else                        metaphor = "Pearl complete.";

    progressMetaphorLabel.setText(metaphor, juce::dontSendNotification);
    progressTechLabel.setText(stageString, juce::dontSendNotification);
    repaint();
}
```

**`setUnverifiedCount()`:**

```cpp
void OutshineExportBar::setUnverifiedCount(int count)
{
    unverifiedCount = count;
    bool hasUnverified = (count > 0);
    unverifiedBadge.setVisible(hasUnverified);
    if (hasUnverified)
        unverifiedBadge.setText(juce::String(count) + " unverified root" +
                                (count == 1 ? "" : "s"), juce::dontSendNotification);
    exportBtn.setEnabled(readyToExport && !hasUnverified);
    exportBtn.setAlpha(exportBtn.isEnabled() ? 1.0f : 0.4f);
}
```

**`paint()` — progress bar:**

```cpp
void OutshineExportBar::paint(juce::Graphics& g)
{
    g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));

    // Top border
    g.setColour(GalleryColors::get(GalleryColors::borderGray()));
    g.drawLine(0, 0, (float)getWidth(), 0, 1.0f);

    // 3px progress bar at absolute bottom, full width
    auto barArea = getLocalBounds().removeFromBottom(kBarH);
    g.setColour(GalleryColors::get(GalleryColors::borderGray()).withAlpha(0.5f));
    g.fillRect(barArea);
    if (currentProgress > 0.0f)
    {
        juce::Colour barColor = (currentProgress > 0.99f || !isExporting)
            ? GalleryColors::get(GalleryColors::xoGold)
            : GalleryColors::get(GalleryColors::xoGold);
        // Error state: bar turns red — set via separate setExportFailed() method
        g.setColour(barColor);
        g.fillRect(barArea.withWidth((int)(barArea.getWidth() * currentProgress)));
    }
}
```

**`resized()`:**

```cpp
void OutshineExportBar::resized()
{
    auto area = getLocalBounds().reduced(8, 0);
    area.removeFromBottom(kBarH);   // progress bar at very bottom

    auto controlRow = area.removeFromTop(kButtonH + 8).withTrimmedTop(4);

    pearlNameField.setBounds(controlRow.removeFromLeft(kNameFieldW).reduced(0, 2));
    controlRow.removeFromLeft(8);
    formatSelector.setBounds(controlRow.removeFromLeft(kFormatW).reduced(0, 2));
    controlRow.removeFromLeft(8);

    cancelBtn.setBounds(controlRow.removeFromRight(80).reduced(0, 2));
    controlRow.removeFromRight(4);
    exportBtn.setBounds(controlRow.removeFromRight(140).reduced(0, 2));
    controlRow.removeFromRight(8);

    // Progress strings
    unverifiedBadge.setBounds(controlRow.removeFromRight(120).reduced(0, 4));
    controlRow.removeFromRight(8);

    // Remaining space: progress labels stacked
    auto labelArea = area.reduced(0, 2);
    progressMetaphorLabel.setBounds(labelArea.removeFromTop(14));
    progressTechLabel.setBounds(labelArea.removeFromTop(13));
}
```

**Time estimate display:** When `setGrainCount(n)` is called, update `progressTechLabel` pre-export: `"Analyzing {N} samples, ~{T} sec"`. Compute T: `(int)(n * 0.5 + n * 1.0 + 2.0)`. Clear this text when export starts.

**`onExportButtonClicked()`:** Open `juce::FileChooser` to select output path, then call `onExportClicked(pearlNameField.getText(), format, chosenPath)`. Disable export button for duration of pipeline.

**Format from UI:**

```cpp
ExportFormat getSelectedFormat() const
{
    switch (formatSelector.getSelectedId())
    {
        case 1: return ExportFormat::XPNPack;
        case 2: return ExportFormat::WAVFolder;
        case 3: return ExportFormat::XPMOnly;
        default: return ExportFormat::XPNPack;
    }
}
```

**Commit message:** `feat(outshine): add OutshineExportBar — pearl name, format selector, gold CTA, progress bar, time estimate`

---

### Task 11: OutshinePreviewPlayer

**File:** `Source/UI/Outshine/OutshinePreviewPlayer.h`
**Depends on:** GalleryColors.h, JUCE audio devices

**Purpose:** "Play C4" button with dedicated private AudioDeviceManager. Plays the zone containing C4 at middle velocity. Auto-stops after 4 seconds or sample tail. Isolated from the plugin host audio graph.

**Implementation requirements:**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../GalleryColors.h"
#include "../../Export/XOutshine.h"   // AnalyzedSample

namespace xolokun {

class OutshinePreviewPlayer : public juce::Component,
                              private juce::Timer
{
public:
    OutshinePreviewPlayer();
    ~OutshinePreviewPlayer() override;

    // Called after analyzeGrains() with the analyzed samples
    void setSamples(const std::vector<AnalyzedSample>& samples);

    // Trigger playback of C4 (MIDI 60) at velocity 100
    void playC4();
    void stopPlayback();

    bool isPlaying() const { return playing; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    void timerCallback() override;
    const AnalyzedSample* findSampleForMidi(int midiNote) const;
    void startAudioDevice();

    juce::TextButton                              playC4Btn { "Play C4" };
    juce::AudioDeviceManager                      deviceManager;
    juce::AudioFormatManager                      formatManager;
    juce::AudioTransportSource                    transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    std::unique_ptr<juce::AudioSourcePlayer>       player;

    std::vector<AnalyzedSample>                   currentSamples;
    bool                                          playing { false };
    static constexpr float                        kMaxPlaybackS = 4.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshinePreviewPlayer)
};

} // namespace xolokun
```

**Constructor:**

```cpp
OutshinePreviewPlayer::OutshinePreviewPlayer()
{
    formatManager.registerBasicFormats();

    setWantsKeyboardFocus(true);
    A11y::setup(*this, "Preview Player", "Play C4 to preview the zone mapping");

    playC4Btn.setColour(juce::TextButton::buttonColourId,
                        GalleryColors::get(GalleryColors::slotBg()));
    playC4Btn.setColour(juce::TextButton::textColourOffId,
                        GalleryColors::get(GalleryColors::textDark()));
    playC4Btn.onClick = [this]() {
        if (playing) stopPlayback();
        else         playC4();
    };
    A11y::setup(playC4Btn, "Play C4", "Preview the instrument at middle C, velocity 100. Space to activate.");
    addAndMakeVisible(playC4Btn);

    setSize(120, 32);
}
```

**`startAudioDevice()`:**

```cpp
void OutshinePreviewPlayer::startAudioDevice()
{
    // Isolated from plugin host — private AudioDeviceManager
    if (!player)
    {
        player = std::make_unique<juce::AudioSourcePlayer>();
        deviceManager.addAudioCallback(player.get());
    }

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);
    if (deviceManager.getCurrentAudioDevice() == nullptr)
    {
        deviceManager.initialiseWithDefaultDevices(0, 2);
    }

    player->setSource(&transportSource);
}
```

**`playC4()`:**

```cpp
void OutshinePreviewPlayer::playC4()
{
    const AnalyzedSample* sample = findSampleForMidi(60);  // MIDI 60 = C4
    if (!sample) return;

    // Stop previous playback
    transportSource.stop();
    transportSource.setSource(nullptr);
    readerSource.reset();

    // Load sample file
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(sample->sourceFile));
    if (!reader) return;

    readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
    transportSource.setSource(readerSource.get(), 0, nullptr,
                              readerSource->getAudioFormatReader()->sampleRate);

    startAudioDevice();

    transportSource.setPosition(0.0);
    transportSource.start();
    playing = true;

    // Update button label
    playC4Btn.setButtonText("Stop");

    // Timer: auto-stop after kMaxPlaybackS seconds
    startTimer((int)(kMaxPlaybackS * 1000.0f));

    repaint();
}
```

**`timerCallback()`:**

```cpp
void OutshinePreviewPlayer::timerCallback()
{
    stopTimer();
    if (playing && (!transportSource.isPlaying() || transportSource.getCurrentPosition() >= kMaxPlaybackS))
        stopPlayback();
}
```

**`stopPlayback()`:**

```cpp
void OutshinePreviewPlayer::stopPlayback()
{
    transportSource.stop();
    playing = false;
    playC4Btn.setButtonText("Play C4");
    repaint();
}
```

**`findSampleForMidi()`:** Find the `AnalyzedSample` whose zone contains `midiNote = 60`. Use the same zone boundary computation as `OutshineZoneMap::setSamples()`. Returns the sample whose zone `[lowMidi, highMidi]` contains 60.

**`keyPressed()`:** Space key calls `playC4()` or `stopPlayback()` when the component has focus. Return `true` to consume.

**Destructor:** Stop playback, remove audio callback, reset transport source and reader source before JUCE audio machinery is torn down.

**Note on Phase 1B MPE animation hooks:** The `OutshineMPEPanel` has `setLiveValue()` methods. In Phase 1B, a MIDI callback from `deviceManager` will call these during playback. In Phase 1A, these hooks are never triggered.

**Commit message:** `feat(outshine): add OutshinePreviewPlayer — isolated audio device, Play C4, auto-stop at 4s`

---

### Task 12: Wire OutshineSidebarPanel to XOlokunEditor

**Files modified:**
- `Source/UI/Outshine/OutshineSidebarPanel.h` (new)
- `Source/UI/XOlokunEditor.h` (modify)

**Purpose:** Column C EXPORT tab gets a mini drop zone, status label, and "Open the Oyster" button. Dropping on the mini zone opens the full window AND auto-triggers CLASSIFY + ANALYZE. Double-launch guarded via `SafePointer<OutshineDocumentWindow>`.

**OutshineSidebarPanel.h:**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "OutshineDocumentWindow.h"
#include "../../Core/XOlokunProcessor.h"

namespace xolokun {

class OutshineSidebarPanel : public juce::Component,
                             public juce::FileDragAndDropTarget
{
public:
    explicit OutshineSidebarPanel(XOlokunProcessor& processorRef)
        : processor(processorRef)
    {
        setWantsKeyboardFocus(false);
        A11y::setup(*this, "Outshine Sidebar", "Drop grains or open the full Outshine window");
        buildLayout();
    }

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }
    void fileDragEnter(const juce::StringArray&, int, int) override;
    void fileDragExit(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray& files, int, int) override;

    // Called by OutshineDocumentWindow after each successful pearl export
    void onPearlExported(const juce::String& pearlName, int numZones);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void buildLayout();
    void launchOutshine();
    void launchWithGrains(const juce::StringArray& filePaths);

    XOlokunProcessor& processor;
    juce::Component::SafePointer<OutshineDocumentWindow> outshineWindow;

    juce::Label       headerLabel;
    juce::Label       dropZoneLabel;
    juce::Label       statusLabel;
    juce::TextButton  openOysterBtn { "Open the Oyster" };

    bool              dropActive { false };
    juce::String      lastPearlStatus { "No pearls forged yet" };

    static constexpr int kHeaderH    = 28;
    static constexpr int kDropZoneH  = 60;
    static constexpr int kStatusH    = 32;
    static constexpr int kButtonH    = 32;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineSidebarPanel)
};

} // namespace xolokun
```

**`buildLayout()`:**

```cpp
void OutshineSidebarPanel::buildLayout()
{
    // Header
    headerLabel.setText("OUTSHINE", juce::dontSendNotification);
    headerLabel.setFont(GalleryFonts::heading(9.0f));
    headerLabel.setColour(juce::Label::textColourId,
                          GalleryColors::get(GalleryColors::xoGoldText()));
    addAndMakeVisible(headerLabel);

    // Drop zone label (text; bounds are painted, not a child component)
    // Status
    statusLabel.setText(lastPearlStatus, juce::dontSendNotification);
    statusLabel.setFont(GalleryFonts::body(11.0f));
    statusLabel.setColour(juce::Label::textColourId,
                          GalleryColors::get(GalleryColors::textMid()));
    addAndMakeVisible(statusLabel);

    // Open button
    openOysterBtn.setColour(juce::TextButton::buttonColourId,
                            GalleryColors::get(GalleryColors::xoGold));
    openOysterBtn.setColour(juce::TextButton::textColourOffId,
                            juce::Colour(GalleryColors::Light::textDark));
    openOysterBtn.onClick = [this]() { launchOutshine(); };
    A11y::setup(openOysterBtn, "Open the Oyster", "Open the full Outshine window");
    addAndMakeVisible(openOysterBtn);
}
```

**`launchOutshine()` — double-launch guard:**

```cpp
void OutshineSidebarPanel::launchOutshine()
{
    if (outshineWindow != nullptr)
    {
        outshineWindow->toFront(true);
        return;
    }
    outshineWindow = new OutshineDocumentWindow(processor);
    outshineWindow->centreWithSize(900, 660);
    outshineWindow->setVisible(true);
}
```

**`launchWithGrains()`:** Launch (or raise) the window, pass grain paths to `OutshineMainComponent::onGrainsChanged()` to auto-trigger CLASSIFY + ANALYZE immediately.

```cpp
void OutshineSidebarPanel::launchWithGrains(const juce::StringArray& filePaths)
{
    launchOutshine();

    if (auto* doc = outshineWindow.getComponent())
    {
        if (auto* mc = dynamic_cast<OutshineMainComponent*>(doc->getContentComponent()))
        {
            // Grains pre-populated; analyzeGrains() fires immediately
            mc->onGrainsChanged(filePaths);
        }
    }
}
```

**XPN upgrade toast:**

```cpp
void OutshineSidebarPanel::filesDropped(const juce::StringArray& files, int, int)
{
    dropActive = false;
    repaint();

    juce::StringArray paths;
    for (const auto& f : files)
    {
        juce::File file(f);
        if (file.hasFileExtension(".xpn"))
        {
            // Show upgrade warning toast before opening
            // useToastStore equivalent in XOlokun: call global toast utility
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::InfoIcon,
                "XPN Upgrade",
                "Original program data will be rebuilt from scratch.\n"
                "Zone map, expression routes, and LUFS normalization will be regenerated.",
                "OK");
        }
        paths.add(f);
    }

    if (!paths.isEmpty())
        launchWithGrains(paths);
}
```

**`paint()`:**

```cpp
void OutshineSidebarPanel::paint(juce::Graphics& g)
{
    g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));

    // Header bar
    auto headerArea = getLocalBounds().removeFromTop(kHeaderH);
    g.setColour(GalleryColors::get(GalleryColors::xoGold));
    g.fillRect(headerArea);
    g.setColour(juce::Colour(GalleryColors::Light::textDark));
    g.setFont(GalleryFonts::heading(9.0f));
    g.drawText("OUTSHINE", headerArea.reduced(8, 0), juce::Justification::centredLeft);

    // Drop zone
    auto b = getLocalBounds().reduced(8);
    b.removeFromTop(kHeaderH);
    auto dropZone = b.removeFromTop(kDropZoneH);
    juce::Colour borderCol = dropActive
        ? GalleryColors::get(GalleryColors::xoGold)
        : GalleryColors::get(GalleryColors::borderGray());
    g.setColour(borderCol);
    float dashLengths[] = { 4.0f, 4.0f };
    // Dashed border via path
    juce::Path dashedPath;
    dashedPath.addRoundedRectangle(dropZone.toFloat().reduced(1), 6.0f);
    juce::PathStrokeType stroke(dropActive ? 2.0f : 1.0f);
    float dashData[] = { 4.0f, 4.0f };
    stroke.createDashedStroke(dashedPath, dashedPath, dashData, 2);
    g.fillPath(dashedPath);

    g.setColour(GalleryColors::get(GalleryColors::textMid()));
    g.setFont(GalleryFonts::body(11.0f));
    g.drawText("Drop a grain here", dropZone, juce::Justification::centred);

    // Section divider
    b.removeFromTop(4);
    g.setColour(GalleryColors::get(GalleryColors::borderGray()));
    g.drawLine(8, (float)(kHeaderH + kDropZoneH + 4),
               (float)(getWidth() - 8), (float)(kHeaderH + kDropZoneH + 4), 1.0f);
}
```

**`onPearlExported()`:** Update `lastPearlStatus` and `statusLabel`. Truncate pearl name at 24 chars with ellipsis.

**resized():**

```cpp
void OutshineSidebarPanel::resized()
{
    auto area = getLocalBounds().reduced(8);
    headerLabel.setBounds(area.removeFromTop(kHeaderH));
    area.removeFromTop(kDropZoneH);   // drop zone is painted, not a child
    area.removeFromTop(8);
    statusLabel.setBounds(area.removeFromTop(kStatusH));
    area.removeFromTop(4);
    openOysterBtn.setBounds(area.removeFromTop(kButtonH));
}
```

**XOlokunEditor.h modification:** In the Column C EXPORT tab construction, add:

```cpp
// In XOlokunEditor.h — within EXPORT tab setup
// Add as a private member:
std::unique_ptr<OutshineSidebarPanel> outshineSidebarPanel;

// In constructor, after EXPORT tab is set up:
outshineSidebarPanel = std::make_unique<OutshineSidebarPanel>(processor);
// Add to EXPORT tab column C with appropriate bounds
exportTabContent->addAndMakeVisible(*outshineSidebarPanel);
```

Include guard: `#include "Outshine/OutshineSidebarPanel.h"` in `XOlokunEditor.h`.

**Commit message:** `feat(outshine): wire OutshineSidebarPanel to EXPORT tab — drop triggers auto-analyze, double-launch guard`

---

## Integration Test Checklist

After all 12 tasks are complete, verify the following manually:

### Functional Tests

- [ ] Drop a single WAV onto the sidebar mini drop zone → full window opens with grain pre-loaded and analysis running
- [ ] Drop a folder of 5 WAVs onto the main drop zone → all 5 appear in grain strip; preview panel populates with zone map
- [ ] Remove a grain chip → zone map redraws with remaining grains; if last grain removed, reverts to shell state
- [ ] Mode A/B: resize window to below 700px → input panel switches to tabbed layout; grain strip remains visible
- [ ] Mode A/B: resize back above 700px → reverts to split layout; grains preserved
- [ ] "Select All WAVs" selects only WAV files in current folder (not directories)
- [ ] "Add Selected →" moves selected files to grain strip
- [ ] Shift+click range select in folder browser; Cmd+click individual toggle
- [ ] "Play C4" button plays the zone containing MIDI 60 at velocity 100; auto-stops after 4 seconds
- [ ] Space key triggers "Play C4" when preview panel has focus
- [ ] Export button is disabled until analysis is complete and no Unknown classifications remain
- [ ] Drop an XPN archive → upgrade warning toast appears before opening
- [ ] Cancel button during export → pipeline aborts, grains preserved, "Pearl cancelled" toast shown
- [ ] Escape key closes the DocumentWindow
- [ ] Re-opening the window from the sidebar raises it (does not create a duplicate)
- [ ] Pearl name auto-fills from source folder name on grain load
- [ ] Format selector cycles through XPN Pack / WAV Folder / XPM Only
- [ ] Progress bar animates left to right during export; turns red on failure
- [ ] Dual progress strings: metaphor line + technical stage line both visible during export
- [ ] Time estimate appears in progress area after clicking "Pearl & Export" and before Stage 1 begins
- [ ] Unverified badge shows correct count; clicking badge auto-scrolls to flagged samples in zone map
- [ ] Zone map tooltip shows range, root note, layer count on hover
- [ ] Amber warning icons appear on zones with `pitchConfidence < 0.15`
- [ ] Rebirth Mode teaser is visible at 35% opacity, shows toast on click, and is not otherwise interactive
- [ ] CC74 row in MPE panel shows "(manual config)" annotation in amber

### Accessibility Tests

- [ ] Tab order follows spec Section 14: Drop Zone → Path Bar → File List → Select All → Add Selected → Grain Chips → Summary → Pearl Name → Format → Export Button
- [ ] All interactive elements have A11y::setup() names and descriptions
- [ ] Focus ring visible on all focused interactive elements
- [ ] Minimum 32×32px touch target on all buttons
- [ ] Escape closes window from anywhere in the tab order

### GalleryColors Token Tests

- [ ] No hardcoded hex values in any component except `#666666` (disabled text) and `#E05252` (error)
- [ ] Dark mode toggle: all components respect `GalleryColors::darkMode()`

---

## Error Handling Reference

All errors must surface as toasts. Refer to spec Section 16 for full table. Key cases:

| Condition | Toast Border | Message |
|---|---|---|
| Unreadable WAV | Gold (info) | "Skipped: {filename} (unreadable)" |
| XPN with no WAVs | Gold (info) | "This XPN contains no samples" |
| Export path not writable | Red (error) | "Export failed: cannot write to {path}" |
| Pipeline exception | Red (error) | Specific error from XOutshine |
| Sample rate > 96 kHz | Red (error) | "Sample rate {N} kHz is not supported. Maximum is 96 kHz." |
| Cancel during export | Gold (info) | "Pearl cancelled — grains preserved." |
| File missing from recent list | Gold (info) | "File not found: {filename}" |

Progress bar turns `#E05252` (red) on any pipeline failure. Window remains open for debugging.

---

## State Persistence Reference

State file: `~/.../XO_OX/Outshine/.outshine-state.json`

```cpp
juce::File OutshineMainComponent::getStateFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("XO_OX")
        .getChildFile("Outshine")
        .getChildFile(".outshine-state.json");
}
```

Persisted keys: `recentGrains` (array, 20 max, deduplicated), `lastExportFormat`, `lastBrowsePath`, `windowWidth`, `windowHeight`. Written synchronously after each grain add and each successful export. Read on window open to restore position and recent grains list.

---

## Known Limitations (Phase 1A)

From spec Section 19:

- **No velocity crossfade:** MPC hardware does not support velocity crossfade at the XPM level. Hard velocity steps only. Document this to users in the Pearl Preview Panel tooltip on the velocity layer count.
- **CC74 not embeddable in XPM:** Slide/CC74 expression must be configured at the MPC track level after loading the pearl. The UI's CC74 row is informational only.
- **Rebirth Mode not functional:** Phase 1B feature. Shown as a dimmed teaser.
- **Live MPE animation hooks exist but are not triggered:** `OutshineMPEPanel::setLiveValue()` and `OutshinePreviewPlayer::deviceManager` MIDI callback integration are Phase 1B work.
- **Dual AfterTouch destinations unverified:** Default to single destination (FilterResonance) until hardware verification. See spec Section 20.
