# Preset Picker Design — XOceanus

**Date**: 2026-04-20
**Status**: Proposed (brainstorm complete, awaiting implementation plan)
**Author**: Josh Cramblet, with Claude/Ringleader brainstorming
**Supersedes (partial)**: `Docs/specs/xoceanus_preset_spec_for_builder.md` — preset metadata schema section
**Related**: `Docs/design/ecological-interface-component-spec-v1.md`, `Source/UI/PresetBrowser/PresetBrowser.h`, `Tools/ui-preview/submarine.html`
**Mythology north star**: "The world IS the ocean" (from `ui-demolition-2026-04-05.md`)

---

## 1. Context & Problem

XOceanus has **19,885 presets** across **73 engines**, including presets that only materialize when specific pairs of engines are coupled (see `Coupling_Knot_Topology.xometa`). The current preset surfaces — the Submarine HTML prototype's modal browser and the production `Source/UI/PresetBrowser/PresetBrowser.h` JUCE component — are mature but were designed for a smaller library. At 20k presets, a flat list with mood chips and a sort dropdown stops functioning as discovery; it becomes a wall.

The real problem isn't "design a preset picker" — it's **discovery and trust at scale**. Users need to:

- Find presets they already know, fast (recall)
- Find presets that fit the moment (search)
- Discover presets they didn't know existed (wonder)
- Understand that coupling-dependent presets exist at all (education)
- Never feel like the library is larger than their comprehension

This spec addresses all four, through three purpose-built surfaces and a structural upgrade to preset metadata.

---

## 2. Design Decisions (locked)

Reference for anyone reading this later — these were settled during the brainstorm.

| # | Decision | Rationale |
|---|----------|-----------|
| 1 | **Three surfaces**: Quick-pick (in-flow), Palette (⌘P), Discover (hero world) | Single-surface designs that try to serve all moments end up mediocre at all of them |
| 2 | **Discover is the hero** | Wonder is the brand promise; aligns with "the world IS the ocean" mythology |
| 3 | **Discover structure**: Ocean (engines as creatures) → Chamber (tap engine) → Atlas cards inside. Plus cross-fleet Atlas and Coupling Reef as additional Discover modes | "Deep Dive × Atlas" combination — structure + wonder |
| 4 | **Peer axes**: Mood · Instrument/Category · Timbre · Collection · Coupling · DNA · Author · Date | Multiple access paths for different mental models |
| 5 | **10-term instrument taxonomy** (new, required field): keys, pads, leads, bass, drums, perc, textures, fx, sequence, vocal | Functional categorization; tight enough for chip UI |
| 6 | **8-term timbre taxonomy** (new, optional field): strings, brass, wind, choir, organ, plucked, metallic, world | Sonic/acoustic-family axis, separate from functional role |
| 7 | **Coupling-dependent presets**: Locked-tease + smart promotion + dedicated Coupling Reef destination | Turns the coupling system into a gameplay loop, not a hidden metadata field |
| 8 | **No audio previews** | Visual thumbnail + DNA signature + metadata carry preset identity |
| 9 | **No tier gating** | Everything free; `tier` field kept as descriptive provenance only |
| 10 | **Schema v1 → v2 migration** with forward-compatible reader | Never break existing preset files |
| 11 | **Fail-soft error handling** with Settings › Library Health reporting | One toast per session; issues logged for user inspection |

---

## 3. Information Architecture

### 3.1 The three surfaces

| Surface | Purpose | Invocation | Primary answer |
|---------|---------|------------|----------------|
| **Quick-pick** | In-flow preset switch inside the current engine | Click preset name in engine header, or keyboard shortcut `⌘/` | "Give me another OPERA sound, now" |
| **Palette** | Global text search across the entire fleet | `⌘P` / `Ctrl+P` from anywhere | "Find that one preset by name / tag / keyword" |
| **Discover** | Exploration across the fleet; the hero | Click ocean/world icon in top bar, or "Discover" tab | "Show me something I don't know yet" |

### 3.2 Invocation hierarchy

By discoverability:
1. **Quick-pick** is always one click away (preset-name button in the engine header is the affordance)
2. **Palette** is always one keystroke away (`⌘P` follows VS Code / Linear / Raycast convention)
3. **Discover** is always one tab/button away from anywhere (persistent nav icon in top bar)

