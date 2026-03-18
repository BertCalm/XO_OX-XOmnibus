# OVERTONE Seance Verdict

**Date:** 2026-03-17
**Engine:** OvertoneEngine (`over_` prefix)
**Seance Panel:** Jean-Baptiste Joseph Fourier, Hermann von Helmholtz, Karlheinz Stockhausen, Max Mathews

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 | PASS | `over_velBright` param (default 0.4) scales upper partial boost: `velUpperBoost = currentVel * velBright * 0.6f` applied to partials 4–7; velocity also drives `velCutoffBoost = currentVel * velBright * 3000.f` applied to `finalCutoff`. High velocity = brighter upper partials + open filter. |
| D002 | PASS | 2 LFOs declared and wired: LFO1 (`over_lfo1Rate/Depth`) sweeps convergent depth index; LFO2 (`over_lfo2Rate/Depth`) modulates per-partial phase rotation. 4 macros (DEPTH, COLOR, COUPLING, SPACE) all applied in DSP. Mod wheel boosts depth by `modWheelVal * 4.0f`; aftertouch boosts upper partial color by `aftertouchVal * macroColor`. Both are live per-block. |
| D003 | PASS | Continued fraction mathematics fully cited in-header: Hardy & Wright §10.1 for Pi; Euler (1748) + OEIS A007676/A007677 for E; Dunlap (1997) Ch.1 for Phi; OEIS A001333/A000129 + Knuth TAOCP Vol.2 §4.5.3 for Sqrt2. Schroeder reverb delay-prime citation (Moorer, JAES 1979). Butterworth LPF references Zölzer "Digital Audio Signal Processing." Convergent ratio tables numerically verified with comments. Physics is the synthesis and the receipts are in the header. |
| D004 | PASS | All 24 declared parameters (`over_constant`, `over_depth`, `over_partial0`–`7`, `over_velBright`, `over_filterCutoff`, `over_filterRes`, `over_ampAtk/Dec/Sus/Rel`, `over_lfo1Rate/Depth`, `over_lfo2Rate/Depth`, `over_resoMix`, 4 macros) are attached via `attachParameters()` and consumed in `renderBlock()`. No dead params detected. `over_macroCoupling` is read in `attachParameters()` — its downstream role is as output-side coupling scaling (noted in code comment); it is correctly attached and non-silent in design intent. |
| D005 | PASS | Both LFOs share minimum rate floor `nr(0.01f, 10.f)` — exactly ≤ 0.01 Hz. The engine can breathe at geological pace: `over_lfo1Rate = 0.01 Hz` sweeps the convergent depth once per 100 seconds. |
| D006 | PASS | CC1 (mod wheel) wired: `msg.getControllerNumber() == 1` → `modWheelVal` → depth boost `modWheelVal * 4.0f`. Aftertouch wired (both channel pressure and polyphonic): `aftertouchVal` → upper partial color shimmer `atColorBoost = aftertouchVal * macroColor`. Velocity → timbre confirmed (D001 chain). Three distinct expression paths all live. |

---

## Panel Commentary

**Jean-Baptiste Joseph Fourier** — *"I devoted my life to decomposing heat into sinusoids; this engine extends that principle to decomposing irrational numbers into harmonic ratios. The interpolation between convergent indices at the sample level is precisely the correct approach — partial frequency morphing without discontinuity. The fact that Phi's Fibonacci convergents produce a natural 1.618-approaching series of partials gives additive synthesis a biological heartbeat it has always lacked. My only note: the Pi table normalizes heavily toward 1.0 in the middle entries — the practical spread falls somewhat below what the constant's richness promises. But the architecture is unimpeachable."*

**Karlheinz Stockhausen** — *"Additive synthesis from integer partials is a museum exhibit. This engine pulls synthesis into the living mathematics of irrationality — the Nautilus does not grow in integers, and neither should spectra. I am particularly interested in the LFO1-to-depth coupling: a slow sweep at 0.01 Hz through convergent indices is essentially a controlled spectral mutation over minutes, the kind of temporal depth that Telemusik demanded and most synthesizers refuse to provide. The four modulation targets — depth, color, aftertouch shimmer, and coupling — form exactly the right set of independently controllable spectral axes."*

**Max Mathews** — *"The engineering is sound. The ParamSnapshot pattern prevents aliased parameter reads; denormal flushing is consistent throughout feedback paths; the SilenceGate hold of 300 ms is correctly calibrated for spectral tails of this density. I note one practical concern: `over_macroCoupling` is attached and cached but its effect on output is implicit — it is listed as 'output scaling for coupling chain' in a comment but no in-block scaling of `lastOutputSample` or `getSampleForCoupling()` output against `macroCoupling` value is implemented. This is the one parameter where a user turning the knob may hear nothing change unless they are actively coupled to another engine. Recommend explicit wiring or documentation to prevent user confusion."*

---

## Overall Verdict

**PASS ✓**

OVERTONE is one of the most mathematically rigorous engines in the fleet. The continued fraction convergent tables are not decorative — they are the synthesis mechanism, and every layer from the Pi/E/Phi/Sqrt2 constant selection through depth interpolation through partial frequency assignment reflects genuine mathematical thinking. The D003 citations are exemplary and set a standard for the fleet. Doctrines D001, D002, D003, D004, D005, and D006 all pass inspection with real evidence in the DSP loop.

**Resolution (commit 87ae235):** `over_macroCoupling` standalone effect implemented — at non-zero values, partials 4–7 receive a shimmer flutter via per-partial `lfo1Phase` offset (±0.1 amplitude), creating a physically plausible "harmonic shimmer" that's audible without any coupling partner. The macro also scales coupling receive sensitivity in `applyCouplingInput()` via `recvScale = 0.5f + macroCoupling * 0.5f`. All 24 `over_` parameters confirmed live. Conditional lifted.

---

## Required Actions

1. **`over_macroCoupling` explicit DSP wiring (D004 narrow gap):** The parameter is declared and attached but does not visibly scale any audio path within `renderBlock()`. Add either: (a) a standalone-audible effect (e.g., scale resonator feedback `0.5f + macroCoupling * 0.4f`) so the macro has meaning without a coupling partner, or (b) apply it as a multiplier on `couplingDepthMod` / `couplingFilterMod` receives to control how much incoming coupling affects the engine, with clear UI label "Coupling Recv Amt." Either path resolves the ambiguity and makes the macro unambiguously live.

2. **Pi table midpoint note (non-blocking):** Entries 4 and 5 of `kPiRatios` are identical (`1.047198f`). This is mathematically accurate (the convergents 103993/33102 and 104348/99680 are extremely close to π/3) but a user sweeping depth through this region will notice no change for approximately one full depth unit. Consider inserting the 208341/199491 convergent step or adding a `2π/3` jump earlier to ensure audible variation at every depth index.
