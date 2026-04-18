# XPN Velocity Map Visualizer — R&D Spec

**Date:** 2026-03-16
**Status:** Concept / Pre-implementation
**Owner:** XO_OX Designs
**Depends on:** `Tools/xpn_keygroup_export.py`, `Tools/oxport.py`

---

## Problem Statement

The Vibe Musical Curve algorithm assigns velocity layer boundaries algorithmically:

```
ghost    (1–20)    30% volume — barely touching
light    (21–50)   55% volume — gentle playing
mid      (51–90)   75% volume — expressive sweet spot
hard     (91–127)  95% volume — full force
```

Instrument-family variants (Piano, Strings, Brass, Woodwind, World, Organ) each ship their own
curve with slightly different split points and labels. As pack design scales to 34+ engines and
150-preset fleet targets, pack designers need to answer these questions before committing samples
to hardware testing:

1. Are all 128 velocity slots covered? (Gaps = silent zones; a performer hitting v=45 gets nothing.)
2. Are there overlapping zones? (Overlaps = double-trigger artifacts on MPC hardware.)
3. Does the curve shape *feel* musical for this instrument family? (A ghost layer spanning 1–40 is
   too wide for piano; fine for organ.)
4. Which samples cover which zones? (Confirming sample file names match the intended dynamics.)
5. For drum programs: do per-pad velocity stacks have consistent structure, or is pad 7 missing a
   ghost layer that every other pad has?

Today there is no answer except "export, load on MPC, and play it." That feedback loop is
minutes-to-hours per iteration. A terminal-native visualizer closes the loop to seconds.

---

## Design Goals

| ID | Goal |
|----|------|
| G1 | Show velocity coverage as an ASCII grid in < 1 second |
| G2 | Detect gaps and overlaps automatically — print warnings |
| G3 | Color-code samples by feliX/Oscar polarity using ANSI codes |
| G4 | Support single-layer, multi-layer, and drum (per-pad) programs |
| G5 | Export to ASCII, HTML, and SVG for pack documentation |
| G6 | Integrate as `--visualize-velocity` flag in `oxport.py` post-export stage |
| G7 | Zero new dependencies — pure Python 3.10+ stdlib |

---

## Architecture

### Module Layout

```
Tools/
  xpn_velocity_visualizer.py    # Standalone CLI tool
  oxport.py                     # Integration hook (--visualize-velocity flag)
```

`xpn_velocity_visualizer.py` is the single source of truth. `oxport.py` calls it as a library
import after the export stage completes, so the visualizer always runs against the final XPM/XPN
output, not intermediate data.

### Data Model

The visualizer consumes a parsed XPM program dict (already produced by `xpn_keygroup_export.py`):

```python
@dataclass
class VelocityLayer:
    sample_name: str        # raw filename, will be truncated to 12 chars for display
    note_low: int           # MIDI note 0–127
    note_high: int          # MIDI note 0–127
    vel_low: int            # 0–127
    vel_high: int           # 0–127
    polarity: str           # "felix", "oscar", or "neutral"

@dataclass
class VelocityMap:
    program_name: str
    program_type: str       # "keygroup" | "drum"
    layers: list[VelocityLayer]
    pad_index: int | None   # None for keygroup; 0–15 for drum
```

The `polarity` field is derived by inspecting the sample filename for feliX/Oscar markers
(configurable heuristic — see Implementation Notes).

### Rendering Pipeline

```
parse_xpm(path)
    → list[VelocityMap]

for each VelocityMap:
    build_grid(map)        → 2D array: grid[note][vel] = layer_index | GAP | OVERLAP
    render_ascii(grid)     → list[str]  (with ANSI codes for terminal)
    detect_anomalies(grid) → list[Anomaly]  (gap ranges, overlap ranges)
    emit_warnings(anomalies)
    if --format html: render_html(grid)
    if --format svg:  render_svg(grid)
```

### Grid Dimensions

The terminal grid is scaled to fit an 80-column display by default (configurable via `--width`):

- **X axis:** velocity 0–127, displayed at 2:1 compression → 64 columns
- **Y axis:** MIDI notes, displayed grouped by octave (C0–C10 = 11 rows by default, or one row
  per semitone if `--full-note-range` is set)

For drum programs, the Y axis is replaced by pad index (0–15), one row per pad.

---

## Implementation Notes

### ASCII Grid Rendering

Each cell in the grid is a single character:

| Symbol | Meaning |
|--------|---------|
| `█` (U+2588) | Covered — single layer |
| `░` (U+2591) | Overlap — two or more layers share this slot |
| `.` | Gap — no layer covers this velocity/note slot |
| `·` | Out-of-range (above highest note or below lowest note) |

The first 12 characters of the sample name are printed to the right of the grid as a legend,
aligned to each layer row.

Example output for a 4-layer piano program spanning C3–C5:

```
PIANO PROGRAM — "Grand Noir" (keygroup, 4 layers)
Velocity:  0         32        64        96       127
           |         |         |         |         |
C5  ████████████████░░░░░░░░░░░░████████████████████  grand-pp.wa
C4  ████████████████░░░░░░░░░░░░████████████████████  grand-mp.wa
C3  ████████████████░░░░░░░░░░░░████████████████████  grand-ff.wa

WARNINGS:
  [OVERLAP] vel=20–21 on C3–C5: "grand-pp.wa" and "grand-mp.wa" share 2 velocity slots
  Recommendation: shift grand-mp VelStart from 20 → 22
```

### Color Coding (ANSI)

Color is applied to the block characters, not the background:

| Sample polarity | ANSI code | Display |
|----------------|-----------|---------|
| feliX-biased | `\033[96m` (bright cyan) | Cyan blocks |
| Oscar-biased | `\033[33m` (yellow/warm orange) | Warm orange blocks |
| Neutral | `\033[97m` (bright white) | White blocks |
| Overlap | `\033[91m` (bright red) | Red blocks — always signals a problem |
| Gap | no color (`.` char) | Uncolored |

Color is suppressed automatically when stdout is not a TTY (piped output, CI environments).
Force color with `--color always`; suppress with `--color never`.

### Polarity Detection Heuristic

The visualizer infers feliX/Oscar polarity from sample filenames using a configurable keyword list:

```python
FELIX_KEYWORDS = ["bright", "high", "snap", "attack", "stab", "light", "thin", "airy"]
OSCAR_KEYWORDS = ["warm", "low", "sub", "deep", "thick", "dark", "round", "heavy"]
```

If neither set matches, polarity is `"neutral"`. Pack designers can override via a sidecar JSON:

```json
{
  "polarity_overrides": {
    "grand-pp.wav": "felix",
    "grand-ff.wav": "oscar"
  }
}
```

Sidecar file path defaults to `{xpm_basename}_polarity.json` alongside the XPM.

### Density View

The `--density` flag renders a second pass showing the count of overlapping layers per cell,
rather than just flagging overlap as binary:

```
Density map (0=gap, 1=clean, 2+=overlap bug):
C5  1111111111111111222222222222111111111111111111111111
C4  1111111111111111222222222222111111111111111111111111
```

This helps distinguish "1 slot of accidental overlap" (cosmetic, MPC handles gracefully) from
"20 slots of overlap" (genuine bug that will cause audible volume spikes or double-triggers).

### Gap vs. Overlap Thresholds

Not all gaps are bugs. The visualizer distinguishes:

- **Hard gap** (> 3 velocity slots with no coverage): always a warning
- **Soft gap** (1–3 slots): informational note only
- **Hard overlap** (> 2 slots): always a warning
- **Soft overlap** (1–2 slots): informational note — MPC OS 3.x handles 1-slot overlaps gracefully

Thresholds are configurable via `--gap-threshold N` and `--overlap-threshold N`.

### Drum Programs — Per-Pad View

For XPM programs with `Type="DrumProgram"`, the Y axis is remapped to pad index:

```
DRUM PROGRAM — "Carbon Kit" (drum, 16 pads)
Velocity:  0         32        64        96       127
           |         |         |         |         |
Pad 01  ████████████████████████████████████████████  kick-sub.w
Pad 02  ████████████████░░░░░░░░████████████████████  snare-gh.w
Pad 03  ████████████████████████████████████████████  hat-clos.w
...
Pad 07  ....................................................  [EMPTY PAD]

WARNINGS:
  [GAP] Pad 07 has no velocity layers assigned
  [OVERLAP] Pad 02 vel=45–46: "snare-gh.wav" and "snare-mid.wav" share 2 slots
```

Empty pads are flagged but not treated as errors — pack designers may intentionally leave pads
unassigned.

---

## Export Modes

### `--format ascii` (default)

Prints to stdout. Suitable for terminal review and CI log capture.

### `--format html`

Generates a self-contained HTML file with:
- Inline CSS (no external dependencies)
- Color-coded cells via `<span>` elements
- Hover tooltip on each cell: sample name, exact velocity range, volume scalar
- Collapsible sections per program when an XPN bundle is passed
- Filename: `{program_name}_velocity_map.html`

### `--format svg`

Generates an SVG grid using `<rect>` elements:
- Each cell is a 4x4px rect; full 128-note × 128-velocity grid = 512x512px output
- Color-coded fill matching the ANSI scheme (cyan/orange/white/red)
- `<title>` element per rect for accessibility/tooltip support in browsers
- Suitable for embedding directly in XO_OX pack documentation pages
- Filename: `{program_name}_velocity_map.svg`

---

## Integration Points

### `oxport.py` — `--visualize-velocity` Flag

Add to the Stage 4 (post-export) pipeline in `oxport.py`:

