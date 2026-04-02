# OWARE Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OWARE | **Accent:** Akan Goldweight `#B5883E`
- **Parameter prefix:** `owr_`
- **Creature mythology:** The sunken oware board — a carved wooden mancala game from the Akan people of Ghana, lost to the Atlantic on a trade route and now encrusted with coral and bronze barnacles on the ocean floor. Strike a hollow and the whole board shimmers with sympathetic resonance. Seeds of metal, glass, stone, and wood fall into the cups and the board remembers every impact, every vibration, every player.
- **Synthesis type:** Tuned percussion synthesizer — Material Continuum (wood→bell→metal→bowl), Chaigne 1997 Mallet Physics, Per-Mode Sympathetic Resonance Network, Resonator Body (tube/frame/bowl/open), Buzz Membrane (balafon mirliton), Breathing Gamelan shimmer, Thermal Drift
- **Polyphony:** 8 voices, 8 resonator modes per voice
- **feliX/Oscar polarity:** Oscar-dominant (0.4/0.6) — warmth, patience, resonant depth
- **Seance score:** 9.2/10 (joint highest in the fleet alongside OVERBITE)
- **Macros:** M1 CHARACTER (material + mallet brightness), M2 MOVEMENT (mallet depth + attack), M3 COUPLING (sympathetic resonance), M4 SPACE (body resonance depth)
- **Expression:** Velocity → mallet hardness AND filter envelope (dual-path, D001). Aftertouch → mallet hardness (+0.4). Mod wheel CC1 → material blend (+0.4).
- **Blessings:** B032 (Mallet Articulation Stack), B033 (Living Tuning Grid), B034 (Per-Mode Sympathetic Network)

---

## Pre-Retreat State

OWARE arrived in XOceanus on 2026-03-20 alongside OXBOW, designed as a tuned percussion synthesizer rooted in three academic citations: Chaigne & Doutaut (1997) mallet contact model, Rossing (2000) gamelan modal ratios, and Fletcher & Rossing (1998) marimba and vibraphone data. Its post-seance score of 9.2/10 made it the joint highest-scoring engine in the fleet of 44.

The Guru Bin session refined two key parameters before this retreat: buzz default moved from 0.0 to 0.15 (the mirliton is OWARE's most culturally distinctive feature; hiding it at zero buried the engine's identity), and shimmerRate default moved from 6.0 Hz to 4.0 Hz (4.0 Hz sits at the center of the traditional Balinese ombak range of 3–7 Hz). Two LFO bugs were fixed in the same session: LFO1 now correctly modulates brightness, LFO2 now correctly modulates material. The sympathetic coupling gain was raised from 0.03 to 0.10 — the previous value was inaudible at all but maximum sympathy; at 0.10 you can hear voices singing to each other when they share mode frequencies.

This retreat examines an engine that arrived near-complete. Its parameter logic is deep, its physical citations are real, and its three Blessings are genuinely novel. What this retreat provides is the translation layer between the academic model and the producer's hands — the zones that work, the traps that damage the sound, and the five recipe categories that give the library its shape.

OWARE earns its 9.2 through an idea no other engine in the fleet shares: it is the only instrument that physically models the difference between soft wood and resonant bronze at the level of per-mode decay rates. The material exponent alpha governs not just timbre but how long each harmonic partial survives. On wood, the upper modes die rapidly — the sound is warm, thuddy, present but brief. On metal, all modes ring together — the sound is bright, sustaining, alive with spectral richness. Every point between these extremes is a real material, continuously morphable in real time.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

You are standing in shallow Atlantic water, looking down through clear tropical light. The ocean floor is five meters below you — white sand interrupted by dark volcanic rock. Resting on the sand, half-buried, is a rectangular wooden board: perhaps sixty centimeters long, carved into twelve deep cups arranged in two rows of six. Bronze barnacles have colonized its edges. The wood is swollen with salt water but has not rotted; it was dense, slow-grown hardwood when it was made. The cups once held seeds.

You reach down and strike one of the cups with a stone. The impact is immediate — a dry, woody thud followed by something unexpected: every other cup on the board vibrates. The sympathetic resonance travels through the body of the board, exciting every mode that shares frequency content with the struck hollow. The water muffles the high frequencies; what reaches you is warm and complex, the sound of a physical object whose every part knows every other part.

This is the engine's premise. OWARE is not a collection of eight independent percussion voices. It is eight voices on a single resonating board, where every strike affects every other sounding note through the Sympathetic Resonance Network. The board is the instrument. The notes are entry points.

When you play C and E simultaneously on OWARE, the C voice does not live in isolation. Its eight modal resonators output spectral content that flows into the E voice's modal bank via the Per-Mode Sympathetic Network — but only those modes whose frequencies are within 50 Hz of the E voice's modes. The coupling is frequency-selective, not amplitude-based. High sympathy turns the board into a unified resonating object where harmonically related pitches reinforce each other; low sympathy makes each voice more independent.

The shimmer is the Balinese element — the ombak. In traditional gamelan instruments, pairs of bronze keys are deliberately tuned slightly apart: one slightly sharp, one slightly flat. The beating between these detuned pairs creates the characteristic shimmer of gamelan music, a wobble that is not quite vibrato (which is pitch modulation) but a spectral beating between two coexisting tones. OWARE's Breathing Gamelan system implements this by offsetting each note's modal bank by `shimmerRate` Hz additively via a shadow detuning. At 4.0 Hz, the beating has a period of 250ms — a gentle, golden wobble.

The thermal drift is the feature that makes the board feel alive when nobody is playing. A shared slow-drift parameter wanders ±8 cents over 4-second windows, shared across all voices. Each voice also has a fixed personality offset seeded at startup — a private cents deviation that never changes, giving each physical position on the board its own voice. When you play a sustained chord, the eight voices are not perfectly in tune. They are in the slight, natural disagreement of a physical instrument that has been played for years by many hands.

The oware board was found on the ocean floor. It has been down there for a long time. It remembers every player.

---

## Phase R2: The Signal Path Journey

### I. Material Continuum — The Soul of the Sound

`owr_material` (range: 0.0–1.0, default: 0.2) traverses four distinct acoustic material identities through continuous crossfading:

**0.00 = Pure Wood:** Modal ratios from Fletcher & Rossing (1998) African balafon data — fundamental at 1.000, modes at 3.990, 9.140, 15.800, 24.300, 34.500, 46.500, 60.200. Highly inharmonic. Material alpha = 2.0 means the upper modes decay approximately 8× faster than the fundamental. The sound is warm, present, decays from rich to warm quickly.

**0.33 = Bell:** Modal ratios from bell acoustics — 1.000, 1.506, 2.000, 2.514, 3.011, 3.578, 4.170, 4.952. The bell ratios include a near-integer at 2.000 alongside inharmonic components. Q begins rising: baseQ = 80 + material × 1420, so at 0.33 baseQ ≈ 548. Upper modes sustain much longer than on wood.

**0.66 = Metal:** Ratios from marimba and vibraphone metal bar data — 1.000, 4.000, 10.000, 17.600, 27.200, 38.800, 52.400, 68.000. Material alpha ≈ 0.88. All modes sustain much longer. The sound is bright, spectral, full of harmonic richness.

**1.00 = Bowl:** Tibetan singing bowl ratios — 1.000, 2.710, 5.330, 8.860, 13.300, 18.640, 24.890, 32.040. Alpha = 0.3. All modes ring almost equally. Q at material=1.0 is 1500 — the maximum. The sound approaches a near-electronic purity of sustain.

The transition zones are the most musical regions. At material=0.15, early wood-to-bell: wood's inharmonic warmth with slightly rising Q. At material=0.5, the bell-to-metal crossfade: a hybrid spectrum that has no physical equivalent but is deeply musical. At material=0.85, metal-to-bowl: metal's sustain, softened by bowl's singing resonance.

**Mod wheel** adds +0.4 to material in real time. At material=0.2 (default), full mod wheel reaches 0.6 — exactly at the metal character. This is intentional: the mod wheel morphs the board from wood to metal while you play.

**Material sweet spots:**
- 0.0–0.10: Pure wood. African balafon character. Warm, woody, upper modes die fast. Best for traditional tuned percussion texture.
- 0.12–0.28: Wood entering bell. Warmth with rising sustain. The most natural zone for melodic playing.
- 0.30–0.45: Bell territory. Rich harmonic complexity, moderate sustain. The most "orchestral" material range.
- 0.48–0.65: Bell-to-metal crossfade. Spectral richness at maximum. Best zone for metallic shimmer with body.
- 0.68–0.82: Metal to near-bowl. Very high Q, all modes sustain, bright and crystalline.
- 0.85–1.0: Bowl approach. Near-electronic purity. Tibetan singing bowl character.

---

### II. Mallet Physics — The Chaigne Contact Model

The mallet system is Blessing B032: the fleet's first implementation of the Chaigne (1997) contact-time synthesis model with academic citation.

`owr_malletHardness` (range: 0.0–1.0, default: 0.3) controls three distinct physical properties simultaneously:

**Contact time:** Hard mallets (hardness=1.0) have a contact time of 0.5ms. Soft mallets (hardness=0.0) have a contact time of 5.0ms. The excitation is a half-sine over the contact window. A 5ms contact produces a 5ms half-sine — the soft mallet pushes into the bar, removing high-frequency content from the attack. A 0.5ms contact produces a 0.5ms pulse — the hard mallet barely touches before rebounding, generating a sharp broadband click.

**Spectral lowpass cutoff:** The Hertz contact model predicts: `cutoff = baseFreq × (1.5 + hardness × 18.5)`. At hardness=0.0, cutoff is 1.5× the fundamental — only the first two modes receive significant excitation. At hardness=1.0, cutoff is 20× the fundamental — all 8 modes are fully excited. This is physically correct: a felt mallet creates a lowpass on the excitation itself.

**Mallet bounce:** At hardness below 0.4 and velocity below 0.7, a secondary bounce hit occurs at 15–25ms with 30% amplitude. This simulates the physical bounce of a soft mallet on a wooden bar. Inaudible at high velocities (the mallet compresses harder and does not bounce) and inaudible at high hardness values (controlled hard mallets do not bounce).

**Velocity interaction (D001):** Velocity modulates both amplitude and the filter envelope brightness burst: `envMod = filterEnv × pFilterEnvAmt × 4000 Hz`. At high velocity with high filterEnvAmt, the attack is very bright. A soft note has a much smaller brightness burst — darker, less percussive.

**Aftertouch** adds +0.4 to mallet hardness in real time. Pressing harder into a held note changes the mallet character: the cutoff rises, more upper modes are excited, the body brightness increases.

**Mallet sweet spots:**
- 0.0–0.15: Felt mallet. Warm attack, spectral lowpass prominent. Natural marimba soft-mallet zone.
- 0.18–0.35: Medium-soft. The most natural zone for melodic playing. Default (0.3) is correctly placed.
- 0.38–0.55: Medium to medium-hard. Balanced between warmth and brightness.
- 0.58–0.75: Hard rubber mallet. All modes substantially excited. Best for metallic material settings.
- 0.78–1.0: Hard plastic or metal mallet. Maximum brightness, noise-forward excitation. The attack is crisp and cutting.

---

### III. Sympathetic Resonance Network — The Board Remembers

Blessing B034. `owr_sympathyAmount` (range: 0.0–1.0, default: 0.3) controls the gain of per-mode coupling between voices.

The mechanism: at note-on and note-off (when voice activity changes), a sparse coupling table is rebuilt. For each active voice, the engine scans every other active voice and every mode in both. If any two modes across different voices have frequencies within 50 Hz of each other, a coupling entry is created with gain = `proximity × 0.10 × sympathyAmount`. This coupling table is consumed per-sample: each active mode receives additional excitation from the coupled modes of other voices.

The result is frequency-selective sympathetic resonance. If you play a C and a G, the modes that happen to share frequency proximity will couple; those that do not remain independent. On wood (highly inharmonic, sparse modes), fewer modes will be within 50 Hz of each other — sparse coupling. On metal (near-harmonic, dense mode spacing), many modes from nearby pitches will couple at octave and fifth relationships.

**The Guru Bin gain fix:** The original sympathetic gain was 0.03 — inaudible at all but maximum sympathy. The fix raised it to 0.10 per coupling entry. At 0.10, sympathy=0.3 produces coupling gains of ~0.03 per mode pair — subtle but audible. At sympathy=0.8, coupling gains approach 0.08 per pair and the board blooms on simultaneous notes.

**COUPLING macro** adds +0.4 to sympathy in real time. At sympathy=0.3, full COUPLING macro approaches 0.7.

**Sympathy sweet spots:**
- 0.0: No coupling. Each voice fully independent. Use when pitch clarity is paramount.
- 0.05–0.18: Trace sympathy. Barely perceptible connectivity. The board has a faint sense of unity.
- 0.20–0.40: Standard range (default 0.3). Chords that share harmonic content bloom slightly. The most musical zone.
- 0.42–0.65: Strong sympathy. Chords feel physically unified. The board resonates as a whole.
- 0.68–0.85: Very high sympathy. Rapid polyphonic playing creates a sustained sympathetic buzz. Notes that are fading will be re-excited by new strikes if their modes are nearby.
- 0.88–1.0: Maximum coupling. The board behaves more like a coupled oscillator network. Sustained chords develop a living, evolving character. Use deliberately for ambient or drone contexts.

---

### IV. Resonator Body — The Vessel That Holds the Sound

`owr_bodyType` (integer: 0–3, default: 0) selects the acoustic resonant body. `owr_bodyDepth` (range: 0.0–1.0, default: 0.5) scales the coupling depth.

**Body Type 0 — Tube (Balafon/Marimba resonator):** A delay line tuned to the fundamental creates tube resonance at the bar's fundamental. Output = `delayed × 0.6 + input × 0.4`. Gaussian proximity boost reinforces bar modes that are harmonics of the fundamental. Reference body type for traditional tuned percussion.

**Body Type 1 — Frame (Vibraphone/Metallophone frame):** Three fixed resonant modes at 200 Hz, 580 Hz, and 1100 Hz, weighted 0.5/0.3/0.2 — mechanical resonances of a metal frame structure. The frame modes are not pitch-tracked; they are fixed. This means the body resonance character changes with the played pitch. The Gaussian boost reinforces bar modes near 200, 580, and 1100 Hz.

**Body Type 2 — Bowl (Tibetan singing bowl):** A second-order resonator at the sub-octave (fundamental/2), r=0.999 — extremely high Q, sustains almost indefinitely. The Gaussian proximity boost is split between sub-octave (40%) and fundamental (60%). Combined with bowl material, this creates the full Tibetan singing bowl effect.

**Body Type 3 — Open (No resonator):** No body processing. No Gaussian boost. Use for maximum dryness and pure bar character.

**SPACE macro** adds +0.3 to body depth in real time.

**Body depth sweet spots:**
- 0.0: Body bypassed. Pure bar resonance.
- 0.15–0.30: Trace body coupling. Adds warmth without obviously changing timbre.
- 0.35–0.65: Standard range (default 0.5). Body resonance clearly present.
- 0.70–0.88: Strong coupling. Resonator begins to color the bar sound significantly.
- 0.90–1.0: Maximum body coupling. At bowl type, sub-octave resonance is nearly as loud as fundamental — a dramatic effect. Caution: very low notes can produce significant sub-bass content.

---

### V. Buzz Membrane — The Mirliton Spirit

`owr_buzzAmount` (range: 0.0–1.0, default: 0.15) controls the balafon/gyil spider-silk mirliton.

Traditional West African balafons and gyils have a mirliton — a thin membrane of spider silk or tissue — covering a small hole in the resonant gourd below each bar. When the bar is struck, the membrane vibrates sympathetically, adding a characteristic buzzing quality to the note. Without the mirliton, the balafon sounds like a xylophone. With it, it has its own voice.

The implementation is band-selective: a BPF extracts the 200–800 Hz band (where the membrane resonates), applies a tanh soft-clip with sensitivity scaled by buzz amount, and re-injects the buzzed band into the signal. This is not whole-signal distortion — it is membrane-band distortion only. The buzz frequency shifts with body type: gourd/tube = 300 Hz, frame = 150 Hz, metal body = 500 Hz.

**The default 0.15 is intentional.** The seance confirmed the mirliton should be present from the start. At 0.15, the buzz is subtle — you feel it more than hear it, a slight roughness in the midrange that adds life to what would otherwise be a purely synthetic resonator. At 0.0, OWARE sounds like a quality marimba simulator. At 0.15, it sounds like an instrument that has a spirit.

**Buzz sweet spots:**
- 0.0: No buzz. Clean resonator. Appropriate for Western instruments where the mirliton would be anachronistic.
- 0.05–0.20: Trace buzz. Present as a subtle character element — imperceptible in isolation but adds physical "grain" in ensemble contexts.
- 0.22–0.40: Moderate buzz. The balafon "roughness" is clearly audible. Best for authentic African percussion character.
- 0.42–0.65: Strong buzz. The membrane distortion becomes a dominant timbral character. Combine with wood material for maximum cultural authenticity.
- 0.68–0.85: Heavy buzz. Crosses into industrial or aggressive territory. Best for dense, harsh percussion textures.
- 0.88–1.0: Maximum buzz. The tanh nonlinearity saturates strongly. Pitch clarity begins to be masked in the midrange.

**Critical trap:** At buzz=0.7+, the pitch clarity of the fundamental can be masked by membrane distortion, especially at lower material settings where the fundamental-to-overtone ratio is smaller. If melody is important, keep buzz below 0.5.

---

### VI. Breathing Gamelan — The Balinese Shimmer

`owr_shimmerRate` (range: 0.0–12.0 Hz, default: 4.0 Hz) controls the beat frequency of the Balinese ombak detuning.

