# XOmnibus Fleet Seance Score Audit
**Generated:** 2026-03-20
**Scope:** All 42 registered engines at time of writing (OXBOW + OWARE added same day — see audit note below)
**Sources:** `Docs/seances/*_verdict.md`, `Docs/seances/*_seance*.md`, `scripture/seances/*.md`, `Docs/seance_cross_reference.md`, `Docs/seance_ombre_orca_octopus.md`, `Docs/seance_ostinato_opensky_oceandeep_ouie.md`, `~/.claude/skills/synth-seance/knowledge/index.md`

---

## Summary

| Metric | Value |
|--------|-------|
| Total engines registered | 44 (42 at time of this audit; OXBOW + OWARE added 2026-03-20, not yet seanced) |
| Engines with formal numeric score | 35 |
| Engines with non-numeric verdict only | 7 (OBSCURA, OPTIC, ORBITAL, ORGANON, OVERBITE, OSPREY, OSTERIA — all approved) |
| Engines with NO seance of any kind | 0 |
| Score range (numeric) | 5.9 → 8.7 |
| Median score (numeric, post-recovery) | ~7.8 |

> NOTE: All 42 engines covered by this audit have been through at least one seance. OXBOW and OWARE (engines 43 and 44, both added 2026-03-20) are NOT covered here — they were registered after this document was written. Their seances are pending. The "UNRATED" classification does not apply to the 42 engines surveyed here — seven have non-numeric verdicts (see Section 4). Those engines are treated as "approval-class" and noted separately.

---

## Score Reference Key

Scores reflect the **most authoritative / most recent** verdict for each engine:
- For engines with multiple seances (e.g., OUTWIT, OVERLAP, ORCA, OCTOPUS), the latest formal ghost council score is used.
- Post-recovery estimated scores (OBLIQUE, OBSIDIAN, ORCA) are flagged as estimates.
- Engines with non-numeric verdicts (APPROVED, Production-ready, etc.) are listed separately.

---

## Complete Fleet Table (sorted by score, ascending)

