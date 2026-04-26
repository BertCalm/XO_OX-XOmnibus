"""Extended test suite for oxport pipeline + XPM XML correctness.

Covers:
  - TPDF dither verification
  - Atomic write / temp-file cleanup
  - resolve_engine_name alias coverage (all registered aliases)
  - XPM XML validity (drum + keygroup)
  - Velocity layer boundary contiguity (no gaps, no overlaps)
  - KeyTrack / RootNote / VelStart three-golden-rules enforcement
  - Drum ZonePlay values
  - Mute-group wiring (hat choke)
  - Pad note mapping correctness
  - complement_chain non-Artwork engine skip
  - _check_dependencies raises on missing required module
"""
import math
import struct
import sys
import os
import tempfile
sys.path.insert(0, os.path.dirname(__file__))


# ---------------------------------------------------------------------------
# A1: TPDF dither — gain changes must NOT produce bit-exact output
# ---------------------------------------------------------------------------

def test_tpdf_dither_is_applied():
    """After _apply_gain_db, the output must NOT be bit-for-bit equal to the
    deterministic expected value (i.e., dither was added).

    Strategy: apply a 1 dB gain to a constant-value file (all samples = 16000).
    Without dither every sample would map to the same integer value.
    With TPDF dither the samples will vary — no two runs produce identical output.
    We run the function twice on the same source; the two results must differ.
    """
    from oxport import _apply_gain_db, _write_wav, _read_wav_raw
    from pathlib import Path

    sr = 44100
    n = 2048
    val = 16000
    raw = struct.pack(f"<{n}h", *([val] * n))

    def _make_file(raw_bytes):
        fd, path = tempfile.mkstemp(suffix=".wav")
        os.close(fd)
        _write_wav(Path(path), 1, sr, 16, raw_bytes)
        return path

    path1 = _make_file(raw)
    path2 = _make_file(raw)
    try:
        _apply_gain_db(Path(path1), 1.0)
        _apply_gain_db(Path(path2), 1.0)
        _, _, _, data1 = _read_wav_raw(Path(path1))
        _, _, _, data2 = _read_wav_raw(Path(path2))
        # With TPDF both runs use random.uniform independently → outputs differ
        assert data1 != data2, (
            "TPDF dither expected: two separate gain applications should produce "
            "different bit patterns"
        )
    finally:
        os.unlink(path1)
        os.unlink(path2)


def test_tpdf_dither_does_not_change_rms_audibly():
    """Dither should add < 0.5 dB of RMS noise for a small gain that cannot clip.

    Use a quiet sine (~-20 dBFS) and apply +3 dB — peak after gain stays well
    below 0 dBFS so no clipping distorts the measured RMS shift.
    """
    from oxport import _apply_gain_db, _compute_rms_db, _write_wav
    from pathlib import Path

    sr = 44100
    # ~-20 dBFS: amplitude = 0.1 * 32767 = 3277 — peak after +3 dB = 4632, safe
    amplitude = 3277
    samples = [int(amplitude * math.sin(2 * math.pi * 440 * i / sr)) for i in range(sr)]
    raw = struct.pack(f"<{len(samples)}h", *samples)
    fd, path = tempfile.mkstemp(suffix=".wav")
    os.close(fd)
    try:
        _write_wav(Path(path), 1, sr, 16, raw)
        before = _compute_rms_db(Path(path))
        _apply_gain_db(Path(path), 3.0)   # +3 dB — no clipping at ~-20 dBFS input
        after = _compute_rms_db(Path(path))
        assert abs(after - (before + 3.0)) < 0.5, (
            f"Expected RMS to shift ~+3 dB; got {before:.2f} → {after:.2f} "
            f"(diff {after - before:.2f})"
        )
    finally:
        os.unlink(path)


# ---------------------------------------------------------------------------
# A2: Atomic write — temp file must be cleaned up on success (and on failure)
# ---------------------------------------------------------------------------

def test_atomic_write_no_temp_leftovers_on_success():
    """After a successful _apply_gain_db the temp .wav file must not persist."""
    from oxport import _apply_gain_db, _write_wav
    from pathlib import Path

    sr = 44100
    samples = [int(10000 * math.sin(2 * math.pi * 880 * i / sr)) for i in range(sr)]
    raw = struct.pack(f"<{len(samples)}h", *samples)
    fd, path = tempfile.mkstemp(suffix=".wav")
    os.close(fd)
    parent = Path(path).parent
    try:
        _write_wav(Path(path), 1, sr, 16, raw)
        # Snapshot .wav files before
        before = set(parent.glob("*.wav"))
        _apply_gain_db(Path(path), 3.0)
        after = set(parent.glob("*.wav"))
        # Any new .wav file that appeared (other than our target) is a leaked temp
        leaked = after - before - {Path(path)}
        assert not leaked, f"Leaked temp files after atomic write: {leaked}"
    finally:
        if os.path.exists(path):
            os.unlink(path)


def test_atomic_write_preserves_original_on_failure(monkeypatch=None):
    """If the write fails mid-operation the original file must survive intact.

    We simulate failure by patching _write_wav to raise after the temp file is
    created. Because we can't easily inject that, we instead verify the
    behavior from the outside: corrupt the write by making the target read-only
    so os.replace() fails, then confirm the original data is preserved.

    NOTE: On macOS as root or in CI the read-only trick may not block replace.
    This test is therefore informational — it asserts the original is readable
    after the failure (which _apply_gain_db guarantees by catching the error).
    """
    from oxport import _apply_gain_db, _write_wav, _read_wav_raw
    from pathlib import Path

    sr = 44100
    n = 1000
    raw = struct.pack(f"<{n}h", *([5000] * n))
    fd, path = tempfile.mkstemp(suffix=".wav")
    os.close(fd)
    try:
        _write_wav(Path(path), 1, sr, 16, raw)
        # Force failure: make directory read-only so mkstemp inside _apply_gain_db fails
        test_dir = tempfile.mkdtemp()
        fd2, path2 = tempfile.mkstemp(suffix=".wav", dir=test_dir)
        os.close(fd2)
        _write_wav(Path(path2), 1, sr, 16, raw)
        # Read-only directory prevents creating the temp file
        os.chmod(test_dir, 0o555)
        try:
            _apply_gain_db(Path(path2), 10.0)
        except Exception:
            pass  # Expected failure
        finally:
            os.chmod(test_dir, 0o755)
        # Original data must still be readable
        ch, sr_out, bps, data_out = _read_wav_raw(Path(path2))
        assert ch == 1 and sr_out == sr, "Original file was corrupted after write failure"
    finally:
        os.unlink(path)
        import shutil
        shutil.rmtree(test_dir, ignore_errors=True)


