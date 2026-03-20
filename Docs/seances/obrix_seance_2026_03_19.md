# Synth Seance — OBRIX (Wave 1)

**Engine:** ObrixEngine.h (1136 lines) | **Gallery Code:** OBRIX
**Accent:** Reef Jade `#1E8B7E` | **Prefix:** `obrix_` | **Params:** 55
**Date:** 2026-03-19 | **Mode:** Review (Wave 1 — post-Constructive Collision)

---

## The Eight Voices

### G1 — Bob Moog: The Filter Philosopher

"I see CytomicSVF — Andrew Simper's topology. A good choice: it's stable across the full frequency range, it doesn't blow up at high resonance, and it maintains its character from 20Hz to 20kHz. Three independent filter instances — one per processor slot — and they're properly coefficiented per-sample. The filter *works*.

But I have two concerns. First: velocity shapes the cutoff of Proc1 and Proc2 — good — but it *misses* Proc3 entirely. The post-mix filter is deaf to the player's touch. That's a broken circuit in the velocity-to-timbre path. A finger hits a key, and the first two stages respond, but the final stage ignores the gesture. Fix this — add `velTimbre` to the Proc3 cutoff calculation on line 553.

Second: the source mix is inverted. `srcMix = 1.0` gives you 100% Source 1. Convention says mix=1.0 should give you 100% of the *second* source — the thing you're mixing *in*. This will confuse every producer who's ever used a crossfader. Swap it.

The CytomicSVF earns my respect. The velocity gap and the inverted mix do not."

**Score: 7/10**

---

### G2 — Don Buchla: The Complexity Poet

"The concept is bold: a modular synthesizer inside a synthesizer. Bricks you can snap together — sources, processors, modulators, effects — each configurable at runtime. This is semi-modular thinking with the freedom of a fully modular system. I'm intrigued.

But the brick independence has a critical flaw. When the user selects Wavefolder on Proc1 — meaning 'fold Source 1' — the fold actually applies to the *post-mix signal*. All three processor slots converge on a single wavefolder path at line 560. The architecture diagram says `Src1 → Proc1` and `Src2 → Proc2` independently, but the code routes wavefolder globally. The Constructive Collision is lying about its topology. If I select wavefolder on Proc1 and LP filter on Proc2, I expect Source 1 to be folded and Source 2 to be filtered. Instead, *everything* gets folded. This is architectural dishonesty.

Ring mod has the same problem — worse, actually. Line 570: `signal = src1 * src2`. This *overwrites* the entire signal chain. Everything before it — both filters, the mix, all modulation — thrown away and replaced with a raw multiplication. The ring mod should *blend*, not *replace*.

The brick *idea* is novel. The brick *routing* is broken. Fix the topology and you have something genuinely original in the fleet."

**Score: 5.5/10**

---

### G3 — Dave Smith: The Protocol Architect

"Let me trace the voice architecture. 8-voice polyphony, oldest-voice stealing, legato mode with portamento, mono mode. Standard Prophet-style allocation. Clean.

But I see a voice leak. When the user switches from Poly8 to Mono mid-performance, `polyLimit_` drops to 1, and the voice search loop only examines `voices[0]`. Voices 1 through 7 continue sounding — orphaned, unreachable, never released. The user hears phantom notes from a mode they've already left. This is the kind of bug that makes a producer distrust the instrument. Clean up voices beyond the new poly limit when the mode changes.

The coupling interface is well-designed. `getSampleForCoupling` outputs real audio (L/R) plus a third channel carrying `brickComplexity` — a 0–1 normalized measure of how many bricks are active. That's architectural metadata as coupling signal. I like that. Channel 2 as complexity — another engine can modulate its behavior based on how 'busy' OBRIX is. Smith approves.

`applyCouplingInput` handles 5 of 12 types. Acceptable for Wave 1. The choices are sensible: AudioToFM, AmpToFilter, LFOToPitch, AmpToPitch, AmpToChoke. No exotic types, but the fundamentals are covered.

MIDI events are processed at the top of the block, ignoring `samplePosition`. Every note triggers at sample 0 regardless of where it falls in the buffer. At 512 samples / 44.1kHz, that's up to 11.6ms of jitter. For pad sounds, irrelevant. For drum triggers and tight sequenced parts, audible and unprofessional. Implement sub-block event processing."

**Score: 6.5/10**

---

