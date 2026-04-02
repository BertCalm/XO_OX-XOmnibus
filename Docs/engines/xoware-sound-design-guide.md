# XOware Sound Design Guide
## "The Resonant Board" — Tuned Percussion Engine

**Accent:** Akan Goldweight `#B5883E`
**Parameter prefix:** `owr_`
**Voices:** 8 polyphonic
**Target score:** 9.8

---

## 1. Engine Identity — The Sunken Oware Board

XOware is named for the oware board — a carved wooden mancala game from the Akan people of Ghana,
one of the most ancient and widely played board games in Africa. In the instrument's mythology, this
particular board never reached its destination. It sank somewhere in the Atlantic on a West African
trade route and now rests on the ocean floor, its cups encrusted with coral and bronze barnacles,
its resonating body still trembling with every current.

Strike a hollow and the whole board shimmers with sympathetic resonance. Seeds of metal, glass, stone,
and wood fall into the cups and the board remembers every impact, every vibration, every player.

The game of oware is about redistribution — seeds flow between cups, fortunes shift, what seemed
empty suddenly fills. XOware does the same thing sonically: energy struck into one note flows into
every other sounding note through the sympathetic resonance network. Nothing is isolated. The board
is one resonating body.

### Cultural Lineage

XOware draws on a specific and documented lineage of tuned percussion traditions:

- **West African balafon and gyil** (Ghana, Burkina Faso) — wooden bar instruments with gourd resonators,
  spider-silk buzz membranes that add the characteristic mirliton buzz. The gyil is the Dagara instrument
  of mourning and ceremony. The Chaigne & Doutaut (1997) contact model underpins the mallet physics.
- **Javanese and Balinese gamelan** — bronze gong-chimes with frame resonators, the defining "beating"
  shimmer of two slightly detuned shadow instruments (the kempur/kemong pairs). Rossing (2000) informs
  the gamelan bar ratios.
- **Western marimba and vibraphone** — tube-resonated wooden and aluminium bar instruments. The
  vibraphone adds a rotating fan that periodically opens and closes the tubes, creating its characteristic
  tremolo shimmer. Fletcher & Rossing (1998) supplies the spectral mode ratios.
- **Tibetan singing bowls** — struck or rimmed bronze bowls with inharmonic partial series and
  extraordinarily long decay. Bowl physics give XOware its most meditative voice.
- **Modal synthesis tradition** (Adrien 1991, Bilbao 2009) — the engine's 8-mode resonator bank
  implements this rigorously.

The parameter gold color (`#B5883E`) references the abrammuo — Akan goldweight scales used to
measure gold dust. Everything on this board has precise weight.

---

## 2. The 7 Pillars Explained for Producers

XOware is built on seven interlocking systems. You do not need to understand the physics to use them —
but knowing what each pillar does gives you conscious control over what is otherwise felt intuitively.

### Pillar 1: Material Continuum

**Parameter:** `owr_material` (0.0 – 1.0)
**What it does:** Continuously morphs the harmonic ratio table that governs which overtones the resonators
produce, and simultaneously controls how fast those overtones decay.

The continuum passes through four archetypal materials:
- **0.0 – 0.33: Wood → Bell** (balafon, marimba, gyil into temple bell)
- **0.33 – 0.66: Bell → Metal** (temple bell into vibraphone, xylophone)
- **0.66 – 1.0: Metal → Bowl** (vibraphone into crystal glass, singing bowl)

The material exponent (alpha) is the hidden mechanism: wood alpha = 2.0 means upper harmonics die
dramatically faster than the fundamental. Metal alpha = 0.3 means all harmonics ring together at
nearly equal rates. This is why wood sounds "warm" (few upper partials survive) and metal sounds
"bright" (all partials persist). At bowl settings, the partial structure becomes bowl-like — tight,
inharmonic, long-ringing.

**Mod wheel** adds up to +0.4 to material in real time, letting you morph from wood toward bell
while holding notes. This is one of the most expressive controls on the engine.

### Pillar 2: Mallet Physics

**Parameters:** `owr_malletHardness` (0.0 – 1.0), velocity sensitivity
**What it does:** Implements the Chaigne contact model — a physically accurate description of
how a mallet compresses against a bar during impact.

Three things change with mallet hardness and velocity:

1. **Contact duration:** Soft mallets (low hardness) stay in contact 5× longer than hard mallets.
   The contact pulse is a half-sine with that duration, shaping the initial transient.
2. **Spectral content:** Soft mallets low-pass filter the excitation — the cutoff runs from
   1.5× the fundamental (very soft) to 20× the fundamental (very hard). Soft mallets physically
   cannot excite high modes. This is not a filter on the output — it is a filter on what goes
   *into* the resonator bank. The result is fundamentally different: soft strikes make the upper
   modes physically absent rather than just quieter.
3. **Mallet bounce:** At low hardness + low velocity, the mallet physically bounces and produces
   a secondary strike 15–25 ms later at 30% amplitude. This is the subtle double-transient you
   hear on real soft-mallet marimba hits.

**Velocity** adds up to +0.5 to the effective hardness. A hard hit on the Balafon preset (hardness 0.38)
reaches effective hardness 0.88. A whisper-touch only hits 0.38. The timbre changes dramatically, not
just the volume — this is Doctrine 1 (D001) fully realized.

**Aftertouch** adds up to +0.3 to hardness after note-on, so you can push into harder material
while sustaining a note. This is the marimba tremolo technique in synthesis form.

### Pillar 3: Sympathetic Resonance Network

**Parameter:** `owr_sympathyAmount` (0.0 – 1.0)
**What it does:** A frequency-selective cross-voice coupling where sounding notes feed energy into
other notes whose modal frequencies are within 50 Hz of them.

This is not amplitude-based coupling (which would just blend voices). It is *spectrum-based*: voice
A's mode 2 at 880 Hz feeds directly into voice B's mode 4 at 876 Hz, because they are close. Voice A's
mode 1 at 220 Hz does nothing to voice B's mode 3 at 660 Hz, because they are 440 Hz apart.

