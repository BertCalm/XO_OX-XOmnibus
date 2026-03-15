# /migrate — Structured Version Migration

Safely migrate parameter IDs, preset formats, engine names, and other versioned artifacts. Handles the tension between "parameter IDs are frozen" and "things need to evolve."

## Usage

- `/migrate params {old} {new}` — Migrate a parameter ID (with alias registration)
- `/migrate engine {old} {new}` — Migrate an engine name (with alias registration)
- `/migrate preset-format {version}` — Upgrade .xometa format to a new schema version
- `/migrate --dry-run` — Show what would change without modifying anything
- `/migrate --audit` — Check for stale aliases, orphaned params, format mismatches

## Process

### 1. Migration analysis

Before touching anything, map the blast radius:

**What references the old name?**
- Source code: parameter registration, DSP access, UI bindings
- Presets: all `.xometa` files using the old ID
- Tests: test fixtures and assertions
- Docs: spec, sound design guides, CLAUDE.md tables
- Tools: Python utilities that reference the ID
- External: any published documentation or user-facing materials

**What's the compatibility contract?**
- Parameter IDs: **frozen after release** — old IDs must remain as aliases
- Engine names: resolved via `resolveEngineAlias()` in `PresetManager.h`
- Preset format: must load all previous versions (migration on load)
- Coupling routes: engine ID changes affect coupling matrix addressing

### 2. Migration plan

Generate a migration plan before executing:

```markdown
## Migration Plan: {old} → {new}
- **Type:** param / engine / preset-format
- **Blast radius:** {n} files, {n} presets, {n} tests

### Changes
| File | Line | Change | Reversible? |
|------|------|--------|-------------|
| {path} | {line} | {description} | yes/no |

### Alias Registration
- Old name: `{old}` → resolves to → `{new}`
- Registration point: {file:line}

### Preset Migration
- {n} presets reference old name
- Strategy: migrate-on-load / batch-rewrite / both

### Rollback Plan
- {How to undo if something goes wrong}
```

### 3. Execute migration (unless --dry-run)

**For parameter migrations:**
1. Add the new parameter ID alongside the old one
2. Register the old ID as an alias in the parameter resolution system
3. Update all internal code to use the new ID
4. Update preset loading to map old → new on load
5. Keep the old ID in the alias table permanently (frozen contract)
6. Run `/test` to verify nothing broke

**For engine name migrations:**
1. Add new engine ID to `EngineRegistry.h`
2. Register old name in `resolveEngineAlias()` in `PresetManager.h`
3. Update UI display names
4. Update coupling matrix references
5. Update CLAUDE.md engine tables
6. Run `/test` and `/preset-qa` to verify

**For preset format migrations:**
1. Bump the format version in the schema
2. Write a migration function: `migrateV{n}ToV{n+1}()`
3. Chain it into the version migration pipeline in `PresetManager.h`
4. Batch-convert factory presets to new format
5. Verify old-format presets still load correctly
6. Run `/preset-qa` on all factory presets

### 4. Verification

After migration:
- [ ] All tests pass (`/test`)
- [ ] All factory presets load without warnings (`/preset-qa`)
- [ ] Old names resolve to new names (alias test)
- [ ] No orphaned references to old name (grep confirms)
- [ ] Docs updated (CLAUDE.md, spec, migration reference)
- [ ] `Docs/xomnibus_name_migration_reference.md` updated with new mapping

### 5. Audit mode (--audit)

Scan for migration health issues:

```markdown
## Migration Audit
### Stale Aliases
| Alias | Resolves To | Still Referenced? | Can Remove? |
|-------|-------------|-------------------|-------------|

### Orphaned Parameters
| Param ID | Registered? | Used in Code? | Used in Presets? |
|----------|-------------|---------------|-----------------|

### Format Mismatches
| Preset | Format Version | Current Version | Needs Migration? |
|--------|---------------|-----------------|-----------------|
```

## Primitives Used
- **validate-fix-loop** — Run migration, verify with tests, fix issues, re-verify
- **bulk-transform** — Batch preset rewriting for format migrations

## Relationship to Other Skills
- References: `Docs/xomnibus_name_migration_reference.md` (update after each migration)
- Validates with: `/test`, `/preset-qa`, `/lint`
- Related: `/research` for understanding migration impact before executing

## Notes
- **Parameter IDs are sacred** — never delete an old param ID, always alias it
- `resolveEngineAlias()` is the single point of engine name resolution
- Preset format migrations must be chained (V1→V2→V3, not V1→V3)
- Always update the migration reference doc as the last step
- `--dry-run` should be the default mindset — analyze before acting
