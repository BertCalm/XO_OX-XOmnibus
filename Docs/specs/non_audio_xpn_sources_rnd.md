# Non-Audio Data Sources for XPN Kit Generation — R&D

**Date**: 2026-03-16
**Project**: XO_OX Designs — XPN/MPC Export Tool Suite
**Status**: R&D / Concept

---

## Central Thesis

Anything with periodicity, intensity variation, or detectable pattern can be mapped to percussion. The world is full of time-series data that no one has thought of as rhythm — earthquake catalogs, cardiac waveforms, protein backbone angles, git commit graphs. These pipelines transform domain-specific signals into playable XPN kits on MPC hardware, giving producers rhythms that are structurally valid by physics, biology, or human activity patterns rather than programmed intuition.

The XPN format is a good target: it supports multi-velocity layers, multiple pads, pad-level filter/envelope settings, and per-sample gain. Most of these pipelines collapse to the same abstract problem: normalize a 1D time-series to velocity range 1–127, discretize timestamps to rhythmic grid positions, and optionally use a secondary signal dimension to select velocity layers.

---

## Pipeline 01 — Seismograph Data

**Concept**: USGS earthquake catalog CSV → percussion kit where seismic events become drum hits

### Data Source & Format
- USGS Earthquake Hazards Program: `earthquake.usgs.gov/earthquakes/search/`
- CSV export: `time, latitude, longitude, depth, mag, magType, nst, gap, dmin, rms, net, id, updated, place, type`
- Key columns: `time` (ISO 8601), `mag` (float, typically 0.0–9.5), `depth` (km)
- Download URL pattern: `https://earthquake.usgs.gov/fdsnws/event/1/query?format=csv&starttime=...&endtime=...`

### Python Parsing Approach
```python
import pandas as pd
import numpy as np

df = pd.read_csv('usgs_earthquakes.csv', parse_dates=['time'])
df = df.sort_values('time')

# Normalize magnitude to velocity 1-127
# Typical catalog range: 0.5 to 7.0 for a regional dataset
df['velocity'] = ((df['mag'] - df['mag'].min()) /
                  (df['mag'].max() - df['mag'].min()) * 126 + 1).astype(int)

# Convert time to seconds-since-start for beat placement
df['seconds'] = (df['time'] - df['time'].iloc[0]).dt.total_seconds()

# Velocity layer selection by depth:
# shallow < 30km = Layer 1 (punchy short attack)
# intermediate 30-70km = Layer 2
# deep > 70km = Layer 3 (sub-bass, long release)
def depth_layer(d):
    if d < 30:   return 1
    if d < 70:   return 2
    return 3

df['layer'] = df['depth'].apply(depth_layer)
```

### Mapping to XPN Parameters
| Source field | XPN target |
|---|---|
| `mag` normalized 1–127 | velocity |
| `depth` < 30km | velocity layer 1 (short attack, hard transient) |
| `depth` 30–70km | velocity layer 2 (mid body) |
| `depth` > 70km | velocity layer 3 (low-end rumble, long release) |
| `seconds` quantized to 1/16 grid | pad trigger timing |
| `latitude/longitude` zone | pad selection (geographic clustering) |

### Creative Sonic Result
Seismically active regions (Pacific Ring of Fire, Cascadia subduction zone, Turkey fault systems) produce dense polyrhythmic kits with violent velocity spikes. Quiet periods produce ghost note fields. A one-year USGS catalog of magnitude 3.0+ events in California generates a 365-beat pattern that is genuinely unpredictable but physically real. Depth-layering creates natural kick/snare/sub separation: shallow crustal events punch like snares, deep subduction zone events rumble like 808 subs.

### Estimated Complexity: **Easy**
pandas + numpy, no exotic dependencies. USGS CSV is well-structured. Quantization to rhythmic grid is a simple modulo operation.

---

## Pipeline 02 — Protein Folding Angles