The result is that playing harmonic intervals produces a halo of interacting partial energy that does
not exist when you play the same notes in isolation. Playing a major third on Vibraphone setting creates
a sympathetic shimmer between the shared overtones. Playing cluster chords with high sympathy creates a
dense, mutually reinforcing web of resonance — exactly as a gamelan ensemble sounds when many instruments
ring simultaneously.

### Pillar 4: Resonator Body

**Parameters:** `owr_bodyType` (0–3), `owr_bodyDepth` (0.0 – 1.0)
**What it does:** Models the cavity or frame beneath the bar that boosts and colors specific modes.

Four body types:
- **0 — Tube:** Gourd or tube resonator. Delay-line model tuned to the note frequency. Harmonics of
  the fundamental receive a Gaussian proximity boost. This is the balafon/marimba model.
- **1 — Frame:** Wooden or metal frame with three fixed resonances at 200 Hz, 580 Hz, and 1100 Hz.
  Modes near these frequencies ring longer. This is the gamelan frame model.
- **2 — Bowl:** Sub-octave and fundamental resonance boost. A second resonator running at half the note
  frequency adds the deep sustaining warmth of a Tibetan bowl. This is the singing bowl / temple bell model.
- **3 — Open:** No body resonance. Pure bar physics, nothing beneath. Wind chimes, kalimba tines, raw
  metal percussion.

Body Depth controls how much the body character bleeds into the bar sound — 0.0 is pure bar, 1.0 is
maximum body coloring.

### Pillar 5: Buzz Membrane

**Parameter:** `owr_buzzAmount` (0.0 – 1.0)
**What it does:** The balafon secret. West African tuned percussion instruments traditionally have
spider-silk membranes covering small holes in the gourd resonators. When the bar vibrates, the
membrane buzzes in sympathy, adding a characteristic raspy halo to the sound — the mirliton effect.

The implementation is band-selective: a BPF extracts the membrane resonance band (frequency depends
on body type: gourd/tube = 300 Hz, frame = 150 Hz, metal = 500 Hz), applies tanh soft-clipping only
to that band (the membrane activates above a threshold), then re-injects the buzz artifacts.

This is not distortion on the full signal. It is distortion on a specific band, mimicking the physical
mechanism precisely. The result is authentically buzzy at low amounts and aggressively rattling at high
amounts.

### Pillar 6: Breathing Gamelan (Shimmer)

**Parameter:** `owr_shimmerRate` (0.0 – 12.0 Hz)
**What it does:** Models the Balinese beat-frequency shimmer — the phenomenon where gamelan ensembles
deliberately tune pairs of instruments (like kempur/kemong) to slightly different pitches, producing a
beating shimmer at a specific Hz rate.

This is implemented as a shadow voice detuning: each active voice has its mode frequencies offset by
a slowly modulating amount between 0 and shimmerRate Hz. The shimmer LFO runs at 0.3 Hz, slowly
undulating the shimmer depth. At 6 Hz, you hear the classic Balinese 6-beat-per-second gamelan shimmer.
At 1–2 Hz, it is a slow oceanic oscillation. At 12 Hz, it becomes a metallic tremolo.

The key distinction from ratio-based vibrato: this is an additive Hz offset, not a semitone ratio.
The beat frequency is constant regardless of note pitch — exactly as it is in gamelan tuning practice.

### Pillar 7: Thermal Drift

**Parameter:** `owr_thermalDrift` (0.0 – 1.0)
**What it does:** A shared slow tuning drift that makes the instrument feel alive when nobody is playing.
Acoustic instruments drift in pitch as temperature changes — metal bars expand, gourd resonators
contract in humidity. Thermal Drift replicates this.

Every ~4 seconds, a new target pitch offset is chosen (up to ±8 cents at maximum), and the engine
extremely slowly drifts toward it (0.001% per sample — barely perceptible in real time, but over
seconds the pitch subtly wanders). Each of the 8 voices also has a fixed "personality" offset
(a per-voice seed from ±2 cents) adding individual character.

The result: a held chord slowly breathes in pitch. Notes played minutes apart will be in slightly
different tuning environments. The instrument is never quite static.

---

## 3. Signal Flow

```
MIDI NOTE
    │
    ▼
┌─────────────────────────────────────────────────────────────────┐
│  VOICE ALLOCATOR (8 voices)                                     │
│                                                                 │
│  Per-voice:                                                     │
│    MIDI note → GlideProcessor → base frequency                  │
│    + Thermal Drift (shared ±8 cents + per-voice ±2 cents)       │
│    + Pitch Bend (±bendRange semitones)                          │
│    + Shimmer Offset (0 to shimmerRate Hz additive)              │
│                          │                                      │
│                          ▼                                      │
│              ┌──────────────────────┐                           │
│              │  MALLET EXCITER      │                           │
│              │  Chaigne contact     │                           │
│              │  Half-sine pulse     │                           │
│              │  + noise (hard)      │                           │
│              │  + mallet LP filter  │                           │
│              │  + bounce (soft)     │                           │
│              └──────────┬───────────┘                           │
│                         │                                       │
│                         ▼                                       │
│    ┌────────────────────────────────────────────────────┐       │
│    │   8-MODE RESONATOR BANK                             │       │
│    │   Mode ratios: wood/bell/metal/bowl (interpolated) │       │
│    │   Q: material-dependent base + mode falloff        │       │
│    │   Per-mode amplitude: mallet hardness rolloff      │       │
│    │   Per-mode decay: material exponent alpha          │       │
│    │   + Body-membrane Q boost (Gaussian proximity)     │       │
│    │   + Sympathetic input from other voices            │       │
│    └────────────────────────────┬───────────────────────┘       │
│                                 │                               │
│                                 ▼                               │
│                   ┌─────────────────────────┐                   │
│                   │   BUZZ MEMBRANE          │                   │
│                   │   BPF (body-type freq)   │                   │
│                   │   tanh on band only      │                   │
│                   │   re-inject artifacts    │                   │
│                   └────────────┬────────────┘                   │
│                                │                                │
│                                ▼                                │
│                   ┌─────────────────────────┐                   │
│                   │   BODY RESONATOR         │                   │
│                   │   Tube: delay-line       │                   │
│                   │   Frame: 3 fixed modes   │                   │
│                   │   Bowl: sub-octave res.  │                   │
│                   │   Open: bypass           │                   │
│                   └────────────┬────────────┘                   │
│                                │                                │
│                                ▼                                │
│              AMP ENVELOPE (decay only, material alpha)          │
│                                │                                │
│                                ▼                                │
│              FILTER ENVELOPE → Cytomic SVF (lowpass)            │
│              Brightness + FilterEnvAmt × 4000 Hz               │
│                                │                                │
│                                ▼                                │
│              NOTE-PITCH STEREO PAN (±1.0 at ±36 semitones)      │
└────────────────────────────────┬────────────────────────────────┘
                                 │
              ┌──────────────────┘
              │  (all 8 voices summed)
              ▼
         STEREO OUTPUT
         → Coupling cache (L/R)
```

