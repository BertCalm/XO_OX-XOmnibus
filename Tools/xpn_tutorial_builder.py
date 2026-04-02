#!/usr/bin/env python3
"""
XPN Tutorial Builder — XO_OX Designs
Creates an educational XPN pack from a source XPM drum program, bundling a
main program with 8 progressive step programs that teach a technique layer by
layer.

Output XPN structure:
  PackName.xpn (ZIP):
    Programs/
      main_program.xpm          — the full-featured program (copy of source)
      Step1_Foundation.xpm      — simplest version (kick zone only)
      Step2_AddSnare.xpm        — step 1 + snare zone
      Step3_AddHats.xpm         — step 2 + hi-hat zone
      Step4_AddPerc.xpm         — step 3 + perc zone
      Step5_FullKit.xpm         — all instruments active
      Step6_VelocityLayers.xpm  — step 5 with 4 velocity layers enabled
      Step7_RoundRobin.xpm      — step 6 + round-robin cycling
      Step8_Challenge.xpm       — exact copy of main_program (full explore)
    Tutorial/
      tutorial.md               — step-by-step written guide
      tutorial_audio_notes.txt  — what each step should sound like
      step_parameters.json      — machine-readable per-step state
    Samples/
      [WAV files — shared across all programs; referenced from source XPM]
    manifest.json               — expansion metadata with tutorial fields

Auto-generation MIDI zones (GM convention):
  Kick zone    — notes 36-37
  Snare zone   — notes 38-39
  Hat zone     — notes 42-46
  Perc zone    — notes 41, 43, 47, 49

XPN golden rules (enforced in all generated programs):
  KeyTrack  = True
  RootNote  = 0
  VelStart  = 0  for empty/muted layers

Usage:
    python xpn_tutorial_builder.py \\
        --source main_program.xpm \\
        --output ./out/ \\
        --name "ONSET_Tutorial" \\
        [--steps step_defs.json]

Step definitions JSON schema (optional):
    [
        {
            "step": 1,
            "label": "Foundation",
            "description": "Kick only — feel the pulse.",
            "enabled_notes": [36, 37],
            "velocity_layers": 2,
            "round_robin": false
        },
        ...
    ]

Pure stdlib. No third-party dependencies.
"""

import argparse
import json
import re
import sys
import zipfile
from datetime import date
from pathlib import Path
from typing import FrozenSet, List, Optional
from xml.etree import ElementTree as ET
from xml.sax.saxutils import escape as xml_escape

# =============================================================================
# CONSTANTS
# =============================================================================

TOOL_VERSION = "1.0.0"
AUTHOR = "XO_OX Designs"
TODAY = str(date.today())

# MIDI note zone definitions (auto-generation mode)
ZONE_KICK  = frozenset([36, 37])
ZONE_SNARE = frozenset([38, 39])
ZONE_HAT   = frozenset([42, 43, 44, 46])
ZONE_PERC  = frozenset([41, 47, 48, 49, 50, 51])

# Cumulative note sets per step (auto-generation)
AUTO_STEP_ZONES = [
    # (label, short_description, enabled_notes_frozenset, vel_layers, round_robin)
    (1, "Foundation",       "Kick only — feel the pulse",
     ZONE_KICK,                                         2, False),
    (2, "AddSnare",         "Kick + snare — backbeat locked",
     ZONE_KICK | ZONE_SNARE,                            2, False),
    (3, "AddHats",          "Pocket full: kick, snare, and hi-hats",
     ZONE_KICK | ZONE_SNARE | ZONE_HAT,                 2, False),
    (4, "AddPerc",          "Full groove: hats, kick, snare, perc",
     ZONE_KICK | ZONE_SNARE | ZONE_HAT | ZONE_PERC,    2, False),
    (5, "FullKit",          "All instruments active — open sandbox",
     None,                                              2, False),   # None = all
    (6, "VelocityLayers",   "Same kit — now with 4 velocity layers",
     None,                                              4, False),
    (7, "RoundRobin",       "Layer 6 + round-robin cycling enabled",
     None,                                              4, True),
    (8, "Challenge",        "Full featured program — your turn to explore",
     None,                                              4, True),    # = main_program copy
]

