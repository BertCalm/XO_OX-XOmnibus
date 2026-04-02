# Oxport V2 Feature Backlog — Prioritized

**Status:** Living Backlog
**Date:** 2026-03-16
**Author:** XO_OX Designs
**Scope:** Post-V1 feature roadmap for the Oxport XPN pipeline after 10+ R&D rounds

---

## Context

After 10+ R&D rounds covering conventional kit tools, unconventional input sources, live performance formats,
educational formats, native MPC expansion design, and novel generative architectures, the Oxport pipeline
has accumulated a deep backlog. This document organizes every known feature by priority tier, with
implementation complexity estimates and owner assignments.

**Owner legend:**
- **Ruby** — docs, spec writing, test fixtures
- **Rex** — XPN format compliance, MPC rule enforcement
- **Vibe** — sound design validation, preset listening pass
- **Hex** — experimental / hardware-forward features (Pi 5 + Elk Audio OS path)

**Complexity legend:** S = hours / M = 1-2 days / L = 3-5 days / XL = 1-2 weeks

---

## P0 — Launch Blockers (must ship before April 2026)

These items gate the economic viability of any pack release. Nothing in P1–P3 matters if P0 is incomplete.

---

### P0.1 — Fleet Render Automation: `renderNoteToWav()` in XPNExporter.h

**What it is:** The single missing ~40 lines of C++ that converts `XPNExporter`'s placeholder silence
generator into a real processor-driven renderer. The scaffolding (buffer allocation, hold/tail timing,
WAV writing, normalization, parallel batch dispatch) is complete. The processor instantiation, MIDI
dispatch, and `processBlock()` loop are the gap.

**Why it's P0:** Without automated rendering, every pack requires manual sample recording — 2+ minutes
per WAV, 32+ WAVs per preset, 100+ presets per engine. No pack is economically viable at that rate.
With fleet render, a full engine ships in under an hour of unattended compute.

**Blocking dependencies:**
- `XOceanusEngine` shared CMake target must be extracted from the plugin target (needed to link `fleet-render` CLI without a display)
- `fleet-render` CLI `Main.cpp` (see `fleet_render_automation_spec.md` Phase 2)
- `xpn_render_spec.py` JSON output format must match `fleet-render --spec` input schema

**Complexity:** M (the code path is fully designed; see `fleet_render_automation_spec.md`)

**Owner:** Rex (format compliance) + Ruby (spec validation)

---

### P0.2 — `xpn_bundle_builder.py`: Extended Source Routing

**What it is:** `xpn_bundle_builder.py` currently handles the original tool suite (velocity expander,
cycle/smart kit tools, drum export). After 10+ R&D rounds, the tool ecosystem has expanded to include
generators that produce WAV files through completely different mechanisms. The bundle builder must
recognize and route these as valid pack input sources:

| Tool | Source Type | Input Format |
|------|-------------|--------------|
| `xpn_braille_kit.py` | Tactile notation | Braille cell → rhythm pattern → WAV |
| `xpn_climate_kit.py` | Environmental data | NOAA CSV or API JSON → weather WAV |
| `xpn_ca_kit.py` | Cellular automata | Wolfram rule + grid → WAV |
| `xpn_transit_kit.py` | Transit data | GTFS stop sequence → rhythm WAV |
| `xpn_lsystem_kit.py` | L-system generative | Turtle grammar → melodic WAV |
| `xpn_attractor_kit.py` | Strange attractors | ODE solver → chaotic WAV |
| `xpn_entropy_kit.py` | Information theory | Shannon entropy curve → dynamic WAV |
| `xpn_pendulum_kit.py` | Physical simulation | Double pendulum → rhythm WAV |
| `xpn_turbulence_kit.py` | Fluid dynamics | Navier-Stokes → texture WAV |
| `xpn_gene_kit.py` | Genomic data | Codon table → melodic WAV |
| `xpn_seismograph_kit.py` | Seismic data | USGS waveform → percussion WAV |
| `xpn_poetry_kit.py` | Poetic meter | Prosody scan → rhythmic WAV |
| `xpn_git_kit.py` | Repository history | Commit diff entropy → glitch WAV |

**Required changes to `xpn_bundle_builder.py`:**
1. Add a `SOURCE_REGISTRY` dict mapping source names to their tool entry points and WAV output schemas
2. Validate that each source's output passes `xpn_qa_checker.py` before packaging
3. Allow mixed-source bundles (e.g., combine `climate` kicks with `ca` hats in one `.xpn`)

**Blocking dependencies:** Each unconventional tool must be stable before it can be registered
(some are still R&D-only at time of writing)

**Complexity:** M

**Owner:** Rex (format compliance), Ruby (source registry spec)

---

### P0.3 — `oxport.py`: `--source` Flag for Unconventional Inputs

