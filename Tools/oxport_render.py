#!/usr/bin/env python3
"""
oxport_render.py — Fleet Render Automation (Phase 1)

Automates WAV rendering from XOlokun by sending MIDI to the plugin
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
import json
import math
import os
import struct
import sys
import time as _time
import time
from pathlib import Path


# ---------------------------------------------------------------------------
# Precise timing
# ---------------------------------------------------------------------------

def _precise_wait(duration_s: float) -> None:
    """Hybrid sleep + busy-wait for sub-ms timing accuracy."""
    if duration_s <= 0:
        return
    deadline = _time.perf_counter() + duration_s
    # Sleep for bulk of duration (save CPU), then busy-wait the tail
    sleep_duration = duration_s - 0.020  # wake 20ms early
    if sleep_duration > 0:
        _time.sleep(sleep_duration)
    # Busy-wait the final ~20ms for precision
    while _time.perf_counter() < deadline:
        pass

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
        for frame_idx in range(n_frames):
            for ch in range(n_channels):
                val = int(scaled[frame_idx, ch])
                # Pack as 3 little-endian bytes (signed)
                b0 = val & 0xFF
                b1 = (val >> 8) & 0xFF
                b2 = (val >> 16) & 0xFF
                f.write(bytes([b0, b1, b2]))

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

def build_render_jobs(spec: dict, presets: list[dict]) -> list[dict]:
    """Expand presets × notes × velocity layers into individual render jobs."""
    defaults = spec.get("defaults", {})
    default_notes = defaults.get("notes", [60])
    default_velocities = defaults.get("velocities", [127])
    default_duration_ms = defaults.get("duration_ms", 3000)
    default_release_ms = defaults.get("release_tail_ms", 500)

    jobs = []
    for preset in presets:
        slug = preset.get("slug", preset.get("name", "unknown")).replace(" ", "_")
        program = preset.get("program")
        bank_msb = preset.get("bank_msb")
        bank_lsb = preset.get("bank_lsb")
        channel = preset.get("channel", 0)

        notes = preset.get("notes", default_notes)
        velocities = preset.get("velocities", default_velocities)
        duration_ms = preset.get("duration_ms", default_duration_ms)
        release_ms = preset.get("release_tail_ms", default_release_ms)

        for note in notes:
            for vi, vel in enumerate(velocities, start=1):
                note_name = _midi_note_name(note)
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
    sd.wait()
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
        sd.wait()
        test_peak = float(np.max(np.abs(test_rec)))
        print(f"    Test note peak: {test_peak:.6f} ({20*math.log10(max(test_peak, 1e-10)):.1f} dBFS)")
        if test_peak < noise_floor * 2:
            print("  [PREFLIGHT FAIL] No signal from test note! Check: XOlokun running? BlackHole routing? MIDI port?")
            raise RuntimeError("Pre-flight failed: no audio signal detected from test note")
        print("  [PREFLIGHT PASS] Audio chain verified.\n")
    else:
        print("  [PREFLIGHT SKIP] No MIDI port — cannot send test note.\n")


# ---------------------------------------------------------------------------
# Render engine
# ---------------------------------------------------------------------------

def render_jobs(jobs: list[dict], output_dir: str, midi_port_name: str | None,
                audio_device, sample_rate: int, preset_load_ms: int = 200):
    """Execute all render jobs: MIDI out + audio record → WAV."""
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

    # A2: Check if the audio device's native sample rate matches the requested rate.
    # A mismatch means the OS will silently resample, which degrades audio quality.
    try:
        device_info = sd.query_devices(device_index)
        device_sr = int(device_info["default_samplerate"])
        if abs(device_sr - sample_rate) > 1:
            print(f"[WARN] Audio device sample rate ({device_sr}Hz) differs from "
                  f"requested ({sample_rate}Hz). OS may resample.")
    except Exception:
        pass  # device query failures must not abort the render session

    # Mandatory pre-flight: verify audio chain before committing to a full render session
    _preflight_check(device_index, sample_rate, port)

    total = len(jobs)
    t_start = time.time()
    last_program = None

    for idx, job in enumerate(jobs):
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
            last_program = pc_key

        # Calculate total recording duration in seconds
        rec_seconds = (job["duration_ms"] + job["release_ms"]) / 1000.0
        rec_frames = int(rec_seconds * sample_rate)

        # Start recording
        recording = sd.rec(rec_frames, samplerate=sample_rate, channels=2,
                           dtype="float32", device=device_index)

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

        # Wait for recording to finish
        sd.wait()

        # B2: Post-capture peak check — catch silent/clipping recordings immediately
        peak = float(np.max(np.abs(recording)))
        if peak < 0.001:  # ~-60 dBFS
            print(f"  [SILENT WARNING] {job['filename']} — no signal detected! Check BlackHole routing.")
        elif peak > 0.99:
            print(f"  [CLIP WARNING] {job['filename']} — signal clipping detected (peak={peak:.4f})")
        elif peak < 0.01:
            print(f"  [LOW LEVEL] {job['filename']} — very quiet signal (peak={peak:.4f})")

        # Save WAV (24-bit)
        out_path = os.path.join(output_dir, job["filename"])
        write_wav_24bit(out_path, recording, sample_rate)

        elapsed = time.time() - t_start
        print(f"Rendered {idx + 1}/{total}: {job['filename']}  ({elapsed:.0f}s elapsed)")

    port.close()
    total_time = time.time() - t_start
    mins = int(total_time // 60)
    secs = int(total_time % 60)
    print(f"\nDone. Rendered {total} WAVs in {mins}m {secs}s")
    print(f"Output: {os.path.abspath(output_dir)}")

# ---------------------------------------------------------------------------
# Dry run
# ---------------------------------------------------------------------------

def dry_run(jobs: list[dict], output_dir: str):
    total_ms = 0
    for idx, job in enumerate(jobs):
        dur = job["duration_ms"] + job["release_ms"]
        total_ms += dur
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
        description="Oxport Fleet Render — automate WAV rendering from XOlokun via MIDI + loopback audio",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 oxport_render.py --list-ports
  python3 oxport_render.py --spec render_spec.json --output-dir ./wavs/
  python3 oxport_render.py --spec render_spec.json --output-dir ./wavs/ --midi-port "XOlokun" --audio-device "BlackHole"
  python3 oxport_render.py --dry-run --spec render_spec.json

Render spec JSON format:
  {
    "defaults": {
      "notes": [60],
      "velocities": [20, 50, 90, 127],
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
        "velocities": [20, 50, 90, 127]
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
                args.sample_rate, args.preset_load_ms)


if __name__ == "__main__":
    main()