**Coupling inputs accepted:**
- `AmpToFilter` → filter cutoff mod (×2000 Hz)
- `LFOToPitch` / `AmpToPitch` → pitch offset (semitones)
- `EnvToMorph` → material blend offset

---

## 4. Material Continuum Guide — What Each Zone Sounds Like

### Zone 1: Wood (0.0 – 0.15)
The balafon–marimba territory. The fundamental dominates and upper modes decay two to three times
faster than the root tone. The result is a warm thud that blooms into a gentle sustain — mostly
fundamental, very little harmonic shimmer. The mode ratios follow Rossing's wood bar data: mode 2
at ~4× fundamental (a stretched major 10th), mode 3 at ~9× (a very sharp compound minor 7th), higher
modes at increasingly wide intervals.

Characteristic presets in this zone: Rosewood Marimba (0.05), Balafon (0.04), Kalimba (0.15).

Sound words: warm, woody, thuddy, organic, round, African, short-singing.

### Zone 2: Bell (0.28 – 0.38)
The crossover region where the mode ratios shift toward the bell table — which is unusual because
bell partials are compressed rather than stretched. Mode 2 is at only 1.5× fundamental (a 7th rather
than the wood 4×), mode 3 at 2.0×, mode 4 at 2.5×. This creates a dense, clustered partial structure
with modes sitting close together in pitch — the signature bell overtone cluster. This is why bells
sound "metallic" but not bright: many partials, tightly spaced.

Characteristic presets: Dawn Bells (0.33), Gamelan Bronze (0.33), Coral Chime (0.33), Temple Bell (0.33),
Spirit Board (0.33).

Sound words: bronze, ceremonial, clustered, inharmonic, East Asian, resonant, temple.

### Zone 3: Metal (0.55 – 0.75)
Metal bar ratios mirror wood but all modes persist equally (alpha approaches 0.3). The fundamental
is no longer disproportionately loud — all 8 modes contribute similar amplitude over the full decay.
Combined with high Q, this creates the bright, "singing" quality of vibraphone and glockenspiel.
Mode spacing is similar to wood but all modes stay alive.

Characteristic presets: Vibraphone (0.50), Wind Chimes (0.72), Glockenspiel (0.80), Metal Rain (0.75).

Sound words: bright, cutting, aluminium, steel, clean, pitched, vibraphone.

### Zone 4: Bowl (0.82 – 1.0)
The bowl mode table uses different ratios: mode 2 at 2.71× (a 12th + minor 3rd), mode 3 at 5.33×,
higher modes at progressively more exotic intervals. These are the characteristic inharmonic partials
of struck brass bowls. Combined with the bowl body type (sub-octave resonance), the result is a
spreading cloud of inharmonic singing tones — crystalline, slightly dissonant, very long-ringing.

Characteristic presets: Singing Bowl (0.66), Crystal Glass (0.82), Deep Resonance (0.66).

Sound words: singing, crystalline, glass, inharmonic, meditative, Tibetan, spreading.

### The Crossover Zones

The material parameter interpolates continuously. The transitions are not abrupt:

- **0.15 – 0.28 (Wood → Bell):** The mode table is morphing. Upper modes are still dying fast (wood
  alpha), but the ratio table is shifting. You get warm bell — deeply resonant without being bright.
- **0.38 – 0.55 (Bell → Metal):** Bell ratios giving way to metal ratios while alpha rises. This is
  where the engine sounds most like a hybrid instrument — something that doesn't exist acoustically.
  The Akan Gold preset (0.45) lives here deliberately: it is the engine's own characteristic sound,
  not a copy of any physical instrument.
- **0.75 – 0.82 (Metal → Bowl):** The sharpest transition. Metal ratios are wide-spaced; bowl ratios
  are much narrower. Passing through this zone creates a shift in the partial cluster that sounds
  almost like a different pitch.

---

## 5. Mallet Physics — How Velocity Changes the Sound and Why

At the physics level, a mallet striking a bar undergoes Hertz contact: the compression of the mallet
felt determines how long contact lasts and which vibrational modes of the bar are excited.

**Hard mallet, high velocity** (hardness 0.8+, velocity 0.8+):
- Contact time: ~0.5 ms (very brief)
- Excitation filter: opens to 20× fundamental (~8800 Hz at A4)
- All 8 modes receive near-equal initial energy
- No bounce
- Noise is dominant in the excitation signal (high noiseMix)
- Sound: bright, cutting, percussive, immediate attack

**Soft mallet, low velocity** (hardness 0.2, velocity 0.2):
- Contact time: ~4.75 ms (very long — almost a bowing gesture)
- Excitation filter: closes to 1.5× fundamental (~330 Hz at A4)
- Only modes 1–2 receive significant energy; modes 3–8 are physically absent
- Bounce active: secondary hit at ~20 ms, 30% amplitude
- Noise minimal (pulse-dominated)
- Sound: round, dark, slow-blooming, almost bowed quality with subtle double-transient

**The bounce is the subtlety**: At piano.ppp dynamics with low hardness, you hear the gentle
double-transient of the mallet rebounding. It gives that particular "soft marimba" quality that
sampled instruments struggle to replicate because each sample is a fixed take.

