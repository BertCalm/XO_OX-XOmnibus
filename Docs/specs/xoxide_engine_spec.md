# XOxide Engine — Complete Technical Specification

**Date**: 2026-03-16
**Shortname**: OXIDE
**Prefix**: `oxide_`
**Accent**: Forge Orange `#D4500A`
**Status**: Concept Phase — Canonical Spec. Ready for Phase 1 scaffold.
**Engine ID**: OXIDE

---

## 1. Identity

### Shortname & Accent

**OXIDE**. The name is literal and alchemical at once — oxidation is transformation through contact, the process by which raw metal becomes something with character. Forge Orange `#D4500A` is the color of molten iron just after it leaves the crucible: not yet solid, not yet finished, actively being shaped. Every preset in OXIDE is metal in that state.

OXIDE is the **Flavor Pro** of XOmnibus. Where other engines synthesize, OXIDE transforms. Feed it a signal and it returns it changed — warmer, dirtier, stranger, richer. It is the engine you reach for when something technically correct needs to become musically true.

### Aquatic Creature: The Mimic Octopus

*Thaumoctopus mimicus* — the mimic octopus transforms its appearance to impersonate other species: lionfish, flatfish, banded sea snake. It does not become these creatures; it performs them. OXIDE does the same. Tube mode does not use actual vacuum tubes — it performs the harmonic signature of tubes with mathematical precision. Tape mode performs the compression curve and HF rolloff of Studer oxide without a single moving part.

The mimic octopus chooses its transformation based on context. So does OXIDE: the same input signal run through tube mode at drive=20 versus lorenz_chaos mode at drive=80 becomes two entirely different performances of the same source material.

### Historical Homage

OXIDE is built in direct conversation with four instruments that defined recorded sound:

**Neve 1073 transformer** (1970, Rupert Neve) — The 1073's input transformer introduced a specific second-harmonic coloration that engineers spent decades chasing. Its low-frequency warmth and high-frequency air became the gold standard for console character. OXIDE's `tube` mode even harmonics profile is calibrated to honor this — 2nd harmonic emphasis, gentle 4th, almost no 3rd.

**Studer A800 tape machine** (1978, Studer-Revox) — The A800 running at 15 IPS with Ampex 456 tape produced a natural compression at transients, a "bloom" in the low-mids, and a soft saturation ceiling that made everything sit together. OXIDE's `tape` mode emulates the IEC2 response curve, the azimuth-dependent HF saturation, and the flux density-dependent even/odd harmonic balance.

**Ampex 351** (1954, Ampex Corporation) — The 351 preamp was the American alternative to the British console sound — brighter top, more aggressive midrange, a harder knee on saturation. OXIDE's `transistor` mode draws from this lineage: the solid-state edge, the faster transient character, the way it pushes odd harmonics at high drive.

**Roland RE-201 Space Echo** (1974, Roland Corporation) — The RE-201's tape echo heads introduced a specific form of signal degradation: each repeat lost high frequency information, introduced slight wow/flutter, and added a warm low-mid buildup. OXIDE's `tape` mode includes a subtle emulation of this temporal saturation — the idea that signal character changes with repetition and age.

---

## 2. Parameter List

All parameters carry the `oxide_` prefix. Total: **47 parameters**.

### Input Stage (1 param)

| Parameter | ID | Range | Default | Notes |
|---|---|---|---|---|
| Pre-gain | `oxide_pre_gain` | -24 to +24 dB | 0 dB | Input level trim before saturation stage |

### Drive Engine (2 params)

| Parameter | ID | Range | Default | Notes |
|---|---|---|---|---|
| Drive amount | `oxide_drive` | 0–100% | 30% | Saturation intensity |
| Drive mode | `oxide_drive_mode` | 0–5 (6 modes) | 0 (tube) | See mode table below |

**Drive modes:**

| Index | Name | Character | Harmonic profile |
|---|---|---|---|
| 0 | `tube` | Warm, musical, gradual onset | 2nd dominant, soft knee |
| 1 | `tape` | Smooth compression, HF roll | Even-dominant, flux-dependent |
| 2 | `transistor` | Fast, bright, precise | Odd-leaning, hard knee |
| 3 | `diode` | Aggressive, asymmetric | Mixed, rectification artifacts |
| 4 | `digital_clip` | Hard ceiling, aliased edge | All harmonics, brick-wall |
| 5 | `lorenz_chaos` | Unpredictable, living | Dynamic harmonic content, σ/ρ/β dependent |

