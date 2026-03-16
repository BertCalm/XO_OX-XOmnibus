# XPN / MPC OS Deep Integration R&D

Deep integration opportunities between XO_OX packs and MPC OS features — going beyond basic XPM
programs to leverage the full MPC feature set available in MPC OS 3.x and later.

---

## 1. Q-Link Integration

Q-Link defaults are already written into XPM output by both `xpn_drum_export.py` and
`xpn_keygroup_export.py`. The established layout is documented in `xpn_qlink_defaults_rnd.md` and
is treated as doctrine. This section focuses on surfacing that doctrine in user-facing materials.

### Canonical layouts by program type

| Program type | Q1 | Q2 | Q3 | Q4 |
|---|---|---|---|---|
| Drum / ONSET | TONE (`FilterCutoff`) | SNAP (`VolumeDecay`) | GRIT (`Resonance`) | SPACE (`Send1`) |
| Melodic keygroup | TONE (`FilterCutoff`) | ATTACK (`VolumeAttack`) | RELEASE (`VolumeRelease`) | SPACE (`Send1`) |
| Atmospheric pad | TONE (`FilterCutoff`) | SWELL (`VolumeAttack`) | DEPTH (`Resonance`) | SPACE (`Send1`) |
| Character / saturation | DRIVE (`Volume`) | TONE (`FilterCutoff`) | BITE (`Resonance`) | SPACE (`Send1`) |

Q4 = SPACE / `Send1` is a fixed convention across all program types. Performers always find reverb
on the last knob. The convention must not be broken for any XO_OX release.

### MPCE_SETUP.md documentation standard

Every pack that ships with an `MPCE_SETUP.md` (or equivalent README) must include a Q-Link
section. Recommended block:

```
## Q-Link Reference

Q-Links are pre-mapped in each program. Physical knob positions:

  Q1 — TONE      Filter cutoff. Open to brighten, close to darken.
  Q2 — ATTACK    Volume envelope attack. Slow for pads, fast for plucks.
  Q3 — RELEASE   Note tail length. Increase for ambient washes.
  Q4 — SPACE     Reverb send. Always on the last knob — reach for it confidently.

These are starting positions. Reassign freely in Program > Q-Links.
```

Adjust the descriptive text per program type. For drum programs substitute SNAP/GRIT for Q2/Q3
with corresponding drum-focused descriptions. The label and parameter must match the XPM file
exactly — copy-paste from the export output to prevent documentation drift.

---

## 2. Step Sequencer Integration

MPC's built-in step sequencer operates at program level via MIDI sequence data stored in `.xpq`
files inside an `.xpj` project. XPM files themselves do not embed sequence data — sequences live
in the project layer above them.

### What XPN packs cannot do directly

A bare XPN bundle (ZIP of XPM + samples) cannot ship a pre-loaded sequence. The MPC loads the
XPM as a program; the sequence grid is empty until the user records or draws notes.

### What XPN packs can do via project templates

A "Starter Kit" project file (see Section 5 on `.xpj` templates) can bundle:
- The XPM programs pre-assigned to tracks
- Pre-drawn step sequences as MIDI data in the tracks
- The sequences saved as `.xpq` files inside the project bundle

This is the correct delivery vehicle for starter beats, not the XPM itself.

### Starter beat design for ONSET packs

An ONSET starter beat template should demonstrate the engine's voice separation logic. Recommended
16-step patterns per voice:

| Voice | Pad # | Suggested starter pattern |
|---|---|---|
| Kick | A01 | Steps 1, 5, 9, 13 (four-on-the-floor baseline) |
| Snare | A02 | Steps 5, 13 (2 and 4) |
| CHat | A03 | Steps 1, 3, 5, 7, 9, 11, 13, 15 (8th notes) |
| OHat | A04 | Steps 7, 15 (upbeat accents) |
| Clap | A05 | Steps 5, 13 (layered on snare) |
| Tom | A06 | Omit — leave for user to compose |
| Perc | A07 | Step 3, 11 (syncopated accent) |
| FX  | A08 | Step 16 only (pre-drop hit) |

The goal is a production-ready loop that immediately demonstrates what the engine sounds like in
context, not a complete composition. Sparse, open, 8 or fewer active voices per step.

### Velocity storytelling in sequences

MPC step sequencer supports per-step velocity. Pack templates should use velocity variation rather
than uniform 100% values. Recommend a slight accent (127) on downbeats, mid-level (90) on
backbeats, and soft (65–75) on ghost notes. This is consistent with the Sonic DNA velocity curve
philosophy established in the XPN tools.

---

## 3. Automation Lanes

MPC records Q-Link movements as automation data inside a sequence. This is a powerful but
underused feature in third-party pack design.

### Parameters that reward automation

| Parameter | Program type | Automation story |
|---|---|---|
| `FilterCutoff` (Q1) | All types | Classic filter sweep — tension and release over 8–16 bars |
| `VolumeAttack` (Q2) | Melodic / pad | Cross-fade from plucked to swelling pad mid-phrase |
| `Send1` (Q4) | All types | Dry-to-wet reverb build into a chorus or breakdown |
| `VolumeDecay` (Q2) | Drums | Short/snappy in verse, longer in bridge — same sample, different feel |
| `Resonance` (Q3) | Melodic | Filter resonance peak for one-bar accent, then release |

