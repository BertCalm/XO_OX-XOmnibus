# XPN Sound Design Workflow — End-to-End R&D Spec

**Version:** 1.0
**Author:** XO_OX Sound Design
**Date:** 2026-03-16
**Status:** Living Document
**Scope:** Complete pipeline from XOmnibus preset to final .xpn on MPC hardware
**Related:** `Tools/xpn_render_spec.py`, `Tools/xpn_drum_export.py`, `Tools/xpn_keygroup_export.py`, `Tools/xpn_smart_trim.py`, `Tools/xpn_normalize.py`, `Docs/specs/sound_design_best_practices_xpn.md`, `Docs/specs/juce_to_mpc_workflow_rnd.md`

---

## Overview

A factory preset in XOmnibus is a live synthesis object. A program inside an .xpn expansion pack is a frozen audio artifact. Everything between those two states — the decisions, the renders, the curation, the listening — is sound design.

This document covers the complete workflow from preset selection to shipping .xpn. It distinguishes what the tools automate, what the sound designer must decide, and where the most common mistakes happen. Read it before starting any pack. Refer back to it during the QA pass.

The pipeline has four layers:

```
XOmnibus Presets (.xometa)
    ↓  [xpn_render_spec.py — generates render instructions]
WAV Render Session (Standalone + DAW-less)
    ↓  [xpn_smart_trim.py, xpn_normalize.py — post-process]
XPM Assembly (xpn_drum_export.py / xpn_keygroup_export.py)
    ↓  [xpn_packager.py, xpn_bundle_builder.py — finalize]
.xpn File → MPC Hardware
```

---

## Part 1: Starting Point — XOmnibus Presets as Source Material

### 1.1 What a .xometa File Contains

Every factory preset is a `.xometa` JSON file storing:
- Engine list (1–4 engines active, referenced by engine ID)
- Parameter snapshot (all `{prefix}_{param}` values, including macro positions)
- 6D Sonic DNA (brightness / warmth / movement / density / space / aggression)
- Mood tag (Foundation / Atmosphere / Entangled / Prism / Flux / Aether / Family)
- 4 macro labels and their current positions (M1–M4)

When the export pipeline runs, it reads these values to determine render strategy. The preset's Sonic DNA influences normalization targets; the engine determines sample density (drum vs. keygroup, range, velocity layers).

### 1.2 The Three-Tool Pipeline

**Step 1 — `xpn_render_spec.py`**

Run this before touching the DAW. Feed it a preset or engine name and it outputs a render specification: a list of WAV filenames to record, with MIDI notes, velocity targets, and variants. This is the producer's session plan. Do not skip this step and attempt to improvise a render order — you will forget variants, leave gaps in velocity coverage, or produce inconsistent filenames that break downstream tools.

```bash
# Generate spec for a single preset
python3 Tools/xpn_render_spec.py --preset Presets/XOmnibus/Foundation/OBLONG_WarmBrass.xometa

# Generate spec for all Onset presets
python3 Tools/xpn_render_spec.py --engine Onset --output-dir /tmp/onset_specs
```

The spec output is the contract. If a WAV is not in the spec, the export tools will not include it.

**Step 2 — Manual render session or Fleet Render**

The sound designer records every WAV called for by the spec. XOmnibus standalone is the render environment (see Part 2). Fleet Render Automation (`Source/Export/XPNExporter.h`) will eventually automate this; until `renderNoteToWav()` is fully wired, manual recording is the bottleneck.

**Step 3 — `xpn_drum_export.py` or `xpn_keygroup_export.py`**

With WAVs recorded and post-processed, the export tools assemble XPM program files and pack the directory. They handle pad routing, choke groups, Q-Link assignments, velocity layer curves, CycleType/CycleGroup round-robin logic, and PadNoteMap. The sound designer does not edit XPM XML by hand. If a parameter is wrong in the output, fix it in the tool or the spec — not in the XML.

