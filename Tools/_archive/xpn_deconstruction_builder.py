#!/usr/bin/env python3
"""
XPN Deconstruction Pack Builder — XO_OX Designs

Creates a 4-bank "signal chain" educational XPN pack where each pad bank
adds one processing step to the same sounds, revealing how a producer
builds a sample from isolation to fully produced variant.

Bank Architecture:
  Bank A — Isolated elements  (raw, unprocessed WAVs)
  Bank B — Lightly processed  (normalize -3dBFS + HP filter + compression sim)
  Bank C — Heavily processed  (saturation + amplitude modulation for thickness)
  Bank D — Variations         (user-provided alternate takes, or auto-reversed Bank A)

Processing Pipeline (pure stdlib — struct for WAV I/O):
  A → B:
    1. Normalize to -3dBFS (peak normalization)
    2. High-pass filter at 40Hz (first-order IIR, matched-Z)
    3. Soft-knee limiting via tanh waveshaping on peaks above threshold

  B → C:
    1. 2nd harmonic saturation: y = x + 0.1*x² (soft asymmetric)
    2. Amplitude modulation: multiply by slow sinusoidal envelope for "thickness"

  Bank D: copy --variants DIR, or reverse Bank A samples

Output structure:
  {name}/
    Programs/
      {name}_BankA.xpm   (raw)
      {name}_BankB.xpm   (light)
      {name}_BankC.xpm   (heavy)
      {name}_BankD.xpm   (variations)
    Samples/
      BankA/*.wav
      BankB/*.wav
      BankC/*.wav
      BankD/*.wav
    Tutorial/
      tutorial.md
    expansion.xml

  → ZIP packaged as {name}.xpn

CLI:
  python3 xpn_deconstruction_builder.py \\
      --input ./stems/ \\
      --output ./out/ \\
      --name "ONSET_Deconstruction" \\
      [--variants ./variants/]

XPN Golden Rules (never break):
  KeyTrack  = True
  RootNote  = 0
  VelStart  = 0 on empty layers
"""

import argparse
import math
import shutil
import struct
import sys
import zipfile
from datetime import date
from pathlib import Path
from typing import List, Optional, Tuple
from xml.sax.saxutils import escape as xml_escape

# ---------------------------------------------------------------------------
# WAV I/O (pure stdlib, struct-based)
# ---------------------------------------------------------------------------

def _read_wav(path: Path) -> Tuple[int, int, int, List[List[float]]]:
    """
    Read a WAV file and return (sample_rate, bit_depth, num_channels, channels).
    channels is a list of lists of float samples in range [-1.0, 1.0].
    Supports 8, 16, 24, 32-bit PCM and 32-bit float.
    """
    with open(path, "rb") as f:
        data = f.read()

    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError(f"Not a valid WAV file: {path}")

    pos = 12
    fmt_data = None
    pcm_data = None

    while pos < len(data) - 8:
        chunk_id = data[pos:pos+4]
        chunk_size = struct.unpack_from("<I", data, pos+4)[0]
        chunk_body = data[pos+8:pos+8+chunk_size]
        if chunk_id == b"fmt ":
            fmt_data = chunk_body
        elif chunk_id == b"data":
            pcm_data = chunk_body
        pos += 8 + chunk_size
        if chunk_size % 2 == 1:
            pos += 1  # pad byte

    if fmt_data is None or pcm_data is None:
        raise ValueError(f"Malformed WAV (missing fmt or data chunk): {path}")

    audio_format = struct.unpack_from("<H", fmt_data, 0)[0]
    num_channels  = struct.unpack_from("<H", fmt_data, 2)[0]
    sample_rate   = struct.unpack_from("<I", fmt_data, 4)[0]
    bit_depth     = struct.unpack_from("<H", fmt_data, 14)[0]

    # Decode samples to float
    num_frames = len(pcm_data) // (num_channels * (bit_depth // 8))
    channels = [[] for _ in range(num_channels)]

    if audio_format == 3 and bit_depth == 32:
        # IEEE float
        for i in range(num_frames):
            for ch in range(num_channels):
                offset = (i * num_channels + ch) * 4
                sample = struct.unpack_from("<f", pcm_data, offset)[0]
                channels[ch].append(max(-1.0, min(1.0, sample)))
    elif audio_format == 1:
        bytes_per_sample = bit_depth // 8
        if bit_depth == 8:
            for i in range(num_frames):
                for ch in range(num_channels):
                    offset = (i * num_channels + ch)
                    raw = pcm_data[offset]
                    channels[ch].append((raw - 128) / 128.0)
        elif bit_depth == 16:
            for i in range(num_frames):
                for ch in range(num_channels):
                    offset = (i * num_channels + ch) * 2
                    raw = struct.unpack_from("<h", pcm_data, offset)[0]
                    channels[ch].append(raw / 32768.0)
        elif bit_depth == 24:
            for i in range(num_frames):
                for ch in range(num_channels):
                    offset = (i * num_channels + ch) * 3
                    b0, b1, b2 = pcm_data[offset], pcm_data[offset+1], pcm_data[offset+2]
                    raw = (b2 << 16) | (b1 << 8) | b0
                    if raw >= 0x800000:
                        raw -= 0x1000000
                    channels[ch].append(raw / 8388608.0)
        elif bit_depth == 32:
            for i in range(num_frames):
                for ch in range(num_channels):
                    offset = (i * num_channels + ch) * 4
                    raw = struct.unpack_from("<i", pcm_data, offset)[0]
                    channels[ch].append(raw / 2147483648.0)
        else:
            raise ValueError(f"Unsupported bit depth {bit_depth} in {path}")
    else:
        raise ValueError(f"Unsupported WAV format {audio_format} in {path}")

    return sample_rate, bit_depth, num_channels, channels


