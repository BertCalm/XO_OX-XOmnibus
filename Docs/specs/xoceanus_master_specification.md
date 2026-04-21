# XOceanus — Master Specification

**Version:** 1.0
**Author:** XO_OX Designs
**Date:** 2026-03-20
**Status:** AUTHORITATIVE — This document is the single source of truth for XOceanus architecture, design, and implementation. All other spec documents are subordinate references.

---

## Document Hierarchy

This master spec consolidates 15 foundation documents. When conflicts exist between documents, this master spec wins. For deep dives, consult the detailed spec referenced in each section.

| Domain | Authoritative Source | Status |
|--------|---------------------|--------|
| Product identity & brand | `xoceanus_brand_identity_and_launch.md` | Locked |
| Visual design system | `xoceanus_technical_design_system.md` | Locked |
| Preset format & taxonomy | `xoceanus_preset_spec_for_builder.md` | Locked |
| Engine modules | `xo_mega_tool_engine_catalog.md` | Locked (rename to XOceanus) |
| Chaining architecture | `xo_mega_tool_chaining_architecture.md` | Locked |
| PlaySurface | `xo_signature_playsurface_spec.md` | Locked |
| XPN export | `xo_mega_tool_xpn_export.md` | Locked |
| XOnset integration | `xo_mega_tool_xonset_integration.md` | Locked |
| Mobile & MIDI | `xoceanus_mobile_and_midi_spec.md` | Locked |
| Repo structure | `xoceanus_repo_structure.md` | Locked |
| Development strategy | `xo_mega_tool_dev_strategy.md` | Locked |
| .xometa schema | `xometa_schema.json` | Locked |

**Superseded documents** (replaced by the specs above — do not use):
- `xo_mega_tool_preset_system.md` — superseded by `xoceanus_preset_spec_for_builder.md`
- `xo_mega_tool_visual_identity.md` — superseded by `xoceanus_technical_design_system.md`

**Living intelligence documents** (not foundation specs — authoritative for their domains):

| Document | Authority For | Notes |
|----------|--------------|-------|
| `../CLAUDE.md` | Engine registry, blessings (B001–B031), debates, architecture rules | Claude Code project guide — always current |
| `seances/seance_cross_reference.md` | Engine health, seance scores, D-violations, P0 bugs | Updated after each seance |
| `prism_sweep_final_report.md` | 12-round quality history, doctrine resolution | Immutable historical record |
| `fleet_health_2026_03_20.md` | Current fleet status snapshot | Latest: 2026-03-20 |
| `xoceanus_landscape_2026.md` | Grand survey baseline (2026-03-14) | Pre-sweep; see fleet_health for current |
| `GOVERNANCE.md` | Update policies, SLAs, naming conventions | See before any doc update |
| `MANIFEST.md` | Full documentation inventory | Discovery starting point |

---

## 1. Product Identity

### 1.1 What XOceanus Is

**XOceanus** (Latin: "for all") is a free, open-source multi-engine synthesizer platform by **XO_OX Designs**. It merges 77 character instruments — each originally a standalone product — into a unified creative environment where engines couple, collide, and mutate into sounds impossible with any single synth.

- **Not** a DAW replacement, a commercial product, or a plugin that tries to do everything
- **Is** a creative tool where cross-engine coupling is the signature feature
- **Philosophy:** Character over feature count. Every feature supports a sonic pillar.

### 1.2 Brand Values

| Value | Expression |
|-------|------------|
| **Character** | 19,500+ presets with evocative names, not "Init Patch 47" |
| **Coupling** | Cross-engine modulation is the defining differentiator |
| **Openness** | Open source, community presets, documented architecture |
| **Craft** | Each engine was a standalone instrument first |
| **Duality** | X and O, tension and release, familiar and alien |

### 1.3 Naming Convention

All XO_OX instruments follow the **XO + O-word** pattern:
- OddfeliX/OddOscar, XOverdub, XOdyssey, XOblong, XObese, XOnset
- XOceanus continues the pattern as the platform instrument

**Detail:** `xoceanus_brand_identity_and_launch.md`

---

## 2. Architecture Overview

### 2.1 Development Strategy: Interface-First Hybrid (Option C)

The `SynthEngine` interface is defined now. Each existing instrument wraps its internals behind this interface via a thin adapter layer. Standalone products continue shipping independently. The XOceanus merge is a mechanical integration step, not a rewrite.

**Core insight:** OddfeliX/OddOscar is already a miniature XOceanus — two engines, a coupling matrix, shared FX. XOceanus generalizes that pattern from 2 engines to N.

### 2.2 The SynthEngine Interface

```cpp
class SynthEngine {
public:
    virtual ~SynthEngine() = default;

    // Lifecycle
    virtual void prepare(double sampleRate, int maxBlockSize) = 0;
    virtual void releaseResources() = 0;

    // Audio
    virtual void renderBlock(juce::AudioBuffer<float>& buffer,
                            juce::MidiBuffer& midi,
                            int numSamples) = 0;

    // Coupling (the XOceanus differentiator)
    virtual float getSampleForCoupling(int channel, int sampleIndex) const = 0;
    virtual void applyCouplingInput(CouplingType type,
                                   float amount,
                                   const float* sourceBuffer,
                                   int numSamples) = 0;

    // Parameters
    virtual juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout() = 0;

    // Identity
    virtual juce::String getEngineId() const = 0;
    virtual juce::Colour getAccentColour() const = 0;
    virtual int getMaxVoices() const = 0;
};
```

### 2.3 The "Normalled Matriarch" Pattern

Inspired by the Moog Matriarch's 90 pre-wired patch points:

1. **Sounds correct by default.** Every engine pair has pre-wired coupling routes with musically useful amounts.
2. **Patching overrides defaults.** User-defined coupling replaces the normalled route for that specific connection only.
3. **Unpatch restores normal.** Removing a user route re-engages the default. No connection is ever "broken."
4. **No sound without an engine.** Empty slots produce silence and consume zero CPU.

### 2.4 Engine Registry

> **Note (2026-03-24):** The `REGISTER_ENGINE` macro shown below was removed. Engines now register
> via static boolean initializers directly in `Source/XOceanusProcessor.cpp`. The macro is preserved
> here for historical reference only.

```cpp
// Factory pattern — engines register at compile time (REMOVED — see note above)
#define REGISTER_ENGINE(EngineClass) \
    static bool registered_##EngineClass = \
        EngineRegistry::instance().registerEngine( \
            #EngineClass, []() { return std::make_unique<EngineClass>(); });

// 4 engine slots, 50ms crossfade on hot-swap
class EngineRegistry {
    std::unordered_map<std::string, EngineFactory> factories;
    std::array<std::unique_ptr<SynthEngine>, 4> activeEngines;
};
```

### 2.5 User Modes

| Mode | Target User | What's Visible |
|------|-------------|----------------|
| **Intuitive** | Preset browser, 4 macros, PlaySurface | Presets, macros, and surface only. Coupling is automatic. |
| **Advanced** | Full coupling matrix, patch cables, per-engine parameters | All routing, modulation, and parameter access. |

Toggle between modes at any time. Preset data is identical — only UI visibility changes.

**Detail:** `xo_mega_tool_chaining_architecture.md`

---

## 2.6 Design Doctrines

The 6 Doctrines are the quality contract every XOceanus engine must satisfy. They emerged empirically from the Prism Sweep (2026-03-14 to 2026-03-20) — specific failure patterns found in real instruments and codified as non-negotiable requirements. All 6 doctrines are now resolved fleet-wide across all 81 engines.

