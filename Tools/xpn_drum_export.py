#!/usr/bin/env python3
"""
XPN Drum Program Exporter — XO_OX Designs
Generates MPC-compatible drum expansion packs (.xpn) from XOnset presets.

For each XOnset preset, produces:
  - A <Program type="Drum"> XPM file (128 instruments, 8 active pads)

Pad layout (GM-convention MIDI notes):
  V1 Kick         → Note 36  (C2)
  V2 Snare        → Note 38  (D2)
  V3 Closed Hat   → Note 42  (F#2)   MuteGroup 1
  V4 Open Hat     → Note 46  (A#2)   MuteGroup 1 (muted by closed hat)
  V5 Clap         → Note 39  (D#2)
  V6 Tom          → Note 41  (F2)
  V7 Percussion   → Note 43  (G2)
  V8 FX/Cymbal    → Note 49  (C#3)

Kit Modes (--mode):
  velocity        4 layers by velocity (pp/mp/mf/ff). Default. Best for dynamics.
                  WAVs: {slug}_{voice}_v1.wav … v4.wav

  cycle           4 round-robin layers. Cycling through samples on successive
                  hits prevents the "machine gun" effect. Best for hats and snares.
                  WAVs: {slug}_{voice}_c1.wav … c4.wav

  random          4 layers picked randomly per hit. Best for organic fx/claps.
                  WAVs: {slug}_{voice}_c1.wav … c4.wav  (same naming as cycle)

  random-norepeat Random pick, never same sample twice in a row.
                  WAVs: {slug}_{voice}_c1.wav … c4.wav

  smart           Per-voice automatic mode assignment (recommended):
                    kick  → velocity  (dynamics matter)
                    snare → velocity  (dynamics matter)
                    chat  → cycle     (machine-gun prevention)
                    ohat  → cycle     (machine-gun prevention)
                    clap  → random    (organic feel)
                    tom   → velocity
                    perc  → cycle
                    fx    → random-norepeat

Per-Voice Smart Defaults (applied in all modes):
  kick:  VelocityToPitch=0.05 (slight pitch follow — classic bounce)
  snare: VelocityToFilter=0.30 (filter opens on hard hits)
  ohat:  OneShot=False, Polyphony=2 (rings until muted by closed hat)
  clap:  Polyphony=2 (stacked hits sound natural)
  fx:    OneShot=False, Polyphony=4 (sustained, overlapping)

WAV file naming:
  Velocity mode:  {preset_slug}_{voice}_v{1-4}.wav
  Cycle/Random:   {preset_slug}_{voice}_c{1-4}.wav
  Smart mode:     use the mode for that voice (see above)

Usage:
    # Export with velocity layers (default)
    python3 xpn_drum_export.py --preset "808 Reborn" \\
        --wavs-dir /path/to/wavs --output-dir /path/to/out

    # Export cycle kit (machine-gun prevention on every pad)
    python3 xpn_drum_export.py --preset "808 Reborn" --mode cycle \\
        --wavs-dir /path/to/wavs --output-dir /path/to/out

    # Export smart kit (per-voice optimal mode)
    python3 xpn_drum_export.py --preset "808 Reborn" --mode smart \\
        --wavs-dir /path/to/wavs --output-dir /path/to/out

    # Print WAV checklist for a preset + mode
    python3 xpn_drum_export.py --checklist "808 Reborn" --mode smart

    # Export all XOnset presets
    python3 xpn_drum_export.py --all-onset --mode smart \\
        --wavs-dir /path/to/wavs --output-dir /path/to/out
"""

import argparse
import json
import shutil
import sys
from datetime import date
from pathlib import Path

try:
    from xpn_cover_art import generate_cover
    COVER_ART_AVAILABLE = True
except ImportError:
    COVER_ART_AVAILABLE = False

REPO_ROOT   = Path(__file__).parent.parent
PRESETS_DIR = REPO_ROOT / "Presets" / "XOmnibus"


# =============================================================================
# KIT MODES
# =============================================================================

# ZonePlay values — per MPC XPM spec
ZONE_PLAY = {
    "velocity":        1,
    "cycle":           2,
    "random":          3,
    "random-norepeat": 4,
}

