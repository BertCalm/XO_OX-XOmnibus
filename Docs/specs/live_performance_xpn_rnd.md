# Live Performance XPN — R&D Session
**Authors**: Scout + Vibe (Community Intel + Sound Design Androids, XO_OX)
**Date**: 2026-03-16
**Status**: R&D Reference — Production Pipeline Candidates

---

## Foreword: The Static Kit Problem

Every XPN pack ever shipped assumes the same temporal relationship to performance: the producer selects a kit before the set, loads it, and plays it. The kit is inert. It does not know what time it is, what room it is in, how many people are listening, or what the producer has been playing for the last four bars. It is a photograph of a sound design decision made weeks ago in a studio.

XOceanus has 34 synthesis engines, a 6D Sonic DNA system, Monster Rancher source-material scanning, a Curiosity Engine, cross-engine coupling, and 2,496 presets across 7 moods. None of this infrastructure has been leveraged to make a kit that CHANGES during performance — that is aware of the room, the crowd, the history of the set, or the timbral gaps in the current mix.

This document is about closing that gap. Not speculatively. Concretely. Every concept below has a defined XPM implementation path, specific parameter recommendations, and a named Oxport tool wherever new tooling is required.

---

## Section 1: Live XPN — Kits That Evolve During Performance

### 1.1 Temporal Kit Morphing

**The Problem With Static Kits at a Live Set Level**

A two-hour set played with a single kit sounds like two hours of the same texture. Even if the beat structures change, the timbral palette does not. The kick sounds the same in the opening set as it does at the peak. Great live performers solve this manually — they switch kits between songs, keep multiple programs loaded, swap pad banks at critical moments. But this requires explicit cognitive effort and interrupts the performance state.

Temporal morphing automates that arc.

**Concept**

Generate N "snapshots" of a core kit at interpolated parameter states between a START identity and an END identity. Each snapshot is a fully rendered, standalone XPN file. The set progresses through them in order. By the end of the set, every sound has drifted — subtly or dramatically — from where it began.

The arc is not random. It is designed. The kick starts warm and round (high sub energy, slow transient) and ends tight and aggressive (punchy 80Hz click, fast transient). The hi-hat starts filtered and intimate (−12dB LPF at 8kHz) and ends bright and cutting (boosted presence at 12kHz). The pad sound starts spacious (long verb tail, −12dB dry) and ends dry and exposed (short hall, −3dB dry). The audience perceives this as the set "building," even if the BPM and song structures stay consistent.

**Architecture**

```
start_kit.xometa   (rendering state at t=0)
end_kit.xometa     (rendering state at t=N_steps)
        │
        ▼
xpn_evolution_builder.py
        │
        ▼
step_01.xpn  step_02.xpn  ...  step_16.xpn
        │
        ▼
Delivery options (see below)
```

**What Gets Interpolated**

Not all parameters interpolate meaningfully. The Oxport tool interpolates within these classes only:

| Parameter Class | Interpolation Method | Example |
|----------------|---------------------|---------|
| Filter cutoff | Linear in log-frequency space (perceptually uniform) | 800Hz → 3200Hz over 16 steps |
| Attack time | Linear in log-time (6ms → 80ms) | Snap → swell |
| Release time | Linear in log-time | 200ms → 1200ms |
| Reverb mix | Linear in dB | −18dB → −6dB wet |
| Saturation drive | Linear in dB | 0dB → +9dB |
| Velocity-to-filter amount | Linear | 0.10 → 0.40 |
| Noise level (perc_) | Linear in dB | −24dB → −12dB |
| Pitch tuning | Linear in semitones | 0 → +2st (subtle detuning over time) |

Parameters that do NOT interpolate (discrete states, would cause audible glitch if crossfaded):
- Kit mode (velocity / cycle / random)
- Mute group assignments
- Sample file assignments
- OneShot flag

**The 16-Step Default**

16 steps maps to 16 songs in a standard set. Step 1 loads at the opening. Step 16 loads at the peak or encore. The producer carries 16 XPN files on an external drive or MPC internal storage and loads the next file during the previous song's final 8 bars — an operation that takes 2–3 seconds and can be done without interrupting playback if the program is not currently active.

**Alternative: 8-Step Arc for Shorter Sets**

For a 45-minute club set (5–8 songs), 8 steps with larger per-step delta creates a more dramatic evolution. The Oxport tool defaults to 16 steps but accepts `--steps 8` for condensed arcs.

**Delivery Option A: Manual Swap (16 Separate XPNs)**

Simplest. Most portable. No MPC firmware dependency.

```
/live_set_evolution/
  step_01_opening.xpn        (warm, intimate)
  step_04_build.xpn          (energy beginning to rise)
  step_08_midpoint.xpn       (balanced)
  step_12_peak_approach.xpn  (bright, aggressive)
  step_16_peak.xpn           (maximum energy state)
```

The producer loads each file to a dedicated program slot before the set begins, then switches active programs between songs. On MPC Live III / MPC X, this is a two-tap gesture from the Program Select view.

**Delivery Option B: Pad Bank Trick (All 16 States in One XPM)**

MPC drum programs support pad banks (Bank A / B / C / D). Each bank holds 16 pads = 4 banks × 16 pads = 64 instrument slots in one program file.

Reframe: instead of 16 pads per bank being 16 drums, use each BANK as a complete kit snapshot with only the most critical 16 sounds. The producer switches banks at key moments — Bank A for the opening 4 songs, Bank B for the build, Bank C for the peak, Bank D for the closing.

This approach loses per-voice flexibility (4-layer velocity per voice per bank would exceed the XPM instrument count limit), but gains instant switching with zero loading time. The entire arc lives in RAM because it loaded at set start.