| ID | Doctrine | Requirement |
|----|----------|------------|
| D001 | Velocity Must Shape Timbre | Velocity drives filter brightness / harmonic content — **not just amplitude**. Implement via `velCutoffBoost` or equivalent wired to filter cutoff per block. |
| D002 | Modulation is the Lifeblood | Minimum: 2 LFOs (rate floor ≤ 0.01 Hz), mod wheel (CC1), aftertouch, 4 working macros, 4+ mod matrix slots. |
| D003 | The Physics IS the Synthesis | Physically-modeled engines must cite sources (papers, formulas, named models) in inline header comments. Rigor required. |
| D004 | Dead Parameters Are Broken Promises | Every declared parameter must affect audio output. Zero tolerance for parameters that do nothing. Verify with DSP trace. |
| D005 | An Engine That Cannot Breathe Is a Photograph | Every engine needs at least one LFO with rate floor ≤ 0.01 Hz. The engine must be capable of autonomous, slow evolution without performer interaction. |
| D006 | Expression Input Is Not Optional | Velocity→timbre (D001) + at least one CC: aftertouch, mod wheel (CC1), or expression pedal. OPTIC is the sole intentional exception (visual engine, no audio). |

**Authoritative source:** `CLAUDE.md` — Seance Findings section.
**Per-engine audit tool:** `/engine-health-check` skill.
**Fleet compliance status:** `Docs/seances/seance_cross_reference.md` — Doctrine Violation Summary.

---

## 3. Engine Modules

### 3.1 The 90 Engines

| Short Name | Source Instrument | Accent Color | Parameter Prefix |
|-----------|------------------|-------------|-----------------|
| **ODDFELIX** | OddfeliX | Neon Tetra Blue `#00A6D6` | `snap_` |
| **ODDOSCAR** | OddOscar | Axolotl Gill Pink `#E8839B` | `morph_` |
| **OVERDUB** | XOverdub | Olive `#6B7B3A` | `dub_` |
| **ODYSSEY** | XOdyssey | Violet `#7B2D8B` | `drift_` |
| **OBLONG** | XOblong | Amber `#E9A84A` | `bob_` |
| **OBESE** | XObese | Hot Pink `#FF1493` | `fat_` |
| **ONSET** | XOnset | Electric Blue `#0066FF` | `perc_` |
| **OVERWORLD** | XOverworld | Neon Green `#39FF14` | `ow_` |
| **OPAL** | XOpal | Lavender `#A78BFA` | `opal_` |
| **ORBITAL** | XOrbital | Warm Red `#FF6B6B` | `orb_` |
| **ORGANON** | XOrganon | Bioluminescent Cyan `#00CED1` | `organon_` |
| **OUROBOROS** | XOuroboros | Strange Attractor Red `#FF2D2D` | `ouro_` |
| **OBSIDIAN** | XObsidian | Crystal White `#E8E0D8` | `obsidian_` |
| **OVERBITE** | XOverbite | Fang White `#F0EDE8` | `poss_` |
| **ORIGAMI** | XOrigami | Vermillion Fold `#E63946` | `origami_` |
| **ORACLE** | XOracle | Prophecy Indigo `#4B0082` | `oracle_` |
| **OBSCURA** | XObscura | Daguerreotype Silver `#8A9BA8` | `obscura_` |
| **OCEANIC** | XOceanic | Phosphorescent Teal `#00B4A0` | `ocean_` |
| **OCELOT** | XOcelot | Ocelot Tawny `#C5832B` | `ocelot_` |
| **OPTIC** | XOptic | Phosphor Green `#00FF41` | `optic_` |
| **OBLIQUE** | XOblique | Prism Violet `#BF40FF` | `oblq_` |
| **OSPREY** | XOsprey | Azulejo Blue `#1B4F8A` | `osprey_` |
| **OSTERIA** | XOsteria | Porto Wine `#722F37` | `osteria_` |
| **OWLFISH** | XOwlfish | Abyssal Gold `#B8860B` | `owl_` |
| **OHM** | XOhm | Sage `#87AE73` | `ohm_` |
| **ORPHICA** | XOrphica | Siren Seafoam `#7FDBCA` | `orph_` |
| **OBBLIGATO** | XObbligato | Rascal Coral `#FF8A7A` | `obbl_` |
| **OTTONI** | XOttoni | Patina `#5B8A72` | `otto_` |
| **OLE** | XOlé | Hibiscus `#C9377A` | `ole_` |
| **OMBRE** | XOmbre | Shadow Mauve `#7B6B8A` | `ombre_` |
| **ORCA** | XOrca | Deep Ocean `#1B2838` | `orca_` |
| **OCTOPUS** | XOctopus | Chromatophore Magenta `#E040FB` | `octo_` |
| **OSTINATO** | XOstinato | Firelight Orange `#E8701A` | `osti_` |
| **OPENSKY** | XOpenSky | Sunburst `#FF8C00` | `sky_` |
| **OCEANDEEP** | XOceanDeep | Trench Violet `#2D0A4E` | `deep_` |
| **OUIE** | XOuïe | Hammerhead Steel `#708090` | `ouie_` |
| **OVERLAP** | XOverlap | Bioluminescent Cyan-Green `#00FFB4` | `olap_` |
| **OUTWIT** | XOutwit | Chromatophore Amber `#CC6600` | `owit_` |
| **OBRIX** | XObrix | Reef Jade `#1E8B7E` | `obrix_` |
| **ORBWEAVE** | XOrbweave | Kelp Knot Purple `#8E4585` | `weave_` |
| **OVERTONE** | XOvertone | Spectral Ice `#A8D8EA` | `over_` |
| **ORGANISM** | XOrganism | Emergence Lime `#C6E377` | `org_` |
| **OXBOW** | XOxbow | Oxbow Teal `#1A6B5A` | `oxb_` |
| **OWARE** | XOware | Akan Goldweight `#B5883E` | `owr_` |
| **OPERA** | XOpera | Aria Gold `#D4AF37` | `opera_` |
| **OFFERING** | XOffering | Crate Wax Yellow `#E5B80B` | `ofr_` |
| **OSMOSIS** | XOsmosis | Surface Tension Silver `#C0C0C0` | `osmo_` |
| **OTO** | XOto | Pipe Organ Ivory `#F5F0E8` | `oto_` |
| **OCTAVE** | XOctave | Hammond Teak `#8B6914` | `oct_` |
| **OLEG** | XOleg | Theatre Red `#C0392B` | `oleg_` |
| **OTIS** | XOtis | Gospel Gold `#D4A017` | `otis_` |
| **OVEN** | XOven | Steinway Ebony `#1C1C1C` | `oven_` |
| **OCHRE** | XOchre | Ochre Pigment `#CC7722` | `ochre_` |
| **OBELISK** | XObelisk | Grand Ivory `#FFFFF0` | `obel_` |
| **OPALINE** | XOpaline | Prepared Rust `#B7410E` | `opal2_` |
| **OGRE** | XOgre | Sub Bass Black `#0D0D0D` | `ogre_` |
| **OLATE** | XOlate | Fretless Walnut `#5C3317` | `olate_` |
| **OAKEN** | XOaken | Upright Oak `#9C6B30` | `oaken_` |
| **OMEGA** | XOmega | Synth Bass Blue `#003366` | `omega_` |
| **ORCHARD** | XOrchard | Orchard Blossom `#FFB7C5` | `orch_` |
| **OVERGROW** | XOvergrow | Forest Green `#228B22` | `grow_` |
| **OSIER** | XOsier | Willow Silver `#C0C8C8` | `osier_` |
| **OXALIS** | XOxalis | Wood Sorrel Lilac `#9B59B6` | `oxal_` |
| **OVERWASH** | XOverwash | Tide Foam White `#F0F8FF` | `wash_` |
| **OVERWORN** | XOverworn | Worn Felt Grey `#808080` | `worn_` |
| **OVERFLOW** | XOverflow | Deep Current Blue `#1A3A5C` | `flow_` |
| **OVERCAST** | XOvercast | Light Slate Gray `#778899` | `cast_` |
| **OASIS** | XOasis | Desert Spring Teal `#00827F` | `oas_` |
| **ODDFELLOW** | XOddfellow | Fusion Copper `#B87333` | `oddf_` |
| **ONKOLO** | XOnkolo | Spectral Amber `#FFBF00` | `onko_` |
| **OPCODE** | XOpcode | Cadet Blue `#5F9EA0` | `opco_` |
| **OXYTOCIN** | XOxytocin | Synapse Violet `#9B5DE5` | `oxy_` |
| **OUTLOOK** | XOutlook | Horizon Indigo `#4169E1` | `look_` |
| **OBIONT** | XObiont | Bioluminescent Amber `#E8A030` | `obnt_` |
| **OKEANOS** | XOkeanos | Cardamom Gold `#C49B3F` | `okan_` |
| **OUTFLOW** | XOutflow | Deep Storm Indigo `#1A1A40` | `out_` |
| **OXIDIZE** | XOxidize | Verdigris `#4A9E8E` | `oxidize_` |
| **ORRERY** | XOrrery | Orrery Brass `#C8A84B` | `orry_` |
| **OBSERVANDUM** | XObservandum | Cuttlefish Teal `#4ECDC4` | `observ_` |
| **OPSIN** | XOpsin | Bioluminescent Cyan `#00FFCC` | `ops_` |
| **OORT** | XOort | Oort Cloud Violet `#9B7FD4` | `oort_` |
| **OUTCROP** | XOutcrop | Mountain Moss `#5B6F57` | `outc_` |
| **ONEIRIC** | XOneiric | Phosphene Lavender `#B8A0FF` | `oner_` |