**Concept**: PDB (Protein Data Bank) backbone dihedral angles → rhythmic gate pattern

### Data Source & Format
- Protein Data Bank: `rcsb.org` — any `.pdb` file
- Relevant data: ATOM records with residue backbone φ (phi) and ψ (psi) dihedral angles
- Standard computation via BioPython's `Polypeptide` module: phi/psi per residue
- Ideal alpha-helix: φ ≈ -57°, ψ ≈ -47°
- Ideal beta-sheet: φ ≈ -119°, ψ ≈ +113°
- Deviation from ideal = structural tension = mapped velocity

### Python Parsing Approach
```python
from Bio.PDB import PDBParser, PPBuilder
import numpy as np

parser = PDBParser(QUIET=True)
structure = parser.get_structure('protein', 'my_protein.pdb')
ppb = PPBuilder()

events = []
ideal_alpha = (-57, -47)
ideal_beta  = (-119, 113)

for pp in ppb.build_peptides(structure):
    phi_psi = pp.get_phi_psi_list()
    for i, (phi, psi) in enumerate(phi_psi):
        if phi is None or psi is None:
            continue
        phi_deg = np.degrees(phi)
        psi_deg = np.degrees(psi)

        # Distance from alpha helix ideal
        dist_alpha = np.sqrt((phi_deg - ideal_alpha[0])**2 +
                             (psi_deg - ideal_alpha[1])**2)
        # Normalize to velocity: max deviation ~180 degrees
        velocity = int(np.clip((dist_alpha / 180.0) * 126 + 1, 1, 127))

        # Secondary structure assignment → pad selection
        if dist_alpha < 30:
            pad = 1   # alpha helix → hi-hat-like metallic hit
        elif np.sqrt((phi_deg - ideal_beta[0])**2 +
                     (psi_deg - ideal_beta[1])**2) < 40:
            pad = 2   # beta sheet → snare-like hit
        else:
            pad = 3   # loop/coil region → ghost note

        # Each residue = 1/64th note position
        timing_64th = i
        events.append({'timing': timing_64th, 'velocity': velocity, 'pad': pad})
```

### Mapping to XPN Parameters
| Source field | XPN target |
|---|---|
| Residue index | 1/64th note timing slot |
| Deviation from alpha-helix ideal | velocity |
| Secondary structure class | pad selection (kick/snare/hat) |
| B-factor (thermal motion) | sample variation / humanization |
| Chain identity | kit bank selection |

### Creative Sonic Result
Alpha-helical proteins (structural proteins like collagen, many membrane proteins) produce tight repetitive patterns with small velocity variation — like a metronomic 16th-note pulse. Intrinsically disordered proteins or loop-heavy enzymes produce highly varied, unpredictable velocity maps. A full human hemoglobin tetramer (PDB: 1GZX, ~574 residues) generates a ~9-bar pattern. Sonically: biologically stable structures sound rhythmically stable; flexible proteins sound loose and organic.

### Estimated Complexity: **Medium**
Requires BioPython (`pip install biopython`). PDB parsing is well-documented. Dihedral angle computation is built-in. Main challenge is handling missing residues and multi-chain structures gracefully.

---

## Pipeline 03 — ECG Heartbeat

**Concept**: Raw ECG waveform → drum kit where cardiac wave peaks trigger different pads

### Data Source & Format
- PhysioNet / MIT-BIH Arrhythmia Database: `physionet.org/content/mitdb/`
- Format: `.dat` + `.hea` header files, read via `wfdb` Python library
- Sample rate: typically 360 Hz (MIT-BIH), some datasets 250 Hz
- Key features: P wave, Q trough, R peak (dominant), S trough, T wave
- RR interval (time between R peaks) → tempo

