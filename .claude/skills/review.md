# /review — Code Review Checklist

Structured code review tailored for audio/synth plugin development. Catches architecture violations, audio thread safety issues, and XOmnibus-specific gotchas.

## Usage

- `/review` — Review all uncommitted changes (staged + unstaged)
- `/review {file}` — Review a specific file
- `/review --pr` — Review changes between current branch and main
- `/review --engine {name}` — Review with engine-specific checks

## Process

### 1. Gather changes

Depending on mode:
- Default: `git diff` + `git diff --cached` (all uncommitted changes)
- File: read the specified file in full
- PR: `git diff main...HEAD`

### 2. Run checklist

Each item is checked against the changed code. Items are **pass**, **fail**, or **N/A**.

#### Audio Thread Safety
- [ ] No `new`/`delete`/`malloc`/`free` in processBlock or render methods
- [ ] No file I/O (`fopen`, `ifstream`, `std::filesystem`) in audio path
- [ ] No `std::mutex`, `std::lock_guard` in audio path (use atomics or lock-free)
- [ ] No `throw`/`try`/`catch` in audio path
- [ ] No `std::string` construction in audio path (allocates)
- [ ] No `std::vector::push_back` in audio path (may reallocate)

#### Parameter Conventions
- [ ] New parameter IDs use correct engine prefix (snap_, morph_, dub_, etc.)
- [ ] No parameter ID renames (frozen after release)
- [ ] Parameter ranges are sensible (0-1 normalized, or meaningful physical units)
- [ ] ParamSnapshot pattern used (cache pointers per block, not per sample)

#### DSP Quality
- [ ] Denormal protection on all feedback paths and IIR filters
- [ ] No division by zero risk (check denominators)
- [ ] Output is bounded (no unbounded gain chains without limiting)
- [ ] Buffer sizes handled correctly (don't assume fixed block size)
- [ ] Sample rate changes handled (recalculate coefficients)

#### Engine Integration
- [ ] Engine implements full SynthEngine interface
- [ ] Engine registered via REGISTER_ENGINE() macro
- [ ] 50ms crossfade used for engine hot-swap transitions
- [ ] Engine is testable without UI (DSP only, no component references)

#### Coupling Compatibility
- [ ] Coupling inputs/outputs declared for MegaCouplingMatrix
- [ ] Coupling types are valid enums
- [ ] Coupling doesn't create unbounded feedback loops

#### Preset Compatibility
- [ ] Changes don't break existing .xometa loading
- [ ] New parameters have sensible defaults for existing presets
- [ ] Parameter removal (if any) handled gracefully in PresetManager

#### General Code Quality
- [ ] No hardcoded magic numbers without explanation
- [ ] Error handling at system boundaries (file I/O, external input)
- [ ] No OWASP top 10 vulnerabilities (injection, XSS, etc.)
- [ ] Memory ownership is clear (RAII, unique_ptr, no raw owning pointers)

### 3. Report

```markdown
## Code Review
- **Scope:** {uncommitted / file / PR}
- **Files Changed:** {count}
- **Lines Changed:** +{added} / -{removed}
- **Verdict:** APPROVE / REQUEST CHANGES / COMMENT

### Checklist Results
| Category | Pass | Fail | N/A |
|----------|------|------|-----|
| Audio Thread Safety | {n} | {n} | {n} |
| Parameter Conventions | {n} | {n} | {n} |
| DSP Quality | {n} | {n} | {n} |
| Engine Integration | {n} | {n} | {n} |
| Coupling | {n} | {n} | {n} |
| Preset Compat | {n} | {n} | {n} |
| General Quality | {n} | {n} | {n} |

### Issues Found
1. **[SEVERITY]** {file}:{line} — {description}
   - Why: {explanation of the risk}
   - Fix: {suggested fix}

### Observations (non-blocking)
- {Optional design comments, style notes, or improvement ideas}
```

### 4. Severity levels

- **CRITICAL** — Will cause crashes, data loss, or audio glitches (audio thread violations, unbounded output)
- **HIGH** — Will cause bugs or break compatibility (wrong param prefix, missing denormal protection)
- **MEDIUM** — Code smell or maintenance risk (magic numbers, unclear ownership)
- **LOW** — Style or minor improvement (naming, comments)

## Notes
- Audio thread safety issues are always CRITICAL — no exceptions
- Parameter ID violations are always HIGH — frozen IDs are a hard rule
- This review is additive to manual review, not a replacement
- When reviewing engine-specific code, load the engine's sound design guide for context
