# RAC Review: Rebranding XOlokun to XOlokun

**Date:** 2026-03-23
**Reviewers:** Ringleader (Joshua), Architect (Raj), Consultant (Rufus)
**Decision:** Rename the synthesizer from XOlokun to XOlokun. "Omnibus" becomes the tagline — "XOlokun — for all."
**Timing:** Pre-V1, zero external users. The rename window is now.

---

## PART I — ARCHITECT REVIEW (Raj)
### Blast Radius Analysis

The rename touches five categories of artifact: (1) C++ source and build, (2) the C++ namespace, (3) the SDK public ABI, (4) documentation and content, and (5) the preset folder hierarchy. Below is the complete manifest.

---

### Category 1: CMakeLists.txt — Build Identity

**File:** `/Users/joshuacramblet/Documents/GitHub/XO_OX-XOlokun/CMakeLists.txt`

This is the authoritative source of the plugin's machine-visible identity. Every downstream artifact — bundle path, AU database entry, settings folder — derives from it.

| Line | Current Value | New Value | Notes |
|------|--------------|-----------|-------|
| `project(...)` | `XOlokun` | `XOlokun` | CMake project name |
| `juce_add_binary_data(XOlokunFont ...)` | `XOlokunFont` | `XOlokunFont` | Font binary data target name |
| `juce_add_plugin(XOlokun ...)` | `XOlokun` | `XOlokun` | JUCE plugin target |
| `PRODUCT_NAME` | `"XOlokun"` | `"XOlokun"` | Human-visible AU name |
| `BUNDLE_ID` | `"com.xo-ox.xolokun"` | `"com.xo-ox.xolokun"` | macOS/iOS bundle ID |
| `PLUGIN_CODE` | `Xomn` | `Xolk` | **CRITICAL — AU four-character code** |
| `target_sources(XOlokun ...)` | `XOlokun` | `XOlokun` | CMake target reference |
| `target_link_libraries(XOlokun ...)` | `XOlokun` | `XOlokun` | Appears multiple times |
| `XOlokunFont` in `target_link_libraries` | `XOlokunFont` | `XOlokunFont` | Font binary data link |

**PLUGIN_CODE critical note:** `Xomn` is the four-character AU/VST3 identifier baked into saved DAW sessions. Changing it to `Xolk` is safe and correct pre-V1 because no external sessions reference the old code. After V1 ships, this must never change. Choose `Xolk` now and lock it.

---

### Category 2: C++ Source Files — Class Names and Filenames

#### Files requiring rename (filename + all internal references):

**`Source/XOlokunProcessor.h` → `Source/XOlokunProcessor.h`**
- `class XOlokunProcessor` → `class XOlokunProcessor`
- `XOlokunProcessor()` constructor → `XOlokunProcessor()`
- `~XOlokunProcessor()` → `~XOlokunProcessor()`
- `getName() { return "XOlokun"; }` → `getName() { return "XOlokun"; }`
- `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOlokunProcessor)` → `(XOlokunProcessor)`

**`Source/XOlokunProcessor.cpp` → `Source/XOlokunProcessor.cpp`**
- `#include "XOlokunProcessor.h"` → `#include "XOlokunProcessor.h"`
- `#include "UI/XOlokunEditor.h"` → `#include "UI/XOlokunEditor.h"`
- All `XOlokunProcessor::` method definitions → `XOlokunProcessor::`
- `XOlokunProcessor::getName()` string literal change propagates from header
- (242 total occurrences of `XOlokun` in this file — the highest count in the codebase)

**`Source/UI/XOlokunEditor.h` → `Source/UI/XOlokunEditor.h`**
- `#include "../XOlokunProcessor.h"` → `#include "../XOlokunProcessor.h"`
- `class XOlokunEditor` → `class XOlokunEditor` (if class is named this — confirm in file)
- `g.drawText("XOlokun", ...)` at line 3624 → `g.drawText("XOlokun", ...)`
- 52 occurrences of XOlokun in this file