def _write_wav(path: Path, sample_rate: int, channels: List[List[float]],
               bit_depth: int = 16) -> None:
    """Write a WAV file from float samples in range [-1.0, 1.0]."""
    num_channels = len(channels)
    num_frames   = len(channels[0]) if channels else 0

    bytes_per_sample = bit_depth // 8
    block_align      = num_channels * bytes_per_sample
    byte_rate        = sample_rate * block_align
    data_size        = num_frames * block_align

    header = struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF",
        36 + data_size,
        b"WAVE",
        b"fmt ",
        16,            # fmt chunk size
        1,             # PCM
        num_channels,
        sample_rate,
        byte_rate,
        block_align,
        bit_depth,
        b"data",
        data_size,
    )

    with open(path, "wb") as f:
        f.write(header)
        if bit_depth == 16:
            for i in range(num_frames):
                for ch in range(num_channels):
                    s = max(-1.0, min(1.0, channels[ch][i]))
                    raw = int(s * 32767)
                    f.write(struct.pack("<h", raw))
        elif bit_depth == 24:
            for i in range(num_frames):
                for ch in range(num_channels):
                    s = max(-1.0, min(1.0, channels[ch][i]))
                    raw = int(s * 8388607)
                    if raw < 0:
                        raw += 0x1000000
                    f.write(bytes([raw & 0xFF, (raw >> 8) & 0xFF, (raw >> 16) & 0xFF]))
        elif bit_depth == 32:
            for i in range(num_frames):
                for ch in range(num_channels):
                    s = max(-1.0, min(1.0, channels[ch][i]))
                    f.write(struct.pack("<i", int(s * 2147483647)))
        else:
            raise ValueError(f"Unsupported output bit depth {bit_depth}")


# ---------------------------------------------------------------------------
# DSP Processing
# ---------------------------------------------------------------------------

def _peak_normalize(channels: List[List[float]], target_db: float = -3.0) -> List[List[float]]:
    """Normalize to target dBFS. Returns new channel lists."""
    target_linear = 10 ** (target_db / 20.0)
    peak = max(abs(s) for ch in channels for s in ch) if channels and channels[0] else 0.0
    if peak < 1e-10:
        return [list(ch) for ch in channels]
    gain = target_linear / peak
    return [[s * gain for s in ch] for ch in channels]


def _highpass_filter_40hz(channels: List[List[float]], sample_rate: int) -> List[List[float]]:
    """
    First-order IIR high-pass filter at 40Hz.
    Uses matched-Z transform: coeff = exp(-2*pi*fc/sr)
    H(z) = (1 - a) * (1 - z^-1) / (1 - a*z^-1)  -- first-order HP
    """
    fc = 40.0
    a = math.exp(-2.0 * math.pi * fc / sample_rate)
    b0 = (1.0 + a) / 2.0    # = (1-a) scaled, standard 1st-order HP
    b1 = -b0
    a1 = -a

    out_channels = []
    for ch in channels:
        filtered = []
        x_prev = 0.0
        y_prev = 0.0
        for x in ch:
            y = b0 * x + b1 * x_prev - a1 * y_prev
            filtered.append(y)
            x_prev = x
            y_prev = y
        out_channels.append(filtered)
    return out_channels


def _soft_knee_limit(channels: List[List[float]], threshold: float = 0.7) -> List[List[float]]:
    """
    Soft-knee limiter via tanh waveshaping for peaks above threshold.
    Below threshold: linear passthrough.
    Above: tanh-shaped to prevent hard clipping.
    """
    inv_thresh = 1.0 / threshold if threshold > 0 else 1.0
    out_channels = []
    for ch in channels:
        limited = []
        for s in ch:
            if abs(s) <= threshold:
                limited.append(s)
            else:
                sign = 1.0 if s > 0 else -1.0
                excess = (abs(s) - threshold) * inv_thresh
                # tanh squashes excess, then re-add threshold portion
                limited.append(sign * (threshold + (1.0 - threshold) * math.tanh(excess)))
        out_channels.append(limited)
    return out_channels