### Pack design philosophy for automation readiness

Presets intended as automation targets should:

1. Sound coherent at both Q-Link extremes. A filter sweep preset should not become inaudible or
   harsh at Min or Max — verify the `Min`/`Max` ranges from `xpn_qlink_defaults_rnd.md` are set
   conservatively enough to avoid extremes.

2. Use meaningful parameter ranges. Automation reads the full knob travel; a narrow range wastes
   the gesture. Aim for perceptible change across the first 10% and the last 10% of travel.

3. Document one "automation idea" per signature program in the pack README. Example: "SPACE knob
   automated from 0 to 0.6 over 8 bars creates the trademark OVERDUB tape-reverb build." This
   gives users a starting point without prescribing their entire arrangement.

The philosophy is: **automation tells a story, not a trick**. One slow filter sweep that reveals a
buried mid-frequency character is more valuable than a fast LFO-style wobble achievable in the
synth itself.

---

## 4. MIDI Program Change

MPC supports loading programs via PC messages during live performance. This is primarily relevant
for performers who switch between kit programs mid-set.

### Numbering conventions

MPC assigns PC numbers to programs based on their position in the project's program list (1-indexed,
0–127 range transmitted). XO_OX packs do not directly control PC numbers because they are assigned
at project load time, not inside the XPM file itself.

**Recommended convention:** Document a suggested program slot ordering in `MPCE_SETUP.md`. This
gives users a consistent reference when building their own PC map.

Proposed slot ordering for a full engine pack (e.g., ONSET pack with 20 programs):

```
Slot  1 (PC 0)  — Foundation kit (the default starting point)
Slot  2 (PC 1)  — High-energy variant
Slot  3 (PC 2)  — Lo-fi / vinyl variant
Slot  4 (PC 3)  — Acoustic / room kit
Slot  5 (PC 4)  — Electronic / digital kit
...
Slot 20 (PC 19) — Wildcard / experimental
```

Include an `MPCE_SETUP.md` table listing program name → recommended slot → expected PC number.
Note clearly that users must preserve this slot order in their project to maintain PC mapping.
Document the caveat: if another program is inserted above slot 1, all PC numbers shift down.

### Live performance workflow note

For performers using the MPC as a live instrument rather than a DAW, PC mapping enables seamless
kit switching. XO_OX pack READMEs should acknowledge this use case explicitly and suggest loading
the full pack into one project at session start, then assigning pads or footswitch to PC sends.

---

## 5. Template Projects (.xpj)

The MPC project format (`.xpj`) is an XML-based file that references programs, sequences, and
track assignments. A project bundle is a folder containing the `.xpj` file plus all referenced
sample and program subdirectories.

### .xpj format overview (partial)

The `.xpj` root element contains:
- `<Programs>` — list of program file paths (relative to project root)
- `<Sequences>` — embedded or referenced MIDI sequence data
- `<Tracks>` — track definitions with program assignments, send levels, and mixer settings
- `<Tempos>` — global tempo and time signature

XO_OX does not currently have a Python tool to generate `.xpj` files. This is a gap worth closing.

### Proposed: xpn_project_template.py

A new Tools script that generates a minimal `.xpj` wrapping one or more XPM programs with:
- Correct relative path references to XPM files
- Track names matching program names
- Default tempo (92 BPM for hip-hop packs, 128 BPM for electronic packs — configurable)
- Starter sequences if `.xpq` sequence data is available (see Section 2)
- Mixer tracks with SPACE send pre-routed to a shared reverb return

This would allow XPN packs to ship as a complete `.xpj` bundle rather than a bare ZIP. The user
opens the project and immediately hears a working context, not an empty grid.

### Template project structure

```
XO_OX_ONSET_Starter.xpj project bundle:
├── XO_OX_ONSET_Starter.xpj
├── Programs/
│   ├── ONSET_Foundation.xpm
│   ├── ONSET_Voltage.xpm
│   └── ONSET_Coastal.xpm
├── Sequences/
│   └── StarterBeat_001.xpq
└── Samples/
    └── (all referenced WAV files)
```

The project template is optional shipping content — the base XPN ZIP must still function without
it. Think of it as the "deluxe edition" of a pack delivery.

---

## 6. AIR FX Integration

MPC ships with the AIR plugin suite: Chorus, Reverb, Compressor, Stereo Width, Delay, Flanger,
Distortion, and others. These are available as insert and send effects on every program track.

### Philosophy: complement, don't prescribe

XO_OX pack READMEs should include AIR FX recommendations as suggestions, not as requirements. The
goal is to help users who want a curated starting point without removing creative freedom from
users who prefer to build their own chains.

Recommended documentation format:

```
## Suggested AIR FX Chains

These are starting points, not requirements. Every program sounds designed and intentional without
any additional FX — the suggestions below extend the character further.

**ONSET Foundation**
- Insert 1: AIR Compressor — Attack: 8ms, Release: 60ms, Ratio: 4:1, Threshold: -12dB
  (Glues the kit without killing transients)
- Send 1: AIR Reverb — Decay: 1.4s, Pre-delay: 18ms, Mix: 35%
  (Room context; already pre-routed via Q4 SPACE send)

**OPAL Cascade (keygroup pad)**
- Insert 1: AIR Chorus — Rate: 0.3Hz, Depth: 40%, Delay: 8ms
  (Slight widening for stereo bloom)
- Insert 2: AIR Reverb — Decay: 3.2s, Size: Large Hall, Mix: 25%
  (For standalone pad use without external reverb)
```

### Per-engine AIR FX character guide

| Engine family | Recommended insert | Rationale |
|---|---|---|
| ONSET (drums) | AIR Compressor → AIR Distortion (subtle) | Punch and saturation without muddying transients |
| OVERDUB / OPAL (pads) | AIR Chorus → AIR Reverb | Width and space — both engines have long tails |
| OBESE / OVERBITE (character) | AIR Distortion → AIR Compressor | Character engines want drive first, glue second |
| OVERWORLD (chip) | AIR Stereo Width → AIR Delay | Classic hardware chip treatment — width + slap echo |
| ODYSSEY / ORGANON (evolving) | AIR Reverb only | Rich internal modulation; external FX should not compete |

Document that AIR FX chain recommendations live in `MPCE_SETUP.md`, not inside the XPM file. XPM
does not store insert effect settings; that state lives in the project layer.

---

## 7. CV/Gate Output

MPC X and MPC Force have physical CV outputs (typically 2 × CV + 2 × Gate jacks). This enables
melodic XPN programs to drive hardware synthesizers and Eurorack modules.

### Recommended CV routing for melodic packs

| Pack type | CV1 | Gate1 | CV2 | Gate2 |
|---|---|---|---|---|
| Lead keygroup | Note pitch | Note gate | Velocity (0–5V) | — |
| Chord keygroup | Chord root | Gate | Chord trigger | — |
| Bass keygroup | Note pitch | Note gate | — | — |
| Atmospheric pad | LFO out (slow) | — | Note gate | — |

Document in `MPCE_SETUP.md` under a `## CV Output` section, clearly marked as MPC X / Force only.
Include a note that CV assignment is done in the MPC's global CV/Gate settings menu, not inside
the XPM program — the pack cannot pre-configure CV routing, only recommend it.

### Voltage reference

MPC CV outputs are typically 0–5V pitch (1V/oct) and 0–5V gate. Most Eurorack modules expect this
standard. Some semi-modular hardware (e.g., Moog Grandmother) can accept it directly. Recommend
users calibrate their receiving module before a live performance.

### Workflow note for XO_OX users

Melodic XPN packs are particularly well-suited for CV use because the keygroup format preserves
note-level MIDI data that maps cleanly to single-voice CV pitch output. Drum programs are not
useful for CV pitch output but Gate outputs can be used to sync Eurorack triggers to the MPC's
internal step sequencer.

---

## 8. Ableton Link

MPC supports Ableton Link for tempo synchronization when used in a networked session alongside
Ableton Live or other Link-compatible software.

### Impact on pack design

None. Ableton Link is a tempo transport protocol. It does not affect sample content, XPM program
structure, Q-Link assignments, sequence data, or anything that lives inside an XPN bundle. A pack
designed for standalone MPC use works identically in a Link-synced session.

### Worth acknowledging in documentation

`MPCE_SETUP.md` should include a brief note for users who use MPC in a DAW-adjacent workflow:

```
## Ableton Link

If you use MPC alongside Ableton Live or other Link-enabled software, all programs and sequences
in this pack sync automatically to Link tempo. No special setup required. The pack is designed for
standalone use and is equally effective in a Link session — tempo matching is transparent.
```

This paragraph prevents user confusion for users who find the pack in a Link context and wonder if
there is any configuration required. The answer is no.

---

## Open Questions

- **`.xpj` format version drift**: MPC OS updates have historically changed project file schemas.
  A `xpn_project_template.py` tool would need to target a specific MPC OS version and document
  that assumption. Needs validation against MPC OS 3.x project files.

- **`.xpq` sequence format**: The step sequencer's MIDI data format in `.xpq` files has not been
  reverse-engineered by the Tools team. Building starter beats as project templates requires either
  RE work or relying on undocumented Akai format details.

- **AIR FX parameter IDs**: AIR FX settings inside an `.xpj` project may be automatable via MIDI
  CC or stored as XML attributes. If the project template tool can pre-configure AIR FX chains,
  the "complement, don't prescribe" philosophy becomes "complement with one click" — a significant
  UX upgrade.

- **CV output assignment in `.xpj`**: Whether `.xpj` stores CV routing configuration has not been
  confirmed. If it does, project templates could ship with pre-configured CV assignments, removing
  the manual setup step for hardware users.

- **Q-Link range validation listening pass**: Current `Min`/`Max` ranges in both Python exporters
  are set conservatively. A formal listening pass with hardware MPC against each range across all
  four Q-Link presets (drum/melodic/atmospheric/character) would confirm or refine the ranges
  before V1 pack release.
