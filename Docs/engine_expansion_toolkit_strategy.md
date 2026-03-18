# Engine Expansion Toolkit — Post-V1 Strategy

> How XOmnibus grows from 34 engines to an open ecosystem,
> and how we give people a playground to learn synthesis by building.

---

## Part 1: Post-V1 Engine Expansion Strategy

### The Core Tension

V1 ships with 34 hand-built engines, each with a distinct sonic identity designed
through the seance process. Post-V1 expansion needs to maintain that quality bar
while opening the door to growth. Three expansion channels:

### Channel A: First-Party Engines (XO_OX)

**Same process as V1.** Ideation → Architect → Sandbox → Integration.

What changes post-V1:
- **Engine identity review before build.** The landscape doc (`xomnibus_landscape_2026.md`)
  maps occupied sonic territory. New first-party engines must fill genuine gaps, not
  overlap existing coverage. Run the seance cross-reference before greenlighting.
- **Versioned engine registry.** Engines added post-V1 ship in point releases.
  The registry gains a `sinceVersion` field so preset files can declare minimum
  version requirements.
- **Backward-compatible preset loading.** If a preset references an engine that
  doesn't exist in the user's version, XOmnibus shows a clear message ("This preset
  requires engine OSTINATO, available in v1.2") rather than silently failing.

### Channel B: Community Engines (Third-Party)

This is the big unlock. The `SynthEngine` interface is already clean enough to
serve as a public API — 10 methods, well-documented responsibilities, no hidden
dependencies. The work is making it *safe* and *discoverable*.

**Technical architecture:**

```
Community Engine (.dylib / .framework / .so)
    │
    ├── Implements SynthEngine interface
    ├── Exports C factory function: xomnibus_create_engine()
    ├── Exports C metadata function: xomnibus_engine_info()
    │       → returns: id, name, accent color, param prefix,
    │         coupling types accepted, version, author
    └── Ships as loadable module (not compiled into XOmnibus)

XOmnibus Host
    │
    ├── Scans ~/Library/XOmnibus/Engines/ (macOS)
    │   └── ~/.local/share/XOmnibus/Engines/ (Linux)
    ├── Loads modules at startup (not on audio thread)
    ├── Validates interface version compatibility
    ├── Sandboxes: no file system access, no network, no UI
    └── Registers validated engines into EngineRegistry
```

**Safety model:**

Community engines run in the same process (required for real-time audio), but
XOmnibus enforces constraints:

| Constraint | How |
|-----------|-----|
| Interface version check | Engine declares `XOMNIBUS_API_VERSION` — reject mismatches |
| Parameter namespace collision | Reject if prefix collides with existing engine |
| Engine ID collision | Reject duplicate IDs |
| Crash isolation | Wrap `renderBlock()` in a watchdog — if an engine stalls for >100ms, mute it and flag to user |
| Resource limits | Engines declare max voices and max buffer size in metadata |
| No UI injection | Community engines have no UI access — XOmnibus auto-generates controls from parameter layout |

**What community engines CAN do:**
- Full DSP (any synthesis technique)
- Declare parameters with standard types (float, choice, bool)
- Accept and produce coupling signals (all 13 types)
- Ship `.xometa` presets
- Define macro mappings

**What community engines CANNOT do:**
- Custom UI (auto-generated from parameters)
- File I/O (no sample loading — that's a deliberate scope limit for V1 community)
- Network access
- Spawn threads

### Channel C: The Engine Forge (Baby's First Engine — see Part 3)

A built-in engine that IS the toolkit. Not an external SDK but a playable
synthesis sandbox inside XOmnibus itself. Details in Part 3.

---

## Part 2: XOmnibus Engine SDK

### What Ships

A standalone package that lets developers build community engines:

```
xomnibus-engine-sdk/
├── include/
│   ├── xomnibus/SynthEngine.h        ← interface (read-only, canonical)
│   ├── xomnibus/CouplingTypes.h      ← enum + helpers
│   ├── xomnibus/ParameterTypes.h     ← param creation helpers
│   └── xomnibus/EngineModule.h       ← C export macros
├── templates/
│   ├── MinimalEngine/                 ← sine osc, 1 param (pitch)
│   ├── SubtractiveEngine/            ← osc→filter→amp, full macro set
│   └── GranularEngine/               ← grain buffer, position/size/density
├── examples/
│   ├── TutorialSineEngine/           ← step-by-step walkthrough
│   └── CouplingDemoEngine/           ← demonstrates coupling I/O
├── tools/
│   ├── validate-engine                ← CLI: loads module, checks interface
│   ├── generate-scaffold              ← CLI: creates boilerplate from template
│   └── preset-lint                    ← CLI: validates .xometa files
├── docs/
│   ├── getting-started.md
│   ├── api-reference.md
│   ├── the-6-doctrines.md            ← quality expectations
│   ├── coupling-guide.md
│   └── preset-format.md
├── tests/
│   └── engine-test-harness/          ← headless host that exercises all 10 methods
└── CMakeLists.txt                    ← builds templates + examples
```

### SDK Design Principles

1. **Header-only interface.** The SDK is just headers — no library to link against.
   Community engines are self-contained dynamic modules.

2. **Template-driven.** `generate-scaffold` asks 5 questions (engine name, param prefix,
   synthesis type, voice count, coupling types) and produces a compilable, testable,
   XOmnibus-loadable engine in under a minute.

3. **Validation before submission.** `validate-engine` runs the 6 Doctrines as automated
   checks where possible:
   - D001: Does velocity affect more than just amplitude? (checks param names)
   - D002: Are there LFO params? Mod matrix slots? (checks parameter layout)
   - D004: Does every declared parameter appear in renderBlock? (static analysis hint)
   - D005: Is there an LFO with rate floor ≤ 0.01 Hz? (checks param ranges)
   - D006: Does the engine accept velocity and at least one CC? (checks MIDI handling)

4. **Test harness.** A headless host that:
   - Creates the engine
   - Calls prepare/renderBlock/reset in sequence
   - Feeds MIDI (note on, note off, pitch bend, mod wheel, aftertouch)
   - Checks output isn't silent, isn't clipping, doesn't contain denormals
   - Tests coupling input/output
   - Verifies parameter changes produce audible differences
   - Measures CPU usage per block

5. **No JUCE dependency.** The SDK interface uses plain C++ and C types.
   Developers can use JUCE internally if they want, but it's not required.
   `AudioBuffer` in the interface is a simple struct with float pointers + channel count.

### Distribution

- **GitHub repo:** `XO_OX/xomnibus-engine-sdk` (MIT license)
- **Community engines:** Curated gallery on xoox.design with ratings + downloads
- **Submission:** PR to a community-engines registry repo with `validate-engine` output

---

## Part 3: The Engine Forge — A Playable Synthesis Playground

### Concept

**FORGE** (working name, follows XO + O-word if we go with something like FOUNDRY
or FORGE — or we break convention intentionally because this isn't really an
instrument, it's a tool). An engine slot in XOmnibus that isn't a fixed synthesizer —
it's a modular patching environment with a curated set of building blocks.

The goal: **anyone can drag blocks together, hear what happens, and learn what
oscillators, filters, envelopes, and modulation actually do.** No code. No SDK.
Just play.

### Architecture

```
┌─────────────────────────────────────────────┐
│                ENGINE FORGE                  │
│                                             │
│  ┌─────┐   ┌──────┐   ┌──────┐   ┌─────┐  │
│  │ OSC │──▶│FILTER│──▶│ AMP  │──▶│ OUT │  │
│  └─────┘   └──────┘   └──────┘   └─────┘  │
│     ▲          ▲          ▲                 │
│  ┌─────┐   ┌──────┐   ┌──────┐             │
│  │ LFO │   │ ENV  │   │ ENV  │             │
│  └─────┘   └──────┘   └──────┘             │
│                                             │
│  Signal path: drag to connect               │
│  Mod path: drag to modulate                 │
└─────────────────────────────────────────────┘
```

### Building Blocks (Curated, Not Infinite)

The key insight: **don't give people Eurorack.** Give them LEGO. Constrained
choices that always sound good and always teach something.

**Sources (pick 1-2):**

| Block | What It Does | What You Learn |
|-------|-------------|----------------|
| Sine | Pure sine wave | Fundamentals, FM |
| Saw | Sawtooth (band-limited) | Harmonics, subtractive |
| Square | Square/pulse with width | Pulse width, hollow tones |
| Noise | White/pink noise | Noise shaping, percussion |
| Wavetable | 8 basic wavetables | Morphing, spectral content |
| Sample | Single-cycle waveforms (built-in library) | Digital synthesis basics |

**Processors (pick 0-3):**

| Block | What It Does | What You Learn |
|-------|-------------|----------------|
| LP Filter | Resonant low-pass (12/24 dB) | Subtractive synthesis |
| HP Filter | Resonant high-pass | Frequency content |
| BP Filter | Band-pass | Formants, vowels |
| Wavefolder | Soft/hard fold | Harmonic distortion |
| Ring Mod | Multiply two signals | Sidebands, metallic |
| Delay | Short delay with feedback | Comb filtering, echoes |
| Chorus | Stereo detune | Width, motion |

**Modulators (pick 0-4):**

| Block | What It Does | What You Learn |
|-------|-------------|----------------|
| ADSR Envelope | Attack/Decay/Sustain/Release | Shaping over time |
| LFO | Sine/Tri/Saw/Square/S&H | Cyclic modulation |
| Velocity | Maps note velocity | Expression |
| Note Pitch | Maps MIDI note | Keyboard tracking |
| Random | Per-note random value | Variation, humanization |
| Macro Knob | User-assignable macro | Performance control |

### How It Works

1. **Start with a template.** Not a blank canvas. Templates like:
   - "Classic Subtractive" (Saw → LP Filter → Amp, with Env on filter + amp)
   - "FM Bell" (Sine → Sine FM, with Env on modulation depth)
   - "Noise Percussion" (Noise → BP Filter → Amp, fast env)
   - "Wavetable Pad" (Wavetable → LP Filter → Chorus, LFO on morph)

2. **Swap blocks.** Tap a block, see alternatives. Swap Saw for Square — hear the
   difference immediately. Swap LP for Wavefolder — hear what distortion does.

3. **Drag modulation.** Drag from LFO to filter cutoff — see the line, hear the wobble.
   Drag from Envelope to wavetable position — hear the sweep.

4. **Every connection is visible and audible.** No hidden routing. When you drag a
   mod connection, the target parameter visually wobbles to show modulation depth.

5. **"What Changed?" mode.** Toggle A/B between your current patch and the template
   you started from. Hear exactly what your modifications did.

6. **Guided challenges.** Optional prompts:
   - "Make a bass sound using only a square wave and a filter"
   - "Create a pluck by making the filter envelope very short"
   - "Add movement to a pad using an LFO on the filter"
   - "Make a percussion sound without using any oscillators"

### Technical Implementation

The Forge is an engine like any other — it implements `SynthEngine`. The difference
is that its signal graph is runtime-configurable:

```cpp
class ForgeEngine : public SynthEngine {
    // Fixed pool of DSP blocks (no runtime allocation)
    std::array<ForgeOscillator, 2> oscillators;
    std::array<ForgeFilter, 3> filters;
    std::array<ForgeModulator, 4> modulators;
    std::array<ForgeEffect, 3> effects;

    // Patching matrix (which block connects to which)
    // Stored as parameter state — survives preset save/load
    ForgePatchMatrix patchMatrix;

    void renderBlock(...) override {
        // 1. Read patch matrix from parameters
        // 2. Process modulators (they need no audio input)
        // 3. Process sources
        // 4. Process processors in chain order
        // 5. Apply modulation
        // 6. Write output
    }
};
```

**Critical design decisions:**

- **Fixed block pool, not dynamic.** 2 sources + 3 processors + 4 modulators + 3 effects
  = 12 blocks max. Pre-allocated in `prepare()`. Inactive blocks cost zero CPU.
  This avoids audio-thread allocation entirely.
- **Patch state is parameters.** The routing matrix is stored as integer parameters
  (source1_target, mod1_target, mod1_depth, etc.). This means patches are
  standard `.xometa` presets and participate in the full preset system.
- **Coupling-compatible.** Forge outputs coupling signals just like any engine.
  You can couple your hand-built Forge patch with ORBITAL or OUROBOROS.
- **No code generation.** Forge doesn't "export" to a real engine. It IS a real
  engine. Your Forge patch can be a preset alongside ODDFELIX presets.

### Forge Presets as Learning Curriculum

The preset library for Forge IS the learning path:

```
Presets/XOmnibus/Foundation/
    FORGE - Bare Sine.xometa          ← just a sine wave, nothing else
    FORGE - First Filter.xometa       ← sine + LP filter
    FORGE - Envelope Shapes.xometa    ← sine + filter + env (try changing ADSR)
    FORGE - Add Movement.xometa       ← above + LFO on cutoff
    FORGE - Two Oscillators.xometa    ← saw + square, detuned

Presets/XOmnibus/Atmosphere/
    FORGE - Pad From Scratch.xometa   ← wavetable + filter + chorus + slow LFO
    FORGE - Evolving Texture.xometa   ← two sources, cross-modulated

Presets/XOmnibus/Flux/
    FORGE - Pluck Lab.xometa          ← fast env experiments
    FORGE - Noise Drum.xometa         ← noise + BP filter + fast amp env
```

Each preset has `"notes"` in the xometa that explain what's happening and
suggest what to change.

---

## Part 4: Sequencing & Priority

### V1 (Ship)
- 34 engines, current architecture, no expansion API yet
- Forge is NOT in V1 — it's a significant UI + DSP effort

### V1.1 (Foundation)
- **Engine version metadata** — `sinceVersion` in registry, graceful missing-engine handling
- **Begin SDK extraction** — pull `SynthEngine.h` + `CouplingTypes.h` into standalone headers with no JUCE types in the interface boundary
- **Write adapter guide for community** — expand `how_to_write_a_xomnibus_adapter.md` into full SDK getting-started

### V1.2 (SDK Beta)
- **Ship `xomnibus-engine-sdk` repo** — templates, validate-engine CLI, test harness
- **Dynamic engine loading** — scan user engine directory, load `.dylib`/`.so` at startup
- **Parameter namespace registry** — prevent collisions

### V1.3 (Forge)
- **Engine Forge** — the playable synthesis playground
- Ships as engine "FORGE" with param prefix `forge_`
- Template presets + guided challenges
- Full coupling compatibility

### V1.4+ (Community)
- **Community engine gallery** on xoox.design
- **In-app engine browser** — discover and install community engines
- **Forge-to-SDK bridge** — export a Forge patch as an SDK project scaffold
  (the signal graph becomes starter code in a real engine template)

---

## Open Questions

1. **Forge naming.** FORGE doesn't follow XO + O-word. Options:
   - OSCILLARY ("oscillation laboratory")
   - OPUS ("your opus")
   - ORIGINERY ("origin of engines")
   - OBSERVATORY ("observe synthesis")
   - Or break convention — Forge is a tool, not an instrument

2. **Sample loading in community engines.** V1 community engines can't load files
   (security boundary). Do we add a sandboxed sample API in V1.3+?

3. **Community engine code signing.** Do we require notarized/signed binaries?
   Or trust-on-first-use with a warning dialog?

4. **Forge complexity ceiling.** 12 blocks is the proposed limit. Is that enough
   to be interesting? Too much to be approachable? Needs user testing.

5. **Revenue model for community.** Free marketplace? Paid tier for premium
   community engines? Donation model? This affects incentives.