| Engine | Score | Top Weakness | Seance Date | Verdict Source |
|--------|-------|--------------|-------------|----------------|
| OBLIQUE | 5.9/10 → ~7.2 est. | Dead percDecay, no LFOs, 6 presets only; post-Round-8A recovery estimated 7.2 | 2026-03-14 | `Docs/oblique_deep_recovery.md`; `Docs/seance_cross_reference.md` |
| OCELOT | 6.4/10 | macro_1–4 all dead wired (post-fix: wired to DSP); Ecosystem Matrix never audibly demonstrated | 2026-03-14 | `Docs/seance_cross_reference.md`; `Docs/ocelot_deep_recovery.md` |
| OBESE | 6.6/10 | Zero LFOs (resolved post-sweep); no CC; Mojo axis unique but nothing modulates it | 2026-03-14 | `Docs/seance_cross_reference.md` |
| OBSIDIAN | 6.6/10 → 8.2 est. | R-channel filter bypass (P0, fixed); formant param ID collision (P0, fixed); velocity amplitude only (resolved Round 9A) | 2026-03-14 | `Docs/seance_cross_reference.md`; `Docs/oblique_deep_recovery.md` Round 9A |
| ODDOSCAR | 6.9/10 | Zero LFOs (resolved); no aftertouch; Moog ladder excellent but engine never evolves | 2026-03-14 | `Docs/seance_cross_reference.md` |
| OBLONG | 7.x/10 | CuriosityEngine unresponsive to touch; D006: no aftertouch (resolved post-sweep) | 2026-03-14 | `Docs/seance_cross_reference.md` |
| OCEANIC | 7.1/10 | Zero velocity response to timbre (D001 — resolved Round 9E); Triple-BBD chorus praised but nothing dynamic | 2026-03-14 | `Docs/seance_cross_reference.md` |
| OWLFISH | 7.1/10 | Mixtur-Trautonium novel but owl_morphGlide dead (fixed), zero LFOs (fixed) | 2026-03-14 | `Docs/seance_cross_reference.md` |
| OTTONI | 7.2/10 | Instrument choice params dead (D004 — fixed); D001 partial across 4 voices; Reverb not SR-scaled (fixed) | 2026-03-14 | `~/.claude/skills/synth-seance/knowledge/index.md` |
| OBRIX | 7.2/10 → 9.4 roadmap | No factory presets at seance time; default Sine patch gives filter nothing to sculpt; LFO ceiling 30 Hz blocks audio-rate crossover | 2026-03-19 | `Docs/seances/obrix_seance_verdict.md` |
| OLE | 7.0/10 | isHusband regression post-SP7.5 fix; 4 dead params; Alliance system blessed (B019) | 2026-03-14 | `~/.claude/skills/synth-seance/knowledge/index.md` |
| OVERDUB | 7.4/10 | Single sine LFO (weakest modulation in fleet); D002 partial; D006: no MIDI CC (resolved) | 2026-03-14 | `Docs/seance_cross_reference.md` |
| ODDFELIX | ~C+ (≈7.0 est.) | snap_macroDepth void-cast (fixed); zero LFOs (fixed); preset schema drift | 2026-03-14 | `Docs/seance_cross_reference.md` (graded ~C+ avg) |
| OHM | 7.6/10 | Mono voice summing — Dad voices not stereo-spread by instrument; D001 half-honored (intensity, not brightness) | 2026-03-14 | `~/.claude/skills/synth-seance/knowledge/index.md` |
| ODYSSEY | 7.6/10 | Climax never demonstrated in presets; crossFmDepth dead (fixed); AfterTouch/ModWheel never fed live MIDI (resolved) | 2026-03-14 | `Docs/seance_cross_reference.md` |
| OVERWORLD | 7.6/10 | ERA triangle original but no expression; mono output; no LFO in adapter (all resolved post-sweep) | 2026-03-14 | `Docs/seance_cross_reference.md` |
| OCEANDEEP | 7.8/10 | No independent filter ADSR (P0 concern — major gap for bass programming); missing pitch bend | 2026-03-20 | `Docs/seances/oceandeep_seance_verdict.md`; `scripture/seances/oceandeep-seance-2026-03-20.md` |
| OMBRE | 7.8/10 | D002 partial — only 1 LFO (2 required); post-SP7.5 fix added LFO2 and reached 8.0 est. | 2026-03-19 | `Docs/seance_ombre_orca_octopus.md`; `Docs/seances/ombre_seance_verdict.md` (8.0 updated) |
| OBBLIGATO | 7.8/10 | FX chain routing misrouted (V2 backlog); D001 Constellation-wide pattern: intensity not brightness | 2026-03-14 | `~/.claude/skills/synth-seance/knowledge/index.md` |
| OUTWIT | 7.9/10 (latest) | Pitch wheel unhandled — no host BPM integration for stepSync/stepDiv; mono Den reverb collapses stereo field; step rate ceiling 40 Hz prevents audio-rate CA territory | 2026-03-20 | `Docs/seances/outwit_seance_verdict.md` (re-seance) |
| OSTINATO | 8.0/10 (initial) → 8.7 (re-seance) | No user-controllable LFO rate/depth/shape (hardcoded breathing LFO only); mono Schroeder reverb collapses stereo drum circle | 2026-03-19 / 2026-03-20 | `Docs/seances/ostinato_seance_verdict.md`; `scripture/seances/ostinato-seance-2026-03-20.md` |
| ORPHICA | 8.0/10 | kBufSize 186ms too short for slow-attack granular textures; D001 half-honored | 2026-03-14 | `~/.claude/skills/synth-seance/knowledge/index.md` |
| OPENSKY | 8.1/10 | sky_subWave parameter not fully dispatched (potential D004 partial); mod matrix placeholder unimplemented | 2026-03-20 | `Docs/seances/opensky_seance_verdict.md`; `scripture/seances/opensky-seance-2026-03-20.md` |
| ORGANISM | 8.1/10 (Docs verdict) → 7.2 (scripture re-seance) | CA parameter jumps 3200 Hz filter cutoff in one sample — audible clicks (CRITICAL DSP, patched 2026-03-20); LCG seed space only 16 bits | 2026-03-19 / 2026-03-20 | `Docs/seances/organism_seance_verdict.md`; `scripture/seances/organism-seance-2026-03-20.md` |
| OVERTONE | 8.1/10 (Docs verdict) → 7.6 (scripture re-seance) | Pi table has collapsed spectral spread at low depth (entries 0-5 all clustered near 1.0); no anti-aliasing fadeout near Nyquist; no filter envelope; declared as 8-voice but implements 1 | 2026-03-19 / 2026-03-20 | `Docs/seances/overtone_seance_verdict.md`; `scripture/seances/overtone-seance-2026-03-20.md` |
| ORCA | 8.1/10 (initial) → 8.6 est. (post-fix) | No aftertouch (D006 FAIL — resolved in commit 2035aa0); echolocation LFO2 default depth 0.0 | 2026-03-19 | `Docs/seance_ombre_orca_octopus.md`; `Docs/seances/orca_seance_verdict.md` (updated: 8.6) |
| OBSIDIAN (recovered) | 8.2 est. | Post Round 9A recovery from 6.6; formant LFO (0.1Hz) and velocity→PD depth added | 2026-03-14 | `Docs/seance_cross_reference.md` Round 9A note |
| ORBWEAVE | 8.4/10 | All 4 strands share single waveform — individual strand waveform selection is the primary V2 enhancement; FDN integer delay lengths produce audible pitch stepping when sweeping delayBase | 2026-03-19 | `Docs/seances/orbweave_seance_verdict.md` |
| OVERLAP | 8.4/10 (latest) | Global filter envelope fights FDN long tails in 6-voice poly (VIS-OVERLAP-001); FDN delay cap 50ms limits sub-bass resonance; integer delay lengths cause pitch stepping | 2026-03-20 | `Docs/seances/overlap_seance_verdict.md` (re-seance) |
| OUIE | 8.5/10 | CURRENT macro is chorus-only (no reverb/delay); unimplemented harmonic lock feature in LOVE section; unison voices 2-4 fall back to generic waveforms | 2026-03-20 | `Docs/seances/ouie_seance_verdict.md`; `scripture/seances/ouie-seance-2026-03-20.md` |
| OUTWIT (first seance) | 8.5/10 → 8.7 revised | See latest seance above | 2026-03-19 | Superseded by 2026-03-20 re-seance |
| OCTOPUS | 8.3/10 | No aftertouch (D006 FAIL — resolved commit c261a81); chromaFilter morph approximation (LP-mix, not true multi-mode SVF) | 2026-03-19 | `Docs/seance_ombre_orca_octopus.md`; `Docs/seances/octopus_seance_verdict.md` |
| OSTINATO (re-seance) | 8.7/10 | Humanization deterministic (fastSin hash repeats over long loops); Schroeder reverb mono-collapses stereo drum circle | 2026-03-20 | `Docs/seances/ostinato_seance_verdict.md`; `scripture/seances/ostinato-seance-2026-03-20.md` |
| ORACLE | 8.6/10 | Zero presets at initial seance (resolved); Buchla gave 10/10 | 2026-03-14 | `Docs/oracle_synthesis_guide.md`; `Docs/seance_cross_reference.md` |
| ORCA (post-fix) | 8.6 est. | D001 FAIL + D006 PARTIAL + AudioToRing resolved commit 2035aa0 | 2026-03-19 | `Docs/seances/orca_seance_verdict.md` |
| OUTWIT (re-seance) | 8.7/10 | Step rate ceiling 40 Hz (audio-rate CA territory blocked); pitch wheel unhandled; mono Den reverb | 2026-03-20 | `Docs/seances/outwit_seance_verdict.md` |
| OSTINATO (latest) | 8.7/10 | See OSTINATO re-seance above | 2026-03-20 | `Docs/seances/ostinato_seance_verdict.md` |

