# Non-Audio Data Sources for XPN Kit Generation — R&D

**Date**: 2026-03-16
**Project**: XO_OX Designs — Oxport XPN Tool Suite
**Status**: Research & Development

---

## Overview

The XPN format encodes drum kits as structured XML with per-pad velocity layers, timing data, sample references, Q-Link assignments, and program metadata. Every one of these fields is a numerical or categorical value that can, in principle, be derived from any dataset — not just recorded audio. This document surveys 10 non-audio data sources, examines how each maps to XPN parameters, and estimates the engineering cost of building a real parser.

The goal is not novelty for its own sake. Each data source must produce kits that are *musically useful* — repeatable, variable in meaningful ways, and distinct from anything a human producer would design by hand.

---

## 1. Seismograph — USGS Earthquake CSV

### Data Source + Format
USGS publishes earthquake event catalogs at earthquake.usgs.gov/earthquakes/search/ as comma-separated value files. Each row represents one event. Key columns: `time` (ISO 8601 UTC), `latitude`, `longitude`, `depth` (km), `mag` (Richter scale), `magType`, `place` (region string). A typical export covers thousands of events over a chosen time window.

### Python Parsing Approach (stdlib only)
```python
import csv
from datetime import datetime

with open('earthquakes.csv', newline='') as f:
    reader = csv.DictReader(f)
    events = [row for row in reader if row['mag']]

events.sort(key=lambda r: r['time'])
origin = datetime.fromisoformat(events[0]['time'].replace('Z', '+00:00'))

for ev in events:
    ts = datetime.fromisoformat(ev['time'].replace('Z', '+00:00'))
    seconds_since_first = (ts - origin).total_seconds()
    magnitude = float(ev['mag'])
    depth_km = float(ev['depth'])
```

`csv.DictReader` handles headers automatically. `datetime.fromisoformat` parses UTC timestamps without third-party libs. Depth and magnitude are cast to float directly.

### Mapping to XPN Parameters
| Seismic Field | XPN Parameter |
|---|---|
| Magnitude (0–9+) | Pad velocity (normalized to 0–127) |
| Depth (0–700 km) | Velocity layer selection (shallow = layer 1, deep = layer 4) |
| Seconds since epoch (mod 2.0) | Note onset timing offset within sequence |
| Latitude band (N/S hemisphere) | Pad bank assignment (A/B) |
| Longitude band (E/W) | Pad column within bank (1–4) |
| Magnitude delta (aftershock chains) | Velocity fade curve across layers |

### Creative Sonic Result
Earthquake clusters produce polyrhythmic kick/impact sequences with physically motivated dynamics — big events hit hard, aftershocks trail off. Deep-focus events trigger bass-heavy lower velocity layers. Geographically sorted events map naturally to stereo-left/right pad placement. Pacific Ring of Fire sequences generate dense syncopated patterns; intraplate events produce sparse, isolated hits.

### Implementation Complexity
**Easy.** USGS CSV schema is stable and well-documented. All parsing is basic float arithmetic and datetime math. The only design decision is the normalization curve for magnitude (linear vs. logarithmic).

---

## 2. Protein Folding Angles — PDB Dihedral Data

### Data Source + Format
The RCSB Protein Data Bank (rcsb.org) distributes structures in PDB format — plain-text files with ATOM records containing x/y/z coordinates for each residue. Phi (φ) and psi (ψ) dihedral angles describe the backbone conformation at each amino acid. A typical protein has 100–500 residues. Dihedral angles range –180° to +180°.

### Python Parsing Approach (stdlib only)
```python
import math

def parse_atoms(path):
    atoms = []
    with open(path) as f:
        for line in f:
            if line.startswith('ATOM'):
                name = line[12:16].strip()
                res_seq = int(line[22:26])
                x, y, z = float(line[30:38]), float(line[38:46]), float(line[46:54])
                atoms.append({'name': name, 'res': res_seq, 'x': x, 'y': y, 'z': z})
    return atoms

def dihedral(p0, p1, p2, p3):
    b0 = [p1[i]-p0[i] for i in range(3)]
    b1 = [p2[i]-p1[i] for i in range(3)]
    b2 = [p3[i]-p2[i] for i in range(3)]
    n1 = cross(b0, b1)
    n2 = cross(b1, b2)
    return math.degrees(math.atan2(dot(cross(n1,n2),b1), dot(n1,n2)))
```

