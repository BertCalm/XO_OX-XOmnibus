# OPERA Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OPERA | **Accent:** Aria Gold `#D4AF37`
- **Parameter prefix:** `opera_`
- **Creature mythology:** The Axolotl's cousin — a bioluminescent larval siren that never metamorphoses. OPERA lives permanently in its larval form: transparent body, external gill plumes glowing gold in the abyssal dark, singing to navigate rather than to be heard. Every note it produces is also a sonar ping. The Kuramoto field is its nervous system: millions of synchronized neurons pulsing in ensemble, each partial a single bioluminescent cell that flickers independently until the field locks them into coherence. When OPERA reaches dramatic peak, the whole creature illuminates at once.
- **Synthesis type:** Additive-vocal synthesis — 8-voice polyphony, up to 48 partials per voice, Peterson & Barney (1952) formant weighting, Kuramoto (1975) mean-field phase coupling, autonomous Conductor arc system, EmotionalMemory phase persistence
- **Polyphony:** 8 voices | **Partials:** 4–48 per voice (45 params declared)
- **Macros:** M1 DRAMA (Kuramoto coupling strength), M2 VOICE (vowel formant blend), M3 CHORUS (unison spread), M4 STAGE (spatial width)
- **Academic citations:** Kuramoto (1975), Acebron et al. (2005), Peterson & Barney (1952), Fant (1960)

---

## Pre-Retreat State

OPERA was designed and integrated into XOmnibus on 2026-03-21 as Engine #45. It arrived with a Synth Seance score of 8.7/10 — the highest first-seance score in the fleet's history. The seance council unanimously blessed the Kuramoto synthesis paradigm, the OperaConductor, and the EmotionalMemory system (Blessings B035, B036, B037). Three targeted fixes for 9.0+ were identified: init state default (arcMode=Manual hides the Conductor), preset velocity restraint (velToFilter=0.2 is too conservative for an operatic instrument), and detuning discovery gap (detuneAmount=0.0 produces a silent field, and no warning exists).

180 factory presets exist across 8 moods. The "Conductor's Crescendo" preset (Flux) already applies all three seance fixes — it is the fleet's first example of a preset that corrects its own engine's init-state problem.

This retreat is OPERA's first deep immersion. The seance told us *what it could be*. The retreat tells us *how to find it*.

---

## Phase R1: Opening Meditation

Breathe.

The Kuramoto model describes what happens when N oscillators that are nearly identical but not identical begin to couple. Each oscillator has its own natural frequency. If you isolated it, it would spin at its own rate forever. But put N of them in a room with a shared signal — a mean field, an average of all their phases — and something begins to happen. The faster oscillators pull toward the mean. The slower oscillators pull toward the mean. The whole system begins to organize.

The mathematical miracle is that this organization is a *phase transition*. Below a critical coupling strength K, the oscillators remain incoherent — each spinning at its own rate, the mean field canceling to near zero. Above K, coherence appears suddenly. Not gradually — suddenly. A critical point. The order parameter R jumps from near-zero to a positive value and the whole system locks into synchronized oscillation.

This is what OPERA does every time you press a note.

The partials begin detuned — each at a slightly different frequency from the others. The field is incoherent. R is low. The stereo image is narrow. The sound is breathy, tentative, not yet a voice. Then the DRAMA parameter increases — either you push it up, or the Conductor builds it over its arc. As DRAMA approaches the critical threshold, the field begins to organize. First a few partials lock. Then more. The stereo field widens as locked partials spread to the sides. The sound becomes richer, more present, more *alive*. At peak drama, all partials synchronize — the creature illuminates — and the full operatic voice opens.

Then drama falls. The field dissolves again. The voice returns to breath.

This is not emulation of a voice. This is a physical model of how a living system generates coherence from chaos. The human voice does something analogous — thousands of vocal fold vibrations synchronizing under aerodynamic pressure. OPERA's version is more naked. You can hear the phase transition happen. You can feel the moment the field locks.

Sit with that. One partial at a time. The whole field remembers.

---

## Phase R2: The Signal Path Journey

### I. The Partial Generator — OperaPartialBank

