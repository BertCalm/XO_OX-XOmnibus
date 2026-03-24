# OCHRE Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OCHRE | **Accent:** Copper Patina `#B87333`
- **Parameter prefix:** `ochre_`
- **Creature mythology:** The copper-framed upright piano in the room where you actually practice — intimate, responsive, warm. Not a concert instrument meant to fill a hall, but a household instrument that rewards nuance and punishes laziness. Copper conducts heat and sound with equal eagerness: every touch is immediately audible, every dynamic decision transparent. The piano that teaches you to listen because it gives you back exactly what you put in.
- **Synthesis type:** Modal synthesis — 16 IIR modal resonators per voice (copper upright ratios), Hunt-Crossley hammer (upright geometry), Caramel saturation waveshaping, 3 body types, 12-string sympathetic network, copper thermal drift
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER (conductivity + caramel), M2 MOVEMENT (LFO2 breath), M3 COUPLING (sympathetic depth), M4 SPACE (body depth + decay)

---

## Pre-Retreat State

XOchre sits at the warm-and-responsive corner of the Kitchen Quad. Where XOven is geological and dark, XOchre is immediate and intimate. Copper's lower acoustic impedance (Z=33.6 MRayl vs cast iron's 36.7 MRayl) means better high-frequency energy transmission at the string-body interface — brighter transients, faster decay, more transparent dynamic response.

The Seance Council scored XOchre 8.55/10. Moog praised the Caramel saturation as "the most musically useful nonlinearity I have heard in a piano synthesizer — not distortion, transformation." Ten existing presets cover Foundation (Warm Keys, Practice Room, Studio Grand, Bright Attack), Atmosphere, Prism, and several others. The Council noted gaps: nothing exploring high-conductivity territory (copper as responsive metal, not just warm acoustic), no Parlour body type with LFO2 breath combination, no extreme caramel showcase.

The conductivity axis is the engine's most underused feature. At low conductivity (0.0–0.2), copper behaves like a heavy wood or brass instrument: warmer attack, slower decay. At high conductivity (0.7–1.0), copper's thermal character dominates: faster attack shaping, more HF content in the excitation, the distinctly "copper" brightness that is warmer than iron but brighter than wood. Few existing presets explore conductivity above 0.6.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

There is a piano in the front parlour of a Victorian terrace house in Bristol, 1887. It is an upright — it had to be, to fit through the door. Its frame is copper, which was fashionable then, and the copper has turned the colour of autumn leaves in the forty years since it was installed. The woman who plays it every morning plays well. She plays with the window open in summer. The copper body has absorbed forty years of morning practice sessions, forty years of scales and sonatas and attempts at Chopin.

Now play a middle C. The attack is immediate — copper is not iron, it does not resist. The note arrives completely within the first few milliseconds. Then it begins to transform. The caramelization begins. Under sustained pressure, copper's vibrational energy reshapes itself: the harsh overtones round off, the fundamental strengthens, the tone becomes sweeter, more complex. Not simpler — more layered. Like sugar becoming caramel.

This is the paradox of XOchre: it responds instantly (conductivity) and transforms slowly (caramel). The attack is transparent, but the sustain is alchemical. Play softly and get honesty. Play hard and get transformation.

The LFO2 on breath setting (slow sine at 0.08 Hz, into caramel depth) is the subtlest feature in the engine. It makes the piano sound like it is breathing — like the instrument itself is alive and slightly asthmatic, the tone wavering barely perceptibly between notes. You can hear it or not hear it depending on what you are listening for. But you feel it in both cases.

---

## Phase R2: The Signal Path Journey

### I. The Hammer — Upright Geometry