def process_bank_b(channels: List[List[float]], sample_rate: int) -> List[List[float]]:
    """
    Bank A → Bank B processing:
      1. Normalize to -3dBFS
      2. High-pass filter at 40Hz (mud removal)
      3. Soft-knee limiting (compression simulation)
    """
    ch = _peak_normalize(channels, target_db=-3.0)
    ch = _highpass_filter_40hz(ch, sample_rate)
    ch = _soft_knee_limit(ch, threshold=0.7)
    return ch


def _2nd_harmonic_saturation(channels: List[List[float]],
                              drive: float = 0.1) -> List[List[float]]:
    """
    2nd harmonic saturation: y = x + drive * x²
    Asymmetric — adds even-order warmth (tape/tube character).
    Output is soft-clipped to [-1, 1].
    """
    out_channels = []
    for ch in channels:
        sat = [max(-1.0, min(1.0, s + drive * s * s)) for s in ch]
        out_channels.append(sat)
    return out_channels


def _amplitude_modulation(channels: List[List[float]], sample_rate: int,
                           rate_hz: float = 0.3, depth: float = 0.12) -> List[List[float]]:
    """
    Slow sinusoidal AM for perceived "thickness."
    gain(t) = 1.0 - depth * (1 - cos(2*pi*rate*t)) / 2
    Depth 0.12 = ±6% amplitude flutter — subtle shimmer, not tremolo.
    """
    out_channels = []
    for ch in channels:
        modulated = []
        for i, s in enumerate(ch):
            t = i / sample_rate
            gain = 1.0 - depth * (1.0 - math.cos(2.0 * math.pi * rate_hz * t)) / 2.0
            modulated.append(s * gain)
        out_channels.append(modulated)
    return out_channels


def process_bank_c(channels: List[List[float]], sample_rate: int) -> List[List[float]]:
    """
    Bank B → Bank C processing:
      1. 2nd harmonic saturation (warmth/edge)
      2. Amplitude modulation (thickness/shimmer)
    """
    ch = _2nd_harmonic_saturation(channels, drive=0.1)
    ch = _amplitude_modulation(ch, sample_rate, rate_hz=0.3, depth=0.12)
    return ch


def _reverse_audio(channels: List[List[float]]) -> List[List[float]]:
    """Reverse all channels in time."""
    return [list(reversed(ch)) for ch in channels]


# ---------------------------------------------------------------------------
# XPM Generation (Drum program — 4 banks × N pads)
# ---------------------------------------------------------------------------

MPC_HEADER = (
    '<?xml version="1.0" encoding="UTF-8"?>\n\n'
    '<MPCVObject>\n'
    '  <Version>\n'
    '    <File_Version>1.7</File_Version>\n'
    '    <Application>MPC-V</Application>\n'
    '    <Application_Version>2.10.0.0</Application_Version>\n'
    '    <Platform>OSX</Platform>\n'
    '  </Version>\n'
)


def _layer_xml(wav_relative: Optional[str], vel_start: int, vel_end: int) -> str:
    """Generate a single <Layer> XML block."""
    if wav_relative is None:
        return (
            '          <Layer>\n'
            '            <SampleFile></SampleFile>\n'
            '            <Active>False</Active>\n'
            '            <Volume>1.000000</Volume>\n'
            '            <Pan>0.500000</Pan>\n'
            '            <Pitch>0.000000</Pitch>\n'
            '            <TuneCoarse>0</TuneCoarse>\n'
            '            <TuneFine>0</TuneFine>\n'
            '            <VelStart>0</VelStart>\n'
            '            <VelEnd>127</VelEnd>\n'
            '            <KeyTrack>True</KeyTrack>\n'
            '            <RootNote>0</RootNote>\n'
            '          </Layer>\n'
        )
    return (
        '          <Layer>\n'
        f'            <SampleFile>{xml_escape(wav_relative)}</SampleFile>\n'
        '            <Active>True</Active>\n'
        '            <Volume>1.000000</Volume>\n'
        '            <Pan>0.500000</Pan>\n'
        '            <Pitch>0.000000</Pitch>\n'
        '            <TuneCoarse>0</TuneCoarse>\n'
        '            <TuneFine>0</TuneFine>\n'
        f'            <VelStart>{vel_start}</VelStart>\n'
        f'            <VelEnd>{vel_end}</VelEnd>\n'
        '            <KeyTrack>True</KeyTrack>\n'
        '            <RootNote>0</RootNote>\n'
        '          </Layer>\n'
    )


