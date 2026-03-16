# XPN Hardware Compatibility Matrix — R&D Spec
## Scout + Rex Research Document — March 2026

*Scout: hardware research, community intelligence, firmware change tracking*
*Rex: XPN format compliance, pack design constraints, Oxport tooling implications*

> **Label key**: [VERIFIED] = confirmed in official documentation or release notes within knowledge cutoff.
> [APPROXIMATE] = community-reported, forum-confirmed, or secondary source.
> [SPECULATIVE] = logical inference from hardware specs or release trajectory.
>
> Knowledge cutoff: August 2025. MPC Live III (October 2025) and MPC XL (January 2026) are post-cutoff;
> details sourced from post-cutoff community intelligence compiled by Kai android team.

---

## Executive Summary

XO_OX XPN packs are designed to their most expressive on MPC Live III and MPC XL with MPCe 3D pads,
but the installed base is overwhelmingly older hardware. As of March 2026, the dominant field units are
MPC One, MPC Live II, and Force — none of which have 3D pads. A compatibility-first pack design strategy
ensures every pack delivers full value on the widest installed base while unlocking progressive
enhancement on MPCe hardware. This spec defines the hardware matrix, feature availability table,
RAM constraints, and actionable pack design rules for cross-hardware excellence.

---

## Section 1: MPC Hardware Matrix

### 1.1 Core Hardware Specifications

| Feature | MPC One | MPC Live II | MPC Live III (MPCe) | MPC X | MPC Key 37 | MPC Key 61 | Force | MPC Studio |
|---------|---------|-------------|---------------------|-------|------------|------------|-------|------------|
| **Release year** | 2019 | 2021 | Oct 2025 | 2018 | 2022 | 2022 | 2018 | 2021 |
| **Pads** | 16 | 16 | 16 MPCe | 16 | 16 | 16 | 64 | 16 |
| **MPCe 3D pads** | No | No | Yes | No | No | No | No | No |
| **Screen size** | 7" | 7" | 7" | 10.1" | 7" | 7" | 7" | None (DAW only) |
| **Standalone** | Yes | Yes | Yes | Yes | Yes | Yes | Yes | No |
| **Keyboard** | No | No | No | No | 37-key | 61-key | No | No |
| **Expansion slots** | 1× USB-A | 1× USB-A + 1× SD | 1× USB-A + 1× SD | 2× USB-A + 1× SD | 1× USB-A | 1× USB-A | 2× USB-A + SD | Host only |
| **Sample RAM (advertised)** | 2GB | 2GB | 4GB | 2GB | 2GB | 2GB | 2GB | Host RAM |
| **Sample RAM (practical)** | ~1.8GB | ~1.8GB | ~3.6GB | ~1.8GB | ~1.8GB | ~1.8GB | ~1.8GB | Host RAM |
| **Onboard storage** | 16GB | 16GB | 32GB | 16GB | 16GB | 16GB | 16GB | None |
| **WiFi** | Yes | Yes | Yes | No | Yes | Yes | Yes | N/A |
| **Battery** | No | Yes | Yes | No | No | No | No | No |
| **Instrument inputs** | 2× 1/4" TS | 2× 1/4" TS | 2× 1/4" TS | 2× 1/4" TS | No | No | 2× 1/4" TS | No |
| **CV/gate** | No | No | No | Yes (4 out) | No | No | No | No |

**[APPROXIMATE]** MPC Live III (MPCe) practical RAM ceiling ~3.6GB after OS overhead. The 4GB figure
is LPDDR4 total — firmware reserves approximately 400MB for OS + engine. Rex recommends targeting
3.2GB max for packs that claim "MPCe optimized" status.

**[VERIFIED]** MPC Studio is DAW-controller-only. It has no standalone audio engine. All sample
playback and program logic runs in MPC Software on the host computer. MPC Studio users experience
packs identically to MPC Desktop software users, subject to host RAM limits.

### 1.2 Force-Specific Differences

Force's 64-pad grid (8×8) is the largest departure from standard MPC pad layout. XPN drum programs
designed for 16 pads load and function correctly on Force — MPC Software maps the 16-pad kit to
the upper-left quadrant of the Force grid by default. [VERIFIED]

Keygroup programs behave identically across Force and MPC units. Force does not support MPCe 3D pad
features — it uses the same pressure-only pad sensing as pre-MPCe hardware.

---

## Section 2: MPCe 3D Pad Feature Availability

### 2.1 Hardware Support Table