### Velocity Programming Tips

1. **Velocity curves matter more on OWARE than most engines.** A linear velocity curve destroys the
   spectrum behavior — you get mostly amplitude change. Use an exponential or S-curve to get genuine
   timbre shifts at the extremes.
2. **Program velocity layers as material layers.** ppp = soft felt, ff = hard rubber. The material
   parameter changes the harmonic structure; velocity changes which harmonics are excited.
3. **The filter envelope amount (`owr_filterEnvAmount`) is velocity-adjacent.** At 0.5+, the filter
   opens dramatically on attack and falls — adding an additional brightness transient on top of the
   mallet hardness. Reduce this on presets where the mallet physics alone is doing the work.
4. **Aftertouch for live expression:** Set hardness to 0.25, play softly, then press down on aftertouch
   while holding. The effective hardness rises to 0.55, the resonators start receiving higher-mode energy,
   and the tone brightens in real time. This is the continuous-expression behavior that separates OWARE
   from sampled percussion.

---

## 6. Sympathetic Resonance — How to Use It, Sweet Spots

### How It Works

When voice A is sounding and voice B is triggered, the engine checks every pair of modes across the
two voices. If any mode in voice A is within 50 Hz of any mode in voice B, that mode in voice B
receives an additional input equal to voice A's mode output × proximity factor × sympathyAmount × 0.03.

The 50 Hz window means the coupling is strongest in the low-to-mid register where modes are close
together, and diminishes in the high register where mode spacing is wide.

### Sweet Spots

**Unison and octave (maximum coupling):** Same note across octaves shares several mode frequencies
exactly. At sympathy 0.6+, playing C4 and C5 together creates a circular energy loop — each voice
feeds the other's matching modes. The result is a sustain that outlasts either note played alone.

**Intervals within the material's mode table:** At wood setting (0.0), mode 2 is at ~4× fundamental.
This means a note and its 2-octave-plus-major-third interval will have coupling between their
fundamentals and second modes. At bell setting (0.33), mode 2 is at 1.5× — so a note and its major
6th above will sympathize strongly. The intervals that "work" with sympathetic resonance change as
you move through the material continuum.

**Dense chords at high sympathy (0.7–1.0):** The Spirit Board preset (sympathy 1.0) uses this to
create a bell resonance web — every voice feeds every other in a complex network. Playing slow,
arpeggiated chords lets each new note excite the existing network. The board "remembers" what was
played 8 seconds ago.

**Sympathy 0.1–0.3 for subtle halo:** Low sympathy adds the natural acoustic effect without
becoming overwhelming. The Rosewood Marimba (0.15) uses this to add the subtle resonance of bar
instruments in a real room — a softening of the hard mallet attack without obvious electronic shimmer.

### Sympathy Too High

Above 0.7 with many voices, the engine can begin to self-sustain. Notes that would normally decay
in 2 seconds may ring for 10+ seconds as each voice feeds the others. This is sonically spectacular
in long-decay, slow-moving contexts (ambient, cinematic). It is disruptive in rhythmic contexts
where decay time and groove feel are linked.

For rhythmic playing: keep sympathy at 0.15–0.35 with short decay times.
For ambient/cinematic: push sympathy to 0.6–1.0 and let the network develop.

---

## 7. Buzz Membrane — The Balafon Secret, How to Dial It In

### Physical Background

The balafon and the gyil (Dagara xylophone of Ghana) have a distinctive buzzing quality that sampled
versions often miss or exaggerate. The source is the mirliton — a piece of spider silk, paper, or
thin membrane stretched over a small hole near the base of each gourd resonator. When the bar
vibrates, the air pressure in the gourd causes the membrane to flutter, adding a band-specific buzz.

The membrane responds primarily to frequencies that match its resonance (typically in the 200–500 Hz
range for gourd-sized resonators). It does not buzz at all frequencies — which is why the effect is
subtle on high notes and more pronounced on low notes.

### Implementation

The Buzz Membrane extracts a band via BPF centered on:
- **Tube/Gourd body (type 0):** 300 Hz
- **Frame body (type 1):** 150 Hz
- **Metal body (type 2):** 500 Hz

The extracted band receives tanh soft-clipping (sensitivity = 5 + amount × 15). The clipped band
is re-injected into the signal at amount strength. At low amounts (0.05–0.15), the buzz is a whisper —
the membrane is just barely activated. At 0.4–0.5, it is clearly audible. At 0.7+, the buzzing becomes
an aggressive rattle that dominates the low-mid texture.

### Dialing It In

**Authentic balafon tone (tube body):**
- Start with tube body (type 0), any wood material
- Bring buzz to 0.3–0.45
- Play low notes (C3–C4): the buzz is most prominent where the 300 Hz band overlaps with bar energy
- High notes (C5+): buzz nearly inaudible because the bar's fundamental is above the membrane band
- This asymmetry is correct and authentic — real balafon buzz is register-dependent

**Frame body buzz:**
- Frame body drops the membrane frequency to 150 Hz
- This creates a lower, heavier buzz — more like a kora or talking drum buzz than a balafon
- Best at 0.2–0.35 with bell material (0.28–0.38)

**Metal body buzz:**
- 500 Hz band — higher, more metallic rattle
- Sounds like a snare strainer rather than a silk membrane
- Use at 0.1–0.25 for subtle metallic texture on metal-material presets

**Amounts to avoid:**
- Above 0.65 on high-sympathy patches: the buzz band becomes part of the sympathetic coupling loop
  and can accumulate into a harsh resonance peak
- Zero buzz on any bell/bowl material preset unless you specifically want "clean": the slight mirliton
  halo at 0.05–0.15 adds warmth to bell material that pure modal synthesis lacks

---

## 8. Thermal Drift — When to Use It, How Much Is Too Much

### The Life Mechanism

Thermal Drift is the parameter that makes OWARE feel like an acoustic instrument in a room rather
than a digital recreation. Acoustic instruments drift in pitch constantly: wood expands and contracts
with humidity, metal bars shift slightly with temperature, instrument bodies flex. A real balafon
tuned in Ghana in January will be slightly differently tuned in July.

