# The Verdict — OBRIX
**Seance Date**: 2026-03-19
**Engine**: OBRIX | Ocean Bricks: Modular Synthesis Toy Box
**Score**: 7.2/10 → roadmap to 9.4/10

---

## The Council Has Spoken

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | CytomicSVF is an honest, serious filter choice — but missing filter key tracking will make OBRIX inconsistent across registers |
| **Buchla** | Five of seven source types are industrial waveform artifacts from 1963. The Wavefolder is the only brick that creates genuinely new timbral territory. |
| **Smith** | COUPLING macro architecture is qualitatively brilliant — combinatorial instruments created at runtime. Presets should be named for their coupling state. |
| **Kakehashi** | Default Sine→LP@8kHz is too polite. The 30-second test lives entirely in what sound greets the first keypress. COUPLING macro should do something audible even with no partner engine loaded. |
| **Pearlman** | Change default source to Saw — filter has nothing to sculpt with Sine. Semi-modular philosophy is correct but normalled routing must be *visible*, not hidden. |
| **Tomita** | One effect slot cannot construct a spatial field, only place a sound somewhere. Pre-delay and diffusion are spatial design; wet amount is merely volume. |
| **Vangelis** | Aftertouch routed to 8 targets is a genuine performance grammar. But zero presets means no demonstration — the instrument cannot sing without a player who already knows what it's doing. |
| **Schulze** | 20-second release and 0.01 Hz LFO floor are celebrated. LFO ceiling at 30 Hz stops him at the door of audio-rate modulation/synthesis crossover. |

---

## Points of Agreement (3+ ghosts converged)

1. **Default source must change from Sine to Saw** (Moog, Pearlman, Tomita, Kakehashi) — Sine has no harmonics above the fundamental; the filter is doing nothing. A Saw source gives the engine an immediate voice.

2. **Zero presets is the primary crisis** (Smith, Kakehashi, Vangelis, Tomita) — Architecture without demonstration is parts on the floor. At minimum 7 starter presets needed before V1.3a is considered shippable. Coupling presets named for their partner engine (e.g., "OBRIX+ONSET: Drum-FM'd Bass") are essential.

3. **COUPLING macro should work even when no partner engine is loaded** (Kakehashi, Smith) — Self-routing fallback: at low values, COUPLING creates gentle internal shimmer/chorus between active bricks. This makes the macro always audible from the first moment.

4. **Aftertouch as first-class performance grammar is correct** (Vangelis, Moog, Buchla) — Routing aftertouch to FilterCutoff, WavetablePos, EffectMix, or Pan gives the engine genuine expressiveness. This should be a highlighted feature in presets.

---

## Points of Contention

**Moog vs. Buchla — The Waveform Question**
- Moog: The CytomicSVF is engineered warmth. Trust the filter.
- Buchla: Five of seven sources are test-equipment artifacts. Wavetable is the only source with genuine timbral ambition, and its tables determine whether the engine is new or nostalgic.
- Resolution: Ship 1-2 ocean-sourced wavetable banks (recorded ocean acoustics, bioluminescent spectra). The architecture supports novelty; the content determines it.

**Vangelis vs. Schulze — Time vs. Immediacy**
- Vangelis: The engine must have first-take magic, must sing before you understand it.
- Schulze: The engine rewards patience; 20-second releases and 100-second LFO periods are the point.
- Resolution: Both are right for different players. Presets should span both modes — "instant" presets (fast attack, modulation already wired, sounds alive immediately) and "patient" presets (long release, slow LFOs, bloom-over-time).

---

## The Prophecy

OBRIX is architecture waiting for its voice. The brick-pool modular philosophy is genuinely novel within XOceanus — a meta-engine that becomes a different instrument depending on what bricks are active and what engines are coupled. But without factory presets demonstrating the coupling states, without a default patch that sounds inviting on first keypress, and without filter key tracking for cross-register consistency, the architecture's promise remains invisible. 