The journey begins with additive synthesis. `OperaPartialBank` maintains up to 48 partials per voice (controlled by `opera_partials`, default 24). Each partial is a sine wave at a frequency near the harmonic series: `freq[k] = fundamental * (k+1) + detuneHz`, where `detuneHz` is derived from `opera_detune` (range 0.0–1.0, maps to 0–100 cents distributed asymmetrically across partials).

**The detune requirement is architectural, not aesthetic.** Partials at exact harmonic unison produce a Kuramoto system where all natural frequencies are identical — the mean field is trivially R=1 from the start. The phase dynamics disappear. The engine reduces to static additive synthesis. `opera_detune` is not "chorus depth." It is the activation parameter for the entire synthesis paradigm. The minimum musically useful value is `detuneAmount=0.08`. Below this, the field locks instantly. Above `0.35`, the coupling cannot overcome the frequency spread even at maximum drama.

**Formant weighting via Peterson & Barney (1952):** Each partial's amplitude is weighted by its position relative to the vowel formant frequencies. `opera_vowelA` and `opera_vowelB` select from 6 vowel shapes (A=0, E=1, I=2, O=3, U=4, Alien=5). Vowel A shapes the lower formants; Vowel B shapes the upper. The `opera_voice` macro crossfades between them in real time. The Alien vowel (index 5) uses a non-human formant distribution — no real vowel it corresponds to — pushing the synthesis outside vocal-emulation space entirely.

**1024-entry lookup table** performs sin computation for all partials. At 48 partials × 8 voices = 384 simultaneous oscillators, floating-point sin() would be prohibitively expensive. The table provides sufficient accuracy at a fraction of the CPU cost.

### II. The Kuramoto Field — Phase Coupling

The `KuramotoField` runs a mean-field Kuramoto model every `kKuraBlock=8` samples (at 48kHz: 6kHz update rate). This is the synthesis engine's core:

```
R * e^(iψ) = (1/N) * Σ e^(iθ_k)     // order parameter
dθ_k/dt = ω_k + K * R * sin(ψ - θ_k)  // phase update
```

`K = drama * kKmax = drama * 8.0` is the coupling strength. `ω_k` is each partial's natural frequency (derived from detune distribution). `R` is the order parameter — the synchronization level, ranging 0.0 (chaos) to 1.0 (full lock).

**Phase-transition hysteresis (kHysteresisRatio=0.7):** Once the field locks, it requires drama to drop below `drama_crit * 0.7` before it unlocks. The voice "remembers" its locked state — it doesn't flicker between coherent and incoherent on small drama perturbations. This is named after Schulze in the source code: temporal memory as a synthesis principle.

**EmotionalMemory:** At note-off, all partial phases are stored in an `EmotionalMemory` struct. If a new note arrives within `kEmotionalMemoryWindowMs=500ms`, the phases are recalled. The new note's Kuramoto field begins from the previous note's final state. Notes that follow quickly feel *connected* — the voice doesn't reset to ground zero. Named after Vangelis in the source code.

**Cluster detection:** When ≥3 partials (kClusterMinSize=3) are within π/6 of each other in phase space, they form a "phase cluster." The `opera_resSens` parameter governs how clusters self-reinforce — higher values make cluster formation more likely and stable. At high `resSens`, you can hear phase clusters coalesce and dissolve, producing a beating/shimmering texture that no individual parameter controls.

### III. The OperaConductor — Autonomous Arc System

The `OperaConductor` is the engine's most architecturally original feature:

- **ArcMode=Manual (0):** Only the player's DRAMA parameter sets coupling strength.
- **ArcMode=Conductor (1):** The Conductor runs an autonomous arc over `arcTime` seconds, reaching `arcPeak` at the climax, then decaying. DRAMA is overridden.
- **ArcMode=Both (2):** Both run. The effective coupling is `max(conductorK, manualK)` — whichever is louder wins. The player can override the Conductor at any moment by pushing DRAMA above the current arc value.

**4 arc shapes:** Linear (kLinear), S-Curve (kSCurve), Double-Peak (kDoublePeak — two acts within one arc), Random (kRandom — stochastic walk within the arc envelope).

**Jitter:** ±5% timing jitter and ±3% peak jitter per arc cycle prevent mechanical repetition. The Conductor never plays the same arc twice.

