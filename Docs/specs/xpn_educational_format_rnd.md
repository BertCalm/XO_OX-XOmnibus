# Educational XPN Pack Formats — R&D

**Date**: 2026-03-16
**Status**: R&D / Concept Spec
**Author**: XO_OX Designs
**Related**: `xpn-tools.md`, `oxport_innovation_round5.md`, `kit_innovation_master_roadmap.md`

---

## The Core Insight

Every XPN pack ever sold is an answer. Here is the kick. Here is the snare. Here is the hat. Here is the final sound.

Nobody sells the question.

XO_OX can sell the question. What if a pack does not just deliver sounds — it teaches something? What if the producer learns as they play? An educational pack is not a novelty gimmick. It is a pedagogical philosophy embedded directly in a product format. The Deconstruction Pack, the A/B Test Pack, the Genre DNA Pack, the Progression Pack, and the Tutorial Mode XPN structure are five ways of making that question audible and actionable on MPC hardware.

The distinction matters because producers who understand *why* sounds work do not just use presets — they modify them, extend them, and eventually design from scratch. The educational pack creates fluency, not dependency.

No other pack studio does this. This is a format innovation.

---

## 1. Deconstruction Pack

### Concept

One famous (or archetypal) beat fully deconstructed across 4 pad banks. Each bank is not a different sound — it is the same sound at a different stage of production. Playing through Bank A → B → C → D is watching a production lesson unfold in real time on the MPC.

### Bank Structure

| Bank | Name | Content |
|------|------|---------|
| A | Isolated | 4 raw elements: kick, snare, hat, bass |
| B | Processed | The same 4 elements with subtle processing: compression, EQ |
| C | Treated | The same 4 elements heavily processed: saturation, pitch shifting, send effects |
| D | Variations | 4 variations on each element — what the producer might have tried before settling |

Bank A is the material. Bank B is craft. Bank C is character. Bank D is creative range.

### XPN / XPM Implementation

Each bank is a separate XPM program within the same XPN ZIP:

```
ONSET_Deconstruction_Vol1.xpn/
  Programs/
    bank_a_isolated.xpm        ← default load program
    bank_b_processed.xpm
    bank_c_treated.xpm
    bank_d_variations.xpm
  Samples/
    bank_a/
      kick_raw.wav
      snare_raw.wav
      hat_raw.wav
      bass_raw.wav
    bank_b/
      kick_compressed.wav
      snare_eq.wav
      hat_subtle_sat.wav
      bass_compressed.wav
    bank_c/
      kick_saturated.wav
      snare_pitched.wav
      hat_effect.wav
      bass_distorted.wav
    bank_d/
      kick_var1.wav  kick_var2.wav  kick_var3.wav  kick_var4.wav
      snare_var1.wav  ...
      hat_var1.wav  ...
      bass_var1.wav  ...
  Tutorial/
    tutorial.md
  expansion.json
```

### XPM Pad Metadata

Every pad in the XPM carries descriptive metadata in the `PadName` field:

```xml
<Pad number="0" name="kick_a_raw" rootNote="C3" bpmRange="85-95" suggestedUse="foundation" />
<Pad number="1" name="snare_a_raw" rootNote="C3" bpmRange="85-95" suggestedUse="foundation" />
<Pad number="4" name="kick_b_compressed" rootNote="C3" processingNote="4:1 comp, 2ms attack" />
```

The `processingNote` annotation in pad names (visible in MPC pad info) is the inline annotation that explains each design decision without requiring the producer to open the tutorial doc.

### `Tutorial/tutorial.md`

```markdown
# {BeatName} — Fully Deconstructed

## Bank A: The Raw Materials
Load `bank_a_isolated`. These are the 4 elements before any processing.
Hit each pad and listen to the unprocessed character. Notice what is
missing — no punch, no air, no glue. This is what the elements sound
like before production decisions are made.

**Kick**: A 55Hz sine body with a pitch sweep from 200Hz → 55Hz over 50ms.
**Snare**: White noise burst + a body tone, no compression, no transient shaping.
**Hat**: High-frequency noise, raw, no high-shelf cut.
**Bass**: Fundamental only, no harmonic saturation.

## Bank B: First Decisions
Load `bank_b_processed`. Same elements, subtle processing applied.
Hit each pad and compare to Bank A. The changes are intentional and minimal.

[continues for Banks C and D...]
```

