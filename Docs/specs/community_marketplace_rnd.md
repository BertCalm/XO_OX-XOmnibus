# Community Marketplace & Creator Economy — R&D

**Date**: 2026-03-16
**Status**: R&D / Pre-Spec
**Owner**: XO_OX Design
**Audience**: Internal strategy, future Barry OB community team

---

## Overview

XO_OX makes instruments with doctrine — sonic DNA, aquatic mythology, emotional polarity. The community marketplace extends that doctrine outward: it invites producers to grow, remix, and challenge XO_OX material while preserving the integrity that makes it worth building on in the first place.

This document specs out five models, a quality gate system, a competitive matrix, and a platform build roadmap. None of these models are mutually exclusive. The recommendation is to launch with **Seed + Grow** and **DNA Challenge** in Year 1, introduce **Collab Chain** in Year 2, and activate the **Creator Tier** alongside platform infrastructure.

---

## 1. Seed + Grow Model

### Concept

XO_OX releases minimal "seed kits" — 4-pad configurations, fully documented, with source XPM files. The seed is intentionally spare: one velocity layer, no round-robins, no processing. It is a beginning, not a product.

Community members "grow" the seed: add velocity layers, new sample takes, alternate mic positions, processing variants, or entirely different instruments that share the same DNA constraints. The best grown versions are reviewed by XO_OX and may be absorbed into official packs.

### Mechanics

**Seed Kit Package Contents**:
- `seed_kit.xpn` — installable for MPC
- `source_samples/` — raw WAV files, unprocessed
- `seed_dna.json` — the 6D DNA profile the kit was grown from
- `grow_notes.md` — explicit invitation + constraints for community growth
- `liner_notes.json` — attribution scaffold (creator fields pre-populated with XO_OX)

**Growth Submission**:
- Submitter forks the seed (Git-style, tracked by seed ID)
- Adds their layers, processes, alternates
- Submits via XO-OX.org submission portal with completed `liner_notes.json`
- Automated QA runs (see Section 5)
- If QA passes, enters human curation queue

**Featured Growth Program**:
- Quarterly, XO_OX selects 1–3 grown variants per seed for inclusion in official packs
- Credit appears in pack liner notes and on creator's public profile
- Revenue share: 10% of pack revenue attributable to the grown kit distributed to credited growers pro-rata
- Patreon share: grower receives a complimentary Tier 2 month for each featured growth

### Design Principles

The seed-and-grow metaphor is not just marketing — it is a workflow constraint. A seed kit that ships with 47 layers is not a seed. XO_OX must resist the temptation to "help" the community by over-specifying the seed. Sparse is the point.

The grown version may be unrecognizable from the seed. That is acceptable. The DNA profile is the thread of continuity, not the samples themselves.

---

## 2. Collab Chain Model

### Concept

Producer A makes a kit. Producer B takes it, remixes it, uploads a variant. Producer C takes B's variant and makes another. The chain is tracked, attributed, and visualized as a branching family tree on XO-OX.org.

This is Git for sample kits. Not metaphorically — structurally.

### Mechanics

**Kit Identity**:
- Every kit gets a UUID on upload: `kit_id`
- Every derived kit records `parent_kit_id`
- Chains are reconstructable from `parent_kit_id` traversal
- Depth limit recommended: 5 generations (prevents noise from accumulating unchecked)

**`liner_notes.json` Schema**:
```json
{
  "kit_id": "uuid-v4",
  "parent_kit_id": "uuid-v4 or null",
  "title": "Kit Name",
  "generation": 2,
  "contributors": [
    {
      "name": "Producer A",
      "role": "originator",
      "patreon_id": "optional",
      "contribution": "Initial drum programming, all samples"
    },
    {
      "name": "Producer B",
      "role": "remixer",
      "patreon_id": "optional",
      "contribution": "Added velocity layers 3-5, reprocessed kick"
    }
  ],
  "dna_profile": {
    "brightness": 6,
    "movement": 4,
    "tension": 7,
    "warmth": 5,
    "density": 3,
    "weight": 8
  },
  "dna_drift": 1.2,
  "created": "2026-04-15",
  "xo_ox_reviewed": false
}
```

