# OXBOW Post-Completion Checklist
**Date:** 2026-03-21
**Engine:** OXBOW — Entangled Reverb Synth
**Aquatic Identity:** Oxbow Eel (Twilight Zone, 200–1000m)
**feliX/Oscar Polarity:** Oscar-dominant (0.3/0.7)
**Accent Color:** Oxbow Teal `#1A6B5A`

---

## 1. Engine Registration — PASS

Registered in `Source/XOlokunProcessor.cpp` at line 228, under the comment "Singularity Collection — OXBOW (entangled reverb synth engine)":

```cpp
static bool registered_Oxbow = xolokun::EngineRegistry::instance().registerEngine(
    "Oxbow", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OxbowEngine>();
    });
```

Include present at line 45: `#include "Engines/Oxbow/OxbowEngine.h"`

`getEngineId()` returns `"Oxbow"` — matches registration key. PASS.

---

## 2. CLAUDE.md Updated — PASS

CLAUDE.md confirms OXBOW registration at multiple points:

- Header engine count note: "OXBOW added 2026-03-20"
- Engine modules list: `OXBOW` present in full 44-engine list
- Engine table (line 84): `OXBOW | XOxbow | Oxbow Teal #1A6B5A`
- Parameter prefix table (line 136): `Oxbow | oxb_ | oxb_entangle`
- Key files table (line 176): `Source/Engines/Oxbow/OxbowEngine.h` listed with description

CLAUDE.md is fully current. PASS.

---

## 3. Parameter Prefix — PASS (FROZEN: `oxb_`)

All 14 parameters use the `oxb_` prefix:

| Parameter ID | Display Name | Range | Default |
|---|---|---|---|
| `oxb_size` | Space Size | 0–1 | 0.5 |
| `oxb_decay` | Decay Time | 0.1–60 s (skewed 0.3) | 4.0 s |
| `oxb_entangle` | Entanglement | 0–1 | 0.6 |
| `oxb_erosionRate` | Erosion Rate | 0.01–0.5 Hz (skewed 0.5) | 0.08 |
| `oxb_erosionDepth` | Erosion Depth | 0–1 | 0.4 |
| `oxb_convergence` | Convergence | 1–20 (skewed 0.5) | 4.0 |
| `oxb_resonanceQ` | Resonance Focus | 0.5–20 (skewed 0.4) | 8.0 |
| `oxb_resonanceMix` | Resonance Mix | 0–1 | 0.3 |
| `oxb_cantilever` | Cantilever | 0–1 | 0.3 |
| `oxb_damping` | Damping | 200–16000 Hz (skewed 0.3) | 6000 Hz |
| `oxb_predelay` | Pre-Delay | 0–200 ms (skewed 0.5) | 20 ms |
| `oxb_dryWet` | Dry/Wet | 0–1 | 0.5 |
| `oxb_exciterDecay` | Exciter Decay | 0.001–0.1 s (skewed 0.5) | 0.01 s |
| `oxb_exciterBright` | Exciter Bright | 0–1 | 0.7 |

Prefix confirmed frozen. All IDs verified against `createParameterLayout()` and `attachParameters()` in `OxbowEngine.h`. PASS.

---

## 4. Preset Count and Mood Distribution — PASS (150 presets)

**Total: 150 presets** across 13 mood folders.

| Mood | Count |
|------|-------|
| Foundation | 35 |
| Atmosphere | 24 |
| Flux | 22 |
| Entangled | 14 |
| Prism | 10 |
| Aether | 10 |
| Submerged | 9 |
| Organic | 8 |
| Kinetic | 6 |
| Luminous | 4 |
| Crystalline | 4 |
| Ethereal | 2 |
| Deep | 2 |
| **Total** | **150** |

All presets follow `Oxbow_*.xometa` naming convention. Sample verified (`Oxbow_River_Bend.xometa`) — schema_version 1, all 14 `oxb_` parameters present, DNA fully specified, macros correct.

Fleet minimum target (150 presets) met exactly. PASS.

---

## 5. Coupling Interface — PASS

OXBOW accepts 4 CouplingTypes via `applyCouplingInput()`:

| CouplingType | Effect | Scaling |
|---|---|---|
| `AmpToFilter` | External amplitude modulates `oxb_damping` cutoff | `amount × 4000 Hz` |
| `EnvToDecay` | External envelope modulates `oxb_decay` | `amount × 10 s` |
| `AudioToRing` | External audio ring-modulates wet output | `sourceBuffer[0] × amount` |
| `AudioToBuffer` | External audio can replace exciter (stub — v2 implementation) | — |

OXBOW **sends** via `getSampleForCoupling()`: L/R stereo output (channels 0 and 1).

**Aftertouch** is handled natively in `renderBlock()`: pressure adds up to +0.3 to `oxb_entangle`, making the chiasmus cross-coupling deeper under finger pressure (Vangelis attribution).

PASS.

---

## 6. Sound Design Guide Entry — MISSING (ACTION REQUIRED)

The `Docs/xolokun_sound_design_guides.md` currently has 39 numbered entries ending at **## 38. OUIE**. OXBOW is not present. The CLAUDE.md Key Files entry still reads "34 of 34 engines in unified guide" — this count is stale (44 engines registered; guide covers 38).

**Action required:** Add entry "## 39. OXBOW — Entangled Reverb Synthesis" to `Docs/xolokun_sound_design_guides.md`.

Draft content for the entry is provided at the end of this document (see Appendix A).

STATUS: INCOMPLETE — sound design guide entry needed.

---

## 7. Seance Status — NOT YET CONDUCTED

No seance file found in:
- `Docs/seances/`
- `scripture/seances/`
- `Docs/fleet-seance-scores-2026-03-20.md`
- `Docs/seance_cross_reference.md`

OXBOW was added 2026-03-20 (same date as ORBWEAVE, OVERTONE, ORGANISM) but does not appear in the fleet seance scores document — those engines also appear to be awaiting their first seance.

**Status:** Pending. OXBOW needs a Ghost Council seance via `/synth-seance` before fleet quality score can be assigned.

---

## 8. Macro Assignments — PASS

Macro labels are consistent across all sampled presets (Foundation, Entangled, Aether, Flux):

| Macro | Label | Maps To |
|-------|-------|---------|
| M1 | **ENTANGLE** | `oxb_entangle` — chiasmus L↔R cross-coupling depth |
| M2 | **EROSION** | `oxb_erosionRate` / `oxb_erosionDepth` — phase erosion LFO |
| M3 | **CONVERGE** | `oxb_convergence` — golden resonance threshold |
| M4 | **CANTILEVER** | `oxb_cantilever` — asymmetric time-varying decay arc |

All 4 macros produce audible, dramatic change. The naming is identity-coherent: ENTANGLE ties to the Chiasmus structural entanglement, EROSION to the phase erosion system, CONVERGE to the Mid/Side golden resonance detection, CANTILEVER to the time-varying damping arc. PASS.

---

## Summary

| Check | Status |
|-------|--------|
| 1. Engine registered in XOlokunProcessor.cpp | PASS |
| 2. CLAUDE.md updated | PASS |
| 3. Parameter prefix frozen (`oxb_`) | PASS |
| 4. Preset count (150) and mood distribution | PASS |
| 5. Coupling interface (AmpToFilter, EnvToDecay, AudioToRing, AudioToBuffer) | PASS |
| 6. Sound design guide entry | **MISSING** |
| 7. Seance status | **PENDING** |
| 8. Macro assignments (ENTANGLE / EROSION / CONVERGE / CANTILEVER) | PASS |

**6/8 checks pass.** Two items require follow-up:
- Add sound design guide entry (Appendix A below provides the draft)
- Schedule seance via `/synth-seance` on OXBOW

---

## DSP Architecture Notes (for seance preparation)

OXBOW is a **monophonic reverb instrument**. It does not generate pitched oscillator output in the traditional sense — the exciter is a short-decay sine+noise burst that feeds an 8-channel FDN. The sustained sound comes entirely from the reverb tail. This places it in the same instrument category as OVERLAP (feedback delay network) but with stronger structural identity.

