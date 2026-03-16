# Pack Pricing, Bundle Design & Revenue Model — R&D

**Date:** 2026-03-16
**Status:** R&D / Strategic Planning
**Author:** XO_OX Designs
**Related:** `pack_economics_strategy_rnd.md`, `patreon_content_calendar_rnd.md`, `mpce_native_pack_design.md`

---

## Context

XO_OX has 34 engines, 2,550 presets, a complete XPN toolchain, and zero shipped packs. This document addresses the commercial mechanics: how packs are priced relative to market, how bundles are structured to maximize both revenue and catalog discovery, what the free flagship pack should be, how Patreon tiers interact with pack releases, whether MPCe-native packs can command a premium, and what Year 1 revenue looks like across three scenarios.

*This document is a strategic complement to `pack_economics_strategy_rnd.md` (release calendar + production cost) and `patreon_content_calendar_rnd.md` (tier structure + content). Read together.*

---

## 1. Pricing Tiers — Market Reference & Positioning

### Market Benchmarks

| Vendor | Category | Price Range | Notes |
|--------|----------|-------------|-------|
| Drum Broker | Curated drum kits | $9.99–$29.99 | Mid-tier brand signal. Slower catalog, higher trust. |
| MSXII | Keygroup / melodic packs | $14.99–$29.99 | Boutique positioning, MPC-native reputation, strong community. |
| Output | Sound design collections | $49–$199 | Premium market. Different buyer. |
| Splice | Subscription / à la carte | $7.99–$19.99/pack | Streaming model reduces upfront friction; lower perceived value per purchase. |
| MPC-Samples.com | Volume packs | $4.99–$14.99 | High volume, lower quality floor, price-competitive. |

**XO_OX position:** Mid-to-upper mid-market. Synthesis depth, liner notes, and coupling recipes justify pricing above MPC-Samples.com and on par with MSXII. Output and Native Instruments are aspirational ceiling, not Year 1 competitors.

### Recommended Price Points

| Pack Type | Price | Rationale |
|-----------|-------|-----------|
| Single engine drum kit (e.g., ONSET Drum Essentials) | $9.99 | Impulse-buy threshold. MPC forum entry price. Match Drum Broker floor. |
| Single engine keygroup (chromatic) | $14.99 | Tuned programs require more work + render time. MSXII baseline. |
| Dual engine pack (e.g., feliX+Oscar Signature Keys) | $14.99 | Dual engine at same price as single keygroup = strong value signal. Drives discovery. |
| Multi-engine (3–5 engines) | $19.99 | Constellation Pack, MPCe Native Pack. Represents product depth. |
| Collection Vol. (8+ engines, unified concept) | $24.99 | Kitchen Essentials Vol. 1. Kitchen table pricing — premium but accessible. |
| Full Collection (all quads, concept-complete) | $34.99–$39.99 | Complete line tier. Reserved for fully shipped collection sets. |
| Anthology / full fleet | $39.99 | Deep catalog justification. Exclusive content required at this price. |
| Educational / tutorial pack | $12.99 | Includes walkthrough PDF, preset deconstruction, signal chain diagrams. Separate from pure sample packs. |

**Rule:** Never go below $9.99. Anything under $9.99 signals commodity. XO_OX is not a commodity.

**Rule:** Never ship a pack without liner notes. A pack without liner notes is a commodity. A pack with liner notes is a premium product. The price difference is $5. The production cost is 1–2 hours. This is the highest-margin decision in the catalog.

---

## 2. Bundle Mechanics

### What Bundles Actually Do

Bundles serve three functions simultaneously: (1) reduce per-pack perceived cost, driving purchase of titles the buyer would not have bought individually; (2) introduce buyers to engines they didn't know they needed; (3) create urgency when time-limited. All three must be present for a bundle to outperform individual sales.

### Recommended Bundle Structures

**Starter Bundle — "First Three" ($29.99, save ~25%)**
- Contents: ONSET Drum Essentials ($9.99) + OddfeliX+OddOscar Signature Keys ($14.99) + any single-engine pack of buyer's choice
- Mechanics: Fixed price, permanent catalog item. The "buyer's choice" third pack is implemented as a discount code for 100% off any single-engine pack under $14.99. This drives catalog exploration.
- Purpose: Lowest-friction entry into the XO_OX catalog for new buyers. This bundle is the bottom of the funnel.

