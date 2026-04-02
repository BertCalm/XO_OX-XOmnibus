# XO-OX.org Site Content Updates
*2026-03-20 | 42-engine parity pass*

---

## Summary of Changes Required

### Current State
- **index.html**: 31 engines listed in instrument gallery
- **aquarium.html**: 31 engine rows in water column, meta description says "Thirty-four species"
- **packs.html**: free packs only, no Transcendental section, says "34 engines, 10,028 presets" in meta
- Counts throughout site reference "thirty-four" or "34" — all outdated

### Target State
- **44 engines** across all pages (was 42 when this doc was written; OXBOW + OWARE added same day)
- **~15,200 presets** (not 10,028)
- Aquarium updated with all 11 missing engines
- ORBWEAVE, OVERTONE, ORGANISM marked as "Preview"
- Transcendental Vol 1 section on packs page (Patreon TODO)
- All "coming soon" language removed for live engines

---

## A) Count/Number Updates (All Pages)

Find and replace these stale numbers:

| Old | New | Location |
|-----|-----|----------|
| "thirty-four engines" | "forty-two engines" | index.html meta, body text |
| "34 engines" | "42 engines" | index.html og/twitter meta, packs.html meta |
| "10,028 presets" | "15,200 presets" | og/twitter meta both pages |
| "Thirty-four species" | "Forty-two species" | aquarium.html meta description |
| "34 engines. 10,028 presets." | "42 engines. 15,200 presets." | og/twitter meta |
| "over twenty-four hundred presets" | "over fifteen thousand presets" | index.html JSON-LD |

---

## B) Aquarium — 11 Missing Engine Rows

Insert the following engine rows into aquarium.html. Insertion points are noted.

### Engines Missing (11 total)
1. **OSPREY** — The Reef, Azulejo Blue `#1B4F8A`
2. **OSTERIA** — The Reef, Porto Wine `#722F37`
3. **OVERLAP** — Open Water, Bioluminescent Cyan-Green `#00FFB4`
4. **OVERTONE** — Open Water, Spectral Ice `#A8D8EA` *(Preview)*
5. **OUTWIT** — The Deep, Chromatophore Amber `#CC6600`
6. **OMBRE** — Thermocline, Shadow Mauve `#7B6B8A`
7. **ORCA** — The Abyss, Deep Ocean `#1B2838`
8. **OCTOPUS** — The Abyss, Chromatophore Magenta `#E040FB`
9. **OBRIX** — The Reef, Reef Jade `#1E8B7E`
10. **ORBWEAVE** — The Abyss, Kelp Knot Purple `#8E4585` *(Preview)*
11. **ORGANISM** — The Abyss, Emergence Lime `#C6E377` *(Preview)*

### HTML Snippets for Each Engine Row

**OSPREY** — insert after oracle row (within The Reef zone):
```html
<div class="engine-row right reveal" data-engine="osprey">
  <div class="engine-node" style="color: #1B4F8A; background: #1B4F8A;"></div>
  <div class="engine-card" style="border-color: rgba(27,79,138,0.2);">
    <div class="engine-card-name" style="color: #1B4F8A;">XOsprey</div>
    <div class="engine-card-creature">The Shoreline Hunter &mdash; Coastal, Alert</div>
    <div class="engine-card-identity">The fish hawk — specialist at the boundary where land meets water. Dives through the surface and emerges with the catch. Five-coastline ShoreSystem models the acoustic character of different shores.</div>
    <div class="polarity-bar"><div class="polarity-track"><div class="polarity-marker" style="left: 35%;"></div></div><span class="polarity-label">The Reef</span></div>
    <div style="position:absolute;top:0;left:0;width:3px;height:100%;background:#1B4F8A;"></div>
  </div>
</div>
```

**OSTERIA** — insert after osprey (still within The Reef zone):
```html
<div class="engine-row left reveal" data-engine="osteria">
  <div class="engine-node" style="color: #722F37; background: #722F37;"></div>
  <div class="engine-card" style="border-color: rgba(114,47,55,0.2);">
    <div class="engine-card-name" style="color: #722F37;">XOsteria</div>
    <div class="engine-card-creature">The Waterfront Tavern &mdash; Cultural, Warm</div>
    <div class="engine-card-identity">The oldest gathering place — simple food, port wine, music from wherever the sailors came from. Five coastlines of cultural warmth. Where creatures pause on their journeys and share what they carry.</div>
    <div class="polarity-bar"><div class="polarity-track"><div class="polarity-marker" style="left: 40%;"></div></div><span class="polarity-label">The Reef</span></div>
    <div style="position:absolute;top:0;left:0;width:3px;height:100%;background:#722F37;"></div>
  </div>
</div>
```