The engine implements this with two layers:
1. **Shared drift:** Every ~4 seconds, a new pitch target (up to ±8 cents at maximum drift = 1.0)
   is chosen. The engine drifts toward it at an extremely slow rate (barely audible in real time, but
   the cumulative drift over a composition is substantial).
2. **Per-voice personality:** Each of the 8 voices has a fixed seed offset (±2 cents × drift amount).
   Voice 1 might always sit 1.3 cents sharp; voice 7 might sit 0.8 cents flat. This adds the natural
   micro-intonation of individual bars on a real instrument.

### When to Use It

**Ambient and drone contexts:** Thermal drift transforms what would be a static tuning into a gently
breathing environment. The chord you hold for 30 seconds will be subtly different at the end than it
was at the beginning. Use 0.3–0.7 freely.

**World music and acoustic emulation:** Any preset attempting to sound like a real instrument should
have at least 0.05–0.15 of drift. No real instrument has exactly fixed pitch. Zero drift makes the
instrument sound like a keyboard sample rather than a physical object.

**Meditative / healing music:** High drift (0.55–0.85) on singing bowl patches creates the sensation
of the bowl's pitch shifting with room temperature. Combined with long decay, the notes seem to live
and breathe. The Singing Bowl preset (0.55) and Deep Resonance (0.72) use this deliberately.

**Electronic / rhythmic contexts:** Keep drift at 0.0–0.10. Above that, the pitch instability will
fight with quantized rhythm and other precisely-tuned instruments. The Metal Rain preset (0.05) and
Glockenspiel (0.02) are nearly drift-free.

### How Much Is Too Much

At drift 0.85+, the pitch can wander ±6–7 cents over a 4-second cycle. In a musical context:
- Against equal-tempered accompaniment: audibly "out of tune" but in a character-rich way
- In solo/ambient context: atmospheric, humanizing, interesting
- In just-intonation context: can enhance or destroy depending on the interval
- Against percussion-only: generally harmless

The Drift Wood preset (0.85) is designed to push this limit — it literally sounds like an old wood
instrument with expanding and contracting bars. It is a character choice, not an error.

**The 0.3 sweet spot:** Drift 0.3 is the "room atmosphere" setting — subtle enough that most
listeners would not identify it as detuning, but enough that the instrument feels like a physical
object rather than a digital one. Most OWARE presets use 0.15–0.45.

---

## 9. Ten Sound Design Recipes

Each recipe gives a starting point with exact parameter values. Start from the Init patch and dial
in each parameter sequentially.

---

### Recipe 1: Bone-Dry Gyil
*West African ceremonial xylophone, no effects needed*

```
owr_material         0.04    (pure wood)
owr_malletHardness   0.45    (medium hard rubber)
owr_bodyType         0       (tube/gourd)
owr_bodyDepth        0.78    (deep resonator coupling)
owr_buzzAmount       0.38    (clear mirliton buzz)
owr_sympathyAmount   0.22    (subtle neighbor resonance)
owr_shimmerRate      0.0     (no gamelan shimmer — this is West African)
owr_thermalDrift     0.12    (gentle atmospheric tuning)
owr_brightness       2600    (Hz, medium-warm brightness)
owr_damping          0.62    (typical wooden damping)
owr_decay            1.0     (fast — gyil is short-decay)
owr_filterEnvAmount  0.42    (attack brightness opens the wood)
```

Play in the C3–C5 range. The buzz is most active on low notes. Try playing pentatonic phrases — the
buzz membrane and gourd resonance will create the characteristic West African tone without a single
reverb unit.

---

### Recipe 2: Vibraphone Jazz
*Milt Jackson sweetness — slow shimmer, tube resonators, loose sympathy*

```
owr_material         0.50    (metal bar)
owr_malletHardness   0.45    (medium felt/yarn)
owr_bodyType         0       (tube resonators)
owr_bodyDepth        0.50    (present but not dominant)
owr_buzzAmount       0.0     (no buzz on vibe)
owr_sympathyAmount   0.25    (gentle sympathetic halo)
owr_shimmerRate      3.5     (3.5 Hz — slow vibe fan rotation)
owr_thermalDrift     0.06    (nearly static)
owr_brightness       7000    (bright aluminium)
owr_damping          0.30    (medium sustain)
owr_decay            3.2     (long vibraphone decay)
owr_filterEnvAmount  0.30    (gentle brightness envelope)
```

The 3.5 Hz shimmer mimics a slow vibe motor. For fast-motor sound, push to 6–7 Hz. Pedal (hold notes
with sympathy) to get the characteristic vibraphone "haze" on chord voicings.

---

### Recipe 3: Balinese Gamelan Gong
*Dense sympathetic network, bell partials, maximum shimmer*

```
owr_material         0.33    (bell material)
owr_malletHardness   0.50    (brass mallet)
owr_bodyType         1       (frame resonator)
owr_bodyDepth        0.60    (prominent frame resonance)
owr_buzzAmount       0.05    (trace bronze corrosion sound)
owr_sympathyAmount   0.72    (strong sympathetic coupling)
owr_shimmerRate      6.2     (6.2 Hz — classic Balinese beating)
owr_thermalDrift     0.15    (slight tuning warmth)
owr_brightness       6000    (medium-bright bronze)
owr_damping          0.22    (long bronze sustain)
owr_decay            4.5     (4-second gamelan ring)
owr_filterEnvAmount  0.30
```

Play in clusters and arpeggios. Let notes ring and stack — the sympathetic network will create a
sustained shimmering texture. The 6.2 Hz shimmer is not applied to all notes uniformly; each voice
has its own LFO phase, so the shimmer beats rotate around the stereo field.

---

### Recipe 4: Tibetan Singing Bowl
*Ultra-long decay, bowl body, meditative drift*