### Oxport Tooling

**New tool required**: `xpn_deconstruction_builder.py`

```bash
python3 xpn_deconstruction_builder.py \
    --config deconstructions/beat_config.json \
    --wavs-dir /path/to/rendered/wavs \
    --output-dir ./build/deconstruction/
```

`beat_config.json` defines the 4-bank structure, pad labels, processing notes, and tutorial text tokens. The tool assembles 4 XPM files pointing to the correct WAV sets and writes the `Tutorial/tutorial.md` from the config's narration fields.

Integration: output is compatible with `xpn_packager.py` for final ZIP assembly.

### Production Time Estimate

| Task | Time |
|------|------|
| Design 4-bank structure + processing chain | 1–2 hrs |
| Render 16 WAVs (4 elements × 4 banks) | 20 min (automated) + 1 hr manual for treated variations |
| Write tutorial.md narration | 1–1.5 hrs |
| XPN assembly + QA | 30 min |
| **Total** | **~4–5 hours** |

---

## 2. A/B Test Pack

### Concept

Same sound, two versions. The producer learns to hear the difference. Every pad pair holds all variables constant except one. Playing Pad A then Pad B is a controlled experiment. After 8 pairs, the producer has trained their ear on 8 fundamental synthesis variables.

This is systematic ear training through production. The MPC is the laboratory. The pads are the apparatus.

### Pad Pair Structure (16 pads, 8 pairs)

| Pads | Variable | A State | B State | What to Listen For |
|------|----------|---------|---------|-------------------|
| 1–2 | Compression on/off | Uncompressed transient | Compressed (4:1, 2ms attack, 100ms release) | Punch arriving, dynamics tightening |
| 3–4 | Stereo width | Mono (center) | Wide stereo (M/S processed) | Space opening around the sound |
| 5–6 | Filter brightness | Dark (cutoff at 2kHz) | Bright (cutoff at 8kHz) | Air appearing in the top end |
| 7–8 | Attack time | Hard attack (1ms) | Soft attack (40ms) | Punch vs. swell — the moment of entry |
| 9–10 | Saturation amount | Clean (drive = 0) | Saturated (drive = 0.7) | Harmonic density, weight increasing |
| 11–12 | Reverb length | Dry (no reverb) | Wet (RT60 = 1.8s) | Room scale appearing around the sound |
| 13–14 | Velocity sensitivity | Flat (all velocities identical) | Expressive (velocity → filter + amp) | Expressive range vs. consistency |
| 15–16 | Contextual fit | Correct context (90 BPM hip-hop character) | Wrong context (cinematic orchestral character) | Why context shapes sound choice |

### Pair 15–16: The Right vs. Wrong Pair

This pair is the most unusual and the most pedagogically valuable. Pad 15 is not "better" than Pad 16 in absolute terms. Pad 16 may be a beautiful sound. But Pad 16 carries energy that belongs to a different genre grammar. A producer who can articulate *why* it feels wrong — not just that it feels wrong — has developed contextual ear that most producers never name.

The liner notes for this pair: "Your first instinct is probably 'this doesn't fit' before you can say why. That instinct is your context ear working. This pair teaches you to name it."

### XPN / XPM Implementation

16 pads in a single XPM. Pad naming convention:

```
{PairNumber}{Side}_{VariableName}
```

Examples:
```
1A_Uncompressed
1B_Compressed
7A_HardAttack
7B_SoftAttack
15A_CorrectContext
15B_WrongContext
```

The `A`/`B` side is visible in the MPC pad name display. The variable name tells the producer what they are testing before they hit the pad.

### ZIP Structure