### G4 — Ikutaro Kakehashi: The Drum Philosopher

"The init patch. Source 1: Saw. Proc1: LP Filter at 8000Hz. Amp envelope: A=0.01, D=0.3, S=0.7, R=0.5. Mod1: Envelope → Filter Cutoff at depth 0.5. This is a filtered saw pad with an envelope sweep. Load it, press a chord, and... it works. It's warm, it moves, it invites you in. This is a good handshake.

But the defaults reveal something: Mod2 is LFO→Cutoff but depth is *zero*. Mod4 is Aftertouch→Cutoff but depth is *zero*. These are correctly wired but intentionally silent. The producer who loads the init patch will never discover aftertouch response unless they go digging. Kakehashi says: make the defaults *audible*. Set Mod2 depth to 0.15 and Mod4 depth to 0.2. Let the producer *hear* that the LFO is breathing and that aftertouch opens the filter. Discovery through sound, not through parameter hunting.

The brick system is accessible — I count 9 source types, 6 processor types, 5 modulator types, 4 effect types. A beginner can start with one oscillator and one filter and make music. An expert can fill all slots and create a complex patch. This is the semi-modular philosophy: opinionated defaults, open architecture.

One concern: the voice mode parameter is labeled 'Poly8' but the label gives no indication of what Poly4 vs Poly8 sounds like. For a beginner, the difference between voice modes is invisible. Consider adding a visual voice meter to the UI — dots that light up as voices activate."

**Score: 7/10**

---

### G5 — Alan R. Pearlman: The Ergonomist

"I designed the ARP 2600 with normalled connections — a signal flow that worked out of the box, with the *option* to repatch. OBRIX does the same thing. The init patch has: Source 1 (Saw) → Proc1 (LP Filter) → Amp → output. Source 2 is off. Proc2 and Proc3 are off. Effects are off. Mod1 is wired and active. Everything else is ready but silent. This is normalled architecture. Pearlman approves.

The 55 parameters break down cleanly: 7 sources, 9 processors, 4 amp envelope, 16 modulators, 9 effects, 4 macros, 1 level, 1 voice mode, 2 expression, 2 gesture. That's a lot of surface area, but the *active* surface at init is just: oscillator type, filter cutoff, filter resonance, ADSR, level. Five knobs to start. The rest unfolds as the producer explores. This is accessible complexity.

My concern is the FLASH gesture system. It's novel — a parameter called `obrix_flashTrigger` that fires a burst envelope when toggled. Four gesture types: Ripple, Pulse, Flow, Tide. But the gesture has no parameter for *intensity* or *speed*. It fires, it decays, it's done. In the 2600, every module had gain staging. The FLASH needs an intensity knob — how strong is the burst? And a decay rate — how long does it ring?"

**Score: 7/10**

---

### G6 — Isao Tomita: The Timbral Painter

"I look at the source palette: Sine, Saw, Square, Triangle, Noise, Wavetable (sine→saw morph), Pulse, Lo-Fi Saw. That's 8 timbral colors across two independent sources, mixed through independent processors. Two sources with independent tuning — semitone offsets up to ±24. This means I can voice intervals: octave, fifth, minor second. Each source through its own filter. The palette is there.

But the Wavetable mode — case 6 in `renderSourceSample` — is a disappointment. It crossfades between sine and saw using the pulse width parameter as a morph control. This is not wavetable synthesis. This is a two-point crossfade wearing a wavetable costume. A wavetable engine scans through a table of waveforms with smooth interpolation, producing continuously evolving harmonics. This scans between two fixed points. The timbral range of 'wavetable' mode is exactly: sine ... saw. Nothing more. Nothing less.

I'd score higher if it were honestly labeled 'Morph' instead of 'Wavetable'. The name creates an expectation the implementation doesn't meet. Rename it, or — in Wave 2 — implement real wavetable scanning with at least 16 frames.

The spatial design is clean. Per-voice pan with modulation. The 8 voices spread across the stereo field when panMod is driven by different mod sources. The effects chain (delay → chorus → reverb) adds dimension. Not remarkable, but functional."

**Score: 6/10**

---

### G7 — Vangelis: The Emotional Engineer

