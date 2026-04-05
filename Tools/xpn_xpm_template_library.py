#!/usr/bin/env python3
"""
XPN XPM Template Library — XO_OX Designs
Provides pre-built XPM program templates for common use cases.

Templates:
  basic_drum_kit        16-pad drum program, single layer per pad
  velocity_drum_kit     8 drum sounds × 4 velocity layers (Vibe's curve), 32 keygroups
  chromatic_instrument  88-key chromatic keygroup, single sample, KeyTrack=True
  multisampled_instrument 8 root notes × 4 velocity layers = 32 keygroups
  oneshot_sampler       16 pads, single layer, optimized for one-shots
  stem_mixer            8 stems across 8 pads with choke groups

XPM Rules (from CLAUDE.md — never break):
  KeyTrack  = True    (samples transpose across zones)
  RootNote  = 0       (MPC auto-detect convention)
  VelStart  = 0       on empty/unused layers (prevents ghost triggering)

Vibe's canonical 4-layer velocity curve:
  v1 → 1–31   (pp)
  v2 → 32–63  (mp)
  v3 → 64–95  (mf)
  v4 → 96–127 (ff)

Usage:
  python xpn_xpm_template_library.py --list
  python xpn_xpm_template_library.py --template basic_drum_kit --describe
  python xpn_xpm_template_library.py --template basic_drum_kit --output program.xpm
  python xpn_xpm_template_library.py --template velocity_drum_kit \\
      --samples kick.wav snare.wav hihat.wav clap.wav tom.wav rim.wav crash.wav perc.wav \\
      --output vdrum.xpm
"""

import argparse
import sys
from pathlib import Path
from xml.etree.ElementTree import Element, SubElement, indent, tostring

# ---------------------------------------------------------------------------
# Velocity layers — Vibe's canonical curve
# ---------------------------------------------------------------------------

# Ghost Council Modified zones (QDD Level 2, adopted 2026-04-04).
# Replaces old even-split [pp:1-31, mp:32-63, mf:64-95, ff:96-127].
VIBE_VELOCITY_LAYERS = [
    (1,  20,  "ghost"),   # vel  1-20  — below pad threshold, ghost notes
    (21, 55,  "light"),   # vel 21-55  — finger drumming sweet spot
    (56, 90,  "medium"),  # vel 56-90  — deliberate hits
    (91, 127, "hard"),    # vel 91-127 — power hits, peak force
]

# ---------------------------------------------------------------------------
# MPC pad → MIDI note mapping (standard MPC layout, bottom-left = pad 1 = A1)
# Pads 1-16 map to MIDI notes 37-52 in MPC convention.
# ---------------------------------------------------------------------------

PAD_MIDI_NOTES = {i: 36 + i for i in range(1, 17)}  # pad 1→37, pad 16→52

# ---------------------------------------------------------------------------
# Chromatic range: MIDI 21 (A0) to MIDI 108 (C8) = 88 keys
# ---------------------------------------------------------------------------

CHROMATIC_LOW  = 21
CHROMATIC_HIGH = 108

# ---------------------------------------------------------------------------
# Stem mixer layout
# ---------------------------------------------------------------------------

STEM_LAYOUT = [
    ("Kick",    1, "A"),
    ("Snare",   2, "A"),
    ("Bass",    3, "B"),
    ("Melody",  4, "B"),
    ("Chord",   5, "C"),
    ("Hat",     6, "C"),
    ("FX",      7, "D"),
    ("Vocal",   8, "D"),
]

# ---------------------------------------------------------------------------
# Template metadata — descriptions for --describe
# ---------------------------------------------------------------------------

