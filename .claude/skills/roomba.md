# /roomba — Automated Repo Health Sweep

Deep, comprehensive repo health scan that finds inconsistencies, drift, and technical debt across the entire XOmnibus codebase. Like a Roomba — thorough, systematic, and leaves things cleaner than it found them.

## Usage

- `/roomba` — Full sweep: registration, presets, docs, build, DSP safety, dead code
- `/roomba --fix` — Fix all auto-fixable issues (safe fixes only, no breaking changes)
- `/roomba presets` — Preset-focused sweep only
- `/roomba engines` — Engine registration + parameter consistency only
- `/roomba docs` — Documentation drift detection only
- `/roomba quick` — Fast pass: critical issues only (registration, build, broken presets)

## Full Sweep Checklist

Run every check below in order. Collect all findings, then produce a single report at the end.

### Phase 1: Engine Registration Integrity

1. **Registry Completeness**
   - Read `Source/XOmnibusProcessor.cpp` — extract all `registerEngine("CanonicalName", ...)` calls
   - Read every `Source/Engines/*/Engine*.cpp` — extract all `REGISTER_ENGINE()` calls
   - Cross-reference: every engine directory must have a canonical registration in the Processor
   - Canonical names must match what `getEngineId()` returns in each engine header
   - Flag: engines registered only via REGISTER_ENGINE (class name) without canonical Processor registration
   - Flag: duplicate registrations (same engine registered with both class name and canonical name)

2. **ValidEngineNames Sync**
   - Read `Source/Core/PresetManager.h` — extract `validEngineNames` list
   - Every canonically-registered engine must appear in validEngineNames
   - Every engine in validEngineNames must have a matching registration
   - Flag: engines missing from validEngineNames

3. **Engine Alias Coverage**
   - Read `resolveEngineAlias()` in PresetManager.h
   - Every legacy name variant (XFoo, short name) should have an alias entry
   - Flag: engines with known legacy names that lack alias resolution

### Phase 2: Parameter Prefix Consistency

1. **Code vs Documentation**
   - For each engine, grep the header for the first parameter ID string pattern
   - Extract the actual prefix (everything before the first `_`)
   - Compare against CLAUDE.md's "Engine ID vs Parameter Prefix" table
   - Flag: any mismatch between documented and actual prefix

2. **Preset Parameter Audit**
   - For a random sample of 50 presets (or all if fast enough), parse JSON
   - For each engine's parameter block, verify param keys use the correct prefix
   - Tally: correct prefix count vs wrong/missing prefix count per engine
   - Flag: engines where >10% of preset params use wrong prefix (likely legacy migration gap)

### Phase 3: Preset Health

1. **Schema Validation**
   - Every .xometa must parse as valid JSON
   - Required fields: schema_version, name, mood, engines, parameters
   - Mood must be in: Foundation, Atmosphere, Entangled, Prism, Flux, Aether
   - Engine names must resolve to valid engines via resolveEngineAlias

2. **Coverage Matrix**
   - Count presets per engine — flag engines with zero presets
   - Count presets per mood — flag moods significantly under-represented
   - Flag: presets referencing engines that don't exist

3. **Quality Checks**
   - Duplicate preset names within same mood
   - Preset names exceeding 30 chars
   - Missing or empty description fields
   - Tags that look like typos (e.g., "dyssey" instead of "Odyssey")

### Phase 4: Build System Integrity

1. **CMakeLists.txt Sync**
   - Find all .h and .cpp files under Source/
   - Compare against files listed in CMakeLists.txt target_sources
   - Flag: files that exist but aren't in CMake (will be invisible to build)
   - Flag: files listed in CMake that don't exist (will break build)

2. **Include Path Verification**
   - Check all target_include_directories entries point to existing paths
   - Check for any #include in engine headers that reference files outside the repo

### Phase 5: DSP Safety

1. **Audio Thread Rules**
   - In all renderBlock/processBlock implementations, grep for:
     - `new ` / `delete ` / `malloc` / `free` / `calloc` / `realloc`
     - `fopen` / `ifstream` / `ofstream` / any file I/O
     - `std::vector::push_back` / `resize` / any growing container ops
     - `std::mutex::lock` / any blocking synchronization
   - Flag: any allocation, I/O, or blocking in audio path

2. **Denormal Protection**
   - Check all IIR filters and feedback paths for denormal guards
   - Patterns: `+ 1e-25f`, `flushDenormals`, `FTZ/DAZ`, `std::fpclassify`

3. **Parameter Caching**
   - Check that renderBlock implementations cache parameter pointers per-block
   - Flag: repeated `getRawParameterValue()` calls inside sample loops

### Phase 6: Documentation Drift

1. **CLAUDE.md Accuracy**
   - Engine count matches actual engine directories
   - Engine table entries match actual code
   - Parameter prefix table matches actual prefixes
   - Key Files table — verify every listed path exists
   - Accent colors — verify documented colors match getAccentColour() returns

2. **Master Spec Drift**
   - If Docs/xomnibus_master_specification.md exists, check engine count claims
   - Cross-reference any spec claims about features vs actual implementation state

3. **Stale Docs**
   - Flag docs that reference engines/features that no longer exist
   - Flag docs that reference old engine names without noting the rename

### Phase 7: Dead Code & Cleanup

1. **TODO/FIXME/HACK Inventory**
   - Grep all source files for TODO, FIXME, HACK, XXX, TEMP, WORKAROUND
   - Categorize by severity and location

2. **Orphaned Files**
   - Files in Source/ not referenced by CMake or any #include
   - Scripts in Tools/ that reference outdated engine names

3. **Git Hygiene**
   - Uncommitted changes
   - Untracked files that should be .gitignored

## Auto-Fix Mode (--fix)

When `--fix` is set, automatically repair these categories (safe, non-breaking):

- Add missing engines to validEngineNames in PresetManager.h
- Add missing source files to CMakeLists.txt
- Fix preset tag typos (when clearly a misspelling of a known engine name)
- Fix obvious documentation discrepancies in CLAUDE.md (engine counts, prefix table)

NEVER auto-fix:
- Parameter IDs in engine code (frozen, breaking change)
- Preset parameter keys (would break existing user presets)
- Engine registration order or naming (requires architectural review)
- Accent colors (brand decision)

## Report Format

```markdown
# XOmnibus Roomba Report
**Date:** {date} | **Mode:** {full/quick/focused} | **Branch:** {branch}

## Health Score: {X}/100

### Critical (blocks runtime)
{numbered list of P0 issues}

### High (data integrity / drift)
{numbered list of P1 issues}

### Medium (consistency / maintainability)
{numbered list of P2 issues}

### Low (cleanup / polish)
{numbered list of P3 issues}

### Engine Coverage Matrix
| Engine | Registered | Valid Names | Presets | MPE | Prefix Match | Color Match |
|--------|-----------|-------------|---------|-----|-------------|-------------|
{row per engine}

### What's Working Well
{positive findings — important for morale}

### Recommended Actions (priority order)
1. {action with file path and line numbers}
2. ...

### Auto-Fixed (if --fix)
{list of changes made}
```

## When to Use

- After any significant coding session (multiple engines touched)
- Before opening a PR
- Weekly maintenance sweep
- After merging a feature branch
- When you suspect drift between streams of work
- Any time you feel "things might be out of sync"

## Notes

- This skill is thorough by design — it takes longer than /lint or /preflight
- The health score is calculated as: 100 - (P0 * 15) - (P1 * 5) - (P2 * 2) - (P3 * 0.5)
- Run with `--fix` to handle the easy stuff, then manually address what remains
- The Engine Coverage Matrix is the single most useful output — check it first
