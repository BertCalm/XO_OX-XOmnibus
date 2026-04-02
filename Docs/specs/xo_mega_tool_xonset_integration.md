# XOnset Mega-Tool Integration Specification

**Version:** 1.0
**Author:** XO_OX Designs
**Date:** 2026-03-08
**Status:** Technical Integration Specification
**Depends on:**
- `xonset_percussive_engine_spec.md` (full XOnset engine design)
- `xo_mega_tool_chaining_architecture.md` (SynthEngine interface, MegaCouplingMatrix, routing)

---

## 1. Overview

XOnset fills the only remaining gap in the mega-tool's engine catalog: percussion synthesis. No other module -- OBESE, OVERBITE, ODDFELIX, ODDOSCAR, OVERDUB, or ODYSSEY -- provides drum sounds. Without ONSET, the mega-tool requires external drum sources or samples.

**Key integration facts:**

- **Built from scratch** against the `SynthEngine` interface. No legacy wrapping or adapter pattern needed. Every other engine (OBESE wraps XOblong, ODDFELIX/ODDOSCAR wrap OddfeliX/OddOscar, OVERDUB wraps XOverdub) requires an adapter layer. ONSET is native.
- **Unique architectural challenge:** XOnset has its own internal 8x8 cross-voice coupling matrix (kick chokes hat, kick ducks snare filter, etc.) that must coexist with the mega-tool's `MegaCouplingMatrix` for cross-engine coupling. No other engine has this dual-matrix situation.
- **Fixed 8-voice architecture.** Unlike melodic engines where `setMaxVoices()` throttles polyphony under CPU pressure, ONSET always runs 8 dedicated drum voices. Voice reduction does not apply -- you cannot have a kit without a kick.

---

## 2. SynthEngine Interface Implementation

### 2.1 Class Declaration

```cpp
#pragma once
#include "Shared/SynthEngine.h"
#include "XOnset/DrumVoice.h"
#include "XOnset/CrossVoiceCouplingMatrix.h"
#include "XOnset/StepSequencer.h"

namespace xo {

REGISTER_ENGINE("onset", XOnsetEngine)

class XOnsetEngine : public SynthEngine
{
public:
    // --- Lifecycle ---
    void prepare(double sampleRate, int blockSize) override;
    void releaseResources() override;

    // --- Rendering ---
    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi) override;

    // --- Coupling I/O ---
    float getSampleForCoupling() const override;
    void applyCouplingInput(float value, CouplingType type) override;

    // --- Parameters ---
    int getParameterCount() const override { return kTotalParams; }
    juce::StringArray getParameterIDs() const override;
    juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout() override;
    void snapshotParameters(
        const juce::AudioProcessorValueTreeState& apvts) override;

    // --- Identity ---
    juce::String getModuleName() const override { return "ONSET"; }
    juce::String getModuleID() const override { return "onset"; }
    EngineColorProfile getColorProfile() const override
    {
        return { juce::Colour(0xFFC8553D),   // terracotta primary
                 juce::Colour(0xFF2A9D8F),   // teal secondary
                 juce::Colour(0xFF1A1A1A),   // dark panel background
                 juce::Colour(0xFFC8553D) }; // terracotta cables
    }

    // --- State ---
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // --- Voice Management ---
    int getActiveVoiceCount() const override;
    void setMaxVoices(int maxVoices) override { /* N/A -- always 8 */ }

    // --- ONSET Extension: per-voice coupling output ---
    float getVoiceOutput(int voiceIndex) const;
    float getVoiceEnvelopeLevel(int voiceIndex) const;
    bool getVoiceTriggerState(int voiceIndex) const;

private:
    static constexpr int kNumVoices = 8;
    static constexpr int kTotalParams = 110;

    std::array<DrumVoice, kNumVoices> voices;
    CrossVoiceCouplingMatrix xvcMatrix;   // internal 8x8
    OnsetStepSequencer sequencer;
    ParamSnapshot snap;
    float masterOutput = 0.0f;

    // Per-voice output cache for granular coupling
    std::array<float, kNumVoices> voiceOutputs = {};

    // External coupling modulation accumulators
    float externalFilterMod = 0.0f;
    float externalDecayMod  = 0.0f;
    float externalBlendMod  = 0.0f;
};

} // namespace xo
```