```python
if args.visualize_velocity:
    from xpn_velocity_visualizer import visualize_xpm
    for xpm_path in exported_xpm_paths:
        visualize_xpm(
            xpm_path,
            format=args.visualize_format,  # ascii|html|svg
            color=args.color,
            density=args.visualize_density,
        )
```

CLI addition to `oxport.py`:

```
--visualize-velocity        Run velocity map visualizer after export
--visualize-format FORMAT   ascii (default), html, svg
--visualize-density         Show density (overlap count) pass after main grid
```

### `xpn_keygroup_export.py` — Inline Validation

Add a `validate_velocity_coverage(layers)` call inside `build_keygroup_program()` before writing
the XPM. This is a non-printing audit that returns a list of `Anomaly` objects. If anomalies
exist, emit a one-line terminal warning pointing the designer to `--visualize-velocity` for
details. Do not fail the export — the warning is advisory.

### `xpn_drum_export.py` — Pad Stack Validation

Same pattern: call `validate_drum_pad_stacks(pads)` and emit advisory warnings for empty pads or
per-pad overlap issues.

---

## CLI Interface

```
python Tools/xpn_velocity_visualizer.py <path_to_xpm_or_xpn> [options]

Arguments:
  path              Path to .xpm file (single program) or .xpn bundle (all programs)

Options:
  --format          ascii|html|svg  (default: ascii)
  --color           auto|always|never  (default: auto)
  --density         Show density overlap count view
  --full-note-range Show one row per semitone instead of per-octave grouping
  --width N         Terminal grid width in columns (default: 80)
  --gap-threshold N Hard gap warning threshold in velocity slots (default: 3)
  --overlap-threshold N  Hard overlap warning threshold (default: 2)
  --polarity-file   Path to polarity override JSON (default: auto-detect sidecar)
  --pad N           Drum programs only: show only pad N (0–15)
  --quiet           Suppress warnings, print grid only
  --summary         Print anomaly summary only, no grid
```

---

## Open Questions

**OQ1 — Polarity keyword list ownership**
The feliX/Oscar keyword heuristic is a best-effort guess. Should the canonical list live in
`xpn_velocity_visualizer.py`, or should it be read from a shared config file (`Tools/xo_ox_config.json`)
so all tools use the same definitions? Recommendation: shared config; defer to next tool sprint.

**OQ2 — XPN bundle support priority**
An `.xpn` file is a ZIP archive containing multiple XPM programs. Supporting XPN input means
iterating all XPMs in the archive and rendering one grid per program (or a single scrollable
multi-program view). Is per-program rendering sufficient, or do pack designers need a fleet view
showing all 20 programs side-by-side? The latter requires a wider terminal or paginated output.

**OQ3 — Velocity curves for concept engines**
OSTINATO, OPENSKY, OCEANDEEP, and OUIE have not yet defined instrument-family velocity curves.
The visualizer should not hard-fail on unknown families — fall back to the Vibe Musical Curve
baseline and log a `[FAMILY_UNKNOWN]` advisory. Once those engines define their curves, add
them to the `VEL_LAYERS_*` table in `xpn_keygroup_export.py`.

**OQ4 — CI integration**
Should a `--strict` flag cause the tool to exit with code 1 on hard gaps or hard overlaps, enabling
use in a pre-commit or GitHub Actions check? This would gate XPN exports in automated pipelines.
Low friction implementation; recommended for the first release of the tool.

**OQ5 — SVG color palette for print**
The ANSI cyan/orange palette is designed for dark terminal backgrounds. The SVG export targets
pack documentation pages which may be light or dark. Should SVG export default to the Gallery
Model palette (warm white shell `#F8F6F3` background, engine accent colors for layers) rather
than the ANSI colors? Likely yes — decouple SVG color scheme from terminal color scheme.

**OQ6 — Interaction with `xpn_adaptive_velocity.py`**
`xpn_adaptive_velocity.py` can modify velocity boundaries post-export. If a designer runs the
visualizer on an XPM that was modified by the adaptive velocity tool, the visualization correctly
reflects the final state. No action needed — this is a feature, not a risk.

---

## Implementation Priority

| Phase | Scope | Effort |
|-------|-------|--------|
| P1 | ASCII renderer + gap/overlap detection + CLI | 1–2 days |
| P2 | ANSI color coding + drum pad view | 0.5 days |
| P3 | `oxport.py` `--visualize-velocity` integration | 0.5 days |
| P4 | HTML export | 1 day |
| P5 | SVG export | 1 day |
| P6 | XPN bundle support (multi-program) | 1 day |

Total estimated effort: 5–6 days of focused Python work. P1–P3 deliver 90% of the value.

---

*Spec authored 2026-03-16. Supersedes no prior spec. Companion tools: `xpn_keygroup_export.py`,
`xpn_drum_export.py`, `oxport.py`. Next action: implement P1 once XPN fleet export sprint resumes.*
