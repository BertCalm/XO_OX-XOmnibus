# XPN Distribution Economics — R&D Spec

**Date**: 2026-03-16
**Status**: R&D / Financial Modeling
**Owner**: XO_OX Designs
**Audience**: Internal strategy, product pricing, release planning

---

## Overview

XO_OX packs are zero-COGS digital products — every sale is pure margin minus platform fees and time value. This document models the full financial picture: channel economics, pricing sensitivity, Year 1 projections, Patreon de-risking, cost structure, and the self-sustaining threshold question. The goal is to make pack release decisions based on real math, not gut feel.

The short answer: **12 packs at mixed price points + 100 Patreon subscribers by Month 6 = self-sustaining by Month 8.**

---

## 1. Revenue Model Per Channel

### 1a. Gumroad

- **Fee**: 10% flat on gross sales
- **Net margin**: 90% of sale price
- **Mechanics**: Direct link, built-in email capture, discount codes, pay-what-you-want floors, bundle products
- **Email list value**: Every buyer is a subscriber. Gumroad's email tool makes follow-on pack launches free marketing.
- **Best for**: Launch channel. Simple. Trusted by producers. No storefront to maintain.

| Sale Price | Gumroad Fee | Net Per Sale |
|-----------|-------------|-------------|
| $9.00     | $0.90       | $8.10       |
| $18.00    | $1.80       | $16.20      |
| $35.00    | $3.50       | $31.50      |
| $75.00 (bundle) | $7.50 | $67.50    |

### 1b. XO-OX.org Direct (Stripe)

- **Fee**: 2.9% + $0.30 per transaction
- **Net margin**: Higher than Gumroad, but requires payment integration, fulfillment logic, and download delivery
- **Mechanics**: Full control over UX, no platform dependency, no competing products in sidebar
- **Break-even vs. Gumroad**: Stripe wins above ~$14 sale price (fee differential exceeds Gumroad 10%)
- **Recommendation**: Phase 2 (Month 4+). Launch on Gumroad first; migrate high-volume SKUs to direct when dev bandwidth exists.

| Sale Price | Stripe Fee  | Net Per Sale | Advantage vs. Gumroad |
|-----------|-------------|-------------|----------------------|
| $9.00     | $0.56       | $8.44       | +$0.34               |
| $18.00    | $0.82       | $17.18      | +$0.98               |
| $35.00    | $1.32       | $33.68      | +$2.18               |
| $75.00    | $2.48       | $72.52      | +$5.02               |

### 1c. Patreon

- **Fee**: 5–12% depending on plan tier (Pro = 8%, Premium = 12%, standard = ~10% effective)
- **Net margin**: ~88–95% of monthly pledge
- **Mechanics**: Recurring revenue, community belonging, tiered benefits, early access
- **XO_OX tiers**:
  - **Listener** ($10/month) — updates + behind-the-scenes
  - **Producer** ($25/month) — monthly free pack + Discord access
  - **Architect** ($75/month) — all packs + early access + name in credits

Patreon's value is not the per-unit margin — it is the **predictable floor** that removes existential risk from development decisions. See Section 4 for full Patreon modeling.

### 1d. Marketplace (Future — Year 2+)

- **Akai MPC Marketplace**: 30–40% revenue share. Reach is significant (every MPC owner sees the storefront). Margin is poor. Best for high-volume low-price discovery packs.
- **MPC-Samples.com**: Similar split. Established audience. Useful for distribution breadth.
- **Strategic read**: Marketplaces are awareness tools, not income tools. Use them for entry-level packs ($9 SIGNAL tier) to pull buyers into the Gumroad/direct ecosystem. Never list flagship or bundle products exclusively on marketplace.

| Channel       | Fee    | Net $18 Pack | Notes                          |
|--------------|--------|-------------|-------------------------------|
| Gumroad      | 10%    | $16.20      | Launch default                |
| XO-OX Direct | ~4.6%  | $17.18      | Phase 2, dev cost to build     |
| Patreon      | ~10%   | N/A (recurring) | Recurring floor             |
| MPC Marketplace | 30–40% | $10.80–12.60 | Discovery only                |

