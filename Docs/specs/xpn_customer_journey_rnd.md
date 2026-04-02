# XPN Pack Customer Journey — R&D Spec

**Date**: 2026-03-16
**Status**: R&D / Pre-Spec
**Owner**: XO_OX Design
**Audience**: Internal strategy, Barry OB community team, Gumroad/email ops

---

## Overview

This document maps the complete journey of an MPC producer from first awareness of XO_OX through purchase, first-use, retention, and community advocacy. Each stage identifies key actions, key emotions, friction points, and XO_OX responses. The goal is a journey where every handoff feels earned — not coerced — and where the product's depth reveals itself naturally over time.

The XPN pack buyer is a specific person: they own an MPC, they care about sound quality and workflow above all, they are skeptical of marketing, and they have been burned by expansion packs that sounded good in the preview and thin on the pads. Our job is to be the exception they tell other producers about.

---

## Stage 1: Discovery

### Key Actions

- Producer encounters XO_OX content while searching for MPC workflow help or new sounds
- Reads a Field Guide post that answers a real question they had (e.g., "how do I build a drum kit from a synth?")
- Watches a YouTube tutorial demonstrating XOceanus + MPC workflow end-to-end
- Sees an r/mpc or r/synthesizers thread where another producer recommends XO_OX
- Finds the XO-OX.org Aquarium or pack listing through organic search
- Hears a community beat challenge entry that credits XO_OX engines

### Key Emotions

- Curiosity: "This looks different from the usual expansion pack"
- Mild skepticism: "Every pack claims to be unique"
- Recognition: "These people actually know MPC — they're not just porting samples"
- Intrigue: "I've never seen a synth described as a water creature before"

### Friction Points

- Discovery is passive — no pull unless the content ranks or circulates
- MPC-specific content is crowded; generic synth tutorials don't reach the right audience
- Field Guide posts must earn trust before they earn clicks to the store
- Brand mythology (aquatic identity, engine characters) can read as pretentious before context is given

### XO_OX Response

- **SEO anchor**: Field Guide posts target long-tail MPC queries ("MPC program file format explained," "XPM velocity layers tutorial," "best MPC expansion packs for sound design")
- **YouTube strategy**: tutorials show the full workflow — XOceanus preset → XPN export → MPC import → played live. No skip steps. Shows mastery.
- **Community presence**: genuine participation in r/mpc and MPC-Samples.com threads, not promotional drops
- **First impression standard**: every piece of XO_OX content must deliver one thing the viewer/reader couldn't find elsewhere. "These people know MPC deeply" is the only acceptable exit emotion.
- **Pack challenge seeding**: community beat challenges give producers a reason to engage before they have spent anything

---

## Stage 2: Consideration

### Key Actions

- Producer lands on XO-OX.org pack listing or Gumroad page
- Downloads the free Foundation Sampler (email captured)
- Listens to hero audio clips for the pack they are considering
- Reads the README for pack structure and compatibility info
- Checks MPCE_SETUP.md to confirm their MPC model is supported
- Compares price against competing expansion packs ($9–$29 range typical)
- Evaluates sample uniqueness: "Can I hear the engine character in these samples?"

### Key Emotions

- Hope: "This might be the one that sounds as good as the preview"
- Anxiety: "Will this actually work on my MPC Live II?"
- Decision paralysis: too many options, unclear differentiation between packs
- Relief when compatibility is explicitly confirmed: "It says right here: MPC Live, One, X, Key 61"

### Friction Points

- **No audio preview**: a pack without hero clips is dead on arrival. Producer cannot risk purchase without hearing it.
- **Unclear MPC model compatibility**: if the listing does not explicitly name supported models, they will not buy
- **Confusing import instructions**: any ambiguity in the setup flow creates fear of a broken purchase
- **Email barrier on free download**: too much friction on the sampler = abandoned without capture
- **Price without context**: $19 feels high without understanding what makes XO_OX different from stock sample packs

### XO_OX Response

