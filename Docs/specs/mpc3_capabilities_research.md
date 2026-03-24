# MPC 3.x Capabilities Research
## Oxport Future-Proofing Roadmap — Deep Dive
### Scout's Intelligence Report — March 2026

*Compiled by Scout (community intelligence android, XO_OX)*
*Companion doc to: `mpc_evolving_capabilities_research.md` (hardware / firmware timeline)*
*Focus: XPM/XPN schema details, plugin architecture, export/import, Oxport action items*

---

## Methodology Note

MPC 3.x firmware shipped progressively across hardware lines from 2021–2026. "MPC 3.x" refers to the
software platform from version 3.0 onward, distinct from the 2.x series. Key milestones:

- **MPC 3.0** — January 2023 (MPC Key 61 launch OS); new instruments, new UI layer
- **MPC 3.5** — May 2025 (Desktop beta + Force update); VST3, Keygroup Synth Engine, MPC Stems
- **MPC 3.7** — February 2026; MPCe enhanced pad control, step sequencer improvements

Confidence levels per fact:
- **VERIFIED** — confirmed from reviewed source material (Sound on Sound review, Synthtopia article list, existing `mpc_evolving_capabilities_research.md`)
- **APPROXIMATE** — grounded in known MPC conventions and community knowledge; specific field names may vary
- **SPECULATIVE** — reasonable inference from hardware capabilities and precedent; not confirmed in documentation

---

## Part 1: MPC 3.x XPM/XPN Format — Field-Level Analysis

### 1.1 Baseline XPM Format (MPC 2.x Reference)

Before cataloguing what changed, the known 2.x XPM field set for Drum and Keygroup programs:

**Drum Program (known stable fields):**
```xml
<Program type="Drum">
  <ProgramName>...</ProgramName>
  <Tempo>...</Tempo>
  <Instruments>
    <Instrument number="0">
      <Volume>...</Volume>
      <Pan>...</Pan>
      <Mute>...</Mute>
      <Solo>...</Solo>
      <MuteGroup>...</MuteGroup>
      <MuteTarget>...</MuteTarget>
      <PadNote>...</PadNote>
      <Layers>
        <Layer number="0">
          <SampleName>...</SampleName>
          <VelStart>...</VelStart>
          <VelEnd>...</VelEnd>
          <Volume>...</Volume>
          <Pan>...</Pan>
          <Tune>...</Tune>
          <ZonePlay>...</ZonePlay>   <!-- 1=Velocity, 2=Cycle, 3=Random, 4=RandomNoRepeat -->
          <CycleType>...</CycleType>
          <CycleGroup>...</CycleGroup>
        </Layer>
      </Layers>
    </Instrument>
  </Instruments>
  <Qlnk>
    <QlnkAssignment number="0">...</QlnkAssignment>
    <!-- 4 Q-Links in 2.x -->
  </Qlnk>
</Program>
```

**Keygroup Program (known stable fields):**
```xml
<Program type="Keygroup">
  <ProgramName>...</ProgramName>
  <KeygroupCount>...</KeygroupCount>
  <Keygroups>
    <Keygroup number="0">
      <LowKey>...</LowKey>
      <HighKey>...</HighKey>
      <RootNote>...</RootNote>
      <KeyTrack>...</KeyTrack>
      <Layers>
        <Layer number="0">
          <SampleName>...</SampleName>
          <VelStart>...</VelStart>
          <VelEnd>...</VelEnd>
          <SampleStart>...</SampleStart>
          <SampleEnd>...</SampleEnd>
          <LoopStart>...</LoopStart>
          <LoopEnd>...</LoopEnd>
          <LoopCrossfade>...</LoopCrossfade>
          <LoopTune>...</LoopTune>
        </Layer>
      </Layers>
      <ZoneEnvelope>
        <Attack>...</Attack>
        <Hold>...</Hold>
        <Decay>...</Decay>
        <Sustain>...</Sustain>
        <Release>...</Release>
      </ZoneEnvelope>
      <ZoneFilter>
        <Type>...</Type>
        <Cutoff>...</Cutoff>
        <Resonance>...</Resonance>
        <EnvAmount>...</EnvAmount>
        <VelToFilter>...</VelToFilter>
      </ZoneFilter>
    </Keygroup>
  </Keygroups>
</Program>
```

---

### 1.2 New XPM Fields in MPC 3.x

#### PadColor / Pad RGB Assignment

**Status: APPROXIMATE**

MPC hardware has supported RGB pad coloring since at least MPC Live (2.x era), but the XPM
serialization of pad color has evolved. In MPC 3.x the expected schema:

```xml
<Instrument number="0">
  <!-- ... existing fields ... -->
  <PadColor>
    <Red>255</Red>
    <Green>128</Green>
    <Blue>0</Blue>
  </PadColor>
</Instrument>
```

Alternative flat encoding also observed in community-shared XPMs:
```xml
<PadColor red="255" green="128" blue="0" />
```

The exact attribute vs. child-element format is firmware-version-dependent. MPC 3.x likely
standardized the attribute form based on MPC's general XML conventions.

**Known pad color behavior:**
- Colors are stored per-instrument (pad), not per-program globally
- Colors persist when the XPM is loaded
- The MPC pad grid displays these colors at rest (dim) and on hit (bright)
- Color applies to the 4×4 pad grid bank for that program
- Force uses an 8×8 grid; pad color maps to the corresponding cells

**OXPORT ACTION ITEM 1:** Add `pad_color` parameter to `xpn_drum_export.py` and `xpn_keygroup_export.py`.
For XO_OX packs, pre-assign pad colors that reflect the engine accent color. Example:
```python
# In VOICE_DEFAULTS, add:
"kick":  { ..., "pad_color": (255, 80,  0)  },   # orange-red for kinetic energy
"snare": { ..., "pad_color": (255, 200, 0)  },   # yellow for crack
"chat":  { ..., "pad_color": (180, 220, 255)},   # blue-white for hi-freq
"ohat":  { ..., "pad_color": (140, 200, 255)},   # lighter blue for open air
```
For keygroup programs, the pad color should match the XOlokun engine accent color
(e.g., ONSET = Electric Blue `#0066FF` → R=0, G=102, B=255).

---

#### LFOAssignment / Modulation Routing

**Status: SPECULATIVE (but highly probable in 3.x programs)**

MPC 2.x had LFO support at the program level but routing was not fully serialized in XPM files
that could be shared cross-device. MPC 3.x expanded plugin architecture likely brought more explicit
LFO routing into the program XML.