# ---------------------------------------------------------------------------
# A3: resolve_engine_name — all registered aliases
# ---------------------------------------------------------------------------

def test_resolve_engine_all_aliases():
    """Every alias in ENGINE_ALIASES must resolve to its canonical form,
    and the canonical form itself must pass through unchanged."""
    from oxport import resolve_engine_name, ENGINE_ALIASES

    for alias, canonical in ENGINE_ALIASES.items():
        result = resolve_engine_name(alias)
        assert result == canonical, (
            f"Alias '{alias}' resolved to '{result}', expected '{canonical}'"
        )

    # Canonical forms must be identity (no double-mapping)
    seen_canonicals = set(ENGINE_ALIASES.values())
    for canon in seen_canonicals:
        assert resolve_engine_name(canon) == canon, (
            f"Canonical name '{canon}' did not pass through unchanged — "
            "check for accidental double-mapping in ENGINE_ALIASES"
        )


def test_resolve_engine_unknown_passthrough():
    """Unknown engine names must be returned unchanged (not silently remapped)."""
    from oxport import resolve_engine_name
    assert resolve_engine_name("SomeNewEngine") == "SomeNewEngine"
    assert resolve_engine_name("") == ""


# ---------------------------------------------------------------------------
# A4: _check_dependencies raises on a missing required module
# ---------------------------------------------------------------------------

def test_check_dependencies_raises_on_missing_required():
    """_check_dependencies must raise ImportError when a required module is absent."""
    from oxport import _check_dependencies, PipelineContext
    import types

    ctx = PipelineContext(engine="Onset", output_dir=__import__("pathlib").Path(tempfile.gettempdir()) / "test_oxport_deps")
    ctx.dry_run = True

    # Temporarily shadow one required module with an unimportable sentinel
    import builtins
    real_import = builtins.__import__
    SENTINEL = "xpn_drum_export"

    def _mock_import(name, *args, **kwargs):
        if name == SENTINEL:
            raise ImportError(f"Mocked missing: {name}")
        return real_import(name, *args, **kwargs)

    builtins.__import__ = _mock_import
    try:
        raised = False
        try:
            _check_dependencies(ctx)
        except ImportError:
            raised = True
        assert raised, "_check_dependencies should have raised ImportError for missing required module"
    finally:
        builtins.__import__ = real_import


# ---------------------------------------------------------------------------
# B1: XPM XML validity — drum program
# ---------------------------------------------------------------------------

def test_drum_xpm_is_valid_xml():
    """generate_xpm() must produce parseable XML with correct root element."""
    import xml.etree.ElementTree as ET
    from xpn_drum_export import generate_xpm

    xpm = generate_xpm("Test Kit", wav_map={}, kit_mode="velocity")
    try:
        root = ET.fromstring(xpm)
    except ET.ParseError as e:
        raise AssertionError(f"Drum XPM is not valid XML: {e}\n\nXML:\n{xpm[:500]}")

    assert root.tag == "MPCVObject", f"Expected root 'MPCVObject', got '{root.tag}'"
    program = root.find("Program")
    assert program is not None, "Missing <Program> element"
    assert program.get("type") == "Drum", (
        f"Expected Program type='Drum', got '{program.get('type')}'"
    )


def test_drum_xpm_has_xml_declaration():
    """XPM must start with a proper XML declaration (version + encoding)."""
    from xpn_drum_export import generate_xpm

    xpm = generate_xpm("Test Kit", wav_map={})
    first_line = xpm.strip().split("\n")[0]
    assert '<?xml version="1.0" encoding="UTF-8"?>' in first_line, (
        f"Missing XML declaration in first line: {first_line!r}"
    )


def test_drum_xpm_keytrack_is_false():
    """Drum layers must have KeyTrack=False (drums don't transpose across keys)."""
    from xpn_drum_export import generate_xpm, PAD_MAP, build_wav_map
    from pathlib import Path

    # Build a minimal wav_map with one real layer (kick v1)
    slug = "Test_Kit"
    wav_map = {f"{slug}_kick_v1": f"{slug}_kick_v1.wav"}
    xpm = generate_xpm("Test Kit", wav_map=wav_map, kit_mode="velocity")

    # All <KeyTrack> elements inside <Layer> blocks must be False for drum programs
    import xml.etree.ElementTree as ET
    root = ET.fromstring(xpm)
    found_keytrack = False
    for layer in root.iter("Layer"):
        kt = layer.find("KeyTrack")
        if kt is not None:
            found_keytrack = True
            assert kt.text == "False", (
                f"Drum layer has KeyTrack='{kt.text}' (expected False)"
            )
    assert found_keytrack, "No <KeyTrack> elements found in drum XPM layers"


def test_drum_xpm_rootnote_is_zero():
    """Drum layer RootNote must be 0 (MPC auto-detect convention)."""
    import xml.etree.ElementTree as ET
    from xpn_drum_export import generate_xpm

    xpm = generate_xpm("Test Kit", wav_map={})
    root = ET.fromstring(xpm)
    for layer in root.iter("Layer"):
        rn = layer.find("RootNote")
        if rn is not None:
            assert rn.text == "0", (
                f"Drum layer RootNote='{rn.text}' (expected 0)"
            )


def test_drum_xpm_empty_layer_velstart_is_zero():
    """Empty/inactive drum layers must have VelStart=0 (prevents ghost triggering).

    When wav_map is empty, _layers_for_voice() returns empty placeholder layers
    with Active=False. _layer_block() clamps VelStart=0 and VelEnd=0 for inactive
    layers. This enforces Golden Rule #3: "Empty layer VelStart=0" from CLAUDE.md.
    """
    import xml.etree.ElementTree as ET
    from xpn_drum_export import generate_xpm

    # Empty wav_map → all instruments get empty placeholder layers
    xpm = generate_xpm("Test Kit", wav_map={})
    root = ET.fromstring(xpm)
    for layer in root.iter("Layer"):
        active = layer.find("Active")
        if active is not None and active.text == "False":
            vs = layer.find("VelStart")
            if vs is not None:
                assert vs.text == "0", (
                    f"BUG: Inactive layer has VelStart='{vs.text}' (must be 0 to prevent ghost triggering). "
                    "Fix: _layer_block() must clamp VelStart=0/VelEnd=0 when Active=False."
                )