```
XO_OX_AB_Fundamentals_Vol1.xpn/
  Programs/
    ab_fundamentals.xpm        ← 16 pads
  Samples/
    pair_01_uncompressed.wav   pair_01_compressed.wav
    pair_02_mono.wav           pair_02_stereo.wav
    pair_03_dark.wav           pair_03_bright.wav
    pair_04_hard_attack.wav    pair_04_soft_attack.wav
    pair_05_clean.wav          pair_05_saturated.wav
    pair_06_dry.wav            pair_06_wet.wav
    pair_07_flat_vel.wav       pair_07_expressive_vel.wav
    pair_08_correct_ctx.wav    pair_08_wrong_ctx.wav
  Tutorial/
    ab_listening_guide.md
  expansion.json
```

### Velocity Sensitivity Demonstration (Pair 13–14)

Pair 14 (expressive velocity) should be a single WAV per velocity zone, not a flat sample. The XPM maps 4 velocity zones to 4 samples on the same pad:

```xml
<Layer sample="pair_07_expressive_pp.wav" velStart="0"   velEnd="31"  />
<Layer sample="pair_07_expressive_mp.wav" velStart="32"  velEnd="63"  />
<Layer sample="pair_07_expressive_mf.wav" velStart="64"  velEnd="95"  />
<Layer sample="pair_07_expressive_ff.wav" velStart="96"  velEnd="127" />
```

The producer plays the pad at different velocities and hears that velocity is not volume — it is timbre. Pad 13 (flat velocity) uses the same sample at all velocities to demonstrate the contrast.

### `Tutorial/ab_listening_guide.md`

One section per pair. Each section: the variable name, the A state, the B state, a one-sentence listening instruction, and a follow-up question ("Could you hear the difference without the label? Play A and B 3 times each with your eyes closed.").

### Production Time Estimate

| Task | Time |
|------|------|
| Design 8 pairs with controlled parameter sets | 2 hrs |
| Render 16 WAVs (+ 4 velocity layers for Pair 14) | 20 min |
| Write listening guide for 8 pairs | 1.5 hrs |
| XPN assembly + QA | 30 min |
| **Total** | **~4.5 hours** |

---

## 3. Genre DNA Pack

### Concept

16 pads. Each pad is the minimal viable beat of a different subgenre. Not a finished beat — the kernel. 2–3 samples demonstrating what makes that subgenre structurally distinctive. Play all 16 pads and you have heard the rhythmic DNA of 16 different musical traditions in one session.

Inside the ZIP: `genre_notes.md` — 16 paragraphs of genre history. One paragraph per pad. The producer who wants to know *why* Pad 7 sounds the way it does can read the paragraph. The genre notes are not required to use the pack, but they are there.

### 16 Subgenre Kernels

| Pad | Subgenre | Kernel Elements | Defining Characteristic |
|-----|----------|----------------|------------------------|
| 1 | UK Garage | 2-step kick pattern + skipping snare | Syncopated 2-step grid, kick displaced from downbeat |
| 2 | Afrobeats | Clave-derived hi-hat grid + talking drum accent | Layered polyrhythm, conversation between hat and accent |
| 3 | UK Drill | Sliding 808, hi-hat triplet drill | 808 pitch slides + dense hi-hat subdivisions |
| 4 | Boom-Bap | Punchy kick + loud snare + classic hat | Kick/snare emphasis, minimal hat, open texture |
| 5 | Footwork | 160 BPM rapid-fire kick + stutter | High BPM, rapid kick permutations |
| 6 | Dancehall | One-drop pattern + rim shot accent | Kick on 3, snare absence on 1 and 3 |
| 7 | Jersey Club | 4×4 kick grid + pitched sample chop | Dense floor kick, vocal chop percussion |
| 8 | Baile Funk | Heavy baile pattern + tamborzão | Distorted kick, tamborzão snare/hat hybrid |
| 9 | Gqom | Repetitive industrial kick + clap | Minimal, hypnotic, kick-heavy |
| 10 | Lo-fi Hip-Hop | Boom-bap with vinyl noise + swing | Quantize swing, noise floor, slow tempo |
| 11 | Juke | 160 BPM permutation base | Similar to Footwork but distinct rhythmic vocabulary |
| 12 | Cumbia | Güiro + caja drum + bass tumbao | Güiro-to-caja relationship defines cumbia grid |
| 13 | Amapiano | Log drum + jazz piano chord stab | Log drum is the genus-defining element |
| 14 | New Jack Swing | Quantized swing beat + synth bass | Hard quantize + swing offset + funk bass |
| 15 | Trap | Hi-hat triplet rolls + 808 sub | Hi-hat subdivision speed + 808 sustain length |
| 16 | Broken Beat | Broken bar phrasing + jazz snare | Bar-length phrases broken across the grid |