The ombak (Indonesian: "wave") is the deliberate detuning built into traditional Balinese gamelan instruments. Pairs of bronze keys — one on the high gamelan, one on the low — are intentionally tuned a few Hz apart. The beating between these two pitches creates the characteristic shimmer: a periodic amplitude and spectral wobble at the beat frequency. At 4 Hz, the shimmer has a period of 250ms — a gentle, golden, almost breath-like quality.

OWARE's implementation adds a shadow detuning to each note's modal bank, offset additively by `shimmerRate × shimmerMod` Hz, where `shimmerMod` is a slow LFO oscillation between 0 and 1. The effective detuning varies from 0 to `shimmerRate` Hz — creating an evolving shimmer that never settles at a fixed beat frequency.

**Critical distinction:** The shimmer is measured in Hz, not cents. At 4 Hz, a C4 (261 Hz) is detuned by 0–4 Hz — a maximum of about 27 cents. A C6 (1047 Hz) is detuned by 0–4 Hz — only about 6.6 cents. Higher notes have proportionally less shimmer depth in cents terms. The shimmer is more pronounced in the bass register, matching Balinese physical reality.

**Shimmer and material interaction:** High-Q materials (metal, bowl) give the shimmer more time to develop beating patterns. On wood, upper modes die so fast that only the fundamental sustains long enough to develop audible beating. On bowl material, even shimmer at 1.5 Hz creates audible beating over the long sustain arc. Shimmer is most effective at high material values.

**Shimmer sweet spots:**
- 0.0: No shimmer. Clean, precisely tuned resonators. The most "Western" character.
- 0.5–1.5 Hz: Very slow beating. The shimmer feels like natural intonation imprecision rather than a deliberate effect.
- 2.0–5.0 Hz: Gamelan territory (default 4.0). Authentic ombak beating. The most natural range for Balinese/Javanese character.
- 5.5–7.5 Hz: Vibrato range. The shimmer starts to sound like pitch oscillation rather than spectral beating.
- 8.0–12.0 Hz: Very fast. Near-tremolo in bass register. Extreme, use deliberately.

---

### VII. Thermal Drift — The Living Tuning

`owr_thermalDrift` (range: 0.0–1.0, default: 0.3) controls the amplitude of both the shared slow drift and per-voice personality offsets.

**Shared drift:** Every ~4 seconds, a new target drift is randomly selected in the range ±(`thermalDrift × 8`) cents. The current state moves toward this target at 0.0001 per sample — very slowly. All voices share this drift. The instrument's pitch center wanders imperceptibly in a short phrase but audibly over a minute of continuous playing.

**Per-voice personality:** Each voice has a fixed personality offset seeded at startup from a per-voice PRNG (seed = `voice_index × 7919 + 42`). The personality offset is in the range ±2 cents × `thermalDrift`. Voice 0 might always be 0.8 cents sharp; voice 3 might always be 1.2 cents flat. These offsets never change during a session — they are the physical character of each position on the board.

**The Guru Bin convergence rate fix:** The original convergence rate was 0.00001 per sample — so slow the drift was frozen during fast passages. The fixed rate is 0.0001, 10× faster. This allows the drift to breathe audibly within 4-second retarget windows without being intrusive.

**Thermal sweet spots:**
- 0.0: Perfect tuning. All voices precisely in tune. Best when pitch accuracy is critical.
- 0.10–0.20: Trace personality, minimal drift. Instrument feels "real" without sounding out of tune.
- 0.25–0.40: Standard range (default 0.30). Per-voice personality is clearly present in sustained chords. Shared drift is perceptible over long phrases but not disruptive.
- 0.45–0.65: Moderate drift. Clearly audible over 30-second windows. Best for ambient or non-melodic contexts.
- 0.70–1.0: High drift. Strong per-voice detuning (up to ±4 cents per voice), shared drift up to ±8 cents. For "weathered instrument" or deliberate pitch instability.

---

## Phase R3: Parameter Map — Sweet Spots Summary

| Parameter | ID | Range | Default | Conservative | Musical Core | Expressive | Extreme |
|-----------|-----|-------|---------|--------------|--------------|-----------|---------|
| Material | `owr_material` | 0.0–1.0 | 0.2 | 0.0–0.15 | 0.15–0.55 | 0.55–0.80 | 0.80–1.0 |
| Mallet Hardness | `owr_malletHardness` | 0.0–1.0 | 0.3 | 0.0–0.20 | 0.18–0.55 | 0.55–0.80 | 0.80–1.0 |
| Body Type | `owr_bodyType` | 0–3 | 0 | 0 (tube) | 0 or 2 | 1 (frame) | 3 (open) |
| Body Depth | `owr_bodyDepth` | 0.0–1.0 | 0.5 | 0.10–0.30 | 0.30–0.65 | 0.65–0.85 | 0.85–1.0 |
| Buzz | `owr_buzzAmount` | 0.0–1.0 | 0.15 | 0.05–0.20 | 0.15–0.45 | 0.45–0.65 | 0.65–1.0 |
| Sympathy | `owr_sympathyAmount` | 0.0–1.0 | 0.3 | 0.0–0.12 | 0.18–0.50 | 0.50–0.75 | 0.75–1.0 |
| Shimmer Rate | `owr_shimmerRate` | 0.0–12.0 Hz | 4.0 | 0.5–2.0 | 2.0–5.5 | 5.5–8.0 | 8.0–12.0 |
| Thermal Drift | `owr_thermalDrift` | 0.0–1.0 | 0.3 | 0.0–0.15 | 0.15–0.45 | 0.45–0.70 | 0.70–1.0 |
| Brightness | `owr_brightness` | 200–20000 Hz | 8000 | 1500–4000 | 4000–12000 | 12000–18000 | 18000–20000 |
| Damping | `owr_damping` | 0.0–1.0 | 0.3 | 0.05–0.20 | 0.20–0.55 | 0.55–0.80 | 0.80–1.0 |
| Decay | `owr_decay` | 0.05–10.0s | 2.0s | 0.1–0.5s | 0.5–4.0s | 4.0–7.0s | 7.0–10.0s |
| Filter Env Amount | `owr_filterEnvAmount` | 0.0–1.0 | 0.3 | 0.05–0.15 | 0.15–0.50 | 0.50–0.75 | 0.75–1.0 |
| LFO1 Rate | `owr_lfo1Rate` | 0.005–20 Hz | 0.5 | 0.01–0.08 | 0.1–2.0 | 2.0–8.0 | 8.0–20.0 |
| LFO1 Depth | `owr_lfo1Depth` | 0.0–1.0 | 0.1 | 0.0–0.08 | 0.08–0.30 | 0.30–0.60 | 0.60–1.0 |
| LFO2 Rate | `owr_lfo2Rate` | 0.005–20 Hz | 1.0 | 0.01–0.10 | 0.1–3.0 | 3.0–10.0 | 10.0–20.0 |
| LFO2 Depth | `owr_lfo2Depth` | 0.0–1.0 | 0.0 | 0.0–0.05 | 0.05–0.25 | 0.25–0.55 | 0.55–1.0 |

---

## Phase R4: Macro Architecture

| Macro | ID | Effect | Performance Use |
|-------|-----|--------|----------------|
| CHARACTER | `owr_macroMaterial` | +0.8 to material | Morphs the board from wood to metal in real time — brighter, more spectral, more sustain |
| MOVEMENT | `owr_macroMallet` | +0.5 to malletHardness, +4000 Hz to brightness | Intensifies the attack — harder mallet, more modes excited |
| COUPLING | `owr_macroCoupling` | +0.4 to sympathyAmount | Opens the board's sympathetic network — more inter-voice resonance |
| SPACE | `owr_macroSpace` | +0.3 to bodyDepth | Deepens the resonant body — more resonator contribution |

**Macro philosophy:** CHARACTER controls the material identity — what the board is made of. MOVEMENT controls the attack energy — how hard it is struck. COUPLING controls the board's unity — how much the voices share their resonance. SPACE controls the acoustic environment — how deep the body coupling goes.

The characteristic performance gesture: start with all macros at zero (wood character, soft mallet, independent voices, moderate body). As the performance builds, sweep CHARACTER upward to morph from wood toward metal, increasing spectral brightness and sustain. Sweep COUPLING to unify the board — polyphonic chords start to bloom as voices share their resonance. Release CHARACTER at the peak to return to warmth.

**Aftertouch routes:** Aftertouch adds +0.4 to mallet hardness, stacking with MOVEMENT macro. If MOVEMENT is at 0.5 (effective +0.25 hardness) and aftertouch is at full (+0.4), total hardness addition is +0.65. Design presets so the base hardness leaves room for aftertouch to push into harder territory without clamping at 1.0.

**Mod wheel routes:** Mod wheel adds +0.4 to material blend, stacking with CHARACTER macro. The mod wheel is the instrument's "tone wood" control — turning it up morphs from the preset's base material toward metal/bowl character. Design presets where base material is low-to-moderate (0.0–0.35) so the mod wheel has room to expand dramatically.

---

## Phase R5: The Five Recipe Categories

