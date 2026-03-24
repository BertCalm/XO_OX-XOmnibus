# Coupling DNA & XPN Pack Design — R&D

**Date:** 2026-03-16
**Scope:** How XOlokun cross-engine coupling should inform XPN pack design, documentation, and future re-render workflows.

---

## 1. Coupling Types Cheat Sheet

The 12 canonical `CouplingType` values from `Source/Core/SynthEngine.h`, with sonic character and XPN pack context for each.

| # | Enum | Sonic Effect | Best XPN Pack Context |
|---|------|-------------|----------------------|
| 1 | `AmpToFilter` | Source amplitude modulates target filter cutoff — the sidechain pump. Louder source = brighter or darker target. | Drum-driven packs: ONSET kick opening a pad filter. Essential for any kit where drums and melodic engines share a slot. |
| 2 | `AmpToPitch` | Source amplitude bends target pitch upward — louder hits push the note sharp, creating a "bloom" effect. | Percussive melodic packs: velocity-sensitive pitch expression without MPE hardware. |
| 3 | `LFOToPitch` | Source LFO sweeps target pitch — synchronized vibrato, detuned drift, or slow wobble locked to source rate. | Atmosphere and texture packs where engines breathe together at a shared tempo. |
| 4 | `EnvToMorph` | Source envelope drives target wavetable/morph position — transient energy opens new timbral territory in the target. | Evolving pad packs: a pluck's envelope forces a granular engine through its morph axis on each hit. |
| 5 | `AudioToFM` | Source audio feeds directly into target FM input — source timbres are baked into the target's carrier frequency. Most aggressive coupling type. | Experimental / Entangled packs: OBSCURA's spring chain into ORGANON's metabolism; OUROBOROS chaos into ORBITAL harmonics. |
| 6 | `AudioToRing` | Source audio ring-modulates target audio — sum and difference frequencies from both engines, destroying both original tones into metallic hybrids. | Industrial / harsh texture packs. Only ORBITAL fully supports this as a target. |
| 7 | `FilterToFilter` | Source filter output feeds target filter input — one engine's EQ curve shapes another's spectral character before that engine's own filter applies. | Spectral layering packs. Currently: OPTIC receives this as an analysis source. Rare in the fleet. |
| 8 | `AmpToChoke` | Source amplitude silences (chokes) target — drum machine open/close hi-hat logic generalized to any engine pair. | Rhythm packs: ONSET voice chokes SNAP or BOB to enforce drum machine articulation rules. |
| 9 | `RhythmToBlend` | Source rhythmic pattern modulates target blend parameter — rhythmic pulsing shapes texture density, granular rate, or morph crossfade. | Groove packs: ONSET's kick/snare pattern drives OPAL's grain density or OBLIQUE's prism blend. |
| 10 | `EnvToDecay` | Source envelope modulates target decay time — longer source sustain stretches target tail, shorter source tightens it. | Natural/organic packs where room size or decay feel breathes with the performance. |
| 11 | `PitchToPitch` | Source pitch retunes target pitch — harmonic tracking, unison stacking, or parallel interval relationships. | Polyphonic packs where engines should stay harmonically locked without programming duplicate notes. |
| 12 | `AudioToWavetable` | Source audio replaces or modulates target wavetable source — target engine synthesizes from the source's spectral content. Requires full audio buffer transfer. | Cross-engine granular packs: ORBITAL harmonics atomized by OPAL; OSPREY excitation fed to OSTERIA's room model. |

---

## 2. XPN Coupling Metadata — `<CouplingDNA>` Block

When a preset is rendered to XPN, the live modulation relationship between engines is frozen into audio. The coupling state that produced the sound vanishes unless explicitly captured.

**Proposal: embed a `<CouplingDNA>` XML comment block inside each `.xpm` file.**

MPC software ignores XML comments — the comment is invisible to playback but preserved as documentation in the file. Any text editor, Python tool, or future re-render pipeline can extract it.

### Schema

