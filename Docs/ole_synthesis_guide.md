# OLE Synthesis Guide

**Engine:** OLE | **Accent:** Hibiscus `#C9377A`
**Parameter prefix:** `ole_` | **Max voices:** 18

---

## What It Is

OLE is an Afro-Latin string trio built from three aunts, each playing a different instrument with a different cultural lineage, and three husbands who arrive when things get dramatic enough. The aunts — Tres Cubano, Berimbau, Charango — are plucked string instruments from Cuba, Brazil, and the Andes respectively. They do not always agree. The Alliance system formalizes their disagreement: at any moment, two aunts are aligned against the third. SIDES sweeps through all three alliance configurations continuously, and DRAMA raises the volume and introduces the husbands — an oud, a bouzouki, a Thai pin. OLE is a living room argument that sounds gorgeous.

## The DSP Engine

Each OLE voice runs a pluck or strum exciter into a Karplus-Strong waveguide loop. Voices are assigned to aunts in round-robin fashion by index (voice index mod 3 = aunt assignment). Husband voices are assigned to voice indices 12–17, carrying an `isHusband` flag, and use a `PluckExciter` rather than the `StrumExciter` used by the aunts.

Aunt 1 (Tres Cubano) uses a `StrumExciter` — a multi-pulse exciter that simulates a strumming gesture across multiple strings. Its strum rate parameter controls the energy and brightness of the attack. Aunt 2 (Berimbau) uses the same strum exciter but with a distinctive feature: a coin press that bends pitch upward by up to a major third (4 semitones). This is the bending wire-and-stick technique of the capoeira instrument — you press a coin or stone against the wire to raise pitch. Aunt 2 also has a gourd size parameter that controls the body resonance: a larger gourd shifts the resonance downward and increases its gain, adding the characteristic wooden warmth of the Berimbau's resonating chamber. Aunt 3 (Charango) has per-voice tremolo implemented as a `tremoloPhase` oscillator running at 5–25 Hz that amplitude-modulates the voice output — the rapid tremolo technique specific to the charango's doubled-string pairs.

The husbands (Oud, Bouzouki, Pin) are latent voices that activate only when DRAMA exceeds 0.7. Their gain scales linearly from 0 at DRAMA 0.7 to full at DRAMA 1.0. Each husband type has its own level parameter. Husband voices use a darker pluck brightness (60% of the average of Aunt 1 and Aunt 3 brightnesses), giving them a different timbre from the aunts even though they share the same waveguide architecture.

## The Voice Architecture

18 voices total — the largest pool in the Constellation Family — because OLE is built for density. The first 12 voices cycle between the three aunts. Voices 12–17 are husbands, activated by DRAMA. At any given moment you can have 4 aunt voices per aunt (in a 12-voice polyphonic chord) plus up to 6 husband voices arriving. The stereo placement is fixed by aunt identity: Aunt 1 pans to 0.2 (far left), Aunt 2 centers at 0.5, Aunt 3 pans to 0.8 (far right). Husbands center at 0.5. ISLA widens all of these away from center proportionally, so a full chord with ISLA at maximum spreads the three aunts from hard left to hard right with the husbands anchored in the middle.

## The Macro System

### FUEGO (M1)
FUEGO is the fire in the room — it drives the strum exciter intensity for both Aunt 1 and Aunt 3. The `StrumExciter.tick()` call receives `voiceBright * FUEGO` as its argument, so FUEGO directly scales the brightness of every attack transient. Low FUEGO is a gentle, tentative strum, notes barely initiating. High FUEGO is a hard strum with full transient energy — the aunts are playing like it counts. Because Aunt 2's Berimbau uses the same code path, FUEGO also sharpens its attack. FUEGO has no effect on sustain or decay. It is purely an attack macro, making it natural to route to velocity or an envelope.