### Python Parsing Approach
```python
import wfdb
import numpy as np
from scipy.signal import find_peaks

record = wfdb.rdrecord('mit-bih-arrhythmia/100')
signal = record.p_signal[:, 0]  # Lead I
fs = record.fs  # 360 Hz

# Detect R peaks (the dominant QRS complex)
r_peaks, _ = find_peaks(signal, height=0.5, distance=int(0.5 * fs))

# Median RR interval → tempo in BPM
rr_intervals = np.diff(r_peaks) / fs  # seconds
bpm = 60.0 / np.median(rr_intervals)

# For each beat: extract P, Q, R, S, T features
events = []
for i, r in enumerate(r_peaks):
    window = signal[max(0, r-100):r+150]
    r_amp = signal[r]

    # R amplitude → velocity (QRS is largest feature, 0.5–2.0 mV range)
    velocity = int(np.clip((r_amp / 2.0) * 126 + 1, 1, 127))

    events.append({
        'pad': 'kick',   # R peak = kick drum
        'timing_samples': r,
        'velocity': velocity,
        'bpm': bpm
    })
    # Could extend: detect P wave (pad=snare), T wave (pad=hat) via windowed peak search
```

### Mapping to XPN Parameters
| Source field | XPN target |
|---|---|
| R peak timing | kick drum trigger |
| P wave | snare or rim shot |
| T wave | hi-hat or shaker |
| QRS amplitude | velocity |
| RR interval | tempo (BPM for XPN) |
| HRV (RR variance) | humanization / timing scatter |
| Arrhythmia events | velocity spike / accent |

### Creative Sonic Result
A healthy resting heart at 72 BPM produces a clean 4/4 kick pattern with slight HRV humanization — sounds like a properly humanized drum loop. Atrial fibrillation produces metrically irregular but physiologically real timing deviations. Tachycardia records from exercise produce dense fast kit patterns. A runner's ECG during a sprint creates a genuine acceleration ramp. Medical data becomes a timing source that is both deeply human and structurally unexpected.

### Estimated Complexity: **Medium**
Requires `wfdb` and `scipy`. Feature detection (P/T waves beyond R peak) requires careful windowing but is well-understood in biomedical signal processing literature.

---

## Pipeline 04 — Satellite Imagery Multi-Spectral

**Concept**: Multi-spectral TIFF (8 bands) → 8-pad velocity curve from per-band brightness histogram

### Data Source & Format
- Sentinel-2 via Copernicus Open Access Hub: `scihub.copernicus.eu`
- Landsat via USGS EarthExplorer: `earthexplorer.usgs.gov`
- Format: GeoTIFF, 8–13 bands, 10–30m resolution
- Bands (Sentinel-2): B1 (coastal aerosol), B2 (blue), B3 (green), B4 (red), B5–B7 (vegetation red edge), B8 (NIR), B11–B12 (SWIR)
- Values: 0–10000 (reflectance ×10000)

### Python Parsing Approach
```python
import rasterio
import numpy as np

velocities = []
with rasterio.open('sentinel2_scene.tif') as src:
    for band_idx in range(1, min(9, src.count + 1)):
        band = src.read(band_idx).astype(float)
        # Mask nodata
        band = band[band > 0]
        # Mean brightness normalized to 1-127
        mean_brightness = np.mean(band)
        # Sentinel-2 reflectance range: typically 0–4000 for most surfaces
        velocity = int(np.clip((mean_brightness / 4000.0) * 126 + 1, 1, 127))
        velocities.append(velocity)

# Each band → one pad's base velocity
# Coastal water (B1) → subtle, low velocity
# Desert/bare soil (B4 high, B8 low) → harsh high velocity
# Forest (B8 NIR high) → mid velocity with long sustain mapping
```

### Mapping to XPN Parameters
| Source field | XPN target |
|---|---|
| Per-band mean brightness | pad base velocity |
| Band 1 (coastal aerosol) | pad 1 — subtle dynamics |
| Band 4 (red/visible) | pad 4 — mid punch |
| Band 8 (NIR) | pad 5 — organic body |
| Band 11/12 (SWIR, heat) | pad 7/8 — harsh spikes |
| Spatial variance | velocity layer spread |
| Scene classification (water/urban/vegetation) | kit mood selection |

