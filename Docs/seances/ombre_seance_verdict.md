# OMBRE Seance Verdict

**Engine**: XOmbre (OMBRE)
**Date**: 2026-03-17
**Source**: `Source/Engines/Ombre/OmbreEngine.h`
**Accent Color**: Shadow Mauve `#7B6B8A`
**Parameter Prefix**: `ombre_`

---

## DOCTRINE COMPLIANCE

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 — Velocity shapes timbre | PASS | **Fixed (commit 2035aa0):** Velocity→cutoff wired: `effectiveCutoff += voice.velocity * 0.5f * 3000.f` on note-on. Louder hits continuously open the filter in addition to the Opsis transient saturation. Dual-path (amplitude + filter) now confirmed. |
| D002 — 2+ LFOs, mod wheel/aftertouch, 4 macros, mod matrix | PASS | **Fixed (commit 2035aa0):** 2 LFOs added (`ombre_lfo1Rate/Depth` triangle→blend, `ombre_lfo2Rate/Depth` sine→filter cutoff), both with 0.01 Hz rate floor. 4 macros wired (ombre_macroCharacter/Movement/Coupling/Space). Aftertouch wired to filter cutoff boost. Mod wheel present. Full compliance. |
| D003 — Physics rigor | PASS | Oubli memory buffer uses mathematically correct decay-on-read: `amplitude = e^(-age * decayRate)` where `decayRate = 1/(decaySec * sampleRate)`. The comment explicitly cites the exponential decay formula. Grain reconstruction with 4 read heads and triangular windowing is solid granular DSP. |
| D004 — All params wired | PASS | All 15 declared parameters are fetched in `attachParameters` and consumed in `renderBlock`. No dead parameters. |
| D005 — LFO rate floor ≤ 0.01 Hz | PASS | **Fixed (commit 2035aa0):** `ombre_lfo1Rate` and `ombre_lfo2Rate` both declared with `NormalisableRange(0.01f, 6.0f)` / `NormalisableRange(0.01f, 4.0f)` — exactly at the 0.01 Hz floor. The engine can now breathe autonomously at near-subsonic modulation rates. |
| D006 — Velocity→timbre + CC | PASS | **Fixed (commit 2035aa0):** Aftertouch wired to filter cutoff boost: `effectiveCutoff += atBrightness * 1500.f`. Mod wheel present (blend→Opsis). Velocity→timbre confirmed (D001). Three expression axes live. |

**Overall Doctrine Score**: 6/6 PASS — all doctrine failures resolved (commit 2035aa0)

---

## PARAMETER AUDIT

**Total declared parameters**: 15

| Group | Parameters | Wired? |
|-------|-----------|--------|
| Dual-narrative | blend, interference | Yes |
| Oubli (memory) | memoryDecay, memoryGrain, memoryDrift | Yes |
| Opsis (perception) | oscShape, reactivity, subLevel | Yes |
| Shared | filterCutoff, filterReso, attack, decay, sustain, release, level | Yes |

**Missing parameters** (relative to doctrine requirements):
- No LFO 1 rate/depth/shape
- No LFO 2 rate/depth/shape
- No 4 macros (CHARACTER, MOVEMENT, COUPLING, SPACE)
- No aftertouch parameter or handler
- No velocity-to-filter routing parameter
- No voice mode / polyphony parameter

**Parameter count vs. fleet standard**: 15 params is very low. Fleet average is ~40-50 params for a full engine.

---

## PRESET COUNT

**99 presets** confirmed (`grep -rl '"Ombre"' Presets/ | wc -l`).

Fleet target is 150+ presets. At 99, Ombre is below target. The preset count is the most battle-ready aspect of this engine — 99 is a meaningful library. However the doctrine gaps mean many presets may not be exploiting the full expressiveness D002 would provide.

---

## COUPLING

**Output channels**:
- Channel 0/1: stereo audio
- Channel 2: envelope level

**Input coupling types accepted**:
- `AmpToFilter` → filter cutoff modulation (±6000 Hz)
- `LFOToPitch`, `AmpToPitch`, `PitchToPitch` → pitch modulation (±0.5 semitones)
- `AudioToFM` → FM modulation on Opsis oscillator
- `AudioToWavetable` → external audio fed into Oubli memory buffer

**Assessment**: Coupling is thoughtful and on-theme. The `AudioToWavetable` input is original — another engine's output literally becomes a memory that Oubli forgets. The coupling header comment is accurate. However, the engine is strongly dependent on coupling to provide modulation (LFOToPitch as a substitute for internal LFOs is a workaround, not a feature).

