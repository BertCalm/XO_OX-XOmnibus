# GARDEN Quad — Visionary Depth Pass
*The Visionary | March 2026 | Pre-DSP Design*

---

## Before We Begin: What the Overview Gets Right (and What It Stops Short Of)

The overview says: *Coupling behavior: Evolutionary — when Garden engines couple, they grow into each other over time.*

That sentence is correct and completely insufficient. "Grow into each other over time" is a description of the effect. It says nothing about the *mechanism*, the *memory*, the *asymmetry*, or the *irreversibility*. A vibrato rate changing over 8 bars is also "growing over time." That's not what the Garden quad is.

The Garden quad is about **state that remembers itself**. A plant that has grown does not un-grow when the sun sets. A string section that has been playing for 20 minutes in a warm hall is not the same instrument it was at minute zero. The Garden coupling mode must be **stateful and non-reversible** within a session — or it is just an LFO with a long wavelength.

This document pushes past the concept into the territory where DSP design decisions must be made.

---

## LEVEL 1: Evolutionary Coupling — What Accumulation Actually Means

### The Problem with Naive "Evolution"

Most "evolving" synthesizers are really just slowly changing LFOs. The parameter moves. It arrives somewhere. If you stop playing, it drifts back. Nothing is learned. Nothing is retained.

True evolutionary coupling requires **accumulators** — internal state variables that are written by the player's behavior, read back by the synthesis engine, and **never automatically reset**. Three dimensions of accumulation matter:

### Dimension 1: Warmth Accumulation (The Heat Sink)

A string section playing in a warm hall changes physically. Catgut and horsehair are temperature-sensitive. Rosin softens. Players' bow arms fatigue and adapt. The ensemble sound after 45 minutes is physically different from minute one — warmer, more blended, less individual variation audible.

In the GARDEN quad, this becomes a **warmth accumulator** `W` — a leaky integrator that increases every time a note is played and decays toward a floor (not zero) during silence:

```
dW/dt = +k_play × velocity × note_count     (during note-on)
dW/dt = -k_decay × (W - W_floor)            (during silence)
```

W_floor > 0. The floor rises slightly each session — long sessions have a higher baseline warmth than short ones.

W maps to:
- Increased high-frequency roll-off in the ensemble blend (rosin softens → less scratch, more core tone)
- Reduced inter-voice phase variance (players "lock in" after extended playing)
- Longer, smoother vibrato attack times (warm muscles → more natural vibrato onset)
- Reduced bow transient harshness on attack (warm rosin → smoother string grip)

The user doesn't hear "warmth parameter changing." They notice, after 20 minutes of playing XOrchard, that the strings have a different quality — softer, more interior, like leaning into a velvet wall. If they start a new session, it's gone. That is the point.

### Dimension 2: Aggression Accumulation (The Stress Tensor)

Plants under stress — drought, wind, predation — produce different chemistry. More tannins, thicker cell walls, altered growth patterns. A solo violin player under emotional duress plays differently: higher bow pressure, faster vibrato, wider pitch deviations. The sound carries the history of what it's been through.

Aggression accumulation `A` tracks playing intensity:

```
dA/dt = +k_vel × (velocity - threshold)²   (when velocity > threshold)
dA/dt = +k_fast × (tempo_density - normal) (when notes come faster than normal)
dA/dt = -k_rest × (elapsed_silence)^0.5    (slower decay — stress lingers)
```

A maps to:
- Increased bow pressure character (more scratch, more transient)
- Wider vibrato depth and irregularity
- More inter-voice detuning in XOrchard (players rushing slightly)
- In XOvergrow: audible bow changes — sul ponticello territory emerges naturally at high A
- In XOsier: chamber quartet tension — voices drift slightly apart, then correct, then drift again

The asymmetry of A's decay (`elapsed_silence^0.5` — slow at first, faster later) is deliberate. Stress doesn't immediately vanish when you stop. The quintet that just played a difficult passage doesn't relax in 2 seconds. They need 30 seconds of silence before their bodies let go.

