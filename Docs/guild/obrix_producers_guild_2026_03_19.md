# Producer's Guild Report — OBRIX

**Date:** 2026-03-19
**Panels convened:** 25 genre specialists + full product team
**Engine:** ObrixEngine.h | Gallery: OBRIX | Accent: Reef Jade `#1E8B7E`
**Seance score:** 6.4/10 (Wave 1) | **Presets:** 0 (blocked until Wave 3 param freeze)
**Identity:** The coral reef — modular brick synthesis, baby brother of XOceanus

---

## Executive Summary

OBRIX is a runtime-configurable modular synthesizer inside XOceanus — 2 sources, 3 processors, 4 modulators, 3 effects, all swappable at runtime. The Guild finds the brick concept genuinely novel (no competitor offers this) but identifies three barriers to adoption: the routing bugs flagged by the seance make the architecture untrustworthy, the zero-preset state means producers can't discover the engine through sound, and the "Wavetable" source type creates a promise the implementation doesn't keep. The strongest guild signal: 18 of 25 specialists want OBRIX as their "sketch pad" synth — the one they reach for when they don't know what sound they want yet. That's the product position.

---

## Genre Panel Highlights

| # | Specialist | Verdict | Top Request |
|---|-----------|---------|-------------|
| 1 | **Beatrice "Trap" Morales** | "The Lo-Fi Saw brick is exactly what I need for grimy sub layers. But I need a sub oscillator — a dedicated -1 octave sine under Source 1." | Sub oscillator brick |
| 2 | **Marcus "Deep" Williams** | "Two oscillators with independent filters is what I pay $200 for in hardware. If the routing actually works as advertised, this is my go-to for deep house pads." | Fix the split routing — then I'm in |
| 3 | **Kai Suzuki** | "The modulation framework is deep enough for ambient. 4 modulators × 8 targets. But the LFO floor at 0.01Hz limits me. I work at 0.002Hz. And I need smooth random." | LFO floor to 0.001Hz + smooth random shape |
| 4 | **Sofia Chen** | "Velocity → filter + wavefolder is two dimensions of touch. For neo-soul Rhodes patches I need three: add velocity → attack time. The init patch velocity curve is good." | Velocity → envelope attack time |
| 5 | **Jerome "Dub" Baptiste** | "The delay effect is functional but it's not a dub delay. I need feedback control separate from the delay time. And ping-pong mode." | Dub-style delay with independent feedback + ping-pong |
| 6 | **Amara Okonkwo** | "Two independent sources at different tunings — I can voice a fifth, an octave, a minor third. This is ensemble synthesis. Give me 3 sources and I have a horn section." | Third source brick (Wave 2+) |
| 7 | **Lars Eriksson** | "For scoring, I need CC11 expression and longer envelopes. 10-second max attack is fine, but 20-second max release isn't enough for scoring pads. I need 60 seconds." | CC11 + 60s max release |
| 8 | **Priya Sharma** | "The init patch is warm and immediate. That's pop-ready. But I need a unison mode — one oscillator detuned across all voices for that modern supersaw width." | Unison detune per source |
| 9 | **Rico Valdez** | "The FLASH gesture is perfect for live drops. Trigger it on the downbeat, the filter bursts open, decays back. But I need it tempo-synced — the decay should lock to a beat division." | Tempo-synced FLASH decay |
| 10 | **Emma Blackwood** | "The wavefolder is the grit I want. But it applies globally — I want to fold Source 1 and keep Source 2 clean. That's the indie trick: dirty lead over clean sub." | Per-source wavefolder |
| 11 | **Dr. James Thornton** | "No sustain pedal response. For any keyboard-forward patch — piano, organ, electric piano — this is mandatory. CC64 as a mod source at minimum." | Sustain pedal (CC64) support |
| 12 | **Zero_1** | "Set Source 1 to noise, Source 2 to Lo-Fi Saw, ring mod them, max the wavefolder. Beautiful destruction. But ring mod *overwrites* everything — it should *blend*. I want to dial in 30% ring mod, not 100%." | Ring mod wet/dry blend |
| 13 | **DJ Phantom** | "Reese bass: Source 1 Saw, Source 2 Saw detuned +7 cents. LP filter with resonance. That's the recipe. OBRIX can do it. But I need the filter to self-oscillate at high resonance for that acid scream." | Filter self-oscillation at high Q |
| 14 | **Yuki Tanaka** | "The chorus effect is good but single-voice. For city pop shimmer I need a multi-voice chorus — 3 delay taps with different rates. And a high-pass in the feedback path to keep it clean." | Multi-voice chorus upgrade |
| 15 | **Nils Frahm Jr.** | "I want to use OBRIX as a felt piano engine. Sine source, gentle LP filter, slow attack, long release. But the ADSR is linear attack — I need exponential. Linear attack sounds mechanical." | Exponential attack option for ADSR |
| 16 | **Zara "Zed" Okafor** | "Square wave bass through LP filter with high resonance. That's garage. OBRIX does it. The source mix direction is backwards though — fix that and ship presets for my genre." | Fix source mix polarity |
| 17 | **Hassan El-Amin** | "No microtonal support. Maqam Bayati needs quarter-tones. At minimum, give me a global fine-tune parameter. Ideally, per-note detuning or Scala import." | Microtonal tuning / fine-tune per source |
| 18 | **Thandi Molefe** | "Amapiano log drums: sine with pitch envelope, short decay, heavy sub. OBRIX can build this if Mod1 is set to Envelope → Pitch with fast decay. Need a preset that demonstrates it." | Amapiano log drum preset |
| 19 | **Viktor Kozlov** | "Two detuned saws through LP filter with chorus — that's the Juno sound. OBRIX does this natively. The chorus isn't rich enough for synthwave though. Need stereo width and deeper modulation." | Richer chorus for synthwave |
| 20 | **MC Dubplate** | "I need a pitch envelope — Mod1 targeting Pitch with short decay for that dancehall stab 'bwaaamp'. The framework supports it. Just need presets." | Dancehall stab presets |
| 21 | **Aiko "Pulse" Nakamura** | "Supersaw requires unison detuning across voices. OBRIX can detune Source 1 and Source 2 separately, but I need 4-8 detuned copies of the same oscillator for a real supersaw stack. This is a Wave 2 feature." | Unison/supersaw mode |
| 22 | **Gabriel Santos** | "Lo-Fi Saw + bitcrusher = vaporwave in a box. But there's no bitcrusher brick. The Lo-Fi Saw is intentionally naive — good. I need a Lo-Fi processor to match: bitcrusher + sample rate reducer." | Bitcrusher processor brick |
| 23 | **Ingrid "Ice" Johansson** | "Industrial: noise through BP filter with high resonance, ring mod, wavefolder. The signal chain is correct. But the wavefolder applies post-mix when I want it on the noise source only. Fix the routing." | Per-slot wavefolder (echoes Buchla, Emma) |
| 24 | **Chen Wei** | "Sine with gentle vibrato — Mod2 as LFO → Pitch at very low depth — gives me an erhu-like lead. The framework is there. I need presets that demonstrate it." | Chinese-instrument presets |
| 25 | **Kwame Asante** | "Organ warmth: two sources at different octaves, both through LP filters, long release, chorus. OBRIX can do this. But I need drawbar-like control — more than 2 source levels. Consider harmonic additive mode in Wave 3." | Additive/drawbar source type |

