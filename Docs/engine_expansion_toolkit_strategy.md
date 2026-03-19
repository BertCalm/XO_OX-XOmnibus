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

> **⚠️ BOARD RESOLUTION (2026-03-18): Channel B same-process `.dylib` loading
> DEFERRED to V2 minimum.** D2 (Security) found that the stated safety model
> (watchdog + constraint declarations) cannot enforce "no threads, no file I/O,
> no network" at the application layer — a loaded `.dylib` can call `pthread_create`,
> `fopen`, or `mmap(..., PROT_EXEC)` without restriction. Memory corruption can
> occur before any watchdog fires. Enforcement requires OS-level process isolation,
> which defeats real-time audio requirements.
>
> **V1.x community expansion proceeds via the preset-only community pack program**
> (`.xometa` + sample bundles, no binary loading). This achieves community economics
> safely. The SDK technical design below is preserved as the V2 target architecture,
> pending a process-isolation solution (separate audio process + IPC, or Apple/MS
> code-signing with notarization).

The `SynthEngine` interface is already clean enough to serve as a public API — 10
methods, well-documented responsibilities, no hidden dependencies. The work is
making it *safe* and *discoverable*. **The architecture below is the V2 target.**

**Technical architecture (V2 target — not V1.x):**

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
    ├── REQUIRED: Code signing + notarization (macOS) / Authenticode (Windows)
    ├── REQUIRED: Process isolation or OS-level sandboxing
    └── Registers validated engines into EngineRegistry
```

**Safety model (V2 — enhanced per Board D2 findings):**

Community engines must run with verifiable trust boundaries:

| Constraint | How |
|-----------|-----|
| Code signing | Mandatory — Apple Developer ID + notarization (macOS) |
| Interface version check | Engine declares `XOMNIBUS_API_VERSION` — reject mismatches |
| Parameter namespace collision | Reject if prefix collides with existing engine |
| Engine ID collision | Reject duplicate IDs |
| Process isolation | Separate audio process + IPC (latency ~1-5ms) — prevents memory corruption |
| Resource limits | Engines declare max voices and max buffer size in metadata |
| No UI injection | Community engines have no UI access — XOmnibus auto-generates controls from parameter layout |

**What community engines CAN do:**
- Full DSP (any synthesis technique)
- Declare parameters with standard types (float, choice, bool)
- Accept and produce coupling signals (all 13 types)
- Ship `.xometa` presets
- Define macro mappings

**What community engines CANNOT do (enforced by process isolation):**
- Custom UI (auto-generated from parameters)
- Access host memory (process boundary)
- File I/O beyond declared sandbox paths
- Network access
- Spawn threads beyond declared limit

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
        // 1. Snapshot patch matrix into BrixPatchState (per-block, atomic)
        // 2. Process modulators (they need no audio input)
        // 3. Process sources in snapshot order
        // 4. Process processors in chain order per snapshot
        // 5. Apply modulation
        // 6. Denormal flush on all feedback accumulators
        // 7. Write output
    }
};
```

**Critical design decisions:**

- **Fixed brick pool, not dynamic.** 2 sources + 3 processors + 4 modulators + 3 effects
  = 12 bricks max per instance. Pre-allocated in `prepare()`. Inactive bricks cost
  zero CPU. This avoids audio-thread allocation entirely.
- **ParamSnapshot for routing (Board D6 mandate).** The patch matrix is captured into
  a `BrixPatchState` struct at the start of each `renderBlock()`. UI parameter changes
  take effect at the next block boundary (≤50ms latency). This prevents race conditions
  between the UI thread writing routing parameters and the audio thread reading them.
- **Denormal protection (Board D6 mandate).** All feedback paths in the brick DSP
  (comb filter, feedback delay, wavefolder, ring mod) must include denormal flush:
  `if (std::abs(v) < 1e-30f) v = 0.0f;` — required by XOmnibus Architecture Rules.
- **Patch state is parameters.** The routing matrix is stored as integer parameters
  (`obrix_source1_type`, `obrix_mod1_target`, `obrix_mod1_depth`, etc.). This means
  patches are standard `.xometa` presets and participate in the full preset system.
- **Multi-instance safe.** Each OBRIX instance in a different slot gets its own
  independent brick pool and patch matrix. Slot-level namespacing uses the standard
  `slot_N_` prefix mechanism (same as any other engine in multi-slot configs).
  Test: 4-instance preset save/load must produce no parameter collision.
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

### Professor Oscar — Character Art Pipeline

Oscar needs to be: simple enough to read at 24px, charming enough to feel alive,
consistent across every context, animatable with smooth organic motion, and
reproducible by anyone working on the project without artistic interpretation drift.

**The approach: Rive (rive.app) — vector animation with state machines.**

#### Why Rive

| Requirement | How Rive Solves It |
|-------------|-------------------|
| **Simple** | Vector-based — Oscar is built from a small number of paths and shapes. Clean at any resolution. |
| **Consistent** | One `.riv` source file = one canonical Oscar. No re-drawing per context. Every instance renders from the same file. |
| **Repeatable** | The Rive editor is visual — any designer or developer opens the file and sees exactly how Oscar is built. No "only the original artist can draw him." |
| **Animatable** | Rive's state machine system drives animations from code events. Gill pulse, celebration, swimming — all driven by parameters, not timeline scrubbing. |
| **Lightweight** | `.riv` files are tiny (typically 10–50 KB). No sprite sheets, no video, no heavy runtimes. |
| **Cross-platform** | Rive has native runtimes for C++ (works with JUCE), iOS (native), and web. Same file everywhere. |
| **Interactive** | State machines respond to inputs — Oscar's gill speed can be driven by LFO rate, his expression can change on Easter egg discovery, his form can shift across levels. All from code, no new art needed. |

#### Why Not Other Options

| Option | Why Not |
|--------|---------|
| **Lottie (After Effects → JSON)** | Great for one-shot animations, weak for interactive state machines. Oscar isn't a loading spinner — he reacts to what you do. Lottie's interactivity is bolted on; Rive's is native. Also, Lottie requires After Effects ($$$) to author. |
| **SVG + CSS animation** | Works for web, doesn't work in JUCE (native C++ UI). We'd need two separate animation systems for desktop and web. SVG animation also gets complex fast for organic motion like gills. |
| **Sprite sheets** | Pixel-based = blurry at some sizes. Large file sizes for smooth animation. Can't be parameter-driven (fixed frame count). No resolution independence. |
| **JUCE hand-coded drawing** | Could work technically (JUCE has Path, AffineTransform, etc.) but means Oscar's art lives in C++ code. No visual editor. Impossible for a designer to adjust. Maintenance nightmare. |
| **3D (Three.js, SceneKit)** | Massive overkill for a small character icon. Heavy runtime. Wrong aesthetic — Oscar should feel 2D, illustrated, warm. |

#### Oscar's Visual Construction

Oscar is built from ~15 vector shapes:

```
OSCAR — Anatomical Breakdown (vector paths)

Head:           Rounded rectangle with soft corners (main body)
Eyes:           2× circles with offset inner circles (pupils)
                Pupils track toward active bricks (subtle parallax)
Mouth:          Simple arc — 3 states: neutral, smile, "ooh"
Gills:          3× feathery paths on each side (6 total)
                These are the primary animation surface
                Each gill is a bezier curve that oscillates independently
Body:           Tapered oval below head (only visible in larger renders)
Legs:           4× small rounded stubs (only visible ≥ 48px)
Tail:           Single curved path with gentle wave (only visible ≥ 48px)
Spots:          3-5 small circles on body (OddOscar's characteristic spots)
```

**Color palette (from OddOscar accent):**

| Part | Color | Hex |
|------|-------|-----|
| Body | Gill Pink | `#E8839B` |
| Body shadow | Deeper pink | `#D06B83` |
| Gills | Lighter pink (translucent) | `#F0A0B4` at 70% opacity |
| Eyes | Near-black | `#2D2D2D` |
| Pupils | Black | `#1A1A1A` |
| Mouth | Dark pink | `#C05A72` |
| Spots | Slightly different pink | `#D4778D` |
| Belly | Warm white | `#F5E6EA` |

At small sizes (16–24px), Oscar simplifies: head + eyes + gills only. No body,
no legs, no tail. The gills are the identity — they're what moves, what
communicates life.

#### State Machine Architecture

Rive state machines let Oscar's behavior be event-driven from code. No timeline
animation — just states and transitions:

```
┌─────────────┐
│   IDLE       │ ← Default state
│  Gills: slow │    Gentle breathing rhythm (0.5 Hz)
│  Eyes: center│    Pupils centered
│  Mouth: —    │    Neutral line
└──────┬───────┘
       │
       ├── onBrickChange ──────▶ ┌──────────────┐
       │                         │   WATCHING     │
       │                         │  Gills: medium │  Eyes track toward
       │                         │  Eyes: track   │  the changed brick
       │                         │  Mouth: —      │
       │                         └──────┬─────────┘
       │                                │ (1.5s) → back to IDLE
       │
       ├── onEasterEgg ────────▶ ┌──────────────┐
       │                         │  CELEBRATION   │
       │                         │  Gills: flash  │  Rapid pink pulse
       │                         │  Eyes: wide    │  Pupils dilate
       │                         │  Mouth: "ooh"  │  Open circle
       │                         │  + body bounce │  Subtle Y-axis bounce
       │                         └──────┬─────────┘
       │                                │ (2.5s) → SPEAKING
       │
       ├── onSpeaking ─────────▶ ┌──────────────┐
       │                         │   SPEAKING     │
       │                         │  Gills: rhyth  │  Pulse matches speech
       │                         │  Eyes: engaged │  Pupils toward text
       │                         │  Mouth: open   │  Gentle open/close
       │                         └──────┬─────────┘
       │                                │ (text done) → IDLE
       │
       ├── onBossActive ───────▶ ┌──────────────┐
       │                         │   ALERT        │
       │                         │  Gills: tense  │  Gills held slightly out
       │                         │  Eyes: focused │  Pupils toward canvas
       │                         │  Mouth: —      │  Determined neutral
       │                         └──────┬─────────┘
       │                                │ (boss defeated) → CELEBRATION
       │
       └── onLevelUp ──────────▶ ┌──────────────┐
                                 │  EVOLUTION     │
                                 │  Gills: glow   │  Bioluminescent flash
                                 │  Body: morph   │  Shape shifts to next form
                                 │  Eyes: close→  │  Close, reopen with new
                                 │        open    │  appearance
                                 └──────┬─────────┘
                                        │ (3s) → IDLE (new form)
```

**Driving from code (C++ / JUCE integration):**

```cpp
// Rive C++ runtime integration
// Oscar responds to OBRIX events via state machine inputs

class OscarCharacter {
    rive::File* riveFile;
    rive::ArtboardInstance* artboard;
    rive::StateMachineInstance* stateMachine;

    // State machine inputs (defined in Rive editor, referenced by name)
    rive::SMIBool*   isSpeaking;
    rive::SMIBool*   isEasterEgg;
    rive::SMIBool*   isBossActive;
    rive::SMINumber* gillSpeed;      // Driven by LFO rate in training mode
    rive::SMINumber* pupilTargetX;   // Driven by active brick position
    rive::SMINumber* pupilTargetY;
    rive::SMINumber* evolutionStage; // 0=baby, 1=adolescent, 2=adult, 3=elder
    rive::SMITrigger* triggerCelebration;
    rive::SMITrigger* triggerEvolution;

    void onEasterEggDiscovered() {
        triggerCelebration->fire();
    }

    void onLFORateChanged(float rateHz) {
        // Oscar's gills breathe with the LFO in training mode
        gillSpeed->value(rateHz);
    }

    void onBrickFocused(float normX, float normY) {
        // Oscar watches what you're doing
        pupilTargetX->value(normX);
        pupilTargetY->value(normY);
    }

    void onLevelCleared(int newLevel) {
        evolutionStage->value(static_cast<float>(newLevel));
        triggerEvolution->fire();
    }

    void render(juce::Graphics& g, juce::Rectangle<float> bounds) {
        // Rive renderer draws Oscar into the JUCE graphics context
        // Artboard scales to bounds — works at any size
        artboard->advance(deltaTime);
        stateMachine->advance(deltaTime);
        renderer->draw(artboard.get());
    }
};
```

#### Oscar Across Levels — One File, Four Forms

Oscar's evolution (baby → adolescent → adult → elder) is handled *within the
same `.riv` file* using Rive's artboard variants or blend states:

| Level | Form | What Changes in the Rive File |
|-------|------|------------------------------|
| 1 | Baby | Base shape. Simple gills (3 per side). Large head-to-body ratio. |
| 2 | Adolescent | Gills more elaborate (5 per side, more bezier complexity). Body slightly longer. Eyes brighter (inner glow layer activates). |
| 3 | Adult | Bioluminescent spots activate (new shape layer, subtle glow animation). Gills fully feathered. Body proportions more balanced. |
| 4 | Elder | Body becomes semi-translucent (opacity drop on body fill). Internal glow (radial gradient from center). Gills ethereal (blur/glow effect). Fewer spots — simpler, more refined. |

The evolution transition (`triggerEvolution`) morphs between forms over 3
seconds using Rive's blend states — shapes interpolate smoothly from one form
to the next. This is one of Rive's strengths: because everything is vector
paths, you can smoothly interpolate between path shapes.

#### Sizes and Contexts

| Context | Size | Detail Level | Oscar Shows |
|---------|------|-------------|-------------|
| OBRIX corner icon | 24–32px | Minimal | Head + eyes + gills only |
| Training mode tooltip | 40–48px | Medium | Head + eyes + gills + body |
| BrixBox header | 64px | Full | Full body including legs + tail |
| Boss intro screen | 128–200px | Hero | Full body, centered, dramatic lighting |
| Secret ending | Full canvas | Cinematic | Oscar swims across screen, full detail |

Rive handles this naturally — the artboard scales, and detail layers can be
toggled based on rendered size (via state machine inputs):

```
if rendered width < 36px → hide body, legs, tail, spots
if rendered width < 56px → hide legs, tail
if rendered width ≥ 56px → show everything
```

#### Production Workflow