### Creative Sonic Result
A coastal estuary scene: B1 (coastal aerosol) is high → pad 1 gets a soft, breathy attack. B8 (vegetation NIR) is high → pad 5 gets organic body. B4 (red) is low over water → pad 4 is ghost note level. Result: a kit built from the reflectance signature of a specific place on Earth. Compare two images of the same location in different seasons → two kit presets that are structurally related but timbrally contrasted.

### Estimated Complexity: **Medium**
Requires `rasterio` and `numpy`. GeoTIFF multi-band reading is straightforward. Main challenge is selecting a geographically interesting scene and normalizing for cloud cover / atmospheric correction.

---

## Pipeline 05 — Poetry Scansion

**Concept**: Iambic pentameter and other metrical verse → velocity-mapped rhythm pattern via CMU Pronouncing Dictionary

### Data Source & Format
- Any poem text file (UTF-8)
- CMU Pronouncing Dictionary: `pronouncing` Python library or `nltk.corpus.cmudict`
- Stress values: 0 = no stress, 1 = primary stress, 2 = secondary stress
- Input example: "Shall I compare thee to a summer's day?" (Shakespeare Sonnet 18)

### Python Parsing Approach
```python
import pronouncing
import re

def stress_sequence(text):
    words = re.findall(r"[a-zA-Z']+", text.lower())
    stresses = []
    for word in words:
        phones = pronouncing.phones_for_word(word)
        if phones:
            # Get stress pattern from first pronunciation
            stress = pronouncing.stresses(phones[0])
            stresses.extend(list(stress))
        else:
            stresses.append('0')  # unknown word = unstressed
    return stresses

def stress_to_velocity(stress_char):
    mapping = {'1': 120, '2': 80, '0': 30}
    return mapping.get(stress_char, 30)

poem = open('sonnet18.txt').read()
lines = poem.strip().split('\n')

events = []
position = 0
for line in lines:
    stresses = stress_sequence(line)
    for syllable_idx, s in enumerate(stresses):
        vel = stress_to_velocity(s)
        events.append({
            'position_16th': position,
            'velocity': vel,
            'pad': 1 if s == '1' else (2 if s == '2' else 3)
        })
        position += 1  # each syllable = 1/16th note
```

### Mapping to XPN Parameters
| Source field | XPN target |
|---|---|
| Primary stress (1) | accent hit — velocity 100–127 |
| Secondary stress (2) | mid hit — velocity 50–90 |
| Unstressed (0) | ghost note — velocity 10–40 |
| Line break | bar boundary / rest |
| Caesura (mid-line pause) | syncopation marker |
| Poem meter (iambic/trochaic) | rhythmic feel preset |

### Creative Sonic Result
Shakespeare's iambic pentameter produces a metronomic da-DUM da-DUM pattern with ghost notes filling the unstressed syllables — a rhythm that is both mathematically regular and naturally musical. Free verse (Whitman, Ginsberg) creates irregular ghost-note-heavy patterns with unexpected accent clusters. Epic poetry (Homer, Virgil in translation) generates long-form drum patterns across hundreds of lines. A full sonnet sequence (154 sonnets) creates a multi-kit pack.

### Estimated Complexity: **Easy**
`pronouncing` library wraps CMU dict cleanly. Text normalization (punctuation, contractions) adds minor complexity. Grid placement of variable-length lines requires a decision about syllable-to-grid mapping.

---

## Pipeline 06 — DNA Codon Sequences

**Concept**: FASTA gene file → 64-pad velocity map based on codon frequency in the gene