### XPN / XPM Implementation

Single XPM with 16 pads. Each pad has 2–3 samples (layered to fire simultaneously on pad hit, creating the kernel groove as a one-shot):

```xml
<!-- Pad 1: UK Garage kernel -->
<Pad number="0" name="01_UKGarage_Kernel" bpmRange="130-140" key="groove">
  <Layer sample="garage_2step_kick.wav" velStart="0" velEnd="127" />
  <Layer sample="garage_skip_snare.wav" velStart="0" velEnd="127" />
  <Layer sample="garage_hat.wav"        velStart="0" velEnd="127" />
</Pad>
```

The samples fire simultaneously on a single pad hit. The producer hears the kernel as a single pad.

### `genre_notes.md` Inside ZIP

```markdown
# Genre DNA Pack — Field Notes

## Pad 1: UK Garage (130–138 BPM)

UK Garage emerged from London in the early 1990s, a synthesis of US House
with the rhythmic sensibility of jungle and drum and bass. The defining
structural feature is the 2-step pattern: the kick drum is displaced from
the downbeat and placed on unexpected grid positions, while the snare
"skips" in ways that feel propulsive rather than settled. The swung
sixteenth-note hi-hat grid ties the elements together...

[16 paragraphs total, one per subgenre]
```

### ZIP Structure

```
XO_OX_GenreDNA_Vol1.xpn/
  Programs/
    genre_dna_16pads.xpm
  Samples/
    genre_01_garage/
      garage_2step_kick.wav
      garage_skip_snare.wav
      garage_hat.wav
    genre_02_afrobeats/
      afrobeats_clave_hat.wav
      afrobeats_talking_drum.wav
    [14 more genre folders...]
  Tutorial/
    genre_notes.md
  expansion.json
```

### Production Time Estimate

| Task | Time |
|------|------|
| Research + design 16 genre kernels | 2–3 hrs |
| Source/render 32–48 kernel samples (2–3 per genre) | 3–4 hrs |
| Write 16 genre paragraphs in genre_notes.md | 2–3 hrs |
| XPN assembly + QA listen pass (all 16 pads) | 1 hr |
| **Total** | **~9–11 hours** |

---

## 4. Progression Pack

### Concept

A kit that teaches how to build a track section by section. 16 pads divided into 4 groups of 4. Each group represents one section of a track's arrangement. The producer pads through the structure and learns arrangement by experiencing it as a physical sequence.

The insight: arrangement is one of the hardest production skills to teach in the abstract. Progression Pack makes it tactile.

### Pad Group Structure

| Pads | Section | Elements | Energy Level |
|------|---------|----------|-------------|
| 1–4 | Intro | Kick only / sparse / 1-2 elements | Minimal — set context, do not resolve |
| 5–8 | Verse | Core groove established / 3-4 elements | Foundation — consistent, not climactic |
| 9–12 | Pre-Chorus | Energy building / filter opening / layers adding | Rising — tension without release |
| 13–16 | Chorus | Full arrangement / all elements / maximum density | Payoff — everything present |

Within each group of 4, each pad represents one layer adding to the section:
- Pad 1 (Intro): kick alone
- Pad 2 (Intro): kick + bass note
- Pad 3 (Intro): kick + bass + sparse pad
- Pad 4 (Intro): full intro — all 3 elements, still minimal

Playing pad 4 → pad 8 → pad 12 → pad 16 is a miniature arrangement arc from intro to chorus.

### XPN / XPM Implementation

Single XPM, 16 pads in 4 banks (or 4 pad groups). Pad naming:

```
{SectionNum}_{SectionName}_{LayerNum}_{Description}
```

Examples:
```xml
<Pad number="0"  name="1_Intro_L1_KickOnly"     />
<Pad number="1"  name="1_Intro_L2_KickBass"      />
<Pad number="2"  name="1_Intro_L3_KickBassPad"   />
<Pad number="3"  name="1_Intro_L4_Full"          />
<Pad number="4"  name="2_Verse_L1_GrooveBase"    />
...
<Pad number="15" name="4_Chorus_L4_FullArrangement" />
```

Each pad is a multi-layer sample composite (all elements mixed to a single WAV, rendered at the appropriate density for that section/layer).

### `Tutorial/progression_guide.md`

One section per pad group (4 sections). Each section explains what makes that arrangement section structurally effective:

```markdown
## Section 1: The Intro (Pads 1–4)

An intro has one job: establish context without resolving it. The listener
should hear the tempo, feel the key, and sense the genre — but not yet
receive the full payoff of the groove.

Load pad 1. This is a single kick. Nothing else. Notice how much space
there is. Space in an intro is not emptiness — it is anticipation.

Load pad 2. The bass note arrives. The key is established. The intro now
has two anchor points: time (kick) and pitch (bass). Add nothing else yet.

[continues through all 4 sections...]
```

### ZIP Structure

```
XO_OX_Progression_Vol1.xpn/
  Programs/
    progression_16pads.xpm
  Samples/
    intro/
      intro_l1_kick.wav
      intro_l2_kick_bass.wav
      intro_l3_kick_bass_pad.wav
      intro_l4_full.wav
    verse/
      [4 WAVs]
    prechorus/
      [4 WAVs]
    chorus/
      [4 WAVs]
  Tutorial/
    progression_guide.md
  expansion.json
```

### Production Time Estimate

| Task | Time |
|------|------|
| Design 4-section arrangement arc + 16 layer builds | 2 hrs |
| Render 16 composite WAVs (each is a mix of multiple elements) | 1–2 hrs |
| Write progression_guide.md (4 sections) | 1.5 hrs |
| XPN assembly + QA | 30 min |
| **Total** | **~5–6 hours** |

---

## 5. Tutorial Mode XPN

### Concept

A format innovation, not just a pack variant. Tutorial Mode XPN extends the standard XPN ZIP structure with a `Tutorial/` directory that lives inside the pack alongside the standard `Programs/` and `Samples/` directories. The pack teaches by its structure — no companion website, no video, no external material required.

A producer who receives this pack without reading any documentation should be able to:
1. Load it on the MPC
2. See that there are multiple programs (Step programs + Final)
3. Navigate through them in order
4. Hear the sound build stage by stage

The `Tutorial/tutorial.md` is supplementary depth, not a requirement for basic understanding.

**No other pack studio does this.** This is a genuine format innovation.

### Standard vs. Tutorial XPN Directory Layout

Standard XPN:
```
PackName.xpn/
  Programs/
    KitName.xpm
  Samples/
    sample_01.wav
  expansion.json
```

Tutorial Mode XPN:
```
PackName_Tutorial.xpn/
  Programs/
    KitName_Final.xpm          ← default load (finished product)
    KitName_Step1.xpm          ← oscillator only
    KitName_Step2.xpm          ← + pitch
    KitName_Step3.xpm          ← + envelope
    KitName_Step4.xpm          ← + filter
    KitName_Step5.xpm          ← + saturation
    KitName_Step6.xpm          ← + space
    KitName_Step7.xpm          ← + coupling
    KitName_Step8.xpm          ← = Final (redundant but explicit)
  Samples/
    step1_base.wav
    step2_pitched.wav
    step3_envelope.wav
    step4_filter.wav
    step5_saturation.wav
    step6_space.wav
    step7_coupled.wav
    step8_final.wav
    [production samples for Final program]
  Tutorial/
    tutorial.md
    tutorial_audio_notes.txt
    step_parameters.json
  expansion.json
  liner_notes.json
```