**DNA Drift Score**:
- Each generation computes Euclidean distance from the root kit's DNA profile
- Drift is displayed on the family tree visualization
- High-drift chains are interesting (community taking it somewhere unexpected)
- Very high drift (> 3.5 on 10-point scale) triggers an XO_OX doctrinal review — not rejection, but a conversation

**Kit Family Tree Visualization**:
- SVG tree rendered on the kit's page at XO-OX.org
- Each node: kit name, creator, generation number, DNA drift badge
- Click any node to view/download that version
- "Inspired by" links between distant relatives (non-parent derivations)

### Attribution Policy

All contributors in a chain are credited indefinitely. Revenue share for featured chain kits is distributed across all credited contributors using an exponential decay formula: originator receives 40%, each subsequent generation receives 60% of the previous generation's share, normalized to 100%.

This formula rewards origination while still making deep-chain contribution worth doing.

---

## 3. DNA Challenge

### Concept

Monthly challenge. XO_OX publishes a DNA constraint brief: "Make a kit where brightness > 8, movement < 3, and weight > 7." Community submits kits. XO_OX ranks by DNA distinctiveness — meaning: how far from the median submission, while still satisfying the constraints. The winner is featured in the next official pack and receives Tier 2 for three months.

### Monthly Challenge Structure

**Week 1**: Challenge announced. DNA constraints published + an audio reference ("Something like this — but make it yours").

**Weeks 2–3**: Submission window open. Automated QA runs on submission. Accepted submissions visible to community (encourages late entries to differentiate).

**Week 4**: XO_OX curation + community vote (Tier 2 Patreon subscribers vote). Winner announced.

**Following Month**: Winner's kit ships in next free pack or as a bonus in a paid pack. Winner credited.

### DNA Constraint Brief Format

```
DNA Challenge #004 — "The Frozen Engine Room"
Month: July 2026

Constraints:
  brightness: < 3
  movement: < 3
  weight: > 8
  tension: > 6
  warmth: any
  density: any

Reference audio: [link — 30-second clip]
Disqualifying territory: No trap hi-hat patterns. This is machinery, not a beat.

Submission deadline: July 21, 2026, 11:59pm ET
```

### Ranking Algorithm

1. Verify all six DNA axes satisfy constraints
2. Compute each submission's DNA vector
3. Compute centroid of all valid submissions
4. Rank by distance from centroid (most distinctive wins)
5. Manual override: XO_OX curatorial veto (rare, but reserved)

The algorithm rewards divergence from the crowd, not just technical compliance. A kit that satisfies constraints but sounds like everyone else's is ranked lower than one that finds an unexpected edge of the constraint space.

---

## 4. XO_OX Creator Tier (Patreon $25/month)

### Position

Between the $15 Producer tier and the $30 Studio tier — or positioned as a distinct "Creator" track at $25. The distinction is directional: Producer tier is for consumers of XO_OX content; Creator tier is for producers who want to contribute back.

### Benefits

- Early access to source XPM files (2 weeks ahead of Studio tier)
- Oxport tool access: full export pipeline including batch export, DNA auto-tagging, xpn_qa_checker.py
- Priority review queue: community submissions from Creator tier reviewed within 10 business days (vs. standard 30-day queue)
- "Creator" badge on community profile
- Monthly Creator-only call: Barry OB's team + 5–10 Creators, 60 minutes, covering upcoming seed kits and challenge briefs
- 1 free submission to DNA Challenge per month (standard limit: 1 per quarter for lower tiers)
- Revenue share eligibility: only Creator tier and above can receive Patreon revenue share for featured community content

### Why $25, Not $15 or $30

$15 is for consumption. $30 is for deep access to the XO_OX build process. $25 buys productive contribution rights. The Creator tier is not about more content — it is about a different relationship with the platform.

### Anti-Abuse Safeguards

- Patreon ID must be linked to submission portal before first submission
- Revenue share requires minimum 3 months of active Creator tier subscription
- Tier suspension (not termination) for submissions that repeatedly fail QA or violate doctrinal standards — with a resubmission path