# Human-readable step names for filenames
STEP_FILENAMES = [
    "Step1_Foundation.xpm",
    "Step2_AddSnare.xpm",
    "Step3_AddHats.xpm",
    "Step4_AddPerc.xpm",
    "Step5_FullKit.xpm",
    "Step6_VelocityLayers.xpm",
    "Step7_RoundRobin.xpm",
    "Step8_Challenge.xpm",
]

# Tutorial text written per step (used when auto-generating tutorial.md)
STEP_TUTORIAL_TEXT = [
    {
        "heading": "Step 1: Foundation — Kick Only",
        "goal": "Lock in the pulse before adding anything else.",
        "instructions": [
            "Load Step1_Foundation.xpm into an MPC track.",
            "Play a simple 4-on-the-floor pattern: pads A1 on every beat.",
            "Notice how the kick sits in the mix — listen for the low-end thud.",
            "Experiment with velocity: ghost hits vs. full hits.",
            "Goal: establish the groove skeleton. Everything else builds on this.",
        ],
        "producer_tip": "A kick that locks with your BPM is worth more than a flashy kit. Get it right first.",
    },
    {
        "heading": "Step 2: Add Snare — Backbeat Locked",
        "goal": "Place the snare against the kick and feel the backbeat snap.",
        "instructions": [
            "Load Step2_AddSnare.xpm. Pad A2 is now active.",
            "Play kick on 1 and 3, snare on 2 and 4 (classic backbeat).",
            "Listen for the relationship between kick tail and snare attack.",
            "Try varying snare velocity — quiet snares create tension.",
        ],
        "producer_tip": "Kick and snare should feel like conversation partners, not competitors. Adjust levels until they breathe together.",
    },
    {
        "heading": "Step 3: Add Hi-Hats — Pocket Full",
        "goal": "Subdivide the time with hi-hats and feel the groove lock in.",
        "instructions": [
            "Load Step3_AddHats.xpm. Pads A4 (closed hat) and B1 (open hat) are now active.",
            "Close hat on 8th notes, open hat on upbeats.",
            "Play the closed hat to mute the open hat (hat choke group is active).",
            "Experiment: 16th-note closed hats for a tighter feel.",
        ],
        "producer_tip": "Hi-hats carry the internal tempo. Humans rush hats; machines play perfect — choose your flavor consciously.",
    },
    {
        "heading": "Step 4: Add Percussion — Full Groove",
        "goal": "Layer percussion to add texture and polyrhythm.",
        "instructions": [
            "Load Step4_AddPerc.xpm. All remaining perc pads are now active.",
            "Try placing perc hits in the spaces between kick and snare.",
            "Stack a perc hit on the snare for a layered sound.",
            "Think of perc as spice — it accents the groove, not replaces it.",
        ],
        "producer_tip": "Less is more at first. Add one perc note at a time and listen before adding another.",
    },
    {
        "heading": "Step 5: Full Kit — Open Sandbox",
        "goal": "All instruments active. Play freely with the full palette.",
        "instructions": [
            "Load Step5_FullKit.xpm. Every pad triggers a sound.",
            "Revisit the patterns from Steps 1-4 and add the FX pad (B4).",
            "Try fills: run from pad to pad in sequence.",
            "At this stage, all pads use 2 velocity layers — notice the dynamic range.",
        ],
        "producer_tip": "Record a 4-bar loop without editing. Listen back and identify the strongest moments.",
    },
    {
        "heading": "Step 6: Velocity Layers — Feel the Dynamics",
        "goal": "Experience how 4 velocity layers transform the expressiveness of each hit.",
        "instructions": [
            "Load Step6_VelocityLayers.xpm. Same kit, now with 4 layers per instrument.",
            "Play the same pattern as Step 5 but vary your pad pressure intentionally.",
            "Ghost hits (very low velocity) should sound thin and distant.",
            "Hard hits (full velocity) should punch through.",
            "The dynamic range between ghost and hard is the expressive range of the kit.",
        ],
        "producer_tip": "Velocity layers are invisible to the eye but everything to the ear. Program a few ghost kicks before the downbeat — the groove will come alive.",
    },
    {
        "heading": "Step 7: Round-Robin — Machine Gun Prevention",
        "goal": "Hear how round-robin cycling prevents the mechanical repetition of identical hits.",
        "instructions": [
            "Load Step7_RoundRobin.xpm. Cycling is now enabled on compatible pads.",
            "Play the same hi-hat pattern you used in Step 3, then Step 7.",
            "Listen for the subtle variation — each hit cycles through a different sample.",
            "Fast 16th-note hats reveal the effect most clearly.",
            "The MPC cycles through sample variants automatically — you just play.",
        ],
        "producer_tip": "Round-robin is most effective on repetitive parts: closed hats, shakers, rides. For kicks and snares, velocity layers usually serve better.",
    },
    {
        "heading": "Step 8: Challenge — Your Turn",
        "goal": "This is the full program with every technique active. Explore on your own.",
        "instructions": [
            "Load Step8_Challenge.xpm — it is the complete main program.",
            "Combine everything: backbeat, hi-hats, perc, velocity shaping, and cycling.",
            "Record a 16-bar sequence without quantize correction.",
            "Listen back and notice where the groove lives and where it breaks down.",
            "Revisit earlier steps to isolate and fix any weaknesses.",
        ],
        "producer_tip": "The goal of this pack is not to give you beats — it's to teach you how to build them. Come back to Step 1 whenever you feel lost.",
    },
]


