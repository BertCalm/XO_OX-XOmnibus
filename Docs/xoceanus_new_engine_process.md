# XOceanus — New Engine Development Process

**Purpose:** Define the process for designing, building, and integrating new synth engine modules into XOceanus — starting as independent sandboxes, ending as first-class gallery exhibitions.

**Invoke via Claude Code:** `/new-xo-engine`

---

## Overview

Every XOceanus engine started as a standalone instrument. That workflow is intentional — independent development gives you creative freedom, fast iteration, and a shippable product at every stage. XOceanus integration happens *after* the instrument has found its voice.

This process codifies that path so new engines are **born standalone, designed for coupling**.

```
┌─────────────────────────────────────────────────────────┐
│                                                         │
│   Phase 0: IDEATION          "What exhibition?"         │
│   ↓                                                     │
│   Phase 1: ARCHITECT         "Design the art"           │
│   ↓                                                     │
│   Phase 2: SANDBOX BUILD     "Build it independently"   │
│   ↓                                                     │
│   Phase 3: INTEGRATION PREP  "Frame it for the gallery" │
│   ↓                                                     │
│   Phase 4: GALLERY INSTALL   "Hang it on the wall"      │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

You can stop at Phase 2 and ship a standalone instrument. Phases 3-4 slot it into XOceanus.

---

## Phase 0: Ideation — "What Exhibition?"

**Goal:** Define the instrument concept before writing any code.
**Time:** 30 minutes to 2 hours
**Output:** Concept brief (1-2 pages)

### 0.1 The XO_OX Concept Test

Every XO_OX instrument must pass three questions:

1. **What's the XO word?** (XO + O-word naming convention)
   - The name should hint at the instrument's character
   - Examples: OddfeliX/OddOscar (duality), XOverdub (layering), XOdyssey (journey), XOblong (shape/warmth), XObese (weight), XOnset (attack)

2. **What's the one-sentence thesis?**
   - "XO_____ is a _____ synth that _____"
   - Must be specific enough that you could explain it to a producer in 10 seconds
   - Bad: "XOpus is a versatile synth" (says nothing)
   - Good: "XOverbite is a bass synth where plush weight meets feral bite"

3. **What sound can ONLY this instrument make?**
   - If the answer is "nothing unique," the concept needs work
   - The unique sound often comes from an unusual coupling of familiar elements

### 0.2 Gallery Fit Check

Before building, verify the new engine adds something the gallery doesn't have:

| Existing Exhibition | Sonic Territory |
|---------------------|-----------------|
| ODDFELIX (OddfeliX/OddOscar X) | Percussive attacks, FM, Karplus-Strong |
| ODDOSCAR (OddfeliX/OddOscar O) | Wavetable pads, Moog ladder warmth |
| OVERDUB (XOverdub) | Dub delays, send/return FX, tape character |
| ODYSSEY (XOdyssey) | Psychedelic pads, formant filters, long journeys |
| OBLONG (XOblong) | Warm fuzzy textures, curious motion, tactile |
| OBESE (XObese) | Heavy sampler, bold industrial character |
| ONSET (XOnset) | Percussive synthesis, circuit precision |

**Ask:** What sonic territory is unoccupied? What coupling possibilities would a new engine open?

### 0.3 Concept Brief Template

```markdown
# [XO_____] — Concept Brief

## Identity
- **Name:** XO_____
- **Thesis:** "XO_____ is a _____ synth that _____"
- **Sound family:** [bass / lead / pad / percussion / texture / FX / hybrid]
- **Unique capability:** What can this make that nothing else can?

## Character
- **Personality in 3 words:** [e.g., "plush, feral, unpredictable"]
- **Engine approach:** [subtractive / wavetable / FM / granular / physical modeling / sample / hybrid]
- **Why this engine:** How does the synthesis method serve the character?

## Gallery Role
- **Sonic gap it fills:** What territory is currently empty?
- **Best coupling partners:** Which existing engines would this pair well with?
- **Coupling types it would use:**
  - As source (sending to other engines): [e.g., "Envelope → Morph on ODYSSEY pads"]
  - As target (receiving from other engines): [e.g., "Audio FM from ODDFELIX percussion"]

## Visual Identity (First Instinct)
- **Accent color idea:** [hex or description]
- **Material/texture idea:** [e.g., "fur texture", "liquid metal", "weathered leather"]
- **Icon concept:** [simple visual that captures the character]

