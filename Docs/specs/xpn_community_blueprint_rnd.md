# XO_OX Pack Community — Full Launch & Sustainability Blueprint
**Date**: 2026-03-16 | **Status**: R&D Spec | **Owner**: XO_OX Designs

---

## Overview

This document is the complete blueprint for launching and sustaining the XO_OX pack community —
from Discord server architecture through the community-contributed pack pipeline through the creator
economy. It is the canonical planning reference for community infrastructure decisions.

**Goal**: Build the most craft-conscious MPC/XOmnibus sample community on the internet. Not the
largest — the most curated, most technically excellent, most creatively ambitious.

---

## 1. Discord Server Architecture

### Server Name
**XO_OX Designs** — matching the brand, not product-specific (the community grows with every new product).

### Channel Structure

```
XO_OX Designs
│
├─ INFORMATION
│   ├─ #welcome               — pinned onboarding: what XO_OX is, how to get packs
│   ├─ #rules                 — community standards, copyright policy, QA floor
│   └─ #announcements         — XO_OX-only posts: releases, betas, challenges
│
├─ COMMUNITY
│   ├─ #general               — open conversation, introductions
│   ├─ #mpc-tips              — workflow, hardware setup, MPC OS questions
│   ├─ #xomnibus-beta         — Beta role only: pre-release feedback channel
│   └─ #showcase              — post tracks, videos, screenshots using XO_OX packs
│
├─ PACKS
│   ├─ #pack-releases         — official + featured community pack drop announcements
│   ├─ #preset-sharing        — share individual .xometa presets (engine-tagged)
│   └─ #community-submissions — submission intake; bot-gated (see §2)
│
├─ FEEDBACK
│   ├─ #bug-reports           — structured bug thread format (see §4)
│   └─ #feature-requests      — upvote-gated feature funnel (see §4)
│
└─ VOICE
    ├─ Drop Zone               — general hang
    └─ Studio Session          — co-working / listening sessions
```

### Roles

| Role | How Granted | Permissions |
|------|-------------|-------------|
| **Listener** | Default on join | Read all public channels |
| **Producer** | Self-assigned via bot (`/role producer`) | Post in #preset-sharing, #showcase, #mpc-tips |
| **Pack Creator** | Assigned after first accepted community submission | Post in #community-submissions, visible credit badge |
| **XO_OX Beta** | Direct invite from XO_OX (10-30 members) | Access #xomnibus-beta; NDA acknowledgment required |
| **Moderator** | XO_OX-appointed | Manage messages, timeout, review #community-submissions |

**Design principle**: Roles gate posting, not reading. Lurkers can see everything. Friction only
appears at contribution points (pack submission, beta access), not passive engagement.

---

## 2. Community Submission Pipeline

### 2.1 Submission Form