### Non-Numeric Verdict Engines (all approved, no score assigned)

| Engine | Verdict Label | Key Finding | Seance Date | Source |
|--------|--------------|-------------|-------------|--------|
| OBSCURA | High / unanimous | "The physics IS the synthesis — this is the only engine where the algorithm is the character." | 2026-03-14 | `Docs/seance_cross_reference.md` |
| OPTIC | Revolutionary | Zero-audio paradigm is 10 years ahead of industry; Blessing B005 | 2026-03-14 | `Docs/seance_cross_reference.md` |
| ORBITAL | APPROVED | Group Envelope System (Blessing B001) — "crown jewel" | 2026-03-14 | `Docs/seance_cross_reference.md` |
| ORGANON | 8/8 PASS | VFE metabolism publishable as academic paper; Blessing B011 (unanimous) | 2026-03-14 | `Docs/seance_cross_reference.md` |
| OVERBITE | Full approval | Best macro system in fleet (Blessing B008); dual-wavefolder praised | 2026-03-14 | `Docs/seance_cross_reference.md` |
| OSPREY | APPROVE/CONDITIONAL | ShoreSystem masterwork (Blessing B012); dead LFO code (fixed Round 3B) | 2026-03-14 | `Docs/seance_cross_reference.md` |
| OSTERIA | Production-grade | ShoreSystem shared (Blessing B012); warmth filter L-only P0 bug (fixed Round 3A) | 2026-03-14 | `Docs/seance_cross_reference.md` |
| ONSET | Ahead of industry | XVC 3-5 years ahead of commercial drums (Blessing B002 + B006); Buchla unanimous | 2026-03-14 | `Docs/seance_cross_reference.md` |
| OUROBOROS | Production-ready | Most scientifically rigorous engine; Lorenz/Rossler/Chua/Aizawa all cited | 2026-03-14 | `Docs/seance_cross_reference.md` |
| OPAL | Concept reviewed | Coupling is crown jewel (V008); opal_smear dead (fixed); Time-Telescope vision | 2026-03-14 | `Docs/seance_cross_reference.md` |
| ORIGAMI | Not formally scored | STFT correct; race condition P0 bug for blockSize < 512 (fixed Round 3A); instantaneousFreq never used | 2026-03-14 | `Docs/seance_cross_reference.md` |

