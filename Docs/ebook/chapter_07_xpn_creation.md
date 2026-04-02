# Chapter 7: Creating Your Own XPN Packs with Oxport

*Written by Rex, XPN Format Specialist, XO_OX Designs*

---

The XPN format is more documented than it appears, and less complicated than it looks. Most of the complexity exists to support edge cases — multi-articulation instrument libraries, 16-layer velocity stacks, round-robin cycle groups across 88 notes — that you will not need for your first pack. The core format, the part you need to produce a functional, playable pack from scratch, reduces to about a dozen fields and three rules. Learn those first. The rest is refinement.

This chapter is written for pack designers, not consumers. If you want to understand what you are loading onto your MPC, Chapter 1 covers the format from the reading side. If you want to build your own packs — from your own samples, your own XOceanus renders, or your own field recordings — this chapter walks the full path from raw audio files to a validated `.xpn` bundle that loads cleanly on hardware.

I am going to be precise about the things that matter and not waste your time on the things that do not. When I say a rule is critical, it is because violating it produces a failure mode I have personally diagnosed more than once. The failures are repeatable, and so are the fixes.

---

## 7.1 Who This Chapter Is For

**You want to build packs, not just consume them.** That might mean:

- You have rendered audio from XOceanus and want to package it for MPC use
- You have a sample library you have recorded or collected and want to organize it as a playable instrument
- You are a sound designer who wants to sell or share custom packs in the Seed+Grow community program
- You are a producer who wants to package a signature set of sounds so you can recall your exact tone on any MPC

This chapter assumes you can run Python scripts from the command line and that you have access to the Oxport toolchain (available from the XO_OX repository at `Tools/`). It assumes you are working on macOS, but the tools are platform-agnostic. It does not assume you know anything about the XPM schema — you will learn what you need as we go.

What this chapter does not cover: the full XPM schema reference (see Appendix A), advanced multi-articulation keygroup design (see Chapter 7 of the outline, which covers that territory in depth), and the complete Oxport tool suite walkthrough (see the tools chapter). This chapter covers the minimum required path for building a first pack well.

> **Tip:** Read this chapter once before you start building. The Three Golden Rules in Section 7.5 contain failure modes that are invisible until you load the pack on hardware. You do not want to discover a ghost-triggering bug after you have spent two days building a 24-program pack. Learn the rules first, then build.

---

## 7.2 The Oxport Pipeline Overview

Oxport is a suite of Python tools living in `Tools/` at the root of the XOceanus repository. The main entry point is:

```bash
python3 Tools/oxport.py run --config my_pack_config.json
```

When you call `oxport.py run`, the pipeline executes in eight ordered stages:

**Stage 1: Render Spec Generation.** Oxport reads your config JSON and produces a render specification: which root notes to render, how many velocity layers, what sample rate, whether loop points should be detected or authored. If you are working from pre-rendered samples rather than live XOceanus renders, this stage is effectively a validation pass — it confirms that your existing samples match the spec before anything is built.

**Stage 2: Sample Categorization.** Your raw audio files are organized into the canonical folder structure (`/Samples/{program_name}/{velocity_zone}/`) and renamed to the XO_OX naming convention (`{ProgramName}_{RootNote}_{VelMin}_{VelMax}.wav`). If your files are already organized, this stage confirms the organization and reports any missing zones or inconsistent naming.

**Stage 3: DNA Badge Assignment.** Each program receives a 6D Sonic DNA badge computed from the engine parameters or, for non-XOceanus samples, inferred from acoustic analysis. You can override computed values with explicit values in the config. This stage writes a `.xometa` companion file for each program — this file follows the program into the manifest and is used by the pack browser for filtering.

**Stage 4: XPM Generation.** For each program in your config, Oxport generates the XML `.xpm` file. This is the most format-sensitive stage. The generator handles key zone calculation, velocity layer stacking, envelope defaults, LFO stubs, FX chain stubs, and Q-Link assignments. Most of the Three Golden Rules (Section 7.5) are enforced here — if your config violates them, this stage will warn you.