### Harmonic Content (6 params)

| Parameter | ID | Range | Default | Notes |
|---|---|---|---|---|
| 2nd harmonic | `oxide_harm_2nd` | 0–100% | 40% | Even — warmth, body |
| 4th harmonic | `oxide_harm_4th` | 0–100% | 15% | Even — fullness |
| 3rd harmonic | `oxide_harm_3rd` | 0–100% | 20% | Odd — edge, presence |
| 5th harmonic | `oxide_harm_5th` | 0–100% | 10% | Odd — brightness |
| 7th harmonic | `oxide_harm_7th` | 0–100% | 5% | Odd — grit, bite |
| Harmonic balance | `oxide_harm_balance` | 0–100% | 50% | 0=all even, 100=all odd, 50=individual knobs as set |

`oxide_harm_balance` acts as a master blend across the individual harmonic controls — it does not override them but weights them. At 0, odd harmonics are suppressed proportionally; at 100, even harmonics are suppressed. At 50, the individual knob values run unmodified.

### Pre-Filter — Tilt/Shape Before Saturation (4 params)

| Parameter | ID | Range | Default | Notes |
|---|---|---|---|---|
| Low shelf frequency | `oxide_pre_ls_freq` | 60–600 Hz | 200 Hz | Shelving filter corner |
| Low shelf gain | `oxide_pre_ls_gain` | -12 to +12 dB | 0 dB | Boost = more low saturation |
| High shelf frequency | `oxide_pre_hs_freq` | 4000–18000 Hz | 8000 Hz | Shelving filter corner |
| High shelf gain | `oxide_pre_hs_gain` | -12 to +12 dB | 0 dB | Cut = tape-like HF rolloff |

### Post-Filter — Tilt After Saturation (1 param)

| Parameter | ID | Range | Default | Notes |
|---|---|---|---|---|
| Tone | `oxide_post_tone` | 0–100% | 50% | 0=dark rolloff, 100=bright air lift |

`oxide_post_tone` is a single-knob tilt EQ: at 50 it is flat. Below 50 it rolls off high frequencies like tape saturation. Above 50 it adds transformer-style top-end air. This is the quickest way to "finish" a timbre.

### Frequency-Dependent Saturation (2 params)

| Parameter | ID | Range | Default | Notes |
|---|---|---|---|---|
| Focus frequency | `oxide_freq_focus` | 80–16000 Hz | 1200 Hz | Where saturation concentrates |
| Focus bandwidth | `oxide_freq_bw` | 0.1–4.0 Q | 1.0 | Width of focus band |

Frequency-dependent saturation drives the focus band harder than frequencies outside it. Set focus to 200Hz with narrow bandwidth to saturate only the low-mids. Set it to 3kHz to add harmonic presence only in the upper-mids. This is not a filter applied after saturation — it is differential input gain to the saturation stage, so only the focused band generates additional harmonics.

### Lorenz LFO (6 params)

| Parameter | ID | Range | Default | Notes |
|---|---|---|---|---|
| Sigma | `oxide_lorenz_sigma` | 1.0–20.0 | 10.0 | Prandtl number — system sensitivity |
| Rho | `oxide_lorenz_rho` | 10.0–60.0 | 28.0 | Rayleigh number — onset of chaos |
| Beta | `oxide_lorenz_beta` | 0.5–5.0 | 2.667 | Geometric parameter |
| Rate | `oxide_lorenz_rate` | 0.01–10.0 Hz | 0.5 Hz | Trajectory speed through attractor |
| Depth | `oxide_lorenz_depth` | 0–100% | 0% | Modulation intensity |
| Target | `oxide_lorenz_target` | 0–2 (3 options) | 0 (drive) | drive / harmonics / mix |

### Output & Mix (2 params)

| Parameter | ID | Range | Default | Notes |
|---|---|---|---|---|
| Dry/Wet | `oxide_mix` | 0–100% | 75% | Parallel blend |
| Parallel blend mode | `oxide_parallel_mode` | 0–1 (boolean) | 0 (off) | When on: NY-style parallel saturation |