### 3.2 Engine Visual Identity

Each engine is an "exhibition" within the gallery. Its accent color fills the active engine panel — knob fills, active indicators, section headers. The shell (gallery walls) remains warm white.

| Engine | Visual Character | Material/Texture |
|--------|-----------------|-----------------|
| ODDFELIX + ODDOSCAR | Dual-tone panels, coupling visualization | Brushed metal |
| OVERDUB | Vintage mixing desk, VU meters, tape reels | Worn wood grain |
| ODYSSEY | Psychedelic gradients, journey visualization | Holographic sheen |
| OBLONG | Rounded corners, soft shadows, tactile | Apple Liquid Glass |
| OBESE | Bold ALL_CAPS, high contrast, industrial | Concrete |
| ONSET | Grid precision, circuit traces, LED indicators | PCB green |
| OMBRE | Duotone split panels, dissolving gradients | Watercolor wash |

### 3.3 Engine Pairing Synergies (Top 15)

| Pair | Coupling Type | What Happens |
|------|--------------|-------------|
| ODDFELIX + ODDOSCAR | Amp→Filter, Pitch Drift | The original OddfeliX/OddOscar — percussion carves pads |
| ONSET + OVERDUB | Amp→Choke, Audio→FM | Drum kit through dub FX chain |
| ODYSSEY + OBLONG | Env→Morph, LFO→Pitch | Odyssey aliens meet Oblong warmth |
| OBESE + ONSET | Audio→Ring, Amp→Choke | Industrial sample-mangling percussion |
| ODDFELIX + ONSET | Rhythm→Blend, Amp→Choke | Dual percussive engines, layered attacks |
| OVERDUB + ODYSSEY | Filter→Filter, Audio→FM | Dub delays modulating psychedelic pads |
| OBLONG + OBESE | LFO→Pitch, Env→Morph | Warm character meets heavyweight sampling |
| ODDOSCAR + OVERDUB | Amp→Filter, Audio→FM | Lush pads through dub processing |
| ODDFELIX + OVERDUB | Amp→Filter, Amp→Choke | Percussive hits into dub echo chamber |
| ODYSSEY + OBESE | Audio→Ring, Env→Morph | Alien textures from fat samples |
| OBLONG + ONSET | Env→Decay, Rhythm→Blend | Warm melodics triggered by drum patterns |
| ODDOSCAR + ODYSSEY | Filter→Filter, LFO→Pitch | Two pad engines creating shifting landscapes |
| OBESE + OVERDUB | Audio→FM, Amp→Filter | Heavy samples through vintage dub FX |
| ODDFELIX + OBESE | Amp→Choke, Audio→Ring | Percussive synthesis meets sampling |
| ONSET + ODYSSEY | Rhythm→Blend, Audio→FM | Drums driving psychedelic textures |
| OMBRE + OPAL | Audio→Wavetable, Amp→Filter | Dissolving memory feeds granular shimmer |
| OMBRE + ODYSSEY | Audio→Wavetable, Amp→Pitch | Drift pad remembers its own past selves |
| OMBRE + OBLIQUE | Audio→FM, Audio→Wavetable | Ghost echoes through prismatic delay |

### 3.4 CPU Budgets

| Configuration | Budget | Use Case |
|--------------|--------|---------|
| 1 engine | ~15-20% | Standard single-engine preset |
| 2 engines (default) | <28% | Standard coupled preset |
| 3 engines | <43% | Complex coupled preset |
| 4 engines (max) | <55% | Hero presets, studio use only |

### 3.5 Parameter Namespacing

Each engine prefixes its APVTS parameter IDs to avoid collisions:

```
snap_filterCutoff      // ODDFELIX engine
morph_wavePosition     // ODDOSCAR engine
dub_sendAmount         // OVERDUB engine
drift_journey          // ODYSSEY engine
bob_curiosity          // OBLONG engine
fat_mojo               // OBESE engine
onset_blend            // ONSET engine
ombre_blend            // OMBRE engine
```

**Detail:** `xo_mega_tool_engine_catalog.md`

---

## 4. Coupling Matrix

### 4.1 Coupling Types (15)

```cpp
enum class CouplingType {
    AmpToFilter,      // Engine A amplitude → Engine B filter cutoff
    AmpToPitch,       // Engine A amplitude → Engine B pitch
    LFOToPitch,       // Engine A LFO → Engine B pitch
    EnvToMorph,       // Engine A envelope → Engine B wavetable/morph position
    AudioToFM,        // Engine A audio → Engine B FM input
    AudioToRing,      // Engine A audio × Engine B audio
    FilterToFilter,   // Engine A filter output → Engine B filter input
    AmpToChoke,       // Engine A amplitude chokes Engine B
    RhythmToBlend,    // Engine A rhythm pattern → Engine B blend parameter
    EnvToDecay,       // Engine A envelope → Engine B decay time
    PitchToPitch,     // Engine A pitch → Engine B pitch (harmony)
    AudioToWavetable, // Engine A audio → Engine B wavetable source
    AudioToBuffer,    // Engine A audio → Engine B ring buffer (continuous stereo streaming)
                      // Designed for OPAL's grain buffer. Unlike AudioToWavetable (snapshots),
                      // streams every block into a pre-allocated circular buffer with freeze support.
    KnotTopology,     // Bidirectional irreducible coupling (B021/B022, ORBWEAVE). Both engines
                      // modulate each other's parameters. Linking number 1–5 sets depth.
    TriangularCoupling // Three-way love-triangle modulation (B040, OXYTOCIN). Intimacy/Passion/
                       // Commitment state drives cross-engine timbral evolution.
};
```

### 4.2 MegaCouplingMatrix