**OBRIX** — insert after oblong, before oracle (The Reef zone):
```html
<div class="engine-row right reveal" data-engine="obrix">
  <div class="engine-node" style="color: #1E8B7E; background: #1E8B7E;"></div>
  <div class="engine-card" style="border-color: rgba(30,139,126,0.2);">
    <div class="engine-card-name" style="color: #1E8B7E;">XObrix</div>
    <div class="engine-card-creature">The Coral Architecture &mdash; Modular, Constructive</div>
    <div class="engine-card-identity">Individual polyps stacking into architecture of staggering complexity. Synthesis bricks that snap together — oscillator, filter, modulation, and FX bricks configured in chains. Each brick alone is modest. Stacked, they become the reef.</div>
    <div class="polarity-bar"><div class="polarity-track"><div class="polarity-marker" style="left: 42%;"></div></div><span class="polarity-label">The Reef</span></div>
    <div style="position:absolute;top:0;left:0;width:3px;height:100%;background:#1E8B7E;"></div>
  </div>
</div>
```

**OVERLAP** — insert after oceanic (Open Water zone):
```html
<div class="engine-row right reveal" data-engine="overlap">
  <div class="engine-node" style="color: #00FFB4; background: #00FFB4;"></div>
  <div class="engine-card" style="border-color: rgba(0,255,180,0.2);">
    <div class="engine-card-name" style="color: #00FFB4;">XOverlap</div>
    <div class="engine-card-creature">The Kelp Forest &mdash; Entangled, Growing</div>
    <div class="engine-card-identity">A cathedral of filtered green light and entangled motion. Kelp blades growing from floor toward light, tangled together. KnotMatrix generates topological resonances — harmonics that loop through each other like living tendrils.</div>
    <div class="polarity-bar"><div class="polarity-track"><div class="polarity-marker" style="left: 50%;"></div></div><span class="polarity-label">Open Water</span></div>
    <div style="position:absolute;top:0;left:0;width:3px;height:100%;background:#00FFB4;"></div>
  </div>
</div>
```

**OVERTONE** — insert after overlap (Open Water zone), marked Preview:
```html
<div class="engine-row left reveal" data-engine="overtone">
  <div class="engine-node" style="color: #A8D8EA; background: #A8D8EA;"></div>
  <div class="engine-card" style="border-color: rgba(168,216,234,0.2);">
    <div class="engine-card-name" style="color: #A8D8EA;">XOvertone <span style="font-size:0.65em;font-weight:400;opacity:0.7;margin-left:0.5em;">Preview</span></div>
    <div class="engine-card-creature">The Nautilus &mdash; Mathematical, Spiraling</div>
    <div class="engine-card-identity">The animal that builds its shell according to a logarithmic spiral. Harmonic series derived from continued fraction approximations of irrational numbers — pi, phi, e. Spectral configurations that are mathematically precise but never periodic.</div>
    <div class="polarity-bar"><div class="polarity-track"><div class="polarity-marker" style="left: 45%;"></div></div><span class="polarity-label">Open Water</span></div>
    <div style="position:absolute;top:0;left:0;width:3px;height:100%;background:#A8D8EA;"></div>
  </div>
</div>
```

**OMBRE** — insert after opal (Thermocline zone):
```html
<div class="engine-row right reveal" data-engine="ombre">
  <div class="engine-node" style="color: #7B6B8A; background: #7B6B8A;"></div>
  <div class="engine-card" style="border-color: rgba(123,107,138,0.2);">
    <div class="engine-card-name" style="color: #7B6B8A;">XOmbre</div>
    <div class="engine-card-creature">The Fading Gradient &mdash; Layered, Contemplative</div>
    <div class="engine-card-identity">The way color fades in water as depth increases. Two narrative voices — one bright and present, one fading and past — blending across the thermocline boundary where clarity becomes uncertainty. Memory meeting forgetting.</div>
    <div class="polarity-bar"><div class="polarity-track"><div class="polarity-marker" style="left: 52%;"></div></div><span class="polarity-label">Thermocline</span></div>
    <div style="position:absolute;top:0;left:0;width:3px;height:100%;background:#7B6B8A;"></div>
  </div>
</div>
```

