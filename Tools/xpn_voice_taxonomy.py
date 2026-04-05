#!/usr/bin/env python3
"""
XPN Voice Taxonomy — Canonical name mapping between engine DSP voices and XPN pad names.
All tools that map voices to pads MUST use this module.
Created 2026-04-04 (QDD Level 4 finding: ONSET taxonomy mismatch).

The mismatch:
  - ONSET engine internal names (used by Python tools):  "chat", "ohat", "fx"
  - C++ XPNDrumExporter.h getPadLayout() names:         "closed_hat", "open_hat", "fx"
  - Result: Python-generated WAV filenames never matched what C++ XPM referenced.

Resolution: this module is the single source of truth. Python tools generate filenames
using engine-internal names (preserving backward compatibility); the C++ exporter
is updated to use the same internal names via the XPN_VOICE_NAMES table.
"""

# ============================================================================
# ONSET engine voice names (engine-internal, used for WAV filename generation)
# These are the names used by OnsetEngine.h and all Python render tools.
# ============================================================================

# All 8 ONSET voice names in canonical order (matches PAD_MAP in xpn_drum_export.py)
ONSET_VOICES = ["kick", "snare", "chat", "ohat", "clap", "tom", "perc", "fx"]

# Maps engine-internal voice name → XPN display label (used in XPM <InstrumentName>)
# The display label is what MPC shows to the user; the filename uses the internal name.
ONSET_VOICE_DISPLAY = {
    "kick":  "KICK",
    "snare": "SNARE",
    "chat":  "CLOSED HAT",   # Engine: "chat"  → display: "CLOSED HAT"
    "ohat":  "OPEN HAT",     # Engine: "ohat"  → display: "OPEN HAT"
    "clap":  "CLAP",
    "tom":   "TOM",
    "perc":  "PERC",
    "fx":    "FX",
}

# Maps engine-internal name → XPN canonical pad name
# XPN canonical names are used in the C++ XPNDrumExporter getPadLayout() PadVoice::name fields.
# The Python tools use internal names; this map converts for any C++ interop.
ONSET_VOICE_MAP = {
    "kick":  "kick",
    "snare": "snare",
    "chat":  "closed_hat",   # Engine uses "chat", C++ XPN pad uses "closed_hat"
    "ohat":  "open_hat",     # Engine uses "ohat", C++ XPN pad uses "open_hat"
    "clap":  "clap",
    "tom":   "tom",
    "perc":  "perc",
    "fx":    "fx",
}

# Reverse map: XPN canonical pad name → engine-internal voice name
ONSET_REVERSE_MAP = {v: k for k, v in ONSET_VOICE_MAP.items()}

# ============================================================================
# Per-voice round robin counts (QDD Ghost Council spec, locked 2026-04-04)
# Uses engine-internal voice names (same as ONSET_VOICES).
# RR count = number of cycle samples needed; 0 = velocity-only (no RR).
# ============================================================================
VOICE_RR_COUNTS = {
    "kick":  0,   # 0 RR if 4+ vel layers (sub-heavy, phase-sensitive)
    "snare": 3,   # Critical: machine-gun effect most audible here
    "chat":  3,   # Critical: 16th-note patterns expose repetition
    "ohat":  4,   # Sustain tails interact, needs most variation
    "clap":  3,   # Transient-heavy, repetition audible
    "tom":   0,   # Phase-sensitive like kick
    "perc":  2,   # Moderate variation needed
    "fx":    3,   # Crash/cymbal equivalent, sustain tails
}

# ============================================================================
# Voice category grouping (for slot budget calculations)
# Uses engine-internal voice names.
# ============================================================================
VOICE_CATEGORIES = {
    "kick":  "bass",
    "snare": "transient",
    "chat":  "cymbal",
    "ohat":  "cymbal",
    "clap":  "transient",
    "tom":   "bass",
    "perc":  "transient",
    "fx":    "cymbal",
}

# ============================================================================
# GM MIDI note assignments for ONSET drum voices
# Uses engine-internal voice names (these are engine-side assignments).
# ============================================================================
ONSET_MIDI_NOTES = {
    "kick":  36,
    "snare": 38,
    "chat":  42,
    "ohat":  46,
    "clap":  39,
    "tom":   41,
    "perc":  43,
    "fx":    49,
}

# ============================================================================
# Public API
# ============================================================================

def canonical_name(engine_voice_name: str, engine: str = "Onset") -> str:
    """Convert engine-internal voice name to XPN canonical pad name.

    Args:
        engine_voice_name: Internal name as used in Python tools (e.g., "chat").
        engine: Engine identifier. Currently only "Onset" is supported.

    Returns:
        XPN canonical name (e.g., "closed_hat"), or the input unchanged if
        no mapping exists.
    """
    if engine == "Onset":
        return ONSET_VOICE_MAP.get(engine_voice_name, engine_voice_name)
    return engine_voice_name


def display_label(engine_voice_name: str, engine: str = "Onset") -> str:
    """Return the MPC display label for a voice (what shows in <InstrumentName>).

    Args:
        engine_voice_name: Internal name as used in Python tools (e.g., "chat").
        engine: Engine identifier.

    Returns:
        Uppercase display label (e.g., "CLOSED HAT").
    """
    if engine == "Onset":
        return ONSET_VOICE_DISPLAY.get(engine_voice_name, engine_voice_name.upper())
    return engine_voice_name.upper()


def rr_count(engine_voice_name: str) -> int:
    """Get the round robin sample count for a voice (by engine-internal name).

    Returns:
        int: Number of cycle/RR samples needed. 0 means velocity-only.
    """
    return VOICE_RR_COUNTS.get(engine_voice_name, 0)


def voice_category(engine_voice_name: str) -> str:
    """Return the category group for a voice (by engine-internal name).

    Returns:
        One of: "bass", "transient", "cymbal".
    """
    return VOICE_CATEGORIES.get(engine_voice_name, "transient")


def compute_slot_budget(voices: list, vel_layers: int = 4) -> int:
    """Compute total XPM slots for a set of voices with RR.

    Args:
        voices: List of engine-internal voice names (e.g., ["kick", "snare", "chat"]).
        vel_layers: Number of velocity layers per voice.

    Returns:
        Total slot count (vel_layers × max(1, rr_count) per voice).
    """
    total = 0
    for v in voices:
        rr = max(1, rr_count(v))  # At least 1 (the base sample)
        total += vel_layers * rr
    return total
