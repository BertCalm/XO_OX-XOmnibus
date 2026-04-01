# XOlokun V1 — Community Activation Plan

**Author:** Barry OB (Community Lead)
**Date:** 2026-03-20
**Status:** Launch Day Ready
**Scope:** Day 1 playbook + 30-day engagement cadence

---

## 1. Community Team Roster & Roles

XO_OX is a solo operation. The community team is the owner wearing different hats across different platforms — but each platform needs a distinct posture. This section maps the hat to the platform so the voice stays consistent even when the schedule is compressed.

### Platform Assignments

| Platform | Who Covers It | Primary Role | Response Window |
|----------|--------------|-------------|-----------------|
| Reddit (r/synthesizers, r/edmproduction, r/WATMM) | Barry OB | Authentic presence, skeptic engagement | 4 hours during launch week; 24 hrs steady state |
| Elektronauts | Barry OB | Gear-literate deep dive, MPC/Syntakt crossover | 8 hours |
| Gearspace | Barry OB | Technical credibility, spec questions | 8 hours |
| Twitter/X | Barry OB | Quick loop drops, milestone announcements | 2 hours during launch; 6 hrs steady state |
| Discord (XO_OX server) | Barry OB + community moderators (when they emerge) | Real-time Q&A, preset sharing, Patreon channel | 1 hour during launch week |
| YouTube comments | Barry OB | Thank-yous, follow-up links, preset requests | 24 hours |
| GitHub Issues | Owner (dev hat) | Bug triage, feature discussion | 48 hours |

### Response Time Targets

**Launch week (days 1-7):**
- Twitter/X mentions: 2 hours
- Discord DMs and #general: 1 hour during waking hours
- Reddit comments: 4 hours
- Elektronauts/Gearspace threads: same-day if posted before 8pm

**Steady state (weeks 2-4):**
- Twitter/X: 6 hours
- Discord: 4 hours
- Reddit: 24 hours
- Elektronauts/Gearspace: 48 hours
- GitHub bugs: 48 hours

### Escalation Protocol

**Bug reports:**
1. Acknowledge publicly within the platform ("Got it, logging this now")
2. Open a GitHub issue with the platform thread linked
3. Reply with the issue URL within 24 hours so the reporter can track it
4. Never promise a fix timeline unless it's already done
5. When fixed: reply to the original post with the version number that resolves it

**Feature requests:**
1. Acknowledge and thank ("That's interesting — noting this")
2. No commitment language. "We'll explore that" is the ceiling
3. If the same request surfaces 5+ times independently, flag it as a community signal
4. Patreon supporters get a vote on feature/expansion priorities — point requesters there

**Hostile posts / "this is just another synth" dismissal:**
1. Do not argue. Do not defend
2. One measured response, then let it go: "Fair — make your own call after trying it"
3. If good-faith skeptics raise legitimate technical questions, answer those specifically
4. If it escalates to personal hostility, stop engaging and let the community respond naturally
5. Never delete negative comments unless they violate platform rules

---

## 2. Launch Day Playbook

### Pre-Launch Checklist (complete before Day 1 posts go live)

- [ ] XO-OX.org is live with all 7 pages rendering correctly
- [ ] Download link verified (AU + Standalone built, installers tested)
- [ ] Discord server is live with #general, #presets, #bugs, #patreon channels
- [ ] At least 2 demo tracks or audio clips are rendered and uploaded
- [ ] The Field Guide has at least 1 published post that serves as a "start here" entry point
- [ ] GitHub releases page has v1.0 tagged with changelog
- [ ] Patreon page is live with correct tier descriptions

### Platform Sequence — First 24 Hours

All times are relative to launch hour (L = launch). Stagger posts to avoid looking like a bot blast.

---

**L+0 — Twitter/X: The Opening**

Post 1 (text + screenshot):
> XOlokun V1 is out. Free, open-source, 42 synthesis engines.
> Download: [link]

Keep it flat and factual. No adjectives. No "finally." No exclamation points. Producers who've shipped things respect restraint.

Post 2 (30 min later, audio loop):
> OBRIX + OCEANDEEP coupled on Atmosphere > [loop file]
> This is what 15 coupling types sounds like