**Stage 5: Manifest Assembly.** Oxport assembles the `manifest.json` from your config metadata plus computed values (program list, sample inventory, schemaVersion, targetHardware). This stage also validates that every sample referenced in every XPM exists in the `/Samples/` folder.

**Stage 6: Cover Art Generation.** If you have specified an engine accent color in the config, Oxport generates cover art at 1200×1200 (master) and 1000×1000 (XPN embed). You can supply your own art instead — in that case, this stage performs dimension and format validation only.

**Stage 7: Bundle Build.** All XPM files, samples, manifest, and cover art are assembled into a `.xpn` ZIP archive. The archive structure is validated against the XPN spec before the ZIP is created.

**Stage 8: Validation Report.** The completed bundle is tested against a schema validator that checks for common format errors, missing fields, and hardware-specific issues (OLED label lengths, sample rate consistency, cover art dimensions). The validator produces a pass/fail report with specific error messages for any issues found.

```bash
# Minimal invocation
python3 Tools/oxport.py run --config my_pack_config.json

# With explicit output path and validation report
python3 Tools/oxport.py run \
  --config my_pack_config.json \
  --output ./dist/MyPack_v1.0.0.xpn \
  --report ./dist/validation_report.txt

# Dry run — validate config and samples without building
python3 Tools/oxport.py run \
  --config my_pack_config.json \
  --dry-run
```

> **Tip:** Always run a `--dry-run` first on a new pack config. The dry run catches missing samples, invalid root notes, and schema violations before the pipeline starts writing files. Discovering these errors at stage 4 is much less irritating than discovering them at stage 8 after the pipeline has done the work.

---

## 7.3 Your First Pack in 15 Minutes

This is the minimum viable path. You will have a validated `.xpn` file at the end that loads on MPC hardware.

**What you need to start:**
- A folder of WAV files (24-bit, any sample rate; Oxport resamples to 44100 or 48000 as needed)
- Python 3.9+ installed
- The Oxport toolchain from `Tools/`

**Step 1: Organize your samples.**

Create a source folder. For a drum pack, name subfolders by voice: `kick/`, `snare/`, `hat_closed/`, `hat_open/`, `clap/`, `tom/`, `perc/`, `fx/`. For a keygroup pack, name subfolders by velocity zone if you have multiple layers: `v001-v032/`, `v033-v064/`, `v065-v096/`, `v097-v127/`. Single-velocity keygroup packs (common for first attempts) can use a flat structure with all samples in one folder.

For keygroup samples, include the root note in the filename. Oxport reads these automatically:

```
MyBass_C2.wav
MyBass_F2.wav
MyBass_A#2.wav
MyBass_D3.wav
```

Supported root note formats: `C2`, `C#2`, `Db2`, `A#2`, `Bb2`. Flats and sharps are both accepted.

**Step 2: Create your config JSON.**

```json
{
  "pack": {
    "name": "My First Pack",
    "vendor": "Your Name",
    "version": "1.0.0",
    "description": "A first XPN pack built with Oxport.",
    "genreTags": ["Electronic", "Hip Hop"],
    "moodTags": ["Foundation"],
    "targetHardware": ["MPC Live II", "MPC One", "MPC X"]
  },
  "programs": [
    {
      "name": "Bass Character",
      "type": "keygroup",
      "samples_dir": "./samples/my_bass/",
      "root_note_from_filename": true,
      "velocity_layers": 4,
      "q_links": [
        {"target": "filter_cutoff", "label": "TONE", "bipolar": false},
        {"target": "reverb_send",   "label": "SPACE", "bipolar": false},
        {"target": "lfo1_rate",     "label": "MOVE",  "bipolar": false},
        {"target": "filter_res",    "label": "EDGE",  "bipolar": false}
      ]
    }
  ],
  "cover_art": {
    "accent_color": "#E9A84A",
    "engine_name": "MY ENGINE"
  }
}
```

**Step 3: Run Oxport.**

```bash
cd /path/to/XO_OX-XOceanus
python3 Tools/oxport.py run --config ./my_pack_config.json --output ./dist/MyFirstPack_v1.0.0.xpn
```

**Step 4: Load and test on MPC hardware.**