```cpp
class MegaCouplingMatrix {
public:
    struct CouplingRoute {
        int sourceSlot;           // 0-3
        int destSlot;             // 0-3
        CouplingType type;
        float amount;             // 0.0 to 1.0
        bool isNormalled;         // true = default, false = user-defined
    };

    // Per-sample processing for tight coupling
    void processSample(std::array<float, 4>& engineOutputs, int sampleIndex);

    // Block-level processing for efficiency
    void processBlock(std::array<juce::AudioBuffer<float>, 4>& buffers, int numSamples);
};
```

### 4.3 Normalled Defaults

17 pre-wired coupling routes are defined for common engine pairs. The coupling amount defaults to a musically useful value (typically 0.2-0.4). Users override by explicitly patching.

### 4.4 XOnset Dual Matrix

XOnset has its own internal voice coupling (8 voices cross-modulating). This runs first, before the mega-tool coupling matrix applies externally:

```
Internal: ONSET voice A → ONSET voice B (internal coupling matrix)
External: ONSET output → other engine input (MegaCouplingMatrix)
```

### 4.5 Routing Modes (3)

| Mode | Signal Flow | FX |
|------|------------|-----|
| **Independent** | Each engine → own FX → master | Per-engine FX chains |
| **Shared** | All engines → shared FX bus → master | Omnisphere pattern |
| **Chain** | Engine A → Engine B → Engine C → master | Serial audio processing |

**Detail:** `xo_mega_tool_chaining_architecture.md`, `xo_mega_tool_xonset_integration.md`

---

## 5. Render Pipeline

### 5.1 Eight-Step Audio Render (per block)

```
1. Cache all parameter values (ParamSnapshot pattern — zero-cost per-sample access)
2. Route MIDI to active engines based on channel/zone mapping
3. Render each active engine: engine.renderBlock()
4. Apply coupling matrix (per-sample for tight coupling types)
5. Route through FX based on routing mode (Independent/Shared/Chain)
6. Apply macro modulation (CHARACTER, MOVEMENT, COUPLING, SPACE)
7. Master FX chain (tape saturation → reverb → compressor) — preset-coupled, optional
8. Master output (hard limiter, DC block, denormal flush)
```

### 5.2 Master FX Rack

A post-mix effects chain that processes the combined output of all active engines. Master FX settings are **preset-coupled** — each `.xometa` stores its own master FX state. This preserves the principle that presets define complete sonic character.

**Chain order (fixed):**
1. **Tape Saturation** — `Saturator.h` — master drive/warmth/harmonic glue
2. **Space Reverb** — `LushReverb.h` — global ambience layer over all engines
3. **Bus Compressor** — `Compressor.h` — output glue, squash/pump control

**Implementation path:**
- `Source/Core/MasterFXChain.h` — thin wrapper around the 3 DSP modules
- Parameter IDs: `master_satDrive`, `master_reverbSize`, `master_reverbMix`, `master_compRatio`, `master_compAttack`, `master_compRelease`, `master_compMix`
- All 7 parameters added to the APVTS parameter tree
- `MasterFXChain::processBlock()` called in step 7 before final limiter
- UI: horizontal strip at bottom of Gallery Model (always visible, not engine-specific)
- Preset format: `.xometa` gains optional `"masterFX"` object (null = bypass all)

**Example preset snippet:**
```json
"masterFX": {
  "satDrive": 0.15,
  "reverbSize": 0.4,
  "reverbMix": 0.2,
  "compRatio": 2.5,
  "compAttack": 0.01,
  "compRelease": 0.1,
  "compMix": 0.5
}
```

**Design principle:** Master FX are not a mixing board — they are part of the instrument sound. A dub preset with 80% master reverb sounds different from the same preset at 20% reverb. The SPACE macro (M4) should map to master reverb mix by default across all presets.

### 5.2 Thread Safety Rules

- **Never** allocate memory on the audio thread
- **Never** perform blocking I/O on the audio thread
- **Never** rename stable parameter IDs after release
- All DSP lives inline in `.h` headers; `.cpp` files are one-line stubs
- Denormal protection required in all feedback/filter paths
- Export systems run on non-audio worker threads
- Engine hot-swap uses 50ms crossfade to prevent clicks

---

## 6. Visual Design System

### 6.1 The Gallery Model

XOceanus is a **gallery**. Each engine is an **exhibition**.

The platform provides a clean, neutral space — warm white walls, consistent typography, predictable navigation — that frames whatever art is on display. Engine accent colors fill only the engine panel interior. The shell never changes.

- **Gallery walls (shell):** Always warm white, always the same layout
- **Art (engine panels):** Changes completely — color, texture, control density
- **Coupling strip:** Golden corridor connecting two exhibitions
- **Preset browser:** Exhibition catalog — organized by mood, searchable by DNA

### 6.2 Dark Mode Default

XOceanus defaults to **dark mode**. This is a deliberate choice:
- Dark mode is the primary performance environment — studio, stage, and late-night sessions
- Engine accent colors are most vivid against the dark shell
- Light mode is available as a toggle for bright environments and marketing screenshots
- Marketing screenshots may be taken in either mode; the dark shell is the canonical brand presentation

> **Note (2026-03-24 revision):** This section previously stated "light mode default." That was reversed in the Gallery Model redesign. Dark mode is canonical. CLAUDE.md is the authoritative reference.

### 6.3 Platform Colors (Shell)

```
Light Mode:
  Background:         #F8F6F3   (warm paper white)
  Surface:            #FFFFFF   (cards, panels)
  Border:             #E0DCD6   (subtle warm grey)
  Text Primary:       #1A1A1A   (near-black)
  Text Secondary:     #7A7670   (warm grey)

Dark Mode:
  Background:         #1A1A1A   (near-black)
  Surface:            #2D2D2D   (cards)
  Border:             #4A4A4A
  Text Primary:       #EEEEEE   (off-white)
  Text Secondary:     #B0B0B0
```

### 6.4 Brand Constants (Never Change Between Modes)

```
XO Gold:              #E9C46A   (logo, macro knobs, active states, coupling strip)
XO Gold Light:        #F4DFA0   (hover state)
XO Gold Dark:         #C4A24E   (pressed state)
Engine accent colors: unchanged between light/dark mode
```

### 6.5 Typography

```
Display/Logo:    Space Grotesk Bold
Headings:        Inter Semi-Bold
Body/Labels:     Inter Regular
Values/Numbers:  JetBrains Mono (monospace for parameter readout)
Preset Names:    Inter Medium, 14px
Mood Badges:     Inter Semi-Bold, 10px, UPPERCASE, letter-spacing 1.5px
```

### 6.6 Type Rules

- Parameter labels: sentence case ("Filter cutoff")
- Macro labels: UPPERCASE ("CHARACTER", "MOVEMENT")
- Preset names: title case ("Velvet Morning")
- Engine names: exact brand spelling ("XOblong")

### 6.7 Layout Architecture

```
┌─────────────────────────────────────────────────────┐
│  HEADER BAR (fixed)                                 │
│  [XO_OX logo] [Preset name ◄ ►] [M1][M2][M3][M4]  │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ENGINE PANEL(S)                                    │
│  ┌──────────────┐ ┌──────────────┐                  │
│  │  Engine A     │ │  Engine B     │                 │
│  │  (accent clr) │ │  (accent clr) │                │
│  └──────────────┘ └──────────────┘                  │
│                                                     │
│  COUPLING STRIP (visible when 2+ engines active)    │
│  [coupling visualization — signal flow arrows]      │
│                                                     │
├─────────────────────────────────────────────────────┤
│  BOTTOM BAR                                         │
│  [Mood tabs] [Preset browser / DNA map / Sequencer] │
│  [PlaySurface — when in performance mode]           │
└─────────────────────────────────────────────────────┘
```

