---
name: architect
description: Review and approve proposed code changes before implementation. Use as a quality gate before applying fixes, enhancements, refactors, or new features to prevent conflicts, regressions, and inconsistent quality across the XOmnibus codebase.
argument-hint: [change-description e.g. "D001 velocity fix for BOB engine", "add breathing LFO to FAT"]
---

# XOmnibus Architect Review Gate

Review the proposed change **$ARGUMENTS** against the full XOmnibus governance framework — Doctrines, Blessings, Debates, Brand Rules, Architecture Rules, and cross-engine consistency — before implementation.

## When to Invoke

- Before modifying any engine's DSP path (renderBlock, processSample, voice structs)
- Before adding new parameters, LFOs, or modulation sources
- Before changing shared DSP classes (CytomicSVF, FastMath, MegaCouplingMatrix)
- Before bulk fleet-wide fixes that touch multiple engines
- Before modifying preset schema or parameter IDs
- Before any change that could affect sonic character or user-facing behavior

## Governance Principals

Read `CLAUDE.md` for the canonical source. The architect enforces all of the following:

### Brand Rules
- XO + O-word naming convention
- Character over feature count — every feature must support a sonic pillar
- Dry patches must sound compelling before effects
- Presets are a core product feature
- Light mode is primary presentation

### Architecture Rules (Non-Negotiable)
- Never allocate memory on the audio thread
- Never perform blocking I/O on the audio thread
- Never rename stable parameter IDs after release
- All DSP lives inline in `.h` headers; `.cpp` files are one-line stubs
- DSP modules must be testable in isolation
- Export systems must run on non-audio worker threads
- Denormal protection required in all feedback/filter paths
- Engine hot-swap uses 50ms crossfade
- ParamSnapshot pattern: cache all parameter pointers once per block

### The 6 Doctrines (Binding)

| ID | Doctrine | Requirement |
|----|----------|-------------|
| D001 | Velocity Must Shape Timbre | Velocity drives filter brightness / harmonic content — not just amplitude |
| D002 | Modulation is the Lifeblood | Min: 2 LFOs, mod wheel/aftertouch, 4 working macros, 4+ mod matrix slots |
| D003 | The Physics IS the Synthesis | Rigor and citation required for physically-modeled engines |
| D004 | Dead Parameters Are Broken Promises | Every declared parameter must affect audio output |
| D005 | An Engine That Cannot Breathe Is a Photograph | At least one LFO with rate floor <= 0.01 Hz |
| D006 | Expression Input Is Not Optional | Velocity→timbre + at least one CC (aftertouch / mod wheel / expression) |

### The 15 Blessings (Protected)

Changes must not degrade or remove any blessed feature. If a change touches an engine with a blessing, verify the blessing remains intact:

| ID | Blessing | Engine | What to Protect |
|----|----------|--------|----------------|
| B001 | Group Envelope System | ORBITAL | Envelope grouping across partials |
| B002 | XVC Cross-Voice Coupling | ONSET | 8-ghost cross-voice architecture |
| B003 | Leash Mechanism | OUROBOROS | Chaotic system with controllable bounds |
| B004 | Spring Reverb | OVERDUB | Metallic splash character |
| B005 | Zero-Audio Identity | OPTIC | Synthesis without sound |
| B006 | Dual-Layer Blend Architecture | ONSET | Circuit + Algorithm crossfade |
| B007 | Velocity Coupling Outputs | OUROBOROS | Velocity as coupling signal |
| B008 | Five-Macro System | OVERBITE | BELLY/BITE/SCURRY/TRASH/PLAY DEAD |
| B009 | ERA Triangle | OVERWORLD | 2D timbral crossfade |
| B010 | GENDY Stochastic + Maqam | ORACLE | Stochastic synthesis with maqam scales |
| B011 | VFE Metabolism | ORGANON | Variational free energy modulation |
| B012 | ShoreSystem | OSPREY + OSTERIA | 5-coastline cultural data |
| B013 | Chromatophore Modulator | OCEANIC | Bio-inspired color modulation |
| B014 | Mixtur-Trautonium Oscillator | OWLFISH | Novel oscillator topology |
| B015 | Mojo Control | OBESE | Orthogonal analog/digital axis |

### The 4 Ongoing Debates (Respect Both Sides)

Changes should not unilaterally resolve a debate without explicit design decision. If a change touches a debated area, flag it:

| ID | Tension | What to Watch |
|----|---------|---------------|
| DB001 | Mutual exclusivity vs. effect chaining | Don't lock coupling types without discussion |
| DB002 | Silence as paradigm vs. accessibility | Don't remove silence capabilities or force sound |
| DB003 | Init patch: immediate beauty vs. blank canvas | Don't change init patch philosophy without discussion |
| DB004 | Expression vs. Evolution: gesture vs. temporal depth | Don't remove either capability |

## Review Checklist

### 1. Architecture Compliance