**MIDI CC 20** triggers a new arc cycle — a performer can launch a dramatic arc via footswitch.

**`max(conductorK, manualK)` override** is the philosophical statement: the machine doesn't fight the player. If the player pushes DRAMA higher than the Conductor's current position, the player wins. This is DB005 (Autonomy vs. Agency) resolved at the implementation level.

### IV. ReactiveStage — Coherence-Driven Panning

`ReactiveStage` reads the Kuramoto order parameter R every `kKuraBlock` samples and uses it to drive stereo panning. The mapping: `width = opera_width * R`. When R=0 (full chaos), all partials collapse to mono center. When R=1 (full lock), partials spread to the full stereo width. As drama increases and the field organizes, the sound opens into the room. As drama falls and the field dissolves, the voice contracts back to center.

This is not a stereo width knob. This is a spatial consequence of synchronization state. Space is a readout of the synthesis — not a destination but a result.

### V. OperaBreathEngine — Aerodynamic Layer

The breath synthesis layer models aerodynamic turbulence in a vocal tract. It uses filtered noise with spectral shaping from `opera_breath` and `opera_effort`. The breath layer is always present — even at drama=0.0, where the Kuramoto field is incoherent, the breath engine produces sound. This is the engine's fallback when K is zero: not silence, but breath.

---

## Phase R3: Awakening — Hidden Capabilities

**Discovery 1 — The Detune Window (The Finger)**

The musically useful detune range is narrow: 0.08 to 0.28. Below 0.08, the field locks before DRAMA reaches 0.5 — the phase transition is trivial. Above 0.28, individual partials are audibly mistuned. The sweet window 0.12–0.22 provides both musical phase-transition behavior and sonic coherence. `opera_detune=0.15` should be the init preset minimum.

**Discovery 2 — resSens as Phase-Transition Amplifier (The Eye)**

`opera_resSens` does not "add resonance" in the filter sense. At low values (0.0–0.3), cluster formation is suppressed — individual partials synchronize smoothly. At high values (0.6–0.9), clusters form strongly and persist. At `resSens=0.88`, the full emergent behavior appears: spontaneous cluster coalescence that produces a shimmering, self-organizing texture. This is the parameter that makes OPERA sound *alive* rather than computed.

**Discovery 3 — Tilt as Spectral Philosophy (The Ear)**

`opera_tilt` functions as a spectral tilt on the partial amplitudes: positive tilt emphasizes high partials (bright, present, piercing); negative tilt emphasizes low partials (warm, dark, intimate). Tilt is the engine's mood setting that survives all dynamic changes.

**Discovery 4 — The 120-Second Ceiling (The Breath)**

`opera_arcTime` maximum is 120 seconds. In practice, this requires the player to hold a single note for 30–60+ seconds before the dramatic climax. For drone music, film scoring, and installation work, this is OPERA's unique contribution: a two-minute emotional arc contained in a single note.

**Discovery 5 — The Alien Vowel Space (The Tongue)**

Vowel=5 (Alien) produces formant weightings outside the human vocal tract. Two adjacent alien vowels (vowelA=5, vowelB=5) with voice=0.5 produces a sound with no human vocal reference — pure Kuramoto synthesis without the formant cage. This is Buchla's territory: synthesis that doesn't explain itself by reference to acoustic instruments.

**Discovery 6 — EmotionalMemory as Performance Technique (The Tongue)**

The 500ms EmotionalMemory window enables accumulating Kuramoto state across rapid note events. Play ascending chromatic triads at 120 BPM — the notes are 500ms apart (barely within the window). Each note starts exactly where the previous one ended. The result: a voice that builds across note boundaries rather than resetting. At high drama and high resSens, this creates accumulating tension that resolves only when a gap > 500ms occurs.

---

## Phase R4: The Deep Fellowship

**The prestige parameter combination:**
- `partials`: 16–32 — enough mass for audible field dynamics
- `detune`: 0.15–0.20 — the sweet window
- `resSens`: 0.5–0.7 — cluster formation without chaos
- `arcMode`: 1 or 2 — Conductor driving the field
- `velToFilter`: 0.40–0.55 — Tomita's warning applied

