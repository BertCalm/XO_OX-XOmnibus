# Fab Five Sound Gala — OBRIX Library Evaluation
**Specialist**: F4 — The Sound Designer
**Date**: 2026-03-20
**Intensity**: Gala (full library audit, 340 presets)
**Previous session score**: F4 Sound = 0/10 (zero presets existed)
**Library state at evaluation**: 340 presets across 8 moods

---

## Executive Summary

Yesterday OBRIX had zero presets. Today it has 340. That is the headline. The raw number is genuinely impressive: OBRIX is now the largest single-engine library in XOlokun, and the coverage — 8 moods, recognizable categories within each, evocative names — reflects real curation effort rather than bulk generation.

The library works. It has a soul. It has a flagship (The Living Reef). It has Journey Mode. It uses FM. The coral reef mythology shows up in the naming with consistency. That said, a gala-intensity evaluation demands honesty about where the library still shows its rapid-assembly origins.

**Overall Sound Design Score: 7.2 / 10**
*(Previous session: 0/10 — score is not an improvement on the previous pass, it is a first real score)*

The 7.2 reflects a genuine library with real strengths and identifiable structural gaps. A 9.0 requires closing those gaps.

---

## Section 1: Mood Distribution

| Mood | Count | Share | Assessment |
|------|-------|-------|------------|
| Flux | 64 | 18.8% | Appropriate — OBRIX's aggression belongs here |
| Foundation | 57 | 16.8% | Appropriate — but quality is uneven (see below) |
| Aether | 55 | 16.2% | Strongest mood in the library |
| Prism | 54 | 15.9% | Well-populated, identity is clear |
| Atmosphere | 47 | 13.8% | Solid |
| Entangled | 25 | 7.4% | Purposeful, technically specialized |
| Family | 20 | 5.9% | Expected for cross-engine pairings |
| Submerged | 18 | **5.3%** | CRITICAL GAP — see Section 5 |

**Total: 340**

The distribution has a recognizable logic. Flux and Foundation carrying the most presets is correct — these are the entry moods, the first zones a new user will explore. Aether is where the Journey Mode and deep modulation presets live, and it earned its 55 presets.

The structural problem is Submerged. At 18 presets (5.3%), it is the only mood that feels unfurnished. Every other mood has enough variation that you could play it exclusively for an evening. Submerged does not have that density yet.

---

## Section 2: DNA Coverage Assessment

### Fleet-Wide DNA Averages (340 presets)

| Dimension | Average | High (>=0.7) | Mid (0.3-0.7) | Low (<=0.3) | Assessment |
|-----------|---------|--------------|----------------|-------------|------------|
| brightness | 0.46 | 22 (6.5%) | 264 (77.6%) | 54 (15.9%) | Midrange-heavy, sparse extremes |
| warmth | 0.49 | 33 (9.7%) | 258 (75.9%) | 49 (14.4%) | Midrange-heavy, but warmth is correct |
| movement | 0.39 | 38 (11.2%) | 163 (47.9%) | 139 (40.9%) | OBRIX is appropriately structural |
| density | 0.41 | 29 (8.5%) | 196 (57.6%) | 115 (33.8%) | Reasonable for a modular architecture |
| space | 0.31 | 35 (10.3%) | 105 (30.9%) | 200 (58.8%) | Heavy toward dry/close — expected |
| aggression | 0.25 | 23 (6.8%) | 85 (25.0%) | 232 (68.2%) | Heavily weighted toward calm |

### Reading the DNA Map

**Aggression is the most extreme skew.** 68% of presets score 0.3 or below. This is correct for a reef-themed engine — the ocean is more often vast and patient than aggressive. But the aggressive presets that do exist (Rust Grind, Concrete Slab, Iron Tide, Demolition Chord at 0.85-0.90) represent a genuinely underexplored corner of OBRIX's personality. The aggressive-AND-bright quadrant has only 7 presets total.

**Space is undersupplied at the extremes.** 200 presets (58.8%) are in the low-space range. This makes sense for Flux and Prism, but it means the engine's signature spatial capabilities (the `distance` + `air` parameters, the matched-Z atmosphere filters) are working at less than half capacity across the library. Only Aether and Atmosphere truly exploit the spatial depth.

**Movement is appropriately varied.** 40.9% in the low range is correct for an engine that includes basses, pads, and sustained textures — not every preset needs constant motion.

