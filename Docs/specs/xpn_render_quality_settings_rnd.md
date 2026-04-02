# XPN Render Quality Settings — R&D

**Topic**: Optimal sample rendering settings when creating XPN pack samples from XOceanus
**Date**: 2026-03-16
**Status**: Reference spec

---

## 1. Render Duration Guidelines

Match render length to the instrument's natural decay envelope. Cutting too short introduces clicks; padding too long wastes storage and slows MPC load times.

| Instrument Type | Target Duration | Notes |
|---|---|---|
| Kick / Snare / Clap | 1–3 s | Capture full body decay to -60 dBFS |
| Hi-hat (closed) | 0.5–1 s | Tight; gate hard if needed |
| Hi-hat (open) | 1.5–3 s | Let ring fully |
| Bass notes | 4–8 s | Sustain + release tail |
| Lead / melodic one-shots | 2–5 s | Release must fully resolve |
| Pads / atmospheres | 8–30 s | Capture full modulation cycle |
| Loops | Exact bar length at target BPM | e.g., 4 bars @ 90 BPM = 10.667 s |

**Formula**: `render_length = decay_time × 2 + envelope_release`

For loop renders: `bar_length_seconds = (bars × 4 × 60) / BPM`. Always render exactly this length — never round — or the MPC grid will drift.

---

## 2. Sample Rate Selection

**XO_OX standard: 44100 Hz.**

The Akai MPC series runs its internal engine at 44100 Hz. Delivering 48000 Hz samples forces the MPC to resample on import, introducing interpolation artifacts that are audible on transient-heavy material (kicks, snares, hi-hats). Resampling also adds a small latency offset that accumulates across a full kit pad map.

44100 Hz also produces smaller files (~8.5% smaller than 48 kHz at equal bit depth), which matters when shipping packs with 300–500 samples.

Never deliver 96 kHz samples in an XPN pack. The MPC will downsample them, and the quality gain from high sample rate does not survive that conversion chain.

---

## 3. Bit Depth: Render vs. Delivery

**Render internally at 32-bit float. Deliver at 24-bit integer with TPDF dither.**

XOceanus's OfflineAudioContext operates at 32-bit float precision. Rendering directly to 24-bit integer truncates the lower 8 bits without dither, producing low-level quantization distortion that is audible on quiet pad tails and long reverb decays.

**TPDF dither** (Triangular Probability Density Function) adds exactly 1 LSB of shaped noise before truncation. At 24-bit depth, TPDF noise sits at approximately -144 dBFS — effectively below the noise floor of any monitoring system or playback chain. Use TPDF, not noise shaping (e.g., POW-r), which can cause inter-sample peaks that clip after MPC's internal gain staging.

**Never deliver 32-bit float WAV to the MPC.** The MPC X and Live II will import 32-bit float files, but the gain staging is undefined and levels will be inconsistent across pads.

---

## 4. Noise Floor Management

**Recommendation: capture raw, then apply a -70 dBFS noise gate with 5 ms release.**

XOceanus synthesis engines (particularly granular, FM, and physical modeling voices) produce synthesis noise at -80 to -90 dBFS during the sustained portion of a note. This is not a defect — it is part of the character.

Do not normalize before gating. Normalization raises the noise floor along with the signal, making the gate threshold harder to set cleanly and potentially introducing pumping artifacts.

Workflow:
1. Render raw at 32-bit float.
2. Apply noise gate: threshold -70 dBFS, attack 0 ms, release 5 ms. This catches the true tail without cutting the signal body.
3. Dither to 24-bit TPDF.
4. Trim post-gate silence to ≤ 50 ms of trailing silence.

---

## 5. Stereo vs. Mono

| Instrument | Format | Reason |
|---|---|---|
| Kick / Snare / Clap / Tom | Mono | Phase coherence in the mix; MPC can widen in the pad channel if desired |
| Hi-hat (closed and open) | Mono | Same |
| Bass (any) | Mono always | Stereo bass causes phase cancellation on mono playback systems |
| Lead melodic | Stereo only if intentional wide field | Chorus/ensemble leads: stereo. Single-voice leads: mono |
| Pads / atmospheres | Stereo | Width and movement are core to the character |
| Loops | Match source | Drum loops: mono. Atmospheric loops: stereo |

Stereo samples use 2× the storage and 2× the MPC voice allocation. Apply stereo only where it carries sonic information.

---

## 6. Fade-Out vs. Hard Stop

Any sample that does not decay to silence naturally before the render endpoint needs a **5 ms linear fade-out** applied at the tail. 5 ms is long enough to prevent a click at all audible frequencies (click artifacts begin above ~200 Hz) and short enough to be inaudible as a volume event.

Do not use cosine or logarithmic fade curves for tail cleanup — they change the perceived decay character. Linear fade at the last 5 ms only.

Samples that reach -70 dBFS naturally before the endpoint do not need a manual fade.

---

## 7. Pre-Roll and Pre-Silence

| Instrument | Pre-silence | Reason |
|---|---|---|
| Kick / Snare / Hat / Perc | 0 ms | Transient must be at sample start; any offset delays the hit relative to the grid |
| Bass / Lead one-shots | 0 ms | Same grid-lock requirement |
| Pads / atmospheres | 5 ms | Allows slow attack to begin before the MPC's internal voice allocation overhead; prevents the first millisecond from being clipped |
| Loops | 0 ms | Loop point must align to bar start exactly |

The MPC adds approximately 3–5 ms of internal latency for sample playback triggering. Do not compensate for this in the sample file — the MPC's timing compensation handles it globally. Adding pre-roll to compensate per-sample creates inconsistency across the pad map.

---

## Summary Table

| Parameter | Setting |
|---|---|
| Sample rate | 44100 Hz |
| Internal render depth | 32-bit float |
| Delivery depth | 24-bit integer |
| Dither algorithm | TPDF |
| Noise gate threshold | -70 dBFS |
| Noise gate release | 5 ms |
| Tail fade (where needed) | 5 ms linear |
| Drum format | Mono |
| Bass format | Mono |
| Pad/atmosphere format | Stereo |
| Loop render length | Exact bar length (no rounding) |
| Kick/snare/hat duration | 1–3 s |
| Bass duration | 4–8 s |
| Pad duration | 8–30 s |