## Mood Affinity
Which moods would this engine's presets naturally gravitate toward?
- Foundation: [yes/no — why]
- Atmosphere: [yes/no — why]
- Entangled: [yes/no — why]
- Prism: [yes/no — why]
- Flux: [yes/no — why]
- Aether: [yes/no — why]
```

### 0.4 Decision Gate

**Proceed to Phase 1 when:**
- The concept brief is written
- The XO word feels right
- The gallery gap is clear
- At least 2 coupling partner ideas exist
- You're excited about the sound

---

## Phase 1: Architect — "Design the Art"

**Goal:** Full technical design before implementation.
**Time:** 2-4 hours
**Invoke:** `Run Synth Architect Protocol` (existing playbook skill)
**Output:** Full spec + architecture blueprint

### 1.1 Run the Synth Architect Protocol

The existing protocol (`synth_playbook/agent_skills/synth_architect_protocol.md`) handles:
- Core product thesis
- Sonic identity + signature behaviors
- Engine architecture + signal flow
- FX identity
- Modulation identity + macro design
- Preset strategy
- Implementation roadmap

### 1.2 Additional XOceanus-Specific Requirements

On top of the standard architect protocol, define:

**Parameter Namespace:**
```
All parameter IDs must use the format: {shortname}_{paramName}
Example: If short name is "poss" → poss_filterCutoff, poss_oscMorph, poss_lfoRate
```

**Macro Mapping (M1-M4):**
| Macro | Label | What It Controls |
|-------|-------|-----------------|
| M1 | CHARACTER | [the engine's defining parameter] |
| M2 | MOVEMENT | [modulation depth] |
| M3 | COUPLING | [engine-internal mod OR coupling amount when coupled] |
| M4 | SPACE | [FX depth — reverb, delay, chorus] |

**Coupling Interface Design:**
- What does `getSampleForCoupling()` return? (usually the post-filter, pre-FX mono mix)
- What coupling types can this engine **receive**? (which CouplingType enums?)
- What does each coupling input do to the engine's sound?
- Are there any coupling types this engine should **never** receive? (e.g., AmpToChoke on a pad engine)

**Voice Architecture:**
- Max voices: [number]
- Voice stealing: [oldest / quietest / lowest-priority]
- Legato mode: [yes / no]

### 1.3 Decision Gate

**Proceed to Phase 2 when:**
- Full spec written
- Parameter list defined with namespaced IDs
- Macro mapping decided
- Coupling interface designed
- Signal flow diagram complete
- Implementation phases defined

---

## Phase 2: Sandbox Build — "Build It Independently"

**Goal:** Build a working standalone instrument you can play.
**Invoke:** `/new-xo-project` to scaffold, then standard playbook protocols
**Output:** Working AU + Standalone plugin

### 2.1 Scaffold the Standalone Project

Use the existing `/new-xo-project` skill:
```
/new-xo-project name=XO_____ identity="..." code=Xo__
```

This creates `~/Documents/GitHub/XO_____/` with the standard structure.

### 2.2 The Dual-Target Rule

**This is the critical difference from pure standalone development.**

From day one, structure your DSP code so it can be wrapped by the `SynthEngine` interface later:

```
src/
├── engine/
│   ├── Voice.h              ← Your voices (standalone-friendly)
│   ├── VoicePool.h           ← Voice management
│   ├── Oscillator.h          ← Your oscillators
│   ├── Filter.h              ← Your filters
│   ├── Envelope.h            ← Your envelopes
│   └── ParamSnapshot.h       ← Parameter caching
├── dsp/
│   ├── [YourEffects].h       ← Effects (inline headers)
│   └── [YourDSP].h           ← Any unique DSP
├── PluginProcessor.h/.cpp    ← Standalone processor
├── PluginEditor.h/.cpp       ← Standalone UI
└── adapter/                  ← THE BRIDGE (can be empty during sandbox)
    └── XO_____Adapter.h      ← SynthEngine implementation (Phase 3)