---

## 5. Quality Gate for Community Content

### Gate Architecture

Community content passes through four sequential gates. Failure at any gate routes to a rejection message with specific, actionable feedback and an explicit invitation to resubmit.

```
Submission
    |
    v
[Gate 1: Automated Technical QA]
    | pass
    v
[Gate 2: Doctrinal Check (D001)]
    | pass
    v
[Gate 3: Barry OB Aesthetic Review]
    | pass
    v
[Gate 4: XO_OX Final Approval]
    |
    v
Published / Featured
```

### Gate 1: Automated Technical QA (xpn_qa_checker.py)

Runs on all submissions before any human review. Checks:

- **Silence**: No pad may have > 500ms of leading silence or > 1s trailing silence
- **Clipping**: No sample exceeds -0.3dBFS peak (hard clips disqualify; near-clips trigger a warning, not rejection)
- **DC offset**: RMS DC offset < 0.5% of full scale
- **Phase**: Stereo samples checked for mono compatibility (phase correlation > -0.5)
- **File integrity**: All referenced samples present, WAV format valid, no zero-byte files
- **DNA profile present**: `liner_notes.json` must include all six DNA axes with values 1–10
- **XPN structure**: Valid pad assignments, note map present, velocity layers ordered ascending

Output: `qa_report.json` with pass/fail per check + specific timestamps for audio issues.

### Gate 2: Doctrinal Check (D001)

D001: Velocity must shape timbre, not just amplitude.

Automated sub-check: compare spectral centroid of lowest velocity layer vs. highest velocity layer. Minimum required shift: 8% change in centroid frequency. If all velocity layers are amplitude-only (centroid within 3%), automatic D001 failure.

Manual override available for doctrinal exceptions (e.g., a kit where the doctrine is intentionally subverted as a design statement — rare, requires explicit XO_OX approval).

D001 rejection message:
> "Your velocity layers are working dynamically but not timbrally. Doctrine D001 requires that hitting harder changes the character of the sound, not just the volume. Try opening a filter at high velocities, or using different samples with different brightness profiles per layer. We want to hear your next version."

### Gate 3: Barry OB Aesthetic Review

Human curation. Barry OB's team (or designated community curators at scale) assess:

- Does this sound like it belongs in XO_OX's catalog?
- Is there a clear point of view? (Kits with no personality are rejected — not on technical grounds, but aesthetic ones)
- Does the kit serve the DNA it claims? (A kit claiming brightness=9 that sounds dark fails this check)
- Is it distinguishable from what XO_OX already ships?

Review turnaround: 30 days standard, 10 days Creator tier.

Rejection at this gate always includes a written paragraph of specific feedback. Barry OB's standard: "We reject what doesn't fit, but we explain why and we mean it when we say come back."

### Gate 4: XO_OX Final Approval

Reserved for kits slated for featured/official inclusion. Checks:

- Legal: submitter confirms original sample ownership or licenses
- Branding: kit name doesn't conflict with existing XO_OX releases
- Commercial readiness: pack-ready documentation present

---

## 6. Competitive Matrix

| Platform | Model | Community Role | Quality Control | Creator Economics |
|---|---|---|---|---|
| **Splice** | Streaming subscription | Passive consumers | Algorithmic surfacing | Revenue share per download, no curation |
| **Drum Broker** | Curated marketplace | None — producer-direct | Manual curation | 60–70% to producer, no community layer |
| **MSXII** | Community-first | Active — Discord, challenges | Self-policed | Premium packs fund community; no formal share |
| **XO_OX** (proposed) | Doctrine-driven ecosystem | Active — grow/chain/challenge | 4-gate: automated + doctrinal + aesthetic + legal | Patreon share for featured work, Creator tier |

### Key Differentiators

**vs. Splice**: Splice is a streaming service with a discovery problem. Any producer can upload anything. Quality is undefined. XO_OX is the opposite — submission is meaningful because standards are real. The XO_OX name on a community kit means something passed four gates.