# =============================================================================
# XPM PARSING
# =============================================================================

def load_xpm(path: Path) -> ET.ElementTree:
    """Parse an XPM XML file and return an ElementTree."""
    try:
        tree = ET.parse(str(path))
    except ET.ParseError as exc:
        sys.exit(f"[ERROR] Failed to parse XPM '{path}': {exc}")
    return tree


def collect_samples_from_xpm(tree: ET.ElementTree) -> list[str]:
    """
    Return all unique sample file paths referenced in the XPM.
    Looks in <File> and <SampleFile> elements inside <Layer> blocks.
    Filters out empty strings.
    """
    samples = set()
    root = tree.getroot()
    for elem in root.iter("Layer"):
        for tag in ("File", "SampleFile", "SampleName"):
            node = elem.find(tag)
            if node is not None and node.text and node.text.strip():
                samples.add(node.text.strip())
    return sorted(samples)


def get_instrument_note(instrument_elem: ET.Element) -> int:
    """
    Determine the MIDI note number for an <Instrument> element.
    Uses the 'number' attribute directly as the MIDI note index.
    """
    try:
        return int(instrument_elem.get("number", "-1"))
    except (ValueError, TypeError):
        return -1


def has_active_layers(instrument_elem: ET.Element) -> bool:
    """Return True if the instrument has at least one active, non-empty layer."""
    for layer in instrument_elem.findall(".//Layer"):
        active_node = layer.find("Active")
        sample_node = layer.find("SampleName")
        if active_node is not None and active_node.text == "True":
            if sample_node is not None and sample_node.text and sample_node.text.strip():
                return True
    return False


def count_active_layers(instrument_elem: ET.Element) -> int:
    """Count layers that are active with a non-empty sample."""
    count = 0
    for layer in instrument_elem.findall(".//Layer"):
        active_node = layer.find("Active")
        sample_node = layer.find("SampleName")
        if active_node is not None and active_node.text == "True":
            if sample_node is not None and sample_node.text and sample_node.text.strip():
                count += 1
    return count


# =============================================================================
# XPM MUTATION HELPERS
# =============================================================================

def silence_instrument(instrument_elem: ET.Element) -> None:
    """
    Mute an instrument by setting VelStart=0 on all layers and Active=False.
    Golden rule: VelStart=0 for empty/muted layers (prevents ghost triggering).
    """
    for layer in instrument_elem.findall(".//Layer"):
        vel_node = layer.find("VelStart")
        if vel_node is not None:
            vel_node.text = "0"
        active_node = layer.find("Active")
        if active_node is not None:
            active_node.text = "False"
        # Clear sample references to keep the file clean
        for tag in ("SampleName", "SampleFile", "File"):
            node = layer.find(tag)
            if node is not None:
                node.text = ""