---

## Section 1b: Missing From This Audit — OXBOW and OWARE

OXBOW (engine 43) and OWARE (engine 44) were both registered 2026-03-20, the same date as this audit.
Neither appears in the fleet table above. Seance status:

| Engine | Seance Status | Notes |
|--------|--------------|-------|
| OXBOW | **NOT SEANCED** (as of 2026-03-20) | Entangled reverb synth; 150 presets; seance pending |
| OWARE | **NOT SEANCED** (as of 2026-03-20) | Tuned percussion; 22 params; see `Docs/seances/oware-seance-2026-03-21.md` for 2026-03-21 verdict |

---

## Section 2: Tiered Score Rankings

### TIER 1 — CRITICAL (below 7.0 or problem-class)

| Rank | Engine | Score | Primary Failure | Status |
|------|--------|-------|-----------------|--------|
| 1 | OBLIQUE | 5.9/10 (→~7.2 est.) | No LFOs, dead params, 6 presets | Recovered (Round 8A) — estimated 7.2 post-recovery |
| 2 | OCELOT | 6.4/10 | macro_1–4 all dead | Partially recovered (macros wired, Deep Recovery done) |
| 3 | OBESE | 6.6/10 | Zero LFOs, no CC, Mojo axis inert | Resolved post-sweep |
| 4 | OBSIDIAN | 6.6/10 (→8.2 est.) | R-channel filter bypass P0; formant ID collision P0 | Recovered (Round 9A) — estimated 8.2 post-recovery |
| 5 | ODDOSCAR | 6.9/10 | Zero LFOs, no aftertouch | Resolved post-sweep |
| 6 | OLE | 7.0/10 | isHusband regression; 4 dead params | Post-SP7.5 fixes applied; regression may persist |

> NOTE: All CRITICAL engines have had doctrine fixes applied via Prism Sweep Rounds 3–12 and SP7.5 QA. The original seance scores reflect pre-fix state. Estimated post-recovery scores are noted where documented.

---

### TIER 2 — NEEDS WORK (7.0–7.9)

| Rank | Engine | Score | Primary Weakness | Seance Date |
|------|--------|-------|------------------|-------------|
| 1 | ODDFELIX | ~C+ (≈7.0) | snap_macroDepth void-cast (fixed); zero LFOs (fixed) | 2026-03-14 |
| 2 | OCEANIC | 7.1/10 | Zero velocity response (resolved); BBD chorus praised but static | 2026-03-14 |
| 3 | OWLFISH | 7.1/10 | Mixtur-Trautonium novel; zero LFOs (fixed); dead morphGlide (fixed) | 2026-03-14 |
| 4 | OTTONI | 7.2/10 | Instrument choice params dead (fixed); D001 partial (fixed); reverb SR-scaling (fixed) | 2026-03-14 |
| 5 | OBRIX | 7.2/10 → 9.4 roadmap | No presets; default Sine source; LFO ceiling 30 Hz | 2026-03-19 |
| 6 | OBLIQUE (post-recovery) | ~7.2 est. | D002 partial (no user LFO rate exposed); prism color LFO fixed at 0.2 Hz | 2026-03-14 |
| 7 | OVERDUB | 7.4/10 | Single sine LFO only; D002 partial; missing MIDI CC (resolved) | 2026-03-14 |
| 8 | OHM | 7.6/10 | Mono voice summing; D001 half-honored (intensity not brightness); SIDES has no LFO | 2026-03-14 |
| 9 | ODYSSEY | 7.6/10 | Climax never demoed in presets; dead mod sources (resolved) | 2026-03-14 |
| 10 | OVERWORLD | 7.6/10 | ERA triangle original; no expression (resolved); mono output | 2026-03-14 |
| 11 | OVERTONE | 7.6/10 (re-seance) | Pi table spectral collapse at low depth; no anti-aliasing near Nyquist (patched 2026-03-20); 1-voice implementation of 8-voice declaration | 2026-03-20 |
| 12 | OCEANDEEP | 7.8/10 | No independent filter ADSR; missing pitch bend | 2026-03-20 |
| 13 | OMBRE | 7.8/10 → 8.0 est. | D002 partial (1 LFO only; updated to 8.0 after SP fix) | 2026-03-19 |
| 14 | OBBLIGATO | 7.8/10 | FX chain routing misrouted; D001 intensity not brightness | 2026-03-14 |
| 15 | OUTWIT | 7.9/10 (latest) | Pitch wheel unhandled; mono Den reverb; step rate ceiling 40 Hz | 2026-03-20 |

