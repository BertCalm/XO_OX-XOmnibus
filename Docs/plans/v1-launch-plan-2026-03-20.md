> ⚠️ **RETIRED — DO NOT USE AS A LIVE PLANNING DOC (2026-04-16)**
>
> The "V1 launch" framing has been retired. XOceanus does not operate on a fixed-version
> release model. Build and refine until ready; ship when ready. This document is preserved
> for historical reference only. See `CLAUDE.md` → *Release Philosophy*.

---

# XOceanus V1 Launch Plan (HISTORICAL)
**Status:** RETIRED 2026-04-16 — preserved for historical reference
**Date:** 2026-03-20
**Product State:** 46 engines | ~22,000+ presets | Build PASS | auval PASS
*(Note: OXBOW + OWARE were added 2026-03-20, pushing fleet from 42 to 44; counts below referencing 42 are historical and should be read as 44)*
**Distribution:** XO-OX.org direct download + Patreon
**Supersedes:** `Docs/plans/v1-launch-plan.md` (written when scope was 34 engines)

---

## Current Reality Check

Before the plan: an honest accounting of where the fleet stands today.

| Asset | State |
|---|---|
| Engines registered | 42 / 42 |
| Total presets | ~22,000+ |
| Build | PASS |
| auval | PASS |
| ORBWEAVE presets | **0 — CRITICAL gap** |
| Seance verdicts | 13 / 42 (31%) |
| Hard gates cleared | Partial (see Pre-Launch section) |

The product is technically shippable. The story, the channels, and several hard gates are not ready. This plan closes those gaps in the 7 days before launch.

---

## Messaging Framework

Define this first. Everything else flows from it.

### Elevator Pitch (1 sentence)

> XOceanus is a free, open-source synthesizer with 42 unique engines — from granular and physical modeling to cellular automata and spectral synthesis — that couple together in real time to produce sounds no single synth can make.

### Short Description (1 paragraph)

> XOceanus is a free plugin for macOS with 46 synthesis engines, ~22,000 factory presets, and a cross-engine coupling system that lets any engine modulate any other. Each engine is a distinct synthesis paradigm — a granular cloud sampler, a dub delay machine, a cellular automaton, a spectral engine driven by continued fractions, a hammerhead duophonic synth. Load four at once, wire them together, and hear what happens when an onset generator drives the filter of a reverb-drenched pad. The core is MIT-licensed and always free. Premium content (Guru Bin Transcendental packs, boutique engines) funds continued development. XO_OX Designs is one person, building in public.

### Full Press Release Draft

---

**FOR IMMEDIATE RELEASE**

**XO_OX Designs Ships XOceanus: 46 Synthesis Engines, ~22,000 Presets, Free**

*March [DATE], 2026 — XO_OX Designs today released XOceanus V1.0, a free, open-source multi-engine synthesizer plugin for macOS. XOceanus brings together 42 distinct synthesis paradigms in a single plugin — from classic subtractive and FM through granular, physical modeling, cellular automata, and spectral synthesis — with a cross-engine coupling system that allows any engine to modulate any other in real time.*

*The plugin ships with approximately 22,000 factory presets across 8 mood categories, organized by a 6-dimensional Sonic DNA system that lets users browse by character rather than category. All synthesis, all presets, and all coupling features are permanently free.*

*"The kid who grew up making music in Fruity Loops demos — who couldn't save, who had to finish songs in one sitting — is building the free instrument so the next generation can use a top-of-the-line plugin," said the developer behind XO_OX Designs. "No barriers. No paywalls. Better than what money can buy."*

*XOceanus is distributed under the MIT license. The plugin currently supports macOS AU format, with AUv3 (iOS) and VST3 planned. Download is available at XO-OX.org.*

*Key features at launch:*
*- 42 synthesis engines, each with distinct DSP character and aquatic mythology identity*
*- MegaCouplingMatrix: 13 coupling types for real-time cross-engine modulation*
*- ~22,000 factory presets with 6D Sonic DNA fingerprinting*
*- 8 mood categories: Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged*
*- 4 macro system: CHARACTER, MOVEMENT, COUPLING, SPACE*
*- Gallery Model UI: warm white shell with engine-specific accent colors*
*- Awakening preset tier: 10 curated hero presets per engine, gold-bordered in the browser*
*- MIT license, open-source, community-driven*