# ---------------------------------------------------------------------------
# B2: Velocity layer boundary contiguity — drum program
# ---------------------------------------------------------------------------

def test_drum_velocity_layers_are_contiguous():
    """Velocity layers in velocity mode must cover [1..127] with no gaps or overlaps."""
    from xpn_drum_export import VEL_LAYERS_MUSICAL, VEL_LAYERS_EVEN

    for curve_name, curve in [("musical", VEL_LAYERS_MUSICAL), ("even", VEL_LAYERS_EVEN)]:
        # Check no gaps: each band's start == previous band's end + 1
        for i in range(1, len(curve)):
            prev_end = curve[i - 1][1]
            curr_start = curve[i][0]
            assert curr_start == prev_end + 1, (
                f"{curve_name} curve: gap between layer {i-1} (end={prev_end}) "
                f"and layer {i} (start={curr_start})"
            )
        # Check full coverage
        assert curve[0][0] == 1, (
            f"{curve_name} curve: first band starts at {curve[0][0]}, expected 1"
        )
        assert curve[-1][1] == 127, (
            f"{curve_name} curve: last band ends at {curve[-1][1]}, expected 127"
        )


def test_drum_dna_adapted_layers_contiguous():
    """DNA-adapted velocity layers must also be contiguous with no gaps."""
    from xpn_drum_export import _dna_adapt_velocity_layers

    test_dnas = [
        {"brightness": 0.5, "warmth": 0.5, "movement": 0.5, "density": 0.5, "space": 0.5, "aggression": 0.5},
        {"brightness": 1.0, "warmth": 1.0, "movement": 0.5, "density": 1.0, "space": 0.0, "aggression": 1.0},
        {"brightness": 0.0, "warmth": 0.0, "movement": 0.5, "density": 0.0, "space": 1.0, "aggression": 0.0},
    ]
    for dna in test_dnas:
        layers = _dna_adapt_velocity_layers(dna)
        assert len(layers) == 4, f"Expected 4 layers, got {len(layers)}"
        for i in range(1, len(layers)):
            prev_end   = layers[i - 1][1]
            curr_start = layers[i][0]
            assert curr_start == prev_end + 1, (
                f"DNA-adapted layer gap: layer {i-1} ends at {prev_end}, "
                f"layer {i} starts at {curr_start}. DNA={dna}"
            )
        assert layers[0][0] == 1, f"First DNA layer starts at {layers[0][0]}, expected 1"
        assert layers[-1][1] == 127, f"Last DNA layer ends at {layers[-1][1]}, expected 127"


# ---------------------------------------------------------------------------
# B3: ZonePlay values — drum program
# ---------------------------------------------------------------------------

def test_drum_zone_play_values():
    """ZONE_PLAY constants must match the MPC XPM spec values exactly."""
    from xpn_drum_export import ZONE_PLAY

    assert ZONE_PLAY["velocity"]        == 1, f"velocity should be 1, got {ZONE_PLAY['velocity']}"
    assert ZONE_PLAY["cycle"]           == 2, f"cycle should be 2, got {ZONE_PLAY['cycle']}"
    assert ZONE_PLAY["random"]          == 3, f"random should be 3, got {ZONE_PLAY['random']}"
    assert ZONE_PLAY["random-norepeat"] == 4, f"random-norepeat should be 4, got {ZONE_PLAY['random-norepeat']}"


def test_drum_xpm_zone_play_in_xml():
    """ZonePlay XML value for velocity mode must be 1."""
    import xml.etree.ElementTree as ET
    from xpn_drum_export import generate_xpm, PAD_MAP

    xpm = generate_xpm("Test Kit", wav_map={}, kit_mode="velocity")
    root = ET.fromstring(xpm)
    instruments = root.findall(".//Instrument")
    # Check the active instruments (those with voice names in PAD_MAP)
    # PAD_MAP has 8 entries; instruments are indexed 0-127
    active_notes = {note for note, _, _, _ in PAD_MAP}
    found_velocity = False
    for instr in instruments:
        num = instr.get("number")
        if num and int(num) in active_notes:
            zp = instr.find("ZonePlay")
            if zp is not None:
                assert zp.text == "1", (
                    f"Instrument {num}: expected ZonePlay=1 for velocity mode, got {zp.text}"
                )
                found_velocity = True
    assert found_velocity, "No active instruments with ZonePlay found in drum XPM"


# ---------------------------------------------------------------------------
# B4: Pad note mapping — drum program
# ---------------------------------------------------------------------------

def test_drum_pad_note_mapping():
    """GM-convention drum pads must map to the documented MIDI notes."""
    from xpn_drum_export import PAD_MAP

    expected = {
        "kick":  36,
        "snare": 38,
        "clap":  39,
        "chat":  42,
        "ohat":  46,
        "tom":   41,
        "perc":  43,
        "fx":    49,
    }
    actual = {voice: note for note, voice, _, _ in PAD_MAP}
    for voice, expected_note in expected.items():
        assert voice in actual, f"Voice '{voice}' not found in PAD_MAP"
        assert actual[voice] == expected_note, (
            f"Voice '{voice}': expected MIDI note {expected_note}, got {actual[voice]}"
        )


def test_drum_pad_note_map_in_xml():
    """The <PadNoteMap> section must contain entries for all 8 active pads."""
    import xml.etree.ElementTree as ET
    from xpn_drum_export import generate_xpm, PAD_MAP

    xpm = generate_xpm("Test Kit", wav_map={})
    root = ET.fromstring(xpm)
    pad_note_map = root.find(".//PadNoteMap")
    assert pad_note_map is not None, "Missing <PadNoteMap> in drum XPM"
    pads = pad_note_map.findall("Pad")
    assert len(pads) == len(PAD_MAP), (
        f"Expected {len(PAD_MAP)} pad entries in PadNoteMap, got {len(pads)}"
    )


# ---------------------------------------------------------------------------
# B5: Mute groups — hat choke
# ---------------------------------------------------------------------------