---

## 2. Pricing Sensitivity Analysis

### Core tiers

Three natural price points map to product depth and producer budget tolerance:

| Tier     | Price | Positioning                                      |
|---------|-------|--------------------------------------------------|
| SIGNAL  | $9    | Entry — one engine, focused use case, max reach |
| FORM    | $18   | Standard — dual-engine, 3–4 kits, full workflow |
| DOCTRINE| $35   | Premium — collection anchor, seance-curated       |

### Break-even: $500 effort value + platform fees

Assumption: $500 represents one focused work session (design, render, QA, publish).

| Pack     | Price | Net/Sale | Sales Needed | Days to 56 Sales (1 sale/day) |
|---------|-------|----------|-------------|-------------------------------|
| SIGNAL  | $9    | $8.10    | 62          | 62 days                       |
| FORM    | $18   | $16.20   | 31          | 31 days                       |
| DOCTRINE| $35   | $31.50   | 16          | 16 days                       |

**Insight**: DOCTRINE packs recover effort cost fastest with lowest sales count. The barrier is audience trust — buyers need to believe a $35 pack is worth it. Build trust with SIGNAL/FORM packs first, then launch DOCTRINE to an established audience.

### Bundle pricing math

Kitchen Essentials collection: 6 packs × $18 list price = **$108 unbundled value**

| Bundle Price | Discount | Net Revenue (Gumroad) | Effective Per-Pack Revenue |
|-------------|----------|----------------------|---------------------------|
| $75         | 31%      | $67.50               | $11.25                    |
| $65         | 40%      | $58.50               | $9.75                     |
| $89         | 18%      | $80.10               | $13.35                    |

**Recommended**: $75 bundle ($67.50 net). This is a strong value signal to buyers while maintaining healthy margin. The bundle should only be available after all 6 individual packs exist — don't pre-sell incomplete collections.

### Price increase strategy

**Rule: never reduce prices.** Intro pricing is a promise that early buyers got the best deal. The mechanics:

1. Launch each pack at intro price (e.g., FORM at $15)
2. After 30 days or 50 sales, move to standard price ($18)
3. Never announce discounts — only announce price increases
4. Bundle pricing is structurally lower, not a "sale"

This approach rewards the early adopter community and builds urgency without manufactured scarcity. As the catalog grows and reputation solidifies, prices for new packs can start higher — the track record justifies it.

---

## 3. Year 1 Revenue Projection

**Assumptions**:
- 12 packs launched across the year (1 per month average)
- Mixed pricing: 5 SIGNAL ($9) + 5 FORM ($18) + 2 DOCTRINE ($35)
- Primary channel: Gumroad (90% of sales Year 1)
- Catalog effect: older packs continue selling at declining rate

### Scenario modeling (monthly average by end of Year 1)

| Scenario    | Total Sales/Month | Avg Net/Sale | Monthly Revenue | Annual Run Rate |
|------------|-------------------|-------------|-----------------|-----------------|
| Conservative | 50               | $18.00      | $900            | $10,800         |
| Moderate    | 150               | $18.00      | $2,700          | $32,400         |
| Optimistic  | 400               | $18.00      | $7,200          | $86,400         |

**Notes on the scenarios**:
- **Conservative**: Likely at launch without marketing — organic discovery only, small audience
- **Moderate**: Achievable with consistent social content, 3–4 YouTube demos, 1 MPC forum presence
- **Optimistic**: Requires a breakout moment — viral demo, marketplace placement, or influencer pickup

### Monthly ramp (conservative scenario, 12-month arc)

| Month | Packs Live | Sales/Month | Revenue |
|-------|-----------|-------------|---------|
| 1     | 1         | 10          | $162    |
| 2     | 2         | 18          | $292    |
| 3     | 3         | 25          | $405    |
| 4     | 4         | 32          | $518    |
| 5     | 5         | 38          | $616    |
| 6     | 6         | 43          | $697    |
| 7     | 8         | 46          | $745    |
| 8     | 9         | 49          | $794    |
| 9     | 10        | 51          | $827    |
| 10    | 11        | 53          | $859    |
| 11    | 12        | 55          | $892    |
| 12    | 12        | 57          | $924    |
| **Total** |       |             | **$7,531** |

