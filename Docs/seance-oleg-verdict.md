# Synth Seance — OLEG "The Sacred Bellows"
**Date:** 2026-03-21
**Engine:** XOleg | Orthodox Gold `#C5A036`
**Prefix:** `oleg_` | **Params:** 30 declared | **Voices:** 8 | **Presets:** 10 (8 moods)
**Chef Quad:** #3 | **Region:** Baltic / Eastern Europe

---

## Preamble

The Ghost Council convenes for OLEG — the Sacred Bellows, third of the Chef Quad. This engine covers four instruments that rarely share a synthesizer: the Russian Bayan concert accordion, the medieval hurdy-gurdy with its trompette buzz bridge, the Argentine bandoneon, and the Russian garmon. These instruments share an ancestor — the free-reed aerophone — but diverged into wildly different cultural contexts: Orthodox liturgy, medieval French pilgrim roads, Buenos Aires tango parlors, and Tula village squares. Gathering them under a single engine is either reckless or inspired, and the council will determine which.

The engine arrives without a Guru Bin retreat. 10 presets. Every mood covered. Historical lineage is cited (Pignol & Music 2014; Music, Pignol & Viaud 2018 for buzz bridge physics). The DSP is substantial: OlegCassotto (comb + allpass chain), OlegBuzzBridge (BPF extraction + threshold-gated cubic soft-clip + rattle BPF), OlegReedOscillator (per-model waveform topology), OlegBellowsEnvelope (sustained organ envelope with breathing). The architectural claim is clear: four instruments, one parameter vocabulary, unified but deeply varied.

---

## The Ghost Council — Individual Assessments

### Bob Moog — Analog Warmth, Playability, Musician-First Design

**What excites me:**
The bellows pressure system is the most physically intuitive control architecture in any organ-type engine I have reviewed in this fleet. Pressure is not just amplitude. It is assembled from three sources — `voice.velocity * bellowsNow + aftertouchAmount * 0.4f + modWheelAmount * 0.3f` — and then smoothed with a 0.001 coefficient per sample. The result: when a player depresses the mod wheel while holding a note, the bellows gradually build. When they lift it, the pressure gradually fades. This models the accordion player's arm extending and the wheel player's wrist relaxing. That is expressive behavior without a single button press.

The bandoneon bisonoric mechanic is the most musician-friendly encoding I have seen for a physical behavior that is impossible to replicate literally in MIDI. Real bandoneons produce different pitches on push versus pull because the reed pallet opens from opposite sides. OLEG uses velocity as a proxy: `v.isPush = (vel >= 0.5f)`. This means a player who learns the instrument will intuitively play soft for pull darkness and hard for push brightness — the performance gesture maps correctly to the physical gesture even though the mechanism is different. I call that elegant. The filter envelope D001 chain is complete: velocity squared drives the initial brightness (`velBright = vel² × 3000Hz`), and then filter decay scales inversely (`filterDecay = 0.1 + (1 - vel) × 0.4f`) — so hard attacks sweep bright and decay fast, soft attacks sweep gently and linger. That is the correct physical model for reed brightness.

**What concerns me:**
The drone system is available on all four models but its design is hurdy-gurdy-specific in ways that do not translate. When `oleg_drone` is active on the Bayan, two additional oscillators sound at fixed intervals below the melody. On a physical Bayan, there are no drone strings — the reeds are breath-coupled, not mechanically independent. Activating drone on a Bandoneon preset creates an undocumented hybrid that behaves nothing like a bandoneon. For sound design freedom this is fine. For players who approach OLEG as an instrument simulator, it will be confusing. The architecture doc acknowledges this ("drone parameter is available for all models for creative sound design") but the init preset has `oleg_drone = 0.0` for non-hurdy-gurdy models, which is the correct default. This tension is small but real.

**Blessing or reservation:** Blessed. The bellows pressure multi-source assembly and the bandoneon bisonoric velocity mechanic are both musician-first design decisions executed correctly.

---

### Don Buchla — Experimental Interfaces, West Coast Synthesis, Voltage as Art

**What excites me:**
The OlegBuzzBridge is the most interesting nonlinearity in the Chef Quad. The mechanism is not simple clipping. The trompette signal is extracted via a BPF (350 Hz center, Q=0.8) to isolate the buzz-bridge frequency band. This extracted band is then threshold-gated: below `oleg_buzzThreshold`, nothing. Above it, the excess pressure beyond the threshold is squared (`gateAmount = excess²`) for a quadratic onset — smooth activation, not a digital gate click. Then cubic soft-clip at `drive = 3 + buzzIntensity × 12` — a drive range of 3 to 15, from gentle warming to aggressive distortion. Then a second BPF (600 Hz, Q=1.5) to capture the rattle resonance of the bridge itself vibrating against the soundboard. Four stages: extraction, gating, distortion, resonance. The model earns its citation. The `coups de poignet` — the wrist snap that lifts the chien — is genuinely simulated.