**Brightness and warmth are healthy bell curves.** No structural problem here; the averages near 0.5 reflect real variety.

### Corner Coverage Gaps

| DNA Corner | Presets | Status |
|------------|---------|--------|
| Aggressive + bright (agg>0.6, bright>0.6) | 7 | Thin |
| Vast + calm (space>0.7, agg<0.3) | 25 | Adequate |
| Warm + dense (warmth>0.7, density>0.7) | 1 | **Critical gap** |
| Cold + sparse (bright>0.6, space>0.6, density<0.4) | 2 | Very thin |
| High movement + high aggression | 5 | Thin |
| Submerged + distance > 0.5 | 4 | Very thin |

The **warm + dense** corner is the most alarming. One preset. OBRIX's brick stacking architecture is perfectly suited for thick, warm, layered sounds — a dual Saw through two LPs with proc2 feedback engaged is a warm-dense machine — but the library does not yet represent this.

### DNA Accuracy

The spatial DNA accuracy check passes cleanly: zero instances of high-space claims without matching distance/air values or reverb FX. This is good craft. Every preset that says it is spacious has the parameters to back it up.

---

## Section 3: Engine Utilization Metrics

### Core Feature Engagement (340 presets)

| Feature | Active | % | Assessment |
|---------|--------|---|------------|
| Dual source (src2 != 0) | 210 | 61.8% | Good |
| All 4 mod slots used | 170 | 50.0% | Acceptable, not great |
| Any FX used | 200 | 58.8% | Good baseline |
| No FX at all | 140 | 41.2% | High — many dry presets |
| Only 0-1 mod slots used | 81 | 23.8% | Concerning |
| FM active (fmDepth > 0) | 49 | 14.4% | **Underused** |
| Drift Bus active | 102 | 30.0% | Adequate |
| Journey Mode active | 23 | 6.8% | See Section 4 |
| Any macro at non-zero | 28 | 8.2% | **Critical gap** |
| Non-zero gesture type | 18 | 5.3% | Low |

### Source Type Distribution (src1)

| Type | Count | Assessment |
|------|-------|------------|
| Type 2 (Saw/primary) | 140 | Heavily dominant |
| Type 6 (Wavetable) | 48 | Good representation |
| Type 1 (Sine) | 52 | Appropriate |
| Type 8 (LoFi variants) | 20 | Could be more |
| Type 5 | 19 | Present |
| Type 4 | 24 | Present |
| Type 7 | 16 | Present |
| Type 3 | 15 | Present |
| Type 0 (unused/off) | 6 | Should be near zero |

Type 2 (Saw/primary) at 140 presets (41% of the library) means 4 in 10 presets share the same fundamental oscillator character. This is a strong signal: the library is Saw-forward in its timbre palette. This is not wrong, but it is a limitation. The LoFi banks (Type 8, only 20 presets) and the Wavetable modal variants deserve more representation.

### FM: Critically Underused

Only 14.4% of presets use FM despite it being one of OBRIX's most distinctive capabilities. FM in OBRIX is not DX7 FM — it is carrier-modulator brick FM with bipolar depth, a sound that ranges from subtle harmonic animation at low depths to aggressive clangorous metallic textures at high depths. The library needs a deliberate FM exploration pass.

### Macro Initial Positions: The Single Most Concerning Finding

**91.8% of presets have all four macros set to zero.**

The macros are CHARACTER, MOVEMENT, COUPLING, and SPACE — four knobs that represent OBRIX's primary real-time performance interface. When a user opens a preset and grabs CHARACTER at zero, nothing happens until they move the knob. Setting macros to useful non-zero starting positions is what gives a preset its first feeling of "being somewhere." It also communicates to the user what direction the macro will take them.

Only 28 presets set any macro to non-zero. Only 1 preset sets all four. This is a design philosophy choice that needs revisiting: macros at zero communicate "start here, go anywhere." But for a 340-preset library, some presets should say "you are already in the middle of the journey — here is where CHARACTER begins to unfold." The Entangled presets have the best macro usage (a few set COUPLING to 0.5-0.8), which makes sense. The pattern should spread.

### Gesture Type: Narrative Gap