#### Files requiring internal edits only (no rename):

**`Source/UI/PerformanceViewPanel.h`**
- `#include "XOlokunEditor.h"` → `#include "XOlokunEditor.h"`

**`Source/UI/ExportDialog/ExportDialog.h`**
- `#include "../XOlokunEditor.h"` → `#include "../XOlokunEditor.h"`
- `config.bundleId = "com.xo-ox.xolokun." + ...` → `"com.xo-ox.xolokun." + ...`

**`Source/UI/PresetBrowser/PresetBrowser.h`**
- `#include "../XOlokunEditor.h"` → `#include "../XOlokunEditor.h"`

**`Source/Core/SynthEngine.h`**
- 5 occurrences — all comments: "XOlokunProcessor" → "XOlokunProcessor"

**`Source/Core/EngineRegistry.h`**
- 6 occurrences — comments only

**`Source/Core/PresetManager.h`**
- Comments at lines 12, 239, 371 → update to XOlokun

**`Source/Core/PresetMorpher.h`**
- 6 occurrences — confirm comment-only, update

**`Source/Core/CouplingPresetManager.h`**
- `.getChildFile("XOlokun")` → `.getChildFile("XOlokun")` — **this is a filesystem path, changes on-disk location of coupling presets**

**`Source/AI/SoundAssistant.h`**
- System prompt strings (lines 696-697, 714, 754, 826, 1029, 1050): "XOlokun Sound Architect" → "XOlokun Sound Architect"; all "XOlokun" references in prompts

**`Source/AI/SecureKeyStore.h`**
- `.getChildFile("XOlokun")` → `.getChildFile("XOlokun")` — **filesystem path**

**`Source/Export/XPNExporter.h`**
- Comments at lines 55, 121, 237, 240
- String at line 237: `"XOlokun - Foundation"` → `"XOlokun - Foundation"`
- String at line 240: `"com.xo-ox.xolokun.foundation"` → `"com.xo-ox.xolokun.foundation"`

**Engine files with `namespace xolokun` comment references:**
- `Source/Engines/Oware/OwareEngine.h` (6 refs)
- `Source/Engines/Oracle/OracleEngine.h` (6 refs)
- `Source/Engines/Onset/OnsetEngine.h` (6 refs)
- `Source/Engines/Oblique/ObliqueEngine.h` (6 refs)
- `Source/Engines/Owlfish/OwlfishEngine.h` (14 refs)
- `Source/Engines/Ocelot/OcelotEngine.h` (11 refs), `OcelotFloor.h` (9), `OcelotEmergent.h` (6)
- `Source/Engines/Obsidian/ObsidianEngine.h` (10 refs)
- `Source/Engines/Morph/MorphEngine.h` (7 refs)
- `Source/Engines/Fat/FatEngine.h` (7 refs)
- `Source/Engines/Overworld/OverworldEngine.h` (7 refs)
- `Source/Engines/Overlap/XOverlapAdapter.h` (7 refs)
- `Source/Engines/Opal/OpalEngine.h`, and many others with 5-6 refs each
- All other engine headers in the 248-file source count

Engines with `namespace xolokun` contain the namespace as a C++ symbol, not just a display string. See Category 3.

**`Source/Engines/Overlap/DSP/FastMath.h`** and **`Source/Engines/Outwit/DSP/FastMath.h`**
- 18 refs each — confirm whether these are namespace or comment references

---

### Category 3: The C++ Namespace — `xolokun`

The codebase uses `namespace xolokun` pervasively — 397 opening/closing pairs found in Source alone. The SDK public headers also use `namespace xolokun` and export symbols `xolokun_create_engine()` and `xolokun_engine_info()`.

**Decision required from the owner:**

Option A — **Rename the namespace to `xolokun`**
- Full consistency. Every source file changes.
- SDK ABI breaks (the function symbols `xolokun_create_engine` change). Since no external SDK users exist yet (SDK Phase 1 was internal), this is safe now.
- Estimated files: all 248 Source files + all 8 SDK files. Mechanical sed replacement.