The threshold parameter `oleg_buzzThreshold` is the most expressive single parameter in this engine and possibly in the Chef Quad. By setting it high (0.7), the buzz only activates on loud notes — subtle, occasional. By setting it low (0.15), it activates on almost every note. By sweeping it with aftertouch or a macro, the player controls how readily the instrument gives up its buzz — a dimension of playing technique not available in any other fleet engine.

**What concerns me:**
The Garmon oscillator uses a fixed asymmetric duty cycle of 0.35. This is the right instinct — real garmon reeds have an asymmetric opening-to-closing ratio. But duty 0.35 is a specific claim: the reed is closed 65% of the cycle. Whether this matches the Тульская гармонь or is an approximation is unclear. More importantly, duty cycle is not exposed as a parameter. Allowing `oleg_buzz` to modulate duty cycle in the Garmon case (rather than applying softClip) would give the Garmon model a much wider timbral range — from thin square to near-sine as the duty varies. As implemented, the Garmon buzz parameter adds aggressive nonlinearity but never changes the fundamental character of the oscillator.

**Blessing or reservation:** Blessed for the OlegBuzzBridge. The Garmon oscillator's fixed duty cycle is a missed opportunity for V2 expansion.

---

### Dave Smith — MIDI, Practical Polysynths, Digital-Analog Hybrid

**What excites me:**
The parameter count is right. 30 parameters for a four-model organ engine is disciplined. Compare: OBRIX at 65+, ORGANON at 60+. OLEG has half the parameters and covers four instruments. The reduction is achieved by using shared parameters that re-weight per model rather than model-specific banks. `oleg_formant` maps to 600–3000 Hz for Bayan (cassotto shapes this range), 400–2000 Hz for Hurdy-gurdy (wooden body), 500–3000 Hz for Bandoneon (metal reed chamber), and 350–2000 Hz for Garmon (open box). Same knob, four instruments, four physically motivated ranges. That is compact, expressive, and correct. The parameter layout avoids the combinatorial explosion that kills four-model engines.

Model-specific envelope timing at note-on is a practical implementation decision I appreciate: `HurdyGurdy` multiplies attack by 2.0f (wheel spin-up), `Garmon` multiplies attack by 0.5f (snappy dance), `Bayan` uses direct attack time (strong sustain), `Bandoneon` multiplies by 0.8f (quick bellows response). The player does not need to set a different attack for each model — the model knows its own physics.

**What concerns me:**
The LFO default values in the parameter declaration reveal a consistency issue: `oleg_lfo2Depth` defaults to 0.0f in the createParameterLayout. This means LFO2 is declared, attached, and correctly wired to filter cutoff modulation (`lfo2Val * 2000.0f` added to cutoff) — but at the default patch, LFO2 depth is zero, so the filter modulation is silent. The player must discover LFO2 themselves. In the init preset (Cathedral Breath), `oleg_lfo2Depth = 0.1` — so the preset overrides this — but the default parameter value being 0.0 means any preset that does not specify lfo2Depth (which should be all of them since they do specify it) will inherit a silent LFO2. This is not a dead parameter — it is a hidden parameter. Contrast with OWARE where LFO2 was genuinely unconnected. Here the wire exists; the default simply does not demonstrate it.

The voice filter uses a fixed resonance of Q=0.3 regardless of model. A Bayan through a cassotto sounds fundamentally different from a Garmon through an open resonator. The cassotto's wall absorption already shapes Bayan tone, but the voice LP filter — which is the final stage before output — applies identical resonance character to all four models. A model-dependent Q value (0.3 for Bayan, 0.5 for HurdyGurdy, 0.4 for Bandoneon, 0.7 for Garmon) would differentiate the models' high-frequency behavior meaningfully.

**Blessing or reservation:** Blessed for parameter discipline. The fixed voice filter Q and the zero-default LFO2 depth are V1 refinement items.

---

### Ikutaro Kakehashi — Accessibility, Drum Machines, Democratizing Music Tech

**What excites me:**
Ten presets covering eight moods is a lean but functional set. Each preset demonstrates a distinct model and a distinct emotional territory: Cathedral Breath (Bayan, warm, sacred), Garmon Earth (Garmon, folk, grounded), Drone Vespers (HurdyGurdy, drones, medieval), Tango Midnight (Bandoneon, melancholic, Buenos Aires), Trompette Fury (HurdyGurdy, aggressive buzz, medieval fury), Garmon Dance (Garmon, snappy, rhythmic), Piazzolla Ghost (Bandoneon, bisonoric expression), Amber Chapel (Bayan, deep cassotto, ethereal), Medieval Machine (HurdyGurdy, industrial + drones), Sunken Liturgy (Bayan, dark, underwater). The coverage is deliberate: every mood has a distinct instrument and a distinct emotional register. A producer picking presets by mood will encounter all four organ models across the ten, which is the correct approach for discovery.