Community members submit via a Google Form (linked in #community-submissions channel description).
Required fields:

| Field | Type | Notes |
|-------|------|-------|
| Creator handle | Text | Must match Discord handle |
| Pack name | Text | Max 40 chars, must be original |
| Pack ZIP URL | URL | Google Drive / Dropbox / WeTransfer link |
| Target engine(s) | Multi-select | All 34 registered XOmnibus engines listed |
| Mood tags | Multi-select | Foundation / Atmosphere / Entangled / Prism / Flux / Aether / Family |
| BPM range | Text | Optional for tonal packs |
| Sample origin declaration | Checkbox | "All samples are original or cleared for commercial use" |
| Art origin | Radio | `original` / `licensed` / `AI-generated` |
| Brief description | Textarea | Max 200 chars — appears in pack listing if featured |

**Submission ZIP structure** (per `community_pack_submission_rnd.md`):
```
contributor_packname_v1/
  pack.yaml
  cover_art.png          # 1400×1400px, PNG, <2MB
  presets/
    preset_name.xometa
  samples/               # optional
    stems/
  LICENSE.txt
```

### 2.2 Automated QA Gate (`xpn_community_qa.py`)

On submission receipt, a bot calls `xpn_community_qa.py` and posts the result to a private
moderator channel. The script runs these checks:

**Structural checks (pass/fail):**
- `pack.yaml` present and schema-valid
- `cover_art.png` present, ≥1400×1400px, <2MB, no ICC profile
- All files in `presets/` have `.xometa` extension
- `LICENSE.txt` present with contributor signature line
- No `.DS_Store`, `__MACOSX`, or hidden files

**Content checks (scored 0–100):**
- Preset count: minimum 5 (0 pts if < 5), up to 50 pts at 20+ presets
- `.xometa` schema validation: 5 pts per valid preset, capped at 25 pts
- Sonic DNA completeness: all 6 dimensions populated = 10 pts
- Macro assignments (M1–M4 populated) = 10 pts
- Mood tagging consistency = 5 pts

**QA score floor: 70/100**. Packs scoring below 70 are auto-rejected with a detailed breakdown
sent to the submitter. Packs scoring 70+ advance to human curation review.

**Sample compliance scan:**
- Runs a duration/silence/clipping check on all WAV files
- Flags samples >10 seconds (likely loops, not stems — requires review)
- Does NOT run copyright fingerprinting (out of scope for v1; note for v2)

### 2.3 Curation Review

Packs that pass automated QA enter a review queue visible to Moderators + XO_OX owners.
Review criteria:

1. **Sound quality**: Does it represent XO_OX sonic standards?
2. **Engine fit**: Does it use the target engine(s) meaningfully?
3. **Preset depth**: Are macros doing real work? Are presets musically distinct?
4. **Cover art**: On-brand, original, not derivative of existing XO_OX art

Curation decision options: `Accept as Featured` / `Accept as Community Unlisted` / `Reject with notes`.

### 2.4 Turnaround SLA

| Stage | Target |
|-------|--------|
| Automated QA (bot) | < 1 hour |
| Curation review | ≤ 7 business days |
| Creator notification | Within 24 hours of decision |
| Featured release scheduling | Within 30 days of acceptance |

Submitters receive a confirmation email with their QA score breakdown within 1 hour of submission.
If no decision is issued within 10 business days, the submission is auto-accepted as Community
Unlisted (no featured release, but included in the free community pack library).

---

## 3. Creator Economy

### 3.1 Revenue Share Model

**Featured packs** sold on Gumroad are revenue-shared on a **50/50 basis** (XO_OX / Creator),
paid monthly once the pack earns ≥ $10.

- XO_OX handles hosting, marketing, customer support, and payment processing
- Creator retains credit and the right to sell the same pack independently elsewhere
- Non-featured (Community Unlisted) packs are free-only; no revenue share applies

**Pricing tiers** (creator chooses, XO_OX approves):

| Tier | Pack Size | Suggested Price |
|------|-----------|-----------------|
| Single Engine | 1 engine, ≥ 10 presets | $5–$8 |
| Multi-Engine | 2–4 engines, ≥ 25 presets | $12–$18 |
| Full Expansion | 4+ engines, ≥ 50 presets, stems included | $20–$30 |

### 3.2 Attribution Requirements

All featured packs must include attribution in `expansion.json`:

```json
{
  "pack_name": "Submerged",
  "version": "1.0",
  "creator": {
    "handle": "beatmaker_x",
    "real_name": "Optional",
    "url": "https://beatmaker-x.com",
    "discord": "beatmaker_x#0001"
  },
  "xo_ox_featured": true,
  "revenue_share": "50/50",
  "release_date": "2026-04-01",
  "engines": ["OPAL", "DRIFT"],
  "mood_tags": ["Atmosphere", "Flux"],
  "qa_score": 84
}
```

Attribution appears in:
- Gumroad product page (creator name + link)
- XO_OX.org pack listing (creator card)
- Pack's README (auto-generated from `expansion.json`)
- Discord #pack-releases announcement

### 3.3 Intellectual Property

- Creator retains ownership of all original samples and presets
- XO_OX receives a non-exclusive license to distribute the pack via XO_OX channels
- Creator may withdraw a pack with 30 days notice (existing purchasers retain access)
- XO_OX may remove a pack immediately if copyright infringement is confirmed

---

## 4. Feedback Loop Infrastructure

### 4.1 Bug Reports

`#bug-reports` enforces a thread format via pinned template:

```
**Pack**: [Pack name + version]
**Engine**: [Engine name]
**MPC OS version**: [e.g., 3.5.1]
**Preset name**: [which preset]
**Steps to reproduce**:
**Expected**:
**Actual**:
**Audio clip (optional)**:
```

Bot auto-tags each thread with `triage` status. Moderators upgrade to `confirmed` / `needs-info` /
`resolved`. Pack creators are pinged on confirmed bugs against their packs.

XO_OX reviews bug threads weekly. Patterns (≥ 3 reports on the same issue) are escalated to
pack creator with a 14-day fix window before the pack is delisted for revision.

### 4.2 Feature Request Funnel

`#feature-requests` uses Discord forum mode with upvote reactions (👍 emoji count tracked).
Format: title = the feature, body = use case description.

**Processing cadence:**
- Weekly: XO_OX reviews all new requests
- Monthly: Top 5 most-upvoted requests are discussed in XO_OX's pack planning session
- Quarterly: Community votes on which approved requests ship in the next 90-day sprint

Requests fall into three categories:
1. **Pack requests** — new engine themes, moods, or genre focuses → added to pack roadmap
2. **Format requests** — XPN structure, QLink defaults, kit modes → forwarded to Tools/ backlog
3. **Engine feature requests** — parameter additions → filed as engine issues in XOmnibus repo

### 4.3 Feedback → Pack Design Decision

Each quarterly planning session begins with a feedback synthesis document (owner: XO_OX lead).
It aggregates:
- Top 5 feature requests by upvote
- Most-reported bug categories
- Showcase posts that reveal unexpected use patterns (e.g., community using OPAL as a drum texture
  engine — implies a dedicated OPAL drum pack has demand)
- Challenge submissions that reveal gaps (e.g., no community could use the OHM engine well → need
  a tutorial pack)

This synthesis directly shapes the next quarter's official pack releases and the monthly challenge topic.

---

## 5. Content Calendar

### Monthly Community Challenge

**Structure:**
- Challenge announced on the 1st of each month in #announcements
- 30-day window to submit a track, clip, or preset pack using the challenge constraint
- Submissions go in #showcase tagged `#challenge-[month]`
- XO_OX + Moderators vote on winner by the 5th of the following month
- Winner announced + featured in #pack-releases

**Prize tiers:**
- 1st place: Featured pack release slot + 50/50 revenue split + XO_OX Beta role
- 2nd place: Community Unlisted release + Pack Creator role
- Top 5: Highlighted in monthly newsletter + permanent #showcase pin

**Sample challenge calendar (Year 1):**

| Month | Challenge | Focus Engine |
|-------|-----------|--------------|
| April 2026 | OBLONG Challenge — most creative Bob engine use | OBLONG |
| May | Underwater World — pure OPAL textures | OPAL |
| June | Drum Machine Wars — ONSET vs. anything else | ONSET |
| July | Open Coupling Challenge — use ≥ 2 coupled engines | Multi-engine |
| August | One Preset Challenge — one preset, full track | Any |
| September | Character Study — OVERBITE extreme sound design | OVERBITE |
| October | Ghost Season — ORACLE stochastic compositions | ORACLE |
| November | Vintage Chip — OVERWORLD ERA triangle deep dive | OVERWORLD |
| December | Year Review Pack — curated "best of 2026" submission | All engines |

Challenges are announced 2 weeks in advance so producers have time to plan.

---

## 6. Growth Tactics

### 6.1 Seed Phase (Months -2 to 0: Before Public Launch)

Recruit 10 beta producers who receive:
- Early access to all XO_OX packs before public release
- Permanent Pack Creator + XO_OX Beta roles
- Their name in the XO_OX website community credits
- First right of refusal on monthly challenge judging spots

**Seed producer sourcing:**
- MPC Forums (r/mpcusers, official Akai forums): post in existing "what are you using" threads
- r/MPC subreddit: identify active, technically knowledgeable commenters
- YouTube: reach out to MPC tutorial creators with ≥ 5K subscribers (not mega influencers —
  craft-focused mid-tier creators convert better for a niche community)
- Direct DM to producers who have shared MPC beats using synthesis (not just chops)
- Twitter/X: search "MPC + VST" or "MPC + synthesis" threads

**Seed producer criteria:**
- Active MPC user (hardware or software)
- Posts music publicly
- Technically inclined (not intimidated by synthesis)
- Communicates constructively

### 6.2 Launch Phase (Month 0–1)

- Post server invite in r/MPC, r/synthesizers, r/edmproduction simultaneously
- Coordinate with 3 beta producers to post "I've been using this" content on launch day
- Submit to MPC forums "Resource" thread with a free pack as the hook
- XO_OX.org front page gets a "Join the Community" CTA banner

**Free launch pack**: A curated "Sampler Pack" with 1 preset per major engine (OBLONG, OPAL,
ONSET, OVERDUB, ODYSSEY) — 5 presets total, no purchase required, Discord-gated download.
This is the primary acquisition mechanism for Month 0.

### 6.3 First 90 Days Plan

| Week | Action |
|------|--------|
| W1 | Server live; seed producers active; free sampler pack gated download |
| W2 | First #mpc-tips post from XO_OX (tutorial: using Q-Links with community packs) |
| W3 | Announce Month 1 challenge (OBLONG) |
| W4 | First community pack submission window opens |
| W5–W8 | First challenge runs; moderate #showcase activity; curate first community submissions |
| W9 | First challenge winner announced + featured pack drops |
| W10–W12 | Second challenge announced; first revenue share payout (if applicable); first bug fix cycle |

**90-day success metrics:**
- 200 Discord members
- 10 community pack submissions received
- 3 packs accepted (auto QA pass + curation)
- 1 featured community pack released
- 50 showcase posts

---

## 7. Moderation Policy

### 7.1 Content Standards for Community Packs

**Hard requirements (automatic rejection):**
- No samples sampled from commercial recordings without clearance
- No AI-generated samples presented as original synthesis (AI-assisted is acceptable; must be
  declared in `pack.yaml` as `sample_origin: ai_assisted`)
- No offensive imagery in cover art (no sexual content, no hate symbols)
- No third-party brand logos or artwork in cover art

**Quality floor:**
- Automated QA score ≥ 70/100 (enforced by `xpn_community_qa.py`)
- Minimum 5 presets per submission
- All presets must load without error in the target XOmnibus version

**Attribution:**
- All samples derived from external sources (even cleared) must be noted in `LICENSE.txt`
- Creator handle must match the Discord account that submitted the form

### 7.2 Community Conduct Standards

- No unsolicited DMs promoting packs or services
- No posting of pirated content (samples, plugins, DAWs)
- No spam-posting the same content across multiple channels
- Critique is welcome; harassment is not — distinguish the work from the person

**Enforcement ladder:**
1. Warning (DM from Moderator)
2. 24-hour timeout
3. 7-day timeout
4. Permanent ban (requires XO_OX owner approval)

Pack submissions from banned users are automatically rejected.

### 7.3 Copyright Infringement Response

On confirmed copyright infringement in a community pack:
1. Pack is delisted immediately (same-day)
2. Submitter is DMed with explanation
3. Submitter's Pack Creator role is suspended pending review
4. If the submitter knowingly submitted infringing content, permanent ban
5. XO_OX notifies any affected rights holders if the pack was sold

---

## 8. Long-Term Sustainability

### 8.1 Paid Membership Tier

**XO_OX Patron** (Patreon, $5–$10/month):
- Access to exclusive monthly "Patron Pack" — a community pack that never goes on general sale
- Voting rights in the quarterly Engine Concept Poll (see §8.3)
- Early access to featured community packs (48 hours before public release)
- Patron-only Discord channel: `#patron-lounge`

**Target Patreon revenue at 200 patrons ($7 avg)**: ~$1,400/month — covers Discord hosting,
QA automation infra, and XO_OX owner time for community management.

### 8.2 Exclusive Community Packs for Patreon

Each month, one community pack is designated as a Patron Exclusive. It is:
- Not listed on Gumroad
- Distributed only via Patreon as a "bonus" attachment
- Still revenue-shared (50/50 of Patreon proceeds attributed to that month's pack)

This creates a direct incentive for productive community members: the most impressive submission
each month becomes the Patron Exclusive — guaranteed audience and revenue.

### 8.3 Community Voting on Engine Concepts

Quarterly, XO_OX publishes a poll (Discord + Patreon) with 3–5 engine concepts from the open
design pool (OSTINATO, OPENSKY, OCEANDEEP, OUIE, and future V2 concepts). Patron votes count 3×
vs. free member votes. The winning concept gets:
- Priority slot in the XOmnibus engine roadmap
- A dedicated community pack challenge on launch
- The voting community gets credited in the engine's identity card as "Championed by the XO_OX
  Community"

This closes the loop between community engagement and product direction — the community feels
genuine ownership over what gets built next.

### 8.4 Community Pack Library Growth Target

| Year | Featured Packs | Community Unlisted | Total Presets (community) |
|------|---------------|-------------------|--------------------------|
| 2026 | 12 | 24 | ~600 |
| 2027 | 30 | 60 | ~2,000 |
| 2028 | 60 | 120 | ~5,000 |

At 60 featured packs (2028), the community preset library rivals the factory library in size — at
which point XO_OX considers a "Community Edition" standalone release: a curated collection of the
best community presets, sold as an official expansion with full revenue share to all contributing
creators.

---

## Implementation Priority

| Priority | Action | Owner | Timeline |
|----------|--------|-------|----------|
| P0 | Stand up Discord server (channels + roles + bot) | XO_OX | Before public launch |
| P0 | Write `xpn_community_qa.py` v1 (structural + scoring) | XO_OX | Before submission window opens |
| P0 | Create Google Form for submissions | XO_OX | Before submission window opens |
| P1 | Draft `expansion.json` schema (attribution fields) | XO_OX | Month 1 |
| P1 | Recruit 10 seed producers | XO_OX | Months -2 to 0 |
| P1 | Produce + release free sampler pack (5 presets) | XO_OX | Launch day |
| P2 | Set up Patreon page with Patron Pack tier | XO_OX | Month 2 |
| P2 | Launch first monthly challenge (OBLONG, April) | XO_OX | Month 1 |
| P3 | Launch first Patron Exclusive community pack | XO_OX | Month 3 |
| P3 | First quarterly Engine Concept Poll | XO_OX | Month 3 |

---

## Open Questions

1. **Bot infrastructure**: Build custom Discord bot vs. use existing (e.g., Carl-bot + custom
   slash commands)? Custom gives tighter QA integration; Carl-bot is faster to deploy.
2. **Gumroad vs. own storefront**: Gumroad is lowest-friction for v1 but takes 10% + Stripe fees.
   At scale, a Lemon Squeezy or direct Stripe integration improves margins.
3. **AI-generated samples**: Current policy is "declare and accept." If community perception shifts
   (as it did in visual art communities), may need to move to outright ban. Revisit at Month 6.
4. **Pack version management**: When creators update packs, how are purchasers notified?
   Gumroad supports version uploads with buyer email notification — confirm this flow works.
5. **International creators**: Revenue share via PayPal/Wise for non-US creators. Confirm tax
   compliance requirements (W-8BEN for US Gumroad, similar for Patreon).

---

*XO_OX Designs — R&D Spec | 2026-03-16*