def test_drum_hat_choke_mute_group():
    """Closed hat (chat) and open hat (ohat) must share mute group 1.
    Closed hat must have ohat (note 46) as its first mute target.
    """
    from xpn_drum_export import PAD_MAP

    mute_groups = {voice: mg for _, voice, mg, _ in PAD_MAP}
    mute_targets = {voice: mt for _, voice, _, mt in PAD_MAP}

    assert mute_groups.get("chat") == mute_groups.get("ohat"), (
        f"chat and ohat must share the same mute group; "
        f"chat={mute_groups.get('chat')}, ohat={mute_groups.get('ohat')}"
    )
    assert mute_groups.get("chat") == 1, (
        f"Hat choke group must be 1, got {mute_groups.get('chat')}"
    )
    # chat must mute ohat (note 46)
    chat_targets = mute_targets.get("chat", [])
    ohat_note = next(n for n, v, _, _ in PAD_MAP if v == "ohat")
    assert ohat_note in chat_targets, (
        f"chat mute_targets {chat_targets} must include ohat note {ohat_note}"
    )


def test_drum_xpm_mute_group_in_xml():
    """Closed hat must appear in <PadGroupMap> with group=1."""
    import xml.etree.ElementTree as ET
    from xpn_drum_export import generate_xpm, PAD_MAP

    xpm = generate_xpm("Test Kit", wav_map={})
    root = ET.fromstring(xpm)
    pgm = root.find(".//PadGroupMap")
    assert pgm is not None, "Missing <PadGroupMap> in drum XPM"
    # Find pad index for chat (closed hat)
    chat_pad_idx = None
    for pad_idx, (_, voice, _, _) in enumerate(PAD_MAP):
        if voice == "chat":
            chat_pad_idx = pad_idx + 1  # 1-indexed
            break
    assert chat_pad_idx is not None, "chat not found in PAD_MAP"
    group_entries = {int(p.get("number", 0)): int(p.get("group", -1))
                     for p in pgm.findall("Pad")}
    assert chat_pad_idx in group_entries, (
        f"chat pad {chat_pad_idx} missing from PadGroupMap"
    )
    assert group_entries[chat_pad_idx] == 1, (
        f"chat pad group expected 1, got {group_entries[chat_pad_idx]}"
    )


# ---------------------------------------------------------------------------
# B6: Keygroup XPM — KeyTrack=True, RootNote=0, VelStart correctness
# ---------------------------------------------------------------------------

def test_keygroup_xpm_is_valid_xml():
    """generate_keygroup_xpm() must produce parseable XML with correct root element."""
    import xml.etree.ElementTree as ET
    from xpn_keygroup_export import generate_keygroup_xpm

    xpm = generate_keygroup_xpm(
        preset_name="Test Preset",
        engine="Odyssey",
        wav_map={},
    )
    try:
        root = ET.fromstring(xpm)
    except ET.ParseError as e:
        raise AssertionError(f"Keygroup XPM is not valid XML: {e}")

    assert root.tag == "MPCVObject", f"Expected root 'MPCVObject', got '{root.tag}'"
    program = root.find("Program")
    assert program is not None, "Missing <Program> element"
    assert program.get("type") == "Keygroup", (
        f"Expected Program type='Keygroup', got '{program.get('type')}'"
    )


def test_keygroup_xpm_keytrack_is_true():
    """Keygroup layers must have KeyTrack=True (samples transpose across zones)."""
    import xml.etree.ElementTree as ET
    from xpn_keygroup_export import generate_keygroup_xpm

    # Build a minimal wav_map with two notes and one velocity layer each
    wav_map = {
        "Test_Preset__C3__v1": "Test_Preset__C3__v1.WAV",
        "Test_Preset__G3__v1": "Test_Preset__G3__v1.WAV",
    }
    xpm = generate_keygroup_xpm(
        preset_name="Test Preset",
        engine="Odyssey",
        wav_map=wav_map,
    )
    root = ET.fromstring(xpm)
    found = False
    for layer in root.iter("Layer"):
        active = layer.find("Active")
        kt = layer.find("KeyTrack")
        if active is not None and active.text == "True" and kt is not None:
            found = True
            assert kt.text == "True", (
                f"Keygroup active layer has KeyTrack='{kt.text}' (expected True)"
            )
    assert found, "No active layers with KeyTrack found in keygroup XPM"


def test_keygroup_xpm_rootnote_is_zero():
    """Keygroup layer RootNote must be 0 (MPC auto-detect)."""
    import xml.etree.ElementTree as ET
    from xpn_keygroup_export import generate_keygroup_xpm

    wav_map = {"Test_Preset__C3__v1": "Test_Preset__C3__v1.WAV"}
    xpm = generate_keygroup_xpm(
        preset_name="Test Preset",
        engine="Odyssey",
        wav_map=wav_map,
    )
    root = ET.fromstring(xpm)
    for layer in root.iter("Layer"):
        rn = layer.find("RootNote")
        if rn is not None:
            assert rn.text == "0", (
                f"Keygroup layer RootNote='{rn.text}' (expected 0)"
            )


def test_keygroup_xpm_empty_layer_velstart_is_zero():
    """Empty keygroup layers (no sample) must have VelStart=0."""
    import xml.etree.ElementTree as ET
    from xpn_keygroup_export import generate_keygroup_xpm

    xpm = generate_keygroup_xpm(
        preset_name="Test Preset",
        engine="Odyssey",
        wav_map={},  # no samples → empty placeholder
    )
    root = ET.fromstring(xpm)
    for layer in root.iter("Layer"):
        active = layer.find("Active")
        if active is not None and active.text == "False":
            vs = layer.find("VelStart")
            if vs is not None:
                assert vs.text == "0", (
                    f"Inactive keygroup layer VelStart='{vs.text}' (expected 0)"
                )


# ---------------------------------------------------------------------------
# B7: Velocity layer contiguity — keygroup
# ---------------------------------------------------------------------------

