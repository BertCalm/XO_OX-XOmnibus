# Skill: /new-xo-engine

**Invoke with:** `/new-xo-engine`
**Status:** LIVE
**Last Updated:** 2026-03-22 | **Version:** 1.0 | **Next Review:** On next engine addition
**Purpose:** End-to-end guide for designing, scaffolding, and integrating a new engine into XOlokun — from concept brief through gallery installation.

---

## When to Use This Skill

Use this skill when:
- Creating a brand-new engine for XOlokun
- Scaffolding an engine concept into code
- Integrating a standalone XO instrument into the XOlokun gallery
- Planning a new engine and need the concept brief template

**For engine quality checks after building, use `/engine-health-check`.**
**For full ghost council evaluation, use `/synth-seance`.**
**For post-build audit, use `/post-engine-completion-checklist`.**

---

## Overview

Every XOlokun engine starts as a concept, gets designed, then built and integrated. This skill walks through all 5 phases:

```
Phase 0: IDEATION          → "What exhibition?"
Phase 1: ARCHITECT         → "Design the art"
Phase 2: SCAFFOLD          → "Build the frame"
Phase 3: INTEGRATION       → "Hang it in the gallery"
Phase 4: VERIFICATION      → "Gallery walk-through"
```

Reference: `Docs/xolokun_new_engine_process.md` for the full standalone→gallery dual-target workflow.

---

## Phase 0: Ideation

### 0.1 The XO_OX Concept Test

Answer three questions:

1. **What's the XO word?** Must follow XO + O-word naming convention.
   - The name should hint at the instrument's character
   - Check existing engines in CLAUDE.md to avoid collision

2. **What's the one-sentence thesis?**
   - "XO_____ is a _____ synth that _____"
   - Must be specific enough to explain to a producer in 10 seconds

3. **What sound can ONLY this instrument make?**
   - If the answer is "nothing unique," the concept needs work

### 0.2 Concept Brief Template

```markdown
# XO_____ — Concept Brief

## Identity
- **Name:** XO_____
- **Engine ID:** _____ (PascalCase, O-prefix)
- **Parameter Prefix:** ______ (frozen forever, lowercase, no trailing underscore)
- **Thesis:** "XO_____ is a _____ synth that _____"
- **Sound family:** [bass / lead / pad / percussion / texture / FX / hybrid]
- **Unique capability:** What can this make that nothing else can?

## Character
- **Personality in 3 words:** [e.g., "expansive, luminous, searching"]
- **Engine approach:** [subtractive / wavetable / FM / granular / physical modeling / hybrid]
- **Accent color:** #______ (must not duplicate existing engines — check CLAUDE.md table)

## Gallery Role
- **Sonic gap it fills:** What territory is currently unoccupied?
- **Best coupling partners:** Which 2-3 existing engines pair well?
- **Coupling types received:** [AmpToFilter, AudioToFM, etc.]

## Mood Affinity
- Foundation: [yes/no]
- Atmosphere: [yes/no]
- Entangled: [yes/no]
- Prism: [yes/no]
- Flux: [yes/no]
- Aether: [yes/no]
- Family: [yes/no]
- Submerged: [yes/no]
```

### 0.3 Decision Gate

Proceed when:
- [ ] Concept brief written
- [ ] XO word feels right and is unique
- [ ] Gallery gap is clear
- [ ] At least 2 coupling partner ideas exist

---

## Phase 1: Architect

### 1.1 Parameter Design

All parameters use `{prefix}_{paramName}` format from day one. Plan:

| Category | Parameters | Notes |
|----------|-----------|-------|
| Oscillator | `{prefix}_oscType`, `{prefix}_oscMix` | Core sound sources |
| Filter | `{prefix}_filterCutoff`, `{prefix}_filterRes` | Always velocity-scaled (D001) |
| Envelope | `{prefix}_attack`, `{prefix}_decay`, `{prefix}_sustain`, `{prefix}_release` | Amp envelope |
| LFO | `{prefix}_lfo1Rate`, `{prefix}_lfo1Depth`, `{prefix}_lfo2Rate`, `{prefix}_lfo2Depth` | Min 2 LFOs (D002/D005), rate floor ≤ 0.01 Hz |
| Modulation | `{prefix}_modWheelDepth`, `{prefix}_aftertouchTarget` | D006 compliance |
| Macro | `{prefix}_macroCharacter`, `{prefix}_macroMovement`, `{prefix}_macroCoupling`, `{prefix}_macroSpace` | M1-M4, all must be audible |
| FX | `{prefix}_reverbMix`, `{prefix}_delayMix` | At minimum |