def _instrument_xml(instrument_num: int, wav_relative: Optional[str]) -> str:
    """Generate a single <Instrument> block for a Drum program pad."""
    is_active = wav_relative is not None
    layer = _layer_xml(wav_relative, vel_start=1, vel_end=127) if is_active else _layer_xml(None, 0, 127)

    return (
        f'      <Instrument number="{instrument_num}">\n'
        f'        <AudioRoute>\n'
        f'          <AudioRoute>0</AudioRoute>\n'
        f'          <AudioRouteSubIndex>0</AudioRouteSubIndex>\n'
        f'          <InsertsEnabled>False</InsertsEnabled>\n'
        f'        </AudioRoute>\n'
        f'        <Send1>0.000000</Send1>\n'
        f'        <Send2>0.000000</Send2>\n'
        f'        <Send3>0.000000</Send3>\n'
        f'        <Send4>0.000000</Send4>\n'
        f'        <Volume>0.707946</Volume>\n'
        f'        <Mute>False</Mute>\n'
        f'        <Pan>0.500000</Pan>\n'
        f'        <TuneCoarse>0</TuneCoarse>\n'
        f'        <TuneFine>0</TuneFine>\n'
        f'        <Mono>False</Mono>\n'
        f'        <Polyphony>4</Polyphony>\n'
        f'        <FilterKeytrack>0.000000</FilterKeytrack>\n'
        f'        <LowNote>0</LowNote>\n'
        f'        <HighNote>127</HighNote>\n'
        f'        <IgnoreBaseNote>False</IgnoreBaseNote>\n'
        f'        <ZonePlay>0</ZonePlay>\n'
        f'        <MuteGroup>0</MuteGroup>\n'
        f'        <MuteTarget1>0</MuteTarget1>\n'
        f'        <MuteTarget2>0</MuteTarget2>\n'
        f'        <MuteTarget3>0</MuteTarget3>\n'
        f'        <MuteTarget4>0</MuteTarget4>\n'
        f'        <SimultTarget1>0</SimultTarget1>\n'
        f'        <SimultTarget2>0</SimultTarget2>\n'
        f'        <SimultTarget3>0</SimultTarget3>\n'
        f'        <SimultTarget4>0</SimultTarget4>\n'
        f'        <LfoPitch>0.000000</LfoPitch>\n'
        f'        <LfoCutoff>0.000000</LfoCutoff>\n'
        f'        <LfoVolume>0.000000</LfoVolume>\n'
        f'        <LfoPan>0.000000</LfoPan>\n'
        f'        <OneShot>True</OneShot>\n'
        f'        <FilterType>2</FilterType>\n'
        f'        <Cutoff>1.000000</Cutoff>\n'
        f'        <Resonance>0.000000</Resonance>\n'
        f'        <FilterEnvAmt>0.000000</FilterEnvAmt>\n'
        f'        <AfterTouchToFilter>0.000000</AfterTouchToFilter>\n'
        f'        <VelocityToStart>0.000000</VelocityToStart>\n'
        f'        <VelocityToFilter>0.000000</VelocityToFilter>\n'
        f'        <VelocityToFilterAttack>0.000000</VelocityToFilterAttack>\n'
        f'        <VelocityToFilterEnvelope>0.000000</VelocityToFilterEnvelope>\n'
        f'        <FilterAttack>0.000000</FilterAttack>\n'
        f'        <FilterDecay>0.000000</FilterDecay>\n'
        f'        <FilterSustain>1.000000</FilterSustain>\n'
        f'        <FilterRelease>0.000000</FilterRelease>\n'
        f'        <FilterHold>0.000000</FilterHold>\n'
        f'        <FilterDecayType>True</FilterDecayType>\n'
        f'        <FilterADEnvelope>True</FilterADEnvelope>\n'
        f'        <VolumeHold>0.000000</VolumeHold>\n'
        f'        <VolumeDecayType>True</VolumeDecayType>\n'
        f'        <VolumeADEnvelope>True</VolumeADEnvelope>\n'
        f'        <VolumeAttack>0.000000</VolumeAttack>\n'
        f'        <VolumeDecay>0.000000</VolumeDecay>\n'
        f'        <VolumeSustain>1.000000</VolumeSustain>\n'
        f'        <VolumeRelease>0.100000</VolumeRelease>\n'
        f'        <VelocityToPitch>0.000000</VelocityToPitch>\n'
        f'        <VelocityToVolumeAttack>0.000000</VelocityToVolumeAttack>\n'
        f'        <VelocitySensitivity>0.500000</VelocitySensitivity>\n'
        f'        <VelocityToPan>0.000000</VelocityToPan>\n'
        f'        <LFO>\n'
        f'          <Speed>0.000000</Speed>\n'
        f'          <Amount>0.000000</Amount>\n'
        f'          <Type>0</Type>\n'
        f'          <Sync>False</Sync>\n'
        f'          <Retrigger>True</Retrigger>\n'
        f'        </LFO>\n'
        f'        <Layers>\n'
        f'{layer}'
        f'        </Layers>\n'
        f'      </Instrument>'
    )