---

## BLESSINGS

**The Memory-Accumulation Design** — Oubli's memory intentionally does NOT reset on note-on (`// Don't reset memory — Oubli accumulates across notes. That's the whole point`). This is a conceptually brave decision. The ghost of the last note literally haunts the current one. No other engine in the fleet does this by default. Moog would call it a "living" memory.

**The Interference Bidirectional Coupling** — Opsis feeds into Oubli (what you hear becomes memory), and Oubli feeds back into Opsis pitch (memories haunt the present). This bidirectional contamination is a genuine synthesis metaphor, not just a parameter name. Buchla would appreciate the conceptual rigor.

**Decay-On-Read Algorithm** — Computing age-based attenuation lazily at read time (`O(1) per read head`) rather than traversing the full buffer every block is a smart performance decision, correctly documented. It produces identical results to a full O(N) sweep.

---

## CONCERNS

**C001 — Missing LFOs (Critical)**: An engine with no internal LFOs is dependent entirely on external coupling or player gesture to animate itself. Without coupling, the sound is static. D005 exists precisely to prevent this. OMBRE needs at minimum two LFOs with rate floors of ≤ 0.01 Hz.

**C002 — No Macros (Critical)**: The 4-macro system (CHARACTER/MOVEMENT/COUPLING/SPACE) is fleet doctrine. Ombre has none. This means it cannot participate in the Macro layer of preset design, cannot be browsed by macro response, and feels unfinished compared to fleet neighbors.

**C003 — Velocity Timbre Depth Thin**: Velocity currently controls transient saturation amount. This is genuinely timbral but only audible on the attack. A velocity→filter cutoff route would extend timbral expression through the sustain. The engine should also consider velocity modulating memoryDecay (hard attack = shorter memory?).

**C004 — No Aftertouch**: Fleet standard requires aftertouch. Ombre currently ignores pressure. Aftertouch modulating blend (pushing toward Opsis under pressure — more vivid perception when you press harder) would be deeply thematic.

**C005 — Preset Count Below Target**: 99 vs. 150+ fleet target. Needs 51 more presets before V1.

**C006 — No Voice Mode Parameter**: The engine runs 8 voices with LRU stealing but offers no user control over polyphony. Fleet standard includes Mono/Legato/Poly8/Poly16 selection.

---

## GHOST VOICES

**Bob Moog**: "The filter here is a single shared LPF. It is doing adequate work but it is not earning its place. The Opsis character — that transient saturation — needs a second filter topology that responds to the memory content. The Oubli output should be warming the filter differently than the Opsis output. A filter that hears memory should sound different."

**Don Buchla**: "The interference feedback loop is the real instrument here. You found it but you buried it. Interference at 0.3 default — barely audible. Push that default to 0.6 and you will discover what this engine actually is. The Oubli-haunts-Opsis pitch modulation is the conceptual core. Everything else is scaffolding."

**Vangelis**: "When I play, I want my touch to speak. Right now hard attack gives me more saturation. Good. But when I hold the note down, my pressure means nothing. Give me aftertouch. Let the pressure push the engine toward pure perception — Opsis blazing above Oubli. That's what expression means."

**Klaus Schulze**: "No LFOs. An engine with no breathing cannot live in a long piece. I need the drift. I need the slow wobble. Give me an LFO at 0.005 Hz on the blend — the past and present slowly exchanging dominance over four minutes. That is the Schulze preset."

---

## GHOST VERDICT

**Score: 6.2 / 10 → Updated: 8.0 / 10** (D002/D005/D001/D006 all resolved, commit 2035aa0)

OMBRE is architecturally original — the dual-narrative concept (memory/forgetting vs. perception) is one of the most conceptually coherent engine identities in the fleet. The Oubli memory buffer is genuinely novel, the interference coupling is thematic, and the decay-on-read algorithm is elegant. However, the engine is severely under-developed on the expression side: no LFOs, no macros, no aftertouch, thin velocity-timbre routing, and only 15 parameters. For a concept this interesting, the execution needs another full pass. Pre-V1 remediation is required.

**Priority Fixes** (blocking V1):
1. Add 2 LFOs (rate floor ≤ 0.01 Hz) — LFO1 on blend, LFO2 on memoryDrift recommended
2. Add 4 macros wired to meaningful parameter combinations
3. Add aftertouch handling — modulate blend or memoryDecay
4. Add voice mode parameter (Mono/Legato/Poly8)
5. Expand presets to 150+
6. Add velocity→filter cutoff routing