```
owr_material         0.66    (metal-bowl crossover)
owr_malletHardness   0.35    (soft wooden wand)
owr_bodyType         2       (bowl resonator)
owr_bodyDepth        0.85    (deep sub-octave resonance)
owr_buzzAmount       0.0     (no buzz on singing bowls)
owr_sympathyAmount   0.45    (inter-bowl coupling)
owr_shimmerRate      2.0     (slow oceanic beating)
owr_thermalDrift     0.55    (significant temperature drift)
owr_brightness       4200    (warm, not harsh)
owr_damping          0.10    (minimal damping)
owr_decay            8.5     (8-second decay)
owr_filterEnvAmount  0.20    (gentle brightness opening)
```

Strike and wait. The bowl takes several seconds to fully develop its partials. The 8.5-second decay
means notes overlap enormously. Best with single notes or slow two-note melodies. The thermal drift
at 0.55 will cause a held note to slowly wander — this is the bowl cooling and contracting.

---

### Recipe 5: Mbira / Thumb Piano
*Short intimate tines, light wood, open body, gentle buzz*

```
owr_material         0.15    (light wood — tine character)
owr_malletHardness   0.30    (light thumb pressure)
owr_bodyType         3       (open — no resonator, just tines)
owr_bodyDepth        0.40    (minor open-body coloring)
owr_buzzAmount       0.05    (tiny buzz for authenticity)
owr_sympathyAmount   0.10    (minimal sympathetic)
owr_shimmerRate      0.0     (no shimmer — mbira is dry)
owr_thermalDrift     0.05    (nearly static)
owr_brightness       4500    (mid-bright tine)
owr_damping          0.70    (fast natural decay)
owr_decay            0.55    (short pluck)
owr_filterEnvAmount  0.45    (attack pop)
```

Play in the upper register (C4–C6) for thumb piano character. Low notes take on a xylophone quality.
Use the buzz at 0.05–0.10 for authentic kalimba buzz; above 0.15 starts to sound like a snare
rattling rather than a tine instrument.

---

### Recipe 6: Cinematic Bell Web
*Maximum resonance field, long decay, slow composition tool*

```
owr_material         0.33    (bell partials)
owr_malletHardness   0.50    (medium brass)
owr_bodyType         1       (frame body)
owr_bodyDepth        0.80    (deep frame coupling)
owr_buzzAmount       0.10    (subtle bronze corrosion)
owr_sympathyAmount   1.00    (maximum bell coupling)
owr_shimmerRate      12.0    (maximum shimmer — metallic tremolo)
owr_thermalDrift     0.35    (significant living drift)
owr_brightness       8000    (open and bright)
owr_damping          0.10    (very long sustain)
owr_decay            6.0     (6-second fundamental)
owr_filterEnvAmount  0.30
```

This is the Spirit Board setting. Play three or four notes slowly, letting each new note interact with
everything sounding. The maximum sympathy creates feedback loops between voices that make the sustain
extend beyond the programmed decay. The 12 Hz shimmer at this level becomes a shimmering metallic
roar on loud chords. Use sparingly — it dominates any mix.

---

### Recipe 7: Staccato Metal Toys
*Short decay, high material, maximum mallet, rhythmic playing*

```
owr_material         0.75    (solid metal)
owr_malletHardness   0.85    (hard brass)
owr_bodyType         3       (open — dry contact)
owr_bodyDepth        0.25    (minimal body)
owr_buzzAmount       0.0     (clean metal)
owr_sympathyAmount   0.08    (nearly isolated voices)
owr_shimmerRate      0.0     (no shimmer — tight rhythmic use)
owr_thermalDrift     0.05    (stable)
owr_brightness       11000   (cutting metal)
owr_damping          0.80    (very fast natural decay)
owr_decay            0.18    (180 ms — staccato)
owr_filterEnvAmount  0.55    (snappy brightness transient)
```

Use as a rhythmic melodic percussion instrument. Works well in patterns where the short decay creates
space. The high brightness and hard mallet deliver the "ping" quality of struck metal toys or a very
bright metallophone. At high velocity, the noise component in the excitation creates a brief metallic
scratch before the pitch settles.

---

### Recipe 8: Drifting Ceremonial Wood
*Maximum thermal drift, slow evolution, ambient texture*

```
owr_material         0.08    (deep wood)
owr_malletHardness   0.35    (medium wood mallet)
owr_bodyType         0       (gourd resonator)
owr_bodyDepth        0.60    (present resonance)
owr_buzzAmount       0.10    (subtle membrane flutter)
owr_sympathyAmount   0.30    (gentle coupling)
owr_shimmerRate      1.5     (slow drift shimmer)
owr_thermalDrift     0.85    (maximum organic drift)
owr_brightness       3500    (warm, not bright)
owr_damping          0.45    (medium sustain)
owr_decay            1.8     (medium decay)
owr_filterEnvAmount  0.38    (moderate attack transient)
```

The Drift Wood setting. This is a deliberately unstable instrument. Melodies will be slightly out of
tune with themselves as the thermal drift accumulates. Useful for scoring scenes where disorientation
or environmental unpredictability is the emotional target. In ambient contexts, the drift creates the
sensation that the instrument is alive and slowly moving.

---

### Recipe 9: Crystal Glass Harmonics
*Bowl material at maximum brightness, ultra-long decay, minimalist*

```
owr_material         0.82    (near-bowl, glass-like ratios)
owr_malletHardness   0.40    (medium rubber — glass wand)
owr_bodyType         2       (bowl sub-octave resonance)
owr_bodyDepth        0.55    (moderate bowl depth)
owr_buzzAmount       0.0     (clean glass)
owr_sympathyAmount   0.35    (selective coupling)
owr_shimmerRate      1.2     (very slow shimmer)
owr_thermalDrift     0.08    (nearly static — glass is stable)
owr_brightness       16000   (maximum glass brightness)
owr_damping          0.08    (barely any damping)
owr_decay            7.0     (7-second decay)
owr_filterEnvAmount  0.18    (subtle)
```

The brightness of 16000 Hz combined with bowl material creates a very specific crystalline quality —
the upper modes of the bowl table are at unusual intervals (2.71×, 5.33×) and with high Q they sing
as individual audible partials at these non-integer ratios. It sounds wrong in the best possible way.

---

### Recipe 10: Akan Bronze Showcase
*All pillars in balance — the engine in full voice*

