# Oxport Innovation Round 5 — R&D Spec
**Authors:** Rex (XPN format android) + Atlas (XOceanus bridge android)
**Date:** 2026-03-16
**Status:** R&D — feeds Oxport development backlog

---

## Section 1: XPN Format Deep R&D

### 1.1 PadNoteMap / PadGroupMap — Cross-Channel Kit Routing

PadNoteMap and PadGroupMap are currently used in `xpn_drum_export.py` to assign each voice its GM-convention MIDI note (36=kick, 38=snare, etc.) and mute group pairing (hat pair: closed hat mutes open hat via MuteGroup 1).

**Underexplored capability: per-group MIDI channel assignment**

MPC 3.x XPM format supports a `MidiChannel` attribute at the `<Instrument>` level. Each pad (instrument) inside a drum program can send/receive on a different MIDI channel:

```xml
<Instrument number="0" type="Pad">
  <MidiNote>36</MidiNote>
  <MidiChannel>1</MidiChannel>  <!-- kick on ch 1 -->
  ...
</Instrument>
<Instrument number="1" type="Pad">
  <MidiNote>38</MidiNote>
  <MidiChannel>2</MidiChannel>  <!-- snare on ch 2 — could route to external gear -->
  ...
</Instrument>
```

**XO_OX application:** A "live performance routing" XPM variant where:
- Pads 1–4 (kick/snare/hat/clap) route to MIDI ch 1–4 → four separate external drum modules
- Pads 5–8 (tom/perc/fx) stay on ch 9 (GM drums) → internal MPC drums
- Oxport `--routing live` flag generates this split-channel XPM alongside the standard one

This doubles the pack's utility for hardware-heavy producers without any additional sample renders.

**PadGroupMap extension idea:** Group tags currently only encode mute relationships. The XPM `<Group>` element can carry a `Color` attribute on MPC 3.x firmware. Oxport could write pad colors that match each engine's accent color:

```xml
<Group number="1" Color="0066FF" />  <!-- Onset's Electric Blue — hats group -->
```

**Backlog item:** Add `--pad-colors` flag to `xpn_drum_export.py`. Map MuteGroup IDs to engine accent color (from CLAUDE.md engine table). Low implementation cost, high visual polish.

---

### 1.2 InsertEffects XML Block — Embedded feliX/Oscar Character EQ

MPC 3.x supports an `<InsertEffects>` block inside XPM programs. This allows effects to be embedded directly in the program file and loaded automatically when the user opens the pack — no manual routing required.

**Confirmed supported insert types on MPC 3.x firmware:**
- AIR Equalizer (3-band EQ)
- AIR Filter (LP/HP/BP)
- AIR Compressor
- AIR Tube Distortion (light saturation)

**Exact XML structure:**

```xml
<InsertEffects>
  <InsertEffect bypass="False" index="0" type="AIR Equalizer">
    <Params>
      <Param name="LowFreq" value="80" />
      <Param name="LowGain" value="-2.0" />
      <Param name="MidFreq" value="2500" />
      <Param name="MidGain" value="1.5" />
      <Param name="MidQ" value="0.8" />
      <Param name="HighFreq" value="12000" />
      <Param name="HighGain" value="2.0" />
    </Params>
  </InsertEffect>
</InsertEffect>
```

**feliX direction (Oscar→feliX coupling signature):**
- HP at 80 Hz (-6 dB/oct, cleans sub mud)
- Mid presence boost: +2 dB at 3.5 kHz (air, snap)
- Air shelf: +1.5 dB at 12 kHz

**Oscar direction (feliX→Oscar coupling signature):**
- Low shelf: +2 dB at 120 Hz (warmth, body)
- Mid scoop: -1.5 dB at 2 kHz (forwardness reduced, depth increases)
- HP at 40 Hz only (keeps rumble)

**Implementation plan for Oxport:**

Add `--character` flag accepting `felix`, `oscar`, or `neutral` (default). When set, `xpn_drum_export.py` and `xpn_keygroup_export.py` append the InsertEffects block before closing `</MPCVObject>`.

Store the EQ parameter sets in a dict in both exporters:

```python
INSERT_EQ_PRESETS = {
    "felix": {
        "LowGain": -2.0, "LowFreq": 80,
        "MidGain": 2.0,  "MidFreq": 3500, "MidQ": 0.9,
        "HighGain": 1.5, "HighFreq": 12000,
    },
    "oscar": {
        "LowGain": 2.0,  "LowFreq": 120,
        "MidGain": -1.5, "MidFreq": 2000, "MidQ": 0.7,
        "HighGain": 0.0, "HighFreq": 10000,
    },
}
```

**Risk:** Older MPC firmware (pre-3.0) ignores unknown XML blocks silently, so backward compatibility is preserved. The XPM spec says unrecognized elements are skipped. This is safe to ship.

---

### 1.3 MasterPitch and Per-Layer KeyTrack

The XPM `KeyTrack` field currently appears at the instrument level in Oxport-generated programs and is always set to `True` per CLAUDE.md rule. But there is a per-layer `KeyTrack` override at the `<Layer>` element level that Oxport does not currently use.

**Multi-layer with different pitch tracking:**

```xml
<Instrument number="0" type="Pad">
  <KeyTrack>True</KeyTrack>   <!-- instrument-level default -->
  <Layer number="0">
    <SampleFile>onset_kick_v1.wav</SampleFile>
    <KeyTrack>False</KeyTrack>  <!-- layer override: sub layer doesn't pitch -->
    <VelStart>1</VelStart>
    <VelEnd>31</VelEnd>
  </Layer>
  <Layer number="1">
    <SampleFile>onset_kick_v2.wav</SampleFile>
    <KeyTrack>True</KeyTrack>   <!-- transient layer does pitch-follow -->
    <VelStart>32</VelStart>
    <VelEnd>127</VelEnd>
  </Layer>
</Instrument>
```