Year 1 total (conservative): ~$7,500. This is below self-sustaining, which is why Patreon matters.

---

## 4. Patreon Recurring Revenue

### Why Patreon changes the math

Pack sales are lumpy — big at launch, declining curve after. Patreon is a **monthly floor** that:
- Funds development before packs are ready to sell
- Removes the "need 30 sales to justify this release" anxiety
- Converts audience into community with ongoing relationship

### Patreon scenarios

| Tier        | Price  | Month 3 | Month 6 | Month 12 |
|------------|--------|---------|---------|---------|
| Listener   | $10    | 25      | 100     | 200     |
| Producer   | $25    | 15      | 60      | 100     |
| Architect  | $75    | 5       | 20      | 35      |

**Month 3 revenue** (25/15/5):
- Gross: (25×$10) + (15×$25) + (5×$75) = $250 + $375 + $375 = $1,000
- Net (~90%): **$900/month**

**Month 6 revenue** (100/60/20):
- Gross: (100×$10) + (60×$25) + (20×$75) = $1,000 + $1,500 + $1,500 = $4,000
- Net: **$3,600/month**

**Month 12 revenue** (200/100/35):
- Gross: (200×$10) + (100×$25) + (35×$75) = $2,000 + $2,500 + $2,625 = $7,125
- Net: **$6,413/month**

### How Patreon de-risks the development cycle

The Producer tier ($25/month) includes one free pack per month. At 60 Producer subscribers by Month 6, that is $1,500/month dedicated specifically to pack development. Each pack costs roughly $100–200 in Claude API time + 4–6 hours of human time. Patreon revenue at Month 6 covers:

- 3–4 pack development cycles per month
- Hosting and infrastructure
- Tooling and subscriptions

The crucial shift: **Patreon turns pack development from a revenue-seeking activity into a service activity.** You build packs because subscribers are waiting for them, not because you need to hit a sales target.

---

## 5. Cost Structure

XO_OX digital products have near-zero COGS. All costs are operational.

### Monthly fixed costs

| Cost Item           | Amount  | Notes                                      |
|--------------------|---------|--------------------------------------------|
| Claude API         | ~$100   | Pack design, preset generation, QA agents  |
| Web hosting        | ~$20    | XO-OX.org (static + CDN)                  |
| Gumroad plan       | $0      | Percentage-based, no monthly fee           |
| Domain renewal     | ~$2     | Amortized monthly                          |
| DAW/tools          | ~$0     | Already owned                              |
| **Total fixed**    | **~$122/month** |                                  |

### Variable costs (per pack)

| Cost Item           | Per Pack | Notes                                      |
|--------------------|---------|--------------------------------------------|
| Claude API (design) | $20–50  | Concept + presets + QA agents              |
| Render time         | $0      | CPU, already owned                         |
| Cover art           | $0      | Generated via existing tools               |
| Upload/publish      | $0      | Time-only                                  |
| **Total variable** | **$20–50** |                                         |

### Time value (shadow cost)

Not a real cash expense but matters for break-even reasoning. A focused pack release session:
- Design + preset writing: 3–4 hours
- Render + QA + export: 1–2 hours
- Publish + announcement: 1 hour
- **Total**: ~5–7 hours per pack

At a notional $75/hour consulting rate: $375–525 per pack. This is the "effort value" number used in Section 2 break-even tables.

### COGS = Zero

Once a pack is made, the marginal cost of the 500th sale is $0.00. The Gumroad fee is proportional, not incremental infrastructure cost. This is the core economic advantage of digital products — every additional sale is pure margin.

---

## 6. Break-Even on XOmnibus Development

### The question

XOmnibus represents 12+ months of development effort. At what monthly revenue does XO_OX become **self-sustaining** — covering its own operating costs and compensating creative time?

### Definition of self-sustaining

Threshold: monthly revenue covers:
1. All fixed costs (~$122/month)
2. Reasonable time compensation for ongoing work (~20 hours/month × $50/hour = $1,000/month)
3. Buffer for development expenses and growth (~$300/month)