### 1.3 What the Sound Designer Decides vs. What the Tools Automate

| Decision | Owner |
|----------|-------|
| Which presets to include in the pack | Sound designer |
| Macro position at render time | Sound designer |
| Mono vs. stereo per sample | Sound designer |
| Sample length (sustain duration, tail capture) | Sound designer |
| feliX-Oscar axis render positions | Sound designer |
| MOVEMENT macro render positions (bake vs. Q-Link) | Sound designer |
| Velocity layer count per sound | Sound designer (informed by spec defaults) |
| Pad assignment (which sound goes to which pad) | Sound designer |
| WAV filename conventions | Tools (enforced by spec output) |
| XPM velocity curve shape (Vibe's musical curve) | Tools |
| Choke group assignment | Tools (physical defaults + override) |
| Q-Link CC routing | Tools (defaults + override) |
| .xpn bundle structure and ZIP packaging | Tools |

The sound designer's judgment is irreplaceable for the first eight rows. The tools are irreplaceable for the last six.

---

## Part 2: Render Session Setup

### 2.1 DAW-Less Workflow

All XPN renders use **XOmnibus Standalone**. No DAW, no insert effects, no bus processing. The render chain is:

```
XOmnibus Standalone → System audio out → DAW (record-only)
```

Or, if a direct capture path is available:

```
XOmnibus Standalone → loopback interface → recording app
```

The DAW is a recorder, not a processor. No EQ, no compression, no reverb on the capture bus. What goes in is what ships.

This is not a limitation. It is the design principle: the sample must sound like the engine, not like the engine through a mix chain the MPC producer cannot replicate.

### 2.2 Recommended Render Settings

| Parameter | Value | Reason |
|-----------|-------|--------|
| Sample rate | 44100 Hz | MPC native; avoid resampling artifacts |
| Bit depth | 24-bit | Headroom for normalization; .xpn strips to 16-bit on older hardware |
| Format | WAV (PCM, uncompressed) | Required by export tools |
| Monitoring level | Consistent — set once, do not adjust | Level inconsistencies across sessions produce uneven programs |
| Clip guard | Leave -3 dBFS headroom minimum | Normalize in post, not by ear |
| System buffer | Lowest stable latency | Tight attacks demand low-latency capture |

### 2.3 Mono vs. Stereo — Decision Per Engine Type

The default is **mono unless stereo is the character.**

| Engine Type | Recommended | Reason |
|-------------|-------------|--------|
| Percussive mono hits (ONSET Kick, Snare, Clap) | Mono | Phase coherence on MPC stereo out; smaller files |
| Wide pads / textural (OPAL, ORGANON) | Stereo | Width is the product |
| Physical model body resonance (OBSCURA, OWLFISH) | Stereo | Resonance chamber imaging |
| Bass sounds, sub-register (OBESE low register, OCEANDEEP) | Mono | Sub frequencies; stereo adds nothing below 80Hz |
| Ensemble/choir textures (OBBLIGATO, OTTONI multi-voice) | Stereo | Spatial spread is the identity |
| Granular textures (OPAL) | Stereo | Grain diffusion across stereo field |
| Keygroup chromatic pitched sounds | Mono unless width is the character | Consistent stereo image across pitch range is difficult to maintain |

When in doubt: render mono first, A/B on MPC hardware. If the stereo version is audibly better in context, keep stereo. If it is just "wider for the sake of it," use mono.

---

## Part 3: The feliX-Oscar Sweep

### 3.1 What the feliX-Oscar Axis Is

Every XOmnibus preset exists on a polarity axis: feliX (neon tetra, brightness, openness, air, high frequency character) versus Oscar (axolotl, depth, weight, darkness, sub-harmonic density). The axis is not a single parameter — it is a composite shift across filter cutoff, oscillator blend, coupling direction, and macro M1 (CHARACTER macro by convention).

For XPN packs, this axis is the primary source of variation across a program. Rather than shipping one render of a preset, the sound designer renders three positions along the axis:

- **Full feliX** — CHARACTER macro at 100%, filter open, bright oscillator blend
- **Center** — CHARACTER macro at 50%, balanced state
- **Full Oscar** — CHARACTER macro at 0%, filter closed, dark oscillator blend

### 3.2 How the Three Renders Become a Program

The three renders are mapped to velocity layers:

| Velocity Layer | Sample | MIDI Velocity Range |
|---------------|--------|---------------------|
| Layer 1 (soft) | Full Oscar | 1–40 |
| Layer 2 (medium) | Center | 41–90 |
| Layer 3 (hard) | Full feliX | 91–127 |

This maps feliX-Oscar polarity to playing intensity in a way that is musically intuitive: soft playing reaches for weight and darkness; hard playing reaches for brightness and air. The MPC producer does not need to understand the XO_OX cosmology to feel this — it responds naturally to their touch.

For programs requiring four velocity layers, add a second feliX render at a slightly different macro position (CHARACTER 80%) as Layer 3, pushing the full-feliX render to Layer 4.

### 3.3 When Three Renders Are Not Enough

For engines with strong coupling character (Entangled mood presets with OPAL, ORGANON, ORBITAL), the feliX-Oscar axis may not capture the full sonic range. In those cases:

- Render six positions: full Oscar / Oscar-center / center / center-feliX / feliX / feliX+COUPLING macro
- Or render the COUPLING macro separately and assign it to a second Q-Link

The goal is that every velocity layer sounds like a distinct, usable sound — not just louder or softer.

---

## Part 4: Macro Sweep Renders

### 4.1 The MOVEMENT Macro

M2 (MOVEMENT) controls temporal evolution: LFO depth, envelope rates, modulation speeds, tremolo intensity, arpeggiation. For evolving pad programs, it is the primary expressive axis after feliX-Oscar.

Standard approach for pads with strong MOVEMENT character:

- Render MOVEMENT at **0%** — static, settled, suspended
- Render MOVEMENT at **50%** — moderate evolution, medium LFO depth
- Render MOVEMENT at **100%** — maximum motion, maximum LFO, maximum rate

These become three programs in the pack, not three velocity layers in one program. A producer wants "Static Pad," "Breathing Pad," and "Living Pad" as separate pads, not as velocity states of the same pad.

### 4.2 Bake vs. Q-Link — The Decision

Bake the macro when:
- The macro produces a continuous, obvious effect (filter sweep, tremolo depth)
- The MPC producer benefits from snapping to pre-designed positions
- The sound is designed around a specific macro state and others are not useful

Leave as Q-Link when:
- The macro is a real-time performance control (vibrato rate, filter brightness)
- The effect requires gradual transition that a pad velocity layer cannot capture
- The producer is likely to want to adjust it live during performance

**Default rule:** MOVEMENT is almost always a Q-Link. CHARACTER is usually baked (feliX-Oscar sweep renders). COUPLING and SPACE macros are case-by-case.

### 4.3 Evolving Pad Render Technique

For MOVEMENT=100% renders, the LFO phase and modulation state at the start of the render matters. To capture a representative state:

1. Play the preset in XOmnibus Standalone for 30 seconds before beginning capture
2. Let the modulation settle into its characteristic motion cycle
3. Record 8–16 seconds minimum — enough to capture at least 2 full LFO cycles
4. The sample loop point, if any, is set post-capture in xpn_smart_trim.py

Never render a 0.5s sample of an evolving pad. The movement IS the sample.

---

## Part 5: Sample Editing Philosophy

### 5.1 The Three Tools and When to Use Each

**`xpn_smart_trim.py`**

Trims silence from the head and tail of a WAV. Use it on every percussive sound (drums, plucks, stabs). The head trim removes pre-attack silence that causes pad response latency on MPC; the tail trim removes DC offset tails and noise floor that add file size without musical value.

Do NOT use aggressive tail trim on reverb or decay sounds. The reverb tail IS the sound. Trim only the silence that follows the reverb tail's natural -60 dBFS floor.

**`xpn_normalize.py`**

Normalizes peak or RMS level to a target. Use it when:
- Renders from the same session have inconsistent levels (MOVEMENT=0% vs. MOVEMENT=100% often renders at different peak levels)
- Mono renders need to compensate for perceived loudness against stereo renders in the same program
- Final QA reveals a pad that is +6 dB louder than its neighbors (destroys pad dynamics)

Do NOT use normalization to fix dynamic range. Normalization raises everything uniformly. If the soft velocity layer sounds too quiet, the answer is a better render (velocity-sensitive synth parameter), not normalization.

**Raw samples (no processing)**

Leave raw when:
- The sound is already at a consistent, appropriate level
- The character includes intentional noise, breath, room sound, or analog imperfection
- Over-trimmed samples would lose the physical gesture (the bow attack, the breath onset, the transient of a plucked string)

### 5.2 The Guiding Principle

**Preserve the character. Remove the silence.**

Silence before the attack makes pads feel sluggish. Silence after the tail wastes memory and can confuse MPC's loop detection. But the character — the imperfect attack, the slightly too-long decay, the gritty noise floor of a modeled resonator — is what makes the sample worth having.

When in doubt, trust the render over the tool. The tool is for logistics. The sound designer's ear is for character.

---

## Part 6: Kit Curation — Program Count and Narrative Arc

### 6.1 Program Count Targets by Pack Tier

| Tier | Pack Name Convention | Programs | Programs per Engine | Notes |
|------|---------------------|----------|---------------------|-------|
| SIGNAL | Single-engine or concept-focused | 12–16 | 12–16 | Entry; one clear identity |
| FORM | Multi-engine, theme-built | 20–24 | 6–8 per engine | Standard release |
| DOCTRINE | Complete ecosystem pack | 30+ | 4–6 per engine across 6–8 engines | Flagship; takes weeks |

Do not pad a SIGNAL pack to 20 programs by including weak presets. Twelve strong programs outperform twenty average ones. Every program must earn its place.

### 6.2 The Arc — Not Just Categories

A pack is not a list. It is a sequence. The MPC producer scrolls through programs. The order creates a narrative.

**Standard arc structure for a 16-program SIGNAL pack:**

| Position | Role | Character |
|----------|------|-----------|
| Programs 01–03 | Entry / Foundation | Immediately useful; no questions asked |
| Programs 04–06 | Character Assertion | The distinctive voice of this engine, fully stated |
| Programs 07–10 | Range Demonstration | Show the full spectrum: bright to dark, sparse to dense |
| Programs 11–14 | Coupling and Complexity | Entangled mood; programs that reward exploration |
| Programs 15–16 | Signature / Closing | Memorable; the one the producer plays last and remembers |

Within each category block, sort by the most likely creative use pattern: drums before pads, bass before melody, wet before dry for effects engines.

**Anti-pattern to avoid:** Five similar pads in a row. Even if all five are excellent individually, back-to-back placement makes them seem redundant.

### 6.3 Naming the Arc

Program names should feel like track titles or instrument descriptions from a specific place and time. They should not be parameter descriptions ("Hi Res Filter Sweep") or generic categories ("Warm Pad 1").

Use the O-Word Vault (`Docs/o-word-vault.md`) and pack identity cards for vocabulary reference. The name is the first sound design impression before the pad is hit.

---

## Part 7: QA Listening Pass

### 7.1 The Hardware-First Principle

All QA must happen on MPC hardware. Listening in the DAW or in Standalone is insufficient. The MPC's D/A converters, the pad response, and the program playback engine are the product. QA in any other environment is not QA.

Minimum hardware setup:
- MPC (Live II, Force, or MPC One) with pack loaded from SD card or internal storage
- Headphones or studio monitors plugged into MPC output (not the DAW)
- PACK_ISSUES.md open for logging

### 7.2 The Systematic Test Protocol

For each program in the pack:

**Velocity test:**
- Hit each pad at pp (velocity ~20), mp (~70), and ff (~120)
- Expected: audible timbral change between layers, not just volume change
- Log: "Program 07, Pad A3 — no timbral difference pp vs. ff"

**Pad coverage test:**
- Trigger every pad — including pads that are not expected to have sound
- Expected: no ghost triggers on empty pads (VelStart=0 on all empty layers)
- Log: "Program 11, Pad D4 — ghost trigger on release"

**Q-Link test:**
- Move each Q-Link from 0 to 100% while holding a sustained note
- Expected: audible, musical response to every assigned Q-Link
- Log: "Program 03, Q-Link 2 — no audible response"

**Tuning test (keygroup programs only):**
- Play C2, C3, C4, C5 in sequence
- Expected: consistent pitch tracking, no octave jumps, no drift
- Log: "Program 09 — pitch drifts +30 cents above C4"

**Level balance test:**
- Play all 16 pads in rapid sequence at the same velocity
- Expected: no single pad dramatically louder or quieter than its neighbors
- Log: "Program 14, Pad B2 — +8 dB above all other pads"

**Tail behavior test:**
- Strike a pad and immediately strike the same pad again
- Expected: choke groups function correctly; retrigger behavior matches intent
- Log: "Program 01, Kick pad — previous hit does not choke on retrigger"

### 7.3 Logging Standard

Every issue goes in `PACK_ISSUES.md` in the pack's working directory with:

```
[P03-A3] Velocity — no timbral change pp vs. ff. Render new feliX/Oscar layers.
[P11-D4] Ghost trigger — empty pad has VelStart > 0. Fix in xpn_keygroup_export.py call.
[P07] Tuning — pitch drift above C4. Re-render at correct root note.
```

Severity tags: `[CRITICAL]` (pack fails), `[MAJOR]` (noticeably wrong, fix before release), `[MINOR]` (cosmetic, acceptable for v1).

Do not ship a pack with any CRITICAL or MAJOR issues open.

---

## Part 8: The Top 10 Sound Design Errors in XPN Packs

These are the errors experienced MPC producers find most frustrating. They are listed in order of how often they occur and how severely they damage the producer's experience.

**Error 1: Velocity layers that only change volume.**
The producer is an expressive player. They want dynamics to change the character of the sound — brighter, more aggressive, more dense. A velocity layer that is just the same sample at a different gain is not an expressive layer. It is a waste of the MPC's capability. Implement the feliX-Oscar sweep. Do the work.

**Error 2: Pre-attack silence on drum pads.**
Hitting a kick pad and waiting 30ms for the transient to arrive is maddening in a live context. Run xpn_smart_trim.py on every percussive sample. If the tool trims too aggressively for your attack, adjust the threshold parameter — do not leave the silence.

**Error 3: Programs that sound like a demo reel, not a tool.**
The sound designer is showing off the engine. Every preset is the most extreme, spectacular state. Macros at 100%, full coupling, full reverb, full movement. The producer loads it, hits a pad, and gets a wall of sound they cannot fit into a track. Include settled, usable versions. Dramatic presets belong at the back of the pack, not the front.

**Error 4: Twelve pads of the same sound at different pitches.**
This is not a keygroup program — it is a sample spread. A keygroup program should map one instrument across a pitch range. A drum program should have 12–16 different sounds. If every pad in a drum kit is the same oscillator waveform at a different pitch, the producer has twelve sounds that all fight for the same frequency space.

**Error 5: Ghost triggers on empty pads.**
The classic XPN bug: VelStart > 0 on an empty layer, causing the pad to trigger a silent sample and choke the bus. The xpn_drum_export.py tool handles this correctly when used properly. If you are editing XPM files manually, you will introduce this bug.

**Error 6: Inconsistent levels across programs in the same pack.**
Program 01 is at -6 dBFS. Program 02 is at -18 dBFS. The producer has to adjust their monitoring level with every program change. Run xpn_normalize.py across all programs in the pack before final packaging. Target RMS level per pack, not per program.

**Error 7: Keygroup programs with pitch drift above the root octave.**
When samples are stretched beyond their recorded range, pitch tracking degrades. A sample recorded at C3 and stretched to C6 will have audible aliasing and pitch inconsistency. Follow the render spec's minor-third sampling density. Do not extend the pitch range beyond what the render covers.

**Error 8: Baked evolution that clashes with itself on retrigger.**
An evolving pad sample that is 8 seconds long, retriggered on beat 2 while the beat-1 tail is still playing, produces two copies of the same LFO modulation at different phases. This sounds like a comb filter. Either trim the pad to a clean loop or set a choke group so retrigger kills the previous instance.

**Error 9: Q-Links assigned but producing no audible change.**
The Q-Link is routed to a parameter that does not move at the sample level because the effect was baked. Example: a MOVEMENT macro assigned to Q-Link 2, but the MOVEMENT effect was fully baked into the sample at a fixed position. The Q-Link moves a parameter that no longer matters. Test every Q-Link on hardware.

**Error 10: No narrative — sixteen programs that could be in any order.**
The producer scrolls through a pack looking for the right sound. If every program is equally bright, equally textured, and equally useful for any context, the pack has no identity. Curate the arc. Name the programs so the sequence makes sense. Give the producer a reason to listen from 01 to 16 rather than just selecting randomly.

---

## Appendix A: Quick Reference — Tool Invocations

```bash
# Generate render spec
python3 Tools/xpn_render_spec.py --preset path/to/preset.xometa

# Batch spec for an engine
python3 Tools/xpn_render_spec.py --engine Oblong --output-dir /tmp/oblong_specs

# Trim silence from percussive samples
python3 Tools/xpn_smart_trim.py --input-dir /tmp/renders/kick --threshold -60

# Normalize a sample directory
python3 Tools/xpn_normalize.py --input-dir /tmp/renders --target-rms -18

# Assemble drum kit XPM
python3 Tools/xpn_drum_export.py --spec /tmp/oblong_specs/kit01.json --output-dir /tmp/programs

# Assemble keygroup XPM
python3 Tools/xpn_keygroup_export.py --spec /tmp/oblong_specs/keys01.json --output-dir /tmp/programs

# Validate all XPM files
python3 Tools/xpn_validator.py --input-dir /tmp/programs

# Package final .xpn
python3 Tools/xpn_packager.py --pack-dir /tmp/programs --output /releases/OBLONG_SIGNAL.xpn
```

## Appendix B: Pack QA Checklist

Before submitting any pack for release:

- [ ] All programs tested on MPC hardware (not emulator, not DAW)
- [ ] Velocity test passed: audible timbral change pp → ff on every pad
- [ ] No ghost triggers on empty pads
- [ ] All Q-Links produce audible response
- [ ] Level balance checked across all programs (xpn_normalize.py run)
- [ ] Keygroup pitch tracking verified across full range
- [ ] Choke group behavior correct for drum programs
- [ ] Program names reviewed against arc narrative
- [ ] PACK_ISSUES.md shows no CRITICAL or MAJOR open items
- [ ] .xpn loads cleanly from cold boot on target MPC hardware
- [ ] Cover art present and correct dimensions (512×512 px minimum)
- [ ] Pack manifest metadata matches actual program count and engine list

---

*End of spec. Questions: reference `sound_design_best_practices_xpn.md` for per-engine render philosophy; reference `xpn_tools.md` for full tool feature documentation.*