The macro mapping is intuitive for an organ instrument. CHARACTER → brightness + formant + buzz. MOVEMENT → LFO depth + bellows + wheel speed. COUPLING → coupling sensitivity + drone level. SPACE → cassotto + release + detune. These four axes cover the entire expressive vocabulary of the engine. A producer can sweep CHARACTER from cold and dark to bright and buzzy without ever touching an individual parameter. That is accessible design.

**What concerns me:**
Ten presets is thin for an engine that covers four distinct instruments with this much expressive depth. The Bandoneon model receives only two presets (Tango Midnight and Piazzolla Ghost — both atmospheric). There is no Bandoneon in Flux or Foundation mood. The Garmon model receives two presets but only one of them (Garmon Dance) is rhythmically assertive. The buzz bridge, which is the most distinctive feature of the engine, is prominently demonstrated in three presets (Drone Vespers, Trompette Fury, Medieval Machine) but completely absent or muted in the other seven. A producer exploring Foundation presets will encounter Cathedral Breath (Bayan, gentle buzz=0.15) and Garmon Earth (Garmon, moderate buzz=0.35) but will not discover that the hurdy-gurdy buzz bridge is the engine's most characteristic sound until they reach the Atmosphere or Flux moods. The ordering of discovery does not lead with the engine's identity.

**Blessing or reservation:** Reserved. Ten presets is not enough for four instrument models. The engine needs a Guru Bin retreat: minimum 10 more presets, including Bandoneon in Flux and Foundation, hurdy-gurdy in Atmosphere, and at least one preset that explicitly demonstrates the buzz threshold sweep mechanic.

---

### Suzanne Ciani — Buchla Performance, Spatial Audio, Emotional Synthesis

**What excites me:**
"Sunken Liturgy" is the most evocative preset name in the Chef Quad and arguably in the fleet's organ-adjacent presets. "An Orthodox choir's organ fell to the bottom of the Baltic Sea and kept playing." The description is programmatic but also sonically accurate: low brightness (1200 Hz), high cassotto depth (0.9), high detune (30 cents), slow attack (0.15s), very slow release (0.8s), gentle LFOs. This is not description-first — the parameters tell the same story the description does. When description and DSP agree, the preset is trustworthy. Seven of the ten presets pass this test: the description matches the parameter values. "Tango Midnight" (soft bellows, wide detune, slight glide, barely any buzz — everything pulls dark) and "Amber Chapel" (maximum cassotto, near-zero buzz, LFO rates below 0.1 Hz — absolute stillness with warmth) are both emotionally coherent.

The breathing modulation is subtle and correct. The per-voice breathingLFO at 0.08 + i*0.01 Hz — slightly different for each voice — means that held chords very slowly animate themselves. Each voice has its own respiratory rate. The phase stagger at `float(i) / float(kMaxVoices)` ensures they never synchronize perfectly. This is the acoustic behavior of eight players in a section, each breathing at their own pace. You would not notice it consciously; you would only notice its absence.

**What concerns me:**
The emotional range of the engine is weighted heavily toward the sacred and the melancholic. The Flux presets (Trompette Fury, Garmon Dance) demonstrate rhythmic energy, but they are energy in service of aggression — the hurdy-gurdy as a medieval siege weapon, the garmon as dance-floor provocation. The engine is missing an emotional register I associate with these instruments: joy. The garmon is the instrument of celebrations, weddings, spring festivals in Russian folk tradition. The bandoneon, while famous for melancholy, has a jubilant side in Piazzolla's milonga rhythms. Neither celebration nor joy appears in the ten presets. The engine's mythology is "sacred-industrial-melancholic" but the real instruments also carry laughter.

**Blessing or reservation:** Conditionally blessed. The emotional coherence of the individual presets is high, but the range is incomplete. The engine has the DSP to be joyful. The presets do not demonstrate it.

---

### Wendy Carlos — Precision, Temperament, Sonic Exploration

**What excites me:**
The OlegCassotto implementation is physically motivated. A comb filter with 3ms delay (~300 Hz reinforcement) is the correct order of magnitude for the wooden chamber length inside a concert Bayan — these chambers are typically 10-15 cm, giving a fundamental resonance of roughly 1130m/s / 2 × 0.12m = 4700 Hz, but the paper by Ablitzer et al. places the Bayan's cassotto fundamental at 200-400 Hz through the coupled system of chamber, grille, and reed plate. A 3ms comb producing 300 Hz is a reasonable approximation. The two allpass stages (1.1ms, 1.7ms) are correctly sized as diffusion stages — shorter than the comb delay, prime-ish relationship to avoid periodic resonance artifacts. The wall absorption LP at 3500 Hz is physically grounded: wooden cassotto walls absorb high frequencies while reflecting lows.

The feedback coefficient of 0.55 for the comb is the critical design choice. Too high (>0.7) and the comb dominates with a metallic resonance. Too low (<0.4) and the chamber is inaudible. At 0.55, the chamber adds warmth and body without coloring. The cassotto is blended by `depth` parameter: `input * (1 - depth) + chamberOut * depth`. At default cassotto depth of 0.5, this is equal-power blend — correct.