**XPM Structure for Bank Trick (4 kit states):**
```xml
<!-- Bank A: Pads 1–16, kit snapshot at t=0 (warm, opening) -->
<Instrument number="0"  name="kick_snap_A"   />  <!-- Pad 1, Bank A -->
...
<Instrument number="15" name="fx_open_A"     />  <!-- Pad 16, Bank A -->

<!-- Bank B: Pads 17–32, kit snapshot at t=5 (building) -->
<Instrument number="16" name="kick_build_B"  />  <!-- Pad 1, Bank B -->
...
<!-- etc. -->
```

**Delivery Option C: OSC-Triggered Program Change**

If the producer runs a timeline DAW (Ableton, Logic) as a clock source alongside the MPC, MIDI Program Change messages can trigger program swaps. This automates the evolution — the kit morphs on the first beat of the designated bar with no producer intervention.

MPC Live III and MPC X respond to MIDI Program Change (CC#0 bank select + PC message). The Oxport tool generates a MIDI sequence file alongside the XPN pack: `evolution_sequence.mid` — a single-track file with 16 Program Change messages, one per 8-bar phrase at the user-specified BPM.

**Oxport Tool: `xpn_evolution_builder.py`**

```
python3 xpn_evolution_builder.py \
  --start-kit Onset_808_Reborn.xometa \
  --end-kit Onset_Apex_State.xometa \
  --steps 16 \
  --output ./live_set_evolution/ \
  --delivery [separate|bank-trick|midi-sequence] \
  --bpm 140 \
  --bars-per-step 8
```

Output manifest (`evolution_manifest.json`):
```json
{
  "arc_name": "808 Reborn → Apex State",
  "steps": 16,
  "bpm": 140,
  "bars_per_step": 8,
  "total_duration_bars": 128,
  "start_dna": {"brightness": 0.3, "aggression": 0.4, ...},
  "end_dna":   {"brightness": 0.8, "aggression": 0.9, ...},
  "per_step_deltas": [
    {"step": 1, "filter_cutoff_hz": 800, "attack_ms": 6, "reverb_mix_db": -18},
    {"step": 2, "filter_cutoff_hz": 945, "attack_ms": 7, "reverb_mix_db": -17.1},
    ...
  ],
  "files": ["step_01_opening.xpn", "step_02.xpn", ...]
}
```

The tool also computes perceptual delta per step: if any single step changes the DNA distance by more than 0.15 (Euclidean in 6D space, same metric as `dna_distance()` in `xpn_curiosity_engine.py`), it flags it as a "jump" in the manifest and recommends splitting that step further.

---

### 1.2 Generative XPN Between Songs: The Set-Aware Curiosity Engine

**Concept**

Rather than pre-rendered snapshots, this mode generates a new kit between songs in real-time using the Curiosity Engine's `mutate` mode as its core, guided by a live context feed: which presets have been used, at what velocity density, for how long.

**The Live Context Profile**

The producer maintains a `set_dna_profile.json` that accumulates data during the performance:

```json
{
  "pads_hit": {"kick": 312, "snare": 289, "chat": 445, "clap": 88},
  "avg_velocity": {"kick": 0.72, "snare": 0.61, "chat": 0.45},
  "peak_velocity_events": 14,
  "estimated_dna": {
    "brightness": 0.55,
    "aggression": 0.68,
    "density": 0.72,
    "space": 0.41
  },
  "songs_played": 4,
  "current_bpm": 140
}
```

This profile is updated manually (producer fills a JSON template after each song, or via a companion iOS shortcut that prompts for ratings). In a future hardware integration, the MPC's MIDI output could auto-populate this during playback — but that is V2. V1 is manual.

**Generation Pipeline (between-song window, ~90 seconds)**

```
set_dna_profile.json
        │
        ▼
Infer what DNA has been OVERREPRESENTED (high density, high aggression)
Infer what DNA has been UNDERREPRESENTED (low brightness, low space)
        │
        ▼
xpn_curiosity_engine.py --mode mutate
  --seed-dna <inverted underrepresented dimensions>
  --n 8
  --engine ONSET
        │
        ▼
8 candidate mutations, scored by DNA distance from current set state
        │
        ▼
Auto-select top-scoring candidate (maximum entropy from set history)
        │
        ▼
Render via xpn_drum_export.py --mode smart
        │
        ▼
New kit ready for next song
```

The inversion logic mirrors `invert_dna_dimensions()` already in `xpn_curiosity_engine.py`. If the set has been high-density and high-aggression for 4 songs, the generated kit skews toward low-density and low-aggression — a textural reset that gives the audience a moment to breathe before the next build. This is the same principle that makes great DJ sets work: contrast is tension management.

**Generation Time Estimate**

With numpy/scipy available and pre-rendered WAV assets (no real-time render), the pipeline above runs in under 10 seconds on a 2021 M1 MacBook Air. The bottleneck is the Curiosity Engine mutation scoring, which evaluates 8 candidates × 6 DNA dimensions — trivial computation. If real-time rendering via XOceanus is required (offline render of 8 voice channels), add 20–40 seconds per kit on Apple Silicon.

---

### 1.3 The Monster Rancher Performance Mode: Venue-Specific Kit

**Concept**

Record 30 seconds of ambient room audio from the MPC's built-in microphone (or a phone) during the soundcheck. Feed it to `xpn_monster_rancher.py`. The kit generated is tuned to the spectral character of that room and that crowd.

**Why This Works**

Every room has a modal structure determined by its dimensions, materials, and contents. A room's modal frequencies reinforce sound at those frequencies and cancel it at others. A kick drum tuned to a room's primary mode (approximately `speed_of_sound / (2 × room_length)` for the fundamental room mode; a 15-meter room has a fundamental around 11Hz, but the first audible harmonic is ~23Hz at the second mode) will energize the room's resonant structure — it sounds physically bigger than a generic kick. A hi-hat designed with spectral peaks at the room's resonant frequencies in the 8–12kHz range will cut through rather than smearing.

This is not superstition. Architects and live sound engineers know this. XPN is the mechanism for baking it into the sample set.

**Implementation**

```
python3 xpn_monster_rancher.py \
  venue_ambient_30sec.wav \
  --output ./tonight_kit/ \
  --mode kit \
  --pads 8 \
  --room-aware True \
  --room-length-meters 12
```

When `--room-aware True` is passed, the tool's spectral fingerprint pipeline (already in Monster Rancher's 8-band analysis) adds a post-processing stage:

1. Extract the room IR peaks from the ambient recording using autocorrelation — locations of strong spectral buildup indicate room modes.
2. Identify the top 3 resonant peaks in the 60–500Hz range (kick/bass territory) and the top 3 in the 6–16kHz range (hat/clap territory).
3. Tag the DNA fingerprint with `room_peaks_low` and `room_peaks_high` arrays.
4. When rendering percussion samples, apply gentle boost (+2dB, Q=4.0) at the identified low peaks for kick and bass sounds; apply narrow boost (+1.5dB, Q=6.0) at high peaks for hats and claps.

The boosts are subtle — the goal is reinforcement, not aggressive EQ. The audience will not notice the processing; they will notice that the kick feels heavier in this room than any kick they have heard in this room before.

**The Room Mode Kick Formula (specific parameters)**

Given a room length L in meters:

```
c = 343 m/s (speed of sound at 20°C)
fundamental_mode_hz = c / (2 × L)

# For a 12-meter club: 343 / 24 = 14.3Hz (subsonic)
# First audible harmonic: 28.6Hz — tune the kick's sub resonance to 28–30Hz
# Second harmonic: 57Hz — boost here for room-sympathetic body
# Third harmonic: 86Hz — this is the click/thud crossover region
```

The kick's attack transient should peak at the third harmonic (L-dependent) for maximum perceived punch in that room. The `--room-length-meters` parameter drives this calculation automatically. If not provided, the tool estimates room length from the ambient recording's early reflection delay (the first reflection after a transient in the ambient noise, multiplied by c/2).

---

### 1.4 The Contextual Kit: Maximum Entropy Curation From What You Just Played

**Concept**

After 4 bars of playing, analyze which pads were hit, at what velocity density, and what spectral regions they occupied. Generate a complementary kit that maximizes entropy — fills the spectral regions that were absent, at the velocity dynamics that were underused.

This is real-time `xpn_complement_renderer.py` logic applied to live performance rather than static preset curation.

**Implementation**

`xpn_complement_renderer.py` already exists in the Tools directory. Its current use is generating preset pairs for the complement curation workflow. The performance mode adaptation requires:

1. A live MIDI capture window (4 bars, read from MPC MIDI output).
2. Inference of which DNA dimensions the captured MIDI implies (high note density → high `density` DNA, mostly soft hits → low `aggression` DNA).
3. A call to the complement renderer with the inferred DNA as input.
4. XPN generation of the complement kit, rendered in the between-song window.

The complement renderer already uses `invert_dna_dimensions()` internally. The extension is the MIDI-to-DNA inference layer, which maps:

| MIDI feature | DNA dimension | Mapping |
|-------------|---------------|---------|
| Note-on events per bar (density) | `density` | linear, 8 hits/bar = 0.5, 16+ = 1.0 |
| Average note velocity | `aggression` | linear, 64 = 0.5, 127 = 1.0 |
| Pitch register (if keygroup pads) | `brightness` | low register = 0.2, high = 0.8 |
| Note duration / sustain | `space` | short one-shots = 0.3, long pads = 0.8 |

The implementation is a 30-line addition to `xpn_complement_renderer.py`: a `--from-midi` flag that accepts a MIDI file captured from the last N bars, runs the inference, and passes the result to the existing complement generation pipeline.

---

## Section 2: Performance-Optimized XPM Design

### 2.1 Low-Latency Kit Architecture

**The Latency Sources in XPN Playback**

MPC sample playback latency has three components:

1. **Buffer latency**: MPC's audio I/O buffer. Fixed at the hardware level (typically 64–256 samples at 44.1kHz = 1.45–5.8ms). Not addressable through XPN design.
2. **Sample seek time**: If the sample is not in RAM — i.e., if disk streaming is active — the first hit will stall while the MPC seeks the file. This is addressable.
3. **Initial silence**: If a sample has several milliseconds of digital silence before the transient (from a sloppy render or overly padded export), that silence is played back as dead air. Perceived latency = actual latency + initial silence. This is fully addressable.

**Targets and Parameters**

```
Max kit RAM footprint:        50MB (fits in MPC Live III sample RAM, no streaming)
Max individual sample size:   1.5MB (mono 16-bit 44.1kHz, ~17 seconds — far longer than any one-shot needs)
Sample rate:                  44.1kHz (MPC native; 48kHz adds 9% more data for no audible benefit on drum one-shots)
Bit depth:                    16-bit PCM for one-shots, 24-bit only for sustained content (keys, pads, long tails)
Channels:                     Mono for kick, snare, hat, clap, tom (stereo = 2× RAM, stereo information is nearly inaudible at high SPL in a club)
                              Stereo retained for FX/Cymbal and long percussion tails
Normalization:                -1.0dBFS — avoids automatic gain compensation lag on first-hit
Start trim:                   0ms silence — sample data begins at first non-zero sample above a -60dBFS threshold
```

**Why Mono for Drums**

Below 200Hz (kick, sub bass), stereo information is inaudible — the wavelengths are longer than ear spacing and the auditory system cannot extract inter-aural time differences. Kick drums are effectively always mono at the playback stage even if rendered stereo. Converting to mono halves the file size with zero perceptual cost.