High A for extended periods (>5 minutes sustained aggression) unlocks a **bleaching** state — strings go brittle, scratchy, raw. You've been pushing them too hard. This is a feature, not a bug.

### Dimension 3: Dormancy (The Winter State)

Plants don't die in winter. They enter dormancy — reduced metabolic activity, preserved energy, waiting. A string section that hasn't played in several minutes has gone cold, stiff, slightly out of tune.

Dormancy `D` accumulates during silence and decays during playing:

```
dD/dt = +k_silence × elapsed_silence  (simple linear accumulation during rest)
dD/dt = -k_play × note_count          (any playing reduces dormancy)
```

D maps to:
- Higher initial bow noise on attack (stiff rosin, cold strings)
- Slightly wider initial pitch variance (strings going slightly out of tune during rest)
- Slower vibrato onset — players need time to "warm in"
- In XOxalis: synth string pads lose some of their shimmer coherence, need to "boot up"

Critically: D affects the *first few seconds* after a silence. The system is responsive — even one note dramatically reduces D. But after 3+ minutes of silence, that first chord back will have a character that was absent during continuous playing. It will sound like coming back to the room after leaving.

### The Interaction Between Accumulators

W, A, and D are not independent. Their combination produces emergent states:

| State | Condition | Sound Character |
|-------|-----------|-----------------|
| Lush | High W, low A, low D | Deep, warm, blended — the ensemble at peak form |
| Heated | High W, high A, low D | Warm but strained — the 45-minute passage before intermission |
| Raw | Low W, high A, low D | Harsh, exposed, over-played — the bleach state |
| Reawakening | Low W, low A, high D | Cold, slightly rough, finding itself — first bars after silence |
| Dormant | Low W, low A, high D extreme | Still, almost motionless — the garden before spring |
| Peaked | High W, low A, medium D | The single most beautiful state — short rest after long warm session |

The **Peaked** state is significant. It only occurs after you've played enough to accumulate W, then taken a meaningful rest (reducing D to mid-range) without raising A. It is the sound of a string section after intermission — warm from the first half, rested, at their most supple. Most players will never notice this state exists. The ones who do will treasure it.

---

## LEVEL 2: Cross-Domain Botanical — Where Biology Becomes DSP

### Phototropism: Growing Toward the Signal

Phototropism is the mechanism by which plants bend toward light sources. Auxin — a growth hormone — accumulates on the shaded side of a stem and causes those cells to elongate more than the illuminated side. The plant bends toward light not because it "wants" to but because of differential growth in response to a gradient.

In the GARDEN quad, **the light source is audio input**. The engines practice acoustic phototropism — they bend toward the most present signal in their coupling context.

Mechanistically: when XOvergrow (solo strings) is coupled with XOrchard (orchestral), XOvergrow's solo voice gradually increases its intonation alignment toward XOrchard's ensemble pitch center. Not unison — alignment. The solo violin doesn't lock to the section, but it stops being quite as free. It bends toward the warmth.

The rate of bending is the phototropism rate `P_rate`. It is slow. 30 seconds to significant audible alignment. Two minutes to near-full alignment. And it is directional — the smaller voice (fewer instruments, lower amplitude) bends toward the larger, just as a sapling bends toward a forest canopy.

DSP implication: `P_rate` controls a frequency-domain leaky attractor. XOvergrow's pitch fundamental drifts toward XOrchard's pitch center at rate P_rate. The drift is applied post-oscillator, pre-filter — so it sounds like a human adjusting intonation, not a pitch shifter engaging.