**What it is:** `oxport.py` currently assumes inputs come from rendered `.xometa` presets.
The `--source` flag routes any valid WAV-producing source through the main pipeline stages
(categorize → expand → export → cover_art → package) without requiring a prior preset render.

**CLI spec:**
```bash
# Route a climate kit output through the full Oxport pipeline
oxport.py run --source climate --climate-data ./noaa_2026.csv --engine Onset --output-dir ./climate_pack/

# Route a git kit output (repository as drum machine)
oxport.py run --source git --repo-path ~/Documents/GitHub/XO_OX-XOceanus --output-dir ./git_pack/

# Route pre-rendered WAVs from any unconventional tool
oxport.py run --source raw-wavs --wavs-dir ./my_custom_wavs/ --output-dir ./custom_pack/
```

**Implementation notes:**
- `--source` values map to `SOURCE_REGISTRY` entries defined in P0.2
- When `--source` is provided, the `render` and `render_spec` stages are skipped; pipeline begins at `categorize`
- `--source raw-wavs` is the escape hatch for any source not in the registry

**Blocking dependencies:** P0.2 `SOURCE_REGISTRY` must exist first

**Complexity:** S (flag parsing + stage bypass logic only)

**Owner:** Ruby (docs + test fixtures), Rex (stage routing validation)

---

## P1 — Ship within 60 days

These features have completed R&D (specs exist) and are ready for implementation. Each has a known
output format and a clear test harness.

---

### P1.1 — `xpn_deconstruction_builder.py`

**Spec reference:** `xpn_educational_format_rnd.md` — Deconstruction Pack format

**What it is:** Builds educational "how this preset works" XPN packs. For each preset, generates
a sequence of WAVs that progressively reveal the synthesis chain: dry oscillator → + filter → + envelope
→ + modulation → + FX → + coupling. The MPC user loads the pack, plays the pads in order, and hears
the preset being built up layer by layer.

**Complexity:** L (novel XPM structure; requires per-stage partial render from fleet-render)

**Blocking dependencies:** P0.1 (fleet render must support partial-chain rendering with selective
engine bypass)

**Owner:** Vibe (layer sequence design), Rex (XPM format), Ruby (educational copy)

---

### P1.2 — `xpn_tutorial_builder.py`

**Spec reference:** `xpn_educational_format_rnd.md` — Tutorial Pack format

**What it is:** Builds curriculum-style XPN packs where each pad is a lesson step. Designed for
producers learning synthesis fundamentals using XO_OX sounds as the medium. Covers oscillators,
filters, envelopes, modulation, effects. Each pad contains a pre-rendered example WAV + embedded
metadata description of what parameter was changed.

**Complexity:** M (mostly metadata and naming logic; WAVs come from fleet render)

**Blocking dependencies:** P0.1

**Owner:** Ruby (curriculum structure), Vibe (sound selection)

---

### P1.3 — `xpn_setlist_builder.py`

**Spec reference:** `live_performance_xpn_rnd.md` — Setlist Pack format

**What it is:** Builds performance-ready XPN packs organized by set structure (intro / build / peak /
breakdown / outro). Each MPC bank maps to a setlist section. WAVs are selected and categorized by
energy level, duration, and harmonic key. Designed so a live performer can navigate banks in real time
without breaking flow.

**Complexity:** M

**Blocking dependencies:** P0.1

**Owner:** Vibe (energy curve design), Rex (bank layout), Hex (hardware performance context)

---

### P1.4 — `xpn_evolution_builder.py`

**Spec reference:** `live_performance_xpn_rnd.md` — Evolution Pack format

**What it is:** Builds XPN packs where each bank represents a temporal evolution of a sound — from
its simplest form to its most developed form. Enables performers to "grow" a sound across a set by
moving between banks. Pairs naturally with XOceanus coupling presets where macros shift timbre over time.

**Complexity:** M

**Blocking dependencies:** P0.1

**Owner:** Vibe (evolution arc design), Rex (bank structure)

---

### P1.5 — `xpn_stems_checker.py`

**Spec reference:** `mpce_native_pack_design.md` — Stems validation section

**What it is:** Validates that stem WAVs included in an XPN pack sum correctly to the mixed reference.
Catches phase cancellation, clipping, level imbalance, and missing elements. Runs as a pre-package
quality gate in the `cover_art` → `package` stage transition.

**Complexity:** S (signal analysis utility; no new XPM logic required)

**Blocking dependencies:** None (standalone validator)

**Owner:** Rex (format spec), Ruby (test fixture generation)

---

### P1.6 — `--venue-size` flag for `xpn_drum_export.py`

**What it is:** Adjusts EQ, compression, and reverb tail durations in exported drum WAVs to suit
the target venue. Values: `intimate` / `club` / `arena` / `festival`. Each profile applies a
post-processing chain before WAV write: low-shelf boost for intimates (reinforces transient warmth),
sub rolloff for arena (protects against PA resonance), extended reverb tails for festival.

