# XOverworld — Concept Brief

## Identity
- **Name:** XOverworld
- **Thesis:** "XOverworld is a chip synthesis engine that recreates the sound architectures of the NES, Genesis, and SNES — not by emulating the output, but by rebuilding how the chips actually worked — including what happened when they broke."
- **Sound family:** Hybrid melodic + percussion (any voice can be either — just like the original hardware)
- **Unique capability:** Three-era chip synthesis with authentic architectural constraints AND authentic hardware glitch modeling. Morph between NES pulse, Genesis FM, and SNES sample playback on a single voice. Dial in hardware failures from "vintage warmth" to "kill screen chaos." The constraints AND the failures ARE the character.

## Character
- **Personality in 3 words:** Nostalgic, glitchy, playful
- **Engine approach:** Multi-mode chip synthesis (pulse/triangle/noise + 4-op FM + BRR sample playback)
- **Why this engine:** Each console generation used a fundamentally different synthesis method. Recreating the *architecture* (not just the sound) means the constraints that made those soundtracks iconic become creative tools — 4 duty cycles, 8 FM algorithms, Gaussian interpolation, channel-count limits.

## Sound Chip Architecture

### Layer 1: 8-BIT (NES / Ricoh 2A03)
- **Pulse oscillator:** 4 selectable duty cycles (12.5%, 25%, 50%, 75%) — each has a distinct character
- **Triangle oscillator:** Fixed-volume, 4-bit stepped waveform — the NES bass sound
- **Noise channel:** Linear-feedback shift register, short-loop (metallic) or long-loop (hiss) modes
- **DPCM:** 1-bit delta-encoded sample playback (the crunch)
- **Character:** Raw, buzzy, limited, iconic. The sound of constraint.
- **Constraints that matter:** No volume envelope on triangle. Noise pitch is non-linear (specific frequency table). DPCM has only 16 rates.

### Layer 2: FM (Sega Genesis / Yamaha YM2612)
- **4-operator FM:** Sine carriers + modulators with individual envelopes, key scaling, detune
- **8 algorithms:** Different operator routing topologies (serial, parallel, mixed)
- **PSG companion:** 3 additional square waves + noise (from SN76489)
- **DAC mode:** One FM channel can play 8-bit PCM samples (the Genesis "voice" trick)
- **Character:** Metallic, brassy, punchy, aggressive. Electric piano, slap bass, the Sonic snare.
- **Constraints that matter:** Only sine oscillators — all timbre comes from modulation. 6 FM channels total. Discrete key-on/off events.

### Layer 3: SAMPLE (SNES / Sony SPC700 + S-DSP)
- **BRR sample playback:** 4-bit ADPCM compression with characteristic warm/muffled quality
- **Gaussian interpolation:** The SNES's signature slightly-smeared, soft sample playback
- **Hardware ADSR:** Attack/Decay/Sustain/Release with specific rate tables
- **8-tap FIR echo:** The iconic SNES reverb — a simple delay with FIR filtering in the feedback loop
- **Pitch modulation:** One channel's output modulates the next channel's pitch
- **Noise mode:** Per-channel noise replacement
- **Character:** Warm, cinematic, orchestral but compressed. Chrono Trigger, Final Fantasy VI, DKC.
- **Constraints that matter:** BRR compression artifacts. Gaussian interpolation rolls off highs. Echo uses shared RAM (historically limited sample memory). Only 8 voices total.

### The Era Blend Axis
- Continuous morph between any two layers on a single voice
- At the extremes: pure NES, pure Genesis, pure SNES
- In between: hybrid timbres impossible on any single console
- Example: NES pulse duty cycling with Genesis FM modulation depth and SNES Gaussian smoothing

## Glitch Engine — Hardware Failure as Instrument

Game glitches weren't random — they were specific hardware failures with specific sonic signatures. XOverworld models these failures as controllable, repeatable synthesis techniques. A **GLITCH** knob (0–1) controls intensity: at 0, the chip is pristine. At 1, the cartridge is barely making contact.

### Glitch Types (Per-Era, Historically Accurate)

#### 8-BIT Glitches (NES)

| Glitch | Hardware Origin | DSP Implementation | Sound |
|--------|---------------|-------------------|-------|
| **Missingno** | Reading garbage memory as tile/sprite data | Inject random values into waveform lookup table at controllable rate. Instead of reading the correct duty-cycle position, read from a corrupted index. | Waveform becomes unpredictable — moments of correct tone punctuated by bursts of wrong data. The iconic "scrambled" sound. |
| **Minus World** | Accessing unintended level data via boundary overflow | Parameter values wrap around their valid range. Cutoff at max wraps to min. Pitch at top wraps to bottom. Triggered by threshold crossing. | Sudden parameter jumps — a note bends up and snaps to the bottom, a filter opens and slams shut. Musical whiplash. |
| **Sprite Flicker** | NES 8-sprite-per-scanline limit causing rapid on/off | Rapid voice muting — when voice count exceeds a threshold, voices rapidly cycle on/off (like the NES flickering sprites). Rate = controllable. | Stuttering, rhythmic voice dropout. At low rates, rhythmic gating. At high rates, granular-like textures. |
| **Cartridge Tilt** | Physical contact interruption corrupting data bus | Momentary bit-corruption bursts. Random bits flip in the waveform data for short durations (10-100ms), then recover. Frequency and duration controllable. | Periodic crackles and data-damage sounds, like a loose connection. Subtle at low glitch, violent at high. |
| **Stuck Note** | Sound driver failing to send note-off | Probability that a voice ignores its note-off event. The note sustains indefinitely (or until the next note steals the voice). Creates unintended drone layers. | Accidental drones building up over time. A melodic line slowly accumulates into a cluster. Very musical at low probability. |