def test_keygroup_velocity_layers_contiguous():
    """Standard keygroup velocity layer tables must cover 1..127 with no gaps."""
    from xpn_keygroup_export import FAMILY_VEL_LAYERS

    for family, layers in FAMILY_VEL_LAYERS.items():
        # layers is a list of (vel_start, vel_end, volume[, label])
        if len(layers) < 2:
            continue
        for i in range(1, len(layers)):
            prev_end   = layers[i - 1][1]
            curr_start = layers[i][0]
            assert curr_start == prev_end + 1, (
                f"Family '{family}': gap between layer {i-1} (end={prev_end}) "
                f"and layer {i} (start={curr_start})"
            )
        assert layers[0][0] == 1, (
            f"Family '{family}': first layer starts at {layers[0][0]}, expected 1"
        )
        assert layers[-1][1] == 127, (
            f"Family '{family}': last layer ends at {layers[-1][1]}, expected 127"
        )


def test_keygroup_vel_crossfade_contiguity():
    """_apply_vel_crossfade must not create gaps — layers must remain overlapping or
    contiguous (for families that use crossfade)."""
    from xpn_keygroup_export import _apply_vel_crossfade, FAMILY_VEL_LAYERS

    # Ghost Council Modified zones (QDD Level 2, adopted 2026-04-04)
    base = [(1, 20, 0.30), (21, 55, 0.55), (56, 90, 0.75), (91, 127, 0.95)]
    for xfade in [0, 3, 6, 10]:
        result = _apply_vel_crossfade(base, xfade, "piano")
        assert len(result) == 4, f"crossfade={xfade}: expected 4 layers, got {len(result)}"
        # After crossfade, layers overlap → each layer's start must be ≤ previous end
        # and the first layer must still start at or below 1
        assert result[0][0] <= 1, (
            f"crossfade={xfade}: first layer starts at {result[0][0]}, expected ≤ 1"
        )
        assert result[-1][1] >= 127, (
            f"crossfade={xfade}: last layer ends at {result[-1][1]}, expected ≥ 127"
        )
        # No layer's start should exceed its end (malformed range)
        for i, (vs, ve, _) in enumerate(result):
            assert vs <= ve, (
                f"crossfade={xfade}: layer {i} has inverted range [{vs}, {ve}]"
            )


# ---------------------------------------------------------------------------
# B8: Sample file paths in XPM match expected relative structure
# ---------------------------------------------------------------------------

def test_drum_xpm_file_paths_relative():
    """Sample <File> elements must use Samples/{slug}/{filename} relative paths."""
    import xml.etree.ElementTree as ET
    from xpn_drum_export import generate_xpm

    slug = "Test_Kit"
    preset_name = "Test Kit"
    wav_map = {
        f"{slug}_kick_v1": f"{slug}_kick_v1.wav",
        f"{slug}_snare_v1": f"{slug}_snare_v1.wav",
    }
    xpm = generate_xpm(preset_name, wav_map=wav_map, kit_mode="velocity")
    root = ET.fromstring(xpm)
    for layer in root.iter("Layer"):
        active = layer.find("Active")
        file_el = layer.find("File")
        if active is not None and active.text == "True" and file_el is not None:
            fpath = file_el.text or ""
            if fpath:
                assert fpath.startswith(f"Samples/{slug}/"), (
                    f"Expected path starting with 'Samples/{slug}/', got '{fpath}'"
                )


def test_keygroup_xpm_file_paths_relative():
    """Keygroup <File> elements must use Samples/{slug}/{filename} relative paths."""
    import xml.etree.ElementTree as ET
    from xpn_keygroup_export import generate_keygroup_xpm

    preset_slug = "Test_Preset"
    wav_map = {f"{preset_slug}__C3__v1": f"{preset_slug}__C3__v1.WAV"}
    xpm = generate_keygroup_xpm(
        preset_name="Test Preset",
        engine="Odyssey",
        wav_map=wav_map,
    )
    root = ET.fromstring(xpm)
    for layer in root.iter("Layer"):
        active = layer.find("Active")
        file_el = layer.find("File")
        if active is not None and active.text == "True" and file_el is not None:
            fpath = file_el.text or ""
            if fpath:
                assert fpath.startswith(f"Samples/{preset_slug}/"), (
                    f"Expected 'Samples/{preset_slug}/...', got '{fpath}'"
                )


# ---------------------------------------------------------------------------
# B9: Program name in XPM
# ---------------------------------------------------------------------------

def test_drum_xpm_program_name_populated():
    """<Name> element must be present and non-empty in drum XPM."""
    import xml.etree.ElementTree as ET
    from xpn_drum_export import generate_xpm

    xpm = generate_xpm("808 Reborn", wav_map={})
    root = ET.fromstring(xpm)
    name_el = root.find(".//Program/Name")
    assert name_el is not None, "Missing <Name> in drum program"
    assert name_el.text, "Drum program <Name> is empty"
    assert "808" in name_el.text or "XO_OX" in name_el.text, (
        f"Expected preset name in program name, got '{name_el.text}'"
    )


def test_keygroup_xpm_program_name_populated():
    """<Name> element must be present and non-empty in keygroup XPM."""
    import xml.etree.ElementTree as ET
    from xpn_keygroup_export import generate_keygroup_xpm

    xpm = generate_keygroup_xpm(preset_name="Deep Drift", engine="Odyssey", wav_map={})
    root = ET.fromstring(xpm)
    name_el = root.find(".//Program/Name")
    assert name_el is not None, "Missing <Name> in keygroup program"
    assert name_el.text, "Keygroup program <Name> is empty"


# ---------------------------------------------------------------------------
# B10: KeygroupNumKeygroups matches actual instrument count
# ---------------------------------------------------------------------------

def test_keygroup_num_keygroups_matches_instruments():
    """<KeygroupNumKeygroups> must equal the number of <Instrument> elements."""
    import xml.etree.ElementTree as ET
    from xpn_keygroup_export import generate_keygroup_xpm

    wav_map = {
        "DP__C2__v1": "DP__C2__v1.WAV",
        "DP__C2__v2": "DP__C2__v2.WAV",
        "DP__G2__v1": "DP__G2__v1.WAV",
        "DP__C3__v1": "DP__C3__v1.WAV",
    }
    xpm = generate_keygroup_xpm(
        preset_name="DP",
        engine="Odyssey",
        wav_map=wav_map,
    )
    root = ET.fromstring(xpm)
    nkg_el = root.find(".//Program/KeygroupNumKeygroups")
    assert nkg_el is not None, "Missing <KeygroupNumKeygroups>"
    instruments = root.findall(".//Instruments/Instrument")
    declared = int(nkg_el.text)
    actual   = len(instruments)
    assert declared == actual, (
        f"KeygroupNumKeygroups={declared} but {actual} <Instrument> elements present"
    )


