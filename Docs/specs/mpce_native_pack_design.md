# MPCe-Native XPN Pack Design Spec
## Scout + Rex Design Document — March 2026

*Scout: community intel, MPCe hardware research, competitive positioning*
*Rex: XPN format compliance, Oxport tooling, technical implementation*

---

## Executive Summary

MPC Live III (Oct 2025) and MPC XL (Jan 2026) introduce MPCe 3D pads: continuous XY tracking per pad with four assignable corners per pad surface. Simultaneously, MPC Stems (Jul 2024) separates audio into 4 stems on standalone hardware. As of March 2026, **no expansion pack creator has designed content purpose-built for either capability.** XO_OX is positioned to be first to market with a complete MPCe-native design philosophy and the tooling to execute it consistently at scale.

This spec defines the architecture, tooling, and competitive strategy for MPCe-native XPN packs.

---

## Section 1: MPCe 3D Pad Architecture

### 1.1 How MPCe Pads Work

Each MPCe pad (available on MPC Live III and MPC XL) has three sensing dimensions:

| Axis | What It Tracks | When It Fires |
|------|---------------|---------------|
| Z (velocity) | Strike force | On attack only (standard) |
| X | Horizontal finger position on pad surface | Continuously while held |
| Y | Vertical finger position on pad surface | Continuously while held |

The pad surface is divided into four corners, each independently assignable to a sample or articulation. Corner selection is determined by the XY position at the moment of strike. Continuous XY tracking while held allows real-time parameter modulation even after the sample is triggered — this is what separates MPCe from prior generation MPC sensitivity (which only offered aftertouch/channel pressure, not positional XY).

**Corner coordinate system (MPC convention):**

```
NW (upper-left) │ NE (upper-right)
────────────────┼────────────────
SW (lower-left) │ SE (lower-right)
```

MPC firmware maps the four corners to four sample slots within a single pad assignment. The user physically navigates to a corner by holding in that quadrant; motion between corners triggers continuous morphing of any assigned modulation targets.

### 1.2 The XO_OX Quad-Corner Kit Philosophy

The quad-corner model is not just a technical feature — it is a **sound design philosophy**. A pad in a well-designed MPCe pack should feel like an instrument with range, not four buttons compressed into one surface.

The design question is always: *what relationship should the four corners represent?* XO_OX defines five canonical quad-corner architectures:

---

#### Architecture 1: feliX-Oscar Polarity (Primary XO_OX Pattern)

Map the entire feliX-Oscar creative axis onto a single pad.

```
NW: feliX Dry         │ NE: feliX Processed
  (clinical, bright,  │   (bright + FX: reverb,
   no reverb)         │    modulation, spatial)
──────────────────────┼──────────────────────
SW: Oscar Dry         │ SE: Oscar Processed
  (warm, organic,     │   (warm + FX: tape,
   no reverb)         │    saturation, space)
```

**Result:** The player moves from clinical to organic on the horizontal axis, and from dry to processed on the vertical axis. This is the most distinctly XO_OX pattern — no other pack creator has this identity axis to design from.

**Sound design directive for Oxport renders:** Four variants of every sample must be rendered — one per corner. Typical processing chain differences:
- feliX variants: no saturator, HPF at 150Hz or higher, clean reverb if any
- Oscar variants: mild saturation, fuller low-mid body, organic space
- Dry variants: no reverb, no delay, flat stereo
- Processed variants: reverb + at least one modulation element (chorus, vibrato, or analog-warmth plugin)

---

#### Architecture 2: Era Corners

Four corners = four production eras. Best for drum packs and any content with strong machine character.

```
NW: Classic Analog     │ NE: 80s Digital
  (808 era, rounded,   │   (909/LinnDrum era,
   warm machine)       │    crisp, open)
───────────────────────┼──────────────────
SW: 90s SP-1200        │ SE: Contemporary
  (compressed, gritty, │   (TR-8S / modern
   bit-reduced)        │    clean processing)
```

