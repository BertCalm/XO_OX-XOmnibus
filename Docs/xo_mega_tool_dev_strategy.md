# XO_OX Mega-Tool Development Strategy

**Version:** 1.0
**Author:** XO_OX Designs
**Date:** 2026-03-08
**Status:** DECISION DOCUMENT -- Ready for execution
**Depends on:** `xo_mega_tool_feasibility.md` (Hybrid A+D architecture selected)

---

## 1. Executive Summary

**Recommendation: Option C -- Interface-First Hybrid Approach.** Define the `SynthEngine` interface and shared systems immediately, then retrofit each existing XO_OX instrument to comply with that interface while continuing independent development. This is the only path that lets individual synths ship as standalone products on their own timelines while making the eventual mega-tool merge a mechanical integration step rather than a rewrite. The mega-tool MVP ships with 2 engines (OddfeliX/OddOscar + XOverdub), a shared PlaySurface, and the coupling matrix -- delivering the "preset monster, multi synth concept tool" vision in 14 weeks.

---

## 2. Strategy: The Interface-First Hybrid Approach

### 2.1 Why Option C Wins

Three options were evaluated. Here is why two of them lose.

**Option A (Ship individually, merge later) fails** because every week of independent development increases architectural drift. Today, each project uses a different internal pattern:

| Project | Engine Class | Namespace | Param Pattern | FX Ownership |
|---------|-------------|-----------|---------------|-------------|
| OddfeliX/OddOscar | `EngineX` / `EngineO` | `XO::` | APVTS direct | Processor owns FX |
| XOverdub | `VoiceEngine` | `xoverdub::` | `ParamSnapshot` struct | Processor owns send/return FX |
| XObese | `SynthEngine` | `xobese::` | `SynthParams` struct | Engine owns `FXChain` |
| XOppossum | `SynthEngine` | (none / global) | `Params::ParamSnapshot` | Engine owns FX chain |
| XOdyssey | (monolithic processor) | (none) | APVTS + `ParamSnapshot` | Processor owns FX |
| XOblong | (monolithic processor) | (none) | APVTS direct | Processor owns FX |

Six different architectural patterns across seven projects. Each month of parallel development adds new divergence. Merging six divergent codebases after they all reach v1.0 is a rewrite, not an integration.

**Option B (Merge now) fails** because it blocks all individual product shipments. XOverdub is protocols-complete. OddfeliX/OddOscar has 114 factory presets. XObese has a build. None of these should wait for a unified codebase before reaching users. Blocking shipping products to build infrastructure is how projects die.

**Option C (Interface-first hybrid) wins** because it captures the benefits of both:

1. **Interface defined now** -- Every engine knows what shape it needs to be. No more drift.
2. **Engines ship independently** -- Each synth wraps its existing internals behind the interface. A thin adapter layer is all that is needed. Standalone AU/VST3 targets remain unchanged.
3. **Merge is mechanical** -- Once an engine implements `SynthEngine`, plugging it into the hub shell is registration, not rewriting.
4. **Coupling morphing preserved** -- The interface explicitly supports `getSampleForCoupling()` and `applyCouplingInput()`, keeping the XO_OX brand differentiator at the protocol level.

### 2.2 The Core Insight

The existing OddfeliX/OddOscar architecture is already a miniature mega-tool. It has:
- Two engines (`EngineX`, `EngineO`) with independent voice management
- A coupling matrix (`CouplingMatrix`) for cross-engine modulation
- A shared FX rack (DubDelay, LushReverb, Phaser, LoFi, MasterCompressor)
- A monolithic processor that calls each engine inline

The mega-tool is OddfeliX/OddOscar's architecture scaled to N engines. The `SynthEngine` interface formalizes what `EngineX` and `EngineO` already do informally.

---

## 3. Phase 1: Interface & Shared Systems (Weeks 1-2)

### 3.1 Define the SynthEngine Interface

Create the canonical interface at:
```
~/Documents/GitHub/OddfeliX/OddOscar/Source/Shared/SynthEngine.h
```