Option B — **Keep the namespace as `xolokun` internally**
- Zero C++ compilation risk.
- The namespace is invisible to end users.
- Precedent exists: JUCE uses `juce::` even though products built on it are not called JUCE. Many companies keep internal technical names distinct from brand names.
- Docs and comments still need updating, but no code logic changes.

**Architect recommendation: Option B for V1.** The namespace is a private implementation detail. Users never see it. Rename it in a dedicated V2 cleanup sprint after V1 ships. Document the divergence explicitly in CLAUDE.md.

The two SDK function symbols (`xolokun_create_engine`, `xolokun_engine_info`) are a slightly stronger case for renaming since they form the public plugin module ABI — but again, zero external SDK users exist. If renamed, do it atomically with all SDK consumers.

---

### Category 4: The SDK — `SDK/include/xolokun/`

**Files requiring changes:**
- `SDK/include/xolokun/EngineModule.h` — 10 refs to `xolokun` namespace + symbol names
- `SDK/include/xolokun/SynthEngine.h` — namespace
- `SDK/include/xolokun/CouplingTypes.h` — namespace
- `SDK/tools/validate_engine.py` — references to xolokun symbols
- `SDK/README.md` — brand text
- `SDK/examples/HelloEngine/README.md` and `HelloEngine.h` — brand text
- `SDK/templates/MinimalEngine/MinimalEngine.h` — brand text

**Directory rename:** `SDK/include/xolokun/` → `SDK/include/xolokun/` (only if namespace is renamed per Option A)

---

### Category 5: Preset Directory and Preset Files

**Critical: Filesystem path change**

`Presets/XOlokun/` is the factory preset directory root. It appears in:
- `CMakeLists.txt` (binary data sourcing, if applicable)
- `Source/Core/PresetManager.h` (hardcoded path lookups — confirm)
- `README.md`
- `CLAUDE.md`

The 13 `.xometa` files in `Presets/XOlokun/` that grep-matched contain only incidental text references in description strings, not structural fields. The preset schema uses `"author": "XO_OX Designs"` and does not embed the plugin name as a required field. This means the `.xometa` files themselves need only cosmetic description updates (optional, not blocking).

**Action:** Rename `Presets/XOlokun/` directory → `Presets/XOlokun/` and update all path references.

**`Presets/Docs/engine_dna_profiles.json`** — 1 reference in description string, update.

---

### Category 6: Documentation — 415 Files

The Docs folder has the highest surface area. Breakdown by priority:

**Tier 1 — Rename file + update contents (filename contains XOlokun):**
- `Docs/xolokun_master_specification.md` → `xolokun_master_specification.md`
- `Docs/xolokun_engine_roadmap.md` → `xolokun_engine_roadmap.md`
- `Docs/xolokun_engine_roadmap_v3.md` → `xolokun_engine_roadmap_v3.md`
- `Docs/xolokun_brand_identity_and_launch.md` → `xolokun_brand_identity_and_launch.md`
- `Docs/xolokun_landscape_2026.md` → `xolokun_landscape_2026.md`
- `Docs/xolokun_launch_audit_2026.md` → `xolokun_launch_audit_2026.md`
- `Docs/XOlokun_Master_Architecture- Volume 2.md.txt` → rename
- `Docs/xolokun_master_fx_design_brief.md`
- `Docs/xolokun_mobile_and_midi_spec.md`
- `Docs/xolokun_mobile_implementation_strategy.md`
- `Docs/xolokun_module_starter_design.md`
- `Docs/xolokun_name_migration_reference.md`
- `Docs/xolokun_new_engine_process.md`
- `Docs/xolokun_preset_spec_for_builder.md`
- `Docs/xolokun_recipe_system_design.md`
- `Docs/xolokun_repo_structure.md`
- `Docs/xolokun_sound_design_guides.md`
- `Docs/xolokun_technical_design_system.md`
- `Docs/xolokun_v2_roadmap.md`
- `Docs/xolokun_volume2_review.md`
- `Docs/how_to_write_a_xolokun_adapter.md`
- `Docs/design/xolokun_design_guidelines.md`
- `Docs/design/xolokun_ui_master_spec_v2.md`
- `Docs/mockups/xolokun-main-ui.html`
- `Docs/concepts/xolokun_collections_vision.md`
- `Docs/plans/xolokun_v1_launch_master_plan.md`