| Engines Active | Layout |
|---------------|--------|
| 1 engine | Full-width panel, all controls visible |
| 2 engines | 50/50 horizontal split, coupling strip between |
| 3 engines | 33/33/33 split, compact controls, coupling strip |

### 6.8 Spacing System

```
Base unit:    4px
xs:           4px   (inner padding)
sm:           8px   (between related controls)
md:           16px  (between control groups)
lg:           24px  (between sections)
xl:           32px  (between engine panels)
```

### 6.9 Component Specs

**Knobs:**
- Standard: 32x32px, circular track with filled arc, engine accent fill
- Macro: 40x40px, XO Gold fill, label below UPPERCASE, gold glow on hover
- Mini: 24x24px (compact 3-engine layout), label to the right

**Buttons:**
- Toggle: 6px radius, off = outline, on = accent fill + white text, 150ms ease-out
- Action: XO Gold fill for primary (Save, Export), surface fill + border for secondary

**Corner Radius:**
- Buttons: 6px, Cards/Panels: 12px, Engine frames: 16px, Modals: 20px, Knobs: 50%

### 6.10 Animation Rules

- Max 300ms for UI transitions, 1s for page transitions
- No animations during audio processing
- GPU-accelerated transforms only (translate, scale, opacity)
- Reduce motion mode available (accessibility)

**Detail:** `xoceanus_technical_design_system.md`

---

## 7. PlaySurface

### 7.1 Core Principle

> "Separate what you play from how it sounds from what happens."

The XO Signature PlaySurface decouples three concerns:
1. **What you play** — Note Input (Zone 1)
2. **How it sounds** — Orbit Path (Zone 2)
3. **What happens** — Performance Strip (Zone 3) + Performance Pads (Zone 4)

### 7.2 Four Zones

```
┌──────────────────────────────────────────────────────────┐
│  Zone 1: NOTE INPUT (3 modes)                            │
│  ┌──────────────────────┐  ┌───────────────────────────┐ │
│  │ [Pad Grid / Fretless │  │ Zone 2: ORBIT PATH        │ │
│  │  Ribbon / Drum Kit]  │  │ (XY timbre control,       │ │
│  │                      │  │  coupling visualization)  │ │
│  │  680×280px           │  │  320×280px                │ │
│  └──────────────────────┘  └───────────────────────────┘ │
│  Zone 3: PERFORMANCE STRIP (full width, 40px)            │
│  [velocity ribbon — horizontal gestural control]         │
│  Zone 4: PERFORMANCE PADS                                │
│  [FIRE] [XOSEND] [ECHO CUT] [PANIC]                     │
└──────────────────────────────────────────────────────────┘
Total: 1060×344px (desktop)
```

### 7.3 Note Input Modes (Zone 1)

| Mode | What It Does | Best For |
|------|-------------|---------|
| **Pad** | 4×4 grid, scale-locked, chord mode, velocity-sensitive | Chord progressions, melodic input |
| **Fretless** | Continuous pitch ribbon, MPE-compatible, no grid | Expressive soloing, pitch sweeps |
| **Drum** | 4×4 kit grid, per-pad voice assignment (ONSET engine) | Beat making, percussion |

### 7.4 Orbit Path (Zone 2)

XY pad where position maps to engine parameters:
- **X axis:** Parameter A (e.g., filter cutoff, wavetable position)
- **Y axis:** Parameter B (e.g., resonance, modulation depth)
- **Visual:** Fading orbit trail shows recent path (last 2 seconds)
- **Coupling viz:** When 2+ engines active, coupling flow is overlaid

### 7.5 Performance Pads (Zone 4)

| Pad | Action | Color |
|-----|--------|-------|
| **FIRE** | Momentary FX spike (drive, filter, distortion) | Engine accent |
| **XOSEND** | Route to dub send FX chain | XO Gold |
| **ECHO CUT** | Kill delay feedback (classic dub technique) | Engine accent |
| **PANIC** | All notes off, reset state | Red |

**Detail:** `xo_signature_playsurface_spec.md`

---

## 8. Preset System

### 8.1 Format: `.xometa`

Single source of truth. JSON files replacing all per-engine C++ presets and `.xocmeta` files.

**Schema:** `xometa_schema.json`

```json
{
  "schema_version": 1,
  "name": "Dub Pressure Machine",
  "mood": "Entangled",
  "engines": ["OddfeliX/OddOscar", "XOnset"],
  "author": "XO_OX",
  "version": "1.0.0",
  "description": "Kick pumps the pad engine, snare brightens the filter.",
  "tags": ["dub", "pump", "coupled"],
  "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
  "couplingIntensity": "Deep",
  "tempo": 130,
  "dna": {
    "brightness": 0.54, "warmth": 0.41,
    "movement": 0.09, "density": 0.16,
    "space": 0.35, "aggression": 0.08
  },
  "legacy": {
    "sourceInstrument": "OddfeliX/OddOscar",
    "sourceCategory": "Entangled",
    "sourcePresetName": null
  },
  "parameters": {
    "OddfeliX/OddOscar": { "xFilterCutoff": 3000, "couplingAmount": 0.8 },
    "XOnset": { "kickDecay": 0.3, "blend": 0.5 }
  },
  "coupling": {
    "pairs": [{
      "engineA": "OddfeliX/OddOscar", "engineB": "XOnset",
      "type": "Amp->Filter", "amount": 0.4
    }]
  },
  "recipe": null,
  "parentPreset": null,
  "sequencer": null,
  "surface": null
}
```

### 8.2 Key Constraints

- Name: max 30 characters, unique across entire library
- Engines: 1-3 active per preset
- `recipe` (optional): soft reference to the recipe name this preset was built on (max 25 chars). Not a dependency — preset loads without the recipe present. Enables "show presets for this recipe" filtering.
- `parentPreset` (optional): name of the factory preset this was derived from (max 30 chars). Enables lineage tracking. Flat lineage — variants of variants still reference the original factory preset.
- Tags: minimum 3
- Coupling types (enum): `Amp->Filter`, `Amp->Pitch`, `LFO->Pitch`, `Env->Morph`, `Audio->FM`, `Audio->Ring`, `Filter->Filter`, `Amp->Choke`, `Rhythm->Blend`, `Env->Decay`
- Coupling intensity: `None`, `Subtle`, `Moderate`, `Deep`

### 8.3 Mood Categories (6)

| Mood | Intent | UI Color |
|------|--------|----------|
| **Foundation** | Bass, drums, rhythmic anchors | Neon Tetra Blue `#00A6D6` |
| **Atmosphere** | Pads, drones, washes, textures | Axolotl Gill Pink `#E8839B` |
| **Entangled** | Cross-coupled, reactive — the XOceanus signature | Gold `#E9C46A` |
| **Prism** | Leads, keys, bells, melodic, articulate | Silver `#B8C4CC` |
| **Flux** | Glitchy, unstable, experimental, lo-fi | Crimson `#C0392B` |
| **Aether** | Cinematic, transcendent, ambient, spiritual | Indigo `#4A3680` |

### 8.4 Macro System (4 Unified)

| Macro | Label | Intent |
|-------|-------|--------|
| M1 | **CHARACTER** | Each engine's defining parameter |
| M2 | **MOVEMENT** | Modulation depth across all active engines |
| M3 | **COUPLING** | Cross-engine interaction strength |
| M4 | **SPACE** | Shared FX depth (reverb, delay, chorus) |

**Per-engine macro targets:**

