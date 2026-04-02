# XPN Educational Pack Series — R&D Brief
**Date**: 2026-03-16
**Status**: Concept / Pre-Production

---

## The Opportunity

No MPC-native tutorial pack studio exists. Native Instruments bundles education into Komplete's documentation. Splice attaches tutorials to samples but not to instruments. The gap: a producer loads a preset, gets a great sound, and learns nothing about why it works or how to push further. Educational XPN packs close that gap — each preset is a lesson, and the pack as a whole is a curriculum.

---

## Concept 1: "The Coupling Primer" Pack Series

**Format**: 3-pack progression sold together ($0 / $0 / $5 or bundled at $8)
**Presets**: 16 pads × 3 packs = 48 total XPN presets across the series
**Target audience**: Intermediate — producers who know XOceanus but haven't engaged coupling

The same 16 source sounds appear in all three packs. Pad labels and routing change; the source material does not. This makes the transformation audible and attributable.

- **Pack 1 — Uncoupled**: Pure engine output. No coupling routes active. Pad names are neutral ("Kick A", "Lead 1"). Establishes the baseline timbre.
- **Pack 2 — Lightly Coupled**: One coupling route per pad. Feed-forward only, modest amounts (10–30%). Pad names shift to describe the relationship ("Kick > Sub Feed", "Lead > Filter Breath"). Producers hear the first transformation.
- **Pack 3 — Deeply Coupled**: Full bidirectional coupling, feedback paths, cross-engine modulation. Pad names describe emergent behavior ("Kick Eating Sub", "Lead in Full Bloom"). Sounds are no longer recognizable as the originals.

Each pack ships with `MPCE_SETUP.md` — a plain-text file (placed alongside the XPN) explaining every coupling route used, the parameter values, and what the producer should listen for. The MD file is the textbook; the XPN is the lab.

**USP**: The only pack series where you can load three kits side by side and hear exactly what coupling does.
**Production effort**: High — requires 3 coordinated build passes and careful preset naming discipline.

---

## Concept 2: "Engine Deep Dive" Single-Engine Educational Packs

**Format**: One pack per engine, 20 presets in 4 sections of 5
**Pricing**: Free (loss leader; drives engine adoption and word-of-mouth)
**Target audience**: Beginner to intermediate — anyone new to a specific engine

The preset names carry the lesson. Section headers are baked into the naming convention:

| Section | Preset names (examples) |
|---|---|
| Foundation (01–05) | "01 Pure Voice", "02 No Effects", "03 Filter Only", "04 Amp Shape", "05 Engine Character" |
| Movement (06–10) | "06 LFO Enters", "07 Filter Breathing", "08 Pitch Drift On", "09 Envelope Wide", "10 Full Motion" |
| Coupled (11–15) | "11 First Feed", "12 OBESE Feeding Back", "13 Cross-Engine Whisper", "14 Feedback Loop", "15 Coupled Tightly" |
| Wild (16–20) | "16 Pushed Hard", "17 Chaos Invited", "18 Broken Beautifully", "19 No Rules", "20 Full Character" |

The producer loads preset 01 and works forward. Each step introduces exactly one new variable. By preset 20 they have heard the engine's full range in a structured sequence and can reverse-engineer any setting by stepping backward through the kit.

For ONSET specifically, the 8 synthesis voices (Kick/Snare/CHat/OHat/Clap/Tom/Perc/FX) map cleanly to sections — each voice family gets its own Foundation→Wild arc within the 20-preset limit.

**USP**: A preset pack that functions as an interactive manual. Zero documentation required because the preset names are the documentation.
**Production effort**: Medium per engine — naming discipline is the real labor, not DSP complexity.

---

## Concept 3: "The XO_OX Method" Flagship Educational Pack

**Format**: Premium bundle — 30 XPN presets + 20-page PDF liner notes + 3 video tutorial links
**Pricing**: $20–25
**Target audience**: Intermediate to advanced — producers ready to commit to a workflow

The 30 presets are designed as a complete curriculum in three movements:

- **Movement 1 — The Axis (presets 1–10)**: 5 pure feliX presets (OPENSKY, OHM, ORPHICA character) and 5 pure Oscar presets (OCEANDEEP, OVERBITE, OBESE character). The producer hears the poles before anything combines.
- **Movement 2 — Coupling Fundamentals (presets 11–20)**: 10 presets that pair one feliX engine with one Oscar engine at escalating coupling depths. Preset names track the depth ("11 feliX Touches Oscar", "15 Balanced Pull", "20 Full Entanglement").
- **Movement 3 — Beat Construction (presets 21–30)**: 10 presets across ONSET + two melodic engines, designed to be used together in one MPC project. The PDF liner notes include a beat-construction diagram showing which pads from which presets go together.

The PDF covers: feliX-Oscar axis theory, coupling parameter reference, the five coupling modes explained in plain language, and a "first beat" walkthrough tied directly to Movement 3 presets. Video tutorials are hosted on XO-OX.org (not YouTube-dependent) and referenced by QR code in the PDF.

**USP**: The first MPC-native curriculum that teaches sound design philosophy, not just sound design technique.
**Production effort**: Very high — the PDF and video production exceed the XPN production in labor.

---

## Pricing Model Recommendation

**Recommended structure**: Freemium with a premium flagship.

- Engine Deep Dives: Free. They serve existing engine owners and convert curious producers into buyers.
- Coupling Primer Pack 1–2: Free. Pack 3 at $5, or bundle all three at $8. The free packs create the context that makes Pack 3 worth buying.
- The XO_OX Method: $20–25 with no free tier. The PDF and video content justify the price; discounting it undercuts the perceived value of the education component.

The market precedent supports this: Splice tutorials are free but platform-locked. Komplete education is bundled into a $600 purchase. XO_OX can occupy the gap — standalone, MPC-native, fairly priced — and own that space before any competitor identifies it.

---

## Production Priority

1. Engine Deep Dive for OBESE — highest-traffic engine, lowest documentation burden, proves the format
2. Coupling Primer series — medium effort, high differentiation
3. The XO_OX Method — ship after site video infrastructure is in place