---

### TIER 3 — ACCEPTABLE (8.0–8.4)

| Rank | Engine | Score | Primary Weakness | Seance Date |
|------|--------|-------|------------------|-------------|
| 1 | OSTINATO | 8.0/10 (initial) → 8.7 (re-seance) | No user LFO module; mono reverb collapses stereo drum circle | 2026-03-19 / 2026-03-20 |
| 2 | ORPHICA | 8.0/10 | kBufSize 186ms too short for slow-attack granular; D001 partial | 2026-03-14 |
| 3 | OPENSKY | 8.1/10 | sky_subWave not fully dispatched (D004 partial); mod matrix unimplemented | 2026-03-20 |
| 4 | ORGANISM | 8.1/10 (Docs) → 7.2 (re-seance) | CA filter cutoff jumps without smoothing — audible clicks (patched 2026-03-20); LCG seed degenerate states | 2026-03-19 / 2026-03-20 |
| 5 | ORCA | 8.1/10 (initial) → 8.6 est. | No aftertouch D006 FAIL (fixed commit 2035aa0); LFO2 default depth 0.0 | 2026-03-19 |
| 6 | OBSIDIAN (post-recovery) | 8.2 est. | Post-recovery estimate; formant LFO breathing added | 2026-03-14 post-sweep |
| 7 | OCTOPUS | 8.3/10 | No aftertouch D006 FAIL (fixed commit c261a81); chromaFilter morph is LP-mix approximation | 2026-03-19 |
| 8 | ORBWEAVE | 8.4/10 | All 4 strands share waveform; FDN integer delay lengths cause pitch stepping on sweeps | 2026-03-19 |
| 9 | OVERLAP | 8.4/10 (latest) | Global filter env fights FDN long tails; FDN delay cap 50ms limits sub-bass; integer delay pitch stepping | 2026-03-20 |

---

### TIER 4 — GOOD (8.5–8.9)

| Rank | Engine | Score | Primary Weakness | Seance Date |
|------|--------|-------|------------------|-------------|
| 1 | OUIE | 8.5/10 | CURRENT macro chorus-only; harmonic lock in LOVE section unimplemented; unison voices fallback to generic waveforms | 2026-03-20 |
| 2 | ORACLE | 8.6/10 | Zero presets at initial seance (resolved); spectral territory wide but preset library must demonstrate it | 2026-03-14 |
| 3 | ORCA (post-fix) | 8.6 est. | LFO2 default depth 0.0; chromaFilter morph (resolved separately) | 2026-03-19 |
| 4 | OUTWIT (re-seance) | 8.7/10 | Step rate ceiling 40 Hz blocks audio-rate CA synthesis; pitch wheel unhandled | 2026-03-20 |
| 5 | OSTINATO (re-seance) | 8.7/10 | Humanization deterministic (fastSin hash repeats); mono Schroeder reverb | 2026-03-20 |

---

### TIER 5 — EXCELLENT (9.0+) and Non-Numeric Approvals

No engine has yet achieved a formal ghost council score of 9.0 or above. The closest are OUTWIT and OSTINATO at 8.7/10.

The following engines received approval-class verdicts without numeric scoring. Based on ghost testimony and post-sweep doctrine compliance, these are treated as high-quality engines:

| Engine | Verdict | Approximate Tier | Key Excellence |
|--------|---------|-----------------|----------------|
| ORGANON | 8/8 PASS | EXCELLENT (9.0+ equivalent) | VFE metabolism — publishable as academic paper; B011 unanimous |
| ORACLE | 8.6/10 | GOOD | GENDY stochastic + Maqam; Buchla gave 10/10 for architecture |
| OBSCURA | High / unanimous | EXCELLENT (estimated) | Physics IS synthesis; D003 fully compliant; unanimous praise |
| OUROBOROS | Production-ready | GOOD-EXCELLENT | Most scientifically rigorous; Lorenz/Rossler/Chua cited; B003 + B007 |
| OVERBITE | Full approval | GOOD-EXCELLENT | Best macro system in fleet (B008); all 8 ghosts approved |
| ONSET | Ahead of industry | EXCELLENT | XVC is 3-5 years ahead of commercial drums; B002 + B006 |
| OPTIC | Revolutionary | EXCELLENT | Zero-audio synthesis is a paradigm invention; B005 |
| OSPREY | APPROVE/CONDITIONAL | ACCEPTABLE-GOOD | ShoreSystem masterwork (B012); conditional on LFO (fixed) |
| OSTERIA | Production-grade | ACCEPTABLE-GOOD | ShoreSystem (B012); warmth P0 fixed; solid foundation |
| ORBITAL | APPROVED | ACCEPTABLE-GOOD | Group Envelope B001; D005 fixed post-sweep |
| OPAL | Concept reviewed | NEEDS WORK | opal_smear dead (fixed); crown jewel coupling concept (V008) |
| ORIGAMI | Not formally scored | UNSCORED | Race condition P0 (fixed); correct STFT implementation |