```

**Key rules for dual-target compatibility:**

1. **DSP in headers.** All audio code lives in `.h` files. This makes it trivially portable.

2. **ParamSnapshot pattern.** Cache parameter pointers once per block. The adapter will map XOceanus APVTS params to the same snapshot struct.

3. **No UI coupling in DSP.** The engine should work with just parameters — no direct UI references in the audio path.

4. **Namespaced parameter IDs from day one.** Use `{shortname}_paramName` format even in standalone. When you integrate, the IDs already match.

5. **MIDI in, audio out.** The engine's core should accept MIDI and produce audio — same contract as `SynthEngine::renderBlock()`.

### 2.3 Build Freely

During Phase 2, iterate as fast as you want:
- Change the signal flow
- Add/remove parameters (but keep the namespace prefix)
- Experiment with effects
- Design presets
- Build the UI

Use the playbook protocols as needed:
- `Run DSP Stability Protocol` — real-time safety audit
- `Run Refinement Protocol` — improvement sweep
- `Run Preset Expansion Protocol` — build the preset library
- `Run CPU Profiling Protocol` — performance check
- `Run Product Identity Protocol` — sharpen the character

### 2.4 Preset Design (XOceanus-Ready from Day One)

Even in standalone mode, design presets in `.xometa` format:

```json
{
  "schema_version": 1,
  "name": "Preset Name",
  "mood": "Foundation",
  "engines": ["XO_____"],
  "author": "XO_OX",
  "version": "1.0.0",
  "description": "What it sounds like",
  "tags": ["tag1", "tag2", "tag3"],
  "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
  "couplingIntensity": "None",
  "tempo": null,
  "parameters": {
    "XO_____": {
      "shortname_param1": 0.5,
      "shortname_param2": 1000
    }
  },
  "coupling": { "pairs": [] }
}
```

**Why now?** When you integrate into XOceanus, these presets copy directly into `Presets/XOceanus/{mood}/`. No migration needed.

### 2.5 Decision Gate

**Proceed to Phase 3 when:**
- The instrument sounds good and has personality
- At least 20 presets designed (all in `.xometa` format)
- All 4 macros produce audible changes in every preset
- DSP stability protocol passes
- You like the product — it's worth exhibiting

---

## Phase 3: Integration Prep — "Frame It for the Gallery"

**Goal:** Write the adapter layer and verify coupling compatibility.
**Time:** 2-4 hours
**Output:** `XO_____Adapter.h` implementing `SynthEngine`, integration spec

### 3.1 Write the Adapter

The adapter is a thin wrapper that maps your standalone engine to the XOceanus `SynthEngine` interface. It lives in your standalone repo at `src/adapter/XO_____Adapter.h`:

```cpp
#pragma once
#include "../engine/VoicePool.h"
#include "../engine/ParamSnapshot.h"
#include <xoceanus/SynthEngine.h>

class XO_____Adapter : public xoceanus::SynthEngine {
public:
    // Identity
    juce::String getEngineId() const override { return "ShortName"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF______); }
    int getMaxVoices() const override { return N; }

    // Lifecycle
    void prepare(double sampleRate, int maxBlockSize) override
    {
        voicePool.prepare(sampleRate, maxBlockSize);
        couplingBuffer.setSize(2, maxBlockSize);
    }

    void releaseResources() override { voicePool.releaseResources(); }

    // Audio
    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi, int numSamples) override
    {
        // Apply any accumulated coupling modulation
        applyCouplingToParams();

        // Render through your existing engine
        voicePool.renderBlock(buffer, midi, numSamples);

        // Cache output for other engines to read
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            juce::FloatVectorOperations::copy(
                couplingBuffer.getWritePointer(ch),
                buffer.getReadPointer(ch), numSamples);
    }

    // Coupling
    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        return couplingBuffer.getSample(channel, sampleIndex);
    }

    void applyCouplingInput(xoceanus::CouplingType type,
                           float amount,
                           const float* sourceBuffer,
                           int numSamples) override
    {
        // Map coupling types to your engine's parameters
        switch (type) {
            case xoceanus::CouplingType::AmpToFilter:
                // Modulate your filter cutoff by source amplitude
                break;
            case xoceanus::CouplingType::AudioToFM:
                // Feed source audio into your FM input
                break;
            // ... handle other relevant types
            default: break; // Ignore unsupported types
        }
    }

    // Parameters
    juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout() override
    {
        // Return your existing parameter layout
        // IDs are already namespaced: shortname_paramName
        return makeParameterLayout();
    }

