# XO_OX XPN Pack Launch Playbook

**R&D Doc — Repeatable launch sequence for every pack release**
Last updated: 2026-03-16

---

## Pre-Launch (T-14 Days)

### Content to Prepare

**Demo video** (post T-7, publish T-1 or launch day):
- Length: 90 seconds max. First 8 seconds must hook — play a finished loop, not an intro card.
- Show: 3–4 pads played live, one filter sweep or macro turn, one full 4-bar phrase. No talking required.
- Format: vertical (9:16) for Reels/TikTok primary cut; export 16:9 for YouTube/Discord secondary.
- Audio: export at -14 LUFS for streaming headroom. No watermark — trust is the conversion strategy.

**Social teaser cadence (T-14 → T-1):**
- T-14: one-liner announcement. "Something dropping in two weeks. MPC heads, stay close."
- T-7: 15-second audio clip (not video). Raw loop, no caption needed.
- T-3: cover art reveal with pack name and a one-sentence character description.
- T-1: full demo video drops everywhere simultaneously.

**Patreon early access post (T-7):**
Template — subject: `[EARLY ACCESS] [Pack Name] — yours before the public`
Body: 2 sentences on what the pack is, direct download link, one question ("What would you build with this?"). Keep it short. Patrons get 48 hours before public release.

**Email list (T-3):**
One email. Subject line formula: `[Pack Name] drops [day]. Here's 30 seconds of it.` Link to audio clip. No paragraph prose — bullet the key facts: price, format (XPN), compatible gear (MPC Live/X/One), download location.

### Export Build Checklist

Run in order before any beta copy leaves the machine:

1. `xpn_validator` — confirm all pad assignments, program references, and note maps are intact.
2. Stems checker — verify every kit voice has a rendered stem at the correct pitch and no clipping (peak < -1 dBFS).
3. Completeness check — all 16 pads populated, no silent placeholder pads in the shipped program.
4. Cover art — 1400×1400 px minimum, exported as JPEG at 85% quality, embedded in bundle root.
5. Final ZIP integrity check — unzip on a clean directory and load into MPC software before sending to beta.

### Beta Tester Outreach (T-10 → T-7)

Send to 3–5 people: at least one MPC Live II user, one MPC One user, one producer who works in a genre outside your primary target. One beta slot reserved for a Patreon tier-3+ member rotating monthly.

Feedback to gather — three questions only:
1. Did everything load without errors?
2. Which 3 pads did you reach for first, and why?
3. Anything that felt out of place or off-level?

Turnaround window: 72 hours. Beta feedback closes T-4.

---

## Launch Day

**Release sequence:**
1. Patreon post goes live at 9 AM (your local time). Download link active.
2. Email list send at 10 AM same day.
3. 48-hour Patreon window. Public release posts go out at T+48h.

**Where to list (public release):**
- XO-OX.org pack page — primary canonical listing with full description and audio demo embed.
- Gumroad — set price, enable "pay what you want" floor at base price. Link from XO-OX.org.
- MPC-Forums (Samples & Sounds subforum) — post with cover art, 2-sentence description, direct link. No walls of text.
- Reddit r/mpc — short post, cover art image, one-line description, link. Reply to every comment in the first 6 hours.
- Bandcamp — optional, use for packs with strong aesthetic/mood identity that benefit from the discovery ecosystem.

**Platform-specific post formats:**

- **Instagram**: Single image (cover art) + 30-second Reel. Caption: pack name, 3 hashtags max (`#MPC`, `#samplepacks`, `#[engine name]`), link in bio.
- **Twitter/X**: Cover art + demo video clip. Two tweets: announcement tweet, then a reply with the direct link. Threading keeps the link out of algo penalty.
- **TikTok**: Vertical video only. Show hands on MPC hardware if possible. No caption needed — the sound is the pitch. Post at 6–8 PM local time for best reach.

**Discord announcement template:**
```
@here [PACK NAME] is out now.

[One sentence: what it is and what engine/character it draws from.]

XO-OX.org → Packs → [Pack Name]
Patreon members already have it — thank you for the early listen.

[Cover art attached]
```
Post in: XO_OX community #releases channel, relevant MPC community servers (#samples or #resources channel).

---

## Post-Launch (T+1 → T+14)

### Metrics to Track

- Downloads in first 48 hours (Patreon vs public split).
- Gumroad conversion rate on "pay what you want" — what percentage pay above floor.
- Patreon tier upgrade count in the 7-day window after launch.
- Reddit/forum engagement: upvotes, replies, DMs asking follow-up questions.
- One qualitative sweep at T+7: read every comment. Note the words people use to describe the pack — these feed the next pack's copywriting.

### Patch Releases (v1.0.1)

If a level issue or corrupt pad is reported post-launch:

1. Fix the specific file only. Do not re-render the entire pack unless necessary.
2. Increment filename to `[PackName]_v101.xpn` — do not replace the original silently.
3. Post a single update note in the original forum/Reddit thread: "v1.0.1 — fixed [specific pad] level. Same link, updated file." One sentence. No apology tour.
4. Email list gets one brief update only if the bug was severe (silent pad, crash on load).

### Cross-Sell Timing

- At T+7 post-launch, reply to your own launch posts with: "If you're into [Pack Name], [Related Pack] pairs well — [one sentence why]."
- On XO-OX.org pack page, add a "Pairs with" section linking 1–2 related packs by engine coupling logic (e.g., OPAL→DUB, OVERWORLD→OPAL).
- Avoid cross-selling at launch — let the new pack breathe for 48 hours before referencing anything else.

---

## Cadence and Seasonal Timing

**Sustainable release rate: 1–2 packs per month.** Two per month requires two complete build pipelines running in parallel. One per month is the quality-preserving default.

**Best months for drops:**
- **January**: "New year new sounds" energy. High producer motivation, low competition from label releases.
- **March/April**: NAMM afterglow, producers coming off winter studio sessions with new hardware.
- **September**: Back-to-work energy, producers re-engaging after summer.
- **November (first week)**: Before holiday noise, rides Black Friday cultural momentum without requiring a discount.

**Avoid**: late December (attention fragmented), late August (slowest engagement month historically for niche audio).

**Between-release momentum (weeks with no pack drop):**
- Alternate weekly between: one tutorial post (how to use a specific engine feature) and one sound design tip (short-form, platform-agnostic).
- Field Guide post cadence: one long-form post per pack cycle reinforces the engine's mythology and keeps the site indexable.
- One "work in progress" audio clip mid-cycle — no branding, no call to action. Presence without sell.
