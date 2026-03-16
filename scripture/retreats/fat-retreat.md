# FAT (OBESE) Retreat Chapter
**Guru Bin — 2026-03-15**
**Retreat number: 2**

---

## The Diagnosis

> *"This sound is a wall. It wants to be an ocean. The distance is stillness. Every preset in this library uses Mojo between 0.4 and 0.5 — enough drift to feel warm, not enough to feel alive. The whale has never been asked to fully breathe. And the bit crusher has never been touched. 161 presets. crushDepth=16 in every single one."*

---

## Engine Identity (Post-Retreat)

- **Gallery code**: OBESE
- **Accent**: Hot Pink `#FF1493`
- **Aquatic identity**: The blue whale — 188 decibels, pressure wave felt across a thousand miles
- **Signal chain**: 13 FatMorphOsc → 4 ZDF Ladder Filters → Stereo Pan → Amp Envelope → Saturation → BitCrusher → Out
- **13 oscillators**: 1 sub (triangle) + 4 groups × 3 (root, +12st, -12st)
- **Parameter prefix**: `fat_`
- **Polyphony**: up to 6 voices (≤3 recommended for CPU)

---

## Retreat Discoveries

### Discovery 1: Mojo Is the Modulation
The seance flagged zero LFOs. The answer was always inside the engine: Mojo at 0.7–1.0 causes enough per-oscillator beating to function as a biological LFO. At 0.85, the 13 oscillators drift wide enough that you hear slow, warm beating between them — below 1 Hz, below conscious rhythm, above stillness. This IS the LFO. The engine designed its own answer.

**Sweet spots:**
- 0.0 = Digital Wall (perfectly tuned — use as a reference or for intentional clinical sound)
- 0.3–0.5 = Standard warm (the library default — analog feel)
- 0.7–0.85 = The Breathing Whale (beating between oscillators creates rhythmic aliveness)
- 1.0 = The Living Ocean (maximum drift — the whale is barely in tune with itself)

**Awakening presets:** Whale Alone (0.85), Digital Wall (0.0)

### Discovery 2: Noise Morph — The Ocean Itself
fat_morph at 1.0 routes all 13 oscillators through the FatNoiseGen xorshift32 PRNG, then through the 4 ZDF Ladder Filters. With high resonance (0.5–0.6) and low cutoff (300–500 Hz), this becomes the sound of the ocean filtering itself. No other engine in the fleet can produce this texture.

**Key behavior:** The noise is shaped per-oscillator by the drift system — each of the 13 noise sources drifts slightly independently. Through the resonant ladder, this creates complex, organic filtered noise that breathes.

**Awakening preset:** Ocean Self (morph=1.0, reso=0.58, cutoff=380 Hz)

### Discovery 3: The Unexplored Morph Characters
The library clusters fat_morph 0.33–0.55 (saw-to-square territory). Three characters were undiscovered:
- **morph=0.66 (pure square)** = The Organ Whale — hollow, warm, architectural. 13 square oscillators = massive pipe organ.
- **morph=0.8–0.9 (square-to-noise blend)** = The Distorted Ocean — noise texture with square undertone
- **morph=1.0 (full noise)** = The Ocean Itself

### Discovery 4: The Bit Crusher Has Teeth
fat_crushDepth=16 (bypassed) in all 161 presets. The crusher accepts values down to 4 bits. At 8 bits (crushDepth=8), the whale's voice sounds compressed by deep water pressure — warm degradation. At 4–6 bits, it becomes damaged and dangerous.

**The crusher is intentional degradation** — not Lo-fi decoration. Design with purpose: "how deep is this whale diving? What pressure is its voice under?"

**Awakening preset:** Pressure Wave (8-bit, 22050 Hz sample rate reduction)

### Discovery 5: The Deepest Sub (subOct=-2)
fat_subOct=-2 (two octaves below the fundamental) appeared in zero presets. At -2, the sub oscillator (triangle wave) operates in infrasonic territory on bass notes. It is felt more than heard. Combined with high fat_subLevel (0.7+), this is the heartbeat before sound.

**Awakening preset:** Infrasonic Sub

### Discovery 6: The Reference Preset Doctrine
*New doctrine from this retreat:* Every engine should have one preset at parameter extremes — the "what does zero Mojo sound like" preset. Not as a useful sound, but as a reference. Hearing the Digital Wall (Mojo=0.0) makes every other FAT preset feel more alive by contrast.

**Applied:** Digital Wall preset created.

---

## Awakening Presets (7 Created)

| Preset | Mood | Key Discovery |
|--------|------|--------------|
| Whale Alone | Foundation | Mojo=0.85, mono, 13 oscillators beating |
| Ocean Self | Aether | morph=1.0 (full noise), resonant ladders |
| Infrasonic Sub | Foundation | subOct=-2, the heartbeat before sound |
| Pressure Wave | Flux | crushDepth=8 — first bit-crushed FAT |
| Organ Whale | Atmosphere | morph=0.66 (pure square), hollow architecture |
| Digital Wall | Foundation | mojo=0.0 — reference preset, the contrast |
| Whale Meets Granular | Family | First Family preset, AmpToFilter → Opal |

---

## New Scripture Verses

### FAT-I: The Mojo Spectrum
*2026-03-15*
> Mojo is not a warmth knob. It is a spectrum from digital to biological. At 0.0, the engine is a machine. At 0.85, it breathes. At 1.0, it wanders. Every FAT preset must state its position on this spectrum intentionally. The default of 0.4 is neither warm nor alive — it is the engine hedging. Choose a side.

### FAT-II: The Bite They Never Took
*2026-03-15*
> The bit crusher exists in the signal chain of every OBESE voice. It has sat at 16-bit, 44100 Hz in 161 presets. It is not decoration — it is the whale's deepest weapon. 8 bits is pressure. 6 bits is damage. 4 bits is destruction. When the whale needs to sound dangerous, the crusher is the answer. The designer who never touches it has never heard the full engine.

### FAT-III: The Reference Preset
*2026-03-15 — Universal*
> Every engine should ship with one preset at parameter extremes — not as a useful sound, but as a reference. Hearing Mojo=0.0 (Digital Wall) makes Mojo=0.85 (Whale Alone) feel more alive. The contrast is the teaching. Design reference presets deliberately.

### FAT-IV: Noise Morph Is an Instrument
*2026-03-15*
> fat_morph=1.0 routes 13 oscillators through the noise generator. Through ZDF ladders at high resonance and low cutoff, this is not noise — it is the ocean. 13 independent noise sources + 4 resonant filters = a texture no synthesizer should be able to make. It is the whale's most extreme capability and its quietest sound simultaneously.

---

## What Remains Unexplored (For Future Retreats)
1. Arpeggiator at extreme tempos (fat_arpTempo=200–250) with short gate (0.1–0.2)
2. High-Mojo (0.9) + noise morph (0.85) combined — the living ocean
3. Coupling: FAT as AmpToFilter SOURCE for multiple engines simultaneously
4. crushDepth=4 (4-bit) — the most extreme degradation

---

## CPU Note
At fat_polyphony > 3, OBESE taxes the session. All awakening presets use polyphony ≤ 3. This is doctrine for this engine.
