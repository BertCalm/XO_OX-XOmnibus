# Wave 9: Onboarding & Discoverability

> **Status**: SPEC LOCKED (design). Implementation pending Wave 7 (OceanStateMachine).
> **Date**: 2026-04-26 | **Owner**: Wave 9 Engineering
> **Blocks on**: Wave 7 (OceanView decomp — XOceanusEditor cold-start path moves into OceanStateMachine)
> **Parallel track**: 9b (tooltip content) can start immediately; 9a and 9c wait for Wave 7.

---

## Sub-phases

| Phase | Name | Depends on | Can start |
|-------|------|-----------|-----------|
| 9a | Sound on First Launch wiring | Wave 7 (OceanStateMachine) | After Wave 7 |
| 9b | Tooltip content fill | Nothing (pure content) | NOW |
| 9c | First-hour walkthrough modal | Wave 7 (OceanStateMachine) | After Wave 7 |

---

## 1. Sound on First Launch Wiring

Sound spec is LOCKED at `Docs/specs/sound-on-first-launch.md`. This section covers the implementation wiring only.

### 1.1 Persistence — LOCKED: PropertiesFile in application data directory

XOceanus already uses `juce::PropertiesFile` (app name: "XOceanus", suffix: "settings", folder: "Application Support") in `SettingsPanel.h`. Onboarding state lives in the **same file** — no second file, no APVTS pollution.

Keys to add:

```
hasPlayedFirstLaunchSound   bool  (default: false)
onboardingWalkthroughStep   int   (default: -1, -1 = not started, 0–9 = last completed)
onboardingDisabled          bool  (default: false)
```

Detection: `settingsFile->getBoolValue("hasPlayedFirstLaunchSound", false)`. If false, it is the first cold launch ever. After playback completes (or user interaction interrupts), set to true and call `settingsFile->saveIfNeeded()`.

### 1.2 Trigger location — LOCKED: XOceanusEditor post-init, after Wave 7

Currently, cold-start logic runs in `XOceanusEditor::initOceanView()` (Phase 6, last constructor helper). After Wave 7 decomposes OceanView into OceanChildren + OceanLayout + OceanStateMachine, the first-breath trigger belongs in `OceanStateMachine::onEditorReady()` — a dedicated callback fired once the editor hierarchy is stable and visible. This avoids the message-thread timing issues that arise from firing inside the constructor.

**Before Wave 7 (interim path)**: trigger from `XOceanusEditor::visibilityChanged()` when `isVisible() == true` and the `hasPlayedFirstLaunchSound` flag is false. Guard with `juce::MessageManager::callAsync` to defer past the constructor.

Do NOT trigger from `AudioProcessor::prepareToPlay` — the audio thread has no access to UI state and the editor may not exist yet.

### 1.3 Audio playback implementation — LOCKED: BinaryData + background decode

Per spec Section 10: pre-rendered 48 kHz / 24-bit stereo WAV embedded as BinaryData.

Implementation steps:
1. Add `Source/Assets/Audio/first_breath.wav` to `CMakeLists.txt` `juce_add_binary_data(XOceanusAssets ...)`.
2. On trigger, spawn a `juce::Thread` (or use `juce::ThreadPool`): decode BinaryData with `juce::MemoryInputStream` + `juce::WavAudioFormat::createReaderFor`. Cache decoded `juce::AudioBuffer<float>` in a `std::shared_ptr` stored on the editor. Do not block the message thread.
3. Play via `juce::AudioTransportSource` or a dedicated `AudioSourcePlayer` attached to the plugin's main audio output (Option A per spec: DAW bus, not system audio).
4. JUCE resampler handles host SR differences automatically via `juce::ResamplingAudioSource`.
5. Interrupt: if user clicks any interactive control before 12 s, call `transportSource.stop()` and fade out over 200 ms using a linear gain ramp on the message thread.