**OUTWIT** — insert after organon (The Deep zone):
```html
<div class="engine-row right reveal" data-engine="outwit">
  <div class="engine-node" style="color: #CC6600; background: #CC6600;"></div>
  <div class="engine-card" style="border-color: rgba(204,102,0,0.2);">
    <div class="engine-card-name" style="color: #CC6600;">XOutwit</div>
    <div class="engine-card-creature">The Chromatophore Mimic &mdash; Adaptive, Cunning</div>
    <div class="engine-card-identity">Deep-water camouflage specialists — the mimic octopus, the cuttlefish, the creature that is never the same twice. Adaptive timbre shifts in response to input. It reads what surrounds it and becomes something that sounds like it belongs there.</div>
    <div class="polarity-bar"><div class="polarity-track"><div class="polarity-marker" style="left: 72%;"></div></div><span class="polarity-label">Oscar-leaning</span></div>
    <div style="position:absolute;top:0;left:0;width:3px;height:100%;background:#CC6600;"></div>
  </div>
</div>
```

**ORCA** — insert after obscura (The Abyss zone):
```html
<div class="engine-row left reveal" data-engine="orca">
  <div class="engine-node" style="color: #1B2838; background: #1B2838; border: 1px solid #4A7FA0;"></div>
  <div class="engine-card" style="border-color: rgba(27,40,56,0.4);">
    <div class="engine-card-name" style="color: #4A7FA0;">XOrca</div>
    <div class="engine-card-creature">The Apex Predator &mdash; Coordinated, Spatial</div>
    <div class="engine-card-identity">The only predator that hunts whales. Coordinated, strategic, uses three-dimensional space as a weapon. The HUNT macro is coordinated predator behavior — at maximum, the entire engine transforms as a single organism committing to the attack.</div>
    <div class="polarity-bar"><div class="polarity-track"><div class="polarity-marker" style="left: 90%;"></div></div><span class="polarity-label">Pure Oscar</span></div>
    <div style="position:absolute;top:0;left:0;width:3px;height:100%;background:#1B2838; border-left: 3px solid #4A7FA0;"></div>
  </div>
</div>
```

**OCTOPUS** — insert after orca (The Abyss zone):
```html
<div class="engine-row right reveal" data-engine="octopus">
  <div class="engine-node" style="color: #E040FB; background: #E040FB;"></div>
  <div class="engine-card" style="border-color: rgba(224,64,251,0.2);">
    <div class="engine-card-name" style="color: #E040FB;">XOctopus</div>
    <div class="engine-card-creature">Distributed Intelligence &mdash; Alien, Reactive</div>
    <div class="engine-card-identity">Two-thirds of an octopus's neurons live in its arms — each arm thinks semi-independently. Eight arm LFOs, chromatophore skin-shifting, ink cloud timbral weapon. Alien intelligence operating below the threshold of consciousness.</div>
    <div class="polarity-bar"><div class="polarity-track"><div class="polarity-marker" style="left: 90%;"></div></div><span class="polarity-label">Pure Oscar</span></div>
    <div style="position:absolute;top:0;left:0;width:3px;height:100%;background:#E040FB;"></div>
  </div>
</div>
```

**ORBWEAVE** — insert after octopus (The Abyss zone), marked Preview:
```html
<div class="engine-row left reveal" data-engine="orbweave">
  <div class="engine-node" style="color: #8E4585; background: #8E4585;"></div>
  <div class="engine-card" style="border-color: rgba(142,69,133,0.2);">
    <div class="engine-card-name" style="color: #8E4585;">XOrbweave <span style="font-size:0.65em;font-weight:400;opacity:0.7;margin-left:0.5em;">Preview</span></div>
    <div class="engine-card-creature">The Kelp Knot &mdash; Topological, Bidirectional</div>
    <div class="engine-card-identity">Kelp growing in fast currents ties itself into loops that can't be undone without cutting. Topological resonances — harmonic series that loop through each other inseparably. The only engine built specifically around a coupling concept: it exists to entangle.</div>
    <div class="polarity-bar"><div class="polarity-track"><div class="polarity-marker" style="left: 92%;"></div></div><span class="polarity-label">Pure Oscar</span></div>
    <div style="position:absolute;top:0;left:0;width:3px;height:100%;background:#8E4585;"></div>
  </div>
</div>
```