### 2.2 Interface Method Mapping

| SynthEngine Method | XOnset Implementation |
|---|---|
| `prepare()` | Allocate 8 `DrumVoice` instances (each with Layer X + Layer O DSP), init `CrossVoiceCouplingMatrix` with default normalled connections (hat choke, kick-to-snare filter, snare-to-hat decay), init `OnsetStepSequencer` at 120 BPM, pre-allocate all filter states and delay lines |
| `releaseResources()` | Zero all voice states, clear delay lines, reset sequencer position |
| `renderBlock()` | Per-voice render (Layer X + Layer O + blend + transient) -> voice mixer -> internal XVC coupling -> character stage -> shared FX (or own FX) -> master output. See Section 2.3 for render order |
| `getSampleForCoupling()` | Returns `masterOutput` -- the post-mixer, post-character mono sum of all 8 voices. This is what other engines "hear" from the drum kit |
| `applyCouplingInput()` | Accumulates external modulation into `externalFilterMod`, `externalDecayMod`, `externalBlendMod` based on `CouplingType`. Applied during next `renderBlock()` |
| `createParameterLayout()` | All ~110 params with `perc_` prefix: `perc_v1_blend` through `perc_v8_pan`, plus globals (`perc_master_level`, `perc_swing`, `perc_bpm`), plus XVC params (`perc_xvc_*`) |
| `getModuleName()` | `"ONSET"` |
| `getModuleID()` | `"onset"` |
| `setMaxVoices()` | No-op. Always 8 dedicated voices. Voice reduction applies to polyphonic melodic engines, not fixed drum kits |

### 2.3 Render Order (Inside renderBlock)

```cpp
void XOnsetEngine::renderBlock(juce::AudioBuffer<float>& buffer,
                                juce::MidiBuffer& midi)
{
    const int numSamples = buffer.getNumSamples();

    // 1. Advance sequencer (generates internal MIDI triggers)
    sequencer.processBlock(midi, numSamples, snap);

    // 2. Render each voice independently
    for (int v = 0; v < kNumVoices; ++v)
    {
        // Apply external coupling modulation to voice params
        voices[v].applyExternalMod(externalFilterMod,
                                    externalDecayMod,
                                    externalBlendMod);
        voices[v].renderBlock(numSamples);
        voiceOutputs[v] = voices[v].getLastSample();
    }

    // 3. Internal cross-voice coupling (XOnset's own 8x8 matrix)
    //    This runs BEFORE mega-tool coupling touches the output.
    xvcMatrix.process(voices, numSamples);

    // 4. Voice mixer (level + pan per voice -> stereo sum)
    juce::AudioBuffer<float> mixBuffer(2, numSamples);
    mixBuffer.clear();
    for (int v = 0; v < kNumVoices; ++v)
        voices[v].mixInto(mixBuffer, snap.voiceLevel[v], snap.voicePan[v]);

    // 5. Character stage (grit / warmth)
    characterStage.process(mixBuffer, numSamples);

    // 6. FX (delay, reverb, lofi -- or bypass if using shared rack)
    if (!useSharedFX)
        fxRack.process(mixBuffer, numSamples);

    // 7. Add to output buffer + cache master for coupling
    for (int ch = 0; ch < 2; ++ch)
        buffer.addFrom(ch, 0, mixBuffer, ch, 0, numSamples);

    masterOutput = mixBuffer.getSample(0, numSamples - 1);

    // 8. Clear external mod accumulators for next block
    externalFilterMod = 0.0f;
    externalDecayMod  = 0.0f;
    externalBlendMod  = 0.0f;
}
```

### 2.4 Per-Voice Coupling Extension

The standard `getSampleForCoupling()` returns the summed kit output. But XOnset also exposes per-voice outputs for granular cross-engine routing:

```cpp
float XOnsetEngine::getVoiceOutput(int voiceIndex) const
{
    if (voiceIndex >= 0 && voiceIndex < kNumVoices)
        return voiceOutputs[(size_t)voiceIndex];
    return 0.0f;
}

float XOnsetEngine::getVoiceEnvelopeLevel(int voiceIndex) const
{
    if (voiceIndex >= 0 && voiceIndex < kNumVoices)
        return voices[(size_t)voiceIndex].getEnvelopeLevel();
    return 0.0f;
}

bool XOnsetEngine::getVoiceTriggerState(int voiceIndex) const
{
    if (voiceIndex >= 0 && voiceIndex < kNumVoices)
        return voices[(size_t)voiceIndex].wasTriggeredThisBlock();
    return false;
}
```

These extension methods are not part of the `SynthEngine` interface. The `MegaCouplingMatrix` accesses them via `dynamic_cast<XOnsetEngine*>` when the source engine's `getModuleID()` returns `"onset"`. This is the only `dynamic_cast` in the coupling system -- justified because per-voice drum coupling is ONSET-specific and cannot be generalized.

---

## 3. Dual Coupling Matrix Challenge

### 3.1 The Problem

XOnset has an internal `CrossVoiceCouplingMatrix` -- an 8x8 matrix where drum voices modulate each other (kick amplitude ducks snare filter, closed hat chokes open hat, snare tightens hat decay). This is fundamental to making the kit sound cohesive.

The mega-tool has the `MegaCouplingMatrix` -- an N-slot matrix where engines modulate each other (ONSET kick amplitude ducks OBESE pad filter, ODDOSCAR LFO modulates ONSET kick decay).

Both matrices must operate simultaneously without parameter collisions, processing order conflicts, or feedback loops.

### 3.2 The Solution: Sequential Processing with Namespace Isolation

**Processing order:** Internal coupling runs FIRST (inside `renderBlock()`), then mega-tool coupling applies to ONSET's outputs.

```
Per-block execution order:
1. ONSET.renderBlock()
   a. Voices render individually
   b. CrossVoiceCouplingMatrix.process()  <-- internal 8x8
   c. Voice mixer -> character -> FX -> masterOutput
2. MegaCouplingMatrix.processSample()     <-- cross-engine
   a. Reads ONSET.getSampleForCoupling() (or getVoiceOutput(n))
   b. Applies modulation to other engines
   c. Applies external modulation back to ONSET via applyCouplingInput()
3. External modulation stored in accumulators
4. Next block: accumulators applied in step 1a
```

**Namespace isolation:** No parameter ID can collide between the two systems.

| Matrix | Parameter Prefix | Example |
|---|---|---|
| Internal cross-voice (XVC) | `perc_xvc_` | `perc_xvc_kick_to_snare_filter` |
| Mega-tool cross-engine | `coupling_AB_` | `coupling_AB_amount` |

The prefixes are structurally incompatible -- `perc_xvc_*` is registered by ONSET's `createParameterLayout()`, while `coupling_AB_*` is registered by the `MegaToolProcessor`. They live in different sections of the APVTS tree and cannot collide.

### 3.3 Feedback Prevention Between Matrices

A potential feedback loop: mega-tool coupling modulates ONSET's kick decay -> kick decay change alters kick amplitude -> kick amplitude changes internal coupling to snare -> snare output changes ONSET's master output -> master output feeds back to mega-tool coupling.

**Safeguard:** The one-block delay between external coupling input (step 3) and its application (step 1a of the next block) breaks any same-sample feedback path. Combined with the `MegaCouplingMatrix`'s existing `tanh()` soft limiter and DC blocker, this prevents runaway feedback.

---

## 4. Sequencer Coordination

### 4.1 The Sync Problem

XOnset has a built-in 16-step sequencer with per-step parameter locks (blend, pitch, decay, snap per step). The mega-tool's OddfeliX/OddOscar-derived architecture includes a 64-step pattern sequencer for melodic engines.

These must not fight over transport control.

### 4.2 Solution: Shared Transport, Independent Patterns

**Recommendation:** Option A/B hybrid -- shared transport with independent pattern data.