TEMPLATE_META = {
    "basic_drum_kit": {
        "title": "Basic Drum Kit",
        "summary": "16-pad drum program. Each pad maps to one sample, single velocity layer.",
        "pads": 16,
        "keygroups": 16,
        "velocity_layers": 1,
        "key_track": False,
        "use_case": "Quick drum kits, one-shot packs, beat starter kits.",
        "samples_needed": "Up to 16 WAV files (extra samples ignored; missing pads left empty).",
    },
    "velocity_drum_kit": {
        "title": "Velocity Drum Kit",
        "summary": (
            "8 drum sounds × 4 velocity layers using Ghost Council Modified zones "
            "(ghost: 1-20, light: 21-55, medium: 56-90, hard: 91-127). 32 keygroups total."
        ),
        "pads": 8,
        "keygroups": 32,
        "velocity_layers": 4,
        "key_track": False,
        "use_case": "Expressive drum kits where velocity drives timbre, not just volume.",
        "samples_needed": (
            "Provide samples grouped by sound then layer: kick_ghost.wav kick_light.wav "
            "kick_medium.wav kick_hard.wav snare_ghost.wav ... (32 files total, 4 per sound). "
            "Or provide 8 files to use the same sample across all layers."
        ),
    },
    "chromatic_instrument": {
        "title": "Chromatic Instrument",
        "summary": (
            "88-key chromatic keygroup program. One sample stretches across the full "
            "keyboard. KeyTrack=True, RootNote=0 (MPC auto-detect)."
        ),
        "pads": 1,
        "keygroups": 88,
        "velocity_layers": 1,
        "key_track": True,
        "use_case": "Single-sample instruments: piano, synth pad, bass, pluck.",
        "samples_needed": "1 WAV file. The sample is mapped across all 88 keys.",
    },
    "multisampled_instrument": {
        "title": "Multisampled Instrument",
        "summary": (
            "8 root notes × 4 velocity layers = 32 keygroups. Each keygroup covers a "
            "~1-octave range. KeyTrack=True within each zone."
        ),
        "pads": 8,
        "keygroups": 32,
        "velocity_layers": 4,
        "key_track": True,
        "use_case": "Quality multisampled instruments: strings, brass, keys, mallets.",
        "samples_needed": (
            "32 WAV files: 8 root notes × 4 velocity layers. "
            "Root notes default to C2 C3 C4 C5 C6 G2 G3 G4 (MIDI 36,48,60,72,84,43,55,67). "
            "Provide files ordered: root0_pp root0_mp root0_mf root0_ff root1_pp ..."
        ),
    },
    "oneshot_sampler": {
        "title": "One-Shot Sampler",
        "summary": (
            "16 pads optimized for one-shots: looping disabled, single velocity layer, "
            "fast attack, no sustain loop points."
        ),
        "pads": 16,
        "keygroups": 16,
        "velocity_layers": 1,
        "key_track": False,
        "use_case": "SFX packs, foley, vocal chops, transition hits, stingers.",
        "samples_needed": "Up to 16 WAV files.",
    },
    "stem_mixer": {
        "title": "Stem Mixer",
        "summary": (
            "8 stems across 8 pads with choke groups: Kick+Snare (A), Bass+Melody (B), "
            "Chord+Hat (C), FX+Vocal (D). Single layer, no key tracking."
        ),
        "pads": 8,
        "keygroups": 8,
        "velocity_layers": 1,
        "key_track": False,
        "use_case": "Stem performance, live mixing, arrangement deconstruction.",
        "samples_needed": "Up to 8 WAV files (Kick, Snare, Bass, Melody, Chord, Hat, FX, Vocal).",
    },
}

# ---------------------------------------------------------------------------
# XML helpers
# ---------------------------------------------------------------------------

def _program_root(name: str, program_type: str = "Keygroup") -> Element:
    root = Element("MPCVObject")
    root.set("Version", "2.1")
    prog = SubElement(root, "Program")
    prog.set("type", program_type)
    SubElement(prog, "Name").text = name
    return root, prog


def _keygroup(parent: Element, kg_num: int, low_note: int, high_note: int,
              root_note: int = 0, key_track: bool = True) -> Element:
    kg = SubElement(parent, "KeyGroup")
    kg.set("number", str(kg_num))
    SubElement(kg, "LowNote").text  = str(low_note)
    SubElement(kg, "HighNote").text = str(high_note)
    SubElement(kg, "RootNote").text = str(root_note)
    SubElement(kg, "KeyTrack").text = "True" if key_track else "False"
    return kg


def _layer(kg: Element, layer_num: int, sample_path: str,
           vel_start: int, vel_end: int) -> Element:
    lay = SubElement(kg, "Layer")
    lay.set("number", str(layer_num))
    SubElement(lay, "SampleName").text = sample_path
    SubElement(lay, "VelStart").text   = str(vel_start)
    SubElement(lay, "VelEnd").text     = str(vel_end)
    return lay


def _empty_layer(kg: Element, layer_num: int) -> Element:
    """Unused layer — VelStart=0 prevents ghost triggering."""
    return _layer(kg, layer_num, "", 0, 0)


def _pretty_xml(root: Element) -> str:
    indent(root, space="  ")
    raw = tostring(root, encoding="unicode", xml_declaration=False)
    return raw


def _resolve_sample(samples: list[str], idx: int) -> str:
    if not samples or idx >= len(samples):
        return ""
    return str(Path(samples[idx]).name) if samples[idx] else ""

# ---------------------------------------------------------------------------
# Template generators
# ---------------------------------------------------------------------------