```xml
<!-- CouplingDNA
  <CouplingDNA version="1.0" preset="Beat Drives Fat" engine_count="2">
    <Route
      source_engine="ONSET"
      target_engine="OBESE"
      coupling_type="AmpToFilter"
      coupling_amount="0.72"
      coupling_polarity="positive"
      normalled="false"
      note="Kick peak opens fat synth filter cutoff. Rendered at 120 BPM, C3 root."
    />
    <Route
      source_engine="ONSET"
      target_engine="OBESE"
      coupling_type="RhythmToBlend"
      coupling_amount="0.35"
      coupling_polarity="positive"
      normalled="false"
      note="Hat pattern drives character blend. Velocity curve: musical/Vibe."
    />
    <RenderConditions
      bpm="120"
      root_note="C3"
      xolokun_version="1.0.0"
      render_date="2026-03-16"
    />
  </CouplingDNA>
-->
```

**Field definitions:**

| Field | Type | Description |
|-------|------|-------------|
| `source_engine` | string | Canonical engine ID (e.g., `ONSET`, `OUROBOROS`) |
| `target_engine` | string | Canonical engine ID |
| `coupling_type` | string | Exact `CouplingType` enum name from `SynthEngine.h` |
| `coupling_amount` | float 0–1 | Normalized amount as stored in `CouplingRoute.amount` |
| `coupling_polarity` | `positive` / `negative` | Whether the route is inverted (ducking vs. pumping) |
| `normalled` | bool | `true` = default route from `isNormalled`; `false` = user-defined |
| `note` | string | Human-readable sound design note — tempo, root, intent |

The `<RenderConditions>` block captures the playback context at export time: BPM and root note affect how coupling sounds in time-domain types (`AmpToFilter` pump timing, `RhythmToBlend` phase), so future re-renders need this data.

**Placement:** Insert the comment block immediately after the opening `<MPCVObject>` tag in the `.xpm`, before the first `<ProgramVersion>` element.

---

## 3. Pack Series Based on Coupling

### Series A: Entangled Series

**Concept:** Same source samples rendered three times — dry, light coupling, heavy coupling. The series teaches producers what coupling actually does to a sound.

Each pack in the series is a triplet:

| Variant | Pack Name Convention | Coupling State |
|---------|---------------------|----------------|
| Dry | `[Engine] Naked` | No coupling routes active. Pure engine output. |
| Light | `[Engine] Tethered` | 1–2 routes, amounts ≤ 0.4. Subtle modulation. |
| Heavy | `[Engine] Entangled` | 2–4 routes, amounts 0.6–1.0. Transformative coupling. |

**Example triplet — OUROBOROS × ORGANON:**
- `Organon Naked` — 64 grain textures, no coupling, engine's intrinsic sound
- `Organon Tethered` — same 64 samples, OUROBOROS `AudioToFM` at 0.3, light chaos injection
- `Organon Entangled` — same 64 samples, OUROBOROS `AudioToFM` at 0.85 + `LFOToPitch` at 0.6, attractor-driven harmonic metabolism

The `<CouplingDNA>` block in each XPM documents exactly what changed between variants. Producers can compare the three XPMs side by side to understand the coupling's contribution.

**Pack count:** 3 packs per engine pair. Recommend 6–8 triplets at launch covering the highest-musical-value pairs from the coupling audit: ONSET×OBESE, OUROBOROS×ORGANON, ORBITAL×OPAL, OSPREY×OSTERIA, OPTIC×OBLIQUE, ONSET×SNAP.

---

### Series B: Cross-Engine Pairs

**Concept:** Stem-style packs where Engine A and Engine B are released as a matched pair. The stems from each engine are documented with their coupling routes, so producers can layer them in MPC knowing which sonic relationships are "intended."

**Structure:**
- Pack A: `[Engine A] — [Pair Name] Side A` — 32 pads, Engine A stems with coupling OUT documented
- Pack B: `[Engine B] — [Pair Name] Side B` — 32 pads, Engine B stems with coupling IN documented