*For media inquiries, press kit, and download: XO-OX.org*

---

### Key Differentiators vs. Competition

| Competitor | Their strength | XOceanus answer |
|---|---|---|
| Vital / Surge XT | Polished, single-engine free synths | 46 engines, coupling between them — different category |
| Omnisphere | Huge preset library, cinematic | Free (Omnisphere is $500), open source, user-extensible |
| Pigments | Multi-engine, beautiful UI | MIT, 46 engines vs. 4, coupling as core synthesis, not FX |
| u-he Hive / Diva | Professional analog emulation | XOceanus is character-driven, not emulation — and free |
| Native Instruments Kontakt | Ecosystem, sample library | XOceanus generates synthesis from scratch, no samples needed |

**The position no one else occupies:** A synthesis ecosystem — not a synth — where the engines are the instruments and the coupling between them is the compositional tool. Free. Open. 42 paradigms.

### Social Media Templates

**Short (280 chars / Twitter/X):**
> XOceanus V1.0 is out. 46 synthesis engines. ~22,000 presets. Cross-engine coupling. MIT license. Free.
>
> Download: XO-OX.org

**Medium (Instagram caption / Reddit body):**
> XOceanus V1.0 ships today.
>
> 42 synthesis engines in one plugin: granular, FM, physical modeling, cellular automata, spectral, dub delay, chip synth, wavetable, and 34 more. Each engine has its own aquatic mythology identity, Sonic DNA fingerprint, and coupling compatibility.
>
> The coupling system is the feature. Load ONSET (percussion) + OPAL (granular) + OVERDUB (dub delay) and wire them: the percussion envelope drives the grain density drives the reverb tail. Three engines, one sound, impossible any other way.
>
> ~22,000 factory presets. 8 moods. 4 macros. MIT license.
>
> Free. Forever. XO-OX.org

**Long (forum post / community seed):**
> I've been building XOceanus for [X months] — a free, open-source multi-engine synth for macOS. It shipped today and I wanted to share it here because this community has been a reference point throughout development.
>
> **What it is:** 42 synthesis engines in one plugin, each with distinct DSP character. Not variations of the same oscillator topology — 42 genuinely different paradigms. ORGANISM is a cellular automaton (Wolfram elementary CA rules driving oscillators). ORACLE is GENDY stochastic synthesis with Maqam tuning. OWLFISH is a Mixtur-Trautonium oscillator. OBRIX is a modular brick synthesis system. ORGANON uses Variational Free Energy metabolism as a control signal. Each one is its own thing.
>
> **The coupling system (MegaCouplingMatrix):** Load up to 4 engines at once and wire them together with 13 coupling types — amplitude-to-filter, LFO-to-pitch, envelope-to-morph, audio-to-FM, rhythm-to-blend, and more. The Entangled mood in the preset browser has dedicated coupling presets showing what this sounds like in practice.
>
> **The preset library:** ~22,000 factory presets with 6D Sonic DNA (brightness, warmth, movement, density, space, aggression). Browse by mood, filter by DNA vector, find similar presets algorithmically. The Awakening tier marks the 10 hero presets per engine — the ones that show the ceiling.
>
> **The economics:** Core plugin + all presets are MIT-licensed and free, permanently. Guru Bin Transcendental preset packs (deep sound design, Scripture lore, XPN export) are premium Patreon content. Premium boutique engines (starting with OSTINATO) come later as paid products. The engine itself never costs money.
>
> Download + Field Guide at XO-OX.org. Questions welcome.

---

## Pre-Launch: T-7 Days

This is the critical window. The plugin exists. The story must be built, the hard gates cleared, and the channels primed.

### Hard Gates — Must Clear Before Launch

These are non-negotiable blockers from the Board. Check each off:

| Gate | Status | Owner | Due |
|---|---|---|---|
| Security audit — git history clean, no secrets exposed | PENDING | Solo dev | T-7 |
| Patreon URL fixed from placeholder `www.patreon.com/cw/XO_OX` | PENDING | Solo dev | T-7 |
| ORBWEAVE preset gap filled (currently 0 — only engine with zero presets) | PENDING | Solo dev | T-6 |
| Community infrastructure — README, CONTRIBUTING, SECURITY, CODE_OF_CONDUCT, issue templates | PENDING | Solo dev | T-5 |
| Minimum 5 hero preset audio clips on XO-OX.org (see `site-sample-recordings.md`) | PENDING | Solo dev | T-4 |
| Download page live at XO-OX.org | PENDING | Solo dev | T-3 |
| All materials say "46 engines" consistently | PENDING | Solo dev | T-3 |
| Preset silence audit — spot-check 50 presets across all 8 moods | PENDING | Solo dev | T-3 |

### Final QA Checklist (T-7 to T-3)

**Build + Audio Thread**
- [ ] `cmake --build build` — 0 errors, ≤7 pre-existing warnings (baseline from Round 12K)
- [ ] `auval -v aumu Xomn XoOx` — PASS
- [ ] Load 4-engine patch with coupling active, idle CPU < 2% (SilenceGate)
- [ ] Spot-check 5 coupling presets from Entangled mood — audio output, macro response
- [ ] Verify no engine crashes on rapid preset switching (hot-swap 50ms crossfade)

**Preset Library**
- [ ] ORBWEAVE: write minimum 50 factory presets (target 150) before launch
- [ ] Spot-check 10 Awakening presets across 5 engines — sound quality + gold border rendering
- [ ] Verify all 8 mood categories have coverage from 46 engines
- [ ] Confirm preset browser loads within 3 seconds on a 2019 MacBook Pro

**Documentation**
- [ ] README.md: what it is, how to build, quick start, link to Field Guide
- [ ] CONTRIBUTING.md: preset submission process, code standards, fnm requirement
- [ ] SECURITY.md: vulnerability reporting
- [ ] CODE_OF_CONDUCT.md: Contributor Covenant
- [ ] GitHub issue templates: Bug report + Feature request
- [ ] Engine count accurate in all Docs: 42 (not 34, not 39)
- [ ] CLAUDE.md engine table current

**Site**
- [ ] XO-OX.org Download page live — direct .zip link, installation instructions
- [ ] At least 5 audio demos embedded or linked from the download page
- [ ] Aquarium (water column atlas) updated with OVERTONE, ORGANISM, ORBWEAVE placements
- [ ] Patreon URL live (replace placeholder)
- [ ] Signal feed has 1-2 teaser posts before launch day

**Press Kit Contents**
Prepare a `/press-kit/` folder at XO-OX.org with:
- `xoceanus-logo-light.svg` + `xoceanus-logo-dark.svg`
- `hero-coupling-view.png` (4K, light mode, 4 engines loaded, coupling active)
- `preset-browser-dna.png` (4K, Sonic DNA radar visible)
- Screenshots of 5-6 distinct engines showing accent color variety
- `one-sheet.pdf` — 1-page: what it is, key stats, download link, contact
- `fact-sheet.md` — plain text version of above
- Audio demo ZIP: 5 hero preset recordings at 44.1kHz 24-bit WAV

### Social Media Scheduling (T-7 to T-1)

The goal in the pre-launch window is to plant the concept, not announce the product. Let curiosity build.

| Day | Content | Channel | Format |
|---|---|---|---|
| T-7 | "42 synthesis paradigms. One plugin. Coming soon." — no link, just intrigue | Twitter/X + Instagram | Screenshot of engine selector showing all 42 names |
| T-6 | "What does coupling sound like?" — 30-second audio clip of ONSET driving OPAL grain density | Twitter/X + Instagram Stories | Audio waveform visualization |
| T-5 | Field Guide: publish "The Water Column Atlas" or "What is Coupling?" post — educational, no sales pitch | XO-OX.org + Signal feed | Blog post |
| T-4 | Engine spotlight: OBRIX — the modular brick engine. Screenshot + 1-paragraph description | Twitter/X | Static image |
| T-3 | Engine spotlight: ORACLE — GENDY stochastic + Maqam tuning. Audio clip | Instagram + Twitter/X | Short audio clip |
| T-2 | "Tomorrow." — minimal, one word, XOceanus logo | All channels | Logo image |
| T-1 | "XOceanus. Tomorrow. Free. XO-OX.org" | All channels | Clean link card |

