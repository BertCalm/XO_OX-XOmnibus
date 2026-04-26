#!/usr/bin/env python3
"""
oxport_render.py — Fleet Render Automation (Phase 1)

Automates WAV rendering from XOceanus by sending MIDI to the plugin
and recording audio output via loopback (BlackHole on macOS).

Requirements:
    pip install mido python-rtmidi sounddevice numpy

Usage:
    python3 oxport_render.py --spec render_spec.json --output-dir ./wavs/
    python3 oxport_render.py --list-ports
    python3 oxport_render.py --dry-run --spec render_spec.json
"""

from __future__ import annotations

import argparse
import fcntl
import json
import math
import os
import shutil
import subprocess
import struct
import sys
import threading
import time as _time
import time
from datetime import datetime, timezone
from pathlib import Path

from engine_registry import get_all_engines


# ---------------------------------------------------------------------------
# Precise timing
# ---------------------------------------------------------------------------

def _precise_wait(duration_s: float) -> None:
    """Hybrid sleep + busy-wait for sub-ms timing accuracy."""
    if duration_s <= 0:
        return
    deadline = _time.perf_counter() + duration_s
    # Sleep for bulk of duration (save CPU), then busy-wait only the tail
    sleep_duration = duration_s - 0.002  # wake 2ms early (was 20ms — reduced CPU burn ~10x)
    if sleep_duration > 0:
        _time.sleep(sleep_duration)
    # Busy-wait only the final ~2ms for sub-millisecond precision
    while _time.perf_counter() < deadline:
        pass

# ---------------------------------------------------------------------------
# BlackHole render lock (Issue #733 — prevent concurrent access data races)
# ---------------------------------------------------------------------------

LOCKFILE = '/tmp/oxport_render.lock'

