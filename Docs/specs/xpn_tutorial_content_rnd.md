# XO_OX Tutorial & Educational Content Strategy — R&D Spec
**Date**: 2026-03-16
**Status**: Research & Design / Pre-Production

---

## The Problem

XOlokun has 34 engines, 2,550 presets, coupling mechanics, and a PlaySurface that takes 15 minutes
to explain. A new producer who loads their first XPN pack on an MPC Live has no map. They hear
something interesting, they have no idea how to push further, and they churn. Educational content is
not supplementary — it is a retention multiplier and a sales vector.

The goal is a layered system: passive (in-pack docs), active (written guides), visual (video), and
persistent (community). Each layer must work independently and reinforce the others.

---

## 1. YouTube Tutorial Video Series

### Format Principles

- No talking-head footage — screen recording, MPC hardware capture, audio waveform visualization
- Chapters always timestamped in description for skip navigation
- Every video ships with a free companion XPN download link in the description
- Thumbnail format: dark background, engine accent color block, 1-line hook text, no human faces

---

### Series A — "XO_OX in 5 Minutes"

**Target audience**: Total beginner. Has never loaded an XPN pack. May be unfamiliar with XOlokun entirely.
**Length**: 4–6 minutes. Hard cap at 6.
**Format**: Single continuous screen recording. No chapter breaks.

Script skeleton:
1. Download the free OBLONG starter pack (link pinned)
2. Copy to MPC storage via USB or Wi-Fi share
3. Load project → navigate to Programs → import XPN
4. First 4 pads: hit them, show Q-Link layout on screen, show one Q-Link sweep
5. Tease coupling: "These pads are alive — one affects another"
6. Close on XO-OX.org / pack catalog link

**Hook**: "No installation wizard. No DAW required. 5 minutes and you're making sound."

Release: First video in the series. Publish before any other video exists.

---

### Series B — "Deep Dive" (One Per Engine)

**Target audience**: XOlokun user who wants to understand a specific engine.
**Length**: 10–15 minutes.
**Chapters**: 5 standard chapters per episode.

| Chapter | Content | Approx length |
|---------|---------|--------------|
| 0:00 — Identity | Engine character, aquatic creature, accent color story | 90 sec |
| 1:30 — Signal Flow | Oscillators/sources → filter → effects, narrated in plain language | 2–3 min |
| 4:00 — Key Parameters | 3 parameters that define the engine's character. Q-Link assignments shown. | 3 min |
| 7:00 — feliX-Oscar Axis | Where this engine sits. Pure feliX vs. pure Oscar vs. hybrid. Audible demo. | 2 min |
| 9:00 — Coupling Demo | One live routing example: this engine as source, one target engine | 2 min |

Priority order for first 8 episodes (by install base and uniqueness):
1. OBLONG (most accessible, gateway engine)
2. OBESE (extreme character — highest wow factor)
3. ONSET (drums — widest appeal outside synth users)
4. OPAL (granular — strong search volume)
5. OVERWORLD (era system is a visual story)
6. ORACLE (GENDY stochastic — most technically interesting)
7. OVERDUB (dub workflow — producer community alignment)
8. OVERBITE (BITE character — meme-able)

**Release cadence**: 1 Deep Dive per month, alternating with Series C and D.

---

### Series C — "MPC + XO_OX Workflow"

**Target audience**: Beat makers. MPC-native producers who want to integrate XOlokun into a full
production workflow, not just play presets.
**Length**: 20–25 minutes.
**Episodes**: Planned 4-episode arc.

| Episode | Title | Core workflow shown |
|---------|-------|---------------------|
| C1 | Drums First | Build a full drum kit with ONSET XPN, Q-Link MACHINE macro, sequence in MPC step sequencer |
| C2 | Chords and Leads | OBLONG pad + OVERWORLD lead. Melodic side-chain coupling demo. |
| C3 | Bass and Texture | OBESE bass via XPN keygroup + OPAL ambient texture. MPC mixer routing. |
| C4 | Full Track Build | Combine drums, chords, bass, texture from C1–C3 into a full 4-bar loop, mix, bounce |