| Feature | MPC One | MPC Live II | MPC Live III | MPC X | MPC Key 37/61 | Force | MPC Studio |
|---------|---------|-------------|--------------|-------|----------------|-------|------------|
| **XY pad position sensing** | No | No | Yes | No | No | No | No |
| **Quad-corner sample routing** | No | No | Yes | No | No | No | No |
| **Continuous XY modulation** | No | No | Yes | No | No | No | No |
| **Aftertouch (channel pressure)** | Yes | Yes | Yes | Yes | Yes | Yes | Yes |
| **Polyphonic aftertouch** | No | No | Yes | No | No | No | No |
| **Velocity** | Yes | Yes | Yes | Yes | Yes | Yes | Yes |
| **Note repeat** | Yes | Yes | Yes | Yes | Yes | Yes | Yes |
| **Full level** | Yes | Yes | Yes | Yes | Yes | Yes | Yes |

**[APPROXIMATE]** MPC XL (January 2026, post-cutoff) is expected to include MPCe 3D pads given its
flagship positioning — treat as MPCe-compatible pending official confirmation. Do not ship packs
with hard MPCe requirements based on MPC XL alone until VERIFIED.

### 2.2 Graceful Fallback on Non-MPCe Hardware

XPN packs that use MPCe quad-corner architecture must fall back cleanly on all pre-MPCe hardware.
The fallback behavior is defined by MPC firmware, not by the XPN format itself:

| MPCe Design Element | Non-MPCe Fallback Behavior |
|---------------------|---------------------------|
| NW corner sample | Full pad triggers NW sample only |
| NE/SW/SE corner samples | Ignored (pad plays NW sample) |
| XY continuous morphing | Pad plays at fixed center position |
| Continuous parameter modulation via XY | Aftertouch (Y-axis) substitutes where mapped |
| Polyphonic aftertouch per-pad | Channel pressure applied to all active notes |

**Rex design rule:** Always design the NW corner (default fallback position) as the strongest,
most complete version of the pad's sound. On non-MPCe hardware, NW is the only thing users hear.
If NW is a sparse or stripped-down variant, the pack degrades badly for 95% of the installed base.

### 2.3 XO_OX Quad-Corner Kit Tiers

Three XPN kit tiers for quad-corner design, based on fallback quality:

**Tier A — Full MPCe (NW anchor)**: NW = complete mix. NE = feliX processed variant. SW = Oscar warm
variant. SE = Oscar processed variant. All four corners are full sounds. Non-MPCe users get NW, which
is already production-ready. [RECOMMENDED for all XO_OX packs]

**Tier B — MPCe Expansion Layer**: Existing 16-pad kit with a second "MPCe extended" program in the
same pack that surfaces quad-corner architecture. Non-MPCe users load the standard program. MPCe
users load the extended program. Two separate XPM files in the same XPN bundle.

**Tier C — MPCe Exclusive (not recommended for v1)**: Pack designed primarily for MPCe with no
meaningful fallback. Acceptable only for future "MPCe Collection" product line. Not appropriate for
general catalog.

---

## Section 3: Sample RAM Limits and Pack Design Constraints

### 3.1 RAM Limits by Hardware Tier

| Hardware Tier | Practical Sample RAM | Recommended Pack Budget | Headroom for User Projects |
|---------------|---------------------|------------------------|---------------------------|
| Legacy (MPC One, Live II, X, Key, Force) | ~1.8GB | 600MB max | 1.2GB for user samples |
| MPCe (MPC Live III, MPC XL est.) | ~3.6GB | 1.2GB max | 2.4GB for user projects |
| MPC Studio / Desktop | Host RAM (typically 8–32GB) | No practical limit | — |

**600MB pack budget rationale**: XO_OX packs are not used in isolation. A producer may load a drum
pack alongside a keygroup pack and have a project with existing audio clips. Consuming more than
600MB per pack risks RAM exhaustion mid-session on legacy hardware, causing voice-stealing or sample
drop. This is the ceiling for any single XPN bundle targeting the full installed base.

### 3.2 Sample Spec Trade-offs

| Spec Choice | RAM Impact | Quality Impact | Recommendation |
|-------------|-----------|----------------|---------------|
| 24-bit vs 16-bit WAV | +50% RAM for 24-bit | Minimal difference post-conversion | **16-bit for drums, 24-bit for keygroups** |
| 48kHz vs 44.1kHz | +9% RAM for 48kHz | No audible difference on MPC hardware | **44.1kHz for all samples** |
| Stereo vs mono drum hits | 2× RAM for stereo | Useful for hi-hats, cymbals | **Mono for kicks/snares, stereo for cymbals** |
| Long vs trimmed samples | Proportional | — | **Trim to content + 100ms tail** |
| Velocity layers × round-robins | Multiplicative | Realism vs. RAM | **4 velocities × 2 round-robins = 8 samples per voice (max)** |

