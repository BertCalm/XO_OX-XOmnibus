# Engine Comparator: Percussion Trio
**ONSET × OSTINATO × OWARE — Side-by-Side Analysis**
Date: 2026-03-21 | Scope: Three engines | Format: Comparative reference

---

## Quick Reference Card

| Dimension | ONSET | OSTINATO | OWARE |
|-----------|-------|----------|-------|
| Identity | "The Surface Splash" | "The Fire Circle" | "The Resonant Board" |
| Accent | Electric Blue `#0066FF` | Firelight Orange `#E8701A` | Akan Goldweight `#B5883E` |
| Mythology | feliX's electric transients at the water's surface | Bioluminescent campfire on the shore — ocean creatures gathering | Sunken mancala board encrusted with coral and bronze |
| Material range | Electronic → Digital physics | Acoustic membranes × 12 traditions | Wood → Bell → Metal → Bowl (continuous morph) |
| Voice architecture | 8 dedicated fixed voices (GM drum map) | 8 assignable seats (any of 12 instruments) | 8-voice polyphonic (MIDI pitched) |
| Rhythmic engine | Pure triggered (no sequencer) | Built-in 16-step pattern sequencer per seat | Pure triggered (no sequencer) |
| Synthesis core | Dual-layer: Circuit (TR-808/909) + Algorithm (FM/Modal/K-S/PD), blendable | Physical modeling: modal membrane + waveguide body per instrument | Physical modeling: 8-mode resonator bank + material continuum + sympathetic resonance |
| Seance score | Ahead of industry / non-numeric (Blessings B002+B006) | 8.7/10 (highest numeric score in fleet as of 2026-03-20) | New — Architect review 2026-03-21 (D003 exemplary; D004 blocking fix pending LFOs) |
| Parameter prefix | `perc_` | `osti_` | `owr_` |
| Blessings | B002 (XVC), B006 (Dual-Layer Blend) | — | — |

---

## 1. Synthesis Approach

### ONSET — Circuit / Algorithm Dual-Layer

ONSET is the only engine in the fleet that simultaneously inhabits two entirely different synthesis paradigms and crossfades between them in real time. Every voice carries a **Layer X (Circuit)** and a **Layer O (Algorithm)** module, blended via equal-power cos/sin curves controlled by the `perc_vN_blend` parameter.

