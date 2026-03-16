# XObese Character Pack — Design Spec

*XO_OX Designs | Pack R&D Document*
*Date: 2026-03-16*

---

## Overview

**Pack Name:** OBESE: Mojo Rising
**Engine:** XObese (OBESE) — Hot Pink `#FF1493`
**Format:** XPN keygroup program files (.xpn) for MPC standalone
**Preset count:** 20 keygroup presets across 5 families
**Identity:** The whale surfaces. 13 oscillators, 4 ZDF ladder filters, Mojo drift biology.

---

## Pack Identity

OBESE: Mojo Rising is a showcase of the full BELLY-to-BITE range of XObese — the engine with thirteen oscillators stacked into a wall of sound, four ZDF ladder filters in stereo groups, and the Mojo drift system that makes it breathe like a living thing.

The five macro axes of XObese are:

| Macro | Parameter | Character at 0 | Character at 10 |
|-------|-----------|---------------|-----------------|
| BELLY | Low-frequency mass / sub oscillator level + filter cutoff low | Thin, high — no sub authority | Maximum sub dominance, filter wide open below 200Hz |
| BITE | Saturation drive + filter resonance | Clean, smooth — digital clarity | Snarling, forward, harmonic density spills out |
| SCURRY | Arpeggiator rate + LFO rate + drift speed | Still, held, sustained | Fast, moving, rhythmically agitated |
| TRASH | Bitcrusher depth + sample-rate reduction | Pristine, unprocessed | Degraded, lo-fi, industrial destruction |
| PLAY DEAD | Amp release + filter decay | Short, clipped, tight | Long, sustained, slow fade to nothing |

The Mojo drift system (B015 — Seance Blessing, 8/8 ghosts) is the whale's biology. At Mojo zero, XObese is a digital supersaw — clinical and precise. At Mojo ten, thirteen oscillators drift independently, producing organic warmth that the ear hears as character rather than detuning.

### feliX-Oscar Position

XObese sits at **70% Oscar / 30% feliX** — mass and sustain dominant, but the arpeggiator and 13-oscillator harmonic richness give it kinetic energy. In this pack, presets range from pure Oscar (deepest BELLY, no SCURRY) to a leaning feliX (high SCURRY, high BITE, fast arpeggio) — but none reach pure feliX. XObese is constitutionally Oscar. That is its power.

---

## Preset Families

### Family 1 — BELLY Heavy (5 presets)

*Deep, sub, warm, minimal harmonics. This is where the whale sleeps.*

The BELLY Heavy presets sit at the Oscar extreme of XObese's range. The sub oscillator is dominant, the four filter groups are tuned low, Mojo drift gives each of the 13 oscillators slight analog character at low rates, and BITE and TRASH are at or near minimum. SCURRY is dormant. These are pads and basses that displace other sounds in the mix.

---

**Preset B01 — Tectonic Sub**

The deepest preset in the pack. The whale at rest.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 9.5 | Sub oscillator at octave -2, filter cutoff 80Hz, massive low-end authority |
| BITE | 1.0 | No saturation — the purity of sub mass |
| SCURRY | 0.5 | Mojo drift only — 13 oscillators breathing but not moving |
| TRASH | 0.0 | Pristine output |
| PLAY DEAD | 7.0 | 2.5-second release — the sub decays slowly, like pressure releasing |

- **feliX/Oscar:** 85% Oscar / 15% feliX
- **Sonic DNA:** brightness 2, warmth 9, movement 2, density 9, space 6, aggression 2
- **Key range:** C1–C4 (extends 3 octaves, focused on bass register)
- **Velocity response:** Velocity controls filter cutoff mod depth only. Low velocity = completely sealed filter, almost no tone above 100Hz. Hard velocity = filter opens to 300Hz, warmth blooms without brightening.
- **Mojo:** 6.0 — the drift is audible as life, not as detuning

---

**Preset B02 — Thermocline**

