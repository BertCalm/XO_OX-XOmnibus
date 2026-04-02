> **STATUS: FLAGGED** | New direction identified; needs dedicated planning session before work begins.

# Synesthesia Engine — Escalation for Visibility

**Date:** 2026-03-17
**Status:** FLAGGED — New direction identified, needs dedicated planning session
**Origin:** SRO Optimization session (logged as "separate workstream" in SRO decisions table)
**Prior art:** `Docs/concepts/xoceanus_collections_vision.md`, `Docs/concepts/culinary_collection_overview.md`

---

## What Is It

The Synesthesia Engine is an AI-powered cross-domain preset generation system. It converts arbitrary creative inputs into XOceanus `.xometa` presets:

```
ANY CREATIVE INPUT ──→ SYNESTHESIA ENGINE ──→ XOCEANUS PRESET
     (recipe)              (AI model)           (.xometa)
     (star chart)          maps input to
     (painting)            6D Sonic DNA
     (poem)                selects engines
     (color palette)       sets parameters
     (photograph)
```

Every Collection has a Synesthesia domain:
- **Depths:** Marine imagery, sonar data, creature behaviors
- **Atelier:** Material properties, craft processes
- **Arcade:** Code snippets, pixel art, game mechanics
- **Frontier:** Physics simulations, chaos math
- **Kitchen Essentials:** Real recipes → presets
- **Constellation (future):** Star charts, planetary positions
- **Laboratory (future):** Equations, molecular structures

## Why It Needs Escalation

1. **It was parked during SRO work** — The SRO session correctly identified it as orthogonal to DSP optimization and logged it as a separate workstream. But "separate workstream" risks becoming "forgotten workstream."

2. **It touches everything** — Synesthesia is not a feature bolt-on. It's the **community growth flywheel** for the Collections architecture. Every collection's community touchpoint feeds through it. Without it, Collections are just preset folders with nice names.

3. **It has unresolved architectural questions:**
   - On-device (privacy, offline) vs cloud (more powerful model) vs hybrid?
   - What's the minimum viable version — recipe→preset only, or multi-domain from launch?
   - How does `NaturalLanguageInterpreter.h` (which already has synesthesia-style mappings for food/taste/mood→DNA) relate to the full Synesthesia Engine? Is it the seed, or a separate thing?
   - Community rating/calibration loop — how does user feedback improve the model over time?
   - Reverse synesthesia (preset → recipe/poem/color palette) — V1 scope or later?

4. **It intersects with existing code** — `Source/AI/NaturalLanguageInterpreter.h` already maps natural language descriptors (including food/taste terms like "sweet", "bitter", "spicy") to 6D Sonic DNA vectors. This is arguably a proto-Synesthesia Engine. The relationship needs to be clarified before both evolve independently.

5. **It's a differentiator** — No other synth does this. The concept of "what does bioluminescence sound like?" or "turn this curry recipe into a pad" is the kind of thing that defines XOceanus as a creative platform rather than just a synthesizer.

## What's NOT Being Asked Right Now

- No DSP work needed
- No engine changes needed
- No build impact

This is purely a **planning and architecture escalation** — making sure the Synesthesia Engine gets its own dedicated session before Collections development advances further.

## Open Questions (Consolidated)

From `culinary_collection_overview.md` §Open Questions #11:
> **Synesthesia Engine scope** — On-device (privacy, offline) vs cloud (more powerful model)? Or hybrid with local fast-draft + optional cloud refinement? What's the minimum viable version — recipe→preset only, or multi-domain from launch?

From `xoceanus_collections_vision.md` §Open Questions #7:
> **Synesthesia Engine MVP** — What's the simplest version that proves the concept? Probably: recipe text → 6D Sonic DNA → engine/preset suggestion. No parameter-level AI at first.

Additional questions surfaced during this escalation:
1. Is `NaturalLanguageInterpreter.h` the foundation, or do we start fresh?
2. Does the Synesthesia Engine run inside the plugin, on a companion website, or both?
3. What AI model/approach? Local embeddings? Cloud LLM? Fine-tuned classifier?
4. How does it interact with the existing preset browser?
5. What's the data model for community ratings and calibration?

## Recommended Next Step

Dedicated planning session focused on:
1. MVP scope definition (single collection, single input type)
2. Architecture decision: local vs cloud vs hybrid
3. Relationship to `NaturalLanguageInterpreter.h`
4. Data flow: input → interpretation → DNA → engine selection → parameter mapping → .xometa
5. Community feedback loop design

---

*Escalated from SRO Optimization session. See `Skills/sro-optimizer/SKILL.md` decisions table, entry 2026-03-17: "CodeShark / Synesthesia → separate workstreams".*