### Data Source & Format
- NCBI GenBank: `ncbi.nlm.nih.gov/nuccore/`
- Format: FASTA (`.fasta` / `.fa`) — plain text, header line starting with `>`, sequence lines
- Codons: 3-nucleotide groups from ACGT alphabet → 4³ = 64 possible codons
- 64 codons → 20 amino acids + 3 stop codons
- Input: any coding DNA sequence (CDS)

### Python Parsing Approach
```python
from collections import Counter

def read_fasta(filename):
    sequences = {}
    current = None
    with open(filename) as f:
        for line in f:
            line = line.strip()
            if line.startswith('>'):
                current = line[1:].split()[0]
                sequences[current] = ''
            elif current:
                sequences[current] += line.upper()
    return sequences

# All 64 codons
BASES = 'ACGT'
ALL_CODONS = [a+b+c for a in BASES for b in BASES for c in BASES]
CODON_TO_PAD = {codon: i for i, codon in enumerate(sorted(ALL_CODONS))}

def gene_to_kit(fasta_path, gene_id):
    seqs = read_fasta(fasta_path)
    seq = seqs[gene_id]
    codons = [seq[i:i+3] for i in range(0, len(seq)-2, 3)
              if len(seq[i:i+3]) == 3]
    counts = Counter(codons)
    total = sum(counts.values())

    pad_velocities = {}
    for codon, pad_idx in CODON_TO_PAD.items():
        freq = counts.get(codon, 0) / total
        # Rare codons → low velocity; frequent codons → high velocity
        velocity = int(np.clip(freq * 127 * 20, 1, 127))  # scale to useful range
        pad_velocities[pad_idx] = velocity

    return pad_velocities
```

### Mapping to XPN Parameters
| Source field | XPN target |
|---|---|
| Codon identity (64 values) | pad selection (4 banks × 16 pads) |
| Codon frequency in gene | base velocity for that pad |
| Codon position in sequence | trigger timing (1 codon = 1/16th note) |
| Stop codon (TAA/TAG/TGA) | rest / velocity 0 |
| GC content | filter cutoff bias |
| Repeat sequences (tandem repeats) | loop markers |

### Creative Sonic Result
Human insulin gene (short, ~333 bp) → a 111-step pattern. BRCA1 tumor suppressor (long, ~81k bp) → a sprawling multi-hour pattern. Highly expressed genes (abundant mRNA) have biased codon usage toward a specific subset → kits with 10–15 active pads while others stay ghost-note level. Viral genomes (HIV, SARS-CoV-2) have distinct codon usage biases compared to human genes — two kits from the same drum samples that have structurally different velocity maps based on evolutionary biology.

### Estimated Complexity: **Easy**
No exotic libraries. Pure Python string parsing. Codon table is static. Main creative decision is how aggressively to scale frequency to velocity range.

---

## Pipeline 07 — Git Commit History

**Concept**: Git repository commit log → drum pattern where coding sessions become polyrhythmic bursts

### Data Source & Format
- Any git repository accessible locally
- `git log --format="%H %ai %s" --numstat` → commit hash, timestamp, subject, file counts
- Key metrics: timestamp (ISO), files changed (from numstat), insertions, deletions
- Public repos via GitHub API: `api.github.com/repos/{owner}/{repo}/commits`

### Python Parsing Approach
```python
import subprocess
import re
from datetime import datetime

def get_commits(repo_path):
    result = subprocess.run(
        ['git', 'log', '--format=%H|%ai|%s', '--shortstat'],
        cwd=repo_path, capture_output=True, text=True
    )
    commits = []
    lines = result.stdout.strip().split('\n')
    current = None
    for line in lines:
        if '|' in line and len(line.split('|')) >= 3:
            parts = line.split('|', 2)
            current = {
                'hash': parts[0],
                'time': datetime.fromisoformat(parts[1]),
                'subject': parts[2],
                'files': 0
            }
            commits.append(current)
        elif current and 'changed' in line:
            m = re.search(r'(\d+) file', line)
            if m:
                current['files'] = int(m.group(1))
    return commits

commits = get_commits('/path/to/repo')
# Time between commits → rhythm spacing
# Files changed → velocity (1 file = 20, 50 files = 127)
# Burst sessions (< 5 min between commits) → 16th note density
# Long gaps (> 1 day) → bar reset
```