def _pad_note_map_xml(num_stems: int) -> str:
    """Generate PadNoteMap: pads 1-N → MIDI notes 36..36+N-1, rest zero."""
    lines = []
    for pad in range(1, 17):
        note = 35 + pad if pad <= num_stems else 0
        lines.append(f'      <Pad number="{pad}">{note}</Pad>')
    return "\n".join(lines)


def _pad_group_map_xml() -> str:
    """All pads in group 0 (no mute groups needed for educational pack)."""
    return "\n".join(
        f'      <Pad number="{pad}">0</Pad>'
        for pad in range(1, 17)
    )


def generate_bank_xpm(bank_name: str, prog_name: str,
                      stem_names: List[str],
                      bank_sample_dir_relative: str) -> str:
    """
    Generate a complete Drum XPM for one bank.
    stem_names: list of WAV filenames (without directory prefix).
    bank_sample_dir_relative: relative path from Programs/ to the bank Samples/.
    """
    num_stems = min(len(stem_names), 16)
    pad_note_xml  = _pad_note_map_xml(num_stems)
    pad_group_xml = _pad_group_map_xml()

    instruments_xml_parts = []
    for i in range(128):
        pad_index = i  # instrument 0 = MIDI 35+1=36, etc.
        if i < num_stems:
            wav_rel = f"{bank_sample_dir_relative}/{stem_names[i]}"
        else:
            wav_rel = None
        instruments_xml_parts.append(_instrument_xml(i, wav_rel))

    instruments_xml = "\n".join(instruments_xml_parts)
    prog_name_escaped = xml_escape(prog_name)

    return (
        MPC_HEADER
        + f'  <Program type="Drum">\n'
        + f'    <Name>{prog_name_escaped}</Name>\n'
        + '    <PadNoteMap>\n'
        + pad_note_xml + '\n'
        + '    </PadNoteMap>\n'
        + '    <PadGroupMap>\n'
        + pad_group_xml + '\n'
        + '    </PadGroupMap>\n'
        + '    <Instruments>\n'
        + instruments_xml + '\n'
        + '    </Instruments>\n'
        + '  </Program>\n'
        + '</MPCVObject>\n'
    )


# ---------------------------------------------------------------------------
# Expansion XML
# ---------------------------------------------------------------------------

def generate_expansion_xml(pack_name: str, pack_id: str,
                            description: str, version: str = "1.0.0") -> str:
    today = str(date.today())
    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n\n'
        f'<expansion version="2.0.0.0" buildVersion="2.10.0.0">\n'
        f'  <local/>\n'
        f'  <identifier>{xml_escape(pack_id)}</identifier>\n'
        f'  <title>{xml_escape(pack_name)}</title>\n'
        f'  <manufacturer>XO_OX Designs</manufacturer>\n'
        f'  <version>{version}</version>\n'
        f'  <type>drum</type>\n'
        f'  <priority>50</priority>\n'
        f'  <img>artwork.png</img>\n'
        f'  <description>{xml_escape(description)}</description>\n'
        f'  <date>{today}</date>\n'
        f'  <separator>-</separator>\n'
        f'</expansion>\n'
    )


# ---------------------------------------------------------------------------
# Tutorial Markdown
# ---------------------------------------------------------------------------

TUTORIAL_TEMPLATE = """\
# {name} — Deconstruction Pack

*Generated {today} by XO_OX Designs*

This pack reveals the producer's signal chain one step at a time.
Each bank contains the same {n_stems} sounds at a different stage of processing.

---

## Bank A — Isolated Elements (Raw)

**What you hear:** The pure, unprocessed stem.

No EQ, no compression, no saturation.
This is the sound as captured or synthesised before any shaping.

**Use it when:** You want maximum headroom for your own processing chain,
or you want to study the source material in complete isolation.

---

## Bank B — Lightly Processed

**Processing applied:**

1. **Normalize to −3 dBFS**
   Peak normalization ensures consistent gain staging across all stems.
   −3 dBFS leaves headroom for downstream processing without clipping.

2. **High-Pass Filter @ 40 Hz** (first-order IIR, matched-Z transform)
   Removes sub-sonic rumble and DC offset bias. Cleans low-end build-up
   in dense arrangements without altering the perceived tone.
   Coefficient: `a = exp(-2π × 40 / sample_rate)`

3. **Soft-Knee Limiter** (tanh waveshaping, threshold 0.7)
   Simulates gentle compression. Peaks above −3 dB are tanh-folded rather
   than hard-clipped, preserving transient character while controlling
   dynamic range. Think: VCA bus glue without a compressor plugin.

**Use it when:** You want clean, consistent sounds ready for arrangement,
without heavy character processing yet applied.

---

## Bank C — Heavily Processed

**Processing applied (starting from Bank B):**

1. **2nd Harmonic Saturation** (`y = x + 0.1 × x²`)
   Asymmetric polynomial distortion introduces the 2nd harmonic —
   the "warmth" harmonic. This is how tape and tube circuits colour sound.
   The asymmetry creates even-order harmonics rather than odd-order buzz.

2. **Amplitude Modulation @ 0.3 Hz, depth 12%**
   A slow sinusoidal envelope multiplied onto the signal.
   `gain(t) = 1.0 − 0.06 × (1 − cos(2π × 0.3 × t))`
   Too slow to hear as tremolo, this creates subtle long-period variation —
   the kind of "breathing" that makes static samples feel alive in a loop.

**Use it when:** You want colour and movement baked in,
or you're studying how saturation + AM transform a clean source.

---

## Bank D — Variations

**What you hear:** Alternate takes or reversed versions.

If a `--variants` directory was supplied during build, Bank D contains
those alternate WAVs (producer's discarded takes, pitch variations, etc.).

If no variants were provided, Bank D contains **reversed** versions of
the Bank A stems — a classic production technique for creating tension,
swell-ins, and textural contrast.

**Use it when:** You want to break out of the "standard" version of a sound,
or you're exploring what the producer considered and rejected.

---

## Pads Layout

Each bank follows identical pad assignments:

| Pad | Stem |
|-----|------|
{pad_table}

---

## Processing Chain Summary

```
Bank A  ──►  Bank B  ──►  Bank C
  (raw)   normalize    saturate (2nd harmonic)
           HP @ 40Hz   AM @ 0.3Hz
           tanh limit
```

---

*XO_OX Designs — XOlokun Platform*
"""