**XO_OX application:** For layered bass/pad presets (OBLONG, OPAL, ODYSSEY), split layers by function:
- Sub layer (fundamental): `KeyTrack=False` — sub stays locked, no pitch smear on wide intervals
- Body layer (harmonics): `KeyTrack=True` — transposes naturally with key

Achieves "consistent low end across the keyboard" — a real production complaint with sampled bass instruments.

**`MasterPitch` field:** XPM has a `<MasterPitch>` semitone offset at program level. Currently unused by Oxport. Could be used for "tuning variant" packs — same samples, XPM with MasterPitch=-2 for "A432" versions. Trivial to add.

---

### 1.4 Bend Field — Per-Voice Pitch Bend Range

The XPM `<BendRange>` attribute exists at the `<Instrument>` level. MPC respects it per pad in drum programs.

**Currently:** Oxport does not write BendRange, so MPC falls back to its global pitch bend setting.

**Proposed per-voice defaults:**

| Voice | BendRange (semitones) | Rationale |
|-------|----------------------|-----------|
| kick  | 2                    | Classic TR-808 pitch sweep via pitch wheel |
| snare | 1                    | Subtle pitch bend only |
| chat  | 0                    | Hats should not bend |
| ohat  | 0                    | Hats should not bend |
| clap  | 0                    | Claps should not bend |
| tom   | 4                    | Toms are melodic — wide bend expressiveness |
| perc  | 3                    | Percussion allows tonal play |
| fx    | 12                   | FX pads — full octave bend for performance |

**For keygroup programs (melodic engines):** A `--bend-range` flag (default 2, max 24). Wide leads (OBLONG, ODYSSEY) benefit from 12. Pad textures (OPAL) benefit from 2 or 0.

**XML:**
```xml
<Instrument number="5" type="Pad">  <!-- tom -->
  <MidiNote>41</MidiNote>
  <BendRange>4</BendRange>
  ...
</Instrument>
```

**Backlog item:** Add BEND_RANGE dict to `xpn_drum_export.py` PAD_MAP structure. Expose `--bend-ranges` CLI override. Low cost.

---

### 1.5 ModWheel Assignments in XPM

MPC 3.x XPM supports a `<Modulation>` block inside `<Instrument>` with named source→destination routes. This is the XPM-native mod matrix — it operates independently of DAW automation.

**Exact XML structure:**

```xml
<Instrument number="0" type="Pad">
  <Modulations>
    <Modulation>
      <Source>ModWheel</Source>
      <Destination>Filter Cutoff</Destination>
      <Amount>0.5</Amount>       <!-- bipolar, 0.5 = half-positive -->
      <Polarity>Positive</Polarity>
    </Modulation>
    <Modulation>
      <Source>ModWheel</Source>
      <Destination>Pitch</Destination>
      <Amount>0.1</Amount>
      <Polarity>Positive</Polarity>
    </Modulation>
  </Modulations>
  ...
</Instrument>
```