### Mapping to XPN Parameters
| Source field | XPN target |
|---|---|
| Files changed | velocity (clamped 1–127) |
| Time between commits | inter-note duration |
| Commit subject keywords ("fix"/"feat"/"refactor") | pad selection |
| Lines added | filter open amount |
| Lines deleted | filter close / mute |
| Burst session (rapid commits) | 32nd/64th note density |
| Long pause (days) | new section / phrase |

### Creative Sonic Result
The Linux kernel git history (50+ years of commits) produces an extraordinarily dense kit with statistically valid burst patterns matching major release cycles. A solo developer's side project produces sparse ghost note patterns with occasional intense velocity spikes. XO_OX's own XOmnibus repo history would produce a kit that literally encodes the development timeline as rhythm. Meta-artistic: the act of building the instrument becomes the drum pattern.

### Estimated Complexity: **Easy**
`subprocess` + stdlib `datetime`. GitHub API version requires `requests`. No DSP or scientific computing.

---

## Pipeline 08 — Weather Patterns

**Concept**: Historical weather time series → kit with cross-fade between sample sets based on temperature/pressure

### Data Source & Format
- NOAA Climate Data Online: `ncdc.noaa.gov/cdo-web/`
- Open-Meteo API (free): `api.open-meteo.com/v1/archive`
- CSV: `date, temp_max, temp_min, precipitation, wind_speed, pressure_hpa, cloud_cover`
- Daily or hourly granularity

### Python Parsing Approach
```python
import requests
import pandas as pd

# Open-Meteo example: 1 year of hourly data for a location
url = "https://archive-api.open-meteo.com/v1/archive"
params = {
    "latitude": 37.77, "longitude": -122.41,
    "start_date": "2025-01-01", "end_date": "2025-12-31",
    "hourly": "temperature_2m,surface_pressure,precipitation,windspeed_10m"
}
data = requests.get(url, params=params).json()
df = pd.DataFrame(data['hourly'])

# Temperature → cross-fade position between 2 sample sets
# e.g., cold (0°C) = sample set A (dark, padded), hot (40°C) = sample set B (bright, dry)
df['xfade'] = ((df['temperature_2m'] - 0) / 40.0).clip(0, 1)

# Pressure drop rate → filter sweep velocity
df['pressure_delta'] = df['surface_pressure'].diff()
df['filter_vel'] = ((df['pressure_delta'].abs() / 5.0) * 127).clip(1, 127).astype(int)

# Precipitation → rim shot accent
df['precip_vel'] = ((df['precipitation'] / 20.0) * 127).clip(0, 127).astype(int)
```

### Mapping to XPN Parameters
| Source field | XPN target |
|---|---|
| Temperature | sample set crossfade (A=cold, B=hot) |
| Pressure drop rate | filter sweep / velocity accent |
| Precipitation | rim shot / accent pad velocity |
| Wind speed | LFO rate proxy (stored as velocity layer hint) |
| Cloud cover % | overall mix wetness / reverb depth |
| Storm events | velocity spike + filter open |
| Calm days | ghost note baseline |

### Creative Sonic Result
One year of San Francisco weather produces a kit that cycles between seasons: foggy summer (cold = dark samples, moderate velocity), dry autumn (warming = brighter samples), storm fronts (pressure drops = velocity spikes). Compare the same date range for two cities (NYC vs Miami) → two kits, structurally similar in timing but tonally contrasted by temperature regime. Hurricanes create extreme velocity events. Drought years are flat and dry.

### Estimated Complexity: **Easy**
Open-Meteo API is free, no authentication. `requests` + `pandas`. Cross-fade is a simple normalized value.