No modal-within-modal. Discover is a tab/view, not a popup. Palette is the only overlay.

### 3.3 Shared state (session-global)

Four pieces of state cross all three surfaces, persisted via `juce::PropertiesFile`:

- **Favorites** — star toggle visible everywhere; flipping in any surface reflects instantly in the others
- **Recent** — persisted list of last 50 presets loaded (up from current 20). Each surface displays a subset: Quick-pick shows top 5 from the session; Discover Chamber and Atlas show the full 50 in a "Recent" collection; Palette ranks recent matches higher in fuzzy search results.
- **Current preset** — highlighted consistently; A/B comparison state lives here
- **Collection pins** — user-pinned editorial collections ("Kitchen: Fusion") show at top of Discover

### 3.4 Not decided here (deferred to downstream specs / implementation)

- Exact color palette for mood dots (TIDEsigns / UIX Studio)
- Motion curves for Ocean ⇄ Chamber transitions
- Localization / text strings
- Touch vs pointer gesture differentiation

---

## 4. Discover (the hero surface)

Four modes, navigable from a persistent top-bar: **[Ocean] [Atlas] [Reef]**. The Chamber mode is entered by tapping a creature on the Ocean.

### 4.1 Mode 1: Ocean (top-level)

**Purpose**: Navigation + wonder. No preset cards at this level.