**Step 1: Design Oscar in Rive Editor (rive.app)**
- Free tier is sufficient for this complexity
- Visual editor — drag paths, set keyframes, build state machine
- The character lives as a single `.riv` file in the repo

**Step 2: Export `.riv` binary**
- Tiny file (~20–50 KB for a character this simple)
- Binary format, version-controlled in `Assets/Characters/oscar.riv`
- Source `.rev` (Rive editor format) kept alongside for editability

**Step 3: Integrate via Rive C++ runtime**
- Rive provides an open-source C++ runtime (MIT license)
- Renders to any graphics backend (OpenGL, Metal, Skia, software)
- JUCE integration: render into a `juce::Image` or directly to a
  `juce::Graphics` context via a thin adapter
- Runtime is lightweight — adds ~200 KB to binary, no heavy dependencies

**Step 4: Wire state machine inputs to OBRIX events**
- Map OBRIX parameter changes to Oscar's state machine inputs
- All behavior is data-driven — no animation code in C++, just event firing
- New Oscar behaviors added in Rive editor, not in code

**Source files in repo:**

```
Assets/
└── Characters/
    ├── oscar.riv              # Compiled Rive binary (what ships)
    ├── oscar.rev              # Rive editor source (for editing)
    ├── oscar_design_notes.md  # Character sheet, color specs, state docs
    └── oscar_test.html        # Standalone test page (Rive web runtime)
```

#### Oscar Design Principles (for anyone editing the `.riv`)

1. **Gills are the soul.** If you can only animate one thing, animate the gills.
   They communicate breathing, excitement, tension, evolution. Everything else
   is secondary.
2. **Eyes create connection.** Pupil tracking toward bricks makes Oscar feel
   aware. The parallax is subtle (2-3px movement max at small sizes) but it
   registers subconsciously.
3. **Less is more at small sizes.** At 24px, Oscar is basically a pink blob with
   eyes and feathery edges. That's enough. Don't try to cram detail into small
   renders.
4. **Pink is the brand.** `#E8839B` is non-negotiable. It's the OddOscar accent
   color. All other colors derive from it. If Oscar doesn't read as "pink
   axolotl" at a glance, something is wrong.
5. **Organic motion only.** No snappy, mechanical transitions. Everything eases
   in and out. Gills use sine-based oscillation. Body bounces use spring
   physics. Oscar is a living creature, not a UI widget.
6. **Accessibility.** Respect `prefers-reduced-motion`. When reduced motion is
   on, gills hold still, pupils don't track, transitions are instant cuts.
   Oscar is still present but static.

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

**Reef Jade `#1E8B7E`** — green-skewed to create clear separation from OCEANIC's
Phosphorescent Teal (`#00B4A0`) and ORGANON's Bioluminescent Cyan (`#00CED1`).
Warm, mineral, like sun-bleached coral viewed through clear tropical water.

> **Board D3 resolution (2026-03-18):** Original `#2CB5A0` was only 12 hex units
> from OCEANIC — visually ambiguous at MPC Store thumbnail scale. Shifted to
> `#1E8B7E` (Reef Jade) for perceptual distinctness. Verify at 1x/2x/3x before shipping.

### Brick Type Visual Language

Every brick category has an aquatic visual metaphor:

| Category | Metaphor | Color | Shape | Pattern (colorblind-safe) |
|----------|----------|-------|-------|--------------------------|
| **Sources** (oscillators) | Shells | Warm neutral `#D4A76A` | Conch spiral icon | Diagonal stripes |
| **Processors** (filters, wavefolder) | Coral | Dark neutral `#3A3A3A` | Branching coral icon | Dot pattern |
| **Modulators** (LFO, env, velocity) | Currents | **Accent: Reef Jade `#1E8B7E`** | Wave/current icon | Horizontal stripes |
| **Effects** (delay, chorus) | Tide Pools | Dark neutral `#3A3A3A` | Pool ripple icon | Gradient pattern |

> **Board D8 resolution (2026-03-18):** Collapsed from 4 distinct colors to
> accent + 2 neutrals. Reef Jade is the dominant visual voice (modulators +
> focus states). Icon differentiation + pattern overlays provide colorblind-safe
> category identification. The original 4-color scheme (amber/pink/teal/violet)
> diluted the accent color's meaning — only modulators wore it, making the
> engine's identity invisible in screenshots and social media.

### On-Brand but Distinct

OBRIX lives inside the Gallery Model (warm white shell on desktop/iPad) but
has permission to be more playful than other engines:

- **Brick textures** have a subtle tactile quality — slightly 3D, like actual
  toy bricks. Other engines have flat, clean UI. OBRIX gets to be toylike.
- **Animations** are bouncier — bricks snap into place with a satisfying
  micro-bounce. Connections draw with a fluid, underwater feel.
- **Sound on interaction** — optional subtle water/click sounds when snapping
  bricks together (off by default, toggle in settings).

On iPhone Pocket, OBRIX stays within the Gallery Model warm white shell —
the shell contracts on small screens but does not disappear. Pocket's
playfulness comes from larger tactile brick buttons, snap haptics, optional
water/click sounds, and increased Oscar presence (20% of screen vs 5%).

> **Board D8 resolution (2026-03-18):** Removed the dark ocean gradient
> exception. Gallery Model is the unifying visual language — if OBRIX Pocket
> gets an exemption, every future engine will request their own. Within 5
> engines, brand cohesion dissolves. The warm shell stays.

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
- **First-party Easter eggs are always secret.** Community-winning combos
  become official via Brix Packs (see Part 8) but ship as curated content,
  not user-injected code.

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

## Part 8: Brix Packs, Community, and the BrixBox

### Brix Packs — Curated Combo Releases

Brix Packs are themed collections of OBRIX brick combos, released periodically
as free content drops. Each pack is a set of `.xometa` presets with specific
brick configurations, parameter tunings, Easter eggs, and Professor Oscar
commentary.

**What's in a Brix Pack:**

```
Brix Pack: "Tide Pool Machines" (Q2 2027)
├── 8-12 curated OBRIX presets
│   ├── Specific brick configurations (which types, which routing)
│   ├── Tuned parameter values (not just "here's a saw + filter")
│   └── Multi-slot presets where applicable
├── 2-3 new Easter egg combos (pack-exclusive)
├── Professor Oscar commentary for each preset
│   └── "Here's what makes this one special..."
└── Optional: 1 new brick variant (e.g., a new wavetable set)
```

**Example Packs:**

| Pack Name | Theme | Key Combos |
|-----------|-------|-----------|
| **Tide Pool Machines** | Percussive, rhythmic, short envelopes | Noise sculpting, comb filter tricks, layered hits |
| **Abyssal Drones** | Deep, slow, evolving textures | Detuned saws, slow LFOs, long delays, coupled OBRIX pads |
| **Coral Keyboards** | Playable melodic instruments | Classic subtractive, FM keys, velocity-responsive |
| **Bioluminescence** | Weird, experimental, glitchy | Ring mod chaos, all-modulator patches, wavefolder extremes |
| **Shallow Waters** | Beginner-friendly, simple, approachable | 3-brick combos max, clear cause-and-effect |
| **Oscar's Favorites** | Professor Oscar's personal picks | Cross-section of techniques, heavy on training commentary |