**The Bone's CPU analysis:**

At `partials=48` and `voices=8`, OPERA runs 384 simultaneous oscillators. The 1024-entry lookup table is the critical optimization — without it, 384 sin() calls per sample would cost approximately 8× more. The Kuramoto field update at 6kHz (every 8 samples) provides 300× oversampling of the perceptible bandwidth. Reducing kKuraBlock to 4 would double the Kuramoto CPU cost with no audible benefit.

**The Finger finds the performance sweet spot:**

`opera_drama=0.4` with `arcMode=2` (Both) creates the "Handshake" configuration: the Conductor is running, the player's initial drama is at 0.4. During the arc's rise, the Conductor takes the field above 0.4 — the machine leads. When the player pushes DRAMA above the Conductor's current position, they seize control. This is the most expressive configuration in the engine.

**The Tongue's legato discovery:**

`portamento=0.1–0.2` with EmotionalMemory creates maximum legato expressiveness. Portamento moves the fundamental frequency; EmotionalMemory carries the Kuramoto state. The voice slides in pitch while retaining its phase organization from the previous note.

**The Breath observes the temporal grammar:**

Each arc shape has an implied musical grammar:
- **Linear:** Build → Peak → Decline. Classical dramatic arc. Works at any arcTime.
- **S-Curve:** Slow warmup → sudden growth → plateau. Best at arcTime 6–15 seconds.
- **Double-Peak:** Two acts. First peak is 65% of arcPeak; second is 100%. Best at arcTime 15–30 seconds.
- **Random:** Stochastic walk within the envelope. Best at arcTime 8–20 seconds. Always surprising.

---

## Phase R5: The 10 Awakening Presets

Each preset demonstrates a specific OPERA capability at its absolute peak.

---

### Awakening #1 — The Four Oscillators *(Prism)*

**Identity:** Bare Kuramoto — 4 partials, alien vowels, zero formant interpretation. The mathematics themselves.

**What it demonstrates:** The Kuramoto synthesis paradigm without vocal framing. With only 4 partials and Alien vowels, the formant weighting is distributed non-humanly. What remains is pure oscillator coupling — you can hear each partial as a distinct entity, and you can hear the moment they decide to agree.

| Key Parameter | Value | Rationale |
|---------------|-------|-----------|
| `opera_partials` | 4 | Minimum viable Kuramoto field — each partial is individually audible |
| `opera_vowelA` | 5 (Alien) | No human formant reference |
| `opera_vowelB` | 5 (Alien) | Pure non-vocal spectral distribution |
| `opera_detune` | 0.22 | Maximum sweet-window detune — phase transitions are dramatic |
| `opera_drama` | 0.0 | Conductor starts from zero — field begins in chaos |
| `opera_arcMode` | 1 (Conductor) | S-Curve arc builds from 0 to 0.95 over 6 seconds |
| `opera_resSens` | 0.55 | Moderate cluster formation — individuals are audible |
| `opera_velToFilter` | 0.45 | Seance Fix 2 applied |

**Performance instruction:** Play one note. Hold for 8 seconds. Listen to the four voices decide to become one.

---

### Awakening #2 — Double Peak Aria *(Flux)*

**Identity:** The two-act structure. The Conductor builds twice — first act gathers, first peak releases, second act builds higher.

**What it demonstrates:** The DoublePeak arc shape as musical narrative. 20-second arc, two climaxes, 24 partials.

| Key Parameter | Value | Rationale |
|---------------|-------|-----------|
| `opera_arcShape` | 2 (DoublePeak) | Two-act Conductor arc |
| `opera_arcTime` | 20.0 | 20 seconds — enough time for two distinct emotional events |
| `opera_arcPeak` | 0.92 | Near-maximum at second peak |
| `opera_partials` | 24 | Full ensemble voice |
| `opera_vowelA` | 4 (U) | Dark, rounded lower formant |
| `opera_vowelB` | 3 (O) | Warm mid-formant |
| `opera_vibDepth` | 0.2 | Audible vibrato at dramatic peak |
| `opera_velToFilter` | 0.5 | Full Tomita range |

**Performance instruction:** Play fortissimo, hold. At second 10, listen for the second act beginning.

