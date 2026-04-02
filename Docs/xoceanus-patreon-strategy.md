# XOceanus Patreon Strategy — "The Expedition"

**XO_OX — March 2026**
**Prepared by: RAC (Ringleader × Architect × Consultant)**

---

## Philosophy

The Patreon is not a paywall. It is an *expedition*. Patrons are the scouts who go first — they fund the journey and get early access, but everything they discover eventually becomes free for everyone. This is "Omnibus — For All" applied to monetization.

**The rule**: Nothing is permanently patron-exclusive. Everything released through a milestone becomes permanently free. Weekly freebies go to patrons one week early, then to everyone. The Expedition funds the work; it doesn't gate the output.

---

## I. Tier Structure — "Depths of the Expedition"

### Explorer — $3/month

*"You've taken the first step into the water. Welcome to the expedition."*

| Benefit | Description |
|---------|-------------|
| **Early Access** | All weekly freebies 1 week before public release |
| **Expedition Feed** | Behind-the-scenes updates, dev process, design decisions |
| **Name on the Wall** | Listed in the Credits page on XO-OX.org |
| **Discord/Community** | Access to the Expedition discussion space |
| **Vote** | Vote on which quad unlocks next at each milestone |

### Diver — $8/month

*"Below the surface now. You can still see the light above, but you're choosing the deep."*

All Explorer benefits, plus:

| Benefit | Description |
|---------|-------------|
| **Monthly Preset Pack** | 15-20 exclusive-for-1-month presets (go free after 30 days) |
| **Coupling Recipe of the Month** | Detailed recipe with JSON + walkthrough |
| **Professor Oscar Bonus Lesson** | One extra lesson per month, early access |
| **Guru Bin Transmission** | Monthly audio commentary from Guru Bin on sound design |
| **Engine Preview** | See new engines 2 weeks before public announcement |

### Abyssal — $20/month

*"The pressure is real. The treasures are real. You are the reason the deep opens."*

All Diver benefits, plus:

| Benefit | Description |
|---------|-------------|
| **Name an Engine** | Suggest O-words for future engines (advisory, not binding) |
| **Monthly 1-on-1** | 15-minute audio/text consultation on sound design or synthesis |
| **Abyssal Preset Library** | Curated "Gold Standard" presets — exclusive for 3 months, then free |
| **Engine Changelog Early** | See exactly what changed in each engine before anyone else |
| **Annual Creature Commission** | One custom bioluminescent creature illustration per year |

---

## II. Milestone Release Strategy

> **Model Clarification (2026-03-30):** Two release models are documented across different files.
> This document describes the **patron-milestone model** (unlock by patron count).
> `kitchen-collection-release-calendar.md` describes the **calendar model** (unlock by date, Aug–Dec 2026).
> The intended resolution: milestone unlocks are the *ceiling* trigger, calendar dates are the *floor* trigger.
> A quad ships when **either** (a) its patron milestone is reached, **or** (b) its calendar ship date passes — whichever comes first.
> The KC engines are code-complete as of 2026-03-23. The gates are editorial/Patreon windows, not build time.
> This reconciliation should be formalized before V1 launch. See issue #413.

### How It Works

1. When the Patreon reaches a patron count milestone, a Kitchen Collection quad unlocks
2. Alternatively, each quad ships on its calendar date regardless of patron count
3. The unlock is **permanent and free for everyone** — not just patrons
4. Patrons vote on which quad unlocks next (from the remaining pool)
5. Each unlock is celebrated with a "Depth Event" — a themed release with accompanying content

### Milestone Map