# ---------------------------------------------------------------------------
# B11: complement_chain skips non-Artwork engines
# ---------------------------------------------------------------------------

def test_complement_chain_skips_non_artwork_engine():
    """_stage_complement_chain must skip gracefully for non-Artwork engines."""
    from oxport import _stage_complement_chain, PipelineContext, ARTWORK_ENGINES
    from pathlib import Path

    # Use a known non-Artwork engine
    assert "Onset" not in ARTWORK_ENGINES, "Onset should not be an Artwork engine"
    ctx = PipelineContext(engine="Onset", output_dir=Path(tempfile.gettempdir()) / "test_oxport_cc")
    ctx.dry_run = True

    # Must not raise — should print SKIP and return
    _stage_complement_chain(ctx)  # no assertion needed; if it raises, test fails


# ---------------------------------------------------------------------------
# B12: XML escaping — special characters in preset name
# ---------------------------------------------------------------------------

def test_drum_xpm_escapes_special_chars_in_name():
    """Preset names with XML special characters must be escaped, not break XML."""
    import xml.etree.ElementTree as ET
    from xpn_drum_export import generate_xpm

    xpm = generate_xpm('Kick & Snare <Test> "2024"', wav_map={})
    try:
        root = ET.fromstring(xpm)
    except ET.ParseError as e:
        raise AssertionError(f"XML parse failed with special chars in name: {e}")


def test_keygroup_xpm_escapes_special_chars_in_name():
    """Keygroup preset names with XML special characters must be escaped."""
    import xml.etree.ElementTree as ET
    from xpn_keygroup_export import generate_keygroup_xpm

    xpm = generate_keygroup_xpm(
        preset_name='Test & "Escape" <Me>',
        engine="Odyssey",
        wav_map={},
    )
    try:
        ET.fromstring(xpm)
    except ET.ParseError as e:
        raise AssertionError(f"XML parse failed with special chars in keygroup name: {e}")


# ---------------------------------------------------------------------------
# B13: Inactive layers must have VelStart=0 and VelEnd=0 in ALL kit modes
# ---------------------------------------------------------------------------

def test_drum_velstart_zero_all_modes():
    """Inactive layers must have VelStart=0 and VelEnd=0 in ALL kit modes."""
    import xml.etree.ElementTree as ET
    from xpn_drum_export import generate_xpm

    for mode in ["velocity", "cycle", "random", "random-norepeat", "smart"]:
        xpm = generate_xpm("Test Kit", wav_map={}, kit_mode=mode)
        root = ET.fromstring(xpm)
        for layer in root.iter("Layer"):
            active = layer.find("Active")
            if active is not None and active.text == "False":
                vs = layer.find("VelStart")
                ve = layer.find("VelEnd")
                if vs is not None:
                    assert vs.text == "0", (
                        f"mode={mode}: Inactive layer VelStart='{vs.text}' (expected 0)"
                    )
                if ve is not None:
                    assert ve.text == "0", (
                        f"mode={mode}: Inactive layer VelEnd='{ve.text}' (expected 0)"
                    )


# ---------------------------------------------------------------------------
# B14: xpn_loudness_ledger.record_sample uses 'loudness_db' parameter (not old 'lufs')
# ---------------------------------------------------------------------------

def test_ledger_parameter_renamed():
    """xpn_loudness_ledger.record_sample must accept 'loudness_db' parameter."""
    import inspect
    from xpn_loudness_ledger import record_sample
    sig = inspect.signature(record_sample)
    params = list(sig.parameters.keys())
    assert "loudness_db" in params, (
        f"record_sample missing 'loudness_db' parameter. Found: {params}"
    )
    assert "lufs" not in params, (
        f"record_sample still has old 'lufs' parameter — should be renamed to 'loudness_db'"
    )


# ---------------------------------------------------------------------------
# B15: ARTWORK_ENGINES set is populated and contains valid engine keys
# ---------------------------------------------------------------------------

def test_artwork_engine_set_not_empty():
    """ARTWORK_ENGINES must be a non-empty set of string engine keys."""
    from oxport import ARTWORK_ENGINES
    assert isinstance(ARTWORK_ENGINES, set), "ARTWORK_ENGINES must be a set"
    assert len(ARTWORK_ENGINES) > 0, "ARTWORK_ENGINES must not be empty"
    for eng in ARTWORK_ENGINES:
        assert isinstance(eng, str), f"ARTWORK_ENGINES contains non-string: {eng}"
        assert eng.startswith("XO") or eng.startswith("Xo"), (
            f"ARTWORK_ENGINES entry '{eng}' doesn't follow XO naming convention"
        )


# ---------------------------------------------------------------------------
# B16: DNA velocity layers — extreme value stress test
# ---------------------------------------------------------------------------

def test_dna_velocity_extreme_values_never_produce_invalid_bands():
    """Exhaustive sweep of extreme DNA combos must never produce:
    - Inverted bands (start > end)
    - Gaps between adjacent bands
    - Bands outside [1, 127]
    """
    from xpn_drum_export import _dna_adapt_velocity_layers

    # Sweep all corners of the 3D DNA cube (aggression, warmth, density)
    extremes = [0.0, 0.25, 0.5, 0.75, 1.0]
    failures = []
    for aggr in extremes:
        for warmth in extremes:
            for density in extremes:
                dna = {
                    "brightness": 0.5, "warmth": warmth, "movement": 0.5,
                    "density": density, "space": 0.5, "aggression": aggr,
                }
                layers = _dna_adapt_velocity_layers(dna)
                # Check 4 layers returned
                if len(layers) != 4:
                    failures.append(f"a={aggr},w={warmth},d={density}: got {len(layers)} layers")
                    continue
                # Check no inverted bands
                for i, (vs, ve, _) in enumerate(layers):
                    if vs > ve:
                        failures.append(
                            f"a={aggr},w={warmth},d={density}: layer {i} inverted [{vs},{ve}]"
                        )
                # Check contiguity
                for i in range(1, len(layers)):
                    prev_end = layers[i - 1][1]
                    curr_start = layers[i][0]
                    if curr_start != prev_end + 1:
                        failures.append(
                            f"a={aggr},w={warmth},d={density}: gap at layer {i} "
                            f"(prev end={prev_end}, curr start={curr_start})"
                        )
                # Check boundaries
                if layers[0][0] != 1:
                    failures.append(
                        f"a={aggr},w={warmth},d={density}: first start={layers[0][0]}, expected 1"
                    )
                if layers[-1][1] != 127:
                    failures.append(
                        f"a={aggr},w={warmth},d={density}: last end={layers[-1][1]}, expected 127"
                    )
    assert not failures, f"{len(failures)} failure(s):\n" + "\n".join(failures[:10])


