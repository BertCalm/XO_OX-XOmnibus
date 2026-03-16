# XOmnibus Fleet Status Snapshots

Running history of key fleet metrics. Most recent entry first.

---

## 2026-03-16 — Wave 95

| Field | Value |
|-------|-------|
| **Date** | 2026-03-16 |
| **Fleet size** | **9,695** `.xometa` presets (+300 from Wave 94) |
| **Fleet cosine diversity score** | **0.2004** (acceptable range: 0.20–0.30) |
| **Latest commit** | `c12f635` — Wave 95: 380 aggression-XHIGH presets + sidecar wiring + gap report |
| **Status** | GREEN — 0 CRITICAL gaps; diversity score holding at floor of acceptable band |

### Per-Mood Preset Counts (Wave 95)

| Mood | Wave 94 | Wave 95 | Delta |
|------|---------|---------|-------|
| Foundation | 1,010 | 1,010 | — |
| Atmosphere | 1,039 | 1,039 | — |
| Entangled | 3,904 | 4,044 | +140 |
| Prism | 1,023 | 1,061 | +38 |
| Flux | 998 | 1,066 | +68 |
| Aether | 969 | 1,019 | +50 |
| Family | 452 | 452 | — |
| **Total** | **9,395** | **9,695** | **+300** |

### Key Changes in Wave 95

- **Aggression-XHIGH targeted fill**: 380 aggressive presets injected fleet-wide to counter the systemic gentle-skew bias; Flux aggression-XHIGH improved to 32.1% (was 28.7%); Prism aggression-XHIGH improved to 19.7% (was 18.9%); Aether aggression-XHIGH improved to 19.1% (was 17.7%)
- **Atmosphere aggression-XLOW**: Still at 50.0% — identity-stable but dominant; Atmosphere aggression-XHIGH at 15.3% (unchanged — next target)
- **Prism brightness-XHIGH**: Still 31.1% — the dark-brightness asymmetry persists; Prism XLOW improved to 19.7% (was 14.9%), partially narrowing the gap
- **Flux movement-XHIGH**: Slightly improved to 39.9% (was 40.9%); XLOW improved to 14.0% (was 9.2%) — slow/still Flux growth is working
- **Aether space-XHIGH**: Improved to 43.0% (was 45.7%) — bimodal split narrowing
- **Sidecar wiring**: Gap report toolchain updated; JSON saved to `per_mood_gap_report_wave92.json`
- **Diversity score**: 0.2004 — still within acceptable band but hugging the floor; next wave should prioritize high-variance DNA injections

### Active WATCH Flags (Wave 95)

| Priority | Mood | Dimension | Value | Note |
|----------|------|-----------|-------|------|
| WATCH | Prism | brightness XHIGH | 31.1% | Dark-Prism fill still needed |
| WATCH | Flux | movement XHIGH | 39.9% | Improving; slow-Flux fill continuing |
| WATCH | Aether | space XHIGH | 43.0% | Improving; space midrange fill needed |
| WATCH | Atmosphere | aggression XLOW | 50.0% | Identity-stable; monitor aggression-XHIGH floor |
| WATCH | Aether | aggression XLOW | 47.0% | Identity-stable |

### Next Recommended Targets

1. **Atmosphere aggression-XHIGH** — bring from 15.3% toward 20%+; targeted aggressive ambient/industrial Atmosphere presets
2. **Prism dark-brightness fill** — brightness XLOW now at 19.7% but XHIGH still at 31.1%; inject 30–50 dark Prism (heavy filter, low-pass spectral)
3. **Family volume push** — still at 452 (target: 600+ before V1); DNA is healthy, pure volume needed
4. **Diversity score buffer** — score at 0.2004 is on the floor of the acceptable band; inject high-variance DNA presets (extreme DNA combos across all 6 dims)
5. **OVERLAP + OUTWIT seances** — both installed + auval PASS; seances unlock coupling-aware presets and new engine DNA for Entangled diversity

---

## 2026-03-16

| Field | Value |
|-------|-------|
| **Date** | 2026-03-16 |
| **Fleet size** | 8,715 `.xometa` presets |
| **Fleet cosine diversity score** | 0.2194 (acceptable range: 0.20–0.30) |
| **Latest commit** | `8a7ddb4` — Wave 92: 400 ultra-diverse presets across 5 moods + AIR plugin research |
| **Tools count** | 338 Python scripts in `Tools/` |

### Key milestones reached by this date

- **31 engines** registered + installed in XOmnibus; auval PASS (2026-03-15)
- **Prism Sweep**: ALL 12 rounds complete
- **Constellation Fast Track**: ALL 5 seances complete (OHM/ORPHICA/OBBLIGATO/OTTONI/OLE) — `836e85a`
- **OVERLAP + OUTWIT**: Phase 4 complete (2026-03-15); installed + auval PASS
- **7 moods** (Foundation / Atmosphere / Entangled / Prism / Flux / Aether / Family)
- **DNA diversity**: Fleet sitting in the 0.20–0.30 acceptable band after Wave 92 push
- **Diversity snapshot** saved: `Docs/snapshots/dna_diversity_analysis.json`
- **Per-mood gap report** saved: `Docs/snapshots/per_mood_gap_report_wave92.json`

### Notes

- V1 scope expanded to include all 4 concept engines (OSTINATO, OPENSKY, OCEANDEEP, OUIE)
- XPN tools documentation written: `Docs/xpn_tools_readme.md` (338 scripts indexed)

---

## 2026-03-16 — Wave 94

| Field | Value |
|-------|-------|
| **Date** | 2026-03-16 |
| **Fleet size** | 9,395 `.xometa` presets (+819 from Wave 92) |
| **Fleet cosine diversity score** | 0.2052 (acceptable range: 0.20–0.30) |
| **Tools count** | 240+ Python scripts in `Tools/` |

### Key milestones

- **All previously critical flags RESOLVED**: Aether space-XLOW (was 4.6% → now 14.2%); Entangled movement-XLOW (was 6.8% → now 9.5%)
- **0 CRITICAL gaps** in Wave 94; 5 WATCH flags (all minor identity asymmetries)
- **Largest mood growth**: Aether +227 presets; Family +108; Entangled +204
- **New flags**: Prism brightness XHIGH (33.0%), Flux movement XHIGH (40.9%), Aether space XHIGH (45.7%)
- **Aggression-XHIGH systemic bias**: improving but still present in Atmosphere (15.3%), Aether (17.7%)
- Full detail: `Docs/snapshots/fleet_status_wave94.md`

### Notes

- V1 scope: 35 engines (31 installed + OSTINATO/OPENSKY/OCEANDEEP/OUIE pending DSP)
- OVERLAP + OUTWIT seances still pending
- Family volume target: 600+ presets before V1

---

<!-- Add future snapshots above this line -->
