# XOrphica — Concept Brief

*Phase 0 | March 2026 | XO_OX Designs*
*A space goddess from an aquatic planet plays a sonic harp*

---

## Identity

**XO Name:** XOrphica
**Gallery Code:** ORPHICA
**Accent Color:** Siren Seafoam `#7FDBCA`
**Parameter Prefix:** `orph_`
**Plugin Code:** `Xorp`
**Engine Dir:** `Source/Engines/Orphica/`

**Thesis:** XOrphica is a microsound harp synth where divine strings split into two worlds — bass notes descend into the deep while treble notes ascend into the light, each transformed by independent FX chains. The goddess doesn't just play the harp — she bends, glitches, fragments, and loops it.

**Sound family:** Hybrid — plucked textures / microsound / split-register FX instrument

**Unique capability:** Note-based crossover routing — each voice is sent to a different FX chain based on its pitch. No other engine in the gallery does this. Combined with per-voice microsound fragmentation, XOrphica creates polyphonic glitch harps where bass and treble exist in different sonic universes simultaneously.

---

## Aquatic Identity

The siphonophore. Not a single animal — a colonial organism. Thousands of specialized zooids fused into something that looks like one creature but is actually a civilization. The gas-filled pneumatophore floats at the exact boundary of air and water — the surface itself. Below it, tendrils of different lengths descend into the column. Short tendrils stay in the sunlit shallows — translucent, catching light, shimmering with iridescence. Long tendrils reach deep — into cold water, into darkness, into pressure. Each tendril is a different instrument. Each depth produces a different song.

XOrphica is this creature. The harp body sits at THE SURFACE — the air-water boundary that defines the entire XO_OX mythology. The strings are the tendrils. When the goddess plucks a high note, that tendril is short — it stays in feliX's light, processed through shimmer reverb and crystal chorus and spectral smear. When she plucks a low note, that tendril reaches deep into Oscar's world — warm tape saturation, dark cathedral reverb, sub-harmonic resonance. The SURFACE macro (M3) raises or lowers the water level. At SURFACE=0, the water is low — almost everything is in the light. At SURFACE=1, the water rises — most notes are submerged in the deep. The goddess controls the ocean itself.

The microsound engine is the siphonophore's defense mechanism. When threatened, siphonophores fragment — individual zooids scatter, the colony breaks apart into shimmering pieces, then reassembles. XOrphica's FRACTURE macro does the same thing to sound: at low values, each string (voice) subtly fragments independently — micro-stutters, grain scatters, tiny glitches that add organic texture. At high values, the entire colony fractures — every voice synchronizes into a cascading glitch event, the harp shattering into a thousand luminous pieces before reforming. It's destruction as performance technique. The goddess doesn't avoid the glitch — she IS the glitch.

The Siren Seafoam accent is the color of the pneumatophore — the gas-filled float that catches sunlight from above and refracts water-light from below. Neither blue nor green. Neither air nor water. The surface itself, rendered as color.

---

## Polarity

**Position:** THE SURFACE — the air-water boundary itself
**feliX-Oscar balance:** 50/50 — the only engine that exists equally in both worlds. She IS the boundary.

The surface is the most liminal position in the water column. Everything above is feliX (light, transient, bright). Everything below is Oscar (depth, sustain, warm). XOrphica spans both because her strings extend in both directions. This makes her the ultimate coupling partner — she can receive from OPENSKY (pure feliX, from above) and OCEANDEEP (pure Oscar, from below) simultaneously. She is where the full column converges.

---

## DSP Architecture

```
MIDI Note → String Engine (Karplus-Strong waveguide)
         |   4 materials: Nylon / Steel / Crystal / Light
         |   Pluck position (bridge ↔ center)
         |   Sympathetic resonance body (tuned comb filters)
         |
         → Microsound Engine (per-voice)
         |   Fragment size: 1ms – 200ms
         |   Modes: Stutter / Scatter / Freeze / Reverse
         |   Density: 1 – 64 grains/sec
         |   Per-voice random seed → polyrhythmic glitch
         |   FRACTURE macro syncs all voices at high values
         |
         → Note-Based Crossover Router
              | Split frequency: 100Hz – 2000Hz (SURFACE macro)
              | Blend zone width: configurable (~1 octave)
              | Each voice routes to one path based on fundamental
              |
         ┌────┘                              └────┐
    LOW PATH                                HIGH PATH
    "The Deep"                              "The Light"
    (Oscar's realm)                         (feliX's realm)
         |                                       |
    Sub Harmonic Gen                        Pitch Shimmer
    (octave-down layer)                     (octave-up reverb tail)
         |                                       |
    Tape Saturation                         Micro Delay
    (warm, slow, body)                      (fast, rhythmic, ping-pong)
         |                                       |
    Dark Delay                              Spectral Smear
    (long, modulated)                       (overtone blur, ethereal)
         |                                       |
    Deep Plate Reverb                       Crystal Chorus
    (large, enveloping)                     (high-freq sparkle)
         |                                       |
         └──────── Recombine (crossfade) ────────┘
                           |
                     Master Output
```

