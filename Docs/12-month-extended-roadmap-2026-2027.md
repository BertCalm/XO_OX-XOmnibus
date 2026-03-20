# XO_OX / XOmnibus — Extended Release Roadmap
## Months 7–12: October 2026 – March 2027

**Companion to:** `Docs/6-month-release-calendar-2026.md`
**Created:** 2026-03-20
**Scope:** Weeks 27–52 of the XOmnibus year — the V2 Collections era, KNOT coupling, community expansion, and the 1-year anniversary arc.

---

## How to Read This Document

Months 7–12 are organized by calendar month, each with:
- **Monthly Theme** — the narrative arc for that month
- **Weekly Releases** — 4 primary deliverables per month
- **Revenue Strategy** — free vs. premium mix and pricing rationale
- **Content Themes** — Field Guide, Signal, social focus
- **Engine Quality Targets** — seance score milestones
- **Community Engagement** — Discord events, showcases, collaborations
- **Milestone Markers** — subscriber / preset library / community targets
- **Decision Points** — where owner input is required before proceeding

### Tag Key

| Tag | Meaning |
|-----|---------|
| `COLL` | V2 Collection content (Kitchen / Travel / Artwork) |
| `KNOT` | KNOT coupling feature development |
| `EQ` | Engine Quality — DSP upgrades |
| `GURU` | Guru Bin Transcendental volume |
| `XPN` | MPC-ready expansion pack |
| `SDK` | SDK Phase 2 — third-party developer tooling |
| `CONTENT` | Field Guide posts, Signal updates, social |
| `COMM` | Community events, challenges, contributions |
| `FEAT` | New feature (architecture, UI, export) |

---

## Pre-Month-7 Assumed State

By the end of Month 6 (late September 2026), the following should be true:

- V1.0 through V1.3a shipped and stable
- All 42 engines above 7.5 seance score (sprint target from months 1–6)
- Guru Bin Vol 1 (OBRIX/OSTINATO/OPENSKY/OCEANDEEP/OUIE) complete
- Guru Bin Vol 2 (original 6 retreat engines) in progress or complete
- Patreon community active with meaningful engagement (target: 200+ patrons)
- 5th-slot fusion mechanic spec written (prerequisite for all V2 collections)
- KNOT coupling SDK-level spec finalized (post-V1 backlog)
- Kitchen Essentials collection designed but not yet built
- Field Guide: 20+ published posts
- ~22,000 factory presets in the library

---

## MILESTONE MARKERS — Full Year

| Target | Metric | Timing |
|--------|--------|--------|
| M1 | 500 Patreon subscribers | End of Month 7 (Oct 2026) |
| M2 | 25,000 XOmnibus downloads | End of Month 7 |
| M3 | $5,000/month gross revenue | Month 8 (Nov 2026) |
| M4 | 100 community-submitted presets accepted into Field Guide showcase | Month 9 |
| M5 | 1,000 Patreon subscribers | End of Month 9 (Dec 2026) |
| M6 | $10,000/month gross revenue | Month 10 (Jan 2027) |
| M7 | 3 active third-party SDK developers | Month 10 |
| M8 | All 3 V2 collections shipped | Month 11 (Feb 2027) |
| M9 | 50,000 total XOmnibus downloads | Month 12 (Mar 2027) |
| M10 | V2 vision publicly announced | Month 12 (Mar 2027 anniversary) |

---

## QUARTER 3 RISK FACTORS (Months 7–9)

1. **Collection adoption risk** — If Kitchen Essentials underperforms ($39 ask on an audience 6 months in), the revenue model needs adjustment. Mitigation: build the community ritual around the launch, not just the product.
2. **KNOT coupling scope creep** — Bidirectional topological coupling is architecturally novel. Allot 6-8 weeks of engineering time, not 2. Mitigation: build the spec before Month 8, implement it early in Month 8.
3. **Community fatigue** — Monthly content cadence from months 1–6 may exhaust the team. Mitigation: Month 7 is explicitly a "consolidation month" before the V2 acceleration.
4. **Preset quality regression** — V2 collection engines will require new presets at volume. Mitigation: use parallel background agents for batches of 15+/engine; never drop below 150 factory presets per new engine.
5. **SDK adoption friction** — SDK Phase 2 only matters if developers find it easy to use. Mitigation: ship a sample third-party engine alongside SDK Phase 2 as proof of concept.