For hats, claps, and snares: the stereo width of a drum one-shot in a club context is obliterated by the room reverb within 50ms. The perceived spatial information comes from the room, not the file. Mono samples at -1dBFS sound just as wide in a club as stereo samples at -3dBFS, because the room provides the spatial smear. Stereo is valuable in headphone listening and studio contexts — not in live performance contexts.

**The `--performance-optimized` Flag for `xpn_drum_export.py`**

Add a `--performance-optimized` flag that applies the following pipeline to every WAV before XPM generation:

```python
def apply_performance_optimizations(wav_path: Path, output_path: Path, voice: str) -> dict:
    """
    Returns a report dict with before/after sizes and applied operations.
    """
    operations = []

    # 1. Load
    sr, data = scipy.io.wavfile.read(wav_path)

    # 2. Stereo-to-mono for one-shot voices
    ONE_SHOT_VOICES = {"kick", "snare", "chat", "ohat", "clap", "tom", "perc"}
    if data.ndim == 2 and voice in ONE_SHOT_VOICES:
        data = data.mean(axis=1)  # L+R average, not sum (avoids 6dB gain)
        operations.append("stereo_to_mono")

    # 3. Bit depth reduction for one-shots (24→16 with triangular dither)
    if data.dtype == np.int32 and voice in ONE_SHOT_VOICES:
        # Triangular dither before quantization
        dither = np.random.triangular(-1.0, 0.0, 1.0, size=data.shape)
        data = (data.astype(np.float64) / 2147483648.0)  # 32-bit signed normalize
        data = (data * 32767.0 + dither).clip(-32768, 32767).astype(np.int16)
        operations.append("24bit_to_16bit_dithered")

    # 4. Silence trim — find first sample above -60dBFS threshold
    threshold_linear = 10 ** (-60 / 20) * np.iinfo(data.dtype).max
    first_nonsilent = np.argmax(np.abs(data) > threshold_linear)
    if first_nonsilent > 0:
        data = data[first_nonsilent:]
        operations.append(f"trimmed_{first_nonsilent}_samples")

    # 5. Normalize to -1.0dBFS
    peak = np.abs(data).max()
    target_peak = np.iinfo(data.dtype).max * 10 ** (-1.0 / 20)
    if peak > 0:
        data = (data * (target_peak / peak)).clip(
            np.iinfo(data.dtype).min, np.iinfo(data.dtype).max
        ).astype(data.dtype)
        operations.append("normalized_to_minus1dBFS")

    # 6. Write
    scipy.io.wavfile.write(output_path, sr, data)

    return {
        "operations": operations,
        "size_before_bytes": wav_path.stat().st_size,
        "size_after_bytes": output_path.stat().st_size,
        "reduction_pct": (1 - output_path.stat().st_size / wav_path.stat().st_size) * 100
    }
```

The flag also generates a `performance_report.json` in the output directory:

```json
{
  "total_size_before_mb": 112.4,
  "total_size_after_mb": 38.1,
  "reduction_pct": 66.1,
  "fits_in_mpc_ram": true,
  "streaming_required": false,
  "per_voice": {
    "kick_v1": {"operations": ["stereo_to_mono", "normalized_to_minus1dBFS"], "reduction_pct": 49.8},
    "kick_v2": {"...": "..."}
  }
}
```

---

### 2.2 Fast-Swap Setlist Architecture

**The Problem**

Loading a new XPN during a live set takes 2–5 seconds on MPC Live III depending on kit size, file system speed, and total sample RAM pressure. Two to five seconds is perceptible silence in a DJ/performance context. It breaks the audience's momentum.

**The Solution: Pre-Load All Sets at Start**

MPC Live III has 8GB total RAM. Sample RAM is a subset — typically 2–3GB practical ceiling before the OS and MPC firmware consume their share. At 50MB per optimized kit, that is 40–60 kits simultaneously in RAM.

A standard live set needs 6–12 kits. All of them fit. Load them all at set start (during soundcheck, before the first song), assign each to a dedicated Program slot, and switching is instantaneous.

**`xpn_setlist_builder.py` — New Tool**

```
python3 xpn_setlist_builder.py \
  --setlist setlist.json \
  --output ./tonight_set/ \
  --target-device mpc-live-3 \
  --performance-optimized
```

**`setlist.json` format:**

```json
{
  "set_name": "XO_OX Live — Club Venue, 2026-03-20",
  "bpm": 140,
  "songs": [
    {"position": 1, "title": "Opening",      "kit": "Onset_808_Reborn",   "bank": "A"},
    {"position": 2, "title": "Build 1",      "kit": "Onset_Grip_State",   "bank": "B"},
    {"position": 3, "title": "First Drop",   "kit": "Onset_Apex_State",   "bank": "C"},
    {"position": 4, "title": "Reset",        "kit": "Onset_Breath_Space", "bank": "D"},
    {"position": 5, "title": "Build 2",      "kit": "Onset_Rising_Heat",  "bank": "A"},
    {"position": 6, "title": "Peak",         "kit": "Onset_Maximum_Self", "bank": "B"}
  ]
}
```

**Tool output:**

1. A `setlist_programs/` directory with all XPN files, naming-stamped for the set.
2. A `ram_budget.json` report: total sample RAM footprint, per-kit breakdown, streaming flag if any kit exceeds the RAM ceiling.
3. A `loading_order.txt`: the optimal sequence for pre-loading at set start (largest kits first, to catch OOM errors early during soundcheck).
4. A `swap_guide.txt`: human-readable card with program slot assignments — "Song 3 (First Drop) → Program Slot C, press PROGRAM C on pad bank strip."

**Device RAM Limits (for validation):**

```python
DEVICE_RAM_MB = {
    "mpc-live-3":    2800,  # practical ceiling, approximate
    "mpc-x-3":       2800,
    "mpc-one-plus":  1400,
    "mpc-one":        700,
    "mpc-mini-mk3":   400,
}
```

