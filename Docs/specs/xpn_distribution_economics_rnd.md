# XPN Distribution Economics — R&D Spec
**Date**: 2026-03-16
**Status**: Working Model (update as actuals come in)

---

## 1. Platform Fee Structure

| Platform | Fee Structure | Notes |
|---|---|---|
| **Gumroad** | 10% flat + 2.9% + $0.30/txn | Simple, strong discovery layer |
| **Bandcamp** | 15% until $5K lifetime revenue, then 10% | Music-adjacent community, good trust signals |
| **Akaipro.com** | ~30–40% rev share (est.) | First-party MPC store, industry-standard split for partner programs |
| **Splice** | ~$0.02–$0.05/sample download | Per-sample micropayments; samples only, not full .xpn programs |
| **XO-OX.org (Stripe)** | 2.9% + $0.30/txn | Highest margin, own the customer relationship |

The Gumroad fee is actually layered: 10% platform fee plus the underlying payment processing (2.9% + $0.30). At a $25 price point the effective combined take is about 12.9% plus $0.30. Akaipro.com does not publish its split publicly — the 30–40% estimate is the industry standard for first-party hardware manufacturer stores. Splice revenue is too small per download to be a meaningful income source; its value is pure discovery.

---

## 2. Net Revenue Per Sale by Price Point and Platform

Formula: `Net = Price × (1 - platform_rate) - fixed_fee`

### Gumroad (10% flat + 2.9% + $0.30)

| Price | Platform Take | Net | Margin |
|---|---|---|---|
| $9 | $1.46 | $7.54 | 83.8% |
| $15 | $2.24 | $12.76 | 85.1% |
| $25 | $3.53 | $21.47 | 85.9% |
| $35 | $4.82 | $30.18 | 86.2% |

### Bandcamp (15% early-phase; 10% after $5K lifetime)

| Price | Net (15% phase) | Net (10% phase) |
|---|---|---|
| $9 | $7.65 | $8.10 |
| $15 | $12.75 | $13.50 |
| $25 | $21.25 | $22.50 |
| $35 | $29.75 | $31.50 |

*Note: Bandcamp charges the platform fee on top of standard payment processing. The above nets assume Bandcamp absorbs payment processing (their published model) and the fee is 15% or 10% of gross.*

### Akaipro.com (35% rev share estimate, no additional transaction fee)

| Price | Net |
|---|---|
| $9 | $5.85 |
| $15 | $9.75 |
| $25 | $16.25 |
| $35 | $22.75 |

### XO-OX.org Direct / Stripe (2.9% + $0.30)

| Price | Net | Margin |
|---|---|---|
| $9 | $8.44 | 93.8% |
| $15 | $14.27 | 95.1% |
| $25 | $24.03 | 96.1% |
| $35 | $33.80 | 96.6% |

**Key comparison**: A $25 sale on Akaipro.com nets $16.25. The same sale direct nets $24.03 — 48% more revenue per transaction. Volume on Akaipro.com would need to be approximately 1.5× direct sales volume just to break even on the margin loss.

---

## 3. Volume Targets to Hit Monthly Revenue Milestones

Using net per sale on each platform at each price point:

### At $9/pack

| Monthly Goal | Direct | Gumroad | Akaipro |
|---|---|---|---|
| $500/mo | 60 sales | 67 sales | 86 sales |
| $1,000/mo | 119 sales | 133 sales | 171 sales |
| $2,000/mo | 237 sales | 265 sales | 342 sales |

### At $15/pack

| Monthly Goal | Direct | Gumroad | Akaipro |
|---|---|---|---|
| $500/mo | 36 sales | 40 sales | 52 sales |
| $1,000/mo | 71 sales | 79 sales | 103 sales |
| $2,000/mo | 141 sales | 157 sales | 206 sales |

### At $25/pack

| Monthly Goal | Direct | Gumroad | Akaipro |
|---|---|---|---|
| $500/mo | 21 sales | 24 sales | 31 sales |
| $1,000/mo | 42 sales | 47 sales | 62 sales |
| $2,000/mo | 84 sales | 94 sales | 123 sales |

### At $35/pack

| Monthly Goal | Direct | Gumroad | Akaipro |
|---|---|---|---|
| $500/mo | 15 sales | 17 sales | 22 sales |
| $1,000/mo | 30 sales | 34 sales | 44 sales |
| $2,000/mo | 60 sales | 67 sales | 88 sales |

**Insight**: $25 direct requires only 21 sales per month to clear $500. With a catalog of 5+ packs each generating 4–5 organic sales per month, that target is achievable without any paid marketing. The $35 tier halves the required volume but demands stronger brand credibility to close the purchase.

---

## 4. Pack Lifecycle Model

Historical data from comparable independent music software products (sample packs, preset libraries, MPC expansions) shows a consistent three-phase curve:

**Month 1–3: Launch Window — 40–60% of lifetime sales**
The first few weeks after a launch announcement drive the majority of unit movement. Email to the list, social posts, and MPC forum threads all spike together. This phase is front-loaded: the first 7 days are the sharpest peak. A Patreon early-access release 2–4 weeks before public launch lets subscribers capture this window at a discount while generating word-of-mouth ahead of the public drop.

**Month 4–12: Long Tail — 30–40% of lifetime sales**
Search discovery, catalog browsing, and cross-promotion from subsequent pack releases sustain a steady trickle. When pack #4 launches, the launch email mentions packs #1–3 in the footer — returning buyers often purchase a back-catalog title in the same session. Each new release is a promotional event for the entire catalog.