---

## Preset Gap Analysis (Maya)

| Category | Missing Preset Archetypes | Guild Endorsements |
|----------|--------------------------|-------------------|
| **Bass** | Sub bass, reese bass, acid bass, amapiano log drum, 808 sub pad, dancehall stab | 8 panels |
| **Pads** | Deep house pad, synthwave pad, ambient evolving, worship pad, neo-soul Rhodes | 7 panels |
| **Leads** | Acid lead, erhu-like lead, grime square lead, progressive pluck, supersaw lead | 6 panels |
| **Textures** | Noise sculpture, lo-fi tape, industrial grind, bitcrushed atmosphere, drone | 5 panels |
| **Keys** | Felt piano, electric piano, organ, city pop keys, gospel organ | 5 panels |
| **Percussion** | Drum stab, tom synthesis, metallic hit, kick layer, lo-fi percussion | 3 panels |
| **FX/Gesture** | FLASH burst presets (all 4 gesture types), transition swoosh, riser | 3 panels |

**Priority:** Bass and pads first — 15 of 25 specialists asked for sounds in these categories. OBRIX's dual-source + independent-filter architecture naturally excels at layered bass and pad design.

**Mood distribution target (150 presets):**
| Foundation | Atmosphere | Entangled | Prism | Flux | Aether | Family |
|-----------|-----------|-----------|-------|------|--------|--------|
| 30 | 25 | 15 | 25 | 25 | 20 | 10 |