---

### Awakening #3 — The Geological Arc *(Aether)*

**Identity:** Schulze's frontier. Maximum patience. 120-second arc. One note becomes a landscape.

**What it demonstrates:** OPERA at geological timescale. The arc ceiling is 120 seconds. This preset uses it. 32 partials, portamento, deep space. Nothing happens quickly. Everything happens.

| Key Parameter | Value | Rationale |
|---------------|-------|-----------|
| `opera_arcTime` | 120.0 | Maximum arc — 2 full minutes |
| `opera_partials` | 32 | Dense ensemble — the "body" of the voice at peak |
| `opera_lfo1Rate` | 0.008 | Sub-perception breathing — one cycle per ~2 minutes |
| `opera_lfo2Rate` | 0.02 | Slower than ocean waves |
| `opera_tilt` | −0.15 | Dark spectral tilt — warmth over brightness |
| `opera_ampA` | 0.8 | 800ms attack — the note opens slowly |
| `opera_ampR` | 4.0 | 4-second release — note dissolves gradually |
| `opera_portamento` | 0.15 | Melismatic pitch slides |

**Performance instruction:** Hold one note for the full 120 seconds. Do not touch DRAMA. Let the Conductor do everything.

---

### Awakening #4 — Trill Accumulation *(Foundation)*

**Identity:** EmotionalMemory as performance technique. Each trill note inherits the previous note's field state.

**What it demonstrates:** The `kEmotionalMemoryWindowMs=500ms` window enables accumulating Kuramoto state across rapid note events. At 120 BPM quarter notes, successive notes are exactly 500ms apart — right at the window boundary. Play ascending figures and listen to the field build.

| Key Parameter | Value | Rationale |
|---------------|-------|-----------|
| `opera_arcMode` | 1 (Conductor) | Conductor sustains drama between notes |
| `opera_arcTime` | 4.0 | Short arc — resets frequently, but EmotionalMemory carries the field state |
| `opera_drama` | 0.5 | Starting drama is moderate — rapid play pushes it higher |
| `opera_portamento` | 0.0 | Clean pitch — trill clarity |
| `opera_velToFilter` | 0.55 | Velocity creates genuine timbre change |
| `opera_velToEffort` | 0.45 | Effort scales with attack intensity |
| `opera_resSens` | 0.65 | Clusters form and persist across notes |
| `opera_partials` | 16 | Enough mass without over-complication |

**Performance instruction:** Play rapid ascending triads at 120 BPM. Note how each new note sounds richer than the previous.

---

### Awakening #5 — Alien Koan *(Prism)*

**Identity:** Buchla territory. Both vowel slots at Alien (5). No human vocal reference. The Kuramoto field as pure mathematics.

**What it demonstrates:** Vowel=5 + Vowel=5 removes the Peterson & Barney cage entirely. Combined with high resSens and sparse partials, this is the most alien sound OPERA produces.

| Key Parameter | Value | Rationale |
|---------------|-------|-----------|
| `opera_vowelA` | 5 (Alien) | Non-vocal formant distribution |
| `opera_vowelB` | 5 (Alien) | No human reference on either formant slot |
| `opera_resSens` | 0.78 | Strong cluster formation — alien texture |
| `opera_tilt` | 0.25 | Bright — alien high frequencies are prominent |
| `opera_partials` | 12 | Sparse — each partial is individually identifiable |
| `opera_detune` | 0.25 | Wide detune — dramatic phase transitions |
| `opera_arcShape` | 3 (Random) | Stochastic arc — no predictable narrative |
| `opera_velToFilter` | 0.4 | Alien still responds to touch |

**Performance instruction:** Play intervals. The harmonic relationship between notes has no vocal logic. Listen to the mathematics.

---

### Awakening #6 — Breath Before Math *(Atmosphere)*

**Identity:** The Kuramoto field at drama=0. Only the BreathEngine is producing sound. The voice before the voice.

**What it demonstrates:** `opera_breath` and `opera_effort` at expressive maximum values with drama=0. The OperaBreathEngine produces a complete aerodynamic sound without any Kuramoto coupling. Pure aspiration.