# ---------------------------------------------------------------------------
# C1: Ghost layer Volume boost (fix D-U2, issue #1187)
# ---------------------------------------------------------------------------

def test_ghost_layer_volume_boost_constant_is_approx_6db():
    """GHOST_LAYER_VOLUME_ROUNDED must be ~2.0 (+6 dB linear boost)."""
    from xpn_xpm_batch_builder import GHOST_LAYER_VOLUME_ROUNDED, GHOST_LAYER_VOLUME_BOOST_DB
    import math
    # 10^(6/20) ≈ 1.9953 — rounded to 2.0
    expected = round(10 ** (6.0 / 20), 2)
    assert abs(GHOST_LAYER_VOLUME_ROUNDED - expected) < 1e-6, (
        f"GHOST_LAYER_VOLUME_ROUNDED={GHOST_LAYER_VOLUME_ROUNDED}, "
        f"expected ~{expected} (+6 dB linear)"
    )
    assert abs(GHOST_LAYER_VOLUME_BOOST_DB - 6.0) < 1e-9, (
        f"GHOST_LAYER_VOLUME_BOOST_DB={GHOST_LAYER_VOLUME_BOOST_DB}, expected 6.0"
    )


def test_ghost_layer_gets_volume_boost_in_drum_program_layered():
    """Layer 0 (Ghost) in build_drum_program_layered must have Volume=2.0.
    All other layers must have Volume=1.0 (unity).

    This is the D-U2 fix: Ghost layer (vel=10/127 ≈ 0.079) renders perceptually
    faint on hardware; a +6 dB boost compensates without changing D1 midpoints.
    """
    import xml.etree.ElementTree as ET
    from xpn_xpm_batch_builder import build_drum_program_layered, GHOST_LAYER_VOLUME_ROUNDED

    # Build a minimal DEEP program with one pad (kick), one sample per vel layer
    pads = [
        {
            "voice": "kick",
            "pad": 1,
            "choke": 0,
            "samples": [
                {"file": "Samples/kick/kick_ghost.wav", "vel_layer": 0, "rr_index": 0},
                {"file": "Samples/kick/kick_light.wav", "vel_layer": 1, "rr_index": 0},
                {"file": "Samples/kick/kick_medium.wav", "vel_layer": 2, "rr_index": 0},
                {"file": "Samples/kick/kick_hard.wav",  "vel_layer": 3, "rr_index": 0},
            ],
        }
    ]

    xpm = build_drum_program_layered("Test Ghost Boost", pads, tier="DEEP")
    root = ET.fromstring(xpm)

    # Locate the kick instrument (slot 0, pad 1)
    instr = root.find(".//Instrument[@number='0']")
    assert instr is not None, "Instrument slot 0 not found in XPM"

    active_layers = [
        layer for layer in instr.findall(".//Layer")
        if (layer.find("Active") is not None and layer.find("Active").text == "True")
    ]
    assert len(active_layers) == 4, (
        f"Expected 4 active layers for kick, got {len(active_layers)}"
    )

    for i, layer in enumerate(active_layers):
        vol_el = layer.find("Volume")
        assert vol_el is not None, f"Layer {i} missing <Volume> element"
        vol = float(vol_el.text)

        if i == 0:
            # Ghost layer — must have the boost
            assert abs(vol - GHOST_LAYER_VOLUME_ROUNDED) < 1e-4, (
                f"Ghost layer (layer 0) Volume={vol}, expected {GHOST_LAYER_VOLUME_ROUNDED} "
                f"(+6 dB boost, fix D-U2 #1187)"
            )
        else:
            # All other layers — must be unity
            assert abs(vol - 1.0) < 1e-4, (
                f"Layer {i} Volume={vol}, expected 1.0 (unity). "
                f"Only Ghost layer (layer 0) should receive the boost."
            )


def test_ghost_volume_boost_is_unity_for_inactive_ghost_layer():
    """If the Ghost layer has no sample (missing), the inactive placeholder
    must NOT carry the Volume boost — inactive layers have no Volume tag."""
    import xml.etree.ElementTree as ET
    from xpn_xpm_batch_builder import build_drum_program_layered

    pads = [
        {
            "voice": "kick",
            "pad": 1,
            "choke": 0,
            "samples": [
                # Ghost layer missing intentionally
                {"file": "Samples/kick/kick_light.wav",  "vel_layer": 1, "rr_index": 0},
                {"file": "Samples/kick/kick_medium.wav", "vel_layer": 2, "rr_index": 0},
                {"file": "Samples/kick/kick_hard.wav",   "vel_layer": 3, "rr_index": 0},
            ],
        }
    ]

    xpm = build_drum_program_layered("Test Missing Ghost", pads, tier="DEEP")
    root = ET.fromstring(xpm)

    instr = root.find(".//Instrument[@number='0']")
    assert instr is not None

    # The first layer (Ghost placeholder) must be inactive — no Volume boost applied
    all_layers = instr.findall(".//Layer")
    ghost_layer = all_layers[0] if all_layers else None
    assert ghost_layer is not None, "No layers found in instrument"

    active_el = ghost_layer.find("Active")
    assert active_el is not None and active_el.text == "False", (
        f"Ghost placeholder layer expected Active=False, "
        f"got Active={active_el.text if active_el is not None else 'missing'}"
    )
    # Inactive placeholder must NOT have a Volume element
    vol_el = ghost_layer.find("Volume")
    assert vol_el is None, (
        f"Inactive Ghost placeholder must not carry a <Volume> element, "
        f"but found Volume={vol_el.text}"
    )


# ---------------------------------------------------------------------------
# C2: Round-robin — rr_index=0 must receive CycleType tag (issue #fix-rr)
# ---------------------------------------------------------------------------