- **Foundation Sampler**: one-click download, email optional but incentivized (email = pack update notifications). 8–12 sounds covering the range of one engine family. Full MPC-ready XPN, not a zip of WAVs.
- **Hero clips**: 60–90 second audio demo per pack, produced at the same standard as the pack itself. Played on MPC hardware, not a DAW render. Producers hear the velocity layers, the round-robins, the engine character in action.
- **Compatibility table**: every listing has an explicit table — MPC One / Live / Live II / X / Key 37 / Key 61 / Force. Green checkmark or explicit note. No vagueness.
- **README + MPCE_SETUP.md**: standardized across all packs. Same file names, same structure, same step count. Reduces learning curve per new pack.
- **Price framing**: the listing copy frames packs as instruments, not samples. "150 presets from a synthesis engine that took 18 months to build" lands differently than "150 samples."

---

## Stage 3: Purchase

### Key Actions

- Producer completes Gumroad checkout (one-time purchase, no subscription required)
- Receives download email with direct XPN link + link to MPCE_SETUP.md
- Downloads XPN file to computer
- Transfers XPN to MPC via USB or WiFi
- Imports XPN into MPC using File > Load program
- Loads first kit, plays first pad

### Key Emotions

- Relief: "Download email arrived instantly"
- Mild frustration: file management on MPC is not always intuitive
- Anticipation: "I'm about to hear what this actually sounds like on my pads"
- **The Aha Moment**: first pad hit sounds right — velocity sensitivity is there, round-robins cycle naturally, the engine character translates from the demo

### Friction Points

- **File management**: MPC file system navigation confuses new users. Where does the XPN go? What folder?
- **WiFi transfer failure**: MPC WiFi is unreliable; USB is more dependable but slower
- **Import error**: malformed XPN (wrong KeyTrack / RootNote / VelStart values) causes silent failures or ghost triggering
- **Missing Aha Moment**: if the pack sounds flat on first play, the journey ends here regardless of what comes next

### XO_OX Response

- **Download email**: sent within 60 seconds of purchase, plain-text subject line ("Your XO_OX pack is ready"), direct download link + a single bold link to setup guide. No upsell in the first email.
- **Getting Started card**: one-page PDF inside every XPN archive. Five steps with screenshots from MPC hardware (not computer). Step 3 explicitly says which folder.
- **XPN compliance**: all packs validated against three critical rules before release — `KeyTrack = True`, `RootNote = 0`, empty layer `VelStart = 0`. Tool suite enforces this automatically.
- **Aha Moment design**: every pack's Kit 01 is designed to land immediately. Full velocity range, most expressive round-robin group, no processing that muddies on first hit. The first kit earns trust for the other 11.
- **Support path**: MPCE_SETUP.md includes a troubleshooting section. Every known failure mode has a numbered fix. Email support address at the bottom.

---

## Stage 4: Retention

### Key Actions

- Producer uses the pack regularly in their workflow
- Receives v1.1 update notification (new kits, fixed bugs, expanded velocity layers)
- Receives first Patreon upsell email 14 days post-purchase
- Reads a Field Guide post that features the engine their pack came from
- Sees or participates in a community challenge using their engine

### Key Emotions

- Satisfaction: "I keep reaching for this pack"
- Delight: "They updated it and I get the new version free"
- Curiosity (Field Guide effect): "I didn't know the engine had this much depth — I want to go deeper"
- Belonging: challenge entries make them feel part of something, not just a customer

### Friction Points

- **Forgetting the pack exists**: no engagement beyond the purchase email = the pack gets buried in their library
- **No update path**: producers who don't know updates are free miss the goodwill from changelog emails
- **Patreon pitch feels transactional**: an upsell email too early or too blunt alienates the relationship that was just built
- **Field Guide posts feel disconnected**: if the post doesn't link back to the pack they own, the content doesn't reinforce the purchase

### XO_OX Response

