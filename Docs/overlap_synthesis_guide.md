# OVERLAP Synthesis Guide

**Engine:** OVERLAP | **Accent:** Bioluminescent Mint `#00FFB4`
**Parameter prefix:** `olap_` | **Voices:** 6 | **Gallery code:** OVERLAP

---

## What It Is

OVERLAP is the Lion's Mane jellyfish — a 6-voice Feedback Delay Network synthesizer whose voices are topologically entangled via mathematical knot routing matrices. Where most synthesizers route signals linearly, OVERLAP routes them through the crossing patterns of physical knots: unknot (self-only), trefoil (two independent 3-cycles), figure-eight (mirrored swap pairs), and torus T(p,q) (a single 6-cycle with harmonic-locked delay ratios).

The FDN is the synthesis — it generates sound by feeding 6 delay lines back through each other according to the knot matrix. Each delay line has a pitch, a dampening filter, and a feedback coefficient. The knot determines which voices "tangle" into which. Change the knot and you change the fundamental topology of the instrument — not just its color, but its species.

Voices also entrain biologically via a Kuramoto oscillator coupling model: each voice's natural frequency pulls slightly toward its neighbors', producing the synchronized pulsing of a real jellyfish bell. The Bioluminescence layer adds harmonic shimmer — a spectral exciter that is OVERLAP's version of the neon glow on the Lion's Mane's trailing filaments.

---

## The DSP Architecture

### Core: Feedback Delay Network (FDN)

The FDN consists of 6 delay lines. Per sample:
1. Read delayed output from each line (via a 1-pole LP dampening filter)
2. Multiply the 6-vector of dampened reads by the routing matrix (matrix multiply)
3. Scale by the feedback coefficient
4. Add exciter input signal per line
5. Write combined value back into each delay buffer

The key insight: the routing matrix IS the knot. A Trefoil matrix routes voice 0 → 1 → 2 → 0 and voice 3 → 4 → 5 → 3, creating two independent resonant 3-cycles. A Figure-Eight matrix creates symmetric swap pairs (0 ↔ 5, 1 ↔ 4, 2 ↔ 3). Torus T(p,q) creates a single 6-cycle with delay length ratios that enforce p:q harmonic locking across all six voices simultaneously.

All matrices are doubly stochastic (column sums = 1), preserving energy at the network level.

The `tangle depth` parameter (`olap_tangleDepth`) interpolates between the identity matrix (pure self-resonance, no inter-voice routing) and the full knot matrix: `M_eff = (1 - depth) * I + depth * M_knot`. At depth 0, all knots sound the same — pure self-resonance. At depth 1, the knot topology fully governs routing.

### Entrainment (Kuramoto Model)

The `Entrainment` module models biological pulse synchronization — the same mathematics that governs fireflies flashing in unison, pacemaker cells synchronizing a heartbeat, and jellyfish bell oscillations. Each voice has its own natural frequency; the entrainment coupling `K` determines how strongly each voice pulls its neighbors toward its phase. Low entrainment gives six independent resonators. High entrainment gives six voices that breathe together — a synchronized pulse.

The `olap_pulseRate` parameter sets the shared target frequency for the Kuramoto coupling. The `olap_entrain` parameter sets the coupling strength K.

### Bioluminescence

A spectral shimmer layer that tracks OVERLAP's output and emphasizes upper harmonics — a light-generating response to the sound. Controlled by `olap_bioluminescence`. At zero, clean FDN output. At maximum, harmonic shimmer that glows with each resonant peak — the neon blue-green light the Lion's Mane produces when agitated.

---

## The Macro System

### KNOT (M1) — `olap_macroKnot`
Scales the tangle depth — how deeply the knot topology routes signals between voices. At zero: self-resonance only (all knot types sound similar). At maximum: the full crossing pattern of the selected knot governs all inter-voice routing. KNOT is the macro that changes the instrument's fundamental species. Default 0.3 — partially tangled.

### PULSE (M2) — `olap_macroPulse`
Scales the pulse rate and LFO modulation depth — the breathing rhythm of the jellyfish bell. Low PULSE is slow, drifting, barely moving. High PULSE is a rapid rhythmic pulsing that can sync to host tempo when `olap_stepSync` is enabled. PULSE is the macro for animating otherwise static pads into living textures.