Copy the `.xpn` file to your MPC SD card. On MPC: **Browse → Expansions → [your pack name]**. Select a program. Verify that all pads or keys respond, that Q-Links produce audible change, and that velocity produces timbral (not just amplitude) variation.

That is the complete minimal path. The remainder of this chapter covers how to build packs that are not just functional but genuinely expressive.

---

## 7.4 Drum Pack vs. Keygroup Pack Design Decisions

Before you open the config JSON, know what you are building. The design decisions are different enough that conflating them produces worse results in both formats.

**Drum Pack Design Logic:**

A drum pack program maps to 16 pads on the MPC. You are designing a kit — a coherent collection of sounds that function together rhythmically. The primary design decisions are:

- **Voice coverage.** All eight ONSET voices (Kick, Snare, CHat, OHat, Clap, Tom, Perc, FX) plus up to eight additional variant pads. The most common variant strategy: two kick variants (soft and heavy), two snare variants (snare and rim), two hat variants (loose and tight), leaving twelve pads for the remaining voices.
- **Character cohesion.** All voices in a kit should share an acoustic world. An electronic kick with an acoustic snare and a lo-fi hat creates friction that makes mixing harder. This does not mean all sounds must be from the same era — it means they must be from the same imaginary physical space.
- **Round-robin depth.** Kick and snare benefit most from round-robin cycling (4–8 cycles prevents the machine-gun effect on repeated hits). Hats benefit from velocity layers more than round-robin. FX and Perc can be single-sample if the transient character is strong.
- **Q-Link macro design.** ONSET's standard macros translate directly: MACHINE (circuit character — the underlying synthesis quality), PUNCH (transient shape — attack time and impact), SPACE (reverb depth — how far back in the room), MUTATE (generative variation — stochastic layer randomization). Keep this standard unless you have a compelling reason to diverge. It is the convention that MPC producers expect.

**Keygroup Pack Design Logic:**

A keygroup pack is a pitched instrument that plays across the full MIDI keyboard range. The primary design decisions are:

- **Root note density.** How many sampled root notes per octave? For most timbres, four root notes per octave (every three semitones) is sufficient. For formant-sensitive content (vocal textures, ORACLE's stochastic programs, OBBLIGATO's wind synthesis), six root notes per octave prevents pitch artifacts in the stretched zones.
- **Velocity layer count.** See Section 7.6 for the full strategy. The minimum for a usable keygroup pack is four layers. Eight layers is the standard for expressive programs. Sixteen layers is reserved for the most dynamically complex programs (OBLONG at maximum Mojo, OPAL at maximum grain density).
- **Register coverage.** The full MIDI range is C-2 to G8. Not all instruments need all of it. Bass programs typically stop at C5 or C6. Atmospheric pads often skip the bottom three octaves. Design for the register where the program is actually useful, and let MPC's pitch-stretch handle the extremes — it will not sound great at +/-2 octaves, but it will not break the pack.
- **Q-Link design.** Keygroup Q-Links should reflect the instrument's expressive identity. See Section 7.7 for the XO_OX standard.

> **Tip:** The single most common mistake in a first keygroup pack is setting all root notes to C4 and relying on MPC's pitch-stretch to cover the full range. This works passably for simple sine-wave timbres and fails audibly for anything with complex harmonic content, formants, or time-domain characteristics. Render your own root notes at minimum intervals for any program you care about.

---

## 7.5 The Three Golden Rules

These three rules exist in XO_OX documentation, the Appendix B checklist, and now here — because they are the three most common sources of silent pack failures. Violate any of them and the pack will load without an error message but will behave incorrectly.

---

**Rule 1: KeyTrack = True**

```xml
<Keygroup keytrack="True">
```

`KeyTrack = True` instructs the MPC to transpose samples up and down from their root note as keys are played. `KeyTrack = False` means every key plays the sample at the same pitch — which is correct for a drum program (you want the kick to sound the same regardless of which pad triggers it) but catastrophically wrong for a keygroup program (every note will sound exactly like C4 regardless of what you play).

The failure mode: a keygroup pack with `KeyTrack = False` loads, all keys respond, the waveform plays back, nothing errors — but the instrument is monopitch. Debugging this from the MPC side is genuinely confusing because the program technically works.

