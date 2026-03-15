#!/usr/bin/env python3
"""
XPN Bundle Builder — XO_OX Designs
Flexible multi-engine expansion pack builder with 3 bundling modes.

Modes:
  1. custom     — Pick-and-choose individual presets from any engine/mood
  2. category   — Filter by mood, tag, genre, engine, coupling
  3. engine     — All presets for one engine (wraps per-engine behavior)

Bundle profiles are JSON files for reproducible builds.

Usage:
    # Build from a bundle profile JSON
    python3 xpn_bundle_builder.py build --profile hip_hop_pack.json \\
        --wavs-dir /path/to/wavs --output-dir /path/to/out

    # Build by category filter (all Foundation presets)
    python3 xpn_bundle_builder.py category --mood Foundation \\
        --pack-name "Foundation Collection" --output-dir /path/to/out

    # Build by tag
    python3 xpn_bundle_builder.py category --tags "808,sub,bass" \\
        --pack-name "808 Bass Pack" --output-dir /path/to/out

    # List all available presets
    python3 xpn_bundle_builder.py list

    # List by filter
    python3 xpn_bundle_builder.py list --engine Onset --mood Foundation

    # Save a custom bundle profile interactively
    python3 xpn_bundle_builder.py save-profile --name "My Pack" \\
        --pick "808 Reborn,Boom Bap OG,Trap Bounce" --output my_pack.json

Preset Index Format (auto-generated from .xometa files):
    {
        "name": "808 Reborn",
        "engine": "Onset",
        "mood": "Foundation",
        "tags": ["808", "circuit", "classic"],
        "coupling": "None",
        "dna": {...},
        "path": "Foundation/Onset/808_Reborn.xometa"
    }

Bundle Profile Format:
    {
        "name": "Hip Hop Collection",
        "description": "808s, boom bap, trap essentials",
        "cover_engine": "ONSET",
        "version": "1.0",
        "pack_id": "com.xo-ox.bundle.hiphop",
        "presets": [
            {"engine": "Onset", "name": "808 Reborn"},
            {"engine": "Onset", "name": "Boom Bap OG"},
            {"engine": "Obese",   "name": "Deep Sub Bass"},
            ...
        ]
    }
"""

import argparse
import json
import os
import shutil
import sys
from pathlib import Path
from typing import List, Optional

try:
    from xpn_cover_art import generate_cover
    COVER_ART_AVAILABLE = True
except ImportError:
    COVER_ART_AVAILABLE = False

try:
    from xpn_drum_export import generate_xpm, generate_expansion_xml, build_wav_map, PAD_MAP
    DRUM_EXPORT_AVAILABLE = True
except ImportError:
    DRUM_EXPORT_AVAILABLE = False

try:
    from xpn_keygroup_export import build_keygroup_wav_map, generate_keygroup_xpm
    KEYGROUP_EXPORT_AVAILABLE = True
except ImportError:
    KEYGROUP_EXPORT_AVAILABLE = False

try:
    from xpn_packager import package_xpn, XPNMetadata
    PACKAGER_AVAILABLE = True
except ImportError:
    PACKAGER_AVAILABLE = False

REPO_ROOT  = Path(__file__).parent.parent
PRESETS_DIR = REPO_ROOT / "Presets" / "XOmnibus"
PROFILES_DIR = REPO_ROOT / "Tools" / "bundle_profiles"

# Engines that produce drum programs (vs keygroup programs)
DRUM_ENGINES = {"onset", "Onset", "ONSET"}

# XO_OX engine colors for mixed-engine pack cover
ENGINE_ORDER = ["ONSET", "OVERWORLD", "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE", "OPAL",
                "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE"]

# Canonical engine name aliases — maps legacy/long names to XOmnibus short names
ENGINE_ALIASES: dict[str, str] = {
    "xoblongbob":  "Oblong",
    "bob":         "Oblong",
    "xodyssey":    "Odyssey",
    "drift":       "Odyssey",
    "xoverdub":    "Overdub",
    "dub":         "Overdub",
    "xobese":      "Obese",
    "fat":         "Obese",
    "xoddcouple":  "OddOscar",  # multi-engine — keep as-is for display, alias to first-listed
    "snap":        "OddfeliX",
    "morph":       "OddOscar",
    "onset":       "Onset",
    "overworld":   "Overworld",
    "opal":        "Opal",
    "ohm":         "OHM",
    "Ohm":         "OHM",
    "orphica":     "ORPHICA",
    "Orphica":     "ORPHICA",
    "obbligato":   "OBBLIGATO",
    "Obbligato":   "OBBLIGATO",
    "ottoni":      "OTTONI",
    "Ottoni":      "OTTONI",
    "ole":         "OLE",
    "Ole":         "OLE",
    "OLE":         "OLE",
}