private:
    VoicePool voicePool;
    juce::AudioBuffer<float> couplingBuffer;
};
```

### 3.2 Write the Integration Spec

Create `docs/xoceanus_integration_spec.md` in your standalone repo:

```markdown
# [XO_____] — XOceanus Integration Spec

## Engine Identity
- **Engine ID:** "ShortName"
- **XOceanus Short Name:** [ALL CAPS, e.g., "POSS"]
- **Accent Color:** #______
- **Max Voices:** N
- **CPU Budget:** <X% single, <28% in dual-engine preset

## Parameter Namespace
- **Prefix:** shortname_
- **Total params:** N
- **Key params for coupling targets:**
  - shortname_filterCutoff — filter frequency (20-20000 Hz)
  - shortname_oscMorph — wavetable/morph position (0-1)
  - [etc.]

## Coupling Compatibility

### As Coupling Source (this engine → others)
| Coupling Type | What It Sends | Good Partners |
|--------------|---------------|---------------|
| getSampleForCoupling() | Post-filter mono mix | Any engine |

### As Coupling Target (other engines → this)
| Coupling Type | What It Does | Musical Effect |
|--------------|-------------|----------------|
| AmpToFilter | Source amp → filter cutoff | Rhythmic filter movement |
| AudioToFM | Source audio → FM input | Metallic/bell harmonics |
| [etc.] | | |

### Unsupported Coupling Types
| Type | Why |
|------|-----|
| AmpToChoke | [This is a pad engine — choking kills the character] |

## Macro Mapping
| Macro | Label | Target Parameter(s) |
|-------|-------|---------------------|
| M1 | CHARACTER | shortname_[param] |
| M2 | MOVEMENT | shortname_[param] |
| M3 | COUPLING | shortname_[param] |
| M4 | SPACE | shortname_[param] |

## Preset Count
- Total: N presets in .xometa format
- By mood: Foundation: N, Atmosphere: N, ...