**vs. Drum Broker**: Drum Broker is a storefront. Excellent curation, no community architecture. Producers submit finished products, not seeds. XO_OX's Seed + Grow and Collab Chain models have no equivalent anywhere in the current landscape.

**vs. MSXII**: MSXII has built genuine community loyalty via Discord and challenges. But their challenges are vibes-based ("make something chill"), not constraint-based. XO_OX's DNA Challenge system brings structure to community creativity without removing the fun. MSXII has no doctrine — everything is accepted if it sounds good to them. XO_OX has doctrine, which means rejection is principled rather than arbitrary.

### The Unsolved Problem All Competitors Have

No platform currently connects community output to the source instruments. Splice samples exist in isolation. Drum Broker kits have no lineage. MSXII challenges produce content that disappears into feeds.

XO_OX's advantage is that community kits are built on XO_OX engines. The Collab Chain tracks lineage back to source. The DNA system connects community kits to the same vocabulary used to design the engines themselves. That is a moat, not a feature.

---

## 7. Platform Build Roadmap (XO-OX.org)

### Phase 1 — Foundation (April–June 2026)

Minimum viable infrastructure to run DNA Challenge and receive Seed + Grow submissions.

- **Submission form**: Upload XPN file + `liner_notes.json` + submitter details
- **xpn_qa_checker.py integration**: Automated QA on upload, immediate feedback
- **Creator profile pages**: Name, bio, submission history, badges earned
- **Basic DNA badge display**: Six-axis radar on each kit page
- **DNA Challenge landing page**: Active challenge brief, countdown, submission button
- **Submission status tracker**: Submitter sees which gate their kit is at

Tech stack recommendation: Next.js (aligned with existing XO-OX.org), Firebase for submissions + profiles, Python microservice for QA checker, R2 (Cloudflare) for XPN file storage.

### Phase 2 — Chain Architecture (July–September 2026)

Once Seed + Grow has produced enough grown kits to make chains meaningful.

- **Kit family tree visualization**: SVG tree, clickable nodes, DNA drift display
- **`parent_kit_id` tracking**: Enforced on all new submissions
- **Chain DNA drift calculator**: Automated on submission
- **"Inspired by" linking**: Manual tag, not tracked parentage

### Phase 3 — Community Scale (October 2026–March 2027)

Infrastructure for sustained community activity.

- **Community feed**: Recent submissions, featured kits, challenge results
- **Voting system**: Tier 2+ Patreon subscribers vote on DNA Challenge finalists (Patreon OAuth integration required)
- **Revenue share dashboard**: Creator tier members see their cumulative earnings
- **Curator portal**: Barry OB's team reviews Gate 3 submissions in a moderation interface
- **Pack attribution pages**: Every official pack lists community contributors with profile links

### Data / Privacy Notes

- Kit UUIDs are public. Creator Patreon IDs are stored internally but not displayed publicly without opt-in.
- Sample files in community submissions are stored for review only — not redistributed without explicit creator consent and Gate 4 approval.
- DNA profiles of all submissions are aggregated for monthly challenge ranking. No personal data in DNA analysis.

---

## Open Questions

1. **Revenue share accounting**: How does XO_OX attribute pack revenue to specific kit contributions when a pack contains 8 kits and 3 are community-grown? Flat pro-rata or weighted by DNA distinctiveness?

2. **Moderation at scale**: Barry OB's team can review 50 submissions/month. What happens at 500? Community curator program (vetted Tier 2 producers), or algorithmic pre-filtering?

3. **Seed kit licensing**: Are source XPM files in seed kits released under Creative Commons? What prevents a grower from selling the grown kit independently, bypassing the Patreon share? A clear license agreement at submission time is required before Phase 1 launch.

4. **DNA Challenge exclusivity**: Does winning a challenge make the kit exclusive to XO_OX, or does the creator retain the right to release it independently? This has to be answered before the first challenge launches.

5. **International payments**: Patreon handles USD natively. Revenue share for non-US creators requires Stripe or Wise integration. Legal review needed.

---

*End of Document*
