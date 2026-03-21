# KITCHEN QUAD SEANCE — Full Verdict

*Four-engine simultaneous evaluation | March 2026*
*Channelled by: Moog, Buchla, Kakehashi*
*Fleet average at evaluation: ~8.8/10*

---

## SEANCE CONDITIONS

The council sat in four rooms simultaneously. Moog occupied the warm end: cast iron and copper, the instruments of the kitchen as analog control. Buchla took the prepared stone — all inharmonics and deliberate chaos. Kakehashi sat with the delicate end: glass, porcelain, the instruments that break. All three ghosts agreed: this quad is more cohesive than any previous family in the fleet, and more musically ambitious. But ambition has a price.

---

## ENGINE #1 — XOVEN | "The Cast Iron Concert Grand"

*Parameter prefix: `oven_` | 27 parameters | 10 presets | Accent: Cast Iron Black #2C2C2C*

---

### Ghost Council — Channel 3

**Robert Moog:** "The cast iron metaphor is the most correct physical analogy I have seen in a software synthesizer. Fletcher & Rossing Table 6.2, Audsley spectral envelopes, Hunt-Crossley contact model — these are real citations doing real work. The Bloom envelope is a stroke of genius. Thermal mass as artistic decision. The micro-rebound at 20-35ms is exactly right for a massive resonant body. I would have shipped this. The MOVEMENT macro is weak — it boosts LFO depth but you could go further. This machine deserves to breathe more."

**Don Buchla:** "The inharmonicity of cast iron's modal ratios — 2.005, 3.018, 4.042 — is correct and audible. I appreciate that the density-to-Q mapping spans 150-600, a useful dynamic range. The sympathetic network is physically argued: shared post-mix, 12 strings, damper pedal engaged. My concern is the competition and couplingResonance parameters. They are wired — I see the fastTanh saturation and the sympathetic drive multiplier — but they are stub behaviors. They do something, but not enough to justify two parameter slots. These need to become real Kitchen-quad coupling mechanisms or be eliminated."

**Ikutaro Kakehashi:** "Players will love this. The Bloom time gives them an expressive dimension no other piano synthesizer provides. The SilenceGate at 500ms is correct — piano tails are long. Ten presets is a thin library for an engine this capable. Midnight Grand, Forged Bright, Iron Cathedral are three strong statements. The others (Bourguignon, Sunken Steinway) show range. But there are holes: nothing exploring the mid-register darkness, nothing percussive-drone, nothing truly slow and geological. The preset library is 6/10 even if the engine is 9/10."

---

### Scoring

| Dimension | Score | Notes |
|-----------|-------|-------|
| Sound Quality | 9.0 | Physically grounded modal synthesis; Audsley envelope is audibly correct; micro-rebound is subtle genius |
| Innovation | 8.5 | Cast iron as piano material is genuinely novel; Bloom envelope is original; micro-rebound differentiates from any commercial piano synth |
| Playability | 8.5 | Velocity→hardness→contact time is transparent; damper pedal sympathetic toggle is essential; Bloom is immediately expressive |
| Preset Coverage | 7.0 | 10 presets documented, all valid; but thin — missing mid-register character, percussive extremes, geological-slow types |
| DSP Efficiency | 9.0 | 3.2% CPU budget rigorously maintained; dynamic mode pruning verified; competition params are light stubs not heavy computation |
| Musical Utility | 8.5 | Grand piano territory but darker, more character; sympathetic network makes chords special; will work across genres |
| Identity / Character | 9.5 | The strongest conceptual identity in the quad; cast iron is unmistakable; "Maillard char" on hard strikes is a real thing |
| **Overall** | **8.7** | Fleet-average equivalent; ceiling is 9.0+ if preset library and MOVEMENT macro are deepened |

---

### Top 3 Concerns

1. **MOVEMENT macro is weak.** It boosts LFO depths (lfo1 +0.3, lfo2 +0.2) but that is it. MOVEMENT on a cast iron piano should also modulate the bloom time, or gently animate the thermal drift. The macro does something but does not deliver the expressive character its name implies.

2. **competition and couplingResonance are D004 borderline.** `paramCompetition` drives a mild fastTanh saturation on the output mix. `paramCouplingRes` adds 0-50% to sympathetic drive when coupled. Both are wired, both produce audible results — but they are clearly stubs designed for Kitchen-quad V2 behavior. A player turning "competition" to max gets output compression, not the adversarial character the name implies. This is a conceptual mismatch.