def _acquire_render_lock():
    """Acquire exclusive render lock. Fails fast if another instance is running."""
    try:
        lock_fd = open(LOCKFILE, 'w')
        fcntl.flock(lock_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        lock_fd.write(str(os.getpid()))
        lock_fd.flush()
        return lock_fd
    except (IOError, OSError):
        # Check if the locking process is still alive
        try:
            with open(LOCKFILE, 'r') as f:
                pid = int(f.read().strip())
            os.kill(pid, 0)  # Check if process exists
            print(f"[ABORT] Another oxport render is running (PID {pid}). Only one render can use BlackHole at a time.")
            sys.exit(1)
        except (ProcessLookupError, ValueError):
            # Stale lockfile — process is gone
            os.remove(LOCKFILE)
            return _acquire_render_lock()

def _release_render_lock(lock_fd):
    """Release render lock."""
    if lock_fd:
        fcntl.flock(lock_fd, fcntl.LOCK_UN)
        lock_fd.close()
        try:
            os.remove(LOCKFILE)
        except OSError as exc:
            print(f"[WARN] Removing render lockfile {LOCKFILE}: {exc}", file=sys.stderr)

# ---------------------------------------------------------------------------
# sd.wait() watchdog (Issue #742 — prevent infinite block on driver hang)
# ---------------------------------------------------------------------------

def _wait_with_timeout(expected_duration_s: float, timeout_margin: float = 2.0) -> bool:
    """Wait for sounddevice recording with a watchdog timer.

    Fires sd.stop() after (expected_duration_s + timeout_margin) seconds if
    sd.wait() has not returned on its own.  Returns True if recording
    completed normally, False if the watchdog fired (driver hang / timeout).
    """
    sd = _sounddevice
    timed_out = threading.Event()

    def _watchdog():
        timed_out.set()
        sd.stop()

    timeout = expected_duration_s + timeout_margin
    timer = threading.Timer(timeout, _watchdog)
    timer.start()
    try:
        sd.wait()
    finally:
        timer.cancel()

    return not timed_out.is_set()

# ---------------------------------------------------------------------------
# Optional dependency imports with graceful fallback
# ---------------------------------------------------------------------------

_mido = None
_sounddevice = None
_numpy = None

def _require_mido():
    global _mido
    if _mido is not None:
        return _mido
    try:
        import mido
        _mido = mido
        return mido
    except ImportError:
        print("ERROR: mido is required for MIDI output.", file=sys.stderr)
        print("  pip install mido python-rtmidi", file=sys.stderr)
        sys.exit(1)

def _require_sounddevice():
    global _sounddevice, _numpy
    if _sounddevice is not None:
        return _sounddevice
    try:
        import sounddevice as sd
        import numpy as np
        _sounddevice = sd
        _numpy = np
        return sd
    except ImportError:
        print("ERROR: sounddevice + numpy are required for audio recording.", file=sys.stderr)
        print("  pip install sounddevice numpy", file=sys.stderr)
        sys.exit(1)

# ---------------------------------------------------------------------------
# WAV writer (24-bit stereo, no extra dependencies)
# ---------------------------------------------------------------------------

def write_wav_24bit(path: str, data, sample_rate: int):
    """Write a numpy float32 array (frames x channels) as 24-bit WAV."""
    np = _numpy
    if data.ndim == 1:
        data = data.reshape(-1, 1)
    n_frames, n_channels = data.shape
    # Clip and scale to 24-bit signed range
    clipped = np.clip(data, -1.0, 1.0)
    scaled = (clipped * 8388607.0).astype(np.int32)
    byte_rate = sample_rate * n_channels * 3
    block_align = n_channels * 3
    data_size = n_frames * n_channels * 3

    with open(path, "wb") as f:
        # RIFF header
        f.write(b"RIFF")
        f.write(struct.pack("<I", 36 + data_size))
        f.write(b"WAVE")
        # fmt chunk
        f.write(b"fmt ")
        f.write(struct.pack("<I", 16))          # chunk size
        f.write(struct.pack("<H", 1))           # PCM
        f.write(struct.pack("<H", n_channels))
        f.write(struct.pack("<I", sample_rate))
        f.write(struct.pack("<I", byte_rate))
        f.write(struct.pack("<H", block_align))
        f.write(struct.pack("<H", 24))          # bits per sample
        # data chunk
        f.write(b"data")
        f.write(struct.pack("<I", data_size))
        # Vectorized 24-bit write: extract 3 bytes per sample via numpy bitwise
        # ops, then write the entire buffer in a single f.write() call.
        # Replaces per-sample Python loop (~3.4B individual write calls on a
        # full run) with a single bulk write — massive throughput improvement.
        try:
            import numpy as _np_local
            # Flatten to interleaved samples: [f0_ch0, f0_ch1, f1_ch0, ...]
            flat = scaled.flatten()
            n_samples = flat.size
            buf = _np_local.empty(n_samples * 3, dtype=_np_local.uint8)
            buf[0::3] = (flat & 0xFF).astype(_np_local.uint8)
            buf[1::3] = ((flat >> 8) & 0xFF).astype(_np_local.uint8)
            buf[2::3] = ((flat >> 16) & 0xFF).astype(_np_local.uint8)
            f.write(buf.tobytes())
        except ImportError:
            # Fallback: per-sample loop (slow — only reached if numpy missing)
            for frame_idx in range(n_frames):
                for ch in range(n_channels):
                    val = int(scaled[frame_idx, ch])
                    b0 = val & 0xFF
                    b1 = (val >> 8) & 0xFF
                    b2 = (val >> 16) & 0xFF
                    f.write(bytes([b0, b1, b2]))

# ---------------------------------------------------------------------------
# Render progress tracker (Feature #738 — per-category moving average ETA)
# ---------------------------------------------------------------------------

class RenderProgressTracker:
    """Track render progress and estimate remaining time using per-voice-category averages.

    Every 10 completed jobs OR every 60 seconds (whichever comes first), prints:
        [PROGRESS] 640/3200 (20.0%)  Rate: 18.3 jobs/min  ETA: 2h 22m  Disk: 1.2/14.2 GB used

    On completion, prints:
        [RENDER COMPLETE] 3200 jobs in 2h 18m. 40 skipped (probe dead). 0 failed.
    """

    PROGRESS_EVERY_N = 10       # print after every N completions
    PROGRESS_EVERY_S = 60       # print at least every N seconds
    MOVING_AVG_WINDOW = 10      # number of recent durations to average per category
    FALLBACK_DURATION_S = 3.5   # default per-job estimate when no data for category

    def __init__(self, total_jobs: int, output_dir: str = ""):
        self.total_jobs = total_jobs
        self.completed = 0          # jobs that were actually rendered
        self.skipped = 0            # jobs skipped (resume or voice_map)
        self.failed = 0             # jobs that produced no audio / timed out
        self.category_times: dict[str, list[float]] = {}  # voice_category -> [durations_s]
        self.start_time = time.time()
        self._last_print_time = self.start_time
        self._last_print_count = 0
        self._output_dir = output_dir

    # ── recording ──────────────────────────────────────────────────────────

    def record(self, voice_category: str, duration_s: float) -> None:
        """Record a completed render and maybe print a progress line."""
        self.category_times.setdefault(voice_category, []).append(duration_s)
        self.completed += 1
        self._maybe_print_progress()

    def record_skipped(self) -> None:
        """Record a skipped job (resume / dead voice)."""
        self.skipped += 1

    def record_failed(self) -> None:
        """Record a failed / silent job."""
        self.failed += 1
        self.completed += 1
        self._maybe_print_progress()

    # ── ETA estimation ─────────────────────────────────────────────────────

    def eta_seconds(self, remaining_jobs_by_category: "dict[str, int]") -> float:
        """Estimate remaining render time using per-category moving averages."""
        total = 0.0
        for cat, count in remaining_jobs_by_category.items():
            times = self.category_times.get(cat, [])
            window = times[-self.MOVING_AVG_WINDOW:] if times else []
            avg = sum(window) / len(window) if window else self.FALLBACK_DURATION_S
            total += avg * count
        return total

    # ── progress printing ──────────────────────────────────────────────────

    def _maybe_print_progress(self) -> None:
        now = time.time()
        since_count = self.completed - self._last_print_count
        since_time = now - self._last_print_time
        if since_count >= self.PROGRESS_EVERY_N or since_time >= self.PROGRESS_EVERY_S:
            self._print_progress(now)
            self._last_print_time = now
            self._last_print_count = self.completed

    def _print_progress(self, now: float | None = None) -> None:
        now = now or time.time()
        elapsed = now - self.start_time
        processed = self.completed + self.skipped  # total touched
        pct = 100.0 * processed / self.total_jobs if self.total_jobs else 0.0

        # Rate: completed (actually rendered) jobs per minute
        rate = (self.completed / elapsed * 60) if elapsed > 0 else 0.0

        # ETA: count remaining jobs by category
        remaining_total = max(self.total_jobs - processed, 0)
        # Build remaining_by_category from global category proportions
        total_cat_jobs = sum(len(v) for v in self.category_times.values())
        remaining_by_cat: dict[str, int] = {}
        if total_cat_jobs > 0:
            for cat, times in self.category_times.items():
                share = len(times) / total_cat_jobs
                remaining_by_cat[cat] = max(1, int(round(remaining_total * share)))
        else:
            remaining_by_cat["unknown"] = remaining_total

        eta_s = self.eta_seconds(remaining_by_cat)
        eta_str = _format_duration(eta_s)

        # Disk used so far
        disk_used_str = "?"
        total_disk_str = "?"
        if self._output_dir:
            try:
                usage = shutil.disk_usage(self._output_dir)
                disk_used_gb = (usage.total - usage.free) / 1_073_741_824
                disk_total_gb = usage.total / 1_073_741_824
                disk_used_str = f"{disk_used_gb:.1f}"
                total_disk_str = f"{disk_total_gb:.1f}"
            except OSError as exc:
                print(f"[WARN] Reading disk usage for output directory: {exc}", file=sys.stderr)

        print(
            f"  [PROGRESS] {processed}/{self.total_jobs} ({pct:.1f}%)  "
            f"Rate: {rate:.1f} jobs/min  ETA: {eta_str}  "
            f"Disk: {disk_used_str}/{total_disk_str} GB used"
        )

    def print_completion(self) -> None:
        """Print final render complete summary."""
        elapsed = time.time() - self.start_time
        elapsed_str = _format_duration(elapsed)
        skip_parts = []
        if self.skipped:
            skip_parts.append(f"{self.skipped} skipped (resume/dead)")
        failed_str = f"{self.failed} failed"
        skip_summary = (", ".join(skip_parts) + ". ") if skip_parts else ""
        print(
            f"\n  [RENDER COMPLETE] {self.completed} jobs in {elapsed_str}. "
            f"{skip_summary}{failed_str}."
        )
        # macOS completion notification (best-effort — never raises)
        try:
            subprocess.run(
                ["osascript", "-e", 'display notification "Oxport render complete" with title "XO_OX"'],
                check=False,
            )
        except Exception as exc:
            print(f"[WARN] Sending macOS completion notification: {exc}", file=sys.stderr)


def _format_duration(seconds: float) -> str:
    """Format a duration in seconds as 'Xh Ym' or 'Ym Zs'."""
    secs = int(seconds)
    if secs >= 3600:
        h = secs // 3600
        m = (secs % 3600) // 60
        return f"{h}h {m}m"
    m = secs // 60
    s = secs % 60
    return f"{m}m {s}s"


def _voice_category_from_job(job: dict) -> str:
    """Extract a voice category string from a render job dict.

    Tries xpn_voice_taxonomy first; falls back to the slug's last component.
    """
    voice_name = job.get("voice_name") or job.get("_voice_name", "")
    if not voice_name:
        slug = job.get("slug", "")
        parts = slug.split("__")
        voice_name = parts[-1] if parts else slug

    if voice_name:
        try:
            from xpn_voice_taxonomy import VOICE_CATEGORIES
            for cat, voices in VOICE_CATEGORIES.items():
                if voice_name.lower() in [v.lower() for v in voices]:
                    return cat
        except (ImportError, AttributeError):
            pass
        # Fallback: use first word of the voice name as a rough category
        return voice_name.split("_")[0] if voice_name else "unknown"

    return "unknown"


# ---------------------------------------------------------------------------
# WAV integrity check
# ---------------------------------------------------------------------------

def _is_valid_wav(path: str) -> bool:
    """Check WAV file has valid RIFF header and matching data size."""
    try:
        with open(path, 'rb') as f:
            # Check RIFF/WAVE markers
            header = f.read(12)
            if len(header) < 12 or header[:4] != b'RIFF' or header[8:12] != b'WAVE':
                return False
            # Read declared file size from RIFF header
            declared_size = int.from_bytes(header[4:8], 'little') + 8
            # Check actual file size matches
            f.seek(0, 2)
            actual_size = f.tell()
            if actual_size < declared_size:
                return False
        return True
    except (IOError, OSError):
        return False

# ---------------------------------------------------------------------------
# Port listing
# ---------------------------------------------------------------------------

def list_ports():
    """Print available MIDI output ports and audio devices."""
    print("=== MIDI Output Ports ===")
    try:
        import mido
        ports = mido.get_output_names()
        if ports:
            for p in ports:
                print(f"  {p}")
        else:
            print("  (none found)")
    except ImportError:
        print("  (mido not installed — pip install mido python-rtmidi)")

    print()
    print("=== Audio Input Devices ===")
    try:
        import sounddevice as sd
        devices = sd.query_devices()
        for i, d in enumerate(devices):
            if d["max_input_channels"] > 0:
                marker = " <-- BlackHole?" if "blackhole" in d["name"].lower() else ""
                print(f"  [{i}] {d['name']}  ({d['max_input_channels']}ch, {int(d['default_samplerate'])}Hz){marker}")
    except ImportError:
        print("  (sounddevice not installed — pip install sounddevice)")

# ---------------------------------------------------------------------------
# Spec loader
# ---------------------------------------------------------------------------

def load_spec(path: str) -> dict:
    with open(path, "r") as f:
        return json.load(f)

def resolve_presets(spec: dict, engine_filter: str | None = None) -> list[dict]:
    """Return flat list of preset entries from spec, optionally filtered."""
    presets = spec.get("presets", [])
    if engine_filter:
        ef = engine_filter.upper()
        presets = [p for p in presets if p.get("engine", "").upper() == ef]
    return presets

def build_render_jobs(spec: dict, presets: list[dict],
                      tier: "str | None" = None,
                      vel_values: "list[int] | None" = None,
                      round_robin: bool = False) -> list[dict]:
    """Expand presets x notes x velocity layers into individual render jobs.

    Args:
        spec:        Render spec JSON (contains defaults for notes, velocities, etc.).
        presets:     Flat list of preset/voice entries (from resolve_presets()).
        tier:        Tier name ("SURFACE" | "DEEP" | "TRENCH").  Informational;
                     used to label jobs when vel_values is also provided.
        vel_values:  Explicit list of MIDI velocity values for each layer.  When
                     provided this overrides the spec's default velocities.
                     When None, spec defaults are used (legacy behaviour).
        round_robin: When True, each voice is expanded for its per-voice RR count
                     from xpn_voice_taxonomy.VOICE_RR_COUNTS.  RR variants are
                     written with the suffix ``__rr{i}`` in the filename, e.g.
                     ``preset__snare__v3__rr1.WAV``.  Voices with rr_count==0
                     produce only the single base sample (no __rr suffix).
    """
    defaults = spec.get("defaults", {})
    default_notes = defaults.get("notes", [60])
    # vel_values kwarg takes priority over the spec's default velocity list
    default_velocities = vel_values if vel_values is not None else defaults.get("velocities", [127])
    default_duration_ms = defaults.get("duration_ms", 3000)
    default_release_ms = defaults.get("release_tail_ms", 500)

    # Load per-voice RR counts if round_robin is active.  Import is lazy so the
    # module is not required when round_robin=False (non-Onset engine paths).
    _rr_map: dict = {}
    if round_robin:
        try:
            from xpn_voice_taxonomy import VOICE_RR_COUNTS
            _rr_map = VOICE_RR_COUNTS
        except ImportError:
            pass  # RR counts unavailable — treat all voices as 0 (no RR expansion)

    jobs = []
    for preset in presets:
        slug = preset.get("slug", preset.get("name", "unknown")).replace(" ", "_")
        program = preset.get("program")
        bank_msb = preset.get("bank_msb")
        bank_lsb = preset.get("bank_lsb")
        channel = preset.get("channel", 0)

        notes = preset.get("notes", default_notes)
        # Per-preset velocity override (e.g. from manual render specs) still works;
        # the tier vel_values is the default, not a hard override at the preset level.
        velocities = preset.get("velocities", default_velocities)
        duration_ms = preset.get("duration_ms", default_duration_ms)
        release_ms = preset.get("release_tail_ms", default_release_ms)

        # Derive the voice name from the slug for RR lookup.
        # Convention: slug is either "voice" alone or "preset__voice" (two underscores).
        # Extract the last component after "__" as the voice name.
        _slug_parts = slug.split("__")
        _voice_name = _slug_parts[-1] if len(_slug_parts) > 1 else slug

        for note in notes:
            for vi, vel in enumerate(velocities, start=1):
                note_name = _midi_note_name(note)

                if round_robin and _rr_map:
                    n_rr = _rr_map.get(_voice_name, 0)
                else:
                    n_rr = 0

                if n_rr == 0:
                    # No round-robin for this voice — single base sample
                    filename = f"{slug}__{note_name}__v{vi}.WAV"
                    jobs.append({
                        "filename": filename,
                        "slug": slug,
                        "program": program,
                        "bank_msb": bank_msb,
                        "bank_lsb": bank_lsb,
                        "channel": channel,
                        "note": note,
                        "velocity": vel,
                        "duration_ms": duration_ms,
                        "release_ms": release_ms,
                    })
                else:
                    # Expand into n_rr round-robin variants: __rr1 .. __rr{n_rr}
                    for rr_i in range(1, n_rr + 1):
                        rr_filename = f"{slug}__{note_name}__v{vi}__rr{rr_i}.WAV"
                        jobs.append({
                            "filename": rr_filename,
                            "slug": slug,
                            "program": program,
                            "bank_msb": bank_msb,
                            "bank_lsb": bank_lsb,
                            "channel": channel,
                            "note": note,
                            "velocity": vel,
                            "duration_ms": duration_ms,
                            "release_ms": release_ms,
                            "rr_index": rr_i,
                            "rr_total": n_rr,
                        })
    return jobs

_NOTE_NAMES = ["C", "Cs", "D", "Ds", "E", "F", "Fs", "G", "Gs", "A", "As", "B"]

def _midi_note_name(note: int) -> str:
    octave = (note // 12) - 1
    name = _NOTE_NAMES[note % 12]
    return f"{name}{octave}"

# ---------------------------------------------------------------------------
# Pre-flight check
# ---------------------------------------------------------------------------

def _preflight_check(device_index, sample_rate: int, midi_port, midi_channel: int = 0) -> None:
    """Mandatory pre-flight: verify audio chain is alive."""
    sd = _require_sounddevice()
    np = _numpy
    print("\n  [PREFLIGHT] Testing audio chain...")
    # Record 0.5s of silence expectation
    silence = sd.rec(int(sample_rate * 0.5), samplerate=sample_rate, channels=2,
                     dtype="float32", device=device_index)
    if not _wait_with_timeout(0.5):
        raise RuntimeError("Pre-flight failed: audio driver timed out during silence measurement")
    noise_floor = float(np.max(np.abs(silence)))
    print(f"    Noise floor: {noise_floor:.6f} ({20*math.log10(max(noise_floor, 1e-10)):.1f} dBFS)")

    # Send a test note (very short, very quiet)
    if midi_port:
        mido = _require_mido()
        midi_port.send(mido.Message("note_on", channel=midi_channel, note=60, velocity=1))
        test_rec = sd.rec(int(sample_rate * 0.3), samplerate=sample_rate, channels=2,
                          dtype="float32", device=device_index)
        time.sleep(0.1)
        midi_port.send(mido.Message("note_off", channel=midi_channel, note=60, velocity=0))
        if not _wait_with_timeout(0.3):
            raise RuntimeError("Pre-flight failed: audio driver timed out during test note recording")
        test_peak = float(np.max(np.abs(test_rec)))
        print(f"    Test note peak: {test_peak:.6f} ({20*math.log10(max(test_peak, 1e-10)):.1f} dBFS)")
        if test_peak < noise_floor * 2:
            print("  [PREFLIGHT FAIL] No signal from test note! Check: XOceanus running? BlackHole routing? MIDI port?")
            raise RuntimeError("Pre-flight failed: no audio signal detected from test note")
        print("  [PREFLIGHT PASS] Audio chain verified.\n")
    else:
        print("  [PREFLIGHT SKIP] No MIDI port — cannot send test note.\n")


# ---------------------------------------------------------------------------
# Stage 0: Two-pass voice probe (QDD D2 — highest-priority fix)
# ---------------------------------------------------------------------------

def probe_voices(voices: list[dict], presets: list[dict], midi_port,
                 audio_device, sample_rate: int = 44100,
                 output_dir: str | None = None,
                 preset_load_ms: int = 400) -> dict:
    """Stage 0: Two-pass voice probe to determine which voices produce audio.

    Pass 1: Send each voice at velocity=20 (lowest production velocity).
    Pass 2: For any voice silent at vel=20, retry at velocity=80.

    Args:
        voices: list of dicts with "name" (engine voice name) and "note" (MIDI note)
        presets: list of preset dicts with at minimum "index" (program number) and "name"
        midi_port: open mido output port object (already opened by caller)
        audio_device: audio input device index (already resolved by caller)
        sample_rate: int
        output_dir: where to write voice_map.json; None = don't write
        preset_load_ms: ms to wait after program change

    Returns:
        voice_map dict mapping preset_name -> {voice_name: True/False}
        Empty dict on circuit-breaker abort.
        Also writes voice_map.json to output_dir when output_dir is given.
    """
    sd = _require_sounddevice()
    np = _numpy
    mido = _require_mido()

    SILENCE_PEAK = 0.002          # ~-54 dBFS — probe peak floor
    SILENCE_RMS  = 0.0005         # ~-66 dBFS — probe RMS floor (both must be below = silent)
    PROBE_DURATION_S = 0.5        # record 0.5s per note
    PROBE_VEL_PASS1 = 20          # first-pass velocity (lowest production velocity)
    PROBE_VEL_PASS2 = 80          # second-pass velocity (quiet voices that need more)
    CIRCUIT_BREAKER_LIMIT = 5     # consecutive all-dead presets before abort

    n_voices = len(voices)
    n_presets = len(presets)
    voice_map: dict[str, dict[str, bool]] = {}
    total_probes = 0
    alive_count = 0
    dead_count = 0
    pass2_recovered = 0
    consecutive_dead_presets = 0

    print(f"\n  [PROBE] Starting two-pass voice probe: {n_presets} presets × {n_voices} voices")
    print(f"  [PROBE] Pass 1 velocity: {PROBE_VEL_PASS1}  |  Pass 2 velocity: {PROBE_VEL_PASS2}  |  Threshold: peak<{SILENCE_PEAK} AND rms<{SILENCE_RMS}")

    for preset_idx, preset in enumerate(presets):
        preset_name = preset.get("name", f"preset_{preset_idx}")
        program = preset.get("index", preset.get("program", preset_idx))
        channel = preset.get("channel", 0)

        # Send program change
        if preset.get("bank_msb") is not None:
            midi_port.send(mido.Message("control_change", channel=channel,
                                        control=0, value=preset["bank_msb"]))
        if preset.get("bank_lsb") is not None:
            midi_port.send(mido.Message("control_change", channel=channel,
                                        control=32, value=preset["bank_lsb"]))
        midi_port.send(mido.Message("program_change", channel=channel, program=program))
        time.sleep(preset_load_ms / 1000.0)

        preset_result: dict[str, bool] = {}
        maybe_dead: list[dict] = []   # voices to retry in Pass 2

        # ── Pass 1 ──
        for voice in voices:
            voice_name = voice["name"]
            note = voice["note"]

            # Clear stuck notes before each probe
            midi_port.send(mido.Message("control_change", channel=channel,
                                        control=123, value=0))   # All Notes Off

            rec_frames = int(PROBE_DURATION_S * sample_rate)
            recording = sd.rec(rec_frames, samplerate=sample_rate, channels=2,
                               dtype="float32", device=audio_device)

            _precise_wait(0.010)  # pre-roll gap (mirrors render engine C5 guard)
            midi_port.send(mido.Message("note_on", channel=channel,
                                        note=note, velocity=PROBE_VEL_PASS1))
            _precise_wait(PROBE_DURATION_S * 0.5)   # hold for half the record window
            midi_port.send(mido.Message("note_off", channel=channel,
                                        note=note, velocity=0))

            completed = _wait_with_timeout(PROBE_DURATION_S)
            if not completed:
                # Watchdog fired — treat as silent for safety
                preset_result[voice_name] = False
                maybe_dead.append(voice)
                total_probes += 1
                continue

            probe_peak = float(np.max(np.abs(recording)))
            rms = float(np.sqrt(np.mean(recording ** 2)))
            total_probes += 1

            if not (probe_peak < SILENCE_PEAK and rms < SILENCE_RMS):
                preset_result[voice_name] = True
                alive_count += 1
            else:
                # MAYBE_DEAD — queue for Pass 2
                preset_result[voice_name] = False
                maybe_dead.append(voice)

            # 50ms inter-probe silence gap
            _precise_wait(0.050)

        # ── Pass 2 — retry silent voices at higher velocity ──
        for voice in maybe_dead:
            voice_name = voice["name"]
            note = voice["note"]

            midi_port.send(mido.Message("control_change", channel=channel,
                                        control=123, value=0))

            rec_frames = int(PROBE_DURATION_S * sample_rate)
            recording = sd.rec(rec_frames, samplerate=sample_rate, channels=2,
                               dtype="float32", device=audio_device)

            _precise_wait(0.010)
            midi_port.send(mido.Message("note_on", channel=channel,
                                        note=note, velocity=PROBE_VEL_PASS2))
            _precise_wait(PROBE_DURATION_S * 0.5)
            midi_port.send(mido.Message("note_off", channel=channel,
                                        note=note, velocity=0))

            completed = _wait_with_timeout(PROBE_DURATION_S)
            if not completed:
                dead_count += 1
                _precise_wait(0.050)
                continue

            probe_peak = float(np.max(np.abs(recording)))
            rms = float(np.sqrt(np.mean(recording ** 2)))
            total_probes += 1

            if not (probe_peak < SILENCE_PEAK and rms < SILENCE_RMS):
                preset_result[voice_name] = True
                alive_count += 1
                pass2_recovered += 1
            else:
                dead_count += 1

            _precise_wait(0.050)

        voice_map[preset_name] = preset_result

        # Report per-preset summary
        preset_alive = sum(1 for v in preset_result.values() if v)
        preset_dead = sum(1 for v in preset_result.values() if not v)
        dead_names = [v for v, alive in preset_result.items() if not alive]
        dead_str = f" (dead: {', '.join(dead_names)})" if dead_names else ""
        print(f"  [PROBE] Preset {preset_idx + 1}/{n_presets}: {preset_name} — "
              f"{preset_alive}/{n_voices} voices alive{dead_str}")

        # Circuit breaker: track consecutive all-dead presets
        if preset_alive == 0:
            consecutive_dead_presets += 1
            if consecutive_dead_presets >= CIRCUIT_BREAKER_LIMIT:
                print(f"\n  [ABORT] {CIRCUIT_BREAKER_LIMIT} consecutive presets produced no audio. "
                      f"Check BlackHole routing and engine state.")
                return {}
        else:
            consecutive_dead_presets = 0

    # Write voice_map.json
    if output_dir is not None:
        os.makedirs(output_dir, exist_ok=True)
        voice_map_path = os.path.join(output_dir, "voice_map.json")
        engine_name = presets[0].get("engine", "Unknown") if presets else "Unknown"
        probe_doc = {
            "engine": engine_name,
            "probe_date": datetime.now(timezone.utc).isoformat(),
            "probe_velocities": [PROBE_VEL_PASS1, PROBE_VEL_PASS2],
            "presets": voice_map,
            "summary": {
                "total_probes": total_probes,
                "alive": alive_count,
                "dead": dead_count,
                "pass2_recovered": pass2_recovered,
            },
        }
        with open(voice_map_path, "w", encoding="utf-8") as f:
            json.dump(probe_doc, f, indent=2)
        print(f"  [PROBE] voice_map.json written → {voice_map_path}")

    print(f"\n  [PROBE COMPLETE] {alive_count}/{total_probes} voices alive across "
          f"{n_presets} presets. {dead_count} silent voices will be skipped.")
    if pass2_recovered:
        print(f"  [PROBE] {pass2_recovered} voice(s) recovered in Pass 2 "
              f"(needed velocity {PROBE_VEL_PASS2} to produce audio).")

    return voice_map


def load_voice_map(output_dir: str) -> dict | None:
    """Load voice_map.json from output_dir. Returns None if not present or unreadable."""
    path = os.path.join(output_dir, "voice_map.json")
    if not os.path.isfile(path):
        return None
    try:
        with open(path, "r", encoding="utf-8") as f:
            doc = json.load(f)
        return doc.get("presets", {})
    except (OSError, json.JSONDecodeError) as e:
        print(f"  [WARN] Could not load voice_map.json: {e}")
        return None


# ---------------------------------------------------------------------------
# Render engine
# ---------------------------------------------------------------------------

def render_jobs(jobs: list[dict], output_dir: str, midi_port_name: str | None,
                audio_device, sample_rate: int, preset_load_ms: int = 200,
                force_rerender: bool = False, voice_map: dict | None = None,
                require_probe: bool = False, adaptive_load: bool = False):
    """Execute all render jobs: MIDI out + audio record → WAV.

    Args:
        force_rerender: When False (default), skip any job whose output WAV
            already exists with a non-zero file size (resume after crash).
            When True, re-render every job unconditionally.
        voice_map: Optional dict mapping preset_name -> {voice_name: bool}.
            Jobs whose voice is marked False are skipped.  Produced by
            probe_voices() / load_voice_map().  None = no filtering.
        require_probe: When True and voice_map is None, abort with a message
            instructing the user to run the probe first (--skip-probe to bypass).
        adaptive_load: When True, after each program change send a quick test note
            and retry with 2× wait (up to 3 retries, max 1600ms) if silent.
    """
    if require_probe and voice_map is None:
        print("[ABORT] --require-probe is set but no voice_map.json was found in the output "
              "directory. Run Stage 0 probe first (or pass --skip-probe to bypass).")
        return

    # Issue #733: Acquire exclusive render lock so that concurrent oxport
    # processes cannot open BlackHole simultaneously (which would produce
    # data races — both processes record the same mixed audio stream).
    _lock_fd = _acquire_render_lock()
    try:
        _render_jobs_locked(jobs, output_dir, midi_port_name, audio_device,
                            sample_rate, preset_load_ms, force_rerender, voice_map,
                            adaptive_load=adaptive_load)
    finally:
        _release_render_lock(_lock_fd)


def _render_jobs_locked(jobs: list[dict], output_dir: str, midi_port_name: str | None,
                        audio_device, sample_rate: int, preset_load_ms: int = 200,
                        force_rerender: bool = False, voice_map: dict | None = None,
                        adaptive_load: bool = False):
    """Internal implementation of render_jobs, called only while holding the lock."""
    mido = _require_mido()
    sd = _require_sounddevice()
    np = _numpy

    os.makedirs(output_dir, exist_ok=True)

    # Open MIDI port
    if midi_port_name:
        port = mido.open_output(midi_port_name)
    else:
        ports = mido.get_output_names()
        if not ports:
            print("ERROR: No MIDI output ports available.", file=sys.stderr)
            sys.exit(1)
        port = mido.open_output(ports[0])
        print(f"Using MIDI port: {ports[0]}")

    # Resolve audio device index
    device_index = None
    if audio_device is not None:
        if isinstance(audio_device, int) or audio_device.isdigit():
            device_index = int(audio_device)
        else:
            devices = sd.query_devices()
            for i, d in enumerate(devices):
                if audio_device.lower() in d["name"].lower() and d["max_input_channels"] > 0:
                    device_index = i
                    break
            if device_index is None:
                print(f"ERROR: Audio device '{audio_device}' not found.", file=sys.stderr)
                sys.exit(1)
    print(f"Using audio device index: {device_index or '(default)'}")

    # A2: Hard-abort if the audio device's native sample rate does not match the
    # requested render rate.  A mismatch causes the OS to silently resample, which
    # corrupts the recorded audio and invalidates the entire render batch.
    # Fix: change your audio device sample rate in Audio MIDI Setup before rendering.
    try:
        device_info = sd.query_devices(device_index)
        device_sr = int(device_info["default_samplerate"])
        if abs(device_sr - sample_rate) > 1:
            print(f"[ABORT] Audio device sample rate ({device_sr}Hz) does not match "
                  f"render target ({sample_rate}Hz). Change your audio device sample rate "
                  f"in Audio MIDI Setup before rendering.")
            port.close()
            return
    except Exception as _dev_err:
        print(f"[WARN] Audio device sample-rate check skipped: {_dev_err}", file=sys.stderr)

    # Mandatory pre-flight: verify audio chain before committing to a full render session
    _preflight_check(device_index, sample_rate, port)

    # Pre-render disk space check — compute expected bytes from job durations and abort
    # early if the output directory does not have enough free space (120% of estimate).
    _total_expected_bytes = sum(
        int(
            (job["duration_ms"] + job["release_ms"]) / 1000.0
            * sample_rate * 2 * 3  # stereo, 24-bit
        )
        for job in jobs
    )
    try:
        _free_bytes = shutil.disk_usage(output_dir).free
        _needed_bytes = _total_expected_bytes * 1.20
        if _free_bytes < _needed_bytes:
            _need_gb = _needed_bytes / 1_073_741_824
            _free_gb = _free_bytes / 1_073_741_824
            print(f"[ABORT] Insufficient disk space. "
                  f"Need ~{_need_gb:.1f}GB (with 20% margin), "
                  f"only {_free_gb:.1f}GB available.")
            port.close()
            return
    except OSError:
        print("[WARN] Could not check disk space before render.")

    total = len(jobs)
    t_start = time.time()
    last_program = None
    n_skipped = 0
    n_skipped_dead = 0
    n_rendered = 0
    n_consecutive_silent = 0  # Stage 5 circuit breaker: consecutive silent renders
    RENDER_SILENT_LIMIT = 5   # abort after this many consecutive silent WAVs

    # Feature #738: per-category moving-average ETA tracker
    tracker = RenderProgressTracker(total_jobs=total, output_dir=output_dir)

    for idx, job in enumerate(jobs):
        _job_t0 = time.time()
        out_path = os.path.join(output_dir, job["filename"])

        # Resume capability: skip jobs whose output already exists and passes WAV
        # integrity check (valid RIFF header + actual size >= declared size).
        # A file with non-zero size but truncated/corrupt data (e.g. from a
        # disk-full crash mid-write) is deleted and re-rendered.
        # Disabled when --force-rerender is set.
        if not force_rerender:
            try:
                if os.path.isfile(out_path):
                    if _is_valid_wav(out_path):
                        print(f"[SKIP] {job['filename']} already exists")
                        n_skipped += 1
                        tracker.record_skipped()
                        continue
                    else:
                        print(f"[CORRUPT] {job['filename']} — invalid WAV, re-rendering")
                        try:
                            os.remove(out_path)
                        except OSError as exc:
                            print(f"[WARN] Removing corrupt WAV {job['filename']}: {exc}", file=sys.stderr)
            except OSError as exc:
                print(f"[WARN] Stat check on {job['filename']} — falling through to render: {exc}", file=sys.stderr)

        # Stage 0 voice_map filter: skip jobs whose voice is marked DEAD by the probe.
        # voice_map is keyed by preset name; the job must carry "preset_name" and
        # "voice_name" (or "slug" derivation) for the filter to apply.
        if voice_map is not None:
            _preset_name = job.get("preset_name", "")
            _voice_name = job.get("voice_name", "")
            _preset_voices = voice_map.get(_preset_name, {})
            if _preset_voices and _voice_name and not _preset_voices.get(_voice_name, True):
                n_skipped_dead += 1
                tracker.record_skipped()
                continue

        # B4: Safety — clear any stuck notes from previous render
        port.send(mido.Message("control_change", channel=job["channel"], control=123, value=0))  # All Notes Off
        port.send(mido.Message("control_change", channel=job["channel"], control=64, value=0))   # Sustain Pedal Off

        # Program change if needed
        pc_key = (job["bank_msb"], job["bank_lsb"], job["program"], job["channel"])
        if job["program"] is not None and pc_key != last_program:
            ch = job["channel"]
            if job["bank_msb"] is not None:
                port.send(mido.Message("control_change", channel=ch, control=0, value=job["bank_msb"]))
            if job["bank_lsb"] is not None:
                port.send(mido.Message("control_change", channel=ch, control=32, value=job["bank_lsb"]))
            port.send(mido.Message("program_change", channel=ch, program=job["program"]))
            time.sleep(preset_load_ms / 1000.0)

            # Issue #740: Adaptive load retry — verify preset loaded before recording
            if adaptive_load:
                _adaptive_wait_ms = preset_load_ms
                _total_waited_ms = preset_load_ms  # already slept before this block
                _loaded_on_retry = False
                for _retry in range(3):
                    # Quick test note: velocity 80, 100ms recording
                    _test_frames = int(0.1 * sample_rate)
                    _test_rec = sd.rec(_test_frames, samplerate=sample_rate,
                                       channels=2, dtype="float32", device=device_index)
                    port.send(mido.Message("note_on", channel=ch,
                                           note=job["note"], velocity=80))
                    _precise_wait(0.08)
                    port.send(mido.Message("note_off", channel=ch,
                                           note=job["note"], velocity=0))
                    _wait_with_timeout(0.1)
                    port.send(mido.Message("control_change", channel=ch,
                                           control=123, value=0))  # All Notes Off
                    _test_peak = float(np.max(np.abs(_test_rec)))
                    if _test_peak >= 0.001:
                        if _retry > 0:
                            _loaded_on_retry = True
                        break  # preset loaded — proceed
                    # Silent — double the wait and retry
                    _adaptive_wait_ms = min(_adaptive_wait_ms * 2, 1600)
                    if _retry < 2:
                        time.sleep(_adaptive_wait_ms / 1000.0)
                        _total_waited_ms += _adaptive_wait_ms
                else:
                    # Exhausted all retries — still silent; voice probe will catch it
                    _preset_name = job.get("preset", job.get("filename", "unknown"))
                    print(f"  [WARN] Preset '{_preset_name}' needed {_total_waited_ms}ms to load "
                          f"(default: {preset_load_ms}ms) — continuing anyway")
                if _loaded_on_retry:
                    _preset_name = job.get("preset", job.get("filename", "unknown"))
                    print(f"  [WARN] Preset '{_preset_name}' needed {_total_waited_ms}ms to load "
                          f"(default: {preset_load_ms}ms)")
                # Brief gap so test-note tail doesn't bleed into the real recording
                _precise_wait(0.100)

            last_program = pc_key

        # Calculate total recording duration in seconds
        rec_seconds = (job["duration_ms"] + job["release_ms"] + 10) / 1000.0  # +10ms for C5 pre-roll
        rec_frames = int(rec_seconds * sample_rate)

        # Start recording
        recording = sd.rec(rec_frames, samplerate=sample_rate, channels=2,
                           dtype="float32", device=device_index)

        # C5: Pre-roll gap — ensure recording buffer is fully initialized
        # before MIDI note arrives at the synth.
        _precise_wait(0.010)

        # Note on
        port.send(mido.Message("note_on", channel=job["channel"],
                               note=job["note"], velocity=job["velocity"]))
        # B1: Precise hybrid sleep for note duration
        _precise_wait(job["duration_ms"] / 1000.0)

        # Note off
        port.send(mido.Message("note_off", channel=job["channel"],
                               note=job["note"], velocity=0))
        # B1: Precise hybrid sleep for release tail
        _precise_wait(job["release_ms"] / 1000.0)

        # Wait for recording to finish — guarded by watchdog to prevent infinite
        # block if BlackHole's driver hangs (Issue #742).
        completed = _wait_with_timeout(rec_seconds)
        if not completed:
            peak = float(np.max(np.abs(recording)))
            rms  = float(np.sqrt(np.mean(np.square(recording))))
            if peak < 0.002 and rms < 0.0005:
                print(f"  [TIMEOUT] {job['filename']} — driver timed out and no audio captured. Skipping.")
                tracker.record_failed()
                continue
            print(f"  [TIMEOUT] {job['filename']} — driver timed out but audio was captured (peak={peak:.4f}). Saving partial recording.")

        # B2: Post-capture dual-metric silence check — peak alone misses slow-attack pads
        peak = float(np.max(np.abs(recording)))
        rms  = float(np.sqrt(np.mean(np.square(recording))))
        if peak < 0.002 and rms < 0.0005:  # peak ~-54 dBFS AND rms ~-66 dBFS
            print(f"  [SILENT WARNING] {job['filename']} — no signal detected! Check BlackHole routing.")
            n_consecutive_silent += 1
            tracker.record_failed()
            if n_consecutive_silent >= RENDER_SILENT_LIMIT:
                print(f"\n[ABORT] {RENDER_SILENT_LIMIT} consecutive silent renders — "
                      f"routing broken or preset not loaded. "
                      f"Check BlackHole loopback and XOceanus state.")
                port.close()
                return
        else:
            n_consecutive_silent = 0  # reset on any live signal
            if peak > 0.99:
                print(f"  [CLIP WARNING] {job['filename']} — signal clipping detected (peak={peak:.4f})")
            elif peak < 0.01:
                print(f"  [LOW LEVEL] {job['filename']} — very quiet signal (peak={peak:.4f})")

        # C6: Inter-sample silence gap — prevent reverb/release tail of
        # previous sample from bleeding into the next recording.
        _precise_wait(0.100)  # 100ms gap

        # Save WAV (24-bit) — guard against disk-full and verify post-write
        try:
            write_wav_24bit(out_path, recording, sample_rate)
        except OSError as exc:
            # Disk full or other I/O failure — clean up partial file and stop
            print(f"[DISK FULL] Failed to write {job['filename']} — insufficient disk space")
            try:
                if os.path.isfile(out_path):
                    os.remove(out_path)
            except OSError as exc:
                print(f"[WARN] Removing partial WAV after disk-full {job['filename']}: {exc}", file=sys.stderr)
            break

        # Post-write integrity check: confirm actual file size matches RIFF-declared size.
        # Catches cases where sounddevice delivered fewer frames than expected so the
        # RIFF header declares more data than was actually written.
        n_frames_written = recording.shape[0]
        n_channels_written = recording.shape[1] if recording.ndim > 1 else 1
        expected_data_size = n_frames_written * n_channels_written * 3
        expected_file_size = 44 + expected_data_size  # 44-byte WAV header
        try:
            actual_file_size = os.path.getsize(out_path)
        except OSError:
            actual_file_size = -1
        if actual_file_size != expected_file_size:
            print(f"[WARN] {job['filename']} — size mismatch "
                  f"(expected {expected_file_size}B, got {actual_file_size}B). "
                  f"File may be truncated.")

        n_rendered += 1

        # Feature #738: record duration in per-category tracker
        _job_elapsed = time.time() - _job_t0
        _cat = _voice_category_from_job(job)
        tracker.record(_cat, _job_elapsed)

        # Periodic disk space re-check every 100 rendered jobs.
        # Other processes may consume disk during a multi-hour render session.
        if n_rendered % 100 == 0:
            try:
                _remaining = total - (idx + 1)
                _avg_bytes = _total_expected_bytes / max(total, 1)
                _still_needed = int(_remaining * _avg_bytes * 1.20)
                _free_now = shutil.disk_usage(output_dir).free
                if _free_now < _still_needed:
                    _need_gb = _still_needed / 1_073_741_824
                    _free_gb = _free_now / 1_073_741_824
                    print(f"[ABORT] Disk space exhausted mid-render after {n_rendered} jobs. "
                          f"Need ~{_need_gb:.1f}GB for remaining {_remaining} jobs, "
                          f"only {_free_gb:.1f}GB free.")
                    port.close()
                    return
            except OSError:
                pass  # stat failure — do not abort

        elapsed = time.time() - t_start
        print(f"Rendered {idx + 1}/{total}: {job['filename']}  ({elapsed:.0f}s elapsed)")

    port.close()
    tracker.print_completion()
    print(f"Output: {os.path.abspath(output_dir)}")

# ---------------------------------------------------------------------------
# Dry run
# ---------------------------------------------------------------------------

def dry_run(jobs: list[dict], output_dir: str):
    total_ms = 0
    for idx, job in enumerate(jobs):
        dur = job["duration_ms"] + job["release_ms"]
        total_ms += dur + 110  # +10ms C5 pre-roll + 100ms C6 inter-sample gap
        print(f"  [{idx + 1:4d}] {job['filename']}  note={job['note']} vel={job['velocity']} dur={dur}ms")

    est_secs = total_ms / 1000.0
    mins = int(est_secs // 60)
    secs = int(est_secs % 60)
    print(f"\n{len(jobs)} jobs, estimated recording time: {mins}m {secs}s")
    print(f"Output dir: {os.path.abspath(output_dir)}")

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Oxport Fleet Render — automate WAV rendering from XOceanus via MIDI + loopback audio. "
                    "See also: oxport.py for the full build pipeline (recommended entry point).",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 oxport_render.py --list-ports
  python3 oxport_render.py --spec render_spec.json --output-dir ./wavs/
  python3 oxport_render.py --spec render_spec.json --output-dir ./wavs/ --midi-port "XOceanus" --audio-device "BlackHole"
  python3 oxport_render.py --dry-run --spec render_spec.json

Render spec JSON format:
  {
    "defaults": {
      "notes": [60],
      "velocities": [10, 38, 73, 109],
      "duration_ms": 3000,
      "release_tail_ms": 500
    },
    "presets": [
      {
        "slug": "Stellar_Drift",
        "engine": "OPAL",
        "program": 0,
        "channel": 0,
        "notes": [48, 60, 72],
        "velocities": [10, 38, 73, 109]
      }
    ]
  }
""")
    parser.add_argument("--list-ports", action="store_true", help="List available MIDI and audio devices")
    parser.add_argument("--spec", type=str, help="Path to render spec JSON")
    parser.add_argument("--output-dir", type=str, default="./wavs", help="Output directory for WAV files")
    parser.add_argument("--midi-port", type=str, default=None, help="MIDI output port name")
    parser.add_argument("--audio-device", type=str, default="BlackHole", help="Audio input device name or index")
    parser.add_argument("--sample-rate", type=int, default=44100, help="Recording sample rate (default: 44100)")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be rendered without recording")
    parser.add_argument("--engine", type=str, default=None, help="Filter to presets for a single engine")
    parser.add_argument("--preset-load-ms", type=int, default=200, help="Wait time after program change (default: 200)")
    parser.add_argument("--force-rerender", action="store_true",
                        help="Re-render all jobs even if output WAV already exists (disables resume skip)")
    parser.add_argument("--adaptive-load", action="store_true", dest="adaptive_load",
                        help="After each program change, send a quick test note to confirm the preset "
                             "loaded before recording. Retries with 2× wait up to 3 times (max 1600ms). "
                             "Adds ~100ms overhead per preset on the happy path. Default: off.")

    args = parser.parse_args()

    if args.list_ports:
        list_ports()
        return

    if not args.spec:
        parser.print_help()
        sys.exit(1)

    if not os.path.isfile(args.spec):
        print(f"ERROR: Spec file not found: {args.spec}", file=sys.stderr)
        sys.exit(1)

    # Validate --engine against the shared registry (case-insensitive lookup)
    if args.engine:
        all_names = get_all_engines()
        match = next(
            (n for n in all_names if n.lower() == args.engine.lower()), None
        )
        if match is None:
            print(
                f"ERROR: Unknown engine: {args.engine!r}. "
                f"Valid engines: {', '.join(all_names)}",
                file=sys.stderr,
            )
            sys.exit(1)
        args.engine = match  # normalise to canonical case

    spec = load_spec(args.spec)
    presets = resolve_presets(spec, args.engine)

    if not presets:
        engine_msg = f" for engine '{args.engine}'" if args.engine else ""
        print(f"No presets found{engine_msg} in spec.", file=sys.stderr)
        sys.exit(1)

    jobs = build_render_jobs(spec, presets)
    print(f"Spec loaded: {len(presets)} presets -> {len(jobs)} render jobs")

    if args.dry_run:
        dry_run(jobs, args.output_dir)
        return

    render_jobs(jobs, args.output_dir, args.midi_port, args.audio_device,
                args.sample_rate, args.preset_load_ms, args.force_rerender,
                adaptive_load=args.adaptive_load)


if __name__ == "__main__":
    main()