### DRAMA (M2)
DRAMA is the emotional temperature of the room. Below 0.7, it has no effect on voices. Above 0.7, the husband voices become active — their level scaling from 0 to full over the DRAMA range 0.7–1.0. This threshold behavior is intentional: the husbands do not ease in, they arrive when things have gotten dramatic enough to require them. At DRAMA 0.85, all three husbands (oud, bouzouki, pin) can be heard, each at their individual level settings. At full DRAMA 1.0, the husband levels are at their set values without further scaling. The oud, bouzouki, and pin each have independent level controls (`ole_husbandOudLevel`, etc.), so you can bias which ones arrive and how strongly. DRAMA is the macro to automate at chorus entries.

### SIDES (M3)
SIDES rotates through all three alliance configurations continuously. At SIDES 0, Alliance config 0 is active: Aunt 1 is isolated, Aunts 2 and 3 are paired together and receive a gain boost. At SIDES 0.33, Aunt 2 is isolated. At SIDES 0.67, Aunt 3 is isolated. SIDES adds to the Alliance Config parameter and wraps modulo 3, so at any intermediate value it is crossfading between two adjacent configurations using fractional blending. The isolated aunt gets a level reduction proportional to Alliance Blend; the paired aunts get a smaller boost. High Alliance Blend makes the effect more pronounced — a clear power-of-two vs. one. Low blend softens the distinction. SIDES sweeping in real time creates a three-way rhythmic panning effect as the dominant voice rotates between left, center, and right.