If the setlist total exceeds the target device ceiling, the tool flags the offending kits and offers two remediation paths: (a) apply `--performance-optimized` to reduce size, (b) split the setlist into two loading windows (load first half before set, swap to second half during a planned break).

---

## Section 3: Audience-Reactive Kit Design

### 3.1 The Room-Aware Kit — Deep Implementation

**Room Mode Physics (Specific)**

Any rectangular room has a set of resonant modes (Schroeder modes) that depend on its dimensions L (length), W (width), H (height). The axial modes — the most energetically significant — occur at:

```
f_L = n × (c / 2L)   for n = 1, 2, 3, ...
f_W = n × (c / 2W)   for n = 1, 2, 3, ...
f_H = n × (c / 2H)   for n = 1, 2, 3, ...
```

For a typical club (L=20m, W=12m, H=4m) at 20°C (c=343 m/s):

```
f_L1 =  8.6Hz,  f_L2 =  17.2Hz,  f_L3 =  25.7Hz  (length modes — sub/subsonic)
f_W1 = 14.3Hz,  f_W2 =  28.6Hz,  f_W3 =  42.9Hz  (width modes — sub)
f_H1 = 42.9Hz,  f_H2 =  85.8Hz,  f_H3 = 128.6Hz  (height modes — kick body territory)
```

The height modes are the most relevant for drum design because they land in the audible bass range. A kick with a resonant body at 85–90Hz will energize the room's height mode in a standard 4-meter ceiling venue. This is why 808 kicks (whose fundamental is tunable but commonly set around 60–80Hz) sound different in different venues — not just louder, but physically bigger or smaller depending on whether their fundamental aligns with a room mode.

**Practical extraction from ambient recording:**

The ambient recording approach (described in Section 1.3) uses autocorrelation to find peaks in the room's spectral response. The specific algorithm:

```python
def extract_room_modes(ambient_wav: np.ndarray, sr: int, n_modes: int = 5) -> list[float]:
    """
    Use autocorrelation of the ambient recording's spectrum to find resonant peaks.
    Returns list of peak frequencies in Hz, sorted by magnitude.
    """
    # Compute magnitude spectrum
    spectrum = np.abs(np.fft.rfft(ambient_wav, n=sr))  # 1-second FFT → 1Hz resolution
    freqs = np.fft.rfftfreq(sr, d=1.0/sr)

    # Focus on 20Hz–500Hz (room mode territory)
    mask = (freqs >= 20) & (freqs <= 500)
    spectrum_band = spectrum[mask]
    freqs_band = freqs[mask]

    # Find local maxima (scipy.signal.find_peaks with prominence threshold)
    from scipy.signal import find_peaks
    peaks, properties = find_peaks(spectrum_band, prominence=0.3 * spectrum_band.max(), distance=5)

    # Sort by prominence, return top n_modes
    sorted_peaks = sorted(peaks, key=lambda i: properties['prominences'][list(peaks).index(i)], reverse=True)
    return [float(freqs_band[i]) for i in sorted_peaks[:n_modes]]
```

Once room modes are identified, the kick's tuning is adjusted to align its resonant body to the closest room mode. The `xpn_monster_rancher.py` tool will accept this list as `--room-modes-hz` when generating percussion samples.

**Impulse Response Method (Higher Accuracy)**

For a more accurate room mode extraction, play a 10-second sine sweep (20Hz → 20kHz) from the PA at soundcheck, record it on a phone 10 meters away, and deconvolve the sweep from the recording to extract the room impulse response. The IR's frequency response reveals room modes directly without the ambient noise ambiguity.

A `--sweep-recording` flag on `xpn_monster_rancher.py` handles this path:

```
python3 xpn_monster_rancher.py \
  room_sweep_recording.wav \
  --mode kit \
  --output ./room_tuned_kit/ \
  --sweep-recording True \
  --sweep-type log-sine-sweep \
  --sweep-duration-seconds 10
```

The tool deconvolves the known sweep signal (generated by `xpn_monster_rancher.py --generate-sweep --duration 10 --output sweep.wav`) from the recorded response, extracts the IR, and uses its frequency response to identify room modes directly.

---

### 3.2 Crowd-Size-Adaptive Kit Tiers

**Psychoacoustic Basis**

The spectral energy distribution of a drum sound that survives propagation in a large outdoor space is fundamentally different from one optimized for near-field headphone listening. The reasons:

1. **Air absorption**: High-frequency energy (above 4kHz) attenuates at approximately 1–3dB per 10 meters of propagation in standard atmospheric conditions. At 50 meters (festival far-field), a hat at 12kHz has lost 5–15dB compared to near-field. The hat must be boosted to compensate.

2. **Sub-bass propagation**: Below 80Hz, sound propagates omni-directionally with minimal loss. A sub kick designed for headphones (-6dB sub to prevent clipping/fatigue) will be inaudible in a festival field. Festival sub needs to be +6–9dB relative to the headphone tier.

3. **Crowd absorption**: A crowd of 5,000 people is a large diffuse absorber in the 500Hz–2kHz range (the resonant cavity of a human torso). Midrange content gets absorbed. The 3kHz presence peak must be boosted (+3dB) to cut through this absorption.

4. **Temporal smear**: Large spaces create early reflections in the 20–100ms range from walls, tent structures, and stage risers. Tight transient attacks can smear into pre-echo. Festival kicks benefit from a slightly longer attack (2–4ms vs. 0.5ms) to avoid pre-ringing perceptual artifacts.

**Three Spectral Tiers (Specific Parameters)**

All values are applied as static EQ adjustments during the WAV rendering/export stage. They do not require XPM-level parameter changes — they are baked into the sample files.