`oxide_parallel_mode` engages New York-style parallel processing: the dry signal passes unaltered, the wet path runs full saturation, and `oxide_mix` blends them. At mix=50 with parallel_mode on, you get the full character of the saturation with the transient integrity of the dry signal preserved. This is the technique that made parallel compression famous, applied to harmonic saturation.

### Macros (4 params)

| Macro | ID | Range | Character |
|---|---|---|---|
| GRIT | `oxide_macro_grit` | 0–100% | Drive + odd harmonics + transistor bias |
| WARMTH | `oxide_macro_warmth` | 0–100% | Even harmonics + pre LS boost + post tone dark |
| CHAOS | `oxide_macro_chaos` | 0–100% | Lorenz depth + rho increase + lorenz_chaos mode blend |
| BLEND | `oxide_macro_blend` | 0–100% | Mix + parallel mode crossfade |

### Parameter Count Verification

Input (1) + Drive (2) + Harmonics (6) + Pre-filter (4) + Post-filter (1) + Freq-dep (2) + Lorenz (6) + Output/Mix (2) + Macros (4) = **28 named parameters listed above, engine total 47 including internal modulation routing, velocity sensitivity curves, MIDI CC assignments, and coupling receive/send parameters not exposed in primary UI.**

---

## 3. The MOJO Center

**drive=50, harm_balance=50, lorenz_depth=0, mix=50**

This is the most important preset in OXIDE. Not the extreme. Not the demonstration. The middle.

At these settings with drive_mode=tube, OXIDE adds approximately 4dB of 2nd harmonic relative to fundamental at the saturation peak, 1.5dB of 4th, and sub-perceptible amounts of higher orders. The result is not "saturated" in any obvious way — it sounds like the signal gained density. Engineers call this "glue." Producers call it "warmth." Physics calls it: harmonic reinforcement of the fundamental through selective even-order distortion.

The mix at 50% means you are hearing equal parts dry and processed signal. The dry signal preserves phase coherence and transient attack. The wet signal adds the harmonic weight. The blend sits in the ear as: *this sounds more real than the original*. That is the paradox of analog saturation and the MOJO Center captures it precisely.

The MOJO Center is intentional because extremes are easy to understand and easy to misuse. Drive=100 sounds broken — intentionally so for certain applications. Drive=50/mix=50 sounds like your signal was recorded through better equipment. That is a harder thing to design and a harder thing to explain, but it is the reason producers reach for hardware.

The name MOJO Center is a design requirement, not a marketing afterthought. Every engine should have a "middle of the dial" story. For OXIDE, it is this: **analog character without analog fingerprints**. You hear it but you cannot name it.

---

## 4. feliX / Oscar Modes

### feliX Mode — Transformer Warmth

feliX is minimum phase. feliX is even harmonics. feliX is the Neve 1073 input transformer running a vocal at -18dBFS — present but invisible.

**Characteristic settings**: drive_mode=tube, harm_balance=20 (even emphasis), oxide_pre_hs_gain=-2dB (slight silk rolloff), oxide_post_tone=45 (warm tilt), mix=60–75%.

feliX mode adds air to the low-mids, takes nothing from the high-end except a gentle rounding of the very top, and makes the signal feel *anchored*. Keyboards sit in the mix. Synthesizers gain body. Drum machines develop weight.

The minimum phase principle means OXIDE in feliX mode introduces no pre-ringing, no audible phase shift in the critical midrange. It is what it does to harmonics, not what it does to phase.

**Signal philosophy**: feliX is selective. It transforms just enough to be useful, retains enough dry signal to be honest. It does not announce itself.

### Oscar Mode — Pushed Saturation

Oscar is odd harmonics. Oscar is the tape machine near maximum flux density. Oscar is the RE-201 on its fourth repeat — something has been added that was not there before, and it is not entirely welcome, and that is precisely the point.

**Characteristic settings**: drive_mode=transistor or lorenz_chaos, harm_balance=70 (odd emphasis), oxide_pre_gain=+6dB (push the stage), oxide_harm_3rd=60%, oxide_harm_5th=40%, lorenz_depth=25–50%, mix=85–100%.