def _build_pad_table(stem_names: List[str]) -> str:
    rows = []
    for i, name in enumerate(stem_names[:16]):
        stem_label = Path(name).stem
        rows.append(f"| {i+1:2d}  | {stem_label} |")
    return "\n".join(rows)


# ---------------------------------------------------------------------------
# Main build logic
# ---------------------------------------------------------------------------

def _collect_stems(input_dir: Path) -> List[Path]:
    """Collect up to 16 WAV files from input directory, sorted."""
    stems = sorted(input_dir.glob("*.wav")) + sorted(input_dir.glob("*.WAV"))
    # Deduplicate (case-insensitive FS might return both)
    seen = set()
    unique = []
    for p in stems:
        key = p.name.lower()
        if key not in seen:
            seen.add(key)
            unique.append(p)
    if len(unique) > 16:
        print(f"  [warn] Found {len(unique)} stems — using first 16 only.", file=sys.stderr)
        unique = unique[:16]
    return unique


def _ensure_dir(path: Path) -> Path:
    path.mkdir(parents=True, exist_ok=True)
    return path


def build_deconstruction_pack(
    input_dir: Path,
    output_dir: Path,
    name: str,
    variants_dir: Optional[Path] = None,
    dry_run: bool = False,
) -> Path:
    """
    Build the full deconstruction pack.
    Returns path to the output .xpn ZIP.
    """
    stems = _collect_stems(input_dir)
    if not stems:
        print(f"[error] No WAV files found in {input_dir}", file=sys.stderr)
        sys.exit(1)

    print(f"  Found {len(stems)} stem(s) in {input_dir}")
    for s in stems:
        print(f"    {s.name}")

    slug = name.replace(" ", "_")
    pack_dir = output_dir / slug
    _ensure_dir(pack_dir)

    samples_root = pack_dir / "Samples"
    programs_dir = pack_dir / "Programs"
    tutorial_dir = pack_dir / "Tutorial"
    for d in [samples_root, programs_dir, tutorial_dir]:
        _ensure_dir(d)

    bank_dirs = {
        "BankA": _ensure_dir(samples_root / "BankA"),
        "BankB": _ensure_dir(samples_root / "BankB"),
        "BankC": _ensure_dir(samples_root / "BankC"),
        "BankD": _ensure_dir(samples_root / "BankD"),
    }

    # Collect variant WAVs if supplied
    variant_wavs: List[Path] = []
    if variants_dir and variants_dir.exists():
        variant_wavs = sorted(variants_dir.glob("*.wav")) + sorted(variants_dir.glob("*.WAV"))
        variant_wavs = variant_wavs[:len(stems)]
        print(f"  Using {len(variant_wavs)} variant(s) from {variants_dir}")

    # Process each stem
    bank_a_names: List[str] = []
    bank_b_names: List[str] = []
    bank_c_names: List[str] = []
    bank_d_names: List[str] = []

    for i, stem_path in enumerate(stems):
        stem_out_name = stem_path.name.lower()
        if not stem_out_name.endswith(".wav"):
            stem_out_name = Path(stem_out_name).stem + ".wav"

        # Bank A: copy raw
        bank_a_dest = bank_dirs["BankA"] / stem_out_name
        if not dry_run:
            shutil.copy2(stem_path, bank_a_dest)
        bank_a_names.append(stem_out_name)

        # Read audio for processing
        try:
            sample_rate, bit_depth, num_channels, channels = _read_wav(stem_path)
        except Exception as exc:
            print(f"  [warn] Could not read {stem_path.name}: {exc} — skipping DSP, copying raw",
                  file=sys.stderr)
            if not dry_run:
                shutil.copy2(stem_path, bank_dirs["BankB"] / stem_out_name)
                shutil.copy2(stem_path, bank_dirs["BankC"] / stem_out_name)
            bank_b_names.append(stem_out_name)
            bank_c_names.append(stem_out_name)
            continue

        # Bank B: normalize + HP + limit
        b_channels = process_bank_b(channels, sample_rate)
        bank_b_dest = bank_dirs["BankB"] / stem_out_name
        if not dry_run:
            _write_wav(bank_b_dest, sample_rate, b_channels, bit_depth=16)
        bank_b_names.append(stem_out_name)
        print(f"    [{i+1:2d}] BankB processed: {stem_out_name}")

        # Bank C: saturation + AM (starting from Bank B audio)
        c_channels = process_bank_c(b_channels, sample_rate)
        bank_c_dest = bank_dirs["BankC"] / stem_out_name
        if not dry_run:
            _write_wav(bank_c_dest, sample_rate, c_channels, bit_depth=16)
        bank_c_names.append(stem_out_name)
        print(f"    [{i+1:2d}] BankC processed: {stem_out_name}")

        # Bank D: variant or reversed raw
        bank_d_dest = bank_dirs["BankD"] / stem_out_name
        if i < len(variant_wavs):
            if not dry_run:
                shutil.copy2(variant_wavs[i], bank_d_dest)
            print(f"    [{i+1:2d}] BankD variant:   {variant_wavs[i].name}")
        else:
            rev_channels = _reverse_audio(channels)
            if not dry_run:
                _write_wav(bank_d_dest, sample_rate, rev_channels, bit_depth=16)
            print(f"    [{i+1:2d}] BankD reversed:  {stem_out_name}")
        bank_d_names.append(stem_out_name)

    # Write XPM files for each bank
    banks = [
        ("BankA", bank_a_names, f"{name} — Bank A (Raw)"),
        ("BankB", bank_b_names, f"{name} — Bank B (Light)"),
        ("BankC", bank_c_names, f"{name} — Bank C (Heavy)"),
        ("BankD", bank_d_names, f"{name} — Bank D (Variations)"),
    ]

    for bank_id, wav_names, prog_name in banks:
        # Relative path from Programs/ dir to Samples/BankX/
        sample_rel = f"../Samples/{bank_id}"
        xpm_xml = generate_bank_xpm(bank_id, prog_name, wav_names, sample_rel)
        xpm_path = programs_dir / f"{slug}_{bank_id}.xpm"
        if not dry_run:
            xpm_path.write_text(xpm_xml, encoding="utf-8")
        print(f"  Wrote XPM: {xpm_path.name}")

    # Write expansion.xml
    pack_id = f"com.xo-ox.deconstruction.{slug.lower()}"
    desc = (
        f"Deconstruction pack for '{name}'. "
        "4 banks revealing the producer's signal chain: "
        "raw stems → normalized+filtered → saturated+modulated → variations."
    )
    exp_xml = generate_expansion_xml(name, pack_id, desc)
    exp_path = pack_dir / "expansion.xml"
    if not dry_run:
        exp_path.write_text(exp_xml, encoding="utf-8")
    print(f"  Wrote expansion.xml")

    # Write tutorial
    pad_table = _build_pad_table(bank_a_names)
    tutorial_text = TUTORIAL_TEMPLATE.format(
        name=name,
        today=str(date.today()),
        n_stems=len(stems),
        pad_table=pad_table,
    )
    tutorial_path = tutorial_dir / "tutorial.md"
    if not dry_run:
        tutorial_path.write_text(tutorial_text, encoding="utf-8")
    print(f"  Wrote Tutorial/tutorial.md")

    # Write deconstruction_notes.txt (processing detail log)
    notes_path = pack_dir / "deconstruction_notes.txt"
    notes_text = _build_notes(name, stems, bank_a_names)
    if not dry_run:
        notes_path.write_text(notes_text, encoding="utf-8")
    print(f"  Wrote deconstruction_notes.txt")

    # Package as .xpn ZIP
    xpn_path = output_dir / f"{slug}.xpn"
    if not dry_run:
        with zipfile.ZipFile(xpn_path, "w", zipfile.ZIP_DEFLATED) as zf:
            for file_path in sorted(pack_dir.rglob("*")):
                if file_path.is_file():
                    arcname = file_path.relative_to(pack_dir)
                    zf.write(file_path, arcname)
        print(f"\n  Packaged: {xpn_path}")
    else:
        print(f"\n  [dry-run] Would package: {xpn_path}")

    return xpn_path