```cpp
#pragma once
#include <JuceHeader.h>

namespace xo {

/// Coupling modulation types for cross-engine interaction
enum class CouplingType
{
    AmpToFilter,     // Engine amplitude -> target filter cutoff
    LFOToPitch,      // Engine LFO -> target pitch (semitones)
    EnvToMorph,      // Engine envelope -> target wavetable/morph position
    AudioToFM,       // Engine audio output -> target osc FM input
    AudioToRing,     // Engine audio * target audio (ring mod)
    FilterToFilter   // Engine filter output -> target filter input (serial cascade)
};

/// Metadata describing an engine's identity and capabilities
struct EngineDescriptor
{
    juce::String engineID;          // e.g., "xoddcouple_x", "xoverdub"
    juce::String displayName;       // e.g., "XOverdub"
    juce::Colour brandColour;       // Engine's signature colour
    int defaultVoiceCount;          // Typical polyphony
    int parameterCount;             // Total owned parameters
    bool supportsDrumMode;          // True for XOnset
};

/// The contract every engine must fulfill to participate in the mega-tool.
class SynthEngine
{
public:
    virtual ~SynthEngine() = default;

    /// Return this engine's identity and capabilities
    virtual EngineDescriptor getDescriptor() const = 0;

    /// Called once when sample rate or block size changes
    virtual void prepare(double sampleRate, int maxBlockSize) = 0;

    /// Reset all voice and FX state (panic)
    virtual void reset() = 0;

    /// Render audio into the provided buffer. MIDI is pre-split per engine.
    virtual void renderBlock(juce::AudioBuffer<float>& buffer,
                             juce::MidiBuffer& midi,
                             int numSamples) = 0;

    /// Return the most recent output sample for cross-engine coupling.
    /// Called once per sample in the coupling matrix loop.
    virtual float getSampleForCoupling() const = 0;

    /// Apply an incoming coupling modulation from another engine.
    /// Called before renderBlock when coupling is active.
    virtual void applyCouplingInput(float value, CouplingType type) = 0;

    /// Return all parameter IDs this engine owns (namespaced).
    /// Used by the hub to register with APVTS.
    virtual juce::StringArray getParameterIDs() const = 0;

    /// Create the parameter layout for this engine's owned parameters.
    /// The hub merges these into a single APVTS.
    virtual juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout() const = 0;

    /// Snapshot parameters from the APVTS into internal state.
    /// Called once per processBlock before renderBlock.
    virtual void snapshotParameters(
        const juce::AudioProcessorValueTreeState& apvts) = 0;

    /// Handle note on/off routed to this engine
    virtual void noteOn(int midiNote, float velocity) = 0;
    virtual void noteOff(int midiNote) = 0;
    virtual void allNotesOff() = 0;

    /// Whether this engine manages its own FX chain or uses the shared rack
    virtual bool ownsEffects() const { return false; }
};

} // namespace xo
```

**Key design decisions:**

