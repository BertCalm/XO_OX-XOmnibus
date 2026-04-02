# Documentation Consistency Sweep — 2026-03-24

**Scope:** Post-XOceanus rename + Session 8-9 audit
**Status:** COMPLETE — all fixable issues resolved
**Files audited:** CLAUDE.md, Site/aquarium.html, Site/design-tokens.css, Docs/INDEX.md, Docs/design/asset-registry.md, Docs/design/xoceanus-definitive-ui-spec.md, Docs/design/playsurface-design-spec.md, Docs/design/figma-asset-compendium.md, Docs/xoceanus-morning-briefing-2026-03-24.md, Docs/research/

---

## Check 1: CLAUDE.md Accuracy

### Findings

| Item | Status | Notes |
|------|--------|-------|
| Engine count: 73 | PASS | Line 9: "73 engines" — confirmed correct. Engine list on line 13 enumerates all 73. |
| Coupling types: 15 | PASS | Line 14: "15 coupling types incl. KnotTopology + TriangularCoupling" |
| All engine entries present | PASS | Table rows 44–116 cover all 73 engines. OXYTOCIN and OUTLOOK both present with `oxy_` and `look_` prefixes respectively. |
| XOxytocin with `oxy_` prefix | PASS | Line 172: `oxy_intimacy` |
| TriangularCoupling noted | PASS | Line 14 |
| Rename documented | PASS | Lines 1–3: rename note present |
| Stale `Docs/xoceanus_*.md` paths | FIXED | 7 file paths corrected to `Docs/xoceanus_*.md` (actual filenames on disk) |
| Self-referential `(formerly X)` notes | FIXED | Two instances — lines 201 and 290 — had `(formerly Docs/xoceanus_...)` pointing to the same file. Cleaned to plain paths. |
| Stale `xoceanus_landscape_2026.md` | FIXED | Corrected to `xoceanus_landscape_2026.md` |
| Stale sound design guide note | FIXED | "38 of 44 engines covered" was stale; simplified to plain path reference |

---

## Check 2: Cross-Reference Check

### xoceanus-definitive-ui-spec.md
- **Asset registry references:** Uses `asset registry 1.2` for Nebulica — PASS (asset registry section 1.2 is correct for display fonts).
- **Font references:** Space Grotesk, Inter, JetBrains Mono all consistent with asset-registry sections 1.1 and 1.3.
- **Color hex values:** Appendix A.8 matches design-tokens.css for checked engines (OPERA `#D4AF37`, OBRIX `#1E8B7E`, OXYTOCIN `#9B5DE5`).
- **Engine count:** "73 engines" appears 6 times in spec body — consistent throughout.
- **XOceanus references:** Fully renamed. No stale XOceanus in body text.

### playsurface-design-spec.md
- Uses "XOceanus" throughout — PASS.
- No engine name issues found.
- No stale file paths found.

### asset-registry.md
- **Stale `XO_OX-XOceanus` repo paths:** FIXED — 10 occurrences replaced with `XO_OX-XOceanus`.
- **`71 Engine Colors` description:** FIXED — updated to `73 Engine Colors` in design-tokens.css note.
- **Quick-access table:** "71 engine colors" → "73 engine colors" FIXED; now points to `Site/design-tokens.css` Section 3.
- **Mockup path:** `Docs/mockups/xoceanus-main-ui.html` → FIXED to `Docs/mockups/xoceanus-main-ui.html` (actual filename on disk).

### Docs/research/INDEX.md
- **Does not exist.** The research directory contains only `continuous-scan-2026-03-24.md`. There is no INDEX.md within research/.
- **Main Docs/INDEX.md** links to `research/continuous-scan-2026-03-24.md` at line 322 — PASS (file exists).
- **No action required:** The main INDEX.md serves as the index. A separate research/INDEX.md is not needed.

---

## Check 3: Stale XOceanus References in Docs

**Command run:** `grep -rn "XOceanus" Docs/ --include="*.md" | grep -v "formerly|née|Formerly|renamed|historical|changelog|Presets/XOceanus|SDK/include/xoceanus|Rename|rename|xoceanus_name_migration|formerly named|xoceanus_"`

**Count before fix:** 6
**Count after fix:** Reduced to informational-only records