**Tier 2 — Content update only (file mentions XOlokun but is not named for it):**
All remaining 415 - 26 = ~389 Docs files with incidental XOlokun mentions. These are seance verdicts, sweep reports, build verifications, etc. These are historical records and do not need to be retroactively rewritten — they document the product as it was during development. Consider adding a one-time header note: "Note: This document was written before the rebrand to XOlokun. References to XOlokun refer to the same product."

---

### Category 7: Site — 18 Files

All 18 site HTML/XML files contain XOlokun references and require updates:
- `site/index.html`, `site/aquarium.html`, `site/manifesto.html`, `site/packs.html`, `site/updates.html`, `site/guide.html`, `site/guide-collections.html`, `site/feed.xml`
- All `site/guide-*.html` files

These are public-facing and must be updated before any V1 announcement.

---

### Category 8: Support Files

**`README.md`** — Multiple references including "XOlokun" heading, preset path, doc references.

**`CLAUDE.md`** — 20+ references: product name, doc paths, preset paths. This is the AI agent project guide and must be updated to reflect the new name so future Claude sessions use the correct name.

**`Skills/`** — 17 skill files reference XOlokun in SKILL.md content. These should be updated as Skills are actively used by agents.

**`scripture/`** — 18 files with XOlokun mentions (mostly in context of the product). These are archival; update the active-use retreat templates.

**`patreon/`** — 7 files. These are public-facing (Patreon descriptions) and must be updated before any announcement.

**`Tools/`** — 249 Python tool files with XOlokun references. Most are in: variable names like `xolokun_version`, `xolokun_idx`, comments, and version string fields like `"xolokun_version": "1.0"`. The version field in generated XPN metadata is the highest-priority update as it appears in customer-facing output.

---

### Order of Operations

Execute in this sequence to minimize broken-build time:

**Step 1 — CMakeLists.txt** (do first, invalidates all targets)
1. Rename `juce_add_binary_data(XOlokunFont` → `XOlokunFont`
2. Rename `juce_add_plugin(XOlokun` → `XOlokun`
3. Update `PRODUCT_NAME`, `BUNDLE_ID`, `PLUGIN_CODE`
4. Update all `target_sources(XOlokun` → `XOlokun`
5. Update all `target_link_libraries`

**Step 2 — Source file renames** (must happen before any build)
1. `Source/XOlokunProcessor.h` → `Source/XOlokunProcessor.h`
2. `Source/XOlokunProcessor.cpp` → `Source/XOlokunProcessor.cpp`
3. `Source/UI/XOlokunEditor.h` → `Source/UI/XOlokunEditor.h`

**Step 3 — Source content edits** (class names, string literals, filesystem paths)
1. Class rename: `XOlokunProcessor` → `XOlokunProcessor`, `XOlokunEditor` → `XOlokunEditor`
2. String literals: `"XOlokun"` → `"XOlokun"` in getName(), drawText(), AI prompts
3. Filesystem path strings: `"XOlokun"` → `"XOlokun"` in `CouplingPresetManager.h`, `SecureKeyStore.h`, `ExportDialog.h`
4. Update all `#include "XOlokunProcessor.h"` / `XOlokunEditor.h` across all files that include them