### 3.3 WAV Count Guidance

For a 16-pad drum program at 4 velocity layers × 2 round-robins, targeting 16-bit/44.1kHz mono:

- Average kick sample: ~150KB (trimmed, ~0.7s)
- Average snare/clap: ~100KB
- Average cymbal (stereo): ~400KB
- Per-pad average: ~175KB × 8 = 1.4MB per pad
- 16 pads total: ~22MB per drum program

A full XPN bundle of 8 drum programs × 22MB = ~176MB — well within the 600MB budget, leaving room
for a companion keygroup program.

For a keygroup program (88 zones, 4 velocity layers, 24-bit/44.1kHz):

- Average multi-sampled key: ~500KB
- 88 zones × 4 layers = 352 samples × 500KB = ~176MB per instrument
- Cap at 2 instruments per pack if including keygroups alongside drums

---

## Section 4: Screen Size Implications

### 4.1 Screen Inventory

| Hardware | Screen | Resolution | Touch |
|----------|--------|------------|-------|
| MPC One | 7" | 800×480 | Yes |
| MPC Live II | 7" | 800×480 | Yes |
| MPC Live III | 7" | 800×480 | Yes |
| MPC X | 10.1" | 1280×800 | Yes |
| MPC Key 37 | 7" | 800×480 | Yes |
| MPC Key 61 | 7" | 800×480 | Yes |
| Force | 7" | 800×480 | Yes |
| MPC Studio | None | — | — |

**[VERIFIED]** MPC X is the only current-generation unit with a larger screen. All others share the
same 7-inch 800×480 panel. MPC X users get more visible list rows and a wider pad grid view, but
the XPN format content (program names, sample names, pad labels) renders identically.

### 4.2 Name Length Constraints

Pack design decisions that are screen-sensitive:

**Program names**: MPC displays approximately 22–24 characters before truncation in the program list
at 7". MPC X shows approximately 30. **Rule: keep program names ≤ 20 characters.**

**Sample/layer names**: The sample browser truncates at approximately 28 characters on a 7" screen.
**Rule: sample file names ≤ 24 characters** (excluding extension). Prefer descriptive stems over
long paths (e.g., `kick_hard_01.wav` not `XO_OX_ONSET_Kick_Velocity_Heavy_Round_Robin_01.wav`).

**Pad labels (in drum programs)**: MPC renders pad labels at 4–6 characters visible in default view.
**Rule: pad labels ≤ 6 characters** (e.g., `KICK`, `SNARE`, `CH`, `OH`, `CLAP`).

**Cover art**: MPC renders expansion pack cover art at 150×150px in the browser. The full-size art
(512×512 or 1024×1024) is displayed on MPC X. **Rule: design cover art to read clearly at 150×150.**
High-contrast logo mark + one legible word. XO_OX artwork template satisfies this by default.

### 4.3 MPC Studio / No-Screen Mode

MPC Studio users interact entirely via MPC Desktop software on a computer monitor. Screen size rules
are irrelevant for Studio-only use. However, since packs must work everywhere, the 7" constraints
remain the binding standard.

---

## Section 5: Standalone vs. DAW Controller Mode

### 5.1 Feature Availability Table

| Feature | Standalone | DAW Controller (MPC as plugin) | MPC Studio (DAW only) |
|---------|------------|--------------------------------|----------------------|
| XPN program loading | Full | Full | Full |
| Sample playback | Full | Full (via MPC plugin) | Full (via MPC plugin) |
| Q-Link assignments | Full | Partial — Q-Links may map to DAW automation | Full |
| Choke groups | Full | Full | Full |
| Note repeat / arp | Full | Full | Full |
| Stems separation | Full | Not available | Not available |
| MPCe 3D pad parameters | Full | Partial — XY data transmitted as MIDI CC | Partial |
| WiFi sample sync | Full | N/A | N/A |
| Expansion manager | Full | N/A | Via Desktop app |
| MIDI output from pads | Full | Full | Full |