Two register layers — sub weight below, wide oscillator mass above. The boundary between depths.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 8.0 | Sub dominant but filter cutoff at 200Hz — more body visible |
| BITE | 1.5 | Trace saturation — warms the upper mass without gritting |
| SCURRY | 1.0 | Very slow LFO on filter cutoff — one cycle per 8 bars |
| TRASH | 0.0 | Clean |
| PLAY DEAD | 6.0 | 2-second release |

- **feliX/Oscar:** 78% Oscar / 22% feliX
- **Sonic DNA:** brightness 3, warmth 9, movement 3, density 8, space 6, aggression 2
- **Key range:** C1–C5
- **Velocity response:** Velocity opens filter — the harder you play, the more the oscillator groups emerge from the low-pass shadow. Soft hits are pure sub; hard hits reveal the full 13-oscillator architecture.

---

**Preset B03 — Pressure Wave**

The whale vocalization at 188 dB. Everything for a thousand miles hears this.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 10.0 | Maximum — all four filter groups open, sub at -1 octave, maximum mass |
| BITE | 2.5 | Slight drive — the edges of the wave start to clip naturally |
| SCURRY | 0.0 | Perfectly still |
| TRASH | 0.0 | No degradation |
| PLAY DEAD | 8.0 | 3.5-second release — this sound doesn't stop quickly |

- **feliX/Oscar:** 90% Oscar / 10% feliX
- **Sonic DNA:** brightness 2, warmth 10, movement 1, density 10, space 5, aggression 5
- **Key range:** C1–C3 only — above C3, the preset intentionally becomes impractical (this is a bass instrument)
- **Velocity response:** At full velocity, the 13 oscillators hit with maximum stereo spread — the four groups fan outward across the panorama. At low velocity, the groups collapse toward center.

---

**Preset B04 — Deep Pad**

The whale-as-chord. A warm, spacious pad with sub grounding.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 7.0 | Sub present but not dominant — this is a full-register pad |
| BITE | 1.0 | Clean |
| SCURRY | 2.0 | Very slow filter LFO, slow Mojo drift variation |
| TRASH | 0.0 | Clean |
| PLAY DEAD | 9.5 | 5+ second release — holds and blooms |

- **feliX/Oscar:** 70% Oscar / 30% feliX
- **Sonic DNA:** brightness 4, warmth 8, movement 4, density 7, space 8, aggression 1
- **Key range:** C2–C6 — usable as a pad across the full keyboard
- **Velocity response:** Attack time velocity-modulated — soft velocity has a 600ms swell; hard velocity has a 50ms attack. The same pad played gently is a slow bloom; played hard it hits like a wave.

---

**Preset B05 — Baleen Filter**

The oscillator mass filtered through a narrow resonant window — like sound passing through a grid of baleen. Deep and humming.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 8.5 | Full low-end mass |
| BITE | 3.5 | Resonance raised — the filter sings at its natural frequency |
| SCURRY | 1.5 | Filter mod LFO at very slow rate, audible as breathing |
| TRASH | 0.0 | Clean |
| PLAY DEAD | 6.5 | 2.5-second release |

- **feliX/Oscar:** 75% Oscar / 25% feliX
- **Sonic DNA:** brightness 4, warmth 8, movement 4, density 8, space 5, aggression 3
- **Key range:** C1–C4
- **Velocity response:** Velocity controls resonance depth — a soft hit keeps the filter Q low; hard hit pushes resonance near self-oscillation, the filter sings a separate tone above the oscillator mass.

---

### Family 2 — BITE Aggressive (5 presets)

*Distorted, forward, crunchy. This is where the whale bites.*

BITE Aggressive presets push XObese's saturation stage (asymmetric waveshaper + DC blocker) and filter resonance hard. BELLY is reduced — the mass is still there, but the upper harmonics generated by distortion are the lead character. These are bass leads, distorted pads, and industrial tones.

---

**Preset A01 — First Strike**

Immediate, forward, no subtlety. The attack is the sound.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 5.0 | Center — balanced sub and body |
| BITE | 8.5 | Heavy saturation — even harmonics dominate |
| SCURRY | 3.0 | Moderate — some LFO movement adds life to the distortion |
| TRASH | 1.0 | Trace crusher — not destruction, just a hint of digital edge |
| PLAY DEAD | 3.0 | Fast release — hit and gone |