### Community Seeding — Pre-Launch Contacts

Before launch day, brief the following communities so they are warm rather than cold-contacted on the day:

**Barry OB's team:** Brief them T-5. Send them the press kit, a pre-release download link, and the posting plan. Their role is to post personal testimonials (not identical copy) on launch day — "I've been using this pre-release and here's what surprised me." Coordinate to avoid identical language across posts. Give them: 3 key talking points, the short description, and 2-3 audio clips to share.

**Beta testers (if any):** T-5. Send download, ask for brief feedback quote that can be used in launch posts. Ask them to be ready to comment/upvote launch day posts to boost visibility.

**KVR Audio:** T-3. Submit a product listing to the KVR free plugin directory. This can go live on launch day with a forum post.

**Elektronauts admin contact:** T-3. Ask if an announcement thread is appropriate. Elektronauts is particularly strong for hardware-forward producers who will appreciate the MPC/XPN angle.

**Reddit mods (r/synthesizers, r/edmproduction):** No advance contact needed — organic posts perform better. But draft the post text T-3 so it's ready to paste.

---

## Launch Day

### Hour-by-Hour Timeline

Launch day begins at 9:00 AM your local time. All times are relative to that anchor.

| Time | Action | Platform | Notes |
|---|---|---|---|
| 8:00 AM | Pre-flight check: build one last time, auval pass, download link live | Local + site | Do not skip this |
| 9:00 AM | **XO-OX.org Download page goes live** | Site | The source of truth |
| 9:05 AM | **GitHub repo goes public** | GitHub | Public release, tag v1.0.0 |
| 9:10 AM | **Patreon page live** | Patreon | With Tier 1 ($3), Tier 2 ($8), Tier 3 ($20) active |
| 9:15 AM | Post to **Twitter/X** — short template | Twitter/X | "XOceanus V1.0 is out. [link]" |
| 9:20 AM | Post to **Instagram** — medium template | Instagram | Screenshot + medium copy |
| 9:25 AM | Post to **r/synthesizers** | Reddit | Full forum-style post — see long template |
| 9:35 AM | Post to **r/edmproduction** | Reddit | Same post, slightly reframed for production angle |
| 9:45 AM | Post to **r/WeAreTheMusicMakers** | Reddit | Frame around the "free instruments for everyone" mission |
| 10:00 AM | Post to **KVR Audio forum** | KVR | Pre-prepared post: product listing thread in appropriate subforum |
| 10:15 AM | Post to **Elektronauts** | Elektronauts | Frame around hardware integration + XPN/MPC angle |
| 10:30 AM | Post to **Gearspace** | Gearspace | Digital instruments / software synths subforum |
| 10:45 AM | Post to **Discord** — #announcements (XO_OX server) | Discord | Pin the post |
| 11:00 AM | Post **Signal feed update** on XO-OX.org | Site | "XOceanus V1.0 is live" with download link |
| 11:30 AM | **Barry OB's team posts** — coordinated but independently phrased | All their channels | Not simultaneous with your posts — stagger by 2 hours |
| 12:00 PM | Begin monitoring and responding to comments | All channels | Every comment in first 6 hours is critical for algorithm |
| 2:00 PM | Send **outreach emails to press** — Sonic State, Bedroom Producers Blog, Attack Magazine | Email | Short pitch: "I shipped this today, here's the press kit" |
| 4:00 PM | Post launch **Field Guide article**: "XOceanus V1.0 — What shipped and what's next" | XO-OX.org + Signal | Honest accounting, roadmap tease |
| 6:00 PM | **YouTube** — upload the "What is XOceanus?" 60-second trailer | YouTube | Short-form, hook in first 5 seconds |
| 8:00 PM | End-of-day check: download count, biggest threads, any P0 bugs reported | All channels | Identify if hotfix needed |

