# /release — Packaging, Versioning & Release Checklist

Structured release workflow: version bump, changelog, final validation, build all formats, tag.

## Usage

- `/release patch` — Bump patch version (bug fixes)
- `/release minor` — Bump minor version (new features, engines)
- `/release major` — Bump major version (breaking changes)
- `/release --check` — Run release readiness check without bumping
- `/release --changelog` — Generate changelog from git history

## Process

### 1. Release readiness check

Run these checks before any version change:

**Code quality:**
- [ ] `/lint` passes clean — no architecture violations
- [ ] `/test` passes — all tests green
- [ ] `/benchmark` passes — no performance regressions
- [ ] `/debug-audio` passes — no DSP health issues

**Preset quality:**
- [ ] `/preset-qa` passes — all factory presets valid
- [ ] All 6 mood categories have balanced preset counts
- [ ] No preset naming conflicts or duplicates
- [ ] Sonic DNA values present and reasonable

**Build verification:**
- [ ] `/build` succeeds for all target formats:
  - macOS: AU, Standalone
  - iOS: AUv3, Standalone
  - VST3 (v2)
- [ ] No new compiler warnings introduced

**Documentation:**
- [ ] `Docs/specs/xoceanus_master_specification.md` reflects current state
- [ ] CLAUDE.md engine tables are current
- [ ] Migration reference is up to date
- [ ] Any new engines have sound design guide entries

**Migration safety:**
- [ ] `/migrate --audit` clean — no orphaned params or stale aliases
- [ ] All parameter IDs stable (no renames without aliases)
- [ ] Preset format version matches latest migration chain

### 2. Changelog generation

Scan git history since last tag:

```markdown
## Changelog v{version}

### New Engines
- {engine}: {one-line description}

### New Features
- {feature}: {one-line description}

### Improvements
- {improvement}: {one-line description}

### Bug Fixes
- {fix}: {one-line description}

### Preset Updates
- Added {n} new presets ({mood breakdown})
- Updated {n} existing presets

### Breaking Changes
- {change}: {migration path}
```

Categorize commits by conventional-commit-style prefixes or by file paths:
- `Source/Engines/` → Engine changes
- `Source/DSP/` → DSP improvements
- `Source/UI/` → UI changes
- `Presets/` → Preset updates
- `Source/Core/` → Core/architecture changes

### 3. Version bump

Update version in all locations:
- `CMakeLists.txt` (project version)
- Plugin format metadata (AU, AUv3, VST3)
- Any version constants in source code
- `framework-manifest.json` if applicable

### 4. Final build & tag

```bash
# Clean build all formats
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Tag the release
git tag -a v{version} -m "Release v{version}"
```

### 5. Release report

```markdown
## Release: v{version}
- **Type:** patch / minor / major
- **Date:** {date}
- **Engines:** {count} ({list any new})
- **Presets:** {count} factory presets
- **Formats:** AU, AUv3, VST3, Standalone (macOS/iOS)

### Readiness Checklist
| Check | Status | Notes |
|-------|--------|-------|
| Lint | ✓/✗ | |
| Tests | ✓/✗ | |
| Benchmarks | ✓/✗ | |
| Audio health | ✓/✗ | |
| Preset QA | ✓/✗ | |
| Builds | ✓/✗ | |
| Docs | ✓/✗ | |
| Migration audit | ✓/✗ | |

### Changelog
{generated changelog}

### Known Issues
- {any issues deferred to next release}
```

## Primitives Used
- **validate-fix-loop** — Iteratively fix any check failures
- **chain-pipeline** — Sequential: lint → test → benchmark → build → tag

## Relationship to Other Skills
- Depends on: `/lint`, `/test`, `/benchmark`, `/debug-audio`, `/preset-qa`, `/build`, `/migrate`
- Fed by: `/recommend` (identifies what should ship)
- This is the terminal skill in the development lifecycle

## Notes
- Never skip the readiness check — it exists to prevent shipping broken builds
- Changelog should be human-readable, not a git log dump
- Tag only after all checks pass
- If any check fails, fix the issue and re-run — don't override
- Major version bumps require extra scrutiny on breaking changes and migration paths