PDB fixed-column format is parsed with string slicing — no external library needed.

### Mapping to XPN Parameters
| Protein Field | XPN Parameter |
|---|---|
| Phi angle (–180 to +180°) | Gate length (negative = short, positive = long) |
| Psi angle (–180 to +180°) | Velocity (remapped 0–127) |
| Residue index (mod 16) | Pad assignment across single bank |
| Residue index (mod 64) | Pad assignment across 4 banks |
| Alpha-helix / beta-sheet classification | Velocity layer (helix = layer 2, sheet = layer 4) |
| Ramachandran region | Q-Link macro assignment |

One residue = one 1/64th note slot in the sequence. A 256-residue protein fills exactly 4 bars of 4/4 at 1/64th note resolution.

### Creative Sonic Result
Alpha-helical regions produce smooth, rolling hi-hat patterns (consistent phi/psi ranges). Beta-sheets generate staccato, angular hits. Disordered loops create irregular fills. Different proteins — an antibiotic peptide vs. a muscle fiber — produce completely different rhythmic fingerprints while remaining internally coherent. The kit is unique, reproducible from the PDB ID, and impossible to reverse-engineer by ear.

### Implementation Complexity
**Hard.** PDB parsing from scratch requires implementing correct dihedral angle math (cross products, atan2, sign handling for –180/+180 wraparound). Secondary structure classification (helix/sheet/loop) requires either reading HELIX/SHEET records or implementing a Ramachandran region classifier. Not a weekend project, but entirely stdlib.

---

## 3. ECG Heartbeat — P/Q/R/S/T Peak Detection

### Data Source + Format
PhysioNet (physionet.org) distributes ECG recordings in WFDB format and as CSV exports. Each row: timestamp (ms) and voltage (mV). A standard 12-lead ECG at 500 Hz captures thousands of samples per minute. The waveform cycles through P, Q, R, S, T waves per heartbeat. RR interval (R-to-R distance in ms) defines the instantaneous heart rate.

### Python Parsing Approach (stdlib only)
```python
import csv

def load_ecg(path):
    times, voltages = [], []
    with open(path) as f:
        for row in csv.reader(f):
            times.append(float(row[0]))
            voltages.append(float(row[1]))
    return times, voltages

def find_peaks(voltages, threshold=0.5, min_gap=200):
    peaks = []
    for i in range(1, len(voltages)-1):
        if voltages[i] > threshold and voltages[i] > voltages[i-1] and voltages[i] > voltages[i+1]:
            if not peaks or i - peaks[-1] > min_gap:
                peaks.append(i)
    return peaks
```

R-peaks (the tall spike) are the most prominent and easiest to detect with a simple threshold + local maximum test. P and T waves require lower thresholds applied within windowed search zones relative to each R-peak.

### Mapping to XPN Parameters
| ECG Feature | XPN Parameter |
|---|---|
| R-peak | Pad 1 (kick anchor) |
| P-peak | Pad 9 or 13 (pre-beat anticipation) |
| Q/S notches | Pad 5 or 6 (transient detail) |
| T-peak | Pad 12 or 16 (post-beat bloom) |
| RR interval (ms) | Sequence tempo (60000 / RR_ms = BPM) |
| Voltage amplitude at each peak | Pad velocity (normalized to 0–127) |
| QRS complex width | Gate / note length |
| Heart rate variability (HRV) | Timing humanization offset |

### Creative Sonic Result
A resting heart at 60 BPM generates a slow, groove-weighted kick pattern with natural HRV-derived swing. An athlete's heart at 180 BPM during exertion becomes an aggressive, compressed drum machine. Clinical arrhythmias (AFib, PVCs) produce polyrhythmic and off-beat patterns that no quantize grid would ever produce. Each person's ECG is a unique rhythmic signature.

### Implementation Complexity
**Medium.** R-peak detection is straightforward with simple threshold logic. Accurately separating P, Q, S, and T waves requires careful window math around each RR cycle but is achievable in pure Python. The main challenge is noise tolerance — real ECG signals have motion artifact and baseline wander.

---

## 4. Satellite Imagery — Multi-Spectral TIFF Histogram