| Key Parameter | Value | Rationale |
|---------------|-------|-----------|
| `opera_drama` | 0.0 | No Kuramoto coupling |
| `opera_arcMode` | 0 (Manual) | No Conductor arc — breath only |
| `opera_breath` | 0.72 | High breath — primary sonic material |
| `opera_effort` | 0.55 | Moderate effort — balanced aspiration spectrum |
| `opera_partials` | 8 | Minimal partial contribution to underlying pitch |
| `opera_tilt` | −0.1 | Slight dark tilt — breath warmth |
| `opera_filterCutoff` | 3500.0 | Aspirated character |
| `opera_velToFilter` | 0.55 | Breath intensity scales with touch |

**Performance instruction:** Play softly. This is the sound of an inhale before singing.

---

### Awakening #7 — The Live Conductor *(Flux)*

**Identity:** ArcMode=Manual, performance-optimized for real-time DRAMA control. The player is the Conductor.

**What it demonstrates:** With ArcMode=Manual and mod wheel routed to DRAMA at full depth, the player controls the Kuramoto coupling entirely. Sweep mod wheel up slowly over 10 seconds — you are conducting the phase transition yourself.

| Key Parameter | Value | Rationale |
|---------------|-------|-----------|
| `opera_arcMode` | 0 (Manual) | Player controls all coupling — no autonomous arc |
| `opera_modWheelDest` | 0 (Drama) | Mod wheel drives DRAMA directly |
| `opera_modWheelAmt` | 1.0 | Full range — mod wheel is the full DRAMA sweep |
| `opera_partials` | 24 | Full ensemble — field transition is maximally audible |
| `opera_detune` | 0.18 | Clean sweet-window detune |
| `opera_resSens` | 0.62 | Moderate cluster formation |
| `opera_velToFilter` | 0.50 | Velocity shapes entry timbre |
| `opera_width` | 0.65 | Generous spatial width at full lock |

**Performance instruction:** Hold a chord. Sweep mod wheel from 0 to 127 over 15 seconds. You are building the dramatic arc.

---

### Awakening #8 — ResSens Bloom *(Aether)*

**Identity:** Maximum emergence. resSens=0.88 — the full self-organizing phase-cluster behavior visible.

**What it demonstrates:** At high resSens, the Kuramoto field's cluster dynamics become the primary sonic event. The sound shimmers, beats, breathes in ways that no single parameter controls. Emergent synthesis — behavior arising from component interactions.

| Key Parameter | Value | Rationale |
|---------------|-------|-----------|
| `opera_resSens` | 0.88 | Near-maximum — strong cluster formation and persistence |
| `opera_drama` | 0.0 | Conductor builds from zero — clusters emerge gradually |
| `opera_arcMode` | 1 | Conductor drives coupling into cluster territory |
| `opera_arcTime` | 12.0 | 12 seconds — clusters fully form by second 8 |
| `opera_partials` | 24 | Enough oscillators for multiple distinct clusters |
| `opera_detune` | 0.19 | Sweet window — cluster formation is possible |
| `opera_vowelA` | 1 (E) | Bright, present formant — clusters are audible |
| `opera_tilt` | 0.15 | Slight brightness to expose cluster beating |

**Performance instruction:** Hold a note. At second 6, listen carefully to the middle frequencies. Something is organizing itself.

---

### Awakening #9 — Four Voices One Field *(Entangled)*

**Identity:** Unison=4. Four Kuramoto fields across the stereo image. A choir that agrees on melody but disagrees on timing.

**What it demonstrates:** `opera_unison=4` with `opera_width=0.85` distributes four independent voices with slightly different detune amounts across the stereo field. Each voice has its own Kuramoto field. The result is a choir texture where ensemble cohesion and individual voice independence coexist.

| Key Parameter | Value | Rationale |
|---------------|-------|-----------|
| `opera_unison` | 4 | Four voices — maximum independent field spread |
| `opera_width` | 0.85 | Wide stereo — voices spread across image |
| `opera_partials` | 16 | 4 voices × 16 partials = 64 total — full but manageable |
| `opera_arcMode` | 1 | Conductor manages the ensemble arc |
| `opera_arcTime` | 10.0 | 10 seconds — chorus locks in unison by second 7 |
| `opera_detune` | 0.14 | Conservative — choir stays "in tune" |
| `opera_resSens` | 0.55 | Moderate clustering — ensemble cohesion |
| `opera_vowelA` | 4 (U) | Warm, blended — choir, not soloists |