**Result:** X-axis is warmth/crispness. Y-axis is vintage/contemporary. For producers who era-blend as part of their workflow, this collapses eight-plus separate programs into four pads.

**Best engines:** ONSET (drum machine DNA spans eras natively), OVERWORLD (ERA Triangle param is built for this).

---

#### Architecture 3: Dynamic Expression Corners

Articulation variants mapped to physical striking posture — the natural way a drummer thinks.

```
NW: Ghost Note         │ NE: Accent / Rimshot
  (quiet, muted,       │   (alternate strike,
   brush-adjacent)     │    bright transient)
───────────────────────┼──────────────────────
SW: Body Hit           │ SE: Flam / Layered
  (solid center,       │   (two-hit compound,
   full velocity)      │    thickening)
```

**Result:** Performance nuance lives on the pad. A drummer-influenced producer navigates this space instinctively — light touch stays NW, hard center hit lands SW, accent accent rolls toward NE.

---

#### Architecture 4: Coupling State Corners

For packs that showcase XOmnibus coupling expressivity — designed for XOmnibus VST3 users in MPC Desktop 3.5+.

```
NW: Engine Solo        │ NE: Coupling Light
  (engine A only,      │   (engine A + B,
   clean separation)   │    subtle blend)
───────────────────────┼────────────────────
SW: Coupling Heavy     │ SE: Full Entangled
  (dominant coupling   │   (both engines at
   modulation)         │    maximum coupling)
```

**Result:** The pad surface performs a journey from pure character to full entanglement. This architecture is specifically suited to Entangled mood packs and coupled renders from the Oxport fleet render pipeline.

---

#### Architecture 5: Instrument Articulation Corners

For melodic instruments — best for pitched keygroup programs.

```
NW: Legato             │ NE: Staccato
  (long, smooth,       │   (short, detached,
   slow attack)        │    punchy)
───────────────────────┼──────────────────────
SW: Tremolo            │ SE: Accent
  (rhythmic amplitude  │   (hard onset,
   modulation)         │    full velocity push)
```

**Result:** A single pad holds a full articulation vocabulary. For melodic pads in keygroup programs, this removes the need for separate programs per articulation.

---

### 1.3 XPM Implementation for Quad Corners

**Current XPN format status (March 2026):**

Akai has not publicly documented a `<PadCornerAssignment>` or equivalent XML block in the XPM 2.x schema. Scout's research on the MPC community (Reddit r/mpcusers, Akai forum, MPC-Forums.com) found:

- MPCe corner assignment is currently configured **in the MPC UI** by the user after loading a program
- No verified third-party tool has reverse-engineered the corner assignment storage format from saved project files
- The feature is hardware-firmware-level and may store state in `.xpj` project files rather than standalone `.xpm` files

**Rex's implementation path — three-tier approach:**

**Tier 1 (Ship Now): Velocity-Zone Proxy**

Use existing XPM velocity layer architecture to express corner intent without native corner tags. Map corners to velocity zones:

```xml
<!-- Velocity proxy for MPCe quad-corner layout -->
<!-- NW = vel 1-32, NE = vel 33-64, SW = vel 65-96, SE = vel 97-127 -->
<Layer>
  <VelStart>1</VelStart>
  <VelEnd>32</VelEnd>
  <SampleFile>kick_felix_dry.wav</SampleFile>
  <!-- NW: feliX Dry — MPCe corner: upper-left -->
</Layer>
<Layer>
  <VelStart>33</VelStart>
  <VelEnd>64</VelEnd>
  <SampleFile>kick_felix_wet.wav</SampleFile>
  <!-- NE: feliX Processed — MPCe corner: upper-right -->
</Layer>
<Layer>
  <VelStart>65</VelStart>
  <VelEnd>96</VelEnd>
  <SampleFile>kick_oscar_dry.wav</SampleFile>
  <!-- SW: Oscar Dry — MPCe corner: lower-left -->
</Layer>
<Layer>
  <VelStart>97</VelStart>
  <VelEnd>127</VelEnd>
  <SampleFile>kick_oscar_wet.wav</SampleFile>
  <!-- SE: Oscar Processed — MPCe corner: lower-right -->
</Layer>
```