### Channel-by-Channel Posting Plan

**Twitter/X**
- Post 1 (9:15 AM): Short template — clean announcement, direct link
- Post 2 (2:00 PM): "Three hours in — here's what people are saying" (if positive reactions are rolling in)
- Post 3 (6:00 PM): YouTube trailer drop
- Engage with every mention in the first 24 hours

**Reddit (r/synthesizers, r/edmproduction, r/WeAreTheMusicMakers)**
- One post per subreddit, 10-minute intervals
- Long template, slightly adapted per community tone
- r/synthesizers: lead with the DSP variety (GENDY, cellular automata, Mixtur-Trautonium)
- r/edmproduction: lead with coupling, preset depth, workflow speed
- r/WeAreTheMusicMakers: lead with the mission (free instruments, no barriers)
- Monitor comments for the first 6 hours, reply to every question

**Elektronauts**
- Frame around hardware producers: XPN export means XOceanus presets run on your MPC without a computer
- Lead with ONSET (percussion), OBRIX (modular bricks), the MPC workflow
- Elektronauts regulars are skeptical of software — be direct, not hype-forward

**Gearspace**
- Straightforward announcement in the software synths subforum
- Include the press kit link
- Gearspace rewards thoroughness — include the full spec table

**KVR Audio**
- Submit product listing before launch, have it go live on launch day
- Post in KVR forum with product details + screenshots
- KVR is an indexing resource — make the listing comprehensive for SEO

**Discord**
- Launch announcement in #announcements, pin it
- Brief welcome message framing the discord as the community home
- Set up the initial channel structure (see Discord structure in Community section)

**YouTube**
- 60-second "What is XOceanus?" trailer — the hook video
- Title: "XOceanus — 42 Synthesis Engines, Free" (SEO-first)
- Description: full short description + download link in first 3 lines
- Tags: free synth plugin, free vst, open source synthesizer, multi-engine synth

### Download Page Go-Live Sequence

1. Upload the compiled AU bundle + standalone app to a permanent host (Backblaze B2 or GitHub Releases — not Google Drive)
2. Test the download link on a clean machine before launch day
3. Download page includes: system requirements (macOS 11+, Apple Silicon native), installation instructions (drag to /Library/Audio/Plug-Ins/Components/), and a "verify your installation" step (open in DAW, check XO_OX manufacturer)
4. Have a backup download mirror ready (GitHub Releases as primary, direct site link as backup)

---

## Post-Launch: T+1 to T+14

### Community Engagement Cadence

The first two weeks determine whether the community self-sustains or dies. Be present.

| Day | Priority Action |
|---|---|
| T+1 | Reply to every unanswered thread from launch day. Post "Day 1 wrap" to Signal feed with honest numbers. |
| T+2 | Post first community prompt: "What's the first coupling patch you made? Share it." in Discord |
| T+3 | Engine spotlight drop: OBRIX — the flagship. Post to Twitter/X + Reddit with audio clip and parameter walkthrough |
| T+5 | Post to Twitter/X: "Most popular engine in the first 5 days, based on preset downloads: [result]" — real data, no spin |
| T+7 | **Week 1 wrap post** on XO-OX.org Signal feed: honest summary, top community moments, any hotfixes shipped |
| T+7 | Launch **Community Challenge #1** (see below) |
| T+10 | Engine spotlight drop: ORGANISM or ORACLE — lean into the unusual engines |
| T+14 | **Guru Bin Transcendental Vol 1 announcement** (see below) |

### Bug Response Protocol

**P0 (audio crash, data loss, silent output on all presets):**
- Hotfix within 24 hours
- Post status update to Discord #announcements and Twitter/X within 2 hours of confirmation
- Do not wait for a "perfect" fix — a known workaround posted immediately is better than silence
- Tag: `v1.0.1` hotfix release