def trim_to_n_layers(instrument_elem: ET.Element, n: int) -> None:
    """
    Keep only the first n layers active; silence the rest.
    Used to reduce 4-layer instruments back to 2-layer for early steps.
    """
    layers = instrument_elem.findall(".//Layers/Layer")
    for i, layer in enumerate(layers):
        if i >= n:
            vel_node = layer.find("VelStart")
            if vel_node is not None:
                vel_node.text = "0"
            vel_end_node = layer.find("VelEnd")
            if vel_end_node is not None:
                vel_end_node.text = "0"
            active_node = layer.find("Active")
            if active_node is not None:
                active_node.text = "False"
            for tag in ("SampleName", "SampleFile", "File"):
                node = layer.find(tag)
                if node is not None:
                    node.text = ""


def enable_round_robin(instrument_elem: ET.Element, cycle_group: int) -> None:
    """
    Set ZonePlay=2 (Cycle) and add CycleType/CycleGroup to each active layer.
    Enables round-robin cycling for machine-gun prevention.
    """
    zone_play = instrument_elem.find("ZonePlay")
    if zone_play is not None:
        zone_play.text = "2"  # Cycle

    for layer in instrument_elem.findall(".//Layers/Layer"):
        active_node = layer.find("Active")
        if active_node is None or active_node.text != "True":
            continue
        # Insert CycleType and CycleGroup after Active if not already present
        if layer.find("CycleType") is None:
            ct_elem = ET.SubElement(layer, "CycleType")
            ct_elem.text = "RoundRobin"
        else:
            layer.find("CycleType").text = "RoundRobin"
        if layer.find("CycleGroup") is None:
            cg_elem = ET.SubElement(layer, "CycleGroup")
            cg_elem.text = str(cycle_group)
        else:
            layer.find("CycleGroup").text = str(cycle_group)


def build_step_tree(source_tree: ET.ElementTree,
                    enabled_notes: Optional[FrozenSet[int]],
                    vel_layers: int,
                    round_robin: bool) -> ET.ElementTree:
    """
    Deep-copy the source tree and mutate it for a step:
      - Silence instruments whose MIDI note is NOT in enabled_notes
        (None means all instruments are enabled)
      - Trim layers to vel_layers per active instrument
      - Optionally enable round-robin cycling
    Returns the mutated tree.
    """
    # Deep copy via serialise + re-parse (stdlib, no copy.deepcopy on ET nodes)
    raw_xml = ET.tostring(source_tree.getroot(), encoding="unicode")
    root_copy = ET.fromstring(raw_xml)
    tree_copy = ET.ElementTree(root_copy)

    instruments_elem = root_copy.find(".//Instruments")
    if instruments_elem is None:
        return tree_copy  # Nothing to mutate; return as-is

    rr_group_counter = 0
    for inst in instruments_elem.findall("Instrument"):
        note = get_instrument_note(inst)
        instrument_is_active = has_active_layers(inst)

        if not instrument_is_active:
            continue  # Already empty — no action needed

        if enabled_notes is not None and note not in enabled_notes:
            # Silence this instrument for this step
            silence_instrument(inst)
            continue

        # Instrument is enabled — apply layer/rr constraints
        if vel_layers < count_active_layers(inst):
            trim_to_n_layers(inst, vel_layers)

        if round_robin:
            rr_group_counter += 1
            enable_round_robin(inst, rr_group_counter)

    return tree_copy


def tree_to_string(tree: ET.ElementTree) -> str:
    """Serialise an ElementTree to a well-formed XML string."""
    raw = ET.tostring(tree.getroot(), encoding="unicode", xml_declaration=False)
    return '<?xml version="1.0" encoding="UTF-8"?>\n\n' + raw + "\n"


# =============================================================================
# STEP DEFINITIONS
# =============================================================================

def auto_step_definitions() -> list[dict]:
    """
    Build the 8-step auto-generation plan.
    Returns a list of dicts with keys:
        step, label, description, enabled_notes (frozenset|None),
        vel_layers, round_robin
    """
    steps = []
    for (step, label, desc, notes, vel, rr) in AUTO_STEP_ZONES:
        steps.append({
            "step": step,
            "label": label,
            "description": desc,
            "enabled_notes": notes,   # frozenset or None (all)
            "vel_layers": vel,
            "round_robin": rr,
        })
    return steps


