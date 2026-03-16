# XPN/XPM Format Future-Proofing — R&D Spec
**Authors:** Hex + Rex (XO_OX format / bridge androids)
**Date:** 2026-03-16
**Status:** R&D — implementation roadmap, not yet in Oxport toolchain

---

## Purpose

The XPN/XPM format is Akai's territory, not ours. Every firmware revision is a potential breaking change. MPCe 3D pads introduce schema territory that doesn't exist yet. Multi-platform export is a pressure point as XO_OX packs reach DAW-native audiences.

This document defines a defensive architecture for the Oxport tool suite — one that degrades gracefully, passes through unknown data, and doesn't require a full rewrite every time Akai ships a `.0` release.

---

## Status Markers

| Marker | Meaning |
|--------|---------|
| **[CONFIRMED]** | Verified MPC behavior in current toolchain |
| **[PLAUSIBLE]** | Logical inference from known schema; likely correct |
| **[PROPOSED]** | XO_OX design spec; no Akai prior art |
| **[SPECULATIVE]** | Hypothesis requiring hardware test against future firmware |

---

## Section 1: Schema Versioning

### The Problem

The current XPN ZIP carries no version field. When Akai adds a field — or removes one — tools that generated the old format either silently produce broken packs or fail with an opaque XML parse error. We have no way to detect, at import time, whether a pack was built against old tools or new.

### Proposal: `format_version` in `bundle_manifest.json`

**[PROPOSED]** Every XPN ZIP produced by Oxport should contain a `bundle_manifest.json` at the root of the archive (see Section 7 for full v2.0 spec). This file carries:

```json
{
  "xo_ox_pack_format": "2.0",
  "xpm_schema_target": "mpc_3.5",
  "created_by": "oxport/1.4.0",
  "created_at": "2026-03-16T00:00:00Z"
}
```

- `xo_ox_pack_format` — our own versioning; bumps when our bundle structure changes
- `xpm_schema_target` — the MPC firmware version the XPMs were authored against
- `created_by` — tool + version that built the pack; essential for regression debugging

### Detection Logic at Import/Validate Time

```python
def detect_pack_version(zip_path: str) -> dict:
    with zipfile.ZipFile(zip_path) as z:
        if "bundle_manifest.json" in z.namelist():
            manifest = json.loads(z.read("bundle_manifest.json"))
            return {
                "version": manifest.get("xo_ox_pack_format", "unknown"),
                "schema_target": manifest.get("xpm_schema_target", "unknown"),
                "legacy": False,
            }
        elif "expansion.json" in z.namelist():
            # v1.x pack — expansion.json present, no bundle_manifest
            return {"version": "1.x", "schema_target": "mpc_3.x", "legacy": True}
        else:
            return {"version": "0.x", "schema_target": "unknown", "legacy": True}
```

**[PROPOSED]** When `legacy: True` is returned, Oxport's validator emits a warning but does not refuse to process — we never break backwards reads.

---

## Section 2: MPCe 3D Pad XML Schema

### What We Know

MPCe ships hardware-level 3D sensing per pad: XY position within the pad surface plus Z (pressure/velocity). This is distinct from aftertouch — it is spatial finger position within the pad boundary. **[SPECULATIVE]** Akai will need to expose this in XPM as per-pad assignment metadata once firmware fully activates the feature.

### Speculative `<PadCornerAssignment>` Schema

**[SPECULATIVE]** A plausible XML extension under each `<Pad>` node:

```xml
<Pad number="1" bank="A">
  <!-- existing fields remain untouched -->
  <PadNote>36</PadNote>
  <PadMidiChannel>1</PadMidiChannel>

  <!-- hypothetical MPCe 3D extension -->
  <PadExpressive3D enabled="true">
    <PadCornerAssignment>
      <CornerNW note="36" velocity="64" />
      <CornerNE note="38" velocity="64" />
      <CornerSW note="40" velocity="64" />
      <CornerSE note="41" velocity="64" />
    </PadCornerAssignment>
    <PadXYMorph>
      <XAxis cc="16" range="0 127" />
      <YAxis cc="17" range="0 127" />
    </PadXYMorph>
    <PadZCurve type="exponential" sensitivity="0.8" />
  </PadExpressive3D>
</Pad>
```