| Macro | ODDFELIX+ODDOSCAR | OVERDUB | ODYSSEY | OBLONG | OBESE | ONSET | OMBRE |
|-------|-----------|-----|-------|-----|-----|-------|-------|
| M1 | OddfeliX+OddOscar | Send Level | JOURNEY | Curiosity | MOJO | MACHINE | BLEND+INTERFERENCE |
| M2 | Bloom | Drive | BREATHE | Oblong Mode | BELLY | PUNCH | DRIFT+REACTIVITY |
| M3 | Coupling | Delay FB | BLOOM | Smear Mix | OVERBITE | SPACE | INTERFERENCE+GRAIN |
| M4 | Delay+Rev | Reverb Mix | FRACTURE | Space Mix | GROWL | MUTATE | DECAY+FILTER |

**Golden rule:** Every macro must produce an audible change across its full range in every preset.

### 8.5 Sonic DNA (6D Fingerprint)

Every preset carries a 6-dimensional vector for similarity search, morphing, and breeding:

| Dimension | Measures |
|-----------|----------|
| **brightness** | Filter openness, harmonics, shimmer |
| **warmth** | Saturation, drift, sub, low harmonics |
| **movement** | LFO depth, modulation, tidal, curiosity |
| **density** | Osc count, unison, detuning, layers |
| **space** | Reverb, delay, chorus wet signal |
| **aggression** | Drive, distortion, resonance, fracture |

**DNA-powered features:**
- **Find Similar** — 5 nearest neighbors by Euclidean distance
- **Find Opposite** — invert vector, find closest match
- **Mood Map** — 2D PCA/t-SNE visualization of all presets
- **Preset Morphing** — parameter interpolation via single fader
- **Preset Breeding** — genetic crossover + DNA-aware mutation

### 8.6 1000-Preset Library Architecture

| Layer | Count | Description |
|-------|-------|-------------|
| Migrated Singles | 588 | Existing presets, one engine each |
| Impossible Sounds | 40 | Coupling-dependent flagships |
| Sessions | 90 | 15 production palettes x 6 presets |
| Evolution Presets | 50 | M1 sweep tells a sonic story |
| Tempo Collections | 75 | 5 genre packs x 15 sync-ready presets |
| Deconstructed Heroes | 80 | 20 heroes x 4 variants |
| Living Presets | 32 | Generative, never-repeating ambient |
| Reimagined Classics | 25 | MVP hits rebuilt with coupling |
| XO Duality | 20 | Opposing forces creating unity |
| **TOTAL** | **1000** | |

### 8.7 Naming Conventions

- 2-3 words, evocative/tactile, max 30 characters
- No technical jargon ("Amber Tide" not "LP Filter Pad")
- No numbers ("Sub Pressure" not "Bass 3")
- No duplicates across entire library
- Cross-engine names hint at both worlds ("Velvet Cosmos" = OBLONG warmth + ODYSSEY space)
- Lexicon: Nature, Sensation, Science, Culture, Objects

### 8.8 Quality Standards

Every preset must pass:
- Sounds compelling dry (before shared FX)
- Velocity response is audible
- All 4 macros produce audible change
- CPU within engine budget
- No clicks/pops during macro sweeps
- Name follows conventions
- Minimum 3 searchable tags

Cross-engine presets additionally:
- Coupling is audible
- Decoupled version still sounds good
- No feedback runaway
- CPU <28% (dual) or <55% (tri)

### 8.9 Preset Browser UX

```
[Mood tabs: Foundation | Atmosphere | Entangled | Prism | Flux | Aether]
    ↓
[Engine filter: All | OddfeliX/OddOscar | XOverdub | XOdyssey | XOblong | XOnset | Multi]
    ↓
[Search bar + tag filter]
    ↓
[Preset grid: name, mood badge, engine icon(s), DNA sparkline, favorite star]
```

**Smart Collections:**

| Collection | Filter Logic |
|-----------|-------------|
| Start Here | Curated 10 best-of |
| Hero Presets | Top 20 flagships |
| Deep Coupling | couplingIntensity == "Deep" |
| Sequencer Patterns | sequencer != null |
| New This Version | created >= currentVersion |
| User Presets | author != "XO_OX" |
| Favorites | user-starred |

### 8.10 Migration Status

| Source | Presets | Status | Tool |
|--------|---------|--------|------|
| OddfeliX/OddOscar | 114 | Done | `migrate_xocmeta_to_xometa.py` |
| XOverdub | 40 | Done | `extract_cpp_presets.py` |
| XOdyssey | 198 | Done | `extract_cpp_presets.py` |
| XOblong | 167 | Done | `extract_cpp_presets.py` |
| XObese | 52 | Pending | Manual export + migration |
| XOnset | 85 | Spec only | N/A |
| **Total in .xometa** | **519** | **All fingerprinted with DNA** | |

**Detail:** `xoceanus_preset_spec_for_builder.md`, `xometa_schema.json`

---

## 9. XPN Export (MPC Compatibility)

### 9.1 The 3 Critical XPM Rules

| Field | Must Be | Why | Previous (Broken) |
|-------|---------|-----|-------------------|
| `KeyTrack` | `True` | Samples transpose across keygroup zone | Was `False` |
| `RootNote` | `0` | MPC auto-detect convention | Was `MIDI+1` |
| Empty layer `VelStart` | `0` | Prevents ghost-triggering empty layers | Was `1` |

**These rules apply to ALL XPM files. No exceptions.**

### 9.2 Rendering Pipeline

```
.xometa preset
    ↓
Load engines + coupling → full signal chain active
    ↓
Render 21 notes per preset (every minor 3rd, C1-C6)
    ↓
Render at 3 velocity layers (32, 80, 120)
    ↓
Export as 16-bit/44.1kHz WAV
    ↓
Build XPM keygroup program per preset
    ↓
Package as .xpn expansion by bundle axis
```

### 9.3 Bundle Axes (6)

Users choose how to organize their expansion packs:

| Axis | Example Packs |
|------|--------------|
| **By Mood** | Foundation Pack, Atmosphere Pack, Flux Pack |
| **By Engine** | OddfeliX/OddOscar Essentials, XOnset Drums |
| **By Instrument Type** | Bass Collection, Pad Collection, Lead Collection |
| **By Vibe** | Lo-Fi Sessions, Dub Pressure, Ambient Worlds |
| **By Genre** | Hip-Hop Kit, Techno Machines, Ambient Tools |
| **Custom** | User-selected presets |

### 9.4 WAV Naming Convention

```
PRESET_NAME__NOTE__v1.WAV     (double underscore separator)
PRESET_NAME__C2__v1.WAV       (example: note C2, velocity layer 1)
```

### 9.5 Drum Kit Export

ONSET engine presets export as drum kits: one pad per voice, each rendered separately through the full coupling chain.

**Detail:** `xo_mega_tool_xpn_export.md`

---

## 10. Mobile & MIDI

### 10.1 Mobile Targets

- **iPhone:** AUv3 + Standalone
- **iPad:** AUv3 + Standalone
- **Framework:** JUCE 8, shared codebase with macOS, conditional compilation via `XO_MOBILE=1`

### 10.2 Mobile Layout Philosophy

Touch is the superior input for the PlaySurface. Mobile is where the PlaySurface comes home.

| Form Factor | Gallery Metaphor |
|-------------|-----------------|
| Desktop (macOS) | Full museum — all exhibitions visible |
| iPad | Gallery wing — most zones visible, tight but complete |
| iPhone Portrait | Single room — one exhibition at a time, swipe between rooms |
| iPhone Landscape | Performance hall — maximum stage (PlaySurface full-width) |

### 10.3 iPhone Layouts