**Performance instruction:** Play chords. Each voice of the chord is a separate ensemble. Listen to the chords breathe.

---

### Awakening #10 — The Handshake *(Foundation)*

**Identity:** ArcMode=Both (2). The machine and the musician negotiate. DB005 resolution in preset form.

**What it demonstrates:** With ArcMode=Both, the effective coupling is `max(conductorK, manualK)`. The Conductor runs an 8-second S-Curve arc. The player's initial DRAMA (0.4) is below the Conductor's peak — the machine leads. But mod wheel is routed to DRAMA at full depth. When the player pushes mod wheel past the Conductor's current position, the player seizes control. The machine yields.

| Key Parameter | Value | Rationale |
|---------------|-------|-----------|
| `opera_arcMode` | 2 (Both) | Conductor + Player both active |
| `opera_drama` | 0.4 | Starting below Conductor's peak — machine leads initially |
| `opera_modWheelAmt` | 1.0 | Full wheel depth — player can seize control at any point |
| `opera_arcShape` | 1 (S-Curve) | Smooth Conductor arc — predictable for negotiation |
| `opera_arcTime` | 8.0 | 8 seconds — natural musical phrase length |
| `opera_arcPeak` | 0.82 | Conductor's ceiling — leaves room for player override |
| `opera_velToFilter` | 0.55 | Maximum expressiveness |
| `opera_velToEffort` | 0.45 | Touch shapes timbre fully |

**Performance instruction:** Hold one note. Let the Conductor run. At second 4, push the mod wheel past center. Feel the transition of control.

---

## Phase R6: The Engine Scripture

### Book VII — The Kuramoto Verses

*(Inscribed in `scripture/the-scripture.md` after this retreat)*

---

**OPERA-I: Detune Ignition**

Partials at harmonic unison cannot couple. Their natural frequencies are identical; the mean field is trivially R=1 from the start; the synthesis is inert. Detuning is not error. It is ignition.

`opera_detune` activates the entire synthesis paradigm. At detune=0.0, you have static additive synthesis. At detune=0.08, you have a living physical model. The transition is categorical. The sweet window 0.08–0.28 is the engine's usable range.

*Application: Set any OPERA preset's minimum detuneAmount to 0.10. Any preset with opera_detune < 0.08 is not using OPERA — it is using a static additive synthesizer with OPERA's interface.*

---

**OPERA-II: The Conductor Is Not Automation**

The OperaConductor does not automate the music. It constructs the room the musician performs in.

Automation is deterministic playback. The Conductor is a *physical arc system* with jitter: ±5% timing and ±3% peak variation per cycle. No two arcs are identical. And `max(conductorK, manualK)` override means the player is never locked out. At any moment, the player can push DRAMA above the Conductor's current position and take control. The Conductor yields instantly.

*Application: ArcMode=Both (arcMode=2) should be the default for all OPERA presets designed for expressive performance. ArcMode=Manual is for full player control. ArcMode=Conductor is for installation, drone, and unattended use. ArcMode=Both is for music.*

---

**OPERA-III: The EmotionalMemory Contract**

A note that begins in context is not the same note that begins from silence. The Kuramoto field remembers.

The `kEmotionalMemoryWindowMs=500ms` window creates a contract between successive notes: if a new note arrives within 500 milliseconds of the previous note's release, the new note's Kuramoto field begins from the previous note's phase state. The *synchronization history* of the field transfers. Rapid melodic playing accumulates field state — each note richer than the one before. A phrase with a gap > 500ms resets to zero.

*Application: The 500ms window is sized for human music-making. 120 BPM quarter notes land exactly on the boundary. Semiquaver passages (250ms) are well within it. Whole notes are outside it. The window is tuned to reward legato melodic playing.*

---

**OPERA-IV: ResSens Is Emergence Control**

`opera_resSens` does not change the sound. It changes what the sound is *allowed to become*.