The MPC loads `KitName_Final.xpm` by default — the producer gets the finished product immediately. The Step programs are available as alternate programs in the Programs menu.

### `Tutorial/tutorial.md`

Step-by-step text guide referencing programs by name:

```markdown
# {KitName} — How This Sound Was Built

## What You Are Looking At

This pack contains the finished {KitName} preset and 7 intermediate steps
showing how it was constructed. Each step adds exactly one signal chain stage.
Load steps in order. Listen for what changes between each step.

## Step 1: Load KitName_Step1

This is just the oscillator. No envelope, no filter, no character.
Hit the pad and listen to the raw frequency. This is the material.

**What you are hearing**: A sine oscillator at 55Hz. Nothing else.
**Parameter in focus**: `perc_kickOscFreq` = 55.0

## Step 2: Load KitName_Step2

The oscillator now has pitch identity — a fast downward sweep from 200Hz
to 55Hz over 50ms. This is what makes a kick sound like a kick rather
than a sine burst.

**What you are hearing**: Pitch envelope. The downward sweep is new.
**Parameters added**: `perc_kickPitchEnvAmount` = 0.8, `perc_kickPitchEnvDecay` = 0.05s

[steps 3–8 continue in this format]
```

### `Tutorial/tutorial_audio_notes.txt`

Terse listening instructions for producers who prefer to skip prose:

```
STEP 1 → STEP 2: Hear the pitch sweep arrive. The "boing" is new.
STEP 2 → STEP 3: Hear the body appear. The attack gets punch.
STEP 3 → STEP 4: Hear the top end close down. Filter is shaping character.
STEP 4 → STEP 5: Hear the weight increase. Saturation adds harmonic density.
STEP 5 → STEP 6: Hear the room appear. The sound gets larger.
STEP 6 → STEP 7: Load two engines, play coupled. Hear the frequency interaction.
STEP 7 → STEP 8: They are identical. You built the finished preset.
```

### `Tutorial/step_parameters.json`

Machine-readable record of every parameter change at each step:

```json
{
  "pack_name": "ONSET_Kick_Foundation_Tutorial",
  "engine": "ONSET",
  "voice": "kick",
  "base_params": {
    "perc_kickOscFreq": 55.0,
    "perc_kickEnvDecay": 0.0,
    "perc_kickFilterCutoff": 22000.0,
    "perc_kickDrive": 0.0
  },
  "steps": [
    { "step": 1, "label": "Base",      "params_added": {} },
    { "step": 2, "label": "+Pitch",    "params_added": { "perc_kickPitchEnvAmount": 0.8, "perc_kickPitchEnvDecay": 0.05 } },
    { "step": 3, "label": "+Envelope", "params_added": { "perc_kickEnvDecay": 0.35, "perc_kickEnvCurve": 0.7 } },
    { "step": 4, "label": "+Filter",   "params_added": { "perc_kickFilterCutoff": 4000.0, "perc_kickFilterEnvAmt": 0.6 } },
    { "step": 5, "label": "+Drive",    "params_added": { "perc_kickDrive": 0.4 } },
    { "step": 6, "label": "+Space",    "params_added": { "perc_kickReverbSend": 0.25, "perc_kickReverbTime": 0.6 } },
    { "step": 7, "label": "+Coupling", "params_added": { "couplingRoute": "ONSET→OPAL", "couplingDepth": 0.5 } },
    { "step": 8, "label": "Final",     "params_added": {} }
  ]
}
```

This file is consumed by `xpn_tutorial_builder.py` for regeneration, by the eBook export pipeline for cross-referencing, and by future website interactive tools.

### Oxport Tooling

**New tool required**: `xpn_tutorial_builder.py`

```bash
python3 xpn_tutorial_builder.py \
    --steps steps/kick_chain.json \
    --narration narration/kick_tutorial.md \
    --wavs-dir /path/to/rendered/step_wavs \
    --output-dir ./build/tutorial/
```

The `narration/kick_tutorial.md` is the human-authored tutorial text. The tool injects parameter tables from `step_parameters.json` automatically at `<!-- PARAMS:stepN -->` placeholder tokens in the narration file. Output `Tutorial/tutorial.md` is the merged document: human narration + auto-generated parameter tables.