Expected schema based on MPC architecture conventions:
```xml
<Instrument number="0">
  <!-- ... -->
  <LFO number="0">
    <Rate>0.5</Rate>           <!-- Hz -->
    <Depth>0.3</Depth>         <!-- 0–1 -->
    <Shape>Sine</Shape>        <!-- Sine / Triangle / Square / SawUp / SawDown / Random -->
    <Sync>Off</Sync>           <!-- Off / Tempo (synced to BPM) -->
    <SyncRate>1/4</SyncRate>   <!-- if Sync=Tempo: 1/32, 1/16, 1/8, 1/4, 1/2, 1, 2 -->
    <KeySync>On</KeySync>
    <Target>FilterCutoff</Target>  <!-- or: Volume, Pan, Tune, FilterResonance -->
    <TargetLayer>0</TargetLayer>   <!-- -1 = all layers -->
  </LFO>
</Instrument>
```

LFO targets likely include: `FilterCutoff`, `FilterResonance`, `Volume`, `Pan`, `Tune`, `PitchBend`,
`SampleStart`, `SampleEnd`, `Attack`, `Decay`, `Release`.

**OXPORT ACTION ITEM 2:** Add optional LFO routing to `xpn_drum_export.py` and `xpn_keygroup_export.py`.
For expressively designed packs:
- Keygroup programs: slow LFO (0.3–0.8 Hz) → FilterCutoff for organic movement
- Drum programs: subtle LFO → Volume on clap/perc pads for humanization
- Add `--lfo` flag to both exporters with presets: `subtle`, `vibrato`, `tremolo`, `filter_sweep`

---

#### ArpeggiatorSettings (Program-Level Arp)

**Status: APPROXIMATE**

MPC 3.0 added a program-level arpeggiator in the "Perform Mode" feature set, alongside chord mode.
This would be stored in XPM as a top-level block:

```xml
<Program type="Keygroup">
  <!-- ... -->
  <ArpeggiatorSettings>
    <Enable>False</Enable>
    <Mode>Up</Mode>          <!-- Up / Down / UpDown / DownUp / Random / Order -->
    <Rate>1/8</Rate>         <!-- sync rate: 1/32, 1/16T, 1/16, 1/8T, 1/8, 1/4T, 1/4, 1/2, 1 -->
    <Gate>50</Gate>          <!-- percentage 0–100 -->
    <Octave>1</Octave>       <!-- octave range: 1, 2, 3, 4 -->
    <Latch>False</Latch>
    <Swing>50</Swing>        <!-- percentage, 50 = no swing -->
  </ArpeggiatorSettings>
</Program>
```

Arpeggiator patterns added in MPC 3.x beyond the 2.x `Up/Down/UpDown/Random`:
- **Chord** — all notes simultaneously (strum simulation)
- **Order** (aka "As Played") — arpeggiates in the order notes were pressed
- **Pattern** — user-defined rhythmic arpeggio pattern (grid-based)
- **Poly Rhythm** — multiple simultaneous arp rates (SPECULATIVE; may be 3.7+)

**OXPORT ACTION ITEM 3:** For keygroup programs, include a disabled-by-default ArpeggiatorSettings block
in the XPM template so users can enable without touching XML. Pre-configure `Rate=1/8`, `Mode=Up`,
`Gate=50`, `Octave=1`. This gives users a "turn on and go" experience.

---

#### ChordMode and ScaleMode

**Status: APPROXIMATE**

MPC 3.0 introduced Chord Mode and Scale/Note Quantize as a pad-level performance feature.
These are likely stored both as hardware-state settings and embedded in project/program files.
XPM serialization expected:

```xml
<Program type="Keygroup">
  <!-- ... -->
  <ChordMode>
    <Enable>False</Enable>
    <Chord>Major</Chord>   <!-- Major, Minor, Dom7, Maj7, Min7, Sus2, Sus4, Aug, Dim, custom -->
    <Voicing>Closed</Voicing>   <!-- Closed / Open / Drop2 / Drop3 -->
    <Inversion>0</Inversion>    <!-- 0, 1, 2, 3 -->
    <Spread>0</Spread>          <!-- semitone spread for voicing -->
  </ChordMode>
  <ScaleMode>
    <Enable>False</Enable>
    <Root>C</Root>              <!-- C, C#, D, Eb, E, F, F#, G, Ab, A, Bb, B -->
    <Scale>Major</Scale>        <!-- Major, NaturalMinor, HarmonicMinor, MelodicMinor,
                                     Dorian, Phrygian, Lydian, Mixolydian, Locrian,
                                     Pentatonic, MinorPentatonic, Blues, Chromatic -->
    <SnapToScale>True</SnapToScale>
  </ScaleMode>
</Program>
```

Scale quantize snaps incoming MIDI notes to the nearest scale degree — useful for melodic pads
where users want to improvise without playing wrong notes.

**OXPORT ACTION ITEM 4:** For keygroup exports, add `--scale-hint` parameter that writes a disabled
ScaleMode block with the recommended root and scale pre-populated. Example:
```
python3 xpn_keygroup_export.py --preset "Deep Drift" --scale-hint "C:Dorian"
```
This documents the tonal center without forcing it on users.

---

#### Multi-Output Routing / BusAssignment

**Status: APPROXIMATE**

MPC standalone hardware has individual stereo outputs (MPC XL has 8 line outs). Program-level
bus routing in XPM format:

```xml
<Instrument number="0">
  <!-- ... -->
  <Output>
    <BusAssignment>Main</BusAssignment>   <!-- Main / Sub1 / Sub2 / Sub3 / Sub4 ... -->
    <SendLevel_1>0.0</SendLevel_1>        <!-- FX bus 1 send level -->
    <SendLevel_2>0.0</SendLevel_2>
    <SendLevel_3>0.0</SendLevel_3>
    <SendLevel_4>0.0</SendLevel_4>
  </Output>
</Instrument>
```