**Engine Collection Bundle — "Complete [Engine Name]" (varies)**
- Contents: All packs featuring a specific engine. Example: All OPAL-featuring packs (a dedicated keygroup pack + any Constellation or Collection pack that includes OPAL programs).
- Price: 20% off sum of individual packs.
- Mechanics: Dynamic — updates automatically as new packs featuring that engine are released.
- Purpose: Rewards fans of a specific engine character. Encourages collecting depth over breadth.

**Seasonal Bundle — Black Friday (November) ($49.99)**
- Contents: All 5 packs shipped in Year 1 (ONSET Drum Essentials + Signature Keys + Constellation + Kitchen Vol. 1 + MPCe Native Pack).
- Individual retail value: $9.99 + $14.99 + $19.99 + $24.99 + $19.99 = $89.95.
- Discount: ~44% off. Deep enough to feel real. Not so deep it devalues the catalog.
- Patreon price: $34.99 (T2+). Patreon holders get the bundle before it goes public.
- Window: 5 days only. Black Friday → Cyber Monday.
- Purpose: Converts casual observers. New MPC hardware owners (holiday gift window) buy this as their entire starter catalog.

**"Buy 3 Get 1 Free" — Permanent Mechanic**
- Any 3 packs purchased together = 4th pack (lowest-priced of the selection) at $0.
- Implemented as cart-level discount code: `XO3FOR4`.
- No time limit, no minimum price.
- Purpose: Reduces decision friction for producers comparing 4 packs. The 4th pack is likely the one they were least sure about — making it free guarantees they experience it.

### Optimal Bundle Size

**3–5 packs** is the discovery-maximizing bundle size. Under 3 packs feels like a discount, not a bundle. Over 5 packs becomes overwhelming to browse and reduces the chance a buyer will actually work through the content. The 5-pack Black Friday bundle is the ceiling for an annual collection. Permanent catalog bundles should stay at 3–4 packs.

---

## 3. Free Pack Strategy

### Why Every Studio Has a Free Flagship

A free pack serves a different job than a paid pack. It is not a loss leader — it is a proof of concept. Its job is to answer one question in the buyer's mind: "Is this studio worth my attention?" The answer must be yes, delivered in under 10 minutes of use.

### Three Candidates for XO_OX's Free Flagship

