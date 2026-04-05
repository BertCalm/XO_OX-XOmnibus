"""
XPN Velocity Standard — Single Source of Truth
Ghost Council Modified zones, adopted 2026-04-04 (QDD Level 2, 8/8 ghosts).
All velocity zone definitions in the Oxport pipeline MUST import from this module.

Cross-language authority: Tools/xpn-spec.json
  The JSON file is the canonical record shared with the TypeScript xpm-creator UI.
  If zone boundaries or render midpoints change, update xpn-spec.json first, then
  mirror the values here.  This Python module remains the Python API (import target
  for oxport.py and all other Python tools); xpn-spec.json is the bridge to TypeScript.
"""

# Zone boundaries (VelStart, VelEnd) — MPC XPM format values
ZONES = [
    (1, 20),    # Ghost — below pad sensitivity threshold, ghost notes
    (21, 55),   # Light — conversational playing, finger drumming sweet spot
    (56, 90),   # Medium — deliberate hits, where dynamics happen
    (91, 127),  # Hard — power hits, peak force
]

# Zone names (for display and documentation)
ZONE_NAMES = ["Ghost", "Light", "Medium", "Hard"]

# Render midpoints — the MIDI velocity sent during sample rendering
# These are zone center values: (start + end) // 2
RENDER_MIDPOINTS = [10, 38, 73, 109]

# Number of velocity layers
NUM_LAYERS = len(ZONES)

# Legacy compatibility: the old VIBE_VELOCITIES constant
# DEPRECATED — use RENDER_MIDPOINTS instead
VIBE_VELOCITIES = RENDER_MIDPOINTS

def vel_start(layer_index):
    """Get VelStart for a layer (0-indexed)."""
    return ZONES[layer_index][0]

def vel_end(layer_index):
    """Get VelEnd for a layer (0-indexed)."""
    return ZONES[layer_index][1]

def layer_for_velocity(vel):
    """Return the layer index (0-3) for a given MIDI velocity value.

    MIDI velocity 0 is conventionally a note-off event, not a Ghost hit.
    Return 0 (Ghost layer index) rather than Hard so callers can distinguish
    note-off from a genuine soft strike.
    """
    if vel == 0:
        return 0  # MIDI note-off convention — not a Hard hit
    for i, (start, end) in enumerate(ZONES):
        if start <= vel <= end:
            return i
    return NUM_LAYERS - 1  # Default to Hard for vel > 127