**Step 4 — Namespace decision** (if Option A chosen)
- Global sed: `namespace xolokun` → `namespace xolokun` across all 248 Source files + 8 SDK files
- Update SDK ABI symbols: `xolokun_create_engine` → `xolokun_create_engine`
- Update SDK include path: `<xolokun/...>` → `<xolokun/...>`
- If Option B: document the namespace divergence in CLAUDE.md

**Step 5 — Build and test**
- `cmake -B build -G Ninja` from repo root
- Resolve any include path errors
- `cmake --build build --target XOlokun_AU`
- auval pass check

**Step 6 — Preset directory rename**
1. `git mv Presets/XOlokun Presets/XOlokun`
2. Update `PresetManager.h` path references
3. Update `README.md`, `CLAUDE.md` references to preset path

**Step 7 — Tools and infrastructure**
- Update `xolokun_version` field name in Python tools to `xolokun_version` (or keep as a legacy compat field)
- Update XPN sidecar spec `"xolokun_version_min"` field

**Step 8 — Docs, Site, Skills, Patreon** (can run in parallel, no build dependency)

**Step 9 — CLAUDE.md update** (last, after all paths stabilized)

---

### Architect Risk Summary

| Risk | Severity | Notes |
|------|----------|-------|
| PLUGIN_CODE change (`Xomn` → `Xolk`) | Low — pre-V1 | Must never change again after V1 |
| BUNDLE_ID change | Low — pre-V1 | Old AU entry in Audio Unit database will become stale; run `auval` after change |
| Namespace change (if Option A) | Medium | ~397 edit sites, but purely mechanical — no logic change |
| Filesystem path strings in source | Medium | `CouplingPresetManager.h`, `SecureKeyStore.h` write to user home directory paths |
| Preset directory rename | Low | `git mv` preserves history |
| 415 Docs files | Low | Content-only, no build impact |

**The rename is architecturally clean.** Pre-V1, there are no saved DAW sessions, no installed plugins, no user presets, and no SDK consumers to protect. The blast radius is large in file count but low in technical risk. A scripted replacement followed by a clean build and auval pass is sufficient validation.

---

## PART II — CONSULTANT REVIEW (Rufus)

### Cultural Research: Who Is Olokun

Olokun is one of the most ancient and significant orishas in the Yoruba religious tradition of West Africa. The name derives from Yoruba: *oló* ("owner") + *okun* ("ocean") — the Owner of the Ocean. Olokun holds dominion over the deep ocean floor — not the surface, not the waves, but the pressurized, lightless, immeasurable abyss where the floor of the sea lies beyond human reach.

Olokun's domains are: the deep ocean, mystery, hidden wealth, transformation, the unconscious, and the boundary between the living and the dead. Devotees approach Olokun with particular reverence because Olokun represents the unknowable — the part of existence that cannot be fully perceived, categorized, or controlled.

**Gender fluidity as theology.** Communities throughout West Africa and the African diaspora understand Olokun as female, male, or androgynous depending on regional tradition. In coastal Yorùbáland, Olokun is often male; in hinterland communities, female. In Candomblé and broader Yorùbá metaphysics, this is not contradiction — it is a deliberate theological quality. Olokun's gender duality reflects the depth and complexity of the ocean itself. That same duality belongs to the name.

**Diaspora reach.** The worship of Olokun survived the Middle Passage and lives in the religions of the African diaspora: Candomblé in Brazil (where Olokun is recognized as a divinity of ocean depth, parent of Yemoja), Santería/Lucumí in Cuba and the United States, Trinidad Orisha tradition, and Vodou. In many Lucumí houses, Olokun is received as a physical vessel — a covered clay pot with chains, kept sealed, containing the depths that must never be fully exposed. The covering is the point: some things are too powerful, too sacred, too complete to be seen all at once. Olokun is depth.

**The Atlantic crossing.** For the African diaspora, the ocean floor is not abstract. Olokun holds the souls of those who died crossing the Atlantic in the Middle Passage. This is a layer of profound weight that any use of the name must acknowledge, not instrumentalize.