Key assumptions baked into this schema guess:
- Corner assignments are note+velocity pairs, not sample overrides (samples are program-level)
- XY morphing sends CCs, not pitch bend (avoids pitch quantization artifacts)
- Z curve is separate from velocity — Z is aftertouch-equivalent while velocity is still strike speed
- The entire `<PadExpressive3D>` block is optional — pads without 3D hardware ignore it

### Graceful Degradation Strategy

**[PROPOSED]** Oxport's XPM writer should support a `--3d-pads` flag. Without the flag, no `<PadExpressive3D>` blocks are written. With the flag, they are appended after existing pad fields.

At read time, any parser that encounters `<PadExpressive3D>` and doesn't recognize it should discard the block without error — this is the standard XML "ignore unknown elements" contract. We enforce this in our own tools (see Section 3).

Practical rule: **design 3D content as enrichment, not a dependency.** A pad that plays correctly without 3D sensing is a pad that works on every MPC in the install base.

---

## Section 3: Unknown Field Tolerance

### The Problem

XPM XML contains fields that Akai has never publicly documented. `<ChokingGroup>`, `<MuteTarget>`, certain filter type enumerations — all observed in the wild, undocumented. Our tools currently round-trip the fields we know and silently drop the rest.

Dropping unknown fields is dangerous. A producer who edits a pack in Akai's own software, adds an undocumented field, and re-imports via Oxport loses that field permanently.

### Proposed: Opaque Pass-Through

**[PROPOSED]** Oxport's XPM read/write layer should operate in two passes:

1. **Parse pass** — extract fields we explicitly handle into structured objects
2. **Preserve pass** — capture the raw XML subtree of any element not in our handled set as an opaque string blob, keyed by element name and position

At write time, opaque blobs are re-serialized in their original position (before the closing tag of the parent element).

```python
class PadConfig:
    note: int
    velocity_start: int
    velocity_end: int
    # ... known fields ...
    _unknown_xml_fragments: list[str] = field(default_factory=list)

def serialize_pad(pad: PadConfig) -> str:
    known_xml = render_known_fields(pad)
    unknown_xml = "\n".join(pad._unknown_xml_fragments)
    return f"<Pad>{known_xml}\n{unknown_xml}\n</Pad>"
```

This gives us read/write idempotence on any XPM file, even ones produced by future firmware we haven't seen.

**[CONFIRMED]** Existing Oxport tools do not have this pass-through. This is a gap that needs closing before v2.0.

---

## Section 4: Firmware Detection Strategy

### The Problem

Pack tools need to know whether the target MPC can handle a given feature. Writing `<PadExpressive3D>` to a pack targeting MPC 3.2 firmware would be noise at best, corruption at worst.

### Option A: User-Declared Target (Current Approach)

The tool accepts a `--firmware-target 3.5` flag. Simple, reliable, no guesswork. Downside: producers forget, or don't know.

### Option B: Read MPC System Info from Expansion Mount

**[SPECULATIVE]** When an MPC expansion drive is mounted on macOS, Akai's software writes a system info file to the root of the drive or the expansion folder. If this file exists and is readable, Oxport could parse it.

Speculative path: `/Volumes/MPC_CONTENT/System/mpc_firmware_info.json` or `.ini`.

Speculative content:
```json
{ "firmware_version": "3.7.0", "hardware_model": "MPCe", "pad_3d_capable": true }
```

**[SPECULATIVE]** We do not know this file exists. Detection should be opportunistic:

```python
def detect_firmware_from_mount(mount_path: str) -> dict | None:
    candidates = [
        os.path.join(mount_path, "System", "mpc_firmware_info.json"),
        os.path.join(mount_path, ".mpc_system"),
    ]
    for path in candidates:
        if os.path.exists(path):
            return parse_firmware_info(path)
    return None  # fall through to user-declared or safe defaults
```