**ORGANISM** — insert after orbweave (The Abyss zone), marked Preview:
```html
<div class="engine-row right reveal" data-engine="organism">
  <div class="engine-node" style="color: #C6E377; background: #C6E377;"></div>
  <div class="engine-card" style="border-color: rgba(198,227,119,0.2);">
    <div class="engine-card-name" style="color: #C6E377;">XOrganism <span style="font-size:0.65em;font-weight:400;opacity:0.7;margin-left:0.5em;">Preview</span></div>
    <div class="engine-card-creature">The Coral Colony &mdash; Emergent, Generative</div>
    <div class="engine-card-identity">Millions of identical simple organisms following identical simple rules — and the result is one of the most complex ecosystems on Earth. Cellular automata generate synthesis parameters. Patterns no human would compose, but that feel inevitable once heard.</div>
    <div class="polarity-bar"><div class="polarity-track"><div class="polarity-marker" style="left: 92%;"></div></div><span class="polarity-label">Pure Oscar</span></div>
    <div style="position:absolute;top:0;left:0;width:3px;height:100%;background:#C6E377;"></div>
  </div>
</div>
```

---

## C) Aquarium — Lineage Section Updates

Add these species to lineage section:

**In "Sixth Generation" block** (add after fifth generation block, before bookends):
```html
<div class="lineage-gen">
  <div class="lineage-gen-title">The Reef Builders</div>
  <div class="lineage-gen-sub">Structural complexity. The ecosystem grows.</div>
  <div class="lineage-species">
    <span class="lineage-tag"><span class="lineage-dot" style="background:#BF40FF;"></span> XOblique</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#00FF41;"></span> XOptic</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#C5832B;"></span> XOcelot</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#00B4A0;"></span> XOceanic</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#87AE73;"></span> XOhm</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#7FDBCA;"></span> XOrphica</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#B8860B;"></span> XOwlfish</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#1E8B7E;"></span> XObrix</span>
  </div>
</div>

<div class="lineage-gen">
  <div class="lineage-gen-title">The Constellation Family</div>
  <div class="lineage-gen-sub">Instruments from every shore. Cultural evolution.</div>
  <div class="lineage-species">
    <span class="lineage-tag"><span class="lineage-dot" style="background:#FF8A7A;"></span> XObbligato</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#5B8A72;"></span> XOttoni</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#C9377A;"></span> XOl&eacute;</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#1B4F8A;"></span> XOsprey</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#722F37;"></span> XOsteria</span>
  </div>
</div>

<div class="lineage-gen">
  <div class="lineage-gen-title">The Deep Explorers</div>
  <div class="lineage-gen-sub">New species discovered in the darkest water.</div>
  <div class="lineage-species">
    <span class="lineage-tag"><span class="lineage-dot" style="background:#00FFB4;"></span> XOverlap</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#CC6600;"></span> XOutwit</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#7B6B8A;"></span> XOmbre</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#1B2838; border: 1px solid #4A7FA0;"></span> XOrca</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#E040FB;"></span> XOctopus</span>
  </div>
</div>

<div class="lineage-gen">
  <div class="lineage-gen-title">The Theorem Engines <span style="font-size:0.8em;opacity:0.6;">(Pi Day 2026 &mdash; Preview)</span></div>
  <div class="lineage-gen-sub">Mathematics, topology, and emergence from simple rules.</div>
  <div class="lineage-species">
    <span class="lineage-tag"><span class="lineage-dot" style="background:#8E4585;"></span> XOrbweave</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#A8D8EA;"></span> XOvertone</span>
    <span class="lineage-tag"><span class="lineage-dot" style="background:#C6E377;"></span> XOrganism</span>
  </div>
</div>
```

---

## D) Index.html — 11 Missing Engine Data Objects

Insert these 11 engine objects into the instruments array in index.html (before the closing `];`).