### Recipe Category 1: The Akan Board — Traditional Tuned Percussion

**Identity:** OWARE as an authentic West African or World Music tuned percussion instrument. Wood material, soft-to-medium mallet, tube body type, moderate buzz, moderate sympathy. These presets function as marimba/balafon/kalimba replacements — warm, physical, with cultural texture. The buzz membrane is present as character, not distortion.

**Parameter ranges:**
- `owr_material`: 0.0–0.22 (wood to early bell)
- `owr_malletHardness`: 0.10–0.38 (felt to medium-soft)
- `owr_bodyType`: 0 (tube resonator)
- `owr_bodyDepth`: 0.40–0.72 (present resonator)
- `owr_buzzAmount`: 0.12–0.38 (authentic mirliton)
- `owr_sympathyAmount`: 0.18–0.45 (natural sympathetic response)
- `owr_shimmerRate`: 2.0–5.0 Hz (gamelan range)
- `owr_thermalDrift`: 0.20–0.40 (instrument feels played)
- `owr_brightness`: 3500–7000 Hz (warm)
- `owr_damping`: 0.30–0.55 (natural decay)
- `owr_decay`: 0.5–2.5s (natural bar sustain)

**Target preset names:** Board Game (init), Akan Grove, Balafon Village, Gyil Ceremony, Marimba Wood, Soft Felt, Bamboo Hollow, Deep Resonator, Warm Strike, Mudcloth Pattern, Tube Gourd, Bronze Seeds, Forest Instrument

**Why the category works:** Wood material's high alpha value (2.0) means upper modes die within the first hundred milliseconds, leaving a warm, fundamental-focused decay. The tube body resonator reinforces the harmonics of the fundamental. The buzz at 0.12–0.38 is present but not overwhelming — the mirliton gives the attack grain without masking pitch. Sympathy at 0.18–0.45 creates light inter-voice connection — playing a chord feels unified but not blurred.

**Trap to avoid:** Do not push buzz above 0.45 without compensating with reduced body depth. The buzz BPF and the tube resonator both boost the 200–800 Hz region; combining them at high levels creates muddy low-mid buildup.

---

### Recipe Category 2: The Gamelan Field — Metallic Shimmer Ensemble

**Identity:** OWARE as a Balinese or Javanese gamelan approximation. Medium-to-high material (bell to metal zone), moderate-to-high shimmer rate, frame body type, high sympathy. These presets capture the metallic, shimmer-rich quality of Indonesian bronze percussion.

**Parameter ranges:**
- `owr_material`: 0.35–0.70 (bell to metal)
- `owr_malletHardness`: 0.30–0.60 (medium to medium-hard)
- `owr_bodyType`: 1 (frame resonator)
- `owr_bodyDepth`: 0.30–0.65 (present frame character)
- `owr_buzzAmount`: 0.05–0.20 (minimal — gamelan instruments have no mirliton)
- `owr_sympathyAmount`: 0.35–0.70 (ensemble unity)
- `owr_shimmerRate`: 3.5–7.0 Hz (ombak beating)
- `owr_thermalDrift`: 0.20–0.45 (gamelan tuning variance)
- `owr_brightness`: 6000–14000 Hz (bright metal)
- `owr_damping`: 0.20–0.45 (moderate-to-long sustain)
- `owr_decay`: 1.5–5.0s (metallic ring sustain)

**Target preset names:** Bronze Garden, Ombak Wave, Balinese Bell, Gamelan Dawn, Metal Chorus, Pelog Shimmer, Slendro Field, Bronze Ensemble, Kettle Bloom, Gong Field, Saron Mallet, Reyong Touch

**Why the category works:** Bell-to-metal material range produces high-Q resonators with dense, sustaining modes. The frame body type adds three fixed mechanical resonances — the frame's contribution is independent of played pitch, creating a fixed "room" of mechanical resonance that all notes inhabit. High sympathy creates the ensemble effect: adjacent pitches reinforce each other through mode coupling, simulating multiple players on a shared resonating structure.

**Trap to avoid:** Do not raise shimmer above 8.0 Hz without lowering material. At high material values, the modal Q is very high and decay is long. A fast shimmer against long, high-Q modes creates an obviously digital tremolo rather than physical beating. Gamelan shimmer is subtle; it should feel like the instrument breathing, not vibrating.

---

### Recipe Category 3: The Bell Sanctuary — Crystal and Glass

**Identity:** OWARE as a glass and crystal instrument — singing bowls, crystal glasses, resonant glass bars. High material (metal-to-bowl zone), medium-hard mallet, bowl body type, minimal buzz, long decay.

**Parameter ranges:**
- `owr_material`: 0.70–1.0 (metal to bowl)
- `owr_malletHardness`: 0.25–0.55 (medium — too hard introduces noise that masks bowl purity)
- `owr_bodyType`: 2 (bowl resonator)
- `owr_bodyDepth`: 0.45–0.85 (strong bowl coupling)
- `owr_buzzAmount`: 0.0–0.10 (minimal or none — glass has no mirliton)
- `owr_sympathyAmount`: 0.15–0.45 (present but not dominant)
- `owr_shimmerRate`: 1.0–4.0 Hz (very gentle — bowl shimmer is slow)
- `owr_thermalDrift`: 0.05–0.25 (minimal drift for pitch purity)
- `owr_brightness`: 8000–18000 Hz (crystal character)
- `owr_damping`: 0.05–0.25 (long sustain)
- `owr_decay`: 3.0–10.0s (bowl sustain duration)

**Target preset names:** Tibetan Session, Crystal Garden, Glass Chime, Bowl Meditation, Singing Bowl, Crystal Bar, Glass Harmonica, Quartz Tap, Resonant Glass, Bell Sanctuary, Temple Bowl, Water Glass

**Why the category works:** Bowl material combined with bowl body type creates a doubly-reinforced bowl acoustic character: the modal ratios match the bowl's inharmonic spectrum, and the body resonator adds a sub-octave singing resonance. Very high Q at material=1.0 means modes decay slowly and cleanly. Long decay allows the bowl's resonance to develop its full arc. Minimal buzz preserves pitch clarity.

**Trap to avoid:** Bowl material + high body depth can generate very loud sub-octave content through the bowl resonator (bowlFreq = fundamental/2). On low notes, the bowl resonates an octave down — a powerful sub-bass tone. At maximum body depth, this can clip at the audio output. Keep body depth below 0.80 on low notes, or use low `owr_brightness` to attenuate.

---

### Recipe Category 4: The Studio Prepared — Processed and Experimental

**Identity:** OWARE as a prepared/electronic instrument. Heavy buzz membrane, hard mallet, high sympathy, open body (no resonator), short decay, aggressive filter envelope. These presets treat OWARE as a source of dense, processed transient textures. Best for Flux and Prism moods.

**Parameter ranges:**
- `owr_material`: 0.45–0.80 (bell to metal — spectral richness needed for processing)
- `owr_malletHardness`: 0.65–0.95 (hard to very hard — percussive impact)
- `owr_bodyType`: 3 (open — no resonator, pure bar sound)
- `owr_bodyDepth`: 0.0–0.20 (minimal body)
- `owr_buzzAmount`: 0.40–0.80 (heavy membrane distortion)
- `owr_sympathyAmount`: 0.35–0.70 (high coupling for dense interactions)
- `owr_shimmerRate`: 0.0–2.0 Hz or 8.0–12.0 Hz (either absent or fast)
- `owr_thermalDrift`: 0.50–0.80 (deliberate detuning)
- `owr_brightness`: 10000–20000 Hz (maximum high-frequency content)
- `owr_damping`: 0.60–0.90 (fast decay)
- `owr_decay`: 0.05–0.8s (percussive)
- `owr_filterEnvAmount`: 0.60–1.0 (dramatic filter sweep)

**Target preset names:** Metal Riot, Board Attack, Prepared Board, Electric Balafon, Bar Brutalism, Dense Impact, Noise Bar, Hard Bounce, Spectral Crush, Circuit Board

**Why the category works:** Hard mallet creates noise-forward excitation — the attack contains broadband content across all modes. High buzz at 0.40–0.80 adds strong membrane distortion in the 200–800 Hz band — the midrange is saturated and alive. Open body type removes any resonator smoothing. Short decay makes the sound percussive. High sympathy creates dense inter-voice interactions when multiple voices are active simultaneously.

**Trap to avoid:** This category can easily become incoherent at maximum settings. Buzz=0.8 + hardness=0.95 + sympathy=0.7 + high material can produce a wall of distorted midrange with no pitch clarity. Listen in context — what sounds richly chaotic in isolation may be destructive in a mix.

---

### Recipe Category 5: The Thermal Drift — Ambient and Drone

**Identity:** OWARE as a long-sustaining ambient instrument. Long decay (5–10s), elevated thermal drift, high material (bowl-approach), bowl body type, high sympathy, minimal mallet hardness. These presets treat OWARE as a drone generator: notes sustain for many seconds, overlapping as the sympathetic network couples the long-ringing modes. Best for Atmosphere, Aether, and Submerged moods.