**Outcome for non-MPCe users:** Velocity layers work normally — harder hits play richer variants. The pack functions perfectly on any MPC hardware. MPCe users can additionally reassign corners in their UI to match the documented intent.

**Tier 2 (Future): Native Corner Tags**

When Akai documents or the community reverse-engineers the XPM corner schema, add native corner block generation to Oxport. Proposed tag structure based on Scout's research pattern from existing Akai XML conventions:

```xml
<PadCornerAssignment padIndex="0">
  <Corner id="NW" sample="kick_felix_dry.wav" />
  <Corner id="NE" sample="kick_felix_wet.wav" />
  <Corner id="SW" sample="kick_oscar_dry.wav" />
  <Corner id="SE" sample="kick_oscar_wet.wav" />
</PadCornerAssignment>
```

**Action:** Monitor Akai firmware release notes for MPC 3.8+. Watch MPC-Forums.com for community XPM reverse-engineering threads. Log any confirmed corner field names in `mpc_evolving_capabilities_research.md`.

**Tier 3 (Documentation Supplement):**

Every MPCe-targeted XPN pack ships with a `MPCE_SETUP.md` file inside the ZIP:

```
# MPCe Corner Setup Guide

This pack is designed for MPCe 3D pads. Each pad holds 4 articulations.
Load the program, then assign corners as follows:

Pad 1 (Kick):
  Upper-left  (NW) = kick_felix_dry     [feliX / clinical]
  Upper-right (NE) = kick_felix_wet     [feliX / processed]
  Lower-left  (SW) = kick_oscar_dry     [Oscar / organic]
  Lower-right (SE) = kick_oscar_wet     [Oscar / processed]

Pad 2 (Snare): ...

Recommended XY mapping:
  X-axis → Filter Cutoff (horizontal = brightness)
  Y-axis → Reverb Send (vertical = space depth)
```

---

### 1.4 Oxport Tool: `xpn_mpce_quad_builder.py`

A new Oxport tool that generates MPCe-optimized XPM programs with quad-corner velocity layers. This replaces manual layer assembly for MPCe pack content.

**CLI interface:**

```bash
python xpn_mpce_quad_builder.py \
  --nw samples/felix_dry/ \
  --ne samples/felix_wet/ \
  --sw samples/oscar_dry/ \
  --se samples/oscar_wet/ \
  --output programs/mpce_quad_kit/ \
  --architecture polarity \
  --xy-map "x=filter_cutoff,y=reverb_send"
```

**Arguments:**

| Flag | Type | Description |
|------|------|-------------|
| `--nw` | dir | Samples for NW corner (upper-left) |
| `--ne` | dir | Samples for NE corner (upper-right) |
| `--sw` | dir | Samples for SW corner (lower-left) |
| `--se` | dir | Samples for SE corner (lower-right) |
| `--output` | dir | Output directory for generated XPM + sample manifest |
| `--architecture` | str | One of: `polarity`, `era`, `expression`, `coupling`, `articulation` |
| `--xy-map` | str | Documented XY modulation intent string (written to MPCE_SETUP.md) |
| `--program-name` | str | Program display name (default: derived from output dir) |
| `--pad-count` | int | Number of pads to generate (default: 16, one XPM per group of 16) |
| `--no-velocity-proxy` | flag | Skip velocity layer proxy (for future native corner format) |

**Output structure:**

```
programs/mpce_quad_kit/
  mpce_quad_kit.xpm         ← XPM with 4-layer velocity proxy per pad
  MPCE_SETUP.md             ← Corner assignment guide for MPCe users
  manifest.json             ← Lists all samples, corners, architecture type
  samples/                  ← Flat sample pool (all 4 × N samples)
```

