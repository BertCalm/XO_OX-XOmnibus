# #25 A/B Compare Diff — Design Spec & Implementation Plan

**Date:** 2026-05-07
**Author:** Day 6 Lane B brainstorm (V1 Ship Campaign)
**Issue:** [#25 (IL-5)](https://github.com/BertCalm/XO_OX-XOmnibus/issues/25)
**Brainstorm prep:** `~/.claude/projects/-Users-joshuacramblet/memory/session-W1-day6-abcompare-brainstorm-prep-2026-05-05.md`
**Day 5 close ledger:** `~/.claude/projects/-Users-joshuacramblet/memory/session-W1-day5-complete-2026-05-06.md`
**Status:** Spec/plan locked. Impl pending dispatch in isolated worktree.

---

## What is #25?

IL-5 ("A/B compare with diff highlight") was STOP-gated at the end of the
Me-First Campaign session 2D. The remaining work is the **diff visualization
layer** — the swap infrastructure (button, component, editor wiring, full
state capture/restore) is already shipped in `Source/UI/Gallery/ABCompare.h`.

The goal of this PR's impl follow-up: when the user has captured A and B
states, knobs whose value differs between A and B should be visually flagged.

---

## Decision table (locked verbatim by user 2026-05-07)

| Q  | Decision | Letter | Rationale |
|----|----------|--------|-----------|
| Q1 | Diff scope: which params? | **D** | On-screen params only, no MemoryBlock deserialization. Capture `apvts.getRawParameterValue()` snapshots into `std::vector<float>` at A/B toggle, diff at draw time. |
| Q2 | Diff trigger: when recalculated? | **B** | Snapshot diff on A/B toggle. Static until next toggle. No live timer, no APVTS listener. |
| Q3 | Diff visualization: how marked? | **A** (with C fallback) | Colored glow ring 2px outside knob track, `Tokens::Color::Warning` (amber). Hover tooltip "A: 0.42 / B: 0.78". **Caveat:** if `GalleryKnob` paint() clips tightly to its bounding rect, fall back to **Q3-C** (color tint on knob fill) and file V1.1 issue for ring widen. Implementing sonnet must audit GalleryKnob geometry before locking the ring approach. |
| Q4 | A vs B mutability: can promote? | **A** | A is reference, B is comparison. No promote / swap / independent-set buttons. Document semantics in tooltip. |
| Q5 | Persistence: across sessions? | **B** (with schema guard) | Persist stateA / stateB / abActive flag via `getStateInformation` / `setStateInformation`. Bump `stateVersion` from 3 → 4. Read inside `if (stateVersion >= 4)` guard. |

---

## File touchpoints (labeled per Day 3 lesson #8)

| File | Status | Role |
|------|--------|------|
| `Source/UI/Gallery/ABCompare.h` | **MUST EXIST** | Header-only A/B widget. Add float snapshot capture + diff API. Q5 persistence hooks. |
| `Source/UI/Gallery/GalleryKnob.h` | **MUST EXIST** | Audit `paint()` for outer-ring clearance; add `setDiffActive(bool, float aVal, float bVal)` API; paint glow ring (Q3-A) or fill tint (Q3-C). |
| `Source/UI/Gallery/EngineDetailPanel.h` | **MUST EXIST** | Iterate visible knobs at A/B toggle, capture per-param `getRawParameterValue()` snapshots, compute diff vs other slot, push to each `GalleryKnob` via new diff API. |
| `Source/UI/Gallery/MacroHeroStrip.h` | **MUST EXIST** | Optional: 4 macro knobs are part of the "on-screen params" envelope per Q1-D. Same diff API call as EngineDetailPanel knobs. |
| `Source/UI/Gallery/ParameterGrid.h` | **MUST EXIST** | Already has `findKnobForParam()` (added in #1532). Reuse for per-paramID lookup during diff dispatch. |
| `Source/XOceanusProcessor.cpp` | **MUST EXIST** | Q5: extend `getStateInformation` to append A/B blobs + abActive bool inside a new `<ABCompareState/>` child element; bump `stateVersion` 3→4. `setStateInformation` reads gated on `stateVersion >= 4`. |
| `Source/XOceanusProcessor.h` | **MUST EXIST** | Q5: declare friend access or public getters for ABCompare's stateA/stateB MemoryBlocks if needed (currently private to ABCompare). Likely cleaner: the persistence loop lives on the editor side and calls into ABCompare via new accessor methods. See pseudocode. |
| `Source/UI/XOceanusEditor.h` | **MUST EXIST** | Wire diff dispatch on A/B toggle: when `oceanView_.onABCompareToggled` fires, after `abCompare.setABActive(...)`, ask ABCompare for both snapshots and dispatch to EngineDetailPanel/MacroHeroStrip for highlighting. |
| `Source/UI/Gallery/Tokens.h` | **REFERENCE** | Reuse existing `Tokens::Color::Warning` (amber) for the glow ring. No new tokens. |
| `Docs/plans/2026-05-04-save-as-design.md` | **REFERENCE** | Format reference only — schema for spec docs. Do not edit. |

---

## Pseudocode

### Q1-D: snapshot capture (in `ABCompare.h`)

```cpp
// New private state: parallel float snapshots alongside the MemoryBlocks
std::vector<std::pair<juce::String, float>> snapshotA;  // (paramID, value)
std::vector<std::pair<juce::String, float>> snapshotB;

// Called from captureState() in addition to MemoryBlock capture
void captureFloatSnapshot(std::vector<std::pair<juce::String, float>>& dest)
{
    dest.clear();
    auto& apvts = processor.getAPVTS();
    // Iterate every APVTS parameter once. For V1, snapshot ALL params, not just
    // currently-visible ones — this handles the Q1-D edge case where preset
    // load between A and B changes the active engine slot.
    for (auto* p : apvts.processor.getParameters())
    {
        if (auto* withID = dynamic_cast<juce::AudioProcessorParameterWithID*>(p))
            dest.emplace_back(withID->paramID, withID->getValue());
    }
}

// Public API for editor to read both snapshots after toggle
const auto& getSnapshotA() const { return snapshotA; }
const auto& getSnapshotB() const { return snapshotB; }
```

### Q3-A primary: glow ring (in `GalleryKnob.h`)

```cpp
// New API
void setDiffActive(bool active, float aVal = 0.0f, float bVal = 0.0f)
{
    auto& props = getProperties();
    const bool wasActive = (bool)props["diffActive"];
    if (active != wasActive)
    {
        props.set("diffActive", active);
        props.set("diffAVal", aVal);
        props.set("diffBVal", bVal);
        repaint();
    }
}

// In paint() — added after existing badge ring draw, before knob fill
if ((bool)getProperties()["diffActive"])
{
    // PRIMARY (Q3-A): 2px ring 2px outside knob track
    auto knobBounds = getKnobBoundsFloat();   // existing helper
    auto outerBounds = knobBounds.expanded(2.0f);
    if (getLocalBounds().toFloat().contains(outerBounds))   // geometry audit
    {
        g.setColour(Tokens::Color::Warning);
        g.drawEllipse(outerBounds, 2.0f);
    }
    else
    {
        // FALLBACK (Q3-C): tint the knob fill amber instead
        // (set a flag; the existing fill draw path reads it)
        // Tagged here for V1.1 ring-widen issue.
    }
}
```

**Geometry audit step (sonnet must execute before locking Q3-A):**
1. Read `GalleryKnob::paint()` (or its LookAndFeel equivalent) and identify
   the maximum extent drawn. If the rotary slider draws to `getLocalBounds()`
   minus a known inset (e.g. 2px), then `expanded(2.0f)` is borderline.
2. Probe with a 1px expanded ring first; if no clipping artifact, widen to
   2px. If clipping observed at any expansion, switch to Q3-C fallback for
   V1 and file V1.1 issue (`[v1.1] Widen GalleryKnob bounds for diff ring overlay`).

### Q5: persistence (in `XOceanusProcessor.cpp`)

```cpp
// In getStateInformation() — after existing children are appended
auto* abChild = xml->createNewChildElement("ABCompareState");
abChild->setAttribute("active", abCompareActive);  // bool, mirrored from editor
// stateA / stateB blobs — base64-encoded MemoryBlocks
juce::MemoryBlock stateABlob, stateBBlob;
editor->getABCompareStates(stateABlob, stateBBlob);  // new editor method
abChild->setAttribute("stateA", stateABlob.toBase64Encoding());
abChild->setAttribute("stateB", stateBBlob.toBase64Encoding());

xml->setAttribute("stateVersion", 4);   // BUMP from 3 → 4

// In setStateInformation() — within the existing version-guarded block
if (stateVersion >= 4)
{
    if (auto* abChild = xml->getChildByName("ABCompareState"))
    {
        const bool active = abChild->getBoolAttribute("active", false);
        juce::MemoryBlock blobA, blobB;
        blobA.fromBase64Encoding(abChild->getStringAttribute("stateA"));
        blobB.fromBase64Encoding(abChild->getStringAttribute("stateB"));
        // Defer to editor: only restore if editor exists; else cache in
        // pendingABCompareState_ for editor construction.
        if (editor != nullptr)
            editor->restoreABCompareStates(active, blobA, blobB);
        else
            pendingABCompareState_ = {active, std::move(blobA), std::move(blobB)};
    }
}
```

**Schema guard contract:** any future breaking change to the ABCompareState
child element MUST bump stateVersion to 5 and add a parallel guard. The
`stateVersion >= 4` block is read-only stable from this PR onward.

---

## Acceptance criteria

The impl PR is acceptable if and only if:

- [ ] cmake Release build green on macOS arm64
- [ ] auval PASS at 44.1k, 48k, 96k sample rates
- [ ] Binary sentinel: `nm` of the built AU binary contains a unique
      symbol from new code (e.g. `setDiffActive` or `getABCompareStates`)
- [ ] Diff cap: total wiring ≤ **125 lines** (brainstorm estimate 50–100 × 1.25
      buffer per Day 5 lesson on plan-vs-actual inflation). STOP and surface
      to user if exceeded.
- [ ] Smoke: load any preset → click A → tweak 4 knobs → click B → 4 amber
      rings (or fill tints if Q3-C fallback fired) appear on the 4 changed
      knobs. Click A → rings persist on the now-restored A state's view of
      those knobs (since they differ from B). Click currently-active button
      → A/B mode exits, all rings clear.
- [ ] Smoke: persistence — capture A and B in DAW session, save session,
      reload session, verify A/B mode still active and stateA/stateB still
      restorable.
- [ ] No new compiler warnings introduced
- [ ] `nm`-grep sentinel from the AU binary, captured in PR body

---

## Surprises documented up front (so impl doesn't blow scope)

1. **Q3-A geometry risk:** the prep memo flagged this. Sonnet has explicit
   authority to fall back to Q3-C without escalation if the geometry audit
   finds clipping. Tag the V1.1 widen issue in the PR body.

2. **APVTS param iteration:** the existing `ABCompare.h` calls
   `processor.getStateInformation(dest)` — an opaque blob. The new float
   snapshot path needs `processor.getAPVTS()` access. Verify accessor exists
   (the editor uses `processor.apvts` member directly already, so this is
   already public).

3. **Editor ↔ ABCompare coupling for Q5:** the existing ABCompare encapsulates
   stateA/stateB as private members. Persistence requires either (a) public
   accessors on ABCompare (cleanest) or (b) editor-level pending state buffer
   (`pendingABCompareState_`) that ABCompare reads from on construction. The
   pseudocode above takes path (a). Sonnet must verify ABCompare construction
   order: it constructs in the editor's member init list; the pending-state
   buffer must live on the *processor*, not the editor, since
   `setStateInformation` runs before the editor exists in some DAW lifecycles.

4. **`stateVersion` already at 3:** prior bumps in v2 (TideWaterlineSteps) and
   v3 (unknown — verify by reading the existing version checks). The 3→4
   bump is mechanical but the new `<ABCompareState/>` child placement must
   not collide with existing children. Read the full `getStateInformation`
   body before adding the child to identify the right insertion point.

5. **`abCompareActive` mirroring:** the editor's `oceanView_.onABCompareToggled`
   maintains UI state. For Q5 persistence, the processor must observe this
   bool — either via a public `bool abCompareActive` member set by the editor
   on toggle, or via `editor->isABCompareActive()` accessor at serialization
   time. Pseudocode assumes the latter.

---

## Diff cap

Brainstorm estimate: **50–100 lines** (per Day 5 close ledger).
Cap (× 1.25 inflation buffer): **125 lines**.

If sonnet impl exceeds 125 lines including pseudocode realization, STOP and
surface to user with the cause. Per Day 5 lesson: prefer V1.1 follow-up issues
over V1 scope creep when hitting framework hardcodes.

---

## Out of scope (V1.1 candidates pre-flagged)

- **Promote B to A** (Q4-B), **swap A↔B** (Q4-D), **independent slot capture**
  (Q4-C) — Q4 locks A=reference / B=comparison for V1.
- **Live deviation meter** (Q2-A) or **APVTS listener auto-refresh** (Q2-C) —
  Q2-B locks static-until-next-toggle for V1.
- **MemoryBlock-deserialized full diff** (Q1-A/B/C) — Q1-D locks float-snapshot
  diff for V1.
- **Auto-saved preset files** (Q5-C) or **named A/B snapshot pairs** (Q5-D) —
  Q5-B locks DAW-session-only persistence for V1.
- **Side-drawer diff list** (Q3-D) or **value badge** (Q3-B) — Q3-A locks
  glow ring (with Q3-C fallback) for V1.

---

## Sentinel for "spec/plan PR closed"

- This file shipped at `Docs/plans/2026-05-07-abcompare-diff-design.md`
- PR open with title `docs(plan): #25 A/B compare diff design (Day 6 Lane B brainstorm)`
- PR body references the brainstorm prep memo + 5 locked decisions