**In Candomblé specifically.** In Brazilian Candomblé, Olokun holds a somewhat esoteric position — recognized in terreiros as the mother of Yemoja (Iemanjá) and owner of the sea, but traditionally without a major public xirê cycle of her own. This makes Olokun a deity of the interior, the contemplative, the deep study — parallel to how an instrument built on depth and coupling rather than surface-level performance would be understood.

---

### Why the Name Fits

XOlokun names a synthesizer that lives in the deep. Its architecture — 73 engines, the coupling system, the depth zones of the aquatic mythology, the unknowable emergent sounds that arise when engines couple — maps directly onto Olokun's attributes:

- **The unknowable floor:** You cannot hear all of XOlokun at once. Coupling creates sounds no single preset or engine can predict. Like the ocean floor: real, present, but impossible to fully surface.
- **Depth zones:** The Aquatic Collection's own mythology (Sunlit, Twilight, Midnight, Hadal zones) mirrors Olokun's domain precisely. This is not retrofitted — the aquatic DNA was already in the instrument.
- **Wealth and transformation:** Olokun is the parent of Ajé, the orisha of great wealth. XOlokun is a tool for creating — for generating musical value, for transformation of sound.
- **Gender fluidity and duality:** The instrument carries its own duality in the name: XO (the embrace, the kiss and the cross, the palindrome). Olokun's gender duality amplifies this.
- **For all.** The motto "for all" carries direct meaning in this context. The African musical traditions that gave birth to virtually every popular Western music genre — blues, jazz, soul, hip-hop, R&B, house, techno — built the world the instrument inhabits. The music this tool creates is built on that foundation. "For all" is not casual. It is a commitment.

---

### Cultural Acknowledgment — Full Draft

The following text is written for three contexts: the plugin's About dialog (brief version), the XO-OX.org website (standard version), and the Field Guide (full version). Use as appropriate.

---

#### ABOUT DIALOG (Brief — ~80 words)

> XOlokun takes its name from Olokun — the Yoruba orisha of the deep ocean, keeper of the floor of the sea, the unknowable depth. The name belongs to a living tradition honored across West Africa, Brazil, Cuba, Trinidad, and throughout the African diaspora.
>
> We chose this name with reverence, not appropriation. The music this instrument serves — hip-hop, soul, Afrobeats, jazz, house, R&B — grew from African and diasporic roots. XOlokun is for all who make it.
>
> *XO_OX Designs, 2026*

---

#### WEBSITE (Standard — ~250 words)

**A Name From the Deep**

XOlokun is named for Olokun — one of the oldest and most powerful orishas in the Yoruba tradition of West Africa. Olokun's name means "owner of the ocean," and Olokun's domain is the deep floor of the sea: the pressurized, lightless, immeasurable abyss where the ocean meets what we cannot know.

The name belongs to a living spiritual tradition practiced across Yorùbáland in present-day Nigeria and Benin, and carried through the African diaspora to Brazil (Candomblé), Cuba and the United States (Santería/Lucumí), Trinidad, and beyond. Olokun is venerated as the keeper of hidden wealth, the guardian of the space between the living and the dead, and the force at the bottom of the world that is too deep to be fully known.

We chose this name because it names what the instrument actually does. XOlokun is not a surface instrument. Its engine coupling system generates sounds that no single preset or engine can predict — emergent, alive, found at depth. Its aquatic mythology lives in the Hadal zone. Its architecture is the floor of the ocean.

We chose this name with reverence. The musical traditions this tool serves — hip-hop, soul, Afrobeats, jazz, R&B, house, techno, and every form that grew from African and diasporic soil — are not background. They are the reason this instrument exists. We acknowledge that foundation explicitly and with gratitude.

XOlokun — for all.

---

#### FIELD GUIDE (Full — ~600 words)

**The Name at the Bottom of the Sea**

When XO_OX Designs chose to name its synthesizer XOlokun, the choice was not decorative. It was a reading of what the instrument had already become.

