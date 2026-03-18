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

### Channel C: OBRIX — The Synthesis Toy Box (see Part 3)

**XObrix** — synthesis building blocks you snap together like toy bricks.
Not an external SDK but a playable synthesis sandbox inside XOmnibus itself.
A toy you learn from. Details in Part 3.

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

## Part 3: OBRIX — The Synthesis Toy Box

### Concept

**OBRIX** — toy bricks for synthesis. The name says it all: snap pieces together,
hear what happens, learn by building. It's a LEGO set where every brick makes sound.

XObrix is a registered engine in XOmnibus — it occupies a slot just like ODDFELIX
or ORBITAL. But instead of a fixed signal path, its graph is runtime-configurable
from a curated set of building blocks. Because it's a standard engine, you can run
up to 4 OBRIX instances simultaneously in the 4 engine slots — each with a completely
different configuration. Couple them together via the MegaCouplingMatrix and you've
got a user-designed multi-engine instrument built entirely from toy bricks.

The goal: **anyone can snap blocks together, hear what happens, and learn what
oscillators, filters, envelopes, and modulation actually do.** No code. No SDK.
Just play.

### Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│  SLOT 1: OBRIX              SLOT 2: OBRIX                          │
│  ┌─────┐  ┌──────┐  ┌───┐  ┌─────┐  ┌──────┐  ┌───┐              │
│  │ SAW │─▶│ LP   │─▶│OUT│  │NOISE│─▶│ BP   │─▶│OUT│              │
│  └─────┘  └──────┘  └───┘  └─────┘  └──────┘  └───┘              │
│     ▲        ▲                 ▲        ▲                          │
│  ┌─────┐  ┌─────┐          ┌─────┐  ┌─────┐                       │
│  │ LFO │  │ ENV │          │ VEL │  │ ENV │                       │
│  └─────┘  └─────┘          └─────┘  └─────┘                       │
│           ║ coupling ║                                             │
│           ╚══════════╝                                             │
│  Each OBRIX is independent — different blocks, different patch.    │
│  Couple them together for cross-engine modulation.                 │
└─────────────────────────────────────────────────────────────────────┘
```

### Multi-Slot OBRIX

Because OBRIX is a standard `SynthEngine`, you can load it into any or all 4
engine slots. Each instance maintains its own block configuration and patch state
independently. This means:

- **Slot 1:** Subtractive bass (Saw → LP Filter → Amp)
- **Slot 2:** Noise percussion (Noise → BP Filter → fast Env)
- **Slot 3:** FM texture (Sine × Sine → Wavefolder → Chorus)
- **Slot 4:** ORBITAL (or any other "real" engine)

All 4 slots participate in the MegaCouplingMatrix as usual. Route the amplitude
of your OBRIX bass into the filter cutoff of your OBRIX percussion. Couple your
FM texture into ORBITAL's wavetable position. The user has effectively designed
a custom multi-engine synthesizer from toy bricks — no code, no SDK, just play.

Each slot's OBRIX configuration saves independently in `.xometa` presets. A
4-slot preset with 3 OBRIX instances and 1 ORBITAL is a perfectly valid XOmnibus
preset file.

### Building Blocks (Curated, Not Infinite)

The key insight: **don't give people Eurorack.** Give them LEGO bricks. Constrained
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

1. **Start with a starter kit.** Not a blank canvas. Starter kits like:
   - "Classic Subtractive" (Saw → LP Filter → Amp, with Env on filter + amp)
   - "FM Bell" (Sine → Sine FM, with Env on modulation depth)
   - "Noise Percussion" (Noise → BP Filter → Amp, fast env)
   - "Wavetable Pad" (Wavetable → LP Filter → Chorus, LFO on morph)
   - "Multi-Slot Jam" (loads 3 OBRIX slots with complementary configs + coupling)

2. **Swap bricks.** Tap a block, see alternatives. Swap Saw for Square — hear the
   difference immediately. Swap LP for Wavefolder — hear what distortion does.

3. **Drag modulation.** Drag from LFO to filter cutoff — see the line, hear the wobble.
   Drag from Envelope to wavetable position — hear the sweep.

4. **Every connection is visible and audible.** No hidden routing. When you drag a
   mod connection, the target parameter visually wobbles to show modulation depth.

5. **"What Changed?" mode.** Toggle A/B between your current patch and the starter
   kit you began with. Hear exactly what your modifications did.

6. **Snap challenges.** Optional build prompts:
   - "Make a bass sound using only a square wave and a filter"
   - "Create a pluck by making the filter envelope very short"
   - "Add movement to a pad using an LFO on the filter"
   - "Make a percussion sound without using any oscillators"
   - "Load 2 OBRIX slots and couple them — make them interact"

### Technical Implementation

OBRIX is an engine like any other — it implements `SynthEngine`. The difference
is that its signal graph is runtime-configurable:

```cpp
class ObrixEngine : public SynthEngine {
    // Fixed pool of DSP bricks (no runtime allocation)
    std::array<ObrixOscillator, 2> oscillators;
    std::array<ObrixFilter, 3> filters;
    std::array<ObrixModulator, 4> modulators;
    std::array<ObrixEffect, 3> effects;