**Visual grammar**:
- Engines rendered as creatures in water-column positions matching aquatic mythology (Surface / Twilight / Abyss bands)
- Creature size hints at preset density (more presets → larger creature)
- Creature glow hints at recent activity (engines you've touched recently glow softer gold)
- Depth bands labeled with mythology names, faint type

**Interactions**:
- Tap a creature → **dive** (transition to Chamber)
- Hover a creature → show engine name, preset count, last-used timestamp
- Drag (optional, future) → rearrange creature positions within their water column
- Drag a creature onto another → preview the coupling arc (educational hint toward Coupling Reef)

**What the Ocean is NOT**:
- Not a preset-browsing surface
- Not a filter surface
- Not where you A/B compare sounds

### 4.2 Mode 2: Chamber (dove into a single engine)

**Purpose**: Browse a single engine's presets with full filter control.

**Anatomy**:
- Breadcrumb at top: `◂ ORGANON · 150 PRESETS`
- Filter chip row below breadcrumb: `mood · instrument · timbre · coupled · collection · ⋮ more`
- Grid of 120×80 preset thumbnail cards below (see `ecological-interface-component-spec-v1.md`)
- Active preset highlighted (gold border + soft glow)
- Preset cards display: mood color dot (top-left), coupling marker `◇`/`⌧` (bottom-right), preset name

**Interactions**:
- Click a card → load preset
- Right-click a card → context menu: Favorite, A/B, Find Similar (DNA), Copy, Save As, Show Metadata
- Double-tap empty chamber background → exit to Ocean
- `esc` → exit to Ocean
- Keyboard navigation: arrow keys to step between cards

**Coupling markers**:
- `◇` = coupling-dependent preset; both required engines currently loaded → loadable
- `⌧` = coupling-dependent preset; partner engine not loaded → **locked tease** (shows thumbnail, name, but greyed out; tapping shows the "Load [engine]" hint)

**Memory contract**: Chamber remembers its last chip state per engine. Returning to ORGANON remembers you were filtering by `pad`.

### 4.3 Mode 3: Cross-fleet Atlas (pan-engine browse)

**Purpose**: Find presets across engine boundaries — "show me every dark-ambient preset regardless of engine."

**Anatomy**:
- Search bar at top (instance-local, not the global palette — this is for refining the current view)
- Full filter arsenal: mood · category · timbre · engine · coupling · author · date · tempo · DNA
- Dense grid of preset cards from every engine
- Cards have a subtle engine badge (e.g., teal pill with engine short name) in the top-right

**Interactions**:
- Click a card → transition into that engine's Chamber with the preset selected (preserves exploration context)
- Right-click → same context menu as Chamber
- Cards use same visual grammar as Chamber (color dot = mood, coupling marker, name)

**Memory contract**: Atlas remembers its filter state globally across sessions.

### 4.4 Mode 4: Coupling Reef (dedicated destination for paired presets)

**Purpose**: Make the coupling system a destination — a first-class feature, not a hidden metadata field.

**Anatomy**:
- Visual field of paired creatures connected by arcs (e.g., `OUROBOROS × ORBITAL`, `OPERA × ONSET`, `ORGANON × OFFERING`)
- Each arc represents a pair of engines that have at least one coupling-preset between them
- Arc thickness hints at preset count (thicker = more presets in this pair)
- Arc color + motion hint at coupling type (KnotTopology, Braid, Mirror, etc.)
- Pair cards below the visual field, scrollable horizontally:
  `[KNOT·7] [BLOOM·4] [DUET·3] [SOUL·5] [WEAVE·2]` — pair name + preset count
- Locked pairs (either engine not loaded) appear greyed with `⌧`

**Interactions**:
- Click a pair card or arc → open the pair's micro-chamber (same grammar as Chamber, scoped to the 3–7 presets of that pairing)
- Click a locked pair → show "Load [engine A] and [engine B] to unlock" hint, with a one-click "load both" action (loads engines; doesn't auto-load a preset — preserves session sovereignty)

**Memory contract**: Reef remembers nothing — always fresh. Discovery moments stay fresh.

**Strategic note**: Coupling Reef is what turns XOceanus's unique coupling system from a feature into a gameplay loop. "Discover which engine pairs unlock which sounds" becomes a memorable product property, not buried metadata.

### 4.5 Navigation model

```
        ┌────── Ocean ──────┐
        │         │          │
        ▼         ▼          ▼
    Chamber   Cross-fleet   Coupling Reef
                Atlas
```

- Top-bar: `[Ocean] [Atlas] [Reef]` as three toggle tabs
- Chamber is a sub-state of Ocean (entered via dive, tracked with Ocean tab active and breadcrumb overlay)
- Deep-linking into a Chamber (e.g., from Palette `⌘⏎`) sets the Ocean tab active with the Chamber overlay open

### 4.6 Filter chip system (shared between Chamber and Atlas)

Chips are the universal control vocabulary. Each chip is a facet of the peer axes.

| Chip | Behavior | Example |
|------|----------|---------|
| **Mood** | Multi-select of 16 mood tags | `mood: atmosphere + deep` |
| **Category** | Single-select of 10 terms (new taxonomy) | `inst: pad` |
| **Timbre** | Single-select of 8 terms (new; optional) | `timbre: strings` |
| **Coupling** | Toggle: show only coupling-dependent | `coupled` |
| **Collection** | Single-select editorial grouping | `Kitchen: Fusion` |
| **Author** | Single or multi | `author: Guru Bin` |
| **Date** | Preset time filter | `last 30 days` |
| **DNA** | Opens slider panel for 6D similarity | `similar to current` |

Active chip = gold; inactive = teal outline. Clicking a chip cycles its state. "⋮ more" chip opens a full filter drawer with every axis available.

### 4.7 First-time user journey (cold start)

1. User opens Discover → lands on **Ocean**. Sees creatures, depth bands. Immediately feels the world.
2. Curiosity pulls them toward a creature → taps → **Chamber** opens.
3. Sees 150 preset cards. Default chip state (`mood: all`, `inst: any`). Plays several. Favorites one.
4. Double-taps background to return to Ocean. Spots the **Reef** tab, tries it. Sees arcs and locked pair cards — hint that more exists to unlock.
5. Plays with coupling in a separate workflow, returns, finds Reef presets now loadable. Aspiration → reward loop.

---

## 5. Quick-pick (in-flow surface)

### 5.1 Purpose & invocation

Switch presets without leaving the engine. Zero context loss.

**Invocation**: click preset name in engine header (with a `▾` affordance), or keyboard `⌘/` *(proposed — see Open Questions)*. Reasoning for `⌘/`: `⌘P` is taken by Palette; `⌘/` is unclaimed in most DAWs and mnemonically "find within here." Needs validation against common DAW hosts (Logic / Ableton / FL / Cubase / Pro Tools) to confirm no conflicts.

### 5.2 Structure

Overlay dropdown anchored to the preset-name button. Three sections:

1. **Current · [ENGINE] · [count]** — all presets for the current engine, with session's last filter applied
2. **Similar · DNA** — top 5 by DNA similarity to the currently-loaded preset (uses the existing 6D Euclidean distance from `PresetBrowser.h`)
3. **Recent** — last 5 loaded in this session, regardless of engine (so you can snap back across engines)

Each row: `[mood dot] [name] [meta]` — compact single-line rows, ~22px tall.

### 5.3 Interactions

- `↑/↓` navigate
- `⏎` load
- `⇧⏎` A/B compare
- `esc` close
- Type any letter → switches to filter mode, typed query scopes the list
- `◇` next to a name = coupling-dependent preset (loadable)
- Coupling-locked presets (`⌧`) are **filtered out of Quick-pick by default** — this is the speed surface, not the discovery surface

### 5.4 Smart promotion

When the current engine has coupling-locked presets whose partner engine is *not* loaded, show a one-line hint at the bottom of Quick-pick:

> `+ 3 more with OPERA`

Clicking the hint shows which locked presets would unlock, with a one-click "load OPERA" affordance. Same session-sovereignty rule as the Reef: loading the partner engine is explicit; no preset loads automatically.

### 5.5 Memory contract

Quick-pick remembers its last filter state per engine. Returning to ORGANON's Quick-pick remembers you were filtering by `pad`.

---

## 6. Palette (global search surface)

### 6.1 Purpose & invocation

Find any preset in the fleet by keystroke.

**Invocation**: `⌘P` / `Ctrl+P` from anywhere in the app. Universal muscle memory from VS Code, Linear, Raycast.

### 6.2 Structure

Centered modal with backdrop blur. Search bar at top, results below, footer with hotkey hints.

Result row anatomy (single line, ~28px):
`[mood dot] [name] [engine pill] [matched tags] [coupling marker]`

Six visual tokens per row, compressed. Color dot + name + engine are primary; tags and couplings are secondary context.

### 6.3 Search grammar

- **Bare text** → fuzzy match across name, tags, description, engine, author
- **Faceted operators** — `mood:deep`, `engine:ORGANON`, `author:guru`, `inst:pad`, `timbre:strings`, `coupling:ouroboros+orbital`, `collection:kitchen`, `tier:transcendental`
- **Operators combine** — `mood:deep pad organ` → deep-mood + pad-category + "organ" fuzzy-match in name/tags/timbre
- **`⌘K`** opens an operator help sheet (inline, not a separate modal)

**Fuzzy matching**:
- Levenshtein distance ≤ 2 for short queries (≤5 chars)
- Trigram overlap ≥ 0.4 for longer queries
- Name matches rank higher than tag matches; exact-prefix outranks fuzzy

### 6.4 Loading behavior

- `⏎` → loads preset in its engine (engine context switches if needed; confirms once in a session if about to leave unsaved work)
- `⌘⏎` → opens the Chamber for that preset's engine in Discover with preset selected (exploration mode — doesn't load)
- `⇧⏎` → A/B with current
- Locked coupling presets render with `⌧`; selecting them shows the "load [engine]" hint instead of loading

### 6.5 Performance contract

Palette must respond in **< 50ms** for any search on 19,885 presets. See Section 8.4 for how the index meets this.

---

## 7. Data Model

### 7.1 Instrument category taxonomy (10 terms, required)

| Term | What fits |
|------|-----------|
| `keys` | Pianos, organs, electric pianos, plucked keys |
| `pads` | Sustained, lush, atmospheric beds |
| `leads` | Monophonic melodic voices |
| `bass` | Sub, mid, growl — low-register forward |
| `drums` | Kicks, snares, hats, toms — tuned drum hits |
| `perc` | Non-kit percussion — hand drums, chimes, bells, hits |
| `textures` | Evolving, non-tonal atmospheres; sound design |
| `fx` | Risers, impacts, transitions, shaped drones |
| `sequence` | Arps, pattern-generators, self-playing patches |
| `vocal` | Vox-adjacent — vowel filters, formant synthesis |

**Why 10 and not more**: a user scanning 10 chips can hold the whole list in working memory. Moods are 16 because they're emotional and overlap; categories are functional and should be tight.

### 7.2 Timbre taxonomy (8 terms, optional/nullable)

| Term | What fits |
|------|-----------|
| `strings` | Violin, cello, ensemble strings, bowed, arco |
| `brass` | Trumpet, trombone, horn section, tuba |
| `wind` | Flute, clarinet, oboe, reed, breath-driven |
| `choir` | Choral, vocal pads (overlaps `vocal` category when polyphonic sustained) |
| `organ` | Pipe, drawbar, church, Hammond-style |
| `plucked` | Guitar, harp, koto, zither — pluck-excitation physics |
| `metallic` | Bells, tines, gongs, hammered-dulcimer |
| `world` | Kora, duduk, shakuhachi, sitar, guzheng |

**Optional** because the vast majority of XOceanus presets are electronic/synthetic with no acoustic emulation target — `timbre: null` is the default. Populated only when a preset has a clear real-world timbral reference.

### 7.3 Schema v2 (`.xometa`)

```diff
 {
   "schema_version": 2,          // bumped 1 → 2
   "name": "HOLLOW ORGAN",
   "mood": "Deep",
+  "category": "pads",           // NEW — required enum of 10
+  "timbre": "organ",            // NEW — optional nullable enum of 8
   "engines": ["ORGANON"],
   "author": "Guru Bin",
   "version": "1.0",
   "description": "A cavernous pipe-organ drone with breath noise...",
   "tags": ["deep","drone","organ","cavern"],
   "tier": "awakening",          // KEEP — descriptive only, no access-control
   "macroLabels": [...],
   "couplingIntensity": "None",
   "dna": { "brightness": 0.2, "warmth": 0.8, "movement": 0.3, "density": 0.7, "space": 0.9, "aggression": 0.1 },
   "coupling": { "pairs": [] },
   "parameters": {...},
   "macroTargets": [...],
   "macros": {...}
 }
```

**Two fields added** (`category`, `timbre`). **One field clarified** (`tier`). Version bump to `2` signals the migration.

**Reader behavior for v1 presets** (schema_version omitted or = 1): treated as `category: null, timbre: null`. Appears in unfiltered results; hidden from category/timbre chip filters until backfilled.

### 7.4 Instrument + timbre backfill plan

One-time project. Pattern is exactly what `/preset-audit` exists for.

1. **Propose phase** (1 sonnet session): agent reads every `.xometa`, inspects `tags[]` + `name` + `description` + `engine` + `dna` signature, proposes `category` + `timbre` with confidence score. Outputs `Docs/fleet-audit/instrument-taxonomy-proposal.csv`.
2. **Review phase** (user, ~2-3 hours): spot-check ~500 of 19,885 rows. Focus on low-confidence rows (est. 10-15% of library).
3. **Apply phase** (1 sonnet session): agent applies the approved `category` + `timbre` to every `.xometa`. Single commit.

**Timbre is mostly null**: estimate 5-15% of library has a clear acoustic emulation reference. Backfill respects this — most rows remain `timbre: null`.

### 7.5 Thumbnail generation pipeline

The `ecological-interface-component-spec-v1.md` spec defined 120×80 generative thumbnails (engine creature silhouette + coupling rings + mood color wash + name strip). Status: spec'd, not implemented.

**Implementation**:
- Offline batch renderer (Python standalone or JUCE-side C++ tool) reads `.xometa` → writes `{preset-name}.png` alongside
- Triggers:
  - Preset save (live, per-preset)
  - Taxonomy backfill (batch, one-time)
  - Schema migration (batch, one-time)
  - User-triggered "Regenerate thumbnails" button in Settings
- Storage: alongside `.xometa` file. Same basename + `.png`. ~19,885 thumbnails × ~8KB each ≈ 160 MB. Acceptable for a desktop plugin.
- Cache hydration: if PNG missing at browse time, card falls back to inline-rendered silhouette + mood color wash, and a lazy thumbnail job is queued

### 7.6 Search index (for Palette and filter chips)

Built at app launch; cached to disk keyed on SHA1 of preset-file mtime list. Skip rebuild if nothing changed.

```cpp
struct PresetIndex {
  std::vector<PresetRecord> records;              // all presets
  std::unordered_map<String, Bitset> byMood;      // 16 entries
  std::unordered_map<String, Bitset> byCategory;  // 10 entries
  std::unordered_map<String, Bitset> byTimbre;    // 8 entries + null
  std::unordered_map<String, Bitset> byEngine;    // 73 entries
  std::unordered_map<String, Bitset> byAuthor;
  std::unordered_map<String, Bitset> byCollection;
  Trie nameIndex;                                 // fuzzy prefix search
};
```

**Performance target**: ≤ 50ms for any search on 19,885 records. Achieved via pre-tokenized lowercase strings + faceted bitset intersection.

### 7.7 Coupling discovery index (for smart promotion + Reef)

```cpp
struct CouplingIndex {
  std::unordered_map<EnginePair, std::vector<PresetId>> byPair;
  std::unordered_map<EngineId, std::vector<PresetId>> missingPartner;
};
```

Built from `coupling.pairs[]` across all presets at app launch. O(N) build.

**Drives three features**:
- **Smart promotion in Quick-pick** — if user is on ORGANON and 3 coupling presets need OPERA, show "+ 3 more with OPERA" hint
- **Coupling Reef population** — iterate `byPair` to draw arcs and pair cards
- **Cross-fleet Atlas "coupled" chip** — filter to only coupling presets using `byPair` union

### 7.8 Persistence surface (mostly existing)

Extends the existing `juce::PropertiesFile` setup in `PresetBrowser.h`.

Changes:
- `recent`: max 20 → max 50 (shared across all three surfaces)
- **Add** `collection_pins` — list of editorial-collection IDs pinned by the user
- **Add** `quickpick_last_filter_per_engine` — map of engineId → filter state
- **Add** `atlas_last_filter_state` — global atlas filter memory
- **Add** `chamber_last_filter_per_engine` — map of engineId → chamber filter state

---

## 8. Edge Cases, Error Handling, Testing

### 8.1 Edge cases

| Case | Behavior |
|------|----------|
| Preset references engine that doesn't exist in this build | Shows in Atlas with `⚠` marker + "unavailable in this build" tooltip. Not loadable. Filtered out of Quick-pick. |
| Coupling target engine missing (partner not loaded) | `⌧` locked card. Smart-promotion hint in Quick-pick if user has the complement. |
| Thumbnail PNG missing | Fallback: inline-rendered silhouette + mood color wash. Background regeneration job queued. |
| Schema v1 preset (missing `category` / `timbre`) | Treated as nulls. Appears in unfiltered results. Hidden from category/timbre chip filters until backfill. |
| Community preset with malformed JSON | Rejected at index build. Logged to `~/.xoceanus/preset-import-errors.log`. One toast: "3 presets failed to load — see Settings › Library Health." |
| Engine has zero presets yet | Chamber shows empty-state creature illustration + "This engine is new — no presets yet. Make the first." CTA |
| Search index build fails (corrupted cache) | Delete cache + rebuild on next launch. No user-visible error. |
| User has zero favorites / recents | Those sections in Quick-pick hide entirely (not "0 results" — simply absent) |
| Extremely long preset name (> 30 chars) | Validator rejects at save. Existing overlong names truncate with ellipsis + full name in tooltip. |
| Preset with `coupling.pairs[]` whose engines don't match `engines[]` | Marked invalid at index build, excluded from Coupling Reef. Logged. |
| User loads preset from Palette while engine has unsaved changes | Standard "Unsaved changes — Load anyway? [Load / Cancel]" modal |

### 8.2 Error-handling philosophy

**Fail soft, surface once**:
- No preset-system error ever crashes the audio engine or the plugin
- At most one toast per session summarizing library-health issues
- Settings › Library Health lists every issue with per-preset remediation (re-download / delete / re-save)
- All errors logged to disk for user-initiated debugging

**Migration safety**:
- Schema version bump is forward-only; reader tolerates v1 presets indefinitely
- First launch after upgrade: one-shot prompt — "XOceanus now categorizes presets by instrument and timbre. Migrate your custom presets? [Yes / Later]"
- If user defers, the prompt reappears once per week until actioned or dismissed permanently

### 8.3 Testing hooks

**Unit tests** (sonnet can write):
- `PresetSchemaValidator` — every `.xometa` in fleet round-trips through validator
- `PresetIndex` — search latency benchmark: <50ms on 19,885 records
- `CouplingIndex` — all `coupling.pairs[]` entries resolve to existing engines
- `ThumbnailRenderer` — golden-image comparison on a fixed sample set (~20 representative presets)
- `TaxonomyMigration` — v1 → v2 round-trip on a representative sample

**Integration tests**:
- Load preset with missing engine → graceful fallback, no crash
- Rapid ↑↓ cycling through 150 Quick-pick presets → no audio glitch (tests voice management)
- Apply all 6 filter chips in Atlas, clear → search latency stays <100ms
- Coupling Reef with zero couplings currently possible → renders all locked pairs correctly

**Performance gates** (block merge):
- Search index build: **<2s cold** on 19,885 presets
- Thumbnail batch regenerate: **<5min** on full library
- First-open Discover to rendered Ocean: **<300ms**

**Manual QA** (you or TIDEsigns):
- Visual polish of Chamber / Atlas / Reef transitions
- Coupling Reef arc rendering at various fleet-load states
- Accessibility pass — WCAG 2.1 AA, matching existing `PresetBrowser.h` standard

**Ongoing library health**:
- `/preset-audit` skill runs nightly against the preset library
- Reports duplicates, schema violations, orphan coupling references, low-confidence taxonomy entries, thumbnail freshness
- Surfaces regressions before ship

---

## 9. Out of Scope (deferred to later specs)

- Community preset submission / moderation workflow
- Preset export format for sharing (`.xometapack`?)
- Collaboration: "producer A shared their Kitchen with producer B"
- Mobile (OBRIX Pocket) preset picker — separate concern per `obrix-pocket-design.md`
- iPad Academy preset picker — separate concern
- Hardware controller (Push, Maschine, MPC) preset picker ergonomics — see `/hardware-expander` skill
- Preset rating / review system
- AI-assisted "create similar" preset generation

---

## 10. Open Questions (for implementation-plan writer)

- Exact interaction model for the Ocean "drag creature onto creature → preview coupling" hint — is this discoverable enough, or does it need a tutorial?
- Should Collection pins be limited to a max count (e.g., 5) to prevent user from pinning everything?
- Should the Atlas search bar's state survive window-close, or reset per session? (I've assumed survive; verify with user.)
- Thumbnail generation at preset-save — blocking (user waits for render) or async (save immediately, render in background)? Assume async unless render is <100ms.
- For community preset validation errors: single session-wide toast, or one toast per error? (Assumed single, summary toast.)
- Validate the `⌘/` Quick-pick shortcut against DAW hosts (Logic, Ableton Live, FL Studio, Cubase, Pro Tools, Reaper, Bitwig) for conflicts before locking. Fallback candidates if conflicted: `⌥P`, `⌘;`, `⌘⇧P`.
- "Unsaved changes" modal in §8.1: does XOceanus currently have a standard unsaved-state dialog, or is this the first time we need one? If first time, defer to a separate UX spec; if existing pattern, reuse it.

---

## 11. Implementation Phasing Suggestion

Handoff hint for the next spec consumer (`/superpowers:writing-plans`):

1. **Phase 1: Schema + backfill** — schema v2 definition, reader tolerance, sonnet-driven category + timbre backfill, migration prompt
2. **Phase 2: Index infrastructure** — PresetIndex + CouplingIndex + thumbnail renderer (no UI changes yet)
3. **Phase 3: Palette** — ⌘P search modal; smallest surface, biggest proof of search index
4. **Phase 4: Quick-pick upgrade** — refactor current in-header dropdown to match spec (sections, filter, smart promotion, coupling markers)
5. **Phase 5: Discover — Ocean + Chamber** — the hero pair; replaces current JUCE preset browser
6. **Phase 6: Discover — Cross-fleet Atlas**
7. **Phase 7: Discover — Coupling Reef**
8. **Phase 8: Polish** — TIDEsigns + UIX Studio pass; motion; accessibility audit
9. **Phase 9: Automation** — `/preset-audit` CI integration; Library Health settings surface

Each phase should be buildable and shippable behind a feature flag so partial rollout is possible.