**Portrait (390x844pt):**
```
┌────────────────────┐
│ Header: preset ◄ ► │  44pt
├────────────────────┤
│                    │
│  Active Zone       │  ~560pt (swipe to change)
│  (Engine / Surface │
│   / Browser)       │
│                    │
├────────────────────┤
│ [M1] [M2] [M3] [M4]│  64pt  (macros always visible)
├────────────────────┤
│ Zone selector tabs │  44pt
└────────────────────┘
```

**Landscape (844x390pt):**
```
┌──────────────────────────────────────┐
│ [≡]  Preset Name  [M1][M2][M3][M4]  │  36pt
├──────────────────────────────────────┤
│                                      │
│         PlaySurface (full width)     │  ~310pt
│    [Note Input]  [Orbit]  [Pads]     │
│                                      │
└──────────────────────────────────────┘
```

### 10.4 iPad Layouts

```
┌──────────────────────────────────────────────────────┐
│ Header: [XO_OX] Preset ◄ ► [M1][M2][M3][M4] [mode] │  48pt
├──────────────────────────────────────────────────────┤
│                                                      │
│  ┌────────────────────┐  ┌────────────────────┐      │
│  │ Engine A Panel     │  │ Engine B Panel     │      │  ~500pt
│  │ (full controls)    │  │ (full controls)    │      │
│  └────────────────────┘  └────────────────────┘      │
│  [Coupling Strip]                                    │
│                                                      │
├──────────────────────────────────────────────────────┤
│ PlaySurface: [Note Input] [Orbit Path] [Pads]        │  ~260pt
└──────────────────────────────────────────────────────┘
```

### 10.5 Sensor Integration (Mobile)

| Sensor | Musical Mapping |
|--------|----------------|
| **Touch pressure** | Velocity / aftertouch |
| **Accelerometer (tilt X)** | Filter cutoff modulation |
| **Accelerometer (tilt Y)** | Effect depth modulation |
| **Gyroscope (rotation)** | Orbit path position |
| **Haptic feedback** | Note onset, pad trigger, coupling threshold |

### 10.6 MIDI Controller Support

**MIDI Learn:** Long-press any knob → move a MIDI CC → mapping saved. Per-preset or global.

**MPE Support:**
- Per-note pitch bend → Fretless mode pitch
- Pressure → velocity/aftertouch
- Slide (CC74) → filter cutoff or wavetable position

**Controller Presets (6):**

| Preset | Target Controller |
|--------|------------------|
| Generic CC | Any MIDI controller |
| Akai MPC | MPC pads + knobs |
| Ableton Push | Push 2/3 grid + encoders |
| Novation Launchpad | Grid-focused |
| Arturia KeyLab | Keys + faders + pads |
| MPE Controller | Linnstrument, Seaboard, Sensel |

**Detail:** `xoceanus_mobile_and_midi_spec.md`

---

## 11. Repository Structure

### 11.1 Monorepo Layout

```
XOceanus/
├── CLAUDE.md                     # Agent project guide
├── CMakeLists.txt                # Top-level build
├── Source/
│   ├── Core/
│   │   ├── SynthEngine.h         # THE interface
│   │   ├── EngineRegistry.h      # Factory + slot management
│   │   ├── MegaCouplingMatrix.h  # Cross-engine modulation
│   │   ├── PresetManager.h       # .xometa loading/saving
│   │   ├── MacroSystem.h         # 4 unified macros
│   │   └── SharedTransport.h     # Clock sync for sequencers
│   ├── Engines/
│   │   ├── Snap/                 # OddfeliX adapter
│   │   ├── Morph/                # OddOscar adapter
│   │   ├── Dub/                  # XOverdub adapter
│   │   ├── Drift/                # XOdyssey adapter
│   │   ├── Bob/                  # XOblong adapter
│   │   ├── Fat/                  # XObese adapter
│   │   └── Onset/                # XOnset adapter
│   ├── DSP/
│   │   ├── CytomicSVF.h          # Shared Cytomic SVF filter
│   │   ├── PolyBLEP.h            # Shared PolyBLEP oscillator
│   │   ├── FastMath.h            # Shared fast math (exp, tanh)
│   │   └── Effects/              # Shared FX modules
│   ├── UI/
│   │   ├── XOceanusLookAndFeel.h # Gallery Model implementation
│   │   ├── PlaySurface/          # PlaySurface components
│   │   ├── CouplingStrip/        # Coupling visualization
│   │   ├── PresetBrowser/        # Mood-based browser
│   │   └── Mobile/               # iOS-specific layouts
│   ├── Export/
│   │   ├── XPNExporter.h         # MPC export pipeline
│   │   └── WAVRenderer.h         # Offline rendering
│   └── XOceanusProcessor.h/.cpp  # Main processor
├── Presets/
│   └── XOceanus/
│       ├── Foundation/           # .xometa files by mood
│       ├── Atmosphere/
│       ├── Entangled/
│       ├── Prism/
│       ├── Flux/
│       └── Aether/
├── Libs/
│   └── JUCE/                    # JUCE 8 (submodule or clone)
├── Tools/
│   ├── compute_preset_dna.py     # DNA fingerprint generator
│   ├── breed_presets.py          # Genetic preset breeding
│   ├── migrate_xocmeta_to_xometa.py
│   └── xpn_export.py            # MPC expansion packager
├── Tests/
│   ├── DSPTests/                 # Unit tests for DSP modules
│   ├── PresetTests/              # Preset validation
│   └── CouplingTests/            # Coupling matrix tests
└── Docs/
    └── *.md                      # All specification documents
```

### 11.2 Build System

```bash
# macOS build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# iOS build
cmake -B build-ios -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=ios-toolchain.cmake \
  -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=YOUR_TEAM_ID
cmake --build build-ios --config Release
```

**Targets:** AU + Standalone (macOS), AUv3 + Standalone (iOS), VST3 (v2)

### 11.3 Migration Plan (5 Phases)

| Phase | Action | Duration |
|-------|--------|----------|
| 1 | Create XOceanus repo, scaffold structure, copy shared DSP | Week 1 |
| 2 | Implement SynthEngine interface, wrap OddfeliX/OddOscar (ODDFELIX+ODDOSCAR) | Weeks 2-4 |
| 3 | Wrap XOverdub (OVERDUB), implement MegaCouplingMatrix | Weeks 5-7 |
| 4 | Wrap remaining engines (ODYSSEY, OBLONG, OBESE) | Weeks 8-11 |
| 5 | Build ONSET, PlaySurface, preset browser, mobile layouts | Weeks 12-14 |

**Existing repos are archived, not deleted.** Each becomes read-only with a README pointing to XOceanus.

**Detail:** `xoceanus_repo_structure.md`

---

## 12. Development Roadmap

### 12.1 MVP Definition (Week 14)

The MVP ships with:
- 2 engines: ODDFELIX + ODDOSCAR (OddfeliX/OddOscar) + OVERDUB (XOverdub)
- MegaCouplingMatrix with 3 coupling types
- PlaySurface (Pad mode only)
- Preset browser with mood tabs
- 154 migrated presets (114 OddfeliX/OddOscar + 40 XOverdub)
- macOS AU + Standalone
- Light mode UI with Gallery Model

### 12.2 v1.0 Target

- All 81 engines wrapped and integrated
- Full coupling matrix (15 types)
- PlaySurface (all 3 modes)
- 17,000+ factory presets with DNA fingerprints
- Preset morphing and breeding
- XPN export
- macOS + iOS (AUv3)
- Light + dark mode
- MIDI Learn + MPE support

### 12.3 Post-v1.0

- VST3 format
- Community preset sharing
- Wavetable file loading for ODYSSEY engine
- Windows port
- Additional engine modules

---

## 13. Shared DSP Library