---

## Pipeline 09 — Stock Market OHLC

**Concept**: Open/High/Low/Close candles → 4-velocity-layer kit with volume as pad sensitivity

### Data Source & Format
- Yahoo Finance via `yfinance` library (free)
- Alpha Vantage API (free tier): `alphavantage.co`
- Format: daily/hourly OHLC + Volume
- Fields: `Open, High, Low, Close, Volume, Date`

### Python Parsing Approach
```python
import yfinance as yf
import numpy as np

ticker = yf.Ticker("SPY")  # S&P 500 ETF as example
hist = ticker.history(period="1y", interval="1d")

# Normalize each OHLC value to 1-127 within period range
def normalize(series):
    return ((series - series.min()) /
            (series.max() - series.min()) * 126 + 1).astype(int)

hist['open_vel']  = normalize(hist['Open'])
hist['high_vel']  = normalize(hist['High'])
hist['low_vel']   = normalize(hist['Low'])
hist['close_vel'] = normalize(hist['Close'])

# Volume → pad sensitivity (high volume day = all pads louder)
hist['vol_scale'] = normalize(hist['Volume'])

# Volatility: daily range (High-Low) / Close
hist['volatility'] = (hist['High'] - hist['Low']) / hist['Close']
# High volatility → LFO rate proxy (encoded in metadata)
hist['lfo_rate_hint'] = (hist['volatility'] * 100).clip(0, 100).astype(int)

# Each trading day = 1 bar; each OHLC value = 1 beat of the bar
# Open = beat 1, High = beat 2, Low = beat 3, Close = beat 4
```

### Mapping to XPN Parameters
| Source field | XPN target |
|---|---|
| Open | beat 1 velocity |
| High | beat 2 velocity (often loudest on trending day) |
| Low | beat 3 velocity (ghost note on bullish day) |
| Close | beat 4 velocity |
| Volume | global velocity scalar / pad sensitivity |
| Volatility index (VIX) | LFO rate hint |
| Price gap (previous close → current open) | accent / velocity jump |
| Earnings announcement days | extreme velocity spike events |

### Creative Sonic Result
Bull market year (SPY 2023): consistent upward velocity trend, Close > Open most days → beat 4 usually louder than beat 1, producing a forward-leaning rhythmic feel. 2020 COVID crash: extreme volatility → wild velocity variation, daily candle swings map to rhythmic chaos then recovery. Comparing two stocks with high/low correlation produces kits that are rhythmically similar or distinct based on price correlation. Crypto (BTC) produces dramatically more volatile kits than treasury bonds.

### Estimated Complexity: **Easy**
`yfinance` is excellent, free, and well-maintained. Pure pandas normalization.

---

## Pipeline 10 — Sports Play-by-Play

**Concept**: NBA/soccer event log → each event type maps to a different pad, crowd noise = velocity

### Data Source & Format
- NBA: `nba.com/stats` or `stats.nba.com` API (undocumented but accessible)
- NBA Play-by-Play via `nba_api` library
- Soccer: StatsBomb open data: `github.com/statsbomb/open-data`
- Format: timestamped event records — `time, event_type, player, team, coordinates`
- NBA event types: made shot, missed shot, free throw, turnover, foul, timeout, rebound
- Soccer: pass, shot, dribble, tackle, clearance, substitution, goal