def load_step_definitions(path: Path) -> list[dict]:
    """
    Load step definitions from a JSON file.
    Validates required keys and converts enabled_notes lists to frozensets.
    Returns a list of step dicts (same schema as auto_step_definitions).
    """
    try:
        with open(path) as f:
            raw = json.load(f)
    except (OSError, json.JSONDecodeError) as exc:
        sys.exit(f"[ERROR] Failed to load step definitions '{path}': {exc}")

    if not isinstance(raw, list):
        sys.exit("[ERROR] Step definitions JSON must be a top-level array.")

    steps = []
    required_keys = {"step", "label", "description"}
    for i, item in enumerate(raw):
        missing = required_keys - set(item.keys())
        if missing:
            sys.exit(f"[ERROR] Step definition index {i} missing keys: {missing}")
        notes_raw = item.get("enabled_notes", None)
        if notes_raw is None:
            notes = None  # all notes enabled
        elif isinstance(notes_raw, list):
            notes = frozenset(int(n) for n in notes_raw)
        else:
            sys.exit(f"[ERROR] Step {item['step']}: enabled_notes must be a list or null.")
        steps.append({
            "step":          int(item["step"]),
            "label":         str(item["label"]),
            "description":   str(item["description"]),
            "enabled_notes": notes,
            "vel_layers":    int(item.get("velocity_layers", 2)),
            "round_robin":   bool(item.get("round_robin", False)),
        })

    # Sort by step number
    steps.sort(key=lambda s: s["step"])
    return steps


# =============================================================================
# TUTORIAL DOCUMENT GENERATORS
# =============================================================================

def generate_tutorial_md(pack_name: str,
                         step_defs: list[dict],
                         custom_steps: bool = False) -> str:
    """Generate tutorial.md with step-by-step instructions."""
    lines = [
        f"# {pack_name} — Tutorial Guide",
        "",
        f"**Generated:** {TODAY}  ",
        f"**Author:** {AUTHOR}  ",
        f"**Format:** Akai MPC Expansion Pack (.xpn)",
        "",
        "---",
        "",
        "## Overview",
        "",
        "This tutorial pack teaches drum programming technique progressively.",
        "Each XPM program in the `Programs/` folder builds on the last.",
        "Follow the steps in order, then experiment freely with `Step8_Challenge.xpm`.",
        "",
        "### Programs in this pack",
        "",
    ]

    lines.append("| File | Step | Focus |")
    lines.append("|------|------|-------|")
    lines.append("| `main_program.xpm` | — | Full-featured reference program |")

    for i, step in enumerate(step_defs):
        filename = STEP_FILENAMES[i] if i < len(STEP_FILENAMES) else f"Step{step['step']}_{step['label']}.xpm"
        lines.append(f"| `{filename}` | {step['step']} | {step['description']} |")

    lines += ["", "---", ""]

    # Write per-step sections
    for i, step in enumerate(step_defs):
        if not custom_steps and i < len(STEP_TUTORIAL_TEXT):
            # Use pre-written rich text
            t = STEP_TUTORIAL_TEXT[i]
            lines.append(f"## {t['heading']}")
            lines.append("")
            lines.append(f"**Goal:** {t['goal']}")
            lines.append("")
            lines.append("**Instructions:**")
            lines.append("")
            for instr in t["instructions"]:
                lines.append(f"- {instr}")
            lines.append("")
            lines.append(f"> **Producer Tip:** {t['producer_tip']}")
            lines.append("")
        else:
            # Minimal auto-generated section for custom steps
            lines.append(f"## Step {step['step']}: {step['label']}")
            lines.append("")
            lines.append(f"**Goal:** {step['description']}")
            lines.append("")
            notes_desc = "all instruments" if step["enabled_notes"] is None else \
                         f"MIDI notes: {sorted(step['enabled_notes'])}"
            lines.append(f"- Active: {notes_desc}")
            lines.append(f"- Velocity layers: {step['vel_layers']}")
            lines.append(f"- Round-robin cycling: {'enabled' if step['round_robin'] else 'disabled'}")
            lines.append("")

        lines.append("---")
        lines.append("")

    lines += [
        "## About XO_OX Designs",
        "",
        "XO_OX builds character instruments — synthesizers and drum engines with",
        "personality, doctrine, and a point of view. XOceanus is the multi-engine",
        "platform at the center of the ecosystem.",
        "",
        f"More at: https://xo-ox.org",
        "",
    ]

    return "\n".join(lines)