**Complexity:** S

**Blocking dependencies:** None

**Owner:** Vibe (profile tuning), Rex (format validation)

---

### P1.7 — `--performance-optimized` flag for `xpn_drum_export.py`

**What it is:** When set, enforces that all drum WAVs in the pack meet MPC live performance latency
requirements: pre-attack trimmed to < 3ms, files ≤ 48 kHz / 24-bit stereo (no 96 kHz upsamples),
file size ≤ 5 MB per sample. Also disables round-robin variants (uses only the strongest take per pad)
to reduce MPC RAM overhead during live use.

**Complexity:** S

**Blocking dependencies:** None

**Owner:** Rex (format + latency validation), Hex (live performance hardware context)

---

## P2 — High Value, No Hard Deadline

These features have clear value but no blocking release dependency. Ship when bandwidth allows.

---

### P2.1 — `xpn_gravitational_wave_kit.py`

**What it is:** Uses LIGO/VIRGO public gravitational wave strain data (`.hdf5` format) to derive
kit timing and spectral content. The chirp signal of a black hole merger maps to a percussion
transient sequence; the ringdown maps to reverb tail shape. Aesthetically in line with the XO_OX
physics-forward design philosophy (see D003).

**Complexity:** L (HDF5 parsing, chirp-to-MIDI mapping algorithm, spectral shaping DSP)

**Owner:** Hex (physics modeling), Rex (XPM output)

---

### P2.2 — `xpn_climate_kit.py` Real API Integration

**What it is:** The R&D prototype uses static NOAA CSV files. The production version adds optional
live API mode: call the NOAA Climate Data Online API at build time to fetch current or historical
weather data for any location. The pack therefore becomes a time- and place-stamped artifact.

**Complexity:** M (REST client + API key management + rate limiting)

**Blocking dependencies:** P0.2 climate source routing

**Owner:** Ruby (API integration spec), Rex (output validation)

---

### P2.3 — `xpn_monster_rancher.py`: Room Mode Extraction

**What it is:** The existing `xpn_monster_rancher.py` generates kits from ambient recordings by
treating recording artifacts as synthesis fodder. The room mode extraction extension analyzes an
ambient WAV for its dominant resonant frequencies (the standing waves of the room) and uses those
frequencies to tune the kit's pitched elements. Every kit would thus be uniquely fingerprinted to
its recording environment.

**Complexity:** L (FFT-based room mode analysis, frequency-to-MIDI-note mapping, multi-point sweep
averaging to separate room modes from instrument tones)

**Owner:** Hex (DSP), Vibe (sound design validation)

---

### P2.4 — MPCe Quad-Corner Tier 2: `<PadCornerAssignment>` XML Stub

**What it is:** The MPC Expression hardware exposes per-corner pressure zones on each pad. Akai has
not yet published the official XPN schema extension for `<PadCornerAssignment>`. The stub implementation
comments out the XML block but generates the correct structure so it can be uncommented and validated
the moment Akai releases documentation.

**Spec reference:** `mpce_native_pack_design.md` — Quad-Corner section

**Complexity:** S (XML template only; no logic change)

**Blocking dependencies:** Akai documentation release (external dependency, unknown timeline)

**Owner:** Rex (format tracking + XML stub), Ruby (changelog note when unblocked)

---

### P2.5 — Parallel Rendering Flag: `--workers N` for All Kit Tools

**What it is:** All kit generation tools (`xpn_drum_export.py`, `xpn_keygroup_export.py`,
`xpn_velocity_expander.py`, etc.) currently render sequentially. Adding `--workers N` enables
multiprocessing via Python's `concurrent.futures.ProcessPoolExecutor`. Each worker handles one
kit slot independently.

**Complexity:** S per tool (the pattern is identical across tools; write once, apply via shared
utility function)

**Owner:** Ruby (implementation template), Rex (output determinism validation — parallel workers
must produce byte-identical WAVs to sequential)

---

## P3 — Research Phase

These features require external dependencies, unproven technology, or significant scope. Treat as
horizon items until a specific release goal is attached.

---

### P3.1 — Neural Timbre Transfer

**What it is:** Use a PyTorch-based timbre transfer model (RAVE or similar) to apply the spectral
character of one XOceanus engine to WAVs rendered by another. Output: a kit that "sounds like Onset
played through the timbral fingerprint of Opal."

**Complexity:** XL

**Blocking dependencies:** PyTorch (large external dependency); GPU recommended for practical
throughput; requires trained model per engine pair (34 engines = up to 1,122 pairs)

**Owner:** Hex (architecture), Vibe (quality gate — model output must pass listening test)

---

### P3.2 — Real-Time Generative Kit via WebSocket API