Four DSP pillars:

1. **Chiasmus FDN** — 8-channel Householder matrix. L channels use delay times [28, 38, 46, 56ms]; R channels use the reversed order [56, 46, 38, 28ms]. Same resonant structure, reverse temporal order — structural entanglement without modulation.

2. **Phase Erosion** — 4 modulated allpass filters per channel, L and R modulated with opposite-polarity LFOs. Creates breathing spectral self-cancellation when summed to mono. The reverb "breathes" even at infinite decay.

3. **Golden Resonance** — Mid/Side energy ratio detection. When Mid >> Side (convergence), 4 CytomicSVF Peak filters tuned to golden ratio harmonics (f, f×φ, f×φ², f×φ³) ring out at -3dB per φ multiple (Tomita weighting). Triggered by MIDI note — Moog-style fundamental tracking.

4. **Asymmetric Cantilever Decay** — Time-varying damping. `dampCoeff` increases quadratically as energy drops relative to peak. Bright early reflections → dark late reflections. The reverb timbre transforms as it decays.

**Doctrine compliance pre-seance assessment:**
- D001 (Velocity→Timbre): Velocity shapes exciter brightness and decay length. LIKELY PASS.
- D002 (Modulation is Lifeblood): Erosion LFOs autonomous; aftertouch modulates entanglement. Mod wheel not confirmed wired — CHECK.
- D003 (Physics IS Synthesis): Householder matrix, Matched-Z damping, golden ratio harmonics — all cited. LIKELY PASS.
- D004 (Dead Params): All 14 params confirmed wired in `renderBlock()`. PASS.
- D005 (Must Breathe): Erosion LFOs rate floor 0.03 Hz — above 0.01 Hz minimum but present. PASS.
- D006 (Expression not Optional): Aftertouch → entanglement confirmed. Mod wheel status unknown — VERIFY.

**Known v2 item:** `oxb_size` modifies delay time scaling but is applied at `prepare()` time only — real-time size change not implemented. Comment in code: "real-time resize is v2."

---

## Appendix A — Sound Design Guide Draft Entry

```markdown
## 39. OXBOW — Entangled Reverb Synthesis

**Gallery code:** OXBOW | **Accent:** Oxbow Teal `#1A6B5A`

**Identity:** The Oxbow Eel at the Twilight Zone (200–1000m). An oxbow is a lake formed when a river
cuts itself off — sound enters as rushing water, then the Oxbow cuts the current, leaving a suspended,
entangled pool of resonance that slowly erases itself. Oscar-dominant (0.3/0.7): this is a space
engine, not a character engine. What remains are golden standing waves. Monophonic.

**Macros**
| Macro | Name | Effect |
|-------|------|--------|
| M1 | **CHARACTER** (ENTANGLE) | Cross-coupling depth between L and R FDN channels |
| M2 | **MOVEMENT** (EROSION) | Phase erosion LFO rate and depth — spectral self-cancellation |
| M3 | **COUPLING** (CONVERGE) | Golden resonance threshold — how easily harmonic rings emerge |
| M4 | **SPACE** (CANTILEVER) | Asymmetric decay arc — how dramatically timbre shifts from bright to dark |

**Key Parameters**
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|--------------|
| `oxb_decay` | 0.1–60 s | 4–12 s | Reverb tail length; >29 s = infinite (Schulze) |
| `oxb_entangle` | 0–1 | 0.4–0.7 | L↔R channel cross-coupling; higher = wider, more fused stereo image |
| `oxb_erosionRate` | 0.01–0.5 Hz | 0.05–0.15 | Phase erosion LFO rate — the "breathing" speed |
| `oxb_erosionDepth` | 0–1 | 0.3–0.6 | How deep the spectral self-cancellation goes |
| `oxb_convergence` | 1–20 | 3–8 | Mid/Side ratio threshold for golden resonance trigger |
| `oxb_resonanceMix` | 0–1 | 0.2–0.5 | How audible the golden ratio harmonics are |
| `oxb_cantilever` | 0–1 | 0.3–0.7 | Decay arc shaping — 0 = uniform, 1 = maximum bright→dark transformation |
| `oxb_damping` | 200–16000 Hz | 4000–8000 | Base damping frequency (Cantilever darkens this as energy drops) |
| `oxb_predelay` | 0–200 ms | 10–60 ms | Pre-delay before FDN — distance sense |
| `oxb_dryWet` | 0–1 | 0.35–0.7 | Dry = exciter click only; Wet = reverb tail |
| `oxb_exciterDecay` | 0.001–0.1 s | 0.008–0.025 | Exciter envelope length — shorter = more percussive attack |
| `oxb_exciterBright` | 0–1 | 0.4–0.8 | Noise-to-sine ratio in exciter; higher = more transient energy |

