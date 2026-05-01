# Wave 2 — Master Audit (FX Chain Validation Queue)

**Date:** 2026-05-01
**Author:** Wave 2.0 master audit (planned in PR #1486's `Docs/plans/2026-05-01-fx-engine-multi-session-plan.md` — that doc lands with PR #1486; until that merges, treat this audit as the standalone artefact for the Wave 2 queue)
**Source plan:** `Docs/specs/2026-04-27-fx-engine-build-plan.md` §4
**Scope:** 20 Wave 2 FX chains in `Source/DSP/Effects/` — current status `designed` in `Docs/engines.json`; goal is to flip to `implemented` after per-chain seance + any doctrine fixes.

This audit ranks the 20 chains by seance priority based on automated header inspection: implementation maturity (real DSP vs scaffold), declared-vs-cached-vs-used parameter consistency (D004), modulation source count (D002), rate-floor presence (D005), and any documented spec drift. The output is a recommended seance queue order so subsequent Wave 2 sessions (2.1–2.20) can pick up tasks in dependency-aware order without re-litigating the priority call.

**Headline finding:** All 20 Wave 2 chains have real DSP (no pure scaffolds) — the work is per-chain doctrine validation + bringing each chain through the seance protocol, not net-new implementation. Estimated total effort: ~120 hr (6 hr/chain × 20). Sessions can run fully in parallel from main.

---

## Ranked Audit Table

Sort key: D005 floor risk first (one chain near the floor), then high-modulation-density chains (more surface area to validate), then conventional-shape chains, then chains with documented "broken rule" intentional design choices that need extra ghost-panel scrutiny.

| Rank | Chain | Impl | D004 Risk | D002 Sources | D005 Floor | Notes |
|------|-------|------|-----------|--------------|------------|-------|
| 1  | **Ornate**     | real | low      | 5 | **0.05 Hz** (near threshold) | Granular Exciter — JFET smasher, 100-grain bank, dual-LFO optical phaser. D005 floor at 0.05 Hz vs ≤0.01 Hz spec target. **First to seance** to settle the rate-floor enforcement question. |
| 2  | **Outage**     | real | low      | 7 | ≤ 0.001 Hz | Lo-Fi Cinema — telephone filter + K-Field LPG + multi-head buffer + vintage chorus + spectral reverb. Highest mod-source count (7) → biggest validation surface. |
| 3  | **Opus**       | real | low      | 6 | ≤ 0.001 Hz | Tomorrow's Microcosm — bucket-brigade vibrato, granular micro-looper, dual optical phaser, memory delay, pitch-smeared reverb. Establishes envelope-driven modulation patterns reused downstream. |
| 4  | **Osmium**     | real | low      | 6 | ≤ 0.001 Hz | Sub-Harmonic Collapse — pitch detection + sub synth + JRC4558 preamp + 4-track tape comp + VHS. Sub-harmonic synthesis quality is the make-or-break — needs dedicated audition. |
| 5  | **Outlaw**     | real | medium   | 6 | ≤ 0.001 Hz | Cybernetic Child — PLL synth, touch-sensitive env filter, plasma distortion (8× OVS), hard VCA panner, magnetic drum echo. Intentional reduced hysteresis on glitch path — verify it's not a bug masquerading as a feature. |
| 6  | **Orogen**     | real | low      | 6 | ≤ 0.001 Hz | Ringing Abyss — transformer ring-mod (8× OVS), distorted plate reverb, granular time-stretch, tuned resonator FDN. High DSP density, well-structured. |
| 7  | **Orrery**     | real | low      | 5 | ≤ 0.001 Hz | Frozen Diamond — optical freeze, synth/reverse swell, dimension chorus, multi-tap delay, resonant synth reverb. Seamless 200 ms freeze loop with crossfade is the wildcard to audition. |
| 8  | **Occlusion**  | real | medium   | 5 | ≤ 0.001 Hz | Spatiotemporal Collapse — unique stereo-throughout architecture (no mono expansion). Hilbert + frequency-shift via PolyBLEP, micro-looper. The stereo routing complexity needs an extra ghost pass. |
| 9  | **Obdurate**   | real | low      | 5 | ≤ 0.001 Hz | Oscillating Drone Wall — self-oscillating fuzz gate, multi-stage VCA phaser, reverse delay trails, lo-fi havoc, plate + width. Resonant feedback paths are the audit risk. |
| 10 | **Oculus**     | real | low      | 5 | ≤ 0.001 Hz | Sentient Grid — plasma distortion, sequenced formant filter (vowel banking), dual optical phaser, prime-spaced multi-tap delay. Prime-spacing trick worth promoting if it ages well. |
| 11 | **Outbreak**   | real | low      | 5 | ≤ 0.001 Hz | Glitch Contagion — VHS degradation (S&H wow/flutter), octave fuzz, decimator, industrial reverb. Clean 11-param implementation. |
| 12 | **Oration**    | real | low      | 4 | ≤ 0.001 Hz | Post-Delay Auto-Wah — 3-voice BPM-synced pitch shifter, analog vibrato, sub-thump env filter, reverb. Tempo-sync interaction with host transport needs explicit test. |
| 13 | **Oubliette**  | real | low      | 5 | ≤ 0.001 Hz | Memory Slicer — Echo Collector, PLL synth, rhythmic slicer, SPX widener. Baseline 12-param chain with all primitives present. |
| 14 | **Oxymoron**   | real | medium   | 4 | ≤ 0.001 Hz | Gated Choir — parallel split with sidechain tap, Leslie horn/drum simulation, ultra-fast gate (<2 ms), resonant shimmer. Gate timing modulation paths need explicit documentation. |
| 15 | **Override**   | real | low      | 4 | ≤ 0.001 Hz | Digital Aggression — bit-crusher, hard-clip drive, sample-rate reduction. Conventional shape; quick seance. |
| 16 | **Obverse**    | real | medium   | 3 | ≤ 0.001 Hz | Reverse Gravity — FET compression (0.1 ms attack), gated reverse reverb, env filter. Low mod-source count (3) — D002 borderline; verify the env filter genuinely contributes. |
| 17 | **Offcut**     | real | low      | 4 | ≤ 0.001 Hz | Crushed Error — two-window overlap-add, bit-crusher, gate-driven freeze. Window overlap math worth verifying. |
| 18 | **Omen**       | real | low      | 4 | ≤ 0.001 Hz | Reverb-Driven PLL — reverb tail drives a phase-locked loop tuned to ambience density. Concept-test chain — does the wildcard land? |
| 19 | **Overshoot**  | real | high     | 2 | ≤ 0.001 Hz | Error Cascade — **Broken Rule:** pitch tracker after delay (intentional pitch errors). Lowest mod-source count (2). Two ghosts needed to confirm broken-rule passes character-over-correctness gate. |
| 20 | **Orison**     | real | high     | 2 | ≤ 0.001 Hz | Shattered Cathedral — **Broken Rule:** 100 % wet reverb first stage. SchmittTrigger gating on reverb tail not cached as APVTS param (chain-internal). Same broken-rule scrutiny as Overshoot. |

---

## Recommended Seance Queue Order

Run sessions 2.1–2.20 in this order. Within the queue, sessions are independent — any 2-5 can run in parallel (per Wave 2 strategy in the multi-session plan), but the priority order surfaces high-risk findings sooner so they can inform later seances.

1. **Ornate** — D005 floor question. If 0.05 Hz fails the doctrine, raise the rate range and re-test. The decision sets precedent for the other 19.
2. **Outage** — Highest mod density. Validate the routing-architecture limits before lower-complexity chains lock in patterns.
3. **Opus** — Envelope-driven modulation template chain. Patterns established here propagate to Outlaw, Oubliette, Obverse.
4. **Osmium** — Sub-harmonic synthesis quality is the wildcard. Audition early to flag any pitch-detection failures before downstream chains rely on similar primitives.
5. **Outlaw** — Intentional reduced hysteresis on glitch path. Confirm it's design intent, not D004 negligence.
6. **Orogen** — High oversampling DSP density. Validates the 8× OVS pipeline pattern reused in Outlaw and Orogen-style chains.
7. **Orrery** — Loop-freeze crossfade — the wildcard. Audition for click-free seam.
8. **Occlusion** — Stereo-throughout routing. Atypical architecture; ensure no L/R channel divergence bugs.
9. **Obdurate** — Resonant self-oscillation paths. Stability under sustained input.
10. **Oculus** — Prime-spaced delay taps. Confirm intended echo-pattern character; consider blessing-promotion if it sounds distinctive.
11. **Outbreak** — Clean lo-fi chain; quick sanity seance.
12. **Oration** — BPM-sync interaction with host; validate offline-render fallback.
13. **Oubliette** — Baseline 12-param chain; no surprises expected.
14. **Oxymoron** — Document gate-timing mod paths for D002 ledger.
15. **Override** — Conventional bit-crusher; quick.
16. **Obverse** — D002 borderline (3 sources). Confirm env filter genuinely contributes; if not, mark D002 weak.
17. **Offcut** — Verify two-window OLA math.
18. **Omen** — Wildcard concept test (reverb-driven PLL). May be the standout, may be inaudible — depends on tuning.
19. **Overshoot** — Broken Rule (pitch-after-delay). Need ≥ 2 ghosts to ratify the design choice or call it a defect.
20. **Orison** — Broken Rule (100 % wet first stage). Same scrutiny as Overshoot. Last in the queue so the Wave 2 protocol is fully tuned by the time we judge the most unusual topology.

---

## Doctrine Risk Summary (across all 20)

| Doctrine | Status |
|----------|--------|
| D001 — velocity → timbre | All chains pass via host-routed CC matrix (FX layer is downstream of voicing). |
| D002 — modulation        | 17 of 20 ≥ 4 mod sources; 3 below the 4-source mark (Obverse 3, Overshoot 2, Orison 2). Obverse is the borderline call (env filter contribution may or may not count). Overshoot + Orison sit at 2, likely PASS once internal modulation is itemised in seance. |
| D003 — physics           | N/A for control FX. |
| D004 — dead params       | 6 chains flagged for declared-vs-cached-vs-used parameter consistency: 4 medium-risk (Outlaw glitch hysteresis, Occlusion stereo routing, Oxymoron gate timing, Obverse env filter) and 2 high-risk on the broken-rule chains (Overshoot, Orison). Per-chain seance will resolve. |
| D005 — must breathe      | Ornate has the only declared floor near the spec threshold (0.05 Hz vs ≤ 0.01 Hz). Other 19 use ≤ 0.001 Hz floors uniformly. |
| D006 — expression        | Host-routed via CC matrix on every chain. Pass. |

**Forecast:** ~14 of 20 chains likely seance ≥ 8.0 on first pass. ~4 will need small fixes (D004 caching, D002 mod-source documentation) and re-seance. ~2 (Overshoot, Orison) need broken-rule defense — may pass at 7.5 with explicit "intentional" tagging in the verdict.

---

## Process Notes for Per-Chain Sessions

For each Wave 2.x session (1 chain = 1 session):

1. `/validate-engine <chain>` — automated D001–D006 check
2. `/synth-seance <chain>` — full ghost panel
3. Fix any doctrine violations inline in the chain header (additive only — `oubl_`, `osmi_`, etc. prefixes are FROZEN per CLAUDE.md)
4. Re-seance until verdict ≥ 8.0
5. `Docs/engines.json` — flip status to `implemented`
6. `python Tools/sync_engine_sources.py`
7. `Docs/seances/seance_cross_reference.md` — add row
8. `Docs/reference/engine-color-table.md` — update if blessing/debate state changes

Each session opens its own PR off main. PRs are independent — merge order is arbitrary.

---

Generated by Wave 2.0 master audit, 2026-05-01.