### Option C: Conservative Safe Defaults

**[PROPOSED]** When firmware version is unknown, write to the lowest common denominator: no 3D pad fields, no features beyond MPC 3.0 schema. This is the correct default for a distribution pack that ships to thousands of producers on mixed firmware.

Feature escalation should be opt-in, not opt-out.

---

## Section 5: Multi-Format Export

### The Opportunity

A significant segment of XO_OX's audience uses DAWs — Ableton, Logic, Kontakt — not MPC. The same sample library and instrument design should serve all of them. Building per-format tools in isolation wastes work and creates divergence.

### Shared Intermediate Representation (IR)

**[PROPOSED]** Oxport defines a Python dataclass layer — `XO_PackIR` — that is format-agnostic:

```
XO_PackIR
├── meta: PackMeta (name, version, engine, mood, sonic_dna)
├── programs: list[ProgramIR]
│   ├── name: str
│   ├── pads: list[PadIR]  (for drum programs)
│   ├── keygroups: list[KeygroupIR]  (for instrument programs)
│   └── qlinks: list[QLinkIR]
└── samples: list[SampleIR]
    ├── path: str
    ├── root_note: int
    ├── velocity_range: tuple[int, int]
    └── loop_points: tuple[int, int] | None
```

Each format exporter consumes `XO_PackIR` and writes its own format:

| Exporter | Output | Target |
|----------|--------|--------|
| `XpnExporter` | `.xpn` ZIP | MPC hardware / standalone |
| `SfzExporter` | `.sfz` + samples | Any SFZ host (Decent Sampler, sfizz, HISE) |
| `NkiExporter` | `.nki` XML | Kontakt (premium tier — requires NKS SDK) |
| `DsPresetExporter` | `.dspreset` | Decent Sampler (free tier, wide reach) |

**[PROPOSED]** The `SfzExporter` is the first priority after XPN — broadest compatibility, no SDK required, works in Reaper/HISE/Bitwig/sfizz.

### SFZ Mapping Sketch

```sfz
<control>
default_path=Samples/

<group> key=36 lovel=0 hivel=127
<region> sample=Kick_Hard.wav lokey=36 hikey=36 pitch_keycenter=36

<group> key=38 lovel=0 hivel=63
<region> sample=Snare_Soft.wav lokey=38 hikey=38
<group> key=38 lovel=64 hivel=127
<region> sample=Snare_Hard.wav lokey=38 hikey=38
```

Velocity layers in `XO_PackIR` map directly to SFZ `lovel`/`hivel` — no data loss.

---

## Section 6: Deprecation Policy

### The Problem

Akai occasionally deprecates fields — writes them to be ignored in new firmware — while old hardware still depends on them. `<ChokingGroup>` is the canonical example: newer firmware uses `<MuteTarget>`, but Live II units on old firmware still parse `<ChokingGroup>`.

### Proposed Policy

**[PROPOSED]** XO_OX maintains a deprecation registry in `Tools/format_deprecation.json`:

```json
{
  "ChokingGroup": {
    "deprecated_as_of": "mpc_3.3",
    "superseded_by": "MuteTarget",
    "write_until": "mpc_4.0",
    "reason": "Legacy hardware compat — Live II units on firmware < 3.3"
  }
}
```

Rules:
1. A deprecated field continues to be written until `write_until` firmware becomes the install-base floor (estimated 18 months after deprecation announcement).
2. A deprecated field is always written alongside its successor, never instead of.
3. When `write_until` firmware is reached, the field moves to `_unknown_xml_fragments` treatment — passed through if present in a source file, not actively written.
4. The registry is a version-controlled artifact. Every deprecation decision is documented with a rationale and date.

**Guiding principle:** We are a pack creator, not a firmware maintainer. We write defensively and never assume the install base is on current firmware.

---