# Velocity layer definitions (mode=velocity)
VEL_LAYERS = [
    (0,   31,  0.35),   # ghost   (pp)
    (32,  63,  0.55),   # soft    (mp)
    (64,  95,  0.75),   # medium  (mf)
    (96, 127,  0.95),   # hard    (ff)
]
VEL_SUFFIXES = ["v1", "v2", "v3", "v4"]

# Cycle/random variant suffixes — all layers span full velocity range
CYCLE_SUFFIXES = ["c1", "c2", "c3", "c4"]

# Smart mode: best ZonePlay per voice
SMART_MODE = {
    "kick":  "velocity",
    "snare": "velocity",
    "chat":  "cycle",
    "ohat":  "cycle",
    "clap":  "random",
    "tom":   "velocity",
    "perc":  "cycle",
    "fx":    "random-norepeat",
}


# =============================================================================
# PAD / VOICE DEFINITIONS
# =============================================================================

# (midi_note, voice_name, mute_group, mute_targets)
PAD_MAP = [
    (36, "kick",  0, [0, 0, 0, 0]),   # V1
    (38, "snare", 0, [0, 0, 0, 0]),   # V2
    (42, "chat",  1, [46, 0, 0, 0]),  # V3 Closed hat → mutes open hat (46)
    (46, "ohat",  1, [0, 0, 0, 0]),   # V4 Open hat
    (39, "clap",  0, [0, 0, 0, 0]),   # V5
    (41, "tom",   0, [0, 0, 0, 0]),   # V6
    (43, "perc",  0, [0, 0, 0, 0]),   # V7
    (49, "fx",    0, [0, 0, 0, 0]),   # V8
]

# Per-voice physical behavior overrides — these match the acoustic reality
# of each drum type and apply regardless of kit mode.
VOICE_DEFAULTS = {
    "kick": {
        "mono":               True,
        "polyphony":          1,
        "one_shot":           True,
        "velocity_to_pitch":  0.05,   # slight pitch follow on hard hits
        "velocity_to_filter": 0.0,
        "velocity_sensitivity": 1.0,
    },
    "snare": {
        "mono":               True,
        "polyphony":          1,
        "one_shot":           True,
        "velocity_to_pitch":  0.0,
        "velocity_to_filter": 0.30,   # filter opens on hard snare hits
        "velocity_sensitivity": 1.0,
    },
    "chat": {
        "mono":               True,
        "polyphony":          1,
        "one_shot":           True,
        "velocity_to_pitch":  0.0,
        "velocity_to_filter": 0.0,
        "velocity_sensitivity": 0.85,
    },
    "ohat": {
        "mono":               False,
        "polyphony":          2,      # overlapping rings sound natural
        "one_shot":           False,  # rings until muted by closed hat
        "velocity_to_pitch":  0.0,
        "velocity_to_filter": 0.0,
        "velocity_sensitivity": 0.80,
    },
    "clap": {
        "mono":               False,
        "polyphony":          2,      # stacked for that layer-clap texture
        "one_shot":           True,
        "velocity_to_pitch":  0.0,
        "velocity_to_filter": 0.15,
        "velocity_sensitivity": 0.90,
    },
    "tom": {
        "mono":               True,
        "polyphony":          2,
        "one_shot":           True,
        "velocity_to_pitch":  0.03,
        "velocity_to_filter": 0.10,
        "velocity_sensitivity": 1.0,
    },
    "perc": {
        "mono":               False,
        "polyphony":          2,
        "one_shot":           True,
        "velocity_to_pitch":  0.0,
        "velocity_to_filter": 0.0,
        "velocity_sensitivity": 0.90,
    },
    "fx": {
        "mono":               False,
        "polyphony":          4,      # sustained + layerable
        "one_shot":           False,  # let it decay naturally
        "velocity_to_pitch":  0.0,
        "velocity_to_filter": 0.0,
        "velocity_sensitivity": 0.75,
    },
}

_VOICE_DEFAULTS_FALLBACK = {
    "mono": True, "polyphony": 1, "one_shot": True,
    "velocity_to_pitch": 0.0, "velocity_to_filter": 0.0,
    "velocity_sensitivity": 1.0,
}


def _voice_cfg(voice_name: str) -> dict:
    return VOICE_DEFAULTS.get(voice_name, _VOICE_DEFAULTS_FALLBACK)


def _resolve_mode(kit_mode: str, voice_name: str) -> str:
    """Return the effective ZonePlay mode string for a voice given kit mode."""
    if kit_mode == "smart":
        return SMART_MODE.get(voice_name, "velocity")
    return kit_mode