Foundation weighted high — the brick architecture is a building tool.

---

## Prioritized Feature Backlog (Maya + Derek + Ingrid)

| Priority | Feature | Guild Votes | Effort | Impact | Market Signal |
|----------|---------|-------------|--------|--------|---------------|
| **P0** | Fix wavefolder/ring mod split routing | 6/25 + seance 3/8 | Low | Critical | Table stakes — broken architecture |
| **P0** | Fix source mix polarity (inverted) | 3/25 + seance 2/8 | Trivial | High | UX bug, not a feature |
| **P0** | Add velocity → Proc3 cutoff | 1/25 + seance 3/8 | Trivial | Medium | D001 compliance |
| **P1** | CC11 expression input | 2/25 + seance 1/8 | Low | Medium | D006 compliance, Gold Star P4 |
| **P1** | CC64 sustain pedal as mod source | 2/25 | Low | Medium | Gold Star P4 |
| **P1** | LFO floor 0.001 Hz + smooth random + saw down shapes | 3/25 + seance 1/8 | Low | Medium | Gold Star P3, P7 |
| **P1** | Default mod depths audible (Mod2=0.1, Mod4=0.15) | 0/25 + seance 3/8 | Trivial | High | Init patch quality |
| **P2** | FLASH intensity + decay parameters | 1/25 + seance 1/8 | Low | Medium | Differentiator |
| **P2** | Tempo-synced FLASH decay | 1/25 | Medium | Medium | Live performance |
| **P2** | Exponential attack envelope option | 1/25 | Low | Low | Quality of life |
| **P2** | Sub oscillator brick (-1 oct sine under Src1) | 1/25 | Medium | High | Bass presets |
| **P2** | Ring mod wet/dry blend (not overwrite) | 2/25 + seance 1/8 | Low | Medium | Usability |
| **P3** | Bitcrusher/sample-rate-reducer processor brick | 2/25 | Medium | Medium | Lo-fi genre |
| **P3** | Unison detune mode (supersaw) | 3/25 | High | High | Pop/trance/EDM |
| **P3** | Multi-voice chorus (3 taps) | 2/25 | Medium | Medium | Synthwave/city pop |
| **P3** | Filter self-oscillation at high Q | 1/25 | Medium | Low | Acid genre |
| **P3** | Per-note microtonal detuning / Scala import | 1/25 | High | Low | Niche but Gold Star P4 |
| **V2** | Third source brick | 1/25 | High | Medium | Ensemble synthesis |
| **V2** | Real wavetable scanning (16+ frames) | 0/25 + seance 1/8 | High | High | Fixes mislabel |
| **V2** | Additive/drawbar source type | 1/25 | High | Medium | Organ synthesis |

---

## XPN Export Enhancements

| Need | Specialists | Notes |
|------|------------|-------|
| Per-brick XPN program maps | Beatrice, Jerome, Thandi | Each brick config as a separate MPC program |
| FLASH as MPC pad trigger | Rico, MC Dubplate | Map FLASH to Pad Bank D |
| Macro-to-Q-Link mapping | Marcus, Aiko | 4 macros → 4 Q-Links on MPC |
| Velocity layer export | Sofia, Lars | Export velocity-split programs (3 layers: soft/med/hard) |

---

## Playable Surface Recommendations