**Internal behavior:**

1. Scan each corner directory, sort samples alphabetically (pad 1 = first file in each dir, pad 2 = second, etc.)
2. Assert all four directories have matching sample counts — error if mismatched
3. For each pad index, generate a `<Pad>` block with 4 `<Layer>` children using velocity proxy ranges
4. Write `<ProgramName>`, `<KeyTrack>True</KeyTrack>`, `<RootNote>0</RootNote>` (mandatory XPM compliance)
5. Embed architecture type + XY map in XPM `<Comment>` field for documentation traceability
6. Write MPCE_SETUP.md from template, populated with per-pad corner filenames

**Error handling:**

- Mismatched sample counts across corners: exit with descriptive error listing counts per corner dir
- Non-WAV files in corner dirs: warn and skip, continue
- Sample file names with spaces: auto-sanitize (replace with underscores) before writing XPM paths

**Location:** `Tools/xpn_mpce_quad_builder.py`

---

### 1.5 Recommended XY Modulation Targets (Per Engine Family)

Documentation for MPCE_SETUP.md templates. These are the designed intent targets — MPCe users configure these manually in their program settings.

| Engine / Content Type | X-Axis Target | Y-Axis Target | Rationale |
|----------------------|---------------|---------------|-----------|
| Drum kit (ONSET) | Pan / Stereo Width | Decay / Tail Length | Horizontal = space, Vertical = envelope |
| Drum kit (OVERWORLD era) | Era crossfade | Filter Cutoff | Horizontal = character era, Vertical = brightness |
| Melodic instrument | Tremolo / Vibrato Depth | Pitch Bend | Horizontal = modulation richness, Vertical = pitch expression |
| Bass (OVERDUB) | Drive / Saturation | Filter Cutoff | Horizontal = grit, Vertical = openness |
| Pad / Atmosphere | Reverb Size | Filter Cutoff | Horizontal = space, Vertical = brightness |
| Coupled render (OPAL+ONSET) | Coupling Depth | Reverb Send | Horizontal = entanglement, Vertical = space |

---

## Section 2: Stems-Aware Loop Pack Design

### 2.1 MPC Stems — What It Does

MPC Stems (standalone hardware, July 2024) uses the zplane Stems Pro algorithm to separate any audio file into up to 4 stems in real time on the device. Expected stem categories: drums, bass, melody, atmosphere (exact labeling confirmed as variable by stem algorithm output, not fixed labels).

Tested hardware: MPC Live III, Force (3.5 update), and all 3.x hardware with sufficient RAM. MPC XL runs it with headroom to spare at 16 GB.

**Key implication:** A full-mix loop at 128 BPM can be imported directly by a user and stem-separated live. XO_OX no longer has to pre-chop everything to be useful. Full loops are first-class content again — with one design constraint: **the mix must be stems-friendly.**

---

### 2.2 Stems-Ready Coupled Render Standard

For any XO_OX coupled render intended for loop packs, define a canonical 4-engine stems configuration:

| Stem Slot | Engine | Frequency Territory | Design Directive |
|-----------|--------|--------------------|--------------------|
| Drums | ONSET | 60–8kHz (percussive) | Keep mix bus clean — no dense pad wash bleeding into drum range |
| Bass | OVERDUB | 30–250Hz dominant | Sub stays below 60Hz; midrange dub elements below 400Hz |
| Melody | OPAL | 300Hz–8kHz (mid-forward) | Granular texture sits above bass; avoid sub-200Hz content |
| Atmosphere | OMBRE | Wide spread, above 1kHz | Spatial diffusion stays out of drum and bass territory |

**Substitution table** — alternative engines per stem slot:

| Stem Slot | Primary | Alt 1 | Alt 2 |
|-----------|---------|-------|-------|
| Drums | ONSET | OBLONG | OCTOPUS |
| Bass | OVERDUB | ORCA | OUTWIT |
| Melody | OPAL | ODYSSEY | ORPHICA |
| Atmosphere | OMBRE | OPAL (long grain) | OHM |