def generate_audio_notes_txt(pack_name: str, step_defs: list[dict]) -> str:
    """Generate tutorial_audio_notes.txt describing what each step should sound like."""
    audio_notes = {
        1: "Kick only. Should sound like a simple, clear thud — no clutter. The kick tail should decay naturally. If it sounds muddy, the low-end is too crowded; trust the foundation.",
        2: "Kick and snare together. The backbeat should click into place. The snare attack should cut through without masking the kick attack transient. Ghost snares should feel tense.",
        3: "Full pocket. Hats subdivide the time between kick and snare. Open hat rings should fade before the next closed hat hit if you are programming hat choke correctly. The groove should feel locked.",
        4: "Dense texture. Perc hits should accent the spaces in the groove, not compete with the backbeat. Listen for polyrhythm — perc that fights the kick is placed wrong.",
        5: "All instruments, 2 velocity layers. The dynamic difference between a ghost hit and a full hit should be audible but not dramatic. This is your sound design reference point.",
        6: "All instruments, 4 velocity layers. Ghost hits should almost disappear. Hard hits should fully punch. The range of expression is now maximum — this is the expressive kit.",
        7: "Round-robin active. Repeated hats, percs, or claps should each sound subtly different. The machine-gun effect (identical repetition) should be gone. The kit should feel human.",
        8: "The complete program. This should sound like a professionally programmed beat — dynamic, human, expressive. If it sounds mechanical, return to Step 6 and study the velocity shaping.",
    }

    lines = [
        f"# {pack_name} — Audio Reference Notes",
        "",
        f"Generated: {TODAY}",
        "",
        "Use these descriptions to check your progress after loading each step program.",
        "If a step does not sound like described, diagnose with the troubleshooting notes.",
        "",
        "---",
        "",
    ]

    for i, step in enumerate(step_defs):
        filename = STEP_FILENAMES[i] if i < len(STEP_FILENAMES) else f"Step{step['step']}_{step['label']}.xpm"
        note_text = audio_notes.get(step["step"], step["description"])
        lines.append(f"## {filename}")
        lines.append("")
        lines.append(note_text)
        lines.append("")
        lines.append("---")
        lines.append("")

    lines += [
        "## Troubleshooting",
        "",
        "**Kit sounds too thin:** Check that samples are loading. Open the program in MPC",
        "browser and verify each pad shows a waveform.",
        "",
        "**No dynamic variation:** Ensure you are varying pad velocity. The MPC touch strip",
        "or a pad controller with pressure sensitivity is recommended.",
        "",
        "**Round-robin not working:** Verify the ZonePlay setting on each instrument is set",
        "to Cycle (2). This is visible in the Pad Edit screen.",
        "",
        "**Ghost notes triggering on wrong pads:** Check that inactive layers have VelStart=0.",
        "This is enforced in all tutorial programs but can be verified in the program editor.",
        "",
    ]

    return "\n".join(lines)


def generate_step_parameters_json(step_defs: list[dict]) -> str:
    """Generate step_parameters.json — machine-readable per-step state."""
    output = []
    for i, step in enumerate(step_defs):
        filename = STEP_FILENAMES[i] if i < len(STEP_FILENAMES) else f"Step{step['step']}_{step['label']}.xpm"
        output.append({
            "step":             step["step"],
            "label":            step["label"],
            "description":      step["description"],
            "file":             f"Programs/{filename}",
            "enabled_notes":    sorted(step["enabled_notes"]) if step["enabled_notes"] is not None else None,
            "velocity_layers":  step["vel_layers"],
            "round_robin":      step["round_robin"],
        })
    return json.dumps(output, indent=2)


# =============================================================================
# MANIFEST / EXPANSION XML
# =============================================================================

def generate_manifest_json(pack_name: str, pack_id: str,
                           description: str, step_count: int) -> str:
    """Generate manifest.json with tutorial metadata fields."""
    manifest = {
        "format":       "xpn-tutorial",
        "version":      "1.0.0",
        "schema":       "2.0",
        "name":         pack_name,
        "id":           pack_id,
        "manufacturer": AUTHOR,
        "date":         TODAY,
        "type":         "drum",
        "tutorial": {
            "enabled":     True,
            "step_count":  step_count,
            "main_program": "Programs/main_program.xpm",
            "steps_dir":   "Programs/",
            "tutorial_dir":"Tutorial/",
        },
        "description":  description,
    }
    return json.dumps(manifest, indent=2)