| Milestone | Patrons | Quad Unlocked | Narrative | Accompanying Content |
|-----------|---------|---------------|-----------|---------------------|
| **Milestone 1** | 10 | Chef (Organs) | *"The first expedition returns with instruments from the shallow reefs."* | 4 engine guides, 40+ presets, 4 coupling recipes, creature atlas entries |
| **Milestone 2** | 25 | Kitchen (Pianos) | *"The divers breach the twilight zone. New creatures surface."* | 4 engine guides, 40+ presets, 4 coupling recipes, Professor Oscar piano lesson |
| **Milestone 3** | 50 | Cellar (Bass) | *"Fifty souls have reached the deep bass frequencies. The Cellar opens."* | 4 engine guides, 40+ presets, 4 coupling recipes, bass design tutorial |
| **Milestone 4** | 100 | Garden (Strings) | *"One hundred explorers. The deep garden reveals itself."* | 4 engine guides, 40+ presets, 4 coupling recipes, string synthesis deep dive |
| **Milestone 5** | 250 | Broth (Pads) | *"A quarter thousand. The primordial broth — where all sound begins."* | 4 engine guides, 40+ presets, 4 coupling recipes, ambient masterclass |
| **Milestone 6** | 500 | Fusion (EP) | *"Five hundred. The deepest trench. Where impossible instruments fuse."* | 4 engine guides, 40+ presets, 4 coupling recipes, spectral fingerprint tutorial |

### Depth Events

When a milestone is reached, the celebration is a **Depth Event**:

**Day 0 — Announcement**:
- Sonar post: "Milestone [N] reached. The [Quad] opens."
- Social: bioluminescent creature reveal for each of the 4 new engines
- Patreon: thank you post with patron roll call

**Day 1 — Release**:
- Engines available in XOceanus update
- 40+ new presets distributed across moods
- Field Guide: "Dispatches from the Deep — [Quad] Discovered"
- Creature Atlas: 4 new entries
- Book of Bin: new chapter from the quad's Guru Bin retreat

**Day 2-7 — Content Wave**:
- 4 coupling recipes (one per engine × another existing engine)
- Professor Oscar lesson relevant to the quad's synthesis type
- Behind-the-scenes: how the quad was designed and built
- Community challenge: "Make a track with the new quad. Best submission gets featured."

**Day 14 — Retrospective**:
- Community showcase: featured tracks using the new quad
- Engagement metric: how many people downloaded, how many presets were made
- Tease of next milestone: "We're [N] explorers from the next depth..."

### Expedition Page — `/expedition`

The live milestone tracker on the site:

```
THE EXPEDITION
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Surface ─────── ○ V1 Launch (LIVE)
                │
Twilight ────── ○ 10 Explorers → Chef Quad
                │    ████████░░ 8/10
                │
Mesopelagic ─── ○ 25 Divers → Kitchen Quad
                │
Bathypelagic ── ○ 50 Bathynauts → Cellar Quad
                │
Abyssal ─────── ○ 100 Abyssals → Garden Quad
                │
Hadal ──────── ○ 250 Hadals → Broth Quad
                │
Trench ──────── ○ 500 Trench Walkers → Fusion Quad

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
"We're 2 explorers from unlocking the Chef Quad.
 Join the expedition."
```

The progress bar is a **depth gauge** — it fills downward. Each milestone reached permanently illuminates that depth level. The visual is a vertical ocean cross-section getting progressively illuminated from top to bottom.

---

## III. Weekly Freebies — Content Calendar

### The Rhythm

Every week, one piece of content drops. Patrons get it one week early. It cycles through content types on a predictable rotation:

| Week | Content Type | Example |
|------|-------------|---------|
| Week 1 | **Preset Pack** (5-10 presets) | "Foundation Series Vol. 3 — 8 OBRIX presets for bass music" |
| Week 2 | **Coupling Recipe** | "The Symbiosis: OUROBOROS → OWARE with KnotTopology" |
| Week 3 | **Professor Oscar Lesson** | "Open Water #4: Introduction to Physical Modeling" |
| Week 4 | **Guru Bin Dispatch** | "The Psalm of Sustain — why release time is a spiritual practice" |

### Monthly Specials (in addition to weeklies)

| Month | Special | Description |
|-------|---------|-------------|
| **Every month** | Creature Art Drop | One new bioluminescent engine illustration, wallpaper-resolution |
| **Every month** | Behind-the-Scenes | Dev log — what was built, what's coming, how decisions were made |
| **Every quarter** | XPN Pack | MPC expansion pack (starts with mpce-perc-001) |
| **Every quarter** | Scripture Chapter | New Book of Bin chapter with illustrated verses |
| **Biannual** | "State of the Deep" | Full ecosystem report — engines built, presets created, community growth |

### Content Production Pipeline

