# Engine Expansion Toolkit — Post-V1 Strategy

> How XOmnibus grows from 34 engines to an open ecosystem,
> and how we give people an ocean of toy bricks to learn synthesis by building.

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

**XObrix** — ocean bricks you snap together to build sound. Not an external SDK
but a playable synthesis sandbox inside XOmnibus itself. A toy you learn from,
guided by Professor Oscar. Details in Part 3.

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

**OBRIX** — ocean bricks for synthesis. Snap pieces together like coral building
a reef. Hear what happens. Learn by building. It's a toy box pulled from the sea.

XObrix is a registered engine in XOmnibus — it occupies a slot just like ODDFELIX
or ORBITAL. But instead of a fixed signal path, its graph is runtime-configurable
from a curated set of building blocks. Because it's a standard engine, you can run
up to 4 OBRIX instances simultaneously in the 4 engine slots — each with a completely
different configuration. Couple them together via the MegaCouplingMatrix and you've
got a user-designed multi-engine instrument built entirely from toy bricks.

The goal: **anyone can snap bricks together, hear what happens, and learn what
oscillators, filters, envelopes, and modulation actually do.** No code. No SDK.
Just play. And when you want to understand *why* it sounds that way, Professor
Oscar is there to explain the science.

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

The key insight: **don't give people Eurorack.** Give them ocean bricks. Constrained
choices that always sound good and always teach something. Each brick type has an
aquatic visual identity — oscillators are shells, filters are coral, modulators
are currents, effects are tide pools.

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

## Part 4: Professor Oscar's Training Mode

### The Character

Oscar the axolotl is already the patient, warm, regenerative half of the XO_OX
origin pair. Where feliX darts and vanishes, Oscar remains. Where feliX is a
flash, Oscar is a tide. He lives in duration. He breathes slowly. He's the
perfect teacher.

**Professor Oscar** appears in OBRIX as an optional training companion — not a
chatbot, not a tutorial wizard, but a quiet presence that explains *why* things
sound the way they do. He teaches the science behind the sound.

### How Training Mode Works

Training mode is a toggle — turn it on, and every action in OBRIX gets context.
Turn it off, and OBRIX is just a toy box. No friction.

**Layer 1: Brick Tooltips (always available)**

When you long-press any brick, Professor Oscar explains what it does in plain
language. Not "12dB/oct resonant low-pass filter" — instead:

> *"This brick removes the bright, buzzy frequencies from your sound.*
> *Turn the cutoff knob down and listen — hear how it gets darker?*
> *That's because higher harmonics are being filtered out.*
> *Every physical object does this naturally — a pillow over a speaker,*
> *your hand over your mouth. This brick gives you that control."*
> — Professor Oscar

**Layer 2: Connection Insights (training mode on)**

When you drag a modulation connection, Oscar explains the relationship:

> *"You just connected your LFO to the filter cutoff.*
> *The LFO is a slow wave — it goes up and down on its own.*
> *Now your filter cutoff will rise and fall with it.*
> *That's the 'wah-wah' sound. Speed up the LFO — it gets faster.*
> *Slow it way down — it becomes a gentle breathing."*
> — Professor Oscar

**Layer 3: Science Cards (training mode on)**

Expandable cards that go deeper. Each brick and connection type has a science
card that explains the physics, math, or psychoacoustics behind it. These are
written for curious beginners, not engineers:

| Topic | Science Card |
|-------|-------------|
| **Why filters sound warm** | Harmonics, overtone series, how acoustic instruments naturally roll off high frequencies. Why a muted guitar string sounds "warm." |
| **What FM synthesis actually is** | Frequency modulation explained with two tuning forks. Why it makes bell sounds. Chowning's discovery at Stanford. |
| **Why detuning creates width** | Beating frequencies, binaural perception, how a chorus of slightly-out-of-tune singers sounds "wide." |
| **What an envelope is** | A letter's journey: Attack (opening it), Decay (the initial rush), Sustain (reading), Release (putting it down). How piano hammers vs. organ keys differ. |
| **Why resonance screams** | Feedback at a specific frequency. Wine glass shattering. Shower acoustics. The Tacoma Narrows Bridge. |
| **What ring modulation does** | Sum and difference frequencies. Why it sounds metallic and "robotic." Dalek voices. |
| **Why noise makes percussion** | All frequencies at once = no pitch. Shaping noise with fast envelopes = snare drums, hi-hats, ocean waves. |
| **What coupling means** | Why two instruments playing together sound different from two instruments playing separately. Sympathetic resonance. Piano strings vibrating without being struck. |