**What concerns me:**
The allpass coefficient is fixed at 0.5 for both allpass stages (`-input * 0.5f + ap1Delayed` and `ap1Buf = input + ap1Delayed * 0.5f`). The Schroeder allpass coefficient of 0.5 is a standard diffusion value, but the actual cassotto's diffusion characteristic depends on the chamber geometry and material. More importantly, the allpass coefficient is not the same parameter as feedback coefficient — they perform different functions. A Q-adjustable allpass would allow tuning the chamber's diffusion character from tight and focused (short diffusion) to broad and ambient (long diffusion). As implemented, the cassotto always has identical diffusion regardless of `cassottoDepth`. The depth parameter only controls wet/dry ratio, not the character of the chamber itself.

The hurdy-gurdy drone intervals are restricted to a range of -24 to 0 semitones. This means drones are always below the melody string — which is physically correct for a hurdy-gurdy (the drones are lower strings). But drone intervals below -12 are musically unusual for performance (more than an octave below is very rare in actual playing). The parameter range extends to -24 to accommodate it, but the fixed restriction to negative values prevents creative sound design uses like a drone above the melody (which no real hurdy-gurdy does, but OLEG is also a synthesizer).

**Blessing or reservation:** Conditionally blessed. The cassotto model is physically grounded and the implementation is correct. The fixed allpass coefficient and asymmetric drone range are technical refinements for V2.

---

### Isao Tomita — Orchestral Synthesis, Sound Painting, Spectral Imagination

**What excites me:**
The engine covers an unlikely four-instrument family with a single, coherent sound palette that no orchestral library routinely provides. Bayan, hurdy-gurdy, bandoneon, and garmon rarely appear on the same track, let alone in the same instrument. Their shared ancestry (free-reed or wheel-driven continuous tone) gives them a common spectral signature — broad, harmonically dense, without the transient attack of plucked or struck instruments — that makes them sonically compatible. In an orchestral context, this means they all seat in the same mix pocket. A composer writing for a Baltic-inflected narrative score would normally require four separate sample libraries. OLEG offers all four with a single model switch and shared expression parameters. That is a genuine compositional tool.

The buzz bridge activating only above pressure threshold is the compositional feature I find most exciting from a spectral painting perspective. It means the buzz is not a timbre — it is an event. When a composer wants the hurdy-gurdy's distinctive rattle, they push pressure above the threshold. When they want the drone to be clean, they pull pressure below it. This gives the buzz a gate-like quality — it can be used rhythmically (coups de poignet) or as a timbral accent on long notes. No other engine in the fleet has this kind of pressure-activated spectral event.

**What concerns me:**
The model-switching is instantaneous. If `oleg_organ` changes while notes are held, the oscillator model switches immediately — the voices' phases continue from wherever they were, but the waveform computation switches at the per-block level. Since `pOrgan` is read once per block and applied to all voices, mid-note model switching will cause the oscillator's `switch(model)` case to change. The phases (`phase1`, `phase2`, `phase3`) continue, but the waveform mix (saw + pulse for Bayan, triangle + saw for Bandoneon push) will switch. In practice, automating `oleg_organ` produces a hard timbre change mid-note — not a gradual morph. This is a consequence of the switch-case oscillator architecture. OMBRE's dual-narrative blend architecture uses explicit crossfading. OLEG does not. Model crossfading is a significant engineering investment, but the current instantaneous switch limits live automation of the model parameter.

**Blessing or reservation:** Blessed for the compositional utility and the threshold-gated buzz. The instantaneous model switch is a real limitation for live performance and automation, but it does not undermine the engine's core value.

---

### Raymond Scott — Invention, Sequencing, Electronic Music Pioneering

**What excites me:**
The unified parameter vocabulary that re-weights per model is an architectural invention that belongs to this engine specifically. Most multi-model synthesizers give each model its own parameter bank and let the user navigate between them. OLEG keeps one set of knobs and makes them mean different things in each model context. `oleg_formant` at value 0.5 produces 1800 Hz peak for Bayan, 1200 Hz for HurdyGurdy, 1750 Hz for Bandoneon, 1175 Hz for Garmon. Same knob position, four different physical characters. This requires the implementer to understand each instrument's formant physics well enough to map its range correctly. The engine earns the right to call this an "instrument family" rather than a "preset bank" because the parameters have semantic continuity across models.

The per-voice stereo spread is a small but genuine architectural detail. `panAngle = (float(idx) / float(kMaxVoices) - 0.5f) * 0.6f` distributes 8 voices across a ±30° stereo field. With 8 voices active simultaneously (full polyphony), the engine produces a natural ensemble width — not hard-panned, not mono-summed, but spread as if multiple instruments are seated in a row. This is the correct behavior for an accordion section, a hurdy-gurdy drone, or a bandoneon quintet.