**[VERIFIED]** MPC Stems (AI stem separation introduced MPC 3.4) is a standalone-only feature.
It requires the Akai AI processing pipeline running on the device. In DAW controller or MPC Desktop
mode, Stems is not accessible. XPN packs that include stems-aware content (e.g., drum programs
tuned to work alongside separated stems) should document this as standalone-recommended.

**[APPROXIMATE]** In DAW controller mode, Q-Link assignments defined in XPN programs are loaded
but the physical Q-Links may be intercepted by the DAW for automation. Ableton Live, Logic Pro,
and FL Studio all handle this differently. XO_OX packs should define Q-Link assignments that map
logically to MIDI CC (filter, reverb, etc.) so that even if the DAW intercepts them, the mapping
is musically intuitive.

### 5.2 Plugin Controller Workflow Notes

When MPC is used as a plugin instrument (MPC Plugin in a DAW), the XPN program is loaded inside the
plugin window. The main DAW project timeline drives tempo for note repeat and arp. BPM-synced effects
within programs will lock to DAW tempo, which is usually desired. No design changes needed for this —
it is the expected behavior.

XPN packs do NOT need separate "controller mode" variants. The same pack file works in all modes.
Capability gaps are hardware/firmware-mediated and invisible to the pack format.

---

## Section 6: iOS MPC App Compatibility

### 6.1 App Inventory

| App | Platform | XPN Support | Notes |
|-----|----------|-------------|-------|
| MPC for iOS | iOS / iPadOS | **Yes — full** [APPROXIMATE] | AUv3 + standalone; shares XPN format with hardware |
| iMPC Pro 2 | iOS | **No** [VERIFIED] | Legacy app, different proprietary format |
| MPC Beats | macOS / Windows | **Yes — full** [VERIFIED] | Free DAW companion, same format as MPC Desktop |

**[APPROXIMATE]** MPC for iOS (the current Akai app, not the legacy iMPC Pro series) uses the same
XPN expansion format as MPC hardware. Packs installed via the expansion manager on hardware can be
transferred to iOS via cloud sync or direct file copy. Sample RAM on iPad is the constraint — iPads
typically have 4–8GB RAM, with iOS reserving a larger portion for the OS than standalone MPC units.

**Practical iOS RAM budget**: Target 400MB per pack for comfortable iOS compatibility. Heavy packs
(up to 600MB) may work but push older iPads (A12 chip, 3GB RAM) to the edge.

**iMPC Pro 2 note**: iMPC Pro 2 is a legacy product with its own sample format. XO_OX does not
target iMPC Pro 2. If users request iMPC Pro 2 compatibility, the answer is: install MPC for iOS
instead, which loads XPN packs natively.

### 6.2 iOS-Specific Considerations

- **AUv3 hosting**: MPC for iOS can load XOmnibus as an AUv3 instrument. This is a separate feature
  from XPN pack loading — XPN packs provide samples/programs, XOmnibus AUv3 provides the synth engine.
  Both can coexist in an iOS project.
- **File access**: XPN pack installation on iOS goes through the Files app or cloud provider.
  Pack archive (.xpn ZIP) must be well-formed with correct directory structure — sloppy path handling
  that works on desktop may break iOS sandbox extraction.
- **Sample rate**: iOS audio interfaces commonly operate at 44.1kHz or 48kHz. XPN samples at 44.1kHz
  are safe. iOS will SRC 48kHz samples cleanly; no special action needed.

---

## Section 7: Firmware Version Requirements

### 7.1 Feature → Minimum Firmware Table

| XO_OX Feature Used | Minimum MPC OS | Notes |
|--------------------|----------------|-------|
| Basic drum program + keygroup | **MPC 2.0** | Universal, all hardware |
| Q-Link assignments in XPM | **MPC 2.6** [APPROXIMATE] | Q-Link metadata in program XML |
| Choke groups | **MPC 2.0** | `ChokeGroup` XML attribute, universal |
| CycleType round-robin | **MPC 2.4** [APPROXIMATE] | `CycleType=Cycle` in XPM layer |
| Expansion manifest (manifest.json) | **MPC 2.6** [APPROXIMATE] | Pack discoverable in expansion browser |
| PadNoteMap / PadGroupMap | **MPC 2.7** [APPROXIMATE] | Advanced pad routing in drum programs |
| MPCe quad-corner (XY routing) | **MPC 3.6** [SPECULATIVE] | Requires MPCe firmware + hardware |
| MPC Stems integration | **MPC 3.4** [VERIFIED] | Standalone only |
| VST3 plugin programs | **MPC 3.5** [VERIFIED] | Desktop/controller only, not standalone |
| Keygroup Synth Engine | **MPC 3.5** [VERIFIED] | Desktop/controller; standalone TBD |
| XPN ZIP packaging | **MPC 2.6** [APPROXIMATE] | Compressed expansion bundle |