### 1.4 Repeatability — DECISION REQUIRED (Option A recommended)

Spec Section 8 leaves the A/B choice open. Recommendation: **Option A** — plays once, ever. Add a "Hear the Greeting Again" button to Settings > Experience section. This satisfies discoverability without cheapening the first-launch moment.

Settings panel addition: one `juce::TextButton` labeled "Hear the Greeting Again" that calls `settingsFile->setValue("hasPlayedFirstLaunchSound", false)` + triggers playback immediately.

### 1.5 Volume management

The greeting targets −12 dBFS ceiling (baked into the WAV). No host gain multiplication. No normalization. Plugin output is post-fader so host master gain applies naturally. No separate channel needed.

### 1.6 No MIDI output

The greeting fires no `MidiMessage`. It must not set `MidiBuffer` contents during its playback window. Guard: `XOceanusProcessor::processBlock` should short-circuit MIDI output while `greetingActive_` is true.

---

## 2. Tooltip Content Strategy

### 2.1 Current state

Tooltip framework landed in Wave 2: `juce::TooltipWindow tooltipWindow{this, 400}` in `XOceanusEditor.h` (line 2366). This activates ALL `setTooltip()` calls across the component tree. The 400ms delay matches standard plugin UX.

Grep count: **70 lines** contain `setTooltip` or `TooltipClient` in `Source/` (excluding worktrees). Of those, roughly 55 are active `setTooltip("...")` calls with content already populated. Most content is terse functional ("Previous preset", "Toggle favorite"). Several are `"Coming in V1.1"` placeholders.

### 2.2 Surfaces to cover (canonical list)

**Already populated (no action needed):**
- Header buttons: engines, chord machine, performance, cinematic, PlaySurface, theme, register lock, prev/next preset, settings, export
- OceanView toolbar: engines button, fav, settings, keys, prev/next preset
- CompactEngineTile: engine name + slot empty state
- PerformanceViewPanel: bake, clear, coupling preset box, route depth sliders, macro knobs
- MasterFXSection: all knobs and buttons dynamically
- ExportDialog: strategy, vel layers, bit depth, sample rate, entangled toggle, sound shape
- PresetBrowserStrip: prev, next, browse, favorite
- MacroSection: Macro 1–4 + VOLUME master
- DnaMapBrowser: dive button
- EnginePickerPopup: category buttons (kCatTooltips array)
- DnaHexagon: DNA dimensions
- SettingsPanel: disabled toggles (V1.1 placeholders are accurate)
- WavetableEditor (Future/): morph slider, prev/next frame, normalize, generate

**Surfaces needing richer content (9b targets):**

| Surface | Current content | Recommended content |
|---------|----------------|---------------------|
| `EngineOrbit` buoys | engineId string | "{EngineName} — {one-line engine identity}" |
| `StatusBar` status indicators | varies | "MIDI activity", "CPU load", "Audio dropout" per slot |
| `MacroHeroStrip` pillars | foundNames[i] | "{MacroName}: {what it sweeps, e.g. 'filter brightness + resonance'}" |
| `OutshineZoneMap` | TooltipClient but no setTooltip found | "XPN velocity zone map — drag boundaries to reshape dynamics" |
| `DrumPadGrid` knobs | `rp->getName(64)` | Already param names — acceptable |
| `ObrixDetailPanel` knobs | `rp->getName(64)` | Already param names — acceptable |
| Settings > Experience section | n/a (not yet built) | "Hear the Greeting Again", "Show walkthrough on next launch" |

### 2.3 Voice and tone — LOCKED: mixed register

- Navigation controls (prev, next, toggle): **terse functional** — 2–4 words max. "Previous preset" is correct.
- Feature controls (macros, coupling, DNA): **terse functional + one parenthetical descriptor** where needed. "Macro 2: MOVEMENT — sweeps LFO rate and coupling depth."
- Engine identity (EngineOrbit buoys): **evocative one-liners** — these are brand touchpoints. "Oxbow — Oscar-pole resonator. The spine of deep water." Max 60 chars.
- Disabled/future features: **honest and brief** — "Coming in V1.1" is correct, not aspirational.