### 1.2 Macro Mapping (M1–M4)

| Macro | Label | Controls |
|-------|-------|----------|
| M1 | CHARACTER | The engine's defining timbral axis |
| M2 | MOVEMENT | Modulation depth / LFO intensity |
| M3 | COUPLING | Internal coupling or mod routing amount |
| M4 | SPACE | FX depth (reverb, delay, chorus) |

### 1.3 Coupling Interface

Define:
- **`getSampleForCoupling()` output:** Usually post-filter, pre-FX mono mix
- **Supported input types:** Which `CouplingType` enums does `applyCouplingInput()` respond to?
- **Voice architecture:** Max voices, voice stealing strategy

### 1.4 Doctrine Pre-Check

Before coding, verify the design satisfies all 6 doctrines:

| Doctrine | Requirement | Design Answer |
|----------|------------|---------------|
| D001 | Velocity → filter brightness, not just amplitude | _____ |
| D002 | 2 LFOs, mod wheel, aftertouch, 4 macros, 4+ mod matrix slots | _____ |
| D003 | Physics rigor (if applicable) | _____ |
| D004 | Every declared param affects audio | _____ |
| D005 | LFO rate floor ≤ 0.01 Hz | _____ |
| D006 | Velocity→timbre + aftertouch or mod wheel | _____ |

---

## Phase 2: Scaffold

### 2.1 Create Engine Header

Create `Source/Engines/{Name}/{Name}Engine.h`:

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/StandardLFO.h"
#include <array>
#include <cmath>

namespace xolokun {

class {Name}Engine : public SynthEngine
{
public:
    {Name}Engine() = default;

    juce::String getEngineId() const override { return "{Name}"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF______); }
    int getMaxVoices() const override { return 8; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        blockSize = maxBlockSize;
        prepareSilenceGate(sampleRate, maxBlockSize, 200.0f);
        // Initialize oscillators, filters, envelopes, LFOs...
    }

    void releaseResources() override {}
    void reset() override { /* Reset all state */ }

    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // 1. Parse MIDI (wake silence gate on noteOn)
        for (const auto& meta : midi)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn()) {
                wakeSilenceGate();
                // Allocate voice, set velocity, note...
            }
            else if (msg.isNoteOff()) { /* Release voice */ }
            else if (msg.isController()) {
                if (msg.getControllerNumber() == 1)
                    modWheel = msg.getControllerValue() / 127.0f;  // D006
            }
            else if (msg.isChannelPressure()) {
                aftertouch = msg.getChannelPressureValue() / 127.0f;  // D006
            }
            else if (msg.isPitchWheel()) {
                pitchBendNorm = (msg.getPitchWheelValue() - 8192) / 8192.0f;
            }
        }

        // 2. Silence gate bypass
        if (isSilenceGateBypassed() && midi.isEmpty()) {
            buffer.clear();
            return;
        }

        // 3. Cache parameters (ParamSnapshot pattern)
        // auto cutoff = pFilterCutoff ? pFilterCutoff->load() : 1000.0f;

        // 4. DSP: render voices, apply filter, mix, FX

        // 5. Copy to coupling buffer
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            juce::FloatVectorOperations::copy(
                couplingBuffer.getWritePointer(ch),
                buffer.getReadPointer(ch), numSamples);

        analyzeForSilenceGate(buffer, numSamples);
    }

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        return couplingBuffer.getSample(channel, sampleIndex);
    }

    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override
    {
        // Map coupling types to engine internals
    }

    juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        // Add all {prefix}_ parameters here
        return { params.begin(), params.end() };
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        // Cache parameter pointers: pFilterCutoff = apvts.getRawParameterValue("{prefix}_filterCutoff");
    }