- [ ] No memory allocation introduced on audio thread
- [ ] No blocking I/O introduced on audio thread
- [ ] DSP remains inline in `.h` headers (no logic in `.cpp` stubs)
- [ ] Parameter IDs are not renamed (frozen after release)
- [ ] Parameter prefix matches engine table in `CLAUDE.md`
- [ ] Engine implements `SynthEngine` interface correctly

### 2. Doctrine Compliance

For each of the 6 Doctrines, verify the change:
- [ ] **D001** — Preserves or improves velocity→timbre mapping
- [ ] **D002** — Preserves or improves modulation infrastructure
- [ ] **D003** — Maintains physics rigor if applicable
- [ ] **D004** — Does not introduce dead parameters
- [ ] **D005** — Preserves or improves autonomous breathing
- [ ] **D006** — Preserves or improves expression input

### 3. Blessing Protection

- [ ] No blessed feature (B001–B015) is degraded, removed, or broken
- [ ] If touching a blessed engine, verify the specific blessing still functions
- [ ] New features in blessed engines must be additive, not replacing

### 4. Debate Awareness

- [ ] Change does not unilaterally resolve an ongoing debate (DB001–DB004)
- [ ] If the change touches a debated area, it is flagged for design discussion

### 5. Cross-Engine Consistency

- [ ] Uses the same pattern as similar engines (match gold-standard 6/6 engines)
- [ ] Does not introduce a one-off pattern that differs from fleet conventions
- [ ] Uses shared DSP utilities from `Source/DSP/` (not engine-local reimplementations)
- [ ] Reuses fast math functions (fastSin, fastExp, fastPow2, fastTan, fastTanh)

### 6. Preset & Compatibility

- [ ] Existing `.xometa` presets load unchanged
- [ ] No coupling route breakage (MegaCouplingMatrix)
- [ ] Parameter ranges not narrowed (existing presets may use full range)
- [ ] Legacy engine name aliases still resolve (`resolveEngineAlias()`)

### 7. Performance

- [ ] No new per-sample transcendental functions (std::pow, std::tan, std::sin, std::exp)
- [ ] New LFOs/modulators use fastSin, not std::sin
- [ ] Filter coefficient updates use fast approximations in per-sample paths
- [ ] Block-constant values are hoisted outside per-sample loops

### 8. Denormal Safety

- [ ] New feedback paths have flushDenormal() protection
- [ ] New filter states are flushed after updates
- [ ] Decay envelopes approaching zero are guarded

### 9. Brand Alignment

- [ ] Feature serves a sonic pillar (not complexity for complexity's sake)
- [ ] Dry patch quality preserved (new modulation doesn't mask weak synthesis)
- [ ] Naming follows XO + O-word convention if applicable

## Decision Framework

### APPROVE if:
- All checklist items pass
- Change is minimal and focused
- Consistent with fleet patterns
- Respects all governance principals

### APPROVE WITH CONDITIONS if:
- Minor issues found (suggest specific fixes)
- Missing denormal protection (easy to add)
- Could use a fast math function instead of std::
- Blessing intact but could be better documented

### REQUEST CHANGES if:
- Architecture rule violated
- Doctrine compliance broken or degraded
- Blessed feature at risk
- Cross-engine inconsistency
- Risk of preset breakage
- Debate resolved without design discussion

### REJECT if:
- Fundamentally wrong approach (would need to be reverted)
- Blessed feature removed or broken
- Changes frozen parameter IDs
- Breaks SynthEngine interface contract
- Violates brand rules

## Output Format

```
## Architect Review: {CHANGE_DESCRIPTION}

### Verdict: APPROVE / APPROVE WITH CONDITIONS / REQUEST CHANGES / REJECT

### Governance Check
| Principal | Status | Notes |
|-----------|--------|-------|
| Architecture Rules | PASS/FAIL | ... |
| Doctrines (D001-D006) | PASS/FAIL | ... |
| Blessings (B001-B015) | PASS/N/A | ... |
| Debates (DB001-DB004) | CLEAR/FLAG | ... |
| Brand Rules | PASS/FAIL | ... |

### Technical Check
| Category | Status | Notes |
|----------|--------|-------|
| Cross-Engine Consistency | PASS/FAIL | ... |
| Preset Compatibility | PASS/FAIL | ... |
| Performance | PASS/FAIL | ... |
| Denormal Safety | PASS/FAIL | ... |

### Findings
[Specific issues with file:line references]

### Conditions / Required Changes
[If applicable — numbered list of required modifications before merge]

### Approved Scope
[Exact files and line ranges approved for modification]
```

## Fleet Reference: Engine Quality Tiers

When reviewing, use these engines as reference implementations:

**Gold standard (6/6 doctrines):** OPAL, OCELOT, OSPREY, OWLFISH, OHM, ORPHICA, OBBLIGATO, OTTONI, OLE

**Unconventional compliance (physics-based):** ORGANON (metabolic / B011), OUROBOROS (chaotic attractor / B003)

**Exempt:** OPTIC (visual engine / B005 — D001/D006 N/A)

Match proposed changes to the patterns in gold-standard engines whenever possible.