**[APPROXIMATE] MPC 2.9 is the last widely-installed version on older hardware (MPC One, X, Live II).**
Users who have not updated may be on 2.9 or earlier. XO_OX packs should not require features that
shipped after MPC 2.9 unless explicitly marked as "MPC 3.x required" in product listing.

**Recommended baseline target for all XO_OX packs: MPC OS 2.9+.**
This covers the installed base as of March 2026 and includes choke groups, round-robin, Q-Link,
and expansion manifests.

### 7.2 Firmware Update Friction

Not all users update firmware. Known reasons for staying on older versions:
- Workflow disruption fear (project compatibility warnings)
- MPC 3.x project incompatibility with MPC 2.x (documented breaking change)
- Older hardware (MPC X, original MPC Live) update cautiousness

**Design implication**: Test all XPN packs on MPC OS 2.9 as the baseline. Features that silently
degrade on older firmware (e.g., Q-Link assignments ignored) are acceptable; features that cause
load errors or corrupted programs are not.

---

## Section 8: Pack Design Recommendations — Cross-Hardware Field Guide

### 8.1 The Compatibility Hierarchy

Design to this hardware tier order, from most constrained to most capable:

1. **Tier 0 — MPC OS 2.9, legacy hardware, 1.8GB RAM** (MPC One/Live II/X/Force/Key)
   The baseline. All packs must load, play correctly, and sound production-ready at this tier.

2. **Tier 1 — MPC OS 3.x, same hardware, 1.8GB RAM**
   Q-Link and manifest features. No new sample content required — just firmware-enabled metadata.

3. **Tier 2 — MPC OS 3.x, iOS / MPC Desktop**
   DAW controller + iOS. Same content, check Q-Link CC mapping is DAW-friendly.

4. **Tier 3 — MPCe hardware (MPC Live III, MPC XL est.), MPC OS 3.6+**
   Quad-corner architecture, polyphonic aftertouch, continuous XY modulation. Progressive enhancement.

### 8.2 Sample Specification Rules (Non-Negotiable)

```
Format:     WAV (PCM, uncompressed)
Bit depth:  16-bit for drums | 24-bit for keygroup melodic samples
Sample rate: 44.1kHz (never 48kHz — no benefit, +9% RAM, some older firmware SRC artifacts)
Channels:   Mono for kicks/snares/claps/toms | Stereo for cymbals/overheads/ambient
Max length: 8 seconds for one-shot hits | No limit for keygroup sustain loops
Trim:       Silence before first transient ≤ 5ms | Tail fade after content + 100ms
Normalization: -0.3 dBFS peak (leave headroom for MPC internal limiting)
File naming: Lowercase, no spaces, max 24 chars + extension (e.g., kick_hard_01.wav)
```

### 8.3 Program Configuration Rules

```
KeyTrack:          True  (required — samples transpose across keygroup zones)
RootNote:          0     (MPC auto-detect; never hardcode unless sample is a fixed pitch)
Empty layer VelStart: 0  (prevents ghost triggering on empty velocity layers)
ChokeGroup:        Assign per-voice type (e.g., hi-hats on group 1, choke each other)
PadName:           ≤ 6 characters
ProgramName:       ≤ 20 characters
QLink assignments: 4 per program, mapped to musically meaningful parameters
                   Suggested default: Q1=Filter Cutoff, Q2=Attack, Q3=Reverb Send, Q4=Tune
```

### 8.4 RAM Budget Enforcement

Run the following check before finalizing any XPN bundle:

```
Total WAV content:        ≤ 600MB (legacy hardware tier)
Largest single program:   ≤ 200MB
WAV files per program:    ≤ 256 (MPC program XML limit [APPROXIMATE])
Keygroup zones:           ≤ 128 per program
Velocity layers per zone: ≤ 8 (4 recommended for balance)
Round-robin count:        ≤ 4 per velocity layer
```

### 8.5 MPCe Progressive Enhancement Checklist

For packs that include MPCe quad-corner architecture:

- [ ] NW corner is a complete, production-ready sound (non-MPCe default)
- [ ] All four corners are full sounds, not stripped variants
- [ ] XPN bundle includes both a standard 16-pad program and an MPCe extended program
- [ ] Continuous XY modulation targets documented in pack notes (e.g., "X axis morphs timbre")
- [ ] Fallback tested on MPC One / Live II before shipping
- [ ] Pack listing clearly states "MPCe Enhanced — Live III / XL" (not required, just optimized)