Oxport sets `KeyTrack = True` by default for all keygroup programs and `KeyTrack = False` for drum programs. If you are hand-editing XPM files, double-check this field first.

---

**Rule 2: RootNote = 0**

```xml
<Layer rootNote="0" ... />
```

`RootNote = 0` tells the MPC to use its own AI root note detection on the sample file — which, in practice, means it reads the root note from the filename (if named per convention) or analyzes the fundamental frequency. Setting an explicit non-zero root note in the XPM will override MPC's detection and can create a pitch offset where the sample plays at a semitone-shifted position from where you expect.

The failure mode: you set `RootNote = 60` (MIDI C4) for a sample recorded at C3. The MPC applies an additional transposition based on the discrepancy between the embedded root note and the actual detected root. The result is a twelve-semitone pitch error that is audible but whose source is not obvious.

Use `RootNote = 0` and trust the filename convention. If your files are named with root notes (`MyBass_C3.wav`), Oxport handles the rest automatically.

---

**Rule 3: VelStart = 0 on empty layers**

```xml
<Layer velStart="0" velEnd="0" sample="" ... />
```

An empty layer — a layer slot with no sample assigned — must have `velStart="0"`. A non-zero `velStart` on an empty layer creates a velocity zone with no sample, which can trigger silence as an active voice, consuming a polyphony slot and occasionally producing a very faint noise artifact (a click from the envelope triggering on nothing).

The failure mode: your pack works normally at low velocities and produces occasional clicking or unexplained voice-stealing at high velocities. The root cause is an empty layer at high velocity that is being triggered.

Oxport never creates empty layers with non-zero `velStart` — this is the one bug that comes exclusively from hand-editing XPM files without following the convention. If you edit XPM files manually, check every empty `<Layer>` element before rebuilding the bundle.

> **Tip:** These three rules are a quick diagnostic for any mystery pack behavior. If a keygroup is monopitch: check KeyTrack. If a program plays at the wrong pitch: check RootNote. If a program has intermittent voice-stealing or clicking: check empty layer VelStart. In order of frequency, this is how these failures appear in the wild.

---

## 7.6 Velocity Layer Strategy

Velocity layers are the difference between a pack that plays and a pack that performs. The right number of layers depends on two things: the program's aggression score and what parameters change between velocities.

The governing principle from XO_OX Doctrine D001: **velocity must shape timbre, not just amplitude.** A layer transition that only adjusts volume is not a velocity layer in any meaningful sense — it is a volume knob tied to note velocity. True velocity expression means the timbre changes: filter brightness opens, harmonic content shifts, attack character transforms. Each velocity layer should be a genuinely different version of the instrument.

**2 layers:** Minimum viable for drum programs with very consistent hit character (open hi-hats, ambient percussion, FX). Appropriate when the instrument has low aggression (DNA score 1–3) and the primary difference between soft and loud is volume, not timbre.

**4 layers:** Standard for Foundation and Atmosphere mood programs. Velocity zones: 0–31, 32–63, 64–95, 96–127. Filter brightness increases at each boundary, with a subtle harmonic shift from zone 2 to zone 3. This is the minimum for any keygroup program distributed publicly.

**6 layers:** Appropriate for mid-aggression programs (DNA score 4–6). Zones: 0–21, 22–42, 43–63, 64–84, 85–105, 106–127. The additional resolution in the middle of the velocity range is where most expressive playing happens — 4-layer stacks sometimes have an audible "step" in the mezzo-forte range that 6-layer stacks eliminate.

**8 layers:** Standard for expressive keygroup programs and high-character drum programs (Kick and Snare). DNA aggression score 6–8. The zone boundaries shift the filter envelope amount progressively, so a very soft note has a slow, dark filter envelope and a very hard hit has a fast, bright filter sweep. Eight layers are enough to make the velocity-to-timbre curve feel continuous rather than stepped to most players.

**16 layers:** Reserved for programs where the entire character transforms across the velocity range. OBLONG (XOblongBob) at maximum Mojo, OPAL at maximum grain density, any program where the soft version and loud version are essentially different instruments. The storage cost is significant (16× more samples per root note), and the perceptual benefit drops off above 8 layers for most players and most program types.