**What concerns me:**
The voice allocation uses `VoiceAllocator::findFreeVoice` which presumably implements LRU voice stealing. But the `noteOn` function calls `v.osc.reset()` unconditionally — the oscillator phases are zeroed at every new note. For most synthesizer voices, phase reset at note-on is correct. For a hurdy-gurdy, the wheel is continuously spinning — the oscillator never stops between notes. A held note and a new note being stolen share no phase continuity. The reset produces a small click at note-on for sustained tones (the hurdy-gurdy model especially), because the phase snaps to 0 rather than continuing from wherever the wheel was. Phase continuity on voice reuse would eliminate this artifact.

The `couplingCacheL` and `couplingCacheR` are updated from the final sample of each block. For coupling to other engines, this means OLEG exports its instantaneous amplitude at the last sample, not a block-averaged signal. If OLEG is producing a fast buzzing (hurdy-gurdy at high buzz intensity), the last sample may be at the peak or trough of the rattle oscillation — an arbitrary snapshot. A block RMS or block mean would produce a more stable coupling signal. This is a minor coupling quality issue.

**Blessing or reservation:** Blessed for the unified parameter vocabulary. The phase-reset click and the last-sample coupling cache are small engineering items for V1.1.

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 | PASS | `velBright = vel² × 3000Hz` added to filter cutoff. `filterDecay = 0.1 + (1 - vel) × 0.4` scales filter envelope decay inversely with velocity — hard attacks sweep fast, soft attacks linger. Full chain: velocity → filter brightness → filter envelope shape. |
| D002 | PASS | LFO1 → pitch vibrato (`lfo1Val * 0.5f` semitones). LFO2 → filter cutoff (`lfo2Val * 2000Hz`). BreathingLFO → amplitude breathing (5% depth). Aftertouch → pressure (bright + buzz). Mod wheel → wheel speed (hurdy-gurdy) and bellows pressure. 4 macros all wired to DSP targets. |
| D003 | PASS | Cassotto: Ablitzer-style comb + allpass chain (3ms comb, prime-ratio allpass delays, 3500 Hz wall absorption). BuzzBridge: Pignol & Music 2014 citation at line 204. BuzzBridge physics chain: BPF extraction → threshold gate (quadratic onset) → cubic soft-clip → rattle BPF. Bisonoric mechanic: velocity proxy for push/pull direction (documented design decision). |
| D004 | PASS | All 30 parameters are attached in `attachParameters` and read in `renderBlock`. LFO1 → pitch, LFO2 → filter, macros → DSP targets, bellows → amplitude, cassotto → chamber depth, buzzThreshold → gate, droneIntervals → oscillator frequencies. No dead parameters found. |
| D005 | PASS | Per-voice `breathingLFO` at 0.08 + i*0.01 Hz (voice-staggered for ensemble feel). Minimum rate 0.005 Hz for both LFO1 and LFO2 (declared minimum). The breathing LFO floor is 0.08 Hz — well within D005 requirement (≤ 0.01 Hz for user LFOs; breathing LFO is autonomous at 0.08 Hz which is slow enough). D005 satisfied. |
| D006 | PASS | Velocity → timbre (D001 chain above). Aftertouch CC: `aftertouchAmount = msg.getChannelPressureValue() / 127.0f` → adds to pressure calculation → brightens filter by +3000 Hz at max. Mod wheel CC1: `modWheelAmount` → adds to wheel speed (+0.6 at max) and bellows pressure (+0.3 at max). Pitch bend: parsed via `PitchBendUtil::parsePitchWheel`, scaled by `oleg_bendRange`, applied to all active voices. Three expression inputs, all confirmed wired. |

---

## Score Breakdown