Each pad's XPM contains a `<CouplingDNA>` block describing which routes from Side A are feeding it. A producer who loads both packs into adjacent programs can reconstruct the full coupling relationship in XOlokun from the documentation alone.

**Inaugural pair: OSPREY × OSTERIA** — "The Shore & The Tavern" (existing ShoreSystem narrative already in Docs). Osprey shore waves feed the tavern room model via `AudioToFM`; Osprey's sea state drives Osteria's elastic tightness via `AmpToFilter`.

---

### Series C: Coupling Showcase

**Concept:** One pack per coupling type. 8 pads each demonstrating that coupling type applied to the same base sample — a reference collection that functions as an interactive manual.

**Structure per pack:** Take a single OPAL granular chord. Render it 8 times with 8 different source engines driving the same coupling type at a standard amount (0.65) and a standard tempo (120 BPM). 8 pads = 8 source engines. The producer hears exactly how ONSET, OUROBOROS, ORBITAL, OBLIQUE, OBSIDIAN, OCEANIC, ORIGAMI, and OSPREY each express `AmpToFilter` differently against the same target.

**12 packs, one per type:**

| Pack | Coupling Type | Base Target | 8 Source Engines |
|------|--------------|-------------|-----------------|
| Showcase 01 | `AmpToFilter` | OPAL chord | ONSET, OUROBOROS, ORBITAL, OBLIQUE, OBSIDIAN, OCEANIC, ORIGAMI, OSPREY |
| Showcase 02 | `AmpToPitch` | OBLONG melody | ONSET, ORBITAL, OUROBOROS, OCEANIC, OBSIDIAN, ORIGAMI, ORACLE, OSTERIA |
| Showcase 03 | `LFOToPitch` | ODYSSEY pad | OHM, ORPHICA, ORBITAL, OUROBOROS, OBLIQUE, OBBLIGATO, OTTONI, OPTIC |
| Showcase 04 | `EnvToMorph` | OPAL texture | ONSET, ORBITAL, OUROBOROS, OVERWORLD, OBLIQUE, ORACLE, ORIGAMI, OSPREY |
| Showcase 05 | `AudioToFM` | ORGANON bass | OUROBOROS, OBSCURA, ORBITAL, OSPREY, OSTERIA, OPTIC, OCEANIC, ORIGAMI |
| Showcase 06 | `AudioToRing` | ORBITAL chord | OUROBOROS, OBSCURA, OSPREY, OPTIC, OCEANIC, ORIGAMI, OBSIDIAN, ORACLE |
| Showcase 07 | `FilterToFilter` | OPTIC analysis | OBLIQUE, ORBITAL, OSPREY, OSTERIA, OBSIDIAN, ORIGAMI, ORACLE, OCEANIC |
| Showcase 08 | `AmpToChoke` | SNAP hit | ONSET (V1 kick), ONSET (V3 snare), ONSET (V5 open hat), ONSET (V6 closed hat), ONSET (V7 clap), ORBITAL, OUROBOROS, OBSCURA |
| Showcase 09 | `RhythmToBlend` | OPAL grain | ONSET, OBLIQUE, ORBITAL, OUROBOROS, OCEANIC, OBSIDIAN, ORIGAMI, ORACLE |
| Showcase 10 | `EnvToDecay` | OVERDUB dub | ONSET, ORBITAL, OUROBOROS, OBLIQUE, OBSIDIAN, OCEANIC, ORIGAMI, ORACLE |
| Showcase 11 | `PitchToPitch` | OBLONG melody | OUROBOROS, ORBITAL, ODYSSEY, OBLIQUE, ORACLE, ORIGAMI, OSPREY, OHM |
| Showcase 12 | `AudioToWavetable` | ORGANON voice | OUROBOROS, ORBITAL, OSPREY, OSTERIA, OBSCURA, OPTIC, OCEANIC, ORIGAMI |