The practical test for layer count: render at minimum velocity (v1) and maximum velocity (v127). If the two extremes sound more than 50% different in timbre, you have material for 8 layers. If they sound the same, you have a Q-Link problem — the parameter difference you want between soft and loud has not been captured in the render. Fix the render first.

Layer transitions: always enable velocity crossfade for keygroup programs (`<VelocityXfade>` in the XPM). Hard-split velocity transitions — where one sample cuts off exactly as another begins — produce an audible step at the boundary. A 10–20ms crossfade window at each boundary makes the transition imperceptible. The exception: drum programs, where you want hard splits between velocity zones to preserve the sharp transient character of each layer.

> **Tip:** Normalize all velocity layers to the same peak level before building the pack. The amplitude difference between layers should come from the instrument's physical behavior (a soft piano note is genuinely quieter), not from inconsistent gain staging in your renders. If layer 1 is at -12dB and layer 8 is at -3dB, the velocity response will feel accurate, but MPC's output will jump unpredictably as you cross layer boundaries. Match your gain staging first.

---

## 7.7 Q-Link Assignment Best Practices

The XO_OX standard for Q-Link assignment is:

| Q-Link | Axis | Label | Target Example |
|--------|------|-------|----------------|
| Q1 | Tone | TONE or engine-specific | filter_cutoff |
| Q2 | Space | SPACE | reverb_send |
| Q3 | Movement | MOVE | lfo1_rate or grain_scatter |
| Q4 | Weight | WGHT | env_attack or drive |

This standard exists because MPC producers develop muscle memory for Q-Link positions across packs. A producer who has used six XO_OX packs expects Q1 to control tonal character, Q2 to control depth, Q3 to control movement, and Q4 to control transient weight or density. Departing from this standard requires a reason.

The label field is displayed on the MPC's OLED screen. The limit is eight characters — eight characters maximum, with no exceptions, because labels longer than eight characters are truncated in ways that make them unreadable. Test your labels on hardware or use the validator's OLED check before shipping.

Good eight-character OLED labels: `TONE`, `SPACE`, `MOVE`, `WGHT`, `BITE`, `DRIVE`, `FILTER`, `REVERB`, `SCATTER`, `DENSITY`. Each of these is immediately comprehensible on a small screen at a glance.

Bad labels: `FILTER-CUTOFF` (truncated to `FILTER-C`, meaningless), `GRAIN SCATTER` (truncated to `GRAIN SC`, odd), `Q1-BRIGHTNESS` (truncated to `Q1-BRIGH`, confusing).

The `bipolar` field matters. A unipolar Q-Link (`bipolar: false`) sweeps from its minimum to its maximum — appropriate for filter cutoff, reverb send, LFO rate. A bipolar Q-Link (`bipolar: true`) sweeps from negative to positive around a center point — appropriate for pitch modulation, EQ tilt, pan position, any parameter where the center position is the neutral state and movement in either direction is meaningful.

Setting `bipolar: true` on a filter cutoff is wrong: the filter does not have a meaningful "neutral center" that you are departing from in two directions. Setting `bipolar: false` on a pitch modulation target is wrong: pitch modulation below center should go down, not stay at minimum. Check every Q-Link's bipolar setting against the semantics of what you are modulating.

The `smoothing` parameter controls how fast the Q-Link responds to movement:
- `0ms`: Switches. Step-function response. Use for: cycle type selection, wave type selector, on/off toggles.
- `20ms`: Fast. Use for: filter cutoff, envelope attack/release, most timbral controls.
- `40ms`: Medium. Use for: pitch controls, LFO rate, spatial parameters.
- `80ms`: Slow. Use for: reverb tail parameters, very slow LFO targets, anything where sudden jumps produce artifacts.