def _gen_basic_drum_kit(name: str, samples: list[str]) -> str:
    root, prog = _program_root(name, "Drum")
    kgs = SubElement(prog, "KeyGroups")
    for pad_num in range(1, 17):
        midi_note = PAD_MIDI_NOTES[pad_num]
        sample    = _resolve_sample(samples, pad_num - 1)
        kg = _keygroup(kgs, pad_num, midi_note, midi_note,
                       root_note=0, key_track=False)
        _layer(kg, 1, sample, 1, 127)
        for extra in range(2, 5):
            _empty_layer(kg, extra)
    return _pretty_xml(root)


def _gen_velocity_drum_kit(name: str, samples: list[str]) -> str:
    """
    8 sounds × 4 velocity layers.
    Sample order: sound0_pp sound0_mp sound0_mf sound0_ff sound1_pp ...
    If only 8 samples are provided, the same sample is used for all 4 layers.
    """
    root, prog = _program_root(name, "Drum")
    kgs = SubElement(prog, "KeyGroups")
    single_sample_per_sound = len(samples) <= 8

    for sound_idx in range(8):
        pad_num   = sound_idx + 1
        midi_note = PAD_MIDI_NOTES[pad_num]
        kg = _keygroup(kgs, pad_num, midi_note, midi_note,
                       root_note=0, key_track=False)
        for layer_idx, (v_start, v_end, _) in enumerate(VIBE_VELOCITY_LAYERS):
            if single_sample_per_sound:
                sample = _resolve_sample(samples, sound_idx)
            else:
                sample = _resolve_sample(samples, sound_idx * 4 + layer_idx)
            _layer(kg, layer_idx + 1, sample, v_start, v_end)
    return _pretty_xml(root)


def _gen_chromatic_instrument(name: str, samples: list[str]) -> str:
    """Single sample spanning 88 keys."""
    root, prog = _program_root(name, "Keygroup")
    kgs = SubElement(prog, "KeyGroups")
    sample = _resolve_sample(samples, 0)
    kg = _keygroup(kgs, 1, CHROMATIC_LOW, CHROMATIC_HIGH,
                   root_note=0, key_track=True)
    _layer(kg, 1, sample, 1, 127)
    for extra in range(2, 5):
        _empty_layer(kg, extra)
    return _pretty_xml(root)


# Default root notes for multisampled_instrument: 8 pitches covering range
_MULTI_ROOT_NOTES = [36, 48, 60, 72, 84, 43, 55, 67]  # C2 C3 C4 C5 C6 G2 G3 G4

def _multi_zone_bounds(root_notes: list[int]) -> list[tuple[int, int]]:
    """Compute low/high note zone boundaries centered around each root note."""
    sorted_roots = sorted(root_notes)
    bounds = []
    for i, root in enumerate(sorted_roots):
        low  = 0  if i == 0 else (sorted_roots[i - 1] + root) // 2 + 1
        high = 127 if i == len(sorted_roots) - 1 else (root + sorted_roots[i + 1]) // 2
        bounds.append((low, high))
    return bounds


def _gen_multisampled_instrument(name: str, samples: list[str]) -> str:
    root, prog = _program_root(name, "Keygroup")
    kgs = SubElement(prog, "KeyGroups")
    root_notes = _MULTI_ROOT_NOTES
    bounds     = _multi_zone_bounds(sorted(root_notes))
    single_sample_per_root = len(samples) <= 8

    kg_num = 1
    for zone_idx, (root_midi, (low, high)) in enumerate(
            zip(sorted(root_notes), bounds)):
        for layer_idx, (v_start, v_end, _) in enumerate(VIBE_VELOCITY_LAYERS):
            if single_sample_per_root:
                sample = _resolve_sample(samples, zone_idx)
            else:
                sample = _resolve_sample(samples, zone_idx * 4 + layer_idx)
            kg = _keygroup(kgs, kg_num, low, high,
                           root_note=root_midi, key_track=True)
            _layer(kg, 1, sample, v_start, v_end)
            for extra in range(2, 5):
                _empty_layer(kg, extra)
            kg_num += 1
    return _pretty_xml(root)


def _gen_oneshot_sampler(name: str, samples: list[str]) -> str:
    """16 pads, one-shot optimized (LoopMode=Off placeholder comment)."""
    root, prog = _program_root(name, "Drum")
    SubElement(prog, "OneShot").text = "True"
    kgs = SubElement(prog, "KeyGroups")
    for pad_num in range(1, 17):
        midi_note = PAD_MIDI_NOTES[pad_num]
        sample    = _resolve_sample(samples, pad_num - 1)
        kg = _keygroup(kgs, pad_num, midi_note, midi_note,
                       root_note=0, key_track=False)
        SubElement(kg, "LoopMode").text = "Off"
        _layer(kg, 1, sample, 1, 127)
        for extra in range(2, 5):
            _empty_layer(kg, extra)
    return _pretty_xml(root)


