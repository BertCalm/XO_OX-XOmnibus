# XO_OX XPN Toolchain Catalog & Documentation

**Generated**: 2026-03-29  
**Location**: `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Tools/`

This directory contains three comprehensive reference documents cataloging the 398-file XPN (Expansion Pack) export pipeline:

---

## Reference Documents

### 1. TOOLCHAIN_CATALOG_SUMMARY.txt (259 lines, 11 KB)
**Quick Executive Summary** — Start here.

Contains:
- **Critical Path**: 10 tools that must work
- **High-Value Support**: 9 supplementary tools
- **Category Breakdown**: 233 core files organized by function
- **Dependency Map Summary**: Call chains and data flow
- **Dead Code List**: 166+ tools not in active pipeline
- **Key Findings & Recommendations**: 5 actionable insights
- **Final Statistics**: Line counts, health metrics

Best for: Getting oriented quickly, understanding which tools matter most, identifying maintenance debt.

### 2. TOOLCHAIN_CATALOG_DETAILED.md (890 lines, 33 KB)
**Comprehensive Detailed Catalog** — The authoritative reference.

Sections:
- **Core Pipeline Orchestrators** (3 tools)
  - oxport.py (2774 lines)
  - xpn_bundle_builder.py (1436 lines)
  - xpn_batch_export.py (371 lines)

- **Render & Audio** (4 tools)
  - oxport_render.py, xpn_drum_export, xpn_keygroup_export, etc.

- **Sample Processing** (6 tools)
  - Categorization, trimming, fingerprinting

- **Sound Design & DNA** (12 tools)
  - DNA computation, interpolation, diversity analysis

- **Cover Art & Visuals** (5 tools)
  - xpn_cover_art.py, spectral fingerprinting

- **Validation & QA** (12+ tools)
  - Preset, XPM, bundle, and pack validators

- **Preset Generation** (15+ tools)
  - Naming, variation, breeding, mood classification

- **Packaging & Bundling** (8 tools)
  - ZIP assembly, licenses, README generation

- **Export & Translation** (6 tools)
  - Format conversion, SFZ export, JSON builders

- **Utility & Helpers** (10+ tools)
  - Kit expansion, choke groups, tuning systems

- **Metadata & Documentation** (6+ tools)
  - Manifests, sidecars, license generation

- **Dead Code** (166+ tools)
  - Comprehensive list with line counts

- **Archive Directory** (165 files)
  - Legacy generators, fixups, converters

Best for: Understanding what each tool does, finding implementation details, integration planning.

### 3. DEPENDENCY_MAP.txt (520 lines, 20 KB)
**Complete Dependency Graph** — Technical deep dive.

Sections:
- **Core Pipeline Orchestration** (10-stage tree)
  - Visual representation of oxport.py calling all subordinate tools
  - Stage 1 (Render Spec) through Stage 10 (Manifest/Metadata)

- **Secondary Support Chains**
  - DNA Computation chain
  - Diversity Analysis tools
  - Preset Generation & Naming
  - Batch & Orchestration
  - Analysis & Reporting
  - Utility & Helper Tools
  - Metadata & Documentation
  - Export & Format Conversion

- **Dead Code** (166+ tools)
  - Organized by type and size

- **Key Integration Points** (5 critical paths)
  - MIDI Rendering (oxport → oxport_render)
  - Classification Hub (xpn_classify_instrument, used by 6 tools)
  - Cover Art (xpn_cover_art, used by 5+ tools)
  - Packaging (xpn_packager)
  - Validation (xpn_pack_qa_report)

- **Dependency Depth & Complexity**
  - 4-level orchestration (70+ reachable tools)
  - Circular dependency check (NONE DETECTED)
  - Self-reference analysis
  - Critical hub tools

Best for: Understanding data flow, integration testing, identifying single points of failure, refactoring decisions.

---

## Quick Stats

| Metric | Value |
|--------|-------|
| **Core Tools** | 233 files |
| **Archived Tools** | 165 files |
| **Total Python Code** | ~180,000 lines |
| **Active in Pipeline** | ~20 tools (~35,000 lines) |
| **Supplementary/One-Off** | ~50 tools (~45,000 lines) |
| **Legacy/Archive** | ~165 tools (~100,000 lines) |

---

## Key Findings

### 1. Core Pipeline is Well-Designed
✓ oxport.py cleanly orchestrates 25 sub-tools  
✓ xpn_classify_instrument acts as reliable hub (used by 6+ others)  
✓ Clear separation: RENDER → CATEGORIZE → EXPORT → QA → PACKAGE  
✓ No circular dependencies detected  

### 2. Hidden Complexity: 70% "Dead Code"
⚠ 166+ tools not called by oxport pipeline  
⚠ Most are specialized preset builders or one-off fixups  
⚠ Many are useful for rapid prototyping but not essential  

### 3. Critical Integration Points
→ oxport → oxport_render (MIDI rendering)  
→ oxport → xpn_classify_instrument (cascades to 5 dependent tools)  
→ oxport → xpn_cover_art (visual branding)  
→ oxport → xpn_packager (ZIP creation)  