# =============================================================================
# XPM LAYER BUILDERS
# =============================================================================

def _layers_for_voice(voice_name: str, kit_mode: str,
                      wav_map: dict, preset_slug: str) -> list[tuple]:
    """
    Return a list of (vel_start, vel_end, volume, sample_name, sample_file)
    tuples for each layer of this voice in the given kit mode.
    """
    effective_mode = _resolve_mode(kit_mode, voice_name)

    if effective_mode == "velocity":
        layers = []
        for i, (vel_start, vel_end, vol) in enumerate(VEL_LAYERS):
            key = f"{preset_slug}_{voice_name}_{VEL_SUFFIXES[i]}"
            name = wav_map.get(key, "")
            layers.append((vel_start, vel_end, vol, name, name))
        return layers

    # cycle / random / random-norepeat — all variants span full vel range
    layers = []
    for suffix in CYCLE_SUFFIXES:
        key = f"{preset_slug}_{voice_name}_{suffix}"
        name = wav_map.get(key, "")
        layers.append((0, 127, 0.707946, name, name))
    return layers


def _layer_block(number: int, vel_start: int, vel_end: int,
                 sample_name: str, sample_file: str, volume: float) -> str:
    active = "True"
    return (
        f'          <Layer number="{number}">\n'
        f'            <Active>{active}</Active>\n'
        f'            <Volume>{volume:.6f}</Volume>\n'
        f'            <Pan>0.500000</Pan>\n'
        f'            <Pitch>0.000000</Pitch>\n'
        f'            <TuneCoarse>0</TuneCoarse>\n'
        f'            <TuneFine>0</TuneFine>\n'
        f'            <VelStart>{vel_start}</VelStart>\n'
        f'            <VelEnd>{vel_end}</VelEnd>\n'
        f'            <SampleStart>0</SampleStart>\n'
        f'            <SampleEnd>0</SampleEnd>\n'
        f'            <Loop>False</Loop>\n'
        f'            <LoopStart>0</LoopStart>\n'
        f'            <LoopEnd>0</LoopEnd>\n'
        f'            <LoopTune>0</LoopTune>\n'
        f'            <Mute>False</Mute>\n'
        f'            <RootNote>0</RootNote>\n'
        f'            <KeyTrack>False</KeyTrack>\n'
        f'            <SampleName>{sample_name}</SampleName>\n'
        f'            <SampleFile>{sample_file}</SampleFile>\n'
        f'            <SliceIndex>128</SliceIndex>\n'
        f'            <Direction>0</Direction>\n'
        f'            <Offset>0</Offset>\n'
        f'            <SliceStart>0</SliceStart>\n'
        f'            <SliceEnd>0</SliceEnd>\n'
        f'            <SliceLoopStart>0</SliceLoopStart>\n'
        f'            <SliceLoop>0</SliceLoop>\n'
        f'          </Layer>'
    )


def _empty_layers() -> str:
    """Four silent placeholder layers for inactive instruments (VelStart=0)."""
    blocks = []
    for i in range(1, 5):
        blocks.append(_layer_block(i, 0, 127, "", "", 0.707946))
    return "\n".join(blocks)


# =============================================================================
# INSTRUMENT BLOCK
# =============================================================================

