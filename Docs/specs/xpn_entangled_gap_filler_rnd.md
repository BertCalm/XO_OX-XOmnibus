<!-- rnd: entangled-gap-filler -->

# Entangled Gap Filler — R&D Spec

**Status**: Design Draft
**Date**: 2026-03-16
**Scope**: Systematic strategy for reaching full Entangled preset coverage across all 561 engine pairs

---

## 1. Gap Analysis Summary

The coupling density heatmap reveals the current state plainly: **105 of 561 engine pairs (18.7%)** have at least one Entangled preset. That leaves **456 uncovered pairs** — a significant blind spot in the mood that is supposed to define XOmnibus as a coupled ecosystem rather than a collection of isolated engines.

**Most richly coupled legacy engines** (covered in the most pairs): OPAL, OBSIDIAN, DRIFT, ONSET, FAT. These engines benefited from early Prism Sweep attention and tend to appear in cross-genre contexts naturally.

**Most isolated legacy engines**: OBLONG, OVERBITE, OVERLAP, OUTWIT. Several of these are newer installs; coupling work was deferred post-integration.

**Constellation engines (OHM, ORPHICA, OBBLIGATO, OTTONI, OLE)** are the most critical gap. OBBLIGATO has zero coupling presets. OHM, ORPHICA, OTTONI, and OLE each have fewer than three pairs covered. The five Constellation engines together represent 5 × 30 = 150 potential pairs (Constellation × all others) plus 10 intra-Constellation pairs — roughly 160 pairs, all of which are essentially empty.

The Entangled mood is the proof-of-concept for the XOmnibus doctrine. Without dense coupling coverage, the mood reads sparse and the inter-engine relationships that define the ecosystem's identity remain invisible to users.

---

## 2. Priority Tier System

**Tier 1 — Constellation × Constellation (10 pairs)**
OHM/ORPHICA, OHM/OBBLIGATO, OHM/OTTONI, OHM/OLE, ORPHICA/OBBLIGATO, ORPHICA/OTTONI, ORPHICA/OLE, OBBLIGATO/OTTONI, OBBLIGATO/OLE, OTTONI/OLE. These are the highest-visibility gap: five new engines with zero cross-talk. Ship these first. Target: 6 presets per pair minimum.

**Tier 2 — Constellation × Top-5 Legacy (25 pairs)**
Each Constellation engine paired with OPAL, OBSIDIAN, DRIFT, ONSET, FAT — the legacy engines with the strongest coupling precedent and the broadest sonic range. These pairings yield the highest yield-per-session because the legacy engine brings established preset vocabulary that the Constellation engine can couple against. Target: 3–6 presets per pair.

**Tier 3 — High-Affinity Legacy Pairs (uncovered)**
Pairs identified via DNA analysis (see Section 3) that have not been covered yet despite strong thematic or sonic affinity. Examples: OVERLAP × OUTWIT (both post-Constellation installs, both experimental), OVERWORLD × OPAL (chip nostalgia × granular shimmer), ONSET × OBLONG (rhythmic × melodic bass tension). Target: 3 presets per pair to establish baseline.

**Tier 4 — Complete Coverage**
All remaining pairs. Many of these will be low-affinity or stylistically distant. For Tier 4, 3 presets per pair is the floor — enough to prove the coupling exists and pass quality gates, not enough to define a relationship deeply.

---

## 3. DNA-Based Pairing Logic

High-affinity pairs share one of three coupling signatures:

**Complementary brightness**: An engine with high spectral density and brightness (OTTONI, OVERDUB) pairs naturally with a dark, subtractive engine (OBSIDIAN, OCEANDEEP). The coupling preset lives in the tension between these poles — one engine handles presence, the other handles depth.