1. `renderBlock` takes a buffer + sample count, not the full `processBlock` signature. The hub handles MIDI splitting and routing.
2. `getSampleForCoupling()` is const and returns the latest sample. This enables per-sample cross-engine modulation when needed.
3. `createParameterLayout()` lets each engine define its own parameters. The hub merges them with namespaced prefixes.
4. `ownsEffects()` defaults to false. Engines that have specialized FX (XOverdub's send/return architecture) override this.

### 3.2 Build Shared PlaySurface Component

Location: `~/Documents/GitHub/OddfeliX/OddOscar/Source/Shared/UI/PlaySurface.h`

The PlaySurface is the unified playing interface described in `xo_pad_surface_spec.md`, extended with three modes:

| Mode | Grid Layout | Expression Mapping | Used By |
|------|------------|-------------------|---------|
| **Pad** | 4x4 scale-locked notes | X=engine blend, Y=expression | OddfeliX/OddOscar, XOppossum, XOdyssey, XOblong |
| **Fretless** | Continuous pitch strip | X=pitch, Y=expression | XOverdub, XObese |
| **Drum** | 8-pad kit layout | X=blend per voice, Y=decay | XOnset |

The PlaySurface emits three signals per touch:
1. MIDI note number + velocity
2. Engine blend value (0.0 - 1.0)
3. Expression value (0.0 - 1.0)

Each engine interprets these through its own mapping table.

### 3.3 Build Shared Preset Browser Shell

Location: `~/Documents/GitHub/OddfeliX/OddOscar/Source/Shared/UI/PresetBrowser.h`

Unified preset format (`.xomega` JSON):

```json
{
    "schema_version": 1,
    "mega_tool_version": "1.0.0",
    "preset_name": "Dub Pressure",
    "author": "XO_OX",
    "category": "Entangled",
    "tags": ["dub", "coupled", "dark"],
    "engines": [
        {
            "engine_id": "xoddcouple",
            "slot": 0,
            "active": true,
            "parameters": {
                "xoddcouple_xFilterCutoff": 800.0,
                "xoddcouple_oMorph": 0.7
            }
        },
        {
            "engine_id": "xoverdub",
            "slot": 1,
            "active": true,
            "parameters": {
                "xoverdub_sendAmount": 0.6,
                "xoverdub_delayTime": 0.375
            }
        }
    ],
    "coupling": {
        "pairs": [
            {
                "source": 0,
                "target": 1,
                "type": "AmpToFilter",
                "amount": 0.35
            }
        ]
    },
    "shared_fx": {
        "reverb_mix": 0.3,
        "delay_mix": 0.0
    }
}
```

The browser supports three views:
1. **By Engine** -- Show all presets from a single engine (standalone browsing)
2. **By Vibe** -- Cross-engine tag-based search ("dub", "cinematic", "bass")
3. **Chained** -- Multi-engine presets only

### 3.4 Build Shared XPN Exporter with Flexible Bundling

Location: `~/Documents/GitHub/OddfeliX/OddOscar/Source/Shared/Export/XPNExporter.h`

Extends the existing `xpn_exporter` tool (`~/Documents/GitHub/XOdyssey/tools/xpn_exporter/xpn_export.py`) with:

| Bundle Mode | Contents | MPC Expansion Name |
|------------|---------|-------------------|
| **Per-Engine** | All presets from one engine | `com.xo-ox.xoddcouple` |
| **By Vibe** | Cross-engine presets matching tags | `com.xo-ox.dub-essentials` |
| **Full Pack** | Every preset across all engines | `com.xo-ox.mega-tool-v1` |
| **Custom** | User-selected subset | User-defined |

WAV naming convention extended: `ENGINE__PRESET__NOTE__v1.WAV`

---

## 4. Phase 2: Engine Compliance (Weeks 3-6)

### 4.1 Priority Order

Engines are wrapped in order of integration difficulty (easiest first) and strategic value:

| Priority | Engine | Effort | Rationale |
|----------|--------|--------|-----------|
| **1** | OddfeliX/OddOscar (EngineX + EngineO) | Low | Already has dual-engine coupling. Extract `EngineX` and `EngineO` as separate `SynthEngine` implementations. `CouplingMatrix` becomes the template for all coupling pairs. |
| **2** | XOverdub | Low | 38 parameters, simplest signal path (voice -> send VCA -> drive -> delay -> reverb -> return). The send/return architecture maps naturally to `ownsEffects() = true`. |
| **3** | XObese | Medium | Already has a `SynthEngine` class in `xobese::` namespace. Needs adapter to match the `xo::SynthEngine` interface. 13-osc "fat" engine + Mojo parameter are unique -- worth preserving. |
| **4** | XOppossum | Medium | Also has a `SynthEngine` class. Bass-forward character with Belly/Bite macros. 122 parameters need careful namespacing (`opossum_` prefix). |
| **5** | XOdyssey | High | Monolithic processor, ~130 parameters, complex Climax system. Needs most refactoring to extract engine from processor. Worth it for the psychedelic pad sound. |
| **6** | XOblong | High | Also monolithic processor. 167 presets need migration to namespaced format. Character engine with PlaySurface already built. |
| **7** | XOnset | Built fresh | Does not exist yet. Built from day 1 against the `SynthEngine` interface per `xonset_percussive_engine_spec.md`. 8-voice drum engine with Circuit/Algorithm blend. |

### 4.2 Wrapping Strategy Per Engine

Each engine gets a thin adapter class. The existing internal architecture is untouched. Example for XOverdub:

```cpp
// File: ~/Documents/GitHub/XOverdub/src/engine/XOverdubAdapter.h

#pragma once
#include "Shared/SynthEngine.h"
#include "engine/VoiceEngine.h"
#include "fx/Drive.h"
#include "fx/TapeDelay.h"
#include "fx/SpringReverb.h"
#include "parameters/ParamSnapshot.h"

namespace xoverdub {

class XOverdubAdapter : public xo::SynthEngine
{
public:
    xo::EngineDescriptor getDescriptor() const override
    {
        return { "xoverdub", "XOverdub",
                 juce::Colour(0xFF2A9D8F), // teal
                 8, 38, false };
    }

    void prepare(double sr, int blockSize) override
    {
        voiceEngine.prepare(sr, blockSize);
        drive.prepare(sr, blockSize);
        tapeDelay.prepare(sr, blockSize);
        springReverb.prepare(sr, blockSize);
    }

    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi, int numSamples) override
    {
        // Existing render logic, unchanged
        voiceEngine.renderBlock(buffer, midi, numSamples, snap);
        // Send/return FX chain
        drive.process(buffer, numSamples, snap);
        tapeDelay.process(buffer, numSamples, snap);
        springReverb.process(buffer, numSamples, snap);
    }

    float getSampleForCoupling() const override { return lastSample; }

    void applyCouplingInput(float value, CouplingType type) override
    {
        if (type == xo::CouplingType::AmpToFilter)
            snap.filterCutoffMod = value;
    }

    bool ownsEffects() const override { return true; } // send/return FX

    // ... remaining interface methods
private:
    VoiceEngine voiceEngine;
    Drive drive;
    TapeDelay tapeDelay;
    SpringReverb springReverb;
    ParamSnapshot snap;
    float lastSample = 0.0f;
};

} // namespace xoverdub
```

### 4.3 Parameter Namespacing Convention

Every parameter ID gets an engine prefix. Existing standalone builds use un-prefixed IDs internally and map them at the adapter boundary.

| Engine | Prefix | Example Internal ID | Example Mega-Tool ID |
|--------|--------|--------------------|--------------------|
| OddfeliX/OddOscar EngineX | `oddx_` | `xFilterCutoff` | `oddx_filterCutoff` |
| OddfeliX/OddOscar EngineO | `oddo_` | `oFilterCutoff` | `oddo_filterCutoff` |
| XOverdub | `dub_` | `sendAmount` | `dub_sendAmount` |
| XObese | `obese_` | `mojo` | `obese_mojo` |
| XOppossum | `opossum_` | `belly` | `opossum_belly` |
| XOdyssey | `odyssey_` | `journey` | `odyssey_journey` |
| XOblong | `bob_` | `warmth` | `bob_warmth` |
| XOnset | `onset_` | `v1_blend` | `onset_v1_blend` |

**Rule: The adapter class handles the mapping. Internal engine code never changes its parameter IDs.** This preserves preset compatibility for standalone builds and obeys the CLAUDE.md rule: "Never rename stable parameter IDs after release."

### 4.4 Standalone Builds Remain Unchanged

Each engine continues to ship as an independent AU/VST3. The standalone `PluginProcessor` wraps the engine directly (no adapter). The adapter exists only for mega-tool integration.

```
XOverdub standalone: XOverdubProcessor -> VoiceEngine + FX (existing code, no changes)
XOverdub in mega-tool: MegaToolProcessor -> XOverdubAdapter -> VoiceEngine + FX
```

Build commands remain unchanged for each project:
```bash
# Standalone build (unchanged)
cd ~/Documents/GitHub/XOverdub
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build

# Mega-tool build (new)
cd ~/Documents/GitHub/OddfeliX/OddOscar
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DXO_MEGA_TOOL=ON && cmake --build build
```

---

## 5. Phase 3: Coupling & Chaining (Weeks 7-10)

### 5.1 Coupling Matrix Extension

The existing `XO::CouplingMatrix` handles one pair: X-to-O and O-to-X. The extended `CouplingMatrixN` handles arbitrary engine pairs.

Location: `~/Documents/GitHub/OddfeliX/OddOscar/Source/Shared/Coupling/CouplingMatrixN.h`

For N active engines, there are N*(N-1)/2 possible coupling pairs:

| Active Engines | Coupling Pairs | CPU Budget per Pair |
|---------------|---------------|-------------------|
| 2 | 1 | < 3% |
| 3 | 3 | < 2% each (6% total) |
| 4 | 6 | < 1.5% each (9% total) |

Each coupling pair exposes:
- **Amount** (0.0 - 1.0): Master intensity of the coupling
- **Type** (enum): Which modulation route is active (see `CouplingType`)
- **Direction**: A->B, B->A, or bidirectional
- **Attenuator**: Per-connection scaling

### 5.2 Normalled Routing Defaults

Every engine pair gets a default coupling that "just works" musically, following the Moog Matriarch pattern. No patching required.

| Source Engine | Target Engine | Default Coupling | Amount | Musical Effect |
|--------------|--------------|-----------------|--------|---------------|
| OddfeliX/OddOscar EngineX | OddfeliX/OddOscar EngineO | AmpToFilter | 30% | Dub pump (existing) |
| OddfeliX/OddOscar EngineO | OddfeliX/OddOscar EngineX | LFOToPitch | 30% | Pitch drift (existing) |
| XOnset (kick) | Any pad engine | AmpToFilter | 20% | Kick ducks pad brightness |
| Any pad engine | XOverdub | EnvToMorph | 15% | Pad dynamics drive delay send |
| XObese | XOppossum | AudioToRing | 0% | Off by default (destructive) |
| XOdyssey | XOnset | LFOToPitch | 10% | Pad LFO subtly detunes drums |

All defaults can be overridden or disconnected by the user.

### 5.3 Patch Cable UI (Advanced Mode)

Location: `~/Documents/GitHub/OddfeliX/OddOscar/Source/Shared/UI/PatchPanel.h`

The patch panel is hidden by default. It appears in Advanced Mode and shows:
- Each engine as a module with labeled I/O jacks
- Existing connections as colored lines (matching engine brand colours)
- Drag-to-connect interaction for new connections
- Per-connection attenuator knob (click the cable to reveal)
- "Clear All" and "Reset to Defaults" buttons

```
+-- Engine A (OddfeliX/OddOscar X) ----+     +-- Engine B (XOverdub) --------+
|                                |     |                                |
|  [Osc Out]    o============o [FM In]                                 |
|  [Filter Out] o            o [Filter In]                             |
|  [Env Out]    o            o [Send Amount]                           |
|  [LFO Out]    o            o [Pitch In]                              |
|  [Amp Out]    o============o [Sidechain In]                          |
|                                |     |                                |
+--------------------------------+     +--------------------------------+
```

### 5.4 Cross-Engine Preset Format

Multi-engine presets bundle the state of all active engines plus their coupling connections. The format is the `.xomega` JSON defined in Section 3.3. Single-engine presets (`.xoc`, `.xopmeta`, etc.) are automatically importable -- the browser wraps them in a single-engine `.xomega` envelope.

---

## 6. Phase 4: Mega-Tool Assembly (Weeks 11-14)

### 6.1 Hub Shell with Engine Selector

Location: `~/Documents/GitHub/OddfeliX/OddOscar/Source/Hub/`

The hub shell is the top-level `AudioProcessor` that owns:
- An array of registered `SynthEngine` instances (max 4 active)
- The extended `CouplingMatrixN`
- A shared FX rack (for engines with `ownsEffects() == false`)
- The unified APVTS (merged from all active engines)
- The preset browser
- The PlaySurface

```cpp
class MegaToolProcessor : public juce::AudioProcessor
{
    static constexpr int kMaxEngines = 4;

    std::array<std::unique_ptr<xo::SynthEngine>, kMaxEngines> engines;
    CouplingMatrixN couplingMatrix;
    SharedFXRack sharedFX;
    juce::AudioProcessorValueTreeState apvts;

    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi) override
    {
        // 1. Snapshot all engine parameters from APVTS
        for (auto& engine : activeEngines)
            engine->snapshotParameters(apvts);

        // 2. Split MIDI to target engines (based on key zones or channel)
        splitMidi(midi);

        // 3. Render each engine into its own buffer
        for (auto& engine : activeEngines)
            engine->renderBlock(engineBuffers[slot], engineMidi[slot], numSamples);

        // 4. Apply coupling matrix (per-sample when needed)
        couplingMatrix.process(activeEngines, engineBuffers, numSamples);

        // 5. Sum engine outputs to master bus
        sumToMaster(buffer, engineBuffers);

        // 6. Apply shared FX rack
        sharedFX.process(buffer, numSamples);
    }
};
```

### 6.2 Two UI Modes

| Mode | Who It Is For | What They See |
|------|-------------|-------------|
| **Intuitive** | Preset browsers, performers | Engine selector tabs + PlaySurface + 4 macro knobs + preset browser. No visible parameters beyond macros. |
| **Advanced** | Sound designers | Full parameter panels per engine + patch cable routing + coupling matrix + per-engine FX controls. |

Toggle between modes with a single button. The default is Intuitive.

### 6.3 Preset Library Strategy

| Source | Preset Count | Migration Path |
|--------|-------------|---------------|
| OddfeliX/OddOscar factory | 114 | Auto-wrap as single-engine `.xomega` |
| XOverdub factory | 40 | Auto-wrap as single-engine `.xomega` |
| XObese factory | ~99 | Auto-wrap as single-engine `.xomega` |
| XOppossum factory | TBD | Auto-wrap as single-engine `.xomega` |
| XOdyssey hero presets | 10 | Auto-wrap as single-engine `.xomega` |
| XOblong factory | 167 | Auto-wrap as single-engine `.xomega` |
| New chained presets | 50-100 | Built from scratch for mega-tool |
| **Total** | **480-530+** | |

This is the "preset monster" -- 500+ presets across every sonic territory, all browsable from a single interface.

### 6.4 XPN Export with Full Bundling

The MPC export pipeline renders each preset as multi-sample WAVs (one note per instrument, Convention 2), then packages them using the validated XPN format from `~/Documents/GitHub/XOdyssey/tools/xpn_exporter/xpn_export.py`.

Bundle options exposed in the UI:
- Export active engine only
- Export by tag/vibe
- Export full pack
- Export custom selection

---

## 7. What to Do with Other Agents NOW

### 7.1 Immediate Actions

| Action | Target | Rationale |
|--------|--------|-----------|
| **Let current tasks finish** | All projects | Do not interrupt in-flight work. XOppossum Phase 7 (UI polish), XOverdub smoke testing, XOblong preset listening pass -- all should complete. |
| **Stop new independent features** | All projects | After current phase completes, no new features (new oscillator modes, new FX, new UI widgets) should be built outside the mega-tool context. |
| **Start SynthEngine adapter work** | OddfeliX/OddOscar, XOverdub | These two are the MVP engines. Adapter classes should be written first. |
| **Freeze parameter IDs** | All projects | Every project should document its final parameter ID list. No new parameters added without `engine_` prefix. |

### 7.2 Agent Task Allocation

| Agent / Worker | Current Task | Next Task (After Current Completes) |
|---------------|-------------|--------------------------------------|
| OddfeliX/OddOscar agent | Core complete | Write `SynthEngine` interface + adapter for EngineX and EngineO |
| XOverdub agent | Smoke testing | Write `XOverdubAdapter` (simplest adapter, template for others) |
| XObese agent | Build complete | Write `XObeseAdapter`, migrate `xobese::SynthEngine` to `xo::SynthEngine` |
| XOppossum agent | Phase 7 UI polish | Finish Phase 7, then write `XOpossumAdapter` |
| XOdyssey agent | v0.7 complete | Extract engine from monolithic processor into adapter |
| XOblong agent | Preset listening | Finish listening pass, then write adapter |
| New agent | -- | Build XOnset from scratch against `xo::SynthEngine` interface |

### 7.3 Shared Code Repository Strategy

The `SynthEngine` interface and shared components live in the OddfeliX/OddOscar repo (since OddfeliX/OddOscar is the seed project for the mega-tool). Other projects reference it via git submodule or source copy.

```
~/Documents/GitHub/OddfeliX/OddOscar/
    Source/
        Shared/                    <-- NEW: shared mega-tool code
            SynthEngine.h          <-- The interface
            Coupling/
                CouplingMatrixN.h  <-- Extended coupling
            UI/
                PlaySurface.h      <-- Shared playing surface
                PresetBrowser.h    <-- Unified browser
                PatchPanel.h       <-- Coupling routing UI
            Export/
                XPNExporter.h      <-- MPC export
        Hub/                       <-- NEW: mega-tool shell
            MegaToolProcessor.h
            MegaToolEditor.h
        Engines/                   <-- Existing OddfeliX/OddOscar engines
            EngineX.h
            EngineO.h
        ...existing OddfeliX/OddOscar code...
```

---

## 8. MVP Definition

### 8.1 Minimum Viable Mega-Tool

The MVP must prove the concept with minimal scope. It answers one question: **"Can two XO_OX engines play simultaneously, couple together, and sound like something neither produces alone?"**

| Component | MVP Scope | NOT in MVP |
|-----------|----------|-----------|
| Engines | 2 active (OddfeliX/OddOscar + XOverdub) | 3+ engines, XOnset, XOdyssey |
| PlaySurface | Pad mode (4x4, scale-locked) | Fretless mode, Drum mode |
| Coupling | 1 pair, 2 coupling types (AmpToFilter, LFOToPitch) | Full CouplingMatrixN, patch cable UI |
| Presets | 154 (114 OddfeliX/OddOscar + 40 XOverdub, auto-wrapped) + 10 new chained | Full 500+ library |
| FX | OddfeliX/OddOscar shared rack + XOverdub send/return | Shared FX rack selection |
| UI Mode | Intuitive only | Advanced mode, patch panel |
| Export | Single-engine XPN | Multi-engine bundling |
| Formats | AU + Standalone (macOS) | VST3, iOS |

### 8.2 Why These Two Engines First

**OddfeliX/OddOscar** is the seed. Its dual-engine coupling matrix is the architectural blueprint. Its EngineX (percussive) and EngineO (pad) cover the two most important sonic territories. 114 factory presets provide immediate content.

**XOverdub** is the simplest engine to wrap (38 parameters, clean signal path) and its send/return FX architecture is sonically complementary -- it adds dub delay and spring reverb to OddfeliX/OddOscar's percussive hits. The coupling scenario (OddfeliX/OddOscar envelope pumping XOverdub's delay send) is immediately musical and demonstrates the mega-tool concept viscerally.

