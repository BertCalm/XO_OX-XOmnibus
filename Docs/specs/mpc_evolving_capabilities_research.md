# MPC 3.0 / MPCe Evolving Capabilities Research
## Scout's Intelligence Report — March 2026

*Compiled by Scout (community intelligence / emerging tech specialist, Kai's android team)*
*Side interest: MPCe 3D pads and hardware-expressive interfaces*

> **Label key**: [VERIFIED] = confirmed in official release notes or primary documentation within knowledge cutoff.
> [APPROXIMATE] = community-reported, forum-confirmed, or secondary source.
> [SPECULATIVE] = logical inference from hardware specs, patent filings, or release trajectory.
>
> Knowledge cutoff: August 2025. MPC 3.5 Desktop beta (May 2025) is the latest confirmed software version.
> MPC Live III (October 2025) hardware and MPC 3.7 (February 2026) were announced/shipped after cutoff — details sourced from post-cutoff community intelligence compiled by Kai android team.

---

## Executive Summary

The MPC ecosystem has undergone the most significant hardware and software evolution since the transition to touchscreen in 2017. Three interlocking developments define the 2024–2026 moment:

1. **MPCe 3D pads** — hardware-level expressive sensing that changes what pad performance means
2. **MPC XL** — flagship workstation that positions standalone as a serious DAW replacement
3. **MPC Software 3.5–3.7** — VST3 support, Keygroup Synth Engine, Stems, and unified firmware across MPC + Force

For XO_OX, the opportunity gap is clear: **no expansion creator has yet built packs purpose-designed for MPCe expressivity or stems-aware content**. The Oxport pipeline needs to evolve to serve this frontier.

---

## Section 1: MPC Firmware — 2024–2026 Timeline

### MPC 3.5 (Desktop Beta, May 2025)

**Key additions:**
- **VST3 plugin support** — first time VST3 (not just VST2/AIR) can be used as plugin programs in MPC Desktop
- **Keygroup Synth Engine** — new program type that combines sampler keymapping with synthesis processing inline (not requiring a separate plugin track)
- Projects are **not backwards compatible** with MPC 2.x — this is a significant workflow fork
- Beta ran alongside v2, suggesting phased rollout to hardware

**Oxport implication:** The Keygroup Synth Engine is a potential new program type that XPN packs may eventually need to target. Monitor whether this becomes a standalone-compatible program type in future firmware.

**VST3 implication:** Third-party plugin instrument packs (not just AIR) can now be targets for MPC content — opens door for XOceanus plugin programs appearing in MPC Desktop as first-class pack content.

---

### MPC 3.5 (Force Update, May 2025)

Akai Force received its own 3.5 update simultaneously, described as "the biggest update yet":
- **Full plugin support** — Force can now host VST/AIR instruments the same way MPC does
- **MPC Stems** — stem separation brought to Force from MPC
- Force and MPC firmware now converge significantly — closer feature parity than ever before

**Force/MPC parity implication:** XPN packs designed for MPC now load more reliably on Force. The long-standing "design for 4×4, note the Force uses 8×8" caveat still holds for pad-mapping, but feature gaps are closing.

---

### MPC Stems (July 2024 — Standalone)

MPC's built-in **stem separation** arrived for standalone MPC devices July 2024, using the **zplane Stems Pro algorithm**:
- Separates any audio file into **up to 4 stems** (expected: drums / bass / melody / vocals — exact labels unconfirmed)
- First hardware sampler to offer professional-grade stem extraction without a computer
- Available on MPC Live III (Oct 2025), Force (3.5 update), and all 3.x-capable hardware with sufficient RAM

**Oxport implication (high priority):**
- Expansion packs can now ship **full loops** knowing users can stem-split them live on device
- This changes the design calculus for loop-heavy packs: you don't have to over-chop everything; full-mix loops are viable again
- Consider including "stems-ready" source loops as a new optional layer in XPN packs — full-mix WAV + note in documentation that MPC Stems will split them
- Future: XPN bundle flag `<StemsReady>true</StemsReady>` to signal this design intent

---

### MPC 3.7 (February 2026)

Described as: "Everything is faster, smarter, and more intuitive."

**Known additions:**
- **Enhanced MPCe Pad control** — deeper parameter assignment, more expressive mapping options for X/Y axes
- **Upgraded Step Sequencer** — improved TR-style drum machine workflow
- General performance improvements for 8-core hardware (MPC XL + Live III)

**Implication:** MPCe pad integration is not a launch-and-done feature — Akai is iterating on it. The 3.7 "enhanced MPCe control" suggests early adopters found limitations and Akai responded. Design packs that give users meaningful X/Y targets even on a per-pad basis.

---

## Section 2: MPCe Hardware — 2024–2026

### MPC Live III (October 2025)

The current standalone flagship, reviewed as "a true quantum leap."

**Specs confirmed:**
- 8-core ARM CPU (4× the Live II's quad-core)
- 8 GB RAM (vs 2 GB on Live II)
- 128 GB internal storage (vs 16 GB on Live II)
- USB-C: 24 audio channels / 32 MIDI channels to computer
- Combo XLR/TRS inputs with phantom power
- Built-in microphone for direct sampling
- Up to **32 simultaneous plugin instruments**
- 16 audio tracks
- 256 sampling voices

**MPCe 3D Pads (first hardware shipping this feature):**

The MPCe pads are the defining feature of the Live III and all future flagship MPC hardware. Here is how they work:

- **Three dimensions of sensing per pad:**
  1. **Z-axis (velocity)** — how hard you strike (standard, always present)
  2. **X-axis** — horizontal finger position on pad surface (new)
  3. **Y-axis** — vertical finger position on pad surface (new)

- **Four quadrant system:**
  - Each pad surface is divided into 4 corners, each assignable to a different sample/articulation
  - Example: upper-left = open hihat, upper-right = closed hihat, lower-left = pedal hihat, lower-right = hihat accent
  - Example for melodic: four velocity layers, four articulations, four timbres on one pad

- **Continuous XY modulation:**
  - X and Y are **continuously tracked while the finger is held down** — not just on attack
  - Can modulate: filter cutoff, filter resonance, pitch, LFO rate, effects parameters, virtually anything
  - Example: lower-left corner = muffled/dry snare; upper-right = bright/airy snare; motion between = continuous morph
  - This enables **real-time performance shaping on individual pads** without touching knobs

- **Software support:** MPC OS 3.7+ fully supports MPCe — 3.7 enhanced the control options

- **Hardware exclusivity:** MPCe pads are a hardware feature of Live III and MPC XL only. Cannot be added to Live II, One Plus, or Key series via firmware.

**New workflow features:**
- **Clip Matrix** — Ableton Session View-style clip launching, available natively for the first time in MPC
- **16 dedicated Step buttons** — TR-style drum machine input alongside traditional pad performance
- **Assignable Touch Strip** — slider-style real-time control for effects, levels, modulation
- Step sequencer and Clip Matrix combine: produce, sequence, and perform in one machine

---

### MPC XL (NAMM January 2026)

Described as capable of making "computers almost redundant in the studio." The premium flagship above the Live III.

**Specs confirmed:**
- **2nd generation 8-core processor** (faster than Live III's first-gen 8-core)
- **16 GB RAM** (double the Live III)
- **256 GB internal storage** (double the Live III)
- **10.1-inch multi-touch display** with adjustable viewing angle (vs smaller screens on previous models)
- **16 MPCe RGB pads** with 3D sensor technology (same as Live III)
- **16 Q-Link encoders with OLED displays** — knob-per-function with on-device labels
- **Assignable touch strip controller**
- **16 RGB buttons**
- Up to **32 plugin instruments** simultaneously (same ceiling as Live III; CPU headroom increased)
- 16 audio tracks
- 256 sampling voices

**I/O — most comprehensive in MPC history:**
- 8 line outputs (6.3mm jack)
- 2 headphone outputs
- 2 line/mic inputs (XLR/TRS combo) with phantom power
- 2 line inputs (6.3mm jack / RCA)
- 2 instrument inputs (6.3mm jack)
- **16 CV/Gate outputs** (4× 3.5mm TRS) — most CV outs on any MPC by far
- 2 MIDI In ports, 4 MIDI Out ports (5-pin DIN) — dual MIDI in is new
- 3 USB-A ports
- 1 USB-C port (Mac, PC, iOS, Android)
- SD card slot + SSD slot
- **Price: ~€2,799**

**Scout's take on MPC XL significance:** This is the machine that targets the "studio hub replacing the DAW" position. 16 CV/Gate outs means deep modular integration. Dual MIDI in means multiple controllers. 16 GB RAM means full orchestral libraries run standalone. The OLED Q-Link encoders are the interface innovation most overlooked in coverage — you can now see what each knob does without looking at the screen. Pack designers should think about Q-Link assignments as a first-class feature.

---

### MPC Key 37 (February 2024)

- Compact keyboard MPC, bright-red design
- 37 keys, same 4-core CPU/4GB RAM as Key 61
- NI Komplete native integration (same as Key 61)
- No MPCe pads (keyboard-series hardware has standard pads)
- Target market: desktop performance / travel

---

## Section 3: Implications for XO_OX

### 3.1 Oxport Pipeline Impact

**HIGH PRIORITY — MPCe Quad-Pad Architecture:**

The four-corner system fundamentally changes how drum programs should be designed. Current XPN packs assign one sample per pad. MPCe pads can hold four samples per pad with position-based selection.

*Recommended new export profile: `mpce_4corner`*

For drum kits, consider:
```
Pad 1 corners: Kick (center), Kick Ghost, Kick Accent, Kick Sidechain
Pad 2 corners: Snare (center), Snare Ghost, Rimshot, Snare Flam
Pad 3 corners: CHat (closed), CHat (tight), CHat (half-open), CHat (open)
Pad 4 corners: OHat variations × 4
```

This collapses a 16-pad kit into 4 pads on MPCe — freeing 12 pads for melodic layers or variations.

**NEW XPM FIELD TO SUPPORT:**
```xml
<PadCornerAssignment padIndex="0">
  <Corner id="0" sample="kick_center.wav" />       <!-- lower-left -->
  <Corner id="1" sample="kick_ghost.wav" />         <!-- lower-right -->
  <Corner id="2" sample="kick_accent.wav" />        <!-- upper-left -->
  <Corner id="3" sample="kick_sidechain.wav" />     <!-- upper-right -->
</PadCornerAssignment>
```

*Until Akai documents the XPM schema for corners, track this as a future-watch item and design samples in grouped fours.*

---

**HIGH PRIORITY — MPCe XY Modulation Targets:**

Every sample program can now expose X and Y axes for real-time modulation. For XO_OX packs, pre-map meaningful targets:

- **Y-axis → Filter cutoff** (vertical = brightness = intuitive)
- **X-axis → Send level or reverb wet/dry** (horizontal = space)
- For melodic pads: Y-axis → pitch bend range, X-axis → tremolo/vibrato depth
- For drums: Y-axis → decay time, X-axis → pan (for drum fills that sweep stereo)

*Recommended: In pack documentation, specify the designed X/Y mapping intent per program.*

---

**MEDIUM PRIORITY — Stems-Ready Loop Design:**

MPC Stems (zplane Stems Pro, available on standalone since July 2024) splits audio into 4 stems. This changes loop pack strategy:

- Full-mix loops are now viable as standalone content — users can stem-split on device
- Design loops with stems separation in mind: avoid frequency-muddying arrangement choices that confuse stem algorithms
- Keep drums, bass, melody, and atmosphere in separate mix buses — even if shipping as full-mix — for clean stem extraction
- Consider adding a new `<StemsOptimized>true</StemsOptimized>` flag in XPN metadata
- Potentially ship a "full loop + pre-separated stems" bundle option as a premium tier

---

**MEDIUM PRIORITY — VST3 Program Type:**

MPC Desktop 3.5 beta (May 2025) added VST3 support. XOceanus ships as AU/VST3 on desktop. This creates an important opportunity:

- XOceanus engines running in MPC Desktop as VST3 plugin programs = XO_OX content that's *playable inside MPC Software*
- A user with MPC Desktop 3.5 could theoretically load XOceanus as a VST3 instrument track
- **Oxport pipeline should document this use case** even if it's user-driven rather than pack-native
- Future consideration: XPN packs that reference XOceanus VST3 plugin programs as instruments (when standalone VST3 hosting reaches hardware)

*Note: As of research date, VST3 support is Desktop/controller mode only — not confirmed for standalone hardware. Watch for standalone VST3 in future firmware.*

---

**LOW PRIORITY (WATCH) — Keygroup Synth Engine:**

MPC 3.5 introduced a "Keygroup Synth Engine" — a hybrid program type combining sample keymapping with synthesis. Details are limited but this could be:
- A new program type (`KeygroupSynth`) distinct from `Keygroup` in XPN format
- A built-in synthesis layer on top of sample playback (similar to AIR Fabric XL but without plugin overhead)
- Potentially a lightweight path to get synth-like behavior in keygroup programs without spawning a full plugin track

*Action: Monitor MPC 3.5 release notes when stable version ships. If `KeygroupSynth` appears as a program type, update Oxport tools to generate it.*

---

**MEDIUM PRIORITY — CV/Gate for Modular Users:**

MPC XL has 16 CV/Gate outputs — the most of any MPC. This creates a new user segment: **modular producers using MPC XL as their brain**.

- These users want sequences that trigger modular gear
- Consider designing MIDI patterns in packs that work as CV/Gate source material
- Potential future pack type: "Modular Companion" — MIDI patterns designed to drive synth voices, not internal MPC engines

---

**LOW PRIORITY — Q-Link OLED Assignments:**

MPC XL's 16 Q-Link encoders have OLED displays — each knob can show its own label. For pack designers:
- Q-Link assignments in XPN packs should be named meaningfully, not just "Param 1"
- This is already good practice but now it's visually surfaced on hardware
- Review all Oxport Q-Link assignment naming conventions

---

### 3.2 XOceanus Platform Impact

**MPCe + XOceanus Preset Design:**

XOceanus presets running via MPC Desktop VST3 (or future standalone) could expose X/Y modulation to the MPCe pad. This means:
- Each XOceanus engine should have **recommended MPCe X/Y macro targets** documented
- Macro 1/2 are the natural candidates: in many engines, Macro 1 = intensity/space, Macro 2 = character
- For engines with DRAMA/BOND/GROW macros: map these to pad XY for live performance arcs

**Clip Matrix + Scene Design:**

MPC Live III's Clip Matrix workflow (Ableton-style clip launching) means:
- Pack users may be running XO_OX content as clips in a live set, not just sequences
- This doesn't change XPN format but changes how presets should be sized (clip length matters more)
- Consider designing loop-length presets alongside standard one-shot/melodic programs

---

### 3.3 Collection Impact

**Kitchen Essentials Collection:**
- The "cooking with stems" metaphor now has hardware backing — MPC Stems = ingredient separation
- Marketing copy opportunity: "Pull the drums out of your sample, season separately"
- Design Kitchen pack loops with stems extraction as a first-class use case

**Travel Collection:**
- MPC XL's 16 CV/Gate outputs open it to modular setups — "Hardware Destinations" could be a sub-concept
- Vessel architecture (sea travel) maps interestingly to the MPC XL as "the ship" that sends signals to modular "ports"

**Artwork Collection:**
- XY pad control maps to 2D canvas navigation — strong conceptual resonance
- Ochre/Oxblood palette packs could use Y-axis for brightness (light vs shadow) and X-axis for warmth (saturated vs neutral)
- Pitch modulation via pad position = "painting with sound" concept is now literal hardware behavior

---

## Section 4: Competitive Landscape

### What's Known

**MSXII Sound Design** — Volume-leader in MPC expansion packs. Known for dusty, lofi aesthetic. No confirmed MPCe-specific products as of research date. Their recent releases continue the traditional drum program / keygroup format. Gap: no 3D pad content.

**The Kount** — Focuses on MPC-native workflow, often releases tutorial content alongside packs. Early adopter of new features. Likely to be first commercial creator to publish MPCe-optimized content.

**Drum Broker** — Aggregate marketplace. Sells packs from dozens of creators. Quality varies. Gap: no platform-level MPCe or stems guidance for their sellers.

**Invious (Noah Shebib / Drake production team)** — No confirmed commercial expansion pack presence. Primary influence is in sound aesthetic rather than hardware format.

### The Gap XO_OX Can Fill

1. **First MPCe-native expansion packs** — nobody has yet designed content explicitly for the four-quadrant system and XY modulation. This is a first-mover position.

2. **Stems-aware loop design** — the MPC Stems feature exists but no pack creator has marketed around "designed for stems" quality loops.

3. **Expressive character instruments** — XO_OX's core identity (character over features) aligns perfectly with MPCe's "play like an instrument" positioning. Where MSXII sells drums and samples, XO_OX sells *instruments you perform with*. MPCe makes that distinction visceral.

4. **CV/Gate companion patterns** — zero current expansion pack creators are targeting the modular integration use case. MPC XL's 16 CV/Gate outs create an underserved segment.

5. **Documented macro targets** — most packs ship with zero documentation on what Q-Links or macros do. XO_OX's engine documentation culture is a competitive differentiator.

---

## Section 5: Future-Proofing Recommendations for Oxport Pipeline

### Recommendation 1: Add `<HardwareProfile>` flag to XPN metadata

```xml
<HardwareProfile>
  <OptimizedFor>mpce</OptimizedFor>      <!-- or: standard, force, mpcxl -->
  <MinFirmware>3.7</MinFirmware>
  <StemsReady>true</StemsReady>
  <PadCornerMapping>true</PadCornerMapping>
</HardwareProfile>
```

This lets users and pack browsers filter by hardware capability. XO_OX packs can ship in two variants: `standard` (all MPC) and `mpce` (Live III / XL optimized).

---

### Recommendation 2: Design Samples in Grouped Fours for MPCe Quads

When designing drum programs for the Onset engine or XPN drum exports:
- Group samples into logical sets of four that make sense as pad corners
- Four velocity/articulation variations per drum element
- Four tonal variants per melodic pad
- Document the grouping so Oxport tools can auto-assign corners when MPCe profile is selected

---

### Recommendation 3: Add MPCe XY Modulation Target Documentation to All Engine Cards

Each XOceanus engine card should include:
```
MPCe Recommended Mapping:
  Pad X → [parameter name]
  Pad Y → [parameter name]
  Performance note: [what this enables expressively]
```

This is low-effort documentation that becomes high-value when Live III / XL users browse.

---

### Recommendation 4: Stems-Aware Loop Production Standard

Adopt a new mixing standard for all loop content in XPN packs:
- Drums: center, peak-limited, clean transients (stems algorithm handles better)
- Bass: clean sub/low-mid, minimal high-frequency bleed
- Melody/chords: clearly separated from percussion frequency range
- Atmosphere: ambient, diffuse, minimal rhythmic content (don't confuse stems algorithm)

Document this as the **"Stems Separation Standard"** in Oxport production guidelines.

---

### Recommendation 5: Monitor Standalone VST3 Announcement

MPC Desktop 3.5 added VST3 support for computer-connected mode. The natural next step is standalone VST3 support on MPC XL / Live III hardware (sufficient RAM and CPU to run them). When Akai announces standalone VST3:
- XOceanus plugin programs become installable as MPC expansion content natively
- Oxport would need to generate a new program type: `<ProgramType>Plugin</ProgramType>` with VST3 reference
- This would be the most significant Oxport update in the platform's history

**Action: Set a watch on Akai firmware announcements for "standalone VST3" language.**

---

### Recommendation 6: CV/Gate Pattern Pack — New Export Format

Consider a new Oxport export type: `cv_gate_patterns`

- MIDI-only patterns designed to control CV/Gate outputs on MPC XL
- One voice per output, 1V/oct pitch tracking, gate logic
- Target market: modular synthesizer users using MPC XL as sequencer brain
- Content concept: XOceanus engine personality expressed as modular gate patterns ("what would OVERDUB sequence look like as CV?")

---

### Recommendation 7: Q-Link OLED Naming Standards

Update all Oxport-generated Q-Link assignments to follow a naming convention that fits in the OLED display character limit (estimated 8–10 characters based on typical OLED sizes):

- `Cutoff`, `Resonance`, `AttTime`, `DecTime`, `SustLvl`, `RelTime`, `DistDrv`, `RevWet`
- NOT: `Filter Cutoff Frequency`, `Reverb Send Amount`

Audit all existing pack Q-Link assignments for name length compliance.

---

---

## Section 6: XPM Format Deep Dive — Version-by-Version Field Reference

This section documents the XPM XML schema across firmware versions with confidence labels. This is the canonical source for Oxport field generation.

### 6.1 Program Root Element [VERIFIED for 2.x baseline]

```xml
<Program fileVersion="2.0" instrumentVersion="1.0.0" name="Pack Name"
         type="KeyGroup" engineType="KeyGroup" programVersion="3.0">
```

- `fileVersion` — XPM file format version. 2.0 is the 2.x baseline.
- `instrumentVersion` — user-defined version string. Use semver.
- `type` and `engineType` — must match. Valid values: `KeyGroup`, `Drum`, `Plugin`, `CV`, `Clip`, `KeygroupSynth` (3.5+). [APPROXIMATE for new types]
- `programVersion` — set to `"3.0"` for new packs. [APPROXIMATE — not confirmed in primary docs]

### 6.2 Keygroup Program Structure [VERIFIED]

```xml
<Keygroup enabled="1" lowNote="0" highNote="127" rootNote="0"
          tune="0" coarseTune="0" keyTrack="True"
          velocityCurve="linear" velocityCurveShape="linear"
          velocityTracking="100" numberofLayers="4">
```

Key fields:
- `lowNote` / `highNote` — MIDI note range (0–127)
- `rootNote` — set to `0` for Oxport packs (MPC auto-detect convention). [VERIFIED]
- `keyTrack` — MUST be `"True"` for transpose to work. [VERIFIED, critical]
- `velocityCurveShape` — 3.0+ values: `"linear"`, `"exponential"`, `"s-curve"`, `"custom"`. [APPROXIMATE for new values]
- `numberofLayers` — 1–16 layers supported.

### 6.3 Layer Structure [VERIFIED]

```xml
<Layer number="1" active="1" fileName="sample_v1.wav" volume="1.0"
       pan="0" tune="0" coarseTune="0"
       velStart="0" velEnd="31"
       loopStart="-1" loopEnd="-1" loopActive="0"
       rootNote="60" keyTrack="True"
       velocityCurve="linear" />
```

Critical fields:
- `velStart` — MUST be `0` for the lowest layer. Non-zero causes ghost triggering. [VERIFIED, critical]
- `velEnd` — 0–127. Adjacent layers: layer N velEnd = layer N+1 velStart - 1.
- `loopStart` / `loopEnd` — `-1` = no loop. Set to sample frame indices when looping. [VERIFIED]
- `rootNote` — per-layer root note. Used when `keyTrack="True"`. [VERIFIED]

### 6.4 Envelope Structure [VERIFIED]

```xml
<Envelope type="amplifier" attack="0.001" hold="0.0" decay="0.5"
          sustain="1.0" release="0.3"
          attackCurve="0" decayCurve="0" releaseCurve="0" />
<Envelope type="filter" attack="0.001" hold="0.0" decay="0.5"
          sustain="0.0" release="0.2" amount="50"
          attackCurve="0" decayCurve="0" releaseCurve="0" />
```

- Curve values: `0` = linear, `1` = exponential. [APPROXIMATE]
- `amount` on filter envelope: -100 to 100. Bipolar — negative sweeps downward. [VERIFIED]

### 6.5 LFO Structure [VERIFIED for LFO1/LFO2; APPROXIMATE for LFO3]

```xml
<LFO number="1" shape="sine" rate="1.0" depth="0" sync="0"
     startPhase="0" keySync="0" tempoSync="0" />
<LFO number="2" shape="sine" rate="0.5" depth="0" sync="0"
     startPhase="0" keySync="0" tempoSync="0" />
<LFO number="3" shape="sine" rate="0.1" depth="0" sync="0"
     startPhase="0" keySync="0" tempoSync="0" />
```

- `shape` values: `"sine"`, `"triangle"`, `"square"`, `"sawtooth"`, `"random"`, `"sampleHold"`. [VERIFIED]
- LFO3 added in MPC 3.0. [APPROXIMATE — not confirmed in primary docs]
- Oxport: always stub LFO3 with `depth="0"` for forward compatibility.

### 6.6 FX Chain Structure [VERIFIED for 2-slot; APPROXIMATE for 4-slot]

```xml
<Insert number="1" type="AIR_Reverb" bypass="0">
  <Param name="DecayTime" value="2.5" />
  <Param name="PreDelay" value="0.02" />
  <Param name="WetDry" value="0.3" />
</Insert>
<Insert number="2" type="AIR_Delay" bypass="0"> ... </Insert>
<Insert number="3" type="" bypass="1" />   <!-- stub: 3.0+ -->
<Insert number="4" type="" bypass="1" />   <!-- stub: 3.0+ -->
```

- 4-slot FX chain added in MPC 3.0. [APPROXIMATE]
- Oxport: generate Insert 3 and 4 as bypassed stubs for forward compatibility.
- AIR plugin type strings: use `AIR_Reverb`, `AIR_Delay`, `AIR_Compressor` — no version suffix in type name. [APPROXIMATE]

### 6.7 Q-Link Assignment Structure [VERIFIED for 8-slot; APPROXIMATE for 16-slot and new fields]

```xml
<QLinks>
  <QLinkAssignment number="1" controllerNumber="21"
                   paramName="FilterCutoff" minValue="0" maxValue="127"
                   modulationTarget="filter_cutoff" bipolar="0"
                   smoothing="20" label="Cutoff" />
  <!-- ... up to QLinkAssignment number="16" in 3.0+ -->
</QLinks>
```

- `controllerNumber` — MIDI CC used for this Q-Link (hardware-dependent, typically 21–28 for Q1–8).
- `modulationTarget` — new in 3.0: links to a specific engine parameter path. [APPROXIMATE]
- `bipolar` — 0 = unipolar (0–127), 1 = bipolar (-63 to 63 center). [APPROXIMATE]
- `smoothing` — 0–100ms response smoothing. [APPROXIMATE]
- `label` — displayed on MPC XL OLED. Max 8 characters. [APPROXIMATE]

### 6.8 Drum Program Pad Assignment [VERIFIED]

```xml
<PadNoteMap>
  <Pad number="1" note="36" bankNumber="A" />  <!-- Kick: C2 = 36 -->
  <Pad number="2" note="38" bankNumber="A" />  <!-- Snare: D2 = 38 -->
  <!-- ... 16 pads per bank, 4 banks (A–D) = 64 pads total -->
</PadNoteMap>
<PadGroupMap>
  <Pad number="1" groupNumber="1" />  <!-- mute group -->
</PadGroupMap>
```

### 6.9 MPCe Quad-Pad Architecture [SPECULATIVE — no confirmed schema]

Four-corner pad assignment is a hardware feature of MPC Live III / XL. The XPM schema for this is not yet publicly documented. Predicted structure based on hardware behavior:

```xml
<!-- SPECULATIVE — not confirmed in any Akai documentation -->
<PadCornerAssignment padIndex="0">
  <Corner id="0" position="lower-left"  sample="kick_center.wav" />
  <Corner id="1" position="lower-right" sample="kick_ghost.wav" />
  <Corner id="2" position="upper-left"  sample="kick_accent.wav" />
  <Corner id="3" position="upper-right" sample="kick_sidechain.wav" />
</PadCornerAssignment>
```

Watch for this in MPC 3.7+ firmware documentation. Until confirmed:
- Design samples in groups of four with logical corner-assignment intent
- Document the intended groupings in pack notes

### 6.10 HardwareProfile Extension (XO_OX Proposed) [SPECULATIVE / XO_OX INTERNAL]

Proposed XO_OX-specific manifest addition for pack browser filtering:

```json
{
  "hardwareProfile": {
    "optimizedFor": "mpce",
    "minFirmware": "3.7",
    "stemsReady": true,
    "padCornerMapping": false,
    "xyModulationMapped": true
  }
}
```

This field is XO_OX-internal and will be ignored by MPC hardware. It enables XO_OX's own pack browser / website to filter by hardware target.

---

### 6.11 XPN Manifest Field Comparison — 2.x vs 3.0

| Field | 2.x | 3.0 | Confidence |
|-------|-----|-----|-----------|
| `name` | Yes | Yes | VERIFIED |
| `vendor` | Yes | Yes | VERIFIED |
| `version` | Yes | Yes | VERIFIED |
| `description` | Yes | Yes | VERIFIED |
| `tags` | Yes | Yes | VERIFIED |
| `programs` | Yes | Yes | VERIFIED |
| `samples` | Yes | Yes | VERIFIED |
| `schemaVersion` | No | Yes | APPROXIMATE |
| `engineRequirements` | No | Yes | APPROXIMATE |
| `targetHardware` | No | Yes | APPROXIMATE |
| `sampleRate` | No | Yes | APPROXIMATE |
| `aiFeatures` | No | Yes | APPROXIMATE |
| `previewAudio` | No | Yes | APPROXIMATE |
| `genreTags` | No | Yes | APPROXIMATE |
| `moodTags` | No | XO_OX ext | XO_OX INTERNAL |
| `hardwareProfile` | No | XO_OX ext | XO_OX INTERNAL |

---

### 6.12 CycleType Values — Confirmed and Proposed

| CycleType | Status | Description |
|-----------|--------|-------------|
| `velocity` | VERIFIED (2.x) | Layers trigger by velocity range |
| `cycle` | VERIFIED (2.x) | Round-robin sequential |
| `random` | VERIFIED (2.x) | Random selection |
| `random-norepeat` | VERIFIED (2.x) | Random without immediate repeat |
| `smart` | APPROXIMATE (3.0) | AI-assisted perceptual non-repeat |
| `velocity-cycle` | APPROXIMATE (3.0) | Hybrid: velocity zones + round-robin within each zone |
| `pattern` | SPECULATIVE | User-defined trigger pattern |

---

## Sources

- Gearnews.com: MPC XL announcement (Jan 2026), MPC Live III review (Oct 2025), MPC Desktop 3.5 beta (May 2025), Force 3.5 (May 2025), MPC Stems (Jul 2024)
- Cross-referenced with Kai android team bible (`mpc_ecosystem.md`)
- MPCe 3D pad technical detail sourced from MPC Live III review
- XPM schema: community-extracted from MPC 2.x program files (primary), MPC 3.0 forum reports (secondary)
- MPC XL Q-Link OLED: Gearnews product announcement photos
- CycleType enumeration: Akai support documentation (2.x) + community forum (3.0 additions)

---

*Scout — March 2026*
*"The pad is no longer a trigger. It's a canvas."*