**Design directive for Oxport fleet renders:**

When generating a coupled render for loop pack use, the render spec JSON should include:

```json
{
  "stems_ready": true,
  "stem_assignments": {
    "drums": "ONSET",
    "bass": "OVERDUB",
    "melody": "OPAL",
    "atmosphere": "OMBRE"
  },
  "mix_discipline": "frequency_separated"
}
```

The fleet render automation (`fleet_render_automation_spec.md`) should read this flag and apply per-stem EQ guards at the mix bus level before rendering the full-mix WAV.

---

### 2.3 `xpn_stems_checker.py` — Validation Tool Spec

A quality gate tool that analyzes four WAV files (intended as pre-separated stems or as candidate source mix buses for MPC Stems compatibility) and reports frequency territory separation quality.

**CLI:**

```bash
python xpn_stems_checker.py \
  --drums renders/onset_drums.wav \
  --bass renders/overdub_bass.wav \
  --melody renders/opal_melody.wav \
  --atmosphere renders/ombre_atmos.wav \
  --output reports/stems_quality_report.json \
  --warn-threshold 0.20
```

**Analysis method:**

1. Load all four WAV files via `scipy.io.wavfile` or `soundfile`
2. For each file, compute RMS energy per frequency band using FFT with 1024-point windows
3. Define analysis bands:
   - Sub: 20–60 Hz
   - Low: 60–250 Hz
   - Low-Mid: 250–2000 Hz
   - High-Mid: 2000–8000 Hz
   - High: 8000–20000 Hz
4. Compute energy percentage each file contributes per band (normalized across all four files)
5. Flag any band where two files each contribute > 20% of total energy (bleed warning)
6. Calculate per-file "dominant territory" — the band where it has highest relative contribution
7. Write `stems_quality_report.json`

**Output: `stems_quality_report.json`**

```json
{
  "analysis_date": "2026-03-16",
  "files": {
    "drums": "onset_drums.wav",
    "bass": "overdub_bass.wav",
    "melody": "opal_melody.wav",
    "atmosphere": "ombre_atmos.wav"
  },
  "band_energy_map": {
    "sub_20_60": { "drums": 0.48, "bass": 0.45, "melody": 0.04, "atmosphere": 0.03 },
    "low_60_250": { "drums": 0.31, "bass": 0.55, "melody": 0.09, "atmosphere": 0.05 },
    "low_mid_250_2k": { "drums": 0.28, "bass": 0.22, "melody": 0.35, "atmosphere": 0.15 },
    "high_mid_2k_8k": { "drums": 0.30, "bass": 0.08, "melody": 0.38, "atmosphere": 0.24 },
    "high_8k_20k": { "drums": 0.20, "bass": 0.04, "melody": 0.32, "atmosphere": 0.44 }
  },
  "dominant_territory": {
    "drums": "low_60_250",
    "bass": "low_60_250",
    "melody": "high_mid_2k_8k",
    "atmosphere": "high_8k_20k"
  },
  "bleed_warnings": [
    {
      "band": "low_60_250",
      "files": ["drums", "bass"],
      "overlap_percent": 0.31,
      "severity": "WARNING",
      "message": "Drums and bass share 31% of low band energy — MPC Stems may struggle to separate cleanly"
    }
  ],
  "stems_score": 74,
  "stems_score_label": "GOOD — minor bleed in low band",
  "recommendation": "Apply HPF at 100Hz to drums bus before final render to improve bass/drums separation"
}
```

**Scoring rubric:**

| Score | Label | Bleed Level |
|-------|-------|-------------|
| 90–100 | EXCELLENT | No warnings |
| 75–89 | GOOD | 1 warning, < 25% overlap |
| 60–74 | ACCEPTABLE | 1–2 warnings, < 35% overlap |
| 40–59 | POOR | Multiple warnings, stems will bleed noticeably |
| < 40 | FAIL | Do not ship as stems-ready content |