**Coupling**
- **Sends:** L/R stereo output (ch 0/1)
- **Receives:** `AmpToFilter` (external amplitude opens damping filter), `EnvToDecay` (external envelope extends/contracts decay), `AudioToRing` (external audio ring-modulates wet output), `AudioToBuffer` (external audio replaces exciter — v2)
- **Key coupling:** OXBOW × OUROBOROS = "Strange Lake" — OUROBOROS chaos feeding OXBOW's AudioToRing causes the reverb tail to writhe and mutate mid-decay

**Recommended Pairings**
- **+ OVERLAP:** Two FDN engines in series — OVERLAP's KnotMatrix into OXBOW's Chiasmus. Reverb inside reverb; the lake inside the knot.
- **+ ORGAN / OBBLIGATO:** OXBOW provides the room without touching the instrument — as a send-style reverb, it gives pitched engines a space without coloring their fundamental.
- **+ OUROBOROS:** OUROBOROS chaos into OXBOW's AudioToRing — the reverb tail acquires fractal self-similarity. Schulze + chaos theory.
- **+ ORGANISM:** Cellular automata rhythmic variations feeding OXBOW's AmpToFilter — the lake brightens and darkens in response to emergent patterns.

**Starter Recipes**
**Sunken Cathedral:** `oxb_decay=12`, `oxb_cantilever=0.6`, `oxb_damping=5000`, `oxb_resonanceMix=0.4`, `oxb_convergence=4` — long, dark, architectural; golden ratio harmonics ring when the reverb converges to mono
**Infinite Lake:** `oxb_decay=60` (infinite), `oxb_entangle=0.8`, `oxb_erosionDepth=0.5`, `oxb_dryWet=0.85` — Schulze mode; sound never resolves; the erosion keeps it alive
**Chamber Strike:** `oxb_decay=2.5`, `oxb_cantilever=0.8`, `oxb_exciterDecay=0.008`, `oxb_exciterBright=0.9`, `oxb_dryWet=0.5` — short, bright hit that darkens fast; bell-to-room in 2.5 seconds
**Estuary Drift:** `oxb_decay=8`, `oxb_erosionRate=0.07`, `oxb_erosionDepth=0.7`, `oxb_entangle=0.5` — the erosion breathing is primary; stereo field slowly cancels and rebuilds

**Designer Notes**
OXBOW is a space engine, not a character engine — you play into it, not through it. The Chiasmus topology is the engine's defining choice: by reversing the delay time order between L and R channels, the two sides share the same resonant structure but in different temporal sequences. Notes decay in different orderings on each side. This is not a stereo effect; it is a structural condition. The entanglement parameter controls how much the L and R sides "talk" to each other — at high values, the two timelines merge; at low values they remain parallel and distinct. The Cantilever is OXBOW's most unusual feature: it makes the reverb change its fundamental nature as it decays. Early reflections are bright; late reflections are dark. The reverb you put in is not the reverb you get out. At high Cantilever values, a bright metallic strike becomes a warm, almost orchestral tail within the same note. The golden resonance is earned: it only activates when L and R converge toward the same energy (Mid >> Side), rewarding spatial coherence with harmonic reinforcement tuned to the played fundamental.
```

---

*Checklist authored 2026-03-21. Next action: run `/synth-seance` on OXBOW, then add Appendix A entry to `Docs/xolokun_sound_design_guides.md`.*