```
┌──────────────────────────────────────────────┐
│              SharedTransport                  │
│  BPM: 120    Position: beat 2.3   Playing: Y  │
│  Time sig: 4/4   Swing: 55%                  │
└──────┬───────────────────────┬────────────────┘
       │                       │
       ▼                       ▼
┌──────────────┐       ┌──────────────┐
│ ONSET Seq    │       │ Melodic Seq  │
│ 16-step      │       │ 64-step      │
│ 8 voice lanes│       │ 4 tracks     │
│ P-locks      │       │ Note data    │
│ Probability  │       │ Conditions   │
└──────────────┘       └──────────────┘
```

**Rules:**

1. **Single BPM source.** The `SharedTransport` owns BPM, play/stop, and song position. Both sequencers read from it. Neither sequencer can set its own BPM independently.
2. **Independent pattern length.** ONSET runs 16-step patterns (or 32/64 with page selection). The melodic sequencer runs 64-step patterns. They loop independently but stay phase-locked to the shared transport.
3. **Swing applies globally.** Swing percentage is a transport-level parameter. Both sequencers apply the same swing curve. Per-engine swing offsets are additive on top of the global swing.
4. **MIDI clock sync.** When the host DAW provides MIDI clock, the `SharedTransport` locks to it. Both sequencers follow. When running standalone, `SharedTransport` is the clock master.

### 4.3 Implementation

```cpp
/// Shared transport state -- owned by MegaToolProcessor,
/// read by all sequencer instances.
struct SharedTransport
{
    std::atomic<double> bpm       { 120.0 };
    std::atomic<bool>   playing   { false };
    std::atomic<double> posBeats  { 0.0 };   // current position in beats
    std::atomic<float>  swing     { 0.5f };  // 0.5 = no swing
    std::atomic<int>    timeSigNum{ 4 };
    std::atomic<int>    timeSigDen{ 4 };

    /// Advance position by one block's worth of beats.
    void advance(int numSamples, double sampleRate)
    {
        if (!playing.load()) return;
        double beatsPerSample = bpm.load() / (60.0 * sampleRate);
        posBeats.store(posBeats.load() + beatsPerSample * numSamples);
    }
};
```

The `OnsetStepSequencer` reads `SharedTransport` and converts beat position to step index. Per-step parameter locks are applied by overriding the relevant voice parameters in the `ParamSnapshot` before voice rendering.

---

## 5. PlaySurface Drum Mode

When ONSET occupies any engine slot, PlaySurface Zone 1 switches to DRUM mode (4x4 grid). This replaces the default melodic keyboard layout.

### 5.1 Pad Layout

```
┌────────┬────────┬────────┬────────┐
│ Tom    │ Perc A │ Perc B │ accent │  Row 4 (auxiliary + accent)
│  V6    │  V7    │  V8    │  all   │
├────────┼────────┼────────┼────────┤
│ Clap   │ HH-C   │ HH-O   │ accent │  Row 3 (upper kit)
│  V5    │  V3    │  V4    │  all   │
├────────┼────────┼────────┼────────┤
│ Kick   │ Snare  │ Kick   │ Snare  │  Row 2 (core, doubled)
│  V1    │  V2    │  V1    │  V2    │
├────────┼────────┼────────┼────────┤
│ patt1  │ patt2  │ patt3  │ patt4  │  Row 1 (pattern select)
└────────┴────────┴────────┴────────┘
```

Row 2 doubles Kick and Snare for two-handed finger drumming -- left-hand kick + right-hand snare without crossing hands.

Row 1 selects between 4 stored patterns (sequencer presets). Tap to switch; hold to copy current pattern to that slot.

### 5.2 Per-Pad XY Control

Each drum pad is an XY control surface within its touch area:

| Axis | Maps to | Range | Musical Effect |
|---|---|---|---|
| X (horizontal) | `v{n}_blend` | 0.0 (left) to 1.0 (right) | Circuit (left) to Algorithm (right) |
| Y (vertical) | `v{n}_decay` | Short (bottom) to Long (top) | Tight hit vs ringing sustain |
| Velocity (pressure) | `v{n}_snap` | Soft (low vel) to Sharp (high vel) | Transient intensity |