- **feliX/Oscar:** 45% Oscar / 55% feliX — this is the most feliX-leaning BITE preset
- **Sonic DNA:** brightness 8, warmth 5, movement 5, density 8, space 3, aggression 9
- **Key range:** C2–C5
- **Velocity response:** Velocity drives BITE macro depth — soft hit = moderate saturation; hard hit = full distortion character. The harmonic density of the sound doubles from soft to hard velocity.

---

**Preset A02 — Blubber Saw**

The XObese supersaw pushed through saturation. 13 oscillators + distortion = a wall of harmonics.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 4.0 | Below center — sub presence exists but it's not the story |
| BITE | 9.0 | Near maximum — dense even harmonics, forward midrange |
| SCURRY | 2.0 | Slow modulation keeps the distorted mass from sounding static |
| TRASH | 2.5 | Slight bit-depth color — sits just above pristine |
| PLAY DEAD | 4.0 | Moderate release |

- **feliX/Oscar:** 50% Oscar / 50% feliX — the balance point, tipped forward by BITE
- **Sonic DNA:** brightness 8, warmth 6, movement 4, density 10, space 3, aggression 8
- **Key range:** C2–C5
- **Velocity response:** Velocity controls BITE + filter cutoff simultaneously — soft = saturated but sealed, hard = saturated and open. Full velocity sounds like a wall of teeth.

---

**Preset A03 — Gut Punch**

Mid-heavy, aggressive, designed for the 300–800Hz zone. Sits forward in any mix.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 3.0 | Low BELLY — filter cutoff raised, sub recessed, mids exposed |
| BITE | 8.0 | Heavy saturation on exposed midrange |
| SCURRY | 4.0 | Faster LFO — the saturated mids modulate, creating rhythmic texture |
| TRASH | 3.0 | Additional grit in the 4–8kHz range from sample-rate reduction |
| PLAY DEAD | 2.5 | Short release — very tight and punchy |

- **feliX/Oscar:** 40% Oscar / 60% feliX
- **Sonic DNA:** brightness 7, warmth 5, movement 7, density 9, space 2, aggression 10
- **Key range:** C2–C6
- **Velocity response:** Attack transient velocity-mapped — hard hits have a sharp click injected before saturation, creating a pick-attack character. Soft hits go immediately into the saturated body.

---

**Preset A04 — Fin Slice**

High-frequency aggression. Unusual for XObese — this is the whale showing its speed.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 2.0 | Low sub presence — this is a high-register voice |
| BITE | 7.5 | Strong saturation, pushing odd harmonics (raised resonance creates self-oscillation shimmer) |
| SCURRY | 5.5 | Active modulation — the saturated high-end moves constantly |
| TRASH | 4.0 | Sample-rate reduction at moderate depth — the digital alias becomes musical |
| PLAY DEAD | 3.5 | Fast-ish — present but not lingering |