**Location:** `Tools/xpn_stems_checker.py`

**Integration:** Run as a post-render step in `fleet_render_automation_spec.md` pipeline. A `stems_score < 60` should block the render from being tagged `"stems_ready": true` in its manifest.

---

### 2.4 Stems Pack Format — Recommended Option

**Three options evaluated:**

**Option A: 4 separate XPMs + Full Mix XPM (RECOMMENDED)**

Structure inside the XPN ZIP:
```
PackName/
  Programs/
    PackName_Full_Mix.xpm
    PackName_Drums.xpm
    PackName_Bass.xpm
    PackName_Melody.xpm
    PackName_Atmosphere.xpm
  Samples/
    (all WAV files flat pool)
  STEMS_GUIDE.md
```

**Rationale:** This is the most flexible and MPC-native approach. Users who want to chop the pre-separated stems manually have immediate access. Users who want the full mix can load it and let MPC Stems do the separation live. Five programs = five load options. No format risk — all valid standard XPM programs.

**Option B: Single XPM, 4 layered keygroups on separate MIDI channels**

The MPC XPM format does not natively support "per-layer MIDI channel" in a meaningful way that most users can access intuitively. This would confuse the majority of users and is not standard MPC workflow. Not recommended.

**Option C: Standalone WAVs in `/Stems/` folder only**

No XPM programs — just organized WAV files. This fails the pack usability test: users should be able to load a program and play immediately. WAV-only bundles belong in a supplementary content tier, not the primary pack format.

**Verdict: Option A.** Five XPMs per coupled loop set. Full-mix program listed first in the Programs folder (alphabetical sort advantage). The `STEMS_GUIDE.md` explains the intended stem mapping and recommends when to use MPC Stems live vs. pre-separated stems.

**Oxport flag to add:**

In pack manifest and XPN bundle metadata:

```json
{
  "stems_ready": true,
  "stems_format": "pre_separated_plus_full_mix",
  "stems_score": 82,
  "stem_programs": ["Full_Mix", "Drums", "Bass", "Melody", "Atmosphere"]
}
```

---

## Section 3: Competitive Position

### 3.1 The Opportunity

MPC Live III shipped October 2025. MPC XL shipped January 2026. As of March 2026, the MPCe pad has been available on shipping hardware for **five months** and no expansion creator has designed packs purpose-built for it.

Scout's research across the main MPC expansion marketplaces (Loopmasters, MPC-Expansion.com, Splice, MSXII, Touch Loops) found:

- Zero packs with documented quad-corner articulation design
- Zero packs with XY modulation targets specified in documentation
- Zero packs with explicit stems-separation quality certification
- The existing top creators (Kryptic Samples, Touch Loops, Capsun, MSXII) are all still shipping standard-format XPN packs designed for MPC Live II workflows

The early adopter window is open. The MPC XL's NAMM 2026 launch drove a second wave of MPCe hardware purchases from established producers. These users are actively looking for content that uses the hardware they just spent €2,799 on.

**Timeline pressure:** When major expansion houses discover this gap, they will move fast. XO_OX advantage window estimate: 6–12 months before well-resourced competitors ship MPCe-designed content. The Oxport tooling (`xpn_mpce_quad_builder.py`) is XO_OX's proprietary moat — it enables consistent quad-corner design at a scale that manual pack-by-pack design cannot match.

---

### 3.2 The "MPCe Exclusive" Badge

**Definition:** An "MPCe Exclusive" badge on an XO_OX pack signals that the pack was **designed from the ground up for MPCe 3D pads**, not retrofitted. Specifically:

A pack earns the badge if it meets all three criteria:
1. **Quad-corner design**: every drum or one-shot pad has four intentionally distinct corner variants following one of the five canonical architectures
2. **XY mapping documentation**: every program ships with a `MPCE_SETUP.md` specifying designed X and Y axis targets
3. **Stems compliance** (for loop packs): `stems_score >= 75` verified by `xpn_stems_checker.py`