## Section 7: XO_OX Pack Format v2.0 Proposal

### Motivation

v1.x packs use `expansion.json` as the canonical metadata file — an Akai-originated format with limited extensibility. Bundle-level metadata (engine attribution, sonic DNA, mood categories, format version) has been crammed into fields that were not designed for it.

### v2.0 Structure

```
pack_name.xpn  (ZIP)
├── bundle_manifest.json       ← NEW canonical metadata (v2.0)
├── expansion.json             ← backward-compat shim (read by MPC hardware)
├── Programs/
│   ├── Kit_01.xpm
│   └── Kit_02.xpm
├── Samples/
│   └── *.wav
└── Artwork/
    └── cover.png
```

### `bundle_manifest.json` Schema

```json
{
  "xo_ox_pack_format": "2.0",
  "pack_name": "Aquatic Depths",
  "pack_slug": "aquatic-depths",
  "engine": "ONSET",
  "mood": "Aether",
  "sonic_dna": {
    "brightness": 0.3,
    "warmth": 0.6,
    "movement": 0.8,
    "density": 0.5,
    "space": 0.9,
    "aggression": 0.2
  },
  "xpm_schema_target": "mpc_3.5",
  "created_by": "oxport/2.0.0",
  "created_at": "2026-03-16T00:00:00Z",
  "programs": ["Programs/Kit_01.xpm", "Programs/Kit_02.xpm"],
  "sample_count": 128,
  "kit_count": 2,
  "version": "1.0.0"
}
```

### `expansion.json` as Shim

The existing `expansion.json` remains in the ZIP for MPC hardware compatibility. It carries only the fields MPC requires for expansion library browsing — name, author, description, thumbnail path. It does NOT duplicate sonic DNA or format metadata. That information lives in `bundle_manifest.json` exclusively.

```json
{
  "name": "Aquatic Depths",
  "author": "XO_OX Designs",
  "description": "Deep drum character from the ONSET engine.",
  "icon": "Artwork/cover.png"
}
```

### Migration Path

- Oxport v2.0 writes both files on every export
- Oxport v1.x packs (expansion.json only) are valid reads — `detect_pack_version()` handles them
- No re-export of existing packs required for MPC compatibility — only needed if XO_OX internal tooling needs the v2.0 metadata

---

## Implementation Priority

| Task | Priority | Effort |
|------|----------|--------|
| Add `bundle_manifest.json` to Oxport export | High | Low — add one JSON writer |
| Unknown field pass-through in XPM reader/writer | High | Medium — XML round-trip refactor |
| `format_deprecation.json` registry (initial entries) | Medium | Low — JSON file, no code |
| `detect_pack_version()` utility | Medium | Low — 20-line function |
| `XO_PackIR` intermediate representation | Medium | Medium — dataclass layer |
| `SfzExporter` first pass | Medium | High — new exporter |
| 3D pad `--3d-pads` flag (spec only, no hardware yet) | Low | Low — spec-writing exercise |
| Firmware detection from mount point | Low | Low — opportunistic, not blocking |

---

## Open Questions

1. Does Akai's expansion browser parse `expansion.json` from inside a ZIP, or does it require extraction first? This affects whether our shim design holds.
2. Will MPCe 3D pad data be per-pad or per-program in the eventual schema? Corner assignments at program level (one XPM, 16 pads each with corners) vs. at pack level (metadata file) are architecturally different.
3. SFZ round-trip fidelity: can all Oxport velocity/choke/mute group behaviors be expressed in SFZ without loss? Choke groups in particular may require `<group>` + `off_by` pairs that don't have clean XPM equivalents.
4. NKI format requires Kontakt NKS SDK agreement. Is a premium-tier Kontakt export worth the SDK overhead, or is Decent Sampler `.dspreset` sufficient for the DAW-native audience?

---

*Spec ends. Next action: open an Oxport GitHub issue for unknown-field pass-through (highest-value low-effort item). Assign bundle_manifest.json to the next Oxport minor release.*