Hitting the left edge of the Kick pad produces a pure 808 kick. Hitting the right edge produces a modal membrane kick. Hitting center produces a hybrid. This is per-hit, per-pad synthesis control.

### 5.3 Visual Feedback

Each pad displays a terracotta-to-teal gradient reflecting the current blend position:

- Blend 0.0: full terracotta (Circuit/analog)
- Blend 0.5: gradient midpoint
- Blend 1.0: full teal (Algorithm/digital)

The gradient updates in real-time as the user's finger moves horizontally. Accent pads pulse white on trigger.

### 5.4 Orbit Path in Drum Mode

The Orbit Path (circular gesture zone) is configurable for ONSET:

- **Default:** Maps to MACHINE macro -- circular gesture morphs the entire kit from analog to digital
- **Alt mode:** Maps to individual voice blend (selected by tapping a pad first, then orbiting)
- **Performance mode:** Maps to SPACE macro (reverb/delay send) for dub throws

---

## 6. Cross-Engine Coupling Scenarios

These are the normalled and user-configurable coupling routes between ONSET and other mega-tool engines.

### 6.1 Normalled Default

From `MegaCouplingMatrix::getNormalledRoutes()`, when one engine is `"onset"`:

```cpp
// ONSET + any melodic engine
else if (idA == "onset" || idB == "onset")
{
    routes.push_back({ CouplingType::AmpToFilter, 0.15f,
                       true, true, false });
}
```

This provides a subtle dub pump: ONSET's kick transients duck the melodic engine's filter cutoff by 15%. Musical without being overpowering. The user can override to 0% (disable) or crank to 100% (full sidechain pump).

### 6.2 Full Coupling Route Table

| Scenario | Route Type | Signal Path | Implementation Detail |
|---|---|---|---|
| ONSET -> any melodic | `AmpToFilter` | Kick envelope follower -> melodic filter cutoff | `getSampleForCoupling()` returns master output; `computeModulation()` uses `AmpToFilter` inversion for pump effect |
| ONSET -> OVERDUB | `SendToFX` | Master output -> OVERDUB send input | Audio routing through OVERDUB's send/return chain. Amount = send level |
| ODDOSCAR -> ONSET | `EnvToDecay` | Pad LFO -> kick decay time | `applyCouplingInput(lfoValue, EnvToDecay)` modulates `perc_v1_decay` via `externalDecayMod` accumulator |
| OVERBITE -> ONSET | `RhythmToBlend` | Bass amplitude -> snare blend | `applyCouplingInput(ampValue, RhythmToBlend)` modulates `perc_v2_blend` via `externalBlendMod` accumulator |
| ONSET -> ODYSSEY | `AmpToFilter` | Hat pattern triggers -> shimmer depth | Per-voice trigger output via `getVoiceTriggerState(3)` -> ODYSSEY's Prism Shimmer depth. Requires `dynamic_cast` for per-voice access |
| OBESE -> ONSET | `LFOToPitch` | OBESE pad LFO -> tom pitch | `applyCouplingInput(lfoValue, LFOToPitch)` adds pitch drift to V6 (tom) |
| ONSET -> ODDFELIX | `TriggerToReset` | Kick trigger -> ODDFELIX envelope reset | Kick hit resets ODDFELIX's oscillator phase and envelope -- creates locked groove |

### 6.3 applyCouplingInput Implementation

```cpp
void XOnsetEngine::applyCouplingInput(float value, CouplingType type)
{
    switch (type)
    {
        case CouplingType::AmpToFilter:
            externalFilterMod += value;
            break;

        case CouplingType::EnvToDecay:
            externalDecayMod += value;
            break;

        case CouplingType::RhythmToBlend:
            externalBlendMod += value;
            break;

        case CouplingType::LFOToPitch:
            // Distribute pitch mod to all voices (scaled per-voice)
            for (auto& v : voices)
                v.addPitchMod(value);
            break;

        case CouplingType::AmpToChoke:
            // External choke kills all ringing voices (panic)
            if (value > 0.5f)
                for (auto& v : voices)
                    v.choke();
            break;

        default:
            break;
    }
}
```

