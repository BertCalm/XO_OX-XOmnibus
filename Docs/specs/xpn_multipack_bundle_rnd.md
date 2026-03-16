# XPN Multi-Pack Bundle Strategy — R&D

**Date**: 2026-03-16
**Status**: R&D / Pre-Spec
**Owner**: XO_OX Design
**Audience**: Internal strategy, site/Gumroad implementation

---

## Overview

XO_OX ships instruments with doctrine — aquatic mythology, sonic DNA, emotional polarity. Packs sold individually tell a story. Packs sold together tell an ecosystem. This document designs the technical, pricing, and marketing architecture for multi-pack bundles across the three V1 collections: Kitchen Essentials, Travel/Water/Vessels, and Artwork/Color.

The goal is not to discount product. The goal is to make the full collection feel inevitable — and to reward the producer who already believes in the system.

---

## 1. Bundle Concepts Already Designed

Three collections enter V1 with complete engine inventories. Each is a natural bundle candidate because each is built around a governing metaphor that makes more sense as a set than as individual items.

### Kitchen Essentials
- **24 engines**, 6 quads × 4 engines per quad
- Architecture: Voice × FX Recipe × Wildcard (boutique synth company)
- Each quad is a cooking station. Together they are a full kitchen.
- Bundle name candidate: **The Full Kitchen** or **Kitchen Essentials Complete**
- Natural tier: DOCTRINE collection (full 24 engines) with FORM sub-bundles (per quad)

### Travel / Water / Vessels
- **20 engines**, 4 thematic sets × 4 engines + Sable Island fusion slot
- Sets: Sail (Woodwinds × Hip Hop), Industrial (Brass × Dance), Leisure (Island Cultural × Island Music), Historical (Historical Percussion × Synth Genres), Sable Island (Fusion × Cross-Genre)
- Bundle name candidate: **The Full Voyage** or **Water Column Complete**
- Natural tier: DOCTRINE collection (20 engines) with FORM sub-bundles (per voyage set)

### Artwork / Color
- **24 engines**, 5 quads + Arcade fusion engine
- Color Quads A + B (12 engines), Showmen, Aesthetes, Magic/Water, Arcade
- Bundle name candidate: **The Full Palette** or **Artwork Complete**
- Natural tier: DOCTRINE collection (full 24 engines) with FORM sub-bundles (per color quad or thematic group)

---

## 2. Technical Packaging

### .XPN File Structure Review

Each `.xpn` file is a ZIP archive containing:
- One or more `.xpm` instrument programs
- Referenced sample WAVs (relative paths)
- Optional `cover.png` or `preview.wav`

The MPC imports one `.xpn` at a time via **Files → Browse → Import Pack**. There is no native "import folder of XPNs" — the user must import each pack manually. This is the core UX constraint for bundles.

### Bundle ZIP Architecture

A bundle download is a standard ZIP containing multiple `.xpn` files:

```
kitchen-essentials-complete.zip
├── bundle.json
├── covers/
│   ├── quad-a-cover.png
│   └── quad-b-cover.png
├── XO_OX_Kitchen_QuadA.xpn
├── XO_OX_Kitchen_QuadB.xpn
├── XO_OX_Kitchen_QuadC.xpn
├── XO_OX_Kitchen_QuadD.xpn
├── XO_OX_Kitchen_QuadE.xpn
└── XO_OX_Kitchen_QuadF.xpn
```

### bundle.json Manifest

Top-level manifest provides metadata for future XO_OX tooling and customer reference:

```json
{
  "bundle_id": "kitchen-essentials-complete",
  "bundle_name": "Kitchen Essentials Complete",
  "version": "1.0.0",
  "release_date": "2026-03-16",
  "collection": "Kitchen Essentials",
  "total_engines": 24,
  "total_presets": 600,
  "packs": [
    {
      "filename": "XO_OX_Kitchen_QuadA.xpn",
      "name": "Kitchen Essentials — Quad A",
      "engines": ["OSTERIA", "OWLFISH", "OHM", "OVERDUB"],
      "preset_count": 100
    }
  ],
  "import_instructions": "Import each .xpn file individually via MPC Files → Browse → Import Pack",
  "dna_range": {
    "brightness": [2, 9],
    "warmth": [3, 8],
    "movement": [2, 9],
    "density": [1, 9],
    "space": [2, 8],
    "aggression": [1, 7]
  }
}
```

### MPC Import Flow for Bundles

Since MPC requires individual `.xpn` imports, the customer workflow is:

1. Download bundle ZIP
2. Unzip to a folder on computer or USB drive
3. Connect USB drive to MPC (or use SD card transfer)
4. On MPC: Files → Browse → navigate to folder → import each `.xpn` in sequence