### Data Source + Format
Sentinel-2 and Landsat imagery from ESA/USGS is distributed as GeoTIFF files with 8–13 spectral bands (visible, NIR, SWIR, etc.). Each band is a 2D array of 16-bit pixel values. For XPN purposes, only the brightness histogram per band is needed — not spatial structure.

### Python Parsing Approach (stdlib only)
```python
import struct

def read_tiff_strip(path):
    # Minimal TIFF IFD parser for single-strip uint16 data
    with open(path, 'rb') as f:
        header = f.read(4)
        endian = '<' if header[:2] == b'II' else '>'
        offset = struct.unpack(endian + 'I', f.read(4))[0]
        f.seek(offset)
        # ... IFD tag parsing for StripOffsets, StripByteCounts, BitsPerSample

def histogram_from_values(values, bins=128):
    mn, mx = min(values), max(values)
    counts = [0] * bins
    span = mx - mn or 1
    for v in values:
        idx = min(int((v - mn) / span * bins), bins - 1)
        counts[idx] += 1
    return counts
```

For production use, a pre-processed CSV export of per-band histograms sidesteps raw TIFF parsing. The imaging pipeline (QGIS, GDAL) generates these exports trivially; the XPN tool accepts that CSV directly.

### Mapping to XPN Parameters
| Spectral Feature | XPN Parameter |
|---|---|
| Band 1 (coastal aerosol) brightness | Pad 1 velocity curve |
| Bands 2–4 (blue/green/red) | Pads 2–4 velocity curves |
| Bands 5–8 (red-edge / NIR) | Pads 5–8 velocity curves |
| Histogram peak position | Velocity center point per pad |
| Histogram spread (std dev) | Velocity layer spread width |
| Band ratio NDVI = (NIR-Red)/(NIR+Red) | Crossfade between two sample sets |

### Creative Sonic Result
A dense forest image (high NDVI) produces pads weighted toward sustained, lush mid-velocity hits. A desert scene (low NDVI, high red/SWIR) generates bright, sparse, high-velocity transients. An urban scene creates a flat, compressed dynamic range across all pads. Seasonal satellite imagery of the same location generates evolving kits that literally sound like spring vs. winter.

### Implementation Complexity
**Hard** (raw TIFF). **Easy** (pre-processed CSV histogram input). The recommended implementation accepts a simple 8-column CSV of band histograms. TIFF parsing in pure Python is painful and fragile; the spec should document the CSV preprocessing step clearly and make it the primary path.

---

## 5. Poetry Scansion — CMU Pronouncing Dictionary

### Data Source + Format
The CMU Pronouncing Dictionary (CMUdict) maps English words to phoneme sequences with stress markers: `0` = unstressed, `1` = primary stress, `2` = secondary stress. Distributed as a plain text file (~125,000 entries). Example: `HELLO  HH AH0 L OW1`. A scanned poem maps each syllable to a stress value.

### Python Parsing Approach (stdlib only)
```python
def load_cmudict(path):
    lexicon = {}
    with open(path) as f:
        for line in f:
            if line.startswith(';;;'):
                continue
            word, *phonemes = line.strip().split()
            stresses = [p[-1] for p in phonemes if p[-1].isdigit()]
            lexicon[word.upper()] = stresses
    return lexicon

def scan_poem(text, lexicon):
    syllables = []
    for word in text.upper().split():
        clean = ''.join(c for c in word if c.isalpha())
        stresses = lexicon.get(clean, ['0'])
        syllables.extend(stresses)
    return syllables
```

### Mapping to XPN Parameters
| Scansion Feature | XPN Parameter |
|---|---|
| Primary stress (`1`) | Full-velocity accent hit |
| Secondary stress (`2`) | Mid-velocity hit (~80 velocity) |
| Unstressed (`0`) | Ghost note (~30 velocity) or rest |
| Syllable position (mod 16) | Pad position in sequence |
| Line break | Bar boundary / sequence reset |
| Metrical foot (iamb/trochee/dactyl) | Q-Link swing parameter |
| Feminine ending (trailing unstressed) | Final note decay tail |

