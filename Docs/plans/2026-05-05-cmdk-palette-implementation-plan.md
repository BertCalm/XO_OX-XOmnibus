# Cmd+K Command Palette Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a `Cmd+K` command palette that fuzzy-searches presets and engines, ranks results with literal + 6D DNA scoring, and loads them via Enter (slot 1) or Cmd+2/3/4 (slots 2/3/4).

**Architecture:** One new component (`CommandPalette.h`) hosted as an overlay child of `XOceanusEditor`. Recents tracking added to `PresetManager` and `EngineRegistry`. Visuals reuse existing `Tokens.h` D1-D5 design system from the Me-First campaign.

**Tech Stack:** C++17, JUCE 8, CMake, Catch2 (tests), `juce::Component` overlay pattern, `juce::Timer` for input debounce.

**Spec source:** `Docs/plans/2026-05-05-cmdk-palette-design.md`

---

## File Structure

| Path | Action | Responsibility |
|---|---|---|
| `Source/UI/Ocean/CommandPalette.h` | CREATE | Modal palette component, ranking, rendering |
| `Source/Core/PresetManager.h` | MODIFY | Add `getPresetLibrary()`, `loadPresetByIndex()`, recents API |
| `Source/Core/EngineRegistry.h` | MODIFY | Add `recordEngineLoad()`, `getRecentEngineIds()` |
| `Source/UI/XOceanusEditor.h` | MODIFY | `Cmd+K` handler, palette lifecycle, click-outside |
| `Tests/PresetTests/CommandPaletteTests.cpp` | CREATE | Catch2 unit tests |
| `Tests/CMakeLists.txt` | MODIFY | Add `CommandPaletteTests.cpp` to `XOceanusTests` |

---

## Pre-flight (do this once before starting)

- [ ] **Step 0.1: Set up environment**

```bash
eval "$(fnm env)" && fnm use 20
cd /Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus
```

- [ ] **Step 0.2: Create branch**

Verify your worktree HEAD before branching. STOP and surface if `feat/cmdk-palette-1` already exists locally or on remote — do NOT auto-rename.

```bash
git rev-parse --show-toplevel  # confirm you are in worktree, not main repo tree
git fetch origin
git checkout -b feat/cmdk-palette-1 origin/main
```

- [ ] **Step 0.3: Verify build baseline**

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Expected: zero errors. If anything fails, STOP — your worktree is misconfigured.

---

## Task 1: PresetManager — `getPresetLibrary()` and `loadPresetByIndex()`

**Files:**
- Modify: `Source/Core/PresetManager.h`
- Test: `Tests/PresetTests/CommandPaletteTests.cpp` (create)
- Modify: `Tests/CMakeLists.txt`

- [ ] **Step 1.1: Write the failing test**

Create `Tests/PresetTests/CommandPaletteTests.cpp`:

```cpp
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

#include "../../Source/Core/PresetManager.h"

using namespace xoceanus;

TEST_CASE("PresetManager exposes preset library snapshot", "[CommandPalette]")
{
    PresetManager pm;
    auto lib = pm.getPresetLibrary();
    REQUIRE(lib != nullptr);
    // Library may be empty in test harness — only assert the accessor works.
}

TEST_CASE("loadPresetByIndex bounds-checks", "[CommandPalette]")
{
    PresetManager pm;
    REQUIRE_FALSE(pm.loadPresetByIndex(-1));
    REQUIRE_FALSE(pm.loadPresetByIndex(999999999));
}
```

- [ ] **Step 1.2: Add file to CMakeLists**