**P1 (incorrect DSP behavior, coupling misroutes, preset loading failures for some engines):**
- Fix within 1 week
- Acknowledge in Discord within 24 hours
- Batch with other fixes into `v1.0.x` point release

**P2 (UI glitches, preset browser edge cases, non-critical quality issues):**
- Track in GitHub Issues
- Batch into V1.1 (Aquatic FX milestone)
- Acknowledge with "tracked, coming in V1.1" response

**Tone:** Never defensive. "Good catch, here's what we know, here's the timeline." XO_OX's credibility is built on transparency, not perfection.

### Content Drip (Field Guide + Signal)

The Field Guide has 14 published posts and 16 planned. Launch creates an opportunity to tie content to momentum:

| Week | Field Guide Content | Signal Update |
|---|---|---|
| T+1 | "XOceanus V1.0 — What shipped and what's next" | Launch day stats, top threads, first patch reports |
| T+2 | "The Coupling System Explained" — full technical walkthrough of MegaCouplingMatrix | Community Challenge #1 announcement |
| T+3 | Engine deep-dive: OBRIX — modular bricks, coral reef mythology, Wave 3 architecture | Engine spotlight: OBRIX Awakening presets live |
| T+4 | Engine deep-dive: ORACLE — GENDY stochastic, Maqam tuning, Buchla blessing | — |
| T+7 | "Two Weeks In" — Week 2 wrap, Community Challenge #1 results | Guru Bin Transcendental Vol 1 teaser |
| T+10 | Engine deep-dive: ORGANISM — cellular automata, Coral Colony, generative patches | — |
| T+14 | **Guru Bin Transcendental Vol 1 announcement post** | Vol 1 pricing, contents, how to access via Patreon |

### Feedback Collection Strategy

**What to collect (passive):**
- GitHub Issues: bugs, feature requests, documentation gaps
- Discord #feature-requests: community wants, organized by upvote
- Reddit/forum threads: organic usage patterns, first impressions, "wish it did X"

**What to actively ask:**
- T+3: Post a 5-question form (not a survey wall) asking: Which engine surprised you? What's the first coupling you tried? What broke? What's missing? What should we build next?
- T+7: In the Week 1 wrap post, share aggregate results from the form — people who gave feedback want to see it acknowledged

**What to actually do with it:**
- P0/P1 bugs: hotfix protocol above
- "Missing engine X" requests: fuel for the bi-monthly free release cadence — announce the pipeline
- "Would pay for Y": fuel for premium boutique engine roadmap
- Coupling pairs people are discovering: curate the best into the next Entangled preset batch

### Guru Bin Transcendental Vol 1 Announcement (T+14)

Wait until T+14 to announce Vol 1. Reasons:
1. Launch day is for the free product — don't introduce monetization in the same breath
2. Two weeks gives you real usage data to know which engines have the most enthusiasm
3. It gives Patreon patrons (early adopters) something to look forward to

**Vol 1 contents (per memory):** OBRIX, OSTINATO, OPENSKY, OCEANDEEP, OUIE — 15-20 presets per engine, Scripture lore, XPN pack, PDF booklet.

**Announcement framing:**
> "Two weeks in, the community has made it clear which engines are resonating. Guru Bin Transcendental Vol 1 is coming — 5 engines, 75-100 deeply designed presets, Scripture, God revelations, XPN pack for MPC users, and a PDF sound design booklet. Patreon supporters get it first. Everything else stays free. Details next week."

---

## Community Structure

### Discord Server Channels

Set up before launch. Do not launch without a Discord:

```
INFORMATION
  #announcements          — pinned releases, updates (bot-only + owner)
  #getting-started        — FAQ, installation, known issues

COMMUNITY
  #general                — open chat
  #show-your-work         — tracks + patches made with XOceanus
  #preset-sharing         — post .xometa files, coupling patches
  #coupling-experiments   — cross-engine discovery (the signature channel)

CREATION
  #sound-design-tips      — techniques, approaches, questions
  #field-guide            — discussion of Field Guide posts
  #feature-requests       — organized by reaction-count

DEVELOPMENT
  #bug-reports            — mirrors GitHub Issues
  #developer-chat         — code contributions, architecture
  #oxport                 — XPN export pipeline, MPC workflow

PATREON
  #patron-only            — Patreon supporter channel (auto-gated)
```

