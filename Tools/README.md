# XO_OX Tools — XPN Export Pipeline

Python toolchain for building MPC-compatible `.xpn` expansion packs from XOlokun presets.

---

## Prerequisites

- **Python 3.10+**
- **pip packages** (install with `pip install -r requirements.txt`):
  - `mido` — MIDI I/O for rendering
  - `sounddevice` — audio capture via BlackHole
  - `numpy` — signal processing
  - `Pillow` _(optional)_ — cover art generation
  - `pyloudnorm` _(optional)_ — true LUFS normalization instead of RMS

---

## BlackHole Setup

BlackHole is a virtual audio loopback used to record XOlokun output directly.

1. Download BlackHole 2ch from https://existential.audio/blackhole/
2. Install and restart audio
3. In XOlokun (standalone): set Audio Output → BlackHole 2ch
4. In oxport: pass `--audio-device BlackHole` (or `BlackHole 2ch`)

---

## Quick Start — Five Commands to First .xpn

```bash
# 1. Full pipeline for the Onset drum engine (needs pre-rendered WAVs)
python3 oxport.py run --engine Onset --wavs-dir ./wavs --output-dir ./build

# 2. Full .oxbuild compile (renders + assembles + packages automatically)
python3 oxport.py build packs/mpce-perc-001.oxbuild --audio-device BlackHole

# 3. Validate the built output (structural checks + Q-Link validation)
python3 oxport.py validate --output-dir ./build

# 4. Check pipeline stage completion status
python3 oxport.py status --output-dir ./build

# 5. Validate .xometa presets (DNA, naming, coupling, parameters)
python3 oxport.py validate --presets
```

---

## Engine Names

Engine names are **title-case** in all spec files and CLI arguments:

```
Onset        Odyssey      Opal         Oblong       Obese
Overdub      Overworld    Organon      Ouroboros    Obsidian
Origami      Oracle       Obscura      Oceanic      Ocelot
Overbite     Orbital      Optic        Oblique      Osprey
Osteria      Owlfish      Ohm          Orphica      Obbligato
Ottoni       Ole          Overlap      Outwit       Ombre
Orca         Octopus      Ostinato     OpenSky      OceanDeep
Ouie         Obrix        Orbweave     Overtone     Organism
Oxbow        Oware        Opera        Offering     Osmosis
Oxytocin     Outlook      Obiont
```

**Only `Onset` is a drum engine** (generates pad-mapped XPM programs). All others produce keygroup programs. The pipeline auto-detects based on engine name.

---

## MIDI Channel Convention

Channels in render spec files are **0-indexed**:

- `0` = MIDI channel 1 (what MPC and XOlokun call "Channel 1")
- `15` = MIDI channel 16

---

## Pipeline Stages

The `run` command chains 10 stages in order:

| Stage | Description |
|-------|-------------|
| `render_spec` | Generate render specifications from .xometa presets |
| `categorize` | Classify WAV samples into voice categories (kick/snare/etc.) |
| `expand` | Expand flat kits into velocity/cycle/round-robin WAV sets (drum only) |
| `qa` | Perceptual QA check on rendered WAVs (clipping, DC offset, silence) |
| `smart_trim` | Auto-trim silence tails and add fade-out on rendered WAVs |
| `export` | Generate .xpm programs (drum or keygroup, per engine) |
| `cover_art` | Generate branded procedural cover art (requires Pillow) |
| `complement_chain` | Generate primary+complement XPM variant pairs (Artwork collection only) |
| `preview` | Generate 15-second preview audio for each program |
| `package` | Package everything into a .xpn archive |

Skip stages with `--skip stage1,stage2`.

---

## Known Limitations

- **`.xpn` is a community-documented format**, not an official Akai SDK. Internal structure is based on reverse-engineering. MPC firmware updates may break compatibility.
- **Normalization is RMS-based** by default. Install `pyloudnorm` for true ITU-R BS.1770 LUFS: `pip install pyloudnorm`.
- **No resume capability**: if the pipeline fails mid-run, re-run from scratch or use `--skip` to bypass completed stages manually.
- **Coupling is lost in XPN export**: Entangled mood presets and coupling routes cannot be encoded in MPC XPM format. The pipeline warns when it encounters them.
- **BlackHole required for real-time rendering**: the `build` command records XOlokun audio via loopback; you must have XOlokun open with BlackHole as the audio output.