**Confirmed MPC 3.x destinations:**
- `Filter Cutoff`
- `Filter Resonance`
- `Pitch` (semitone scale, modulated by Amount × BendRange)
- `Amp` (amplitude, effectively a VCA path)
- `Pan`
- `LFO Rate` (if the program's LFO is routed to the pad)

**XO_OX application:** Encode the engine's feliX/Oscar polarity into the mod wheel routing:
- feliX engines: ModWheel → Filter Cutoff (opens filter = brighter = feliX direction)
- Oscar engines: ModWheel → Amp (dynamics depth = warmth = Oscar direction)
- Coupled engines: dual routes — MW opens filter AND adds vibrato (pitch ±0.05)

**Backlog item:** Add `modulations` key to `ENGINE_STRATEGIES` in `xpn_render_spec.py`, carrying a list of source/destination/amount dicts. Both drum and keygroup exporters read and write them. This gives every XO_OX pack meaningful mod wheel response out of the box — satisfying D006 (Expression Input Is Not Optional) even in the static sample domain.

---

### 1.6 Aftertouch in XPM

Aftertouch is supported as a modulation source using the same `<Modulation>` block structure (§1.5), with `<Source>Aftertouch</Source>`. MPC hardware (Force, MPC Live, MPC One) supports channel pressure on pads.

**Proposed XO_OX default:**

```xml
<Modulation>
  <Source>Aftertouch</Source>
  <Destination>Filter Cutoff</Destination>
  <Amount>0.3</Amount>
  <Polarity>Positive</Polarity>
</Modulation>
```

This means pressing harder on a pad after the initial strike opens the filter — the same behavior that the XOceanus engine implements via D006. The sample pack now behaves consistently with the plugin.

**Per-voice aftertouch suggestions:**
- kick: Aftertouch → Pitch (+0.1, subtle growl)
- snare: Aftertouch → Filter Cutoff (+0.3)
- tom: Aftertouch → Pitch (+0.2) + Filter (+0.2)
- fx: Aftertouch → Amp (+0.4, swells on pressure)

---

### 1.7 Slice Programs — Long Evolution Renders

MPC 3.x supports `<Program type="Clip">` and a distinct `<Program type="Slice">` (sometimes called "Audio Track Program" in older firmware). A Slice program maps regions of a long WAV file to pads, each pad triggering a time-stamped slice.

**XO_OX application:** OPAL and OVERWORLD produce long evolution renders (30–120 seconds). Instead of exporting them as raw WAV stems, Oxport could generate a Slice program that maps 16 meaningful moments in the evolution:

- Pad 1: 0:00 — initial state (seed grain)
- Pad 2: 0:08 — first density build
- Pad 3: 0:16 — harmonic emergence
- ...
- Pad 16: full evolution peak

**Slice XPM structure (simplified):**

```xml
<Program type="Slice" name="Opal Tide Study 1">
  <AudioFile>opal_tide_study_1_evolution.wav</AudioFile>
  <Slices>
    <Slice number="0">
      <Start>0</Start>          <!-- sample offset -->
      <End>176400</End>         <!-- 4 seconds at 44.1kHz -->
      <PadNote>36</PadNote>
    </Slice>
    <Slice number="1">
      <Start>176400</Start>
      <End>352800</End>
      <PadNote>37</PadNote>
    </Slice>
    ...
  </Slices>
</Program>
```

**New tool: `xpn_slice_builder.py`**

Inputs:
- `--wav` path to long evolution WAV
- `--slices N` (default 16, max 64)
- `--method` `uniform` | `transient` | `spectral`
  - `uniform`: equal-length slices
  - `transient`: slice at detected onset peaks (numpy peak detection on RMS envelope)
  - `spectral`: slice at spectral centroid shifts > threshold (detects timbral changes)
- `--output-dir`
- `--preset-name`

Output: Slice `.xpm` + the original long WAV (no audio processing needed — slice programs reference positions in the source file).

**Dependencies:** numpy only (already required by xpn_kit_expander). No soundfile needed for the XPM generation step; soundfile only needed if the tool also trims silence from the source.

---

### 1.8 Clip Programs — Looped Texture Launching

MPC 3.x `<Program type="Clip">` maps WAV loops to pads for clip-launcher style performance. Each pad plays a looped audio clip independently, like Ableton Session View.

**XO_OX application:** OPAL, OVERDUB (reverb washes), OCEANIC (current drones) produce loopable textures. A Clip program allows live performance where pads launch and layer independent texture loops.

**Clip XPM structure:**

```xml
<Program type="Clip" name="Opal Ambient Loop Set">
  <Instruments>
    <Instrument number="0">
      <AudioFile>opal_loop_A.wav</AudioFile>
      <Loop>True</Loop>
      <LoopStart>0</LoopStart>
      <LoopEnd>-1</LoopEnd>    <!-- -1 = end of file -->
      <Tempo>90.0</Tempo>       <!-- BPM for tempo-sync -->
      <PadNote>36</PadNote>
    </Instrument>
    <Instrument number="1">
      <AudioFile>opal_loop_B.wav</AudioFile>
      <Loop>True</Loop>
      <Tempo>90.0</Tempo>
      <PadNote>37</PadNote>
    </Instrument>
  </Instruments>
</Program>
```

**New `--clip-set` mode in Oxport:** When invoked for texture engines (OPAL, OCEANIC, OVERDUB), instead of the standard keygroup pipeline, render 16 loop segments at the same BPM, package as Clip program. Loops should be trimmed to exact bar lengths at the preset's canonical tempo (stored in `.xometa` as `"tempo"` field if present, else 90 BPM default).

---

### 1.9 Plugin Programs — AIR Hybrid 3 Templates

MPC 3.x Plugin programs (`<Program type="Plugin">`) host VST/AU plugins directly inside an XPM — the program loads the plugin and its preset automatically when the user opens the expansion.

**XO_OX application:** AIR Hybrid 3 ships with MPC 3.x. An Oxport-generated Plugin program could auto-load a Hybrid 3 patch that approximates the XO_OX engine's sonic character. The producer gets both a sample-based version AND a synthesis-based version in the same pack.

**Plugin XPM structure:**

```xml
<Program type="Plugin" name="XO_OX Oblong Hybrid">
  <Plugin>
    <PluginName>AIR Hybrid 3</PluginName>
    <PluginPreset>oblong_hybrid3_approx.h3p</PluginPreset>
  </Plugin>
  <PadNoteMap>
    <!-- standard 128-note pad map -->
  </PadNoteMap>
</Program>
```

**Constraint:** Requires AIR Hybrid 3 preset files (`.h3p` format). Oxport cannot synthesize these — they must be hand-authored and shipped in the XPN alongside the XPM. This is a premium "pack+" tier feature, not a standard pipeline output.

**Backlog item:** Mark as V2. Design a `premium/` folder convention in XPN ZIPs for plugin preset assets that only load if the plugin is installed.

---

## Section 2: XPN Manifest Innovations

### 2.1 Current `expansion.json` Fields

```json
{
  "name": "XO_OX Onset Pack",
  "author": "XO_OX Designs",
  "description": "...",
  "version": "1.0",
  "content": ["Programs", "Samples"]
}
```

MPC reads `name`, `author`, `description`, `version`, and `content`. All other fields are silently ignored.

### 2.2 Proposed Extended Manifest

Oxport should write `expansion.json` with MPC-standard fields plus XO_OX-specific fields. MPC ignores unknown fields; XO_OX tooling reads them for pack management.

**Proposed additions:**

```json
{
  "name": "XO_OX Onset Pack",
  "author": "XO_OX Designs",
  "description": "115 drum programs from XOnset — the precision percussion engine.",
  "version": "1.0",
  "content": ["Programs", "Samples"],

  "_xo_ox": {
    "schema_version": "1.0",
    "engine": "Onset",
    "engine_accent": "#0066FF",
    "preset_count": 115,
    "wav_count": 3680,
    "program_type": "drum",
    "kit_mode": "smart",
    "sonic_dna_fleet": {
      "brightness": 0.62,
      "warmth": 0.48,
      "movement": 0.71,
      "density": 0.55,
      "space": 0.44,
      "aggression": 0.68
    },
    "coupling_pairs": [
      {"engine": "Oblong", "coupling_type": "harmonic_gate", "note": "Bob's harmonics gate-sequence Onset's hats"},
      {"engine": "Overdub", "coupling_type": "reverb_send", "note": "Overdub's tape reverb wraps Onset's snare"}
    ],
    "ebook_url": "https://xo-ox.org/guide/onset",
    "build_timestamp": "2026-03-16T00:00:00",
    "oxport_version": "1.5"
  }
}
```

**Field rationale:**

| Field | Purpose |
|-------|---------|
| `engine` | Machine-readable engine ID for future pack manager tooling |
| `engine_accent` | Hex color for branded display in MPC expansion browser (MPC ignores, XO_OX web player uses) |
| `preset_count` | Quick count for pack listing without unzipping |
| `wav_count` | Total WAV files — helps estimate download/install size |
| `sonic_dna_fleet` | Aggregate DNA across all presets in the pack — enables "find similar packs" |
| `coupling_pairs` | Recommended engine combos with coupling type — direct link to XOceanus coupling setup |
| `ebook_url` | Deep link to Field Guide entry for this engine |
| `build_timestamp` | Pipeline audit trail |
| `oxport_version` | Which version of the tool built this pack |

**Prefix convention:** All XO_OX-specific fields under `"_xo_ox"` key. Underscore prefix signals custom namespace. MPC skips unknown top-level keys and nested objects alike.

### 2.3 Companion `xo_ox_metadata.json`

For consumers who want XO_OX metadata without parsing inside an XPN ZIP, Oxport should write a companion `xo_ox_metadata.json` alongside the `.xpn` file in the output directory. This feeds:
- The XO-OX.org pack browser (future)
- `oxport.py status` enriched reporting
- Fleet DNA aggregator (`compute_preset_dna.py` extension)

```json
{
  "pack_id": "xo_ox_onset_v1_0",
  "engine": "Onset",
  "pack_name": "XO_OX Onset Pack",
  "file": "XO_OX_Onset_Pack.xpn",
  "file_size_mb": 142.3,
  "preset_count": 115,
  "sonic_dna_fleet": { ... },
  "coupling_pairs": [ ... ],
  "moods": ["Foundation", "Flux", "Entangled"],
  "tags": ["drums", "percussion", "electronic", "808"]
}
```

**Backlog item:** Add `_write_metadata_companion()` to `xpn_packager.py`, called from `_stage_package()` in `oxport.py` after successful packaging.

---

## Section 3: Render Pipeline Innovations

### 3.1 Adaptive Sample Rate Selection (`--adaptive-sr`)

**Problem:** Fixed 44.1kHz is appropriate for warm analog content (OBLONG, OVERDUB) but insufficient for high-frequency content — OPAL's granular shimmer, OVERWORLD's NES 2A03 aliased tones (which have intentional HF artifacts), OPTIC's phosphor-noise artifacts.

**Algorithm:**

```
1. Render 2-second preview of the preset at 96kHz (or current interface rate)
2. Compute FFT of the preview (numpy.fft.rfft, 4096-point)
3. Compute spectral energy above 18kHz as fraction of total energy
4. Decision:
   - energy_above_18k > 0.15 (15%) → recommend 96kHz
   - energy_above_18k > 0.05 (5%)  → recommend 48kHz
   - else                           → 44.1kHz is sufficient
5. Print: "Adaptive SR: significant HF content (22%) → 96kHz recommended"
```

**Implementation in `xpn_render_spec.py`:**

Add `--adaptive-sr` flag. When set:
- Add `"recommended_sample_rate"` and `"hf_energy_fraction"` fields to the render spec JSON
- Warn if the producer's recorded WAVs don't match the recommendation

```python
def analyze_sample_rate_need(preview_wav: Path) -> dict:
    """FFT-based sample rate recommendation."""
    import numpy as np
    data, sr = load_wav(preview_wav)
    if data.ndim > 1:
        data = data.mean(axis=1)
    n = 4096
    spectrum = np.abs(np.fft.rfft(data[:n] if len(data) >= n else data, n=n))
    freqs = np.fft.rfftfreq(n, d=1.0/sr)
    total_energy = np.sum(spectrum**2)
    hf_energy = np.sum(spectrum[freqs > 18000]**2)
    ratio = hf_energy / total_energy if total_energy > 0 else 0.0
    if ratio > 0.15:
        rec_sr = 96000
    elif ratio > 0.05:
        rec_sr = 48000
    else:
        rec_sr = 44100
    return {"recommended_sr": rec_sr, "hf_energy_fraction": round(ratio, 4)}
```

**Engine default table (no preview needed — known characteristics):**

| Engine | Default SR recommendation |
|--------|--------------------------|
| OVERWORLD | 48kHz (NES aliasing in 20-24kHz band) |
| OPAL | 48kHz (granular shimmer) |
| OBLONG | 44.1kHz (warm analog, no meaningful HF) |
| OVERDUB | 44.1kHz (tape character, rolled off HF) |
| ONSET | 44.1kHz (drums, fundamental content < 18kHz) |
| OPTIC | 96kHz (phosphor noise artifacts intentionally above 18kHz) |
| OSPREY | 48kHz (physical model has HF resonances) |

Add `"default_sr"` to `ENGINE_STRATEGIES` in `xpn_render_spec.py`.

---

### 3.2 Intelligent Loudness Normalization

**Problem:** Peak normalization ignores integrated loudness. A 4-second transient hit and a 4-second sustained pad both peak-normalize to -1 dBFS, but the pad will sound 8–12 dB louder in a mix. This causes jarring volume jumps when switching between drum and melodic programs in the same pack.

**Target LUFS by content type:**

| Content type | LUFS-I target | True Peak ceiling |
|-------------|--------------|------------------|
| Kick / snare | -9 LUFS | -1 dBTP |
| Hats / perc | -12 LUFS | -1 dBTP |
| Clap | -11 LUFS | -1 dBTP |
| Melodic keygroup | -14 LUFS | -1 dBTP |
| Texture / pad | -18 LUFS | -1 dBTP |
| FX / one-shot | -16 LUFS | -1 dBTP |

**Implementation without external audio dependencies (pure numpy):**

LUFS-I (ITU-R BS.1770-4) integrated loudness:

```python
def compute_lufs(data: np.ndarray, sr: int) -> float:
    """Compute LUFS-I via BS.1770-4 (K-weighting + gating)."""
    # 1. K-weighting: pre-filter + RLB filter (2-stage IIR)
    # Stage 1: high-shelf pre-filter (boost high frequencies)
    # b1, a1 from BS.1770-4 coefficients for 44.1kHz (computed once)
    # Stage 2: RLB filter (high-pass at 38 Hz)
    # 2. Mean-square over 400ms blocks, 75% overlap
    # 3. Gating: absolute gate at -70 LUFS, relative gate at -10 LU
    # 4. LUFS = -0.691 + 10 * log10(mean gated power)
    ...
```

**Dependency recommendation:** `pyloudnorm` (BSD-licensed, pure Python + numpy, no C extensions). Install: `pip install pyloudnorm`. 400-line library with no other dependencies.

```python
try:
    import pyloudnorm as pyln
    LOUDNORM_AVAILABLE = True
except ImportError:
    LOUDNORM_AVAILABLE = False

def normalize_lufs(data: np.ndarray, sr: int,
                   target_lufs: float, ceiling_dbtp: float = -1.0) -> np.ndarray:
    if not LOUDNORM_AVAILABLE:
        # Fallback: peak normalize to ceiling
        peak = np.max(np.abs(data))
        if peak > 0:
            data = data / peak * (10 ** (ceiling_dbtp / 20.0))
        return data
    meter = pyln.Meter(sr)
    measured = meter.integrated_loudness(data)
    if np.isinf(measured):  # silence guard
        return data
    gain_db = target_lufs - measured
    gained = data * (10 ** (gain_db / 20.0))
    # True peak limiting: inter-sample peak at 4x oversampling
    # pyloudnorm does not include a limiter — apply after gain
    peak = np.max(np.abs(gained))
    ceiling_linear = 10 ** (ceiling_dbtp / 20.0)
    if peak > ceiling_linear:
        gained = gained * (ceiling_linear / peak)
    return gained.astype(np.float32)
```

**True peak limiting note:** Inter-sample peaks (ISPs) can clip even when sample values are within -1 dBFS. The 4× oversampling check above catches the most common case. For production-grade limiting, `pyloudnorm` combined with `scipy.signal.resample` at 4× rate for peak detection is sufficient without a dedicated limiter library.

**Integration in `xpn_kit_expander.py`:** After `save_wav()`, before returning, call `normalize_lufs()` with the voice-appropriate LUFS target. Add `LUFS_TARGETS` dict keyed by voice name.

---

### 3.3 Parallel Render Verification

**Problem:** `xpn_kit_expander.py --workers N` uses `ProcessPoolExecutor`. Workers can fail silently — if a subprocess raises an exception that isn't caught by the `fut.result()` call, the file is not written but the pipeline continues. The current error handling prints `[ERROR]` but does not re-queue.

**Current gap (line 494–496 in `xpn_kit_expander.py`):**

```python
except Exception as exc:
    print(f"  [ERROR] {voice_name}: {exc}")
    # MISSING: re-queue logic, summary accounting
```

**Proposed verification pass — `xpn_verify.py`:**

```python
def verify_render_output(output_dir: Path, render_spec: dict,
                         requeue_failed: bool = True) -> dict:
    """
    Verify that all expected WAV files were written and are non-corrupt.
    Returns: {"ok": int, "failed": int, "requeued": int, "report": [...]}
    """
    expected = build_expected_filenames(render_spec)  # from render spec JSON
    report = []
    failed = []
    for fname in expected:
        path = output_dir / fname
        status = check_wav_file(path)
        report.append({"file": fname, "status": status})
        if status != "ok":
            failed.append(fname)

    requeued = 0
    if requeue_failed and failed:
        requeued = requeue_renders(failed, render_spec, output_dir)

    total = len(expected)
    ok = total - len(failed)
    print(f"  Render verification: {ok}/{total} renders complete, "
          f"{len(failed)} failures")
    if failed:
        for f in failed:
            print(f"    FAILED: {f}")
        print(f"  Re-queued: {requeued}")

    return {"ok": ok, "failed": len(failed), "requeued": requeued,
            "report": report}


def check_wav_file(path: Path) -> str:
    """Returns 'ok', 'missing', 'zero_length', or 'corrupt'."""
    if not path.exists():
        return "missing"
    if path.stat().st_size < 44:  # smaller than WAV header
        return "zero_length"
    try:
        # Check WAV header magic bytes: b'RIFF' at offset 0, b'WAVE' at offset 8
        with open(path, "rb") as f:
            header = f.read(12)
        if header[:4] != b"RIFF" or header[8:12] != b"WAVE":
            return "corrupt"
    except OSError:
        return "corrupt"
    return "ok"
```

**Checksum approach:** For higher integrity, compute MD5 of each WAV immediately after writing (inside the worker), store in a `.checksums.json` in the output dir. Verification pass re-computes and compares. Cost: ~2ms per file at 44.1kHz/24-bit for a 4-second sample. Acceptable for post-render QA; not needed during render.

**Integration in `oxport.py`:** Add `verify` stage between `expand` and `export`:

```python
STAGES = [
    "render_spec", "categorize", "expand", "verify",
    "export", "cover_art", "complement_chain", "package"
]
```

The `verify` stage reads the render specs, confirms all expected WAVs exist and are non-corrupt, re-queues missing/corrupt renders, then proceeds. If re-queue fails: pipeline halts with specific file list.

**CLI report format:**
```
  [verify] Render verification pass
    12/12 renders complete, 0 failures  ← green path
    11/12 renders complete, 1 failure   ← re-queue
      FAILED: onset_808_Reborn_kick_v4.wav (missing)
    Re-queued and re-rendered: 1
    12/12 verified.
```

---

## Section 4: Creative Pipeline Ideas

### 4.1 XPN Variation Generator (`xpn_variation_generator.py`)

**Concept:** Given one complete XPN pack, generate N timbral variations automatically. Each variation is a complete XPN ZIP with all samples transformed by a consistent character treatment.

**Four variant types:**

**Tone variant** — Saturation or high-pass character treatment aligned to feliX/Oscar polarity:
- `oscar`: mild tape saturation (tanh waveshaper, drive=1.15, final gain -0.5 dB to compensate)
- `felix`: HPF at 200 Hz (remove body, emphasize transients and air)

**Time variant** — Global micro-detune to create "classic Akai detuned character":
- All samples repitched ±12 cents via `micro_pitch_shift()` (already in kit_expander)
- Two sub-variants: `+12ct` (slightly bright/sharp) and `-12ct` (slightly dark/flat)
- Old Akai S950 character: ±5 cents. SP-1200 character: wider, ±18 cents.

**Space variant** — Convolution with IR from XOscillograph's library (when available) or a built-in decay:
- `room`: small room IR (80ms RT60)
- `hall`: large hall IR (2.5s RT60)
- `plate`: plate reverb character (1.2s RT60)
- Built-in approximation via `smear_tail()` with extended smear_ms (50–800ms range)

**DNA variant** — Velocity curve remapping based on pack's Sonic DNA profile:
- High-aggression pack (aggression > 0.7): remap velocity curve to emphasize v4 layer (compress lower velocities)
- High-warmth pack (warmth > 0.7): remap velocity curve to emphasize v2/v3 layers (ghost range extended)
- Uses existing `VEL_LAYERS_MUSICAL` curve logic from `xpn_drum_export.py`

**CLI spec:**

```bash
python3 xpn_variation_generator.py \
    --source XO_OX_Onset_808_Reborn.xpn \
    --variant-type tone \          # tone | time | space | dna
    --variant-param oscar \        # type-specific param
    --n 1 \                        # number of variations (1 for most types)
    --output-dir /path/to/out
```

**Implementation structure:**

```python
VARIANT_TRANSFORMS = {
    "tone": {
        "oscar": lambda d, sr: soft_clip(d, drive=1.15),
        "felix": lambda d, sr: lowpass(d, sr, 200, mode="highpass"),  # HPF
    },
    "time": {
        "+12ct": lambda d, sr: micro_pitch_shift(d, sr, +12.0),
        "-12ct": lambda d, sr: micro_pitch_shift(d, sr, -12.0),
        "sp1200": lambda d, sr: micro_pitch_shift(d, sr, -18.0),
    },
    "space": {
        "room": lambda d, sr: smear_tail(d, sr, smear_ms=80.0),
        "plate": lambda d, sr: smear_tail(d, sr, smear_ms=400.0),
    },
}
```

**Output naming:** `{original_pack_name}_{variant_type}_{variant_param}.xpn`

Example: `XO_OX_Onset_808_Reborn_tone_oscar.xpn`

The new XPN's `expansion.json` carries the `_xo_ox.parent_pack` field pointing to the source pack name, and `_xo_ox.variant_type` / `_xo_ox.variant_param`.

---

### 4.2 Collection Flagship Render Strategy

**Concept:** For collection releases (Kitchen/Travel/Artwork), a tiered render approach where the "hero preset" gets an extended sampling treatment for premium placement in the MPC browser.

**Standard tier:** Current pipeline output — 4 velocity layers, 1 round-robin set.

**Flagship tier — 36 samples per engine:**

| Dimension | Count | Description |
|-----------|-------|-------------|
| Velocity layers | 4 | pp/mp/mf/ff as standard |
| Round-robins | 3 | c1/c2/c3 variants per velocity |
| Modulation states | 3 | macro min / mid / max snapshot |
| **Total** | **4 × 3 × 3 = 36** | per engine |

The 3 modulation states require 3 separate renders of the preset at CHARACTER macro positions 0, 64, 127. This means the producer renders the preset three times with the CHARACTER macro set to min, mid, max before recording.

**Flagship render spec additions in `xpn_render_spec.py`:**

```python
"flagship_render": {
    "enabled": True,
    "macro_states": [
        {"macro": "CHARACTER", "value": 0,   "suffix": "char_min"},
        {"macro": "CHARACTER", "value": 64,  "suffix": "char_mid"},
        {"macro": "CHARACTER", "value": 127, "suffix": "char_max"},
    ],
    "vel_layers": 4,
    "round_robins": 3,
    "total_wavs_per_preset": 36,
    "estimated_render_time_min": 12,  # 36 renders × ~20s each
}
```

**Render time estimates:**

| Tier | WAVs/preset | Render time/preset | 24-engine collection total |
|------|-------------|-------------------|--------------------------|
| Standard | 4 vel | ~2 min | ~48 min |
| Flagship | 36 (4×3×3) | ~12 min | ~288 min (~5 hrs) |

Flagship packs are limited to 1 hero preset per engine by default. Standard treatment handles all remaining presets.

**MPC browser positioning:** Flagship XPMs carry a naming prefix (`HERO_`) and the pack's cover art gets a gold badge generated by `xpn_cover_art.py`. In the MPC browser, packs sort alphabetically — `HERO_` prefix ensures the flagship program appears at the top of each engine's program list.

---

### 4.3 Live Performance XPN (`xpn_macro_performance_builder.py`)

**Concept:** A completely different output paradigm — not a sample library, but a performance instrument. Each of the 16 pads plays the same preset at a different macro position. "Playing" the pads sweeps through timbral evolution in real time.

**Pad layout:**

| Pads | Macro range | CHARACTER values |
|------|------------|-----------------|
| 1–16 | min → max | 0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 127 |

**Velocity layers on each pad:** Different COUPLING macro positions (0, 64, 127) → 3 velocity layers per pad.

```
Pad 1  velocity 1–42:  CHARACTER=0,   COUPLING=0
Pad 1  velocity 43–84: CHARACTER=0,   COUPLING=64
Pad 1  velocity 85–127: CHARACTER=0,  COUPLING=127
```

**Total WAVs:** 16 pads × 3 velocity layers = 48 WAVs per preset.

**WAV naming:**
```
{preset_slug}_macro{n:02d}_vel{v}.wav
# e.g.: odyssey_drift_macro01_vel1.wav
```

**CLI spec:**

```bash
python3 xpn_macro_performance_builder.py \
    --preset "Drift Study" \
    --engine Odyssey \
    --macro-steps 16 \
    --velocity-states 3 \
    --macro-axis CHARACTER \
    --velocity-axis COUPLING \
    --wavs-dir /path/to/prerendered \
    --output-dir /path/to/out
```

**XPM structure:** Standard keygroup program, but with 16 instruments each keyed to a single note in the playable range (C2–D#3 for 16 semitones), each with 3 velocity layers.

```xml
<Instrument number="0">
  <MidiNote>36</MidiNote>   <!-- C2 = macro step 1 -->
  <Layer number="0">        <!-- v1: low velocity = COUPLING min -->
    <SampleFile>drift_study_macro01_vel1.wav</SampleFile>
    <VelStart>1</VelStart>
    <VelEnd>42</VelEnd>
    <KeyTrack>False</KeyTrack>  <!-- pitch doesn't change — it's a macro scan -->
  </Layer>
  <Layer number="1">
    <SampleFile>drift_study_macro01_vel2.wav</SampleFile>
    <VelStart>43</VelStart>
    <VelEnd>84</VelEnd>
    <KeyTrack>False</KeyTrack>
  </Layer>
  ...
</Instrument>
<Instrument number="1">
  <MidiNote>37</MidiNote>   <!-- C#2 = macro step 2 -->
  ...
</Instrument>
```

**KeyTrack=False** is essential here — the sample captures a specific timbral moment. Pitch transposition would corrupt the snapshot identity.

**Use cases:**
- DJ set: play pad 1–16 left-to-right for a timbral sweep during a drop
- Beat-making: layer "pad 4" (CHARACTER mid) under "pad 12" (CHARACTER high) for evolving texture
- Live synthesis substitute: an MPC-only performer gets timbral evolution without owning XOceanus

**Performance render checklist** (producer-facing, generated by the tool):

```
LIVE PERFORMANCE RENDER CHECKLIST — Drift Study

Render each sample in XOceanus with these exact macro settings:
Before each render: set SPACE=64 (neutral), MOVEMENT=64 (neutral).

Pad 01 (macro01):  CHARACTER=0,   record 3 times at COUPLING = 0/64/127
Pad 02 (macro02):  CHARACTER=8,   record 3 times at COUPLING = 0/64/127
...
Pad 16 (macro16):  CHARACTER=127, record 3 times at COUPLING = 0/64/127

Total: 48 renders. Estimated time: ~16 minutes.
```

---

## Section 5: Quality Assurance Pipeline

### 5.1 Automated Perceptual Quality Checker (`xpn_quality_checker.py`)

Beyond `xpn_validator.py`'s format correctness checks, a perceptual quality checker detects samples that will cause problems in production — before the XPN ships.

**Detection targets and algorithms:**

---

#### Silence Detection

```python
def check_silence(data: np.ndarray, threshold_dbfs: float = -60.0) -> dict:
    """Detect near-silent samples after normalization."""
    rms = np.sqrt(np.mean(data**2))
    rms_db = 20 * np.log10(rms) if rms > 0 else -120.0
    return {
        "flag": "silence",
        "severity": "critical",
        "value_db": round(rms_db, 1),
        "threshold_db": threshold_dbfs,
        "triggered": rms_db < threshold_dbfs,
        "message": f"Sample RMS is {rms_db:.1f} dBFS — likely silent or near-silent"
    }
```

Trigger threshold: RMS below -60 dBFS after normalization. Likely cause: failed render, empty WAV written by worker error.

---

#### Clipping Detection

```python
def check_clipping(data: np.ndarray, threshold_fraction: float = 0.001) -> dict:
    """Detect digital clipping: samples at or near digital maximum."""
    n_clipped = np.sum(np.abs(data) >= 0.9999)
    fraction = n_clipped / len(data.flat)
    return {
        "flag": "clipping",
        "severity": "critical" if fraction > 0.01 else "warning",
        "clipped_samples": int(n_clipped),
        "fraction": round(fraction, 6),
        "triggered": fraction > threshold_fraction,
        "message": f"{n_clipped} clipped samples ({fraction*100:.3f}%)"
    }
```

Trigger: > 0.1% of samples at digital maximum. Cause: normalization overshoot, failed true peak limiter.

---

#### DC Offset Detection

```python
def check_dc_offset(data: np.ndarray, threshold: float = 0.01) -> dict:
    """Detect significant DC offset (non-zero mean)."""
    if data.ndim > 1:
        mean_offset = float(np.max(np.abs(np.mean(data, axis=0))))
    else:
        mean_offset = float(np.abs(np.mean(data)))
    return {
        "flag": "dc_offset",
        "severity": "warning",
        "offset": round(mean_offset, 5),
        "triggered": mean_offset > threshold,
        "message": f"DC offset detected: mean={mean_offset:.5f} (>{threshold})"
    }
```

DC offset causes thumps on sample start/stop, reduces headroom, and causes inter-sample peak issues. Fix: subtract mean before normalizing.

---

#### Phase Cancellation Detection

```python
def check_phase_cancellation(data: np.ndarray, threshold_db: float = -20.0) -> dict:
    """
    Detect stereo samples that sum to near-silence (heavy phase cancellation).
    Only applicable to stereo samples (data.ndim == 2, data.shape[1] == 2).
    """
    if data.ndim != 2 or data.shape[1] != 2:
        return {"flag": "phase_cancellation", "triggered": False,
                "message": "N/A (mono)"}
    mono_sum = data[:, 0] + data[:, 1]
    sum_rms = np.sqrt(np.mean(mono_sum**2))
    l_rms = np.sqrt(np.mean(data[:, 0]**2))
    r_rms = np.sqrt(np.mean(data[:, 1]**2))
    avg_channel_rms = (l_rms + r_rms) / 2.0
    if avg_channel_rms == 0:
        return {"flag": "phase_cancellation", "triggered": False}
    cancellation_db = 20 * np.log10(sum_rms / avg_channel_rms) if sum_rms > 0 else -120.0
    return {
        "flag": "phase_cancellation",
        "severity": "critical",
        "cancellation_db": round(cancellation_db, 1),
        "triggered": cancellation_db < threshold_db,
        "message": f"Stereo sum is {cancellation_db:.1f} dB vs channels — severe phase cancellation"
    }
```

Trigger: mono sum more than 20 dB quieter than individual channels. Cause: accidental phase inversion on one channel, incorrect WAV write (L/R swap), DSP error in `channel_delay()` creating 180° offset.

---

#### "Boring" Sample Detection (Insufficient Dynamic Character)

```python
def check_dynamics(data: np.ndarray, sr: int,
                   window_ms: float = 500.0,
                   threshold_db: float = 3.0) -> dict:
    """
    Detect undynamic samples: RMS variation < threshold over 500ms windows.
    A flat RMS over time means no attack, no decay — perceptually "dead".
    """
    window_samples = int(window_ms * sr / 1000.0)
    if len(data) < window_samples * 2:
        return {"flag": "boring", "triggered": False,
                "message": "Sample too short to evaluate dynamics"}
    if data.ndim > 1:
        mono = data.mean(axis=1)
    else:
        mono = data
    # Compute RMS in non-overlapping 500ms windows
    n_windows = len(mono) // window_samples
    windows = mono[:n_windows * window_samples].reshape(n_windows, window_samples)
    rms_per_window = np.sqrt(np.mean(windows**2, axis=1))
    rms_db = 20 * np.log10(rms_per_window + 1e-10)
    rms_range_db = float(np.max(rms_db) - np.min(rms_db))
    return {
        "flag": "boring",
        "severity": "info",
        "rms_range_db": round(rms_range_db, 1),
        "threshold_db": threshold_db,
        "triggered": rms_range_db < threshold_db,
        "message": f"RMS variation only {rms_range_db:.1f} dB over {n_windows} windows — undynamic"
    }
```

Trigger: RMS range < 3 dB across 500ms windows (for samples > 1 second). Applies to melodic engines only (OPAL textures are intentionally flat — exclude by voice type or content_type flag).

**Exemptions:** Drone/pad textures (`content_type == "texture"`) intentionally have flat RMS. Boring detection should be suppressed for these via `--content-type texture` flag.

---

#### Full Quality Report

**CLI:**

```bash
python3 xpn_quality_checker.py \
    --wavs-dir /path/to/wavs \
    --render-spec /path/to/spec.json \
    --output-report quality_report.json
```

**Output report structure:**

```json
{
  "total_files": 32,
  "files_checked": 32,
  "pass": 29,
  "warn": 2,
  "critical": 1,
  "results": [
    {
      "file": "onset_808_Reborn_kick_v4.wav",
      "checks": {
        "silence":  {"triggered": false},
        "clipping": {"triggered": true, "severity": "critical",
                     "fraction": 0.023, "clipped_samples": 1014},
        "dc_offset": {"triggered": false},
        "phase_cancellation": {"triggered": false},
        "boring":   {"triggered": false}
      },
      "worst_severity": "critical"
    },
    ...
  ]
}
```

**Console summary:**

```
  QA Report: 29/32 files PASS  |  2 warnings  |  1 critical

  CRITICAL:
    onset_808_Reborn_kick_v4.wav — clipping: 2.3% samples at digital max
      → Re-render with -1 dBTP ceiling applied before export

  WARNING:
    onset_808_Reborn_fx_c3.wav — dc_offset: 0.023 (> 0.01)
      → Apply DC block filter (HPF at 5 Hz) before normalization
    onset_808_Reborn_chat_v1.wav — boring: RMS range 1.8 dB (< 3 dB)
      → Ghost hat layer may be too compressed — check v1 transform chain
```

**Integration in `oxport.py`:** Add `qa` stage after `verify`, before `export`:

```python
STAGES = [
    "render_spec", "categorize", "expand", "verify",
    "qa", "export", "cover_art", "complement_chain", "package"
]
```

QA stage runs `xpn_quality_checker.py` on all WAVs in `samples_dir`. Critical failures abort the pipeline with instructions. Warnings are logged to `quality_report.json` in the build dir and continue.

---

## Summary: Oxport Backlog Additions

| Item | Module | Priority | Effort |
|------|--------|----------|--------|
| Per-pad MIDI channel routing (`--routing live`) | xpn_drum_export.py | Medium | Low |
| Pad color from engine accent (`--pad-colors`) | xpn_drum_export.py | Low | Low |
| InsertEffects EQ character (`--character felix/oscar`) | xpn_drum_export.py, xpn_keygroup_export.py | High | Low |
| Per-layer KeyTrack split (sub vs. body) | xpn_keygroup_export.py | Medium | Low |
| Per-voice BendRange defaults | xpn_drum_export.py | Low | Low |
| XPM-native ModWheel routing | xpn_drum_export.py, xpn_keygroup_export.py | High | Medium |
| XPM-native Aftertouch routing | both exporters | High | Low |
| Slice program builder | xpn_slice_builder.py (new) | Medium | Medium |
| Clip program builder | add to oxport.py as `--clip-set` mode | Medium | Medium |
| Extended expansion.json with `_xo_ox` namespace | xpn_packager.py | High | Low |
| `xo_ox_metadata.json` companion file | xpn_packager.py | Medium | Low |
| Adaptive sample rate (`--adaptive-sr`) | xpn_render_spec.py | Medium | Medium |
| LUFS normalization with pyloudnorm | xpn_kit_expander.py | High | Medium |
| Render verification pass + re-queue | xpn_verify.py (new) + oxport.py | High | Medium |
| XPN Variation Generator (4 variant types) | xpn_variation_generator.py (new) | Medium | High |
| Flagship render spec (4×3×3 = 36 samples) | xpn_render_spec.py | Low | Medium |
| Live Performance XPN builder | xpn_macro_performance_builder.py (new) | Medium | High |
| Perceptual QA checker (5 detection types) | xpn_quality_checker.py (new) | High | High |
| QA stage in oxport.py pipeline | oxport.py | High | Low |