```
TIER: bedroom
Target: headphone listening, studio monitoring, 1–2 meter listening distance
  Sub (20–80Hz):    -6dB   — prevents low-end mud and ear fatigue
  Bass (80–200Hz):  +0dB   — flat
  LoMid (200–500Hz): +0dB  — flat
  Mid (500–1kHz):   +0dB   — flat
  Presence (1–4kHz): +2dB  — slight forward, compensates for headphone bass proximity
  Air (4–8kHz):     +0dB   — flat
  HiAir (8–16kHz):  +1dB   — gentle air for openness

TIER: club
Target: 200–800 person venue, PA at 50–100dB SPL, listening at 5–20 meters
  Sub (20–80Hz):    +3dB   — room reinforcement, 120Hz mode common in club rooms
  Bass (80–200Hz):  +2dB   — kick body punch
  LoMid (200–500Hz): -1dB  — mud reduction in reverberant spaces
  Mid (500–1kHz):   +0dB   — flat
  Presence (1–4kHz): +1dB  — mild forward for clarity
  Air (4–8kHz):     +0dB   — flat
  HiAir (8–16kHz):  -1dB   — reduce sibilance fatigue at high SPL

TIER: festival
Target: 2000+ person outdoor, 100–115dB SPL, listening at 20–100+ meters
  Sub (20–80Hz):    +6dB   — outdoor sub propagation compensation
  Bass (80–200Hz):  +3dB   — large PA bass reinforcement
  LoMid (200–500Hz): -3dB  — crowd absorption compensation
  Mid (500–1kHz):   -2dB   — crowd absorption
  Presence (1–4kHz): +3dB  — cut through crowd and absorption
  Air (4–8kHz):     +2dB   — compensate air absorption at 20 meters
  HiAir (8–16kHz):  +4dB   — aggressive air compensation for far-field
```

**Oxport Flag Implementation**

The `--venue-size` flag is added to `xpn_drum_export.py`. It accepts `bedroom`, `club`, or `festival`. When provided, after all other processing (including `--performance-optimized` if also specified), the EQ adjustments above are applied as a cascaded biquad shelf/peak filter bank using matched-Z transform coefficients (not Euler approximation — consistent with the CLAUDE.md audio DSP doctrine: use `exp(-2*π*fc/sr)` formulation).

The filter bank implementation:

```python
def apply_venue_eq(data: np.ndarray, sr: int, tier: str) -> np.ndarray:
    """
    Apply venue-tier EQ as a cascaded biquad filter bank.
    Uses matched-Z IIR design for accurate frequency response.
    Bands: [sub, bass, lomid, mid, presence, air, hiair]
    Centers: [40, 120, 350, 700, 2500, 6000, 12000] Hz
    """
    from scipy.signal import sosfilt, butter

    EQ_TABLE = {
        "bedroom":  [-6, 0, 0, 0,  2, 0,  1],
        "club":     [ 3, 2,-1, 0,  1, 0, -1],
        "festival": [ 6, 3,-3,-2,  3, 2,  4],
    }
    CENTERS_HZ   = [40, 120, 350, 700, 2500, 6000, 12000]
    Q            = 1.41  # 0.707 octave bandwidth per band

    gains = EQ_TABLE[tier]
    output = data.astype(np.float64)

    for center_hz, gain_db in zip(CENTERS_HZ, gains):
        if gain_db == 0:
            continue
        # Peaking EQ via bilinear transform (scipy does not expose matched-Z directly;
        # use bilinear for peaking since error at audio frequencies << 1% for Q=1.41)
        w0 = 2 * np.pi * center_hz / sr
        alpha = np.sin(w0) / (2 * Q)
        A = 10 ** (gain_db / 40.0)
        b = [1 + alpha*A, -2*np.cos(w0), 1 - alpha*A]
        a = [1 + alpha/A, -2*np.cos(w0), 1 - alpha/A]
        from scipy.signal import lfilter
        output = lfilter(b, a, output)

    return output.clip(np.iinfo(data.dtype).min, np.iinfo(data.dtype).max).astype(data.dtype)
```

**Three-Tier Pack Format**

A single XPN bundle generated with `--venue-size all` produces three sibling XPN files:

```
Onset_808_Reborn_bedroom.xpn
Onset_808_Reborn_club.xpn
Onset_808_Reborn_festival.xpn
```

Each XPN references its tier-specific WAV assets from a shared parent directory structure:

```
Onset_808_Reborn/
  wavs/
    bedroom/
      kick_v1.wav   kick_v2.wav ...
      snare_v1.wav  ...
    club/
      kick_v1.wav   ...
    festival/
      kick_v1.wav   ...
  Onset_808_Reborn_bedroom.xpn
  Onset_808_Reborn_club.xpn
  Onset_808_Reborn_festival.xpn
  tier_report.json
```

Total size for three tiers with `--performance-optimized`: approximately 120–150MB. Well within a USB drive or MPC internal storage capacity.

---

## Section 4: Novel XPN Kit Types Only Possible With XOceanus

### 4.1 The Coupled Live Kit

**Why This Kit Could Not Exist Before XOceanus**

Standard XPN kits are acoustically independent. Pad 1's sample has no relationship to Pad 2's sample beyond what a human producer decided during sound design. The samples are static artifacts. They do not communicate with each other during playback.

XOceanus's MegaCouplingMatrix enables cross-engine modulation in real-time during synthesis. When ONSET's kick voice is rendered offline with ORBITAL's envelope coupled to OPAL's grain density, the rendered WAV contains an acoustic event that could not have been produced by any single engine. More importantly, the causal relationships encoded in the coupling can be preserved across a multi-pad kit if the rendering strategy makes them explicit.

**The Coupled Pad System**

Render three categories of states:

```
Solo states (6 pads):
  Pad 1: kick_solo.wav        — kick rendered in isolation, no coupling active
  Pad 2: bass_solo.wav        — bass rendered in isolation
  Pad 3: lead_solo.wav        — lead rendered in isolation
  Pad 4: sub_solo.wav
  Pad 5: texture_solo.wav
  Pad 6: accent_solo.wav

Coupled states (8 pads):
  Pad 7:  kick+bass.wav       — rendered with kick→bass coupling active
  Pad 8:  bass+lead.wav       — rendered with bass→lead coupling active
  Pad 9:  kick+lead.wav       — rendered with kick→lead coupling active
  Pad 10: kick+bass+lead.wav  — all three coupled simultaneously
  Pad 11: sub+texture.wav     — sub oscillation modulates texture grain density
  Pad 12: accent+lead.wav     — accent envelope shapes lead brightness
  Pad 13: sub+kick.wav        — sub tunes kick pitch in real-time
  Pad 14: full_chorus.wav     — all six voices rendered with all coupling active

Negative space pads (2 pads):
  Pad 15: anti_kick.wav       — inverted phase relative to Pad 1 (for production layering)
  Pad 16: anti_lead.wav       — inverted phase relative to Pad 3
```

**Performance Practice**

The producer learns to think in coupling states rather than instrument types. A standard groove uses the solo pads (1–6) for the structural beat. When the build arrives, layering in Pad 7 (kick+bass) adds the coupling color — the bass now breathes in sympathy with the kick. At the peak, Pad 10 (kick+bass+lead) brings the full coupled texture — a sound that no single instrument produces and that could not be achieved by simply playing pads 1, 2, and 3 simultaneously, because the coupling relationship alters the individual timbres.

This is the first commercial drum kit format that encodes causal acoustic relationships as XPN pads. The sounds are different depending on what combination you play — not because the MPC is routing internally, but because the causal relationship was captured during offline render.

**Render Strategy — Specific Parameters**

Each coupled state requires a dedicated offline render from XOceanus with specific coupling configurations. The `xpn_coupling_recipes.py` tool (already in Tools/) handles coupling recipe selection. The new requirement is a render orchestration layer:

```python
# Pseudo-code for coupled state render orchestration
# (New module: xpn_coupled_kit_renderer.py)

COUPLING_STATES = {
    "kick+bass": {
        "source_engine": "ONSET",
        "source_voice": "kick",
        "target_engine": "ORBITAL",
        "target_voice": "bass",
        "coupling_type": "ENVELOPE_FOLLOWER",
        "coupling_amount": 0.65,
        "coupling_param": "orb_filterCutoff",
    },
    "bass+lead": {
        "source_engine": "ORBITAL",
        "source_voice": "bass",
        "target_engine": "OPAL",
        "coupling_type": "AMPLITUDE_TO_GRAIN_DENSITY",
        "coupling_amount": 0.50,
        "coupling_param": "opal_grainDensity",
    },
    # ... etc.
}
```

**Oxport Tool: `xpn_coupled_kit_renderer.py`**

```
python3 xpn_coupled_kit_renderer.py \
  --kit-spec coupled_live_kit.json \
  --engines ONSET,ORBITAL,OPAL \
  --output ./coupled_live_kit/ \
  --render-all-states True
```

The tool:
1. Generates solo renders for each voice at each velocity layer.
2. For each coupled state, generates an offline render with the specified coupling active.
3. Assembles the XPM with pad assignments as specified in the kit specification.
4. Writes a `coupling_guide.pdf` (or plain text equivalent) explaining which pads correspond to which coupling states — producer documentation for the live set.

---

### 4.2 The Negative Space Kit

**Concept**

The 16 pads are not 16 instruments. They are 16 spectral regions. Each pad, when triggered, adds energy precisely to the frequency band that its number corresponds to. The producer learns the band assignments. The kit becomes a mixing tool in real-time: if the mix sounds thin in the low-mids, hit Pad 4 (200–500Hz region). If it sounds dull, hit Pad 7 (presence, 2–4kHz). If it needs air, hit Pad 9 (8–12kHz).

**The 16-Band Assignment**

Use a perceptually spaced band structure (Bark scale approximation), not linear frequency:

```
Pad  1: Sub-bass     20–40Hz       (chest resonance, room energization)
Pad  2: Sub          40–80Hz       (kick sub fundamental territory)
Pad  3: Deep Bass    80–120Hz      (kick body, bass guitar fundamentals)
Pad  4: Bass         120–200Hz     (bass warmth, low-mid definition)
Pad  5: Low-Mid      200–350Hz     (honk/mud territory — gentle fill)
Pad  6: Mid-Low      350–500Hz     (body of most instruments)
Pad  7: Mid          500–800Hz     (nasal presence, vocal body)
Pad  8: Mid-High     800Hz–1.2kHz  (upper harmonic content)
Pad  9: Upper Mid    1.2–2kHz      (attack intelligibility)
Pad 10: Presence     2–3kHz        (cut-through frequency for all instruments)
Pad 11: Presence+    3–4kHz        (sibilance threshold, clarity edge)
Pad 12: Air-Low      4–6kHz        (string/vocal brightness)
Pad 13: Air          6–8kHz        (top-end air, sparkle)
Pad 14: High Air     8–12kHz       (cymbal sheen, fine detail)
Pad 15: Ultra Air    12–16kHz      (room air, reverb tail texture)
Pad 16: Brilliance   16–20kHz      (octave above presence — subconscious shimmer)
```

**What Each Pad Contains**

Each pad is a 500ms–2 second texture sample tuned to that band. The content type varies by band:

- Sub/bass pads (1–4): sine tones, sub oscillations, filtered noise with sharp LPF cutoff. Near-monophonic, pure energy delivery.
- Mid pads (5–9): filtered pink noise, vowel-shaped resonances, gentle harmonic stacks. Blends into any mix.
- Upper/presence pads (10–12): transient-rich content — noise bursts, filtered white noise, bright percussive tones with fast attack and 300ms release.
- Air pads (13–16): high-frequency shimmer, reverb tail textures, filtered noise with HPF. Almost imperceptible as individual sounds — perceived as mix "opening up."