---

## 4. Coupling as Velocity Dimension

**Can coupling amount map to velocity in an XPN pack?**

Short answer: yes, but only through separate renders — MPC has no runtime coupling concept.

**How it works:**

A velocity-stratified coupling pack requires N separate renders of the same preset at N coupling amounts, each assigned to a velocity layer in the XPM:

| Velocity Range | Coupling Amount Rendered | Subjective Feel |
|---------------|------------------------|----------------|
| 1–40 | 0.0 (dry / uncoupled) | Natural, unaffected |
| 41–80 | 0.35 (light coupling) | Subtle pump or drift |
| 81–110 | 0.65 (medium coupling) | Noticeable modulation |
| 111–127 | 0.95 (heavy coupling) | Transformative, saturated coupling |

**Implementation requirements:**

1. The preset must be rendered 4 times (or more) with coupling amount swept between runs. The XPN export pipeline already supports per-layer sample assignment; the gap is automating the coupling-amount parameter between render passes.

2. The `<CouplingDNA>` block for a velocity-stratified XPM would include a `velocity_map` attribute on each `<Route>`:

```xml
<Route
  source_engine="ONSET"
  target_engine="OBESE"
  coupling_type="AmpToFilter"
  coupling_amount="velocity_mapped"
  velocity_map="0:1-40, 0.35:41-80, 0.65:81-110, 0.95:111-127"
  coupling_polarity="positive"
/>
```

3. **Limitation:** This captures amplitude-domain coupling (pump, bloom, choke) well. Frequency-domain types (`AudioToFM`, `AudioToWavetable`) sound fundamentally different at different amounts — not just "more/less of the same thing." For these, velocity-stratification produces 4 genuinely distinct instruments on one pad, which is musically valid but not "more coupling = louder velocity."

4. **Practical ceiling:** 4 render passes per pad = 4× storage cost per pack. Recommend reserving velocity-stratified coupling for flagship showcase packs (Series C) where the educational value justifies the overhead. Standard Series A/B packs render one coupling state per sample.

---

## 5. Sonnet-Ready Deliverable: `xpn_coupling_docs_generator.py`

Reads a `.xometa` preset file, extracts all active coupling routes, and generates the `<CouplingDNA>` comment block ready to embed in a `.xpm`.