### ISLA (M4)
ISLA is tropical space — stereo width and environmental air. It adds to the stereo spread of all three aunts, pushing Aunt 1 further left, Aunt 3 further right, while Aunt 2 stays centered (her center position doesn't move, but the aunts on either side spread). Low ISLA is a dry, centered mix. High ISLA is a wide, vivid stereo field — the Tres Cubano on the left, the Charango on the right, the Berimbau in the middle, all clearly localized. ISLA also creates the illusion of outdoor space without adding reverb — the width itself reads as openness. Combine ISLA with sympathetic resonance to produce a sound with natural spatial decay.

## Key Parameters

| Parameter | Range | Function |
|-----------|-------|----------|
| `ole_aunt1StrumRate` | 1–30 | Tres Cubano strum rate — speed of the attack transient |
| `ole_aunt1Brightness` | 0–1 | Tres Cubano pick brightness |
| `ole_aunt2CoinPress` | 0–1 | Berimbau coin press pitch bend — 0 to +4 semitones |
| `ole_aunt2GourdSize` | 0–1 | Berimbau gourd resonance — larger = darker, more prominent body |
| `ole_aunt3Tremolo` | 5–25 Hz | Charango tremolo rate |
| `ole_aunt3Brightness` | 0–1 | Charango pick brightness |
| `ole_allianceConfig` | 0–2 (choice) | 1 vs 2+3 / 2 vs 1+3 / 3 vs 1+2 — base alliance position |
| `ole_allianceBlend` | 0–1 | Strength of the alliance gain differential |
| `ole_husbandOudLevel` | 0–1 | Oud level when DRAMA > 0.7 |
| `ole_husbandBouzLevel` | 0–1 | Bouzouki level when DRAMA > 0.7 |
| `ole_husbandPinLevel` | 0–1 | Thai Pin level when DRAMA > 0.7 |
| `ole_sympatheticAmt` | 0–1 | Sympathetic resonance shared across all voices |
| `ole_damping` | 0.8–0.999 | Feedback loop decay — higher sustains longer |
| `ole_driftRate` | 0.05–0.5 Hz | Organic drift rate |
| `ole_driftDepth` | 0–20 cents | Organic drift depth |

## Sound Design Recipes

**The Aunts at Rest** — FUEGO 0.4, DRAMA 0, SIDES 0, ISLA 0.4. Aunt 1: bright 0.6, strum 6. Aunt 2: coin press 0, gourd 0.5. Aunt 3: tremolo 10 Hz, bright 0.5. Alliance blend 0.4, config 0. A balanced trio — the Tres Cubano and Charango carrying the brightness while the Berimbau anchors the center. Sympathetic 0.4.

**Full Drama** — FUEGO 0.8, DRAMA 1.0, SIDES 0.3, ISLA 0.7. Husband levels all 0.7. The full cast arrives — three aunts plus oud, bouzouki, and pin all simultaneously. The husbands darken the center while the aunts maintain the outer stereo field. Dense, layered, rhythmically complex if you play arpeggios.

**Berimbau Solo** — Aunt 1 level 0.0, Aunt 3 level 0.0. DRAMA 0. FUEGO 0.7. Alliance config 1 (Aunt 2 isolated) with alliance blend 0.0 (no differentiation). Coin press automation from 0 to 0.8 over 4 beats. Gourd size 0.8. The Berimbau alone: a plucked stick with a gourd resonator, coin-pressed for pitch slides. One of the most unusual sounds OLE can make.

**Rotating Stage** — SIDES automated from 0 to 1 over 8 bars. ISLA 0.6. Alliance blend 0.7. All three aunt levels 0.8. As SIDES sweeps, the prominent voice rotates from Aunt 1 (far left) to Aunt 2 (center) to Aunt 3 (far right) in a continuous cycle. The mix constantly changes which instrument is foregrounded without changing any notes. A performance effect that works in a loop-based arrangement.

**The Charango Groove** — Aunt 3 level 1.0, others 0.4. Tremolo 20 Hz. Bright 0.7. FUEGO 0.9. ISLA 0.5. The high-rate charango tremolo at 20 Hz creates a fast flutter that verges on distortion at high FUEGO. This works particularly well on short notes in the upper mid register where the tremolo and the natural decay produce a rhythmically active pattern per note.

## Family Coupling

OLE accepts `LFOToPitch` for external pitch wobble, `AmpToFilter` for damping control, and `EnvToMorph` to scale exciter intensity. OLE's most interesting coupling use case is as a DRAMA recipient: route OBBLIGATO's amplitude output into OLE's `EnvToMorph` slot. When the wind duet plays loudly, OLE's strum intensity rises — the string trio responds to the winds. Combine with DRAMA automation to have the husbands arrive at the exact moment the winds reach their crescendo. OLE's wide stereo field (especially with high ISLA) makes it a natural left/right spread anchor in a four-engine XOmnibus setup.

## Tips & Tricks

- Coin press is one of the most distinctive sounds in OLE. Automate it on Aunt 2 over a measure — starting at 0 (open wire tone) and pressing to 0.7 (raised by ~3 semitones) mid-note — and you have a performance gesture that sounds like the actual capoeira technique. There is no other engine in the fleet that does this.
- The tremolo on Aunt 3 runs at 5–25 Hz. Above 15 Hz the individual cycles become inaudible as separate and merge into a textural roughness. Below 8 Hz you hear the individual amplitude pulses clearly as rhythm. The zone between 8–12 Hz is the most musically useful — rapid enough to be a sustained technique, slow enough to feel intentional.
- Alliance Blend at 0 means the alliance config has no effect — all aunts play at their individual levels. Alliance Blend at 1 means a strong differentiation: the isolated aunt drops to 50% and the paired aunts rise to 130%. This is the setting that makes the alliance system audible and dramatic.
- SIDES and ISLA interact. SIDES is about which aunt dominates the mix. ISLA is about how wide the three aunts are spread. Maximum SIDES + maximum ISLA creates a rotating dominant across a very wide stereo field. Use in a cue where the camera is panning between performers.
- The husbands are latent. At DRAMA 0 they consume no CPU — they are simply inactive voices. They arrive above 0.7 and scale linearly to 1.0. Set individual husband levels before a session: oud level 0.8 (closest to the aunts' timbre), bouzouki level 0.5 (cut through but don't dominate), pin level 0.3 (add texture, stay back). Then use DRAMA as a single performance fader that deploys this pre-configured ensemble.