## QUARTER 4 RISK FACTORS (Months 10–12)

1. **Anniversary scope inflation** — Year 1 anniversary is tempting to overload. Keep V2 vision as a vision announcement, not a feature ship.
2. **Photography / Synesthesia Engine** — This concept requires new DSP primitives not yet built. Do not commit to a ship date before the spec is complete.
3. **Artist collaboration logistics** — Commissions, contracts, and credit take longer than expected. Start outreach in Month 9.
4. **Community contribution quality control** — First community-contributed preset pack needs a curatorial process. Barry OB curation model must be defined before Month 11.
5. **V2 architecture pressure** — 5th-slot fusion mechanic may require breaking changes to the preset schema. Freeze the V2 architecture before announcing it.

---

## MONTH 7 — October 2026
### Theme: "Collections Open"
*Kitchen Essentials is V2. The free era ends here. The boutique era begins.*

**Revenue strategy:** First paid collection launch ($39–49). Patreon Transcendental Vol 3 ships. Free tier remains XOmnibus core + Awakening presets. The message: "XOmnibus is free forever. The depth costs."

**Milestone target:** 500 Patreon subscribers. Kitchen Essentials revenue covers 1 month of development.

---

### Week 27 — Oct 5–11 `COLL` `GURU`

**Primary** `COLL`
Kitchen Essentials V2 collection launch.
- 28 engines, 6 quads, accompanying zine (PDF)
- 5th-slot fusion mechanic enabled (prerequisite ships with this release)
- Pricing: $39 introductory / $49 standard

**Secondary** `GURU`
Guru Bin Transcendental Vol 3 — Fleet Expansion set.
- 15-20 Transcendental presets each for ORBWEAVE, OVERTONE, ORGANISM
- XPN pack + PDF booklet per engine
- Patreon-exclusive for the first 2 weeks, then direct sale

**Narrative:** *"V2 is here. Kitchen Essentials is 28 engines reimagined around the physical world — warmth, grain, steam, resonance. The free synth just became a platform."*

**Content:** Field Guide post: "What Is a Collection? The V2 Philosophy." Signal post: Kitchen Essentials launch announcement with full engine card gallery.

**Community:** "Kitchen Sessions" showcase — community is invited to share their first patches using Kitchen Essentials engines. Best 5 featured in Signal.

---

### Week 28 — Oct 12–18 `EQ` `CONTENT`

**Primary** `EQ`
Post-V1 seance pass on the three newest engines.
- OVERTONE: Fix Pi table spectral collapse at low depth, add anti-aliasing fadeout near Nyquist, implement true 8-voice polyphony
- ORGANISM: Validate CA click fix from 2026-03-20 patch, expand LCG seed to 32-bit, add filter smoothing
- Both target seance scores: 8.5+

**Secondary** `CONTENT`
Field Guide post: "The Coral Colony Explained — ORGANISM's Cellular Automata" (deep technical + musical guide).

**Narrative:** *"The newest engines are getting their first quality pass. OVERTONE and ORGANISM were built in a week. Now they're being deepened."*

---

### Week 29 — Oct 19–25 `XPN` `COMM`

**Primary** `XPN`
XPN Pack #3 — "The Kitchen Frequency."
- 12–16 MPC-ready XPN packs built from Kitchen Essentials engines
- Drum + melodic + texture packs
- Oxport pipeline generates all packs automatically

**Secondary** `COMM`
Community Preset Showcase Event #1 — "Best of the First Six Months."
- Open call: submit your best XOmnibus preset (factory or original)
- 10 winners featured in next Field Guide post
- Top 3 get Guru Bin Transcendental tier for free (1 month Patreon)

**Narrative:** *"Six months of XOmnibus. Thousands of presets. What has the community built? Let's find out."*

---

### Week 30 — Oct 26 – Nov 1 `FEAT` `CONTENT`