**Year 2+: Catalog Passive — 10–20% of lifetime sales**
Revenue continues at low-but-real volume with zero marginal cost after production. A 20-pack catalog generating an average of 2–3 passive sales per pack per month produces meaningful recurring income from work that was done years earlier.

**Implication for launch strategy**: The launch window is disproportionately valuable. Patreon early access, pre-release social content, and coordinated announcement timing all serve to maximize sales in this first window. Delaying a launch by two weeks to do more prep is nearly always a mistake — the opportunity cost of missing peak launch momentum exceeds the benefit of incremental polish.

---

## 5. Catalog Value Calculation

**Monthly passive income estimate per pack** (long-tail phase, months 4–12):

Assume a $25 pack averages 6 organic sales per month during the long-tail phase across all channels, with 80% of those on direct/Stripe and 20% on Gumroad. Blended net per sale: approximately $22.50.

- 1 pack in catalog: 6 × $22.50 = **$135/month**
- 5 packs: ~5 × 5 sales × $22.50 = **$563/month** (slight organic decay, cross-promo compensates)
- 10 packs: ~10 × 4 sales × $22.50 = **$900/month**
- 20 packs: ~20 × 3 sales × $22.50 = **$1,350/month**

**Break-even for $500/month passive** (long-tail phase only, no launch spike contribution):
Approximately 4–5 packs at $25 each generating consistent organic traffic. Achievable within 9–12 months of quarterly releases if community presence is maintained.

**Catalog flywheel mechanics**: Each new pack release provides a reason to email the full list. That email generates traffic to the store page where all packs are visible. Conversion rate on back-catalog from launch emails is reported at 8–15% for engaged music software lists — meaning 100 list subscribers clicking through results in roughly 8–15 additional back-catalog purchases per launch event. With a 10-pack catalog and monthly releases, this compounds significantly.

---

## 6. Platform Mix Recommendation

**XO-OX.org direct (Stripe) — Primary channel**
96%+ margin, full control over UX, own the customer email address. The brand premium that XO_OX commands — deep engine-specific presets, narrative identity, Aquatic Mythology lore — is best expressed through a direct storefront. Buyers who arrive at XO-OX.org are already sold on the brand before they see the price. Every sale here is worth 1.5× a Gumroad sale in net revenue.

**Gumroad — Second channel for discovery and email capture**
Free packs (10-preset engine samplers, one per flagship engine) live on Gumroad specifically to capture email addresses from buyers who would never search directly for XO-OX.org. Gumroad's marketplace browsing surfaces these free packs to new audiences. A buyer who downloads a free OPAL sampler pack is a warm prospect for the full $25 OPAL expansion. Use Gumroad discount codes for Patreon subscribers — keeps the benefits tangible and trackable.

**Akaipro.com — Long-term goal, not immediate**
The placement value is real: buyers inside the MPC software store are already in purchase mode, and the audience is exactly right. However, the 35% rev share is a significant margin hit, and the partner application process requires demonstrated catalog depth and likely an exclusive review period. Target application once: (1) the catalog has 8+ packs, (2) monthly sales volume is documented and can be shown to a partner manager, and (3) pack quality has been validated by real MPC users. Until then, Akaipro.com is an aspiration, not a channel.

**Splice — Sample packs only, discovery vehicle only**
Full .xpn programs cannot be distributed via Splice — the format is incompatible. Rendered WAV stems from existing packs (individual drum hits, melodic loops, one-shot instruments) are Splice-appropriate. Revenue will be negligible — $20–50/month for a mid-sized contribution. The value is that Splice users who encounter XO_OX sounds are a pipeline to full pack buyers. Do not create Splice-specific content; repurpose stems that are already rendered as part of the XPN QA process.

---

## 7. First-Year Revenue Projection

**Assumptions**: 4 packs released in Year 1 (Q1, Q2, Q3, Q4). First two priced at $25, third at $25, fourth (premium collection anchor) at $35. Patreon launches at Month 2 and ramps slowly from 15 subscribers to 60 over the year. 75% of sales on direct, 25% on Gumroad.

### Conservative Scenario
- Pack sales: 180 total units across all 4 packs, blended net $22/unit = **$3,960**
- Patreon: 25 average monthly subscribers × $12 average pledge × 10 months = **$3,000**
- **Year 1 Total: ~$6,960**

### Base Scenario
- Pack sales: 320 total units, blended net $22.50/unit = **$7,200**
- Patreon: 45 average subscribers × $14 avg × 10 months = **$6,300**
- **Year 1 Total: ~$13,500**

### Optimistic Scenario
- Pack sales: 600 total units (one viral community moment), blended net $23/unit = **$13,800**
- Patreon: 80 average subscribers × $16 avg × 10 months = **$12,800**
- **Year 1 Total: ~$26,600**

The conservative path is achievable with zero paid marketing — consistent forum presence, monthly emails to the growing list, and one community post per launch. The base scenario requires one external amplification event (a YouTube demo, a well-placed Reddit post, or coverage in an MPC-focused newsletter). The optimistic scenario requires all of the above plus Akaipro.com placement by Q3.

**Bottom line**: The math works at $25 with a modest catalog. The most critical variable is not price or platform choice — it is release cadence. Four packs per year with consistent promotion outperforms ten packs released in a burst with no follow-through. Consistent presence compounds: each launch email to a slightly-larger list, each new pack promoting the slightly-deeper catalog.

---

*Update this model with actual sales data after the first pack ships. Track per-channel conversion rates and adjust platform mix accordingly. The Akaipro.com rev share estimate should be renegotiated if a partner relationship is established — 30% with premium placement is a reasonable opening ask.*