### String Engine

Karplus-Strong waveguide physical modeling at the core — this IS a plucked string instrument.

| Material | Exciter | Damping | Character |
|----------|---------|---------|-----------|
| **Nylon** | Soft noise burst, 2ms | Heavy LP, warm rolloff | Classical harp — intimate, warm, rounded |
| **Steel** | Sharp noise burst, 0.5ms | Light LP, bright sustain | Celtic harp — metallic, bright, ringing |
| **Crystal** | Filtered click, 1ms | Very light damping, high Q | Glass harp — long sustain, bell-like, otherworldly |
| **Light** | *Not KS* — additive harmonics | N/A — sine partials bloom | Divine strings — the goddess's own material. Sine fundamentals with harmonic series that swell rather than pluck. Alien, ethereal, pure tone |

**Pluck position:** 0.0 = near bridge (bright, thin), 1.0 = center (warm, full). Maps to exciter injection point in the delay line.

**Sympathetic resonance:** A bank of 8-12 tuned comb filters modeling unplucked strings. When you pluck C3, the C4, G3, C2 comb filters gently activate. Creates a living, breathing resonance body. Resonance amount is controlled by the DIVINE macro.

### Microsound Engine

Per-voice granular fragmentation — each voice has its own micro-engine instance with independent random seed.

| Mode | Behavior | Musical Effect |
|------|----------|---------------|
| **Stutter** | Micro-repeat at rhythmic subdivisions | Glitch-hop harp, rhythmic fragmentation |
| **Scatter** | Random grain offset ± fragment size | Shimmering dissolution, like light through water |
| **Freeze** | Loop current grain indefinitely | Sustain pedal from hell — freezes the pluck mid-ring |
| **Reverse** | Play grains backward | Time reversal — the string unplucks itself |

**Fragment size:** 1ms (pure glitch, pitched artifacts) → 200ms (micro-loops, audible repetition)
**Density:** 1 grain/sec (sparse, occasional) → 64 grains/sec (continuous granular texture)

**FRACTURE gradient:**
- 0.0 = Clean — no microsound, pure string
- 0.0–0.3 = Subtle per-voice fragmentation, organic imperfection
- 0.3–0.7 = Obvious glitch, each voice fracturing at its own rate → polyrhythmic texture
- 0.7–1.0 = Global synchronization — all voices lock to the same fragment pattern → cascading shatter

### Crossover Router

Note-based, not frequency-domain. Each voice is routed to LOW or HIGH based on its MIDI note number relative to the crossover point.

- **Split point:** Controlled by SURFACE macro (M3). Default: MIDI note 60 (C4, ~262Hz)
- **Blend zone:** ±6 semitones around the split point. Notes within the zone get a weighted mix of both paths
- **Pitch bend through the zone:** When a note bends through the crossover, its FX character transforms — the note "crosses the surface." This is a performance moment.
- **SURFACE at 0.0:** Split at ~MIDI 36 (C2). Almost everything in the Light path. Only sub-bass enters the Deep.
- **SURFACE at 1.0:** Split at ~MIDI 84 (C6). Almost everything in the Deep path. Only the highest notes escape into the Light.

### FX Path LOW — "The Deep"

| Slot | DSP | Key Params | Character |
|------|-----|-----------|-----------|
| Sub Harmonic | Octave-down pitch shift + LP filter | Amount, filter cutoff | Adds weight beneath plucked notes — Oscar's gravity |
| Tape Saturation | Asymmetric tanh + gentle LP rolloff | Drive, warmth | Warm analog body — the pressure of deep water |
| Dark Delay | Modulated delay line, LP in feedback path | Time (sync), feedback, mod depth | Long, dark echoes — sound sinking into the abyss |
| Deep Plate | Dattorro plate reverb, LP on input | Size, decay, dampen | Cathedral underwater — enormous, enveloping, warm |

### FX Path HIGH — "The Light"