def _build_notes(name: str, stems: List[Path], bank_a_names: List[str]) -> str:
    lines = [
        f"Deconstruction Pack — {name}",
        f"Generated: {date.today()}",
        f"Stems: {len(stems)}",
        "",
        "=" * 60,
        "BANK A — Raw Stems",
        "=" * 60,
        "No processing applied. Direct copies of input WAVs.",
        "",
    ]
    for n in bank_a_names:
        lines.append(f"  {n}")
    lines += [
        "",
        "=" * 60,
        "BANK B — Lightly Processed",
        "=" * 60,
        "Processing chain:",
        "  1. Peak normalize to -3.0 dBFS",
        "     Formula: gain = 10^(-3/20) / peak_abs",
        "",
        "  2. High-pass filter at 40 Hz (first-order IIR)",
        "     Transform: matched-Z (exp-based, not Euler)",
        "     Coefficient: a = exp(-2*pi*40/sample_rate)",
        "     Transfer: H(z) = b0*(1 - z^-1) / (1 + a1*z^-1)",
        "     Purpose: removes sub-sonic mud, cleans DC bias",
        "",
        "  3. Soft-knee limiting (tanh waveshaping)",
        "     Threshold: 0.7 linear (-3.1 dBFS)",
        "     Formula for |x| > threshold:",
        "       sign(x) * (T + (1-T)*tanh((|x|-T)/T))",
        "     Purpose: controls peaks without hard clipping transients",
        "",
        "=" * 60,
        "BANK C — Heavily Processed",
        "=" * 60,
        "Built on Bank B output. Additional processing:",
        "",
        "  1. 2nd Harmonic Saturation",
        "     Formula: y = clip(x + 0.1*x², -1, 1)",
        "     Effect: adds even-order harmonic warmth (tape/tube character)",
        "     The x² term is asymmetric — produces 2nd harmonic, not buzz",
        "",
        "  2. Amplitude Modulation",
        "     Rate: 0.3 Hz  (sub-tremolo — too slow to hear as tremolo)",
        "     Depth: 12% amplitude",
        "     Formula: gain(t) = 1.0 - 0.06*(1 - cos(2*pi*0.3*t))",
        "     Effect: 'breathing' — makes static samples feel alive in loops",
        "",
        "=" * 60,
        "BANK D — Variations",
        "=" * 60,
    ]
    if bank_a_names:
        lines += [
            "Source: auto-generated reversed versions of Bank A stems.",
            "(Pass --variants DIR to use your own alternate takes instead.)",
            "",
            "Reversal: channels reversed in time, no other processing.",
            "Classic technique: reverse swell-ins, tension-building textures.",
        ]
    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Build a 4-bank Deconstruction XPN pack (signal chain educational).",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 xpn_deconstruction_builder.py \\
      --input ./stems/ \\
      --output ./out/ \\
      --name "ONSET_Deconstruction"

  python3 xpn_deconstruction_builder.py \\
      --input ./stems/ \\
      --output ./out/ \\
      --name "Drum Lab 01" \\
      --variants ./alt_takes/

  python3 xpn_deconstruction_builder.py \\
      --input ./stems/ \\
      --output ./out/ \\
      --name "ONSET_Deconstruction" \\
      --dry-run
        """,
    )
    parser.add_argument("--input",    required=True,  help="Directory of isolated WAV stems (Bank A source)")
    parser.add_argument("--output",   required=True,  help="Output directory for the pack")
    parser.add_argument("--name",     required=True,  help="Pack name, e.g. 'ONSET_Deconstruction'")
    parser.add_argument("--variants", default=None,   help="Optional directory of variant WAVs for Bank D")
    parser.add_argument("--dry-run",  action="store_true",
                        help="Print what would be done without writing files")

    args = parser.parse_args()

    input_dir  = Path(args.input).expanduser().resolve()
    output_dir = Path(args.output).expanduser().resolve()
    variants_dir = Path(args.variants).expanduser().resolve() if args.variants else None

    if not input_dir.exists() or not input_dir.is_dir():
        print(f"[error] --input directory not found: {input_dir}", file=sys.stderr)
        sys.exit(1)

    if variants_dir and (not variants_dir.exists() or not variants_dir.is_dir()):
        print(f"[error] --variants directory not found: {variants_dir}", file=sys.stderr)
        sys.exit(1)

    if not args.dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    print(f"\nXPN Deconstruction Builder — XO_OX Designs")
    print(f"  Pack name : {args.name}")
    print(f"  Input     : {input_dir}")
    print(f"  Output    : {output_dir}")
    if variants_dir:
        print(f"  Variants  : {variants_dir}")
    print(f"  Dry run   : {args.dry_run}")
    print()

    build_deconstruction_pack(
        input_dir=input_dir,
        output_dir=output_dir,
        name=args.name,
        variants_dir=variants_dir,
        dry_run=args.dry_run,
    )

    print("\nDone.")


if __name__ == "__main__":
    main()