### Creative Sonic Result
Iambic pentameter (Shakespeare) produces a da-DUM da-DUM kick pattern with natural forward momentum. Trochaic verse (Longfellow) flips to DUM-da, creating a march feel. Dactylic hexameter (Homer) generates triplet-feel patterns. Free verse produces irregular accent bursts. The same poem read aloud and played as a kit would share rhythmic DNA.

### Implementation Complexity
**Easy.** CMUdict is a static text file distributed freely. The parsing is trivial string splitting. The main design decision is how to handle unknown words (fall back to all-unstressed).

---

## 6. DNA Codon Sequences — FASTA Input

### Data Source + Format
FASTA format encodes nucleotide or protein sequences as plain text: a `>identifier` header line followed by the sequence string (`ATGCGATCC...`). DNA codons are 3-nucleotide triplets that encode amino acids. There are 64 possible codons (4^3). NCBI GenBank provides FASTA exports for any gene or genome region.

### Python Parsing Approach (stdlib only)
```python
def parse_fasta(path):
    sequences = {}
    current_id = None
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line.startswith('>'):
                current_id = line[1:].split()[0]
                sequences[current_id] = ''
            elif current_id:
                sequences[current_id] += line.upper()
    return sequences

def extract_codons(seq):
    return [seq[i:i+3] for i in range(0, len(seq)-2, 3) if len(seq[i:i+3]) == 3]

# All 64 codons assigned to pads 0-63 (4 banks x 16 pads)
CODON_PAD = {codon: idx for idx, codon in enumerate(sorted(
    [a+b+c for a in 'ATGC' for b in 'ATGC' for c in 'ATGC']))}
```

64 codons map cleanly to 64 pads across 4 banks of 16 — a perfect fit for MPC's pad structure.

### Mapping to XPN Parameters
| DNA Feature | XPN Parameter |
|---|---|
| Codon identity (1 of 64) | Pad assignment (4 banks × 16 pads) |
| Codon frequency in sequence | Velocity weight (common codons hit harder) |
| Synonymous codon group | Velocity layer within pad |
| Start codon (ATG) | Bank A, Pad 1 — kick anchor |
| Stop codons (TAA/TAG/TGA) | Sequence end marker / reset |
| GC content of 16-codon window | Q-Link filter cutoff curve |
| ORF (open reading frame) length | Kit sequence length |

### Creative Sonic Result
High-GC genes (common in heat-adapted bacteria) produce dense, high-frequency pad clusters. AT-rich regions produce sparse, open patterns. Different organisms' versions of the same gene (e.g., human vs. yeast cytochrome C) produce similar melodic shapes but different rhythmic emphasis — like two musicians playing the same song in different dialects.

### Implementation Complexity
**Easy.** FASTA parsing is trivially string-based. The codon-to-integer mapping is a static dictionary. Frequency counting is a hand-rolled dict. The 64-pad layout maps perfectly to MPC's 4-bank structure with no remainder.

---

## 7. Git Commit History — Repository Activity

### Data Source + Format
`git log --format="%H %at %s" --numstat` outputs commit hash, Unix timestamp, subject, and per-file change counts. `git log --shortstat` provides summary line counts. Export via Python's `subprocess`. Timestamps are Unix epoch integers.

### Python Parsing Approach (stdlib only)
```python
import subprocess

def get_git_log(repo_path):
    result = subprocess.run(
        ['git', '-C', repo_path, 'log',
         '--format=%H|%at|%an', '--shortstat'],
        capture_output=True, text=True
    )
    commits = []
    lines = result.stdout.strip().split('\n')
    for i, line in enumerate(lines):
        if '|' in line:
            h, ts, author = line.split('|')
            stats = lines[i+2] if i+2 < len(lines) else ''
            files = int(stats.split('file')[0].strip()) if 'file' in stats else 0
            commits.append({'hash': h, 'timestamp': int(ts), 'files': files})
    return commits
```

### Mapping to XPN Parameters
| Git Feature | XPN Parameter |
|---|---|
| Files changed per commit | Pad velocity (0 files = ghost, 20+ = accent) |
| Inter-commit interval (seconds) | Timing offset / note spacing |
| Author identity (hash mod 16) | Pad bank assignment |
| Commit message keyword ('fix'/'feat'/'refactor') | Velocity layer |
| Lines added vs. deleted ratio | Attack vs. decay character |
| Merge commit | Accent marker / section boundary |
| Commit burst (3+ commits < 1 hour) | Drum fill trigger |