def normalize_engine(name: str) -> str:
    """Return canonical XOmnibus engine short name."""
    return ENGINE_ALIASES.get(name.lower(), name)


# =============================================================================
# PRESET INDEX
# =============================================================================

class PresetIndex:
    """Scans all .xometa files and builds a searchable index."""

    def __init__(self, presets_dir: Path = PRESETS_DIR):
        self.presets_dir = presets_dir
        self._index: List[dict] = []
        self._load()

    def _load(self):
        self._index = []
        for mood_dir in sorted(self.presets_dir.iterdir()):
            if not mood_dir.is_dir():
                continue

            # Flat presets directly in mood dir (older multi-engine schema: engines=[...])
            for xmeta in sorted(mood_dir.glob("*.xometa")):
                try:
                    with open(xmeta) as f:
                        data = json.load(f)
                    engines = data.get("engines", [])
                    engine_name = engines[0] if engines else "Unknown"
                    rel_path = xmeta.relative_to(self.presets_dir)
                    self._index.append({
                        "name":     data.get("name", xmeta.stem),
                        "engine":   engine_name,
                        "engines":  engines,
                        "mood":     data.get("mood", mood_dir.name),
                        "tags":     data.get("tags", []),
                        "coupling": data.get("couplingIntensity", "None"),
                        "dna":      data.get("dna", {}),
                        "description": data.get("description", ""),
                        "path":     str(rel_path),
                        "abs_path": str(xmeta),
                    })
                except Exception as e:
                    print(f"  [WARN] Could not load {xmeta.name}: {e}")

            # Subfolder presets (newer single-engine schema: engine inferred from dir)
            for engine_dir in sorted(mood_dir.iterdir()):
                if not engine_dir.is_dir():
                    continue
                engine_name = engine_dir.name
                for xmeta in sorted(engine_dir.glob("*.xometa")):
                    try:
                        with open(xmeta) as f:
                            data = json.load(f)
                        rel_path = xmeta.relative_to(self.presets_dir)
                        self._index.append({
                            "name":     data.get("name", xmeta.stem),
                            "engine":   engine_name,
                            "engines":  [engine_name],
                            "mood":     data.get("mood", mood_dir.name),
                            "tags":     data.get("tags", []),
                            "coupling": data.get("couplingIntensity", "None"),
                            "dna":      data.get("dna", {}),
                            "description": data.get("description", ""),
                            "path":     str(rel_path),
                            "abs_path": str(xmeta),
                        })
                    except Exception as e:
                        print(f"  [WARN] Could not load {xmeta.name}: {e}")

    def all(self) -> List[dict]:
        return list(self._index)

    def search(
        self,
        engine: Optional[str] = None,
        mood: Optional[str] = None,
        tags: Optional[List[str]] = None,
        coupling: Optional[str] = None,
        preset_names: Optional[List[str]] = None,
        min_brightness: Optional[float] = None,
        min_aggression: Optional[float] = None,
    ) -> List[dict]:
        results = self._index

        if engine:
            canonical = normalize_engine(engine).lower()
            results = [p for p in results if normalize_engine(p["engine"]).lower() == canonical]

        if mood:
            mood_lower = mood.lower()
            results = [p for p in results if p["mood"].lower() == mood_lower]

        if tags:
            tag_set = {t.lower().strip() for t in tags}
            results = [p for p in results
                       if tag_set & {t.lower() for t in p["tags"]}]

        if coupling and coupling.lower() != "any":
            coup_lower = coupling.lower()
            results = [p for p in results
                       if p["coupling"].lower() == coup_lower]

        if preset_names:
            name_set = {n.lower() for n in preset_names}
            results = [p for p in results if p["name"].lower() in name_set]

        if min_brightness is not None:
            results = [p for p in results
                       if p.get("dna", {}).get("brightness", 0) >= min_brightness]

        if min_aggression is not None:
            results = [p for p in results
                       if p.get("dna", {}).get("aggression", 0) >= min_aggression]

        return results

    def by_name(self, name: str) -> Optional[dict]:
        name_lower = name.lower().strip()
        for p in self._index:
            if p["name"].lower() == name_lower:
                return p
        return None

    def engines(self) -> List[str]:
        """Return canonical (normalized) engine names."""
        return sorted({normalize_engine(p["engine"]) for p in self._index})

    def moods(self) -> List[str]:
        return sorted({p["mood"] for p in self._index})

    def tags(self) -> List[str]:
        all_tags = set()
        for p in self._index:
            all_tags.update(p["tags"])
        return sorted(all_tags)

    def summary(self) -> str:
        lines = [f"Preset Index: {len(self._index)} total presets"]
        for engine in self.engines():
            count = len(self.search(engine=engine))
            lines.append(f"  {engine:12s}: {count}")
        lines.append(f"\nMoods: {', '.join(self.moods())}")
        lines.append(f"Top tags: {', '.join(self.tags()[:20])}")
        return "\n".join(lines)