# =============================================================================
# ZIP / XPN ASSEMBLY
# =============================================================================

def build_xpn(pack_name: str,
              source_xpm_path: Path,
              output_dir: Path,
              step_defs: list[dict],
              custom_steps: bool = False) -> Path:
    """
    Assemble the XPN ZIP file.

    - Programs/main_program.xpm  — copy of source
    - Programs/Step*.xpm         — generated step programs
    - Tutorial/tutorial.md
    - Tutorial/tutorial_audio_notes.txt
    - Tutorial/step_parameters.json
    - manifest.json

    WAV samples are NOT bundled (user must supply them separately; this mirrors
    the xpn_drum_export.py convention where samples live in Samples/<slug>/).
    A Samples/ placeholder README is written to document the expected layout.

    Returns the path to the created .xpn file.
    """
    output_dir.mkdir(parents=True, exist_ok=True)
    safe_name = re.sub(r"[^\w\-.]", "_", pack_name)
    xpn_path = output_dir / f"{safe_name}.xpn"

    pack_id = f"com.xo-ox.tutorial.{safe_name.lower().replace('_', '-')}"
    description = f"Educational drum tutorial pack: {pack_name}. {len(step_defs)} progressive steps."

    source_tree = load_xpm(source_xpm_path)

    print(f"[INFO] Building {xpn_path.name}")
    print(f"[INFO] Source XPM: {source_xpm_path}")
    print(f"[INFO] Steps: {len(step_defs)}")

    with zipfile.ZipFile(xpn_path, "w", zipfile.ZIP_DEFLATED) as zf:

        # --- main_program.xpm (verbatim copy of source) ---
        main_xpm_str = tree_to_string(source_tree)
        zf.writestr("Programs/main_program.xpm", main_xpm_str)
        print(f"  [+] Programs/main_program.xpm")

        # --- Step programs ---
        for i, step in enumerate(step_defs):
            filename = STEP_FILENAMES[i] if i < len(STEP_FILENAMES) else \
                       f"Step{step['step']}_{step['label']}.xpm"
            arc_path = f"Programs/{filename}"

            if step["step"] == 8:
                # Step 8 = Challenge = verbatim copy of main_program
                step_xml = main_xpm_str
            else:
                step_tree = build_step_tree(
                    source_tree,
                    step["enabled_notes"],
                    step["vel_layers"],
                    step["round_robin"],
                )
                step_xml = tree_to_string(step_tree)

            zf.writestr(arc_path, step_xml)
            notes_desc = (f"{len(step['enabled_notes'])} notes"
                          if step["enabled_notes"] is not None else "all notes")
            print(f"  [+] {arc_path}  ({notes_desc}, {step['vel_layers']}vel, "
                  f"{'RR' if step['round_robin'] else 'no-RR'})")

        # --- Tutorial documents ---
        tutorial_md = generate_tutorial_md(pack_name, step_defs, custom_steps)
        zf.writestr("Tutorial/tutorial.md", tutorial_md)
        print(f"  [+] Tutorial/tutorial.md")

        audio_notes = generate_audio_notes_txt(pack_name, step_defs)
        zf.writestr("Tutorial/tutorial_audio_notes.txt", audio_notes)
        print(f"  [+] Tutorial/tutorial_audio_notes.txt")

        step_params = generate_step_parameters_json(step_defs)
        zf.writestr("Tutorial/step_parameters.json", step_params)
        print(f"  [+] Tutorial/step_parameters.json")

        # --- Samples README ---
        samples_readme = (
            f"# Samples\n\n"
            f"Place your WAV files here, organised in a subfolder matching your preset slug.\n\n"
            f"Expected layout:\n"
            f"  Samples/<preset_slug>/<preset_slug>_kick_v1.wav\n"
            f"  Samples/<preset_slug>/<preset_slug>_kick_v2.wav\n"
            f"  ... etc.\n\n"
            f"All Programs in this pack reference samples relative to the XPN root.\n"
            f"Sample paths are preserved from the source XPM: {source_xpm_path.name}\n"
        )
        # Collect samples referenced in source so the README is informative
        sample_refs = collect_samples_from_xpm(source_tree)
        if sample_refs:
            samples_readme += f"\nSamples referenced in source XPM ({len(sample_refs)}):\n"
            for s in sample_refs:
                samples_readme += f"  {s}\n"

        zf.writestr("Samples/README.md", samples_readme)
        print(f"  [+] Samples/README.md  ({len(sample_refs)} references noted)")

        # --- manifest.json ---
        manifest_str = generate_manifest_json(pack_name, pack_id, description, len(step_defs))
        zf.writestr("manifest.json", manifest_str)
        print(f"  [+] manifest.json")

    print(f"\n[OK] Created: {xpn_path}  ({xpn_path.stat().st_size:,} bytes)")
    return xpn_path