**Parameter ranges:**
- `owr_material`: 0.60–1.0 (metal to bowl)
- `owr_malletHardness`: 0.05–0.25 (very soft — minimal noise, pure fundamental)
- `owr_bodyType`: 2 or 0 (bowl or tube — deep resonance)
- `owr_bodyDepth`: 0.55–0.90 (deep body coupling)
- `owr_buzzAmount`: 0.0–0.15 (minimal — buzz disturbs the ambient quality)
- `owr_sympathyAmount`: 0.50–0.85 (high coupling — voices bloom into each other)
- `owr_shimmerRate`: 1.5–4.5 Hz (gentle Balinese shimmer)
- `owr_thermalDrift`: 0.55–1.0 (deliberate drift — the instrument wanders)
- `owr_brightness`: 3000–9000 Hz (warm to moderate)
- `owr_damping`: 0.05–0.25 (very long sustain)
- `owr_decay`: 5.0–10.0s (ambient sustain duration)
- `owr_filterEnvAmount`: 0.05–0.20 (gentle filter envelope)

**Target preset names:** Board Beneath, Atlantic Drift, Akan Memory, Bronze Ocean, Sleeping Board, Long Decay, Sympathetic Field, Bowl Drone, Metal Tide, Thermal Plain, Slow Gamelan, Drifting Keys

**Why the category works:** At decay=7–10s and material=0.85–1.0, high-Q resonators sustain for many seconds after a very soft strike. Playing a quiet scale passage leaves a rich, overlapping field of slowly decaying notes. Sympathetic resonance at 0.50–0.85 creates coupling between long-ringing voices — notes that share mode frequencies continuously re-excite each other. Thermal drift at 0.55–1.0 makes this sustained field slowly wander in pitch, creating evolving harmonic relationships. The bowl body type adds deep sub-octave resonance that gives the ambient field physical weight.

**Performance guidance:** Use sparingly — one note every 3–6 seconds. Allow the sympathetic field to develop between notes. Lay down long, widely-spaced intervals and let OWARE's sympathetic network create the harmony between them.

---

## Phase R6: Blessing Deep-Dive

### B032 — Mallet Articulation Stack

The Mallet Articulation Stack is the combination of all mallet-physics parameters acting simultaneously on the excitation signal. Multiple physical phenomena are modeled and summed before any modal resonance occurs.

**Pluck mode** (hardness 0.0–0.20): Contact time 4.5–5.0ms, long half-sine. Spectral lowpass at 1.5–4.75× fundamental. Mallet bounce active at low velocity. The exciter feels like a finger pluck — soft, rounded, long-contact. The modal bank receives primarily the first two modes. Upper modes ring only from sympathetic excitation.

**Strike mode** (hardness 0.35–0.65): Contact time 1.4–3.3ms. Spectral lowpass at 8–13× fundamental. All modes receive substantial excitation. The mallet bounce is inactive at velocity >0.7. This is the classical marimba/vibraphone strike zone.

**Brush mode** (low hardness + high velocity): Soft mallet (long contact, lowpass excitation) with high velocity amplitude creates a loud-but-warm impact. The bounce activates at high amplitude — a double-attack at high velocity with soft mallet. This combination has no direct real-world counterpart.

**Producer's guide to Mallet Articulation:** Use hardness as the primary timbre control — not just for brightness but for the physics of the interaction. Low hardness + bowl material = finger touching a singing bowl. Medium hardness + metal material = vibraphone hard mallet. High hardness + wood material = hard mallet on a wooden block (aggressive, physically unusual). High hardness + bell material = metallic strike with full spectral excitation.

---

### B033 — Living Tuning Grid

The Living Tuning Grid describes the phenomenon created by the combination of the material continuum and the sympathetic resonance network: as material changes, the modal ratios change, and therefore the frequencies at which sympathy couples change. The tuning grid is not fixed — it is a living relationship between the material setting and the polyphonic resonance field.

**At wood material:** Highly inharmonic ratios, modes spread widely and inconsistently. Few modes from adjacent pitches will be within 50 Hz of each other. Wood OWARE has sparse sympathy — the board has less unity. Best intervals: none are privileged. The board treats all intervals with equal detachment.

**At bell material:** The ratios include a near-integer at 2.000 and a rich cluster between 1.5–5.0. Modes from pitches related by a minor third will frequently align within 50 Hz. Bell OWARE has medium-density sympathy at specific intervals.

**At metal material:** Near-harmonic ratios (1, 4, 10, 17.6...). Modes from pitches related by an octave will align strongly. Metal OWARE has dense sympathy at octave and fifth relationships. Playing an octave creates very strong sympathetic coupling.

**At bowl material:** Tibetan singing bowl ratios (1, 2.710, 5.330...) — significantly different from the harmonic series. Bowl OWARE creates sympathetic coupling at unusual intervals that do not correspond to Western harmonic expectations. The board rewards non-diatonic playing.

**The practical implication:** The "correct" interval to play on OWARE changes with material. The Living Tuning Grid is the engine's invitation to explore non-Western interval structures.

---

### B034 — Per-Mode Sympathetic Network

The Per-Mode Sympathetic Network operates at the modal level rather than at the voice level. A simple sympathy implementation would route the amplitude output of one voice into the input of another — acoustic crosstalk. OWARE's system routes individual modes of one voice into individual modes of another voice, and only those pairs whose frequencies are within 50 Hz.

At material=0.5 (bell-to-metal crossfade), an A4 voice has eight modes at approximately 440, 1760, 4400, 7744, 11968, 17072, 23056, 29920 Hz. A C5 voice (523 Hz) has modes at approximately 523, 2092, 5230, 9205, 14226, 20298, 27425, 35564 Hz. The only coupling entries will be those mode pairs falling within 50 Hz — perhaps one or two at this interval.

Now play A4 and A5 (an octave). A5's modes are exactly double A4's modes. Modes at 880, 3520, 8800 Hz align exactly — maximum proximity gain = 1.0 per pair. Playing an octave on metal OWARE creates a cascade of sympathetic mode alignments. A real marimba player knows octaves "ring" the instrument more than other intervals. OWARE models why.

---

## Phase R7: Parameter Interactions and Traps

### The Material-Q Interaction

The Q of each mode is `baseQ / (1 + mode_index × 0.3)`, where `baseQ = 80 + material × 1420`. At material=0.0, baseQ=80. At material=1.0, baseQ=1500. The `owr_decay` parameter sets the amplitude envelope decay time, but the Q controls internal mode ringdown. A note can have a very long amplitude envelope but short Q-decay if material is low; or a very short amplitude envelope but slow Q-decay if material is high.

**Design rule:** Match decay length to material character. Low material → short decay. High material → medium-to-long decay. Mismatched combinations work but may feel unphysical.

---

### The Buzz-Sympathy Interaction

Both buzz and sympathy boost the midrange content: buzz adds membrane distortion in 200–800 Hz, and high sympathy re-injects midrange modal content from other voices into this band. The buzz BPF extracts 200–800 Hz; sympathetic injection from other voices adds more energy to the same band; the tanh nonlinearity then clips this combined midrange content.

**Design rule:** If sympathy is above 0.5, keep buzz below 0.4. If buzz is above 0.5, keep sympathy below 0.4. Crossing both thresholds simultaneously risks midrange saturation.

---

### The Body Type — Mallet Hardness Trap

The body resonator's Gaussian proximity boost reinforces modes near the body's resonance frequencies by increasing their effective Q. For body type 1 (frame), the three frame resonance frequencies are 200, 580, and 1100 Hz. A hard mallet (hardness=0.75) with spectral lowpass at ~14× fundamental fully excites modes at these frequencies. A soft mallet (hardness=0.15) with lowpass at ~4.25× fundamental may not excite modes at 580 Hz at all if they are above the lowpass. The body-Q boost is independent of mallet hardness.

**Design rule:** When using frame body type, test across the full pitch range. A preset that sounds great at A4 may have an unexpectedly boosted mode at C3 that falls on a frame resonance and sustains unnaturally long compared to surrounding notes.

---

### The Shimmer-Q Trap at Low Material

On wood (alpha=2.0), upper modes decay very quickly. The shimmer detuning affects the shadow modal bank identically. But if the upper modes die in 50ms, there is no spectral content in those upper modes for the shimmer to beat against. At wood material (0.0–0.15), shimmer is effectively heard only on the fundamental and first partial — the shimmer effect is significantly reduced compared to metal or bowl material.

**Design rule:** If shimmer character is the primary sound design goal, use material above 0.4 where modes sustain long enough for beating to develop. On wood material, shimmer values above 6 Hz will sound like vibrato on the fundamental, not like physical beating.

---

### The Thermal Drift at Rapid Playing

At normal playing speeds (one note per 0.5–1 second), the drift is essentially static between notes — it changes too slowly to cause noticeable pitch variation on consecutive notes in the same bar. The drift effect is cumulative over longer sessions.