**What it is:** A WebSocket server wrapping the kit generation tools. An MPC or external DAW
connects and requests a new kit mid-session. The server generates WAVs on demand and streams them
back as a ready-to-load XPN archive. Enables "infinite kit" live performance mode.

**Complexity:** XL

**Blocking dependencies:** P0.1 (fleet render must be fast enough for interactive latency —
target < 5 seconds for a 16-pad kit); network protocol design; MPC client-side integration
(unknown if Akai supports live XPN injection)

**Owner:** Hex (server + protocol), Rex (XPN streaming format)

---

### P3.3 — Community Submission Pipeline (Patreon API Integration)

**What it is:** Patreon members submit WAV samples or `.xometa` presets via a web form. A server-side
Oxport pipeline validates, categorizes, and packages community contributions into monthly community
XPN packs. Revenue model: community packs are a Patreon-tier benefit.

**Complexity:** XL

**Blocking dependencies:** Patreon API access (currently placeholder `www.patreon.com/cw/XO_OX`);
server infrastructure; legal/IP review of community submissions; XPN quality gate automation
must be production-hardened before accepting untrusted inputs

**Owner:** Ruby (submission spec + legal copy), Rex (input validation hardening)

---

### P3.4 — `xpn_gene_kit.py` Protein Folding Extension

**What it is:** The existing `xpn_gene_kit.py` maps DNA codons to pitch sequences. The protein
folding extension adds a second layer: uses AlphaFold2 confidence scores (pLDDT) to drive velocity
dynamics. High-confidence residues (pLDDT > 90) produce strong, consistent hits; low-confidence
disordered regions produce ghost notes and stochastic velocity. The result is a kit whose
expressiveness is determined by the structural certainty of a real protein.

**Complexity:** XL

**Blocking dependencies:** AlphaFold2 Python API (large dependency); requires protein structure
database access; pLDDT-to-velocity mapping algorithm needs empirical tuning by Vibe

**Owner:** Hex (data pipeline), Vibe (velocity curve calibration)

---

## Summary Table

| ID | Feature | Priority | Complexity | Owner | Status |
|----|---------|----------|-----------|-------|--------|
| P0.1 | `renderNoteToWav()` in XPNExporter.h | P0 | M | Rex + Ruby | Blocked on XOceanusEngine target extraction |
| P0.2 | `xpn_bundle_builder.py` extended sources | P0 | M | Rex + Ruby | Ready to implement |
| P0.3 | `oxport.py --source` flag | P0 | S | Ruby + Rex | Blocked on P0.2 |
| P1.1 | `xpn_deconstruction_builder.py` | P1 | L | Vibe + Rex | Spec complete |
| P1.2 | `xpn_tutorial_builder.py` | P1 | M | Ruby + Vibe | Spec complete |
| P1.3 | `xpn_setlist_builder.py` | P1 | M | Vibe + Rex | Spec complete |
| P1.4 | `xpn_evolution_builder.py` | P1 | M | Vibe + Rex | Spec complete |
| P1.5 | `xpn_stems_checker.py` | P1 | S | Rex + Ruby | Spec complete |
| P1.6 | `--venue-size` for drum export | P1 | S | Vibe + Rex | Ready to implement |
| P1.7 | `--performance-optimized` for drum export | P1 | S | Rex + Hex | Ready to implement |
| P2.1 | Gravitational wave kit | P2 | L | Hex + Rex | R&D complete |
| P2.2 | Climate kit real API integration | P2 | M | Ruby + Rex | R&D complete |
| P2.3 | Monster Rancher room mode extraction | P2 | L | Hex + Vibe | R&D phase |
| P2.4 | MPCe quad-corner XML stub | P2 | S | Rex + Ruby | Waiting on Akai docs |
| P2.5 | `--workers N` parallel flag, all tools | P2 | S | Ruby + Rex | Ready to implement |
| P3.1 | Neural timbre transfer | P3 | XL | Hex + Vibe | Research only |
| P3.2 | Real-time generative kit WebSocket | P3 | XL | Hex + Rex | Research only |
| P3.3 | Community submission pipeline | P3 | XL | Ruby + Rex | Research only |
| P3.4 | Gene kit protein folding extension | P3 | XL | Hex + Vibe | Research only |

---

## Critical Path to April 2026

```
P0.1: XOceanusEngine CMake target extraction
      └─> renderNoteToWav() implementation (~40 lines)
          └─> fleet-render CLI Main.cpp
              └─> oxport.py render stage
                  └─> FIRST PACK SHIPS

P0.2: SOURCE_REGISTRY in bundle_builder
      └─> P0.3: --source flag in oxport.py
          └─> unconventional packs viable
```

P1 features can be developed in parallel once P0 is clear. P1.5 (`xpn_stems_checker.py`)
has no blocking dependencies and can begin immediately as a confidence-building first task.