Do not explain the coupling. Let the sound be the point.

---

**L+1 — Reddit r/synthesizers: The Authentic Drop**

Title: "I spent the last year building a free, open-source multi-engine synth. 76 engines, ~15,000 presets. Here's what I learned about coupling."

Format: Personal post, not a launch announcement. Open with the honest version of what this was — a solo project that grew into something unexpected. Talk about one specific design decision: why coupling became the organizing principle instead of more knobs per engine. Link to the Field Guide post on coupling philosophy.

Key framing decisions:
- Lead with the conceptual insight, not the spec sheet. r/synthesizers readers can smell marketing.
- Include one concrete technical detail (the KnotMatrix topology, the water column atlas, the seance process — pick one)
- End with an invitation, not a CTA: "Happy to answer questions about the DSP or the design choices"

Do NOT post the full engine list. Do NOT say "15,200 presets." Scale claims invite skepticism before they invite curiosity.

---

**L+2 — Elektronauts: The MPC Thread**

Start a thread in the MPC/Akai section (or General Synthesis if that's more active):

Title: "XOlokun V1 — free synth platform with XPN expansion export. Works with MPC."

Open by acknowledging the audience: "I know Elektronauts is MPC-first. The XPN export pipeline was built specifically for this workflow." Then detail the export path: AU plugin → Oxport tools → .xpn bundle → MPC program. Show a screenshot of a rendered expansion in MPC. Be specific about which engines produce the most MPC-useful output (ONSET for drums, OBRIX for textured samples, OPENSKY for melodic pads).

This post can be more spec-forward because Elektronauts readers want to know if it actually works with their gear.

---

**L+3 — Discord: Go Live**

Pin a #start-here message with three links: download, Field Guide entry point, GitHub. Open a #launch-day thread and answer questions in real time for a 2-hour window. Then let it breathe.

Do not moderate aggressively on day one. Let early adopters set the tone.

---

**L+4 — Gearspace: The Technical Credibility Play**

Gearspace readers skew older, more skeptical, and more gear-literate. The right entry point is not a product announcement — it's a design discussion thread.

Title: "Cross-engine coupling in a multi-synth architecture — some DSP decisions I had to make"

Open with a technical question that naturally leads to XOlokun: how do you route modulation between engines that were designed independently? Describe the MegaCouplingMatrix approach. Mention the 15 coupling types. Let the product emerge from the technical discussion, not the other way around.

If it lands, it lands. Gearspace cannot be forced. A genuine technical thread that happens to describe XOlokun is worth ten press releases.

---

**L+6 — Twitter/X: Engine Spotlight (first of many)**

Pick one engine that photographs well and has a clear personality. OBRIX (Reef Jade, modular brick architecture) or ORGANISM (Emergence Lime, cellular automata) work for this — they have concepts that translate to 280 characters.

> ORGANISM engine: cellular automata rules generate the timbre.
> Rule 30: unstable, chaotic. Rule 110: complex, rhythmic. Rule 90: fractal symmetry.
> You set the rule. XOlokun evolves the sound.
> [screenshot]

---

**L+8 — Reddit r/edmproduction: The Workflow Post**

Different angle from r/synthesizers. This audience wants to know: can I use this in a track? How does it fit my workflow?

Post a before/after: a plain MIDI clip, then the same MIDI through OBLONG + OVERDUB coupled on Character, then the same passage with OMBRE blended in. Short descriptions of what changed at each step. Link to the full Field Guide post on workflow integration.

---

**L+12 — Reddit r/WeAreTheMusicMakers: The Personal Story**

WATMM is the right place for the "why I built this" version. Not a launch post — a creative journal entry that happens to mention XOlokun is out now. Talk about the aquatic mythology angle, the seance process, the decision to name things after water-column depth zones. Let the mythology be human, not mystical.

---

**L+24 — Consolidation Post**

Twitter/X recap of day-one response. Thank the community. Surface one good question from Discord and answer it publicly. This signals that engagement is monitored and valued.

---

### How to Handle the Skeptic Crowd

Reddit and Gearspace will have skeptics. Some are good-faith, some are reflexive. Know the difference.

**Good-faith skeptics ask things like:**
- "How does this compare to Bitwig's Grid?"
- "What's the CPU overhead of 4 coupled engines?"
- "Is this actually 76 engines or 76 variations of one architecture?"

Answer these directly, technically, without hedging. These readers can become your most effective advocates if you engage honestly. "The Grid is modular and more open-ended; XOlokun is opinionated — each engine has a character, coupling is curated not arbitrary. Different tools." That's an honest answer that respects both tools and the questioner.

**Reflexive skeptics say things like:**
- "Just another free synth nobody will use"
- "76 engines is marketing fluff"
- "This will be abandoned in 6 months like everything else"

One response maximum, no defensiveness:
- "Fair concern. The Field Guide has 14 posts and the first engine shipped 18 months ago. Make your own call."
- Then stop.

Do not feed the thread. The good-faith readers can see the difference. Your silence is more credible than a defense.

---

### Free vs. Patreon — What to Share Where

**Always free, share loudly:**
- The download itself (76 engines, all presets, XPN export tools)
- Field Guide posts (technical depth, mythology, sound design philosophy)
- Preset videos and audio loops
- GitHub discussions and technical architecture
- Awakening tier presets (10 hero presets per engine — these are the advertisement)

**Patreon — tease the experience, not the content:**
- Studio Sessions (behind-the-scenes sound design) — share a 60-second clip, link to full version for patrons
- Transcendental preset packs — share one preset from a pack, not the pack itself
- Vote rights on next engine expansion — mention this exists, don't make it sound exclusive ("patrons help shape what comes next")
- Lore booklets — share one illustration or one mythology passage publicly; the full booklet is the reward
- Quarterly 1:1 calls ($20 tier) — just mention it's limited to 20 patrons; scarcity is self-evident

**Never frame Patreon as "locking features."** The framing is: "XOlokun is complete as it is. Patreon funds what comes next."

---

## 3. Ongoing Engagement — First 30 Days

### Weekly Content Rhythm

**Every Monday: Engine of the Week**
One engine, one post per platform. Format: Name + accent color + one sentence mythology + one audio loop + one fact about the synthesis architecture. Keep it short. This is not a tutorial — it's a character introduction.

Rotate through engines in order of accessibility: start with OPENSKY (euphoric, immediately readable), OBLONG (warm, familiar), ONSET (drums, universal), then move toward the more conceptual engines (OUROBOROS, ORACLE, ORGANISM) once the audience trusts the project.

**Every Wednesday: Field Guide post (or cross-post)**
Either publish a new post or cross-post an existing one that fits a current conversation thread. If someone on Discord asked about coupling last week, the Wednesday post should link to the coupling philosophy piece. Let community questions drive the editorial calendar.

**Every Friday: Preset of the Week**
One preset, one audio clip, one paragraph about what's happening sonically. No parameter spoilers — describe what you hear, not how it was made. "Something like a fluorescent organism expanding in pressure-dark water" beats "filter envelope at 60% with OBRIX Brick 3 feeding OCEANDEEP's resonance."

Tag the preset mood and engine. Invite the community to remix it: "What would you do differently with ORGANISM coupled in?"

**Every other weekend: Community Showcase**
Surface a user-submitted track or preset that used XOlokun. One paragraph of genuine commentary — not a repost, a response. What did they do that surprised you? This makes contributors feel seen and shows the community that the project is alive and used.

---

### Community Challenges

**Month 1 Challenge: The Water Column**

Challenge the community to create a track that moves through the water column — from surface to trench. Use engines from different depth zones in the aquarium atlas. Share results with #WaterColumn on Twitter/X and in Discord.

Criteria (optional, not a competition): Does the track feel like it descends? Does each section have a distinct character? Can you feel the pressure change?

Prize: Pick one submission for a Field Guide feature. No cash. The feature is the prize.

**Month 1 Challenge: 4-Engine Coupling**

Use all 4 engine slots with at least 3 active coupling routes. Share the preset file and the audio. Describe the coupling chain in one sentence.

This serves a secondary purpose: it generates community presets that demonstrate advanced coupling, which Barry OB can share as social proof.

---

### User Preset Showcase Pipeline

1. Community members share presets in Discord #presets channel (`.xometa` files)
2. Barry OB auditions them weekly — listening pass, not just reading param values
3. Selected presets get a "Community Pick" designation in Discord
4. Monthly: 3-5 Community Picks get featured in a Twitter/X thread
5. Quarterly: The best Community Picks from that quarter are packaged into a free "Community Collection" download

Do not curate by technical correctness — curate by character. A preset that sounds memorable but makes no DSP sense is more valuable to the community than a technically correct but boring one.

---

### Converting Free Users to Patreon — The Natural Path

Do not push Patreon at people. Let the product do it.

The conversion moment happens when a user:
- Finishes a track they love using XOlokun and wants to support what made it possible
- Sees a Studio Session clip and wants the full walkthrough
- Has a feature idea and learns that Patreon supporters get a vote
- Gets a response to a bug report and wants to thank the team beyond a comment

Structure the conversion invitation around these moments:

- After someone shares a track they made: "Glad this worked for your track. If you want to go deeper on the engine, patrons get monthly Studio Sessions. No pressure — just mentioning it."
- After resolving a bug: "Fixed in [version]. If you want to support the project, www.patreon.com/cw/XO_OX — patrons vote on what gets built next."
- In the weekly Engine of the Week post: a single line at the bottom — "The Studio Session walkthrough for [ENGINE] is on Patreon."

Never lead with Patreon. Never end a post with a Patreon pitch. Let it be one more thing available, not the point.

---

### Surfacing Community Feedback to the Dev Team

The dev team is the owner. The community team is also the owner. This section is about not losing signals.

**Weekly review (Friday, 30 min):**
Read through the week's Discord messages, Reddit comments, Gearspace replies. Flag:
- Bug reports not yet logged in GitHub (log them immediately)
- Feature requests that surfaced 3+ times (add to a `Docs/community-signals.md` running list)
- Presets or tracks worth showcasing next week
- Questions that suggest a missing Field Guide post

**Monthly synthesis (end of month):**
Review `community-signals.md`. Are there patterns? Is the same confusion coming up repeatedly? That's a documentation gap. Write a Field Guide post that answers it.

Are there feature requests that align with what's already planned? Mention them in the next Patreon update: "Several of you asked about [X] — this is already in progress."

Are there feature requests that are genuinely out of scope? Say so clearly once, publicly, with a brief reason: "MIDI export isn't on the V1 roadmap — it requires a different export architecture than XPN. It's on the V2 list."

---

## 4. Tone Guide

### How XO_OX Talks to Producers

**The mental model:** You are a sound designer talking to other sound designers. You shipped something you're proud of. You're not trying to sell it — they can have it for free. You're sharing it.

**What this sounds like:**

Good:
> "OCEANDEEP has a pressure macro. It doesn't add reverb — it narrows the stereo field and boosts the low-mids in a way that actually sounds like depth. I was surprised it worked."

Bad:
> "Featuring our innovative Pressure Macro technology, XOlokun delivers immersive spatial synthesis experiences."

Good:
> "The coupling between ORCA and ONESET is not gentle. The echolocation LFO from ORCA is hitting the ONSET percussion release. It's a lot. I like it."

Bad:
> "XOlokun's revolutionary cross-engine coupling system enables producers to achieve sounds previously impossible."

**Rules:**
- Write in first person. "I built this" not "XO_OX Designs has created."
- Use specific sonic language. "Bright, vowel-y, slightly unstable" beats "rich and expressive."
- Name the engine, name the macro, name the coupling type. Be specific.
- Admit uncertainty. "I'm not sure this preset translates across audio interfaces — let me know" is honest and invites engagement.
- Keep it short. Most posts should be under 150 words. If you need more, write a Field Guide post and link to it.

---

### Handling Criticism

Three types of criticism. Different responses for each.

**Type 1: Technical criticism (something is actually wrong)**
Engage immediately. Thank them. Fix it or explain why it's designed that way. If it's a bug: "You're right, this is wrong. Logged at [GitHub link]." If it's a design choice: explain the tradeoff, not the conclusion. "This was intentional — the alternative caused [specific problem]. Happy to revisit it if that tradeoff isn't working."

**Type 2: Opinion criticism ("this sounds too digital / too complex / not my style")**
One sentence of acknowledgment, no defense: "Fair — it's not for everyone." Do not try to change their opinion. Do not send them the settings to make it sound different. They told you what they think. Respect it.

**Type 3: Bad-faith attacks (trolling, dismissal without engagement)**
One response maximum, then silence. The response should be neutral and factual: "You can download it and test it at [link]." After that, nothing. Do not reply to follow-ups. Do not acknowledge quote-tweets. The community will handle it. If they don't, it doesn't matter.

---

### When to Be Technical vs. Accessible

**Be technical when:**
- Answering a specific DSP or architecture question
- Posting on Elektronauts, Gearspace, or GitHub
- Someone is debugging an issue
- The Field Guide topic requires it

**Be accessible when:**
- Posting on Twitter/X (assume the audience is varied)
- Posting on Reddit (even r/synthesizers has many beginners)
- Describing what a preset does vs. how it's made
- Introducing an engine for the first time

**The rule:** accessible first, technical on request. Lead with what it sounds like, follow with how it works. Never lead with architecture when sound will do.

---

### The Mythology — What to Share Publicly vs. Patreon

**Public (share freely):**
- The water column atlas concept — it's a great hook and immediately visual
- Engine character descriptions (OCEANDEEP is the trench, OPENSKY is the surface euphoria)
- The feliX-Oscar polarity — this is central to the product identity, not an easter egg
- Aquarium depth zone assignments — it's a creative way to describe sonic character
- The fact that every engine has a mythology

**Patreon (reward for support):**
- Full lore booklets for each engine (the Guru Bin Retreat text)
- The complete Aquatic Mythology document in illustrated form
- Studio Sessions that explore engine mythology through sound design
- Vote on which retreat gets written next

**Internal (never share):**
- The full seance transcripts and ghost council debates — these are your development process, not your product
- Specific technical implementation details that could be used to clone the DSP work
- The RAC deliberation records, funding plan specifics, V1 scope decisions

**The principle:** The mythology makes the product feel alive. Share enough that people feel they're discovering a world. Reserve enough that there's always something more for patrons to find.

---

## Appendix: Launch Day Message Templates

### Reddit Launch Post Template (r/synthesizers)

> **Title:** Free multi-engine synth — 76 engines, cross-engine coupling, ~15,000 presets. Here's what I learned building it.
>
> I've been building XOlokun for about a year. It started as a way to put six character instruments I'd built separately into one platform. Then the coupling between engines became more interesting than the engines themselves.
>
> The basic idea: you load up to 4 engines, then route modulation between them. One engine's LFO shapes another engine's filter. A drum hit from ONSET triggers a harmonic shift in OPENSKY. The 15 coupling types are the core of the sound design workflow — not the individual engine controls.
>
> It's free, open-source, AU + standalone for macOS. 76 engines ranging from straightforward (OBLONG, OPENSKY) to genuinely weird (OUROBOROS, ORGANISM).
>
> Download + Field Guide: [XO-OX.org]
> GitHub: [link]
>
> Happy to answer questions about the DSP architecture, the coupling system, or the design decisions that didn't work.

---

### Discord Pinned Message Template

> **Welcome to XO_OX Designs — XOlokun V1 is live.**
>
> **Start here:** XO-OX.org — download, Field Guide, engine guide
> **Bugs:** #bugs channel, or GitHub Issues (links above)
> **Presets:** Share .xometa files in #presets — we listen to everything
> **Patreon:** www.patreon.com/cw/XO_OX — Studio Sessions, early access, vote on what comes next
>
> The server is small and real. Ask questions, share what you make, be direct about what isn't working.

---

### Engine of the Week Template (Twitter/X)

> **Engine of the Week: [ENGINE NAME]**
> [One sentence: what it sounds like]
> [One sentence: what makes it unusual]
> Character: [Accent color + creature mythology in 6 words]
> [Audio loop or screenshot]
> All 76 engines, free: XO-OX.org

---

*Document maintained by Barry OB. Update after each major community milestone.*