```json
"q_links": [
  {
    "target": "filter_cutoff",
    "label": "TONE",
    "bipolar": false,
    "smoothing_ms": 20,
    "range_min": 20,
    "range_max": 18000
  },
  {
    "target": "reverb_send",
    "label": "SPACE",
    "bipolar": false,
    "smoothing_ms": 40,
    "range_min": 0,
    "range_max": 100
  },
  {
    "target": "lfo1_rate",
    "label": "MOVE",
    "bipolar": false,
    "smoothing_ms": 40,
    "range_min": 0.1,
    "range_max": 8.0
  },
  {
    "target": "amp_attack",
    "label": "WGHT",
    "bipolar": false,
    "smoothing_ms": 20,
    "range_min": 0,
    "range_max": 500
  }
]
```

> **Tip:** Use the `range_min` and `range_max` fields to constrain Q-Link range to the musically useful portion of each parameter. A filter cutoff Q-Link that sweeps the full 20Hz–20kHz range is theoretically complete but practically useless — the interesting action happens between 80Hz and 8kHz. Constrain the range to where the parameter actually sounds good, and the Q-Link will feel more expressive and less like a dial that spends half its travel in inaudible territory.

---

## 7.8 Advanced Pack Types

Once you have shipped a standard drum or keygroup pack, three advanced formats are worth understanding. They require more design discipline but produce packs that are genuinely more valuable than another standard program collection.

**Tutorial Mode Pack.** A Tutorial Mode pack is organized as a pedagogical progression through an engine's capabilities. The manifest includes the `tutorialMode: true` flag. Programs are named and ordered to tell a story: what is this engine at its simplest? What happens when you add modulation? What does it sound like at full expression? Q-Link assignments in Tutorial Mode packs are intentionally descriptive of the engine's most important parameters rather than optimized for live performance. To build a Tutorial Mode pack, start from the simplest possible version of the program (single oscillator, filter fully open, no modulation) and add one dimension of complexity per program until you reach the engine's expressive ceiling.

**Deconstruction Pack.** A Deconstruction Pack takes one source sound and systematically varies a single parameter across all programs in the pack. A deconstruction of OPAL's grain size parameter: sixteen programs where grain size steps from 1ms to 500ms and every other parameter is held constant. A deconstruction of OBLONG's Mojo axis: twelve programs spanning the full clean-to-saturated range at constant pitch and velocity. Deconstruction packs are valuable design tools — they make a parameter's sonic effect immediately audible — and they are compelling catalog items for sound designers who want to understand an engine deeply. The manifest field `packType: "deconstruction"` marks these explicitly.

**Generative Kit.** A Generative Kit uses the `CycleType: "random-norepeat"` setting on all voices, combined with larger round-robin pools (8–12 cycles per voice), to produce drum programs that never repeat in the same order. Every time the kit is triggered, a different combination of velocity layers and cycle variants is selected. This is not random variation for its own sake — a well-designed generative kit uses the variation to simulate the micro-timing and timbral inconsistency of a live drummer. The design discipline required is that every sample in the pool must be musically compatible with every other sample in the same voice's pool. You cannot put a tight snare and a loose snare in the same random pool without the rhythm feeling broken; you can put eight slightly different versions of the same snare in the pool and the rhythm will feel alive.

```json
{
  "name": "Generative Snare",
  "voice": "snare",
  "cycle_type": "random-norepeat",
  "layers": [
    { "sample": "snare_v01_rr1.wav", "vel_start": 0, "vel_end": 63 },
    { "sample": "snare_v01_rr2.wav", "vel_start": 0, "vel_end": 63 },
    { "sample": "snare_v01_rr3.wav", "vel_start": 0, "vel_end": 63 },
    { "sample": "snare_v01_rr4.wav", "vel_start": 0, "vel_end": 63 },
    { "sample": "snare_v02_rr1.wav", "vel_start": 64, "vel_end": 127 },
    { "sample": "snare_v02_rr2.wav", "vel_start": 64, "vel_end": 127 },
    { "sample": "snare_v02_rr3.wav", "vel_start": 64, "vel_end": 127 },
    { "sample": "snare_v02_rr4.wav", "vel_start": 64, "vel_end": 127 }
  ]
}
```

> **Tip:** Before building a Generative Kit, test your round-robin pool by triggering all samples in sequence. They should feel like the same hit with natural variation — not like different sounds. If two samples in the pool have a perceptible character difference (one has more crack, one has more body), a player will hear the kit as inconsistent rather than alive. Natural variation is what you are after; inconsistency is what you are avoiding.