**Badge usage:**
- Pack listing hero image: small MPCe logo + "Designed for MPCe" text badge
- Pack description copy: dedicated paragraph explaining the quad-corner architecture type used
- XO-OX.org packs page: filter toggle "MPCe Native" to surface these packs

**What the badge is NOT:** A claim that the pack requires MPCe hardware. Standard velocity layers work for all MPC users. The badge communicates design intent and rewards users who have the hardware.

---

### 3.3 Marketing Language Framework

**Do not say:** "Compatible with MPCe" — this is a low bar (everything is technically "compatible")

**Say instead:**
- "Designed for MPCe 3D pads — each pad holds four characters"
- "feliX-Oscar polarity on a single pad surface"
- "Move your finger across the pad and move through the sound"
- "The XY axis is the performance. The pad is the instrument."
- "Four corners. One intention."

**The XO_OX differentiation claim:** "XO_OX is the only expansion creator that designs content around XO_OX's own feliX-Oscar polarity axis — meaning the quad-corner layout has philosophical coherence, not just four random articulations crammed onto one pad."

This is a claim no competitor can make because they don't have the feliX-Oscar framework.

---

### 3.4 Engine Pairs That Map Best to Quad-Corner feliX-Oscar

The feliX-Oscar polarity architecture (Architecture 1) requires four render variants: feliX Dry, feliX Processed, Oscar Dry, Oscar Processed. These engine pairings produce the most distinct and useful corner contrast:

| Quad Pack Theme | feliX Engine | Oscar Engine | Best Corner Architecture |
|-----------------|-------------|-------------|--------------------------|
| Drums / Percussion | ONSET (clinical) | ONSET (warm, OBLONG tuning) | feliX-Oscar Polarity |
| Bass | OVERDUB | OVERDUB (warmer tape settings) | feliX-Oscar Polarity |
| Granular Texture | OPAL | OHM | feliX-Oscar Polarity |
| Ambient Pad | ODYSSEY | OMBRE | feliX-Oscar Polarity |
| Metallic / Bell | OVERWORLD (NES era) | OVERWORLD (YM2612 era) | Era Corners |
| Brass | OTTONI | OBBLIGATO | Articulation Corners |
| Voice-adjacent | ORPHICA | OHM | feliX-Oscar Polarity |
| Rhythmic Synth | OBLONG | OCTOPUS | Coupling State Corners |

**Priority build order for MPCe packs:**

1. **ONSET quad kit** — most immediate value for drum producers; feliX-Oscar drum quad is the strongest first impression
2. **OPAL + ODYSSEY melodic quad** — granular + wavetable corner contrast is highly playable
3. **OVERDUB + ORCA bass quad** — sub energy contrast makes this a production powerhouse
4. **OHM + OMBRE atmosphere quad** — ambient producers are the most vocal MPCe adopters

---

### 3.5 Competitive Analysis — Why Now

| Factor | Status |
|--------|--------|
| MPC Live III hardware in market | 5 months (Oct 2025 → Mar 2026) |
| MPC XL hardware in market | 2 months (Jan 2026 → Mar 2026) |
| MPCe-designed packs from any creator | 0 confirmed as of Mar 2026 |
| MPC Stems standalone | 8 months (Jul 2024 → Mar 2026) |
| Stems-certified pack content from any creator | 0 confirmed as of Mar 2026 |
| XO_OX Oxport tooling readiness | Spec complete; build sprint 1 week |
| Nearest competitor tooling equivalent | None known |

**Scout's read on the window:** The major expansion houses (Loopmasters, MSXII, Touch Loops) have large catalogs but slow design-to-publish pipelines — typically 3–6 months from concept to marketplace listing. A boutique creator with a clear design philosophy and existing tooling infrastructure can ship before they have internal approval to start the project.