# =============================================================================
# CLI
# =============================================================================

def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="xpn_tutorial_builder",
        description=(
            "XPN Tutorial Builder — creates an educational XPN pack with a "
            "main program + 8 progressive step programs that teach drum technique."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Auto-generate 8 steps from a source XPM
  python xpn_tutorial_builder.py \\
      --source ./my_kit.xpm \\
      --output ./out/ \\
      --name "ONSET_Tutorial"

  # Use a custom step definitions file
  python xpn_tutorial_builder.py \\
      --source ./my_kit.xpm \\
      --output ./out/ \\
      --name "ONSET_Tutorial" \\
      --steps ./my_step_defs.json

  # Print the auto-generated step plan without building
  python xpn_tutorial_builder.py \\
      --source ./my_kit.xpm \\
      --name "ONSET_Tutorial" \\
      --dry-run
""",
    )
    p.add_argument(
        "--source", required=True, metavar="XPM",
        help="Path to the source XPM file (main program / full kit).",
    )
    p.add_argument(
        "--output", metavar="DIR", default=".",
        help="Output directory for the generated XPN file. Default: current directory.",
    )
    p.add_argument(
        "--name", required=True, metavar="NAME",
        help="Pack name, e.g. 'ONSET_Tutorial'. Used as the XPN filename and manifest title.",
    )
    p.add_argument(
        "--steps", metavar="JSON",
        help=(
            "Path to a step definitions JSON file. "
            "If omitted, 8 steps are auto-generated from MIDI note zones."
        ),
    )
    p.add_argument(
        "--dry-run", action="store_true",
        help="Print the step plan and exit without building the XPN.",
    )
    p.add_argument(
        "--version", action="version", version=f"xpn_tutorial_builder {TOOL_VERSION}",
    )
    return p


def main() -> None:
    parser = build_arg_parser()
    args = parser.parse_args()

    source_path = Path(args.source)
    if not source_path.exists():
        sys.exit(f"[ERROR] Source XPM not found: {source_path}")
    if source_path.suffix.lower() != ".xpm":
        print(f"[WARN] Source file does not have .xpm extension: {source_path.name}")

    # Load step definitions
    custom_steps = False
    if args.steps:
        steps_path = Path(args.steps)
        if not steps_path.exists():
            sys.exit(f"[ERROR] Step definitions file not found: {steps_path}")
        step_defs = load_step_definitions(steps_path)
        custom_steps = True
        print(f"[INFO] Loaded {len(step_defs)} step(s) from {steps_path.name}")
    else:
        step_defs = auto_step_definitions()
        print(f"[INFO] Auto-generating {len(step_defs)} steps from MIDI note zones")

    # Dry run — print plan and exit
    if args.dry_run:
        print(f"\nStep plan for '{args.name}':")
        print(f"  Source: {source_path}")
        print()
        print(f"  {'Step':<6}  {'Label':<22}  {'Enabled Notes':<30}  {'VelLayers':<10}  {'RR'}")
        print(f"  {'-'*6}  {'-'*22}  {'-'*30}  {'-'*10}  {'-'*5}")
        for step in step_defs:
            notes_str = ("all" if step["enabled_notes"] is None
                         else str(sorted(step["enabled_notes"])))
            print(
                f"  {step['step']:<6}  {step['label']:<22}  {notes_str:<30}  "
                f"{step['vel_layers']:<10}  {'yes' if step['round_robin'] else 'no'}"
            )
        print()
        return

    output_dir = Path(args.output)
    build_xpn(
        pack_name=args.name,
        source_xpm_path=source_path,
        output_dir=output_dir,
        step_defs=step_defs,
        custom_steps=custom_steps,
    )


if __name__ == "__main__":
    main()