private:
    double sr = 44100.0;
    int blockSize = 512;
    float modWheel = 0.0f;
    float aftertouch = 0.0f;
    float pitchBendNorm = 0.0f;
    juce::AudioBuffer<float> couplingBuffer;
    // Oscillators, filters, voices, LFOs, envelopes...
};

} // namespace xolokun
```

### 2.2 File Structure

```
Source/Engines/{Name}/
├── {Name}Engine.h          ← Full DSP implementation (inline)
└── (optional: additional DSP headers)
```

All DSP in `.h` files. No `.cpp` stubs needed for modern pattern.

---

## Phase 3: Integration

### 3.1 Register in XOlokunProcessor.cpp

Add include at the top (after existing includes):
```cpp
#include "Engines/{Name}/{Name}Engine.h"
```

Add registration (after last registered engine):
```cpp
// {NAME} — [brief description]
static bool registered_{Name} = xolokun::EngineRegistry::instance().registerEngine(
    "{Name}", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::{Name}Engine>();
    });
```

### 3.2 Update PresetManager.h

Add to `validEngineNames` array:
```cpp
"{Name}",
```

Add to `frozenPrefixForEngine()` map:
```cpp
{ "{Name}", "{prefix}" },
```

### 3.3 Update CLAUDE.md (4 sections)

1. **Product Identity header** — add engine name to `**Engine modules (registered):**` list
2. **Engine Modules table** — add row: `| {NAME} | Source Instrument | Accent Color \`#RRGGBB\` |`
3. **Parameter Prefix table** — add row: `| {Name} | \`{prefix}_\` | \`{prefix}_exampleParam\` |`
4. **Key Files table** (if notable) — add row with engine path and description

### 3.4 Update Master Spec

Add engine row to `Docs/xolokun_master_specification.md` section 3.1.

---

## Phase 4: Verification

### 4.1 Doctrine Checklist

Run `/engine-health-check {Name}` after building. All 6 doctrines must pass:

```
D001 (Velocity→Timbre):    ✅ PASS
D002 (Modulation Depth):   ✅ PASS
D003 (Physics Rigor):      ✅ PASS or N/A
D004 (No Dead Params):     ✅ PASS
D005 (Engine Breathes):    ✅ PASS
D006 (Expression):         ✅ PASS
```

### 4.2 Integration Checklist

```
[ ] Engine compiles in XOlokun build
[ ] Engine appears in EngineRegistry (getEngineId matches registration)
[ ] Single-engine presets load and play correctly
[ ] All 4 macros produce audible change in every preset
[ ] getSampleForCoupling() returns valid samples
[ ] applyCouplingInput() produces audible modulation
[ ] CPU within budget (<28% dual, <55% tri)
[ ] No clicks/pops during engine hot-swap (50ms crossfade)
[ ] Accent color renders correctly in gallery shell
[ ] CLAUDE.md updated (all 4 sections)
[ ] PresetManager.h updated (validEngineNames + frozenPrefixForEngine)
[ ] XOlokunProcessor.cpp updated (include + registration)
```

### 4.3 Preset Minimum

- At least 8 factory presets in `.xometa` format
- At least 2 moods represented
- All presets have 6D Sonic DNA values
- All presets have working M1–M4 macros

---

## Quick Reference: Related Skills

| After This Phase | Use This Skill |
|-----------------|---------------|
| Phase 2 (scaffold) | `/engine-health-check` — verify doctrine compliance |
| Phase 2 (scaffold) | `/mod-matrix-builder` — add mod matrix if needed |
| Phase 3 (integration) | `/preset-architect` — create factory presets |
| Phase 3 (integration) | `/dna-designer` — assign Sonic DNA values |
| Phase 4 (verification) | `/coupling-preset-designer` — create Entangled presets |
| Phase 4 (verification) | `/synth-seance` — full ghost council evaluation |
| After completion | `/post-engine-completion-checklist` — 5-point post-build audit |

---

## Anti-Patterns

| Don't | Do Instead |
|-------|-----------|
| Use arbitrary parameter ID formats | Always `{prefix}_{paramName}` from day one |
| Allocate memory in renderBlock | Allocate in prepare(), use pre-allocated buffers |
| Skip velocity→filter wiring | Always wire velocity to filter brightness (D001) |
| Forget mod wheel / aftertouch | Wire CC1 and channel pressure in MIDI handler (D006) |
| Create LFOs with rate floor > 0.01 Hz | Use `NormalisableRange(0.01f, 20.0f, ...)` (D005) |
| Add parameters without DSP wiring | Every param must affect audio output (D004) |
| Use trailing underscore in prefix | Prefix is `"snap"` not `"snap_"` in frozenPrefixForEngine |

---

*Born standalone, designed for coupling. Build your art. Frame it. We'll hang it.*