---

## Section 3: Engine-by-Engine Detail (All 42)

### Engines with formal numeric ghost council scores

| # | Engine | Final Score | Ghost Min | Ghost Max | Top Ghost Concern | File |
|---|--------|-------------|-----------|-----------|-------------------|------|
| 1 | OBRIX | 7.2/10 | Kakehashi 7/8 (implied) | Schulze praised (8.5 est.) | No presets; default Sine; LFO ceiling 30 Hz | `Docs/seances/obrix_seance_verdict.md` |
| 2 | OCEANDEEP | 7.8/10 | Buchla/Smith 7/10 | Vangelis 9/10 | No filter ADSR; missing pitch bend | `Docs/seances/oceandeep_seance_verdict.md` |
| 3 | OPENSKY | 8.1/10 | Smith 7/10 | Pearlman 8.5/10 | sky_subWave D004 partial; shimmer/chorus mutual exclusion debate | `Docs/seances/opensky_seance_verdict.md` |
| 4 | ORBWEAVE | 8.4/10 | — | — | Uniform strand waveform; integer FDN delay pitch stepping | `Docs/seances/orbweave_seance_verdict.md` |
| 5 | ORGANISM | 8.1 (Docs) / 7.2 (re-seance) | — | — | CA parameter jumps without smoothing (CRITICAL — patched 2026-03-20) | `Docs/seances/organism_seance_verdict.md`; `scripture/seances/organism-seance-2026-03-20.md` |
| 6 | OSTINATO | 8.7/10 (latest) | Moog 9/10 | Buchla 9/10 | Humanization deterministic; mono reverb; hardcoded breath LFO | `Docs/seances/ostinato_seance_verdict.md` |
| 7 | OUIE | 8.5/10 | Moog 7.5/10 | Buchla 9/10 | CURRENT macro chorus-only; LOVE harmonic lock unimplemented | `Docs/seances/ouie_seance_verdict.md` |
| 8 | OUTWIT | 8.7/10 (revised) | Kakehashi 7.0/10 | Buchla 10.0/10 | Pitch wheel unhandled; step rate ceiling; mono reverb | `Docs/seances/outwit_seance_verdict.md` |
| 9 | OVERLAP | 8.4/10 (revised) | Vangelis 7.5/10 | Buchla 9.5/10 | Global filter env vs. FDN tails; delay cap 50ms; integer pitch stepping | `Docs/seances/overlap_seance_verdict.md` |
| 10 | OVERTONE | 8.1 (Docs) / 7.6 (re-seance) | — | — | Pi table spectral collapse; no anti-alias Nyquist fadeout (patched 2026-03-20); 1-voice only | `Docs/seances/overtone_seance_verdict.md`; `scripture/seances/overtone-seance-2026-03-20.md` |
| 11 | OMBRE | 7.8/10 → 8.0 est. | — | — | D002 partial: 1 LFO (2 required) | `Docs/seances/ombre_seance_verdict.md` |
| 12 | ORCA | 8.1/10 → 8.6 est. | — | — | D006 aftertouch absent (fixed commit 2035aa0) | `Docs/seances/orca_seance_verdict.md` |
| 13 | OCTOPUS | 8.3/10 | — | — | D006 aftertouch absent (fixed commit c261a81); chromaFilter morph LP-mix only | `Docs/seances/octopus_seance_verdict.md` |
| 14 | OHM | 7.6/10 | — | — | Mono voice summing; D001 partial | Seance #25, `~/.claude/skills/synth-seance/knowledge/index.md` |
| 15 | ORPHICA | 8.0/10 | — | — | kBufSize 186ms; D001 partial | Seance #26, index.md |
| 16 | OBBLIGATO | 7.8/10 | — | — | FX chains misrouted; D001 Constellation-wide pattern | Seance #27, index.md |
| 17 | OTTONI | 7.2/10 | — | — | Instrument choice params dead (fixed); D001 partial (fixed) | Seance #28, index.md |
| 18 | OLE | 7.0/10 | — | — | isHusband regression; 4 dead params | Seance #29, index.md |
| 19 | ORACLE | 8.6/10 | Buchla 10/10 | — | Zero presets (resolved); wide spectral territory | Seance #21, `Docs/oracle_synthesis_guide.md` |
| 20 | OVERDUB | 7.4/10 | — | — | Single sine LFO; D002 partial | Seance #7, index.md |
| 21 | OCEANIC | 7.1/10 | — | — | Zero velocity timbral response (resolved Round 9E) | Seance #10, index.md |
| 22 | ODYSSEY | 7.6/10 | — | — | Climax never demoed; dead mod sources (resolved) | Seance #11, index.md |
| 23 | OVERWORLD | 7.6/10 | — | — | No expression; ERA triangle stranded (resolved) | Seance #12, index.md |
| 24 | OWLFISH | 7.1/10 | — | — | Mixtur-Trautonium novel but zero LFOs (fixed) | Seance #14, index.md |
| 25 | OBESE | 6.6/10 | — | — | Zero LFOs (fixed); Mojo axis un-modulated (partially resolved) | Seance #15, index.md |
| 26 | OCELOT | 6.4/10 | — | — | macro_1–4 all dead (fixed Round 3B) | Seance #3, index.md |
| 27 | OBLIQUE | 5.9/10 → ~7.2 est. | — | — | Dead params; 6 presets; zero LFOs (all resolved Rounds 8A) | Seance #19, index.md |
| 28 | OBSIDIAN | 6.6/10 → 8.2 est. | — | — | R-channel P0 + formant ID P0 (fixed); velocity amplitude only (fixed) | Seance #20, index.md |
| 29 | ODDFELIX | ~C+ ≈7.0 | — | — | snap_macroDepth void-cast (fixed); zero LFOs (fixed) | Seance #17, index.md |
| 30 | ODDOSCAR | 6.9/10 | — | — | Zero LFOs (fixed); no aftertouch (fixed) | Seance #18, index.md |
| 31 | OBLONG | 7.x/10 | — | — | CuriosityEngine not touch-responsive | Seance #1, index.md |