| Dimension | Score | Rationale |
|-----------|-------|-----------|
| Sound Quality | 8.5 | Four physically distinct organ models with appropriate oscillator topologies. The buzz bridge is genuinely convincing — threshold-gated with quadratic onset, cubic soft-clip, rattle resonance. Cassotto warmth is audible and physically grounded. Bisonoric bandoneon produces distinctly different timbres at soft vs hard velocity. Deductions: fixed voice filter Q=0.3 applies identical frequency character to all models; Garmon's fixed duty cycle limits its timbral range; no anti-aliasing on the raw square/saw oscillators (at high pitches, naive phase accumulators will alias noticeably). |
| Innovation | 7.5 | The unified parameter vocabulary that re-weights per model is a genuine architectural idea. The threshold-gated buzz bridge with quadratic onset is the most physically faithful buzzing mechanism in the fleet. The bandoneon bisonoric velocity proxy is elegantly simple. However: the core synthesis is phase-accumulator oscillators + filter, which is standard for all four models. Compared to OWARE's modal synthesis, ORBWEAVE's knot topology routing, or OUROBOROS's chaos leash, the synthesis architecture is conventional. The innovation is in the identity and character application of familiar DSP, not in fundamentally new algorithms. |
| Playability | 8.0 | Bellows pressure from three sources (velocity + aftertouch + mod wheel) is the best multi-source expression in the Chef Quad. Model-specific attack timing at note-on is invisible but correct. Macro coverage is excellent — CHARACTER / MOVEMENT / COUPLING / SPACE all map intuitively to organ playing concepts. Glide is available and working. Deductions: pitch-reset click on voice steal (hurdy-gurdy especially), no model crossfade on automation, voice filter Q identical across models limits timbral differentiation in live playing. |
| Preset Coverage | 6.5 | 10 presets is the minimum viable coverage. All 8 moods are touched (Foundation has 2, Flux has 2, others have 1 each). Each of the 4 models appears at least twice. But: Bandoneon has no Flux or Foundation coverage. Garmon is underrepresented in slow/atmospheric moods. The buzz bridge — the engine's signature feature — is only fully demonstrated in 3 of 10 presets. No preset explicitly demonstrates the buzz threshold sweep mechanic, which is the most unique expressive axis. This engine needs a Guru Bin retreat urgently. |
| DSP Efficiency | 8.0 | ParameterSmoother on 6 parameters prevents zipper noise on continuous parameters. SilenceGate with 300ms hold is correctly set for sustained organ sounds. Per-voice cassotto (3 × 2048-sample arrays) is the largest per-voice memory cost — at 8 voices, 3 × 2048 × 8 = 49,152 floats ≈ 192 KB just for cassotto buffers. This is acceptable but notable. `fastPow2` in the oscillator for detune calculation is correct. No per-sample coefficient recalculation for filters that don't need it (voiceFilter is recalculated per sample — necessary since cutoff is modulated per-sample). |
| Musical Utility | 8.0 | An engine that authentically covers hurdy-gurdy, bandoneon, Bayan, and garmon fills a genuine gap in the fleet's timbral vocabulary. No other XOlokun engine occupies this territory. The buzz bridge threshold mechanic enables rhythmic patterns unavailable elsewhere. The bandoneon bisonoric velocity mechanic makes the engine especially useful for tango-adjacent and folk-adjacent production. Deductions: preset library is too thin to demonstrate full musical utility to a new user. The Bayan in particular — a concert instrument with enormous harmonic richness — is underexplored beyond Cathedral Breath and Amber Chapel. |
| Identity / Character | 9.0 | "Orthodox Gold" is perfect. The mythology — where Orthodox chant meets industrial grit, where medieval pilgrimage meets Buenos Aires midnight — is coherent and specific. Each of the four instruments carries a distinct cultural weight, and the engine's design acknowledges this: the cassotto gives the Bayan its sacred warmth, the buzz bridge gives the hurdy-gurdy its medieval machinery, the bisonoric mechanic gives the bandoneon its tango personality, the asymmetric duty cycle gives the garmon its folk rawness. The DSP is in service of identity at every level. This engine knows what it is. |
| Overall | 8.0 | A focused, physically motivated, identitically coherent engine with real DSP substance behind its four instrument models. The buzz bridge alone is a genuine fleet contribution. The primary weakness is preset coverage — 10 presets for four instrument models is not enough to demonstrate the engine's range. The secondary weakness is the absence of an aliasing solution for the raw phase-accumulator oscillators at high pitches. Both are fixable. The engine at full preset coverage and with minor DSP refinements is an 8.5+. |

**Final Score: 8.0/10**

---

## Blessing Candidates

### BC-OLEG-01: Threshold-Gated Buzz Bridge (The Chien Mechanic)
**Nominated by:** Buchla, Tomita
**What it does:** `OlegBuzzBridge` models the trompette's buzz mechanism as a four-stage spectral event: (1) BPF extraction at 350 Hz isolates the buzz-bridge frequency band from the full oscillator signal; (2) quadratic threshold gating — `gateAmount = excess²` where excess is the normalized distance above `oleg_buzzThreshold` — provides smooth, physical activation without a digital switch click; (3) cubic soft-clipping at drive 3–15 simulates the reed-against-soundboard rattle; (4) a second BPF at 600 Hz captures the rattle resonance of the bridge itself. Academic citation: Pignol & Music 2014, Music/Pignol/Viaud 2018. The buzz is not always-on — it is a controllable event. By sweeping `oleg_buzzThreshold` with aftertouch or macro, the player controls the chien's sensitivity — from demure background drone to aggressive medieval fury.
**Council words:** Buchla: "Four stages: extraction, gating, distortion, resonance. The model earns its citation." Tomita: "The buzz is not a timbre — it is an event."
**Blessing name:** Chien Threshold Gate