```javascript
{ name: "Osprey", code: "OSPREY", color: "#1B4F8A",
  desc: "Shoreline synthesis — coastal ecology across five cultural coastlines",
  wave: "noise", freq: 220, fm: 0, fmAmt: 0,
  synthType: "Coastal Ecology Synthesis",
  tagline: "The osprey hunts at the boundary — not fully water, not fully land. Five-coastline ShoreSystem captures the acoustic character and cultural rhythm of different shores. The signal ecology of the coast.",
  features: ["Five-coastline ShoreSystem with cultural rhythm data", "Shore blend parameter for coastline crossfading", "Boundary-zone synthesis at the water's edge", "Sibling engine to OSTERIA — shared ShoreSystem data"],
  coupling: ["OSTERIA", "ONSET", "OVERDUB"],
  prefix: "osprey_", format: "XOceanus native" },
{ name: "Osteria", code: "OSTERIA", color: "#722F37",
  desc: "Cultural warmth — the waterfront tavern, music from every port",
  wave: "additive", freq: 196, fm: 0.3, fmAmt: 20,
  synthType: "Cultural Warmth Synthesis",
  tagline: "The oldest gathering place — simple food, port wine, music from wherever the sailors came from. Five coastlines of cultural warmth. ShoreSystem sibling to OSPREY. Where creatures pause and share what they carry.",
  features: ["Five-coastline ShoreSystem — cultural harmony synthesis", "Porto wine warmth: Q-bass harbor hum", "Cultural rhythm presets from global coastal traditions", "Sibling engine to OSPREY — shared ShoreSystem data"],
  coupling: ["OSPREY", "OHM", "OBLONG"],
  prefix: "osteria_", format: "XOceanus native" },
{ name: "Overlap", code: "OVERLAP", color: "#00FFB4",
  desc: "Kelp forest synthesis — entangled harmonics through KnotMatrix resonance",
  wave: "fm", freq: 220, fm: 1.5, fmAmt: 60,
  synthType: "KnotMatrix Resonance",
  tagline: "A cathedral of filtered green light and entangled kelp. KnotMatrix generates topological resonances — harmonics that loop through each other like living tendrils. FDN reverb is the cathedral space inside the forest.",
  features: ["KnotMatrix topological resonance engine", "FDN (Feedback Delay Network) reverb — cathedral forest space", "Bioluminescent harmonic shimmer", "Entanglement coupling: pair with ORBWEAVE for maximum topology"],
  coupling: ["ORBWEAVE", "OPAL", "OVERDUB"],
  prefix: "olap_", format: "XOceanus native" },
{ name: "Outwit", code: "OUTWIT", color: "#CC6600",
  desc: "Chromatophore camouflage — adaptive timbre that shifts to complement its environment",
  wave: "fm", freq: 196, fm: 3.0, fmAmt: 120,
  synthType: "Adaptive Camouflage Synthesis",
  tagline: "The mimic octopus. The cuttlefish. Creatures that are never the same twice. Adaptive timbre reads spectral content of input and shifts harmonic character to complement or disguise itself. The great mimic.",
  features: ["Spectral adaptive camouflage — reads and shifts to input", "Chromatophore AM modulation for timbral skin-shifting", "Mimic modes: complement, contrast, dissolve", "Deep-water character — works best in the dark"],
  coupling: ["OCELOT", "OCTOPUS", "ORGANON"],
  prefix: "owit_", format: "XOceanus native" },
{ name: "Ombre", code: "OMBRE", color: "#7B6B8A",
  desc: "Dual narrative synthesis — the gradient between memory and forgetting",
  wave: "sine", freq: 220, fm: 0.5, fmAmt: 30,
  synthType: "Dual Narrative Synthesis",
  tagline: "The gradient where color fades. Two voices — bright and present, fading and past — blending at the thermocline. The boundary where clarity becomes uncertainty. Memory meeting forgetting in the same sound.",
  features: ["Dual-voice narrative blend with MEMORY/FORGETTING axis", "Thermocline gradient filter modeling", "Shadow depth parameter — how far into forgetting", "Perception vs reality modulation modes"],
  coupling: ["OVERDUB", "ODYSSEY", "OCEANDEEP"],
  prefix: "ombre_", format: "XOceanus native" },
{ name: "Orca", code: "ORCA", color: "#1B2838",
  desc: "Apex predator synthesis — wavetable body, echolocation, HUNT macro",
  wave: "wavetable", freq: 110, fm: 0, fmAmt: 0,
  synthType: "Apex Predator Synthesis",
  tagline: "The only predator that hunts whales. Coordinated, strategic, spatial. The HUNT macro drives filter cutoff, resonance, echolocation resonance, and sub-bass simultaneously — the most cohesive macro in the fleet.",
  features: ["HUNT macro: coordinated predator behavior in one parameter", "Echolocation resonance — click train return modeling", "Breach dynamics: wavetable body leaving the water", "Spatial separation: three-dimensional ocean geometry"],
  coupling: ["OCEANDEEP", "ONSET", "OVERBITE"],
  prefix: "orca_", format: "XOceanus native" },
{ name: "Octopus", code: "OCTOPUS", color: "#E040FB",
  desc: "Decentralized alien intelligence — 8 arm LFOs, chromatophores, ink cloud",
  wave: "wavetable", freq: 220, fm: 2.0, fmAmt: 80,
  synthType: "Distributed Intelligence Synthesis",
  tagline: "Two-thirds of neurons in the arms. Eight arm LFOs operating semi-independently. Chromatophore skin-shifting AM modulation. Ink cloud timbral weapon. EnvToMorph coupling conducts all 8 arms simultaneously from outside.",
  features: ["8 arm LFOs with semi-independent rates and phase", "Chromatophore AM modulation — timbral skin color", "Ink cloud: harmonic noise burst timbral weapon", "Fleet-leading coupling: external env conducts arm neurological tempo"],
  coupling: ["OUTWIT", "ONSET", "ORGANON"],
  prefix: "octo_", format: "XOceanus native" },
{ name: "Obrix", code: "OBRIX", color: "#1E8B7E",
  desc: "Modular brick synthesis — stackable building blocks, the coral architecture",
  wave: "additive", freq: 220, fm: 0, fmAmt: 0,
  synthType: "Modular Brick Synthesis",
  tagline: "Individual polyps stacking into architecture of staggering complexity. Oscillator bricks, filter bricks, modulation bricks, FX bricks snap together in configurable chains. Each brick alone is modest. Stacked, they become the reef.",
  features: ["Stackable synthesis bricks: oscillator, filter, mod, FX", "Configurable brick chains with independent addressability", "Reef jade architecture — maximum adaptability", "Blessing B016: Brick Independence — bricks individually addressable regardless of coupling"],
  coupling: ["OVERDUB", "OPAL", "ORACLE"],
  prefix: "obrix_", format: "XOceanus native" },
{ name: "Orbweave", code: "ORBWEAVE", color: "#8E4585",
  desc: "Topological knot coupling — bidirectional harmonic entanglement. Preview.",
  wave: "fm", freq: 196, fm: 2.5, fmAmt: 100,
  synthType: "Topological Knot Synthesis",
  tagline: "Kelp growing in fast currents ties itself into knots that cannot be undone without cutting. Topological resonances — harmonic series looping through each other inseparably. Built specifically around a coupling concept: it exists to entangle.",
  features: ["KnotMatrix topological resonance synthesis", "Bidirectional harmonic entanglement routing", "Kelp knot purple: the bruised color of pressure", "Preview engine — DSP complete, seance pending"],
  coupling: ["OVERLAP", "ODYSSEY", "OBRIX"],
  prefix: "weave_", format: "XOceanus native" },
{ name: "Overtone", code: "OVERTONE", color: "#A8D8EA",
  desc: "Continued fraction spectral synthesis — the nautilus. Preview.",
  wave: "additive", freq: 330, fm: 0, fmAmt: 0,
  synthType: "Continued Fraction Spectral Synthesis",
  tagline: "The nautilus builds its shell according to a logarithmic spiral. Harmonic series derived from continued fraction approximations of irrational numbers — pi, phi, e. Mathematically precise but never periodic.",
  features: ["Continued fraction harmonic series generation", "Irrational number spectrum: pi, phi, e, sqrt(2)", "Nautilus shell spiral parameter mapping", "Preview engine — DSP complete, seance pending"],
  coupling: ["ORACLE", "ORBITAL", "OBSCURA"],
  prefix: "over_", format: "XOceanus native" },
{ name: "Organism", code: "ORGANISM", color: "#C6E377",
  desc: "Cellular automata synthesis — coral colony emergence. Preview.",
  wave: "noise", freq: 220, fm: 0, fmAmt: 0,
  synthType: "Cellular Automata Synthesis",
  tagline: "Millions of identical simple organisms following simple rules produce one of the most complex ecosystems on Earth. Cellular automata generate synthesis parameters. Patterns no human would compose but that feel inevitable once heard.",
  features: ["Conway-family + custom aquatic cellular automata", "Emergence: synthesis parameters from rule-based evolution", "Coral colony rule presets across automata families", "Preview engine — DSP complete, seance pending"],
  coupling: ["OCELOT", "OCTOPUS", "OUROBOROS"],
  prefix: "org_", format: "XOceanus native" },
```