XO_OX's advantage is not just being first — it's being first with a **coherent design language** that makes every future MPCe pack part of a recognizable family. Once producers learn the feliX-Oscar quad layout, every new XO_OX MPCe pack is immediately intuitive.

---

## Section 4: Implementation Roadmap

### Phase 1 — Tooling (Week of 2026-03-17)

| Task | Owner | Output |
|------|-------|--------|
| Build `xpn_mpce_quad_builder.py` | Rex | `Tools/xpn_mpce_quad_builder.py` |
| Build `xpn_stems_checker.py` | Rex | `Tools/xpn_stems_checker.py` |
| Write MPCE_SETUP.md template | Rex | `Tools/templates/MPCE_SETUP.md` |
| Add stems quality gate to fleet render pipeline | Rex | Update `fleet_render_automation_spec.md` |

### Phase 2 — First MPCe Pack (Week of 2026-03-24)

| Task | Owner | Output |
|------|-------|--------|
| Render ONSET feliX-Oscar quad variants (16 pads × 4 corners = 64 samples) | Sound design | `renders/onset_mpce_quad/` |
| Run quad builder, verify XPM output | Rex | `programs/ONSET_MPCe_Quad.xpm` |
| Run stems checker on ONSET + OVERDUB coupled loop | Rex | `reports/onset_overdub_stems.json` |
| Write MPCE_SETUP.md for first pack | Scout | Pack documentation |
| Package as XPN bundle | Rex | First MPCe-badged pack |

### Phase 3 — Site + Marketing (Week of 2026-03-31)

| Task | Owner | Output |
|------|-------|--------|
| Write "Designed for MPCe" landing section on XO-OX.org/packs | Scout | Site copy |
| Create MPCe badge asset | Design | Badge image |
| Draft pack listing copy using framework language (Section 3.3) | Scout | Listing copy |
| Field Guide post: "Why We Designed the Quad-Corner Pack" | Scout | Blog post |

---

## Appendix A: XPM Compliance Checklist for MPCe Packs

All MPCe-targeted XPMs must pass standard XPN compliance in addition to quad-corner requirements:

- [ ] `<KeyTrack>True</KeyTrack>` set on all keygroup programs
- [ ] `<RootNote>0</RootNote>` on all layers
- [ ] Empty layer `<VelStart>0</VelStart>` (prevents ghost triggering)
- [ ] All four velocity proxy layers present per pad (VelStart: 1, 33, 65, 97)
- [ ] Sample filenames match exactly (case-sensitive)
- [ ] No spaces in sample filenames
- [ ] `MPCE_SETUP.md` present in pack root
- [ ] Architecture type documented in `manifest.json`
- [ ] Stems score >= 75 if `"stems_ready": true` claimed

---

## Appendix B: Q-Link Assignments for MPCe Packs

MPC XL's OLED Q-Link displays make named Q-Link assignments visible on hardware. All MPCe packs targeting MPC XL should assign Q-Links with meaningful labels. Recommended standard assignment per program type:

**Drum program (quad-corner kit):**

| Q-Link | Assignment | Label |
|--------|-----------|-------|
| Q1 | Filter Cutoff (Master) | OPEN |
| Q2 | Reverb Send (Master) | SPACE |
| Q3 | Attack (Master) | SHAPE |
| Q4 | Swing Amount | FEEL |

**Melodic program (keygroup):**

| Q-Link | Assignment | Label |
|--------|-----------|-------|
| Q1 | Filter Cutoff | BRIGHT |
| Q2 | Reverb Wet/Dry | ROOM |
| Q3 | Pitch Bend Range | BEND |
| Q4 | LFO Rate | PULSE |

Label convention: single word, all caps, ≤ 8 characters (OLED display width).

---

*Spec authored by Scout + Rex, XO_OX Android Team*
*March 16, 2026 — Feeds into Collection Build Week 2026-03-17*