**Release cadence:** One Brix Pack per quarter. Free. Delivered as a preset
folder update — no app update required if we support downloadable preset packs.

### Community Submissions

Users can submit their OBRIX combos for consideration. The submission is just
a preset file — the `.xometa` already contains the full brick configuration,
parameter values, and routing. No special format needed.

**Submission flow:**

1. **Build something in OBRIX.** Play with it. Love it.
2. **Save the preset.** Standard `.xometa` save.
3. **Submit via xoox.design/brix** — upload the `.xometa` + a short description
   of what makes it interesting. Optional: 15-second audio clip.
4. **Community votes.** Other users can load submitted presets and upvote.
5. **Quarterly curation.** XO_OX reviews top-voted submissions each quarter.

**What we're looking for:**
- Surprising combinations (not just "saw + filter" — everyone does that)
- Good parameter tuning (the knob positions matter as much as the brick choices)
- Interesting multi-slot configurations (coupling between OBRIX instances)
- Teaching value (does this combo reveal something about synthesis?)
- Fun Easter egg potential (could this become a named combo?)

### Quarterly Brix Champion

Each quarter, the best community submission becomes an **official Brix Pack
entry**. The winner gets:

- **Their combo included in the next Brix Pack** — with their name credited
- **A custom Easter egg** built around their combo — XO_OX designs the name,
  reskin, and Oscar reaction based on the community winner's creation
- **"Brix Champion Q[X] [Year]" badge** in the community gallery
- **Their preset becomes permanent** — never removed from future versions

The quarterly cycle:

```
Month 1: Submissions open (community builds + uploads)
Month 2: Voting period (community plays + votes)
Month 3: Winner announced → XO_OX builds Easter egg + pack integration
         → Next Brix Pack ships with winner included
```

### The BrixBox — Your Personal Collection

The BrixBox is a persistent personal inventory where discovered Easter eggs
and earned Brix Pack items live. It's not a preset browser — it's a trophy case.

**How it works:**

```
┌─────────────────────────────────────────────┐
│  🧱 YOUR BRIXBOX                    42/87   │
│                                             │
│  ── DISCOVERED ──────────────────────────── │
│  ✦ CHAIN SAW MAN         found 2027-01-15  │
│  ✦ DEEP SEA DIVER        found 2027-01-18  │
│  ✦ PURE THOUGHT          found 2027-02-03  │
│  ○ ????????               (undiscovered)    │
│  ○ ????????               (undiscovered)    │
│  ○ ????????               (undiscovered)    │
│                                             │
│  ── BRIX PACKS ──────────────────────────── │
│  ◆ Tide Pool Machines      12/12 collected  │
│  ◆ Abyssal Drones           8/10 collected  │
│  ◇ Coral Keyboards          0/11 (locked)   │
│                                             │
│  ── CHAMPION COMBOS ─────────────────────── │
│  ★ "Thermal Vent" by @synthwitch  Q1 2027  │
│  ★ "Glass Jellyfish" by @deepcurrent Q2    │
│                                             │
│  ── STATS ───────────────────────────────── │
│  Bricks placed: 1,247                       │
│  Combos tried: 318                          │
│  Easter eggs found: 3/10 (+pack eggs)       │
│  Packs completed: 1                         │
└─────────────────────────────────────────────┘
```

**BrixBox rules:**

- **Easter eggs appear when discovered.** The undiscovered slots show `????????`
  — you know how many exist but not what they are. This creates the hunt.
- **Brix Pack items "collect" when you load and play them.** Download a pack,
  load a preset, play at least one note — it's in your BrixBox. This rewards
  engagement, not just downloading.
- **Pack Easter eggs stay in your BrixBox even if you uninstall the pack.**
  Once found, always yours.
- **Champion combos are highlighted** with the creator's name and quarter.
  Loading a champion combo shows a brief credit overlay.
- **Stats are for fun, not competition.** No leaderboards. No sharing pressure.
  Your BrixBox is yours. The count at the top (42/87) is the only hint at
  how much is left to find.
- **BrixBox sync is opt-in (Board D7 mandate).** On first BrixBox access:
  "Sync your discoveries to iCloud?" with explicit opt-in. Show what data
  syncs. Provide Settings → Privacy → "Clear iCloud Data" with confirmation.
  "BrixBox data is personal. Never shared, never sold, never used for analytics."
  Merge strategy: last-write-wins (not union — union amplifies compromise, per D2).
- **Collection stats are opt-in (Board D7/P1).** The "42/87" counter and
  `????????` slots are hidden by default. Users opt into "Show Collection Stats"
  in BrixBox settings. BrixBox without stats is just a list of discoveries.

### Progressive Unlocks — The Roguelite Layer

OBRIX starts simple. Not everything is available on day one. As you build,
discover, and collect, new capabilities permanently unlock. Every session
expands what's possible in the next one.

**The principle:** Roguelites gate capability, not content. You never lose
access to sounds you've made. But the *palette* of what you can build grows
as you play. This keeps early sessions focused (not overwhelmed by 40 brick
types) and gives experienced users a reason to keep exploring.

**Unlock tiers:**

```
TIER 0 — WASHED ASHORE (first launch)
├── Source bricks: Sine, Saw, Square, Noise
├── Processor bricks: LP Filter, HP Filter
├── Modulator bricks: Envelope, LFO
├── Effects: none yet
├── Max bricks per instance: 6
├── Slots: 1 OBRIX instance
└── Oscar says: "Welcome to the reef. Let's start with the basics."

TIER 1 — SHALLOW REEF (10 combos tried)
├── + Source: Triangle, Wavetable, Pulse
├── + Processor: BP Filter, Wavefolder
├── + Effects: Delay, Chorus
├── Max bricks per instance: 8
└── Oscar says: "You're getting comfortable. Here — more building material."

TIER 2 — OPEN WATER (3 Easter eggs discovered)
├── + Source: FM Pair, Sub Oscillator, Noise (colored)
├── + Processor: Comb Filter, Ring Mod, Distortion
├── + Modulator: Velocity, Aftertouch, Random S&H
├── + Effects: Reverb, Phaser
├── Max bricks per instance: 10
├── Slots: 2 OBRIX instances (coupling unlocked)
└── Oscar says: "Three discoveries. You've earned deeper water."

TIER 3 — THE DEEP (1 Brix Pack completed)
├── + All remaining brick types
├── + Effects: Granular Delay, Spectral Freeze
├── + Modulator: Macro mapping, Cross-instance mod
├── Max bricks per instance: 12
├── Slots: 4 OBRIX instances
└── Oscar says: "Full toolkit. Every brick. Every connection. Build anything."

TIER 4 — ABYSSAL (all base Easter eggs discovered + 2 packs completed)
├── + Secret brick variants (visual-only — alternate skins for each type)
├── + Oscar's "Deep Cuts" — hidden science cards with advanced synthesis theory
├── + BrixBox gets an alternate deep-ocean theme
├── + Unlocks "Architect Mode" — save custom brick kits as starter templates
└── Oscar says: "You've seen everything. Now you can reshape it."
```