---

## E) Packs Page — Transcendental Vol 1 Section

Add a new section between the packs grid and the support section in packs.html:

```html
<!-- ═══════════ TRANSCENDENTAL VOL 1 ═══════════ -->
<section class="transcendental-section reveal" id="transcendental">
  <div class="section-label">Guru Bin — Transcendental</div>
  <h2>Premium preset volumes.<br><em>Deeper exploration.</em></h2>
  <p style="max-width:640px;margin:0 auto 2rem;text-align:center;color:var(--text-dim);">
    Awakening presets are free, forever. Transcendental volumes go further — 15 to 20 presets per engine, deeper sound design, each volume accompanied by a PDF lore booklet exploring the aquatic mythology behind the sounds. Released through Patreon.
  </p>

  <div class="transcendental-vol">
    <div class="vol-header">
      <div class="vol-badge">Vol 1</div>
      <div class="vol-title">The New Species</div>
      <div class="vol-subtitle">OBRIX &middot; OSTINATO &middot; OPENSKY &middot; OCEANDEEP &middot; OUIE</div>
    </div>
    <div class="vol-engines">
      <div class="vol-engine" style="border-color: rgba(30,139,126,0.3);">
        <div style="color:#1E8B7E;font-weight:600;">OBRIX</div>
        <div style="font-size:0.8rem;color:var(--text-dim);margin-top:0.25rem;">The Coral Architecture — 15 presets</div>
      </div>
      <div class="vol-engine" style="border-color: rgba(232,112,26,0.3);">
        <div style="color:#E8701A;font-weight:600;">OSTINATO</div>
        <div style="font-size:0.8rem;color:var(--text-dim);margin-top:0.25rem;">The Drum Circle — 15 presets</div>
      </div>
      <div class="vol-engine" style="border-color: rgba(255,140,0,0.3);">
        <div style="color:#FF8C00;font-weight:600;">OPENSKY</div>
        <div style="font-size:0.8rem;color:var(--text-dim);margin-top:0.25rem;">Pure feliX — 15 presets</div>
      </div>
      <div class="vol-engine" style="border-color: rgba(45,10,78,0.5);">
        <div style="color:#7B68EE;font-weight:600;">OCEANDEEP</div>
        <div style="font-size:0.8rem;color:var(--text-dim);margin-top:0.25rem;">Pure Oscar — 15 presets</div>
      </div>
      <div class="vol-engine" style="border-color: rgba(112,128,144,0.3);">
        <div style="color:#708090;font-weight:600;">OUIE</div>
        <div style="font-size:0.8rem;color:var(--text-dim);margin-top:0.25rem;">The Hammerhead — 15 presets</div>
      </div>
    </div>
    <div class="vol-includes">
      <span>&#10003; 75 premium presets</span>
      <span>&#10003; PDF lore booklet — aquatic mythology for all 5 engines</span>
      <span>&#10003; XPN format — MPC-compatible</span>
    </div>
    <!-- TODO: Replace www.patreon.com/cw/XO_OX with real Patreon URL when account is set up -->
    <a href="https://www.patreon.com/cw/XO_OX" class="transcendental-btn" target="_blank" rel="noopener">
      Support on Patreon to access Transcendental Vol 1
    </a>
    <div style="font-size:0.75rem;color:var(--text-faint);text-align:center;margin-top:0.5rem;">
      Awakening tier (free) presets always available in XOceanus directly.
    </div>
  </div>
</section>
```