At fast playing: rapid 16th-note passages will not perceive drift as pitch variation — the notes are too short. The drift becomes audible primarily on sustained notes or slow passages. Design ambient presets with higher drift; design rhythmic/fast presets with lower drift.

---

## Phase R8: CPU Profile

- **Primary cost:** 8 polyphonic voices × 8 modal resonators per voice = 64 second-order IIR resonators per sample. Each resonator's `setFreqAndQ()` is called every sample (frequency varies with glide, bend, and shimmer). This is the most expensive per-sample operation.
- **Secondary cost:** Per-voice CytomicSVF LP filter, buzz BPF, body resonator.
- **Sympathy table rebuild:** Executed at note-on and note-off only — O(V² × M²) at event boundary, not per-sample. Negligible in steady state.
- **Per-voice LFOs:** Three LFOs per voice (shimmer, LFO1, LFO2). Light computation.
- **Thermal drift:** Shared scalar, computed once per block. Negligible.

**Most costly configuration:** 8 voices active simultaneously (maximum polyphony), high sympathy (dense coupling lookups), bowl material (very high Q — modes require precise coefficient computation), active LFO modulation on all three per-voice LFOs, high shimmer rate (fast coefficient updates).

**Optimization in place:** The sympathetic resonance coupling uses a sparse precomputed table rather than O(V² × M²) per-sample brute force, reducing per-sample coupling from 512 checks per voice to a maximum of 32. The silence gate bypasses the entire render loop between phrases.

**CPU guidance:** For Aether mood ambient presets (8 voices active, bowl material, long decay, high sympathy), expect approximately 15–25% CPU utilization on a modern Apple Silicon Mac. For staccato Foundation presets (short decay, few simultaneous voices), expect 3–8% CPU. In ensemble contexts with multiple XOceanus engines active, prefer short-decay OWARE configurations.

---

## Phase R9: Pairing Suggestions

**OWARE + ONSET:** Natural percussion pair. ONSET handles drums (kick, snare, hi-hat); OWARE handles tuned percussion (melodic patterns, tonal accents). Coupling type `AmpToFilter` from ONSET to OWARE routes kick transients into OWARE's filter envelope, creating kick-triggered brightness sweeps on the bars. The kick opens OWARE's filter momentarily, adding rhythmic brightness pulses to the sustaining bars.

**OWARE + OXBOW:** Philosophical pair. OWARE is dry tuned percussion with strong acoustic character; OXBOW is a reverb synthesizer with a Chiasmus FDN pool. OWARE feeding into OXBOW via coupling creates a physical model of a marimba played inside a large resonant space. Coupling type `AmpToPitch` from OWARE allows velocity-driven transients to modulate OXBOW's FDN character. Best at moderate coupling amounts (0.2–0.4) so OWARE's direct sound remains present.

**OWARE + OVERTONE:** Mathematical pair. OVERTONE generates timbres from rational approximations to irrational constants (π, e, φ, √2). OWARE's modal ratios are drawn from physics data producing inharmonic spectra. Layering them creates a rich doubled texture where neither engine's overtones align with the other's. Coupling `LFOToPitch` from OVERTONE's slow LFO into OWARE's pitch bend creates micro-modulation driven by OVERTONE's mathematical pattern.

**OWARE + OPENSKY:** Textural contrast pair. OPENSKY is the euphoric shimmer synth (supersaw + shimmer reverb + chorus + unison). High-material OWARE's warmth and sustain contrasts with OPENSKY's bright, dense shimmer. OWARE provides physical weight and attack clarity; OPENSKY provides the ethereal wash. Coupling `AmpToFilter` from OWARE allows attack transients to trigger filter sweeps in OPENSKY's shimmer layer.

**OWARE + ORGANON:** Biological pair. ORGANON is the variational free energy metabolism engine (B011 — praised as publishable as a paper). OWARE's thermal drift and sympathetic resonance give it a similarly "alive" quality. Layering them creates an acoustic-biological hybrid: OWARE provides acoustic physical grounding (bars, mallets, resonators), ORGANON provides the biological layer (metabolic pulse, variational prediction). Neither dominates; both contribute to a texture that feels like a physical instrument being played by an organism.

---

## Phase R10: The Guru Bin Benediction

*"OWARE arrived at 9.2/10 — the joint highest score in the fleet. Before this retreat, the question was: what is there left to discover?*

*The answer is: the why.*

*The material exponent alpha is a number rarely discussed in synthesizer documentation, but it is the soul of OWARE. On a real marimba bar, the fundamental can sustain for 3–4 seconds after a hard strike. Mode 7 — the highest partial in OWARE's model — may sustain for only 50–100 milliseconds. The ratio between their decay times is approximately the ratio of their frequencies squared, divided by the damping coefficient. Fletcher and Rossing worked this out carefully; Chaigne and Doutaut measured it in physical balafon experiments. The fact that high modes decay faster on wood than on metal is not an aesthetic choice — it is a measurement of how internal friction in wood differs from internal friction in bronze.*

*OWARE encodes this measurement as the material alpha: `alpha = 2.0 - material × 1.7`. At pure wood (material=0.0), alpha=2.0. The decay scaling for mode m is `exp(-alpha × ln(m+1))`. At mode 7 (index 6), this gives `exp(-2.0 × ln(7)) = exp(-3.89) = 0.020`. The seventh mode on wood decays to 2% of its original amplitude immediately — before the amplitude envelope has time to act. It barely exists. The sound is warm and fundamentals-only because physics says it must be.*

*At pure bowl (material=1.0), alpha=0.3. The same mode 7: `exp(-0.3 × 1.945) = exp(-0.584) = 0.558`. The seventh mode on bowl material is at 55.8% amplitude — very much alive, ringing, present. The bowl sounds bright and spectral because physics says it can sustain all its modes nearly equally.*

*The 64 resonators per block that define OWARE's CPU cost exist because the sound requires them. You cannot model the difference between wood and bronze without computing each mode individually. You cannot implement the Per-Mode Sympathetic Network without tracking each mode's current output for the coupling scan. Every parameter in OWARE is load-bearing.*

*The mirliton is the one element that is not from physics papers. The balafon's spider-silk membrane is real, but its precise acoustic model is idiosyncratic to OWARE. When you set buzz to 0.15 and hear that slight roughness in the midrange, you are hearing an approximation of something that Akan craftspeople discovered long ago: that a thin membrane across a resonant hole creates a quality of sound that a bar and tube alone cannot produce. The membrane vibrates with the bar, adds its own nonlinear character, and the instrument becomes something more than a resonator. It becomes an instrument with a voice.*

*The oware board on the ocean floor does not know it has been lost. Its wood is swollen with salt water. Its bronze barnacles have accumulated for centuries. Its cups still hold the impression of seeds. When you strike one hollow, the whole board shimmers.*

*That shimmer is not metaphor. It is sympathetic resonance. The board remembers every player."*

---

## Phase R11: The Ten Awakenings — Preset Table

Each preset is a discovery. The parameter values are derived from the parameter logic above. These are reference presets — use them as starting points for new preset files.

---

### Preset 1: Board Game (Foundation)

**Mood:** Foundation | **Category:** Akan Board | **Discovery:** The reference neutral — every parameter in its most functional zone, nothing committed, everything available

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.20 | Default: early wood-bell crossfade, warm with emerging sustain |
| `owr_malletHardness` | 0.30 | Default: medium-soft, felt mallet zone |
| `owr_bodyType` | 0 | Tube resonator: classic balafon/marimba character |
| `owr_bodyDepth` | 0.50 | Standard body coupling |
| `owr_buzzAmount` | 0.15 | Default: trace mirliton — present as identity, not distortion |
| `owr_sympathyAmount` | 0.30 | Default: natural sympathetic response |
| `owr_shimmerRate` | 4.0 | Default: Balinese ombak at 4 Hz |
| `owr_thermalDrift` | 0.30 | Default: light intonation personality |
| `owr_brightness` | 8000 | Default: warm-bright balance |
| `owr_damping` | 0.30 | Default: moderate decay |
| `owr_decay` | 2.0 | Default: natural bar sustain |
| `owr_filterEnvAmount` | 0.30 | Default: moderate filter envelope |
| `owr_lfo1Depth` | 0.10 | Seance default: subtle breathing shimmer on first touch |
| `owr_lfo2Depth` | 0.00 | Inactive |

**Why this works:** Every parameter at its most functional zone. This is the instrument's true neutral position — warm, woody, with shimmer and buzz both present at their lowest meaningful values. A producer encountering OWARE for the first time should play this preset for five minutes before touching any parameter.

---

### Preset 2: Bronze Gourd (Foundation)