**Layer X — Circuit models:**
- `BridgedTOsc` — Roland TR-808 bridged-T oscillator topology. Sine oscillator with exponential pitch envelope sweep (up to 4 octaves downward chirp), sub-oscillator (triangle, one octave below), diode-starvation tanh saturation. Used for Kick and Tom voices.
- `NoiseBurstCircuit` — TR-808/909 hybrid. Dual body oscillators (180 Hz and 330 Hz, the 808 snare's resonant pair) mixed with high-pass filtered noise. Clap mode retriggers the noise burst 3 times at 10ms intervals, recreating the original 808 multi-burst VCA circuit. Used for Snare, Clap.
- `MetallicOsc` — TR-808 hi-hat: 6 square-wave oscillators at non-harmonic frequencies (205.3, 304.4, 369.6, 522.7, 800.0, 1048.0 Hz, measured from original schematics) summed through dual bandpass filters + highpass. Used for Hi-Hats, Perc B.

**Layer O — Algorithm models:**
- FM synthesis (Yamaha DX7 lineage): 2-operator FM with modulator envelope
- Modal resonators (Stanford CCRMA/IRCAM physical modeling): parallel bandpass resonators
- Karplus-Strong plucked string (with adjustable brightness and string-to-snare blend via snare-wire coupling parameter)
- Phase distortion (Casio CZ-101): DCW warping across sine/rectified/clipped waveshape zones

The **Transient Designer** (`OnsetTransient`) fires a pitched sine spike (4×–16× base frequency, 1–6ms) plus a noise burst (1–3ms) before the main voice body on every trigger — modeling stick-on-membrane impact energy.

**Character Stage** (post-mix): tanh saturation grit + LP warmth filter, followed by tape delay → Schroeder reverb → bit-depth LoFi rack.

**Cross-Voice Coupling (XVC)** is ONSET's defining architectural feature (Blessing B002): voice peak amplitudes from the previous block feed forward as modulation sources to other voices. Kick opens snare filter. Snare tightens hat decay. Kick ducks tom pitch. Snare shifts perc blend toward algorithm. This is not coupling via the MegaCouplingMatrix — it is internal, operating within a single engine instance at the voice level.

---

### OSTINATO — Physical Modeling: Modal Membrane + Waveguide Body

OSTINATO models the acoustics of real-world hand drums with genuine physical rigor. Each of 8 seats can host any of 12 instruments, each instrument fully characterized by:

**Modal membrane (`OstiModalMembrane`):** Up to 8 parallel CytomicSVF bandpass resonators, each tuned to a vibrational mode of the drum membrane. Modal frequency ratios derived from circular membrane Bessel function zeros (Kinsler & Frey), with per-instrument tuning offsets from published acoustic measurements (Raman 1934 for tabla; Brindle et al. JASA 2005 for djembe; Fletcher & Rossing for taiko). The Q of each resonator is derived from the mode's decay coefficient — longer-decaying modes are more resonant.

**Waveguide body (`OstiWaveguideBody`):** Four body shapes, each with a distinct resonance model:
- Cylindrical (djembe, conga, surdo): delay line + allpass + reflection coefficient
- Conical (doumbek, bongos): shorter delay, brighter reflection
- Box (cajón): multi-tap delay, wide spatial resonance
- Open (frame drum, tongue drum): minimal body coloring, direct radiation

**Excitation** is articulation-specific: each of the 3–4 articulations per instrument has its own noise burst duration, pitch spike ratio, pitch spike duration, brightness offset, and exciter-mix balance. Djembe tone vs. slap vs. bass vs. mute are genuinely different physical models, not just EQ offsets.

**Radiation filter** shapes the "open" vs. "mute" character after the waveguide body.

The `Beatbox` entry in the 12-instrument table is a synthetic/vocal percussion instrument with non-physical modal ratios, giving OSTINATO a bridge back to electronic sounds without breaking its architecture.

---

### OWARE — Physical Modeling: Material Continuum + Sympathetic Resonance Network

OWARE models tuned percussion instruments — instruments where pitch is intentional and polyphony is expected. Its synthesis is organized around 7 architectural pillars:

**1. Material Continuum:** Four material types (wood, bell, metal, bowl) each with distinct static mode ratio tables derived from Rossing (2000) and Fletcher & Rossing (1998). The `owr_material` parameter morphs continuously across all four, interpolating mode ratios and Q values simultaneously. A material exponent alpha (2.0 at wood → 0.3 at metal) controls differential decay: wood upper modes die fast, metal upper modes ring together with the fundamental.

**2. Mallet Physics (`OwareMalletExciter`):** Chaigne contact model (1997). Contact time is velocity and hardness dependent (5ms soft → 0.5ms hard). A spectral lowpass on the excitation signal models the Hertz contact model: soft mallets excite only low modes, hard mallets excite all modes. Mallet bounce (15–25ms secondary hit at 30% amplitude) activates for soft, slow strikes — the physics of a mallet bouncing off the bar.

**3. Sympathetic Resonance Network:** Each voice's modal output is visible to all other active voices via `OwareMode::lastOutput`. If any two sounding modes across different voices are within 50 Hz of each other, the closer voice injects a frequency-proximity-weighted signal into the other's mode. This is spectrum-selective coupling, not amplitude-based — a gamelan string can reinforce a vibraphone bar at a shared harmonic without affecting unrelated modes.

**4. Resonator Body (`OwareBodyResonator`):** Three body types with Gaussian proximity decay boost: tube harmonics reinforce membrane modes that fall near tube resonances; frame body uses fixed resonances at 200/580/1100 Hz; bowl model uses sub-octave + fundamental resonances. The boost is computed once at note-on (Improvement #7) and stored per-voice per-mode to avoid per-sample overhead.

**5. Buzz Membrane:** BPF-extracted tanh nonlinearity modeling the balafon/gyil spider-silk mirliton. The 200–800 Hz band is extracted, soft-clipped with a sensitivity-scaled tanh, then re-injected. Band-selective, not whole-signal saturation.

**6. Breathing Gamelan (`OwareVoice::shimmerLFO`):** Balinese beat-frequency shimmer implemented as an additive Hz offset (not ratio-based cents). Each voice carries a shadow LFO that modulates between in-tune and detuned states — the shimmer rate is in Hz, which is the physically correct Balinese model (gamelan pairs are tuned to beat at a fixed cycle rate, not a fixed ratio).

**7. Thermal Drift:** A shared slow tuning scalar (PRNG-driven, new target every ~4 seconds, approached via a very slow first-order filter) plus per-voice personality seeds create permanent micro-detuning personality. The feature that makes OWARE feel alive between notes.

---

## 2. Voice Architecture

### ONSET — 8 Dedicated Fixed Voices

8 voices, each permanently mapped to a role, MIDI note, circuit type, and default algorithm:

| Voice | Role | Circuit | Default Algo | MIDI | Base Freq |
|-------|------|---------|-------------|------|-----------|
| V1 | Kick | BridgedT | Modal | 36 (C2) | 55 Hz |
| V2 | Snare | NoiseBurst | FM | 38 (D2) | 180 Hz |
| V3 | HH Closed | Metallic | FM | 42 (F#2) | 8000 Hz |
| V4 | HH Open | Metallic | FM | 46 (A#2) | 8000 Hz |
| V5 | Clap | NoiseBurst | PhaseDist | 39 (D#2) | 1200 Hz |
| V6 | Tom | BridgedT | Modal | 45 (A2) | 110 Hz |
| V7 | Perc A | BridgedT | Karplus-Strong | 37 (C#2) | 220 Hz |
| V8 | Perc B | Metallic | Modal | 44 (G#2) | 440 Hz |

Each voice has 10 parameters (blend, algoMode, pitch, decay, tone, snap, body, character, level, pan) plus envelope shape. The kit identity is fixed: you tune and morph the sounds, but the roles are assigned. Hat choke (V3 chokes V4) is a hardwired relationship that can be disabled via `perc_xvc_hat_choke`.

**Per-voice breathing LFO:** Each voice carries a hardcoded 0.08 Hz LFO that modulates its filter cutoff by ±15%, satisfying D005 with minimal overhead.

---

### OSTINATO — 8 Assignable Seats

8 seats (indexed 0–7, mapped to MIDI notes 36–43 and alternate octave 48–55). Each seat is independently configurable — any seat can hold any of the 12 instruments. The default configuration places Djembe, Taiko, Conga, Tabla, Cajón, Doumbek, Frame Drum, and Surdo around the circle, giving an immediately playable world percussion ensemble.

**Polyphony:** `getMaxVoices()` returns `kNumSeats * 2` (16) — each seat can run two sub-voices simultaneously for flam/ghost triggering.

**Autonomous vs. manual:** Seats operate in two modes simultaneously. The 16-step pattern sequencer fires autonomously at the engine's tempo. MIDI input overrides with live triggers (primary octave = assigned articulation, alternate octave = articulation+1). Both modes coexist within the same render block.

**CIRCLE coupling:** Previous-block seat peak amplitudes create sympathetic ghost triggers in adjacent seats (circular topology), creating emergent ensemble interaction independent of MIDI input or the pattern sequencer.

---

### OWARE — 8-Voice Polyphonic

8 voices allocated by `VoiceAllocator` (oldest-voice stealing on overflow). All voices share the same instrument character — there is no fixed role assignment. Every MIDI note triggers a fresh voice at the corresponding pitch, with material, body type, and mallet hardness applied uniformly.

The engine is chromatic by nature: `owr_material` controls tonal character across the full keyboard range, not individual voice roles. Pan is derived from MIDI note number (note 60 = center, ±36-note range maps to full stereo field), so chords automatically spread across the stereo image.

**Voice carry-over:** When a note-off arrives, `ampLevel` is damped to 30% (not killed), and the filter envelope releases — simulating the natural decay of a muted bar rather than a hard cut.

---

## 3. Material Range

### Electronic → Acoustic → Metallic Spectrum

```
ELECTRONIC                        ACOUSTIC                         METALLIC / TUNED
    |                                  |                                 |
  ONSET                           OSTINATO                           OWARE
808/909 circuits                 World drums                    Marimba → Vibraphone
FM / Phase Distortion           Modal membranes                Bell → Gamelan
Karplus-Strong                  Waveguide bodies               Singing Bowl
LoFi / Bit-crush                Physical articulations         Sympathetic resonance
```

**ONSET** covers the full electronic drum spectrum: from the pure warmth of 808 analog topology through FM metallic complexity to Karplus-Strong percussive string sounds. The LoFi rack (bit-depth reduction + sample rate reduction) extends the range into degraded/industrial textures. The Blend axis can point a single voice simultaneously at both ends of the analog/digital divide.

**OSTINATO** covers the acoustic hand drum world exclusively, but does so with extraordinary breadth: West African goblet drums, Brazilian surdos, Indian tablas (with physically-accurate harmonic modes from Raman's 1934 research), Japanese taiko with thick-membrane mode compression, Middle Eastern doumbeks. The Beatbox entry (instrument 11) provides a bridge back to synthetic sounds within the physical modeling architecture. Decay times range from the sharp crack of a rim shot to the deep resonance of a large djembe bass tone.

**OWARE** covers tuned percussion instruments and spans a material continuum from highly inharmonic (wood bars with rapidly-decaying upper modes) through bell-like (near-harmonic partials, long sustain) to metal (all modes ring together) to bowl (sub-octave resonance, glass-like clarity). No other engine in the fleet provides continuous morphing between wood/metal/glass as a real-time synthesis parameter. The buzz membrane adds the roughness of balafon spider-silk to any point on the continuum.

---

## 4. Rhythmic Capability

### ONSET — Pure Triggered, No Sequencer

ONSET has no internal sequencer. It is a kit instrument that responds to external MIDI triggers. Rhythm comes from the DAW, from an MPC, from a human playing pads. The engine provides rhythmic intelligence only through XVC (Cross-Voice Coupling): the kit reacts to its own rhythmic density — kick hits affect snare color, snare hits affect hat decay. The "rhythm brain" effect is emergent from playing, not pre-programmed.

This makes ONSET the best choice when the producer controls the rhythm externally and wants precise individual voice control.

---

### OSTINATO — 16-Step Pattern Sequencer, 96 Archetypes

OSTINATO is the only engine in the percussion trio with a built-in autonomous sequencer. The pattern system is the engine's most distinctive rhythmic feature:

- **96 total patterns:** 8 patterns per instrument × 12 instruments
- Patterns are world-rhythm archetypes: West African 12/8 polyrhythm, Afro-Cuban clave derivations, Indian tala fragments, Brazilian samba/baião figures, Middle Eastern mizán patterns
- Each step stores velocity (0.0–1.0) and articulation index
- **GATHER macro** controls pattern density (steps at high velocity are skipped when GATHER is low) and humanization (timing displacement controlled by a hash-based humanizer)
- **FIRE macro** drives exciter energy, filter resonance, and compression simultaneously
- **CIRCLE macro** activates inter-seat ghost triggers — adjacent seats sympathetically respond to each other's peak levels

Live MIDI input coexists with the sequencer: notes 36–43 trigger seats directly, notes 48–55 trigger with alternate articulation. A producer can play fills over the running pattern, or jam patterns against live sequences.

**Tempo sync:** The sequencer responds to `osti_tempo` (BPM), `osti_swing`, and `osti_masterDecay` globals. Patterns can be stopped per seat by setting pattern volume to 0.

---

### OWARE — Pure Triggered, Sympathetic Resonance as Emergent Rhythm

OWARE has no sequencer. Like ONSET, it is a triggered instrument. However, the sympathetic resonance network creates a subtle form of rhythmic intelligence: when multiple notes are played rapidly, earlier-sounding voices whose modes overlap with new voices receive additional excitation — making dense rhythmic passages feel more cohesive and resonant than sparse ones.

The thermal drift adds organic timing feel at the synthesis level: because each voice's pitch drifts independently (with per-voice personality seeds), rapid phrase passages have natural micro-timing variation even when triggered at grid-perfect quantization.

---

## 5. Coupling Compatibility

### What Each Engine Exports for Coupling

| Engine | `getSampleForCoupling` output | Coupling buffer channels |
|--------|-------------------------------|--------------------------|
| ONSET | Post-FX stereo master output | 2 (L, R) |
| OSTINATO | Post-FX stereo + envelope follower | 3 (L, R, envelope) |
| OWARE | Post-filter stereo output | 2 (L, R via `couplingCacheL/R`) |

OSTINATO uniquely exports a 3-channel coupling buffer: the third channel carries a one-pole envelope follower (~10ms, `0.002` coefficient), giving downstream engines a smoothed loudness signal without the transient spikes of raw audio.

### What Each Engine Accepts as Coupling Input

**ONSET accepts:**
- `AmpToFilter` → `couplingFilterMod` (tone parameter offset for all voices)
- `EnvToDecay` → `couplingDecayMod` (decay shortening for all voices)
- `RhythmToBlend` → `couplingBlendMod` (Layer X/O blend shift for all voices)
- `AmpToChoke` → forces all voices to choke (hard mute)

**OSTINATO accepts:**
- `AmpToFilter` → `couplingFilterMod` (brightness offset for all seats)
- `EnvToDecay` → `couplingDecayMod` (decay multiplier for all seats)
- `RhythmToBlend` → pattern density modulation (externally adjustable via coupling)
- `AmpToChoke` → triggers ghost notes on a random seat (unusual: choke becomes a ghost trigger)
- `AudioToFM` → maps to `couplingFilterMod * 0.3f` (exciter brightness)

**OWARE accepts:**
- `AmpToFilter` → `couplingFilterMod` (added to effective brightness, ±2000 Hz)
- `LFOToPitch` → `couplingPitchMod` (pitch offset in semitones)
- `AmpToPitch` → `couplingPitchMod`
- `EnvToMorph` → `couplingMaterialMod` (material parameter shift)

### Strongest Coupling Pairs

**ONSET → OSTINATO (ONSET leads):** `AmpToChoke` routing creates the most literal "machine meets human" interaction in the fleet: each ONSET trigger fires a ghost note in a random OSTINATO seat, layering an organic acoustic membrane response under the electronic hit. The `RhythmToBlend` coupling additionally adjusts OSTINATO's pattern density in response to ONSET's rhythmic density. Use this for electronic beats with live acoustic texture.

**OWARE → ONSET (OWARE modulates ONSET):** OWARE exports its output audio; ONSET accepts `AmpToFilter` which modulates all voice tone parameters. Sustained OWARE resonances change ONSET's spectral color in real time. A long vibraphone chord brightens or darkens the kit textures as the chord evolves. Use this for melodic percussion lines that affect the kit character.

**OSTINATO → OWARE (OSTINATO rhythmically excites OWARE):** OSTINATO's envelope follower output (channel 2) → OWARE's `AmpToFilter` → shifts OWARE brightness with every drum hit. The drum pattern creates a rhythmic brightening pulse in the tuned percussion timbre. OSTINATO's `RhythmToBlend` output can also drive OWARE's `EnvToMorph`, making the material of the tuned instrument continuously shift with the drum rhythm.

**OWARE → OSTINATO (material → exciter):** OWARE's output → OSTINATO `AudioToFM` → shifts exciter brightness. Long resonating tones from OWARE continuously recolor the OSTINATO membrane excitation. Use for sustained metallic textures (gamelan, singing bowls) that permeate the rhythmic fabric.

**All three together:** The trio forms a complete signal graph. ONSET triggers OSTINATO ghost notes. OSTINATO's envelope follower drives OWARE's brightness. OWARE's output modulates ONSET's filter. A true feedback-free coupling loop across electronic, acoustic, and tuned percussion.

---

## 6. Use Case Matrix

### When to Use ONSET

**Primary use cases:**
- Electronic, hip-hop, trap, techno, house — any genre rooted in drum machine history
- When precise individual voice control is essential (independent EQ, pan, decay per voice)
- When the blend axis between analog warmth and digital complexity is a creative tool
- When XVC emergent kit behavior is desired (kick affecting snare, etc.)
- LoFi production: the bit-depth LoFi rack and tape delay are built into the FX chain
- Sound design: Karplus-Strong and Phase Distortion voices can produce non-drum textures (pitched plucks, metallic hits, glitchy transients)

**ONSET does not provide:**
- Pitched melodic percussion (no keyboard tracking for rhythmic pattern)
- World acoustic drum textures
- Pattern sequencing

**Best presets territory:** 808/909 kits, hybrid analog/digital kits, film percussion, glitch/industrial percussion, drum machine-adjacent sound design.

---

### When to Use OSTINATO

**Primary use cases:**
- World music production: West African, Afro-Cuban, Brazilian, Indian, Middle Eastern, Japanese rhythms
- Any situation requiring autonomous pattern generation (generative music, background layers, film score)
- Ensemble percussion layering: 8 seats with independent instruments creates a full hand-drum orchestra
- Live performance: MIDI triggers override or augment the running pattern; GATHER tightens or loosens the feel in real time
- Organic acoustic drum sounds without sample libraries
- Film/game audio where rhythmic texture needs to run without a sequencer host

**OSTINATO does not provide:**
- Electronic drum machine sounds (the Beatbox instrument is the closest approximation)
- Melodic/pitched percussion
- Per-voice blend between synthesis paradigms

**Best presets territory:** Drum circles, ceremonial rhythms, ambient tribal texture, world fusion beds, organic groove with CIRCLE coupling active.

---

### When to Use OWARE

**Primary use cases:**
- Tuned melodic percussion: any genre needing marimba, vibraphone, balafon, gamelan, bells
- Chromatic melodic lines played on percussion timbre (the instrument responds to MIDI pitch)
- Material morphing as a real-time expressive tool: wood → metal → glass in a single phrase
- Sympathetic resonance as texture: chords create sympathetically resonating networks
- Gamelan-style ensemble passages where beat-frequency shimmer is intrinsic to the sound
- Cinematic score: sustained metallic tones, bowl-like drones, bell clusters
- Sound design: the material continuum and body types produce textures unavailable in any sampled percussion library

**OWARE does not provide:**
- Un-pitched rhythmic percussion
- Pattern sequencing
- Electronic analog drum sounds

**Best presets territory:** Gamelan textures, balafon-inspired melodic lines, vibraphone jazz, metallic sci-fi percussion, bowl drones, pitched film score percussion.

---

## 7. Combined Ensemble Recipe: Complete Percussion Section

When all three engines are loaded simultaneously and coupled, they cover the full percussion space without overlap:

```
ONSET          = The Kit             (electronic rhythm backbone)
OSTINATO       = The Circle          (acoustic world rhythm texture)
OWARE          = The Melody          (tuned percussion color and resonance)
```

### Routing Setup

```
ONSET  → [AmpToChoke]     → OSTINATO   (amount: 0.3–0.6)
ONSET  → [AmpToFilter]    → OWARE      (amount: 0.2–0.4)
OSTINATO → [AmpToFilter]  → OWARE      (ch 2 envelope follower, amount: 0.15–0.3)
OWARE  → [AudioToFM]      → OSTINATO   (amount: 0.1–0.2)
```

**What this produces:**
- ONSET kick triggers a ghost response in a random OSTINATO seat — the electronic kick and an acoustic membrane speak together
- ONSET rhythm creates a brightening pulse in OWARE's timbre — the kit gives the tuned bars a rhythmic brightness envelope
- OSTINATO's drumming continuously recolors OWARE's output — the acoustic circle and the tuned board breathe together
- OWARE's sustained resonances seep into OSTINATO's excitation color — long metallic tones shade the drum membrane brightness

### Macro Settings for Full Ensemble

| Engine | Macro | Suggested Value | Effect |
|--------|-------|----------------|--------|
| ONSET | MACHINE | 0.4 | Lean slightly toward circuit warmth |
| ONSET | PUNCH | 0.5 | Neutral aggression |
| ONSET | SPACE | 0.3 | Light reverb presence |
| ONSET | MUTATE | 0.15–0.3 | Organic hit-to-hit variation |
| OSTINATO | GATHER | 0.5–0.7 | Medium-tight: some humanize with rhythmic lock |
| OSTINATO | FIRE | 0.4 | Moderate intensity; preserve dynamic range |
| OSTINATO | CIRCLE | 0.3–0.5 | Active ghost trigger network |
| OSTINATO | SPACE | 0.2–0.4 | Acoustic room without washing the kit |
| OWARE | MATERIAL (macro) | 0.0–0.4 | Stay in wood/bell range for warmth |
| OWARE | MALLET (macro) | 0.3–0.5 | Medium hardness — both low and high mode excitation |
| OWARE | COUPLING (macro) | 0.3 | Active sympathetic resonance |
| OWARE | SPACE (macro) | 0.2 | Body depth moderate |

### Recommended Starting Pattern (OSTINATO defaults)
The default instrument assignment gives you: Djembe (seat 1), Taiko (seat 5), Conga (seat 3), Tabla (seat 4), Cajón (seat 5), Doumbek (seat 6), Frame Drum (seat 7), Surdo (seat 8). Set all 8 seats to Pattern 1 (foundational) for a full world drum circle from a single preset slot.

### Layering Strategy

**Foundation layer:** ONSET at nominal mix level with modest SPACE macro. This is the rhythmic anchor.

**Texture layer:** OSTINATO at -3 to -6 dB below ONSET. GATHER at 0.6 for tight-ish feel. The circle provides acoustic warmth without competing with the kit's core transients.

**Melodic color layer:** OWARE responding to note input (melodic line or sparse chords). At -6 to -9 dB below ONSET. Long decay (2–5 seconds), material in the bell-to-metal range, sympathetic resonance active. OWARE's sustained tones create harmonic context that ONSET and OSTINATO play against.

**CIRCLE coupling depth:** Keep OSTINATO CIRCLE at 0.3–0.4. Above 0.6, the ghost trigger network becomes too dense and competes with the intentional ONSET hits. Below 0.2, the ensemble loses the emergent organic texture that distinguishes this trio from three independently-triggered engines.

---

## Appendix: Technical Summary

| Spec | ONSET | OSTINATO | OWARE |
|------|-------|----------|-------|
| Lines of code (approx.) | ~2000 | ~2100 | ~900 |
| Max voices | 8 (fixed) | 16 (8 seats × 2 sub-voices) | 8 (polyphonic) |
| Internal LFOs | Per-voice breathing LFO (0.08 Hz, hardcoded) | Per-seat `OstiBreathingLFO` (rate parameter-controlled, D005 PASS) | Per-voice shimmerLFO (rate hardcoded 0.3 Hz); engine-level LFO1/2 declared but dead (D004 block) |
| Envelope types | AD / AHD / ADSR per voice (shape parameter) | AD only per seat | Amplitude: exponential decay; Filter: ADSR via FilterEnvelope |
| Sequencer | None | 16-step per seat, 96 archetypes | None |
| FX chain | Character (grit+warmth) → Tape Delay → Schroeder Reverb → LoFi | Schroeder Reverb + VCA compressor | SVF per-voice only (no master FX) |
| Denormal protection | Yes (flushDenormal throughout) | Yes | Yes |
| Coupling output channels | 2 (stereo) | 3 (stereo + envelope follower) | 2 (cached L/R) |
| Coupling input types | AmpToFilter, EnvToDecay, RhythmToBlend, AmpToChoke | AmpToFilter, EnvToDecay, RhythmToBlend, AmpToChoke, AudioToFM | AmpToFilter, LFOToPitch, AmpToPitch, EnvToMorph |
| D003 compliance | Full (808/909 schematics + DX7 FM + Stanford modal cited) | Full (Bessel zeros, Raman 1934, JASA 2005, Fletcher & Rossing cited) | Exemplary (Chaigne 1997, Rossing 2000, Bilbao 2009, matched-Z filter coefficients) |
| Presets | 115 | Not specified in header | 20 (early) |
| Fleet seance status | Non-numeric: "Ahead of industry" (B002+B006) | 8.7/10 (highest numeric score) | New engine; D004 block pending |

---

*Generated from direct source code analysis of OnsetEngine.h, OstinatoEngine.h, OwareEngine.h.*
*Cross-referenced with: fleet-seance-scores-2026-03-20.md, architect-review-oware-2026-03-21.md, CLAUDE.md blessing/doctrine tables.*