```
owr_material         0.45    (bell-metal transition)
owr_malletHardness   0.50    (medium all-purpose)
owr_bodyType         1       (frame body)
owr_bodyDepth        0.60    (balanced frame coupling)
owr_buzzAmount       0.25    (present but not dominant buzz)
owr_sympathyAmount   0.50    (active but controlled coupling)
owr_shimmerRate      5.0     (active Balinese shimmer)
owr_thermalDrift     0.30    (room-temperature living pitch)
owr_brightness       6500    (warm-bright balance)
owr_damping          0.28    (medium-long sustain)
owr_decay            3.0     (3-second fundamental)
owr_filterEnvAmount  0.35    (responsive attack transient)
```

This is the Akan Gold setting — designed to showcase all 7 pillars simultaneously at moderate depth.
The material is in the bell-metal crossover where mode ratios are shifting, the sympathy network is
active but not runaway, the shimmer is the classic 5 Hz Balinese beating, and the buzz adds textural
warmth without dominating. Use this as a reference starting point for building new patches.

---

## 10. Coupling Recommendations

XOware accepts four coupling types from other engines. Here are the most productive pairings.

### OWARE + OSTINATO (EnvToMorph)

OSTINATO produces rhythmic ostinato patterns with a strong, structured envelope. Routing OSTINATO's
envelope output as `EnvToMorph` into OWARE causes the material to morph in sync with the rhythmic
pattern — each OSTINATO hit briefly pushes OWARE's material toward metal or bowl, then it snaps back.

**Setup:**
- OSTINATO engine active in a pattern slot, producing 1/8 or 1/4 note pulses
- Coupling: OSTINATO → OWARE, type `EnvToMorph`, amount 0.3–0.5
- OWARE material at 0.15–0.25 (start in wood zone)
- EnvToMorph shifts material by ±amount each beat

**Effect:** Each rhythmic pulse briefly brightens the OWARE tone as material pushes toward bell/metal.
The drum rhythm is heard not just as volume but as timbral shifts in the melodic percussion. Works
well in Afrobeat, West African fusion, cinematic underscore.

**Alternative:** Set `EnvToMorph` negative (if interface supports it) to pull OWARE toward wood on
every OSTINATO hit — making each pulse darker rather than brighter. Creates an unusual suppression
effect rather than the typical accent-on-attack.

### OWARE + ONSET (AmpToFilter)

ONSET is a tuned percussion drum engine. Routing ONSET's amplitude as `AmpToFilter` into OWARE
drives OWARE's filter cutoff in sync with drum hits.

**Setup:**
- ONSET in its own engine slot with short, punchy hits
- Coupling: ONSET → OWARE, type `AmpToFilter`, amount 0.4–0.6
- OWARE brightness at 2000–3500 Hz (dark starting point)
- AmpToFilter adds up to ±2000 Hz per unit of amount

**Effect:** Every drum hit briefly opens OWARE's filter, creating a ducking/opening effect where the
melodic percussion brightens on each drum accent. Particularly effective for high-life or jùjú patterns
where the texture layers tightly.

### OWARE + ONSET (LFOToPitch)

ONSET has a rich LFO system. Routing an ONSET LFO as `LFOToPitch` into OWARE adds an external pitch
modulation to all OWARE notes — creating a pitch drift or vibrato driven by the drum engine's LFO.

**Setup:**
- ONSET LFO2 set to a very slow rate (0.05–0.2 Hz), sine shape
- Coupling: ONSET LFO → OWARE, type `LFOToPitch`, amount 0.1–0.2
- This adds ±0.2 semitones of pitch modulation at the given LFO rate

**Effect:** The melodic percussion slowly wavers in pitch across the composition — as though the
entire instrument is heated and cooled by the drums. Adds a ritual, environmental quality.

### OWARE + OXBOW (AmpToFilter)

OXBOW is a reverb-synthesis engine with slow-building resonant fields. Routing OXBOW's amplitude
as filter modulation gives OWARE a reverberant brightness — notes played in OXBOW's resonant space
cause OWARE to open.

**Effect:** Creates the impression of a percussion instrument in a large resonant space, where the
ambience feeds back into the direct sound's brightness.

### Self-Coupling Trick (Coupling Macro)

The `owr_macroCoupling` parameter (CHARACTER macro mapped) adds +0.4 to sympathy. At maximum, this
pushes sympathy from any starting point toward maximum. Assign this to a macro knob and sweep it
during performance to gradually activate the full sympathetic resonance network in real time — from
a dry individual instrument sound to a full resonance web.

---

## 11. Genre Applications

### World Music — West African

**Target:** Authentic balafon, gyil, and marimba tones for jùjú, Afrobeat, highlife, and ceremony.