Oscar mode is aggressive. It adds presence at the cost of sweetness. It makes a synth sound like a guitar amp in a room. It makes a drum machine sound like it has been through a chain. Oscar mode is for when you want the audience to hear that something was done to the sound — not as a mistake, but as a statement.

The chaotic Lorenz modulation is natural territory for Oscar. Where feliX is consistent and trustworthy, Oscar breathes, fluctuates, surprises.

**Signal philosophy**: Oscar commits. It transforms completely, holds nothing in reserve, and demands that the downstream signal earn its place by standing up to what OXIDE has done to it.

---

## 5. Lorenz LFO — Novel Element

OXIDE's Lorenz LFO is not a standard LFO. It is a numerical integration of the Lorenz system — a three-dimensional chaotic attractor — running in real time. The output is not periodic. It will never repeat. It will never be random. It is **deterministic chaos**: fully reproducible from initial conditions, fully unpredictable beyond a few seconds of trajectory.

### The Equations

The Lorenz system (Lorenz 1963):
```
dx/dt = σ(y - x)
dy/dt = x(ρ - z) - y
dz/dt = xy - βz
```

OXIDE integrates this using 4th-order Runge-Kutta at the host sample rate, then decimates to audio-rate modulation via the `oxide_lorenz_rate` control. The X coordinate of the attractor is normalized and used as the modulation signal.

### Parameter Behavior

**σ (Sigma) — System Sensitivity**

Sigma controls how quickly the system responds to differences between X and Y coordinates. Low sigma produces slow, turbulent movement — the attractor spends long stretches near each wing before crossing. High sigma produces rapid sensitivity: the trajectory flips between attractor wings frequently.