### ENTRAIN (M3) — `olap_macroEntrain`
Scales the Kuramoto coupling strength — how tightly the 6 voices synchronize with each other. Low ENTRAIN gives six independent resonators that drift apart (useful for chorus width and subtle detuning). High ENTRAIN gives six voices that breathe as one organism — a single unified pulse. The transition between independent and synchronized is the sound of a jellyfish forming.

### BLOOM (M4) — `olap_macroBloom`
Scales the bioluminescence and chorus mix simultaneously — the spectral glow of the instrument. Low BLOOM is clean and dark. High BLOOM adds shimmer, upper harmonic excitation, and a subtle chorus spread. BLOOM is the macro to automate when you want OVERLAP to "light up" — during a release, a key change, a moment of tension releasing.

---

## Key Parameters

| Parameter | Range | Function |
|-----------|-------|----------|
| `olap_knot` | 0–3 | Knot topology: 0=Unknot, 1=Trefoil, 2=Figure-Eight, 3=Torus |
| `olap_torusP` | 1–5 | Torus p value (harmonic ratio numerator, only for Torus knot) |
| `olap_torusQ` | 1–5 | Torus q value (harmonic ratio denominator, only for Torus knot) |
| `olap_tangleDepth` | 0–1 | Interpolation: identity (0) → full knot routing (1) |
| `olap_feedback` | 0–0.99 | FDN self-resonance intensity (approaches infinite sustain near 1) |
| `olap_dampening` | 0–1 | 1-pole LP coefficient on delay line reads (0=bright, 1=dark) |
| `olap_delayBase` | 5–100ms | Base delay length per voice |
| `olap_spread` | 0–1 | Stereo spread across 6 voices |
| `olap_filterCutoff` | 200–16000Hz | Pre-FDN filter cutoff |
| `olap_filterRes` | 0–1 | Filter resonance |
| `olap_filterEnvAmt` | -1–1 | Envelope modulation of filter cutoff |
| `olap_entrain` | 0–1 | Kuramoto coupling strength |
| `olap_pulseRate` | 0.01–10Hz | Entrainment target frequency |
| `olap_bioluminescence` | 0–1 | Spectral shimmer/upper harmonic excitation level |
| `olap_current` | 0–1 | Ocean current drift — gentle pitch drift per voice |
| `olap_currentRate` | 0.01–0.5Hz | Rate of ocean current modulation |
| `olap_chorusMix` | 0–1 | Chorus wet level |
| `olap_chorusRate` | 0.01–1Hz | Chorus LFO rate |
| `olap_diffusion` | 0–1 | Post-FDN diffusion/tail spread |

---

## Sound Design Recipes

**The Jellyfish Pad** — Knot: Trefoil. TangleDepth 0.8. Feedback 0.85. Dampening 0.5. Entrain 0.4. PulseRate 0.3Hz. Bioluminescence 0.3. BLOOM at 0.5. The two 3-cycles create two distinct resonant families that breathe at the same rate but maintain their topology.

**The Knot Drone** — Knot: Figure-Eight. TangleDepth 1.0. Feedback 0.95. Dampening 0.7 (dark and sustained). Entrain 0.9 (locked to single pulse). Spread 0.8. A single massive resonant object — all six voices mirroring each other, very dark, almost infinite sustain. Drone territory.

**The Harmonic Lock** — Knot: Torus T(3,2). TangleDepth 0.7. TorusP=3, TorusQ=2 (fifth harmonic relationship). The delay length ratios lock voices into a 3:2 frequency ratio — a perfect fifth emerges from the routing matrix alone, without an oscillator. Add FilterEnvAmt for a sweeping bright attack.

**The Shimmer Release** — Any knot with low tangleDepth. BLOOM at maximum on a long release. The bioluminescence layer becomes the main texture as the fundamental fades — the jellyfish glowing on its way back into darkness.

---

## Coupling Notes

OVERLAP couples outward as a spatial/textural engine — its FDN output is rich in late resonance and diffuse space. Receive from ONSET (transient excitation that feeds the FDN input — each drum hit becomes a new knot excitation) and ORACLE (stochastic breakpoints that nudge the delay times, destabilizing the topology). Drive OPAL (OVERLAP's diffuse output as grain source — granular clouds from jellyfish resonance) and OVERDUB (the spring reverb receives OVERLAP's spatial content and adds another dimension of depth).

The most important coupling dimension: OVERLAP responds to any Amp source as FDN excitation. An engine's VCA output triggering OVERLAP's input produces acoustic resonance — the knot network "ringing" in response to another engine's sound. This is the Lion's Mane coupling: light touches produce deep, multi-second resonant responses.