**Self-sustaining threshold: ~$1,422/month (~$1,500 rounded)**

### When does XO_OX reach $1,500/month?

| Revenue Source       | Month 3  | Month 6  | Month 9  |
|--------------------|---------|---------|---------|
| Pack sales (conservative) | $500 | $800 | $900 |
| Patreon (conservative)   | $900 | $3,600 | $5,200 |
| **Combined**             | **$1,400** | **$4,400** | **$6,100** |

**Self-sustaining by Month 3–4** if Patreon and pack sales build concurrently. This is realistic if:
- Patreon is launched at the same time as the first pack
- The Producer tier ($25) is positioned as the default subscription (not the $75 Architect)
- Monthly free pack for Producer subscribers creates immediate tangible value

### Recovering XOmnibus development cost

At $1,500/month net after reaching self-sustaining, the running surplus goes toward recovering sunk development cost. This is a long-horizon question — XOmnibus is the catalog, not the product. The product is the packs. XOmnibus development cost is recovered through the lifetime value of the ecosystem it enables.

---

## 7. Price Increase Strategy (Full Protocol)

### Principles

1. **Never reduce prices** — discounting signals low confidence and trains buyers to wait
2. **Intro pricing is a reward**, not a feature — early adopters get the best price as a thank-you
3. **Price rises over time** as catalog, reputation, and community size grow
4. **Bundle discounts are structural**, not promotional — bundles exist to sell more at once, not to clear old stock

### Protocol per pack release

```
Launch at intro price → 30-day or 50-sale trigger → move to standard price → never look back
```

| Event                         | Action                                           |
|------------------------------|--------------------------------------------------|
| Pack releases                 | Announce intro price + limited window (30 days)  |
| 30-day or 50-sale mark        | Price moves to standard, no announcement needed  |
| Catalog reaches 6+ packs      | Bundle launches at structural discount           |
| 12+ months of catalog         | New packs can launch at higher intro price       |
| Reputation milestone (press, viral) | New packs launch at premium, existing unchanged |

### Price escalation over time (FORM tier example)

| Period          | Intro Price | Standard Price | Rationale                     |
|----------------|-------------|---------------|-------------------------------|
| Months 1–6     | $15         | $18           | Building trust, small audience |
| Months 7–12    | $18         | $22           | Established catalog, repeat buyers |
| Year 2+        | $22         | $25           | Known brand, community proof   |

This creates a permanent archive effect: buyers who found XO_OX early locked in lower prices. Word-of-mouth carries this. The community becomes promotional agents because they feel they got the better deal.

---

## Summary: The Self-Sustaining Model

| Component                  | Month 3   | Month 6   | Month 12  |
|---------------------------|-----------|-----------|-----------|
| Pack sales (net)           | $500      | $800      | $1,500    |
| Patreon (net)              | $900      | $3,600    | $6,400    |
| **Total monthly**          | **$1,400**| **$4,400**| **$7,900**|
| Fixed costs                | $122      | $122      | $122      |
| **Net surplus**            | **$1,278**| **$4,278**| **$7,778**|

The model is not about pack sales. **It is about Patreon.** Packs are the proof of value. Patreon is the business. Build the packs, publish the packs, grow the Patreon, and the development cycle becomes self-funding within one quarter.

---

## Open Questions

1. **Patreon launch timing**: Before or simultaneously with first pack? Argument for before: establishes recurring base before any product exists, purely on brand trust.
2. **iOS / AUv3 monetization**: XOmnibus as AUv3 — App Store pricing model separate from pack economics. Not modeled here.
3. **Wholesale / licensing**: Licensing XO_OX engine presets to hardware manufacturers (Akai OEM). High-margin, high-effort to negotiate. Year 3 conversation.
4. **Email list velocity**: Gumroad email capture is only valuable if used. Monthly email = pack announcement cadence. What is the right cadence without burning the list?
5. **Patreon vs. Substack**: Substack handles payments + publishing in one tool. Worth evaluating as an alternative if Patreon fee tier is unfavorable.