**Unlock philosophy — what we DON'T do:**

- **No timer gates.** Never "wait 24 hours to unlock." Progress is always
  earned by doing, never by waiting.
- **No paywalls.** Every tier is free. Brix Packs accelerate progression
  (completing a pack jumps you to Tier 3) but aren't required.
- **No loss.** This is roguelite, not roguelike. You never lose progress.
  No "prestige reset." No "start over for a bonus." What you earn, you keep.
- **No artificial scarcity.** We don't hide brick types behind random drops
  or loot boxes. The unlock path is deterministic — play more, get more.
- **No grind.** Tier thresholds are tuned so natural exploration hits them.
  10 combos is maybe 30 minutes of curious building. 3 Easter eggs is a few
  sessions of "what happens if I try this?" No one should feel stuck.

**Why this works for OBRIX specifically:**

The real problem with modular synthesis for beginners isn't that it's hard —
it's that it's *overwhelming*. 40 brick types on a blank canvas is paralyzing.
The roguelite unlock system solves this by design:

- **Session 1:** Four oscillators, two filters, an envelope, an LFO. That's
  already enough to build a synth. The constraint is the teacher.
- **Session 3:** You've tried 10 things, you're comfortable. Now here's
  wavetables and effects. The expansion feels earned and exciting.
- **Session 8:** You found CHAIN SAW MAN and DEEP SEA DIVER. You're thinking
  in combinations now. Here's ring mod and comb filters — you're ready.
- **Session 15:** Full toolkit. You don't need guardrails anymore.

The progression *is* the tutorial. No separate "learn mode" needed — the
unlock gates naturally teach you synthesis in the right order.

**Oscar adapts to your tier:**

At each unlock, Oscar doesn't just announce new bricks — he contextualizes them
based on what you've already built:

> *"You just unlocked the Wavefolder. Remember that buzzy saw patch you made?*
> *Try running it through this. It'll fold the waveform back on itself —*
> *more harmonics, more edge. Like crumpling paper."*
> — Professor Oscar (Tier 1 unlock, personalized)

Oscar tracks your most-used brick types and tailors unlock messages to reference
your actual patches. Generic fallbacks exist for edge cases, but the personal
touch makes each unlock feel like a gift, not a gate.

**Persistent unlock state:**

```json
// brixbox.json additions
{
  "tier": 2,
  "combosTried": 47,
  "discoveredEggs": ["CHAIN_SAW_MAN", "DEEP_SEA_DIVER", "BELL_BOY"],
  "packsCompleted": 0,
  "unlockedBricks": ["sine", "saw", "square", "noise", "triangle",
                      "wavetable", "pulse", "lp_filter", "hp_filter",
                      "bp_filter", "wavefolder", "envelope", "lfo",
                      "delay", "chorus", "fm_pair", "sub_osc", ...],
  "tierUnlockDates": {
    "1": "2027-01-20T14:32:00Z",
    "2": "2027-02-08T19:15:00Z"
  }
}
```

**Bypass for power users:**

An "Unlock All" toggle exists in OBRIX settings. Flipping it immediately
grants Tier 3 access (all bricks, all slots). Tier 4 still requires
discovery — you can't skip the Easter egg hunt. No judgment, no penalty.
Some people want the full canvas from minute one, and that's fine.

The toggle is worded carefully:

> **Unlock all bricks immediately**
> *Skip progressive discovery. All brick types and 4 slots available now.*
> *Easter egg discoveries and BrixBox collection still work normally.*

Not "cheat mode." Not "skip tutorial." Just a preference.

### Boss Mode — Optional Synthesis Challenges

