# XPN Pack Feedback Loops — R&D Spec

**Date**: 2026-03-16
**Status**: Draft

---

## 1. Feedback Channels

Feedback arrives across five surfaces: Discord (real-time reactions, #pack-talk channel), MPC-Forums threads (gear-specific critique, velocity/tuning issues), Gumroad reviews (public-facing, high signal-to-noise), Patreon posts (engaged subscribers, more candid), and Twitter/Instagram DMs (praise-heavy, low actionable content).

Aggregation without overwhelm: route all five into a single weekly review using a shared note (Notion or plain markdown log). Spend 20 minutes every Monday reading everything. Do not respond in real time. Tag each item as one of: BUG / PRESET REQUEST / WORKFLOW FRICTION / PRAISE. Only BUG items get an immediate response. The rest accumulate until threshold (see Section 4).

Designated signal-to-noise filter: ignore standalone praise unless it names a specific preset. Named presets in positive context confirm what to expand. Named presets in negative context flag what to patch.

---

## 2. Quantitative Signals

Gumroad conversion rate (visitors to purchases) establishes pack appeal before any words are written. Below 3% suggests the preview audio or description is failing, not the pack itself. Refund rate above 5% is a quality signal — cross-reference with which presets reviewers mention.

Download counts alone are vanity. The meaningful pair is download count vs. Patreon tier upgrades in the 14 days after a pack release. If a free-tier pack drives tier upgrades, the pack is functioning as a funnel. If it does not, either the content ceiling is wrong or the upgrade CTA is buried.

Track these four numbers per release cycle: conversion rate, refund rate, 14-day Patreon delta, and repeat-buyer rate (what percentage of purchasers buy a second pack within 90 days). Repeat-buyer rate is the long-term health metric.

---

## 3. The Pack Pulse Framework

Three questions, delivered via Gumroad follow-up email 7 days post-download:

1. **"What did you make with this pack?"** (open text, 1–3 sentences) — surfaces use cases the pack designer did not anticipate. Recurring answers become the next pack's marketing copy.

2. **"Which preset did you use most?"** (multiple choice, full preset list from the pack) — identifies anchors. The top 3 answers inform which presets get expanded in v1.1.0 and which engine character to double down on.

3. **"What would make this pack better?"** (open text) — raw improvement signal. Filter for specificity: "more bass" is noise; "the kick presets clip at velocity 127" is actionable.

Delivery sequence: Gumroad follow-up email at day 7 (automated), Patreon post comment thread at day 14 (manual, post a pinned comment asking the same three questions), Discord #pack-feedback thread at day 21 (one post, link to the Gumroad form). Do not run all three simultaneously — stagger to catch different audience segments.

---

## 4. Iteration Triggers

**v1.0.1** (patch, silent release): Any confirmed clipping, level inconsistency across presets, broken macro assignments, or naming errors. Threshold: 1 confirmed report. Turnaround target: 72 hours. Announce in Discord only.

**v1.1.0** (minor update, announced): 4 or more unique users requesting the same preset type or mood gap, OR the Pack Pulse "most used" results reveal a clear category the pack underserves. Adds 4–8 new presets. Existing buyers get the update free via Gumroad. Announce across all channels.

**New pack**: Different engine character, different mood, or a community co-creation origin (see Section 5). Do not patch into an existing pack what is genuinely a new sonic identity. The rule: if you would give it a different name in the Aquarium water column, it is a new pack.

---

## 5. Community Co-Creation

Three lightweight mechanisms, run in sequence not parallel:

**Vote for the next pack** — bimonthly Discord poll, 4 options max, 72-hour window. Options are mood/character descriptions, not engine names. Winning concept enters the production queue.

**Beta listener cohort** — 5–10 Patreon Tier 2+ members receive a preview build 2 weeks before release. Single-question brief: "Does this feel like [mood descriptor]?" Yes/No plus one sentence. Gate the cohort at Tier 2 to ensure they are active producers, not casual listeners.

**DNA Challenge** — quarterly. Community submits 8–16 bar stems (any format). Best 3 stems by community vote become the harmonic/rhythmic DNA for a new pack. Credit the submitters in the pack description. This creates investment before the pack ships.

---

## 6. Feedback to Oxport Tooling

Pack feedback is a direct input queue for the XPN tool suite. Route the Pack Pulse open-text responses through a monthly review specifically looking for workflow friction, not just content requests.

Pattern examples and their tooling responses:

- "I wish there were more velocity layers on the kicks" → raise default velocity layer count in the drum kit builder from 4 to 6; add an 8-layer option as an explicit flag in `generate_xpms.py`.
- "The round-robin cycling feels random and disconnected" → review CycleGroup assignment logic; tighten the smart-cycle algorithm to bias toward adjacent timbres.
- "Presets are too loud compared to my other packs" → add a pack-level normalization pass in the bundler; target -18 LUFS integrated as a default.
- "I can't tell which presets are leads vs. pads" → enforce category tagging in the manifest schema; surface it in the pack's preset naming convention.

The tooling changelog should cite the specific feedback batch that triggered each change. This closes the loop visibly — community members who submitted feedback see their words become features.

---

*This document is a living spec. Update the iteration thresholds after the first three pack releases when real conversion and refund data is available.*