| Feature | Specialists | Rationale |
|---------|------------|-----------|
| **Brick Selector Pads** | Kakehashi (seance), Priya, Gabriel | 4 pads that cycle source/proc types without opening the full UI |
| **FLASH on Pad Bank D** | Rico, MC Dubplate, Beatrice | One-touch gesture trigger for live drops |
| **Macro Faders** | All 25 | CHARACTER / MOVEMENT / COUPLING / SPACE as primary faders |
| **XY Pad: Cutoff × Resonance** | Marcus, DJ Phantom, Zara | Two-finger filter sweep on PlaySurface |

---

## Technical Roadmap (Ingrid)

### Wave 2a — Routing Fixes (1-2 sessions)

1. **Fix wavefolder per-slot routing** — apply fold to `sig1`/`sig2`/`signal` based on which proc slot selected it
2. **Fix ring mod blend** — `signal = signal * (1-ringAmt) + (src1*src2) * ringAmt` instead of overwrite
3. **Fix source mix polarity** — swap `srcMix` and `(1-srcMix)`
4. **Add velTimbre to Proc3** — one line: `proc3Cut + cutoffMod * 0.3f + velTimbre`
5. **Wire CC11 and CC64** — add to MIDI handler, route as mod sources

*Dependency: none. All surgical fixes. Can ship same session.*

### Wave 2b — Expression Depth (1-2 sessions)

1. **LFO: lower floor to 0.001Hz, add saw-down (case 5) + smooth random (case 6)**
2. **Default mod depths** — Mod2 depth=0.1, Mod4 depth=0.15
3. **FLASH intensity + decay params** (2 new params: `obrix_flashIntensity`, `obrix_flashDecay`)
4. **Exponential attack option** for ADSR
5. **Sub-block MIDI processing** — process events at sample position within the block

*Dependency: Wave 2a must ship first (routing must be correct before adding depth).*

### Wave 3 — New Bricks (2-3 sessions, param freeze after)

1. **Sub oscillator brick** (-1 oct sine, mixed under Source 1)
2. **Bitcrusher processor brick** (bit depth + sample rate reduction)
3. **Ring mod becomes a blend** (wet/dry on the existing brick)
4. **Multi-voice chorus** (3 taps, rate spread)
5. **Unison detune mode** (flag per source: stack N detuned copies)

*Dependency: params freeze after Wave 3 — preset authoring begins.*

### Wave 4 — Preset Library + Polish (3-5 sessions)

1. **150 factory presets** across 7 moods per gap analysis
2. **Rename "Wavetable" to "Morph"** (or implement real wavetable in v2)
3. **Voice cleanup on poly→mono switch**
4. **CPU profiling at 96kHz/64 buffer**
5. **XPN export templates**

---

## Ops Notes (The Trio)

**Remy:** "The P0 routing fixes are surgical — 5-10 lines each. Ship them before anything else touches the engine. The longer they sit, the more presets get designed around broken behavior."

**Sam:** "Wave 3 param freeze is critical. We have 0 presets. Every new parameter added after presets exist is a migration headache. Lock the parameter IDs at 55+N (where N is Wave 2-3 additions) and don't touch them again."

**Fio:** "The sub-block MIDI processing is higher effort than it looks. The current loop structure processes all MIDI at block start, then runs the sample loop. Sub-block processing requires either block-splitting or interleaving MIDI events into the sample loop. Estimate 2-3 hours, not 30 minutes. Don't underestimate it."

---

## The Foreseer's Vision

### The Unseen Issue

Nobody mentioned the **init patch output level**. The current default level is 0.8 with velocity typically around 0.5-0.9, producing output near 0dBFS. The Gold Star Standard says -12 to -6dB. Every preset authored at the current level will be too hot for a mix bus. Fix the default output level to 0.65 *before* preset authoring begins in Wave 4 — otherwise 150 presets will ship mastering-level and producers will EQ-fight every one of them.

### The Unseen Opportunity

OBRIX's brick system + XOceanus coupling = **a configurable coupling processor**. No other engine in the fleet can reconfigure its signal path at runtime. If OBRIX receives coupling input and routes it through its brick chain (wavefolder, filter, ring mod, effects), it becomes a *programmable coupling processor* — not just a sound source but a sound *transformer*. The producer doesn't just couple ONSET → OBRIX for added harmonics. They configure OBRIX's bricks to *shape* the coupled signal: drum transients through a wavefolder, pad audio through a chorus, bass through a resonant filter. OBRIX becomes the fleet's universal coupling insert.