Each episode starts where the previous one ended. A viewer who watches all 4 has made a complete track.

---

### Series D — "Pack Design Secrets"

**Target audience**: Patreon tier 2+ subscribers. Aspiring sound designers. People who want to
understand how XO_OX presets are built, not just consumed.
**Length**: 15–20 minutes.
**Format**: Behind-the-scenes. Screen share of XOlokun parameter window + real-time audio.

Episode concepts (3-episode arc):

- **D1 — Sonic DNA**: How the 6D DNA (brightness, warmth, movement, density, space, aggression)
  shapes every preset decision. Walk through 3 presets with different DNA profiles and explain
  each parameter choice.
- **D2 — Coupling as Composition**: How bidirectional coupling routes create emergent behavior.
  Build one coupled preset from scratch, live, with narration.
- **D3 — Character Over Features**: Why dry patches must sound compelling. The feliX-Oscar
  spectrum as a design constraint. Walk through a "boring preset" → "character preset" transformation.

**Placement**: Patreon exclusive first, YouTube public 30 days later.

---

### Release Cadence

One video per month, minimum. Suggested rotation:
- Month 1: Series A (5-minute intro) — launch anchor
- Month 2: Series B Deep Dive #1 (OBLONG)
- Month 3: Series C Episode 1 (Drums First)
- Month 4: Series B Deep Dive #2 (OBESE)
- Month 5: Series D Episode 1 (Sonic DNA) — Patreon drop, then public
- Month 6: Series B Deep Dive #3 (ONSET) — timed with ONSET pack launch

Rule: every video ships the same week as a pack release when possible. The video drives pack sales;
the pack drives video replays.

---

## 2. Written Getting-Started Guide (XO-OX.org)

**Location**: xo-ox.org/guide (currently hosting the Field Guide — expand this section)
**Format**: Single-page scrollable guide with anchor links, mobile-optimized, dark ocean theme

### Section 1 — Installing a Pack on MPC

Step-by-step hardware import flow with annotated screenshots:
1. Download the .xpn ZIP from XO-OX.org or Patreon
2. Unzip — you get a folder: `PackName/Programs/`, `PackName/Samples/`, `PackName/README.md`
3. Copy the folder to MPC internal storage: `/Internal Storage/Expansion Packs/XO_OX/`
4. On MPC: Menu → Programs → Load → navigate to the XPN file
5. Confirm sample paths — MPC will prompt if samples are in a non-default location
6. Save as a Project for instant recall

Hardware-specific notes: MPC Live II, MPC X, MPC One covered separately. MPC Renaissance / Studio
(legacy): note USB drive method only, no Wi-Fi.

---

### Section 2 — First Sounds

- Playing pad programs: velocity expression, how to use the 16 pads as a timbral map
- Q-Links: the 4 knobs and what they do (always labeled in the XPN README)
- Adjusting Q-Link range: MPC long-press to enter Q-Link assign mode
- Note: Q-Link defaults are set by XO_OX design intentionally — start there before reassigning

---

### Section 3 — The feliX-Oscar Axis

Plain-language explanation of the polarity system, written for a producer who has never heard of it:

- feliX engines: bright, additive, melodic, sociable (neon tetra energy)
- Oscar engines: dark, subtractive, abrasive, solitary (axolotl regeneration energy)
- Most engines are hybrids — OBLONG is slightly feliX, OBESE is strongly Oscar, ORACLE is deeply Oscar
- In a pack: feliX presets fill the top of the mix; Oscar presets own the low-mid and sub

This section links to the full Aquarium page (xo-ox.org/aquarium) for the depth-zone atlas.

---

### Section 4 — MPCE_SETUP.md in Practice

What the file is, why it ships with every pack, and how to use it:
- Quad-corner pad assignments (pads 1/4/13/16 = corners of the 4×4 grid)
- XY morphing: hold two corner pads simultaneously → intermediate timbre
- The 4 macro zones and what they do in a typical pack
- Note: not all packs use all 4 corners — check the file before assuming