The tool produces the full Tutorial Mode XPN directory structure — all Step XPMs, `Tutorial/` directory contents, `expansion.json`, `liner_notes.json` — and hands off to `xpn_packager.py` for final ZIP assembly.

### Tutorial Mode as Pack Extension (Not Replacement)

Tutorial Mode is attached to existing major pack releases as an optional variant, not a separate product. When a new engine spotlight pack ships, a Tutorial Mode version ships alongside it:

- `ONSET_Drum_Essentials_v1.0.zip` — standard production kit
- `ONSET_Drum_Essentials_Tutorial_v1.0.zip` — same kit + Tutorial directory + Step programs

The Tutorial version is free to owners of the standard version (Patreon benefit) or bundled at the same price. It costs ~2–3 additional production hours beyond the base pack.

### Production Time Estimate (Tutorial Mode add-on)

| Task | Time (over base pack) |
|------|----------------------|
| Design step-by-step parameter chain | 1 hr |
| Render step WAVs (one per step, automated with fleet render) | 15 min |
| Write narration text for tutorial.md | 1–1.5 hrs |
| Write tutorial_audio_notes.txt | 15 min |
| Build with xpn_tutorial_builder.py + QA | 30 min |
| **Total add-on time** | **~3–3.5 hours** |

---

## Format Summary

| Format | Pads | Teach | Production Hours | Price | Distribution |
|--------|------|-------|-----------------|-------|-------------|
| Deconstruction Pack | 16 (4 banks × 4) | How a sound was built | 4–5 hrs | $9.99 | Free or Patreon |
| A/B Test Pack | 16 (8 pairs) | Ear training: one variable per pair | 4.5 hrs | Free / Patreon | Free (basic), Patreon (advanced) |
| Genre DNA Pack | 16 (1 per genre) | Genre grammar via minimal kernels | 9–11 hrs | $12.99 | Paid — educational premium |
| Progression Pack | 16 (4 sections × 4) | Track arrangement by section | 5–6 hrs | $9.99 | Paid or Patreon |
| Tutorial Mode XPN | Base + 8 step programs | Signal chain construction | +3–3.5 hrs | Bundle / free to owners | Paired with major pack launches |

---

## Implementation Priority

1. **Tutorial Mode XPN** — attach to the May 2026 OddfeliX+OddOscar launch. Highest visibility, demonstrates format innovation on the flagship pack. Requires `xpn_tutorial_builder.py`.

2. **A/B Test Pack** — free release alongside the first Field Guide post that covers synthesis fundamentals. Zero revenue cost, maximum discovery value.

3. **Deconstruction Pack** — free release alongside ONSET Drum Essentials. The ONSET signal chain (8 voices, clear construction stages) is the best possible deconstruction candidate.

4. **Genre DNA Pack** — paid release in Q3 2026. Requires the most research investment; the `genre_notes.md` content is also reusable as Field Guide post material.

5. **Progression Pack** — paid or Patreon release Q4 2026. Pairs with a Field Guide post on arrangement.

---

## New Oxport Tools Required

| Tool | Purpose | Integrates With |
|------|---------|----------------|
| `xpn_deconstruction_builder.py` | Assembles 4-bank deconstruction XPN from WAVs + config | `xpn_packager.py`, `xpn_liner_notes.py` |
| `xpn_tutorial_builder.py` | Builds Tutorial Mode XPN with step programs + Tutorial/ directory | `xpn_packager.py`, eBook export pipeline |

Both tools follow the existing Oxport pipeline conventions: JSON config input, WAV directory input, output directory compatible with `xpn_packager.py` for final ZIP assembly.

---

*Cross-reference: [xpn-tools.md](../xpn-tools.md) | [oxport_innovation_round5.md](oxport_innovation_round5.md) | [kit_innovation_master_roadmap.md](kit_innovation_master_roadmap.md) | [kit_curation_innovation_rnd.md](kit_curation_innovation_rnd.md)*