A printed or PDF quick-start card inside the bundle ZIP should diagram this flow. Target: import time under 5 minutes for a full collection. Include a numbered `INSTALL.txt` at the root of every bundle ZIP.

Future tooling opportunity: a desktop installer app (`XO_OX Pack Manager`) that batch-imports `.xpn` files via MPC's USB connection — this would eliminate the manual loop entirely and is worth scoping for Year 2.

---

## 3. Pricing Architecture

### Three Bundle Tiers

| Tier | Name | Scope | Price Range |
|------|------|-------|------------|
| SIGNAL | Individual Pack | 4 engines, ~100 presets | $15–20 |
| FORM | Sub-Collection Bundle | 8–12 engines, one thematic group | $35–50 |
| DOCTRINE | Full Collection | 20–24 engines, complete mythology | $75–100 |

### Pricing Rationale

- **SIGNAL** packs price at $15–20 because they are fully formed instruments, not samples. A single engine has deep character.
- **FORM** bundles are 2–3 SIGNAL packs. Effective discount: 15–20%. This is not charity — it rewards commitment to the sub-narrative (e.g., "the brass voyage" or "the color quad").
- **DOCTRINE** collections price at $75–100 because they are a creative infrastructure investment. Compare: a Splice subscription is $120/year. A DOCTRINE collection is a permanent asset.

### Volume Discount Signal vs. Full Price Signal

Bundles should not communicate "cheaper because we couldn't sell them individually." The framing is always completeness, not discount. Marketing language: "The Full Kitchen" not "Save 25% on Kitchen Essentials."

The discount exists but lives in the math, not the headline.

---

## 4. Complementary Pairs

### Designed Pairs

**The Full Column** — OPENSKY + OCEANDEEP
- OPENSKY: Euphoric shimmer, pure feliX, Sunburst `#FF8C00`
- OCEANDEEP: Abyssal bass, pure Oscar, Trench Violet `#2D0A4E`
- These two engines are the top and bottom of the water column atlas — light and dark, surface and abyss, feliX and Oscar in their most elemental form
- Pair price: $28 (vs. $20 × 2 = $40 individual)
- Marketing: "Light breaks the surface. Darkness holds the floor."

**ODDFELIX + ODDOSCAR** — The Odd Pair
- The neon tetra and the axolotl — the original polarity
- Natural introduction bundle for new customers who want to understand the XO_OX axis before committing to a full collection
- Pair price: $25 (vs. $20 × 2)
- Marketing: "Where it all began."

**OVERDUB + OPAL** — The Dub Column
- OVERDUB is the tape delay + spring reverb send architecture; OPAL is the granular shimmer receiver
- These two couple beautifully in XOmnibus (OVERDUB→OPAL is a documented coupling route)
- Pair price: $28
- Marketing: "Send it. Scatter it."

### Pair vs. Full Collection Positioning

Pairs serve three customer types:
1. **Budget-conscious producers** who want to test the ecosystem before buying a full collection
2. **XOmnibus users** who already have some engines and want the missing half of a polarity pair
3. **Gift purchasers** who want a clean, low-risk entry point

Pairs should always link to their parent collection page. Every pair sale is a pipeline into a FORM or DOCTRINE purchase.

---

## 5. Progressive Unlock Model

### Mechanics

The model rewards commitment without requiring it upfront:

- **Pack 1**: Full price ($15–20)
- **Pack 2** from same collection: 20% off (via Gumroad discount code delivered with Pack 1 receipt)
- **Pack 3** from same collection: 30% off (via code delivered with Pack 2 receipt)
- **Pack 4+**: upgrade path — pay the difference to the FORM bundle price

### Gumroad Implementation

1. Customer purchases Pack 1 → receipt email includes a `COLLECTION-STEP2` discount code (20% off any pack from the same collection, single use, 90-day expiry)
2. Customer purchases Pack 2 with that code → new receipt includes `COLLECTION-STEP3` code (30% off, 60-day expiry)
3. After Pack 3, the discount path ends — instead, include a message: "You've collected 3 of 6 Kitchen Essentials packs. Upgrade to the Full Kitchen Collection and we'll apply what you've already paid."

The upgrade credit logic requires manual fulfillment in V1 (customer emails xo-ox.org with receipts). Automate in Year 2 via Gumroad API if volume justifies it.

### Psychology

The progressive model does two things:
1. It makes the second purchase feel earned, not discounted — the producer unlocked it by believing in the first one
2. It creates narrative momentum — each purchase is a step in a journey, not a transaction

---

## 6. Bundle Marketing

### Collection Landing Page vs. Individual Pack Pages

Both are needed. The architecture:

- **Collection landing page** (`xo-ox.org/packs/kitchen-essentials`): tells the full mythology, shows all 24 engines in the water column, showcases the complete DNA range, has a single CTA: "Get the Full Kitchen" (DOCTRINE price). Secondary CTAs: individual quad pages.
- **Individual pack pages** (`xo-ox.org/packs/kitchen-quad-a`): deep dive on 4 engines, audio previews for each, coupling diagram, preset spotlights. CTA: "Get Quad A" (SIGNAL price). Secondary CTA: "See the Full Kitchen."

Every individual page points up. Every collection page points down into individual previews. The funnel works in both directions.

### Launch Sequencing: Individual First vs. Bundle First

**Recommendation: Individual first, bundle at collection completion.**

Reasons:
- Individual packs build anticipation. Releasing Kitchen Quad A in January, Quad B in February, etc. gives the community 6 months of engagement before the collection lands.
- Each individual release is a content moment (Field Guide post, Signal update, demo video).
- The collection launch becomes an event: "The Full Kitchen is now complete." That framing is impossible if everything launches simultaneously.
- Progressive unlock codes activate naturally as packs release — customers who bought Quad A get a discount code and see Quad B announced. The timing aligns.

**Exception**: Complementary pairs (OPENSKY + OCEANDEEP) launch simultaneously because their polarity story requires both halves.

---

## 7. Cross-Collection Bundles

### The XO_OX Complete V1 Library

Once all three collections are complete, the cross-collection bundle is the capstone product.

**Scope**: Kitchen Essentials (24) + Travel/Water/Vessels (20) + Artwork/Color (24) = 68 engines, ~1,700 presets

**Name**: "The XO_OX V1 Complete Library" or "The Full Aquarium"

**Launch price**: $149–179 (limited-time, 60–90 day window from collection completion)
**Catalog price**: $199–229 (permanent, after launch window closes)

The launch window creates a genuine event. The catalog price means it remains aspirational. No artificial scarcity — the bundle is always available, but the launch price is not.

### Limited-Time Bundle Window Strategy

- **Announce 30 days before**: "The Full Aquarium bundle goes live on [date]. Launch price: $149 for 60 days."
- **At launch**: Signal post + Field Guide entry "Why the full ecosystem matters." Email to existing customers with loyalty code (additional 10% off — see below).
- **At 30 days**: Reminder post "30 days left at launch price." No urgency theater — just information.
- **At 60 days**: Price moves to catalog rate. No countdown timer on the page. Clean.

### Loyalty Discount for Existing Pack Owners

Customers who already own individual packs or sub-collections should be able to upgrade.

V1 approach: email xo-ox.org with receipts → manual credit toward V1 Complete Library. This requires trust and goodwill, which XO_OX has. Automate in Year 2.

A simpler alternative: offer a flat loyalty code (`ALREADY-IN`) that gives 15% off the Complete Library to anyone on the XO_OX mailing list, no proof required. The goodwill signal is worth the margin.

---

## 8. Open Questions

- **MPC firmware limits**: Confirm the maximum number of `.xpn` packs a single MPC project can reference. If there's a limit near 24–68 packs, the DOCTRINE and Complete Library tiers need a different packaging approach (e.g., consolidated `.xpn` files per collection rather than one per quad).
- **Gumroad multi-product bundles**: Test whether Gumroad's native bundle feature handles 6+ products cleanly, or whether a single ZIP download product is cleaner for the customer.
- **Pack naming consistency**: Establish a canonical naming convention before first release. Suggested: `XO_OX_{Collection}_{SubName}_v1.0.xpn`. Lock this before any public sales.
- **Sample deduplication**: If two packs in the same collection share sample content (e.g., a shared drum layer), the bundle ZIP may contain duplicates. The XPN Tool Suite's packager should be updated to deduplicate shared samples at bundle build time.
- **Audio preview clips**: Each bundle needs a 60–90 second preview that demonstrates the collection's range — not individual engine demos strung together, but a produced piece that uses all engines in the collection. This is a production task, not a technical one, but it gates the marketing launch.

---

## Summary Recommendations

| Priority | Action |
|----------|--------|
| P0 | Add `bundle.json` manifest to XPN Tool Suite packager — generates automatically from pack metadata |
| P0 | Write `INSTALL.txt` template for all bundle ZIPs |
| P1 | Set up Gumroad progressive discount code delivery flow (Pack 1 receipt → Step 2 code) |
| P1 | Design collection landing page template (mythology + engine grid + DNA range + CTA hierarchy) |
| P2 | Spec the upgrade credit / loyalty discount process before first public pack sale |
| P2 | Test Gumroad bundle product type vs. single ZIP download for customer UX |
| P3 | Scope XO_OX Pack Manager desktop app (batch MPC import) for Year 2 |