Olokun is one of the most ancient orishas in the Yoruba tradition — a spiritual cosmology originating in what is now Nigeria and Benin, practiced by tens of millions of people today. The name is Yoruba: *oló* (owner) + *okun* (ocean). Olokun is the Owner of the Ocean — not the surface, not the shore, not the waves. The floor. The part of the ocean that no human has fully seen or measured. The part that remains itself regardless of who looks.

Olokun's attributes are the deep: hidden wealth, mystery, the boundary of the known, transformation, and the sacred weight of what cannot be fully surfaced. In Lucumí tradition (Santería), Olokun is received as a sealed vessel — covered, because some powers are complete in themselves and do not require exposure to be real. In Candomblé, Olokun is the mother of Yemoja (Iemanjá), the ocean's depth beneath the ocean's face. Across the African diaspora, in Brazil, Cuba, Trinidad, the United States, and wherever Yoruba tradition took root through the violence of the Middle Passage, Olokun endures.

That crossing matters here. The ocean floor Olokun holds is also the resting place of those who did not survive the Middle Passage. The name carries that weight. We do not use it lightly.

The musical traditions XOlokun was built to serve — hip-hop, soul, R&B, Afrobeats, jazz, house, techno, and every genre that grew from African and diasporic roots — exist because African people carried their creativity, their spiritual practices, their rhythms, and their intelligence through centuries of oppression and built the musical world that the entire planet now inhabits. That is not metaphor. It is the actual origin story of the music this instrument makes.

XOlokun's architecture reflects its name. The coupling system generates sounds that no single engine or preset can predict — emergence from depth, not from surface controls. The Aquatic Collection mythology was already organized by depth zones: Sunlit, Twilight, Midnight, Hadal. The floor was always there. Olokun named it.

Olokun's gender is fluid across traditions — male in coastal Yorùbáland, female in hinterland practice, androgynous in broader Yorùbá metaphysics. This is not ambiguity. It is the ocean's own complexity refusing to be simplified. The name XOlokun carries its duality honestly: XO (the palindrome, the kiss and the cross, the both/and) plus Olokun (the depth that holds everything, the force that cannot be reduced).

The motto "for all" — inherited from the Latin omnibus that preceded the rebrand — takes on a specific meaning in this context. It names a commitment: to build instruments that honor the cultures whose music fills them, to make tools accessible to the producers in Lagos and Atlanta and Rio and London and Manila who are making the music that matters, and to acknowledge that "for all" is not a casual phrase. It is a promise.

XO_OX Designs welcomes critique, correction, and relationship from practitioners of Yoruba tradition, Candomblé, Lucumí/Santería, and diasporic communities who hold Olokun sacred. We are learning, and we are listening.

*XOlokun — for all.*

*XO_OX Designs, 2026*

---

### Trademark and Competitive Research

**"XOlokun" as a composite mark:** No products found under the name "XOlokun" or "XO Lokun" in any search of music software, plugins, or commercial products. The composite is new.

**"Olokun" alone:**
- OLOKUN is an active psychedelic art pop duo based in California (Butterfly Williams + Winston Berger), active as of 2025-2026 and releasing music. They operate as musicians, not software vendors. No trademark registration appears to exist (the USPTO search returned no specific Olokun registrations in software/music technology).
- Multiple individual tracks and albums use "Olokun" as a title (Isaac Soto, Azagatel, Drala, XAVO). These are song titles, not product names.
- Yoruba Records has released a track titled "Olokun." Again, a song title.
- No commercial software, plugin, or synthesizer under the name "Olokun" was found.

**Assessment:** The composite mark "XOlokun" in the International Class 9 (software) or Class 41 (entertainment/music services) category appears clear. The OLOKUN music duo operates in a different class (performing artists, recordings) and the composite "XOlokun" with the XO prefix distinguishes further.