def _instrument_block(instrument_num: int, voice_name: str,
                      mute_group: int, mute_targets: list,
                      wav_map: dict, preset_slug: str,
                      kit_mode: str) -> str:
    """Generate one <Instrument> XML block."""
    is_active = bool(voice_name)
    cfg = _voice_cfg(voice_name) if is_active else _VOICE_DEFAULTS_FALLBACK

    effective_mode = _resolve_mode(kit_mode, voice_name) if is_active else "velocity"
    zone_play = ZONE_PLAY.get(effective_mode, 1)

    if is_active:
        layer_data = _layers_for_voice(voice_name, kit_mode, wav_map, preset_slug)
        layers_xml = "\n".join(
            _layer_block(i + 1, vs, ve, sn, sf, vol)
            for i, (vs, ve, vol, sn, sf) in enumerate(layer_data)
        )
    else:
        layers_xml = _empty_layers()

    mono_str     = "True" if cfg["mono"] else "False"
    oneshot_str  = "True" if cfg["one_shot"] else "False"

    mute_xml = "\n".join(
        f"        <MuteTarget{i+1}>{t}</MuteTarget{i+1}>"
        for i, t in enumerate(mute_targets)
    )
    simult_xml = "\n".join(
        f"        <SimultTarget{i+1}>0</SimultTarget{i+1}>"
        for i in range(4)
    )

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
        f'        <Mono>{mono_str}</Mono>\n'
        f'        <Polyphony>{cfg["polyphony"]}</Polyphony>\n'
        f'        <FilterKeytrack>0.000000</FilterKeytrack>\n'
        f'        <LowNote>0</LowNote>\n'
        f'        <HighNote>127</HighNote>\n'
        f'        <IgnoreBaseNote>False</IgnoreBaseNote>\n'
        f'        <ZonePlay>{zone_play}</ZonePlay>\n'
        f'        <MuteGroup>{mute_group}</MuteGroup>\n'
        f'{mute_xml}\n'
        f'{simult_xml}\n'
        f'        <LfoPitch>0.000000</LfoPitch>\n'
        f'        <LfoCutoff>0.000000</LfoCutoff>\n'
        f'        <LfoVolume>0.000000</LfoVolume>\n'
        f'        <LfoPan>0.000000</LfoPan>\n'
        f'        <OneShot>{oneshot_str}</OneShot>\n'
        f'        <FilterType>2</FilterType>\n'
        f'        <Cutoff>1.000000</Cutoff>\n'
        f'        <Resonance>0.000000</Resonance>\n'
        f'        <FilterEnvAmt>0.000000</FilterEnvAmt>\n'
        f'        <AfterTouchToFilter>0.000000</AfterTouchToFilter>\n'
        f'        <VelocityToStart>0.000000</VelocityToStart>\n'
        f'        <VelocityToFilterAttack>0.000000</VelocityToFilterAttack>\n'
        f'        <VelocityToFilter>{cfg["velocity_to_filter"]:.6f}</VelocityToFilter>\n'
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
        f'        <VolumeRelease>0.000000</VolumeRelease>\n'
        f'        <VelocityToPitch>{cfg["velocity_to_pitch"]:.6f}</VelocityToPitch>\n'
        f'        <VelocityToVolumeAttack>0.000000</VelocityToVolumeAttack>\n'
        f'        <VelocitySensitivity>{cfg["velocity_sensitivity"]:.6f}</VelocitySensitivity>\n'
        f'        <VelocityToPan>0.000000</VelocityToPan>\n'
        f'        <LFO>\n'
        f'          <Speed>0.000000</Speed>\n'
        f'          <Amount>0.000000</Amount>\n'
        f'          <Type>0</Type>\n'
        f'          <Sync>False</Sync>\n'
        f'          <Retrigger>True</Retrigger>\n'
        f'        </LFO>\n'
        f'        <Layers>\n'
        f'{layers_xml}\n'
        f'        </Layers>\n'
        f'      </Instrument>'
    )


# =============================================================================
# XPM GENERATION
# =============================================================================

def generate_xpm(preset_name: str, wav_map: dict,
                 kit_mode: str = "velocity") -> str:
    """Generate complete drum program XPM XML string."""
    preset_slug = preset_name.replace(" ", "_")
    prog_name   = f"XO_OX-{preset_name}"

    note_to_pad = {note: (voice, mg, mt) for note, voice, mg, mt in PAD_MAP}

    parts = []
    for i in range(128):
        if i in note_to_pad:
            voice, mg, mt = note_to_pad[i]
        else:
            voice, mg, mt = "", 0, [0, 0, 0, 0]

        parts.append(_instrument_block(
            instrument_num=i,
            voice_name=voice,
            mute_group=mg,
            mute_targets=mt,
            wav_map=wav_map,
            preset_slug=preset_slug,
            kit_mode=kit_mode,
        ))

    instruments_xml = "\n".join(parts)

    pad_json = json.dumps(
        {"ProgramPads": {"Universal": {"value0": False},
                         "Type":      {"value0": 5},
                         "universalPad": 32512}},
        separators=(",", ":"),
    )

    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n\n'
        '<MPCVObject>\n'
        '  <Version>\n'
        '    <File_Version>1.7</File_Version>\n'
        '    <Application>MPC-V</Application>\n'
        '    <Application_Version>2.10.0.0</Application_Version>\n'
        '    <Platform>OSX</Platform>\n'
        '  </Version>\n'
        '  <Program type="Drum">\n'
        f'    <Name>{prog_name}</Name>\n'
        f'    <ProgramPads>{pad_json}</ProgramPads>\n'
        '    <PadNoteMap></PadNoteMap>\n'
        '    <PadGroupMap></PadGroupMap>\n'
        '    <Instruments>\n'
        f'{instruments_xml}\n'
        '    </Instruments>\n'
        '  </Program>\n'
        '</MPCVObject>\n'
    )