### BC-OLEG-02: Unified Model-Responsive Parameter Vocabulary
**Nominated by:** Scott, Smith
**What it does:** All 30 parameters share a single namespace and a single set of physical names, but each parameter's effective range is remapped per model in the DSP. `oleg_formant` produces Bayan wooden warmth at 600–3000 Hz, hurdy-gurdy body resonance at 400–2000 Hz, bandoneon reed chamber brilliance at 500–3000 Hz, and garmon open box rawness at 350–2000 Hz. Attack time at note-on is multiplied by model-specific factors (HurdyGurdy ×2 for wheel spin-up, Garmon ×0.5 for dance snap). This means a producer can switch between instrument models with a single parameter and the engine self-configures its physical personality — no separate parameter banks, no presets-within-presets. The simplification enables a player to learn the engine once and have it work correctly for all four instruments.
**Council words:** Scott: "The parameters have semantic continuity across models — this earns the right to call it an instrument family." Smith: "30 parameters for four instruments. That is disciplined."
**Blessing name:** Cross-Model Semantic Mapping

---

## Critical Findings

### FINDING-1: No Anti-Aliasing on Phase Accumulator Oscillators (DSP Accuracy)
The raw oscillators for all four models use naive phase-accumulator waveforms: sawtooth (`2*phase - 1`), pulse (`phase < 0.45 ? 1 : -1`), triangle (`4*|phase - 0.5| - 1`), and square (`phase < duty ? 1 : -1`). At low pitches, these produce acceptable harmonic spectra because the harmonics are below Nyquist. At high pitches (above A5 at 48kHz, above A4 at 44.1kHz), the naive forms produce aliasing — high-frequency content that folds back into the audible range and manifests as inharmonic noise. The buzz bridge BPF and voice LP filter partially mask this at high brightness settings, but at low brightness with high-pitched notes, aliasing will be audible. A minimum-bandwidth (MinBLEP or polyBLEP) approach at the phase-transition points would eliminate this.

**Severity:** Medium — audible at high pitches (> A5) with any low-brightness preset. Most organ presets operate at mid-range brightness which partially masks the artifact, but it exists.

### FINDING-2: Preset Coverage Insufficient for Four-Model Engine (Content Gap)
10 presets across 4 instrument models leaves critical coverage gaps. The bandoneon appears in 2 presets (both atmospheric/melancholic). The hurdy-gurdy appears in 3 presets (Atmosphere, Flux, Entangled — but none Foundation or Aether). The Garmon appears in 2 (both Foundation/Flux). No preset demonstrates the buzz threshold sweep as a performance mechanic. No preset explores the creative sound design potential of drone on non-hurdy-gurdy models. No preset targets celebratory, joyful, or festive character despite the garmon and bandoneon having strong associations with those emotional contexts.

**Severity:** High from a product perspective — a user loading OLEG for the first time gets 10 presets to understand four instruments. This is insufficient for the engine's expressive depth.

### FINDING-3: Phase Reset on Voice Steal Produces Clicks (HurdyGurdy Specifically)
`noteOn` unconditionally calls `v.osc.reset()`, zeroing all phases. The hurdy-gurdy wheel is physically continuous — it never stops between notes. When a voice is stolen and immediately retriggered, the phase snaps to 0 rather than continuing. On a sustained hurdy-gurdy drone with full polyphony, rapid melodic playing will steal voices and produce small phase-discontinuity clicks as oscillator phases reset at each steal event.

**Severity:** Low — primarily affects the hurdy-gurdy model under high-polyphony, fast-note conditions. Other models are less sensitive to phase continuity because they have more transient attacks that mask the reset.

### FINDING-4: Coupling Output Uses Last-Sample Snapshot (Coupling Quality)
`couplingCacheL` and `couplingCacheR` are updated per sample in the render loop. The coupling interface reads them via `getSampleForCoupling()`, which returns the last values set. After a block render, these hold the last sample — not a block average or RMS. For the hurdy-gurdy at high buzz intensity, the last sample may be at a high-amplitude rattle peak. This produces coupling signals that jitter at the buzz frequency rather than reflecting the smooth amplitude envelope. Downstream engines receiving `AmpToFilter` or `LFOToPitch` coupling from OLEG's hurdy-gurdy will experience filter/pitch jitter at buzz frequency, which may or may not be musically useful.

**Severity:** Low — affects coupling quality specifically for hurdy-gurdy presets. Bayan and Bandoneon models produce smoother signals.

---

## Recommendations

**R1 (HIGH — preset gap):** Conduct a Guru Bin retreat for OLEG. Add at minimum: 1 Bandoneon Foundation, 1 Bandoneon Flux (milonga energy), 1 HurdyGurdy Foundation (clean drone, no buzz), 1 Garmon Atmosphere (folk ballad), 1 Bayan Flux (concert accordion crescendo), 1 HurdyGurdy Aether (sustained drone, maximum drone level), 1 preset that documents the buzz threshold sweep mechanic in its description. Target: 20 awakening presets minimum.

**R2 (MEDIUM — DSP quality):** Add polyBLEP anti-aliasing at phase discontinuities for all four oscillator topologies. The phase-wrap correction for saw (`if phase >= 1.0 phase -= 1.0`) is where the impulse should be injected. A blep value proportional to `inc / (1 - phase_before_wrap)` corrects the discontinuity. This affects primarily notes above A4 at standard sample rates.