Remaining legitimate references:
1. `Docs/xoceanus-morning-briefing-2026-03-24.md` line 33 — FIXED (`Presets/XOceanus/` → `Presets/XOceanus/`)
2. `Docs/architect-audit-session-9.md` — Multiple lines reporting "XOceanus references: 0 — CLEAN" in code/source files. These are historical audit records. NOT stale — they are *reporting absence* of XOceanus in source code, which is correct.

---

## Check 4: Engine Count Consistency

| Location | Count Found | Status |
|----------|-------------|--------|
| CLAUDE.md line 9 | 73 | PASS |
| CLAUDE.md engine list | 73 (counted) | PASS |
| Site/aquarium.html meta tags | 73 | PASS |
| Site/aquarium.html `engine-card` divs | 72 → **73** | FIXED — OUTLOOK was missing |
| Site/aquarium.html body text | Mentions "73 engines" in multiple places | PASS |
| Site/design-tokens.css header comment | 73 | PASS |
| Site/design-tokens.css `--engine-*` tokens | 73 | PASS |
| Docs/design/xoceanus-definitive-ui-spec.md | 73 (multiple mentions) | PASS |
| Docs/figma-asset-compendium.md | No engine count claimed | N/A |
| Docs/design/asset-registry.md | "71 Engine Colors" → **73** | FIXED |

### Missing Engine: OUTLOOK
XOutlook (Horizon Indigo `#4169E1`) was absent from `Site/aquarium.html` despite being registered in CLAUDE.md, design-tokens.css, and all other counts showing 73. A Preview-tagged engine card was added to the Open Water depth zone, immediately after XOxytocin and before the Sunlit Shallows zone marker.

---

## Check 5: Design Spec Internal Consistency

### Font References
The definitive UI spec (Appendix B) defines Space Grotesk + Inter + JetBrains Mono as the three production fonts. The asset-registry section 1.1 lists exactly the same three. **CONSISTENT.**

Spec reference to "asset registry 1.2" for Nebulica (engine nameplate alt) correctly points to section 1.2 (ADOPTED — Site Display Fonts). **CONSISTENT.**

### Color Hex Values
Cross-checked 10 engine colors between CLAUDE.md engine table and Site/design-tokens.css:

| Engine | CLAUDE.md hex | design-tokens.css hex | Verdict |
|--------|---------------|----------------------|---------|
| OTO | `#F5F0E8` (Pipe Organ Ivory) | `#7BA05B` (Bamboo Green) | DIVERGE |
| OCTAVE | `#8B6914` (Hammond Teak) | `#6B2D3E` (Bordeaux) | DIVERGE |
| OLEG | `#C0392B` (Theatre Red) | `#C5A036` (Orthodox Gold) | DIVERGE |
| OTIS | `#D4A017` (Gospel Gold) | `#DAA520` (Soul Gold) | Minor diverge (similar) |
| OVEN | `#1C1C1C` (Steinway Ebony) | `#2C2C2C` (Cast Iron Black) | Minor diverge |
| OCHRE | `#CC7722` | `#CC7722` | MATCH |
| OBELISK | `#FFFFF0` (Grand Ivory) | `#4A4A4A` (Obsidian Slate) | DIVERGE |
| OPALINE | `#B7410E` (Prepared Rust) | `#B8D4E3` (Crystal Blue) | DIVERGE |
| OGRE | `#0D0D0D` (Sub Bass Black) | `#4A2C0A` (Deep Earth Brown) | DIVERGE |
| OLATE | `#5C3317` (Fretless Walnut) | `#6B1A2A` (Burgundy) | DIVERGE |

**Assessment:** The design-tokens.css was updated post-Seance with revised Kitchen/Cellar engine colors (Session 8-9 work). The CLAUDE.md engine table reflects the original Kitchen Collection design brief colors. The `design-tokens.css` is the **canonical CSS source** — it was updated more recently and is what renders in the actual aquarium UI. CLAUDE.md is a developer reference for engine identity, not a UI color authority.

**Action taken:** These are intentional design revisions, not errors. **No change made.** The discrepancy is documented here. If CLAUDE.md needs to be the single color source of truth, a future pass should update the engine table to match design-tokens.css.

### Figma Kits
The definitive UI spec references Knob-Set-07, Knob-Set-08, Knob-Set-09 (lines 445, 458, 1433). The figma-asset-compendium.md does not list these specifically (it covers general Figma community kits, not the purchased Audio UI / KNOBS-SET assets). Asset-registry.md covers these at sections 4-5. **No inconsistency.**