# =============================================================================
# EXPANSION.XML
# =============================================================================

def generate_expansion_xml(pack_name: str, pack_id: str,
                            description: str, version: str = "1.0.0") -> str:
    today = str(date.today())
    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n\n'
        f'<expansion version="2.0.0.0" buildVersion="2.10.0.0">\n'
        f'  <local/>\n'
        f'  <identifier>{pack_id}</identifier>\n'
        f'  <title>{pack_name}</title>\n'
        f'  <manufacturer>XO_OX Designs</manufacturer>\n'
        f'  <version>{version}.0</version>\n'
        f'  <type>drum</type>\n'
        f'  <priority>50</priority>\n'
        f'  <img>artwork.png</img>\n'
        f'  <description>{description}</description>\n'
        f'  <separator>-</separator>\n'
        f'</expansion>\n'
    )


# =============================================================================
# WAV MAP
# =============================================================================

def build_wav_map(wavs_dir: Path, preset_slug: str,
                  kit_mode: str = "velocity") -> dict:
    """
    Build {stem: filename} lookup for all WAVs that belong to this preset.
    Handles both velocity (v1-v4) and cycle/random (c1-c4) naming.
    """
    wav_map = {}
    if not wavs_dir or not wavs_dir.exists():
        return wav_map
    for wav_file in sorted(wavs_dir.glob("*.wav")):
        stem = wav_file.stem
        if stem.startswith(preset_slug):
            wav_map[stem] = wav_file.name
    return wav_map


def _required_wavs(preset_slug: str, kit_mode: str) -> list[str]:
    """Return the list of WAV filenames required for a given preset + mode."""
    files = []
    for _, voice, _, _ in PAD_MAP:
        effective = _resolve_mode(kit_mode, voice)
        if effective == "velocity":
            for suf in VEL_SUFFIXES:
                files.append(f"{preset_slug}_{voice}_{suf}.wav")
        else:
            for suf in CYCLE_SUFFIXES:
                files.append(f"{preset_slug}_{voice}_{suf}.wav")
    return files


def print_wav_checklist(preset_name: str, kit_mode: str = "velocity"):
    slug = preset_name.replace(" ", "_")
    print(f"\nWAV checklist for '{preset_name}' — mode: {kit_mode}\n")
    for note, voice, _, _ in PAD_MAP:
        effective = _resolve_mode(kit_mode, voice)
        cfg = _voice_cfg(voice)
        suffixes = VEL_SUFFIXES if effective == "velocity" else CYCLE_SUFFIXES
        note_desc = "vel layers (pp→ff)" if effective == "velocity" else f"variants ({effective})"
        print(f"  {voice:6s}  [{note_desc}]")
        for suf in suffixes:
            print(f"    {slug}_{voice}_{suf}.wav")
    print()


# =============================================================================
# PACK BUILDER
# =============================================================================