3. **LFO2 targets body resonance (Q) but the effect is subtle to the point of inaudibility at default depths.** LFO2 depth defaults to 0.0, and the Q modulation formula `modeQ *= (1.0f + lfo2Val * 0.2f)` gives only ±20% Q variation at full depth. At normal playing depths this is below perceptual threshold. LFO2 needs either a stronger target (sympathetic depth, density) or the modulation range needs to triple.

---

### Top 3 Enhancement Suggestions

1. **Deepen the MOVEMENT macro.** Wire it to: LFO depths (current), + `bloomTime` (+0.3 max), + thermal drift rate (speeds up the 6-second cycle to 2-second cycle at max). Let MOVEMENT make the iron feel like it is expanding and contracting thermally.

2. **Commit to Kitchen-quad coupling or remove stubs.** Either implement the adversarial competition mechanic (XOven's massive impedance suppresses weaker-bodied engines when both are sounding — a proper physics-grounded coupling) or rename `competition` to `driveCharacter` and make it a pure waveshaping parameter that earns its slot. The stub behavior is honest but the naming is a lie.

3. **Add 5-7 presets hitting gaps.** Priority: (a) a dark mid-register ballad preset that sits below 1kHz brightness, (b) a fast-decay percussive preset with zero bloom (iron as percussion), (c) a slow geological preset that takes 30 seconds to fully bloom. These are the moods the current 10 miss.

---

### Blessing Candidates

- **B_OVEN_01 — Bloom Envelope as Physical Metaphor:** The only piano synthesizer where the instrument has to thermally warm up before it fully speaks. The Bloom parameter is not an attack knob — it is a material truth about cast iron. Unanimous ghost approval.
- **B_OVEN_02 — Maillard Char Noise Model:** Noise content in hammer excitation mapped as culinary transformation (char on a sear). The cooking metaphor is doing DSP work, not just marketing. Buchla specifically called this "honest to its physics."

---

### Priority Fixes

| Priority | Fix |
|----------|-----|
| P0 | None — all parameters wired, D001-D006 compliant |
| P1 | MOVEMENT macro: expand to include bloomTime and thermal drift rate animation |
| P1 | LFO2: increase Q modulation range from ±20% to ±60%, or switch target to sympathetic depth |
| P2 | competition/couplingResonance: commit to proper Kitchen-quad coupling behavior or rename for honesty |
| P2 | Preset library: add 5 presets covering dark-mid, percussive-zero-bloom, geological-slow |

---

---

## ENGINE #2 — XOCHRE | "The Copper Upright"

*Parameter prefix: `ochre_` | 23 parameters | ~11 presets | Accent: Copper Patina #B87333*

---

### Ghost Council — Channel 3

**Robert Moog:** "Copper over cast iron as a material choice is brilliant because the difference is audible in the physics: higher thermal conductivity = faster response = copper Upright is MORE IMMEDIATE than the iron Grand. The impedance relationship (33.6 MRayl vs 36.7 MRayl) is small but the damping character is very different. The Caramel Saturation is the most interesting synthesis idea in the quad — waveshaping as culinary transformation, sweetening under pressure. But LFO2 is dead. I see it in the code: `(void) lfo2Val; // Reserved for future V2 features`. A dead LFO on a declared parameter is a D004 violation. This must be fixed before release."

**Don Buchla:** "The Three Body Types (Practice Room / Parlour / Studio) implemented as three sets of fixed modal frequencies — 3 resonators at chosen Hz — is a reasonable simplification. The values chosen (150/420/950 Hz for Practice Room, 120/350/780 for Parlour) are reasonable approximations. But the body switcher is an integer param that changes character dramatically, and there is no interpolation or crossfade. Switching body type mid-session produces a discontinuity. For a live performance context this is acceptable. For a modulation target, it is not. Do not expose this to the mod matrix."

**Ikutaro Kakehashi:** "The copper Upright has the best macro architecture in the quad. CHARACTER→conductivity+brightness+hardness, MOVEMENT→caramel, COUPLING→sympathetic, SPACE→body depth. This is a complete, self-consistent expressive system. Except LFO2 does nothing. For a player, that is a broken promise. The second LFO appears in every engine in the fleet. Players will turn it on, hear nothing, and assume the engine is broken. Fix this before launch. Wire it to caramel amount or body depth — either will produce audible, musical results."

---

### Scoring

| Dimension | Score | Notes |
|-----------|-------|-------|
| Sound Quality | 8.5 | Modal bank correctly characterized for copper; caramel saturation adds warmth; body types work; shorter decay than XOven is distinctive |
| Innovation | 8.0 | Caramel Saturation as synthesis mechanic is genuinely new; copper-as-upright is a good material argument; thermal drift at 0.3s is faster and more expressive than XOven's 4s |
| Playability | 8.5 | Immediate response characteristic (shorter contact time) is accurate and feel-able; conductivity as "responsiveness knob" is intuitive |
| Preset Coverage | 7.5 | 11 presets (including one quarantined); good spread across body types; Caramel Glaze and Sugar Work explore the saturation mechanic well |
| DSP Efficiency | 8.5 | Similar budget to XOven but smaller (~3.3% CPU); body resonator adds 3 modes at fixed positions, negligible cost |
| Musical Utility | 8.5 | Practice room / parlour / studio differentiation is genuinely useful for matching production contexts |
| Identity / Character | 8.0 | Copper Upright is distinct from the iron Grand in character; caramel mechanic is unique; but the identity is slightly less singular than XOven or XOpaline |
| **Overall** | **8.2** | Below fleet average; LFO2 dead is a D004 violation that prevents a clean seance pass |

---

### Top 3 Concerns

1. **LFO2 is dead — D004 violation.** `(void) lfo2Val;` in the render loop, line 727. LFO2 is declared, exposed, included in preset JSON, and does absolutely nothing. This is a broken promise to every player who reaches for the second LFO. This is the only hard P0 in the quad.

2. **Sympathetic resonance is scaled at 0.04 (line 354) vs. XOven's shared sympathetic network which does more work.** The Ochre sympathetic network at maximum `amount=1.0` contributes `sum * 1.0 * 0.04f`, where sum is the sum of 12 resonator outputs. This is very quiet — a whisper of sympathetic resonance. Upright pianos do have less sympathetic ring than grands, so the physics argument is sound, but musically this means the Sympathy parameter does almost nothing audible at practical settings. The scaling needs to be 0.08-0.12 to be musically meaningful.

3. **No architecture documentation.** XOven has `oven-architecture.md`, ObeliskEngine has `obelisk-architecture.md`, OpalineEngine has `opaline-architecture.md`. Ochre has no architecture doc. For a quad engine this creates documentation inconsistency and makes it harder to maintain and explain.

---

### Top 3 Enhancement Suggestions

1. **Wire LFO2 immediately.** Best candidates: (a) LFO2 → caramel saturation amount (musical, creates breathing warmth), or (b) LFO2 → body depth (makes the intimate body character animate). Either requires two lines of code. This is the highest-leverage fix in the entire quad.

2. **Raise sympathetic scaling from 0.04 to 0.08.** Double the contribution and the feature becomes audible without breaking the "less than grand" physics argument (which would be violated around 0.3+). At 0.08, the sympathetic strings will be perceptible and rewarding.

3. **Write the Ochre architecture doc.** Follow the format of `oven-architecture.md`. Add the missing doc before Kitchen Collection launch so the quad is complete. Include the caramel saturation mechanic explanation — it is the engine's best selling point and deserves its own section.

---

### Blessing Candidates

- **B_OCHRE_01 — Caramel Saturation:** Waveshaping as transformation not distortion — velocity drives gentle sweetening, exactly as heat transforms sucrose. The culinary metaphor is doing synthesis work. Kakehashi called it "the most human saturation I have seen in a synthesizer."

---

### Priority Fixes

| Priority | Fix |
|----------|-----|
| P0 | LFO2: wire to caramel amount or body depth — `(void) lfo2Val` must be eliminated |
| P1 | Sympathetic scaling: raise from 0.04 to 0.08-0.10 for audible musical contribution |
| P1 | Write Ochre architecture doc (mirrors oven-architecture.md format) |
| P2 | Consider adding body type crossfade parameter for smooth transitions |
| P2 | macroMovement: currently only targets caramel — consider also animating thermal drift rate |

---

---

## ENGINE #3 — XOBELISK | "The Cold Monolith" (Prepared Stone Piano)

*Parameter prefix: `obel_` | 30 parameters | 10 presets | Accent: Obsidian Slate #4A4A4A*

---

### Ghost Council — Channel 3

**Robert Moog:** "The preparation system is the finest piece of conceptual engineering in the Kitchen quad. The position-sensitivity formula — `sin^2(N * pi * position)` — is real prepared piano physics. Position 0.5 kills even modes. Position 0.33 kills every third mode. Cage's insight operationalized correctly. The ChainBuzzer using a displacement threshold and broadband noise is the sitar jawari principle encoded. This is not imitation — it is understanding. I have two concerns. First: the `macroCoupling` and `macroSpace` parameters are loaded but never applied to any effective parameter. They are dead. Second: the Bolt Rattle module's frequency formula `40.0f + modeFreq * 0.05f` is arbitrary — the bolt's actual resonant frequency depends on its mass and the string tension, neither of which is parameterized. This is a simplification too far."

**Don Buchla:** "Buchla approves of the Prepared Piano concept unreservedly. Stone is the correct material for a prepared instrument — the high Q (200-1000) means preparations produce extreme, long-lasting spectral sculpting rather than mild coloration. The rubber preparation's nearly-complete Q reduction (up to 95%) on contacted modes is correct and radical. The glass preparation actually INCREASING Q by up to 30% is correct — glass is rigid, it reduces radiation loss at contact points. These are not approximations; they are physics. What Buchla objects to is the `obel_stoneTone` parameter. It is declared and applied to `effectiveStoneTone`, which is then — I read the render loop — never used in the modal computation. It is pure dead wire. The mod wheel drives `effectiveStoneTone` but `effectiveStoneTone` does not appear in any subsequent calculation."

**Ikutaro Kakehashi:** "Players will misunderstand this engine. The name is Obelisk, the accent is dark, the concept is Cage, and the preparations are abstract. The preset names help — Bolt Rattle, Rubber Mute, Sonatas Interludes — these are concrete entry points. But the cold/warm axis that `stoneTone` is supposed to provide does not work because the parameter is dead. A player who sweeps the mod wheel expecting tonal change will hear nothing. This erodes trust. Fix the stoneTone routing before release. The chain buzz and bolt rattle presets are strong. The glass ring preset is the most beautiful thing in the quad."

---

### Scoring

| Dimension | Score | Notes |
|-----------|-------|-------|
| Sound Quality | 9.0 | Inharmonic stone ratios are distinctive; preparation system produces genuinely novel timbres; chain buzz and rubber mute are unlike anything in the fleet |
| Innovation | 9.5 | The preparation system is original to the fleet; position-sensitivity formula is real physics implemented correctly; this is the highest-innovation engine in the quad |
| Playability | 8.0 | The preparation type selector (integer, 5 states) is a strong expressive lever; but changing preparation type mid-preset is not smooth; cold stone requires players to understand the system before it rewards them |
| Preset Coverage | 8.5 | 10 presets with clear preparation-type coverage; Sonatas Interludes (named after Cage's masterwork) is a strong statement; Frozen Cathedral is beautiful |
| DSP Efficiency | 9.0 | 3.2% CPU maintained; bolt rattle AM and chain buzzer are lightweight additions; HF noise shaper only active during attack phase |
| Musical Utility | 8.5 | The most specialized engine in the quad — this is for composers and sound designers; will limit broad accessibility but rewards those who engage |
| Identity / Character | 9.5 | XObelisk has the most singular identity in the fleet since OWARE; the Cage reference is not decoration, it is the entire engine concept |
| **Overall** | **8.8** | Fleet-average equivalent; two dead parameters prevent a higher score |

---

### Top 3 Concerns

1. **`obel_stoneTone` is dead — D004 violation.** The parameter is declared, loaded (`effectiveStoneTone` computed via mod wheel + pStoneTone), and then `effectiveStoneTone` is never used in any modal calculation or filter routing. The mod wheel says it controls "cold ↔ warm axis" but produces no audible result. This must be wired before release.

2. **`macroCoupling` and `macroSpace` are loaded but unapplied — D004 violation (partial).** Both macros are read from APVTS (lines 654-655) but neither appears in any `effectiveX` computation. The architecture doc lists `obel_macroCoupling` as "reserved for coupling matrix" and `obel_macroSpace` as "reserved for reverb/space" — but in the current implementation, these are truly dead. Players who move them will hear nothing. Reserve is acceptable in alpha; shipped with "reserved" is a broken promise.

3. **Bolt Rattle frequency formula is arbitrary.** `40.0f + modeFreq * 0.05f` produces a rattle frequency between 40 Hz and ~200 Hz depending on mode frequency, but these numbers are not grounded in physics — the bolt's actual rattle frequency depends on its mass, the contact geometry, and the string tension. The formula produces a musical result, but the documentation's claim of physical accuracy is not fully supported here. This is a minor concern but notable in an engine that otherwise cites its physics carefully.

---

### Top 3 Enhancement Suggestions

1. **Wire `stoneTone` immediately.** Cold (0) should boost Q of upper modes (stone rings more clearly when cold) and tilt the HF noise down. Warm (1) should slightly broaden mode bandwidth (thermal expansion) and boost the fundamental relative to partials. Even a simple: `float qMod = 1.0f + effectiveStoneTone * 0.4f;` applied to mode Q would make this parameter live and musically meaningful.

2. **Give `macroCoupling` and `macroSpace` temporary targets.** Until the Kitchen-quad coupling mechanics are built: COUPLING → prep depth (coupling a neighboring engine changes what is inserted into the stone) is a conceptually coherent stub. SPACE → HF noise amount (more space = more air in the attack noise) is similarly coherent. Both can be replaced by proper quad coupling mechanics in V2.

3. **Add a `prepTransition` parameter or make position/depth LFO-targetable.** The most musically powerful use of the preparation system is animating preparation depth via LFO2 (already the case in Living Stone preset). But prep position is not animatable. A slow LFO-driven position sweep — the bolt slowly dragging along the string — would be unique and musically rich.

---

### Blessing Candidates

- **B_OBELISK_01 — Position-Sensitive Preparation Physics:** The `sin^2(N * pi * position)` formula for mode sensitivity is real prepared piano physics, not approximation. This is Cage's insight encoded in DSP. Buchla gave it the highest possible endorsement: "I would have built this in hardware if I could have."
- **B_OBELISK_02 — Glass Preparation Increases Q:** Every other preparation damps or disrupts. Glass preparation INCREASES Q because glass is rigid and reduces radiation loss. This counterintuitive physical truth produces a preparation type unlike any of the others — additive ring rather than subtraction.

---

### Priority Fixes

| Priority | Fix |
|----------|-----|
| P0 | `obel_stoneTone`: wire to modal Q tilt (cold = higher upper Q, warm = broader bandwidth) |
| P0 | `obel_macroCoupling` and `obel_macroSpace`: wire to interim targets (coupling→prepDepth, space→hfNoise) |
| P1 | Document the physics limitation of the bolt rattle frequency formula — either fix the formula or add a caveat in the architecture doc |
| P2 | Consider exposing prep position as a modulatable continuous parameter (distinct from integer prepType) |

---

---

## ENGINE #4 — XOPALINE | "The Porcelain Bell"

*Parameter prefix: `opal2_` | 27 parameters | 10 presets | Accent: Crystal Blue #B8D4E3*

---

### Ghost Council — Channel 3

**Robert Moog:** "The Fragility mechanic is the most musically courageous decision in the entire quad. A synthesizer whose character changes irreversibly when you hit it too hard. The voice crackState persists across the note lifetime — once cracked, always cracked. You cannot un-crack the creme brulee. This is not a bug turned feature; this is a design principle: consequence. I want this in every keyboard player's hand. My concern is the `macroCoupling` parameter — it is loaded but I find no computation where it affects any `effective*` variable. Like Obelisk's COUPLING, it is declared and wired to APVTS but produces no DSP output. Fix this. The four-instrument selector (Celesta / Toy Piano / Glass Harp / Porcelain Cups) with per-instrument modal tables is excellent — the most extensive timbral range in the quad in a single engine."

**Don Buchla:** "The Glass Harp modal ratios — 1.000, 1.594, 2.136, 2.653... — are derived from circular plate shell mode pairs. This is correct. Circular plate resonances are strongly inharmonic in exactly this pattern. The Celesta ratios being exactly harmonic (1, 2, 3, 4...) is also correct — tuned bar resonators with resonator tubes produce extremely pure partials. The physics is right. What Buchla finds troubling is the Toy Piano modal table. The first eight values (1.000, 2.756, 5.404...) are the stone plate ratios from XObelisk's kStoneRatios table exactly, and the second eight values (1.500, 3.010, 5.982...) appear to be a shifted version. Short stiff metal tines have their own beam mode ratios (from the Euler-Bernoulli beam equation: 1.000, 6.267, 17.547...). The current Toy Piano ratios are not physically grounded for metal tines. This does not ruin the engine but it weakens D003 compliance."

**Ikutaro Kakehashi:** "I think of every child who ever played a toy piano and I endorse this engine completely. The four-instrument selector is a gift to producers who need that specific sound. The shimmer LFO (up to ±6 cents per mode, scaled by mode index) creates the crystalline beating that makes glass instruments shimmer in real life. The thermal personality seeds per voice mean that eight simultaneous notes all drift differently — exactly correct for eight real glass vessels. The crack mechanic is the best feature in the fleet since the Caramel Saturation. But `macroCoupling` is dead. And the prefix is `opal2_` — why? The original `opal_` prefix belongs to XOpal (granular). But new players seeing `opal2_` in a parameter list will be confused. Document this explicitly."

---

### Scoring

| Dimension | Score | Notes |
|-----------|-------|-------|
| Sound Quality | 9.0 | Glass Harp and Porcelain Cup ratios are accurate and beautiful; crack mechanic produces genuinely novel timbral deformation; shimmer LFO is physically motivated and musical |
| Innovation | 9.5 | Fragility mechanic — velocity * fragility > threshold → irreversible crack state with detuning — is unique in the entire synthesizer landscape; crack detuning scaling by mode index (higher modes shift more) is correct physics |
| Playability | 9.0 | Four distinct instruments with one selector; fragility threshold is immediately expressive; shimmer responds to SPACE macro; crystal drive adds one more dimension |
| Preset Coverage | 8.5 | 10 presets across all 4 instruments plus fragility extremes; Shattered Glass and Thermal Fracture are strong demonstrations of the crack mechanic; Music Box is charming |
| DSP Efficiency | 9.0 | 3.0% CPU — lightest in the quad; HF noise shaper only during exciter active; very high Q modes benefit from dynamic pruning at -140 dB threshold |
| Musical Utility | 8.5 | Celesta and Toy Piano cover immediate scoring needs; Glass Harp is for experimental contexts; the crack mechanic makes this the quad's most expressive engine for dynamic players |
| Identity / Character | 9.5 | Creme brulee as DSP principle — beautiful because breakable — is the sharpest conceptual identity in the quad; the per-voice crack state is genuinely unforgettable |
| **Overall** | **8.9** | Above fleet average; one dead parameter prevents 9.0+ |

---

### Top 3 Concerns

1. **`opal2_macroCoupling` is dead — D004 violation.** The parameter is loaded but never applied to any `effective*` variable in the render loop. A player moving the COUPLING macro hears nothing. This is the same category of issue as Obelisk's COUPLING/SPACE macros. The fix is straightforward: wire to coupling output depth or a secondary timbral axis.

2. **Toy Piano modal ratios are borrowed from stone plate physics, not beam physics.** The kToyPianoRatios table (1.000, 2.756, 5.404, 8.933...) matches kStoneRatios exactly for the first eight entries. Metal tines in a toy piano are short, stiff beams — their modes follow the Euler-Bernoulli free-free bar equation: f_n proportional to (1.194², 2.988², 5.000², ...). The current ratios produce a plausible sound but are not the correct physical model for metal tines. D003 is partially compromised.

3. **`opal2_` prefix is a source of ongoing confusion.** The `opal_` prefix belongs to XOpal (granular, lavender, completely different engine). XOpaline uses `opal2_` to avoid collision. This is technically correct but creates a naming irregularity: every other engine in the fleet has a clean 4-6 character prefix. Document this explicitly in the architecture doc and CLAUDE.md prefix table, and add a comment in the parameter declarations explaining the collision.

---

### Top 3 Enhancement Suggestions

1. **Wire `macroCoupling` to coupling input depth.** The most natural mapping: COUPLING macro scales how strongly XOpaline receives from the MegaCouplingMatrix. At COUPLING=0, external signals don't enter. At COUPLING=1, full coupling. This is consistent with how coupling macros work in other engines and gives players an intuitive lever.

2. **Correct the Toy Piano modal ratios.** Replace with Euler-Bernoulli free-free bar ratios: 1.000, 6.267, 17.547, 34.386... (the actual physics of short stiff metal tines). The sound will change — it will be brighter and more percussive, which is accurate to real toy pianos. If the current ratios are preferred for musical reasons, add a comment acknowledging the deliberate deviation from physics.

3. **Add a per-instrument crack threshold variation.** Currently `crackThreshold = 0.65f` is the same for all four instruments. In reality: glass cracks at lower velocity than porcelain, and porcelain cracks at lower velocity than metal (celesta, toy piano). Per-instrument threshold values would make the fragility mechanic more physically accurate and add timbral differentiation between instruments without adding parameters.

---

### Blessing Candidates

- **B_OPALINE_01 — Irreversible Crack State (The Fragility Mechanic):** A synthesizer whose timbre changes permanently when struck too hard, and the crack detuning accurately scales by mode index (higher modes shift more). This is the most musically courageous implementation decision in the Kitchen quad. All three ghosts endorsed it without qualification. Moog: "This is consequence as composition." Kakehashi: "Every pianist knows this feeling. Now they can program it."
- **B_OPALINE_02 — Per-Instrument Modal Tables with Differential Character:** Four instruments with physically grounded modal ratios, per-instrument Q, per-instrument amplitude rolloff, per-instrument exciter character, per-instrument sympathetic scale. This is the most complete instrument-selector implementation in the fleet.

---

### Priority Fixes

| Priority | Fix |
|----------|-----|
| P0 | `opal2_macroCoupling`: wire to coupling input depth scaling |
| P1 | Document `opal2_` prefix explanation prominently (CLAUDE.md, architecture doc, header comment) |
| P1 | Toy Piano ratios: update to Euler-Bernoulli free-free bar ratios OR add explicit comment documenting deliberate deviation |
| P2 | Per-instrument crack threshold variation (glass 0.55, porcelain 0.65, toy piano 0.75, celesta 0.80) |
| P2 | Add 2-3 presets demonstrating cross-instrument fragility variation |

---

---

## CROSS-QUAD ANALYSIS

### 1. Material Character Differentiation

The four engines occupy a genuine physical spectrum:

| Engine | Impedance Z | Decay Character | Attack Character | Identity Pole |
|--------|-------------|-----------------|------------------|---------------|
| XOven | 36.72 MRayl | Longest (up to 15s) | Slow bloom possible | MASSIVE / DARK |
| XOchre | 33.60 MRayl | Medium (up to 8s) | Immediate, responsive | WARM / TRANSPARENT |
| XObelisk | 10.26 MRayl | Long but cold (up to 15s) | Percussive, cold snap | INHARMONIC / PREPARED |
| XOpaline | 12.58 MRayl | Glass-long but delicate | Instant crystalline snap | FRAGILE / CRYSTALLINE |

The differentiation is real and audible in the modal ratios alone: XOven's slightly-stretched near-harmonic ratios vs XOchre's tighter-spaced copper ratios vs XObelisk's wildly inharmonic stone plate ratios vs XOpaline's four completely different ratio tables. No two engines will overlap sonically at any parameter position. The quad passes the differentiation test.

One gap: XOchre and XOven overlap at low brightness settings. Both are warm, both are sustained, both have similar sympathetic networks. The copper caramel saturation is the primary differentiator at low brightness, but a player who does not engage that mechanic may hear XOchre as a lighter-weight XOven. The Conductivity parameter is the key differentiator — it should be more prominently featured in the KITCHEN preset narrative.

### 2. Resonant Coupling Readiness

The engines are structurally coupling-ready through the MegaCouplingMatrix but are not yet Kitchen-quad-specifically coupled.

Current coupling support across all four:
- `AmpToFilter` → filter cutoff modulation (+2000 Hz range)
- `LFOToPitch` → pitch modulation (±2 semitones)
- `AmpToPitch` → pitch displacement
- `EnvToMorph` → varies by engine (body mod / conductivity / prep depth / instrument selector)

The XOven architecture doc provides the theoretical framework for Kitchen-quad coupling — the acoustic impedance transmission coefficient `T = 4*Z1*Z2 / (Z1+Z2)^2`. XOven → XOpaline gives T ~= 0.38, the strongest contrast coupling in the family. This coupling mathematics is documented but not implemented in the DSP — the `EnvToMorph` coupling type is used generically, not specifically routed through the impedance formula.

**Coupling readiness assessment: 6/10.** The engines expose coupling endpoints and respond to incoming signals, but the Kitchen-quad-specific coupling physics (impedance transmission, sympathetic resonance between materials, adversarial competition) is documented as V2 features and the stubs (competition, couplingResonance) are thin. The quad could couple now via the MegaCouplingMatrix, but the magic of cross-material resonance is not yet realized.

Most promising coupling pairs:
- **XOven → XOpaline:** Cast iron's massive modal cloud loads into glass's narrow-Q structures. Conceptually the richest pair. The "glass smoked in cast iron" image from the architecture doc.
- **XOchre → XOven:** Copper's faster response cross-driving iron's massive sustain. Like a responsive upright teaching a concert grand to be more immediate.
- **XObelisk → any:** Preparation depth driven by external amplitude. External aggression changes what is inserted between XObelisk's strings.

### 3. Strongest / Weakest Engine

**Strongest: XOpaline (8.9) or XObelisk (8.8)**

XOpaline holds the most original concept in the fleet (irreversible crack state) and has the best implementation of an instrument-selector. XObelisk holds the most physically rigorous preparation system and the highest innovation score. Both are above fleet average.

**Weakest: XOchre (8.2)**

XOchre is the only engine with a hard P0 (LFO2 dead, `(void) lfo2Val`). It is also the only engine without an architecture doc. It is also the only engine whose sympathetic resonance is inaudible at practical settings (0.04 scaling). These three issues together push it below fleet average despite a strong conceptual core and the fleet's best saturation mechanic.

### 4. Quad Cohesion

The Kitchen quad holds together better than any previous engine family in XOmnibus. The reasons:

- **Shared material narrative:** All four derive from acoustic material physics with citations. The impedance spectrum (36.72 → 33.60 → 10.26 → 12.58 MRayl) provides a physical ordering that maps to sonic character.
- **Shared culinary mythology:** Cast iron/Maillard sear, Caramel/copper saucepan, Marble/prepared table surface, Creme brulee/porcelain ramekin. The kitchen metaphor holds across all four without feeling forced.
- **Shared DSP utilities:** All four use StandardLFO, FilterEnvelope, CytomicSVF, GlideProcessor, ParameterSmoother, VoiceAllocator, SilenceGate, FastMath. This means bug fixes and improvements to shared utilities propagate to all four simultaneously.
- **Shared parameter philosophy:** All four have 2 LFOs (one with D004 issue in Ochre), mod wheel → brightness, aftertouch → hardness, 4 macros (CHARACTER/MOVEMENT/COUPLING/SPACE). The player learns one engine and understands the others.

The main cohesion gap is the dead COUPLING macros across three of four engines (Obelisk, Opaline, and Oven's competition stub). The quad promises cross-engine coupling but does not yet deliver it at the macro level. This will be the defining V2 improvement.

---

## DEAD PARAMETER AUDIT — Kitchen Quad D004 Status

| Engine | Parameter | Status | Fix Priority |
|--------|-----------|--------|-------------|
| XOchre | `ochre_lfo2Depth` / `ochre_lfo2Rate` / `ochre_lfo2Shape` | DEAD (void cast) | P0 |
| XObelisk | `obel_stoneTone` | DEAD (loaded, never applied) | P0 |
| XObelisk | `obel_macroCoupling` | DEAD (loaded, never applied) | P0 |
| XObelisk | `obel_macroSpace` | DEAD (loaded, never applied) | P0 |
| XOpaline | `opal2_macroCoupling` | DEAD (loaded, never applied) | P0 |
| XOven | `oven_competition` | STUB (minimal behavior — fastTanh on output) | P2 |
| XOven | `oven_couplingResonance` | STUB (minimal behavior — sympathetic drive scalar) | P2 |

**Total D004 violations: 5 dead, 2 stubs.** This is the most pressing issue for the quad before release.

---

## DOCTRINE COMPLIANCE SUMMARY

| Engine | D001 | D002 | D003 | D004 | D005 | D006 |
|--------|------|------|------|------|------|------|
| XOven | PASS | PASS | PASS | PASS (stubs are functional) | PASS | PASS |
| XOchre | PASS | **FAIL** (LFO2 dead) | PASS | **FAIL** (LFO2 dead) | PASS | PASS |
| XObelisk | PASS | PASS | PASS | **FAIL** (3 dead params) | PASS | PASS |
| XOpaline | PASS | PASS | **PARTIAL** (Toy Piano ratios) | **FAIL** (COUPLING dead) | PASS | PASS |

**For Kitchen Collection launch, all P0 items must be resolved before these engines are seanced at 9.0+ threshold.**

---

## FINAL SUMMARY SCORES

| Engine | Score | vs Fleet Avg | Status |
|--------|-------|-------------|--------|
| XOven | 8.7 | At average | PASS with P1/P2 improvements |
| XOchre | 8.2 | -0.6 below average | **HOLD** — P0 required |
| XObelisk | 8.8 | +0.0 at average | **HOLD** — P0 required (3 dead params) |
| XOpaline | 8.9 | +0.1 above average | **HOLD** — P0 required (1 dead param) |
| **Quad Avg** | **8.65** | -0.15 below fleet | All 4 engines need P0 pass before release |

The Kitchen quad is conceptually the strongest engine family in the fleet. The material-physics grounding, the culinary mythology, the shared DSP infrastructure, and the genuine timbral differentiation are all fleet-best achievements. But 5 dead parameters and 2 stubs prevent a clean seance verdict. Resolve D004 violations, write the Ochre architecture doc, double the Ochre sympathetic scaling, and re-seance. The quad ceiling is 9.0/9.0/9.2/9.3 after fixes.

---

*Seance complete. The ghosts have left the room. The iron is still warm.*

*Docs/seance-kitchen-quad-verdict.md — Written 2026-03-21*