# =============================================================================
# BUNDLE SPEC
# =============================================================================

class BundleSpec:
    """Describes what goes into an XPN pack."""

    def __init__(self, name: str, description: str = "",
                 cover_engine: str = "ONSET", version: str = "1.0",
                 pack_id: str = None):
        self.name = name
        self.description = description
        self.cover_engine = cover_engine
        self.version = version
        self.pack_id = pack_id or f"com.xo-ox.bundle.{name.lower().replace(' ', '-')}"
        self.presets: List[dict] = []  # list of index entries

    def add_preset(self, preset_entry: dict):
        if preset_entry not in self.presets:
            self.presets.append(preset_entry)

    def add_presets(self, preset_list: List[dict]):
        for p in preset_list:
            self.add_preset(p)

    @property
    def drum_presets(self):
        return [p for p in self.presets if p["engine"] in DRUM_ENGINES]

    @property
    def keygroup_presets(self):
        return [p for p in self.presets if p["engine"] not in DRUM_ENGINES]

    def to_profile(self) -> dict:
        return {
            "name":         self.name,
            "description":  self.description,
            "cover_engine": self.cover_engine,
            "version":      self.version,
            "pack_id":      self.pack_id,
            "presets": [
                {"engine": p["engine"], "name": p["name"]}
                for p in self.presets
            ],
        }

    @classmethod
    def from_profile(cls, profile: dict, index: PresetIndex) -> "BundleSpec":
        spec = cls(
            name=profile["name"],
            description=profile.get("description", ""),
            cover_engine=profile.get("cover_engine", "ONSET"),
            version=profile.get("version", "1.0"),
            pack_id=profile.get("pack_id"),
        )
        not_found = []
        for ref in profile.get("presets", []):
            entry = index.by_name(ref["name"])
            if entry:
                spec.add_preset(entry)
            else:
                not_found.append(ref["name"])
        if not_found:
            print(f"  [WARN] {len(not_found)} presets not found in index:")
            for n in not_found:
                print(f"    '{n}'")
        return spec

    def print_summary(self):
        print(f"\nBundle: {self.name}")
        print(f"  {len(self.presets)} presets total")
        print(f"  Drum programs: {len(self.drum_presets)}")
        print(f"  Keygroup programs: {len(self.keygroup_presets)}")
        if self.presets:
            engines = {}
            for p in self.presets:
                engines[p["engine"]] = engines.get(p["engine"], 0) + 1
            for eng, cnt in sorted(engines.items()):
                print(f"    {eng}: {cnt}")


# =============================================================================
# XPM STUBS FOR NON-ONSET ENGINES
# =============================================================================