### 8.6 iOS Compatibility Additions

- [ ] Pack total size ≤ 400MB (iOS RAM headroom on A12/A14 iPads)
- [ ] No spaces in directory paths within the XPN ZIP
- [ ] manifest.json uses forward slashes only (backslash breaks iOS extraction)
- [ ] Cover art file ≤ 1MB (iOS sandbox extraction is slower with large assets)

### 8.7 Q-Link Best Practices for DAW Controller Mode

Q-Link assignments should be mappable as MIDI CC in DAW controller mode:

| Q-Link | Suggested Target | MIDI CC (default) | Rationale |
|--------|-----------------|-------------------|-----------|
| Q1 | Filter Cutoff | CC 74 | Universal filter CC |
| Q2 | Attack Time | CC 73 | Standard envelope CC |
| Q3 | Reverb Send | CC 91 | Standard reverb CC |
| Q4 | Program-specific | CC 72/75/76/77 | Engine character control |

Avoid assigning Q-Links to parameters that have no meaningful MIDI CC analog (e.g., round-robin
reset, sample swap) — these silently no-op in DAW controller mode and confuse users.

---

## Section 9: Known Edge Cases and Hazards

### 9.1 Force 64-Pad Grid

Force loads 16-pad drum programs into the upper-left quadrant. This is the expected behavior and
requires no special handling. However, if a drum program is designed with performance in mind
(e.g., left-hand / right-hand layout), the Force quadrant placement may feel awkward. Consider
including a Force-optimized layout variant for flagship drum packs (all 16 pads remapped to
Force's preferred upper-left quadrant zone).

### 9.2 MPC Key 37/61 Keyboard Mapping

MPC Key units have a physical keyboard. Keygroup programs respond to both the keyboard and the 16
pads. Drum programs can be played from the keyboard using the chromatic mapping. No special XPN
handling needed — the firmware manages this. However, keygroup programs should define a sensible
root note and velocity curve since Key users may prefer keyboard feel over pad feel.

### 9.3 MPC X Large Screen

MPC X users can see more of the sample browser and program list at once. Pack with many programs
(10+) benefit from careful naming to take advantage of the larger view — logical alphabetical
ordering or numeric prefixes allow faster navigation.

### 9.4 MPC Studio No-Standalone Mode

MPC Studio is controller-only. Standalone-only features (Stems, WiFi sync) are permanently
unavailable. Studio users are by definition DAW-integrated; pack design rules for DAW controller
mode apply. No pack should be marketed with standalone features as a selling point to Studio users.

### 9.5 MPC 3.x Project Incompatibility

Projects saved in MPC 3.x cannot be opened in MPC 2.x. XPN packs (expansion content) are not
project files and are NOT affected by this incompatibility. A pack installed on MPC 2.9 hardware
will load into a MPC 2.9 project; the same pack installed on MPC 3.x hardware will load into a
MPC 3.x project. No cross-version contamination.

---

## Open Questions (Research Needed)

1. **MPC XL MPCe confirmation**: Verify whether MPC XL (Jan 2026) has full MPCe 3D pad sensing
   or a subset. Official specs needed before shipping MPCe-targeted content marketed as XL-compatible.

2. **MPC 3.6 firmware features**: Confirm what new XPN format capabilities (if any) shipped in
   MPC 3.6/3.7. Specifically: are MPCe quad-corner assignments formally supported in XPM XML,
   or handled by a new program type?

3. **iOS RAM limits by device**: Build tested RAM ceiling for MPC for iOS on A12 / A15 / M1 / M2
   iPads. The 400MB guideline is conservative — actual ceiling may be higher on M-series iPad.

4. **Force firmware parity**: Confirm whether Force received the MPC 3.5 / 3.6 firmware updates
   or remains on a separate branch. If Force diverged, it may lack features that MPC hardware
   received post-3.4.

5. **MPC Beats desktop RAM behavior**: Confirm whether MPC Beats (free desktop version) enforces
   any artificial RAM or program count limits compared to full MPC Software.

---

*Spec status: Research draft — March 2026. Verify open questions before shipping MPCe-specific packs.*
*Cross-reference: `mpce_native_pack_design.md`, `mpc_evolving_capabilities_research.md`, `xpn-tools.md`*