**Primary** `FEAT`
Subscription model refinement release.
- Patreon tier restructuring based on 6 months of data
- New tier option: "Studio Pass" — all Transcendental volumes + all XPN packs, $20/month
- Update XO-OX.org pricing/support page

**Secondary** `CONTENT`
Field Guide post: "Six Months In — What We Learned About Sound Design at Scale." Honest retrospective on preset philosophy, seance system, and what's next.

**Narrative:** *"Reflection before acceleration. Here is what XO_OX learned building 42 engines and 22,000 presets."*

**DECISION POINT:** Confirm Studio Pass pricing ($20/month) and whether Transcendental volumes are included vs. sold separately. Owner must decide before Week 30 ships.

---

## MONTH 8 — November 2026
### Theme: "KNOT Coupling + New Engine"
*The coupling system gets its most radical mode. Bidirectionality changes everything.*

**Revenue strategy:** KNOT coupling is free (it's a core feature). The new engine demonstrating KNOT is free. The Guru Bin presets for both are premium. The narrative creates desire for the Transcendental tier.

**Milestone target:** $5,000/month gross revenue. Travel/Water collection enters final prep.

---

### Week 31 — Nov 2–8 `KNOT` `EQ`

**Primary** `KNOT`
KNOT Coupling Phase 1 — SDK + architecture.
- KnotTopology bidirectional coupling type fully implemented in MegaCouplingMatrix
- Linking number 1–5 (discrete integer steps, each adds parameter pair entanglement)
- Bidirectional: not sender→receiver but mutual, irreducible co-movement
- SDK updated: new `CouplingType::KnotTopology` with full documentation

**Secondary** `EQ`
OBLIQUE deep quality pass (score target: 8.5+).
- OBLIQUE has the most complex recovery history in the fleet (5.9 → ~7.2 est.)
- This pass is the final push to make it a flagship engine, not just a recovered one
- Target: new seance yielding formal 8.5+ score

**Narrative:** *"KNOT changes coupling forever. Two engines become topologically inseparable. The linking number is how deeply they're tied."*

---

### Week 32 — Nov 9–15 `KNOT` `FEAT`

**Primary** `KNOT` + New Engine Concept
KNOT Demonstration Engine — **OKNOTTED** (working name, owner to decide).
- A new engine designed specifically to exhibit KNOT coupling behavior
- Concept: Möbius strip synthesis — a voice that folds back on itself
- Every parameter has a natural "partner parameter" that can be KNOT-linked to any external engine
- Gallery code: TBD | Accent: Linking Plum `#8E4585` (shares KNOT's topology accent)
- This becomes engine #43 in XOmnibus

**Secondary** `CONTENT`
Field Guide post: "KNOT Coupling — When Two Engines Become One." Includes interactive diagram of linking number behavior.

**Narrative:** *"Engine 43 exists to demonstrate something that didn't exist before. KNOT coupling. Topological synthesis."*

**DECISION POINT:** Confirm new KNOT demonstration engine name, concept, and whether it ships in Month 8 or slips to Month 9. Backup plan: KNOT coupling ships without a dedicated new engine (uses existing engine pairs).

---

### Week 33 — Nov 16–22 `GURU` `XPN`

**Primary** `GURU`
Guru Bin Transcendental Vol 4 — KNOT Sessions.
- 15-20 Transcendental presets specifically designed as KNOT-coupled pairs
- Each preset pair in the pack comes as a duo: Engine A + Engine B + the KNOT config
- PDF booklet: "The Entanglement Scripture" — lore + sound design notes
- Patreon-exclusive 2 weeks, then $12 direct

**Secondary** `XPN`
XPN Pack #4 — "Signal/Noise" — built from OUTWIT + ORGANISM (generative + cellular).
- First XPN pack using generative engine sources
- Each sample is a unique cellular automata output, never repeatable

**Narrative:** *"The first Guru Bin presets designed for two engines at once. KNOT presets aren't presets — they're relationships."*

---

### Week 34 — Nov 23–29 `COLL` `CONTENT`

**Primary** `COLL`
Travel/Water/Vessels collection pre-launch content push.
- Engine identity cards for all Travel collection engines (public teaser)
- "Coming December" landing page on XO-OX.org
- 3 preview presets available free (Awakening tier)

**Secondary** `CONTENT`
Field Guide post: "Water in Synthesis — How XO_OX Hears the Ocean." Deep piece connecting the Aquatic Mythology framework to the Travel collection's design philosophy.

**Narrative:** *"The ocean was always the metaphor. Now it's the product."*

---

## MONTH 9 — December 2026
### Theme: "Holiday Bundle + Year's Best + Travel Collection"
*The biggest content month of the year. Three major releases. The community gets a gift.*

**Revenue strategy:** Holiday bundle pricing ($69 for Kitchen + Travel, saving $19 vs. individual). "Best of 2026" free pack drives new downloads. Patreon gifting feature (buy a month for a friend). Travel collection standard price $39–49.

**Milestone target:** 1,000 Patreon subscribers. $8,000+ gross revenue for the month.

---

### Week 35 — Dec 7–13 `COLL`

**Primary** `COLL`
Travel/Water/Vessels collection launch.
- 20 engines, vessel pairs, water-column-inspired sound design
- Accompanying zine: "The Depth Atlas" — full Aquatic Mythology visual guide
- Holiday bundle available immediately: Kitchen + Travel for $69

**Secondary** `CONTENT`
Signal post: Travel collection launch. Field Guide post: "The Vessel Pairs — How KNOT Coupling Shapes the Travel Collection." (First collection where KNOT coupling is a design primitive.)

**Narrative:** *"Kitchen was warmth and grain. Travel is depth and drift. Two collections, one philosophy: the physical world as synthesis material."*

---

### Week 36 — Dec 14–20 `CONTENT` `COMM`

**Primary** `CONTENT`
"Best of 2026" curated preset pack — free download.
- 100 presets curated from across all 43 engines
- 10 presets per mood (Foundation/Atmosphere/Entangled/Prism/Flux/Aether/Family/Submerged + Bonus)
- Curator notes by Guru Bin for each preset
- This is the best advertisement XO_OX has

**Secondary** `COMM`
Holiday Gifting Campaign.
- Patreon "gift a month" feature activated (if platform supports it)
- "Give someone an engine" social campaign — share your favorite preset + tag a friend
- Discord holiday listening party: 2-hour curated XOmnibus playback

**Narrative:** *"The best 100 sounds from a year of 43 engines. Free. No strings. Just sound."*

---

### Week 37 — Dec 21–27 `CONTENT`

**Primary** `CONTENT`
Year in Review — Field Guide annual post.
- Full 2026 retrospective: engines built, presets designed, community grown, decisions made
- "The 10 Most Unexpected Sounds of 2026" — editor's picks from the preset library
- KNOT coupling origin story
- What the seance system taught us about synthesis quality

**Secondary** `CONTENT`
Signal post: Year in Review summary (shorter, more personal). Social: 12 days of XOmnibus (one engine highlight per day, Dec 25 – Jan 5).

**Narrative:** *"A year of building a new kind of synthesizer. Here's what we learned."*

---

### Week 38 — Dec 28 – Jan 3 `EQ` `COMM`

**Primary** `EQ`
Fleet health sweep — end-of-year quality audit.
- Re-seance any engines that have had significant DSP changes since last formal score
- Priority: OVERTONE, ORGANISM (post-fix scores), OBLIQUE (post-KNOT-pass)
- Target: all 43 engines at 8.0+ entering 2027

**Secondary** `COMM`
Community Year-End Challenge: "Resolution Patches."
- Theme: design a preset that sounds like starting something new
- Best submission wins a Transcendental volume of their choice
- Submissions open through January 10

**Narrative:** *"Quality is not a launch milestone. It's a permanent commitment."*

---

## MONTH 10 — January 2027
### Theme: "V1.5 Architecture + SDK Phase 2"
*The platform grows. The walls come down for third-party developers.*

**Revenue strategy:** SDK Phase 2 is free (developer goodwill = long-term moat). V1.5 is a free upgrade. The Artwork/Color collection is in prep — no revenue this month. Transcendental Vol 5 ships for Patreon income. The investment this month is ecosystem, not direct revenue.

**Milestone target:** $10,000/month gross revenue (from cumulative collection sales + Patreon). 3 active third-party SDK developers with engines in development.

---

### Week 39 — Jan 4–10 `FEAT`

**Primary** `FEAT`
V1.5 Launch.
- What makes V1.5 worth announcing:
  1. KNOT coupling (ships Month 8, V1.5 formalizes it as a named version milestone)
  2. Engine #43 (OKNOTTED or equivalent)
  3. 5th-slot fusion mechanic stable (collections require it)
  4. SDK Phase 2 (ships this week or next)
  5. iOS AUv3 beta (if ready)
- Changelog published to both Field Guide and Signal

**Secondary** `CONTENT`
Field Guide post: "V1.5 — What Changed, Why It Matters." Technical + philosophical.

**Narrative:** *"V1.5 is not a feature update. It's the platform growing up."*

**DECISION POINT:** Confirm V1.5 feature set. What specifically ships vs. what moves to V2. The 5th-slot fusion mechanic and iOS must both be explicitly scoped before this announcement.

---

### Week 40 — Jan 11–17 `SDK`

**Primary** `SDK`
SDK Phase 2 — Third-Party Developer Toolkit.
Building on SDK Phase 1 (`SDK/include/xomnibus/` — JUCE-free SynthEngine.h + CouplingTypes.h + EngineModule.h):
- Sample third-party engine shipped alongside SDK (proof of concept, full source)
- Developer documentation site (static, linked from XO-OX.org)
- `CONTRIBUTING_ENGINE.md` — step-by-step guide for building a JUCE-free engine that integrates with XOmnibus
- Engine review process: what it takes to get a community engine added to the gallery
- SDK Phase 2 adds: MIDI mapping helpers, preset schema validators, coupling compatibility matrix

**Secondary** `CONTENT`
Field Guide post: "Building for XOmnibus — A Developer's Introduction."

**Narrative:** *"The first engine XO_OX didn't build. Here's how it was made."*

---

### Week 41 — Jan 18–24 `GURU` `COLL`

**Primary** `GURU`
Guru Bin Transcendental Vol 5 — The Theorem Engines.
- OVERTONE, ORGANISM, ORBWEAVE: 15-20 Transcendental presets each
- PDF booklet: "The Pi Day Scripture" — origin story of all three Theorem engines
- These are the first Transcendental volumes for engines born from a mathematical proof

**Secondary** `COLL`
Artwork/Color collection design sprint (internal — not public yet).
- Engine identity cards finalized for all Artwork collection engines
- 16 O-Word Vault names assigned (see [o-word-vault.md])
- Color theory foundation: each engine's accent is derived from the artwork's dominant palette
- No public announcement yet — this is pre-production

**Narrative:** *"The math engines deserved scripture. OVERTONE is the Nautilus. ORGANISM is the Coral Colony. ORBWEAVE is the Kelp Knot. Now they have lore."*

---

### Week 42 — Jan 25–31 `EQ` `COMM`

**Primary** `EQ`
Targeted seance pass: ODDFELIX + ODDOSCAR + OCEANIC.
- These three have the longest tail of unresolved quality debt from the original fleet
- ODDFELIX: snap_macroDepth void-cast lineage, preset schema drift — full audit
- ODDOSCAR: Confirm LFO + aftertouch integration is stable at 7.5+
- OCEANIC: D001 resolution (velocity → timbre) fully demonstrated in presets
- All three target 8.0+ post-pass

**Secondary** `COMM`
Artist Collaboration Outreach — Month 1.
- Identify 5 artists whose music aligns with XO_OX's sound design philosophy
- Personal outreach (not mass email)
- Offer: custom Guru Bin preset pack in their name, featured in Field Guide + Signal
- This is the beginning of the artist series launching Month 11

**Narrative:** *"Community is not downloads. It's the people making music with your instrument."*

---

## MONTH 11 — February 2027
### Theme: "Community Expansion + Artwork Collection"
*The community starts making things XO_OX didn't design. The Artwork collection launches.*

**Revenue strategy:** Artwork/Color collection launch ($39-49). Community-contributed preset pack as a free "community edition" bonus for Patreon supporters. Artist collaboration series drives aspirational brand value. Artist packs may be sold at $10-15 each as limited editions.

**Milestone target:** All 3 V2 collections shipped. Community-contributed content in the official library for the first time.

---

### Week 43 — Feb 1–7 `COLL`

**Primary** `COLL`
Artwork/Color collection launch.
- 16 O-Word Vault engines, color-theory-derived accent palette
- Each engine's sound design is inspired by a specific artwork (visual + sonic correspondence)
- Accompanying PDF: "The Color Scripture" — a visual-sonic translation guide
- Holiday Bundle extended: All 3 collections for $99 (saving $28 vs. individual)

**Secondary** `CONTENT`
Field Guide post: "Color as Synthesis — How Artwork/Color Was Designed."

**Narrative:** *"The third collection. The physics world, the water world, the color world. XO_OX was always describing the world through synthesis. Now it's explicit."*

---

### Week 44 — Feb 8–14 `COMM`

**Primary** `COMM`
Community Preset Pack — "The Flock Vol 1."
- First community-contributed preset pack in XOmnibus history
- 50 presets curated from community submissions (open call ran through Month 9-10)
- Barry OB curation process applied: each preset earns entry by passing a quality bar, not just by being submitted
- Released as free bonus for all Patreon supporters (any tier)
- Contributors are credited in the `.xometa` file and in a Field Guide post

**Secondary** `CONTENT`
Field Guide post: "The Flock Speaks — How We Curated Vol 1 of the Community Pack."

**Narrative:** *"The first sound XO_OX didn't design is in the library. This is what we built it for."*

**DECISION POINT:** Define the Barry OB curation process before Month 10. What is the quality bar? Who reviews? What feedback do rejected submitters get? This process must be documented and consistent before the first community pack ships.

---

### Week 45 — Feb 15–21 `COMM` `CONTENT`

**Primary** `COMM`
Artist Collaboration Series — Launch.
- First 2 of 5 artists go live: custom Guru Bin preset packs in their name
- Each artist pack: 10-15 presets, artist bio, "How I Use XOmnibus" short essay
- Available on XO-OX.org and Patreon (artist packs: $10 each / included in Studio Pass)
- Artists promoted across all XO_OX channels

**Secondary** `CONTENT`
Signal post: Artist series launch. Field Guide long-form interview with first artist.

**Narrative:** *"XOmnibus was designed for a specific kind of musician. Here they are."*

---

### Week 46 — Feb 22–28 `EQ` `SDK`

**Primary** `EQ` + `SDK`
First community engine review — SDK Phase 2 outcome.
- If a third-party developer has a completed engine, this is the week it gets formally reviewed
- Review process: `/post-engine-completion-checklist` + seance + XO_OX design brief review
- If the engine passes, it becomes engine #44 (or #43+ depending on Month 8)
- If no community engine is ready, use this week to publish "Engine Review Report #1" — documenting what the SDK team learned from developer outreach

**Secondary** `GURU`
Guru Bin Transcendental Vol 6 — Original Fleet Vol 2 (continued).
- Covers ODDFELIX, ODDOSCAR, OCEANIC, OBLIQUE (the engines with the hardest quality histories)
- 15-20 presets each, with Scripture entries acknowledging their journey

**Narrative:** *"The hardest engines to build have the most interesting scripture."*

---

## MONTH 12 — March 2027
### Theme: "Anniversary + V2 Vision"
*One year of XOmnibus. The community retrospective. The next chapter announced.*

**Revenue strategy:** Anniversary bundle (all-time best deal — 3 collections + all Guru Bin vols + Studio Pass discounted). V2 vision is an announcement, not a product — it creates desire for the next purchase cycle. Photography / Synesthesia Engine concept revealed (vision only, no ship date).

**Milestone target:** 50,000 total XOmnibus downloads. V2 architecture announced. Community is emotionally invested in what comes next.

---

### Week 47 — Mar 1–7 `CONTENT` `COMM`

**Primary** `CONTENT`
Anniversary Countdown: "7 Days of XOmnibus."
- One engine deep-dive per day for 7 days (social + Signal)
- Each post focuses on the engine's origin story, seance journey, and best preset
- Day 7 is the anniversary post: full year retrospective

**Secondary** `COMM`
Community retrospective: "What Did You Build?"
- Open call for community to share XOmnibus patches, recordings, and compositions
- Top submissions featured in anniversary post
- Discord listening party: 3 hours of community-made XOmnibus music

**Narrative:** *"One year. 43+ engines. Tens of thousands of presets. Let's hear what was made."*

---

### Week 48 — Mar 8–14 `CONTENT` `FEAT`

**Primary** `CONTENT`
One Year Anniversary — Major Release.
- Updated XO-OX.org homepage for the anniversary
- "Year One: The Complete Story" — long-form Field Guide post (think 10,000 words)
- Covers: V1 origin, the 42-engine fleet, the seance system, KNOT coupling, the three collections, the community, and the SDK
- This is the document people share when they want to explain XO_OX to someone new

**Secondary** `FEAT`
Anniversary engine quality release.
- Every engine with a score below 8.5 gets a targeted quality pass
- Goal: fleet-wide minimum 8.5 seance score entering Year Two
- This is the "clean sweep" before V2 architecture begins

**Narrative:** *"Year One is the foundation. Everything that follows is built on what we learned."*

---

### Week 49 — Mar 15–21 `FEAT` `COMM`

**Primary** `FEAT`
V2 Vision Announcement.
What V2 means (in order of certainty):
1. **Collections arc complete** — Kitchen/Travel/Artwork shipped → what's Phase 4?
2. **Photography / Synesthesia Engine** — visual submission → synthesis parameter mapping (community photographs → engine character). Vision announced, no ship date.
3. **KNOT V2** — more topological coupling types (torus, trefoil, Hopf fibration)
4. **iOS full release** — AUv3 out of beta, full feature parity
5. **VST3 launch** — Windows/Linux support

**Secondary** `COMM`
Community "V2 Vote."
- Discord poll: which V2 feature does the community want most?
- Not a commitment — a conversation
- Results published in Signal, genuinely inform prioritization

**Narrative:** *"Year Two doesn't start with a plan. It starts with a question: what should synthesis be capable of next?"*

**DECISION POINT:** Confirm exactly which V2 features are "vision" (no date) vs. "committed" (will ship). Never announce a committed feature without a development plan behind it. The distinction matters for community trust.

---

### Week 50 — Mar 22–28 `COLL` `GURU`

**Primary** `COLL`
Photography Collection concept reveal.
- Not a product launch — a concept drop
- Full engine card for the Synesthesia Engine concept
- "Submit your photographs" — collect 100 community images to guide the engine's design
- This is the best possible lead generator for Year Two

**Secondary** `GURU`
Guru Bin Transcendental Vol 7 — Anniversary Edition.
- 10 presets across 10 different engines — one per retreat chapter
- Designed specifically for the anniversary week
- Free for all Patreon supporters at any tier
- This is the year-end gift

**Narrative:** *"The next collection starts with your photographs. The community builds the engine."*

---

### Week 51/52 — Mar 29 – Apr 4 `FEAT` `SDK` `COMM`

**Primary** `FEAT`
V1.5.x stability and housekeeping release.
- Final bug fixes, preset migrations, SDK documentation updates
- All collection engines audited for doctrine compliance (D001–D006)
- Open issues from community bug reports addressed in batch

**Secondary** `COMM` + `SDK`
Year Two community kickoff.
- SDK Developer Jam: 30-day challenge to build a community engine
- First prize: paid artist collaboration, engine integrated into XOmnibus gallery
- Field Guide post: "How to Build an XOmnibus Engine — A Beginner's Guide"

**Narrative:** *"Year One is closed. Year Two is open."*

---

## ENGINE QUALITY TARGETS — End of Year Summary

| Tier | Target | Target Score | Deadline |
|------|--------|-------------|----------|
| CRITICAL (was ≤7.0) | OCELOT, OBESE, ODDOSCAR, ODDFELIX, OBLIQUE | 8.5+ | Month 10 |
| NEEDS WORK (was 7.0–7.9) | OCEANIC, OWLFISH, OTTONI, OVERDUB, OHM, ODYSSEY, OVERWORLD, OBBLIGATO, OUTWIT, OVERTONE, ORGANISM | 8.0+ | Month 9 |
| GOOD+ (8.0+) | All remaining engines | 8.5+ | Month 12 |
| Fleet minimum | All 43+ engines | 8.5/10 | End of Month 12 |

---

## REVENUE SUMMARY — Months 7–12

| Month | Primary Revenue Driver | Estimated Range |
|-------|----------------------|-----------------|
| Oct 2026 (M7) | Kitchen Essentials launch + Transcendental Vol 3 | $2,000–4,000 |
| Nov 2026 (M8) | KNOT coupling (free) drives Patreon growth + Transcendental Vol 4 | $3,000–5,000 |
| Dec 2026 (M9) | Travel collection + Holiday Bundle + best-of-2026 | $5,000–9,000 |
| Jan 2027 (M10) | V1.5 upgrade (free) + Transcendental Vol 5 + cumulative collections | $7,000–10,000 |
| Feb 2027 (M11) | Artwork collection + Artist packs + The Flock Vol 1 (Patreon) | $8,000–12,000 |
| Mar 2027 (M12) | Anniversary bundle + Transcendental Vol 7 + Photography concept | $6,000–10,000 |

**Model notes:**
- All revenue estimates assume Patreon at 500–1,000 patrons at $5–20/month average
- Collection sales are one-time purchases; Patreon is recurring
- SDK licensing to commercial developers is a Month 12+ revenue category (not modeled here)
- These are conservative estimates — viral moments (a well-placed YouTube video, a featured post) can spike revenue 5–10x in a single week

---

## DECISION POINTS INDEX

| # | Decision | When Needed | Stakes |
|---|----------|-------------|--------|
| DP1 | Studio Pass pricing ($20/month) and what it includes | Before Week 30 | Revenue model clarity |
| DP2 | KNOT demonstration engine name + concept | Before Week 32 | Engine #43 identity |
| DP3 | Backup plan if no community engine ready by Week 46 | Before Week 40 | SDK credibility |
| DP4 | V1.5 exact feature set | Before Week 39 | Announcement integrity |
| DP5 | Barry OB curation process for community presets | Before Month 10 | Community trust |
| DP6 | V2 vision: what's committed vs. what's aspirational | Before Week 49 | Long-term trust |
| DP7 | Photography / Synesthesia Engine scope (vision vs. roadmap) | Before Week 50 | Year 2 direction |
| DP8 | iOS full release timing | Before V1.5 announcement | Platform expansion |
| DP9 | Artist collaboration compensation model | Before Month 11 | Artist relations |
| DP10 | VST3 / Windows support timeline | Before V2 announcement | Market expansion |

---

## APPENDIX: New Engine Concepts for Year 2

These engines are not committed but are the strongest candidates for V2 expansion. All require owner decision before any development begins.

### OKNOTTED (Engine #43 candidate)
- **Concept:** Möbius strip synthesis — a voice that folds back on itself topologically
- **Design purpose:** Demonstrates KNOT coupling natively; every parameter has a natural "partner"
- **Gallery code:** TBD | **Accent:** Linking Plum `#8E4585`
- **Status:** Concept only — no development until DP2 resolved

### OSYNESTHESIA (Photography Collection engine)
- **Concept:** Visual submission → synthesis parameter mapping. Community photographs drive engine character.
- **Design purpose:** The bridge between visual art and synthesis — a truly community-designed engine
- **Gallery code:** TBD | **Accent:** TBD (derived from submitted photographs)
- **Status:** Vision only — requires new DSP primitives not yet specified

### OXOBE (Utility Engine #3)
- **Concept:** Spectral morphing tool — transform one engine's output timbral character toward another's
- **Design purpose:** Third utility engine after XObserve and XOxide; fills the "transformation" gap between them
- **Gallery code:** TBD | **Accent:** TBD
- **Status:** Concept stage — see [xobserve-xoxide-concepts.md] for utility engine identity framework

---

*This document covers Months 7–12 of the XO_OX release year. For Months 1–6 (March–September 2026), see `Docs/6-month-release-calendar-2026.md`. For the full product philosophy, see `Docs/xomnibus_master_specification.md`.*