def generate_keygroup_xpm_stub(preset_name: str, engine: str,
                                wav_map: dict) -> str:
    """
    Generate a Keygroup XPM for non-ONSET engines.
    Delegates to xpn_keygroup_export.generate_keygroup_xpm when available.
    Falls back to a minimal valid (but silent) XPM if the exporter is missing.
    """
    if KEYGROUP_EXPORT_AVAILABLE:
        return generate_keygroup_xpm(preset_name, engine, wav_map)

    # Fallback: minimal valid XPM — MPC can load it but no audio until WAVs added
    prog_name = f"XO_OX-{engine}-{preset_name}"[:32]
    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<MPCVObject>\n'
        '  <Version>\n'
        '    <File_Version>1.7</File_Version>\n'
        '    <Application>MPC-V</Application>\n'
        '    <Application_Version>2.10.0.0</Application_Version>\n'
        '    <Platform>OSX</Platform>\n'
        '  </Version>\n'
        '  <Program type="Keygroup">\n'
        f'    <Name>{prog_name}</Name>\n'
        '    <KeygroupNumKeygroups>1</KeygroupNumKeygroups>\n'
        '    <Instruments>\n'
        '      <Instrument number="0">\n'
        '        <LowNote>0</LowNote>\n'
        '        <HighNote>127</HighNote>\n'
        '        <Layers>\n'
        '          <Layer number="1">\n'
        '            <SampleName></SampleName>\n'
        '            <SampleFile></SampleFile>\n'
        '            <VelStart>0</VelStart>\n'
        '            <VelEnd>0</VelEnd>\n'
        '          </Layer>\n'
        '        </Layers>\n'
        '      </Instrument>\n'
        '    </Instruments>\n'
        '  </Program>\n'
        '</MPCVObject>\n'
    )


# =============================================================================
# PACK BUILDER
# =============================================================================

def build_bundle(spec: BundleSpec, wavs_dir: Optional[Path],
                 output_dir: Path, dry_run: bool = False) -> dict:
    """
    Build a complete XPN bundle from a BundleSpec.

    output_dir/
      {bundle_slug}/
        Expansion.xml
        artwork.png
        artwork_2000.png
        bundle_manifest.json     (preset list with metadata)
        drums/
          {preset}.xpm            (drum programs)
        keygroups/
          {preset}.xpm            (keygroup programs)
        [WAV files]
    """
    slug = spec.name.replace(" ", "_")
    pack_dir = output_dir / slug
    drums_dir = pack_dir / "drums"
    keys_dir = pack_dir / "keygroups"

    if not dry_run:
        pack_dir.mkdir(parents=True, exist_ok=True)
        if spec.drum_presets:
            drums_dir.mkdir(exist_ok=True)
        if spec.keygroup_presets:
            keys_dir.mkdir(exist_ok=True)

    print(f"\nBuilding bundle: {spec.name}")
    spec.print_summary()

    # Cover art
    if not dry_run and COVER_ART_AVAILABLE:
        try:
            generate_cover(
                engine=spec.cover_engine,
                pack_name=spec.name,
                output_dir=str(pack_dir),
                preset_count=len(spec.presets),
                version=spec.version,
                seed=hash(spec.name) % 10000,
            )
        except Exception as e:
            print(f"  [WARN] Cover art: {e}")

    # Expansion.xml
    exp_xml = generate_expansion_xml(
        pack_name=spec.name,
        pack_id=spec.pack_id,
        description=spec.description or f"XO_OX Designs — {len(spec.presets)} presets",
        version=spec.version,
    ) if DRUM_EXPORT_AVAILABLE else f"<!-- {spec.name} -->"

    if not dry_run:
        (pack_dir / "Expansion.xml").write_text(exp_xml, encoding="utf-8")

    # Bundle manifest (human-readable index of what's in the pack)
    manifest = {
        "name": spec.name,
        "version": spec.version,
        "preset_count": len(spec.presets),
        "presets": [
            {
                "name": p["name"],
                "engine": p["engine"],
                "mood": p["mood"],
                "type": "drum" if p["engine"] in DRUM_ENGINES else "keygroup",
            }
            for p in spec.presets
        ]
    }
    if not dry_run:
        with open(pack_dir / "bundle_manifest.json", "w") as f:
            json.dump(manifest, f, indent=2)
        print(f"  Manifest: bundle_manifest.json ({len(spec.presets)} presets)")

    # Drum programs (XOnset)
    if spec.drum_presets and DRUM_EXPORT_AVAILABLE:
        print(f"\n  Drum programs ({len(spec.drum_presets)}):")
        for p in spec.drum_presets:
            preset_slug = p["name"].replace(" ", "_")
            wav_map = build_wav_map(wavs_dir, preset_slug) if wavs_dir else {}
            xpm = generate_xpm(p["name"], wav_map)
            xpm_path = drums_dir / f"{preset_slug}.xpm"
            if not dry_run:
                xpm_path.write_text(xpm, encoding="utf-8")
            print(f"    ✓ {p['name']} ({len(wav_map)} WAVs found)")

    # Keygroup programs
    if spec.keygroup_presets:
        print(f"\n  Keygroup programs ({len(spec.keygroup_presets)}):")
        for p in spec.keygroup_presets:
            preset_slug = p["name"].replace(" ", "_")
            wav_map = {}
            if wavs_dir:
                if KEYGROUP_EXPORT_AVAILABLE:
                    wav_map = build_keygroup_wav_map(wavs_dir, preset_slug)
                else:
                    # Fallback: manual glob for {slug}__NOTE__vel.WAV
                    for wf in wavs_dir.glob(f"{preset_slug}__*.WAV"):
                        wav_map[wf.stem] = wf.name
            xpm = generate_keygroup_xpm_stub(p["name"], p["engine"], wav_map)
            xpm_path = keys_dir / f"{preset_slug}.xpm"
            if not dry_run:
                xpm_path.write_text(xpm, encoding="utf-8")
            print(f"    ✓ {p['name']} [{p['engine']}] ({len(wav_map)} WAVs found)")

    # Copy WAVs if wavs_dir provided
    if wavs_dir and not dry_run:
        copied = 0
        for wf in list(wavs_dir.glob("*.wav")) + list(wavs_dir.glob("*.WAV")):
            dst = pack_dir / wf.name
            if not dst.exists():
                shutil.copy2(wf, dst)
                copied += 1
        if copied:
            print(f"\n  WAVs: {copied} files copied")

    print(f"\n  Output: {pack_dir}")
    return {"pack_dir": str(pack_dir), "manifest": manifest}