This requires one architectural addition: a "coupling input as source" brick type. Source 3: External (coupling input). Route it through Proc3 and the FX chain. Suddenly OBRIX isn't just a synth — it's a modular effects processor for the entire fleet.

### The Foreseer's Dominoes

**Decision: Ship OBRIX as a "sketch pad" synth (the position 18/25 specialists endorsed)**

```
→ 1. Producers use OBRIX as their first-reach instrument for ideation
→ 2. OBRIX presets become the most-shared in the community (because they're starting points, not finished sounds)
→ 3. The community demands more brick types — user-submitted brick ideas flood the Discord
→ 4. XO_OX must choose: curate brick additions (slow, quality-controlled) or open the brick SDK (fast, chaotic)
→ 5. The Brick Drop strategy (new bricks every 2-4 weeks) positions XO_OX as a living-instrument company, not a ship-and-forget plugin
→ 6. Competitors cannot replicate the Brick Drop — their architectures are fixed. OBRIX's runtime configurability is a structural moat
→ 7. OBRIX becomes the engine that *defines* XOceanus to new users — the gateway drug. The reef grows.
```

**Decision: Add "coupling input as source" brick (the unseen opportunity)**

```
→ 1. OBRIX becomes usable as an effects processor, not just a synth
→ 2. Every Entangled preset now has a second dimension — not just "which engines couple" but "how OBRIX shapes the coupling"
→ 3. Producers who don't like OBRIX's synthesis still use it as a coupling processor — expanding the user base
→ 4. The fleet's coupling system becomes audibly richer — coupling through OBRIX's wavefolder sounds fundamentally different from raw coupling
→ 5. Tutorial content writes itself: "Route ONSET through OBRIX's resonant filter for shaped drum transients"
→ 6. Other synth companies notice — "a synth that's also a programmable effects processor" has no precedent
→ 7. OBRIX graduates from "baby brother" to "the engine that makes the fleet sound different" — its identity shifts from toy to tool
```

### The Prediction

In 18 months, producers will not think of OBRIX as "the modular synth inside XOceanus." They will think of it as "the instrument that changes every time I open it." The Brick Drop strategy — new source types, new processor types, new effects — turns OBRIX into a subscription-like experience inside a one-time-purchase product. Each drop is a reason to open XOceanus again. The reef doesn't just grow — it *evolves*. And producers who've been using OBRIX for 18 months will have patches that literally couldn't have existed 12 months earlier, because the bricks they're built from didn't exist yet.

The danger: if the Brick Drop pace is too slow (quarterly, not biweekly), the reef feels stagnant. If the pace is too fast (weekly), quality control fails and broken bricks erode trust. The sweet spot is every 2-3 weeks, with each drop being 1-2 new brick types + 10-15 presets that demonstrate them. Not a product update — a *reef growth event*.

### Vision Statement

OBRIX is not a synthesizer. It is a construction site at the edge of the ocean. Every producer who opens it builds something different — not because they're choosing from a menu, but because they're *assembling* from parts. The parts grow over time. The combinations are infinite. And because the parts can also process sound from other engines, OBRIX doesn't just add to the fleet — it multiplies it. The baby brother becomes the nervous system. The reef becomes the ecosystem. Build in the order that gets the reef growing fastest: fix the routing, deepen the expression, add the bricks, then flood it with presets. By the time the presets arrive, the reef will be ready to hold them.

---

## Next Session Starting Points

1. **Wave 2a routing fixes** — fix wavefolder/ring mod per-slot, source mix polarity, Proc3 velocity, CC11+CC64. Surgical, low-risk, immediately improves seance score by ~0.5 points.

2. **Default output level to 0.65** — do this before any preset authoring. One line change, prevents 150 presets shipping too hot.

3. **LFO floor + shapes** — lower to 0.001Hz, add saw-down and smooth random. Small code change, large impact on ambient/generative producers (the vocal minority who write the most forum posts).

---

*The Guild disperses. The reef awaits its first presets.*
*XO_OX Designs | Producer's Guild Report — OBRIX | 2026-03-19*