| Slot | DSP | Key Params | Character |
|------|-----|-----------|-----------|
| Pitch Shimmer | Octave-up pitch shift in reverb tail | Amount, decay | Ascending harmonics — feliX's light refracting |
| Micro Delay | Short ping-pong delay, HP in feedback | Time (fast), feedback, spread | Quick rhythmic echoes — surface ripples |
| Spectral Smear | FFT freeze/blur on overtone structure | Smear amount, freeze | Overtones blur into a halo — divine glow |
| Crystal Chorus | Stereo chorus, HP input filter | Rate, depth, spread | High-frequency sparkle — bioluminescent shimmer |

---

## Macro System

| Macro | Name | Controls | Musical Intent |
|-------|------|----------|---------------|
| M1 | **PLUCK** | String material blend + exciter sharpness + damping + pluck position | The character of the goddess's touch — from gentle nylon caress to aggressive steel strike to crystalline shatter to pure divine light |
| M2 | **FRACTURE** | Microsound amount + fragment size + density + per-voice→global sync | How much the goddess tears the sound apart — clean string → subtle organic movement → polyrhythmic glitch → total siphonophore fragmentation |
| M3 | **SURFACE** | Crossover split point + blend zone width + LOW/HIGH FX balance | Raises and lowers the water's surface — controls which notes enter the deep and which stay in the light. THE architectural macro. |
| M4 | **DIVINE** | Sympathetic resonance amount + shimmer + reverb + harmonic bloom + stereo width | The goddess's presence — how much divinity radiates from the instrument. Bare harp → celestial resonance → overwhelming divine shimmer |

---

## Voice Architecture

- **Max voices:** 16 — enough for rich arpeggios and chords (harps are inherently polyphonic)
- **Voice stealing:** Oldest note — a real harp's strings ring until damped
- **Legato mode:** No — harps don't glide. But PLUCK macro at extreme can simulate continuous bowing/bending
- **Glide:** Off by default, available for experimental presets (the goddess bending reality)

---

## Coupling Thesis

XOrphica is THE SURFACE — the convergence point of the entire water column. She can receive from above and below simultaneously, and her split-register architecture means coupling affects the two halves differently.

### As Coupling Source (XOrphica → others)

| Route | What It Sends | Partner | Musical Effect |
|-------|--------------|---------|---------------|
| `getSampleForCoupling()` | Post-crossover recombined output (full harp) | Any | Harp texture modulating other engines |
| Amplitude envelope | Pluck transients | ONSET, OVERDUB | Harp plucks trigger drum hits or dub throws |
| High-path output | Treble shimmer only | OPENSKY, ODYSSEY | Shimmer drives euphoric/psychedelic bloom |
| Low-path output | Bass body only | OCEANDEEP, OVERBITE | Bass plucks reinforce sub weight |

### As Coupling Target (others → XOrphica)

| Route | Source | What It Does | Musical Effect |
|-------|--------|-------------|---------------|
| `AmpToFilter` | ONSET | Drum amplitude → harp filter/damping | Drums play the harp — rhythmic filtering |
| `AudioToWavetable` | OPAL | Granular output → microsound source | Double-layer granular — extreme fragmentation |
| `EnvToMorph` | ODYSSEY | JOURNEY macro → SURFACE position | The journey raises and lowers the water around the harp |
| `LFOToPitch` | OPENSKY | Shimmer LFO → harp pitch | Sky makes the strings waver — divine vibrato |
| `AmpToFilter` | OCEANDEEP | Sub amplitude → sympathetic resonance | Deep bass makes unplucked strings sing |

### Signature Pairings

| Pairing | Name | What Happens |
|---------|------|-------------|
| **ORPHICA → OVERDUB** | "Divine Dub" | Harp through tape delay + spring reverb. The goddess plays through dub architecture. |
| **OPENSKY → ORPHICA** | "Celestial Strings" | Euphoric shimmer from above drives sympathetic resonance — the sky plays the harp |
| **ORPHICA → OCEANDEEP** | "Abyssal Pluck" | Bass strings feed the abyss — gentle plucks become sub-bass earthquakes |
| **ONSET → ORPHICA** | "Rhythm Harp" | Drum transients trigger micro-loops on individual strings — the kit plays the harp |
| **ODYSSEY → ORPHICA** | "Journey Surface" | JOURNEY macro moves SURFACE — as the journey progresses, the water rises and falls around the goddess |
| **ORPHICA × ORPHICA** | "Goddess Duet" | Two instances: one all-Deep, one all-Light — the goddess plays herself across the full column |