---

## Section 4: Engines with No Seance (NONE)

Every engine in the 42-engine fleet has been through at least one seance. There are NO unrated engines.

The seance knowledge tree (`~/.claude/skills/synth-seance/knowledge/index.md`) records 38 seances held across 35 engines (some engines re-seanced 2-3 times). The 7 engines with non-numeric verdicts (OBSCURA, OPTIC, ORBITAL, ORGANON, OVERBITE, OSPREY, OSTERIA, ONSET, OUROBOROS, OPAL, ORIGAMI) all received substantive ghost council assessments — they were approved without a competitive numerical scoring context because they earned unanimous or near-unanimous approval from the council.

---

## Section 5: Priority Re-Seance Queue

Based on significant changes since last seance, these engines should be prioritized for re-seance:

| Priority | Engine | Reason | Last Score |
|----------|--------|---------|------------|
| P0 | ORGANISM | Pi table fix + PolyBLEP patch applied 2026-03-20 — score will have changed significantly from 7.2 | 7.2 (scripture re-seance) |
| P0 | OVERTONE | Pi table + Nyquist fadeout patched 2026-03-20 — projected score 9.3+ if fixes hold | 7.6 (scripture re-seance) |
| P1 | OBRIX | Wave 3 DSP complete (65 params); 150 presets in progress; score should be reassessed | 7.2 |
| P1 | ORCA | Post-fix estimate 8.6 but no formal re-seance held | 8.1/8.6 est. |
| P1 | OMBRE | Post-SP7.5 fix estimate 8.0 but initial seance was pre-fix | 7.8/8.0 est. |
| P2 | OCTOPUS | Aftertouch fix applied (commit c261a81); LFO2 AudioToRing fixed — score should improve | 8.3 |
| P2 | OLE | isHusband regression impact unclear; needs verification | 7.0 |
| P3 | OPENSKY | sky_subWave D004 partial was flagged; fix would be straightforward | 8.1 |
| P3 | OSTINATO | Humanization determinism is the remaining fixable concern | 8.7 |
| P3 | ORPHICA | kBufSize short-buffer limitation could be addressed | 8.0 |

> Seven non-numeric "approval-class" engines (OBSCURA, OPTIC, ORGANON, OUROBOROS, ONSET, OVERBITE, ORBITAL) are strong candidates for formal numeric ghost council seances in V1.1+ for cross-engine ranking completeness.

---

## Section 6: Fleet-Wide Doctrine Status (2026-03-20)