    // Patching matrix (which brick connects to which)
    // Stored as parameter state — survives preset save/load
    ObrixPatchMatrix patchMatrix;

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

- **Fixed brick pool, not dynamic.** 2 sources + 3 processors + 4 modulators + 3 effects
  = 12 bricks max per instance. Pre-allocated in `prepare()`. Inactive bricks cost
  zero CPU. This avoids audio-thread allocation entirely.
- **Patch state is parameters.** The routing matrix is stored as integer parameters
  (`obrix_source1_type`, `obrix_mod1_target`, `obrix_mod1_depth`, etc.). This means
  patches are standard `.xometa` presets and participate in the full preset system.
- **Multi-instance safe.** Each OBRIX instance in a different slot gets its own
  independent brick pool and patch matrix. The parameter prefix handles namespacing
  per slot automatically (same as any other engine in multi-slot configs).
- **Coupling-compatible.** OBRIX outputs coupling signals just like any engine.
  Couple your hand-built OBRIX patch with ORBITAL, OUROBOROS — or another OBRIX.
- **No code generation.** OBRIX doesn't "export" to a real engine. It IS a real
  engine. Your OBRIX patch sits alongside ODDFELIX presets in the browser.

### OBRIX Presets as Learning Curriculum

The preset library for OBRIX IS the learning path:

```
Presets/XOmnibus/Foundation/
    OBRIX - Bare Sine.xometa          ← just a sine wave, nothing else
    OBRIX - First Filter.xometa       ← sine + LP filter
    OBRIX - Envelope Shapes.xometa    ← sine + filter + env (try changing ADSR)
    OBRIX - Add Movement.xometa       ← above + LFO on cutoff
    OBRIX - Two Oscillators.xometa    ← saw + square, detuned

Presets/XOmnibus/Atmosphere/
    OBRIX - Pad From Scratch.xometa   ← wavetable + filter + chorus + slow LFO
    OBRIX - Evolving Texture.xometa   ← two sources, cross-modulated

Presets/XOmnibus/Entangled/
    OBRIX - Coupled Pair.xometa       ← 2 OBRIX slots, coupled via AmpToFilter
    OBRIX - Brick Orchestra.xometa    ← 3 OBRIX slots, each with different role
    OBRIX - Meets Orbital.xometa      ← OBRIX slot 1 + ORBITAL slot 2, coupled

Presets/XOmnibus/Flux/
    OBRIX - Pluck Lab.xometa          ← fast env experiments
    OBRIX - Noise Drum.xometa         ← noise + BP filter + fast amp env
```

Each preset has `"notes"` in the xometa that explain what's happening and
suggest what to change. Multi-slot presets include notes about the coupling
routes and why they produce the interaction they do.

---

## Part 4: Sequencing & Priority

### V1 (Ship)
- 34 engines, current architecture, no expansion API yet
- OBRIX is NOT in V1 — it's a significant UI + DSP effort

### V1.1 (Foundation)
- **Engine version metadata** — `sinceVersion` in registry, graceful missing-engine handling
- **Begin SDK extraction** — pull `SynthEngine.h` + `CouplingTypes.h` into standalone headers with no JUCE types in the interface boundary
- **Write adapter guide for community** — expand `how_to_write_a_xomnibus_adapter.md` into full SDK getting-started

### V1.2 (SDK Beta)
- **Ship `xomnibus-engine-sdk` repo** — templates, validate-engine CLI, test harness
- **Dynamic engine loading** — scan user engine directory, load `.dylib`/`.so` at startup
- **Parameter namespace registry** — prevent collisions

### V1.3 (OBRIX)
- **XObrix** — the synthesis toy box
- Ships as engine "Obrix" with param prefix `obrix_`
- Starter kit presets + snap challenges
- Multi-slot support (up to 4 OBRIX instances, each independently configured)
- Cross-instance coupling via MegaCouplingMatrix
- Full coupling compatibility with all existing engines

### V1.4+ (Community)
- **Community engine gallery** on xoox.design
- **In-app engine browser** — discover and install community engines
- **OBRIX-to-SDK bridge** — export an OBRIX patch as an SDK project scaffold
  (the brick graph becomes starter code in a real engine template)

---

## Open Questions

1. **OBRIX accent color.** Needs to be distinct from existing 34 engines.
   Candidates: Construction Yellow `#FFD700`? Brick Red `#CB4154`? Something
   that says "toy" and "build."

2. **Sample loading in community engines.** V1 community engines can't load files
   (security boundary). Do we add a sandboxed sample API in V1.3+?

3. **Community engine code signing.** Do we require notarized/signed binaries?
   Or trust-on-first-use with a warning dialog?

4. **OBRIX complexity ceiling.** 12 bricks per instance is the proposed limit.
   With 4 slots that's 48 bricks total — probably plenty. But does a single
   instance need more than 12? Needs user testing.

5. **Revenue model for community.** Free marketplace? Paid tier for premium
   community engines? Donation model? This affects incentives.

6. **OBRIX multi-slot presets.** When a preset uses 3 OBRIX slots + 1 ORBITAL,
   should the preset browser show it as an OBRIX preset, an ORBITAL preset,
   or a new "Multi" category? Current preset system is per-engine.