Only 18 presets (5.3%) use non-zero gestureType. More troubling: in most of these, `flashTrigger` is still 0.0, which means the FLASH gesture mechanism exists but is not armed. Only two presets have flashTrigger > 0 (Flash Architecture at 0.80 and The Living Reef at 0.30). FLASH is one of OBRIX's most theatrical capabilities — the ability to trigger a surge-style event mid-note — and it exists in almost no presets.

---

## Section 4: Journey Mode Coverage

**23 presets use Journey Mode (6.8%)**

Journey Mode (note-off suppressed, infinite evolution) is the most distinctive temporal feature in OBRIX. It is what separates OBRIX from every Saw-through-filter synth in the world: the ability to release a key and have the sound continue growing, morphing, breathing.

**Journey Mode distribution by mood:**

| Mood | Journey Count | Out of |
|------|--------------|--------|
| Aether | 14 | 55 |
| Atmosphere | 3 | 47 |
| Foundation | 2 | 57 |
| Entangled | 2 | 25 |
| Family | 1 | 20 |
| Submerged | 1 | 18 |
| Flux | 0 | 64 |
| Prism | 0 | 54 |

Aether is the correct home for Journey Mode — it claims 61% of all Journey presets. The gap in Flux and Prism is meaningful: Flux's aggressive textures could produce compelling journey-style chaos (an industrial drone that evolves on its own) and Prism's crystalline quality is ideal for Journey Mode sustained chord transformations. Zero Journey presets in either is a missed opportunity.

The named Journey presets show genuine craft: Journey Mandala, Journey Infinite, Journey Pair, Journey Entangle, Drift Meditation, Infinite Reef, Feedback Drone. These names are strong. The presets themselves (spot-checked Jade Cathedral and Thermal Layer) are well-structured Journey Mode implementations — dual sources, long releases, genuine modulation arcs.

**Assessment**: Journey Mode coverage is acceptable in depth (quality is good) but needs breadth expansion, particularly into Flux and Prism.

---

## Section 5: Submerged Mood — Critical Gap

**18 presets, 5.3% of the library — the ocean floor is poorly furnished.**

The Submerged mood represents OBRIX's most unique capability: the distance and air spatial parameters are physically designed for "deep pressurized darkness." Ocean Floor, Abyssal Throne, Void Signal, Ghost Ship — the naming is there. But the execution reveals a gap:

**Submerged has 0% all-4-mods usage.** Not a single Submerged preset uses all four modulation slots. The maximum is two. This means the Submerged presets, designed for the most texturally complex mood, are paradoxically among the engine's least modulated sounds.

**Most Submerged presets do not use distance.** Distance > 0.5 appears in only 4 of 18 Submerged presets (Ocean Floor, Void Signal, Ghost Ship, Deep Memory). The signature spatial parameter of the engine is unused in 14 of its 18 deepest presets. This is a DNA accuracy concern: the mood is Submerged, but the spatial processing says "surface."

**Specific gaps in the Submerged library:**
- No "crushing pressure" preset that uses multiple processors with resonance and heavy distance/air
- No Journey Mode + Submerged combination beyond Ocean Floor (1 of 18)
- No FM + Submerged combination: zero of the 18 Submerged presets use FM
- Reese Machine and Pulse Bass feel like Flux presets that got misassigned — their DNA shows near-zero space

---

## Section 6: Signature Sound Audit

The five signature recipes designed before the library was built:

| Signature | Found | Status |
|-----------|-------|--------|
| Living Reef | YES — "The Living Reef" [Aether] | **Fully present and is the flagship** |
| Driftwood Bass | PARTIAL — "Driftwood" [Foundation], lesson preset | Concept exists, not a full recipe |
| Bioluminescent | PARTIAL — "Bioluminescent Bell" [Aether] + "Bioluminescent Flash" [Foundation lesson] | Exists as variants, not as the designed sine→BP recipe |
| Coral Construction | NO | Missing |
| FLASH Storm | NO | Missing — gestureType→SURGE concept is present but no dedicated storm preset |

**Assessment**: 1 of 5 signatures is fully realized (The Living Reef is genuinely excellent). Two others exist in partial or variant form. Two (Coral Construction, FLASH Storm) are missing entirely.

The Living Reef is the strongest argument for the library's quality: 11.5 feature-richness score, all systems active simultaneously (FM + Journey + all 3 FX + all 4 mods + aftertouch), the full OBRIX organism. It earns its flagship designation.