# =============================================================================
# PREDEFINED BUNDLE PROFILES
# =============================================================================

PREDEFINED_PROFILES = {
    "hip-hop": {
        "name": "Hip Hop Collection",
        "description": "808s, boom bap, trap — the essentials of hip hop production",
        "cover_engine": "ONSET",
        "tags_filter": ["808", "boom-bap", "trap", "hip-hop", "sub", "punchy"],
        "max_count": 20,
    },
    "electronic": {
        "name": "Electronic Toolkit",
        "description": "Techno, house, minimal, synthwave — electronic drum essentials",
        "cover_engine": "ONSET",
        "tags_filter": ["techno", "house", "minimal", "synthwave", "electronic", "dance"],
        "max_count": 20,
    },
    "coupling": {
        "name": "Coupling Showcase",
        "description": "All cross-engine coupling presets — designed for XOmnibus",
        "cover_engine": "ONSET",
        "mood_filter": "Entangled",
        "max_count": 50,
    },
    "experimental": {
        "name": "Experimental Collection",
        "description": "Aether and Flux presets — beyond conventional percussion",
        "cover_engine": "OPAL",
        "mood_filter": ["Aether", "Flux"],
        "max_count": 30,
    },
    "ambient": {
        "name": "Ambient Textures",
        "description": "Atmospheric and spatial percussion for cinematic production",
        "cover_engine": "ODYSSEY",
        "mood_filter": "Atmosphere",
        "max_count": 20,
    },
    "hybrid": {
        "name": "Hybrid Machines",
        "description": "Dual-layer synthesis presets — circuit meets algorithm",
        "cover_engine": "ONSET",
        "tags_filter": ["hybrid", "dual-layer", "morph", "blend", "crossfade"],
        "max_count": 20,
    },
    "replicas": {
        "name": "Classic Machines",
        "description": "808, 909, Linn, DMX, SP1200, Simmons — classic machines reimagined",
        "cover_engine": "ONSET",
        "tags_filter": ["808", "909", "linndrum", "SP1200", "DMX", "Simmons",
                       "CZ", "FM", "classic", "retro", "analog"],
        "max_count": 20,
    },
    "foundation": {
        "name": "Foundation Pack",
        "description": "All Foundation presets — workable, reliable, production-ready",
        "cover_engine": "ONSET",
        "mood_filter": "Foundation",
        "max_count": 50,
    },
    # Guru Bin Founder's Signature — 5 canonical presets, one per founding engine.
    # Each preset was selected by Guru Bin as the soul-representative of its engine.
    # Preset names resolve to:
    #   OVERDUB  → Foundation/Dub_Bass_Foundation.xometa  (name: "Dub Bass Foundation")
    #   OBESE    → Foundation/Obese_Bass.xometa           (name: "Obese Bass")
    #   ONSET    → Flux/Future_909.xometa                 (name: "Future 909")
    #   ODYSSEY  → Flux/Drift_Glide_Runner.xometa         (name: "Drift Glide Runner")
    #   OBLONG   → Entangled/Nervous System.xometa        (name: "Nervous System")
    "founders-signature": {
        "name": "Founder's Signature",
        "description": "Five presets. Five founding engines. Guru Bin's canonical starting point for XO_OX.",
        "cover_engine": "OVERDUB",
        "preset_names": [
            "Dub Bass Foundation",
            "Obese Bass",
            "Future 909",
            "Drift Glide Runner",
            "Nervous System",
        ],
        "max_count": 5,
    },
}