### Community Challenge #1 (T+7)

Title: **"The Corridor"**

Prompt: "Build a patch where disabling the coupling strip makes the sound collapse. The coupling must be load-bearing — not decoration. Post your patch as a .xometa file + a 30-second audio clip: first the full patch, then COUPLING macro at 0."

Prize: Featured on XO-OX.org as a Guest Exhibition. Named preset in the next factory batch.

This challenge is designed to teach coupling while generating community content. The winning submissions become educational examples.

---

## Post-Launch Roadmap Signaling

At launch, signal what comes next. This converts scope cuts into anticipation.

| Milestone | Timing | Announcement Frame |
|---|---|---|
| V1.1 — Aquatic FX Suite | 4-6 weeks post-launch | "The FX layer that sounds like XO_OX regardless of engine is in build" |
| Guru Bin Transcendental Vol 1 | T+14 (see above) | Premium deep dive, Patreon-first |
| OSTINATO (first premium boutique engine) | Post-V1.1 | "A full drum circle instrument in one slot — voice + custom FX" |
| Bi-monthly free engine releases | Ongoing cadence | Name the first 2-3 candidates publicly to demonstrate the pipeline is real |
| VST3 | V2 timeline | "It's on the roadmap, macOS AU first" |
| iOS AUv3 | V2 timeline | "Built from day one for iOS, validating in V2" |

---

## Success Metrics (First 14 Days)

These are the signals that tell you whether the launch worked. Do not obsess over vanity numbers — obsess over engagement quality.

| Metric | Target (14 days) | Why It Matters |
|---|---|---|
| Downloads | 500+ | Adoption floor for a credible V1 |
| GitHub stars | 200+ | Open-source health signal |
| Discord members | 100+ | Community viability |
| Reddit upvotes (top thread) | 200+ | Organic discovery |
| Bug reports filed | 10-30 | People are using it (no reports = no users OR silent failures) |
| Preset .xometa files shared in Discord | 5+ | Active coupling experimentation |
| Patreon supporters | 20+ | Revenue viability signal for Tier 2 content creation |

**Warning signals:**
- Zero bug reports after 3 days: people aren't using it, or the download isn't working
- All feedback is "what about VST3?" — strong signal to communicate the roadmap clearly
- No Discord activity outside of #announcements: community seeding failed, need to ask prompting questions
- Negative Reddit thread getting traction: respond directly, not defensively, in the thread

---

## Appendix: Pending Pre-Launch Work Items

These items were identified in existing docs as open before launch. Track them here:

| Item | Doc Reference | Status |
|---|---|---|
| Record 20 hero preset audio clips for site | `site-sample-recordings.md` | PENDING |
| Create XObese .xpn bundle + deploy | MEMORY.md | PENDING |
| Fix Patreon URL across all pages | MEMORY.md + multiple docs | PENDING |
| ORBWEAVE 150 factory presets | `fleet_health_2026_03_20.md` | CRITICAL / PENDING |
| Aquarium water column update for OVERTONE, ORGANISM, ORBWEAVE | `morning_plan_2026-03-20.md` | PENDING |
| Seances for 29 engines without verdicts | `fleet_health_2026_03_20.md` | Non-blocking for V1 |
| SNAP AmpToFilter coupling wire | `release_readiness_12k.md` | Deferred to V1.1 |
| AudioToBuffer Phase 3 | `release_readiness_12k.md` | Deferred to V1.1 |
| Drift FX gap (1,353 standalone params not exposed) | `drift_fx_gap_analysis.md` | Deferred to V1.1 |
| Sound design guide: 4 missing Constellation engines | `release_readiness_12k.md` | PENDING |

---

*This document supersedes `Docs/plans/v1-launch-plan.md` (34-engine scope, March 17 decisions).
The current product has 46 engines and ~22,000+ presets. All messaging should reflect the actual state.*
