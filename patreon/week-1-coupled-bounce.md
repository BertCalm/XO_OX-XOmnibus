# FOUNDER'S FREQUENCY -- Week 1
## Coupled Bounce
**Engine(s)**: ONSET + OVERBITE | **Mood**: Entangled | **Coupling**: Rhythm->Blend + Amp->Filter

---

### The Story

Marcus from the Producer's Guild hit me up last November with a challenge: "I want a one-finger trap beat that doesn't sound like a loop." He makes hard-hitting trap and drill, the kind of stuff where the kick and the bass are so locked together they feel like one instrument. But he was tired of sidechaining. Tired of drawing volume automation. Tired of the bass and the kick being two separate things that he had to manually glue together every single time.

So I built Coupled Bounce. It's the first Founder's Signature preset, and it's the one that made me realize coupling is not just a feature -- it's a philosophy. The kick doesn't trigger the bass. The kick IS the bass. They're one organism.

I chose this preset to lead the series because it's the one that makes people's jaws drop the fastest. One note. C1. Hold it. That's a full trap bounce -- kick, sub bass, and the interaction between them -- from a single key press.

### The Science

Let's talk about what's actually happening when you press that key.

**The Kick -- ONSET's BridgedT Oscillator**

The kick voice uses ONSET's BridgedT oscillator, which models the actual circuit topology of the Roland TR-808 kick drum. In the original 808 hardware, a bridged-T network creates a resonant circuit that produces a decaying sine wave -- the fundamental "boom." The pitch starts high (the transient click) and sweeps down exponentially to the fundamental frequency.

We're modeling this with a sine oscillator fed through an exponential pitch envelope. When you trigger the kick, the pitch starts around 150-200 Hz (the transient) and drops to the fundamental at about 55 Hz (A1) over roughly 30-50 milliseconds. That exponential curve is key -- it's not linear. The pitch drops fast at first, then slows as it approaches the fundamental. This is what gives an 808 kick that characteristic "bwoooom" rather than a flat thud. The mathematics are the same as a capacitor discharging through a resistor: V(t) = V0 * e^(-t/RC). Nature uses exponentials everywhere. So does good synthesis.

On top of the BridgedT fundamental, there's a noise burst layer (the "click" at the very start) processed through the TransientDesigner, which shapes the first 5ms of the hit with a pitch spike and filtered noise.

**The Bass -- OVERBITE's Belly Oscillator**

Underneath the kick, OVERBITE is running its Belly oscillator -- a pure sine sub with a table lookup (2048-sample wavetable, linearly interpolated). Belly is oscillator A in OVERBITE's voice architecture: specifically designed for weight, not harmonics. It's the "lure" of the anglerfish. Warm, round, inviting. A sine wave at the same fundamental pitch as the kick, but sustained.

**The Coupling -- Where the Magic Happens**

Here's the part that makes this preset special. Two coupling routes are active:

1. **Rhythm->Blend** (ONSET -> OVERBITE, amount: 0.55): ONSET's rhythmic pattern data modulates OVERBITE's internal blend parameter. When the kick hits, OVERBITE's character shifts toward its Bite chain. When the kick is silent, OVERBITE relaxes back toward pure Belly. The bass literally changes character with the rhythm.

2. **Amp->Filter** (ONSET -> OVERBITE, amount: 0.40): ONSET's amplitude envelope directly opens OVERBITE's filter cutoff. Each kick transient pushes the bass filter open, adding upper harmonics that coincide with the kick's attack. As the kick decays, the filter closes, and the bass returns to its pure sub state.

The result: press one key, and the kick punches through the bass. The bass breathes with the kick. They share a frequency band without fighting because they're literally modulating each other's timbral space in real time. This isn't sidechain compression faking interaction -- this is actual per-sample coupling where the kick's amplitude is an input to the bass's filter coefficient calculation on every single sample.

Think of it like this: sidechain compression is a bouncer keeping two people apart. Coupling is choreography -- they dance together.

### How to Use It