---

## Section 7: Quality Spot-Check

Sampled 5 presets across non-lesson moods:

**[Flux] Rust Grind**: Dual source active, but only 1/4 mods used, 0/3 FX, no FM, no drift, macros at zero. Gritty industrial texture that delivers its promise — but feels like a single-brick sketch rather than a full construction. The DNA (aggression 0.90) is accurate.

**[Prism] Tidal Pluck**: 2/4 mods, 3/3 FX — strongest spot-check result. All three FX slots carrying distinct processing. This is what a Prism preset should be. No FM, no drift, macros zero.

**[Atmosphere] VHS Pad**: 1/4 mods, 2/3 FX. The dual source is active (Lo-Fi + Sine), which is the right choice for VHS aesthetic. Light modulation is appropriate here. Acceptable.

**[Entangled] Spatial Knot**: 4/4 mods (best spot-check result), 1/3 FX. COUPLING macro set to 0.80 — good practice for an Entangled preset. Shows what macro usage looks like when done right.

**[Submerged] Deep Memory**: 1/4 mods only. This is the Submerged problem in miniature. The name and DNA are good (space 0.65, warmth 0.70, brightness 0.20), but a single mod slot means the sound does not evolve. Deep Memory with no evolution is just Deep.

---

## Section 8: Top 3 Presets in the Library

These three best exemplify what OBRIX is and can be:

### 1. The Living Reef [Aether]
The platonic ideal of an OBRIX preset. Every system active: FM (0.42), Journey Mode, all 3 FX (delay+chorus+reverb), all 4 mods (wavetable morphing, pan sweep, filter envelope, aftertouch dynamics), 3 active processors (LP formant whisper + Wavefold + BP selective resonance). DNA: movement 0.95, space 0.85, density 0.80. Wavetable (Harmonic) modulating Saw via FM. The description is worth reading in full. This preset does not demonstrate OBRIX — it *is* OBRIX.

### 2. Jade Cathedral [Aether]
Journey Mode + dual wavetable + 3 FX + Drift Bus. Space 0.85, warmth 0.65, movement 0.75. 4-second release. Dual wavetable sources tuned an octave apart (src2Tune=12), mixed equally, running through LP + double-LP processing. The description: "built by coral polyps over 10,000 years." Perfect tonal narrative. The best showcase of OBRIX's spatial processing.

### 3. Reef Cathedral [Aether]
All three FX slots active with distinct purpose (Chorus for width + Reverb for hall + Flanger for stone resonance). Space 0.95 — the highest space score in the library. distance=0.7, air=0.9 placing the listener deep in the interior. 0.4s attack ensures chords build rather than attack. Wavetable + Square source. Polyphony 4. The spatial DNA is impeccable and every parameter choice has a reason. The 9th chord of OBRIX presets.

---

## Section 9: Top 3 Recommendations

### Recommendation 1: Macro Position Pass — 300+ presets need non-zero starting positions

This is the highest-leverage single action available. 91.8% of presets (312 of 340) have all four macros at zero. This is not wrong per se, but it creates a flat, feature-identical start position across almost the entire library. A targeted pass setting macros to musically meaningful starting positions — CHARACTER at 0.25-0.5 for presets that are already somewhat processed, SPACE at 0.3-0.6 for presets that benefit from reverb breath, MOVEMENT at 0.2-0.4 for presets that should already be in gentle motion — would transform the user's first-touch experience across the whole library. Priority: immediately actionable, no new presets required.

### Recommendation 2: Submerged Deep Pass — 15 new presets with full modulation and spatial depth

The current 18 Submerged presets need reinforcement AND correction. A dedicated 15-preset pass should:
- Set distance >= 0.5 on every new Submerged preset (the ocean floor is far away)
- Use all 4 mod slots on at least 8 of the 15
- Implement Journey Mode on at least 5 of the 15
- Use FM on at least 4 (metallic pressure textures, bell+sub combinations)
- Fill the warm+dense corner (warmth > 0.7, density > 0.7) — currently 1 preset fleet-wide

Target names to fill: Trench Collapse, Pressure Gradient, Benthic Hum, Midnight Weight, Black Smoker, Manganese Floor, The Hadal Zone, Geological Time, Brine Pool Drone, Deep Convergence, Abyssal Plain, Hydrostatic, Weight of Water, Current Memory, Sediment Drift.