**Option A: ONSET Drum Essentials — Free Version (Recommended)**
- 4 kits (subset of the $9.99 paid version's 8 kits)
- 2 velocity layers (vs. 4 in the paid version)
- No round-robin cycles
- Full liner notes included
- *Why this works:* Drums are the universal entry point for MPC producers. Every MPC user needs new drum kits. ONSET's synthesized drums are genuinely distinct from sampled drums — the synthesis identity is immediately audible and interesting. A free drum kit costs nothing to produce relative to the paid version and directly demonstrates the value of the full $9.99 pack. Conversion path is obvious: "Same engine, twice the kits, twice the velocity layers."
- *Conversion goal:* 25–35% of free pack downloaders buy the full $9.99 version within 30 days.

**Option B: Engine Sampler — 1 Preset Per Engine (34 presets)**
- 1 chromatic keygroup program per engine (34 total), each covering C3–C5 only
- No velocity layers — single sample per note
- Short tail — 2-second render per note
- *Why this works:* Introduces the full fleet in one download. Ambitious producers immediately want more depth from the engines they connect with.
- *Why it doesn't:* 34 programs is overwhelming for a free pack. Producers don't know where to start. Discovery requires context (the liner notes must work very hard to compensate).
- *Conversion goal:* 15–20% purchase any paid pack within 30 days.

**Option C: OBESE Character Pack — Free (2 kits)**
- 2 OBESE keygroup programs with full Mojo Control documentation
- Demonstrates character synthesis without drums
- *Why it doesn't:* Too niche for a first impression. OBESE's identity (hot pink, bass-forward, aggressive) appeals to a specific producer type. A free pack needs to appeal to the widest possible first audience.

**Recommendation: Option A.** The free ONSET Drum Essentials is the correct flagship. Drums are universal. Synthesized drums are immediately interesting to MPC producers who are tired of sample-based kits. The conversion path to the $9.99 paid version is clear and low-friction.

### Free Pack Conversion Design Rules

1. Include liner notes in the free version — same quality as paid. This is how the brand communicates before the sale.
2. The free version's liner notes reference the paid version's additional content exactly once, at the end: "The full Drum Essentials pack includes 8 kits with 4 velocity layers and round-robin cycles. Available at XO-OX.org."
3. Never cripple the free version in ways that make it unusable. The 4 free kits must be genuinely production-ready.
4. Distribute the free pack everywhere: XO-OX.org, MPC Forum, Splice (free category), Gumroad. Friction-free download, no email required.
5. After download: one optional email capture for "new pack announcements." Non-intrusive. High conversion from producers who loved the free kit.

---

## 4. Patreon Integration

### Tier Interaction with Pack Releases

| Tier | Monthly fee | Pack benefit | Early access window |
|------|-------------|-------------|---------------------|
| Listener ($5/mo) | $5 | Behind-the-scenes posts + free packs 1 week early | 1 week before public |
| Producer ($15/mo) | $15 | 1 paid pack per month at no charge (value $15–$25) + early access | 2 weeks before public |
| Studio ($30/mo) | $30 | All T2 benefits + source XPM files + Oxport early builds + quarterly 1-on-1 | 3 weeks before public |

**The early access window is a marketing instrument, not just a perk.** The 3-week Studio early access means Studio tier patrons are writing about, posting about, and sharing the pack before it goes public. This is free pre-launch marketing from your most engaged users.

### Release Calendar Rhythm

The cadence that works without burning out production:

- **Monthly:** One major pack release (or one Patreon-exclusive behind-the-scenes post in consolidation months).
- **Quarterly:** One free pack published publicly (drives new Patreon discovery).
- **Annually:** One Black Friday bundle + one anniversary/anthology product.

Patreon subscribers on the Producer tier must receive their monthly pack within the first 7 days of each month. If a pack release slips (fleet render delay, QA issue), the monthly pack for that month is the free pack that month — never skip a delivery.

### Exclusive Patreon Content (Not Sold Publicly)

- Source XPM files (Studio tier): the raw programs before velocity layer normalization and XPN packaging. These are for producers who want to modify kit structure, not just use it.
- Oxport early builds (Studio tier): unreleased Oxport versions, 60-90 days before public.
- Monthly challenge winner showcase (all tiers): community tracks made with XO_OX packs, curated by Kai.

### Patreon Revenue as Production Underwrite

At 50 T2 subscribers ($15/mo): $750/month recurring.
At 20 T3 subscribers ($30/mo): $600/month recurring.
50 T2 + 20 T3 + 30 T1: $750 + $600 + $150 = **$1,500/month recurring before any retail sale.**

This is the production underwrite model. Patreon covers overhead and production time; retail sales are upside. Target: reach the 50T2/20T3 milestone before the May 2026 OddfeliX+OddOscar launch.

---

## 5. MPCe Premium Pricing

### The Case For a 20–30% Premium

MPCe-native quad packs are not interchangeable with standard velocity-layer packs. They require:
- XY pad position metadata (currently non-standard in XPM format)
- Pressure-velocity integration (3D pad axis)
- 4-corner morphing programs (quad-corner architecture)
- Hardware-specific QA on MPCe / MPC Key 61

This is genuinely more work, genuinely different design, and genuinely exclusive to a subset of MPC hardware. The premium is justified.

**Standard multi-engine pack:** $19.99
**MPCe-native equivalent:** $24.99 (25% premium)

### The Messaging

The messaging cannot be "it costs more because we did more work." That is a production argument, not a buyer argument. The correct framing:

> "Standard packs sound great on every MPC. MPCe Native packs were designed to live in the three dimensions your pads actually have. If you own an MPCe or MPC Key 61, this pack uses capabilities no other sample pack studio is building for."

The premium is justified by exclusivity and capability, not by labor cost. Buyers pay for differentiation, not for effort.

### Caveat

If the MPCe quad-corner XPM extension is not finalized and tested on hardware before September 2026, do not launch the pack. Ship the MPCe Native Pack at standard pricing ($19.99) with a "MPCe Enhanced" badge if the full quad-corner feature is incomplete. Do not charge a premium for a feature that is only partially implemented.

---

## 6. Year 1 Revenue Model

All figures are monthly. Year 1 = April 2026 – March 2027.

### Conservative Scenario — Break-Even

*Assumption: XO_OX is unknown. Zero community, zero email list, MPC Forum posts only. Free pack downloaded ~200 times. No Splice, no Drum Broker.*

| Product | Units/mo | Price | Monthly Revenue |
|---------|----------|-------|----------------|
| ONSET Drum Essentials ($9.99) | 15 | $9.99 | $150 |
| OddfeliX+OddOscar Keys ($14.99) | 8 | $14.99 | $120 |
| Constellation Pack ($19.99) | 5 | $19.99 | $100 |
| Kitchen Vol. 1 ($24.99) | 3 | $24.99 | $75 |
| Patreon — T1 ($5) | 20 | $5 | $100 |
| Patreon — T2 ($15) | 15 | $15 | $225 |
| Patreon — T3 ($30) | 5 | $30 | $150 |
| **Total** | | | **~$920/mo** |

Break-even is achievable without an existing audience. This covers hosting, tools, and part of production time.

---

### Moderate Scenario — $2–3K/month

*Assumption: 6 months of MPC Forum presence, 1 well-received free pack (500+ downloads), Field Guide cross-promotion driving organic discovery, Black Friday bundle in month 8.*

| Product | Units/mo | Price | Monthly Revenue |
|---------|----------|-------|----------------|
| ONSET Drum Essentials ($9.99) | 40 | $9.99 | $400 |
| OddfeliX+OddOscar Keys ($14.99) | 20 | $14.99 | $300 |
| Constellation Pack ($19.99) | 15 | $19.99 | $300 |
| Kitchen Vol. 1 ($24.99) | 8 | $24.99 | $200 |
| MPCe Native ($24.99) | 6 | $24.99 | $150 |
| Starter Bundle ($29.99) | 5 | $29.99 | $150 |
| Patreon — T1 ($5) | 50 | $5 | $250 |
| Patreon — T2 ($15) | 40 | $15 | $600 |
| Patreon — T3 ($30) | 15 | $30 | $450 |
| **Total** | | | **~$2,800/mo** |

Black Friday bundle month: add ~$2,500 one-time (50 units × $49.99).

---

### Optimistic Scenario — $5–8K/month

*Assumption: Drum Broker listing approved, Splice early catalog, 1 viral MPC Forum post, YouTube/social presence with 2K+ followers, XO-OX.org driving direct sales.*

| Product | Units/mo | Price | Monthly Revenue |
|---------|----------|-------|----------------|
| ONSET Drum Essentials ($9.99) | 120 | $9.99 | $1,199 |
| OddfeliX+OddOscar Keys ($14.99) | 60 | $14.99 | $899 |
| Constellation Pack ($19.99) | 40 | $19.99 | $800 |
| Kitchen Vol. 1 ($24.99) | 25 | $24.99 | $625 |
| MPCe Native ($24.99) | 20 | $24.99 | $500 |
| Bundles (mix) | 15 | $34.99 avg | $525 |
| Patreon — T1 ($5) | 100 | $5 | $500 |
| Patreon — T2 ($15) | 80 | $15 | $1,200 |
| Patreon — T3 ($30) | 30 | $30 | $900 |
| **Total** | | | **~$7,150/mo** |

Black Friday bundle month: add ~$8,000–$12,000 one-time (160–240 units × $49.99).

---

### Key Levers

Three decisions determine which scenario plays out:

1. **Fleet Render Automation.** Without it, the pack catalog stays thin (1–2 packs). With it, 10+ packs/year becomes feasible. The moderate and optimistic scenarios require fleet render. See `pack_economics_strategy_rnd.md` for the implementation case.

2. **Free pack distribution volume.** The free ONSET drum kit's download count is the leading indicator for paid conversion. 200 free downloads → conservative scenario. 1,000+ free downloads (MPC Forum viral post, Splice free listing) → moderate scenario within 90 days.

3. **Patreon subscriber count before launch.** If the Producer tier reaches 40 subscribers before the first paid pack ships, the conservative scenario becomes the floor, not the ceiling. Patreon is the insurance policy against a slow retail launch.

---

*Cross-reference: [pack_economics_strategy_rnd.md](pack_economics_strategy_rnd.md) | [patreon_content_calendar_rnd.md](patreon_content_calendar_rnd.md) | [mpce_native_pack_design.md](mpce_native_pack_design.md) | [fleet_render_automation_spec.md](fleet_render_automation_spec.md)*