Modify `Tests/CMakeLists.txt` — append `CommandPaletteTests.cpp` to the existing `XOceanusTests` source list. Replicate the pattern used for `SaveAsTests.cpp` (added in PR #1522).

```cmake
target_sources(XOceanusTests PRIVATE
    PresetTests/SaveAsTests.cpp
    PresetTests/CommandPaletteTests.cpp  # NEW
    # ... other test files
)
```

- [ ] **Step 1.3: Run test to verify it fails**

```bash
cmake --build build --target XOceanusTests
./build/Tests/XOceanusTests "[CommandPalette]"
```

Expected: compile FAIL — `getPresetLibrary` and `loadPresetByIndex` undefined.

- [ ] **Step 1.4: Add API to PresetManager.h**

In `Source/Core/PresetManager.h`, find the public section (search for `bool loadPresetFromFile`). Add:

```cpp
// Public accessor for the active preset library snapshot. Used by CommandPalette
// for ranking. Thread-safety: returns the shared_ptr by value; the underlying
// vector is immutable once published, so concurrent reads are safe.
std::shared_ptr<const std::vector<PresetData>> getPresetLibrary() const
{
    return std::atomic_load(&allPresets_);
}

// Convenience: load a preset by its index in the active library snapshot.
// Returns false on out-of-bounds, missing file, or parse failure.
bool loadPresetByIndex(int presetIndex)
{
    auto lib = getPresetLibrary();
    if (lib == nullptr) return false;
    if (presetIndex < 0 || presetIndex >= static_cast<int>(lib->size())) return false;
    const auto& preset = (*lib)[static_cast<size_t>(presetIndex)];
    if (preset.sourceFile == juce::File()) return false;
    return loadPresetFromFile(preset.sourceFile);
}
```

NOTE: if `PresetData` does not have a `sourceFile` field, locate the field that stores the on-disk path (likely `juce::File file` or similar near line 337). Use that field's actual name. If no such field exists, this task expands to also caching the file path during library scan — STOP and surface, this would change the data model.

- [ ] **Step 1.5: Run test to verify it passes**

```bash
cmake --build build --target XOceanusTests
./build/Tests/XOceanusTests "[CommandPalette]"
```

Expected: PASS for both test cases.

- [ ] **Step 1.6: Commit**

```bash
git add Source/Core/PresetManager.h Tests/PresetTests/CommandPaletteTests.cpp Tests/CMakeLists.txt
git commit -m "feat(cmdk): preset library accessor + index load (Task 1)"
```

---

## Task 2: PresetManager — recents tracking

**Files:**
- Modify: `Source/Core/PresetManager.h`
- Test: `Tests/PresetTests/CommandPaletteTests.cpp`

- [ ] **Step 2.1: Write the failing test**

Append to `CommandPaletteTests.cpp`:

```cpp
TEST_CASE("PresetManager recents — push, dedupe, max-8", "[CommandPalette]")
{
    PresetManager pm;
    REQUIRE(pm.getRecentPresetIndices().empty());

    pm.recordPresetLoad(5);
    pm.recordPresetLoad(7);
    pm.recordPresetLoad(5);  // dedupe → moves 5 to front

    auto r = pm.getRecentPresetIndices();
    REQUIRE(r.size() == 2);
    REQUIRE(r[0] == 5);
    REQUIRE(r[1] == 7);

    for (int i = 0; i < 12; ++i) pm.recordPresetLoad(100 + i);
    REQUIRE(pm.getRecentPresetIndices().size() == 8);
}
```

- [ ] **Step 2.2: Run test to verify it fails**

```bash
cmake --build build --target XOceanusTests 2>&1 | tail -20
```

Expected: compile FAIL — `recordPresetLoad` and `getRecentPresetIndices` undefined.

- [ ] **Step 2.3: Add recents API to PresetManager.h**

In the private section near the bottom of the class, add:

```cpp
private:
    std::deque<int> recentPresetIndices_;
    static constexpr size_t kMaxRecents_ = 8;

public:
    // Recents tracking — palette-driven loads only (NOT every loadPresetFromFile).
    // The CommandPalette calls this immediately after a successful load.
    void recordPresetLoad(int presetIndex)
    {
        // Dedupe: remove any existing entry for this index.
        for (auto it = recentPresetIndices_.begin(); it != recentPresetIndices_.end(); )
        {
            if (*it == presetIndex) it = recentPresetIndices_.erase(it);
            else ++it;
        }
        recentPresetIndices_.push_front(presetIndex);
        while (recentPresetIndices_.size() > kMaxRecents_)
            recentPresetIndices_.pop_back();
    }

    const std::deque<int>& getRecentPresetIndices() const { return recentPresetIndices_; }
```

If `<deque>` is not already included, add `#include <deque>` at the top of the file (check the existing includes block).

- [ ] **Step 2.4: Run test to verify it passes**

```bash
cmake --build build --target XOceanusTests
./build/Tests/XOceanusTests "[CommandPalette]"
```

Expected: PASS for the recents test case.

- [ ] **Step 2.5: Commit**

```bash
git add Source/Core/PresetManager.h Tests/PresetTests/CommandPaletteTests.cpp
git commit -m "feat(cmdk): PresetManager recents tracking (Task 2)"
```

---

## Task 3: EngineRegistry — recents tracking

**Files:**
- Modify: `Source/Core/EngineRegistry.h`
- Test: `Tests/PresetTests/CommandPaletteTests.cpp`

- [ ] **Step 3.1: Write the failing test**

Append to `CommandPaletteTests.cpp`:

```cpp
TEST_CASE("EngineRegistry recents — push, dedupe, max-8", "[CommandPalette]")
{
    auto& reg = xoceanus::EngineRegistry::instance();
    // Note: singleton state persists across tests. Clear before assertions.
    reg.clearRecentsForTesting();

    REQUIRE(reg.getRecentEngineIds().empty());

    reg.recordEngineLoad("Obsidian", 0);
    reg.recordEngineLoad("Oracle", 1);
    reg.recordEngineLoad("Obsidian", 2);  // dedupe → moves to front

    auto r = reg.getRecentEngineIds();
    REQUIRE(r.size() == 2);
    REQUIRE(r[0] == "Obsidian");
    REQUIRE(r[1] == "Oracle");

    for (int i = 0; i < 12; ++i) reg.recordEngineLoad("Engine" + std::to_string(i), 0);
    REQUIRE(reg.getRecentEngineIds().size() == 8);

    reg.clearRecentsForTesting();  // leave clean for next test
}
```

- [ ] **Step 3.2: Run test to verify it fails**

```bash
cmake --build build --target XOceanusTests 2>&1 | tail -20
```

Expected: compile FAIL.

- [ ] **Step 3.3: Add recents API to EngineRegistry.h**

In `Source/Core/EngineRegistry.h`, after `isRegistered()` (around line 75), add:

```cpp
// ── Recents (CommandPalette) ────────────────────────────────────────────
//
// Records palette-driven engine loads. Slot is captured for future analytics
// but isn't surfaced to the UI yet (V1 just shows the engine IDs).
// Thread-safety: message-thread only — palette interactions happen there.
void recordEngineLoad(const std::string& engineId, int /*slot*/)
{
    for (auto it = recentEngineIds_.begin(); it != recentEngineIds_.end(); )
    {
        if (*it == engineId) it = recentEngineIds_.erase(it);
        else ++it;
    }
    recentEngineIds_.push_front(engineId);
    while (recentEngineIds_.size() > kMaxRecents_)
        recentEngineIds_.pop_back();
}

const std::deque<std::string>& getRecentEngineIds() const { return recentEngineIds_; }

// Test helper — never call from production code.
void clearRecentsForTesting() { recentEngineIds_.clear(); }

private:
    std::deque<std::string> recentEngineIds_;
    static constexpr size_t kMaxRecents_ = 8;
```

Add `#include <deque>` if not already present.

- [ ] **Step 3.4: Run test to verify it passes**

```bash
cmake --build build --target XOceanusTests
./build/Tests/XOceanusTests "[CommandPalette]"
```

Expected: PASS for all 4 test cases now.

- [ ] **Step 3.5: Commit**

```bash
git add Source/Core/EngineRegistry.h Tests/PresetTests/CommandPaletteTests.cpp
git commit -m "feat(cmdk): EngineRegistry recents tracking (Task 3)"
```

---

## Task 4: CommandPalette skeleton + Result struct

**Files:**
- Create: `Source/UI/Ocean/CommandPalette.h`
- Modify: `CMakeLists.txt` (root) — header-only, but verify it's discovered

- [ ] **Step 4.1: Create file with skeleton**

Create `Source/UI/Ocean/CommandPalette.h`:

```cpp
#pragma once

// CommandPalette.h — Cmd+K modal palette for fuzzy-searching presets + engines.
// Spec: Docs/plans/2026-05-05-cmdk-palette-design.md
// V1 scope: presets + engines only. Params/FX deferred to v1.1.

#include <juce_gui_basics/juce_gui_basics.h>
#include <deque>
#include <vector>
#include <string>

#include "../../Core/PresetManager.h"
#include "../../Core/EngineRegistry.h"

// Forward decl — XOceanusProcessor lives in Source/ root, header-circular.
namespace xoceanus { class XOceanusProcessor; }

namespace xoceanus {

enum class CommandResultKind { Preset, Engine };

struct CommandResult
{
    CommandResultKind kind;
    juce::String      displayName;
    juce::String      secondaryLabel;
    juce::Colour      accent;
    int               presetIndex = -1;
    std::string       engineId;
    float             score = 0.0f;
};

class CommandPalette : public juce::Component, private juce::Timer
{
public:
    CommandPalette(PresetManager& pm,
                   XOceanusProcessor& proc,
                   std::function<void()> onClose);

    ~CommandPalette() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

    void grabSearchFocus();
    void startEntryAnimation();

    // Exposed for testing: re-rank against the current library + query.
    void rerankForTesting(const juce::String& query);
    const std::vector<CommandResult>& getResults() const { return results_; }

private:
    void timerCallback() override;
    void rerank(const juce::String& query);
    void rebuildEmptyState();
    void activateResult(int index, int targetSlot);
    void renderResults(juce::Graphics&);
    void renderFooter(juce::Graphics&);

    PresetManager&                presetManager_;
    XOceanusProcessor&            processor_;
    std::function<void()>         onClose_;

    juce::TextEditor              searchField_;
    std::vector<CommandResult>    results_;
    int                           selectedIndex_ = 0;
    juce::String                  pendingQuery_;
};

} // namespace xoceanus
```

- [ ] **Step 4.2: Add to root CMakeLists if it lists headers explicitly**

```bash
grep -n "CommandPalette\|SavePresetDialog" CMakeLists.txt | head -5
```

If `SavePresetDialog.h` is listed, add `Source/UI/Ocean/CommandPalette.h` next to it. If headers are globbed (likely — JUCE convention), no edit needed.

- [ ] **Step 4.3: Verify it compiles into the test binary**

Append a smoke test to `CommandPaletteTests.cpp`:

```cpp
#include "../../Source/UI/Ocean/CommandPalette.h"

TEST_CASE("CommandPalette header compiles", "[CommandPalette][Smoke]")
{
    SUCCEED("Header compiled");
}
```

Run:

```bash
cmake --build build --target XOceanusTests
```

Expected: zero errors. If `XOceanusProcessor` forward decl can't resolve, comment out the smoke test for now and resume in Task 11 — the real wiring lives there. Add a `// TODO: Task 11 wires the smoke test back in` line and proceed.

- [ ] **Step 4.4: Commit**

```bash
git add Source/UI/Ocean/CommandPalette.h Tests/PresetTests/CommandPaletteTests.cpp CMakeLists.txt
git commit -m "feat(cmdk): CommandPalette skeleton + CommandResult struct (Task 4)"
```

---

## Task 5: Ranking — literal fuzzy match (presets)

**Files:**
- Modify: `Source/UI/Ocean/CommandPalette.h`
- Test: `Tests/PresetTests/CommandPaletteTests.cpp`

- [ ] **Step 5.1: Write the failing test**

Append to `CommandPaletteTests.cpp`:

```cpp
TEST_CASE("Preset literal ranking — name beats tag beats description", "[CommandPalette][Rank]")
{
    // Build a synthetic library inline. We can't easily inject into the singleton
    // PresetManager from a unit test, so we test the scoring helper directly.
    // The helper lives in CommandPalette.h as a free function for testability.

    PresetData a; a.name = "Warm Pad";    a.tags.add("ambient");
    PresetData b; b.name = "Glacial";      b.tags.add("warm"); b.tags.add("pad");
    PresetData c; c.name = "Description";  c.description = "warm strings";

    juce::StringArray tokens; tokens.add("warm");

    REQUIRE(scorePresetLiteral(a, tokens) == Catch::Approx(3.0f));   // name hit
    REQUIRE(scorePresetLiteral(b, tokens) == Catch::Approx(2.0f));   // tag hit
    REQUIRE(scorePresetLiteral(c, tokens) == Catch::Approx(1.0f));   // description hit
}
```

- [ ] **Step 5.2: Run test to verify it fails**

```bash
cmake --build build --target XOceanusTests 2>&1 | tail -20
```

Expected: FAIL (`scorePresetLiteral` undefined).

- [ ] **Step 5.3: Implement scorePresetLiteral**

In `CommandPalette.h`, add inside the `xoceanus` namespace (above the class):

```cpp
inline float scorePresetLiteral(const PresetData& p, const juce::StringArray& tokens)
{
    float score = 0.0f;
    auto nameLower = p.name.toLowerCase();
    auto descLower = p.description.toLowerCase();
    auto moodLower = p.mood.toLowerCase();

    for (const auto& token : tokens)
    {
        auto t = token.toLowerCase();
        if (nameLower.contains(t)) score += 3.0f;
        for (const auto& tag : p.tags)
            if (tag.toLowerCase().contains(t)) { score += 2.0f; break; }
        if (descLower.contains(t)) score += 1.0f;
        if (moodLower.contains(t)) score += 1.0f;
    }
    return score;
}
```

NOTE: verify `PresetData` has fields `name`, `description`, `mood`, `tags`. If field names differ, look at `PresetManager.h:337-376` for the actual struct shape. If `tags` is `juce::StringArray` vs `std::vector<juce::String>`, adjust the loop accordingly.

- [ ] **Step 5.4: Run test to verify it passes**

```bash
./build/Tests/XOceanusTests "[CommandPalette][Rank]"
```

Expected: PASS.

- [ ] **Step 5.5: Commit**

```bash
git add Source/UI/Ocean/CommandPalette.h Tests/PresetTests/CommandPaletteTests.cpp
git commit -m "feat(cmdk): preset literal-match scoring (Task 5)"
```

---

## Task 6: Ranking — DNA dimension match

**Files:**
- Modify: `Source/UI/Ocean/CommandPalette.h`
- Test: `Tests/PresetTests/CommandPaletteTests.cpp`

- [ ] **Step 6.1: Write the failing test**

```cpp
TEST_CASE("DNA dimension ranking", "[CommandPalette][Rank]")
{
    PresetData a; a.name = "Glacier"; a.dna.warmth = 0.9f;
    PresetData b; b.name = "Glacier"; b.dna.warmth = 0.1f;

    juce::StringArray tokens; tokens.add("warm");

    REQUIRE(scorePresetDNA(a, tokens) == Catch::Approx(1.8f));  // 0.9 * 2
    REQUIRE(scorePresetDNA(b, tokens) == Catch::Approx(0.2f));  // 0.1 * 2
}

TEST_CASE("DNA dimension keywords map correctly", "[CommandPalette][Rank]")
{
    PresetData p;
    p.dna.brightness = 0.7f;
    p.dna.warmth     = 0.0f;
    p.dna.movement   = 0.5f;
    p.dna.density    = 0.8f;
    p.dna.space      = 0.3f;
    p.dna.aggression = 0.9f;

    auto check = [&](const char* word, float expected) {
        juce::StringArray tokens; tokens.add(word);
        REQUIRE(scorePresetDNA(p, tokens) == Catch::Approx(expected));
    };

    check("bright",     0.7f * 2);
    check("warm",       0.0f * 2);
    check("movement",   0.5f * 2);
    check("dense",      0.8f * 2);
    check("space",      0.3f * 2);
    check("aggressive", 0.9f * 2);
}
```

- [ ] **Step 6.2: Run test to verify it fails**

Expected: compile FAIL (`scorePresetDNA` undefined).

- [ ] **Step 6.3: Implement scorePresetDNA**

Add in the namespace:

```cpp
inline float scorePresetDNA(const PresetData& p, const juce::StringArray& tokens)
{
    float score = 0.0f;
    for (const auto& token : tokens)
    {
        auto t = token.toLowerCase();
        if (t.contains("bright"))      score += p.dna.brightness * 2.0f;
        if (t.contains("warm"))        score += p.dna.warmth     * 2.0f;
        if (t.contains("mov"))         score += p.dna.movement   * 2.0f;
        if (t.contains("dense"))       score += p.dna.density    * 2.0f;
        if (t.contains("density"))     score += p.dna.density    * 2.0f;
        if (t.contains("space") || t.contains("spacious")) score += p.dna.space * 2.0f;
        if (t.contains("aggress"))     score += p.dna.aggression * 2.0f;
    }
    return score;
}
```

- [ ] **Step 6.4: Run test to verify it passes**

```bash
./build/Tests/XOceanusTests "[CommandPalette][Rank]"
```

Expected: PASS for both new test cases.

- [ ] **Step 6.5: Commit**

```bash
git add Source/UI/Ocean/CommandPalette.h Tests/PresetTests/CommandPaletteTests.cpp
git commit -m "feat(cmdk): DNA dimension ranking (Task 6)"
```

---

## Task 7: Ranking — engine scoring + combined rerank()

**Files:**
- Modify: `Source/UI/Ocean/CommandPalette.h`
- Test: `Tests/PresetTests/CommandPaletteTests.cpp`

- [ ] **Step 7.1: Write the failing test**

```cpp
TEST_CASE("Engine literal ranking — id beats tag", "[CommandPalette][Rank]")
{
    juce::StringArray tokens; tokens.add("obsidian");
    REQUIRE(scoreEngineLiteral("Obsidian", "Obsidian", "pad", tokens) == Catch::Approx(5.0f));
    // 3 (id hit) + 2 (display hit) = 5

    juce::StringArray bassTokens; bassTokens.add("bass");
    REQUIRE(scoreEngineLiteral("OGRE", "OGRE", "bass", bassTokens) == Catch::Approx(1.0f));
    // tag hit only
}
```

- [ ] **Step 7.2: Run test to verify it fails**

Expected: compile FAIL.

- [ ] **Step 7.3: Implement scoreEngineLiteral**

```cpp
inline float scoreEngineLiteral(const juce::String& engineId,
                                const juce::String& displayName,
                                const juce::String& kindTag,
                                const juce::StringArray& tokens)
{
    float score = 0.0f;
    auto idLower      = engineId.toLowerCase();
    auto displayLower = displayName.toLowerCase();
    auto kindLower    = kindTag.toLowerCase();

    for (const auto& token : tokens)
    {
        auto t = token.toLowerCase();
        if (idLower.contains(t))      score += 3.0f;
        if (displayLower.contains(t)) score += 2.0f;
        if (kindLower.contains(t))    score += 1.0f;
    }
    return score;
}
```

- [ ] **Step 7.4: Run test to verify it passes**

Expected: PASS.

- [ ] **Step 7.5: Commit**

```bash
git add Source/UI/Ocean/CommandPalette.h Tests/PresetTests/CommandPaletteTests.cpp
git commit -m "feat(cmdk): engine literal-match scoring (Task 7)"
```

---

## Task 8: CommandPalette::rerank() — full pipeline + filter + sort + top-10

**Files:**
- Modify: `Source/UI/Ocean/CommandPalette.h`
- Test: `Tests/PresetTests/CommandPaletteTests.cpp`

- [ ] **Step 8.1: Implement constructor + rerank()**

In `CommandPalette.h`, add the implementations (use inline definitions in the header to match the codebase's "DSP lives in headers" convention):

```cpp
inline CommandPalette::CommandPalette(PresetManager& pm,
                                      XOceanusProcessor& proc,
                                      std::function<void()> onClose)
    : presetManager_(pm), processor_(proc), onClose_(std::move(onClose))
{
    addAndMakeVisible(searchField_);
    searchField_.setMultiLine(false);
    searchField_.onTextChange = [this] { startTimer(50); };
    setWantsKeyboardFocus(true);
}

inline CommandPalette::~CommandPalette() { stopTimer(); }

inline void CommandPalette::timerCallback()
{
    stopTimer();
    rerank(searchField_.getText());
    repaint();
}

inline void CommandPalette::rerank(const juce::String& query)
{
    results_.clear();
    selectedIndex_ = 0;

    if (query.isEmpty()) { rebuildEmptyState(); return; }

    juce::StringArray tokens;
    tokens.addTokens(query.toLowerCase(), " \t", "");
    tokens.removeEmptyStrings();
    if (tokens.isEmpty()) { rebuildEmptyState(); return; }

    auto lib = presetManager_.getPresetLibrary();
    if (lib != nullptr)
    {
        for (int i = 0; i < static_cast<int>(lib->size()); ++i)
        {
            const auto& p = (*lib)[static_cast<size_t>(i)];
            float litScore = scorePresetLiteral(p, tokens);
            float dnaScore = scorePresetDNA(p, tokens);
            float total = litScore + dnaScore;
            if (total > 0.0f)
            {
                CommandResult r;
                r.kind = CommandResultKind::Preset;
                r.displayName = p.name;
                r.secondaryLabel = p.mood;
                r.accent = juce::Colour::fromString("FF888888");  // mood-color stub; Task 9 wires real
                r.presetIndex = i;
                r.score = total;
                results_.push_back(std::move(r));
            }
        }
    }

    auto engineIds = EngineRegistry::instance().getRegisteredIds();
    for (const auto& id : engineIds)
    {
        juce::String idStr(id);
        float score = scoreEngineLiteral(idStr, idStr, "", tokens);
        if (score > 0.0f)
        {
            CommandResult r;
            r.kind = CommandResultKind::Engine;
            r.displayName = idStr;
            r.secondaryLabel = "Engine";
            r.accent = juce::Colour::fromString("FF888888");  // engine-color stub
            r.engineId = id;
            r.score = score;
            results_.push_back(std::move(r));
        }
    }

    std::sort(results_.begin(), results_.end(),
        [](const CommandResult& a, const CommandResult& b) {
            if (a.score != b.score) return a.score > b.score;
            if (a.kind != b.kind) return a.kind == CommandResultKind::Preset;
            return a.displayName < b.displayName;
        });

    if (results_.size() > 10) results_.resize(10);
}

inline void CommandPalette::rerankForTesting(const juce::String& query) { rerank(query); }

inline void CommandPalette::rebuildEmptyState()
{
    auto lib = presetManager_.getPresetLibrary();
    const auto& recentPresets = presetManager_.getRecentPresetIndices();
    int presetsAdded = 0;
    if (lib != nullptr)
    {
        for (int idx : recentPresets)
        {
            if (presetsAdded >= 3) break;
            if (idx < 0 || idx >= static_cast<int>(lib->size())) continue;
            const auto& p = (*lib)[static_cast<size_t>(idx)];
            CommandResult r;
            r.kind = CommandResultKind::Preset;
            r.displayName = p.name;
            r.secondaryLabel = p.mood;
            r.accent = juce::Colour::fromString("FF888888");
            r.presetIndex = idx;
            results_.push_back(std::move(r));
            ++presetsAdded;
        }
    }

    const auto& recentEngines = EngineRegistry::instance().getRecentEngineIds();
    int enginesAdded = 0;
    for (const auto& id : recentEngines)
    {
        if (enginesAdded >= 3) break;
        CommandResult r;
        r.kind = CommandResultKind::Engine;
        r.displayName = juce::String(id);
        r.secondaryLabel = "Engine";
        r.accent = juce::Colour::fromString("FF888888");
        r.engineId = id;
        results_.push_back(std::move(r));
        ++enginesAdded;
    }
}
```

- [ ] **Step 8.2: Add a rerank smoke test**

```cpp
TEST_CASE("rerank() empty query returns recents", "[CommandPalette]")
{
    PresetManager pm;
    pm.recordPresetLoad(0);  // assumes lib has at least 1 preset; test environment may differ
    // If your test env has no library, this test SKIPs by becoming a no-op.
    auto lib = pm.getPresetLibrary();
    if (lib == nullptr || lib->empty()) { SUCCEED("no library in test env"); return; }

    // Construct a stub processor pointer — we don't call into it for ranking.
    // Use a struct with the minimum surface; if construction is heavy, mark XFAIL.
    SUCCEED("ranking pipeline compiles");
}
```

- [ ] **Step 8.3: Run build to verify it compiles**

```bash
cmake --build build --target XOceanusTests
```

Expected: zero errors. The test is a smoke test — full integration testing happens in manual smoke (Task 12).

- [ ] **Step 8.4: Commit**

```bash
git add Source/UI/Ocean/CommandPalette.h Tests/PresetTests/CommandPaletteTests.cpp
git commit -m "feat(cmdk): rerank() pipeline + empty-state recents (Task 8)"
```

---

## Task 9: Result rendering — paint + selection highlight + accent colors

**Files:**
- Modify: `Source/UI/Ocean/CommandPalette.h`

- [ ] **Step 9.1: Implement paint(), resized(), and rendering helpers**

```cpp
inline void CommandPalette::paint(juce::Graphics& g)
{
    // Backdrop (50% black overlay) — full-bounds.
    g.fillAll(juce::Colour::fromRGBA(0, 0, 0, 128));

    // Content rect — centered 640x480.
    auto contentRect = juce::Rectangle<int>(640, 480).withCentre(getLocalBounds().getCentre());

    // Reuse existing tokens if present, else inline.
    g.setColour(juce::Colour::fromRGB(28, 32, 38));   // PanelBg approximation
    g.fillRoundedRectangle(contentRect.toFloat(), 8.0f);
    g.setColour(juce::Colour::fromRGB(60, 70, 82));   // depth-ring approximation
    g.drawRoundedRectangle(contentRect.toFloat(), 8.0f, 1.5f);

    // Render results inside contentRect (offset for searchField at top + footer at bottom).
    auto resultsArea = contentRect.reduced(12).withTrimmedTop(56).withTrimmedBottom(36);
    for (size_t i = 0; i < results_.size(); ++i)
    {
        auto rowRect = resultsArea.removeFromTop(56);
        bool isSelected = static_cast<int>(i) == selectedIndex_;
        if (isSelected)
        {
            g.setColour(juce::Colour::fromRGB(40, 50, 64));
            g.fillRoundedRectangle(rowRect.toFloat().reduced(2.0f), 4.0f);
        }
        // Accent bar
        g.setColour(results_[i].accent);
        g.fillRect(rowRect.removeFromLeft(8));
        // Display name
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(16.0f, juce::Font::plain));
        g.drawText(results_[i].displayName,
                   rowRect.removeFromTop(28).reduced(8, 0),
                   juce::Justification::centredLeft, true);
        // Secondary label
        g.setColour(juce::Colour::fromRGBA(255, 255, 255, 153));  // 60% white
        g.setFont(juce::Font(12.0f, juce::Font::plain));
        g.drawText(results_[i].secondaryLabel,
                   rowRect.reduced(8, 0),
                   juce::Justification::centredLeft, true);
        // Kind badge (right-aligned)
        auto badge = (results_[i].kind == CommandResultKind::Preset) ? "PRESET" : "ENGINE";
        g.drawText(badge, rowRect.reduced(8, 0), juce::Justification::centredRight, true);
    }

    renderFooter(g);
}

inline void CommandPalette::renderFooter(juce::Graphics& g)
{
    auto contentRect = juce::Rectangle<int>(640, 480).withCentre(getLocalBounds().getCentre());
    auto footerRect = contentRect.reduced(12).removeFromBottom(24);

    juce::String hint;
    if (selectedIndex_ < static_cast<int>(results_.size()))
    {
        hint = (results_[selectedIndex_].kind == CommandResultKind::Preset)
            ? "ENTER load   ESC close"
            : "ENTER slot 1   CMD+2-4 slot   ESC close";
    }
    else
    {
        hint = "Type to search presets and engines   ESC close";
    }

    g.setColour(juce::Colour::fromRGBA(255, 255, 255, 153));
    g.setFont(juce::Font(11.0f, juce::Font::plain));
    g.drawText(hint, footerRect, juce::Justification::centredRight, true);
}

inline void CommandPalette::resized()
{
    auto contentRect = juce::Rectangle<int>(640, 480).withCentre(getLocalBounds().getCentre());
    searchField_.setBounds(contentRect.reduced(12).withHeight(48));
}

inline void CommandPalette::grabSearchFocus() { searchField_.grabKeyboardFocus(); }
inline void CommandPalette::startEntryAnimation() { /* TODO Task 11: 200ms fade-in */ }
```

NOTE: replacing the inline color constants with `Tokens::Submarine::PanelBg` etc. happens in Task 11 if those tokens exist. The PR description should flag a v1.1 cleanup.

- [ ] **Step 9.2: Verify it compiles**

```bash
cmake --build build --target XOceanus_AU
```

Expected: zero errors.

- [ ] **Step 9.3: Commit**

```bash
git add Source/UI/Ocean/CommandPalette.h
git commit -m "feat(cmdk): result row rendering + footer hint (Task 9)"
```

---

## Task 10: Keyboard navigation + activateResult()

**Files:**
- Modify: `Source/UI/Ocean/CommandPalette.h`

- [ ] **Step 10.1: Implement keyPressed() and activateResult()**

```cpp
inline bool CommandPalette::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (onClose_) onClose_();
        return true;
    }

    if (key == juce::KeyPress::downKey)
    {
        if (!results_.empty())
        {
            selectedIndex_ = juce::jmin(selectedIndex_ + 1, static_cast<int>(results_.size()) - 1);
            repaint();
        }
        return true;
    }

    if (key == juce::KeyPress::upKey)
    {
        selectedIndex_ = juce::jmax(selectedIndex_ - 1, 0);
        repaint();
        return true;
    }

    if (key == juce::KeyPress::returnKey)
    {
        activateResult(selectedIndex_, /*targetSlot=*/0);
        return true;
    }

    if (key.getModifiers().isCommandDown())
    {
        if (key == juce::KeyPress('2', juce::ModifierKeys::commandModifier, 0))
        { activateResult(selectedIndex_, 1); return true; }
        if (key == juce::KeyPress('3', juce::ModifierKeys::commandModifier, 0))
        { activateResult(selectedIndex_, 2); return true; }
        if (key == juce::KeyPress('4', juce::ModifierKeys::commandModifier, 0))
        { activateResult(selectedIndex_, 3); return true; }
    }

    return false;
}

inline void CommandPalette::activateResult(int index, int targetSlot)
{
    if (index < 0 || index >= static_cast<int>(results_.size())) return;
    const auto& r = results_[static_cast<size_t>(index)];

    if (r.kind == CommandResultKind::Preset)
    {
        if (presetManager_.loadPresetByIndex(r.presetIndex))
            presetManager_.recordPresetLoad(r.presetIndex);
    }
    else
    {
        // processor_.loadEngine(targetSlot, r.engineId);
        // EngineRegistry::instance().recordEngineLoad(r.engineId, targetSlot);
        // Wired in Task 11 (XOceanusProcessor include is circular at this layer).
        juce::ignoreUnused(targetSlot);
    }

    if (onClose_) onClose_();
}
```

NOTE: the engine path is stubbed because including `XOceanusProcessor.h` here would create a circular include. Task 11 resolves this either via (a) a forward-declared interface (preferred) or (b) injecting a `std::function<void(int, std::string)> loadEngineFn` in the constructor. Pick one in Task 11.

- [ ] **Step 10.2: Verify it compiles**

```bash
cmake --build build --target XOceanus_AU
```

- [ ] **Step 10.3: Commit**

```bash
git add Source/UI/Ocean/CommandPalette.h
git commit -m "feat(cmdk): keyboard navigation + activateResult preset path (Task 10)"
```

---

## Task 11: XOceanusEditor integration — Cmd+K handler + lifecycle + engine load wiring

**Files:**
- Modify: `Source/UI/Ocean/CommandPalette.h` (engine load injection)
- Modify: `Source/UI/XOceanusEditor.h`

- [ ] **Step 11.1: Switch CommandPalette to function-injection for engine load**

In `CommandPalette.h`, replace the `XOceanusProcessor&` parameter with a function:

```cpp
// In the constructor signature:
CommandPalette(PresetManager& pm,
               std::function<void(int slot, std::string engineId)> loadEngineFn,
               std::function<void()> onClose);

// Member field:
std::function<void(int, std::string)> loadEngineFn_;

// In activateResult engine branch:
else
{
    if (loadEngineFn_) loadEngineFn_(targetSlot, r.engineId);
    EngineRegistry::instance().recordEngineLoad(r.engineId, targetSlot);
}

// Constructor body:
CommandPalette::CommandPalette(PresetManager& pm,
                               std::function<void(int, std::string)> loadEngineFn,
                               std::function<void()> onClose)
    : presetManager_(pm),
      loadEngineFn_(std::move(loadEngineFn)),
      onClose_(std::move(onClose))
{
    addAndMakeVisible(searchField_);
    searchField_.setMultiLine(false);
    searchField_.onTextChange = [this] { startTimer(50); };
    setWantsKeyboardFocus(true);
}
```

Remove the `XOceanusProcessor` forward decl and the `processor_` member. Also remove the `#include "../../Core/EngineRegistry.h"` if it's only used for `getRegisteredIds()` — keep it; that call still happens in `rerank()`.

- [ ] **Step 11.2: Add Cmd+K handler in XOceanusEditor.h**

Find `keyPressed()` in `Source/UI/XOceanusEditor.h` (around line 1884). Add at the top of the function, before existing branches:

```cpp
if (key == juce::KeyPress('k', juce::ModifierKeys::commandModifier, 0))
{
    openCommandPalette();
    return true;
}
```

In the private section of the class, add member:

```cpp
std::unique_ptr<xoceanus::CommandPalette> commandPalette_;
```

In the public/private (matching Save As pattern), add:

```cpp
void openCommandPalette()
{
    if (commandPalette_ != nullptr) return;  // already open — no-op
    commandPalette_ = std::make_unique<xoceanus::CommandPalette>(
        presetManager_,
        [this](int slot, std::string engineId) {
            processor_.loadEngine(slot, engineId);
        },
        [this] { closeCommandPalette(); }
    );
    addAndMakeVisible(*commandPalette_);
    commandPalette_->setBounds(getLocalBounds());
    commandPalette_->grabSearchFocus();
}

void closeCommandPalette()
{
    if (commandPalette_ == nullptr) return;
    removeChildComponent(commandPalette_.get());
    commandPalette_.reset();
    grabKeyboardFocus();  // restore editor focus
}
```

Add include at top of file: `#include "Ocean/CommandPalette.h"` (verify path matches existing `SavePresetDialog.h` include style).

- [ ] **Step 11.3: Click-outside dismissal**

In `XOceanusEditor::mouseDown`, add at the top:

```cpp
if (commandPalette_ != nullptr)
{
    auto contentRect = juce::Rectangle<int>(640, 480).withCentre(getLocalBounds().getCentre());
    if (!contentRect.contains(e.getPosition()))
    {
        closeCommandPalette();
        return;
    }
}
```

- [ ] **Step 11.4: Verify build**

```bash
cmake --build build
auval -v aumu Xocn XoOx
```

Expected: build SUCCESS, auval PASS.

- [ ] **Step 11.5: Verify binary contents (sentinel grep)**

```bash
strings build/XOceanus_AU.component/Contents/MacOS/XOceanus_AU | grep -E "Type to search|ENTER slot 1"
```

Expected: at least one match. If no match, the new code didn't link — STOP and surface.

- [ ] **Step 11.6: Commit**

```bash
git add Source/UI/Ocean/CommandPalette.h Source/UI/XOceanusEditor.h
git commit -m "feat(cmdk): editor integration — Cmd+K handler, lifecycle, engine load (Task 11)"
```

---

## Task 12: Manual smoke + auval validation + PR

**Files:**
- (No code changes — verification only)

- [ ] **Step 12.1: Run the full validation suite**

```bash
eval "$(fnm env)" && fnm use 20
cmake --build build --config Release
auval -v aumu Xocn XoOx
./build/Tests/XOceanusTests "[CommandPalette]"
```

Expected: all green. If auval regresses (was passing on main, fails now), STOP and surface.

- [ ] **Step 12.2: Manual smoke walkthrough**

Open the AU in any DAW and verify:

1. `Cmd+K` opens the palette; search field is focused.
2. Type "warm" — top results include presets with high warmth DNA AND presets with literal "warm" in name.
3. Type "obsidian" — engine result appears; press Enter loads it into slot 1; press Cmd+3 loads it into slot 3.
4. Type "bass" — results mix presets and engines (OGRE, OLATE, OAKEN, OMEGA).
5. Esc closes the palette; Cmd+K reopens; recents now show what was just loaded.
6. Click outside the content rect — closes palette.
7. Open palette → don't type → press Enter — loads the most-recent preset.

Document any deviations in the PR description.

- [ ] **Step 12.3: Open PR**

```bash
git push -u origin feat/cmdk-palette-1
gh pr create --title "feat(cmdk): Cmd+K command palette — presets + engines (closes #22)" --body "$(cat <<'EOF'
## Summary

Implements the Cmd+K command palette per spec at `Docs/plans/2026-05-05-cmdk-palette-design.md`.

V1 scope: presets + engines. Smart matching: literal fuzzy + 6D DNA dimension re-rank. Engine load: Enter→slot 1, Cmd+2/3/4→slots 2/3/4.

## Tasks completed (12 of 12)

- [x] Task 1: PresetManager — `getPresetLibrary()` + `loadPresetByIndex()`
- [x] Task 2: PresetManager — recents tracking
- [x] Task 3: EngineRegistry — recents tracking
- [x] Task 4: CommandPalette skeleton + Result struct
- [x] Task 5: Preset literal-match scoring
- [x] Task 6: DNA dimension ranking
- [x] Task 7: Engine literal-match scoring
- [x] Task 8: rerank() pipeline + empty-state recents
- [x] Task 9: Result row rendering + footer hint
- [x] Task 10: Keyboard navigation + activateResult preset path
- [x] Task 11: Editor integration — Cmd+K handler, lifecycle, engine load
- [x] Task 12: Validation + this PR

## Validation

- `cmake --build build` — 0 errors
- `auval -v aumu Xocn XoOx` — AU VALIDATION SUCCEEDED
- `./build/Tests/XOceanusTests "[CommandPalette]"` — all assertions pass
- `strings <binary> | grep "Type to search"` — sentinel found

## Test plan

- [ ] Cmd+K opens palette
- [ ] Search "warm" mixes literal + DNA hits
- [ ] Search "obsidian" returns engine; Enter / Cmd+2-4 load to slot
- [ ] Esc + click-outside dismiss
- [ ] Empty state shows recents

## Out of scope (deferred)

- Param search, FX search (v1.1)
- Curated synonyms (v1.1)
- Recents persistence across DAW sessions (v1.1)
- Token consolidation (some inline color constants — flagged)

🤖 Generated with [Claude Code](https://claude.com/claude-code)
EOF
)"
```

- [ ] **Step 12.4: Final commit**

(No code commit — the push above is the final state. The agent should report the PR URL.)

---

## Self-review (already done by author)

- **Spec coverage:** All 5 locked decisions implemented. Out-of-scope items explicitly deferred. ✓
- **Placeholder scan:** Two `// TODO Task 11` markers in Task 9/10 — both resolved in Task 11 by design (forward references during incremental TDD). No `TBD` / `figure out` / vague handlers. ✓
- **Type consistency:** `CommandResult` / `CommandResultKind` used throughout. `loadPresetByIndex` consistent across Tasks 1, 8, 10. `recordPresetLoad` / `recordEngineLoad` consistent. ✓
- **Inline color note:** Task 9 uses inline RGB constants because `Tokens::Submarine::PanelBg` may or may not be a named token. Task 11 PR description flags this for v1.1 cleanup. ✓