### Creative Sonic Result
A solo developer's late-night session becomes an erratic, high-intensity snare burst. A disciplined team's CI pipeline produces metronomic, consistent hits. A repository's 10-year history becomes a historical rhythm track — quiet periods map to whisper-soft hi-hats, crunch periods become wall-of-sound fills. The XO_OX-XOceanus repo itself would generate a kit that sounds like the project was built.

### Implementation Complexity
**Easy.** Git log output is well-structured and scriptable. `subprocess` is stdlib. The main design choice is normalization: how to scale files-changed and time-intervals into the 0–127 velocity range without clipping at either extreme.

---

## 8. Weather Patterns — Historical Time Series

### Data Source + Format
NOAA Global Historical Climatology Network (GHCN) distributes daily station observations as CSV: `STATION, DATE, ELEMENT, VALUE`. Key elements: `TMAX`/`TMIN` (temperature in tenths of °C), `PRCP` (precipitation), `PRES` (station pressure). A year of daily data for one station is ~1,460 rows (4 elements × 365 days).

### Python Parsing Approach (stdlib only)
```python
import csv
from datetime import date

def load_ghcn(path, station_id):
    records = {}
    with open(path) as f:
        for row in csv.DictReader(f):
            if row['STATION'] != station_id:
                continue
            d = date.fromisoformat(row['DATE'])
            records.setdefault(d, {})[row['ELEMENT']] = float(row['VALUE']) / 10.0
    return records
```

### Mapping to XPN Parameters
| Weather Feature | XPN Parameter |
|---|---|
| Temperature (daily high) | Crossfade position between two sample layers |
| Temperature anomaly (vs. 30-yr avg) | Velocity deviation from base |
| Precipitation (mm) | Reverb send / decay tail length |
| Pressure (hPa) | Filter cutoff position |
| Temperature range (TMAX–TMIN) | Velocity layer spread |
| Storm event flag | Accent velocity override |
| Seasonal position (day of year) | Q-Link macro position |

The crossfade between two sample sets (e.g., "winter" samples vs. "summer" samples) driven by temperature is this source's defining feature — it produces kits that smoothly evolve over time rather than making binary pad assignments.

### Creative Sonic Result
A year of weather data from New Orleans becomes a jazz kit that swells in summer heat and pulls back in mild winter. An Icelandic station produces sparse, cold, percussive hits that occasionally explode into storm-driven accents. A tropical station near the equator produces a near-constant moderate-energy groove with rainfall-driven fills. The kit literally embodies climate.

### Implementation Complexity
**Medium.** GHCN CSV parsing is simple. The complexity is in the crossfade math — interpolating between two complete sample sets based on a continuous temperature value requires defining what "sample set A" and "sample set B" mean in XPN terms (likely two complete programs merged into one with velocity-layer crossfade).

---

## 9. Stock Market OHLC — Open/High/Low/Close Data

### Data Source + Format
Yahoo Finance, Alpha Vantage, and Polygon.io distribute daily OHLC data as CSV: `Date, Open, High, Low, Close, Volume, Adj Close`. Each row is one trading day. Volume is share count. Yahoo Finance allows CSV export from the browser without an API key.

### Python Parsing Approach (stdlib only)
```python
import csv

def load_ohlc(path):
    rows = []
    with open(path) as f:
        for row in csv.DictReader(f):
            rows.append({
                'date': row['Date'],
                'open': float(row['Open']),
                'high': float(row['High']),
                'low': float(row['Low']),
                'close': float(row['Close']),
                'volume': float(row['Volume'])
            })
    return rows

def normalize(value, mn, mx):
    return int(127 * (value - mn) / (mx - mn + 1e-9))
```

### Mapping to XPN Parameters
| OHLC Feature | XPN Parameter |
|---|---|
| Open price | Velocity layer 1 (floor) |
| High price | Velocity layer 2 (peak) |
| Low price | Velocity layer 3 (dip) |
| Close price | Velocity layer 4 (resolution) |
| Volume | Pad sensitivity / Q-Link response scaling |
| Daily price range (High–Low) | Velocity spread within pad |
| Bullish candle (Close > Open) | Rising velocity accent direction |
| Bearish candle (Close < Open) | Falling velocity decay direction |
| Volume spike (>2× 20-day avg) | Fill trigger |
| Price gap (Open > prev Close) | Sequence position skip / rest |