**Layer 4: Waveform Visualizer**

When training mode is on, a real-time oscilloscope appears at the bottom of the
OBRIX view. It shows the waveform at each stage of the signal path:

```
[Source: Saw] ──▶ [Filter: LP] ──▶ [Output]
   ╱╲╱╲╱╲           ∿∿∿∿∿∿          ∿∿∿∿∿∿
   raw saw        filtered         final output
```

As you change parameters, the waveform changes in real time. Connect the dots
between what you hear and what you see. This is synthesis made visible.

**Layer 5: "Why Does It Sound Like That?" Button**

A single button (Oscar's face) that, when pressed, gives a plain-language
description of the current patch:

> *"Right now you have a sawtooth wave — that's the buzzy, bright one —*
> *going through a low-pass filter set fairly dark. An envelope is opening*
> *the filter quickly when you play a note, then closing it slowly.*
> *That's why it goes 'bwow' — bright at the start, then muffled.*
> *Your LFO is gently moving the filter up and down, which adds that*
> *subtle breathing quality. This is basically how a Minimoog bass works."*
> — Professor Oscar

### Professor Oscar's Personality in Training Mode

- **Patient.** Never rushes. Never says "obviously" or "simply."
- **Analogies first.** Always explains with real-world comparisons before technical terms.
- **Encouraging.** "That sounds great" when you make something good. "Try this" when
  you're stuck. Never "that's wrong."
- **Axolotl quirks.** Occasional gentle references to water, breathing, coral, tides.
  "A filter is like water — it absorbs the sharp edges." Not overdone.
- **Progressive depth.** First explanation is simple. Tap "tell me more" for the
  physics. Tap again for the math. Most people never go past the first level, and
  that's fine.
- **Visual identity.** Oscar appears as a small pink axolotl icon (Gill Pink `#E8839B`)
  in the corner of the OBRIX view. Gills animate slowly when he's "speaking."
  Matches the OddOscar engine accent.

### Technical Notes

- Training mode content is stored as structured data, not hardcoded strings.
  This enables localization and future expansion.
- Science cards render as expandable overlays — no navigation away from the
  brick-building interface.
- The waveform visualizer reuses OPTIC's `OpticBandAnalyzer` infrastructure
  (already built for audio-reactive visualization).
- "Why Does It Sound Like That?" uses a rule-based descriptor, not AI.
  It reads the current patch matrix and parameter values and assembles a
  description from templates. Deterministic, instant, works offline.

---

## Part 5: OBRIX Pocket — iPhone-Optimized Interface

### Inspiration

Teenage Engineering Pocket Operators: tiny, immediate, constrained, delightful.
Everything on one screen. No menus. No hidden panels. You see the whole instrument
and you play it. That energy, applied to OBRIX on iPhone.

### Design Philosophy

The desktop/iPad OBRIX interface is a drag-and-connect brick canvas. That works
with a mouse or a large touch screen. On iPhone, we don't shrink that interface
down — we **reimagine it** as a single-screen instrument inspired by pocket
synthesizers.

```
┌─────────────────────────────────────┐
│  OBRIX POCKET              ≡  ?    │
│                                     │
│  ┌─────────────────────────────┐    │
│  │      ∿∿∿  WAVEFORM  ∿∿∿    │    │  ← mini scope (training mode)
│  └─────────────────────────────┘    │
│                                     │
│  ┌──────┐  ┌──────┐  ┌──────┐      │
│  │ SRC  │  │ FLT  │  │ FX   │      │  ← brick selector row
│  │ SAW▾ │  │ LP ▾ │  │ DLY▾ │      │  ← tap to cycle types
│  └──────┘  └──────┘  └──────┘      │
│                                     │
│  ┌──┐ ┌──┐ ┌──┐ ┌──┐ ┌──┐ ┌──┐    │
│  │CT│ │RS│ │AT│ │DC│ │SP│ │DP│    │  ← 6 knobs (context-sensitive)
│  └──┘ └──┘ └──┘ └──┘ └──┘ └──┘    │
│                                     │
│  [MOD: LFO→CT ▾]  [ENV ▾]         │  ← mod routing strip
│                                     │
│  ┌─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┐    │
│  │ │█│ │█│ │ │█│ │█│ │█│ │ │    │  ← mini keyboard / pads
│  └─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┘    │
│                                     │
│  [M1] [M2] [M3] [M4]   ● REC      │  ← macros + record
└─────────────────────────────────────┘
```

### Key Differences from Desktop OBRIX

| Aspect | Desktop/iPad | iPhone Pocket |
|--------|-------------|---------------|
| **Brick selection** | Drag from palette, visual canvas | Tap brick slots to cycle types |
| **Mod routing** | Drag lines between bricks | Tap mod strip — pick source → target from list |
| **Parameters** | Full knob panels per brick | 6 context-sensitive knobs (tap a brick slot to load its params) |
| **Waveform** | Large oscilloscope panel | Mini scope strip at top |
| **Layout** | Freeform canvas | Fixed single-screen grid |
| **Coupling** | Full MegaCouplingMatrix view | Simplified — tap coupling badge to pick type + target |
| **Play** | PlaySurface 4-zone | Mini keyboard or 8-pad drum grid |

### Pocket Interaction Model

1. **Tap a brick slot** (SRC / FLT / FX) → the 6 knobs below load that brick's
   parameters. Cutoff, resonance, attack, decay — whatever's relevant. Labels
   update dynamically. This is the Pocket Operator approach: same knobs, different
   context based on what's selected.

2. **Tap the brick type label** (e.g., "SAW") → cycles through available types
   for that slot. SAW → SQR → SINE → NOISE → WAVE → off. Immediate sonic change.
   Hold to see the full list.

3. **Mod routing strip** shows active modulation routes as compact chips.
   Tap to add: pick modulator (LFO/ENV/VEL/NOTE/RND/MACRO) → pick target
   (any parameter on any brick) → set depth with a slider. Max 4 routes visible,
   swipe for more.

4. **Mini keyboard** at the bottom — 1 octave, swipe left/right to shift octave.
   Or toggle to **8-pad mode** for percussion/drum building. Velocity from
   touch force (existing `MobileTouchHandler` infrastructure).

5. **Record button** — tap to capture a loop. Plays back while you twist knobs.
   Instant gratification for sound design on the go.

### Pocket-Specific Features

**Shake to randomize.** Shake the phone — Oscar randomly reconfigures the bricks.
Uses the existing `SensorManager` accelerometer infrastructure. A fun discovery
tool that produces musically valid results (random within curated constraints,
not pure chaos).

**Gyro-filter.** Tilt the phone to sweep the filter cutoff. Already supported
by `SensorManager.h` motion modes — just needs a default mapping in OBRIX Pocket.

**Tap tempo LFO.** Tap the LFO brick twice to set LFO rate from your tapping
rhythm. Natural on a phone, awkward on desktop.

**Pocket presets.** Presets designed specifically for the 6-knob interface.
The same `.xometa` format, but with `"pocket": true` metadata flag so the
preset browser can filter. Desktop presets still load on Pocket (just with
knobs mapped to the most important 6 params), but Pocket presets are curated
for the constrained interface.

### Training Mode on Pocket

Professor Oscar adapts to the smaller screen:

- **Tooltips** appear as bottom sheets (half-screen slide-up) instead of popovers.
- **Science cards** are swipeable carousel cards, not expandable panels.
- **Waveform visualizer** is the mini scope strip at the top — always visible
  in training mode, hidden otherwise.
- **"Why Does It Sound Like That?"** is triggered by tapping Oscar's face in
  the top-right corner. Response appears as a temporary bottom sheet overlay.
- **Haptic feedback** reinforces learning: different haptic patterns for
  different brick types when you cycle through them. Feel the difference
  between a saw (sharp buzz) and a sine (smooth pulse).

### Visual Identity — Ocean Bricks

The Pocket interface leans into the aquatic identity:

- **Background:** Deep ocean gradient — dark at bottom (keyboard), lighter at
  top (scope). Not the Gallery Model warm white — Pocket OBRIX has its own mood.
- **Brick slots:** Rounded rectangles with subtle wave texture. Active brick
  glows with its type color (shells = warm amber, coral = pink, currents = cyan).
- **Knobs:** Translucent, like sea glass. Ring indicator in aquatic teal.
- **Oscar:** Small animated axolotl in the top-right. Gills pulse with the LFO
  rate when training mode is on. Tappable.
- **Transitions:** When you cycle brick types, the mini scope waveform morphs
  smoothly — like something shifting shape underwater.

### Relationship to Full OBRIX

OBRIX Pocket is **not a separate engine.** It's an alternative UI layer over
the same `ObrixEngine` instance. The DSP is identical. A patch built on Pocket
opens on desktop with the full canvas view, and vice versa.

The existing `MobileLayoutManager` already handles layout mode detection
(`iPhonePortrait`, `iPhoneLandscape`). OBRIX Pocket activates when
`LayoutMode == iPhonePortrait` and the active engine is OBRIX. In landscape
or on iPad, the full canvas interface loads instead.

```
ObrixEngine (DSP — identical everywhere)
    │
    ├── ObrixCanvasView (desktop + iPad)
    │     └── drag-and-connect brick builder
    │
    └── ObrixPocketView (iPhone portrait)
          └── Pocket Operator-style single screen
```

---

## Part 6: OBRIX Visual Identity — Ocean Bricks

### Accent Color

**Reef Teal `#2CB5A0`** — sits between OCEANIC's Phosphorescent Teal (`#00B4A0`)
and ORGANON's Bioluminescent Cyan (`#00CED1`) but distinct from both. Warmer,
more mineral, like sun-bleached coral viewed through clear tropical water.

### Brick Type Visual Language

Every brick category has an aquatic visual metaphor:

| Category | Metaphor | Color Family | Shape |
|----------|----------|-------------|-------|
| **Sources** (oscillators) | Shells | Warm amber / sand `#D4A76A` | Conch spiral icon |
| **Processors** (filters, wavefolder) | Coral | Living pink / reef `#E8839B` | Branching coral icon |
| **Modulators** (LFO, env, velocity) | Currents | Ocean blue / teal `#2CB5A0` | Wave/current icon |
| **Effects** (delay, chorus) | Tide Pools | Deep violet / iridescent `#7B6B8A` | Pool ripple icon |

The brick colors create a natural visual grouping — you can see at a glance
that the pink things are processors and the teal things are modulators,
without reading labels.

### On-Brand but Distinct

OBRIX lives inside the Gallery Model (warm white shell on desktop/iPad) but
has permission to be more playful than other engines:

- **Brick textures** have a subtle tactile quality — slightly 3D, like actual
  toy bricks. Other engines have flat, clean UI. OBRIX gets to be toylike.
- **Animations** are bouncier — bricks snap into place with a satisfying
  micro-bounce. Connections draw with a fluid, underwater feel.
- **Sound on interaction** — optional subtle water/click sounds when snapping
  bricks together (off by default, toggle in settings).

On iPhone Pocket, OBRIX drops the Gallery Model warm white entirely and goes
full ocean — dark gradient background, luminous bricks, underwater atmosphere.
This is deliberate: Pocket OBRIX is a different *experience*, not just a
smaller layout.

---

## Part 7: Easter Egg Brick Combos

### The Idea

Certain brick combinations trigger a **secret reskin** — the OBRIX instance
gets a custom name, custom visual treatment, and Professor Oscar reacts with
delight. These break every naming convention on purpose. They're toys. They're
fun. They reward curiosity.

The user doesn't know these exist until they stumble into one. When the combo
triggers, the bricks flash, Oscar does a little dance, and the patch name
auto-fills with the Easter egg name. The user can keep it or rename it.

### Known Combos

| Combo | Easter Egg Name | Visual Reskin |
|-------|----------------|---------------|
| 2× Saw + Wavefolder | **CHAIN SAW MAN** | Bricks turn warning orange, jagged teeth on the scope |
| Sine + Sine (FM routed) | **BELL BOY** | Bricks turn brass gold, scope shows bell curve decay |
| Noise + BP Filter + fast Env | **SNARE DRUM MACHINE** | Bricks turn chrome, military stencil font |
| 2× Square (detuned) + Chorus | **CHIPTUNE KID** | Bricks turn pixel green, 8-bit font, scope pixelates |
| Wavetable + slow LFO + Delay | **DEEP SEA DIVER** | Bricks turn deep blue, bubbles rise in background |
| Noise + LP Filter (cutoff very low) | **OCEAN FLOOR** | Bricks go abyssal dark, bioluminescent glow edges |
| Sine + Ring Mod + Sine | **ROBOT VOICE** | Bricks turn steel, Dalek-style modulated scope |
| All modulators active, no effects | **CHAOS THEORY** | Bricks jitter subtly, Lorenz attractor on scope |
| 2× Source + 3× Filter (all slots used) | **FILTER MANIAC** | Bricks turn neon pink, Professor Oscar puts on sunglasses |
| Single bare Sine, nothing else | **PURE THOUGHT** | Everything fades to white except the sine wave. Silence around simplicity. |

### Rules for Easter Eggs

- **Detection is parameter-based.** The combo check runs when any brick type
  changes — reads `obrix_source1_type`, `obrix_proc1_type`, etc. No audio
  analysis needed, just parameter pattern matching.
- **Reskins are cosmetic only.** The DSP is identical — the Easter egg name
  doesn't change the sound. It celebrates what the user already built.
- **Oscar reacts.** Each Easter egg has a unique Professor Oscar line:
  - CHAIN SAW MAN: *"Two saws and a fold? You've built a monster. I love it."*
  - PURE THOUGHT: *"Just a sine wave. Nothing else. Sometimes the simplest thing is the most beautiful."*
  - DEEP SEA DIVER: *"Ah, you found my home."*
- **Reskins persist in presets.** If you save while an Easter egg is active,
  the preset stores the Easter egg name. Loading it restores the reskin.
  The `"easterEgg"` field in `.xometa` is optional.
- **Discoverable but not documented.** No menu lists them. No achievement
  screen. Just play, stumble, discover. Word of mouth is the distribution.
- **Community can't add Easter eggs** (first-party only). This keeps them
  rare, surprising, and on-brand.

### Professor Oscar's Easter Egg Reaction System

When an Easter egg triggers, Oscar's response follows a pattern:

1. **Gills flash** (pink pulse animation)
2. **Short text bubble** appears near his icon (2-3 seconds)
3. **Scope reskin** applies with a smooth 0.5s crossfade
4. **Brick colors** shift to the Easter egg palette over 0.3s
5. **Haptic** — a unique "discovery" haptic pattern (distinct from all other haptics)

If the user breaks the combo (swaps a brick), the reskin fades back to normal
over 0.5s. Oscar says nothing — no guilt, no loss. Just back to building.

---

## Part 8: Sequencing & Priority

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
- **XObrix** — the ocean brick synthesis toy box
- Ships as engine "Obrix" with param prefix `obrix_`, accent color Reef Teal `#2CB5A0`
- Starter kit presets + snap challenges
- Multi-slot support (up to 4 OBRIX instances, each independently configured)
- Cross-instance coupling via MegaCouplingMatrix
- Full coupling compatibility with all existing engines
- Professor Oscar training mode (tooltips, connection insights, science cards, waveform visualizer)
- Easter egg brick combos (CHAIN SAW MAN, DEEP SEA DIVER, PURE THOUGHT, etc.)
- **OBRIX Pocket** — iPhone-optimized Pocket Operator-inspired single-screen interface
- Aquatic visual identity (ocean gradient, shell/coral/current/tide pool brick types)

### V1.4+ (Community)
- **Community engine gallery** on xoox.design
- **In-app engine browser** — discover and install community engines
- **OBRIX-to-SDK bridge** — export an OBRIX patch as an SDK project scaffold
  (the brick graph becomes starter code in a real engine template)

---

## Open Questions

1. **Sample loading in community engines.** V1 community engines can't load files
   (security boundary). Do we add a sandboxed sample API in V1.3+?

2. **Community engine code signing.** Do we require notarized/signed binaries?
   Or trust-on-first-use with a warning dialog?

3. **OBRIX complexity ceiling.** 12 bricks per instance is the proposed limit.
   With 4 slots that's 48 bricks total — probably plenty. But does a single
   instance need more than 12? Needs user testing.

4. **Revenue model for community.** Free marketplace? Paid tier for premium
   community engines? Donation model? This affects incentives.

5. **OBRIX multi-slot presets.** When a preset uses 3 OBRIX slots + 1 ORBITAL,
   should the preset browser show it as an OBRIX preset, an ORBITAL preset,
   or a new "Multi" category? Current preset system is per-engine.

6. **Training mode depth vs. overwhelm.** Five layers of training content is a
   lot. Should layers 3-5 (science cards, scope, "why does it sound like that")
   unlock progressively as the user builds more patches? Or always available?

7. **Easter egg expansion.** Should we ship with 10 combos and add more in
   updates? Community-suggested combos (but first-party implemented)?

8. **Pocket landscape mode.** iPhone landscape could show a wider brick canvas
   (closer to iPad). Or it could show a split: keyboard left, bricks right.
   Needs prototyping.

9. **Oscar localization.** Training mode text needs translation. Oscar's
   personality needs to survive localization — casual analogies don't always
   translate well. Consider hiring writers per language, not just translators.