### 2.4 Length budget

- Navigation buttons: ≤ 30 chars
- Feature knobs and sliders: ≤ 60 chars (JUCE tooltip wraps at ~200px; aim for one line)
- Engine buoys: ≤ 80 chars (two short lines acceptable)
- Settings items: ≤ 80 chars

### 2.5 Authoring approach

Sub-phase 9b is pure content authoring, no Wave 7 dependency. Dispatch as a standalone content pass:

1. Run a grep to collect every `setTooltip("...")` call with its file + context.
2. Fill EngineOrbit buoys first (highest user-facing impact — visible on first launch for all 86 engines). Pattern: `setTooltip(engineId + " — " + engineTagline)`. Engine taglines are available from `Docs/reference/engine-color-table.md` and Seance verdicts.
3. Fill StatusBar slots with accurate descriptions per status indicator type.
4. Fill MacroHeroStrip with sweep descriptions derived from the macro definitions.
5. File followup issue for any surfaces requiring new `setTooltip` calls not yet wired.

---

## 3. First-Hour Walkthrough

### 3.1 Trigger — LOCKED: same flag as Sound on First Launch, opt-in

The walkthrough is NOT forced. After the 12-second greeting completes (or is dismissed), a single non-modal prompt appears: "Want a quick tour? (2 min)" with [Take the Tour] and [Skip] buttons. Skip sets `onboardingDisabled = true`. Take the Tour sets `onboardingWalkthroughStep = 0`.

Re-launchable from Settings > Experience > "Restart Walkthrough" — sets `onboardingWalkthroughStep = 0` and `onboardingDisabled = false`.

### 3.2 Format — LOCKED: floating spotlight bubbles, not modal

Modal sequences block interaction. The walkthrough uses **floating `juce::CalloutBox`-style bubbles** that point at the target control. The target control is highlighted with a 2px XO Gold ring painted over it. All other UI remains interactive — the user can click away at any step.

Each bubble:
- Title: short (≤ 6 words)
- Body: 1–2 sentences, conversational, not promotional
- Footer: "[step X of 8]  [Skip all]  [Next →]"

### 3.3 Steps — LOCKED: 8 steps

| Step | Title | Target | Body |
|------|-------|--------|------|
| 0 | "Press anything" | PlaySurface | "XOceanus makes sound on first touch. These pads, keys, or frets are all the same instrument." |
| 1 | "Meet your engines" | CompactEngineTile slot 0 | "Each slot holds one engine. Right now it is Odyssey. Hover it to see what it is." |
| 2 | "The four macros" | MacroSection knob 0 (CHARACTER) | "CHARACTER sweeps the engine's core color — brightness, grit, or breath. Try it." |
| 3 | "Browse the ocean" | DnaMapBrowser or PresetBrowserStrip | "Thousands of presets, organized by mood. Dive picks a random visible one." |
| 4 | "Couple two engines" | EngineOrbit buoy (slot 1 or 2) | "Load a second engine and the coupling arc appears. Drag the arc to wire them together." |
| 5 | "The chord machine" | cmToggleBtn | "Press this to open the chord machine — generative harmonic structure, no theory required." |
| 6 | "Save your first preset" | PresetBrowserStrip favBtn | "Favorite this preset so it appears in your personal collection. Your changes persist." |
| 7 | "XOuija" | XOuija panel or OceanView XOuija button | "XOuija is a live improvisation interface. Move a cell to shift pitch, coupling depth, and character simultaneously." |

Each step is skippable individually. Closing the plugin mid-tour persists `onboardingWalkthroughStep` so it resumes at the same step on next launch (unless `onboardingDisabled` is true).

### 3.4 Skip mechanism — LOCKED: every step has [Skip all]