**Heliotropism vs. Phototropism distinction**: Heliotropism follows a moving source (sunflowers following the sun). Phototropism responds to a fixed gradient. The GARDEN coupling mode should support both:
- Phototropism: align toward a stable coupled signal
- Heliotropism: track a moving coupled signal (one engine following another's pitch modulation in real-time, with a configurable lag)

### Mycorrhizal Networks: Underground Communication

The "Wood Wide Web" — mycorrhizal fungi connecting root systems of trees across a forest floor — allows trees to share nutrients, chemical signals, and distress signals underground. A tree under pest attack releases chemical signals that neighboring trees pick up through the fungal network and respond to by increasing their own defensive chemistry. The communication is slow, chemical, invisible, and real.

This maps to **cross-engine state propagation** in the GARDEN quad — specifically, the propagation of the **A accumulator (stress)** between engines through the coupling matrix.

When XOvergrow (wild, raw) is under high stress (A approaching bleach state), the mycorrhizal channel transmits a fraction of that stress signal to XOrchard (orchestral). XOrchard doesn't go into the same bleach state — it's more robust, more ensemble-averaged — but it does respond. Bow pressure character increases slightly. The ensemble loses some of its polish. It's as if the orchestral section can *hear* the solo violinist under stress and responds without being told to.

The mycorrhizal network parameters:
- **Network conductance** `M_c`: how much stress propagates between coupled engines (0 = isolated, 1 = full network)
- **Network delay** `M_delay`: how long before the propagated signal arrives (seconds — this is underground communication, not wireless)
- **Response asymmetry**: XOrchard receives stress from XOvergrow and responds at 60% amplitude. XOvergrow receives resource/warmth from XOrchard at 80% amplitude. The forest feeds the sapling; the sapling signals danger.

The sound of high mycorrhizal conductance: the coupled engines *breathe together* during stress states, *settle together* during warmth. The quartet and the soloist are not just playing in the same space. They are sharing something.

### Seasonal Cycles: Timbral States as Phenological Stages

Plants have four distinct physiological states across a year: spring (growth, rapid change, high energy), summer (peak expression, maximum resource, stable), autumn (harvesting, withdrawal, color change), winter (dormancy, conservation, waiting).

The GARDEN quad implements **Session Seasons** — not calendar-based, but accumulated play-time-based. A session has its own seasonal cycle:

| Season | Trigger | String Character |
|--------|---------|-----------------|
| **Spring** | Session begins | Reawakening — slightly uneven vibrato, bow finding grip, pitch settling. Beautiful imperfection. The garden coming back to life. |
| **Summer** | 15-20 min of sustained play, high W, moderate A | Peak expression — lush, warm, even, deep. The string section at their most fully realized. Every voice contributing. Maximum blend. |
| **Autumn** | High A over extended time, or W reaching saturation plateau | Rich but beginning to show strain — deeper tones dominating, some upper-register roll-off. The harvest — concentrated, complex, not quite as bright. |
| **Winter** | Long silence after extended session, or deliberate user trigger | Dormant — slow, cold, interior. Strings retreat into their lower registers. Vibrato slows to near-stillness. The garden waiting. |

Session seasons are not clocks. They respond to how you play. An aggressive 5-minute session can push directly from Spring to Autumn, skipping Summer entirely. A gentle, patient hour of playing might sustain Summer for its entire length. Playing through a Winter brings Spring faster on the return than a first Spring — the garden remembers it has been here before.

**The seasonal display** (UI): A small circular indicator in the Garden quad showing the current season — not a percentage, not a number. An icon: seedling / full leaf / turning leaf / bare branch. No values. Players learn to read the garden without quantifying it.

---

## LEVEL 3: Strings That Are Grown, Not Played — The Radical Premise

### The Paradigm Shift

Every string synthesizer in history is built on the same model: **you play, it sounds**. Trigger in, audio out. The note-on is a command. The string has no choice.

The GARDEN quad's third and deepest layer inverts this. In **Growth Mode**, a note-on is not a command — it is a **seed**. The harmonic content of the resulting sound develops over time like a germinating plant, not like a struck or bowed instrument.

What does this mean for a musician used to performing?

It means you are no longer performing. You are **tending**.

### Germination: The Harmonic Seed

In Growth Mode, a note-on plants a seed. The seed contains:
- **Root pitch** (the fundamental frequency from the MIDI note)
- **Genetic material** (the current state of W, A, and D at moment of planting)
- **Soil condition** (the current seasonal state)
- **Planting depth** (velocity — but reinterpreted as how deep the roots go, not how loud the attack is)

The germination process unfolds over 5-30 seconds (configurable):

**Phase 1: Germination (0-2 seconds)**
Nearly nothing is audible. A sub-harmonic — barely a root, more a presence — begins. This is the seed under soil. The player can feel this is happening even if they can't quite hear it.

**Phase 2: Emerging (2-8 seconds)**
The fundamental becomes audible. First one harmonic partial appears — the octave, then the fifth, then the third. They don't arrive together. They emerge sequentially, each with its own timing envelope, like leaves unfolding in sequence. The sound is sparse, crystalline, growing.

**Phase 3: Vegetative (8-20 seconds)**
Rapid growth. Harmonics multiply. The sound fills out. Vibrato develops — not from an LFO, but from the W and A state accumulated at planting. A high-A seed grows a nervous, tight vibrato. A high-W seed grows a warm, settled one. This phase sounds like strings finding their voice.

**Phase 4: Flowering (20-30 seconds, or triggered by expression)**
Peak harmonic content. The sound is "in bloom" — full, present, as rich as it will become. This is where you get the maximum from the note. Expression pedal at this phase = the gardener tending in full bloom. Subtle pressure changes. Water and sun.

**Phase 5: Seed-setting / Decay**
Note-off triggers not an immediate release but a **seed-setting phase** — the flower closes, the fruit develops, energy concentrates downward as harmonics peel away in reverse order (treble partials first, bass last). The sound moves from full to interior. Then the bass fundamental fades. The garden stores what it learned from this note in the W, A, and D accumulators.

### Tending: What Expression Controls Do in Growth Mode

In Growth Mode, expression controls are not patch parameters — they are **garden interventions**:

**Mod wheel = Sunlight**
High mod wheel in Phase 2-3 (emerging/vegetative) accelerates growth — harmonics develop faster, the flowering phase arrives sooner. Too much sun too early (high mod wheel in Phase 1) burns the seedling — the germination stutters and produces a thinner, more fragile sound. Moderate, consistent light in Phase 3-4 produces maximum bloom.

**Aftertouch = Water**
Water during flowering (Phase 4) extends the flower's duration before seed-setting. High aftertouch after note-off slows the decay — the fruit holds on the vine longer before dropping. No aftertouch = the flower sets seed quickly.

**Expression pedal = Shade/Pruning**
Pulling the expression pedal during Phase 3 or 4 doesn't reduce volume — it *prunes* specific harmonic branches. The sound becomes more directed, less lush, more architectural. Like a bonsai artist removing growth to reveal structure. The notes that survive pruning are stronger.

**Pitch bend = Wind**
Pitch bend in Growth Mode doesn't transpose — it *stresses* the plant. Low pitch bend (downward) applies weight, bending the harmonics toward the earth — the sound becomes darker, lower, more root-centric. Upward pitch bend stretches the plant toward light — harmonics brighten and shift upward. Extreme pitch bend tears the structure — controlled harmonic distortion, the bow striking too close to the bridge.

### The Polyharmonic Garden

When multiple seeds are playing simultaneously in Growth Mode, the garden becomes polyharmonic — multiple plants sharing resources and space.

**Competition**: Two seeds of similar pitch planted close together compete. The stronger one (higher initial velocity / better soil conditions / higher W at planting) takes more of the harmonic space. The weaker one grows in its shadow — quieter, thinner, turned slightly toward whatever light it can find.

**Cooperation**: Two seeds of complementary pitch (fifth apart, octave apart) cooperate. The larger (lower) root feeds the smaller — the mycorrhizal mechanism. Both grow more fully together than they would alone. An interval of a fifth in Growth Mode sounds like two plants that have co-evolved.

**Overcrowding**: More than 4 simultaneous seeds without sustaining expression creates stress — the A accumulator rises for the whole garden. The sound becomes complex and slightly strained. Not broken — strained. Like a bed that has too many things planted in it.

### The Garden as Temporal Instrument

Growth Mode reframes the relationship between musician and time.

In conventional synthesis: **time serves the musician**. The note lasts as long as you hold it.

In Growth Mode: **the musician negotiates with time**. The note has its own timing — its own germination rate, its own flowering duration. You can accelerate (sun), extend (water), prune (expression), stress (bend). You cannot simply command it.

This is a different instrument category than any existing synthesizer. It is closer to a **living system** than a playback mechanism.

The DSP question this raises: **Growth Mode must be opt-in**. There is a legitimate version of XOrchard, XOvergrow, XOsier, and XOxalis that functions as a conventional high-quality strings synthesizer with evolutionary state in the background. Growth Mode is the deep end — for players willing to learn a different relationship with time.

---

## Per-Engine Deep Specifics in Growth Mode

### XOrchard (Orchestral) — The Managed Forest

Growth Mode for XOrchard is **plantation management**. You are not planting individual seeds — you are managing a section.

Each chord voicing plants a **cohort** of seeds simultaneously. The 60-voice orchestral section germinates as a unit — but not identically. Individual voices within the cohort have slight timing offsets (derived from W and A). The result is a section that blooms over 15-25 seconds, voices arriving in waves rather than simultaneously.

Unique to XOrchard Growth Mode: the **conductor parameter** — one voice (the concertmaster) influences the others. The highest note in a voicing is the concertmaster. The rest of the section times its germination to match. If the concertmaster's seed is planted deep (high velocity), the entire section grows more slowly, more deliberately.

The concept of **sectional blend** enters Growth Mode: during Phase 3 (vegetative), XOrchard's coupling matrix applies the mycorrhizal network across its own internal voices, not just across engines. The 60 voices share stress, warmth, and dormancy signals internally. This is why an orchestral section sounds like one organism, not 60 individuals.

### XOvergrow (Wild Garden / Solo Strings) — The Weed

XOvergrow in Growth Mode is **the weed that grows wherever it finds space**.

A single seed. No section management. No conductor. Just one voice finding its way.

What makes XOvergrow's Growth Mode distinctive: it **responds to silence** more than to playing. The moments between notes are where XOvergrow develops. Plant a note, let it flower, stop playing — and during the silence, the harmonic tail of that note continues to change, evolve, and eventually transform into something new. The silence is part of the growth.

XOvergrow also produces **runners** — like strawberry plants sending out horizontal shoots. A high-A note in Growth Mode can trigger a sympathetic resonance in the low register: a quiet, unpredictable sub-harmonic that wasn't planted, that the player didn't ask for, that emerged from the stress. It might appear 8 seconds after the triggering note. It is uncontrollable by design.

The solo strings paradigm in Growth Mode is about **accepting what the plant decides to do**. You tend it. You don't command it.

### XOsier (Chamber Strings / Herb Garden) — The Kitchen Garden

XOsier in Growth Mode is **the herb garden** — intentional, cultivated, intertwined. Four voices. A quartet.

In Growth Mode, XOsier's four voices are not independent seeds. They are **companion plants** — grown together, their root systems deliberately intertwined.

The companion planting relationships:
- Violin I and Violin II: **symbiotic** — each accelerates the other's harmonic development. Planting them close (small interval) produces mutual acceleration. Planting them far (wide interval) produces productive independence.
- Viola: **stabilizer** — the viola's growth rate determines how fast or slow the entire quartet's seasonal state moves. A viola planted with low velocity (shallow root) slows the quartet. High velocity = driving the group forward.
- Cello: **nutrient provider** — the cello's harmonic richness (determined by W at planting) feeds the upper voices. A well-nurtured cello (high W) makes the upper voices bloom more fully. A cold, stressed cello produces a leaner quartet.

The herb garden metaphor holds for the recipe cookbook side too: each XOsier preset is a specific herb blend. Rosemary + thyme + sage have companion planting relationships that are as real as their sonic relationships.

### XOxalis (Geometric Garden / Synth Strings) — The Succulent

XOxalis is the geometric plant — clover patterns, fractal leaf structures, mathematical growth. In Growth Mode, XOxalis germinates according to **mathematical phyllotaxis** rather than organic growth.

Phyllotaxis is the mathematical arrangement of leaves around a stem — governed by the golden angle (137.5°). Each new leaf emerges at 137.5° from the previous one, creating spiral patterns that maximize sunlight exposure. This is not random. It is optimal.

XOxalis's Growth Mode applies phyllotaxis to harmonic emergence: each new partial appears at an interval derived from the golden ratio (φ = 1.618...) from the previous partial. The resulting harmonic sequence is:

`fundamental → fundamental × φ → fundamental × φ² → fundamental × φ³ → ...`

This produces a harmonic series that is *related to* but *distinct from* the natural harmonic series. It sounds close to strings — but wrong in an interesting way. Too even. Too mathematical. Like a string instrument built by a mathematician rather than a luthier.

XOxalis's Growth Mode is not warm. It is precise. The geometric garden grows according to plan.

The synth strings paradigm: XOxalis is the only GARDEN engine where Growth Mode produces a sound that could be described as *inhuman* — not because it's harsh, but because it's too orderly. That is its character. The Solina and JP-8 pad never existed in nature. XOxalis's Growth Mode produces the string sound that mathematics would design.

---

## Evolutionary Coupling Between the Four — The Full Garden

When all four GARDEN engines are coupled (the full quad), the evolutionary coupling reaches its most complex expression:

**The Succession Sequence**

Botanical succession describes how an ecosystem develops over time after a disturbance. Pioneer species (hardy, fast-growing, undemanding) colonize first. Intermediate species follow as conditions improve. Climax species (slow-growing, resource-demanding, stable) arrive last and define the mature ecosystem.

In the full GARDEN quad, the succession sequence is:

1. **XOxalis** is the pioneer — the synth strings that can grow in any condition. First to respond to a note-on. Fast germination. Synthetic precision.
2. **XOvergrow** is the intermediate — solo strings finding space once XOxalis has established. More variable, more alive, a little slower.
3. **XOsier** is the later intermediate — chamber strings emerging as conditions stabilize. The quartet needs warmth (W > threshold) before it sounds fully itself.
4. **XOrchard** is the climax species — orchestral strings that require established conditions to bloom fully. Slow to arrive, most resource-demanding, most stable once established.

In a full-quad session that begins with all four engines active: XOxalis sounds first (geometric, synthetic, present). XOvergrow emerges within the first minute. XOsier takes 3-5 minutes to fully establish. XOrchard sounds like XOrchard — the real, deep, lush orchestral string — only after sustained playing in the right conditions.

This is the ecological argument for the GARDEN quad's design: **the best sound requires patience and cultivation**. You cannot force the climax ecosystem. You grow into it.

**Cross-Succession Mycorrhizal Network**

In the full quad, the mycorrhizal network connects all four engines:
- XOxalis feeds XOvergrow (pioneer supports intermediate)
- XOvergrow feeds XOsier (intermediate supports chamber)
- XOsier supports XOrchard (chamber enables orchestral)
- XOrchard feeds back to XOxalis (climax species stabilize the whole ecosystem)

The feedback loop is the key: the mature ecosystem sustains the pioneer. The orchestral strings, once achieved, make the synthetic strings more beautiful too. Everything becomes more.

---

## DSP Design Constraints Surfaced by This Analysis

The following are non-negotiable requirements arising from the visionary pass:

1. **Accumulators are session-persistent until explicit reset**: W, A, and D must survive note-on/off cycles. They only reset on session end or explicit user action.

2. **No automatic return to zero**: The floor of W is above zero. Dormancy D has a maximum. The accumulators represent real physical states that don't vanish between notes.

3. **Mycorrhizal delay must be perceptible**: The delay between stress in one engine and response in another must be long enough to be interesting — minimum 2 seconds, default 8 seconds. Instant propagation defeats the ecology.

4. **Growth Mode note-ons must not produce immediate full sound**: The germination phase (Phase 1, near-silence for 0-2 seconds) is non-negotiable. Players who cannot tolerate this are using Growth Mode incorrectly.

5. **The seasonal display is never a number**: No W=73%, A=12%, D=4% readouts. The icon display only. The goal is intuition, not control.

6. **Expression controls in Growth Mode must be distinct from Standard Mode**: Mod wheel = sun in Growth Mode, not standard modulation. Pitch bend = wind, not pitch shift. These remappings must be clearly documented and cleanly switchable.

7. **Competition between simultaneous seeds must be audible but not destructive**: Two competing seeds should sound different from two cooperating seeds. The competition is not distortion — it is *restraint*. One voice pulling back slightly as another takes space.

8. **Growth Mode is per-engine, not quad-wide**: Each engine can independently be in Growth Mode or Standard Mode. A session with XOrchard in Growth Mode and XOvergrow in Standard Mode is valid. The coupling still applies in either configuration.

---

## What This Changes About the Recipe Cookbook

The Recipe Design Process (recipe-design-process.md) maps cooking techniques to synthesis. For the GARDEN quad, the mapping goes deeper than any other quad because the synthesis IS cooking in time, not just an analogy.

The science bridge for GARDEN cookbook recipes is not "bowing a string is like sautéing vegetables." The bridge is:

*"The way a starter culture transforms flour and water into sourdough over 48 hours — irreversibly, statefully, with memory of its environment — is the same way the GARDEN quad's accumulators transform a note-playing session into a living ecosystem. You cannot undo a 48-hour ferment. You cannot undo 45 minutes of warming strings."*

This means GARDEN recipes must be **slow food**. Not slow to cook necessarily, but **slow in philosophy**. The bread that takes three days. The ferment that needs a week. The garden that requires a season.

The GARDEN recipe table:

| Engine | Garden Zone | Recipe Philosophy | Example Dishes |
|--------|-------------|------------------|---------------|
| XOrchard | Orchard | Long cultivation — fruit that needed years to mature | Calvados (apple spirits, years of aging), Tarte Tatin (cooked orchard fruit, patience), Kimchi (fermented cabbage, time-transformed) |
| XOvergrow | Wild Garden | Foraged — found in the world, not planted | Elderflower cordial (gathered wild, seasonal), Nettle soup (aggressive harvest of "weeds"), Wild garlic pesto (impossible to grow, must find) |
| XOsier | Herb Garden | Companion planting — multiple flavors in deliberate relationship | Persillade (parsley + garlic in union), Bouquet garni (herbs tied together to infuse together), Herb butter (individually present, collectively transformed) |
| XOxalis | Geometric Garden | Mathematical precision — recipes with exact ratios | Macarons (French patisserie, exact weights non-negotiable), Tempering chocolate (precise temperature physics), Japanese tamago sando (engineered egg softness) |

The seasonal recipe rotation for XOrchard: three recipes, one per season — spring (new growth, delicate), summer (peak fruit, rich), autumn (harvest, complex, preserved). The winter recipe is an absence — a note in the cookbook that says: *"Winter in the orchard: no recipe. Walk among the dormant trees. Return in spring."*

---

## What the Visionary Doesn't Know Yet

Three unresolved questions that require the Architect's assessment before DSP design:

**Q1: Accumulator persistence across sessions.** Should W survive a plugin restart? If yes, the garden has memory across days — a long-cultivated session lives inside the patch. If no, every session starts from Spring. Both are defensible. This is a product decision with real UX implications.

**Q2: Growth Mode and live performance.** Growth Mode was conceived for studio use — patient cultivation, exploratory tending. Can it work live? A performer who needs a sound immediately and reliably cannot wait 20 seconds for a note to germinate. The answer might be: Growth Mode has a **Live setting** that accelerates all phases by 10x (2-second full bloom instead of 20), sacrifices some of the nuance, but enables the philosophy in performance contexts.

**Q3: The visual language for Season display.** The proposal is icons only: seedling / full leaf / turning leaf / bare branch. But there's a deeper question: should the UI *show* the season at all? A garden doesn't have a readout telling you what season it is. You look at the garden and know. Maybe the UI shows nothing — and the engine's behavior teaches the player to read the season from the sound itself. This would be the most radical position. Worth the Architect's input.

---

*End of Visionary Pass — GARDEN Quad*
*Push this to the Architect for DSP feasibility and cost assessment.*
*Do not commit.*