---

### Section 5 — FAQ

Common installation problems and solutions. Minimum 8 entries:

| Problem | Solution |
|---------|---------|
| "Sample not found" on load | Move Samples/ folder to the exact path in the XPN header; check README |
| Q-Links do nothing | Confirm you're in the correct Program; check Q-Link assign mode |
| Pack sounds too quiet | Check MIDI note velocity; XO_OX presets use full velocity range by design |
| XPN won't import | Confirm MPC OS version ≥ 3.4; older OS may not support all XPN features |
| No audio on MPC standalone | Check output routing in main project settings |
| Samples sound pitched wrong | Set RootNote to 0 (C-2) in the Program editor — matches XO_OX export default |
| "Program not found" after OS update | Re-import XPN from the original folder; paths reset on OS update |
| Can I use these in a DAW? | Yes — load MPC as plugin or use MPC Desktop software |

---

## 3. In-Pack Documentation

Every XPN pack ships with 3 files alongside the .xpn program files:

### README.md

Template fields (mandatory for every pack):
```
# [Pack Name] — XO_OX Pack

**Engine**: [Engine short name]
**Version**: [x.x]
**Programs**: [count] programs

## Programs List
[Program name] — [1-line description]
...

## Q-Link Layout (all programs)
Q1: [parameter]
Q2: [parameter]
Q3: [parameter]
Q4: [parameter]

## feliX-Oscar Balance
[Short sentence on where this pack sits on the axis]

## Coupling Notes
[If the pack uses coupling: which engines are pre-routed, what types]
```

---

### MPCE_SETUP.md

Template fields (mandatory):
```
# [Pack Name] — MPCE Quad-Corner Setup

## Corner Assignments
Pad 1 (bottom-left):  [character of this corner]
Pad 4 (bottom-right): [character]
Pad 13 (top-left):    [character]
Pad 16 (top-right):   [character]

## XY Morphing Guide
Holding pads 1+4: [what intermediate timbre you get]
Holding pads 1+13: [description]
Holding pads 4+16: [description]
Holding pads 13+16: [description]

## Macro Zones
[Description of what each quadrant of the 4×4 grid covers]
```

---

### PACK_NOTES.txt

Free-form sound designer notes. Plain text (not Markdown) for maximum MPC compatibility.
Minimum 150 words. Should cover:
- What inspired the pack character
- One surprising preset and how to get the most from it
- A recommended pairing: "this pack works best alongside [other XO_OX pack]"
- One parameter to explore beyond the Q-Link defaults

---

## 4. Interactive XOlokun Guide (XO-OX.org/guide)

### Per-Engine Audio Examples

Each engine card on the guide page includes:
- 1 audio example (10–15 seconds, MP3, autoplay on hover)
- The example must demonstrate the engine at its most characteristic — not a neutral demo
- Clip naming convention: `{engine-short-name}_guide_demo_v1.mp3`

Recording spec: render at 44.1kHz / 24-bit, normalize to -1 dBFS, fade out last 0.5 seconds.
Target: 34 clips total (one per registered engine). Priority: the 8 Deep Dive video engines first.

---

### "Find Your Engine" Quiz

4-question interactive quiz. Results recommend a starting engine.

Question 1 — Sonic texture preference:
- "Warm and harmonic" → feliX leaning
- "Gritty and abrasive" → Oscar leaning
- "Evolving and unpredictable" → chaotic/generative engines

Question 2 — Production role:
- "Melody and leads" → OBLONG, OVERWORLD, ODYSSEY
- "Bass and sub" → OBESE, OVERBITE, OCEANDEEP (when available)
- "Texture and atmosphere" → OPAL, ORACLE, OUROBOROS
- "Drums and percussion" → ONSET

Question 3 — Complexity tolerance:
- "I want to play presets, not program" → OBLONG, OBESE
- "I want to tweak but not rebuild" → OVERWORLD, OPAL
- "I want to go deep" → ORGANON, ORACLE, OUROBOROS

