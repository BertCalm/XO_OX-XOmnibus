# Preset Schema Fixes — Round 4

**Date:** 2026-03-14
**Scope:** Three schema failures identified in the Round 3C audit.

---

## Fix 1: XOverworld — UPPER_SNAKE_CASE key drift (P0)

**Problem:** Overworld presets used UPPER_SNAKE_CASE parameter keys
(e.g. `ow_ERA`, `ow_PULSE_DUTY`, `ow_FM_ALGORITHM`, `ow_CRUSH_BITS`) but the
engine registers camelCase IDs (e.g. `ow_era`, `ow_pulseDuty`, `ow_fmAlgorithm`,
`ow_crushBits`). Every Overworld parameter in every affected preset was silently
ignored at load time.

**Root cause:** Some older presets in `XOverworld/Presets/Factory/` and the
XOceanus `Thunderforce.xometa` were written against a draft naming convention
before the Parameters.h constants were finalized.

**Fix:** `Tools/fix_overworld_presets.py` — builds a 76-entry UPPER_SNAKE →
camelCase map derived from `XOverworld/src/engine/Parameters.h` and rewrites
every affected `.xometa` file in place. Handles both flat and multi-engine
nested `"parameters"` objects.

**Result:**
- Files fixed: **40** (39 Factory presets + `Thunderforce.xometa` in XOceanus)
- Keys renamed per file: **76**
- Total key renames: **3040**

---

## Fix 2: Ocelot — `ocelot_sampleRateRed` typo (15 presets)

**Problem:** 15 presets used the C++ variable name `sampleRateRed` as the JSON
key instead of the actual registered string `"ocelot_sampleRate"`. The variable
is declared in `OcelotParameters.h` as:

```cpp
constexpr const char* sampleRateRed = "ocelot_sampleRate";
```

The value was correct; the key was wrong.

**Fix:** Python string replacement across all `.xometa` files under
`Presets/XOceanus/`. Each occurrence of `"ocelot_sampleRateRed"` was replaced
with `"ocelot_sampleRate"`.

**Affected presets:**
Canopy Rain, Prowling Groove, Jungle Pulse (Flux), Night Hunt, Forest Floor,
Tropicalia (Entangled), Snow Silence, Ice Chimes (Prism), Crystal Cave,
Frozen Wood (Foundation), Coral Drift, Bioluminescent (Aether), Deep Current,
Submerged, Abyss (Atmosphere).

**Result:**
- Files fixed: **15**
- Key occurrences renamed: **15**

---

## Fix 3: Fat (XObese) — parameter name drift (2 presets)

**Problem:** Two Entangled presets referenced legacy Fat parameter names that
were renamed during the engine's development:

| Ghost name | Canonical name |
|---|---|
| `fat_filterCutoff` | `fat_fltCutoff` |
| `fat_filterReso` | `fat_fltReso` |
| `fat_attack` | `fat_ampAttack` |
| `fat_decay` | `fat_ampDecay` |
| `fat_sustain` | `fat_ampSustain` |
| `fat_release` | `fat_ampRelease` |

**Affected presets:**
- `Presets/XOceanus/Entangled/Supersize_Organism.xometa` — 6 keys renamed
- `Presets/XOceanus/Flux/Midnight_Strobe.xometa` — 2 keys renamed
  (`fat_filterCutoff`, `fat_filterReso`)

**Fix:** Python script replacing each ghost key with the canonical name in the
nested `"parameters"."Obese"` block.

**Result:**
- Files fixed: **2**
- Keys renamed: **8**

---

## Summary

| Fix | Impact | Files | Keys |
|-----|--------|-------|------|
| Fix 1 — Overworld UPPER_SNAKE | P0 — 100% preset failure | 40 | 3040 |
| Fix 2 — Ocelot sampleRateRed | P1 — 15 presets lost 1 param | 15 | 15 |
| Fix 3 — Fat ghost params | P2 — 2 presets lost 6–8 params | 2 | 8 |
| **Total** | | **57** | **3063** |

All fixes are backward-compatible. No preset values were changed, only key names.
The fix script is at `Tools/fix_overworld_presets.py` and can be re-run safely.