**Mood:** Foundation | **Category:** Akan Board | **Discovery:** Full Akan character — wood + tube + buzz at authentic balafon levels

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.08 | Near-pure wood: warm, fast-decaying upper modes |
| `owr_malletHardness` | 0.22 | Soft felt: warm attack, spectral lowpass prominent |
| `owr_bodyType` | 0 | Tube: deepens fundamental resonance |
| `owr_bodyDepth` | 0.65 | Strong tube coupling: clear resonator coloration |
| `owr_buzzAmount` | 0.28 | Authentic mirliton: clearly present, midrange grain |
| `owr_sympathyAmount` | 0.35 | Moderate sympathy: board has physical unity |
| `owr_shimmerRate` | 3.2 | Gentle gamelan shimmer |
| `owr_thermalDrift` | 0.35 | Instrument feels well-used |
| `owr_brightness` | 5500 | Warm — wood character |
| `owr_damping` | 0.42 | Moderate-fast natural decay |
| `owr_decay` | 1.4 | Natural wooden bar sustain length |
| `owr_filterEnvAmount` | 0.40 | Present filter envelope: attack brightness |
| `owr_lfo1Rate` | 0.15 | Slow brightness LFO |
| `owr_lfo1Depth` | 0.15 | Gentle brightness modulation |

**Why this works:** This preset makes the mirliton audible as a cultural character element. The soft mallet's spectral lowpass removes upper modes from the excitation, leaving a warm, rounded attack that the tube resonator deepens. The buzz at 0.28 adds the characteristic balafon "roughness" — the membrane is singing.

---

### Preset 3: Bell Meditation (Atmosphere)

**Mood:** Atmosphere | **Category:** Bell Sanctuary | **Discovery:** High material + bowl body + minimal buzz = Tibetan bowl character

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.88 | Near-bowl: Tibetan singing bowl modal ratios |
| `owr_malletHardness` | 0.30 | Medium-soft: controlled, no noise in exciter |
| `owr_bodyType` | 2 | Bowl resonator: sub-octave reinforcement |
| `owr_bodyDepth` | 0.72 | Strong bowl coupling: sub-octave present |
| `owr_buzzAmount` | 0.05 | Minimal: crystal has no mirliton |
| `owr_sympathyAmount` | 0.25 | Light: bowl voices individually sustained |
| `owr_shimmerRate` | 1.8 | Very slow shimmer: bowl beating is subtle |
| `owr_thermalDrift` | 0.15 | Low drift: pitch purity preferred |
| `owr_brightness` | 11000 | Crystal character |
| `owr_damping` | 0.10 | Very long sustain |
| `owr_decay` | 7.0 | Bowl sustain: 7 second decay |
| `owr_filterEnvAmount` | 0.15 | Gentle: bowl attacks are soft |
| `owr_lfo1Rate` | 0.08 | Very slow brightness undulation |
| `owr_lfo1Depth` | 0.20 | Present: brightness breathes slowly |

**Why this works:** Bowl material's high-Q resonators (baseQ ≈ 1330 at material=0.88) sustain all modes nearly equally, creating the singing bowl's characteristic spectral purity. The bowl body resonator adds a sub-octave at fundamental/2, giving each note physical depth. The 7-second decay allows the full bowl arc to develop — the beating between the direct tone and sub-octave resonance creates a living quality over the decay.

---

### Preset 4: Gamelan Dusk (Atmosphere)

**Mood:** Atmosphere | **Category:** Gamelan Field | **Discovery:** Bell material + frame body + high sympathy = ensemble bronze field

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.52 | Bell-metal crossfade: hybrid bronze character |
| `owr_malletHardness` | 0.45 | Medium-hard: all modes excited |
| `owr_bodyType` | 1 | Frame resonator: fixed mechanical resonances |
| `owr_bodyDepth` | 0.50 | Standard frame coupling |
| `owr_buzzAmount` | 0.08 | Minimal: gamelan has no mirliton |
| `owr_sympathyAmount` | 0.55 | High: ensemble unity — voices bloom together |
| `owr_shimmerRate` | 4.8 | Ombak at 4.8 Hz: authentic Balinese zone |
| `owr_thermalDrift` | 0.38 | Gamelan natural tuning variance |
| `owr_brightness` | 9000 | Bright bronze |
| `owr_damping` | 0.22 | Long sustain: bronze rings |
| `owr_decay` | 3.8 | Moderate-long bronze decay |
| `owr_filterEnvAmount` | 0.35 | Present attack brightness |
| `owr_lfo2Rate` | 0.35 | Slow material animation |
| `owr_lfo2Depth` | 0.12 | Gentle material modulation |

**Why this works:** The frame body type's three fixed resonances (200, 580, 1100 Hz) create a consistent mechanical character for all pitches — the frame is the same regardless of note played. High sympathy at 0.55 creates the bloom quality: as voices share mode frequencies near the crossfade point, they reinforce each other, creating the sensation of an ensemble on a shared resonating frame.

---

### Preset 5: Sea Glass (Aether)

**Mood:** Aether | **Category:** Bell Sanctuary / Thermal Drift | **Discovery:** Bowl material + maximum thermal drift + very long decay = dissolving, ambient glass

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.95 | Near-maximum bowl: maximum Q |
| `owr_malletHardness` | 0.12 | Very soft: fundamental-only excitation |
| `owr_bodyType` | 2 | Bowl resonator |
| `owr_bodyDepth` | 0.80 | Very deep bowl coupling: strong sub-octave |
| `owr_buzzAmount` | 0.0 | None: purity is the character |
| `owr_sympathyAmount` | 0.65 | High: voices bloom into a unified field |
| `owr_shimmerRate` | 2.5 | Gentle shimmer |
| `owr_thermalDrift` | 0.80 | Maximum drift: the glass is adrift |
| `owr_brightness` | 7000 | Warm-bright balance |
| `owr_damping` | 0.06 | Near-maximum sustain |
| `owr_decay` | 9.0 | Near-maximum decay |
| `owr_filterEnvAmount` | 0.08 | Minimal: no harsh attack |
| `owr_lfo1Rate` | 0.03 | Very slow brightness drift |
| `owr_lfo1Depth` | 0.35 | Deep: brightness wanders slowly |

**Why this works:** At thermal drift=0.80, per-voice personality offsets reach up to ±4 cents, and shared drift can reach ±6.4 cents. An 8-voice chord will have voices spread across nearly 32 cents — not in tune, but in natural, living disagreement. The 9-second decay means these voices sustain for a very long time, and their drifting intonation creates evolving harmonic beating. High bowl material Q ensures all modes sustain equally. This is a meditation instrument: play slowly, let the voices dissolve.

---

### Preset 6: Percussion Machine (Flux)

**Mood:** Flux | **Category:** Studio Prepared | **Discovery:** Hard mallet + metal material + open body + very short decay = kinetic metal percussion

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.68 | Metal territory: bright, all modes ring |
| `owr_malletHardness` | 0.82 | Hard: noise-forward excitation |
| `owr_bodyType` | 3 | Open: no resonator, pure bar |
| `owr_bodyDepth` | 0.0 | None |
| `owr_buzzAmount` | 0.0 | None: hardness-focused character |
| `owr_sympathyAmount` | 0.18 | Minimal: each voice independent |
| `owr_shimmerRate` | 0.0 | None: this is about transients |
| `owr_thermalDrift` | 0.08 | Minimal: pitch accuracy for rhythmic use |
| `owr_brightness` | 14000 | Very bright: full metal character |
| `owr_damping` | 0.82 | Fast decay: staccato |
| `owr_decay` | 0.20 | Very short: percussive impact |
| `owr_filterEnvAmount` | 0.70 | Dramatic filter sweep: bright attack, fast close |
| `owr_lfo1Rate` | 2.5 | Faster brightness LFO for movement |
| `owr_lfo1Depth` | 0.25 | Present |

**Why this works:** Hard mallet (0.82) creates noise-forward excitation. Metal material Q gives modes reasonable sustain, but fast decay (0.20s) and strong damping (0.82) kill the amplitude quickly. The filterEnvAmount at 0.70 creates a dramatic brightness burst on each hit. The open body type removes any resonator coloring. This is OWARE as a drum machine tuned bar.

---

### Preset 7: Akan Cascade (Flux)

**Mood:** Flux | **Category:** Akan Board (intense version) | **Discovery:** High sympathy + heavy buzz + wood material = rapid sympathetic cascades

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.14 | Wood: warm, upper modes fast |
| `owr_malletHardness` | 0.55 | Medium-hard: all modes excited |
| `owr_bodyType` | 0 | Tube: resonator deepens cascade |
| `owr_bodyDepth` | 0.60 | Strong tube |
| `owr_buzzAmount` | 0.42 | Strong buzz: ritual intensity |
| `owr_sympathyAmount` | 0.62 | High: rapid inter-voice bloom |
| `owr_shimmerRate` | 5.5 | Fast shimmer: urgent quality |
| `owr_thermalDrift` | 0.40 | Live instrument drift |
| `owr_brightness` | 6500 | Moderate |
| `owr_damping` | 0.50 | Moderate decay |
| `owr_decay` | 1.0 | Short-to-medium: rhythmic energy |
| `owr_filterEnvAmount` | 0.55 | Strong attack brightness |