**Recommended action:** Commission a formal USPTO trademark search for "XOlokun" in Class 9 (computer software) and Class 41 (entertainment services) before V1 launch. This is standard pre-launch diligence. The informal research here suggests the mark is available, but a formal search is required before filing.

**Note on cultural trademark ethics:** Some practitioners and scholars have raised concerns about trademarking the names of sacred figures. "Olokun" as a standalone word is a religious name held by a community, not a word that can or should be owned. The composite "XOlokun" — XO_OX's brand prefix plus Olokun — is the actual mark being used, which is both legally and ethically the appropriate approach: the brand owns the composite, not the sacred name itself.

---

### Ethics Director Note

This review was written with the understanding that it should be reviewed by a cultural advisor with direct knowledge of Yoruba tradition and the African diaspora before the Cultural Acknowledgment text appears in any public-facing product. The text above is a first draft for internal review.

Recommended next step: identify an advisor — a practitioner, scholar, or cultural organization connected to Yoruba religion or the diaspora — and share the Cultural Acknowledgment for feedback before V1. The XO_OX community channels (Patreon, Discord) may be a route to finding such a reviewer. Organizations such as the Yoruba Cultural Alliance or academic departments of African and African Diaspora Studies at major universities could also be approached.

The owner's stated intention — "I want to completely honor that culture. I hope the tool clearly demonstrates deep respect and reverence" — is the right foundation. Honoring that intention requires going beyond self-review.

---

## PART III — RINGLEADER SUMMARY

### Decision

The rename to XOlokun is approved on all three axes reviewed:

1. **Architecturally feasible:** Large surface area, low technical risk. Pre-V1 timing is ideal. The namespace question (Option A vs B) is the only genuine architectural decision; Option B (keep `namespace xolokun` internally, rename only the user-visible product) is the safer V1 path.

2. **Culturally grounded:** Olokun fits the instrument's actual architecture and mythology in specific, non-superficial ways. The commitment to reverence is sound in intention. External cultural review before public launch is required to complete due diligence.

3. **Legally clear:** No competing "XOlokun" products found. Formal USPTO search recommended before V1 filing.

### Immediate Actions (by priority)

| # | Action | Who | When |
|---|--------|-----|------|
| 1 | Namespace decision: Option A or B | Owner | Before any code changes |
| 2 | Execute rename in CMakeLists.txt | Claude | Next session |
| 3 | Rename and update `XOlokunProcessor.*`, `XOlokunEditor.h` | Claude | Next session |
| 4 | Update string literals and filesystem path strings in source | Claude | Next session |
| 5 | Build + auval pass | Owner/Claude | Immediately after code |
| 6 | `git mv Presets/XOlokun Presets/XOlokun` + path update in PresetManager | Claude | Same session |
| 7 | Update `CLAUDE.md`, `README.md`, `site/` | Claude | Same session |
| 8 | Update `Skills/`, `patreon/`, active `Docs/` files | Claude | Same session |
| 9 | Identify cultural advisor for Acknowledgment review | Owner | This week |
| 10 | Formal USPTO trademark search | Owner | Before V1 announcement |

### What Does Not Change

- All individual engine names (ONSET, OWARE, OVERDUB, etc.) — these follow the XO + O-word convention and are unchanged
- The `.xometa` preset file format and schema
- The XPN export format
- The `.xocoupling` format
- All parameter IDs (these must never change post-V1 regardless)
- The XO_OX company name and domain (xo-ox.org)
- The repo name `XO_OX-XOlokun` on GitHub (can be renamed at the owner's discretion)

### The Motto

"XOlokun — for all."

Omnibus becomes the tagline, not the name. The Latin root ("for all") lives in the motto. The depth lives in the name.

---

*RAC review complete. Prepared by Claude Code for the XO_OX Designs team.*
*Sources consulted: Wikipedia (Olokun), Daily-IFÁ, Beyond Roots, Santería Guide, Lightworkers Garden, Historical Nigeria, Afrodeities, USPTO trademark search resources, Bandcamp artist research.*