def build_predefined_profile(profile_key: str, index: PresetIndex,
                              wavs_dir: Optional[Path], output_dir: Path,
                              dry_run: bool = False) -> dict:
    """Build a pack from one of the predefined profile templates."""
    if profile_key not in PREDEFINED_PROFILES:
        raise ValueError(f"Unknown profile '{profile_key}'. Available: {list(PREDEFINED_PROFILES.keys())}")

    tmpl = PREDEFINED_PROFILES[profile_key]
    spec = BundleSpec(
        name=tmpl["name"],
        description=tmpl["description"],
        cover_engine=tmpl.get("cover_engine", "ONSET"),
    )

    # Collect presets by filters
    tags_filter = tmpl.get("tags_filter", [])
    mood_filter = tmpl.get("mood_filter")
    preset_names = tmpl.get("preset_names", [])
    max_count = tmpl.get("max_count", 50)

    if preset_names:
        results = index.search(preset_names=preset_names)
    elif tags_filter:
        results = index.search(tags=tags_filter)
    elif mood_filter:
        if isinstance(mood_filter, list):
            results = []
            for m in mood_filter:
                results += index.search(mood=m)
        else:
            results = index.search(mood=mood_filter)
    else:
        results = index.all()

    # Deduplicate and limit
    seen = set()
    for p in results:
        if p["name"] not in seen and len(spec.presets) < max_count:
            spec.add_preset(p)
            seen.add(p["name"])

    return build_bundle(spec, wavs_dir, output_dir, dry_run)


# =============================================================================
# CLI
# =============================================================================

def cmd_list(args, index: PresetIndex):
    results = index.search(
        engine=args.engine,
        mood=args.mood,
        tags=args.tags.split(",") if args.tags else None,
    )
    print(f"\n{'NAME':<35} {'ENGINE':<12} {'MOOD':<12} {'TAGS'}")
    print("-" * 90)
    for p in results:
        tags_str = ", ".join(p["tags"][:4])
        print(f"{p['name']:<35} {p['engine']:<12} {p['mood']:<12} {tags_str}")
    print(f"\n{len(results)} presets")


def cmd_build(args, index: PresetIndex):
    profile_path = Path(args.profile)
    if not profile_path.exists():
        # Try the built-in profiles dir
        profile_path = PROFILES_DIR / args.profile
        if not profile_path.exists():
            profile_path = PROFILES_DIR / (args.profile + ".json")
    if not profile_path.exists():
        print(f"ERROR: Profile not found: {args.profile}")
        return 1

    with open(profile_path) as f:
        profile_data = json.load(f)

    spec = BundleSpec.from_profile(profile_data, index)
    wavs_dir = Path(args.wavs_dir) if args.wavs_dir else None
    output_dir = Path(args.output_dir)
    build_bundle(spec, wavs_dir, output_dir, args.dry_run)
    return 0


def cmd_category(args, index: PresetIndex):
    spec = BundleSpec(
        name=args.pack_name or "Custom Category Pack",
        description=args.description or "",
        cover_engine=args.cover_engine or "ONSET",
    )
    results = index.search(
        engine=args.engine,
        mood=args.mood,
        tags=args.tags.split(",") if args.tags else None,
        coupling=args.coupling,
    )
    spec.add_presets(results)
    if not spec.presets:
        print("No presets matched the filter criteria.")
        return 1
    wavs_dir = Path(args.wavs_dir) if args.wavs_dir else None
    output_dir = Path(args.output_dir)
    build_bundle(spec, wavs_dir, output_dir, args.dry_run)
    return 0