---

## 7.9 Distributing Your Pack

**The expansion.json manifest.** Every public XO_OX-compatible pack requires a well-formed `manifest.json` at the root of the XPN bundle. The minimum required fields for distribution:

```json
{
  "schemaVersion": "3.0",
  "name": "My Pack Name",
  "vendor": "Your Name or Studio Name",
  "version": "1.0.0",
  "description": "One to three sentences describing what the pack is for and what it sounds like.",
  "genreTags": ["Hip Hop", "Electronic"],
  "moodTags": ["Foundation"],
  "targetHardware": ["MPC Live II", "MPC One", "MPC X", "MPC One+"],
  "programs": ["path/to/program1.xpm", "path/to/program2.xpm"],
  "samples": "Samples/",
  "artwork": "Artwork/cover_1000x1000.png",
  "previewAudio": "preview.mp3"
}
```

The `previewAudio` field is optional but strongly recommended for any pack distributed publicly. A 30-second audio preview embedded in the bundle allows MPC Beats and the online pack browser to play the preview before download. If you do not supply one, Oxport can generate one automatically from the first program's root note.

**MPC Forum distribution.** The MPC Forum (mpcforums.com) has a dedicated expansion pack section. Thread format: pack name as thread title, one-paragraph description, DNA badge table (copy the format from any XO_OX official pack page), download link, version history. Include a Q-Link map in the first post — this is the single piece of information that forum members check before downloading.

**XO_OX Seed+Grow submission.** To submit a pack to the XO_OX community library:

```bash
# Run the full validation suite before submitting
python3 Tools/validate_manifest.py --strict --firmware 3.0 MyPack_v1.0.0.xpn
python3 Tools/batch_tester.py --mpc-beats-path /Applications/MPC\ Beats.app MyPack_v1.0.0.xpn
```

Both validators must pass with zero errors (warnings are acceptable). Submit the validated bundle plus a pack description (minimum 200 words covering the source engine, design intent, and Q-Link map) to the XO_OX community Discord `#seed-grow-submissions` channel. The Community Curator handles technical review; a soul review follows for packs that pass the technical gate.

The soul review evaluates one thing: does the pack serve the player, or does it serve the designer's ego? A pack that covers the obvious territory of an engine and makes that territory accessible is passing the soul review. A pack that showcases unusual presets that are only interesting to people who already know the engine deeply and contains nothing for a player who is encountering the engine for the first time is not. Design for the player.

**Version control.** Use semantic versioning for all public packs: `1.0.0` for initial release, `1.0.1` for sample fixes or minor program corrections, `1.1.0` for new programs added to an existing pack, `2.0.0` for a full reconstruction. The version field in the manifest is machine-read by MPC Beats for update notification — version strings that do not follow semver format are treated as `0.0.0`.

```bash
# Package a release with version tagging and checksum
python3 Tools/package_release.py \
  --xpn MyPack_v1.0.0.xpn \
  --version 1.0.0 \
  --changelog "Initial release. 16 programs across 4 ONSET voice families." \
  --output ./releases/MyPack_v1.0.0_release.zip
```

The release ZIP contains the `.xpn` bundle, a `README.txt` with install instructions and the Q-Link map table, a `CHANGELOG.txt`, and a `SHA256.txt` checksum file for integrity verification.

> **Tip:** Ship a 1.0 release and nothing else until you have loaded it on hardware and played it in a real session. The most common source of 1.0.1 hotfixes is discovering a velocity-transition click, a Q-Link label that is truncated poorly on the OLED, or a root note mismatch — all of which are invisible in the validation report but audible the first time a player uses the pack in production. Play your own pack before you release it.

---

Building your own packs is the point where you stop being a consumer of the XPN format and start being a creator of it. The format is explicit about what it requires, patient with the things it allows, and ruthlessly clear about what constitutes a failure. The Three Golden Rules are three rules because three is all it took to encode the most common failures. Learn them, follow them, and the format will do exactly what you tell it to.

The rest is sound design.

---

*Next: Chapter 8 — Cover Art and Visual Identity*