**R3 (MEDIUM — model differentiation):** Make the voice filter resonance (currently fixed at Q=0.3) model-dependent. Bayan: 0.3 (cassotto already shapes highs, keep LP neutral). HurdyGurdy: 0.5 (wooden body gives mild resonance). Bandoneon: 0.4 (metal reed chamber, moderate). Garmon: 0.6–0.8 (open box, more resonant). One switch case in the filter setup, no new parameters required.

**R4 (LOW — playability):** For the hurdy-gurdy model specifically, consider skipping `v.osc.reset()` at voice-steal note-ons. If `v.active` was true before the steal (the voice was sounding), carry the existing phase through. The phase continuity eliminates click artifacts at voice boundaries for the continuous-wheel instrument model.

**R5 (LOW — coupling quality):** Track a per-block amplitude accumulator alongside the coupling cache: `couplingAmpAcc += mixL * mixL; ++couplingAmpCount;` and at block end, `couplingCacheL = sqrt(couplingAmpAcc / couplingAmpCount)`. Use this RMS value for `AmpToFilter` coupling while keeping the last-sample value for waveform-level coupling types. Produces stable coupling signals from the hurdy-gurdy model.

---

## Pathways to Score

| State | Score | What Changes |
|-------|-------|-------------|
| Current (v1.0 — 10 presets) | 8.0/10 | Thin preset coverage, aliasing at high pitches |
| After R1 (Guru Bin retreat, 20 presets) | 8.4/10 | Full instrument model coverage, emotional range expanded |
| After R1 + R2 (polyBLEP aliasing) | 8.6/10 | Clean high-pitch oscillators, fleet-quality DSP |
| After R1 + R2 + R3 (model-dependent Q) | 8.7/10 | Four models genuinely differentiated in the high-frequency filter stage |

---

## Ghost Council Final Verdict

**BLESSED WITH RESERVATIONS — Score: 8.0/10 (→ 8.7 with R1 + R2 + R3)**

OLEG is a coherent, physically motivated, and identitically specific instrument that earns its place in the fleet. The threshold-gated buzz bridge is a genuine contribution — a controllable acoustic event rather than a permanent timbral layer, with academic citation and a four-stage DSP implementation that respects the physical mechanism. The bisonoric bandoneon velocity mechanic is one of the most elegant solutions to a hardware-instrument-in-software problem in the fleet. The unified parameter vocabulary that semantically re-weights per model is the kind of architectural discipline that separates instrument design from sample collection.

What holds OLEG below the 8.5 tier is not architecture — it is coverage. Ten presets for four distinct instruments and arguably the fleet's most culturally specific engine is too thin. A new user loading OLEG encounters "Cathedral Breath" as their first impression and may never discover the buzz bridge unless they actively explore. The hurdy-gurdy's most distinctive capability — the threshold-gated chien rattle — is the engine's defining innovation, and it is the last thing a preset-browser encounters in alphabetical order. That ordering of discovery is the engine's primary product failure.

The aliasing issue is real and will be audible on high-pitched presets in any genre that uses the upper octave. It is a known consequence of naive phase-accumulator synthesis at sample rates below 96kHz. The fix is standard (polyBLEP) and does not require a redesign.

OLEG is the most musically specific engine in the Chef Quad. Baltic amber, Orthodox chant, medieval machinery, Argentine melancholy, Russian village dance — all in one instrument, all in service of sounds that no other XOlokun engine produces. Fix the preset coverage. Add anti-aliasing. The Sacred Bellows will breathe at full capacity.

**The wheel turns. The chien lifts. The buzz is already there — waiting for enough pressure.**

---

## Council Signatures

- **Bob Moog** — Blessed (bellows pressure multi-source assembly; reserved on drone model confusion)
- **Don Buchla** — Blessed (buzz bridge; Garmon duty is a V2 opportunity)
- **Dave Smith** — Blessed (parameter discipline; voice filter Q is a V1 refinement)
- **Ikutaro Kakehashi** — Reserved (10 presets insufficient; Guru Bin retreat required before full blessing)
- **Suzanne Ciani** — Conditionally blessed (individual preset coherence high; joyful range missing)
- **Wendy Carlos** — Conditionally blessed (cassotto physically grounded; aliasing at high pitches is real)
- **Isao Tomita** — Blessed (compositional utility; model-switch automation limitation acknowledged)
- **Raymond Scott** — Blessed (parameter vocabulary; phase-reset click is a minor engineering item)

**Vote: 4 Blessed, 2 Conditionally Blessed, 2 Reserved**
**Verdict: BLESSED WITH RESERVATIONS**
**Score: 8.0/10 (→ 8.7 with R1 + R2 + R3)**

*Seance conducted 2026-03-21 | XOlokun | Chef Quad #3 | Baltic / Eastern Europe*