### 4. Maintenance Debt
• validate_presets.py (620 lines) is standalone but used by oxport  
• xpn_render_spec.py and xpn_render_spec_generator.py are duplicates  
• Some tools have self-references in imports  

### 5. Missing Documentation
• No tool docstring index  
• No dependency diagram in codebase  
• No "critical path" documentation for oxport  
• No "utilities vs. pipeline" categorization  

---

## Critical Path (10 Must-Have Tools)

1. **oxport.py** (2774 lines) — Master orchestrator
2. **oxport_render.py** (386 lines) — WAV rendering
3. **xpn_render_spec.py** (611 lines) — Render spec generation
4. **xpn_sample_categorizer.py** (383 lines) — Categorization
5. **xpn_classify_instrument.py** (547 lines) — Classification hub
6. **xpn_drum_export.py** (377 lines) — Drum XPM generation
7. **xpn_keygroup_export.py** (408 lines) — Pitched XPM generation
8. **xpn_cover_art.py** (1107 lines) — Cover art generation
9. **xpn_packager.py** (478 lines) — ZIP assembly
10. **xpn_pack_qa_report.py** (650 lines) — QA aggregation

---

## Classification Legend

| Category | Count | Examples |
|----------|-------|----------|
| **RENDER** | 4 | oxport_render, xpn_drum_export, xpn_render_spec |
| **EXPORT** | 5 | oxport, xpn_bundle_builder, xpn_packager |
| **CATEGORIZE** | 2 | xpn_classify_instrument, xpn_sample_categorizer |
| **TRIM** | 1 | xpn_smart_trim |
| **QA** | 5+ | xpn_pack_qa_report, validate_presets, xpn_validator |
| **COVER_ART** | 3 | xpn_cover_art, xpn_optic_fingerprint |
| **NAMING** | 6 | xpn_preset_name_generator, xpn_normalize |
| **DIVERSITY** | 12 | compute_preset_dna, xpn_auto_dna, xpn_dna_interpolator |
| **PACK** | 8 | xpn_bundle_builder, xpn_collection_sequencer |
| **PRESET** | 15+ | breed_presets, various generators |
| **UTILITY** | 10+ | xpn_kit_expander, xpn_choke_group_assigner |
| **METADATA** | 6 | xpn_manifest_generator, xpn_expansion_json_builder |
| **ANALYSIS** | 10+ | xpn_fleet_health_dashboard, xpn_pack_health_dashboard |

---

## How to Use These Documents

### For Quick Understanding
1. Start with **TOOLCHAIN_CATALOG_SUMMARY.txt**
2. Read "Critical Path" section (10 tools)
3. Skim "Key Findings & Recommendations"

### For Integration Testing
1. Read **DEPENDENCY_MAP.txt** sections 1–5
2. Focus on "Key Integration Points" (5 critical paths)
3. Use "Dependency Depth & Complexity" to plan test order

### For Refactoring/Cleanup
1. Review **TOOLCHAIN_CATALOG_DETAILED.md** sections on dead code
2. Use **DEPENDENCY_MAP.txt** to understand what can be safely archived
3. Check reverse dependencies before modifying hub tools (xpn_classify_instrument, xpn_cover_art)

### For New Development
1. Understand the 10-stage pipeline in **DEPENDENCY_MAP.txt**
2. Identify which stage your new tool belongs in
3. Check if existing tools in that stage can be reused/extended
4. Plan integration point with oxport.py if adding to active pipeline

### For Pipeline Health Monitoring
1. Track "Critical Path" tools for breaking changes
2. Monitor hub tools (xpn_classify_instrument, xpn_cover_art) for regression
3. Run integration tests on the 5 critical paths from DEPENDENCY_MAP.txt

---

## Files Included in This Catalog

| File | Purpose | Size | Format |
|------|---------|------|--------|
| TOOLCHAIN_CATALOG_SUMMARY.txt | Executive summary | 11 KB | Plain text |
| TOOLCHAIN_CATALOG_DETAILED.md | Detailed reference | 33 KB | Markdown |
| DEPENDENCY_MAP.txt | Dependency graph | 20 KB | Plain text (tree format) |
| README_CATALOG.md | This file | — | Markdown |

---

## Generated Metadata

- **Date**: 2026-03-29
- **Repository**: /Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/
- **Tools Scanned**: 398 files (233 core + 165 archive)
- **Total Code**: ~180,000 lines of Python
- **Dependency Analysis**: Automated scan for imports, subprocess calls, cross-tool references
- **Circular Dependency Check**: PASS (none detected)

---

## Next Steps

1. **Documentation**: Add brief docstrings to each tool (currently missing)
2. **Reorganization**: Move 50+ specialized preset generators to `/Tools/generators/` subdirectory
3. **Validation**: Run integration tests on the 5 critical paths
4. **Monitoring**: Set up CI checks to detect new dead code or circular dependencies
5. **Cleanup**: Archive or delete 166+ unused tools (or document their manual use cases)

---

**Questions?** Refer to the detailed catalog or dependency map.  
**Maintenance?** Update these documents when tools are added/removed from the pipeline.

EOF

cat /Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/Tools/README_CATALOG.md