### 8.3 MVP Build Command

```bash
cd ~/Documents/GitHub/OddfeliX/OddOscar
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DXO_MEGA_TOOL=ON
cmake --build build
```

The `XO_MEGA_TOOL` CMake flag conditionally includes the hub shell, adapter classes, and shared components. Without the flag, the build produces the standard OddfeliX/OddOscar standalone plugin.

---

## 9. Risk Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| **Parameter ID collision across engines** | High | Critical | Strict `engine_` namespacing enforced at adapter boundary. CI test validates uniqueness across all registered engines. |
| **CPU overload with 3+ engines** | Medium | High | Default to 2 active engines. Voice count auto-reduction: 2 engines = full polyphony, 3 engines = 75% each, 4 engines = 50% each. Inactive engines at 0% CPU. Per feasibility study: 2 engines = ~31%, 3 = ~43%, 4 = ~55%. |
| **Preset compatibility breakage** | Medium | Critical | Standalone builds never touch existing preset formats. Mega-tool uses `.xomega` wrapper format with embedded engine-native state. Migration is read-only wrapping, not format conversion. |
| **UI complexity overwhelms users** | High | Medium | Intuitive mode is default and hides everything except engine selector, macros, PlaySurface, and preset browser. Advanced mode is opt-in. |
| **Loss of individual instrument identity** | Medium | High | Each engine retains its brand colour, name, and visual identity within the hub. The mega-tool is not a homogenized super-synth -- it is a collection of character instruments that can talk to each other. |
| **Interface design wrong on first attempt** | Medium | Medium | The interface is an abstract C++ class, not a plugin format. Changing it requires recompilation, not protocol versioning. Phase 2 is the feedback loop: wrapping the first two engines will reveal what the interface got wrong. |
| **Scope creep delays MVP** | High | High | MVP scope is locked (Section 8). No feature belongs in MVP unless it directly supports "2 engines + PlaySurface + coupling." The patch cable UI, drum mode, XOnset, multi-engine XPN bundling -- all post-MVP. |
| **XPN export breaks with namespaced parameters** | Low | Medium | XPN exporter operates on rendered WAVs, not parameters. The rendering step uses the engine's internal (un-prefixed) parameter state. Namespacing is transparent to export. |
| **JUCE version incompatibility across projects** | Low | Medium | All projects standardize on JUCE 8.0.12. The mega-tool build fetches JUCE once; engines link against it. No per-engine JUCE copies. |