> **Board D7 resolution (2026-03-18):** Boss Mode is an **optional challenge**,
> not a progression gate. Users can advance to the next level without completing
> a Boss. Bosses reward trophies + Oscar reactions but do not block access to
> content. "Respect the user — no dark patterns" (XO_OX value #3).

Each level offers a **Boss** — an optional synthesis challenge that celebrates
what you've learned. Not a quiz. Not a gate. A creative challenge with a target
*quality* and Oscar as the judge. Complete it for a trophy; skip it if you want.

**How Boss Mode works:**

1. **Oscar offers the Boss.** When you reach the Boss tier, Oscar presents a challenge
   (not a gate — the next level unlocks regardless via tier progression):

   > *"You've explored every corner of the shallows. Before we go deeper,*
   > *I want to hear something from you. Build me a sound that breathes —*
   > *something that changes on its own, without you touching anything.*
   > *Use what you've learned. I'll be listening."*
   > — Professor Oscar (Level 1 Boss)

2. **The constraint.** Each Boss has a creative constraint — not "use exactly
   these bricks" but a *sonic quality* to achieve:

   | Level | Boss Name | Challenge |
   |-------|-----------|-----------|
   | 1 | **THE TIDE** | *"Build a sound that breathes."* — Must use at least one LFO modulating a timbral parameter. The sound must evolve audibly without user input. Tests: modulation fundamentals. |
   | 2 | **THE STORM** | *"Build a sound that responds to how hard you play."* — Must use velocity and/or aftertouch meaningfully. Gentle playing and aggressive playing must sound distinctly different. Tests: expression and dynamics. |
   | 3 | **THE ABYSS** | *"Build a sound using two OBRIX instances that couldn't exist with one."* — Must use cross-instance coupling. The two instances must depend on each other. Tests: coupling and interaction. |
   | 4 | **THE KRAKEN** | *"Build something I've never heard before."* — Open-ended. Oscar evaluates originality based on how different your configuration is from all known Easter eggs and Brix Pack presets. Tests: creative synthesis mastery. |

3. **Oscar evaluates.** This is parameter-based analysis, not audio AI.
   Oscar checks whether the sonic quality is present:
   - THE TIDE: Is an LFO routed to filter/pitch/wavefold? Is rate > 0? ✓
   - THE STORM: Is velocity mapped to a timbral param? Does the mapping
     range exceed a threshold? ✓
   - THE ABYSS: Are two OBRIX slots active? Is cross-instance modulation
     non-zero? ✓
   - THE KRAKEN: Is the brick configuration hash dissimilar (>70% different)
     from all known combos in the Easter egg + pack database? ✓

4. **Victory.** When you pass, Oscar reacts big:
   - Gills flash rapidly (celebration animation)
   - The entire OBRIX canvas does a ripple wave effect
   - A unique **Boss Trophy** appears in your BrixBox (distinct from Easter eggs)
   - Oscar delivers a personalized victory line referencing your specific patch
   - **The next level unlocks.**

5. **No failure state.** You can't "lose" a Boss fight. Oscar gives hints if
   you're stuck. You can leave and come back. The Boss challenge stays active
   until you beat it — no timer, no penalty. But you can't advance to the next
   level without completing it.

   > *"Not quite breathing yet — your LFO is connected, but it's modulating*
   > *volume, not timbre. Try routing it to the filter cutoff instead.*
   > *That's where the life is."*
   > — Professor Oscar (Boss hint)

**Boss Mode visual treatment:**

When a Boss is active, the OBRIX canvas gets a subtle environmental shift:
- THE TIDE: Gentle wave animation at canvas edges
- THE STORM: Distant lightning flickers in the background
- THE ABYSS: Canvas darkens, bioluminescent particles drift upward
- THE KRAKEN: Tentacle silhouettes at canvas edges, ink wisps in the scope

These are ambient — atmospheric, not distracting. They set the mood without
cluttering the workspace.

### Levels — The Long Game

Levels are the macro structure. Each level is a complete progression arc
(tiers + boss), and new levels ship over the lifetime of OBRIX. This is the
long-tail engagement model.

**Level structure:**

```
LEVEL 1: "THE REEF" (ships with OBRIX v1.3)
├── Tier 0: Washed Ashore — basic bricks
├── Tier 1: Shallow Reef — +waveforms, +effects
├── Tier 2: Open Water — +FM, +ring mod, +expression, 2 slots
├── Tier 3: The Deep — full toolkit, 4 slots
├── Tier 4: Abyssal — secret skins, deep science cards
├── BOSS: THE TIDE — "build a sound that breathes"
└── Reward: Level 2 unlocks + "Reef Master" BrixBox badge

LEVEL 2: "THE TRENCH" (ships ~6 months post-launch)
├── New brick types: Granular source, Spectral filter, Probability gate
├── New modulator: Euclidean rhythm generator
├── New effect: Convolution (impulse responses from real ocean spaces)
├── New Easter eggs: 5 Trench-exclusive combos
├── Tiers 0–4 with Trench-specific unlock thresholds
├── BOSS: THE STORM — "build a sound that responds to how hard you play"
└── Reward: Level 3 unlocks + "Trench Diver" badge + Oscar outfit change

LEVEL 3: "THE RIFT" (ships ~12 months post-launch)
├── New brick types: Physical modeling string/membrane, Formant source
├── New modulator: Chaos/Lorenz attractor, Markov chain sequencer
├── New effect: Shimmer reverb, Spectral morph
├── Cross-engine bricks: bricks that reference other XOmnibus engines
│   (e.g., an "Orbital Resonator" brick that uses Orbital's group envelope)
├── New Easter eggs: 5 Rift-exclusive combos
├── BOSS: THE ABYSS — "build a sound using two instances that couldn't exist with one"
└── Reward: Level 4 unlocks + "Rift Walker" badge + Oscar deep-sea form

LEVEL 4: "THE LEVIATHAN" (ships ~18 months post-launch)
├── New brick types: Machine learning source (neural audio synthesis),
│   Feedback network (multiple bricks in a loop), Meta-brick (brick that
│   controls other bricks)
├── New modulator: Audio-rate modulation (FM/AM between any bricks)
├── Cross-engine bricks: expanded — any engine's signature DSP as a brick
├── New Easter eggs: 5 Leviathan-exclusive combos
├── BOSS: THE KRAKEN — "build something I've never heard before"
└── Reward: "Leviathan" title + Oscar final form + secret ending sequence
```

**Level release cadence:**

| Level | Ships | Theme | New Content |
|-------|-------|-------|------------|
| 1: The Reef | V1.3 (OBRIX launch) | Fundamentals | ~25 brick types, 10 Easter eggs |
| 2: The Trench | V1.3 + ~6 months | Texture & rhythm | +5 brick types, +5 Easter eggs, granular/spectral |
| 3: The Rift | V1.3 + ~12 months | Interaction & cross-engine | +6 brick types, +5 Easter eggs, cross-engine bricks |
| 4: The Leviathan | V1.3 + ~18 months | Mastery & the unknown | +4 brick types, +5 Easter eggs, meta-bricks, neural audio |

**Each level is free.** Levels are app updates, not DLC. The progressive
release model keeps OBRIX fresh without charging for content.

**Cross-level BrixBox:**

```
┌─────────────────────────────────────────────┐
│  🧱 YOUR BRIXBOX                    67/142  │
│                                             │
│  ── LEVEL 1: THE REEF ──────────── ★ CLEAR  │
│  ✦ Easter eggs: 8/10                        │
│  ◆ Boss trophy: THE TIDE                    │
│  ◆ Badge: Reef Master                       │
│                                             │
│  ── LEVEL 2: THE TRENCH ────────── ◉ ACTIVE │
│  ✦ Easter eggs: 3/5                         │
│  ○ Boss: THE STORM (not yet attempted)      │
│  Tier: 2/4 — Open Water                     │
│                                             │
│  ── LEVEL 3: THE RIFT ──────────── 🔒 LOCKED│
│  (Complete Level 2 Boss to unlock)          │
│                                             │
│  ── LEVEL 4: THE LEVIATHAN ─────── 🔒 LOCKED│
│  (Complete Level 3 Boss to unlock)          │
│                                             │
│  ── BRIX PACKS ──────────────────────────── │
│  ◆ Tide Pool Machines      12/12            │
│  ◆ Abyssal Drones           8/10            │
│  ◆ Trench Echoes (L2)       4/10            │
│                                             │
│  ── CHAMPION COMBOS ─────────────────────── │
│  ★ "Thermal Vent" by @synthwitch  Q1 2027  │
│  ★ "Glass Jellyfish" by @deepcurrent Q2    │
│                                             │
│  ── LIFETIME STATS ──────────────────────── │
│  Bricks placed: 3,891                       │
│  Combos tried: 847                          │
│  Easter eggs found: 11/15 (L1+L2)          │
│  Bosses defeated: 1/4                       │
│  Levels cleared: 1                          │
│  Time in OBRIX: 48 hours                    │
└─────────────────────────────────────────────┘
```

**Oscar evolves across levels:**

> **Board D8 resolution (2026-03-18):** V1 ships with **2 Oscar forms only**
> (Baby + Adult). Baby = first-launch celebrations + milestone rewards.
> Adult = default teaching state. This halves Rive rig maintenance and keeps
> pedagogy clear. Adolescent + Elder forms deferred to V2 with data-informed
> design (validate which moments resonate before expanding the arc).

Oscar isn't just a static guide — he grows with you.

| Level | Oscar's Form | Personality Shift | V1 Status |
|-------|-------------|-------------------|-----------|
| 1: The Reef | Baby axolotl | Warm, encouraging, analogies-first | **V1 — ships** |
| 2: The Trench | Adult — bioluminescent markings appear | Speaks as a peer, references your history | **V1 — ships** |
| 3: The Rift | Adolescent — more elaborate gills | More conversational, philosophical | V2 — deferred |
| 4: The Leviathan | Elder — translucent, glowing, mythical | Sparse, poetic. Only speaks when it matters. | V2 — deferred |

The Adult form is the default teaching state. Baby appears for first-launch
and milestone moments. The full 4-stage arc remains the long-term vision,
validated by data from V1 before committing to additional Rive rigs.

**The secret ending:**

After defeating THE KRAKEN, a final sequence plays:

> *"When you arrived, you had a sine wave and a dream.*
> *Now you have... this. Something no one has ever built.*
> *I have nothing left to teach you.*
> *But I think you have something to teach me."*
>
> *[Oscar swims off screen. The canvas goes quiet.]*
> *[After 3 seconds, a new brick type appears: "OSCAR"]*
> *[It's a brick that generates Oscar's voice as a wavetable source.]*
> *[Professor Oscar IS the instrument now.]*

The OSCAR brick is the ultimate collectible. A wavetable set derived from
axolotl vocalizations (yes, axolotls make sounds — soft clicks and squeaks).
It's a real, usable, weird, wonderful brick. The final gift.

**Why levels work as a release strategy:**

- **Solves the "shipped and forgotten" problem.** Most synth plugins ship,
  get reviewed, and fade. Levels give OBRIX a reason to be in the news every
  6 months — "OBRIX Level 3 just dropped."
- **Community stays active.** Each level brings new Easter eggs, new Brix Pack
  potential, new Boss challenges for the community to discuss.
- **Development is sustainable.** Each level is 4-6 new brick types + 5 Easter
  eggs + 1 Boss. Scoped, predictable, shippable.
- **Cross-engine bricks (Level 3+) deepen the XOmnibus ecosystem.** Once OBRIX
  can reference other engines' DSP, it becomes the gateway drug for the entire
  platform. "I love this Orbital Resonator brick — what's the full Orbital
  engine like?"

### Levels, Packs, and Champions — How They Interleave

```
Timeline:
─────────────────────────────────────────────────────────────────
V1.3    Q1        Q2        Q3        V1.3+6mo  Q5        Q6
OBRIX   Pack 1    Pack 2    Pack 3    LEVEL 2   Pack 4    Pack 5
launch  Shallow   Oscar's   Brix      The       Trench    ...
L1      Waters    Faves     Champ     Trench    Echoes
        +Champ Q1           Q1 winner +new eggs  +Champ Q4
─────────────────────────────────────────────────────────────────

Brix Packs ship quarterly regardless of level releases.
Levels ship ~every 6 months.
Packs can be level-specific (Trench Echoes uses Level 2 bricks)
or cross-level (Oscar's Favorites spans all available bricks).
Champions are always from the current highest available level.
```

### Technical: Level Data Structure

```json
// brixbox.json — expanded for levels
{
  "currentLevel": 2,
  "levels": {
    "1": {
      "status": "cleared",
      "tier": 4,
      "bossDefeated": true,
      "bossDate": "2027-04-15T20:00:00Z",
      "discoveredEggs": ["CHAIN_SAW_MAN", "DEEP_SEA_DIVER", ...],
      "badge": "reef_master"
    },
    "2": {
      "status": "active",
      "tier": 2,
      "bossDefeated": false,
      "discoveredEggs": ["ANGLERFISH", "PRESSURE_DROP", "VENT_WHISTLE"],
      "badge": null
    },
    "3": { "status": "locked" },
    "4": { "status": "locked" }
  },
  "bossTrophies": ["THE_TIDE"],
  "oscarForm": "adolescent",
  "oscarBrick": false,
  "lifetimeStats": {
    "bricksPlaced": 3891,
    "combosTried": 847,
    "totalEggs": 11,
    "bossesDefeated": 1,
    "timeInObrix": 172800
  }
}
```

Level definitions ship as JSON manifests alongside the brick type definitions.
When a new level is available (via app update), OBRIX checks the player's
Boss state to determine if it should be unlocked or shown as locked.

### BrixBox on iPhone Pocket

On Pocket, the BrixBox is accessed via a dedicated tab (swipe left from the
main OBRIX Pocket view). It's a compact scrollable list with the same
sections. Tapping any collected item loads it immediately — no navigation
to a separate preset browser.

When you discover an Easter egg on Pocket:
1. The discovery animation plays (Oscar's gills flash, bricks reskin)
2. A subtle **"Added to BrixBox"** toast appears at the top
3. A badge dot appears on the BrixBox tab

### Technical Implementation

```
BrixBox storage:
├── Local: ~/Library/XOmnibus/BrixBox/brixbox.json
│   ├── discovered: ["CHAIN_SAW_MAN", "DEEP_SEA_DIVER", ...]
│   ├── collected: { "tide_pool_machines": ["preset1", "preset2", ...] }
│   ├── stats: { bricksPlaced: 1247, combosTried: 318 }
│   └── syncToken: "..." (for cloud sync)
├── iCloud: com.xoox.xomnibus/BrixBox/
└── Merge strategy: union (never lose discoveries)
```

- **Discovery detection** reuses the Easter egg parameter check —
  same `obrix_source1_type` / `obrix_proc1_type` pattern matching.
- **Pack collection** triggers on `noteOn` event while a pack preset is active.
- **Stats increment** on brick type changes (bricksPlaced) and unique
  configuration hashes (combosTried).
- **No server dependency for core BrixBox.** Works fully offline.
  Community voting and pack downloads need network, but your personal
  collection is local-first.

### Brix Pack Distribution

Two models, depending on platform constraints:

**Model A: In-app download (preferred)**
- Pack metadata lives on xoox.design/api/brixpacks
- User taps "Get Pack" in OBRIX → downloads preset folder + Easter egg
  definitions → unpacks to `Presets/XOmnibus/BrixPacks/{packName}/`
- No app update required. Works on macOS, iOS.

**Model B: App update bundle (fallback for App Store restrictions)**
- Packs ship in point releases (v1.3.1, v1.3.2...)
- All packs are included in the app bundle
- Locked until "activated" by the user in the BrixBox (so you still get
  the collection/discovery experience)

### How Brix Packs, Easter Eggs, and BrixBox Connect

```
User builds combo in OBRIX
    │
    ├── Matches a built-in Easter egg? → Discovery animation → BrixBox
    │
    ├── Matches a Brix Pack Easter egg? → Discovery animation → BrixBox
    │   (only if pack is installed)
    │
    ├── Something interesting? → Submit to community
    │   │
    │   └── Wins quarterly? → Becomes official Easter egg
    │       → Ships in next Brix Pack
    │       → All users can discover it
    │       → Winner's name in Champion section
    │
    └── Just playing? → Stats increment → Combos tried goes up
        → Eventually stumble into an egg you didn't know existed
```

The whole system is a flywheel: OBRIX is fun → users build combos → best
combos become packs → packs bring users back → more combos → repeat.

---

## Part 9: Sequencing & Priority

> **Board Resolution (2026-03-18):** Version numbering below is PROVISIONAL.
> The ratified V1 tiered release plan (2026-03-17) specifies V1.1 = Aquatic FX
> suite, V1.2 = preset expansion + XPN #2. **Owner decision required:** do these
> strategy phases supersede the ratified plan, or do they begin at V1.3+?
> Do NOT make public commitments on version numbers until this is resolved.

> **Board D9 resolution:** SDK and OBRIX develop on **parallel tracks**, not
> sequentially. OBRIX ships when consumer value is ready. SDK ships when
> developer tooling is ready. Neither blocks the other.

### V1 (Ship)
- 34 engines, current architecture, no expansion API yet
- OBRIX is NOT in V1 — it's a significant UI + DSP effort

### Post-V1 Track A: Infrastructure (SDK)
- **Engine version metadata** — `sinceVersion` in registry, graceful missing-engine handling
- **SDK extraction** — pull `SynthEngine.h` + `CouplingTypes.h` into standalone headers with no JUCE types in the interface boundary
- **Ship `xomnibus-engine-sdk` repo** — templates, validate-engine CLI, test harness
- **Parameter namespace registry** — prevent collisions
- **Dynamic engine loading deferred to V2** (per Board D2 security resolution)

### Post-V1 Track B: OBRIX (Level 1: The Reef)

> **Board D1 scope warning:** OBRIX is estimated at 2,500-3,500 engineering hours.
> Phase ruthlessly: V1.3a (core DSP + basic UI) → V1.3b (Oscar training mode) →
> V1.3c (BrixBox + progressive unlock). Pocket deferred to V1.4 minimum.

- Ships as engine "Obrix" with param prefix `obrix_`, accent color Reef Jade `#1E8B7E`
- Starter kit presets + snap challenges
- Multi-slot support (up to 4 OBRIX instances, each independently configured)
- Cross-instance coupling via MegaCouplingMatrix
- Full coupling compatibility with all existing engines
- Professor Oscar training mode (tooltips, connection insights, science cards, waveform visualizer)
- Easter egg brick combos (CHAIN SAW MAN, DEEP SEA DIVER, PURE THOUGHT, etc.)
- **Progressive unlock system** — roguelite Tiers 0–4 + Boss Mode (THE TIDE)
- **BrixBox** — personal trophy case with level progress, eggs, boss trophies
- **OBRIX Pocket** — iPhone-optimized Pocket Operator-inspired single-screen interface
- Aquatic visual identity (ocean gradient, shell/coral/current/tide pool brick types)

### V1.4 (Packs + Community)
- **Brix Pack infrastructure** — downloadable preset packs (or bundled in updates)
- **First Brix Pack ships** — "Shallow Waters" (beginner-friendly) + "Oscar's Favorites"
- **Community submission portal** on xoox.design/brix
- **First Quarterly Brix Champion** selected

### V1.5 (Level 2: The Trench — ~6 months post-OBRIX)
- **New brick types:** Granular source, Spectral filter, Probability gate,
  Euclidean rhythm generator, Convolution effect (ocean impulse responses)
- **5 new Easter eggs** (Trench-exclusive)
- **Boss: THE STORM** — "build a sound that responds to how hard you play"
- **Oscar evolves** to adolescent form (more elaborate gills, conversational)
- Ongoing quarterly Brix Packs + Champions (level-specific packs begin)

### V1.6 (Level 3: The Rift — ~12 months post-OBRIX)
- **New brick types:** Physical modeling string/membrane, Formant source,
  Chaos/Lorenz attractor, Markov chain sequencer, Shimmer reverb, Spectral morph
- **Cross-engine bricks** — bricks that use other XOmnibus engines' DSP
  (Orbital Resonator, Ouroboros Feedback, etc.)
- **5 new Easter eggs** (Rift-exclusive)
- **Boss: THE ABYSS** — "build a sound using two instances that couldn't exist with one"
- **Oscar evolves** to adult form (bioluminescent markings, speaks as peer)

### V1.7 (Level 4: The Leviathan — ~18 months post-OBRIX)
- **New brick types:** Neural audio synthesis, Feedback network (brick loops),
  Meta-brick (brick that controls other bricks), Audio-rate modulation
- **Expanded cross-engine bricks** — any engine's signature DSP as a brick
- **5 new Easter eggs** (Leviathan-exclusive)
- **Boss: THE KRAKEN** — "build something I've never heard before"
- **Oscar final form** — elder, translucent, glowing
- **Secret ending sequence** — OSCAR brick unlocks (axolotl vocalization wavetable)

### V1.8+ (Community Engines + OBRIX-to-SDK)
- **Community engine gallery** on xoox.design
- **In-app engine browser** — discover and install community engines
- **OBRIX-to-SDK bridge** — export an OBRIX patch as an SDK project scaffold
  (the brick graph becomes starter code in a real engine template)
- Ongoing quarterly cadence: submissions → voting → winner → Brix Pack
- Potential Level 5+ based on community demand and new DSP research

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

7. **Brix Pack pricing.** Strategy says free. But if packs get expensive to
   curate (custom Easter eggs, Oscar commentary, new brick variants), is a
   mix of free + premium packs sustainable? Or does free drive enough
   engagement that it pays for itself in platform growth?

8. **Community moderation.** Submission portal needs content moderation —
   preset names, descriptions, audio clips. Automated + human review?
   Community flagging? How much overhead is this?

9. **BrixBox gamification ceiling.** Stats and counts are "for fun" but
   there's a fine line before it feels like a mobile game. No XP, no
   levels, no daily streaks. But the ???????? slots are inherently
   completionist bait — is that okay? Probably yes, but worth monitoring.

10. **Pocket landscape mode.** iPhone landscape could show a wider brick canvas
   (closer to iPad). Or it could show a split: keyboard left, bricks right.
   Needs prototyping.

11. **Oscar localization.** Training mode text needs translation. Oscar's
   personality needs to survive localization — casual analogies don't always
   translate well. Consider hiring writers per language, not just translators.

12. **Sustainability model (Board P0-5).** Everything is currently promised free.
    Finalize funding model before V1 ship: Patreon tiers? B2B SDK licensing?
    Premium boutique engines? "Core content free; curated premium packs optional"?
    The promise of "free forever" creates obligations that must be backed by revenue.

13. **Accessibility (Board P0-3).** Screen reader support for brick canvas,
    colorblind mode (icon + pattern + color), motor accessibility (button
    alternatives for all gestures), WCAG 2.1 Level AA minimum. Non-negotiable
    for OBRIX launch per XO_OX value: "if someone can't use it, it's broken."

---

## Appendix: Board Resolution Summary (2026-03-18)

Full Board (9 directors) + Khan Sultan reviewed this strategy. 12 P0 breaches
identified and resolved inline above. Key resolutions:

| P0 | Resolution | Director |
|----|-----------|----------|
| Version numbering collision | PROVISIONAL — owner decision required | D1 |
| Channel B same-process loading | Deferred to V2; preset-only community model for V1.x | D2 |
| Accessibility triad | Screen reader + colorblind + motor — WCAG 2.1 AA required | D5, D7 |
| OBRIX DSP safety | ParamSnapshot for routing + denormal protection mandatory | D6 |
| "Free forever" sustainability | Finalize funding model before V1 ship | D7 |
| Boss Mode forced gate | Reframed as optional challenge, not progression blocker | D7 |
| iCloud sync consent | Explicit opt-in, data dictionary, delete path | D7 |
| Gallery Model Pocket exception | Removed — Pocket stays in warm white shell | D8 |
| 4-color brick palette | Collapsed to accent + 2 neutrals | D8 |
| Oscar 4-stage arc | Scoped to 2 stages (Baby + Adult) for V1 | D8 |
| SDK→OBRIX sequential | Parallel tracks; neither blocks the other | D9 |
| Reef Teal proximity | Shifted to Reef Jade `#1E8B7E` | D3 |

**Owner decisions still pending:**
1. Version numbering: V1.1 = Aquatic FX (ratified) or SDK extraction (strategy)?
2. Sustainability model: Patreon / B2B licensing / hybrid?
3. XObrix naming: rename to XO+O-word or formally ratify neologism exception?