### Python Parsing Approach
```python
from nba_api.stats.endpoints import playbyplayv2
import pandas as pd

# Example: retrieve play-by-play for a game
pbp = playbyplayv2.PlayByPlayV2(game_id='0022300001')
df = pbp.get_data_frames()[0]

# Event type mapping to pad
EVENT_PAD = {
    1: 1,   # Made shot → kick drum
    2: 2,   # Missed shot → snare
    3: 3,   # Free throw → hi-hat
    4: 4,   # Rebound → clap
    5: 5,   # Turnover → rim shot
    6: 6,   # Foul → crash
}

# Game clock to beat position
def clock_to_seconds(clock_str):
    # Format: "PT12M00.00S" (NBA) or "MM:SS"
    parts = clock_str.replace('PT','').replace('S','').split('M')
    return int(parts[0]) * 60 + float(parts[1])

events = []
for _, row in df.iterrows():
    pad = EVENT_PAD.get(row['EVENTMSGTYPE'], 8)
    time_sec = clock_to_seconds(row['PCTIMESTRING'])
    # Score margin → velocity (blowout = low energy, close game = high velocity)
    score_diff = abs(row.get('SCOREMARGIN', 0) or 0)
    velocity = max(1, min(127, 127 - score_diff * 2))
    events.append({'pad': pad, 'time_sec': time_sec, 'velocity': velocity})
```

### Mapping to XPN Parameters
| Source field | XPN target |
|---|---|
| Event type | pad selection |
| Game clock | timing (seconds → beat quantization) |
| Score margin | inverse velocity (close game = high energy) |
| Quarter / Half | section boundary |
| Timeout | silence / rest |
| Goal / Made shot | velocity 127 accent |
| Overtime | extended pattern / extra bars |
| Fast break (< 5s possession) | 16th note density burst |

### Creative Sonic Result
A close playoff game in the 4th quarter generates dense, high-velocity patterns with rapid event clustering — genuine crowd energy mapped to drum velocity. A blowout game generates predictable, low-energy ghost-note patterns. Comparing the same team's win vs loss games produces rhythmically distinct kits from the same sample palette. Soccer's lower event density produces sparser, more syncopated patterns than basketball. A full 48-minute NBA game → a roughly 5-minute drum pattern at 120 BPM.

### Estimated Complexity: **Medium**
`nba_api` is stable but the API can be rate-limited. StatsBomb open data is JSON and straightforward. Event type normalization varies between sports data providers.

---

## Implementation Notes

### Shared Infrastructure
All 10 pipelines share a common output target: a Python dict structure that maps to the XPN tool suite's existing `generate_xpms.py`:

```python
{
    'bpm': float,
    'events': [
        {
            'pad': int,          # 1-16
            'position_16th': int, # beat position in 1/16th note units
            'velocity': int,     # 1-127
            'layer': int         # 1-4 velocity layer
        }
    ],
    'pad_settings': {
        pad_idx: {
            'base_velocity': int,
            'filter_cutoff': float,  # 0.0-1.0
            'attack': float,
            'decay': float
        }
    }
}
```

### Quantization Strategy
Non-audio sources have continuous timing. Quantization decisions:
- **Hard quantize**: snap to nearest 1/16th (grid-locked, robotic feel)
- **Soft quantize**: snap + random offset within ±1/32nd (humanized)
- **Free time**: preserve raw timing, generate non-quantized XPN (requires MPC swing/quantize at playback)

Recommended default: soft quantize at 75% strength.

### Priority for Implementation
| Pipeline | Data availability | Musical interest | Complexity | Priority |
|---|---|---|---|---|
| Git Commit History | Immediate (any repo) | High (meta-artistic) | Easy | **1** |
| Poetry Scansion | Immediate (any text) | High (lyrical connection) | Easy | **2** |
| ECG Heartbeat | Free download | Very high | Medium | **3** |
| Seismograph | Free USGS API | High | Easy | **4** |
| Weather | Free API | High (geographic identity) | Easy | **5** |
| Stock OHLC | Free API | Medium | Easy | **6** |
| DNA Codon | Free NCBI | High (biological) | Easy | **7** |
| Sports Play-by-Play | Moderate access | Medium | Medium | **8** |
| Satellite Imagery | Free but large files | Medium | Medium | **9** |
| Protein Folding | Free but niche | Niche | Medium | **10** |

---

*Document prepared for XO_OX Designs R&D. Part of the XPN Tool Suite expansion research.*