---

## 10. Success Criteria

### 10.1 Phase Gate Criteria

| Phase | Gate | Measured By |
|-------|------|------------|
| **Phase 1** (Weeks 1-2) | `SynthEngine` interface compiles. PlaySurface renders pad grid with scale lock. Preset browser lists presets from JSON directory. | Unit tests pass. Visual inspection of PlaySurface prototype. |
| **Phase 2** (Weeks 3-6) | OddfeliX/OddOscar EngineX and EngineO each pass through `SynthEngine` interface and produce identical audio output to their standalone versions. XOverdub adapter produces identical audio. | A/B null test: standalone output minus adapter output = silence (within floating-point tolerance). |
| **Phase 3** (Weeks 7-10) | OddfeliX/OddOscar EngineX coupled to XOverdub via AmpToFilter produces audible dub pump effect. Coupling amount of 0.0 produces no modulation. | Listening test by Joshua. CPU measurement < 35% at 44.1kHz/512 buffer. |
| **Phase 4** (Weeks 11-14) | MVP builds and runs as AU + Standalone on macOS. Preset browser loads all 154+ presets. PlaySurface triggers notes on both engines. At least 10 chained presets sound compelling without explanation. | auval passes. Joshua plays it for 30 minutes and wants to keep going. |

### 10.2 Ultimate Success Metric