def _gen_stem_mixer(name: str, samples: list[str]) -> str:
    root, prog = _program_root(name, "Drum")
    kgs = SubElement(prog, "KeyGroups")
    for i, (stem_name, pad_num, choke_group) in enumerate(STEM_LAYOUT):
        midi_note = PAD_MIDI_NOTES[pad_num]
        sample    = _resolve_sample(samples, i)
        kg = _keygroup(kgs, pad_num, midi_note, midi_note,
                       root_note=0, key_track=False)
        SubElement(kg, "PadName").text    = stem_name
        SubElement(kg, "ChokGroup").text  = choke_group
        _layer(kg, 1, sample, 1, 127)
        for extra in range(2, 5):
            _empty_layer(kg, extra)
    return _pretty_xml(root)


# ---------------------------------------------------------------------------
# Dispatch table
# ---------------------------------------------------------------------------

GENERATORS = {
    "basic_drum_kit":         _gen_basic_drum_kit,
    "velocity_drum_kit":      _gen_velocity_drum_kit,
    "chromatic_instrument":   _gen_chromatic_instrument,
    "multisampled_instrument": _gen_multisampled_instrument,
    "oneshot_sampler":        _gen_oneshot_sampler,
    "stem_mixer":             _gen_stem_mixer,
}

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def cmd_list() -> None:
    print("Available XPM templates:\n")
    for key, meta in TEMPLATE_META.items():
        print(f"  {key:<26} {meta['title']}")
        print(f"  {'':26} Pads: {meta['pads']}  "
              f"Keygroups: {meta['keygroups']}  "
              f"Vel layers: {meta['velocity_layers']}  "
              f"KeyTrack: {meta['key_track']}")
        print()


def cmd_describe(template: str) -> None:
    meta = TEMPLATE_META[template]
    print(f"\n  Template : {template}")
    print(f"  Title    : {meta['title']}")
    print(f"  Summary  : {meta['summary']}")
    print(f"  Pads     : {meta['pads']}")
    print(f"  Keygroups: {meta['keygroups']}")
    print(f"  Vel layers: {meta['velocity_layers']}")
    print(f"  KeyTrack : {meta['key_track']}")
    print(f"  Use case : {meta['use_case']}")
    print(f"  Samples  : {meta['samples_needed']}")
    if template == "velocity_drum_kit":
        print("\n  Ghost Council Modified velocity curve (adopted 2026-04-04):")
        for v_start, v_end, label in VIBE_VELOCITY_LAYERS:
            print(f"    Layer {VIBE_VELOCITY_LAYERS.index((v_start, v_end, label)) + 1}:"
                  f"  vel {v_start:>3}–{v_end:<3}  ({label})")
    print()


def main() -> None:
    parser = argparse.ArgumentParser(
        prog="xpn_xpm_template_library.py",
        description="XPN XPM Template Library — XO_OX Designs",
    )
    parser.add_argument("--list",     action="store_true",
                        help="List all available templates")
    parser.add_argument("--template", metavar="NAME",
                        help="Template to generate or describe")
    parser.add_argument("--describe", action="store_true",
                        help="Print detailed description of the template")
    parser.add_argument("--samples",  nargs="+", metavar="FILE",
                        help="Sample WAV paths to populate the template")
    parser.add_argument("--output",   metavar="FILE",
                        help="Output XPM file path (default: <template>.xpm)")
    parser.add_argument("--name",     metavar="NAME",
                        help="Program name embedded in XPM (default: template title)")
    args = parser.parse_args()

    if args.list:
        cmd_list()
        return

    if not args.template:
        parser.print_help()
        return

    template = args.template
    if template not in GENERATORS:
        print(f"Error: unknown template '{template}'.", file=sys.stderr)
        print(f"Run with --list to see available templates.", file=sys.stderr)
        sys.exit(1)

    if args.describe:
        cmd_describe(template)
        return

    prog_name = args.name or TEMPLATE_META[template]["title"]
    samples   = args.samples or []
    xml_str   = GENERATORS[template](prog_name, samples)

    output_path = Path(args.output) if args.output else Path(f"{template}.xpm")
    output_path.write_text(xml_str, encoding="utf-8")
    print(f"Written: {output_path}")
    print(f"Template : {template}")
    print(f"Name     : {prog_name}")
    if samples:
        print(f"Samples  : {len(samples)} file(s) provided")
    else:
        print("Samples  : none — empty template (fill SampleName fields manually)")


if __name__ == "__main__":
    main()