#### FM Glitches (Genesis)

| Glitch | Hardware Origin | DSP Implementation | Sound |
|--------|---------------|-------------------|-------|
| **Register Bleed** | Z80/68K bus conflict corrupting YM2612 registers | FM operator parameters randomly receive values intended for other operators. Modulation index, ratio, or envelope from one operator bleeds into another. | Timbre shifts mid-note — a clean FM piano suddenly gets a metallic overtone as the wrong modulation index appears. |
| **Algorithm Crash** | Sound driver writing invalid algorithm number | The FM algorithm (operator routing) changes mid-note. Smooth transitions between routing topologies — carriers become modulators, parallel becomes serial. | Continuous timbral morphing as the operator topology shifts. Sounds like the synth is "rethinking" its architecture in real-time. |
| **DAC Conflict** | PCM sample playback fighting with FM for the DAC | Digital noise/aliasing bursts injected into the FM output when GLITCH is high. Like the Genesis trying to play a sample and FM simultaneously through one converter. | Characteristic Genesis "crunch" — that lo-fi DAC distortion that defined the console's aggressive character. |
| **Clock Drift** | YM2612 clock instability | Sample rate of the FM synthesis fluctuates. Pitch drifts in non-musical intervals, then snaps back. | Detuned, wobbly pitch — like a Genesis with a dying crystal oscillator. Subtle = vintage warmth. Extreme = seasick. |

#### SAMPLE Glitches (SNES)

| Glitch | Hardware Origin | DSP Implementation | Sound |
|--------|---------------|-------------------|-------|
| **Echo Overflow** | Echo buffer writing into sample RAM | The FIR echo's delay buffer wraps around and overwrites the BRR sample data currently being played back. Creates feedback where the echo IS the instrument. | Self-referential feedback loop — the reverb tail corrupts the source, which feeds the reverb, which corrupts further. Evolving, unpredictable textures. |
| **BRR Decode Error** | Corrupted ADPCM block headers | The BRR decoder receives wrong scale/filter coefficients, producing distorted sample output. Controllable: how many blocks per second get corrupted. | The warm SNES sample becomes crunchy and broken. At low rates, occasional glitchy transients. At high rates, full sample destruction. |
| **Pitch Mod Runaway** | Channel pitch modulation feedback | SNES pitch modulation (one channel modulates the next) feeds back into itself with gain >1. Pitch modulation depth grows exponentially before being hard-clipped. | Pitch vibrato that accelerates into wild pitch sweeps. Starts musical, becomes chaotic, creates siren-like effects. |
| **SPC Hang** | SPC700 processor stall | One voice freezes — its sample playback position locks, creating a sustained single-sample tone (essentially a DC offset or very low frequency). Other voices continue normally. | One voice becomes a frozen drone/buzz while others play normally. The "stuck pixel" of audio. |

### Glitch Parameters

```
ow_glitchAmount      0.0–1.0    Master glitch intensity (0 = pristine, 1 = barely functional)
ow_glitchType        enum       Which glitch family is active (or "ALL" for random selection)
ow_glitchRate        0.1–20 Hz  How often glitch events trigger (tempo-syncable)
ow_glitchDepth       0.0–1.0    How severe each glitch event is
ow_glitchSync        bool       Sync glitch events to tempo (rhythmic glitching)
ow_glitchMix         0.0–1.0    Dry/glitched blend (for parallel glitch processing)
```

### Glitch + Coupling Interaction

The glitch system creates unique coupling possibilities:

| Coupling Route | Effect |
|---------------|--------|
| **External engine → XOverworld glitch amount** | Other engines' dynamics drive glitch intensity. ONSET kick hits increase glitch. ODYSSEY Climax triggers glitch cascade. |
| **XOverworld glitch output → external engine** | Glitch events modulate other engines' parameters. A Missingno burst opens ODDOSCAR's filter. A Sprite Flicker gates OBESE's output. |
| **Glitch as mod source** | The glitch event itself becomes a modulation source in XOlokun's coupling matrix — `GlitchToFilter`, `GlitchToMorph`, `GlitchToPitch`. |

### Glitch Design Philosophy

- **At 0%, XOverworld is a faithful chip synthesizer.** Pure, clean, historically accurate.
- **At 25%, subtle artifacts add vintage character.** Like a well-loved cartridge — occasional crackle, slight pitch drift. Most producers will live here.
- **At 50%, the glitches become musical.** Rhythmic stuttering, timbral morphing, echo feedback. Flux mood territory.
- **At 75%, the hardware is failing.** Dramatic parameter jumps, severe corruption, frozen voices. Experimental/sound design.
- **At 100%, this is a kill screen.** The chip is dying. Full corruption. Beautiful chaos.

This creates a spectrum from "pristine chiptune" to "vaporwave datamosh" on a single knob.

---

## Drum Synthesis

Unlike ONSET (dedicated drum voices), XOverworld makes drums the way the original hardware did — **the same voices that play melody also play drums**:

### Kick
- **8-BIT:** Triangle with fast pitch sweep (NES classic technique)
- **FM:** Algorithm 7 with high-ratio modulator, fast pitch envelope
- **SAMPLE:** BRR-encoded kick sample with Gaussian smoothing

### Snare
- **8-BIT:** Short noise burst (long-loop mode) + triangle transient
- **FM:** Dual-operator with noise modulator, bright metallic snap
- **SAMPLE:** BRR snare with pitch envelope

### Hi-Hat
- **8-BIT:** Noise channel, short-loop (metallic) for closed, long-loop for open
- **FM:** 6-operator metallic patch (non-harmonic ratios), amplitude envelope shapes open/closed
- **SAMPLE:** BRR cymbal with amplitude envelope