---

## 7. Macro Integration

### 7.1 XOnset's Four Macros

| Macro | Name | Internal Targets | Range |
|---|---|---|---|
| M1 | MACHINE | All 8 voice blend positions simultaneously | 0.0 (pure circuit) to 1.0 (pure algorithm) |
| M2 | PUNCH | All voice snap + body params | 0.0 (soft) to 1.0 (aggressive) |
| M3 | SPACE | Shared reverb send + delay feedback | 0.0 (dry) to 1.0 (drowned) |
| M4 | MUTATE | Randomize blend + character within +/-20% | 0.0 (stable) to 1.0 (maximum drift) |

### 7.2 Macro Behavior in Mega-Tool Modes

**Intuitive Mode (single-knob coupling):**

- MACHINE is exposed as a mega-tool-level macro on the header bar alongside the BLEND knob (Intuitive mode's coupling control)
- MACHINE and BLEND interact: MACHINE sets the drum kit's analog/digital character, BLEND sets how much that character feeds into other engines via coupling
- The other 3 macros (PUNCH, SPACE, MUTATE) are accessible via the ONSET parameter page but not on the header bar

**Advanced Mode (full coupling matrix):**

- All 4 macros are accessible on ONSET's parameter page
- MACHINE position can be used as a coupling source: "MACHINE -> OBESE filter cutoff" means turning MACHINE from analog to digital also opens the pad engine's filter
- Each macro is exposed as a modulatable parameter: `perc_macro_machine`, `perc_macro_punch`, `perc_macro_space`, `perc_macro_mutate`

### 7.3 MACHINE as Coupling Source

```cpp
// In MegaCouplingMatrix, MACHINE macro can feed coupling routes
// by reading the ONSET engine's macro parameter value.
float machineMod = onsetEngine->getParameterValue("perc_macro_machine");
// This value (0.0-1.0) can modulate other engines' parameters
// just like any coupling source.
```

---

## 8. Preset Format

### 8.1 Single-Engine ONSET Preset (.xomega)

```json
{
    "schema_version": 1,
    "preset_name": "808 Reborn",
    "category": "Circuit Kits",
    "author": "XO_OX",
    "engines": [
        {
            "module": "onset",
            "slot": "A",
            "parameters": {
                "perc_v1_blend": 0.15,
                "perc_v1_pitch": 0.0,
                "perc_v1_decay": 0.45,
                "perc_v1_tone": 0.4,
                "perc_v1_snap": 0.3,
                "perc_v1_body": 0.8,
                "perc_v1_character": 0.2,
                "perc_v1_level": -6.0,
                "perc_v1_pan": 0.0,
                "perc_v1_algo_mode": 1,
                "perc_v1_env_shape": 0
            },
            "sequencer": {
                "steps": 16,
                "lanes": [
                    {
                        "voice": 0,
                        "pattern": [1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0],
                        "locks": {
                            "3": { "perc_v1_blend": 0.5 },
                            "11": { "perc_v1_snap": 0.8 }
                        }
                    }
                ]
            }
        }
    ],
    "coupling": [],
    "routing_mode": "independent"
}
```

### 8.2 Multi-Engine Preset with Drums

```json
{
    "schema_version": 1,
    "preset_name": "Dub Pressure",
    "category": "XO Fusion Kits",
    "engines": [
        { "module": "onset", "slot": "A", "parameters": { "..." : "..." } },
        { "module": "morph", "slot": "B", "parameters": { "..." : "..." } },
        { "module": "dub",   "slot": "C", "parameters": { "..." : "..." } }
    ],
    "coupling": [
        {
            "pair": "AB",
            "routes": [
                { "type": "AmpToFilter", "amount": 0.40, "enabled": true }
            ]
        },
        {
            "pair": "AC",
            "routes": [
                { "type": "SendToFX", "amount": 0.35, "enabled": true }
            ]
        }
    ],
    "routing_mode": "shared_fx"
}
```

### 8.3 Preset Types

| Type | Contents | Use Case |
|---|---|---|
| **Kit preset** | 8 voice parameters + macros, no sequencer data | Load a drum sound without overwriting patterns |
| **Pattern preset** | Sequencer lanes + p-locks only | Load a rhythm into any kit |
| **Full preset** | Kit + pattern + FX + coupling routes | Complete drum configuration |

Kit and pattern presets are partial -- they only write their subset of parameters when loaded, leaving the rest unchanged. The preset loader checks for the presence of `"sequencer"` and `"coupling"` keys to determine which type was saved.

---

## 9. XPN Export for Drum Kits

### 9.1 Export Strategy

Each ONSET kit exports as one MPC drum program. The 8 voices map to 8 pads in the MPC's drum program layout.

| Voice | MPC Pad | MIDI Note | WAV Filename |
|---|---|---|---|
| V1 Kick | Pad 1 | C1 (36) | `KITNAME__Kick__v1.WAV` |
| V2 Snare | Pad 2 | D1 (38) | `KITNAME__Snare__v1.WAV` |
| V3 HH-C | Pad 3 | F#1 (42) | `KITNAME__HH_Closed__v1.WAV` |
| V4 HH-O | Pad 4 | A#1 (46) | `KITNAME__HH_Open__v1.WAV` |
| V5 Clap | Pad 5 | D#1 (39) | `KITNAME__Clap__v1.WAV` |
| V6 Tom | Pad 6 | A1 (45) | `KITNAME__Tom__v1.WAV` |
| V7 Perc A | Pad 7 | C#1 (37) | `KITNAME__Perc_A__v1.WAV` |
| V8 Perc B | Pad 8 | G#1 (44) | `KITNAME__Perc_B__v1.WAV` |

WAV naming follows the XPN exporter convention: double-underscore separators, `__v1` suffix.

### 9.2 MACHINE Variants

Each kit can optionally export three versions representing the MACHINE macro at different positions:

| Variant | MACHINE | Character | Suffix |
|---|---|---|---|
| Circuit | 0.0 | Pure analog -- all voices at Layer X | `_Circuit` |
| Hybrid | 0.5 | Blended -- the "new sound" zone | `_Hybrid` |
| Algorithm | 1.0 | Pure digital -- all voices at Layer O | `_Algorithm` |

Example: `808_Reborn_Circuit__Kick__v1.WAV`, `808_Reborn_Hybrid__Kick__v1.WAV`, `808_Reborn_Algorithm__Kick__v1.WAV`.

### 9.3 Velocity Layers

For multi-layer MPC drum programs, each voice renders at three velocity levels:

| Layer | Velocity | Snap Behavior | Filename |
|---|---|---|---|
| Soft | 64 | Low snap, muted transient | `KITNAME__Kick__v1.WAV` |
| Medium | 100 | Medium snap | `KITNAME__Kick__v2.WAV` |
| Hard | 127 | Full snap, sharp transient | `KITNAME__Kick__v3.WAV` |

This produces 8 voices x 3 velocities = 24 WAVs per kit, or 72 WAVs per kit with MACHINE variants. The XPN exporter's `PadNoteMap` assigns velocity split points: 0-79, 80-109, 110-127.

### 9.4 XPM Generation

The `xpn_export.py` tool generates the `.xpm` program file. Key settings for drum programs:

- `Application=MPC-V`
- `ProgramType=Drum` (not Keygroup)
- One `Pad` element per voice with `RootNote=MIDI+1`, `KeyTrack=False`
- `FilterType=2`, `Cutoff=1.0` (bypass filter -- use the synth's own tone shaping)
- Choke groups: HH-C and HH-O share `MuteGroup=1`

---

## 10. Build Plan

### Phase 1: Core Voices (Week 1-2)

**Goal:** 8 working drum voices with Layer X (circuit topologies) only.

1. Define `xonset` namespace + `Parameters.h` with all ~110 parameter IDs using `perc_` prefix
2. Implement `ParamSnapshot` for XOnset parameters (reuse pattern from OddfeliX/OddOscar)
3. Implement `DrumVoice` base with 4 circuit topologies:
   - Bridged-T kick (V1, V6)
   - Noise-burst snare (V2, V5)
   - 6-oscillator metallic hat (V3, V4)
   - Self-oscillating filter kick (V1 alt)
4. Implement voice pool (8 voices, trigger-per-note, no polyphony per voice)
5. Implement per-voice AD envelope + Cytomic SVF filter (direct reuse from existing code)
6. Wire up to MIDI (General MIDI drum map)
7. **Gate:** Each circuit voice sounds correct in isolation

**Reusable modules:** `CytomicSVF` (100%), `AdsrEnvelope` adapted to AD/AHD (90%), `ParamSnapshot` pattern (100%), `PolyBLEP` for metallic oscillators (80%).

### Phase 2: Algorithm Layer + Blend (Week 2-3)

**Goal:** Layer O algorithms implemented, blend morphing working.

8. Implement FM percussion (2-op + feedback, 4 ratio presets)
9. Implement Modal resonator (8-mode bank, membrane/bar/plate materials)
10. Implement Karplus-Strong percussion (delay line + averaging filter)
11. Implement Phase Distortion percussion (non-linear phase accumulator, 3 waveshapes)
12. Implement equal-power blend crossfade engine
13. Implement Transient Designer (pre-blend click/snap injection)
14. Implement adaptive `character` parameter routing (layer-dependent)
15. **Gate:** Blend smoothly morphs between layers with no artifacts or level jumps

### Phase 3: Cross-Voice Coupling + Sequencer (Week 3-4)

**Goal:** Internal coupling matrix and step sequencer with parameter locks.

16. Implement `CrossVoiceCouplingMatrix` (6 coupling types from spec Section 6.2)
17. Implement default normalled connections (hat choke, kick->snare filter at 15%, snare->hat decay at 10%)
18. Implement 16-step sequencer with per-voice lanes
19. Implement per-step parameter locks (blend, pitch, decay, snap)
20. Implement probability triggers + condition system + ratchets
21. Implement 4 macros (MACHINE, PUNCH, SPACE, MUTATE)
22. **Gate:** Coupling creates audible, musical interaction between voices. MACHINE macro morphs entire kit smoothly.

### Phase 4: SynthEngine Interface + Mega-Tool Integration (Week 4-5)

**Goal:** XOnset works as a registered `SynthEngine` in the mega-tool.

23. Implement full `SynthEngine` interface (all methods from Section 2)
24. Register with `REGISTER_ENGINE("onset", XOnsetEngine)`
25. Implement cross-engine coupling outputs (per-voice via `getVoiceOutput()` + master via `getSampleForCoupling()`)
26. Implement `applyCouplingInput()` with all supported `CouplingType` handlers
27. Implement PlaySurface drum mode (4x4 pad layout, blend-on-X, decay-on-Y, velocity-to-snap)
28. Implement `SharedTransport` integration for sequencer sync
29. Implement shared FX routing toggle (`ownsEffects()` returns false by default, uses mega-tool rack)
30. **Gate:** XOnset couples with another engine -- kick hits pump pad engine filter

### Phase 5: Presets + Polish (Week 5-6)

**Goal:** 85 factory presets, XPN export, CPU optimization.

31. Design 10 hero presets (808 Reborn, Membrane Theory, Quantum Kit, Living Machine, Time Stretch, Dub Pressure, Future 909, Particle Drums, Analog Heart, Mutant Factory)
32. Design remaining 75 presets across 6 categories: 15 circuit, 15 algorithm, 20 hybrid, 15 coupled, 10 morphing, 10 fusion
33. XPN export pipeline: render all kits x 3 MACHINE variants x 3 velocity layers
34. CPU optimization pass (NEON/SSE for modal resonators, sine LUT for FM, inactive voice skip)
35. auval validation
36. **Gate:** All presets load/save correctly in .xomega format. CPU stays under 15% at 44.1kHz/512.

**Estimated timeline:** 4-6 weeks, parallelizable with other engine wrapping work (OBESE, OVERBITE, OVERDUB adapters).

---

*CONFIDENTIAL -- XO_OX Internal Design Document*