- **feliX/Oscar:** 35% Oscar / 65% feliX — the most feliX XObese can be without becoming something else
- **Sonic DNA:** brightness 9, warmth 3, movement 8, density 7, space 4, aggression 9
- **Key range:** C3–C6 (bass register is not this preset's home)
- **Velocity response:** Velocity directly controls filter resonance depth — whisper-soft gets mild character; hard hits self-oscillate the filter. Unpredictable in a designed way.

---

**Preset A05 — Breach**

XObese surfacing at full speed. Maximum energy, the moment the whale breaks the water.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 6.5 | Sub present — this is still a whale |
| BITE | 10.0 | Maximum saturation — complete harmonic saturation of the output |
| SCURRY | 6.0 | Active — the distorted mass is in motion |
| TRASH | 5.0 | Bitcrusher visible — adds industrial-digital texture to the saturated signal |
| PLAY DEAD | 3.0 | Short release — the breach is an event, not a hold |

- **feliX/Oscar:** 55% Oscar / 45% feliX
- **Sonic DNA:** brightness 9, warmth 6, movement 8, density 10, space 3, aggression 10
- **Key range:** C2–C5
- **Velocity response:** Velocity is the trigger force. At velocity 100+, all four filter groups fan to maximum stereo width and BITE is at full maximum. At velocity 50, BITE is at 7.0, width at 70%. At velocity 20, BITE drops to 5.0 — still aggressive, but not a breach.

---

### Family 3 — SCURRY Active (4 presets)

*Movement, LFO, rhythmic character. The whale's arpeggiator. Circling.*

SCURRY presets showcase XObese's arpeggiator (5 patterns, 5 rates, 1–3 octave range) and the Mojo drift as a rhythmic tool. These are not static pads. They are bass instruments that generate internal movement.

---

**Preset S01 — Whale Song**

The arpeggiator at slow rate, Mojo drift maximum — the biological rhythm of the whale's actual vocalization.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 7.5 | Strong sub — this arpeggio has weight |
| BITE | 2.0 | Clean, warm — the movement is the character |
| SCURRY | 7.0 | Arpeggiator active at 1/4 rate, UpDown pattern, 2-octave range |
| TRASH | 0.0 | Clean |
| PLAY DEAD | 5.5 | Each note decays before the next — separated, not blurred |

- **Arpeggiator settings:** Pattern = UpDown, Rate = 1/4, Octaves = 2
- **Mojo:** 8.5 — maximum biological drift, the chord oscillators drift independently
- **feliX/Oscar:** 65% Oscar / 35% feliX
- **Sonic DNA:** brightness 4, warmth 8, movement 9, density 7, space 7, aggression 2
- **Key range:** C2–C5
- **Velocity response:** Velocity controls arpeggiator note gate length — soft hits produce staccato dots, hard hits sustain until the next note arrives. The rhythm changes with touch intensity.

---

**Preset S02 — Thermal Vent**

Fast SCURRY — 1/16th note arpeggio, heavy low-end, rhythmically aggressive.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 8.0 | Deep sub under the fast arpeggio pattern |
| BITE | 3.5 | Slight saturation — the fast notes need some edge to cut |
| SCURRY | 9.0 | Arpeggiator at 1/16 rate, Up pattern, 1-octave range |
| TRASH | 1.5 | Trace digital color |
| PLAY DEAD | 2.5 | Very short — rapid staccato attack |

- **Arpeggiator settings:** Pattern = Up, Rate = 1/16, Octaves = 1
- **feliX/Oscar:** 60% Oscar / 40% feliX
- **Sonic DNA:** brightness 5, warmth 7, movement 10, density 8, space 3, aggression 6
- **Key range:** C2–C4
- **Velocity response:** Velocity controls Mojo drift rate — soft velocity plays at standard drift, hard velocity accelerates drift speed, making the 13 oscillators more chaotic in real time.

---

**Preset S03 — Migration Path**

The whale in transit. Random arpeggio pattern at medium rate.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 6.5 | Present, not dominant |
| BITE | 1.5 | Near clean |
| SCURRY | 8.0 | Arpeggiator Random pattern, 1/8T rate, 3-octave range |
| TRASH | 0.0 | Clean |
| PLAY DEAD | 6.0 | Moderate decay — each note breathes |

- **Arpeggiator settings:** Pattern = Random, Rate = 1/8T (triplet — gives natural swing), Octaves = 3
- **Mojo:** 7.0
- **feliX/Oscar:** 60% Oscar / 40% feliX
- **Sonic DNA:** brightness 5, warmth 7, movement 10, density 6, space 6, aggression 3
- **Key range:** C2–C6
- **Velocity response:** Velocity scales octave range in real time — soft hits limit arpeggio to 1 octave; hard hits unlock the full 3-octave range. The randomness plus velocity-octave control is the performance instrument.

---

**Preset S04 — Countercurrent**

Two arpeggio patterns in opposing directions, achieved via filter group modulation creating the illusion of simultaneous up and down movement.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 7.0 | Strong bass foundation under the moving parts |
| BITE | 4.0 | Moderate saturation to separate the moving texture from the bass |
| SCURRY | 8.5 | Arpeggiator Down pattern + slow LFO on filter panning |
| TRASH | 2.0 | Trace grit |
| PLAY DEAD | 5.0 | Each note decays naturally |

- **Arpeggiator settings:** Pattern = Down, Rate = 1/8, Octaves = 2
- **Additional modulation:** LFO 2 on pan of filter Groups 2 and 3 in opposition (Group 2 left→right as Group 3 right→left), rate = 0.25 Hz. Creates the sensation of two voices moving toward each other.
- **feliX/Oscar:** 62% Oscar / 38% feliX
- **Sonic DNA:** brightness 5, warmth 7, movement 9, density 8, space 7, aggression 4
- **Key range:** C2–C5

---

### Family 4 — TRASH Industrial (3 presets)

*Noise, destruction, extreme saturation. The whale beached. The sound past the edge.*

TRASH presets push the bitcrusher and sample-rate reduction to their limits. These are not "musical" in the conventional sense — they are designed sounds that disrupt, distort, and deconstruct. Three presets is the right number. More would be a museum of noise; three is a collection of tools.

---

**Preset T01 — Salvage Yard**

A warm, recognizable bass tone slowly disintegrating under TRASH pressure.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 7.0 | The bass is still there — this is degradation, not erasure |
| BITE | 4.0 | Moderate saturation under the TRASH character |
| SCURRY | 2.0 | Slight movement so the noise isn't static |
| TRASH | 7.0 | Strong bitcrusher — 6-bit depth, 22kHz sample rate |
| PLAY DEAD | 5.0 | Medium release — the decay shows the degradation slowly fading |

- **feliX/Oscar:** 55% Oscar / 45% feliX
- **Sonic DNA:** brightness 6, warmth 5, movement 4, density 9, space 3, aggression 8
- **Key range:** C2–C5
- **Velocity response:** Velocity controls TRASH macro depth — soft hit = moderate crusher (4-bit), hard hit = extreme crusher (2-bit alias). The distortion character literally changes with velocity.

---

**Preset T02 — Cetacean Static**

Whale song filtered through a broken radio. Industrial noise with biological origin.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 5.5 | Present — the underlying biology is recognizable |
| BITE | 6.0 | Saturation + resonance raise the noise floor |
| SCURRY | 4.0 | Arpeggiator engaged — the static has a pattern |
| TRASH | 8.5 | Heavy bitcrusher — the signal is audibly destroyed |
| PLAY DEAD | 4.0 | Short — the noise event ends |

- **Arpeggiator settings:** Pattern = AsPlayed, Rate = 1/8T, Octaves = 1
- **feliX/Oscar:** 48% Oscar / 52% feliX — the TRASH character pushes this slightly feliX
- **Sonic DNA:** brightness 7, warmth 4, movement 7, density 10, space 2, aggression 10
- **Key range:** C2–C4
- **Velocity response:** Hard velocity disables TPDF dithering on the bitcrusher — the alias becomes harsher and more digital at high velocity. Soft velocity uses full dithering for a slightly more musical lo-fi character.

---

**Preset T03 — Final Depth**

Maximum TRASH. Beyond music, into sound design. This is the pack's most extreme preset.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 3.0 | Low — the sub has been mostly eaten by TRASH |
| BITE | 7.0 | Distortion + TRASH creates compound harmonic destruction |
| SCURRY | 6.5 | Fast modulation — the noise is not static |
| TRASH | 10.0 | Maximum — 4-bit depth, 8kHz sample rate |
| PLAY DEAD | 3.5 | Short — this sound doesn't linger |

- **feliX/Oscar:** 45% Oscar / 55% feliX
- **Sonic DNA:** brightness 8, warmth 2, movement 9, density 10, space 1, aggression 10
- **Key range:** C3–C5 only — below C3, the bitcrusher artifact frequencies become unmusical. This preset is designed for mid-register use as an effect or texture layer.
- **Velocity response:** At velocity 127, SCURRY is pushed to 8.0 and arpeggiator engages at maximum rate. Hard velocity makes the industrial noise rhythmically active; soft velocity leaves it as a held texture.

---

### Family 5 — MOJO Center (4 presets)

*The intentional middle. Neither clean nor destroyed. Mojo at full biology.*

These four presets answer the question: what happens when you refuse to extremize XObese? BELLY=5, BITE=5, SCURRY=5, TRASH=3, PLAY DEAD=7 is not a compromise — it is a specific, designed center-state where all five systems are active at moderate levels simultaneously. The Mojo drift at 9.0 is the variable that makes this interesting. At the dial center, all energy is on the biological drift.

The MOJO Center presets are the philosophical heart of the pack.

---

**Preset M01 — Mojo Rising (Title Preset)**

The namesake preset. All five macros at center. Mojo at maximum. The whale just being the whale.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 5.0 | Center — sub present, not dominant |
| BITE | 5.0 | Center — warm saturation, not clean but not distorted |
| SCURRY | 5.0 | Center — arpeggiator at 1/8 rate, slow LFO on filter, moderate activity |
| TRASH | 3.0 | Below center — slight bitcrusher texture, just enough to add lo-fi warmth |
| PLAY DEAD | 7.0 | Above center — notes hold and breathe, 2.5-second release |

- **Mojo:** 9.0 — maximum biological drift
- **feliX/Oscar:** 70% Oscar / 30% feliX — the engine's natural resting polarity
- **Sonic DNA:** brightness 5, warmth 7, movement 6, density 7, space 7, aggression 5
- **Key range:** C2–C6
- **Velocity response:** Velocity controls Mojo drift rate — the harder you play, the faster the 13 oscillators drift. Soft playing is warm and stable; hard playing is alive and slightly unpredictable. This is the velocity-as-biology approach: you are controlling how "awake" the engine is.

**What this sounds like:** A warm, wide bass chord with internal shimmer from the 13 oscillators drifting at different rates. The arpeggiator traces a slow 1/8 pattern. The saturation adds even harmonics that the ear hears as warmth. The bitcrusher trace gives it an analog cassette character. Nothing is extreme. Everything is present. This is XObese in its natural state.

---

**Preset M02 — Open Water**

The center position in motion. SCURRY raised slightly to make the center-state explore itself.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 5.0 | Center |
| BITE | 5.0 | Center |
| SCURRY | 6.5 | Raised — arpeggiator more active, LFO faster, drift accelerated |
| TRASH | 3.0 | Below center |
| PLAY DEAD | 7.0 | Above center |

- **Mojo:** 9.0
- **Arpeggiator settings:** Pattern = UpDown, Rate = 1/8, Octaves = 2
- **feliX/Oscar:** 65% Oscar / 35% feliX
- **Sonic DNA:** brightness 5, warmth 7, movement 8, density 7, space 6, aggression 5
- **Key range:** C2–C5
- **Velocity response:** Velocity controls arpeggiator gate length AND Mojo rate simultaneously. Soft = legato arpeggio, slow drift. Hard = staccato, fast drift. The relationship between legato and staccato playing completely changes the rhythmic texture.

**What this sounds like:** The center-state in motion. The 13 oscillators are drifting faster. The arpeggio covers 2 octaves in an up-down pattern. The saturation adds harmonic density to each arpeggio note. The overall character is expansive, moving, warm, and complex. This is what XObese does when you let it swim.

---

**Preset M03 — Benthic Pressure**

The center-state weighted down. BELLY raised to 7 while BITE drops to 4 — the same Mojo biology, but with gravity.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 7.0 | Above center — the sub reasserts authority |
| BITE | 4.0 | Below center — saturation recedes, warmth increases |
| SCURRY | 5.0 | Center — moderate movement, not fast |
| TRASH | 3.0 | Below center |
| PLAY DEAD | 7.0 | Above center |

- **Mojo:** 8.5
- **feliX/Oscar:** 75% Oscar / 25% feliX
- **Sonic DNA:** brightness 4, warmth 8, movement 5, density 8, space 6, aggression 4
- **Key range:** C1–C4
- **Velocity response:** Velocity controls BELLY-to-BITE balance in real time — soft hits are pure BELLY character (sub dominant, warm); hard hits shift to moderate BITE (saturation introduces harmonic content). The same preset played with different intensity sounds like two related but distinct instruments.

**What this sounds like:** The title preset but heavier. The sub is more present. The saturation is lighter. The overall character is warm, massive, and slightly slow — like something very large moving through water at depth. The Mojo drift gives it life without making it feel agitated. This is XObese as a foundation instrument.

---

**Preset M04 — Mojo Control (Seance Blessing Reference)**

The B015 Seance Blessing preset — designed explicitly to demonstrate the Mojo axis as an orthogonal organic/digital dimension. This preset uses the center macro positions as a stable platform to show what Mojo does across its range.

| Macro | Value | Rationale |
|-------|-------|-----------|
| BELLY | 5.0 | Center |
| BITE | 5.0 | Center |
| SCURRY | 5.0 | Center |
| TRASH | 3.0 | Below center |
| PLAY DEAD | 7.0 | Above center |

- **Mojo:** Maps to Q-Link 1 — the performer controls Mojo in real time
- **feliX/Oscar:** 70% Oscar / 30% feliX
- **Sonic DNA:** brightness 5 (at Mojo 0) → 6 (at Mojo 10), warmth 5 → 8, movement 4 → 7, density 8 (constant), space 6, aggression 4 (constant)
- **Key range:** C2–C6
- **Velocity response:** Standard center-state velocity response — velocity controls filter cutoff brightness, standard D001 compliance.

**Q-Link assignments for this preset:**
- Q1: Mojo (the entire point of this preset)
- Q2: BELLY
- Q3: BITE
- Q4: PLAY DEAD (release time)

**What this sounds like:** This preset is designed to be performed, not just played. At Mojo 0, XObese is a digital supersaw — clinical, precise, impressive. As Q1 is raised, the 13 oscillators begin to drift independently. By Mojo 5, warmth is clearly audible. By Mojo 8, the sound is alive — breathing rather than sustaining. At Mojo 10, no two notes sound identical because each note triggers fresh random-walk drift seeds for all 13 oscillators. The five macro positions provide a stable harmonic/dynamic context; Mojo is the dimension of life within that context. This is B015 as a playable instrument.

---

## XPN Keygroup Structure

### Standard Keygroup Architecture

```
Per Preset:
  Octave spread: 5 octaves (C2–C6) standard; reduced for extreme presets (C1–C4 for BELLY Heavy, C3–C6 for TRASH extremes)
  Sample count per note: 4 (one per velocity layer + one additional for hard attack transient)
  Velocity layers: 4
    Layer 1: velo 0–31   (whisper — filter at 15% open, Mojo rate floor)
    Layer 2: velo 32–63  (soft — filter at 40% open)
    Layer 3: velo 64–95  (medium — filter at 70% open, standard Mojo)
    Layer 4: velo 96–127 (hard — filter fully open, elevated Mojo rate)
  Round-robin cycles: 2 per velocity layer (Mojo drift guarantees no two samples identical)
  Total samples per preset: 5 octaves × 12 semitones × 4 velocity layers × 2 RR = 480 samples

  Practical note spacing for XPN keygroup zones:
    Every minor-third (3 semitones) sampled from C2 to C6
    Zones: C2, Eb2, F#2, A2, C3, Eb3, F#3, A3, C4, Eb4, F#4, A4, C5, Eb5, F#5, A5, C6
    = 17 root notes sampled, zones cover ±1.5 semitones in each direction
```

### Velocity Layer Design Philosophy

The four-layer architecture is not just about amplitude. Each layer is designed with a different BELLY/BITE balance baked in at the render stage:

| Layer | Velocity | BELLY offset | BITE offset | Character |
|-------|----------|-------------|-------------|-----------|
| 1 (whisper) | 0–31 | +1.0 | -1.5 | Warmer, darker, more sub |
| 2 (soft) | 32–63 | +0.5 | -0.5 | Near preset nominal |
| 3 (medium) | 64–95 | 0 | 0 | Exact preset settings |
| 4 (hard) | 96–127 | -0.5 | +1.5 | Brighter, more harmonic, more saturated |

This means velocity in OBESE: Mojo Rising shapes timbre fundamentally (D001 compliance) — not just amplitude. A whisper-soft hit is a different version of the sound, not just a quieter one.

### MOJO Center Presets — Reduced Sample Count

The four MOJO Center presets use a reduced keygroup structure:

```
Root notes: Every 4 semitones (C2, E2, Ab2, C3, E3, Ab3, C4, E4, Ab4, C5, E5, Ab5, C6)
= 13 root notes
Velocity layers: 4 (same as standard)
Round-robin: 3 per layer (Mojo drift produces maximum variation — 3 RR is enough to feel infinite)
Q-Link remapping in .xpn header: Q1 = Mojo (B015 reference)
```

### XPN File Naming Convention

```
OBESE_MojoRising_[FamilyCode][PresetNumber]_[PresetName].xpn

Family codes:
  B = BELLY Heavy
  A = BITE Aggressive
  S = SCURRY Active
  T = TRASH Industrial
  M = MOJO Center

Examples:
  OBESE_MojoRising_B01_TectonicSub.xpn
  OBESE_MojoRising_M01_MojoRising.xpn
  OBESE_MojoRising_T03_FinalDepth.xpn
```

Pack folder: `OBESE Mojo Rising/`
Subfolder: `OBESE Mojo Rising/Keygroups/`
Cover art: `OBESE Mojo Rising/Artwork/` (Hot Pink `#FF1493` theme, whale silhouette)

---

## Seance Demands — Pack Compliance

From the XPN Tools Seance (5 demands — all must be addressed in this pack):

| Demand | Compliance |
|--------|-----------|
| Transitional morph snapshots | MOJO Center family provides the explicit center-state documentation. M04 (Mojo Control) is designed as a live Mojo morph instrument. |
| Dry variants | Each BELLY Heavy and MOJO Center preset includes a "dry" Q-Link configuration (Q4 = zero FX send) documented in .xpn metadata. |
| Stereo width velocity | All presets: hard velocity (96–127) triggers full 4-group stereo spread (center → left → right → wide). Soft velocity collapses Groups 2–3 toward center. |
| Emotional engine distinctness | Each of the 20 presets has a `"characterBrief"` field in .xpn metadata. Examples: B01 = "The ocean floor, still and dark"; A05 = "The surface break, violent and fast"; M01 = "Neither clean nor destroyed — just alive." |
| Invisible intelligence | Mojo drift seeds are randomized per note-on trigger using xorshift32 — no two notes from the same preset sound identical. This is built into the engine; the pack preserves Mojo ≥ 6.0 on all presets to keep this active. |

---

## DNA Distribution Across Pack

No two presets may share identical DNA vectors. The full 20-preset suite must span the following coverage:

| DNA | Pack Min | Pack Max | Required spread |
|-----|---------|---------|-----------------|
| Brightness | 2 | 9 | ≥ 3 presets below 4; ≥ 3 presets above 7 |
| Warmth | 2 | 10 | ≥ 3 presets above 8; ≥ 2 presets below 5 |
| Movement | 1 | 10 | ≥ 3 presets above 9; ≥ 3 presets below 3 |
| Density | 6 | 10 | XObese is constitutionally dense — floor at 6 for all presets |
| Space | 1 | 8 | ≥ 2 presets above 7; ≥ 3 presets below 3 |
| Aggression | 1 | 10 | ≥ 2 presets at 10; ≥ 2 presets below 3 |

The BELLY Heavy family anchors the warmth-high / brightness-low / movement-low quadrant. The BITE Aggressive family anchors the aggression-high / brightness-high end. The MOJO Center family owns the density-8/warmth-7 middle ground. The TRASH family owns the aggression-10 extreme. The SCURRY family owns the movement-9/10 positions. Together, they cover the full DNA space without clustering.

---

*XO_OX Designs | OBESE: Mojo Rising | XObese character pack*
*Hot Pink `#FF1493` | The whale, thirteen harmonics, Mojo at nine*