The mega-tool succeeds when a user opens it, browses presets, and discovers a sound that could only exist because two XO_OX engines are coupled together -- a sound that neither engine produces alone, that no other instrument on the market can make.

That is the XO_OX brand promise made real: **two engines, one conversation, infinite character.**

---

## Appendix A: Engine Inventory & Integration Complexity

| Engine | Source Location | Namespace | Processor Class | Engine Class(es) | Param Count | Voice Count | FX Ownership | Adapter Effort |
|--------|----------------|-----------|----------------|-------------------|-------------|-------------|-------------|---------------|
| OddfeliX/OddOscar X | `~/Documents/GitHub/OddfeliX/OddOscar/Source/Engines/EngineX.h` | `XO::` | `OddfeliX/OddOscarProcessor` | `EngineX` | ~24 (of 52 shared) | 8 | Processor | Low -- extract from processor |
| OddfeliX/OddOscar O | `~/Documents/GitHub/OddfeliX/OddOscar/Source/Engines/EngineO.h` | `XO::` | `OddfeliX/OddOscarProcessor` | `EngineO` | ~20 (of 52 shared) | 16 | Processor | Low -- extract from processor |
| XOverdub | `~/Documents/GitHub/XOverdub/src/engine/VoiceEngine.h` | `xoverdub::` | `XOverdubProcessor` | `VoiceEngine` + FX | 38 | 8 | Engine (send/return) | Low -- clean separation already |
| XObese | `~/Documents/GitHub/XObese/Source/DSP/SynthEngine.h` | `xobese::` | `XObeseProcessor` | `SynthEngine` | 45 | 13-osc unison | Engine (`FXChain`) | Medium -- existing SynthEngine needs interface alignment |
| XOppossum | `~/Documents/GitHub/XOppossum/Source/Engine/SynthEngine.h` | (none) | `PluginProcessor` | `SynthEngine` | 122 | VoiceManager | Engine (full chain) | Medium -- largest param set, needs prefix |
| XOdyssey | `~/Documents/GitHub/XOdyssey/Source/` (monolithic) | (none) | monolithic | (inline in processor) | ~130 | 24 | Processor | High -- must extract engine from processor |
| XOblong | `~/Documents/GitHub/XOblong/Source/` (monolithic) | (none) | monolithic | (inline in processor) | large | varies | Processor | High -- must extract engine, 167 presets to migrate |
| XOnset | Does not exist yet | `xonset::` | N/A | Built fresh | ~110 | 8 (drum) | Configurable | N/A -- built to interface |