**Why Noise Layers Work Better Than Tones for Mid Pads**

Pure tones create their own pitch presence and interact with harmonic content in the mix, potentially creating beating or dissonance. Filtered noise has energy across the band without a defined pitch — it fills the spectral hole without adding melody. The negative space kit uses noise-based textures for bands 3–12 and pure tones only for bands 1–2 (sub, where pitch identity is critical for room resonance coupling).

**XPN Implementation: Single-Layer Samples, No Velocity Layering**

The negative space kit does not benefit from velocity layering in the traditional sense. Instead, velocity controls the gain of the spectral fill — higher velocity = more fill. This is handled by the XPM's `VelocityToVolume` mapping (set to 1.0 for all pads) with a careful normalization: the samples are normalized so that maximum velocity produces exactly +0dB of spectral addition — not clipping, just filling the hole to flat. Lower velocities reduce the fill, allowing the producer to blend gradually.

```xml
<Instrument number="0" name="pad_01_sub_bass">
  <Volume>100</Volume>
  <VelocityToVolume>1.00</VelocityToVolume>
  <VelocityToFilter>0.00</VelocityToFilter>
  <OneShot>False</OneShot>
  <Polyphony>2</Polyphony>
  <!-- No MuteGroup — spectral fills coexist -->
  <Layer number="0" velocityStart="0" velocityEnd="127"
         sampleFile="pad_01_sub_bass.wav" rootNote="0" />
</Instrument>
```

**The Diagnostic Mode: Making Gaps Audible**

Standard use: producer triggers pads that feel absent from the mix.

Advanced use: the kit diagnoses. Run a side-chain input from the main mix output into the MPC's input channel. A companion iOS app (using the MPC's WiFi API) analyzes the incoming audio in real-time and illuminates the MPC pad LED for any pad whose spectral band is below the average energy by more than 6dB. The producer sees which pads are "needed" by the mix.

This is the iOS companion integration — V2 scope — but the XPN kit design supports it without modification. The pads are the interface. The intelligence about when to trigger them can come from the producer's ear or from a diagnostic tool.

**Oxport Tool: `xpn_negative_space_kit.py`**

```
python3 xpn_negative_space_kit.py \
  --band-count 16 \
  --texture-type noise \
  --duration-seconds 1.0 \
  --output ./negative_space_kit/ \
  --name "Spectral Fill Vol 1"
```

The tool generates 16 WAV files (one per band) using bandpass-filtered white/pink noise shaped with exponential envelopes, then assembles the XPM. Each band's WAV uses the matched-Z biquad bandpass from the existing audio DSP pattern in the codebase — `exp(-2*π*fc/sr)` formulation.

For the ultra-low bands (1–2, sub territory), the tool generates 1-second pitched tones at the target band's center frequency (30Hz and 60Hz respectively), shaped with a 50ms attack and 800ms exponential decay. These are not pure sines — they have 5% second harmonic and 2% third harmonic added to give the sub tones perceptible timbral character even through small PA monitors that cannot reproduce 30Hz directly.

---

## Appendix: Oxport Tool Summary

New tools specified in this document:

| Tool | Flag/Mode | Purpose |
|------|-----------|---------|
| `xpn_evolution_builder.py` | New tool | Temporal kit morphing — interpolated snapshot sets |
| `xpn_setlist_builder.py` | New tool | Pre-load all set kits, RAM budget report, swap guide |
| `xpn_coupled_kit_renderer.py` | New tool | Coupled pad systems from XOceanus coupling states |
| `xpn_negative_space_kit.py` | New tool | 16-band spectral fill kit generation |
| `xpn_drum_export.py` | `--performance-optimized` | Auto-trim, mono conversion, 16-bit dither, normalization |
| `xpn_drum_export.py` | `--venue-size [bedroom\|club\|festival]` | Three-tier spectral shaping for playback context |
| `xpn_monster_rancher.py` | `--room-aware True --room-length-meters N` | Room-mode-tuned kit generation |
| `xpn_monster_rancher.py` | `--sweep-recording True --sweep-duration-seconds N` | IR deconvolution for precise room mode extraction |
| `xpn_complement_renderer.py` | `--from-midi [file] --n-bars N` | MIDI-to-DNA inference for live contextual kit generation |
| `xpn_curiosity_engine.py` | `--live-context set_dna_profile.json` | Set-DNA-aware mutation for between-song generation |

Existing tools used as-is:
- `xpn_coupling_recipes.py` — coupling type selection for coupled kit renders
- `xpn_drum_export.py` — core XPM generation pipeline
- `xpn_monster_rancher.py` — spectral fingerprint analysis

---

## Appendix: XPM Structural Constraints Reference

Constraints that bound every implementation in this document:

```
Max instruments per XPM:          128 (limits bank-trick approach to 4 banks × 16 instruments, single layer)
Max velocity layers per pad:      4 (current XPM format; upgrade path to 8 under review)
Max simultaneous programs in RAM: hardware-dependent (see DEVICE_RAM_MB table)
Program Change response:          MIDI Program Change + Bank Select (CC#0); MPC firmware ≥ 2.10
Pad bank switching latency:       <5ms (in-RAM bank switch) vs. 2–5 seconds (disk load)
XPM File_Version:                 1.7 (Application_Version: 2.10.0.0, Platform: OSX)
KeyTrack rule:                    must be True for all layers (per XPN export doctrine)
RootNote convention:              0 for auto-detect (per XPN export doctrine)
Empty layer VelStart:             0 (per XPN export doctrine — prevents ghost triggering)
```
