# OVERWORLD V2 Concept: Dual Triangle ERA System
**Origin:** Guru Bin Retreat conversation — 2026-03-15
**Status:** Design concept, not yet implemented
**Scope:** XOverworld V2 / XOlokun OVERWORLD engine upgrade

---

## The Idea

The current ERA system uses a single triangle with 3 chip engines at its vertices. The player navigates a 2D barycentric space, blending between those 3 engines.

The Dual Triangle ERA system introduces a **second triangle on the same plane**. Both triangles are independently assigned chip engines. The player's cursor navigates the combined shape — which changes based on how the two triangles are positioned relative to each other.

Because OVERWORLD has exactly **6 chip engines** (NES, FM, SNES, Game Boy, PC Engine, Neo Geo), and a dual triangle has exactly **6 vertices**, the mapping is perfect. One console per point.

---

## The Geometry

The user controls the relative position, rotation, and scale of Triangle B on the same plane as Triangle A. This produces a continuous range of configurations:

### Configuration Modes

| Shape | How Achieved | Musical Result |
|-------|-------------|----------------|
| **Single triangle** | Full overlap, B on top of A | Identical to current system. 3 engines. |
| **Star of David** | B inverted, centered — classic hexagram | All 6 consoles reachable. 6 points, 6 overlapping regions. Maximum complexity. |
| **Partial overlap** | B offset partially into A | Intersection zone blends up to 6 engines; outer zones are pure per-triangle. |
| **One-point touch** | B touching A at a single vertex | Two triangles sharing one console. Hinge point for morphing between systems. |
| **Separate triangles** | B fully outside A | Two independent 3-engine zones. Cursor switches between them at the gap. |
| **Stretched star** | B scaled larger/smaller than A | Asymmetric star — some console territories are larger than others. |

---

## The Cursor Behavior

The cursor position (ow_era, ow_eraY) navigates the combined shape.

**Inside Triangle A only:** Barycentric blend of A's 3 engines. Same as today.

**Inside Triangle B only:** Barycentric blend of B's 3 engines.

**Inside the intersection (overlap) zone:** The cursor receives a weighted blend from *both* triangles simultaneously — up to 6 engines contributing. The blend weight from each triangle is proportional to how deeply the cursor sits inside that triangle's area.

**At the center of the Star of David:** Maximum 6-engine blend. All consoles contributing equally.

**At a star point:** Isolation of one console. Pure vertex behavior.

---

## New Parameters Required

### Triangle B Vertex Assignments
```
ow_vertexD    — chip engine at Triangle B vertex 1 (0–5)
ow_vertexE    — chip engine at Triangle B vertex 2 (0–5)
ow_vertexF    — chip engine at Triangle B vertex 3 (0–5)
```

### Triangle B Positioning
```
ow_triBx          — Triangle B center X offset from Triangle A center (-1.0 to +1.0)
ow_triBy          — Triangle B center Y offset from Triangle A center (-1.0 to +1.0)
ow_triBRotation   — Triangle B rotation relative to Triangle A (0–360°)
ow_triBScale      — Triangle B scale relative to Triangle A (0.5–2.0)
```

### Configuration Presets (snap positions)
```
ow_eraConfig  — named configurations:
  0 = Single (full overlap)
  1 = Star (inverted, centered)
  2 = Offset right
  3 = Offset left
  4 = One-point touch
  5 = Separated
  6 = Custom (use triBx/triBy/triBRotation directly)
```

### Path Automation
```
ow_pathMode   — automated cursor travel path:
  0 = Off (manual only)
  1 = Star perimeter (cycles all 6 points in sequence)
  2 = Intersection orbit (circles the overlap zone)
  3 = Pendulum (sweeps between two opposite points)
  4 = Lissajous (complex figure-8 within the combined shape)
  5 = Random walk (Perlin noise, stays within shape bounds)
  6 = Bounce (reflects off shape edges)

ow_pathRate   — speed of automated travel (0.01–10 Hz)
ow_pathDepth  — how far the path travels (0–1, relative to shape size)
```

---

## Intersection Blend Math

When the cursor is at position P:

1. Compute barycentric coordinates of P relative to Triangle A → weights `wA1, wA2, wA3`
2. Compute barycentric coordinates of P relative to Triangle B → weights `wB1, wB2, wB3`
3. Determine if P is inside A (`sumA > 0`), inside B (`sumB > 0`), or in both
4. In intersection: normalize total weight across all contributing engines
5. Engines assigned to both triangles (if vertexD=vertexA, etc.) accumulate weight

```
// Pseudocode
float aInside = barycentricWeight(P, triangleA);  // 0 outside, 1 deep inside
float bInside = barycentricWeight(P, triangleB);
float total = aInside + bInside;

engineWeight[vertexA] += aInside * wA1 / total;
engineWeight[vertexB] += aInside * wA2 / total;
engineWeight[vertexC] += aInside * wA3 / total;
engineWeight[vertexD] += bInside * wB1 / total;
engineWeight[vertexE] += bInside * wB2 / total;
engineWeight[vertexF] += bInside * wB3 / total;
```

---

## UI Concept

The ERA pad visualizes both triangles:
- Triangle A: drawn in the engine accent color (Neon Green)
- Triangle B: drawn in a complementary color (or the same, dimmed)
- Intersection zone: highlighted — a distinct fill showing the 6-engine blend area
- Star points: labeled with console names
- Cursor: single dot navigating the combined shape
- Path preview: dotted line showing the automated path

The configuration control (ow_eraConfig) could be a visual selector showing miniature versions of each shape configuration — like a chord diagram selector.

---

## Musical Applications

### The Canonical Star (ow_eraConfig=1)
Triangle A: NES / FM / SNES
Triangle B: Game Boy / PC Engine / Neo Geo
Result: Perfect hexagram, one console per point. Path = Star Perimeter traces all 6 consoles in sequence. Every chip emulator in one automated journey.

### The Hinged Morph (ow_eraConfig=4, one-point touch)
Triangle A: NES / FM / SNES with point touching
Triangle B: Game Boy / FM / PC Engine (FM shared)
Result: FM is the hinge. The cursor pivots between two different triangles through the shared console. Moving left = NES/SNES territory. Moving right = GB/PCE territory. Always passing through FM.

### The Asymmetric Star
ow_triBScale = 1.5, ow_triBRotation = 30°
Result: Triangle B is larger. Some consoles have more navigable territory than others. The star has long points and short points — some consoles are harder to isolate, some are easier.

### The Drift Star
ow_pathMode = 5 (random walk), ow_triBx/triBy slowly automating via LFO
Result: The shape itself moves while the cursor walks inside it. Consoles drift in and out of the blend as the triangle shifts. The system becomes a living, morphing 6-console texture.

---

## Implementation Notes

### DSP Cost
The additional blend math is negligible — barycentric calculations are just dot products. The chip engine voices are already instantiated. The only cost is the blending logic per block, which adds <0.1% CPU.

### Preset Compatibility
All new parameters default to:
- ow_vertexD=3, ow_vertexE=4, ow_vertexF=5 (GB, PCE, Neo Geo)
- ow_triBx=0, ow_triBy=0, ow_triBRotation=180° → Star of David as default V2 behavior
- ow_eraConfig=1 (Star) for new presets; old presets get ow_eraConfig=0 (Single/overlap) for identical behavior

### V1 Compatibility
If ow_eraConfig=0 (Single triangle / full overlap), the system behaves identically to V1. All existing presets are unaffected.

---

## Why This Belongs to OVERWORLD Specifically

Other engines could theoretically have dual morphing systems. OVERWORLD is the *only* engine where the math resolves perfectly:

- 6 chip engines = 6 triangle vertices = 6 points of a hexagram
- The Star of David is not a design metaphor — it is the actual optimal configuration for accessing all 6 consoles
- The geometry was already implicit in the architecture. This concept makes it explicit.

No other engine in the fleet has this exact numerical coincidence.

---

## Next Steps

- [ ] Prototype the blend math in isolation (Python or standalone JUCE test)
- [ ] Design UI for the dual triangle ERA pad
- [ ] Write parameter additions to XOverworld's Parameters.h
- [ ] Spec the path automation engine (6 modes)
- [ ] Create V2 awakening presets demonstrating each configuration mode
- [ ] Seance: present to ghosts for architectural blessing before implementation