To maintain the weekly cadence:

| Content Type | Production Time | Who Makes It | Model Tier |
|-------------|----------------|-------------|------------|
| Preset Pack | 1-2 hours | Exo Meta + Guru Bin skills (Sonnet) | Automated after design |
| Coupling Recipe | 30 min | Manual + Coupling Cookbook skill | Sonnet |
| Professor Oscar Lesson | 2-4 hours | Manual writing + WebAudio demo | Opus (creative) + Sonnet (code) |
| Guru Bin Dispatch | 1 hour | Scripture Keeper + writing | Sonnet |
| Creature Art | 2-4 hours | Manual illustration or AI-assisted | External/manual |
| Behind-the-Scenes | 1 hour | Writing from session notes | Manual |
| XPN Pack | 4-8 hours | Oxport pipeline | Automated after setup |

**Weekly time commitment**: ~2-4 hours of content creation per week, with skills handling the heavy lifting for presets and recipes.

### Freebies Gallery Integration

Every freebie:
1. Published on Patreon (patrons get it week -1)
2. Published on `/freebies` gallery (everyone gets it week 0)
3. Tagged with type, engine, level, and date
4. Permanently available — the gallery is an ever-growing library
5. RSS feed available for subscribers who want notifications

---

## IV. Beyond Milestones — Post-Kitchen Collection

After all 6 Kitchen Collection quads are unlocked, the milestone system continues:

| Milestone | Patrons | Unlock |
|-----------|---------|--------|
| 750 | **Companion FX Pack** — all 6 Kitchen FX engines |
| 1,000 | **First Community Engine** — engine built from community feedback |
| 2,000 | **Outshine Integration** — sample instrument forge goes live |
| 5,000 | **Mobile (iOS) Release** — AUv3 version |

These are aspirational but they set a visible roadmap. The community knows: the more people join, the more opens up. It's not "pay to unlock" — it's "grow together to unlock."

---

## V. Revenue Projections (Conservative)

| Timeframe | Patrons (est.) | Monthly Revenue | Notes |
|-----------|---------------|-----------------|-------|
| Month 1-3 | 15-30 | $75-200 | Early adopters, Milestone 1 likely reached |
| Month 4-6 | 40-80 | $200-500 | Milestone 2 reached, weekly cadence established |
| Month 7-12 | 80-200 | $500-1,500 | Milestones 3-4, community word-of-mouth |
| Year 2 | 200-500 | $1,500-4,000 | All Kitchen quads unlocked, brand established |

**Not a business plan — a sustainability model.** The goal isn't to get rich from Patreon. The goal is to fund continued development so the "for all" promise is sustainable. Every dollar funds more free content.

---

## VI. Messaging Templates

### Launch Post

> **The Expedition Begins**
>
> XOceanus is free. The code is open. The presets are free. The lessons are free. But building the deep ocean takes time, and time costs money.
>
> The Expedition is how we fund the dive. Every patron is an explorer who helps us go deeper. And everything we find in the deep — every engine, every preset, every lesson — eventually surfaces for everyone.
>
> Join at any depth. Explorer ($3), Diver ($8), or Abyssal ($20). Every explorer counts. When we reach 10, the first Kitchen Collection quad unlocks — free, for everyone, forever.
>
> The deep opens when we go together.

### Milestone Approaching

> **We're [N] explorers from [Quad Name].**
>
> At [threshold] patrons, the [Quad] opens — [4 engine names], [number] new presets, coupling recipes, creature profiles, and a new chapter of the Book of Bin. Free for everyone, permanently.
>
> Every new explorer brings us closer to the next depth.
> Join the expedition: [link]

### Milestone Reached

> **[Quad Name]: UNLOCKED.**
>
> [Threshold] explorers. The [depth zone] opens.
>
> [4 engine names] are now free in XOceanus for everyone.
> [Number] new presets. [Number] coupling recipes. New creatures in the atlas.
>
> This is what "for all" looks like.
>
> Next depth: [N] explorers to [Next Quad]. The descent continues.

---

*"The deep opens when we go together."*

*Prepared by the RAC — Ringleader, Architect, Consultant*
*For XO_OX — March 2026*