Clicking [Skip all] at any step sets `onboardingDisabled = true` and `onboardingWalkthroughStep = 8` (completed). The bubble disappears immediately. No confirmation dialog.

---

## 4. Persistence (PropertiesFile, not APVTS)

APVTS is for audio parameters saved in DAW sessions. Onboarding state is per-installation, not per-session. It lives in the `juce::PropertiesFile` ("XOceanus.settings") alongside dark mode and CPU meters preferences.

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `hasPlayedFirstLaunchSound` | bool | false | Set to true after playback completes or is interrupted |
| `onboardingWalkthroughStep` | int | -1 | -1 = not started; 0–7 = last completed step; 8 = fully done |
| `onboardingDisabled` | bool | false | True if user pressed Skip All at any step |

These keys are read once in `SettingsPanel` constructor (pattern matches existing keys). Write on change, `saveIfNeeded()` after each write.

---

## 5. Integration with Wave 7

Wave 7 decomposes OceanView (2532 lines) into OceanChildren, OceanLayout, and OceanStateMachine. The cold-start trigger (3a wiring) must land in `OceanStateMachine::onEditorReady()`, not in `XOceanusEditor`'s constructor or `initOceanView()`. The walkthrough coordinator (9c) also attaches to OceanStateMachine events:

- `onEditorReady` → check `hasPlayedFirstLaunchSound`, fire greeting
- `onGreetingComplete` → check `onboardingDisabled`, show tour prompt if false
- `onWalkthroughStep(int step)` → advance or terminate walkthrough

Do not implement 9a or 9c against the current monolithic OceanView — it will be refactored out. The interim `visibilityChanged()` path (section 1.2) is acceptable for prototyping only.

---

## 6. Community Dimension

Barry OB's team (per Khan Sultan's standing recommendation) should review the 8-step walkthrough copy (section 3.3) before final implementation. Loop-in checklist:
- [ ] Share step copy with Barry OB's team as a Google Doc / Notion draft
- [ ] Collect one round of feedback (aim for 3–5 external users, not internal only)
- [ ] Final copy locked before 9c PR opens

Feedback target: tone, step ordering, missing surfaces. Not: technical implementation.

---

## 7. Implementation Estimates

| Phase | Effort | Complexity | Risk | Notes |
|-------|--------|-----------|------|-------|
| 9a: Sound on First Launch wiring | 1–2 days | Low-Medium | Medium | BinaryData embed is straightforward; timing/thread safety around `visibilityChanged` needs care. Risk: AudioTransportSource lifecycle in plugin context (AU sandbox). |
| 9b: Tooltip content fill | 0.5 day (1st pass) + 0.5 day (EngineOrbit taglines) | Low | Low | Pure content, no arch deps. EngineOrbit taglines require reading 86 seance verdicts. Can be parallelized across engine groups. |
| 9c: Walkthrough modal | 2–3 days | Medium | Medium | CalloutBox positioning in a plugin editor window is tricky — JUCE CalloutBox assumes desktop-level bounds; may need custom implementation. Step targeting requires stable component IDs post-Wave 7. |
| **Total** | **4–6 days** | — | — | Assumes Wave 7 complete before 9a/9c begin. 9b is fire-and-forget parallel. |

---

## 8. Open Decisions (require user input before implementation)

| # | Decision | Options | Recommendation |
|---|----------|---------|---------------|
| D1 | Repeatability (spec Section 8) | A: once ever; B: every cold boot | **Option A** — "Hear the Greeting Again" in Settings |
| D2 | Audio routing | A: DAW bus; B: system audio | **Option A** — simpler, no platform permissions |
| D3 | Walkthrough opt-in timing | A: prompt after greeting; B: always show on first launch | **Option A** — less aggressive |
| D4 | Barry OB copy review | async (before PR) or sync (before spec lock) | Before 9c PR, async with issue-gated merge |