"I press a key softly. Velocity 40. The filter opens gently — `velTimbre = 40/127 * 2000 = 630Hz` of additional cutoff. The wavefolder barely activates — `velFoldBoost = 1 + 0.31 * 2 = 1.63`. I press hard. Velocity 120. The filter throws open — `velTimbre = 1890Hz`. The wavefolder bites — `velFoldBoost = 2.89`. The journey between pp and ff is a filter sweep AND a harmonic enrichment. Two dimensions of velocity expression. Moog asked for four. I'll settle for two that are *meaningful* over four that are subtle.

Aftertouch is wired — Mod4 defaults to Aftertouch → Filter Cutoff. But depth is zero. I lean into the key and nothing happens. This is a piano with a sustain pedal that's disconnected. Wire it. Set the default depth to at least 0.15. Let me *feel* the key respond to my pressure.

Mod wheel drives the filter — line 486: `cutoffMod += modWheel_ * 4000.0f`. That's a wide sweep. Good. I can perform with the left hand while playing with the right. The engine responds to gesture.

But where is Expression (CC11)? It's not handled anywhere. The engine only responds to CC1 (mod wheel) and channel pressure. CC11 is a standard expressive input, and it's absent. D006 requires it. Add it.

The FLASH gesture system is emotionally interesting — a burst that decays. Ripple, Pulse, Flow, Tide. Four textures of momentary energy. I can trigger it from a pad during performance. This is a good instinct. But it needs to respond to velocity — a soft flash and a hard flash should feel different."

**Score: 6.5/10**

---

### G8 — Klaus Schulze: The Time Sculptor

"LFO rate range: 0.01 Hz to 30 Hz. The minimum is a 100-second cycle. Acceptable by D005, but the Gold Star Standard now requires 0.001 Hz — a 16-minute cycle. At 0.01 Hz, I can create a breathing pad that evolves over a minute and a half. At 0.001 Hz, I can create a landscape that shifts over an entire performance. The difference between 'interesting' and 'geological'. Lower the floor.

The LFO shapes are limited: sine (0), triangle (1), saw (2), square (3), sample-and-hold (4). Five shapes. The Gold Star requires seven, adding saw-down and smooth random. These two shapes are essential for asymmetric modulation and organic drift respectively. Add them.

I set up a patch: Source 1 Saw, Proc1 LP at 2000Hz, Mod1 as LFO → Cutoff at depth 0.5 and rate 0.05Hz. Hold a chord. The filter sweeps up and down over 20 seconds. Good. But only one modulator is active — the others are at zero depth. The init patch does not *breathe* by default. I need to manually activate modulation to hear evolution.

The engine *can* breathe. But it doesn't *choose* to breathe. In a seance, that's the difference between an engine that's alive and one that has the *potential* for life. Set a default slow LFO modulating *something* — even at subtle depth. Let the init patch move without being asked.

Autonomous modulation targets: 4 modulators can target 8 destinations each. 32 possible routings. The framework is deep. But the init patch activates only 1 of 32. Turn the framework into a garden — seed it with life."

**Score: 6/10**

---

## The Verdict — OBRIX (Wave 1)

### Seance Date: 2026-03-19

### The Council Has Spoken

| Ghost | One-Line Verdict |
|-------|-----------------|
| Moog | "The filter is correct but velocity skips Proc3 and the source mix is backwards." |
| Buchla | "The Constructive Collision is a lie — wavefolder and ring mod ignore the split routing." |
| Smith | "Voice leak on poly→mono switch; MIDI timing ignores sample position." |
| Kakehashi | "Good handshake, but the defaults hide the instrument's expressiveness." |
| Pearlman | "Semi-modular architecture is sound; FLASH gesture needs intensity and decay controls." |
| Tomita | "The Wavetable mode is a two-point crossfade wearing a wavetable costume." |
| Vangelis | "Two meaningful velocity dimensions, but aftertouch and expression CC11 are disconnected." |
| Schulze | "The LFO framework is deep but the engine doesn't choose to breathe by default." |

### Points of Agreement

1. **The split-processor routing is broken** (Buchla, Smith, Moog — 3/8). Wavefolder and ring mod apply globally instead of per-slot, contradicting the documented architecture.
2. **Velocity→timbre path is incomplete** (Moog, Vangelis — 2/8). Proc3 lacks velocity influence; aftertouch defaults to zero.
3. **The init patch hides expressiveness** (Kakehashi, Schulze, Vangelis — 3/8). Modulation and expression defaults are wired but silent.
4. **The brick concept is genuinely novel** (Buchla, Pearlman, Kakehashi — 3/8). Runtime-configurable synthesis modules with ocean identity — nothing else in the fleet does this.