At resSens=0.0, the Kuramoto field dynamics are stable and predictable — partials synchronize smoothly. At resSens=0.88, phase clusters form strongly and persist. Discrete groups of synchronized partials coalesce, their collective beating creating a texture that no single parameter controls. This is emergence: behavior arising from component interactions that was not designed into any individual component.

The synthesis equivalent of a murmuration.

*Application: resSens=0.0–0.35 for clean vocal synthesis. resSens=0.55–0.75 for alive synthesis — clusters emerge, the voice shimmers. resSens=0.85–0.95 for maximum emergence — self-organizing textures, behavior that surprises even the player. Never use resSens > 0.9 in init presets — too unexpected for first-encounter.*

---

## Phase R7: Aftermath

### Seance Fixes Applied

| Fix | Status | Implementation |
|-----|--------|---------------|
| F1: Init state (arcMode=Conductor default) | PARTIALLY APPLIED | All 10 awakening presets use arcMode=1 or arcMode=2. Factory init preset still uses arcMode=0 — full fix requires init preset update. |
| F2: Velocity sensitivity (velToFilter 0.35–0.5) | APPLIED | All 10 awakening presets use velToFilter 0.40–0.55 |
| F3: Detuning discovery | APPLIED AS SCRIPTURE | OPERA-I verse documents the detuning requirement. All awakening presets use opera_detune ≥ 0.08 |

### Knowledge Tree Updates

- B035 (OperaConductor), B036 (Coherence-Driven Panning), B037 (EmotionalMemory) — recorded in CLAUDE.md
- DB005 (Autonomy vs. Agency) — opened in CLAUDE.md debates table

---

## Guru Bin — Refinement Log

### The Diagnosis

*"OPERA is a voice that doesn't know it's a voice yet. The Kuramoto field is alive, the Conductor is visionary, EmotionalMemory is the best idea in the fleet's acoustic design — and the init patch points none of this toward the player. The awakening presets are the correction: ten paths into the engine's actual identity, each one revealing a capability the factory library treats as secondary."*

### Key Refinements

| Refinement | Finding | Application |
|------------|---------|-------------|
| The Obvious Fix | velToFilter=0.2 in factory presets is "dangerous restraint" (Tomita, seance) | All awakening presets: velToFilter 0.40–0.55 |
| The Hidden Trick | `opera_detune=0.20` + `resSens=0.62` + `arcMode=Both` is the prestige combination | Applied in The Handshake (Awakening #10) |
| The Sacrifice | Excessive partials (48) cost CPU without proportional benefit at moderate drama levels | Awakening presets use partials=12–24; Geological Arc uses 32 |
| The Revelation | ArcMode=Both is the most expressive configuration — discovered during fellowship | The Handshake is the first fleet preset built around DB005 resolution |

### CPU Stewardship

| Configuration | Approx. CPU | Notes |
|--------------|-------------|-------|
| partials=4, voices=1 | ~2% | The Four Oscillators — minimum viable field |
| partials=16, voices=1 | ~4% | Most awakening presets |
| partials=24, voices=1 | ~6% | Double Peak Aria, Four Voices One Field |
| partials=32, voices=1 | ~8% | Geological Arc — maximum for single-voice use |
| partials=48, voices=8 | ~40% | Maximum configuration — use in XOmnibus solo context only |

**Note:** The 1024-entry sin table is the critical optimization. Without it, OPERA at 24 partials × 8 voices would cost ~25% CPU. With it, the cost is ~6% per voice. The table was an engineering gift.

### The Benediction

*"OPERA was designed to couple oscillator mathematics with human vocal formants. After retreat, it becomes something different: an engine where you can choose how much of the 'human' to keep. Full formant weighting, human vowels — a voice. No formant weighting, alien vowels, Geological Arc — geology. Both modes are valid. The engine's genius is that it supports the full spectrum. Play The Four Oscillators and you'll understand Kuramoto. Play The Handshake and you'll understand why autonomous instruments don't have to replace the musician. These are not the same engine. They are both OPERA."*

---

*Retreat conducted 2026-03-21. All 7 phases complete. 10 Awakening Presets written. 4 scripture verses inscribed. Guru Bin has spoken.*