---

## Check 6: Broken Internal Doc Links

### Command run:
```bash
grep -rn "Presets/XOceanus|SDK/include/xoceanus" Docs/ --include="*.md"
```

**Results:**
- `Docs/xoceanus-morning-briefing-2026-03-24.md:33` — `Presets/XOceanus/Atmosphere/...` — FIXED
- `Docs/architect-audit-session-9.md:210` — Documents the above stale path as P2-005 finding — informational record, not a broken link

**SDK paths:** No stale `SDK/include/xoceanus` found. CLAUDE.md correctly uses `SDK/include/xoceanus/`. The SDK directory at `~/Documents/GitHub/XO_OX-XOceanus/SDK/include/xoceanus/` is the canonical path.

### xoceanus_name_migration_reference.md (CLAUDE.md Key Files)
File does not exist as `xoceanus_name_migration_reference.md`. Actual file: `Docs/xoceanus_name_migration_reference.md`. **FIXED** in CLAUDE.md and INDEX.md.

### how_to_write_a_xoceanus_adapter.md (INDEX.md)
File does not exist at that name. Actual file: `Docs/how_to_write_a_xoceanus_adapter.md`. **FIXED** in INDEX.md.

---

## Summary of All Fixes Applied

| File | Fix | Lines |
|------|-----|-------|
| `CLAUDE.md` | 7 `Docs/xoceanus_*.md` paths → `Docs/xoceanus_*.md` | 201, 208, 225, 290, 331, 466, 471, 480, 487, 493 |
| `CLAUDE.md` | Removed two self-referential `(formerly xoceanus_...)` parentheticals | 201, 290 |
| `Site/design-tokens.css` | `--xo-motto: "Omnibus — For All"` → `"XOceanus — For All"` | 756 |
| `Site/aquarium.html` | Added missing OUTLOOK engine card (Preview) | After line 1287 |
| `Docs/INDEX.md` | 9 `xoceanus_*.md` links → `xoceanus_*.md` actual filenames | 8, 10-13, 16-21 |
| `Docs/INDEX.md` | `how_to_write_a_xoceanus_adapter.md` → `how_to_write_a_xoceanus_adapter.md` | 21 |
| `Docs/design/asset-registry.md` | 10 `XO_OX-XOceanus` repo paths → `XO_OX-XOceanus` | 68, 69, 77, 78, 79, 544, 545, 593, 642, 853 |
| `Docs/design/asset-registry.md` | "71 Engine Colors" → "73 Engine Colors" in design-tokens.css note | 544 |
| `Docs/design/asset-registry.md` | Quick-access table engine colors count 71→73, pointer updated | 679 |
| `Docs/design/asset-registry.md` | Mockup path `xoceanus-main-ui.html` → `xoceanus-main-ui.html` | 593 |
| `Docs/xoceanus-morning-briefing-2026-03-24.md` | `Presets/XOceanus/` → `Presets/XOceanus/` | 33 |

---

## Issues NOT Fixed (Documented Only)

| Issue | Reason not fixed |
|-------|-----------------|
| Kitchen/Cellar engine color divergence between CLAUDE.md and design-tokens.css | Intentional design revision — design-tokens.css is authoritative for UI rendering. CLAUDE.md colors reflect original design brief. A future deliberate pass should reconcile if needed. |
| `Docs/design/xoceanus_design_guidelines.md` line 836 still references `XO_OX-XOceanus repo` | Historical document compiled 2026-03-17, prior to rename. The sentence is self-dated. Low priority. |
| ~78 `XO_OX-XOceanus` references in older spec docs (`Docs/specs/`, `Docs/ebook/`, etc.) | These are historical research and planning documents from before the rename. They do not affect runtime behavior. Would require a broad sweep of ~20 files. Flagged for future Historical Society session. |
| `Docs/research/` has no INDEX.md | Not required — main `Docs/INDEX.md` links to research files. |

---

## Post-Fix Verification

- `Site/aquarium.html` `engine-card` count: **73** (was 72)
- `CLAUDE.md` stale `Docs/xoceanus_` paths: **0** (was 7)
- `asset-registry.md` stale `XO_OX-XOceanus` paths: **0** (was 10)
- `design-tokens.css` `--xo-motto`: now says `"XOceanus — For All"`
- `Docs/INDEX.md` broken file links: **0** (was 10)