The upright piano hammer strikes from below (not from above as in a grand). This creates shorter contact time (3–6ms vs. grand's 4–8ms), faster response, and more immediate dynamic resolution. `OchreHammerModel` implements this difference explicitly.

`ochre_hardness` (0.0–1.0, default 0.4) drives the contact duration and excitation character. At 0.0: 3ms contact, very soft felt, only fundamental and first few modes excited. At 1.0: 0.5ms contact, hard synthetic hammer, full modal excitation, high noise mix.

The `caramelAmount` in the excitation signal is computed as `velocity² × (0.5 + conductivity × 0.5)`. This means caramel is velocity-sensitive AND conductivity-sensitive — playing softly on a high-conductivity preset yields a clean copper transient, while playing hard yields a sweetened, harmonically rich onset. This is the Maillard mechanism applied to copper, not iron: less aggressive, more confectionary.

**Playing insight:** At hardness 0.25–0.40, the upright geometry produces the fastest dynamic response in the fleet. Velocity changes are immediately audible in both amplitude and timbre. This is the "responsive" character that distinguishes an upright from a grand — more intimate, less forgiving.

### II. The Conductivity Axis

`ochre_conductivity` (0.0–1.0, default 0.5) is the defining XOchre parameter. It affects:
1. **Excitation character:** `noiseMix = hardness² × (0.3 + conductivity × 0.7)` — higher conductivity means more HF content in the hammer excitation
2. **Caramel amount:** `caramelAmount = velocity² × (0.5 + conductivity × 0.5)` — conductivity scales the saturation character
3. **Mallet cutoff:** `cutoffMult = 2.0 + hardness × 22.0 + conductivity × 8.0` — conductivity pushes the spectral cutoff higher, letting more upper modes be excited

At conductivity 0.0: XOchre behaves like a very warm upright, almost wood-like in its HF absorption. At conductivity 1.0: the copper character is unmistakable — bright attack, fast decay, immediate. The sweet spot is 0.4–0.7, where copper's responsiveness and warmth coexist.

### III. Caramel Saturation

`ochre_caramel` (0.0–1.0, default 0.3) is the post-modal saturation. The waveshaping function applies `fastTanh` to the modal output, with the saturation amount driven by the preset value plus velocity modulation. Light caramel (0.1–0.25): barely audible but adds the characteristic rounding to harsh upper partials. Heavy caramel (0.6+): complex harmonic enrichment, the "sweetened under pressure" character that is genuinely unlike any other piano synthesizer nonlinearity in the fleet.

Caramel does not add distortion harmonics. It redistributes existing modal energy by soft-clipping the highest amplitude modes, which is audibly different from hard-clipping. The result is a sound that appears to have more body even at lower levels — because the energy that would have been in sharp transients is now in a sustained harmonic cloud.

### IV. Three Body Types

`ochre_bodyType` (0=Practice Room, 1=Parlour, 2=Studio) selects the acoustic environment:

- **Practice Room (0):** Dry, close, with a bright high-frequency character that emphasizes the copper's quick attack. The piano in the room where learning happens — not flattering, but honest. Conductivity and hardness are at their most transparent here.
- **Parlour (1):** Warm bloom, the most charming body type. A salon piano in a room with heavy curtains and a fireplace. The body resonances are broader and lower, giving chords a cushioned warmth. Best with medium caramel and sympathetic resonance.
- **Studio (2):** Balanced and professional. Controlled room, correct monitoring distance, accurate reproduction. The most neutral body — best for producers who want the copper character without a room character added.

`ochre_bodyDepth` (0.0–1.0, default 0.4) crossfades between dry modal output and body-processed signal. At 0.6–0.75, the body provides a noticeable character anchor. At 0.9+, the room dominates and the individual modal modes blend into the body's acoustic housing.

### V. The Breath — LFO2 on Caramel

The most musically useful modulation routing: LFO2 (slow sine, 0.04–0.2 Hz) into the effective caramel depth. At a depth of 0.15–0.25, the saturation character gently oscillates — notes played at the caramel peak have a slightly rounder, richer quality; notes played at the trough are slightly cooler and more transparent. The oscillation is below the threshold of conscious awareness but above the threshold of felt experience.

Setting: `lfo2Rate 0.08`, `lfo2Depth 0.20`, `lfo2Shape 1` (sine). The MOVEMENT macro multiplies lfo2Depth, allowing the breath to be engaged and deepened from the macro controls.

---

## Phase R3: Preset Design

### Awakening Preset 1 — "Copper Breath" (Organic)

The LFO2 breath showcase. Slow sine LFO2 into caramel, Parlour body, medium conductivity. The piano that breathes.

- conductivity 0.45, caramel 0.40, bodyType 1, bodyDepth 0.60
- lfo2Rate 0.08, lfo2Depth 0.22, lfo2Shape 1 (sine)
- hardness 0.30, sympathy 0.40, thermalDrift 0.40, decay 2.5
- DNA: brightness 0.35, warmth 0.85, movement 0.40, density 0.50, space 0.45, aggression 0.10

### Awakening Preset 2 — "High Conductivity" (Prism)

Full copper character — maximum conductivity reveals the engine's brightest, most immediate face. Tight decay, Studio body, hardness moderate.

- conductivity 0.88, hardness 0.50, bodyType 2, bodyDepth 0.50
- caramel 0.20, decay 1.0, brightness 14000.0, sympathy 0.15
- thermalDrift 0.55, lfo1Rate 0.3, lfo1Depth 0.08
- DNA: brightness 0.75, warmth 0.50, movement 0.20, density 0.40, space 0.30, aggression 0.40

### Awakening Preset 3 — "Caramel at Depth" (Deep)

Maximum caramel with slow decay and Parlour body. The saturation character at full expression — harmonic richness piled on top of itself.

- caramel 0.78, bodyType 1, bodyDepth 0.70, decay 5.5
- conductivity 0.35, hardness 0.25, sympathy 0.50
- brightness 5000.0, thermalDrift 0.55, lfo1Rate 0.07, lfo1Depth 0.06
- DNA: brightness 0.30, warmth 0.92, movement 0.20, density 0.70, space 0.60, aggression 0.05

### Awakening Preset 4 — "Afternoon Practice" (Foundation)

The intimate, real upright piano sound. Practice Room body, medium everything, the honest center of the engine.

- bodyType 0, bodyDepth 0.55, conductivity 0.50, hardness 0.38
- caramel 0.28, decay 1.8, sympathy 0.25, thermalDrift 0.35
- brightness 9000.0, hfCharacter 0.25, filterEnvAmount 0.35
- DNA: brightness 0.50, warmth 0.65, movement 0.15, density 0.45, space 0.35, aggression 0.20

### Awakening Preset 5 — "Patina Song" (Atmosphere)

Thermal drift at high settings, sympathetic resonance, Parlour body. The feel of an old copper-framed piano that has seen many seasons — slightly detuned between voices, rich with sympathetic life.

- thermalDrift 0.75, sympathy 0.60, bodyType 1, bodyDepth 0.65
- conductivity 0.40, caramel 0.45, hardness 0.28, decay 3.2
- lfo1Rate 0.05, lfo1Depth 0.05 (imperceptible movement)
- DNA: brightness 0.35, warmth 0.90, movement 0.30, density 0.55, space 0.60, aggression 0.05

### Awakening Preset 6 — "Copper Hammer" (Kinetic)

Hard hammer, high conductivity, short decay — the percussive copper character. Staccato playing at its most focused. The copper piano as rhythmic instrument.

- hardness 0.78, conductivity 0.70, decay 0.7, bodyType 2
- caramel 0.15, sympathy 0.10, brightness 13000.0
- ampDecay 0.5, ampSustain 0.1, ampRelease 0.4
- DNA: brightness 0.70, warmth 0.45, movement 0.10, density 0.50, space 0.20, aggression 0.65

### Awakening Preset 7 — "Sugar Work" (Crystalline)

Caramel at medium, conductivity high, HF character elevated. The crystallized-sugar quality — translucent, hard, sweet. Notes ring brightly with a caramel edge.

- conductivity 0.72, caramel 0.52, hfCharacter 0.55
- bodyType 2, bodyDepth 0.45, hardness 0.42, decay 2.2
- brightness 12000.0, sympathy 0.30, thermalDrift 0.45
- DNA: brightness 0.70, warmth 0.55, movement 0.15, density 0.40, space 0.35, aggression 0.35

### Awakening Preset 8 — "Copper Lullaby" (Aether)

Minimum conductivity (warmest copper), maximum caramel, very slow lfo1 on brightness. The upright at its most intimate and warm — a child falling asleep to the sound of a piano in the next room.

- conductivity 0.08, caramel 0.65, bodyType 1, bodyDepth 0.75
- decay 4.0, sympathy 0.55, hardness 0.18, brightness 4500.0
- lfo1Rate 0.03, lfo1Depth 0.06, lfo1Shape 0 (sine — barely perceptible sway)
- DNA: brightness 0.22, warmth 0.95, movement 0.20, density 0.55, space 0.65, aggression 0.02

### Awakening Preset 9 — "Thermal Couple" (Entangled)

Designed for coupling with XOven — conductivity at midpoint, caramel open, sympathetic high. When paired with the Iron Chorale preset, the copper and iron bodies communicate through the sympathetic networks, creating a compound instrument.

- conductivity 0.55, caramel 0.40, sympathy 0.68
- bodyType 1, bodyDepth 0.60, decay 3.5, thermalDrift 0.50
- lfo2Rate 0.12, lfo2Depth 0.15, lfo2Shape 1
- DNA: brightness 0.40, warmth 0.80, movement 0.35, density 0.60, space 0.55, aggression 0.15

### Awakening Preset 10 — "Morning Room" (Luminous)

The copper upright at dawn — conductivity just above minimum, caramel light, all settings at their most transparent. A piece being practiced for the hundredth time, quietly, before the household wakes.

- conductivity 0.22, caramel 0.18, bodyType 0, bodyDepth 0.45
- hardness 0.35, decay 1.6, sympathy 0.20, thermalDrift 0.25
- brightness 8500.0, hfCharacter 0.18, filterEnvAmount 0.28
- DNA: brightness 0.45, warmth 0.70, movement 0.12, density 0.35, space 0.35, aggression 0.08

---

## Phase R4: Scripture

*Four verses for the Copper Upright*

---

**I. Conductivity**

Copper does not insulate.
It carries. It transmits.
Whatever you put in arrives immediately
on the other side.

This is why the copper piano
has no secrets.
Play badly and it tells you.
Play beautifully and it tells everyone.

---

**II. Caramel**

Sucrose is sharp, precise, crystalline.
Under heat it becomes something else —
complex, rounded, layered.
The same molecules, different arrangement.

You have not added anything.
You have transformed what was already there.
This is not distortion.
This is becoming.

---

**III. The Parlour Body**

The room shapes the note as much as the string.
Heavy curtains soften the attack.
The fireplace contributes its own harmonic.
The wallpaper has a Helmholtz resonance
that nobody calculated.

The body type is not decoration.
It is where the sound lives
after it leaves the string.

---

**IV. Practice**

She plays the same phrase for an hour.
Each iteration is different —
not because she intends it,
but because the piano has warmed,
the copper has drifted,
the sympathetic strings have remembered
the morning's other phrases.

Practice changes the instrument.
The instrument changes the practice.
Neither is in control.

---

## Phase R5: Retreat Summary

XOchre is the responsive, intimate counterpart to XOven's mass. Its defining arc is the conductivity-caramel axis: at low conductivity and high caramel, it is the warmest, softest upright imaginable; at high conductivity and medium caramel, it becomes the brightest, most immediate copper instrument in the fleet. No other engine in XOlokun offers a transparency-transformation axis of this musical clarity.

The ten presets cover the full conductivity range (0.08 to 0.88), all three body types (Practice Room, Parlour, Studio), and the LFO2-as-breath technique that gives XOchre its most distinctive living quality. The coupling-ready preset (Thermal Couple) enables the most natural Kitchen Quad pairing: copper and iron playing together through shared sympathetic resonance.

XOchre's optimal zone: `conductivity` 0.30–0.70, `caramel` 0.20–0.55, `bodyDepth` 0.45–0.70. The LFO2 breath at `lfo2Rate` 0.05–0.15 and `lfo2Depth` 0.12–0.25 is effective in almost every context. Thermal drift above 0.70 produces an unstable, aged character that suits atmospheric playing but disrupts melodic work requiring consistent intonation.

*Retreat complete — 2026-03-21*