## Appendix B: CPU Budget Projections

All measurements at 44.1 kHz, 512-sample buffer, Apple M-series:

| Configuration | Engine A | Engine B | Coupling | Shared FX | Total |
|--------------|---------|---------|----------|----------|-------|
| MVP (OddfeliX/OddOscar + XOverdub) | ~20% (both X+O) | ~8% | ~3% | ~6% | **~37%** |
| OddfeliX/OddOscar + XOnset | ~20% | ~12% | ~3% | ~6% | **~41%** |
| XObese + XOppossum | ~10% | ~15% | ~3% | ~6% | **~34%** |
| XOdyssey + XOverdub + XOnset | ~15% | ~8% + ~12% | ~6% (3 pairs) | ~8% | **~49%** |
| 4 engines (max) | varies | varies | ~9% (6 pairs) | ~8% | **< 55%** |

## Appendix C: Timeline Summary

```
Week  1-2:   Phase 1 -- Interface definition + shared UI components
Week  3-6:   Phase 2 -- Engine compliance (OddfeliX/OddOscar, XOverdub, XObese, XOppossum)
Week  7-10:  Phase 3 -- Coupling matrix extension + routing defaults + patch cable UI
Week  11-14: Phase 4 -- Hub shell assembly + preset library + MVP testing
Week  15+:   Post-MVP -- XOdyssey adapter, XOblong adapter, XOnset fresh build,
                         Advanced mode UI, multi-engine XPN export, iOS port
```

---

*CONFIDENTIAL -- XO_OX Internal Design Document*