def cmd_predefined(args, index: PresetIndex):
    wavs_dir = Path(args.wavs_dir) if args.wavs_dir else None
    output_dir = Path(args.output_dir)
    print(f"\nAvailable predefined packs:" if args.profile_key == "list"
          else f"Building predefined: {args.profile_key}")
    if args.profile_key == "list":
        for k, v in PREDEFINED_PROFILES.items():
            print(f"  {k:<20} — {v['name']}")
        return 0
    build_predefined_profile(args.profile_key, index, wavs_dir, output_dir, args.dry_run)
    return 0


def cmd_save_profile(args, index: PresetIndex):
    names = [n.strip() for n in args.pick.split(",")]
    presets = []
    not_found = []
    for name in names:
        entry = index.by_name(name)
        if entry:
            presets.append({"engine": entry["engine"], "name": entry["name"]})
        else:
            not_found.append(name)

    if not_found:
        print(f"[WARN] Not found: {not_found}")

    profile = {
        "name":         args.name,
        "description":  args.description or "",
        "cover_engine": args.cover_engine or "ONSET",
        "version":      "1.0",
        "pack_id":      f"com.xo-ox.custom.{args.name.lower().replace(' ', '-')}",
        "presets":      presets,
    }

    output = Path(args.output or f"{args.name.replace(' ', '_')}.json")
    with open(output, "w") as f:
        json.dump(profile, f, indent=2)
    print(f"Saved profile: {output} ({len(presets)} presets)")
    return 0


def cmd_summary(args, index: PresetIndex):
    print(index.summary())


def main():
    parser = argparse.ArgumentParser(
        description="XPN Bundle Builder — XO_OX Designs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = parser.add_subparsers(dest="command")

    # list
    p_list = sub.add_parser("list", help="List presets in the index")
    p_list.add_argument("--engine",  help="Filter by engine")
    p_list.add_argument("--mood",    help="Filter by mood")
    p_list.add_argument("--tags",    help="Filter by tags (comma-separated)")

    # summary
    sub.add_parser("summary", help="Show preset index summary")

    # build (from profile JSON)
    p_build = sub.add_parser("build", help="Build pack from a bundle profile JSON")
    p_build.add_argument("--profile",    required=True, help="Path to bundle profile JSON")
    p_build.add_argument("--wavs-dir",   help="Directory of rendered WAVs")
    p_build.add_argument("--output-dir", required=True, help="Output directory")
    p_build.add_argument("--dry-run",    action="store_true")

    # category
    p_cat = sub.add_parser("category", help="Build pack by filter criteria")
    p_cat.add_argument("--engine",     help="Filter by engine")
    p_cat.add_argument("--mood",       help="Filter by mood")
    p_cat.add_argument("--tags",       help="Filter by tags (comma-separated)")
    p_cat.add_argument("--coupling",   help="Filter by coupling level")
    p_cat.add_argument("--pack-name",  help="Pack display name")
    p_cat.add_argument("--description",help="Pack description")
    p_cat.add_argument("--cover-engine", default="ONSET")
    p_cat.add_argument("--wavs-dir",   help="Directory of rendered WAVs")
    p_cat.add_argument("--output-dir", required=True)
    p_cat.add_argument("--dry-run",    action="store_true")

    # predefined
    p_pre = sub.add_parser("predefined", help="Build from a predefined pack template")
    p_pre.add_argument("profile_key", help="Profile key (or 'list' to list all)")
    p_pre.add_argument("--wavs-dir",   help="Directory of rendered WAVs")
    p_pre.add_argument("--output-dir", default="/tmp/xo_ox_bundles")
    p_pre.add_argument("--dry-run",    action="store_true")

    # save-profile
    p_save = sub.add_parser("save-profile", help="Save a custom bundle profile JSON")
    p_save.add_argument("--name",         required=True, help="Pack name")
    p_save.add_argument("--pick",         required=True, help="Comma-separated preset names")
    p_save.add_argument("--description",  help="Pack description")
    p_save.add_argument("--cover-engine", default="ONSET")
    p_save.add_argument("--output",       help="Output JSON path")

    args = parser.parse_args()
    if not args.command:
        parser.print_help()
        return 1

    index = PresetIndex()

    dispatch = {
        "list":         cmd_list,
        "summary":      cmd_summary,
        "build":        cmd_build,
        "category":     cmd_category,
        "predefined":   cmd_predefined,
        "save-profile": cmd_save_profile,
    }
    return dispatch[args.command](args, index) or 0


if __name__ == "__main__":
    sys.exit(main())