### Toms / Percussion
- **8-BIT:** Triangle with medium pitch sweep + noise layer
- **FM:** Low-ratio modulation with pitch envelope
- **SAMPLE:** BRR tom/perc samples

### Per-Voice Mode
Each voice has a toggle: **MELODIC** or **PERC**
- Melodic: pitch tracks keyboard, sustain available
- Perc: fixed pitch (or per-note pitch table), one-shot envelope, no sustain

## Macro Mapping (M1-M4)

| Macro | Label | What It Controls | Musical Intent |
|-------|-------|-----------------|----------------|
| M1 | ERA | Blend position across 8-BIT ↔ FM ↔ SAMPLE layers | The defining axis — morph between console generations |
| M2 | CIRCUIT | Chip-specific parameters: duty cycle (8-BIT), FM algorithm (FM), interpolation mode (SAMPLE) | Timbral character within the current era |
| M3 | GLITCH | Glitch intensity (0 = pristine, 1 = kill screen) — replaces COUPLING for single-engine; routes to coupling amount when coupled | The hardware failure axis — pristine to chaos |
| M4 | SPACE | SNES-style FIR echo depth + external FX | Space and depth |

**M3 design note:** GLITCH replaces the standard COUPLING macro when XOverworld runs solo. When coupled with another engine, M3 blends between glitch intensity and coupling amount — at M3=0 both are off, at M3=0.5 coupling is active, at M3=1.0 coupling + full glitch. This means coupling and glitching are related concepts: coupling IS a kind of controlled signal corruption between engines.

## Gallery Role
- **Sonic gap it fills:** Chip/retro synthesis — completely unoccupied. No engine recreates console-era sound architecture.
- **Differentiation from OBESE's bitcrusher:** OBESE applies bit reduction as a post-effect. XOverworld builds from chip architecture up — the synthesis method itself produces the character, not a filter applied after.
- **Differentiation from ONSET drums:** ONSET has dedicated voice roles (kick, snare, hat). XOverworld's voices are fluid — any can be melodic or percussive, reflecting how the original hardware worked.

## Coupling Partners

### Best Pairing: XOverworld + OVERDUB
- **Route:** XOverworld output → OVERDUB send chain (tape delay + spring reverb)
- **Musical effect:** 8-bit melodies through analog dub FX — lo-fi meets lo-fi in completely different dimensions
- **Why it's special:** NES sounds are dry and direct. OVERDUB's tape delay adds warmth and space that the original hardware couldn't produce. The contrast is striking.

### Strong Pairing: XOverworld + ONSET
- **Route:** ONSET drum triggers → XOverworld pitch/trigger
- **Musical effect:** Real synthesized drums driving chip melody triggers — modern rhythm section with retro melodic response
- **Coupling type:** RhythmToBlend (ONSET pattern drives XOverworld era blend — drums morph the console generation)

### Strong Pairing: XOverworld + ODYSSEY
- **Route:** ODYSSEY Climax system → XOverworld era blend + FM algorithm
- **Musical effect:** As the JOURNEY macro builds, chip sounds evolve from simple NES pulses to complex Genesis FM to cinematic SNES samples — a journey through gaming history
- **Coupling type:** EnvToMorph (ODYSSEY envelope → XOverworld era position)