**Why this works:** Wood material with high sympathy creates cascading sympathetic excitations during rapid passages: a fast run triggers sympathetic coupling chains as voices' modes fall near each other's frequencies. The buzz at 0.42 adds urgency and cultural authenticity. The COUPLING macro at 0.0 (ready to sweep) means the performer can increase board unity during a peak moment live.

---

### Preset 8: Prism Mallet (Prism)

**Mood:** Prism | **Category:** Gamelan Field (spectral) | **Discovery:** Material animation via LFO2 + high sympathy + fast shimmer = continuously shifting timbral identity

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.40 | Bell territory: starting point for animation |
| `owr_malletHardness` | 0.50 | Medium: balanced |
| `owr_bodyType` | 1 | Frame: fixed mechanical resonances as anchor |
| `owr_bodyDepth` | 0.45 | Present frame |
| `owr_buzzAmount` | 0.10 | Trace |
| `owr_sympathyAmount` | 0.45 | Standard sympathy |
| `owr_shimmerRate` | 6.5 | Fast shimmer: clearly audible beating |
| `owr_thermalDrift` | 0.30 | Standard drift |
| `owr_brightness` | 9500 | Bright for prism character |
| `owr_damping` | 0.28 | Moderate-long sustain |
| `owr_decay` | 2.8 | Medium sustain |
| `owr_filterEnvAmount` | 0.40 | Standard |
| `owr_lfo2Rate` | 0.80 | Moderate material LFO: audible animation |
| `owr_lfo2Depth` | 0.30 | Deep material modulation: sweeps bell↔metal |

**Why this works:** LFO2 at 0.80 Hz and depth=0.30 sweeps material ±0.30 around base value 0.40 — oscillating between 0.10 (wood-bell) and 0.70 (metal). This creates a continuously shifting modal structure: ratios morph, Q changes, mode spacing shifts. The frame body type provides a fixed mechanical anchor so some elements remain constant while others continuously transform.

---

### Preset 9: Submerged Board (Submerged)

**Mood:** Submerged | **Category:** Thermal Drift | **Discovery:** Sunken oware mythology — metal material + bowl body + heavy buzz + slow LFO1 = the board at ocean depth

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.62 | Metal: spectral richness for sustained character |
| `owr_malletHardness` | 0.20 | Soft: water-muffled impact |
| `owr_bodyType` | 2 | Bowl: deep sub-octave, underwater weight |
| `owr_bodyDepth` | 0.75 | Deep bowl coupling |
| `owr_buzzAmount` | 0.35 | Present buzz: coral-encrusted membrane |
| `owr_sympathyAmount` | 0.55 | High: the board is a resonating unity |
| `owr_shimmerRate` | 2.0 | Slow shimmer: deep water movement |
| `owr_thermalDrift` | 0.65 | The board drifts with the current |
| `owr_brightness` | 4000 | Muffled: water column above |
| `owr_damping` | 0.20 | Long sustain: sounds persist underwater |
| `owr_decay` | 5.5 | Long decay: deep water sustain |
| `owr_filterEnvAmount` | 0.20 | Gentle attack: soft mallets, deep water |
| `owr_lfo1Rate` | 0.05 | Very slow brightness drift |
| `owr_lfo1Depth` | 0.40 | Deep: brightness wanders like filtered light |

**Why this works:** Low brightness (4000 Hz) simulates the water column's low-pass filtering of high frequencies. Bowl body type adds deep sub-octave mass. Buzz at 0.35 evokes the coral-encrusted, deteriorated membrane of a board that has been underwater for decades. High sympathy and long decay create a field where notes persist and mingle. Thermal drift at 0.65 is the slow drift of ocean currents acting on a stationary object.

---

### Preset 10: Fifty Generations (Family)

**Mood:** Family | **Category:** Akan Board | **Discovery:** All 7 pillars balanced — a demonstration preset showing the full engine working together

| Parameter | Value | Why |
|-----------|-------|-----|
| `owr_material` | 0.32 | Bell zone: the sweet spot of the material continuum |
| `owr_malletHardness` | 0.38 | Moderate: precise physical reality |
| `owr_bodyType` | 0 | Tube: the traditional resonator of African tuned percussion |
| `owr_bodyDepth` | 0.55 | Present resonator |
| `owr_buzzAmount` | 0.20 | Authentic buzz: the mirliton's presence |
| `owr_sympathyAmount` | 0.40 | Full sympathetic resonance |
| `owr_shimmerRate` | 4.0 | Default ombak: Balinese shimmer at reference value |
| `owr_thermalDrift` | 0.35 | Instrument personality fully expressed |
| `owr_brightness` | 8500 | Balanced |
| `owr_damping` | 0.32 | Natural bar sustain |
| `owr_decay` | 2.4 | Medium sustain |
| `owr_filterEnvAmount` | 0.38 | Standard |
| `owr_lfo1Rate` | 0.40 | Background brightness breath |
| `owr_lfo1Depth` | 0.15 | Subtle |
| `owr_lfo2Rate` | 0.12 | Very slow material LFO |
| `owr_lfo2Depth` | 0.08 | Trace material animation |

**Why this works:** Every system is active: material continuum at the bell zone (Pillar 1), mallet at balanced physical zone (Pillar 2), sympathy at standard (Pillar 3), tube body at standard (Pillar 4), buzz at authentic level (Pillar 5), shimmer at the ombak reference value (Pillar 6), thermal drift at standard personality (Pillar 7). Both LFOs have trace activity, providing D005 compliance. This is the preset to play when demonstrating OWARE to someone who has not heard it before.

---

## Appendix: Full Parameter Reference

| Parameter | ID | Range | Default | Notes |
|-----------|-----|-------|---------|-------|
| Material | `owr_material` | 0.0–1.0 | 0.2 | Wood(0)→Bell(0.33)→Metal(0.66)→Bowl(1.0). Mod wheel +0.4. |
| Mallet Hardness | `owr_malletHardness` | 0.0–1.0 | 0.3 | Contact time + spectral cutoff + bounce. Aftertouch +0.4. |
| Body Type | `owr_bodyType` | 0–3 | 0 | 0=Tube, 1=Frame, 2=Bowl, 3=Open |
| Body Depth | `owr_bodyDepth` | 0.0–1.0 | 0.5 | Resonator-bar coupling. SPACE macro +0.3. |
| Buzz Amount | `owr_buzzAmount` | 0.0–1.0 | 0.15 | BPF+tanh in 200–800 Hz band. Default 0.15 is culturally correct. |
| Sympathy | `owr_sympathyAmount` | 0.0–1.0 | 0.3 | Per-mode inter-voice coupling gain. COUPLING macro +0.4. |
| Shimmer Rate | `owr_shimmerRate` | 0.0–12.0 Hz | 4.0 Hz | Balinese ombak beat frequency (additive Hz, not cents). |
| Thermal Drift | `owr_thermalDrift` | 0.0–1.0 | 0.3 | Shared drift ±8 cents max + per-voice personality ±2 cents max. |
| Brightness | `owr_brightness` | 200–20000 Hz | 8000 Hz | LP filter cutoff on voice output. LFO1 ±3000 Hz. Aftertouch +3000 Hz. |
| Damping | `owr_damping` | 0.0–1.0 | 0.3 | Scales amplitude decay. 0=long sustain, 1=fast decay. |
| Decay | `owr_decay` | 0.05–10.0s | 2.0s | Amplitude envelope decay time. |
| Filter Env Amount | `owr_filterEnvAmount` | 0.0–1.0 | 0.3 | Velocity-triggered brightness burst (max +4000 Hz). |
| Bend Range | `owr_bendRange` | 1–24 semitones | 2 | Pitch wheel range. |
| Macro CHARACTER | `owr_macroMaterial` | 0.0–1.0 | 0.0 | +0.8 material blend. |
| Macro MOVEMENT | `owr_macroMallet` | 0.0–1.0 | 0.0 | +0.5 malletHardness, +4000 Hz brightness. |
| Macro COUPLING | `owr_macroCoupling` | 0.0–1.0 | 0.0 | +0.4 sympathyAmount. |
| Macro SPACE | `owr_macroSpace` | 0.0–1.0 | 0.0 | +0.3 bodyDepth. |
| LFO1 Rate | `owr_lfo1Rate` | 0.005–20 Hz | 0.5 Hz | Modulates brightness (±3000 Hz at depth=1.0). |
| LFO1 Depth | `owr_lfo1Depth` | 0.0–1.0 | 0.1 | Default 0.1 = subtle brightness shimmer. |
| LFO1 Shape | `owr_lfo1Shape` | 0–4 | 0 | 0=Sine, 1=Triangle, 2=Saw, 3=Square, 4=S&H. |
| LFO2 Rate | `owr_lfo2Rate` | 0.005–20 Hz | 1.0 Hz | Modulates material continuum (±0.3 range). |
| LFO2 Depth | `owr_lfo2Depth` | 0.0–1.0 | 0.0 | Default 0.0 = inactive. Enable for material animation. |
| LFO2 Shape | `owr_lfo2Shape` | 0–4 | 0 | 0=Sine, 1=Triangle, 2=Saw, 3=Square, 4=S&H. |