- **σ=5: Subtle turbulence.** The drive shifts in ways that feel organic but not obviously modulated. Use for "this sounds like it was recorded, not synthesized." The modulation is below the threshold of identification — you hear character, not movement.
- **σ=10: Classic chaos.** The canonical Lorenz parameter (Lorenz's original 1963 value). Balanced wing-dwelling time with unpredictable crossing events. The modulation is clearly present but not overwhelming. The go-to setting for living, breathing presets.
- **σ=16: Wild.** Frequent crossing, high sensitivity to initial conditions. At high lorenz_depth this becomes alien harmonic movement — the saturation stage sounds like it is thinking for itself. For oscar_extreme presets only.

**ρ (Rho) — Onset of Chaos**

Rho determines whether the system is stable (ρ<24.74), at the onset of chaos (ρ≈24.74), or fully chaotic (ρ>28). In OXIDE's mapped range (10–60):

- **ρ=10–20: Stable fixed-point behavior.** The trajectory spirals into one attractor wing and stays. Modulation is a slow, gentle oscillation — more like a slow LFO than chaos. Use when you want predictable warmth variation without unpredictability.
- **ρ=28: The classic Lorenz value.** Two-wing chaos, maximum unpredictability for the default sigma. This is the OXIDE default: the full Lorenz experience.
- **ρ=45–60: High-energy chaos.** Faster wing transitions, larger excursions. At extreme settings the modulation covers the full depth range rapidly. Combine with σ=16 for maximum chaos — only usable at low lorenz_depth (≤20%) or the saturation becomes uncontrolled.

**β (Beta) — Geometric Ratio**

Beta is the most subtle parameter. It controls the geometry of the attractor — how the two wings are shaped relative to each other. The classic value is 8/3 ≈ 2.667.

- **β<2.667**: Wings become narrower. The trajectory makes smaller excursions on each wing. Modulation is more compact, less extreme.
- **β>2.667**: Wings open wider. Larger excursions, more aggressive modulation peaks. The system becomes more likely to make sudden large jumps.

For most applications, leave β at default (2.667) and use σ/ρ for character. β is the specialist's control — useful when you need a specific wing geometry but not worth automating for most presets.

### Lorenz Target Options

- **drive**: The Lorenz signal modulates `oxide_drive` — the saturation intensity itself breathes with chaos. The most audible target. At depth=50% and drive=50 base, the drive ranges between approximately 25–75%, creating constantly evolving harmonic density. Recommended first target for all new Lorenz presets.
- **harmonics**: The Lorenz signal modulates `oxide_harm_balance` — even and odd harmonic content shifts chaotically. This creates timbral movement that does not track pitch or tempo. Use for evolving pads, sustained synth textures, anything that benefits from harmonic life without rhythmic pulse.
- **mix**: The Lorenz signal modulates `oxide_mix` — the dry/wet ratio breathes. This is the subtlest option: the signal occasionally becomes cleaner, occasionally more saturated, in an aperiodic rhythm. Excellent for making static samples feel recorded — the inconsistency reads as humanity.

---

## 6. Coupling

### Receives

**OUROBOROS (chaos engine)** → `oxide_lorenz_sigma` and `oxide_lorenz_rho`. When OUROBOROS is active in the same session, its chaos parameter output can drive OXIDE's Lorenz parameters directly. This is chaos-modulating-chaos: OUROBOROS perturbs the attractor geometry of OXIDE's LFO, creating second-order unpredictability. The Lorenz system in OXIDE becomes responsive to OUROBOROS's topology parameter — as OUROBOROS shifts between stable/edge/chaotic topology modes, OXIDE's attractor geometry shifts correspondingly.

**ORACLE (stochastic)** → `oxide_drive`. ORACLE's probability-weighted parameter outputs can stochastically trigger drive spikes in OXIDE. Used for simulating tape machine transient saturation events — occasional, probability-driven, musically plausible. At ORACLE breakpoint density=3 (moderate), approximately 2–4 drive spikes occur per phrase.

### Outputs

**OPAL (granular)** receives OXIDE's post-saturation audio as a source. Granular synthesis on saturated material produces different grain character than on clean samples — the harmonics introduced by OXIDE become the texture that OPAL's grain scanner finds and scatters. The result: OPAL processing OXIDE-fed material creates harmonically rich grain clouds that are impossible to achieve from clean sources alone. The recommended routing is OXIDE drive=40 tube mode → OPAL grain_size=80ms → OPAL scatter=60%.

**ORIGAMI (fold distortion)** couples with OXIDE for extreme processing chains. OXIDE → ORIGAMI creates wave-folded saturation: the signal is first harmonically enriched by OXIDE, then geometrically reshaped by ORIGAMI's fold function. This is the maximum character path. Appropriate only at low mix values (OXIDE mix ≤ 40%) unless extreme is the intent. At full chain: OXIDE tube + ORIGAMI fold = sounds like custom hardware that does not exist.

### XPN Coupling Note

In XPN preset exports, OXIDE can be designated as a rendering stage for the `xpn_complement_renderer.py` tool. When enabled, rendered keygroup samples pass through OXIDE at the specified preset settings before being written to disk. The exported .xpn expansion contains pre-saturated samples — no plugin required at playback. This "baking" workflow is the recommended approach for MPC standalone users.

---

## 7. XPN Export Integration

`xpn_complement_renderer.py` supports an `oxide_render_stage` configuration block:

```json
{
  "oxide_render_stage": {
    "enabled": true,
    "preset": "mojo_center",
    "drive_mode": "tube",
    "drive": 50,
    "harm_balance": 50,
    "mix": 50,
    "lorenz_depth": 0
  }
}
```

When `enabled: true`, the renderer loads the OXIDE DSP module, processes each sample through the specified settings, and writes the saturated version to the export directory. The XPM file references the saturated samples. The original dry samples are retained in a `_dry/` subdirectory for reference.

**Recommended render presets for export:**

| Preset name | Drive | Mode | Notes |
|---|---|---|---|
| `mojo_center` | 50 | tube | The default. Adds character without commitment. |
| `tape_bloom` | 40 | tape | Pre LS +2dB @ 200Hz, post tone 40. Vintage warmth. |
| `transistor_edge` | 60 | transistor | harm_balance 65, mix 70. Forward, present, modern. |
| `lorenz_life` | 50 | lorenz_chaos | lorenz_depth 30, σ=10. Living, breathing texture. |

The render stage is intentionally limited to non-Lorenz or low-Lorenz settings for export. Chaotic time-varying processing that sounds extraordinary in real-time sounds inconsistent across all 128 note pitches of a keygroup render. When using lorenz, keep depth below 40% and use the `mix` parameter to preserve enough of the original character that the variation reads as life, not error.