### Interesting Pairing: XOverworld + OBESE
- **Route:** XOverworld pulse waves → OBESE's 13-oscillator stack
- **Musical effect:** Chip melodies exploded into massive stereo width — "what if the NES had 65 oscillators?"
- **Coupling type:** AudioToFM (chip audio as FM source into OBESE's oscillator bank)

### Interesting Pairing: XOverworld + OBLONG
- **Route:** OBLONG's warm character stages → XOverworld
- **Musical effect:** The soft, fuzzy warmth of OBLONG applied to crisp chip sounds — nostalgic warmth amplified

### Glitch-Specific Coupling: XOverworld Glitch → Any Engine
- **Route:** XOverworld's glitch events as modulation source → external engine parameters
- **Musical effect:** Game glitches corrupt other engines. A Missingno burst opens ODYSSEY's filter. Sprite Flicker gates OBESE's 13 oscillators. Echo Overflow drives OBLONG's smear mix. The glitch becomes a rhythmic/textural controller for the entire gallery.
- **Coupling type:** New type — `GlitchToParam` — XOverworld's glitch event output (0 when clean, 1 during glitch event) routes to any target parameter
- **Why this matters:** No other engine has a "controlled chaos" output. This makes XOverworld uniquely valuable as a coupling source — even without playing notes, its glitch system can animate other engines.

## Visual Identity

### Default Accent
- **Accent color:** Electric Green `#39FF14` — CRT phosphor green, the color of retro gaming
- **Material/texture:** CRT scanlines — subtle horizontal line pattern overlaid on the panel, pixel grid at edges
- **Icon concept:** A pixel-art controller D-pad, or a simple 8-bit waveform (stepped square wave)
- **Panel character:** Dark background within the panel (like a CRT screen), with bright green UI elements. Pixel font for value readouts. Subtle CRT curvature on panel corners.

### Color Theme Switcher — "Player Select"

XOverworld's engine panel includes a **theme selector** (dropdown or carousel) that swaps the accent color palette. Each theme is a subtle nod to an iconic character or console through color alone — no names, no logos, just the colors that anyone who grew up with these games will instantly recognize.

The theme only changes the XOverworld panel's accent colors within the gallery frame. The XOlokun shell (warm white walls, XO Gold macros) stays untouched — the gallery walls don't repaint for each exhibition, but the art inside changes.

#### Theme Palettes

| Theme Name | Nod To | Primary | Secondary | Accent | Panel Tint |
|------------|--------|---------|-----------|--------|------------|
| **CRT Phosphor** | Default / retro CRT | `#39FF14` Electric Green | `#2D8B0E` Dark Green | `#ADFF2F` Bright Green | Dark `#0A1A0A` (CRT black-green) |
| **Plumber Red** | Mario | `#E52521` Red | `#049CD8` Blue | `#FBD000` Gold | Dark `#1A0A0A` |
| **Plumber Green** | Luigi | `#43B047` Green | `#049CD8` Blue | `#FFFFFF` White | Dark `#0A1A0A` |
| **Blue Bomber** | Mega Man | `#4A90D9` Sky Blue | `#2563A3` Dark Blue | `#7AB5E8` Cyan | Dark `#0A0A1A` |
| **Hedgehog** | Sonic | `#0066CC` Cobalt | `#CC0000` Red | `#FFD700` Gold Ring | Dark `#0A0A1A` |
| **Hero's Tunic** | Link | `#1B813E` Forest Green | `#8B6914` Brown | `#C4A94D` Triforce Gold | Dark `#0A140A` |
| **Bounty Hunter** | Samus | `#FF6600` Orange | `#CC0000` Varia Red | `#00CC44` Visor Green | Dark `#1A0E0A` |
| **Kong Country** | DK & Diddy | `#8B4513` Brown | `#CC3333` Red Tie | `#FFD700` Banana Yellow | Dark `#140E0A` |
| **Pocket** | Game Boy | `#9BBB0F` Light Olive | `#8BAB0F` Mid Olive | `#306230` Dark Olive | `#0F380F` (actual GB dark) |
| **Vampire Killer** | Castlevania | `#7B2D8B` Purple | `#8B0000` Blood Red | `#C0C0C0` Silver Whip | Dark `#140A14` |
| **Shadow Warrior** | Ninja Gaiden | `#1A1A2E` Midnight | `#CC0000` Red | `#C0C0C0` Steel | Dark `#0A0A14` |
| **Star Road** | Super Mario World / SNES | `#FFD700` Star Gold | `#FF6B35` Sunset | `#4ECDC4` Cape Blue | Dark `#1A140A` |
| **Frozen Cavern** | Metroid / sci-fi | `#00CED1` Ice Cyan | `#2E4057` Deep Space | `#66FF66` Scan Green | Dark `#0A1414` |
| **Sunset Drive** | OutRun / racing | `#FF6B9D` Pink | `#C44569` Magenta | `#F8B500` Sunset Gold | Dark `#1A0A14` |
| **Emerald Coast** | Tropical / island stages | `#00BFA5` Teal | `#FFB74D` Sand | `#4FC3F7` Sky Blue | Dark `#0A1A14` |

#### Theme Switcher Implementation

```
ow_colorTheme     enum (0-14)    Selects the active color palette
```

- **UI behavior:** When the theme changes, the XOverworld panel's accent colors smoothly crossfade (200ms)
- **Preset storage:** The selected theme is saved per-preset in `.xometa` — a "Kong Country" bass preset loads with brown/red/yellow, a "Blue Bomber" lead loads with blues
- **Gallery rule:** The theme ONLY affects the XOverworld engine panel. The XOlokun shell, other engine panels, coupling strip, and preset browser are unaffected. Each engine is its own exhibition — XOverworld's exhibition just has multiple rooms.
- **Default:** "CRT Phosphor" (Electric Green) — the engine's primary brand color for marketing and screenshots

#### Why Per-Preset Themes Work

A producer building a Genesis-style track might use "Hedgehog" blue for their FM bass presets and "Shadow Warrior" dark for their Ninja Gaiden-inspired leads. The visual theme reinforces the sonic era they're working in — the panel becomes a mood board for the sound.

## Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| Foundation | Strong | Chip drums, bass lines — the rhythmic foundation of game music |
| Atmosphere | Moderate | SNES-era ambient pads (DKC water levels, Metroid) |
| Entangled | Strong | Coupled with ONSET or OVERDUB, chip sounds become something new |
| Prism | Very strong | Chip melodies ARE prismatic — bright, clear, articulate, melodic |
| Flux | Moderate | Glitchy chip artifacts, noise mode, DPCM crunch |
| Aether | Low-Moderate | SNES reverb can go cinematic, but this isn't the engine's primary territory |

## Preset Philosophy: Iconic Sounds, Subtle Nods

Every preset reverse-engineers a specific iconic game soundtrack synth sound. The names give knowing winks to the source without crossing into trademark territory — a producer who grew up with these games will smile, but the names work on their own as evocative sound descriptions.

**Naming rules:**
- Never use game titles, character names, or franchise-specific terms
- Nod through setting, feeling, or mechanic — not proper nouns
- Names should work even if you've never played the game
- Reference the *world* or *feeling*, not the *product*

---

### Franchise Coverage Map

Research across NES, Genesis, and SNES eras identified these iconic soundtracks. Every franchise below has at least one preset. Franchises marked ★ have multiple presets across eras.

| Franchise | Era(s) | Composer(s) | Iconic Sound | Coverage |
|-----------|--------|-------------|-------------|----------|
| ★ **Mario** | NES, SNES | Koji Kondo | Bouncy pulse, coin SFX, overworld joy | 5 presets |
| ★ **Zelda** | NES, SNES | Koji Kondo | Adventure arps, dungeon drones, fanfares | 3 presets |
| ★ **Mega Man** | NES, SNES | Tateishi, Yamamoto | Tight pulse arps, stage select, energetic | 3 presets |
| ★ **Castlevania** | NES, SNES, Genesis | Yamashita, various | Gothic minor key, driving, dark | 3 presets |
| ★ **Metroid** | NES, SNES | Tanaka, Yamamoto | Atmospheric, sparse, eerie, echo-heavy | 3 presets |
| ★ **Sonic** | Genesis | Nakamura, Senoue | Slap bass, ring bells, speed | 4 presets |
| ★ **Streets of Rage** | Genesis | Yuzo Koshiro | House/techno FM bass, dancefloor leads | 3 presets |
| ★ **DKC** | SNES | David Wise | Atmospheric BRR, underwater, jungle | 4 presets |
| ★ **Final Fantasy** | NES, SNES | Nobuo Uematsu | Crystal theme, opera, orchestral | 4 presets |
| ★ **Chrono Trigger** | SNES | Yasunori Mitsuda | Emotional strings, time themes, wind | 3 presets |
| **Ninja Gaiden** | NES | Keiji Yamagishi | Cinematic rock-influenced, driving pulse | 2 presets |
| **Contra** | NES, SNES | Kazuki Muraoka | Aggressive military, heavy triangle bass | 2 presets |
| **Duck Tales** | NES | Yoshihiro Sakaguchi | Moon theme, dreamy, technically impressive | 1 preset |
| **Punch-Out!!** | NES | Yukio Kaneoka | Dramatic, training montage, boxing ring | 1 preset |
| **Golden Axe** | Genesis | Tohru Nakabayashi | Fantasy FM brass, medieval, dramatic | 2 presets |
| **Final Fight** | Genesis, SNES | Various | Beat-em-up FM bass, punchy, rhythmic | 1 preset |
| **Shinobi** | Genesis | Yuzo Koshiro | Eastern-influenced FM, atmospheric | 2 presets |
| **Thunder Force** | Genesis | Technosoft Sound Team | Heavy metal FM, aggressive leads | 1 preset |
| **Ecco the Dolphin** | Genesis | Spencer Nilsen | Ambient underwater FM, slow, mysterious | 1 preset |
| **ToeJam & Earl** | Genesis | John Baker | Funk FM bass, groovy, chill | 1 preset |
| **Comix Zone** | Genesis | Howard Drossin | Grunge/alt rock from FM(!) | 1 preset |
| **Secret of Mana** | SNES | Hiroki Kikuta | Mystical BRR, world music, flute | 2 presets |
| **EarthBound** | SNES | Suzuki, Tanaka | Quirky, unusual, experimental sampling | 2 presets |
| **Star Fox** | SNES | Hajime Hirasawa | Synthetic orchestra, heroic, compressed | 2 presets |
| **F-Zero** | SNES | Kanki, Ishida | High energy, rock guitar samples, racing | 1 preset |
| **Kirby** | NES, SNES | Jun Ishikawa | Cheerful, round, soft, playful | 1 preset |
| **Silver Surfer** | NES | Tim & Geoff Follin | Boundary-pushing arps, prog rock chiptune | 1 preset |
| **Batman (Sunsoft)** | NES | Naoki Kodaka | Dark, driving, incredible Sunsoft bass | 1 preset |
| **Journey to Silius** | NES | Naoki Kodaka | Heavy Sunsoft bass, dark, pulsing | 1 preset |
| **Phantasy Star** | Genesis | Tokuhiko Uwabo | Sci-fi FM, atmospheric, ethereal | 1 preset |
| **Beyond Oasis** | Genesis | Yuzo Koshiro | Rich FM, one of the best FM scores | 1 preset |
| **Gunstar Heroes** | Genesis | Norio Hanzawa | Energetic action FM, fast-paced | 1 preset |
| **ActRaiser** | SNES | Yuzo Koshiro | Orchestral BRR, religious, powerful | 1 preset |
| **Super Metroid** | SNES | Kenji Yamamoto | Dark ambient, pioneering echo/reverb | 1 preset |
| **Plok** | SNES | Follin brothers | Genre-hopping, technically stunning | 1 preset |
| **Mega Man X** | SNES | Setsuo Yamamoto | Hybrid chip+sample, energetic, evolved | 1 preset |

---

### NES Era (8-BIT Layer) — Presets

| Preset Name | Mood | Type | Franchise Nod | What's Being Modeled |
|-------------|------|------|---------------|---------------------|
| "Quest Field" | Prism | Melodic | Zelda overworld | Pulse arp (25% duty), triangle bass pedal, bouncy tempo |
| "Dungeon Echo" | Aether | Pad | Zelda dungeons | Sparse pulse drone, dark, reverberant, foreboding |
| "Robot Roster" | Prism | Melodic | Mega Man stage select | Tight pulse arpeggios, rapid-fire 12.5% duty, punchy |
| "Weapon Get" | Prism | SFX | Mega Man victory jingle | Fast ascending pulse arp, bright, triumphant, short |
| "Moon Gravity" | Atmosphere | Melodic | Duck Tales Moon | Soaring pulse lead (50% duty), slow triangle sub, dreamy |
| "Vampire Stairwell" | Flux | Melodic | Castlevania Bloody Tears | Dark minor key pulse lead, aggressive 25% duty, driving |
| "Holy Water" | Atmosphere | Melodic | Castlevania atmosphere | Eerie pulse arpeggios, slow, gothic, reverberant |
| "Bouncy Plumber" | Prism | Melodic | Mario theme | Playful pulse melody, staccato triangle bass, bright |
| "Underground" | Foundation | Melodic | Mario underground | Staccato bass pulse, 12.5% duty, mysterious, rhythmic |
| "Star Power" | Prism | Melodic | Mario invincibility | Fast pulse arps, bright, urgent, joyful energy |
| "Planet Zebes" | Aether | Pad | Metroid Brinstar | Sparse, eerie pulse drones, noise texture, cold |
| "Escape Sequence" | Flux | Melodic | Metroid escape timer | Frantic pulse arps, escalating pitch, urgent, tense |
| "Jungle Cartridge" | Foundation | Melodic | Contra bass | Heavy triangle bass, aggressive pulse riff, 50% duty |
| "Alien Lair" | Flux | Melodic | Contra alien stage | Aggressive noise + pulse, dissonant, threatening |
| "Dream Balloon" | Atmosphere | Melodic | Kirby | Cheerful, round pulse tones, gentle, soft attack |
| "Crystal Cavern" | Prism | Melodic | Final Fantasy crystal theme | Slow pulse arpeggios, reverberant, mystical |
| "Battle Fanfare" | Prism | Melodic | Final Fantasy battle | Fast pulse lead, driving triangle bass, heroic |
| "Coin Chime" | Prism | Perc | Mario coin | Pitched pulse with fast pitch-up sweep, bright ping |
| "Shadow Blade" | Flux | Melodic | Ninja Gaiden Act 1 | Rock-influenced pulse, driving, cinematic, intense |
| "Cliff Temple" | Prism | Melodic | Ninja Gaiden Act 4 | Dark pulse arps over triangle bass, atmospheric, tense |
| "Title Belt" | Prism | Melodic | Punch-Out training | Dramatic pulse fanfare, punchy triangle, motivational |
| "Silver Nebula" | Prism | Melodic | Silver Surfer | Boundary-pushing thick arpeggios, prog rock, dense |
| "Dark Knight Bass" | Foundation | Melodic | Batman (Sunsoft) | That incredible Sunsoft bass technique — deep, dark, pulsing |
| "Silius Drive" | Foundation | Melodic | Journey to Silius | Heavy Sunsoft triangle bass, driving pulse lead, dark |
| "Warp Pipe" | Flux | SFX | Mario pipe | Rapid descending pulse pitch sweep |
| "Pixel Snare" | Foundation | Drum | NES noise snare | Short noise burst, long-loop LFSR, tight |
| "Triangle Kick" | Foundation | Drum | NES triangle kick | Fast pitch sweep (high→low), no volume envelope |
| "Metallic Hat" | Foundation | Drum | NES noise hi-hat | Short-loop noise (metallic), very short decay |
| "8-Bit Kit" | Foundation | Drum | General NES kit | Triangle kick + noise snare + metallic hat combined |

### Genesis Era (FM Layer) — Presets

| Preset Name | Mood | Type | Franchise Nod | What's Being Modeled |
|-------------|------|------|---------------|---------------------|
| "Loop De Loop" | Foundation | Melodic | Sonic Green Hill bass | FM slap bass, algorithm 5, bright attack, tight decay |
| "Ring Scatter" | Prism | Melodic | Sonic ring collect | FM bell arp, low ratio modulator, crystalline |
| "Spring Yard" | Prism | Melodic | Sonic Spring Yard keys | FM electric piano, algorithm 4, warm detuned |
| "Casino Night" | Prism | Melodic | Sonic 2 Casino Night | Jazzy FM keys, smooth, bright, playful |
| "Bare Knuckle" | Foundation | Melodic | Streets of Rage 2 bass | Deep FM bass, algorithm 7, sub-heavy, house-influenced |
| "Neon Uppercut" | Prism | Melodic | Streets of Rage 2 lead | Bright FM lead, high modulation index, punchy, dancefloor |
| "Go Straight" | Foundation | Melodic | Streets of Rage 2 opening | Driving FM bass + lead layer, techno energy, urgent |
| "Dolphin Song" | Aether | Pad | Ecco the Dolphin | Soft FM pad, low modulation, underwater, slow |
| "Shinobi Flute" | Prism | Melodic | Revenge of Shinobi | Eastern-scale FM lead, narrow vibrato, sharp |
| "Oboro Shadow" | Atmosphere | Melodic | Shinobi III atmosphere | Atmospheric FM, dark, ninja stealth, mysterious |
| "Funk Alien" | Flux | Melodic | ToeJam & Earl | Funky FM bass, algorithm 6, wah-like, groovy |
| "Thunder Shred" | Flux | Melodic | Thunder Force IV | Aggressive FM lead, high feedback, screaming, fast |
| "Street Brawl" | Foundation | Melodic | Final Fight | Punchy FM bass, short decay, rhythmic, beat-em-up |
| "Axe Warrior" | Foundation | Melodic | Golden Axe wilderness | Fantasy FM brass, dramatic, medieval, majestic |
| "Skeleton Ride" | Flux | Melodic | Golden Axe beast ride | Dark FM lead, aggressive, driving, battle energy |
| "Comic Crunch" | Flux | Melodic | Comix Zone | Grunge/alt rock from FM synthesis — distorted, raw, 90s |
| "Distant Stars" | Aether | Pad | Phantasy Star | Sci-fi FM pad, ethereal, spacey, mysterious |
| "Oasis Garden" | Atmosphere | Melodic | Beyond Oasis | Rich FM, one of the most lush FM tones ever, warm |
| "Run And Gun" | Prism | Melodic | Gunstar Heroes | Energetic FM lead, fast-paced, action, bright |
| "Genesis Snare" | Foundation | Drum | Sonic snare | FM noise burst, high-ratio modulator, bright snap |
| "Mega Drive Kick" | Foundation | Drum | Genesis kick | FM algorithm 7, fast pitch sweep, sub-heavy |
| "PSG Hat" | Foundation | Drum | Genesis PSG hi-hat | PSG noise, very short, metallic |
| "Blast Processing" | Foundation | Drum | Full Genesis kit | FM kick + snare + PSG hats + tom fills |

### SNES Era (SAMPLE Layer) — Presets

| Preset Name | Mood | Type | Franchise Nod | What's Being Modeled |
|-------------|------|------|---------------|---------------------|
| "Coral Drift" | Atmosphere | Pad | DKC Aquatic Ambiance | BRR pad, Gaussian interpolation, FIR echo, underwater warmth |
| "Minecart Dawn" | Prism | Melodic | DKC Mine Cart Carnage | Bright BRR melody, fast tempo, rhythmic, joyful |
| "Jungle Canopy" | Atmosphere | Pad | DKC2 forest themes | Layered BRR pads, light FIR echo, tropical warmth |
| "Bramble Hymn" | Aether | Pad | DKC2 Stickerbush Symphony | Hauntingly beautiful BRR, atmospheric, wide echo, emotional |
| "Time Gate" | Aether | Pad | Chrono Trigger wind scene | Orchestral BRR strings, long reverb, cinematic, emotional |
| "Epoch Wind" | Atmosphere | Pad | Chrono Trigger 600 AD | Warm BRR woodwind, soft Gaussian smoothing, gentle |
| "Millennial Fair" | Prism | Melodic | Chrono Trigger opening | Bright BRR melody, festive, joyful, layered |
| "Opera Balcony" | Aether | Melodic | FF6 Opera Scene | BRR vocal/choir sample, dramatic, lush reverb |
| "Phantom Ship" | Aether | Pad | FF6 darker themes | Dark BRR strings, low Gaussian, eerie, wide echo |
| "Veldt Sunrise" | Atmosphere | Pad | FF6 The Veldt | Warm BRR strings, gentle, pastoral, wide |
| "Prelude Harp" | Prism | Melodic | FF series prelude arp | Classic ascending BRR harp arpeggio, crystalline, iconic |
| "Lower Norfair" | Aether | Pad | Super Metroid atmosphere | Dark ambient BRR, pioneering echo/reverb, Eno-inspired |
| "Sacred Grove" | Aether | Pad | Secret of Mana ambient | Mystical BRR flute, long reverb, ancient, reverent |
| "Mana Fortress" | Flux | Melodic | Secret of Mana final area | Intense BRR, aggressive, urgent, layered |
| "Quirky Town" | Prism | Melodic | EarthBound Onett | Unusual BRR tones, slightly off-kilter, charming, warm |
| "Fuzzy Pickles" | Flux | Melodic | EarthBound battle music | Distorted BRR samples, experimental, weird, wonderful |
| "Pixel Orchestra" | Prism | Melodic | Star Fox Corneria | Full BRR orchestral stab, bright, heroic, compressed |
| "Barrel Roll" | Foundation | Melodic | Star Fox bass | Punchy BRR bass, short decay, driving |
| "Mach Circuit" | Foundation | Melodic | F-Zero Big Blue | Driving BRR rock guitar, fast, racing energy |
| "Sky Fortress" | Prism | Melodic | ActRaiser orchestral | Powerful BRR orchestra, religious overtones, majestic |
| "Contra Blitz" | Flux | Melodic | Contra III | Aggressive BRR, fast, military, action |
| "Gothic Stairwell" | Aether | Melodic | Super Castlevania IV | Gothic BRR organ + strings, atmospheric SNES reverb |
| "Maverick Hunter" | Prism | Melodic | Mega Man X | Energetic hybrid BRR + chip feel, evolved, powerful |
| "Folksy Bounce" | Prism | Melodic | Plok (Follin brothers) | Genre-hopping BRR, technically stunning, surprising |
| "Cape Flight" | Prism | Melodic | Super Mario World | Joyful BRR, layered, adventure, Koji Kondo warmth |
| "Mode 7 Pad" | Atmosphere | Pad | General SNES atmospheric | Warm Gaussian-smoothed pad, medium echo, floating |
| "BRR Snare" | Foundation | Drum | SNES sample snare | BRR-compressed snare, Gaussian-softened transient |
| "Sample Kick" | Foundation | Drum | SNES sample kick | BRR kick, warm low end, slightly muffled by compression |

### Cross-Era Presets (Blend Axis) — The XOverworld Signature

| Preset Name | Mood | Type | Blend | What's Being Modeled |
|-------------|------|------|-------|---------------------|
| "Console Evolution" | Entangled | Melodic | 8-BIT→FM→SAMPLE sweep | M1 (ERA) sweeps through all three generations in one patch |
| "16-Bit Upgrade" | Prism | Melodic | 8-BIT→SAMPLE morph | NES pulse that warms into SNES sample — the generational leap |
| "Blast to Barrel" | Entangled | Melodic | FM→SAMPLE morph | Genesis punch softening into SNES warmth — Sonic to DKC |
| "Cartridge Swap" | Flux | Melodic | Rapid era switching | LFO on ERA blend — rapidly alternating between chip sounds |
| "Boss Rush" | Flux | Melodic | FM+8-BIT layered | Aggressive FM lead + NES pulse arp, final boss energy |
| "Save Point" | Atmosphere | Pad | SAMPLE + 8-BIT | SNES pad warmth with NES triangle bass — rest and safety |
| "New Game Plus" | Entangled | Melodic | All three blended | All eras present simultaneously — the ultimate hybrid |
| "Two Player" | Entangled | Melodic | 8-BIT + FM split | Keyboard split: left hand NES bass, right hand Genesis lead |
| "Overworld Theme" | Prism | Melodic | 8-BIT dominant | The hero preset — classic adventure pulse melody, quest energy |
| "Game Over" | Aether | Pad | SAMPLE + noise | Descending BRR tones, slow, melancholic, fading echo |

### Glitch Presets — Hardware Failure as Art

| Preset Name | Mood | Type | Era | Glitch Type | Description |
|-------------|------|------|-----|-------------|-------------|
| "Missing Number" | Flux | Melodic | 8-BIT | Missingno | Pulse melody with corrupted waveform table — moments of clarity between bursts of wrong data |
| "Minus World" | Flux | Melodic | 8-BIT | Minus World | NES bass with parameter wrapping — pitch bends up and snaps to bottom, eerie loop |
| "Sprite Overflow" | Flux | Melodic | 8-BIT | Sprite Flicker | Rapid voice muting creates rhythmic stuttering gate effect, tempo-synced |
| "Loose Cartridge" | Flux | Melodic | 8-BIT | Cartridge Tilt | Clean pulse melody with periodic data-corruption crackles, subtle contact problems |
| "Ghost Notes" | Atmosphere | Pad | 8-BIT | Stuck Note | Low probability stuck notes build accidental drone clusters over time — hauntingly musical |
| "Register Spill" | Flux | Melodic | FM | Register Bleed | FM electric piano where operator parameters randomly shift mid-note — unstable beauty |
| "Algorithm Zero" | Flux | Melodic | FM | Algorithm Crash | FM lead with shifting operator topology — the synth rethinks its routing in real-time |
| "DAC Warfare" | Flux | Melodic | FM | DAC Conflict | Genesis bass with characteristic DAC crunch, aliasing bursts on attack transients |
| "Crystal Decay" | Flux | Pad | FM | Clock Drift | FM pad with dying-oscillator pitch drift — detuned, wobbly, seasick |
| "Echo Eats Itself" | Flux | Pad | SAMPLE | Echo Overflow | SNES pad where the FIR echo overwrites sample RAM — evolving self-referential feedback |
| "Corrupt Cartridge" | Flux | Melodic | SAMPLE | BRR Decode Error | Clean SNES melody progressively corrupted by wrong ADPCM decode coefficients |
| "Pitch Runaway" | Flux | Melodic | SAMPLE | Pitch Mod Runaway | SNES vibrato that accelerates into wild pitch sweeps before snapping back |
| "Frozen Channel" | Atmosphere | Drone | SAMPLE | SPC Hang | One voice freezes into a sustained buzz while others continue — the stuck pixel of audio |
| "Kill Screen" | Flux | Melodic | ALL | All at 100% | Full corruption across all eras — the game is over, the hardware is dying, beautiful chaos |
| "Continue Screen" | Atmosphere | Pad | ALL | All at 25% | Subtle artifacts across all eras — like a well-loved cartridge, warm and worn |
| "Glitch Hop Kit" | Flux | Drum | 8-BIT+FM | Sprite Flicker + DAC | Drum kit with stuttering voice gating and DAC crunch — production-ready glitch percussion |
| "Datamosh Pad" | Flux | Pad | SAMPLE | Echo Overflow + BRR | SNES echo feedback eating its own sample data — evolving, never-repeating ambient texture |
| "Speed Run" | Flux | Melodic | ALL | Minus World + fast rate | Rapid parameter boundary wrapping — musical whiplash at tempo-synced intervals |

### Drum Kits (Cross-Era)

| Preset Name | Mood | Type | Era | Description |
|-------------|------|------|-----|-------------|
| "Chiptune Kit" | Foundation | Drum | 8-BIT | Full NES percussion: triangle kick, noise snare, metallic hat, noise crash |
| "Mega Drive Kit" | Foundation | Drum | FM | Full Genesis percussion: FM kick, FM snare, PSG hats, FM tom |
| "Sample Kit" | Foundation | Drum | SAMPLE | Full SNES percussion: BRR kick, BRR snare, BRR hats, BRR toms |
| "Era Morph Kit" | Entangled | Drum | Blend | Each drum voice at a different era position — NES kick, FM snare, SNES hat |
| "Boss Battle Kit" | Flux | Drum | FM+8-BIT | Aggressive kit for battle themes — punchy FM kick, noise crash, fast |

---

### Preset Library Summary

| Mood | Count | Primary Source |
|------|-------|---------------|
| Foundation | 28 | All eras (drums, bass, Sunsoft bass, beat-em-up) |
| Atmosphere | 14 | SAMPLE pads, NES ambient, Genesis atmosphere, glitch ambient |
| Entangled | 5 | Cross-era blends |
| Prism | 30 | All eras (melodic leads, arps, keys, fanfares) |
| Flux | 30 | Aggressive FM, NES action, glitch system, experimental |
| Aether | 13 | SNES cinematic, gothic, sci-fi, dungeon |
| **Total** | **120** | |

**Franchise coverage:** 36 franchises/games represented across 120 presets.

**Flux mood impact:** XOverworld alone contributes 30 Flux presets — nearly matching the entire existing XOlokun Flux count (36). This engine fills the gallery's weakest category.

**Visual identity impact:** 15 color themes give producers a visual connection to the franchise/era they're working in. The theme is saved per-preset, so "Bare Knuckle" loads with Hedgehog blue and "Coral Drift" loads with Kong Country brown/red/yellow.

## Parameter Namespace
- **Prefix:** `ow_` (overworld)
- **Examples:** `ow_era`, `ow_dutyCycle`, `ow_fmAlgorithm`, `ow_brrInterpolation`, `ow_noiseMode`

## Voice Architecture
- **Max voices:** 8 (matching the SNES's channel count — the highest of the three consoles)
- **Voice stealing:** Oldest note (matching original hardware behavior)
- **Legato:** Yes (for FM lead lines)
- **Per-voice mode:** Melodic or Percussive toggle
