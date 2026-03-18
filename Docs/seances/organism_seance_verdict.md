# ORGANISM Seance Verdict

**Date:** 2026-03-17
**Engine:** OrganismEngine (`org_` prefix)
**Identity:** The Coral Colony — Emergence Lime `#C6E377`
**Seance Panel:** John Conway (cellular automata / Game of Life), Stephen Wolfram (elementary CA / A New Kind of Science), Max Mathews (digital synthesis pioneer), Suzanne Ciani (modular expressionist)

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 | PASS | `velCutoffBoost = currentVel * velCutoff * 3000.f` — velocity adds up to +3000 Hz to filter cutoff on note-on, scaling with the dedicated `org_velCutoff` depth parameter. Timbre change is pronounced and musically meaningful. |
| D002 | PASS | Two LFOs present (LFO1: step-rate/filter mod; LFO2: filter cutoff offset). Mod wheel wired (CC1 morphs rule index across curated set). Aftertouch wired (mutation rate override). Four working macros: `org_macroRule` (rule sweep), `org_macroSeed` (re-seed), `org_macroCoupling` (cross-engine), `org_macroMutate` (mutation rate). Mod matrix effectively has 4+ slots via the CA output channels (cellFilterOut, cellAmpRate, cellPitchOut, cellFXOut each feeding different parameters). |
| D003 | PASS | The cellular automaton is correctly implemented: 1D elementary CA with 16-cell circular wrap, Wolfram 8-bit rule lookup (neighborhood index = `left*4 + center*2 + right`), proper modular arithmetic for wrap-around. The `OrgScopeHistory` moving-average system is a legitimate signal-smoothing technique. biquad filter uses bilinear-transform-derived coefficients (`w0 = 2π·fc/sr`, `alpha = sinW/(2Q)`) — standard matched design. The reverb uses a Schroeder-style 4-comb + 4-allpass topology. All DSP has mathematical integrity. No physically-modeled claims that exceed the implementation. |
| D004 | PARTIAL | 23 out of 24 parameters demonstrably affect audio. One concern: `org_macroCoupling` is declared and attached (`p_macroCoupling` pointer obtained) but its value is never read in `renderBlock()` — the coupling amount is only applied via `applyCouplingInput()` from external routing, not from the macro parameter itself. The macro label "COUPLING" implies it controls cross-engine coupling depth, but without routing that value to `couplingFilterMod` or `couplingPitchMod` internally, it is effectively a dead parameter until the XOmnibus coupling matrix reads it separately. This should be confirmed or the macro should be wired to a visible parameter effect (e.g., coupling receive sensitivity). |
| D005 | PASS | Both LFOs have a rate floor of `0.01 Hz` (`nr(0.01f, 10.f)`). At 0.01 Hz the period is 100 seconds — the engine can breathe on geological timescales. |
| D006 | PASS | Velocity shapes timbre (D001 confirmed). CC1 (mod wheel) morphs the Wolfram rule index, producing a continuous timbral transformation. Aftertouch overrides mutation rate (`aftertouchVal * 0.3f` added to `effectiveMutate`). Both channel pressure and polyphonic aftertouch messages are parsed. Three distinct expression axes covered. |

---

## Panel Commentary

**John Conway** finds the engine's circular-wrap 16-cell topology elegant: "The decision to loop cell 0 back to cell 15 is the correct choice — you get proper periodic boundary conditions, and the state space never leaks off an edge into silence. The scope-averaging history is a thoughtful addition; it prevents the abrupt timbral jumps that plague naive CA-to-audio mappings. I would have liked to see the rule space extended to 2D or totalistic rules, but as a first-generation CA synth this is rigorous."

**Stephen Wolfram** focuses on the curated rule set and the blend mechanism: "Rule 110 as the default is the right choice — it is the Turing-complete rule, the most complex elementary CA known. The macro sweep across {30, 90, 110, 184, 150, 18, 54, 22} is a well-curated tour of Class III and Class IV behavior. My only concern is the bit-blend interpolation between adjacent rules: xor-blending by majority vote is not a continuous mapping — you will hear discontinuous timbral jumps at the blend threshold of 0.5. A proper interpolation would require probabilistic rule selection per bit per step, but I acknowledge that is a significantly heavier computation."

**Suzanne Ciani** addresses the expressive architecture: "The mod wheel morphing the rule index rather than a conventional filter parameter is genuinely inspired — it gives the performer a lever that changes the engine's generative character, not just its brightness. Aftertouch driving mutation rate is equally compelling: a light touch produces a stable colony; a firm touch introduces entropy. These are meaningful performance gestures. The dead `org_macroCoupling` macro is the one shadow on what is otherwise a fully expressive instrument."

---

## Overall Verdict

**PASS ✓**

ORGANISM is doctrine-compliant on all six counts and represents one of the most conceptually coherent generative engines in the fleet. The cellular automaton is correctly implemented per Wolfram's formalism, the output mapping is well-considered (cells partitioned into functional quadrants covering filter, envelope rate, pitch, and reverb), and the expression architecture is notably sophisticated — mod wheel morphs rule identity, aftertouch introduces biological mutation, and velocity shapes the timbral entry point. The `OrgScopeHistory` moving-average system is a quiet piece of genuine engineering that prevents the raw CA output from being aurally chaotic.

**Resolution (commit 87ae235):** `org_macroCoupling` standalone effect implemented — `effectiveMutate += macroCoupling * 0.01f` makes the COUPLING macro increase automaton mutation rate audibly in isolation. The macro also scales coupling receive sensitivity in `applyCouplingInput()` via `recvScale = 0.5f + macroCoupling * 0.5f`. All 24 `org_` parameters confirmed live. Conditional lifted.

---

## Required Actions

1. **[D004 — REQUIRED] Wire `org_macroCoupling` to audio.** The value is snapshotted in `renderBlock()` but never consumed. Either: (a) multiply incoming `couplingFilterMod` / `couplingPitchMod` by a sensitivity scalar derived from `p_macroCoupling`, so the macro controls how strongly this engine responds to external coupling; or (b) document explicitly that `org_macroCoupling` is read by `MegaCouplingMatrix` rather than by the engine itself, and verify that pipeline path exists. Without confirmation of path (b), this is a dead parameter and a broken promise.

2. **[D002 — ADVISORY] Confirm macro routing in XOmnibus preset system.** The `COUPLING` macro label on `org_macroCoupling` suggests it should behave like the standard coupling macro seen on other engines (e.g., `olap_macroCoupling`). Verify the preset format and coupling strip correctly bind this macro to the coupling receive amount.

3. **[D003 — ADVISORY] Document rule-blend discontinuity.** The per-bit majority-vote blend between adjacent curated rules produces non-continuous transitions at blend = 0.5. This is not a bug — it is a known limitation of integer rule-blending. Add a comment in the source noting this behavior so future maintainers do not attempt to "fix" it into a smooth linear interpolation (which is not mathematically meaningful for CA rules).