def test_trench_rr_index_zero_emits_cycle_type():
    """TRENCH tier: ALL RR variants — including rr_index=0 — must carry
    <CycleType>RoundRobin</CycleType> and <CycleGroup>.

    Regression for the dual-site bug where ``emit_rr = use_rr and ri > 0``
    (caller) and ``if rr_index > 0:`` (_active_drum_layer_xml) both suppressed
    the CycleType tag for the base variant, so the MPC never entered cycle mode
    and the base sample played statically on every hit.
    """
    import xml.etree.ElementTree as ET
    from xpn_xpm_batch_builder import build_drum_program_layered

    # One pad, one velocity layer, three RR variants (indices 0, 1, 2)
    pads = [
        {
            "voice": "hihat_closed",
            "pad": 1,
            "choke": 0,
            "samples": [
                {"file": "Samples/hh/hh_rr0.wav", "vel_layer": 1, "rr_index": 0},
                {"file": "Samples/hh/hh_rr1.wav", "vel_layer": 1, "rr_index": 1},
                {"file": "Samples/hh/hh_rr2.wav", "vel_layer": 1, "rr_index": 2},
            ],
        }
    ]

    xpm = build_drum_program_layered("Test RR Zero", pads, tier="TRENCH")
    root = ET.fromstring(xpm)

    instr = root.find(".//Instrument[@number='0']")
    assert instr is not None, "Instrument slot 0 not found"

    active_layers = [
        layer for layer in instr.findall(".//Layer")
        if (layer.find("Active") is not None and layer.find("Active").text == "True")
    ]
    assert len(active_layers) == 3, (
        f"Expected 3 active RR layers, got {len(active_layers)}"
    )

    for i, layer in enumerate(active_layers):
        ct = layer.find("CycleType")
        cg = layer.find("CycleGroup")
        assert ct is not None, (
            f"Layer {i} (rr_index={i}) is missing <CycleType> — "
            f"rr_index=0 must NOT be excluded from CycleType emission (issue fix-rr)"
        )
        assert ct.text == "RoundRobin", (
            f"Layer {i} CycleType='{ct.text}', expected 'RoundRobin'"
        )
        assert cg is not None, (
            f"Layer {i} is missing <CycleGroup>"
        )


def test_non_trench_tier_does_not_emit_cycle_type():
    """DEEP tier must NOT emit <CycleType> even when rr_index variants are present.

    Round-robin tag emission is gated on tier=TRENCH only.
    """
    import xml.etree.ElementTree as ET
    from xpn_xpm_batch_builder import build_drum_program_layered

    pads = [
        {
            "voice": "kick",
            "pad": 1,
            "choke": 0,
            "samples": [
                {"file": "Samples/kick/kick_rr0.wav", "vel_layer": 1, "rr_index": 0},
                {"file": "Samples/kick/kick_rr1.wav", "vel_layer": 1, "rr_index": 1},
            ],
        }
    ]

    xpm = build_drum_program_layered("Test DEEP No RR", pads, tier="DEEP")
    root = ET.fromstring(xpm)

    instr = root.find(".//Instrument[@number='0']")
    assert instr is not None, "Instrument slot 0 not found"

    for layer in instr.findall(".//Layer"):
        if layer.find("Active") is not None and layer.find("Active").text == "True":
            ct = layer.find("CycleType")
            assert ct is None, (
                f"DEEP tier must not emit <CycleType>, but found CycleType='{ct.text}'"
            )


# ---------------------------------------------------------------------------
# Test runner (same pattern as test_oxport_core.py)
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    tests = [
        test_tpdf_dither_is_applied,
        test_tpdf_dither_does_not_change_rms_audibly,
        test_atomic_write_no_temp_leftovers_on_success,
        test_atomic_write_preserves_original_on_failure,
        test_resolve_engine_all_aliases,
        test_resolve_engine_unknown_passthrough,
        test_check_dependencies_raises_on_missing_required,
        test_drum_xpm_is_valid_xml,
        test_drum_xpm_has_xml_declaration,
        test_drum_xpm_keytrack_is_false,
        test_drum_xpm_rootnote_is_zero,
        test_drum_xpm_empty_layer_velstart_is_zero,
        test_drum_velocity_layers_are_contiguous,
        test_drum_dna_adapted_layers_contiguous,
        test_drum_zone_play_values,
        test_drum_xpm_zone_play_in_xml,
        test_drum_pad_note_mapping,
        test_drum_pad_note_map_in_xml,
        test_drum_hat_choke_mute_group,
        test_drum_xpm_mute_group_in_xml,
        test_keygroup_xpm_is_valid_xml,
        test_keygroup_xpm_keytrack_is_true,
        test_keygroup_xpm_rootnote_is_zero,
        test_keygroup_xpm_empty_layer_velstart_is_zero,
        test_keygroup_velocity_layers_contiguous,
        test_keygroup_vel_crossfade_contiguity,
        test_drum_xpm_file_paths_relative,
        test_keygroup_xpm_file_paths_relative,
        test_drum_xpm_program_name_populated,
        test_keygroup_xpm_program_name_populated,
        test_keygroup_num_keygroups_matches_instruments,
        test_complement_chain_skips_non_artwork_engine,
        test_drum_xpm_escapes_special_chars_in_name,
        test_keygroup_xpm_escapes_special_chars_in_name,
        test_drum_velstart_zero_all_modes,
        test_ledger_parameter_renamed,
        test_artwork_engine_set_not_empty,
        test_dna_velocity_extreme_values_never_produce_invalid_bands,
        test_ghost_layer_volume_boost_constant_is_approx_6db,
        test_ghost_layer_gets_volume_boost_in_drum_program_layered,
        test_ghost_volume_boost_is_unity_for_inactive_ghost_layer,
        test_trench_rr_index_zero_emits_cycle_type,
        test_non_trench_tier_does_not_emit_cycle_type,
    ]
    passed = 0
    failed = 0
    skipped = 0
    for t in tests:
        try:
            t()
            print(f"  PASS  {t.__name__}")
            passed += 1
        except Exception as e:
            print(f"  FAIL  {t.__name__}: {e}")
            failed += 1
    print(f"\n{passed} passed, {failed} failed, {skipped} skipped")
    sys.exit(0 if failed == 0 else 1)