### Creative Sonic Result
A volatile tech stock (GME during its 2021 surge) produces an extreme kit where velocity swings from whisper to full accent within a few bars, with massive volume-spike fills. A stable treasury ETF produces a tight, compressed, almost metronomic pattern. Crash periods (2008, 2020) generate dense, overlapping accent cascades. The kit captures market emotion as rhythm.

### Implementation Complexity
**Easy.** CSV parsing is trivial. OHLC normalization is straightforward min-max scaling. The main design decision is the time window: daily bars produce slow-evolving patterns; intraday 1-minute bars produce hyper-dense sequences.

---

## 10. Sports Play-by-Play — NBA / Soccer Event Sequences

### Data Source + Format
NBA play-by-play data is available from stats.nba.com as JSON or pre-scraped CSVs on Kaggle. Each row: `game_clock`, `period`, `event_type` (field goal, foul, timeout, turnover, etc.), `team`, `player`. Soccer event data (Statsbomb, Opta) follows similar conventions. A full NBA game contains 400–600 events.

### Python Parsing Approach (stdlib only)
```python
import csv

EVENT_PAD_MAP = {
    'field_goal_made': 1,
    'field_goal_missed': 2,
    'free_throw_made': 3,
    'free_throw_missed': 4,
    'rebound': 5,
    'turnover': 6,
    'foul': 7,
    'timeout': 8,
    '3pt_made': 9,
    'block': 10,
    'steal': 11,
    'assist': 12,
}

def parse_pbp(path):
    events = []
    with open(path) as f:
        for row in csv.DictReader(f):
            events.append({
                'time_seconds': float(row['game_clock']),
                'event': row['event_type'],
                'pad': EVENT_PAD_MAP.get(row['event_type'], 16)
            })
    return events
```

### Mapping to XPN Parameters
| Sports Feature | XPN Parameter |
|---|---|
| Event type | Pad assignment (per EVENT_PAD_MAP) |
| Home team event | Pad bank A/C |
| Away team event | Pad bank B/D |
| Game clock position | Sequence timing |
| Score differential at event | Velocity (blowout = flat; close game = variable) |
| Made vs. missed | Velocity layer (made = full, missed = mid) |
| Overtime period | Extended sequence region |
| Crowd-response events (slam dunk, triple) | Accent marker |

### Creative Sonic Result
A close fourth-quarter NBA game generates a dense, high-intensity kit with rapid alternating pad hits as both teams trade baskets. A dominant first-half blowout creates an asymmetric pattern weighted to one pad bank. A soccer match with few goals produces sparse, atmospheric patterns punctuated by sudden goal-burst accents. Different sports have completely different rhythmic vocabularies — basketball is fast and polyrhythmic; baseball is sparse and meditative.

### Implementation Complexity
**Medium.** The parsing is simple CSV/JSON work. The complexity is data availability — NBA CSV exports require scraping or Kaggle datasets; the EVENT_PAD_MAP must be tuned per sport/data source. The game clock time-mapping requires careful handling of stoppages and overtime.

---

## Implementation Priority Matrix

**The 3 to build first:**

### Priority 1 — Poetry Scansion (CMU Pronouncing Dictionary)
**Reason:** Lowest implementation barrier. CMUdict is a static text file distributed freely, the parsing is trivial string processing, and the creative result — a kit that plays your lyrics as rhythm — is immediately legible and demonstrable to any producer. It has natural crossover appeal with the XO_OX field guide and community, and requires no external data fetching or API access. Build time: 1–2 days.

### Priority 2 — Git Commit History
**Reason:** The XO_OX developer community is deeply technical. A script that generates a kit from a GitHub repository's commit log is a natural promotional tool — shareable, reproducible, and surprising. The `subprocess` + `git log` pipeline is entirely stdlib. Every user can immediately apply it to their own projects. The "generate a kit from your codebase" concept is inherently viral in dev-adjacent music production circles. Build time: 2–3 days including normalization tuning.

