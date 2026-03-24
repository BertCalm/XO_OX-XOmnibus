# Founder's Frequency -- Patreon Content Series

**Series type**: Free-tier weekly blog posts
**Platform**: Patreon (public / free tier)
**Cadence**: Weekly, every Friday
**Author**: XO_OX Designs

---

## Overview

Founder's Frequency is a free weekly series showcasing XO_OX's Founder's Signature presets. Each post explains the sound science (DSP/physics), usage tips, macro guides, and genre applications in an accessible but technically precise voice.

The series serves three goals:
1. **Teach** -- real synthesis education backed by actual DSP implementation details
2. **Demonstrate** -- show what XOlokun coupling can do that other synths cannot
3. **Distribute** -- the Founder's Signature presets are free, forever

---

## Season 1: Founder's Signature Vol. 1 (5 presets)

| Week | Preset | Engine(s) | Mood | Target Genre | Post File |
|------|--------|-----------|------|-------------|-----------|
| 1 | Coupled Bounce | ONSET + OVERBITE | Entangled | Trap / Hip-Hop | `week-1-coupled-bounce.md` |
| 2 | Belly to Bite | OVERBITE | Foundation | Bass / All genres | `week-2-belly-to-bite.md` |
| 3 | Coral Investigations | OBLONG | Atmosphere | Keys / Ambient | `week-3-coral-investigations.md` |
| 4 | Thermocline Dispatch | OVERDUB + ODYSSEY | Entangled | Dub / Ambient | `week-4-thermocline-dispatch.md` |
| 5 | Tidal Dialogue | ONSET + ODYSSEY | Entangled | Cinematic / World | `week-5-tidal-dialogue.md` |

### Scheduling Strategy

- **Week 1** (Coupled Bounce): Lead with the most viral preset. One-finger trap beat demos convert views to downloads.
- **Week 2** (Belly to Bite): Broadest appeal. Bass players, producers, film composers all find value.
- **Week 3** (Coral Investigations): The keyboard player's preset. Targets a different audience from Weeks 1-2.
- **Week 4** (Thermocline Dispatch): Deepest atmospheric content. Rewards readers who've followed the series.
- **Week 5** (Tidal Dialogue): The emotional closer. The most technically sophisticated coupling preset, saved for an audience that now understands what coupling means.

---

## Post Format

Each post follows a consistent structure (~1000-1500 words):

1. **Header**: Preset name, engine(s), mood, coupling type
2. **The Story**: Why this preset exists. Which Producer's Guild specialist asked for it. Personal and specific.
3. **The Science**: DSP/physics explanation. Accessible but real. Explains synthesis topology, algorithm choices, why the coupling interaction works at a physics level. Uses analogies to real instruments or physical phenomena.
4. **How to Use It**: 4-5 specific production scenarios with genre context, key signatures, macro positions, what to listen for.
5. **Macro Guide**: Table mapping M1-M4 to their functions with sweet spot recommendations.
6. **Pair It With**: 2-3 complementary XOlokun presets or engine combinations.
7. **The Download**: Link to Founder's Signature Vol. 1 XPN pack.

---

## Tone

Bold, warm, technically precise. Like a friend who happens to be a DSP engineer explaining why their favorite preset is special. Not academic, not marketing -- genuine enthusiasm backed by real knowledge.

Key principles:
- Use real DSP terminology but always explain it
- Reference actual implementation details (class names, algorithms, parameter values)
- Make analogies to physical phenomena (acoustic instruments, physics, nature)
- Name the Producer's Guild specialists by name -- make the requests personal
- Never condescend. Assume the reader is intelligent but may not know synthesis jargon.

---

## XPN Distribution

The Founder's Signature Vol. 1 pack is built using `Tools/xpn_bundle_builder.py`:

```bash
python3 Tools/xpn_bundle_builder.py build \
  --profile patreon/xpn-bundle-config.json \
  --output-dir /path/to/output
```

See `xpn-bundle-config.json` in this directory for the bundle configuration.

---

## Content Pipeline

### Upcoming seasons (planned, not yet written)

- **Season 2**: Founder's Signature Vol. 2 (5 presets, coupling-focused)
- **Season 3**: "How It Works" deep dives (engine architecture teardowns)
- **Season 4**: Producer's Guild Commissions (user-requested presets with design process)

### Production workflow

1. Design preset in XOlokun (save as .xometa)
2. Write blog post following the format template
3. Generate XPN pack using bundle builder
4. Publish post on Patreon (free tier)
5. Include XPN download link in post

---

## File Index

```
patreon/
  README.md                          -- this file
  xpn-bundle-config.json             -- XPN bundle builder configuration
  week-1-coupled-bounce.md           -- Week 1 post
  week-2-belly-to-bite.md            -- Week 2 post
  week-3-coral-investigations.md     -- Week 3 post
  week-4-thermocline-dispatch.md     -- Week 4 post
  week-5-tidal-dialogue.md           -- Week 5 post
```