## Visual Identity
- **Accent color:** #______
- **Material/texture:** [description]
- **Icon concept:** [description]
- **Panel character:** [what makes this engine's panel visually distinct]
```

### 3.3 Test Coupling Locally

Before integrating into XOceanus, verify the adapter works:
1. The adapter compiles against the `SynthEngine.h` header
2. `getSampleForCoupling()` returns valid samples (not NaN, not silence when playing)
3. `applyCouplingInput()` produces audible modulation for each supported type
4. `createParameterLayout()` returns properly namespaced parameters

### 3.4 Decision Gate

**Proceed to Phase 4 when:**
- Adapter compiles and works
- Integration spec is complete
- Coupling has been tested (at least informally)
- Presets are all in `.xometa` format
- You're ready to hang this on the gallery wall

---

## Phase 4: Gallery Install — "Hang It on the Wall"

**Goal:** Register the engine in XOceanus and verify end-to-end.
**Time:** 1-2 hours
**Output:** New engine live in XOceanus

### 4.1 Copy Engine Source

Copy the DSP headers and adapter into XOceanus:

```
XO_OX-XOceanus/
└── Source/
    └── Engines/
        └── ShortName/
            ├── XO_____Adapter.h      ← the adapter
            ├── XO_____Adapter.cpp    ← contains REGISTER_ENGINE macro
            ├── Voice.h                ← copied from standalone
            ├── VoicePool.h
            ├── Oscillator.h
            ├── Filter.h
            ├── Envelope.h
            ├── ParamSnapshot.h
            └── [any other DSP headers]
```

### 4.2 Register the Engine

Create `XO_____Adapter.cpp`:
```cpp
#include "XO_____Adapter.h"
REGISTER_ENGINE(XO_____Adapter)
```

That's it. The macro registers the engine factory at program start.

### 4.3 Add to CMakeLists.txt

Add the new engine source files to XOceanus's CMake target.

### 4.4 Copy Presets

```bash
cp -r ~/Documents/GitHub/XO_____/Presets/*.xometa \
      ~/Documents/GitHub/XO_OX-XOceanus/Presets/XOceanus/{mood}/
```

Presets are already in `.xometa` format with the right parameter namespace — they just work.

### 4.5 Run Sonic DNA

```bash
python3 Tools/compute_preset_dna.py
```

This fingerprints the new presets and injects DNA vectors for similarity search.

### 4.6 Update Gallery Identity

Add the new engine to:
- `CLAUDE.md` — engine modules table
- `Docs/xoceanus_master_specification.md` — engine catalog
- `Docs/xoceanus_technical_design_system.md` — visual DNA table, accent color
- `Docs/xoceanus_brand_identity_and_launch.md` — engine sub-mark

### 4.7 Design Cross-Engine Presets

Now the fun part — create presets that couple the new engine with existing ones:
- 5-10 "Entangled" mood presets using the new engine + an existing engine
- Test all supported coupling types
- Verify M3 (COUPLING) produces audible change
- Verify decoupled version still sounds good

### 4.8 Verification Checklist

```
[ ] Adapter compiles in XOceanus build
[ ] Engine appears in EngineRegistry
[ ] Single-engine presets load and play correctly
[ ] All 4 macros produce audible change
[ ] Coupling: engine can send (getSampleForCoupling works)
[ ] Coupling: engine can receive (applyCouplingInput works)
[ ] Cross-engine presets sound compelling
[ ] Decoupled cross-engine presets still sound good
[ ] DNA computed for all new presets
[ ] CPU within budget (<28% dual, <55% tri)
[ ] No clicks/pops during engine hot-swap (50ms crossfade)
[ ] Visual: accent color renders correctly in gallery shell
[ ] Gallery docs updated
```

---

## The Dual-Target Architecture — Why It Works

```
                    STANDALONE                          XOCEANUS
                    ─────────                           ────────
                    ┌──────────────┐                    ┌──────────────┐
  Your DSP ──────► │ VoicePool.h  │ ◄── same files ──► │ VoicePool.h  │
  (portable)       │ Oscillator.h │                    │ Oscillator.h │
                    │ Filter.h     │                    │ Filter.h     │
                    └──────┬───────┘                    └──────┬───────┘
                           │                                   │
                           ▼                                   ▼
                    ┌──────────────┐                    ┌──────────────┐
  Wrapper ────────► │ PluginProc.  │                    │ Adapter.h    │
  (different)       │ PluginEdit.  │                    │ (SynthEngine)│
                    └──────────────┘                    └──────┬───────┘
                           │                                   │
                           ▼                                   ▼
                    ┌──────────────┐                    ┌──────────────┐
  Host ───────────► │ AU/VST3 host │                    │ XOceanus     │
                    │ (standalone) │                    │ (gallery)    │
                    └──────────────┘                    └──────────────┘
```

The DSP is **identical** in both targets. Only the wrapper changes. This means:
- Bug fixes in standalone automatically apply in XOceanus
- New features in standalone are available in XOceanus after a file copy
- The standalone product continues to exist independently
- You never have to "port" anything — it's already there

---

## Quick Reference: What's Different from Pure Standalone

| Aspect | Pure Standalone | XOceanus-Ready Standalone |
|--------|----------------|--------------------------|
| Parameter IDs | Any format | Namespaced: `{short}_{param}` |
| Preset format | Any | `.xometa` from day one |
| DSP location | Anywhere | Inline `.h` headers |
| UI coupling | Can reference DSP | DSP must work without UI |
| Macro design | Optional | Required: M1-M4 always audible |
| Coupling | N/A | Define what coupling types you accept |
| Accent color | Optional | Required: choose one from the start |
| Voice count | Any | Must declare max for CPU budgeting |

---

## Appendix: Coupling Type Reference

When designing your engine's coupling compatibility, reference these types:

| Coupling Type | Source Sends | Target Receives |
|--------------|-------------|-----------------|
| `AmpToFilter` | Amplitude envelope | Filter cutoff modulation |
| `AmpToPitch` | Amplitude envelope | Pitch modulation |
| `LFOToPitch` | LFO output | Pitch vibrato/detune |
| `EnvToMorph` | Envelope shape | Wavetable/morph position |
| `AudioToFM` | Raw audio | Frequency modulation input |
| `AudioToRing` | Raw audio | Ring modulation (multiply) |
| `FilterToFilter` | Filter output | Second filter input |
| `AmpToChoke` | Amplitude gate | Kill/mute the target |
| `RhythmToBlend` | Rhythm pattern | Blend/mix parameter |
| `EnvToDecay` | Envelope shape | Decay time modulation |
| `PitchToPitch` | Pitch value | Pitch harmony/tracking |
| `AudioToWavetable` | Raw audio | Wavetable source buffer |

---

*This process is the gallery's open call for new exhibitions. Build your art. Frame it. We'll hang it.*