```python
#!/usr/bin/env python3
"""
xpn_coupling_docs_generator.py — XO_OX Designs
Reads a .xometa preset and emits a <CouplingDNA> XML comment block
for embedding in the corresponding .xpm file.

Usage:
    python xpn_coupling_docs_generator.py Beat_Drives_Fat.xometa
    python xpn_coupling_docs_generator.py Beat_Drives_Fat.xometa --bpm 120 --root C3
    python xpn_coupling_docs_generator.py Beat_Drives_Fat.xometa --inject output.xpm
"""

import argparse
import json
import re
import sys
from datetime import date
from pathlib import Path

XOLOKUN_VERSION = "1.0.0"

POLARITY_LABELS = {
    True:  "negative",  # inverted (ducking) routes
    False: "positive",
}


def load_preset(path: Path) -> dict:
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def extract_routes(preset: dict) -> list[dict]:
    """Return coupling routes from a .xometa preset dict."""
    return preset.get("couplingRoutes", [])


def build_coupling_dna_xml(preset: dict, bpm: int, root_note: str) -> str:
    routes = extract_routes(preset)
    preset_name = preset.get("name", "Unknown Preset")
    engines = preset.get("engines", [])
    engine_count = len([e for e in engines if e])

    lines = [
        "<!-- CouplingDNA",
        f'  <CouplingDNA version="1.0" preset="{preset_name}" engine_count="{engine_count}">',
    ]

    if not routes:
        lines.append("    <!-- No coupling routes active in this preset -->")
    else:
        for r in routes:
            src  = r.get("sourceEngine", r.get("sourceSlot", "?"))
            dest = r.get("destEngine",   r.get("destSlot",   "?"))
            ctype   = r.get("type",     "Unknown")
            amount  = r.get("amount",   0.0)
            inverted = r.get("inverted", False)
            normalled = r.get("isNormalled", False)
            note = r.get("note", "")

            lines.append(
                f'    <Route\n'
                f'      source_engine="{src}"\n'
                f'      target_engine="{dest}"\n'
                f'      coupling_type="{ctype}"\n'
                f'      coupling_amount="{amount:.2f}"\n'
                f'      coupling_polarity="{POLARITY_LABELS[inverted]}"\n'
                f'      normalled="{str(normalled).lower()}"\n'
                f'      note="{note}"\n'
                f'    />'
            )

    lines += [
        f'    <RenderConditions',
        f'      bpm="{bpm}"',
        f'      root_note="{root_note}"',
        f'      xolokun_version="{XOLOKUN_VERSION}"',
        f'      render_date="{date.today().isoformat()}"',
        f'    />',
        "  </CouplingDNA>",
        "-->",
    ]
    return "\n".join(lines)


def inject_into_xpm(xpm_path: Path, dna_xml: str) -> None:
    """Insert CouplingDNA comment after opening <MPCVObject> tag."""
    text = xpm_path.read_text(encoding="utf-8")
    marker = "<MPCVObject>"
    if marker not in text:
        print(f"Warning: {marker} not found in {xpm_path}. Appending to top.", file=sys.stderr)
        text = dna_xml + "\n" + text
    else:
        text = text.replace(marker, marker + "\n" + dna_xml, 1)
    xpm_path.write_text(text, encoding="utf-8")
    print(f"Injected CouplingDNA into {xpm_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate <CouplingDNA> block from .xometa preset")
    parser.add_argument("preset", type=Path, help=".xometa file path")
    parser.add_argument("--bpm",    type=int, default=120,  help="Render BPM (default 120)")
    parser.add_argument("--root",   type=str, default="C3", help="Root note (default C3)")
    parser.add_argument("--inject", type=Path, default=None, help=".xpm to inject block into")
    args = parser.parse_args()

    if not args.preset.exists():
        sys.exit(f"Error: {args.preset} not found")

    preset = load_preset(args.preset)
    dna_xml = build_coupling_dna_xml(preset, bpm=args.bpm, root_note=args.root)

    if args.inject:
        inject_into_xpm(args.inject, dna_xml)
    else:
        print(dna_xml)


if __name__ == "__main__":
    main()
```

**Usage examples:**

```bash
# Print CouplingDNA block to stdout
python xpn_coupling_docs_generator.py Presets/XOlokun/Entangled/Glass\ Shard\ Coupling.xometa

# Print with render context
python xpn_coupling_docs_generator.py Glass_Shard_Coupling.xometa --bpm 94 --root D2

# Inject directly into exported XPM
python xpn_coupling_docs_generator.py Glass_Shard_Coupling.xometa \
    --inject ./XPN_Export/Glass_Shard_Coupling/Glass_Shard_Coupling.xpm
```

**Integration point:** Call this from `xpn_packager.py` immediately after XPM generation, before ZIP packaging. The `couplingRoutes` key in `.xometa` must be populated by `PresetManager.h` on save — verify the field is present in presets exported from XOlokun before running at scale.

---

## Summary

The coupling system is XOlokun's deepest differentiator — sounds that cannot exist in any single-engine synthesizer. XPN export freezes that live modulation into audio, but the `<CouplingDNA>` block preserves the knowledge of what produced the sound. The three pack series (Entangled, Cross-Engine Pairs, Coupling Showcase) turn coupling knowledge into a curriculum: producers learn by listening to what coupling does, comparing dry against entangled, reading the embedded documentation, and eventually returning to XOlokun to reconstruct and remix the coupling state themselves. The velocity-stratified coupling dimension extends this into the pad itself, where soft touches play the uncoupled engine and hard strikes deliver the fully entangled version.