| Doctrine | Fleet Status | Remaining Gaps |
|----------|-------------|----------------|
| D001 — Velocity shapes timbre | RESOLVED fleet-wide (Round 9E + later) | Some engines have "intensity not brightness" half-compliance (OHM, ORPHICA, OBBLIGATO) |
| D002 — 2+ LFOs + mod matrix | SUBSTANTIALLY RESOLVED | OSTINATO hardcoded LFO only; OBRIX Wave 4 pending |
| D003 — Physics rigor | N/A to most engines | OCEANDEEP D003 PARTIAL; OBSCURA/OUROBOROS/ORACLE full compliance |
| D004 — No dead params | RESOLVED (Rounds 3B, 7D, SP7.5) | OPENSKY sky_subWave potential partial; OUTWIT stepSync dead (host transport gap) |
| D005 — Engines must breathe | RESOLVED (Rounds 5A, 8A, 8B, SP7.5) | All engines confirmed autonomous modulation ≤ 0.01 Hz floor |
| D006 — Expression required | SUBSTANTIALLY RESOLVED | 22/22 MIDI-capable engines have mod wheel; 23/23 have aftertouch (Optic exempt) |

---

## Section 7: Score Distribution

```
Score Range  | Count | Engines
-------------|-------|--------
9.0+         |   0   | (none scored above 8.7 yet)
8.5–8.9      |   5   | OUTWIT(8.7), OSTINATO(8.7), ORACLE(8.6), ORCA-est(8.6), OUIE(8.5)
8.0–8.4      |   9   | OVERLAP(8.4), ORBWEAVE(8.4), OCTOPUS(8.3), OBSIDIAN-est(8.2), OPENSKY(8.1), ORGANISM(8.1/7.2), OVERTONE(8.1/7.6), ORCA-init(8.1), ORPHICA(8.0), OSTINATO-init(8.0), OMBRE-est(8.0)
7.0–7.9      |  15   | OCEANDEEP(7.8), OMBRE(7.8), OBBLIGATO(7.8), OUTWIT-latest(7.9), OVERWORLD(7.6), ODYSSEY(7.6), OHM(7.6), OVERTONE-re(7.6), OBELONG(7.x), OTTONI(7.2), OBRIX(7.2), OBLIQUE-est(7.2), OVERDUB(7.4), OCEANIC(7.1), OWLFISH(7.1)
Below 7.0    |   6   | ODDOSCAR(6.9), OBESE(6.6), OBSIDIAN-orig(6.6), OCELOT(6.4), ODDFELIX(~7.0 est.), OBLIQUE-orig(5.9)
Non-numeric  |  11   | ORGANON, ORACLE(num), OUROBOROS, OVERBITE, ONSET, OPTIC, OBSCURA, OSPREY, OSTERIA, ORBITAL, OPAL, ORIGAMI
```

**Fleet median score (numeric engines, post-recovery estimates):** ~7.8/10
**Fleet mean score (numeric engines only, no estimates):** ~7.7/10
**Highest confirmed score:** OUTWIT 8.7/10, OSTINATO 8.7/10
**Lowest confirmed score:** OBLIQUE 5.9/10 (pre-recovery); OCELOT 6.4/10 (post-fix)

---

## Section 8: Critical Open Issues by Engine

| Engine | Critical Open Issue | Impact |
|--------|--------------------|----|
| ORGANISM | CA parameter smoothing patched 2026-03-20 — verify no regression | Audio quality |
| OVERTONE | Pi table + Nyquist fadeout patched 2026-03-20 — verify score improvement | DSP correctness |
| OBRIX | 0 presets at seance; 150 in progress (Wave 4) — engine invisible without demonstration | Product readiness |
| OCEANDEEP | No filter ADSR — classic bass "pluck" impossible; no pitch bend | Expressive range |
| OUTWIT | Pitch wheel unhandled (VIS-OUTWIT-001); host transport not wired (stepSync dead) | Performance + D004 |
| OVERLAP | Global filter env vs. FDN tails (VIS-OVERLAP-001); integer delay pitch stepping | Audio quality |
| OUIE | LOVE harmonic lock unimplemented; CURRENT macro chorus-only | Feature completeness |
| OLE | isHusband regression status unclear post-SP7.5 | Correctness |
| OPENSKY | sky_subWave parameter not dispatched — D004 partial | Dead parameter |
| OBBLIGATO | FX chain routing misrouted — V2 backlog | Audio routing |

---

*Audit complete. 42 engines surveyed (of 44 total — OXBOW + OWARE were added the same day and are not included). 0 of the 42 surveyed engines had no seance. Data sourced from 19 verdict/seance files across 2 directories + seance knowledge tree.*

*Next recommended action: Re-seance ORGANISM and OVERTONE with their 2026-03-20 patches applied — both had critical DSP issues identified and patched in the same session as this audit.*