Modules used by multiple engines, extracted to `Source/DSP/`:

| Module | Used By | Purpose |
|--------|---------|---------|
| `CytomicSVF.h` | ODDFELIX, ODDOSCAR, ODYSSEY, OBLONG, OBESE, OMBRE | State-variable filter (LP/HP/BP) |
| `PolyBLEP.h` | ODDFELIX, ONSET | Band-limited oscillators |
| `FastMath.h` | All | Fast exp, tanh, sin approximations |
| `WavetableOscillator.h` | ODDOSCAR, ODYSSEY, OBLONG | Wavetable rendering |
| `DubDelay.h` | OVERDUB, ODDFELIX+ODDOSCAR | Ping-pong delay with feedback |
| `LushReverb.h` | OVERDUB, ODYSSEY | Algorithmic reverb |
| `Compressor.h` | OVERDUB, OBESE | Dynamics processing |
| `Saturator.h` | OBESE, ODYSSEY, OVERDUB | Tube/tape/digital saturation |

---

## 14. Accessibility

### 14.1 Requirements

- All knobs keyboard-navigable (arrow keys for value, tab for focus)
- Color is never the only indicator — always paired with shape or text
- Minimum contrast ratio 4.5:1 for body text, 3:1 for large text
- Screen reader labels for all interactive elements
- Focus ring: 2px XO Gold outline on all focusable elements
- Reduce motion mode: all animations instant

### 14.2 Color Blindness

- Mood colors tested against deuteranopia, protanopia, tritanopia
- Foundation (terracotta) and Flux (crimson) distinguished by luminance
- All mood badges include text labels, never color-only

---

## 15. File Locations Reference

### 15.1 Specifications

| Document | Path |
|----------|------|
| **This master spec** | `synth_playbook/docs/xoceanus_master_specification.md` |
| Brand identity | `synth_playbook/docs/xoceanus_brand_identity_and_launch.md` |
| Technical design system | `synth_playbook/docs/xoceanus_technical_design_system.md` |
| Preset system (builder) | `synth_playbook/docs/xoceanus_preset_spec_for_builder.md` |
| .xometa schema | `synth_playbook/docs/xometa_schema.json` |
| .xometa examples | `synth_playbook/docs/xometa_examples.json` |
| Engine catalog | `synth_playbook/docs/xo_mega_tool_engine_catalog.md` |
| Chaining architecture | `synth_playbook/docs/xo_mega_tool_chaining_architecture.md` |
| PlaySurface | `synth_playbook/docs/xo_signature_playsurface_spec.md` |
| XPN export | `synth_playbook/docs/xo_mega_tool_xpn_export.md` |
| XOnset integration | `synth_playbook/docs/xo_mega_tool_xonset_integration.md` |
| Mobile & MIDI | `synth_playbook/docs/xoceanus_mobile_and_midi_spec.md` |
| Repo structure | `synth_playbook/docs/xoceanus_repo_structure.md` |
| Development strategy | `synth_playbook/docs/xo_mega_tool_dev_strategy.md` |
| Feasibility study | `synth_playbook/docs/xo_mega_tool_feasibility.md` |
| XOnset engine spec | `synth_playbook/docs/xonset_percussive_engine_spec.md` |

### 15.2 Tools

| Tool | Path |
|------|------|
| DNA fingerprint generator | `tools/compute_preset_dna.py` |
| Preset breeding | `tools/breed_presets.py` |
| OddfeliX/OddOscar migrator | `tools/migrate_xocmeta_to_xometa.py` |
| C++ preset extractor | `tools/extract_cpp_presets.py` |
| XPM fixer | `tools/fix_xobese_xpms.py` |
| XPN packager | `tools/xpn_export.py` (from XOdyssey, needs update) |

### 15.3 Presets

| Asset | Path |
|-------|------|
| Migrated .xometa presets | `Presets/XOceanus/{mood}/*.xometa` |
| Legacy .xocmeta presets | `Presets/Factory/*.xocmeta` |

---

## 16. Glossary

| Term | Definition |
|------|-----------|
| **Coupling** | Cross-engine modulation where one engine's output controls another engine's parameters |
| **DNA** | 6-dimensional sonic fingerprint (brightness, warmth, movement, density, space, aggression) |
| **Exhibition** | An engine's visual presentation within the Gallery Model |
| **Gallery** | The neutral shell (warm white) that frames engine panels |
| **Mood** | One of 6 browsing categories (Foundation, Atmosphere, Entangled, Prism, Flux, Aether) |
| **Normalled** | Pre-wired default coupling route (can be overridden by explicit patching) |
| **PlaySurface** | Unified playing interface with 4 zones and 3 note input modes |
| **XO Gold** | `#E9C46A` — the brand color constant, used for coupling strip, macros, active states |
| **.xometa** | JSON preset format (replaces .xocmeta and C++ preset definitions) |
| **.xpn** | MPC expansion pack format |
| **.xpm** | MPC keygroup program format |

---

## 17. Engine Addenda (Later Additions)

### 17.1 OBRIX — Modular Brick Synth (Added 2026-03-19, Wave 1 Complete)

**Identity:** Baby brother of XOceanus — a living reef that grows over time. Teaching instrument that ships as a Standard Brick Kit and grows via periodic brick drops (5-40 LOC each). Gallery code: OBRIX. Accent: Reef Jade `#1E8B7E`. Prefix: `obrix_`.

**Architectural Core — The Constructive Collision:**
- Proc 1 processes Source 1 independently (Src1 → Proc1 → mix)
- Proc 2 processes Source 2 independently (Src2 → Proc2 → mix)
- Proc 3 is a post-mix insert (wavefolder/ring mod/filter after sources merge)
- This split routing is OBRIX's defining identity — not a subtractive synth with options but a construction set for timbral collisions

**Wave 1 Status (2026-03-19, commit de89586):** COMPLETE
- 55 parameters (expanded from 34 in earlier revision)
- PolyBLEP anti-aliasing on Saw/Square/Triangle/Pulse; Lo-Fi Saw (type 8) intentionally naive
- All 4 modulators wired with routes to pitch, cutoff, amplitude, pulse-width, fold-depth
- 3 FX slots wired in series (FX1→FX2→FX3); Delay, Reverb, Chorus, BitCrush available
- Expression inputs: pitch bend (`obrix_pitchBendRange` 2–24 st) + portamento (`obrix_glideTime`)
- Coupling output fix: returns real `lastSampleL/R`; channel 2 carries brick complexity ratio
- Macro remap: CHARACTER → cutoff + fold (exponential); MOVEMENT → LFO scaling + stereo detune
- FLASH gesture system: `obrix_gestureType` + `obrix_flashTrigger` for one-shot visual-sonic bursts
- Reverb damping tracks param (Guru Bin fix); all DSP profiler issues resolved (4 bugs fixed)

**Seance Verdict:** 6.8/10 current → 9.8 target | See `Docs/seances/obrix_seance_verdict.md`

**Roadmap:** 4 waves → see `Docs/specs/obrix_flagship_roadmap.md`
**Brick Drop Strategy:** See `Docs/specs/obrix_brick_drop_strategy.md`

**Governance:** All 6 doctrines D001-D006 PASS. New Blessing B016: Brick Independence.

**⚠️ Preset Constraint:** Do NOT author OBRIX presets until after Wave 3 param freeze. Waves 1-3 add ~18 new parameters that would break presets written against the Wave 1 schema. Factory presets (target: 150 across all moods per Lesson/Genre/Place taxonomy) are Wave 4 work.

---

*This document is the single source of truth for XOceanus. All implementation should reference this spec. For deep dives into specific domains, follow the references to the detailed specification documents listed in Section 15.*