Add this CSS to packs.html `<style>` block:
```css
/* Transcendental section */
.transcendental-section {
  padding: clamp(3rem, 8vw, 6rem) clamp(1.5rem, 5vw, 6rem);
  border-top: 1px solid var(--border);
  text-align: center;
}
.transcendental-section h2 {
  font-family: 'Cormorant Garamond', serif;
  font-size: clamp(2rem, 5vw, 3.2rem);
  color: var(--cream);
  font-weight: 300;
  margin-bottom: 1rem;
}
.transcendental-section h2 em { color: var(--gold); font-style: italic; }
.transcendental-vol {
  max-width: 720px;
  margin: 0 auto;
  background: var(--bg-card);
  border: 1px solid var(--border);
  border-radius: 1rem;
  padding: 2rem;
}
.vol-header {
  margin-bottom: 1.5rem;
}
.vol-badge {
  display: inline-block;
  background: var(--gold);
  color: var(--bg-deep);
  font-size: 0.7rem;
  font-weight: 700;
  letter-spacing: 0.1em;
  padding: 0.25rem 0.75rem;
  border-radius: 2rem;
  margin-bottom: 0.5rem;
}
.vol-title {
  font-family: 'Space Grotesk', sans-serif;
  font-size: 1.4rem;
  color: var(--cream);
  font-weight: 600;
}
.vol-subtitle {
  font-size: 0.85rem;
  color: var(--text-dim);
  margin-top: 0.25rem;
  letter-spacing: 0.04em;
}
.vol-engines {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
  gap: 0.75rem;
  margin-bottom: 1.5rem;
}
.vol-engine {
  background: var(--bg-surface);
  border: 1px solid;
  border-radius: 0.5rem;
  padding: 0.75rem;
  text-align: left;
}
.vol-includes {
  display: flex;
  flex-wrap: wrap;
  gap: 0.75rem 1.5rem;
  justify-content: center;
  margin-bottom: 1.5rem;
  font-size: 0.8rem;
  color: var(--text-dim);
}
.transcendental-btn {
  display: inline-block;
  padding: 0.875rem 2rem;
  background: var(--gold);
  color: var(--bg-deep);
  border-radius: 0.375rem;
  font-weight: 600;
  font-size: 0.9rem;
  text-decoration: none;
  letter-spacing: 0.02em;
  transition: opacity 0.2s;
}
.transcendental-btn:hover { opacity: 0.85; }
```