- **Changelog email**: sent within 48 hours of any pack update (v1.1, v1.2, etc.). Subject: "Your [Pack Name] pack just got bigger — free update inside." Two lines of changelog, one download link. Nothing else.
- **14-day Patreon email**: the first retention email is not a pitch — it is a "what to try next" guide. What other kits in the pack haven't they explored? What Field Guide post goes deepest on this engine? Patreon is mentioned once, in the footer, with a single sentence: "If you want early access to every new pack, Patreon is where it happens."
- **Field Guide integration**: every pack page links to its companion Field Guide post. Every Field Guide post about an engine links to the pack. The loop is explicit.
- **Pack challenges**: each new pack launch includes a community challenge with a specific brief. Producers who already own the pack have a reason to open it again. New producers have a reason to buy.

---

## Stage 5: Advocacy

### Key Actions

- Producer tags XO_OX in a post sharing a beat made with the pack
- Recommends XO_OX in an r/mpc thread or YouTube comment
- Submits their own community pack for consideration
- Joins Discord and participates in the #made-with-xo-ox channel
- Refers a friend who purchases

### Key Emotions

- Pride: "I made this with something most producers haven't heard of yet"
- Identification: "XO_OX represents how I think about sound — it's not just gear, it's a philosophy"
- Generosity: they want other producers they respect to know about this
- Ownership: community members feel they helped build the thing, not just bought it

### Friction Points

- **No social tag to use**: if "made with XO_OX" isn't a known tag, posts don't aggregate and don't recruit
- **Community pack process is opaque**: interested producers don't know how to submit or what the standards are
- **Discord feels empty early**: a channel with no activity discourages first posts
- **No referral incentive**: word-of-mouth happens anyway, but a structured referral path (discount code, early access) amplifies it

### XO_OX Response

- **Social tag standard**: `#madewithxoox` — included in every pack README, every Gumroad listing footer, and every getting-started email. One tag, always the same.
- **Community pack pipeline**: [community_pack_submission_rnd.md](community_pack_submission_rnd.md) defines the full submission spec. The short version lives in a pinned Discord post: three requirements (XPN compliance, liner notes, audio demo) and a submission email.
- **Discord seeding**: Barry OB team posts weekly made-with drops from internal XO_OX productions before the community reaches critical mass. The channel has content before it has members.
- **Referral mechanic**: a Gumroad referral link in the "thank you" email. If a referred buyer purchases, the referrer gets a 15% credit toward their next pack. Low overhead, high goodwill.
- **Advocacy as identity**: the strongest advocacy tool is not a discount — it is the feeling that XO_OX represents something the producer believes in. Aquatic mythology, doctrine-driven sound design, character over feature count. Producers who identify with the philosophy recruit for free.

---

## Journey Summary Table

| Stage | Entry Emotion | Exit Emotion | Key XO_OX Asset | Primary Friction |
|-------|--------------|--------------|-----------------|-----------------|
| Discovery | Skeptical curiosity | "These people know MPC" | Field Guide + YouTube tutorials | Content must earn trust before earning clicks |
| Consideration | Hopeful anxiety | Informed confidence | Hero clips + compatibility table | No audio preview = no purchase |
| Purchase | Anticipation | Aha Moment delight | Getting Started card + Kit 01 design | File management on MPC hardware |
| Retention | Satisfaction | Curiosity + belonging | Changelog email + Field Guide integration | Pack gets buried without re-engagement |
| Advocacy | Pride + identification | Community ownership | `#madewithxoox` + Discord + referral link | No tag or pipeline = advocacy stays invisible |

---

## Open Questions for Future R&D

1. **Gumroad vs. direct storefront**: at what pack volume does it make sense to own the checkout experience and capture richer purchase data?
2. **Audio preview format**: 60-second hero clip vs. interactive web player (play individual kits, not just a produced demo)?
3. **MPC model fragmentation**: as Akai releases new hardware, how does the compatibility guarantee scale without per-device testing overhead?
4. **Email platform**: current email is manual / Gumroad native. At what list size does a proper ESP (Mailchimp, ConvertKit) become necessary for segmentation (by pack purchased, by MPC model)?
5. **Community pack quality gate**: who is the final human reviewer? What is the turnaround SLA for submissions?

---

*See also: [community_pack_submission_rnd.md](community_pack_submission_rnd.md), [community_marketplace_rnd.md](community_marketplace_rnd.md), [xpn-tools.md](../concepts/xpn-tools.md)*