### Points of Contention

**Buchla vs. Pearlman** on the wavefolder routing:
- Buchla: "The architecture must be honest. If the diagram shows split routing, the code must split."
- Pearlman: "The current behavior — global wavefold — is actually more useful for most users. The fold *should* apply post-mix. Just change the diagram to match the code."
- **Resolution:** Both valid. Offer per-slot AND post-mix wavefolder as an option. For now, fix the documentation to match current behavior, and add per-slot fold in Wave 2.

**Kakehashi vs. Schulze** on default modulation depth:
- Kakehashi: "Defaults should be audible. A beginner should hear the LFO breathing."
- Schulze: "Default modulation should be subtle — a whisper, not a shout. If the default LFO is too obvious, it fights the musical context."
- **Resolution:** Set Mod2 (LFO→Cutoff) depth to 0.1 — audible in solo, subliminal in a mix.

### The Prophecy

OBRIX has the architectural DNA of a flagship. The brick system — source, processor, modulator, effect, each independently configurable — is unique in the fleet and genuinely novel in the industry. But Wave 1 ships with routing bugs that contradict its own identity, expression gaps that hide its potential, and an init patch that doesn't demonstrate what the engine can become. Wave 2 must fix the routing topology, deepen the wavetable mode, lower the LFO floor, and ensure every expression input is not just wired but *alive by default*. The reef is young — but the coral is growing in the right direction.

### Blessings & Warnings

| Ghost | Blessing | Warning |
|-------|----------|---------|
| Moog | CytomicSVF is the right filter for this architecture | Source mix polarity will confuse every user |
| Buchla | Brick Independence concept — B016 confirmed | Wavefolder/ring mod routing breaks the Constructive Collision promise |
| Smith | Channel 2 coupling as brick complexity — architectural metadata | Voice orphaning on poly→mono switch |
| Kakehashi | Init patch is warm and inviting (Saw + LP + envelope sweep) | Aftertouch/LFO defaults at zero hide the instrument |
| Pearlman | Normalled defaults — 5 active parameters, rest ready to unfold | FLASH needs intensity and decay parameters |
| Tomita | Lo-Fi Saw is an honest brick — intentionally naive, labeled correctly | Wavetable mode is mislabeled — it's a 2-point morph |
| Vangelis | Velocity → filter + wavefolder = two meaningful expression dimensions | CC11 (Expression) is completely absent |
| Schulze | 4 modulators × 8 targets = 32 routing possibilities | LFO floor 0.01Hz is the D005 minimum, not the Gold Star standard |

### What the Ghosts Would Build Next (Wave 2 Priorities)

| Ghost | Feature |
|-------|---------|
| Moog | Per-source filter character (LP/HP/BP with slope options — 12dB, 24dB) |
| Buchla | Per-slot wavefolder with independent fold depth — honor the Constructive Collision |
| Smith | Sub-block MIDI event processing — sample-accurate note timing |
| Kakehashi | Preset randomizer — "shake the reef" button that randomizes brick types within musical constraints |
| Pearlman | FLASH intensity and decay parameters — the gesture needs gain staging |
| Tomita | Real wavetable scanning with 16+ frames — earn the "Wavetable" label |
| Vangelis | CC11 expression input + release velocity response |
| Schulze | LFO floor to 0.001 Hz + smooth random shape + init patch default modulation |

---

## Composite Score

| Ghost | Score |
|-------|-------|
| Moog | 7.0 |
| Buchla | 5.5 |
| Smith | 6.5 |
| Kakehashi | 7.0 |
| Pearlman | 7.0 |
| Tomita | 6.0 |
| Vangelis | 6.5 |
| Schulze | 6.0 |
| **Average** | **6.4/10** |

**Assessment:** Wave 1 establishes the foundation. The brick concept (B016) is novel and worth protecting. But routing bugs (wavefolder/ring mod), expression gaps (Proc3 velocity, CC11, aftertouch defaults), and the wavetable mislabel prevent a score above 7. The roadmap says 6.8→9.8 across 4 waves. This 6.4 is below the Wave 1 target of 6.8 — the routing topology fix alone would recover ~0.5 points. Fix the Constructive Collision routing, wire the expression defaults, and Wave 1 reaches its target.

---

*The ghosts return to the ether. The reef continues to grow.*
