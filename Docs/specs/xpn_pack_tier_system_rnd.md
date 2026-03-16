# XPN Pack Tier System — R&D Spec
**Date**: 2026-03-16
**Status**: Research / Pre-production

---

## The Problem

Pack releases have followed a single-depth model: one pack per engine, one price point, one audience. This leaves money on the table at both ends — curious newcomers who won't spend $25 on an unfamiliar engine, and serious producers who want everything and feel underserved by a flat 20–40 preset release.

A tiered system creates a funnel, a progression, and a collectible logic.

---

## Tier Names: SIGNAL / FORM / DOCTRINE

Borrowed from XO_OX's own internal language.

- **SIGNAL** — the first transmission. What the engine says when you turn it on.
- **FORM** — the engine's full body. Every pole, every character state.
- **DOCTRINE** — the definitive artifact. How the engine thinks, not just what it sounds like.

---

## Tier 1: SIGNAL (Free or $9–12)

**Contents**
- 20 presets, 2 velocity layers
- Covers the engine's most immediately readable character presets — the sounds that answer "what is this?"
- Standard XPM format, no extras
- Single mood category per preset (no axis variations)

**Excluded and why**
- No coupling presets — coupling assumes the buyer owns multiple engines
- No signature/showcase presets — those are the hook for FORM
- No liner notes — friction reduction is the point

**Target audience**: First contact. Producers browsing Gumroad, Patreon free tier, anyone who heard a demo.

**Pricing rationale**: Free builds the list. $9–12 covers Gumroad fees and signals non-zero value. Either works depending on engine maturity — newer engines launch free, proven engines at $9.

**Production effort**
- Without Fleet Render Automation: ~3–4 hours (preset design + XPN export + cover art)
- With Fleet Render Automation: ~1–1.5 hours (automation handles XPN packaging, cover art generation, velocity layer rendering)

---

## Tier 2: FORM ($15–18)

**Contents**
- 40 presets, 4 velocity layers
- Full feliX–Oscar axis coverage: explicitly designed presets at both poles and midpoints
- 5 "signature" presets that demonstrate capabilities unique to this engine (e.g., OHM's COMMUNE drift, OVERLAP's knot-topology bloom, ONSET's MACHINE macro sweep)
- 1 MPCe quad-corner layout variant — maps 4 character states to the four corner pads for live performance
- All 7 moods represented (Foundation / Atmosphere / Entangled / Prism / Flux / Aether / Family)

**Excluded and why**
- No coupling showcase — FORM is still a single-engine product
- No round-robin samples — doubles production time, reserved for DOCTRINE
- No PDF — keeps delivery simple

**Target audience**: The working producer. Owns an MPC, buys packs deliberately, wants a complete toolkit not a sampler.

**Pricing rationale**: $15–18 is the sweet spot for impulse + intent. Below $20 reduces friction. The delta from SIGNAL ($5–8) is small enough to feel like an upgrade, not a repurchase.

**Production effort**
- Without Fleet Render Automation: ~8–10 hours
- With Fleet Render Automation: ~3–4 hours

---

## Tier 3: DOCTRINE ($25–35)

**Contents**
- 80+ presets, 8 velocity layers where appropriate (drums always; melodic engines at producer discretion)
- Full ADSR grid variants: representative presets rendered across 3–4 envelope shapes
- feliX–Oscar axis sweep: 5-step progression presets for key sounds
- Coupling showcase: 3 coupling states (solo / soft-coupled / hard-coupled) for 8 anchor presets
- Round-robin samples: 3 RR hits per velocity layer for all drum/percussion content
- PDF liner notes: sound design rationale, engine identity card, Sonic DNA breakdown, recommended Q-Link assignments
- All SIGNAL and FORM content included

**Excluded and why**
- Nothing. This is the complete artifact.

**Target audience**: Sound design collectors, XO_OX community members, producers who work deeply with one engine for a season.

**Pricing rationale**: $25–35 reflects total production time and positions as a premium collectible. The PDF liner notes justify the top of range — they reframe it as a document, not just a sample pack.

**Production effort**
- Without Fleet Render Automation: ~20–25 hours
- With Fleet Render Automation: ~8–10 hours (PDF and coupling presets remain manual)

---

## Upgrade Path

Tiers should be purchasable as a progression. Gumroad supports this via bundle discounts and coupon codes:

- Bought SIGNAL at $9? Upgrade to FORM for $8 (not $18).
- Bought FORM at $18? Upgrade to DOCTRINE for $12 (not $35).
- Full progression: $9 + $8 + $12 = $29 vs. $35 standalone DOCTRINE — slight incentive to upgrade incrementally.

Implementation: Gumroad coupon codes included in SIGNAL/FORM download confirmation emails. Manual for now; automatable if volume justifies it.

---

## Gumroad + Patreon Overlap

**Proposed boundary**:
- **Patreon**: SIGNAL tier is the Patreon free benefit. Patron-exclusive early access to FORM before Gumroad release (2-week window). DOCTRINE is never Patreon-exclusive — it's a standalone purchase.
- **Gumroad**: All three tiers available permanently. Patreon members get a discount code for FORM and DOCTRINE.

This keeps Patreon valuable (early access, discount) without cannibalizing Gumroad sales on the deep-value tiers.

---

## Open Questions

1. Should DOCTRINE include stems/multi-track renders, or does that belong to a separate "Session" product?
2. Engine-specific liner notes require per-engine writing time (~2 hours each) — budget this explicitly before committing DOCTRINE to the schedule.
3. Coupling showcase presets require the buyer to own both engines. Consider a "simulated coupling" single-engine approximation as a fallback for buyers who don't.