**1. Trap / Drill Foundation (Key: C1-E1)**
Play single notes in the low register. The coupling is tuned so that each note gives you a complete kick-bass event. Play C1 for the deepest boom, move up to D1 or E1 for a slightly tighter feel. At 140 BPM, quarter notes on C1 give you an instant trap foundation. Add a hi-hat pattern from another track and you're halfway to a finished beat.

**2. Half-time Bounce (Key: F1-A1, Tempo: 70-80 BPM)**
Play dotted quarter notes or half notes in the F1-A1 range. At this tempo, the coupling has time to fully breathe -- you'll hear the bass filter open and close with each kick hit, creating a pumping motion that grooves without any compressor involved. Perfect for phonk or slowed-down drill.

**3. Rolling 808 (Key: C1, held + pitch bend)**
Hold C1 and slowly sweep the pitch bend wheel down. Because the coupling is per-sample, the pitch relationship between kick and bass stays locked as you glide. You get that classic rolling 808 slide, but with both engines moving together. Turn M2 (MOVEMENT) up to 60% to add wow modulation to the slide.

**4. Melodic Trap (Keys: C1-C2 chromatic)**
Play a melody in the bass register. Each note triggers a kick-bass coupling event, but the pitch changes give you melodic content. The coupling amounts are tuned so the kick transient stays punchy regardless of pitch, while the bass follows the note. This is where Coupled Bounce becomes a melodic instrument rather than a drum machine.

**5. Sound Design -- Decouple and Distort**
Pull M3 (COUPLING) down to zero. Now you have ONSET and OVERBITE running independently -- a drum machine and a bass synth that you can mix freely. Slowly bring M3 back up and listen to how the coupling gradually binds them together. This is a great way to understand what coupling actually does to sound.

### Macro Guide

| Macro | What It Does | Sweet Spot |
|-------|-------------|-----------|
| M1 (CHARACTER) | Blends ONSET's kick between Circuit (808 analog) and Algorithm (FM digital). At 0, pure analog warmth. At 1, metallic FM attack. | 0.25 -- warm with a hint of digital bite on the transient |
| M2 (MOVEMENT) | Adds wow modulation to OVERBITE's filter and subtle pitch drift to both engines. Creates analog-style movement. | 0.35 -- enough drift to feel alive, not enough to sound detuned |
| M3 (COUPLING) | Master coupling intensity. At 0, ONSET and OVERBITE are independent. At 1, they're fused. Controls both Rhythm->Blend and Amp->Filter amounts proportionally. | 0.55 (default) -- the sweet spot where kick opens bass without overwhelming it |
| M4 (SPACE) | Blend between dry and ONSET's FX rack (delay + reverb). Adds room around the kick while keeping the bass dry and focused. | 0.15 -- just a touch of room on the kick. Above 0.4, the bass starts getting washy |

### Pair It With

- **"Coral Investigations"** (Week 3 preset, OBLONG solo) -- Layer Coupled Bounce on the left hand and play Coral Investigations with the right. The CuriosityEngine's behavioral LFOs add organic movement above the trap foundation. The combination of locked rhythm below and wandering texture above is a full production in two presets.

- **Any ODYSSEY atmosphere preset** -- ODYSSEY's Voyager Drift pads sit perfectly above the kick-bass coupling. The drift movement in ODYSSEY is slow enough that it never fights the rhythmic coupling. Try "Velvet Drift" or "Deep Meridian" from the Atmosphere category.

- **OUROBOROS chaos presets** -- For experimental producers: layer an OUROBOROS strange attractor preset at low volume behind Coupled Bounce. The chaotic modulation creates micro-variations in the coupling that make every bar slightly different. Controlled unpredictability.

### The Download

This preset is included in the **Founder's Signature Vol. 1** XPN pack -- 5 presets, free, for XOmnibus and MPC.
[Download link placeholder]

---
*Next week: Belly to Bite -- the 5-macro beast that all 8 Seance ghosts blessed.*
*Founder's Frequency is a free weekly series from XO_OX Designs.*