**Shared creature family**: The aquatic mythology gives every engine a creature identity. Engines from the same depth zone (pelagic vs. abyssal vs. surface) share physical characteristics that translate to sonic affinity. ORPHICA (siphonophore, colonial) couples naturally with ORGANISM (coral colony, cellular). OVERLAP (Lion's Mane jellyfish, resonant knot) couples naturally with OUTWIT (octopus, eight-arm complexity).

**feliX-Oscar axis tension**: feliX engines (OPENSKY, OPAL, ORPHICA) are bright, airy, upward. Oscar engines (OBSIDIAN, OVERDUB, ONSET) are dark, grounded, downward. The highest-drama coupling presets live at feliX-Oscar crossings: one engine pulls skyward, the other anchors. These pairs generate the most emotionally distinct presets and should be prioritized within Tier 3.

---

## 4. Naming Conventions for Coupling Presets

Names should encode the relationship, not just describe the sound.

**Constellation × Constellation**: Use mythological fusion or compound concepts. OHM + OTTONI = *Resonant Congregation*, *Bell Drone*, *Harmonic Assembly*. The names should feel like events or rituals, not textures.

**feliX + Oscar pairings**: Use tension vocabulary — words that imply pull, contrast, or negotiation. OPAL + OBSIDIAN = *Surface Tension*, *Light Through Dark Water*, *The Boundary Layer*. Avoid cozy names; these are oppositions.

**Same-family pairings** (creature kin): Use ecological or behavioral language. OVERLAP + ORGANISM = *Colony Drift*, *Entangled Growth*, *The Reef Knot*.

**Legacy × Constellation**: The legacy engine's established identity should dominate the name with the Constellation engine's character as modifier. FAT + OHM = *Saturated Commune*, *Heavy Drone Gathering*.

---

## 5. Batch Generation Strategy

**3 presets per pair** = minimum viable. Demonstrates the coupling exists, covers one mood variant (e.g., slow evolving pad). Ships a pair from zero to present.

**6 presets per pair** = healthy. Covers a slow/fast split and at least one rhythmic or percussive variant. Sufficient for Tier 1 and Tier 2 pairs.

**12 presets per pair** = rich. Reserved for the highest-affinity and most compositionally interesting pairs. Justifies a dedicated coupling section in the Field Guide.

**Tooling**: Use `xpn_constellation_coupling_pack.py` for batch generation of Tier 1 and Tier 2 pairs — it handles macro assignment, coupling parameter scaffolding, and preset file naming automatically. For Tier 3 and Tier 4, manual curation is preferred: these pairs are lower-affinity and automation tends to produce generic results without strong DNA guidance. Manual curation sessions of 6–8 pairs per session are sustainable.

---

## 6. Quality Gates

A coupling preset must demonstrate actual coupling — not two engines playing simultaneously but independently. Three tests:

1. **Macro dependency test**: At least one macro must visibly alter both engines' behavior simultaneously. If a macro moves only one engine, it is not a coupling preset — it is a layered preset.

2. **Coupling intensity levels**:
   - *Light coupling*: Shared macro, independent voices. One engine reacts to the other's parameter space but audio paths remain separate.
   - *Medium coupling*: Audio routing entanglement — one engine's output feeds a parameter (e.g., filter, reverb send) in the other.
   - *Deep coupling*: Modulation cross-patch — one engine's LFO or envelope drives a core parameter in the other. These presets should behave differently every time a note is held vs. released.

3. **Silence test**: When one engine is muted, the remaining engine should sound incomplete or wrong — not just quieter. If both engines sound fine in isolation, the coupling needs to be stronger.

---

## 7. Roadmap to Full Coverage

At 3 presets per pair as the minimum, full coverage across 561 pairs = **1,683 minimum new presets**. At 3 sessions per week generating 8 pairs per session:

- **Tier 1 complete** (10 pairs × 6 presets): 1 session
- **Tier 2 complete** (25 pairs × 3–6 presets): 3–4 sessions
- **50% coverage** (280 pairs total): ~17 sessions from current baseline
- **75% coverage** (420 pairs total): ~32 sessions
- **100% coverage** (561 pairs, 3 presets minimum): ~48 sessions

Realistically, Tier 4 pairs will require less time per pair once the DNA-pairing logic is systematized. A generation script calibrated to feliX-Oscar tension and creature-family affinity should compress Tier 4 by 30–40%.

The Entangled mood becomes the ecosystem's proof of life at 50% coverage. That is the meaningful milestone — ship it loudly.

---

*Next action: Generate Tier 1 Constellation × Constellation batch. Start with OHM + OTTONI (drone × brass = highest contrast) and OBBLIGATO + ORPHICA (dual wind × microsound = closest kin).*