- Material 0.0–0.15 (wood only)
- Body type 0 (tube/gourd), depth 0.6–0.8
- Buzz 0.2–0.45 (essential for authenticity)
- Sympathy 0.15–0.3 (subtle — real balafons don't have electronic sympathy, but acoustic room does this naturally)
- Shimmer 0.0 (Balinese shimmer is wrong for West African context)
- Drift 0.08–0.2 (subtle human-instrument tuning)
- Decay 0.6–1.5 (typical for these instruments)

Presets: Balafon, Rosewood Marimba, Buzz Ritual (for ceremonial intensity), Kalimba.

### World Music — Gamelan and Southeast Asian

**Target:** Javanese and Balinese bronze instruments, Thai gongs, Khmer roneat.

- Material 0.28–0.50 (bell through metal-bell)
- Body type 1 (frame) or 2 (bowl)
- Buzz 0.0–0.08 (trace only — gamelan instruments have minimal buzz)
- Sympathy 0.5–0.8 (gamelan ensemble is highly sympathetically active)
- Shimmer 5.0–7.0 Hz (defining characteristic of Balinese gamelan)
- Drift 0.10–0.20 (bronze instruments in a warm climate)
- Decay 3.0–8.0 (long bronze sustain)

Presets: Gamelan Bronze, Coral Chime, Mbira Constellation, Dawn Bells.

### Ambient and Drone

**Target:** Slowly evolving textural sustain, meditative, non-rhythmic.

- High sympathy (0.5–1.0) — the network is the texture
- Long decay (5.0–10.0)
- High drift (0.4–0.8)
- Low shimmer (0.5–2.5 Hz) or zero — slow breathing rather than rapid oscillation
- Material anywhere in bowl zone (0.65–1.0) or bell zone (0.28–0.40)
- Body type 2 (bowl) or 1 (frame)
- Play sparsely — let the sympathetic network do the work

Presets: Singing Bowl, Deep Resonance, Spirit Board (maximal version).

### Cinematic Scoring

**Target:** Emotional underscore, tension, ceremony, non-Western scenes.

**For ceremony and wonder:** Bell material (0.28–0.40), frame body, sympathy 0.5+, shimmer 3–6 Hz,
long decay. Add sparse single notes and let the resonance network fill the space.

**For tension and unease:** High drift (0.6+), moderate sympathy, buzz membrane at 0.3–0.5 (creates
an unsettling rattling quality), decay medium-short (1.0–2.0). The buzz membrane at high amounts
creates a tense, cage-like rattling.

**For emotional resolution:** Wood material (0.0–0.15), gourd body, medium sympathy (0.3–0.5),
zero shimmer, drift 0.15. Clean and warm, evoking physical instruments and human presence.

Presets: Mbira Constellation (showcase), Sunken Bronze (mysterious depth), Dawn Bells (gentle ceremony).

### Electronic and Experimental

**Target:** Hybrid sounds that don't map to any real instrument, timbral extremes.

- Use the Akan Gold zone (0.40–0.50): this material setting doesn't correspond to any real instrument
- Maximum shimmer (10–12 Hz) creates metallic tremolo rather than gentle gamelan beating
- Maximum buzz (0.6–0.9) on metal body type creates aggressive digital-organic rattling
- Pitch bend range at 12–24 semitones — OWARE's modal synthesis responds to pitch bend very
  differently from synthesis-based engines (every mode shifts, not just the fundamental oscillator)
- Coupling into other engines: the Sympathetic Resonance output from OWARE can be used as a modulation
  source to trigger filter changes in analytical/electronic engines

Experiment with material 0.75–0.82: this is the metal-to-bowl crossover where mode spacing changes
abruptly. Chords played in this zone have an unusual "collapsing" quality as mode clusters shift.

---

## Quick Reference: Parameter Map

| Parameter | ID | Range | Default | Role |
|---|---|---|---|---|
| Material | `owr_material` | 0–1 | 0.20 | Wood→Bell→Metal→Bowl spectrum |
| Mallet Hardness | `owr_malletHardness` | 0–1 | 0.30 | Contact time, spectral content |
| Body Type | `owr_bodyType` | 0–3 | 0 | 0=Tube, 1=Frame, 2=Bowl, 3=Open |
| Body Depth | `owr_bodyDepth` | 0–1 | 0.50 | Body resonance coupling strength |
| Buzz Amount | `owr_buzzAmount` | 0–1 | 0.00 | Mirliton membrane intensity |
| Sympathy | `owr_sympathyAmount` | 0–1 | 0.30 | Cross-voice spectral coupling |
| Shimmer Rate | `owr_shimmerRate` | 0–12 Hz | 6.0 | Balinese beat-frequency shimmer |
| Thermal Drift | `owr_thermalDrift` | 0–1 | 0.30 | Slow organic pitch wandering |
| Brightness | `owr_brightness` | 200–20000 Hz | 8000 | Output filter cutoff |
| Damping | `owr_damping` | 0–1 | 0.30 | Decay time scaling |
| Decay | `owr_decay` | 0.05–10 s | 2.0 | Fundamental decay time |
| Filter Env | `owr_filterEnvAmount` | 0–1 | 0.30 | Attack brightness transient |
| Bend Range | `owr_bendRange` | 1–24 st | 2 | Pitch wheel range |

**Mod wheel:** Adds +0.4 to material (wood → bell)
**Aftertouch:** Adds +0.3 to mallet hardness + +3000 Hz to brightness

---

## Factory Preset Index

| Preset | Mood | Material | Body | Key Feature |
|---|---|---|---|---|
| Rosewood Marimba | Foundation | 0.05 | Tube | Pure wood, warm soft mallet |
| Balafon | Foundation | 0.04 | Tube | Buzz 0.40, authentic West African |
| Kalimba | Foundation | 0.15 | Open | Intimate pluck, short decay |
| Temple Bell | Foundation | 0.33 | Bowl | Inharmonic bell, 6s decay |
| Vibraphone | Foundation | 0.50 | Tube | 3.5 Hz shimmer, jazz sustain |
| Wind Chimes | Foundation | 0.72 | Open | Bright metal, 7.5 Hz shimmer |
| Dawn Bells | Atmosphere | 0.33 | Tube | 4 Hz shimmer, gentle morning |
| Coral Chime | Atmosphere | 0.33 | Frame | Buzz 0.18, coral character |
| Deep Resonance | Atmosphere | 0.66 | Bowl | Dark, 7.5s, thermal 0.72 |
| Mbira Constellation | Atmosphere | 0.35 | Frame | Showcase all pillars |
| Akan Gold | Entangled | 0.45 | Frame | Full-engine showcase |
| Sunken Bronze | Entangled | 0.55 | Bowl | Ocean floor mystery |
| Spirit Board | Entangled | 0.33 | Frame | Max sympathy + shimmer |
| Crystal Glass | Prism | 0.82 | Bowl | 16kHz brightness, 7s decay |
| Singing Bowl | Prism | 0.66 | Bowl | Thermal 0.55, meditative |
| Gamelan Bronze | Prism | 0.33 | Frame | 6.2 Hz shimmer, dense |
| Glockenspiel | Prism | 0.80 | Open | Hard mallet, precision |
| Metal Rain | Flux | 0.75 | Open | Staccato, 0.18s decay |
| Buzz Ritual | Flux | 0.06 | Tube | Buzz 0.70, ceremonial |
| Drift Wood | Flux | 0.08 | Tube | Thermal 0.85, drifting |