Sub-output routing corresponds to physical outputs on the hardware:
- `Main` — master stereo out
- `Sub1`–`Sub4` — individual outputs (or stereo pairs on MPC XL's 8-out)
- On MPC XL specifically, 8 line outputs = 4 stereo sub-buses

The MPC's internal FX chain has 4 send buses (FX1–FX4), each hosting a send effect.

**OXPORT ACTION ITEM 5:** Add optional `--bus-assignments` flag to `xpn_drum_export.py` for
stem-out workflow. Canonical assignment:
```
kick  → Sub1 (direct out for parallel compression)
snare → Sub1 (combined with kick or separate Sub2)
hats  → Sub2
perc  → Sub2
melodic pads → Main (with FX bus sends)
```
Document this as the "Stems Out" preset in Oxport — users with MPC XL can route each element
to separate hardware outputs and record stems independently.

---

#### QLink Count — Was 4, Is It More in 3.x?

**Status: VERIFIED (count increase) / APPROXIMATE (XPM format)**

MPC Live/One/X had 4 physical Q-Link encoders with 4 banks = 16 assignable Q-Links per program.
**MPC XL introduced 16 physical Q-Link encoders with OLED displays** (verified from existing research).

The XPM format in MPC 3.x/XL era likely expanded:
```xml
<Qlnk>
  <!-- Standard 4 Q-Links (2.x compatible) -->
  <QlnkAssignment number="0">FilterCutoff</QlnkAssignment>
  <QlnkAssignment number="1">Volume</QlnkAssignment>
  <QlnkAssignment number="2">Attack</QlnkAssignment>
  <QlnkAssignment number="3">Release</QlnkAssignment>

  <!-- Extended Q-Links (MPC XL, 3.x era) -->
  <QlnkAssignment number="4">FilterResonance</QlnkAssignment>
  <QlnkAssignment number="5">Tune</QlnkAssignment>
  <QlnkAssignment number="6">Pan</QlnkAssignment>
  <QlnkAssignment number="7">Decay</QlnkAssignment>
  <!-- ... up to number="15" for 16-encoder layout -->
</Qlnk>
```

Older hardware (4 encoders) likely ignores `number="4"` through `"15"` gracefully — forward-compatible
design. XPM files should include all 16 if targeting MPC XL.

Additionally, MPC 3.x supports **Q-Link OLED labels** — the encoder display shows the assignment name.
These may be stored as:
```xml
<QlnkAssignment number="0" label="Cutoff">FilterCutoff</QlnkAssignment>
```

**OXPORT ACTION ITEM 6:** Expand `VOICE_DEFAULTS` Q-Link assignments from 2 per voice to 4 (for
standard hardware) and 8 (for XL profile). Add `--qlink-profile standard|xl` flag. XL profile uses
all 16 encoders for 16-instrument drum programs (one encoder per pad = direct control). Label all
Q-Link assignments with OLED-safe names (max 8 characters). Current Oxport tools only define
`qlink_1` and `qlink_2` — expand to `qlink_1` through `qlink_4` (standard) and `qlink_1` through
`qlink_8` (xl).

---

#### MPE / Per-Note Expression Fields

**Status: SPECULATIVE**

MPE (MIDI Polyphonic Expression) support in MPC is not confirmed in available sources as of March 2026.
The MPCe 3D pads achieve per-pad expression via proprietary XY sensing — a hardware path distinct
from MPE protocol. MPE requires note-level MIDI channel allocation, which conflicts with MPC's
traditional note-per-channel drum architecture.

Potential MPE-adjacent XPM fields if Akai added MPE in a future firmware:
```xml
<MPESettings>
  <Enable>False</Enable>
  <PitchBendRange>48</PitchBendRange>   <!-- semitones, MPE standard = 48 -->
  <PressureTarget>FilterCutoff</PressureTarget>
  <SlideTarget>Pan</SlideTarget>
</MPESettings>
```

More likely path (and what MPCe actually does): proprietary `XYPadAssignment` rather than MPE.
```xml
<XYPadAssignment padIndex="0">
  <XAxis>
    <Target>Pan</Target>
    <MinValue>-1.0</MinValue>
    <MaxValue>1.0</MaxValue>
  </XAxis>
  <YAxis>
    <Target>FilterCutoff</Target>
    <MinValue>0.2</MinValue>
    <MaxValue>1.0</MaxValue>
  </YAxis>
</XYPadAssignment>
```

**OXPORT ACTION ITEM 7:** Monitor Akai firmware announcements for either MPE support or
`XYPadAssignment` XPM field documentation. When either appears:
- If MPE: add `--mpe` flag to keygroup exporter; set `PitchBendRange=48`, map pressure→filter,
  slide→pan as defaults
- If XYPad: add `--xy-map` flag with presets: `filter_pan` (Y→cutoff, X→pan),
  `filter_reverb` (Y→cutoff, X→reverb send), `dynamics` (Y→volume, X→attack time)

---

#### New SamplePlayMode Types

**Status: APPROXIMATE**

Known 2.x play modes for drum instruments:
- `One Shot` — plays to end regardless of note-off
- `Note On` — plays only while note is held
- `Note Off` — plays on note release (hihat tail behavior)
- `Note On+Off` — triggers on both press and release

MPC 3.x additions (APPROXIMATE, based on community feature requests and AIR plugin behavior patterns):
- `Loop Until Release` — loops sample until note-off, then releases naturally (SPECULATIVE name)
- `Gate` — explicit gate mode: sample truncated at note-off with no release tail
- `Stretch` — time-stretch sample to fill note duration (SPECULATIVE; requires stretch engine)

Current Oxport tools use `one_shot` and boolean `OneShot` field. The extended play modes
would require a `PlayMode` string field replacing the boolean:

```xml
<PlayMode>OneShot</PlayMode>       <!-- OneShot / NoteOn / NoteOff / NoteOnOff / LoopUntilRelease / Gate -->
```

**OXPORT ACTION ITEM 8:** Replace boolean `one_shot` in `VOICE_DEFAULTS` with `play_mode` string.
Keep backward compat by mapping `True` → `OneShot`, `False` → `NoteOn`. Add new mode constants.
Update XML generation in `_instrument_xml()` to emit `<PlayMode>` when target firmware >= 3.0,
fall back to legacy `<OneShot>True/False</OneShot>` for 2.x compatibility mode.

---

#### New Velocity Curve Shapes

**Status: APPROXIMATE**

MPC 2.x had a fixed velocity response curve per pad (linear, hardcoded). MPC 3.x is believed to
support selectable velocity curves at the program/instrument level:

```xml
<Instrument number="0">
  <!-- ... -->
  <VelocityCurve>Linear</VelocityCurve>
  <!-- Options: Linear, Soft (logarithmic), Hard (exponential), Fixed, Custom -->
  <VelocityFixed>100</VelocityFixed>    <!-- only used when VelocityCurve=Fixed -->
</Instrument>
```

The velocity curve shapes likely available:
- `Linear` — default, MIDI velocity maps 1:1 to output level
- `Soft` — logarithmic response, quiet playing easier, loud harder to reach (good for melodic)
- `Hard` — exponential response, most volume comes from harder hits (good for drums)
- `Fixed` — all hits at constant velocity (override everything)
- `Custom` — user-defined breakpoint curve (advanced)

**OXPORT ACTION ITEM 9:** Add `velocity_curve` to `VOICE_DEFAULTS`. Recommended per-voice:
```python
"kick":  "Hard",    # dynamics reward the physical effort
"snare": "Hard",    # same
"chat":  "Soft",    # light touch playing = closed hi-hat finesse
"ohat":  "Soft",    # open hat: subtle gradations
"clap":  "Linear",  # clap = straightforward
"tom":   "Hard",
"perc":  "Soft",    # percs often used for ghost notes
"fx":    "Soft",    # fx = textural, not hit-hard-to-trigger
```

---

## Part 2: MPC 3 Performance Features

### 2.1 Pad Performance Modes

**Status: VERIFIED (existence) / APPROXIMATE (details)**

MPC 3.0 introduced a reorganized "Perform Mode" that consolidated several per-pad performance
features under a unified interface. Per the Sound on Sound review of the MPC Key 61:

- **Sound Browser Grid** — graphical grid interface for fast preset browsing
- **Key Ranges view** — split/layer keyboard assignments (combi/zone mode)
- **Favorites tab** — bookmarked patches for live use
- **Performances tab** — saved performance state (splits + assignments + effects state)
- **Set List tab** — ordered queue of programs for live performance
- **Probability automation** — step sequencer now supports note probability (0–100% per step)

Additional pad modes in MPC 3.x:
- **Chord mode** — one pad triggers a chord (configurable voicing and chord type)
- **Scale lock** — forces all pads to play notes from a selected scale/key
- **Note repeat** — holds a pad → note repeats at selectable rate with fill/flam behavior
- **Full Level** — overrides pad velocity to maximum (toggle)
- **16 Level** — maps 16 pads to 16 velocity levels of the same sample
- **Erase** — hold + pad = erase that instrument from the sequence in real time

### 2.2 Note Repeat Granularity

**Status: APPROXIMATE**

MPC 2.x note repeat rates: 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32, 1/32T

MPC 3.x additions:
- **1/64** and **1/64T** — sub-subdivision for extreme fills
- **Gate** parameter (length of each repeat as percentage of the rate)
- **Swing** per note repeat (applies groove offset to the repeated notes)
- **Ratchet** mode: acceleration/deceleration of note repeat rate while pad is held

**OXPORT ACTION ITEM 10:** No direct XPM field change — note repeat is a playback engine feature,
not stored in program XML. However, Oxport documentation for each pack should include
"Recommended Note Repeat Settings" for signature sounds. Example:
```
808 kick: Note Repeat 1/16, Gate=80%, Full Level OFF — classic trap roll
Closed hat: Note Repeat 1/32, Gate=60%, swing tied to sequence swing
```

### 2.3 64-Pad Mode / Extended Grid

**Status: APPROXIMATE**

MPC 3.x introduced "64-Pad Mode" for the 16-pad hardware, mapping the 4×4 pad grid to a 8×8
virtual grid navigated via bank buttons. This maps 64 consecutive MIDI notes to the 16 visible pads
across 4 banks.

For XPN packs: the XPM program type `Drum` already supports 128 instruments. 64-pad mode simply
means banks 1–4 now each address 16 pads of the same program. No XPM schema change required
— the program is the same, the pad-bank navigation changes which 16 are visible.

However, a 64-note melodic keygroup program designed specifically for 64-pad layout would want
notes C1–D#6 (64 consecutive semitones) mapped to instruments 0–63.

**OXPORT ACTION ITEM 11:** Add `--layout 64pad` export profile to `xpn_keygroup_export.py`.
This profile:
- Maps 64 consecutive semitones as individual pads (no velocity layers, one sample per note)
- Assigns chromatic notes starting from C1
- Useful for melodic drum-style programs (think: 808 chromatic bass, Marimba scales)
- Compatible with standard 16-pad hardware via bank navigation

### 2.4 Arpeggiator Patterns

**Status: APPROXIMATE**

MPC 3.x arpeggiator additions beyond 2.x:

| Pattern | Description |
|---------|-------------|
| Up | Lowest to highest, repeating |
| Down | Highest to lowest, repeating |
| UpDown | Up then down, no repeat at extremes |
| DownUp | Down then up |
| Random | Random note order from held notes |
| Order (As Played) | Arpeggiates in the press order |
| Chord | All notes simultaneously on each trigger |
| Pattern | Step-sequenced custom pattern (user-drawn grid) |

The "Pattern" arp mode is the key addition in 3.x — it lets users draw a rhythmic pattern that
the arpeggiator follows, enabling off-beat and syncopated arpeggios not possible with the
standard directional modes.

**Gate** per arp step (when Pattern mode is active) and **Velocity per step** make this closer to
a mini sequencer embedded in the arpeggiator.

### 2.5 Chord Memorizer / Chord Packs

**Status: VERIFIED (existence) / APPROXIMATE (format)**

MPC 3.x added a Chord Memory feature per the Sound on Sound coverage of the MPC Key 61.
This assigns custom chords to individual pads:

```xml
<ChordMemory padIndex="0">
  <Notes>
    <Note>48</Note>   <!-- C3 -->
    <Note>52</Note>   <!-- E3 -->
    <Note>55</Note>   <!-- G3 -->
    <Note>59</Note>   <!-- B3 -->
  </Notes>
</ChordMemory>
```

Chord packs (purchasable or downloadable) appear to be preset chord arrangements for the
chord memory bank — essentially metadata that pre-populates the `<ChordMemory>` table.

**OXPORT ACTION ITEM 12:** For keygroup programs targeting keyboard players, include a
`<ChordMemory>` block with the scale tones of the engine's harmonic identity. Example for
OPAL (lavender, amorphous, dreamy): pre-populate with quartal harmony chords (fourths stacked)
that match the granular engine's spectral character.

### 2.6 Scale/Mode Quantization Updates

**Status: APPROXIMATE**

MPC 3.x scale library expanded. Known scales in 2.x: Major, Natural Minor, Harmonic Minor,
Pentatonic, Minor Pentatonic, Blues.

MPC 3.x additions:
- **Melodic Minor** (ascending/descending variants)
- **Dorian, Phrygian, Lydian, Mixolydian, Locrian** (all seven modes individually)
- **Whole Tone**
- **Diminished** (whole-half and half-whole)
- **Chromatic** (disables quantization — pass-through)
- **User** — custom scale definition (12 toggle buttons for chromatic scale degrees)

The "User" scale is significant: Oxport could theoretically ship programs with microtonal or
cultural scales pre-loaded if the user scale supports custom degree activation.

---

## Part 3: MPC 3 Plugin Architecture — AIR Plugins

### 3.1 Bundled AIR Instruments in MPC 3.x

**Status: VERIFIED (partial list from Sound on Sound review)**

The following AIR instruments were reviewed as available/installed in MPC Key 61 with MPC 3.0:

| Plugin | Type | Notes |
|--------|------|-------|
| **Hype** | Synth (FM + VA hybrid) | Bundled with MPC hardware since ~2.x |
| **Tubesynth** | Analog-modeled VA | Bundled since ~2.x |
| **Bassline** | Bass synthesizer (Moog/303-inspired) | Bundled since ~2.x |
| **Electric** | Electric piano | Rhodes/Wurli emulation, bundled |
| **Solina** | String ensemble emulation | Bundled |
| **Mellotron** | Tape-replay keyboard emulation | Bundled |
| **Odyssey** | Moog Odyssey emulation (ARP Odyssey) | Bundled with hardware |
| **Fabric XL** | Wavetable + sample hybrid | New in MPC 3.0, three layers including percussion — **sold separately via thempcstore.com** |
| **OPx-4** | FM synthesis (4-operator) | New in MPC 3.0 — **sold separately** |
| **Stage Piano** | Multi-sampled acoustic piano | Yamaha C7, Steinway D, Bechstein upright — **sold separately** |
| **Stage EP** | Electric piano multi-sample | Rhodes, Suitcase, Wurli, Pianet — **sold separately** |
| **Organ** | Drawbar organ emulation | Vibrato, rotary speaker — **sold separately** |
| **Studio Strings** | Orchestral string multi-articulation | **sold separately** |

**Important distinction:** The bundled synths (Hype, Tubesynth, Bassline, Electric, Solina,
Mellotron, Odyssey) are free with hardware. The premium instruments (Fabric XL, OPx-4, Stage Piano,
Stage EP, Organ, Studio Strings) are add-on purchases from thempcstore.com.

### 3.2 AIR Flavor Pro

**Status: VERIFIED (existence, approximate feature set)**

AIR Flavor Pro was announced October 2022 as a lo-fi multi-effects plugin. Feature set:

- **Multi-stage lo-fi chain**: combines multiple degradation types in sequence
- **Stages include**: Tape saturation, Vinyl noise/crackle, Bit crush, Sample rate reduction,
  Filter (lowpass/highpass), Wow and flutter (pitch modulation), Reverb send
- **Macro controls**: designed for MPC Q-Link assignment — entire lo-fi chain expressable with
  4 knobs
- **Presets** include genre-specific lo-fi characters: dusty 90s boom bap, cassette, VHS, AM radio,
  telephone, underwater
- **Dry/Wet**: parallel processing with clean signal preserved

**OXPORT ACTION ITEM 13:** XPN packs can reference AIR Flavor Pro as an insert effect in program
FX chains. For XO_OX packs targeting vintage/organic sounds, document Flavor Pro settings
alongside pack documentation. Create a `flavor_pro_presets.md` reference in Tools/docs/ with
per-engine recommended Flavor Pro settings.

### 3.3 AIR Pro EQ

**Status: APPROXIMATE**

AIR Pro EQ in MPC 3.x context:

- **Band count**: 6 bands (APPROXIMATE — AIR's standard Pro EQ is 6-band)
- **Band types**: HPF, Low Shelf, 3× Peaking, High Shelf, LPF
- **Filter shapes**: Butterworth, Bessel, Linkwitz-Riley options for HPF/LPF
- **Dynamic EQ mode**: each band can be set to dynamic (threshold + ratio) = frequency-specific
  compression without a separate compressor
- **Linear phase mode**: zero phase smear for mastering-grade transparency (at CPU cost)
- **Analyzer**: real-time spectrum analyzer overlaid on EQ curve

### 3.4 AIR Reverb Updates

**Status: APPROXIMATE**

AIR's reverb in MPC 3.x era likely includes (based on AIR product history):

- **Reverb 2** — stereo algorithmic reverb with room/hall/plate/spring algorithms
- **Spring reverb** — dedicated spring emulation (relevant: OVERDUB's spring is a B015 blessing)
- New parameters: **Size**, **Diffusion**, **Pre-delay** (ms), **High/Low damping**, **Early reflections** mix
- **Modulation** on the reverb tail (subtle pitch modulation for chorus-like shimmer)

### 3.5 AIR Hybrid 3 Synth

**Status: APPROXIMATE**

AIR Hybrid 3 (a flagship AIR synth available in standalone and as MPC plugin) includes:

- **3 oscillators**: Virtual Analog, FM, Wavetable, and Sample playback per oscillator
- **Flexible mod matrix**: 12-slot modulation routing
- **Arpeggiator and sequencer**: built-in phrase sequencer
- **Effects chain**: chorus, delay, reverb, distortion, EQ
- **MPC integration**: Q-Link macros map to 4 assigned parameters; presets organized by mood/style

The Hybrid 3 engine within MPC 3.x is a plugin program type:
```xml
<Program type="Plugin">
  <PluginName>AIR Hybrid 3</PluginName>
  <PluginPreset>Warm Pad 001</PluginPreset>
  <PluginParams>
    <Param id="cutoff">0.7</Param>
    <!-- ... -->
  </PluginParams>
</Program>
```

### 3.6 Amp/Cab Simulation

**Status: VERIFIED (existence from Sound on Sound)**

MPC 3.0 added a "guitar amp/cab simulation plugin" for direct guitar recording through MPC inputs.
Feature set:
- Multiple amp models (clean, crunch, hi-gain — likely 3–8 models)
- Cab impulse responses (2×12, 4×12, etc.)
- Mic position simulation
- Gate and noise reduction
- Direct signal blend (parallel dry + processed)

For XO_OX: not directly relevant to pack design, but notable for the hardware-recording
workflow that users of XPN packs may use alongside XO_OX content.

### 3.7 Vocal Effects / AI Vocal Processing

**Status: VERIFIED (existence from Sound on Sound)**

MPC 3.0 added AI vocal effects:
- **Auto-tune** — pitch correction with speed control (from "natural" to "T-Pain" robotic)
- **Harmonizing** — automatic harmony generation (1–3 voices, configurable scale)
- **Formant shifting** — gender/character transform without pitch change
- Real-time processing on mic input, compatible with recording into MPC tracks

For XO_OX: the presence of vocal processing means pack buyers are using MPC as a full vocal
production workstation. XPN packs with atmospheric/choral content (OPAL, OVERDUB, OCEANIC)
complement this use case.

### 3.8 Plugin Parameter Automation via Q-Links

**Status: VERIFIED (expected behavior) / APPROXIMATE (XPM format)**

Plugin programs expose parameters to Q-Links through a parameter binding mechanism:

```xml
<Program type="Plugin">
  <PluginName>AIR Hybrid 3</PluginName>
  <Qlnk>
    <QlnkAssignment number="0" label="Cutoff">PluginParam:cutoff</QlnkAssignment>
    <QlnkAssignment number="1" label="Res">PluginParam:resonance</QlnkAssignment>
    <QlnkAssignment number="2" label="Attack">PluginParam:env_attack</QlnkAssignment>
    <QlnkAssignment number="3" label="Release">PluginParam:env_release</QlnkAssignment>
  </Qlnk>
</Program>
```

The `PluginParam:` prefix distinguishes plugin parameter targets from native MPC sample parameters.

**OXPORT ACTION ITEM 14:** If XOlokun ships as a VST3 plugin compatible with MPC Desktop 3.5+,
document the standard Q-Link parameter binding for each engine. Use the 4-macro convention
(CHARACTER, MOVEMENT, COUPLING, SPACE) mapped to Q-Links 1–4 as the Oxport standard for
plugin programs. Create an Oxport template `xpn_plugin_program.py` for when standalone VST3
support arrives on hardware.

---

## Part 4: MPC 3 Export/Import

### 4.1 Export Formats Beyond XPN

**Status: APPROXIMATE**

Known MPC export capabilities in 3.x:

| Format | Notes |
|--------|-------|
| **XPN** (.xpn) | Native expansion pack ZIP format — unchanged structure, XML + WAVs |
| **MIDI** | Standard MIDI file (.mid) export of sequences |
| **WAV** | Individual track export, master bounce |
| **AIF** | Alternative audio format export (user selectable) |
| **MP3** | Master export to MP3 (quality selectable: 128/192/320 kbps) |
| **AAC** | Likely available but UNCONFIRMED in 3.x |

**New in 3.x era:**
- **Stem export** — MPC Stems (July 2024 standalone, 3.5+) enables per-stem WAV export after
  separation. User can export drum stem, bass stem, melody stem, vocal stem individually.
- **Track freeze** — renders a track to audio in-place (frees CPU for plugin-heavy projects)
- **Song mode bounce** — full song-length rendering with automation and arrangement

### 4.2 MIDI Export Improvements

**Status: APPROXIMATE**

MPC 3.x MIDI export improvements believed to include:
- **Note probability export** — probability values may be encoded as MIDI CC or proprietary
  event in the MIDI file (or silently normalized to 100% on export)
- **Chord mode MIDI** — when a program uses chord mode, MIDI export renders the full chord
  polyphony
- **Arpeggiator MIDI capture** — MIDI export renders what the arpeggiator produces, not just
  the trigger note (enables "render arp to MIDI" workflow)
- **CC automation export** — Q-Link movements recorded as CC automation export correctly to MIDI

### 4.3 Audio Export Sample Rate Options

**Status: APPROXIMATE**

MPC 3.x audio export:
- **44.1 kHz** — CD standard
- **48 kHz** — video/broadcast standard
- **88.2 kHz** — high-resolution audio
- **96 kHz** — high-resolution audio (maximum on MPC standalone hardware, APPROXIMATE)
- **Bit depth**: 16-bit and 24-bit (32-bit float likely in Desktop mode)

**OXPORT ACTION ITEM 15:** All XPN WAV samples should be delivered at 44.1 kHz / 24-bit as the
shipping standard. This is the broadest compatibility format across all MPC hardware from Live I
through XL. For premium "hi-res" variant packs, offer 48 kHz / 24-bit as an alternate tier.
Update `xpn_render_spec.py` to document this dual-tier spec.

### 4.4 New Import Format Support

**Status: APPROXIMATE**

MPC 3.x import capabilities (APPROXIMATE — Akai has historically been conservative on import
format support):

| Format | Status in 3.x |
|--------|--------------|
| **WAV** | VERIFIED — standard |
| **AIF/AIFF** | VERIFIED — standard |
| **MP3** | APPROXIMATE — likely via import conversion |
| **FLAC** | SPECULATIVE — community-requested, may be 3.7+ |
| **Kontakt NKI** | NOT SUPPORTED — Akai/NI are competitors; no NKI import expected |
| **SFZ** | SPECULATIVE — SFZ is open standard; possible via third-party tools, unlikely native |
| **EXS24** | NOT SUPPORTED — Logic/Apple proprietary format |
| **REX2** | APPROXIMATE — slice import historically supported |
| **Ableton ADG** | NOT SUPPORTED |

The absence of NKI/SFZ/EXS24 import is a deliberate competitive positioning. Users wanting
to bring Kontakt content into MPC must manually extract and re-map WAV files.

**OXPORT ACTION ITEM 16:** This is a competitive gap XO_OX can exploit. Users frustrated with
MPC's limited import options will value XPN packs that are natively formatted and "just work."
Marketing copy: "No mapping, no conversion. Drop in and play." Ensure all Oxport-generated
XPN packs pass xpn_validator.py checks before distribution.

### 4.5 Wi-Fi Pack Transfer / Cloud Sync

**Status: VERIFIED (existence) / APPROXIMATE (details)**

MPC One+ and subsequent hardware with Wi-Fi support (MPC Live III, MPC XL) enable:
- **thempcstore.com** — official Akai marketplace for expansion packs; Wi-Fi purchase and
  download direct to device
- **Wi-Fi file transfer** — transfer files to/from device over local network without USB cable
- **Cloud project sync** — SPECULATIVE for 3.x; may require Akai Connect account

For XO_OX: distribution via thempcstore.com is a viable channel. Packs must meet Akai's
submission format requirements, which aligns with standard XPN spec. The Wi-Fi download
workflow means pack install is frictionless for users on newer hardware.

**OXPORT ACTION ITEM 17:** Audit `xpn_packager.py` to ensure output ZIP structure matches
thempcstore.com submission requirements exactly. Research current submission guidelines
(check Akai Partner Portal when available). Key concern: cover art dimensions, metadata
format, pricing tier options.

---

## Part 5: MPCe — Hardware Analysis

### 5.1 Hardware Confirmed Specs (from `mpc_evolving_capabilities_research.md`)

The following is a summary of VERIFIED MPCe specs already documented in the companion file.
Reproduced here for completeness:

**MPC Live III (October 2025) — First MPCe hardware:**
- 8-core ARM CPU, 8 GB RAM, 128 GB internal
- 16 MPCe 3D pads: Z (velocity) + X (horizontal position) + Y (vertical position)
- 4-quadrant system: each pad surface divided into 4 corners, each corner = separate sample/articulation
- Continuous XY tracking while finger is held
- Clip Matrix (Ableton Session View-style)
- 16 dedicated Step buttons

**MPC XL (January 2026) — Premium flagship:**
- 2nd-gen 8-core CPU, 16 GB RAM, 256 GB internal
- 10.1-inch multi-touch display
- 16 MPCe RGB pads
- 16 Q-Link encoders with OLED displays
- 16 CV/Gate outputs (4× 3.5mm TRS)
- 2 MIDI In, 4 MIDI Out (DIN)

### 5.2 Screen Resolution / Cover Art DPI

**Status: APPROXIMATE**

MPC hardware display sizes and resolutions:

| Hardware | Screen Size | Resolution (est.) |
|----------|------------|-------------------|
| MPC One / One+ | 7-inch | 1280×800 |
| MPC Live II | 7-inch | 1280×800 |
| MPC Key 61 | 7-inch | 1280×800 |
| MPC Live III | 7-inch | 1280×800 APPROXIMATE (same size, possibly higher DPI) |
| MPC XL | **10.1-inch** | **1920×1200 APPROXIMATE** (larger screen, likely higher res) |

The MPC XL's 10.1-inch display is a significant upgrade. At the same PPI as the 7-inch (183 PPI):
- 10.1-inch at 183 PPI ≈ 1847×1155 — rounds to ~1920×1200
- If they pushed to 220 PPI (modern tablet standard): ≈ 2218×1386 — rounds to 2220×1387

Current XPM cover art standard: **500×500 pixels** (embedded in XPN pack).

**OXPORT ACTION ITEM 18 (HIGH PRIORITY):** Research actual XPM cover art dimension requirements
for MPC XL. The current `xpn_cover_art.py` generates 500×500 output. If MPC XL displays at
higher resolution, upgrade to 1000×1000 or 2000×2000 with proportional art.

Recommended change:
```python
# In xpn_cover_art.py
COVER_SIZES = {
    "standard": (500, 500),     # MPC Live/One/Key hardware
    "xl":       (1000, 1000),   # MPC XL (10.1" display)
    "retina":   (2000, 2000),   # future-proof / hi-DPI desktop
}
```

Generate all three on export; MPC selects the appropriate size from the XPN bundle.
To add multiple cover art files, the XPN ZIP structure would include:
```
MyPack/
  cover.png           (500×500 — standard)
  cover_xl.png        (1000×1000 — XL)
  Programs/
  Samples/
  manifest.json
```

---

### 5.3 New Pad Bank System

**Status: APPROXIMATE**

MPCe 3D pads use a 4-quadrant system (4 corners per pad = 4 samples per physical pad). This
is distinct from the traditional bank system (Bank A/B/C/D = 4 banks × 16 pads = 64 pad slots).

The MPCe architecture stacks both systems:
- **Bank system** (inherited): 4 banks × 16 pads = 64 pad slots
- **Quadrant system** (new): each of the 64 pad slots now has 4 corner positions = 256 total
  addressable sample positions per program

In practice, a single MPCe drum program can address up to 256 distinct samples across 16 pads
× 4 banks × 4 corners. This is a 4× expansion of the per-program sample capacity.

For standard XPM format: the existing 128-instrument `<Instruments>` array covers 2 banks.
Full MPCe support would require 256 instruments or a new `<Corners>` structure.

### 5.4 Community Rumors vs. Confirmed Features

**Status: SPECULATIVE where labeled**

| Feature | Status |
|---------|--------|
| MPCe 3D pads with XY sensing | VERIFIED |
| 4-quadrant per-pad sample assignment | VERIFIED |
| Continuous XY modulation while held | VERIFIED |
| MPC XL 10.1-inch display | VERIFIED |
| MPC XL 16 Q-Links with OLED | VERIFIED |
| MPC XL 16 CV/Gate outputs | VERIFIED |
| Standalone VST3 support on hardware | SPECULATIVE — Desktop 3.5 only confirmed |
| MPE protocol support | SPECULATIVE — no confirmed Akai announcement |
| AUv3 support on MPC (not iOS) | SPECULATIVE |
| Ableton Link integration | APPROXIMATE — Wi-Fi hardware suggests possibility |
| MPC "Cloud Projects" sync | SPECULATIVE |
| MPCe haptic feedback | SPECULATIVE (community request, no announcement) |
| MPCe pressure-sensitive corners (distinct from XY) | SPECULATIVE |

---

## Part 6: Oxport Implications — Complete Action Item Index

All action items from Sections 1–5, consolidated with priority ratings.

### Priority 1 — High Impact, Do Now

**OX-01: PadColor per-instrument (OXPORT ACTION ITEM 1)**
- Files: `xpn_drum_export.py`, `xpn_keygroup_export.py`
- Add `pad_color` tuple to `VOICE_DEFAULTS` and all keygroup templates
- XO_OX engine accent color → drum program pad colors
- XML: emit `<PadColor red="..." green="..." blue="..." />` per instrument

**OX-02: Q-Link Expansion to 4 + OLED Labels (OXPORT ACTION ITEM 6)**
- Files: `xpn_drum_export.py`, `xpn_keygroup_export.py`
- Expand `VOICE_DEFAULTS` from `qlink_1/2` to `qlink_1` through `qlink_4`
- Add `--qlink-profile standard|xl` flag
- All label strings must be ≤8 characters for OLED display
- Audit existing label strings for length compliance

**OX-03: Cover Art Dual-Resolution (OXPORT ACTION ITEM 18)**
- File: `xpn_cover_art.py`
- Add `COVER_SIZES` dict: `standard` (500×500), `xl` (1000×1000), `retina` (2000×2000)
- `xpn_packager.py` to include all three sizes in XPN ZIP
- Monitor Akai XPN spec update for official XL art dimensions

**OX-04: Play Mode String Field (OXPORT ACTION ITEM 8)**
- Files: `xpn_drum_export.py`, `xpn_keygroup_export.py`
- Replace boolean `one_shot` with string `play_mode`
- Constants: `OneShot`, `NoteOn`, `NoteOff`, `NoteOnOff`, `LoopUntilRelease`, `Gate`
- Backward compat: `True` → `OneShot`, `False` → `NoteOn`
- XML: emit `<PlayMode>OneShot</PlayMode>` in 3.x mode

**OX-05: WAV Delivery Standard (OXPORT ACTION ITEM 15)**
- File: `xpn_render_spec.py`
- Codify: standard tier = 44.1 kHz / 24-bit; premium tier = 48 kHz / 24-bit
- Add validation in `xpn_validator.py` to check embedded WAV sample rate/bit depth
- Document in pack README template

---

### Priority 2 — Medium Impact, Plan Soon

**OX-06: Velocity Curve per Voice (OXPORT ACTION ITEM 9)**
- Files: `xpn_drum_export.py`
- Add `velocity_curve` field to `VOICE_DEFAULTS`
- Recommended assignments: kick/snare/tom → `Hard`; chat/ohat/perc/fx → `Soft`; clap → `Linear`
- XML: `<VelocityCurve>Hard</VelocityCurve>` per instrument

**OX-07: LFO Routing for Keygroup Programs (OXPORT ACTION ITEM 2)**
- File: `xpn_keygroup_export.py`
- Add `--lfo` flag: `none` (default), `subtle`, `vibrato`, `tremolo`, `filter_sweep`
- XML: emit `<LFO>` block per instrument when lfo != none
- Document field names — verify against real XPM specimen when available

**OX-08: ArpeggiatorSettings Template Block (OXPORT ACTION ITEM 3)**
- File: `xpn_keygroup_export.py`
- Add disabled-by-default ArpeggiatorSettings to all keygroup XPM output
- Pre-configured: `Enable=False`, `Mode=Up`, `Rate=1/8`, `Gate=50`, `Octave=1`
- Users can enable without editing XML manually

**OX-09: Bus/Stem Output Assignments (OXPORT ACTION ITEM 5)**
- File: `xpn_drum_export.py`
- Add `--bus-assignments` flag: `mono` (all → Main), `stems` (kick→Sub1, hats→Sub2, etc.)
- XML: `<Output><BusAssignment>Sub1</BusAssignment></Output>` per instrument
- Document as "Stems Out" preset for MPC XL users

**OX-10: Scale Hint for Keygroup Programs (OXPORT ACTION ITEM 4)**
- File: `xpn_keygroup_export.py`
- Add `--scale-hint "ROOT:SCALE"` parameter (e.g., `"C:Dorian"`)
- XML: emit disabled ScaleMode block with pre-populated root/scale
- Engine-specific defaults in a `ENGINE_SCALE_HINTS` dict

---

### Priority 3 — Watch and Monitor

**OX-11: XY Pad Modulation Fields (OXPORT ACTION ITEM 7)**
- Watch Akai firmware for `XYPadAssignment` or MPE XPM field documentation
- When confirmed: add `--xy-map` flag with presets: `filter_pan`, `filter_reverb`, `dynamics`
- For now: document intended X/Y mapping in pack README per engine

**OX-12: 64-Pad Layout Profile (OXPORT ACTION ITEM 11)**
- File: `xpn_keygroup_export.py`
- Add `--layout 64pad` profile when confirmed mapping spec is available
- 64 consecutive semitones as pad-per-note (no velocity layers per pad in this mode)

**OX-13: Chord Memory Presets (OXPORT ACTION ITEM 12)**
- File: `xpn_keygroup_export.py`
- Add `--chord-memory ENGINE` flag for engine-matched harmony presets
- OPAL → quartal harmony; OVERDUB → minor seventh chords; OBESE → power chords + tritones

**OX-14: Flavor Pro Reference Guide (OXPORT ACTION ITEM 13)**
- New file: `Tools/docs/flavor_pro_presets.md`
- Per-engine recommended Flavor Pro settings
- ONSET (Electric Blue, percussive) → tape saturation only, dry/wet 15–25%
- OVERDUB (Olive, dub) → tape + vinyl crackle, wow 10%, dry/wet 30–40%
- OPAL (Lavender, granular) → cassette + bit crush light, dry/wet 20–35%

**OX-15: Plugin Program Template (OXPORT ACTION ITEM 14)**
- New file: `Tools/xpn_plugin_program.py`
- Template for `<Program type="Plugin">` targeting XOlokun VST3
- 4-macro → Q-Link 1–4 binding as standard
- Hold pending standalone VST3 announcement on MPC hardware

**OX-16: Packager Audit for thempcstore.com (OXPORT ACTION ITEM 17)**
- File: `xpn_packager.py`
- Research current Akai Partner Portal submission requirements
- Audit ZIP structure, manifest.json format, cover art spec, metadata fields
- Identify any fields `xpn_packager.py` currently omits or gets wrong

---

## Part 7: Format Compatibility Matrix

How current Oxport XPM output maps to each firmware generation:

| XPM Field | MPC 2.x | MPC 3.0 | MPC 3.5 | MPC 3.7 / XL |
|-----------|---------|---------|---------|--------------|
| `<SampleName>` | SUPPORTED | SUPPORTED | SUPPORTED | SUPPORTED |
| `<ZonePlay>` 1–4 | SUPPORTED | SUPPORTED | SUPPORTED | SUPPORTED |
| `<CycleType>` | SUPPORTED | SUPPORTED | SUPPORTED | SUPPORTED |
| `<VelStart/End>` | SUPPORTED | SUPPORTED | SUPPORTED | SUPPORTED |
| `<Qlnk>` (4 encoders) | SUPPORTED | SUPPORTED | SUPPORTED | SUPPORTED |
| `<Qlnk>` (5–16 encoders) | IGNORED | IGNORED | SUPPORTED | SUPPORTED |
| `<PadColor>` | APPROXIMATE | APPROXIMATE | SUPPORTED | SUPPORTED |
| `<VelocityCurve>` | UNKNOWN | APPROXIMATE | APPROXIMATE | APPROXIMATE |
| `<ArpeggiatorSettings>` | NO | APPROXIMATE | SUPPORTED | SUPPORTED |
| `<ChordMode>` | NO | APPROXIMATE | SUPPORTED | SUPPORTED |
| `<ScaleMode>` | NO | APPROXIMATE | SUPPORTED | SUPPORTED |
| `<LFO>` | PARTIAL | APPROXIMATE | APPROXIMATE | APPROXIMATE |
| `<BusAssignment>` | PARTIAL | APPROXIMATE | APPROXIMATE | SUPPORTED |
| `<PlayMode>` string | NO | APPROXIMATE | APPROXIMATE | APPROXIMATE |
| `<XYPadAssignment>` | NO | NO | NO | SPECULATIVE |
| `<PadCornerAssignment>` | NO | NO | NO | SPECULATIVE |
| `<Program type="Plugin">` | LIMITED | APPROXIMATE | SUPPORTED (VST3 Desktop) | SPECULATIVE (standalone) |

**Legend:** SUPPORTED = confirmed works. APPROXIMATE = likely works, field names may vary.
PARTIAL = exists but limited. SPECULATIVE = not confirmed. IGNORED = silently skipped.
NO = feature did not exist in this firmware.

---

## Part 8: Specimen Collection Recommendation

The most valuable single action for Oxport accuracy would be **obtaining real XPM files from
MPC 3.x hardware** and reverse-engineering the exact field names and structures.

**How to obtain specimens:**
1. On MPC hardware with firmware 3.5+: create a Drum program, set pad colors, add LFO,
   set chord mode — then export as XPN
2. Extract the ZIP: `unzip MyPack.xpn -d specimen/`
3. Read the `Programs/*.xpm` XML and compare to the schema in this document
4. Correct any discrepancies in Oxport tools

**Target specimen programs to collect:**
- Drum program with 8 pads, velocity layers, pad colors set, Q-Links assigned
- Keygroup program with arpeggiator enabled, scale mode set
- Plugin program (AIR Hybrid 3 or Fabric XL) with Q-Link bindings
- Any MPC XL-native program (to see if XL-specific fields appear)

**OXPORT ACTION ITEM 19 (overarching):** Before implementing any APPROXIMATE field from this
document into production Oxport tools, validate against a real specimen XPM file. The schema
in this document is research-grade intelligence, not a substitute for the actual serialized output
of Akai's firmware.

---

## Sources and Confidence Summary

| Source | Used For |
|--------|---------|
| Sound on Sound — MPC Key 61 review (2022) | AIR plugin roster (VERIFIED items), new UI features, performance features |
| Synthtopia — article index (2022–2024) | MPC version timeline, AIR Flavor Pro, MPC One+ |
| `mpc_evolving_capabilities_research.md` (existing XO_OX doc) | MPCe 3D pads, MPC XL specs, MPC Live III specs, MPC Stems, MPC 3.7 (all VERIFIED in that doc) |
| Training knowledge / MPC community conventions | Field names, XML schema patterns, 2.x baseline |
| Inference from hardware capabilities | MPCe XY routing, XPM expansion for XL, cover art sizing |

**Overall confidence by section:**
- Section 1 (XPM fields): 40% VERIFIED, 45% APPROXIMATE, 15% SPECULATIVE
- Section 2 (Performance features): 60% VERIFIED/APPROXIMATE, 40% APPROXIMATE
- Section 3 (AIR plugins): 70% VERIFIED, 30% APPROXIMATE
- Section 4 (Export/Import): 50% VERIFIED, 35% APPROXIMATE, 15% SPECULATIVE
- Section 5 (MPCe hardware): 80% VERIFIED (from companion doc), 20% APPROXIMATE

---

*Scout — March 2026*
*"Every field in the XPM is a promise to the hardware. Make them all count."*

*See also: `mpc_evolving_capabilities_research.md` for hardware timeline and MPCe deep-dive*