---

## F) Text Corrections — "Coming Soon" Removal

Search for any instance of "coming soon" in site HTML files. As of 2026-03-20, the following are live (not coming soon):
- OSTINATO, OPENSKY, OCEANDEEP, OUIE — live since 2026-03-18
- OBRIX — live since 2026-03-19
- All Constellation engines (OHM, ORPHICA, OBBLIGATO, OTTONI, OLE) — live since 2026-03-14
- OVERLAP, OUTWIT, OMBRE, ORCA, OCTOPUS — live since 2026-03-15

Still legitimately "preview": ORBWEAVE, OVERTONE, ORGANISM (built 2026-03-20, seance pending).

---

## G) Aquarium Meta / OG Tag Updates

Update in `<head>` of aquarium.html:
```html
<!-- OLD -->
<meta name="description" content="The Aquarium — XO_OX Water Column Atlas. Every engine is a creature. Thirty-four species in one ecosystem.">
<meta property="og:description" content="34 engines. 10,028 presets. Free and open-source. By XO_OX Designs.">
<meta name="twitter:description" content="34 engines. 10,028 presets. Free and open-source.">

<!-- NEW -->
<meta name="description" content="The Aquarium — XO_OX Water Column Atlas. Every engine is a creature. Forty-two species in one ecosystem.">
<meta property="og:description" content="42 engines. 15,200 presets. Free and open-source. By XO_OX Designs.">
<meta name="twitter:description" content="42 engines. 15,200 presets. Free and open-source.">
```

Update in `<head>` of index.html:
```html
<!-- OLD -->
<meta name="description" content="XO_OX Designs — Character synthesis. Thirty-four engines, over twenty-four hundred presets, infinite coupling. Free & open-source.">
<meta property="og:description" content="34 engines. 10,028 presets. Free and open-source. By XO_OX Designs.">
<meta name="twitter:description" content="34 engines. 10,028 presets. Free and open-source.">

<!-- NEW -->
<meta name="description" content="XO_OX Designs — Character synthesis. Forty-two engines, over fifteen thousand presets, infinite coupling. Free & open-source.">
<meta property="og:description" content="42 engines. 15,200 presets. Free and open-source. By XO_OX Designs.">
<meta name="twitter:description" content="42 engines. 15,200 presets. Free and open-source.">
```

Update JSON-LD in index.html:
```json
"description": "Multi-engine character synthesizer with forty-two engines, over fifteen thousand presets, and cross-engine coupling. Free and open-source."
```

---

## H) Aquarium Hero Subtitle

Update the hero engine count text in aquarium.html:

Find: `Thirty-four species in one ecosystem`
Replace: `Forty-two species in one ecosystem`

Find any inline `thirty-four` references in body text:
- "Every engine is a creature. The water column is their world." — no change needed
- "Where all thirty-four characters live" → "Where all forty-two characters live"

---

*XO_OX Designs | Site Content Update Plan | 2026-03-20*