### Recommendation 3: Signature Recipe Completion — Coral Construction and FLASH Storm

Two of the five pre-designed signature recipes have not been realized. These are not arbitrary — they were conceived to represent distinct OBRIX capabilities:

**Coral Construction**: Pulse + detuned Saw, dual LP processing, proc1 and proc2 running in parallel. This recipe showcases OBRIX's split-path Constructive Collision at its most literal: two sources (Pulse and detuned Saw) given completely independent processing, meeting at the mix point as a collision of two architectural materials. 5-8 presets in Foundation and Prism. Naming: Coral Architecture, First Construction, Polyp Stack, Reef Framework, Building Block, Structural Reef, Calcium Scaffold.

**FLASH Storm**: Noise source, resonant BP filter, gestureType=SURGE with flashTrigger set to a non-zero value. This is the theatrical OBRIX — the reef experiencing a storm event. The FLASH gesture is the most underused capability in the library (18 presets with non-zero gestureType, but mostly flashTrigger=0). 5 presets in Flux and Atmosphere. Naming: Flash Storm, Surge Event, Electrical Reef, Spark Protocol, Storm Brick.

---

## Section 10: Library Health Summary

### Structural Strengths
- 340 presets is a substantial, credible library at launch
- Aether (55 presets) is the strongest mood — deep, coherent, Journey Mode-rich
- The naming language is consistent with the coral reef mythology
- DNA spatial accuracy is solid — high-space presets have the parameters to back the claim
- The Living Reef is a genuine flagship preset worth showing in any demo
- Source type variety is decent (9 types represented)
- Drift Bus usage (30%) is appropriate for the Berlin School ambitions

### Structural Weaknesses
- 91.8% of presets have all macros at zero — the library's biggest single quality gap
- FM used in only 14.4% of presets — OBRIX's most distinctive capability is underrepresented
- Submerged has 0% all-4-mods usage and mostly ignores the distance parameter
- Saw/Type 2 dominates at 41% of src1 — the library skews timbrally toward one source
- Warm+dense DNA corner has 1 preset — entire quadrant unexplored
- FLASH gesture mechanism is active in only 2 presets with armed trigger

### Score Projection

| Area | Current | After Recommendations |
|------|---------|----------------------|
| Mood coverage | 6.5 | 8.0 (Submerged pass) |
| DNA corner coverage | 6.0 | 8.0 (warm+dense, cold+sparse fills) |
| Feature utilization | 6.5 | 8.5 (macros, FM, FLASH pass) |
| Signature sounds | 6.0 | 8.5 (2 missing recipes built) |
| Journey Mode | 7.5 | 8.5 (Flux/Prism Journey expansion) |
| Flagship quality | 9.0 | 9.0 (The Living Reef is already excellent) |
| **Overall** | **7.2** | **8.8** |

---

## Appendix: Per-Mood Feature Utilization

| Mood | Count | DualSrc% | AllMods% | AnyFX% | Journey |
|------|-------|---------|---------|--------|---------|
| Flux | 64 | 62% | 39% | 55% | 0 |
| Foundation | 57 | 30% | 28% | 28% | 2 |
| Aether | 55 | 82% | 62% | 87% | 14 |
| Prism | 54 | 69% | 56% | 48% | 0 |
| Atmosphere | 47 | 72% | 62% | 96% | 3 |
| Entangled | 25 | 64% | 84% | 56% | 2 |
| Family | 20 | 30% | 75% | 30% | 1 |
| Submerged | 18 | 83% | 0% | 56% | 1 |

Atmosphere at 96% FX usage is the standout — nearly every Atmosphere preset is running at least one effect. This is correct: Atmosphere is defined by spatial character. Aether at 87% and 62% all-4-mods is the healthiest overall mood. Foundation at 28% FX and 28% all-mods reflects its educational purpose — these are building-block presets, deliberately simple.

The Submerged 0% all-4-mods at 83% dual-source tells a specific story: the presets have two sources, which gives them sonic body, but they are not using modulation to give those sources a life arc. They exist. They do not evolve.

---

*Report by F4 — The Sound Designer*
*Gala session: 2026-03-20*
*Context: 340 presets evaluated (previous session: 0)*