Question 4 — feliX-Oscar instinct:
- Show the feliX/Oscar creature images, ask which one feels right intuitively

Results: Show recommended engine card, link to its Deep Dive video, link to its free starter pack.

---

### Field Guide Integration

Each engine's guide entry links to the corresponding Field Guide post (if published).
The Field Guide serves as the deep companion — the guide page is the index, the Field Guide is
the encyclopedia. 14 posts currently published; remaining 16 planned posts fill the gaps as engines
ship.

---

## 5. Community Knowledge Base

### Discord Architecture

Primary channel: **#mpc-tips**

Pinned threads (maintained by XO_OX, updated monthly):
1. "First Pack Setup — step by step" (links to written guide section 1)
2. "Q-Link Mastery — workflows producers use" (community-contributed)
3. "Coupling Explained — starter routes that work" (links to educational pack series)
4. "MPC OS Compatibility Chart" (updated when new MPC OS drops)
5. "Request a Tutorial" (community upvotes determine next video topic)

### "Made with XO_OX" Thread

Community-contributed session screenshots and audio clips. Format:
- Screenshot of MPC sequence or waveform
- Pack/engine used
- One tip about what made it work

Moderation rule: no promotion, no links to personal stores in this thread. Pure sharing.

Incentive: monthly "Made with XO_OX" feature on the XO-OX.org updates feed. Selected
contributors get a free pack.

---

## 6. Onboarding Email Sequence

Trigger: any pack purchase on XO-OX.org (or Patreon subscription confirmation).

| Day | Subject line | Core content |
|-----|-------------|-------------|
| Day 0 | "Your [Pack Name] is ready" | Download link, 2-sentence pack intro, link to getting-started guide |
| Day 3 | "3 things to try in your first hour" | Link to written guide sections 2–3; embed the 5-minute YouTube video |
| Day 7 | "What other producers are making" | 2 "Made with XO_OX" community examples; soft Patreon upsell: "Want the behind-the-scenes?" |
| Day 14 | "Go deeper: the Field Guide" | Link to the most relevant Field Guide post for the engine they purchased |
| Day 30 | "Ready for the next one?" | 15% discount code for next pack; link to pack catalog |

Email design rules:
- Plain text layout with one image (pack cover art at the top)
- No more than 3 links per email
- All emails link to Discord at the footer: "Join 400+ producers at discord.gg/xoox"
- Unsubscribe must work in one click (legal requirement, also brand courtesy)

Day 7 Patreon upsell copy model: "Pack Design Secrets videos go to Patreon supporters first.
If you want to see how these presets were built — the DNA decisions, the coupling routes,
the intentional dead ends — that's the Patreon."

---

## Open Questions

1. **Audio example recording**: Who records the 34 engine clips? Needs a consistent signal chain
   and room. Define spec before production starts.

2. **Quiz implementation**: Static JavaScript on the guide page, or a Typeform/Tally embed?
   Static is faster and doesn't require a third-party account.

3. **Video localization**: English-first. Spanish and Portuguese subtitles in year 2 — both are
   large MPC user communities in Latin America and Brazil.

4. **Email platform**: Mailchimp, ConvertKit, or Beehiiv? Decision gates the Day 0 automation.
   ConvertKit favored for creator-native tagging by pack purchased.

5. **Deep Dive backlog pacing**: 34 engines at 1 per month = 34 months. Priority the top 12
   (most popular engines + newest releases) for the first year.

---

## Success Metrics

| Metric | 6-month target |
|--------|---------------|
| YouTube subscribers | 1,000 |
| Average view duration (Deep Dive) | ≥ 55% (8+ min of 15) |
| Email Day 0 open rate | ≥ 55% |
| Email Day 7 click rate (Patreon link) | ≥ 8% |
| Discord #mpc-tips active monthly | ≥ 50 contributors |
| "Find your engine" quiz completions | ≥ 200/month at steady state |