The Brick Drop release strategy — new bricks every 2-4 weeks — is the right long game. The ghosts urge: ship fewer bricks more beautifully before shipping more bricks quickly. The first preset library should be a curated gallery of 7-15 scenes, not a catalog of configurations. One scene: dawn breaking over open water.

The LFO audio-rate ceiling (30 Hz → 80+ Hz) is the single technical addition that would unlock the next tier. When modulation crosses into audio rate, it becomes synthesis, and OBRIX stops being a subtractive engine with flexible routing and becomes something genuinely unprecedented.

---

## Blessings & Warnings

| Ghost | Blessing | Warning |
|-------|----------|---------|
| Moog | CytomicSVF topology — serious, correct filter choice | Missing filter key tracking; inconsistent cross-register behavior |
| Buchla | Wavefolder in the Processor pool — the only genuinely new timbral brick | 5/7 waveforms are test-equipment artifacts; wavetable content defines originality |
| Smith | COUPLING macro — combinatorial instrument creation at runtime | Zero coupling presets; the ecosystem feature is invisible without demonstration |
| Kakehashi | Aftertouch + 4 Macros = accessible expressiveness | COUPLING macro does nothing audible when no partner engine loaded |
| Pearlman | Fixed brick pool = identity through constraint | Default Sine source gives filter nothing to sculpt; should be Saw |
| Tomita | SPACE as a performance macro = spatial intention | 1 effect slot cannot construct spatial field; pre-delay missing |
| Vangelis | Aftertouch routable to 8 targets = true performance grammar | 0 presets = no first-take magic; engine silent until a skilled player arrives |
| Schulze | 20s release + 0.01 Hz LFO = real temporal sculpture possible | LFO ceiling 30 Hz stops audio-rate modulation/synthesis crossover |

---

## What the Ghosts Would Build Next

| Ghost | Next Addition |
|-------|--------------|
| Moog | Filter key tracking: cutoff scales with note pitch (param: `obrix_filterKeyTrack`, 0-100%) |
| Buchla | Ocean-sourced wavetable banks: 8-16 tables drawn from recorded ocean acoustics and bioluminescent spectra |
| Smith | Coupling presets named for partner engines: "OBRIX+ONSET", "OBRIX+OPAL", "OBRIX+DRIFT" |
| Kakehashi | COUPLING self-routing fallback: when no partner loaded, macro creates internal shimmer between active bricks |
| Pearlman | Change default src1Type from Sine to Saw in ObrixEngine.h parameter default |
| Tomita | Pre-delay parameter on Reverb brick (2nd effect param, currently unused) |
| Vangelis | 7 starter presets, minimum — at least 3 with aftertouch already wired to demonstrate performance grammar |
| Schulze | LFO rate ceiling: extend from 30 Hz → 80 Hz to enter audio-rate modulation territory |

---

## Seance Score Breakdown

| Dimension | Score | Notes |
|-----------|-------|-------|
| Architecture originality | 9/10 | Brick-pool modular meta-engine is genuinely novel |
| Filter quality | 8/10 | CytomicSVF is excellent; missing key tracking |
| Source originality | 5/10 | 5/7 waveforms conventional; Wavetable potential unrealized |
| Expressiveness | 8/10 | Aftertouch + 4 macros + velocity = strong performance grammar |
| Spatial depth | 5/10 | 1 FX slot, basic reverb/chorus, no pre-delay |
| Preset library | 0/10 | Zero presets |
| Temporal depth | 7/10 | 20s release + 0.01 Hz LFO excellent; 30 Hz ceiling a limitation |
| Coupling architecture | 9/10 | COUPLING macro + 13 types = combinatorial ecosystem brilliance |
| First-take accessibility | 5/10 | Default patch too polite; no presets to demonstrate architecture |
| **TOTAL** | **7.2/10** | |

---

*Summoned by the Medium on 2026-03-19. Ghosts: Moog, Buchla, Smith, Kakehashi, Pearlman, Tomita, Vangelis, Schulze.*