### Unsupported Coupling Types

| Type | Why |
|------|-----|
| `AmpToChoke` | The harp should ring — choking kills the instrument's fundamental nature |
| `AudioToRing` | Ring modulation destroys the plucked string identity |

---

## Signature Sound

XOrphica's signature is the split. Play a C2 and a C5 simultaneously — the C2 enters the Deep path (warm tape saturation, dark delay, cathedral plate reverb) while the C5 enters the Light path (shimmer, micro-delay, spectral smear, crystal chorus). The same instrument, playing the same chord, but each note lives in a different world. Add FRACTURE and the notes begin to fragment independently — the bass note stutters at one rate while the treble note scatters at another, creating polyrhythmic glitch texture from a simple two-note chord. Sweep SURFACE from 0 to 1 and the water rises — the C5 crosses the boundary, its shimmer dissolving into warmth as it submerges. This is a sound no other engine can make: polyphonic microsound harp with split-register FX processing.

The "Emily the Harpist" inspiration: a real performer bending, looping, and FX-processing a harp in real time, but the XOrphica goddess has two FX chains — one for her low strings, one for her high strings — and the microsound engine gives her the ability to shatter any string into a thousand luminous fragments.

---

## Visual Identity

- **Accent color:** Siren Seafoam `#7FDBCA` — the color of the surface itself, where sky-light meets water
- **Material/texture:** Translucent membrane — like looking at a jellyfish bell from below, light filtering through organic tissue, iridescent edges
- **Icon concept:** A harp silhouette where the strings are vertical lines of different lengths — short strings at top (light colored), long strings extending down (dark colored). The harp body sits at a horizontal line representing the water's surface. Below the line, the strings blur into tendrils.
- **Panel character:** The engine panel should feel like looking up through water at the surface — light coming from above, shapes softened by refraction. The crossover split should be visually represented as a horizontal water line that moves up and down with the SURFACE macro.

---

## Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| **Foundation** | Medium | Clean harp tones with minimal FRACTURE serve as harmonic beds |
| **Atmosphere** | High | The split-register processing creates vast, layered atmospheric textures |
| **Entangled** | High | The SURFACE macro + coupling creates emergent cross-world behavior |
| **Prism** | High | Crystal and Light string materials + the Light FX path = prismatic shimmer |
| **Flux** | High | FRACTURE-driven microsound creates constantly shifting texture |
| **Aether** | Medium | Freeze mode + Deep path reverb = suspended, barely-there resonance |

---

## Parameter Count Estimate

| Category | Params | Examples |
|----------|--------|---------|
| String Engine | 8 | material, pluck position, damping, brightness, body, sympathetic amount, tune, string count |
| Microsound | 6 | mode, fragment size, density, scatter, freeze trigger, reverse |
| Crossover | 3 | split point (SURFACE), blend width, path balance |
| FX Path LOW | 8 | sub amount, tape drive, delay time, delay feedback, delay mod, reverb size, reverb decay, reverb dampen |
| FX Path HIGH | 8 | shimmer amount, micro-delay time, micro-delay feedback, smear amount, smear freeze, chorus rate, chorus depth, chorus spread |
| Macros | 4 | PLUCK, FRACTURE, SURFACE, DIVINE |
| Master | 3 | volume, pan, glide |
| **Total** | **~40** | Focused — the split architecture provides complexity without parameter bloat |

---

## Emily the Harpist — Design Inspiration

The YouTube harpist who bends, loops, and processes her instrument with pedal effects. Key behaviors to capture in XOrphica:

1. **The bend** — pitch manipulation beyond acoustic limits. The goddess bends a note and it slides through the crossover zone, transforming from shimmer to warmth mid-bend.
2. **The loop** — Freeze mode in the microsound engine captures a fragment and sustains it infinitely. Layer frozen loops at different registers and they accumulate in different FX paths.
3. **The gliss** — running fingers across all strings. An arpeggiator or glissando mode that triggers strings sequentially, each one fragmenting as it enters the microsound engine.
4. **The dual FX** — the defining feature. Bass notes get dark, warm, heavy processing. Treble notes get bright, shimmery, spacious processing. The same gesture produces two sonic worlds.
5. **The build** — starting clean, gradually adding FRACTURE, sweeping SURFACE, turning up DIVINE. The performance arc from simple harp to cosmic divine glitch.

---

*XO_OX Designs | XOrphica — the goddess plays the surface, and every string reaches into a different depth*