### Priority 3 — Stock Market OHLC
**Reason:** OHLC data is the most universally available non-audio dataset on the internet. Every producer has heard of it; many follow markets. The 4-layer (Open/High/Low/Close) velocity mapping is elegant and has a direct musical logic. Yahoo Finance CSV export requires no API key. The "generate a kit from a stock ticker" concept is meme-able and immediately viral. Build time: 2–3 days.

**Deferred rationale for the remaining 7:**
- Seismograph: Conceptually compelling but USGS data format changes occasionally and magnitude normalization decisions are non-trivial.
- DNA codons: The 64-codon/64-pad mapping is architecturally perfect but sourcing relevant FASTA files is non-obvious for non-biologists.
- ECG: Highest creative ceiling (personal heartbeat as kit) but accurate P/Q/S/T detection is genuinely hard in pure Python.
- Satellite imagery: Beautiful concept but preprocessing dependency (GeoTIFF to histogram CSV) creates friction.
- Protein folding, Weather, Sports: Excellent medium-term targets once the first three prove the concept.

---

## Monster Rancher Extension

`xpn_monster_rancher.py` generates unique drum kits by hashing arbitrary files into a DNA fingerprint — the same file always produces the same kit, different files produce different kits. The 10 data sources above each provide a new class of *meaningful* DNA input: not random bytes, but structured data where the resulting kit is *legible* — you can hear the source in the output.

### Shared Integration Interface

All 10 sources hook into the same interface:

```python
class DNASource:
    def get_name(self) -> str: ...
    def get_dna_bytes(self) -> bytes: ...

# xpn_monster_rancher.py calls:
source = SeismographDNA('usgs_2024_pacific.csv')
kit = generate_kit_from_dna(source.get_dna_bytes(), name=source.get_name())
```

`generate_kit_from_dna()` already exists in `xpn_monster_rancher.py`. Adding a new data source is purely additive — implement `DNASource`, register in the CLI `--source` argument parser, document here.

### Per-Source DNA Encoding

**Seismograph**
Each event's `(latitude, longitude, depth, magnitude)` tuple is serialized as 4 bytes and appended. The USGS event ID becomes the monster's name. Pacific Ring of Fire queries produce dense, attack-heavy monsters.

**Protein Folding**
The PDB ID (e.g., `1AKE`) is the monster name. Each residue's dihedral angle pair (phi, psi) is quantized to 2 bytes and appended. Proteins with similar functions but different organisms produce recognizably related monsters — same genus, different species.

**ECG**
Patient ID or recording date is the monster name. RR intervals (as 16-bit ms values) form the DNA. The same person at rest vs. exercise produces a monster and its powered-up variant — same base DNA, different rhythm signature.

**Satellite Imagery**
Geographic bounding box (lat/lon) and acquisition date form the monster name. Band histogram values (8 bands × 128 bins) provide 1024 bytes of highly specific DNA. The same location across seasons produces seasonal monster variants.

**Poetry Scansion**
Poem title or first line is the monster name. The syllable stress sequence (`010201010...`) is serialized as nibbles and appended as DNA bytes. Two sonnets by the same poet produce sibling monsters.

**DNA Codons**
Gene symbol (e.g., `BRCA1`, `COX1`) is the monster name. The codon sequence — after encoding each of the 64 codons as a 6-bit value — is the DNA. Organisms sharing a gene produce related monsters.

**Git Commit History**
Repository name is the monster name. Each commit's `(timestamp mod 65536, files_changed)` pair becomes 3 bytes of DNA. Two forks of the same repo produce fork-monsters that diverge after their branch point. The XO_OX-XOceanus repo would produce a legendary-tier monster.

**Weather**
Station ID and year form the monster name. Daily TMAX/TMIN/PRCP values (quantized to bytes) provide ~1,095 bytes of seasonal DNA. Two years from the same station produce a climate-drift monster pair.

**Stock OHLC**
Ticker symbol and date range form the monster name. Each day's `(close-open, high-low, volume_rank)` vector provides 3 bytes of DNA. Bull market periods produce aggressive, high-velocity monsters; bear markets produce compressed, subdued ones.

**Sports Play-by-Play**
Game ID (e.g., `NBA-2024-GSW-LAL-Game7`) is the monster name. Each event type is encoded as a single byte and appended in game-clock order. Overtime games produce longer DNA than regulation games. Championship games produce legendary monsters.

---

*End of document — non_audio_xpn_sources_rnd.md*