def build_drum_pack(preset_name: str, wavs_dir: Path, output_dir: Path,
                    pack_id: str = None, version: str = "1.0",
                    description: str = "", kit_mode: str = "velocity",
                    dry_run: bool = False, generate_art: bool = True) -> dict:
    """Build a complete drum XPN pack for one XOnset preset."""
    preset_slug = preset_name.replace(" ", "_")
    pack_id   = pack_id or f"com.xo-ox.onset.{preset_slug.lower()}"
    pack_dir  = output_dir / preset_slug
    if not dry_run:
        pack_dir.mkdir(parents=True, exist_ok=True)

    wav_map = build_wav_map(wavs_dir, preset_slug, kit_mode) if wavs_dir else {}
    required = _required_wavs(preset_slug, kit_mode)
    missing  = [f for f in required if f.replace(".wav", "") not in wav_map]

    xpm_content = generate_xpm(preset_name, wav_map, kit_mode)
    xpm_path    = pack_dir / f"{preset_slug}.xpm"
    if not dry_run:
        xpm_path.write_text(xpm_content, encoding="utf-8")
        print(f"  XPM:  {xpm_path.name}  (mode={kit_mode})")

    if not description:
        description = (
            f"XOnset drum kit — {preset_name}. "
            f"Dual-layer synthesis percussion by XO_OX Designs."
        )
    exp_content = generate_expansion_xml(
        pack_name=f"XOnset: {preset_name}",
        pack_id=pack_id,
        description=description,
        version=version,
    )
    if not dry_run:
        (pack_dir / "Expansion.xml").write_text(exp_content, encoding="utf-8")
        print(f"  Manifest: Expansion.xml")

    if generate_art and not dry_run and COVER_ART_AVAILABLE:
        try:
            generate_cover(
                engine="ONSET", pack_name=preset_name,
                output_dir=str(pack_dir), preset_count=1,
                version=version, seed=hash(preset_name) % 10000,
            )
        except Exception as e:
            print(f"  [WARN] Cover art: {e}")

    if wavs_dir and not dry_run:
        copied = 0
        for wav_name in wav_map.values():
            src = wavs_dir / wav_name
            dst = pack_dir / wav_name
            if src.exists() and not dst.exists():
                import shutil
                shutil.copy2(src, dst)
                copied += 1
        if copied:
            print(f"  WAVs: {copied} copied")

    if missing:
        print(f"  [MISSING] {len(missing)} WAVs — run --checklist to see full list")

    return {"pack_dir": str(pack_dir), "missing_wavs": missing}


def build_all_onset_packs(wavs_dir: Path, output_dir: Path,
                          version: str = "1.0", kit_mode: str = "velocity",
                          dry_run: bool = False) -> list:
    onset_presets = []
    for mood_dir in PRESETS_DIR.iterdir():
        if mood_dir.is_dir():
            onset_dir = mood_dir / "Onset"
            if onset_dir.exists():
                for xmeta in onset_dir.glob("*.xometa"):
                    import json as _json
                    with open(xmeta) as f:
                        data = _json.load(f)
                    onset_presets.append({
                        "name":        data["name"],
                        "mood":        data["mood"],
                        "description": data.get("description", ""),
                    })

    print(f"Found {len(onset_presets)} XOnset presets")
    results = []
    for p in onset_presets:
        print(f"\nBuilding: {p['name']} ({p['mood']})")
        results.append(build_drum_pack(
            preset_name=p["name"],
            wavs_dir=wavs_dir,
            output_dir=output_dir,
            version=version,
            description=p["description"],
            kit_mode=kit_mode,
            dry_run=dry_run,
            generate_art=True,
        ))
    return results


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Drum Program Exporter — XO_OX Designs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--preset",     help="Single preset name to export")
    parser.add_argument("--all-onset",  action="store_true",
                        help="Export all XOnset presets")
    parser.add_argument("--mode",       default="velocity",
                        choices=["velocity", "cycle", "random",
                                 "random-norepeat", "smart"],
                        help="Layer selection mode (default: velocity)")
    parser.add_argument("--wavs-dir",   help="Directory containing WAV files")
    parser.add_argument("--output-dir", default=".", help="Output directory")
    parser.add_argument("--version",    default="1.0")
    parser.add_argument("--dry-run",    action="store_true")
    parser.add_argument("--checklist",  metavar="PRESET",
                        help="Print required WAV filenames for a preset + mode")
    parser.add_argument("--no-art",     action="store_true")
    args = parser.parse_args()

    if args.checklist:
        print_wav_checklist(args.checklist, args.mode)
        return 0

    output_dir = Path(args.output_dir)
    wavs_dir   = Path(args.wavs_dir) if args.wavs_dir else None

    if args.dry_run:
        print(f"DRY RUN — mode: {args.mode}\n")

    if args.all_onset:
        build_all_onset_packs(wavs_dir, output_dir, args.version,
                              args.mode, args.dry_run)
    elif args.preset:
        print(f"Building drum pack: {args.preset}  mode={args.mode}")
        build_drum_pack(
            preset_name=args.preset,
            wavs_dir=wavs_dir,
            output_dir=output_dir,
            version=args.version,
            kit_mode=args.mode,
            dry_run=args.dry_run,
            generate_art=not args.no_art,
        )
    else:
        parser.print_help()
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
