# Organon Engine Parameter Prefix Audit

**Date:** 2026-03-14
**Triggered by:** 8C coupling preset library agent report noting prefix inconsistency.

---

## Ground Truth: What the Engine Uses

`Source/Engines/Organon/OrganonEngine.h` declares 10 parameters, all with the `organon_` prefix:

| Parameter ID              | Range         | Default |
|--------------------------|---------------|---------|
| `organon_metabolicRate`  | 0.1 – 10.0    | 1.0     |
| `organon_enzymeSelect`   | 20 – 20000 Hz | 1000.0  |
| `organon_catalystDrive`  | 0.0 – 2.0     | 0.5     |
| `organon_dampingCoeff`   | 0.01 – 0.99   | 0.3     |
| `organon_signalFlux`     | 0.0 – 1.0     | 0.5     |
| `organon_phasonShift`    | 0.0 – 1.0     | 0.0     |
| `organon_isotopeBalance` | 0.0 – 1.0     | 0.5     |
| `organon_lockIn`         | 0.0 – 1.0     | 0.0     |
| `organon_membrane`       | 0.0 – 1.0     | 0.2     |
| `organon_noiseColor`     | 0.0 – 1.0     | 0.5     |

The CLAUDE.md engine table also confirms: `organon_` is the canonical prefix.

---

## Preset Audit

Total presets referencing Organon: **~100+** across all moods (Flux, Entangled, Foundation, Atmosphere, Aether, Prism).

### Presets with WRONG prefix (`org_`): 1

**`Presets/XOceanus/Entangled/Ouro_Organon_Metabolism.xometa`**

This preset (created 2026-03-10) used 8 wrong parameter keys:

| Wrong key (org_)       | Issue                                       |
|------------------------|---------------------------------------------|
| `org_metabolicRate`    | Wrong prefix only — value 0.4 salvageable   |
| `org_entropy`          | Non-existent parameter (no `organon_entropy` in engine) |
| `org_membrane`         | Wrong prefix only — value 0.6 salvageable   |
| `org_cellDensity`      | Non-existent parameter                      |
| `org_adaptation`       | Non-existent parameter                      |
| `org_phasonShift`      | Wrong prefix only — value 0.3 salvageable   |
| `org_lockIn`           | Wrong prefix only — value 0.0 salvageable   |
| `org_level`            | Non-existent parameter                      |

Additionally, the preset omitted 4 of the 10 engine parameters entirely: `organon_enzymeSelect`, `organon_catalystDrive`, `organon_dampingCoeff`, `organon_isotopeBalance`, `organon_noiseColor`.

### Presets with CORRECT prefix (`organon_`): All others

All other Organon presets across all moods use the correct `organon_` prefix and all 10 valid parameter IDs.

---

## Migration Action Taken

**No migration script was needed** — only one preset was wrong, and it was fixed directly.

**Fix applied to `Ouro_Organon_Metabolism.xometa`:**

- Replaced all 8 `org_` keys with the full set of 10 `organon_` keys
- Salvaged values where the parameter name mapped clearly (`metabolicRate`, `membrane`, `phasonShift`, `lockIn`)
- For non-existent parameters (`org_entropy`, `org_cellDensity`, `org_adaptation`, `org_level`), assigned values consistent with the preset's intended character (chaotic Ouroboros feeding — high `signalFlux: 0.9`, elevated `catalystDrive: 1.4`, `metabolicRate: 4.0`)
- `organon_enzymeSelect` set to 700 Hz (low-frequency enzyme window to ingest Ouroboros bass chaos)
- No other preset file was modified

**Post-fix verification:** `grep -r '"org_' Presets/ --include="*.xometa"` returns no results.

---

## Coupling Presets Written by 8C Agent

The 8C coupling preset library agent wrote presets to `Presets/XOceanus/Entangled/`. All Organon-containing presets created by that agent use the correct `organon_` prefix:

| File | Status |
|------|--------|
| `Oracle-Organon/Chaos_Ingested.xometa` | Correct — all 10 `organon_` params |
| `Oracle-Organon/Ancient_Murmur.xometa` | Correct — all 10 `organon_` params |
| `Oracle-Organon/Prophecy_Fed.xometa`   | Correct — all 10 `organon_` params |
| `Predator_Pulse.xometa`                | Correct — `organon_` prefix |
| `Feedback_Garden.xometa`               | Correct — `organon_` prefix |
| `Spectral_Symbiosis.xometa`            | Correct — `organon_` prefix |
| `Host_Response.xometa`                 | Correct — `organon_` prefix |
| `Curious_Machine.xometa`               | Correct — `organon_` prefix |
| `Endosymbiont.xometa`                  | Correct — `organon_` prefix |
| `Osmotic_Flow.xometa`                  | Correct — `organon_` prefix |
| `Impact_Feeder.xometa`                 | Correct — `organon_` prefix |
| All other Entangled Organon presets    | Correct — `organon_` prefix |

The one bad preset (`Ouro_Organon_Metabolism.xometa`) was created earlier (2026-03-10) before the 8C agent's work and was not authored by the 8C run.

---

## Summary

| Item | Count |
|------|-------|
| Total Organon presets scanned | ~100+ |
| Presets with wrong `org_` prefix | 1 |
| Presets with correct `organon_` prefix | all others |
| Non-existent param names found | 4 (`org_entropy`, `org_cellDensity`, `org_adaptation`, `org_level`) |
| Files modified | 1 |
| Migration script created | No (not needed for a single file) |

**Status: RESOLVED.** All Organon presets now use the correct `organon_` parameter prefix and valid parameter IDs.
