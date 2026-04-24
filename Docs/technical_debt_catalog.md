# Technical Debt Catalog

**Generated**: 2026-03-19 Haiku Audit
**Status**: 7 items identified, all categorized

---

## Active TODOs (Deferred to Phase 2)

### High Priority (V1 Ship Blockers)

| File | Line | Type | Description | Phase | Blocker? |
|------|------|------|-------------|-------|----------|
| `Source/Engines/Ocelot/OcelotEngine.h` | 103 | TODO | `applyCouplingInput` stub — coupling is currently a no-op. Implement engine-specific modulation routing before V1 ships. | Phase 2 | YES |
| `Source/Engines/Owlfish/OwlfishEngine.h` | 153 | TODO | Same as Ocelot: `applyCouplingInput` stub — implement routing before ship | Phase 2 | YES |

### Medium Priority (Post-V1, Before V1.1)

| File | Line | Type | Description | Phase |
|------|------|------|-------------|-------|
| `Source/Engines/Ocelot/OcelotEmergent.h` | 23 | TODO | Voiced source (LF pitch oscillator), full Klatt-style vocal tract | Phase 2 |
| `Source/Engines/Ocelot/OcelotVoice.h` | 142 | TODO | Real spectral proxy (currently 0.5f placeholder) | Phase 2 |
| `Source/Engines/Ocelot/OcelotCanopy.h` | 19 | TODO | Full additive partial bank + wavefolder | Phase 2 |
| ~~`Source/Engines/Orbital/OrbitalEngine.h`~~ | ~~1277~~ | ~~TODO~~ | ~~Organ drawbar tuning decision~~ | **RESOLVED** 2026-03-19 (see `Docs/orbital-drawbar-decision-2026-03-19.md`) |

### Debug Markers

| File | Line | Type | Description |
|------|------|------|-------------|
| `Source/Core/ChordMachine.h` | 50 | MARKER | `XXXXXXXXXXXXXXXX` — sustained pad gate = 1.0 (intentional debug marker, not an error) |

---

## Analysis

### Ocelot (6 Phase 2 items)
Ocelot is the richest Phase 2 candidate. Its coupling routing is blocked on the engine-specific modulation architecture, which is foundational. The voiced source, spectral proxy, and additive bank are ambitious Phase 2 extensions that build on the Phase 1 core.

### Owlfish (1 Phase 2 item)
Owlfish shares the coupling routing blocker with Ocelot but has no other Phase 2 deferred work. Its Phase 1 is relatively stable.

### Orbital (1 Tuning Decision)
The drawbar tuning is a creative decision, not a technical blocker. This should be decided before final preset authoring.

---

## Recommendations

1. **Ocelot/Owlfish Coupling**: Schedule this as an independent feature branch (Phase 1.5) before Phase 2. Blocking cross-engine mod on just 2 engines is manageable.

2. **Orbital Drawbar Tuning**: This is a creative decision, not a code debt. Schedule a design decision session with the user to finalize the organ character (e.g., "B3-like 888 draw", "organ jazz", "psychedelic").

3. **Ocelot Phase 2**: Explicitly plan as V1.2+ work. Mark in CLAUDE.md as Phase 2 candidate.

4. **ChordMachine Marker**: The `XXXXXXXXXXXXXXXX` is intentional (debug marker for sustained gate). Leave as-is; convert to proper comment if it becomes confusing in the future.

---

## Debt Health

- **V1 Blockers**: 2 (both coupling routing, manageable)
- **Post-V1 Scope**: 4 (all Ocelot Phase 2 extensions, ambitious but non-critical)
- **Creative Decisions**: 1 (Orbital drawbar — not technical debt)
- **Debug Markers**: 1 (harmless)

**Overall**: Low technical debt. All TODOs are intentional deferrals with clear phase assignments.

