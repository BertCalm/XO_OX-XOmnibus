#!/usr/bin/env python3
"""
XPN Validator / Linter — XO_OX Designs
Validates .xpn expansion archives against the XPN/XPM format specification.

Checks every rule from Rex's XPN Bible:
  - ZIP structure (Expansions/, Programs/, Samples/)
  - Manifest presence and required fields
  - XPM XML validity (encoding, booleans, velocity layers, KeyTrack, paths)
  - Sample file integrity (WAV format, non-zero length, references)
  - Preview file naming (must match .xpm base name)
  - Cover art presence
  - Orphaned samples (in ZIP but not referenced by any program)

Severity levels:
  [CRITICAL]  Will cause MPC load failure or silent breakage
  [WARNING]   May cause unexpected behavior (ghost triggers, wrong pitch, etc.)
  [INFO]      Style or best-practice recommendation

Usage:
    # Validate a single .xpn file
    python3 xpn_validator.py MyPack.xpn

    # Validate all .xpn files in a directory
    python3 xpn_validator.py /path/to/packs/

    # JSON output for CI pipelines
    python3 xpn_validator.py MyPack.xpn --json

    # Auto-fix trivial issues (boolean casing, etc.)
    python3 xpn_validator.py MyPack.xpn --fix
"""

import argparse
import json
import os
import struct
import sys
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional, Union
from xml.etree import ElementTree as ET


# =============================================================================
# FINDING MODEL
# =============================================================================

CRITICAL = "CRITICAL"
WARNING  = "WARNING"
INFO     = "INFO"


@dataclass
class Finding:
    """A single validation finding."""
    severity: str           # CRITICAL / WARNING / INFO
    rule: str               # Short rule ID (e.g. "vel-start-zero")
    message: str            # Human-readable description
    file: str = ""          # File path within the ZIP
    line: int = 0           # Line number (XML findings)
    fixable: bool = False   # Can --fix auto-repair this?

    def to_dict(self) -> dict:
        d = {
            "severity": self.severity,
            "rule": self.rule,
            "message": self.message,
        }
        if self.file:
            d["file"] = self.file
        if self.line:
            d["line"] = self.line
        if self.fixable:
            d["fixable"] = True
        return d

    def __str__(self) -> str:
        loc = ""
        if self.file:
            loc = f" {self.file}"
            if self.line:
                loc += f":{self.line}"
        fix = " [FIXABLE]" if self.fixable else ""
        return f"  [{self.severity}]{loc} ({self.rule}) {self.message}{fix}"


@dataclass
class ValidationResult:
    """Aggregated results for one .xpn file."""
    xpn_path: str
    findings: list = field(default_factory=list)

    @property
    def passed(self) -> bool:
        return not any(f.severity == CRITICAL for f in self.findings)

    @property
    def counts(self) -> dict:
        c = {CRITICAL: 0, WARNING: 0, INFO: 0}
        for f in self.findings:
            c[f.severity] = c.get(f.severity, 0) + 1
        return c

    def to_dict(self) -> dict:
        return {
            "file": self.xpn_path,
            "passed": self.passed,
            "counts": self.counts,
            "findings": [f.to_dict() for f in self.findings],
        }

    def print_report(self):
        status = "PASS" if self.passed else "FAIL"
        counts = self.counts
        print(f"\n{'=' * 60}")
        print(f"  {status}  {self.xpn_path}")
        print(f"  Critical: {counts[CRITICAL]}  "
              f"Warning: {counts[WARNING]}  "
              f"Info: {counts[INFO]}")
        print(f"{'=' * 60}")
        for f in self.findings:
            print(str(f))
        if not self.findings:
            print("  No issues found.")
        print()


# =============================================================================
# BOOLEAN CASING
# =============================================================================

# Rex's Golden Rule #12: True/False (capitalized), not true/1
VALID_BOOLEANS = {"True", "False"}

# XPM elements that hold boolean values
BOOLEAN_ELEMENTS = {
    "Active", "KeyTrack", "OneShot", "Loop", "Mute", "Mono",
    "IgnoreBaseNote", "InsertsEnabled", "Sync", "Retrigger",
    "FilterDecayType", "FilterADEnvelope", "VolumeDecayType",
    "VolumeADEnvelope",
}

# Map of bad boolean values to correct ones (for --fix)
BOOL_FIX_MAP = {
    "true": "True", "false": "False",
    "TRUE": "True", "FALSE": "False",
    "1": "True", "0": "False",
    "yes": "True", "no": "False",
}


# =============================================================================
# STRUCTURE CHECKS
# =============================================================================

def check_zip_valid(xpn_path: str, findings: list) -> Optional[zipfile.ZipFile]:
    """Verify the file is a valid ZIP archive."""
    try:
        zf = zipfile.ZipFile(xpn_path, "r")
        # Test for corruption
        bad = zf.testzip()
        if bad:
            findings.append(Finding(
                CRITICAL, "zip-corrupt",
                f"Corrupt entry in ZIP: {bad}",
            ))
        return zf
    except zipfile.BadZipFile:
        findings.append(Finding(
            CRITICAL, "zip-invalid",
            "File is not a valid ZIP archive",
        ))
        return None
    except Exception as e:
        findings.append(Finding(
            CRITICAL, "zip-error",
            f"Cannot open file: {e}",
        ))
        return None


def check_structure(zf: zipfile.ZipFile, findings: list) -> dict:
    """
    Validate the XPN directory structure.
    Returns a dict of categorized file lists for downstream checks.
    """
    names = zf.namelist()
    catalog = {
        "programs": [],     # .xpm files
        "samples": [],      # audio files in Samples/
        "previews": [],     # preview audio in Programs/
        "manifest": None,
        "expansion_xml": None,
        "artwork": [],
        "all": names,
    }

    has_expansions = False
    has_programs = False
    has_samples = False

    seen_basenames = {}
    for name in names:
        parts = name.split("/")
        top = parts[0] if parts else ""

        # Check for absolute paths (Rex Rule #5)
        if name.startswith("/") or name.startswith("\\"):
            findings.append(Finding(
                CRITICAL, "abs-path",
                f"Absolute path in ZIP: {name}",
                file=name,
            ))

        # Check for path traversal
        if ".." in name:
            findings.append(Finding(
                CRITICAL, "path-traversal",
                f"Path traversal in ZIP entry: {name}",
                file=name,
            ))

        # Duplicate basenames
        basename = os.path.basename(name)
        if basename and not name.endswith("/"):
            if basename in seen_basenames:
                findings.append(Finding(
                    WARNING, "dup-filename",
                    f"Duplicate filename '{basename}' "
                    f"(also at {seen_basenames[basename]})",
                    file=name,
                ))
            else:
                seen_basenames[basename] = name

        # Categorize
        if top == "Expansions":
            has_expansions = True
            if len(parts) == 2:
                if parts[1] == "manifest":
                    catalog["manifest"] = name
                elif parts[1] == "Expansion.xml":
                    catalog["expansion_xml"] = name

        elif top == "Programs":
            has_programs = True
            if name.lower().endswith(".xpm"):
                catalog["programs"].append(name)
            elif name.lower().endswith((".mp3", ".wav")) and not name.lower().endswith(".xpm"):
                catalog["previews"].append(name)

        elif top == "Samples":
            has_samples = True
            if not name.endswith("/"):
                catalog["samples"].append(name)

        # Artwork (root level or Expansions/)
        if basename.lower() in ("artwork.png", "artwork_2000.png"):
            catalog["artwork"].append(name)

    # Required directories
    if not has_expansions:
        findings.append(Finding(
            CRITICAL, "no-expansions-dir",
            "Missing Expansions/ directory",
        ))
    if not has_programs:
        findings.append(Finding(
            CRITICAL, "no-programs-dir",
            "Missing Programs/ directory",
        ))
    if not has_samples:
        findings.append(Finding(
            WARNING, "no-samples-dir",
            "Missing Samples/ directory (no audio content)",
        ))

    # Manifest
    if not catalog["manifest"]:
        findings.append(Finding(
            CRITICAL, "no-manifest",
            "Missing Expansions/manifest — MPC won't show the pack",
        ))

    if not catalog["programs"]:
        findings.append(Finding(
            CRITICAL, "no-programs",
            "No .xpm program files found in Programs/",
        ))

    return catalog


# =============================================================================
# MANIFEST CHECKS
# =============================================================================

def check_manifest(zf: zipfile.ZipFile, catalog: dict, findings: list):
    """Validate the Expansions/manifest content."""
    if not catalog["manifest"]:
        return

    try:
        content = zf.read(catalog["manifest"]).decode("utf-8")
    except Exception as e:
        findings.append(Finding(
            CRITICAL, "manifest-read-error",
            f"Cannot read manifest: {e}",
            file=catalog["manifest"],
        ))
        return

    required_fields = {"Name"}
    found_fields = set()

    for line_num, line in enumerate(content.splitlines(), 1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            findings.append(Finding(
                WARNING, "manifest-bad-line",
                f"Manifest line {line_num} has no '=' separator: {line!r}",
                file=catalog["manifest"],
                line=line_num,
            ))
            continue
        key, _, val = line.partition("=")
        key = key.strip()
        val = val.strip()
        found_fields.add(key)

        if key == "Name" and not val:
            findings.append(Finding(
                CRITICAL, "manifest-empty-name",
                "Manifest Name= field is empty",
                file=catalog["manifest"],
                line=line_num,
            ))

    for req in required_fields:
        if req not in found_fields:
            findings.append(Finding(
                CRITICAL, "manifest-missing-field",
                f"Manifest missing required field: {req}",
                file=catalog["manifest"],
            ))

    # Recommended fields
    for rec in ["Version", "Author"]:
        if rec not in found_fields:
            findings.append(Finding(
                INFO, "manifest-recommended-field",
                f"Manifest missing recommended field: {rec}",
                file=catalog["manifest"],
            ))


# =============================================================================
# XPM PROGRAM CHECKS
# =============================================================================

def _find_line_number(xml_text: str, search_text: str, occurrence: int = 1) -> int:
    """Find approximate line number for a text pattern in raw XML."""
    count = 0
    for i, line in enumerate(xml_text.splitlines(), 1):
        if search_text in line:
            count += 1
            if count >= occurrence:
                return i
    return 0


def check_xpm(zf: zipfile.ZipFile, xpm_path: str, catalog: dict,
              findings: list) -> set:
    """
    Validate a single .xpm program file.
    Returns set of sample file paths referenced by this program.
    """
    referenced_samples = set()

    try:
        raw = zf.read(xpm_path).decode("utf-8")
    except UnicodeDecodeError:
        findings.append(Finding(
            CRITICAL, "xpm-encoding",
            "XPM file is not valid UTF-8",
            file=xpm_path,
        ))
        return referenced_samples
    except Exception as e:
        findings.append(Finding(
            CRITICAL, "xpm-read-error",
            f"Cannot read XPM: {e}",
            file=xpm_path,
        ))
        return referenced_samples

    # Check XML declaration
    if not raw.lstrip().startswith("<?xml"):
        findings.append(Finding(
            WARNING, "xpm-no-xml-decl",
            "Missing <?xml version=\"1.0\" encoding=\"UTF-8\"?> declaration",
            file=xpm_path,
        ))
    elif 'encoding="UTF-8"' not in raw[:200] and "encoding='UTF-8'" not in raw[:200]:
        findings.append(Finding(
            WARNING, "xpm-encoding-decl",
            "XML declaration missing encoding=\"UTF-8\"",
            file=xpm_path,
        ))

    # Parse XML
    try:
        root = ET.fromstring(raw)
    except ET.ParseError as e:
        findings.append(Finding(
            CRITICAL, "xpm-parse-error",
            f"Invalid XML: {e}",
            file=xpm_path,
        ))
        return referenced_samples

    # Find <Program> element
    program = root.find(".//Program")
    if program is None:
        # Try root-level Program
        if root.tag == "Program":
            program = root
        else:
            findings.append(Finding(
                CRITICAL, "xpm-no-program",
                "No <Program> element found",
                file=xpm_path,
            ))
            return referenced_samples

    prog_type = program.get("type", "")
    if prog_type not in ("Keygroup", "Drum", "Plugin", "MIDI"):
        findings.append(Finding(
            WARNING, "xpm-unknown-type",
            f"Unknown program type: {prog_type!r}",
            file=xpm_path,
        ))

    is_drum = prog_type == "Drum"
    is_keygroup = prog_type == "Keygroup"

    # Check all Instrument elements
    instruments = program.findall(".//Instrument") or []

    # Also check Layer elements (drum export format uses Layers/Layer)
    layers = program.findall(".//Layer") or []

    # Unified check for both Instrument and Layer elements
    _check_instrument_elements(
        instruments + layers, raw, xpm_path, is_drum, is_keygroup,
        referenced_samples, findings, catalog,
    )

    # Check boolean casing across entire tree
    _check_boolean_casing(root, raw, xpm_path, findings)

    return referenced_samples


def _check_instrument_elements(
    elements: list, raw_xml: str, xpm_path: str,
    is_drum: bool, is_keygroup: bool,
    referenced_samples: set, findings: list, catalog: dict,
):
    """Validate Instrument or Layer elements."""
    sample_names_in_zip = {os.path.basename(s) for s in catalog["samples"]}
    sample_paths_in_zip = set(catalog["samples"])

    for elem in elements:
        # --- VelStart on empty layers (Rex Rule #3, #9, #10) ---
        vel_start_el = elem.find("VelStart")
        vel_end_el = elem.find("VelEnd")
        active_el = elem.find("Active")
        file_el = elem.find("File")
        sample_file_el = elem.find("SampleFile")
        sample_name_el = elem.find("SampleName")

        vel_start = int(vel_start_el.text or "0") if vel_start_el is not None and vel_start_el.text else 0
        vel_end = int(vel_end_el.text or "0") if vel_end_el is not None and vel_end_el.text else 0
        is_active = (active_el is not None and active_el.text == "True")

        # Determine if this is an empty layer
        has_file = False
        file_path = ""
        for f_el in [file_el, sample_file_el]:
            if f_el is not None and f_el.text and f_el.text.strip():
                has_file = True
                file_path = f_el.text.strip()
                break

        if not has_file and not is_active:
            # Empty layer — VelStart must be 0
            if vel_start != 0:
                line = _find_line_number(raw_xml, f"<VelStart>{vel_start}</VelStart>")
                findings.append(Finding(
                    CRITICAL, "vel-start-empty",
                    f"Empty layer has VelStart={vel_start} (must be 0 — ghost trigger risk)",
                    file=xpm_path, line=line,
                ))
            if vel_end != 0:
                line = _find_line_number(raw_xml, f"<VelEnd>{vel_end}</VelEnd>")
                findings.append(Finding(
                    WARNING, "vel-end-empty",
                    f"Empty layer has VelEnd={vel_end} (should be 0)",
                    file=xpm_path, line=line,
                ))
        elif has_file and is_active:
            # Active layer — VelStart should be >= 1
            if vel_start == 0 and vel_end > 0:
                line = _find_line_number(raw_xml, "<VelStart>0</VelStart>")
                findings.append(Finding(
                    WARNING, "vel-start-zero-active",
                    "Active layer has VelStart=0 — will trigger on zero-velocity notes",
                    file=xpm_path, line=line,
                ))

        # --- KeyTrack correctness (Rex Rules #1, #7) ---
        keytrack_el = elem.find("KeyTrack")
        if keytrack_el is not None and keytrack_el.text:
            kt = keytrack_el.text.strip()
            if is_drum and kt == "True" and has_file and is_active:
                line = _find_line_number(raw_xml, "<KeyTrack>True</KeyTrack>")
                findings.append(Finding(
                    WARNING, "keytrack-drum",
                    "Drum program has KeyTrack=True (should be False for drums)",
                    file=xpm_path, line=line,
                ))
            elif is_keygroup and kt == "False" and has_file and is_active:
                line = _find_line_number(raw_xml, "<KeyTrack>False</KeyTrack>")
                findings.append(Finding(
                    WARNING, "keytrack-keygroup",
                    "Keygroup program has KeyTrack=False (should be True for keygroups)",
                    file=xpm_path, line=line,
                ))

        # --- Sample reference validation ---
        if has_file and file_path:
            referenced_samples.add(file_path)

            # Check absolute path
            if file_path.startswith("/") or file_path.startswith("\\"):
                line = _find_line_number(raw_xml, file_path)
                findings.append(Finding(
                    CRITICAL, "sample-abs-path",
                    f"Absolute sample path: {file_path}",
                    file=xpm_path, line=line,
                ))
            # Check path traversal
            elif ".." in file_path:
                line = _find_line_number(raw_xml, file_path)
                findings.append(Finding(
                    CRITICAL, "sample-path-traversal",
                    f"Path traversal in sample reference: {file_path}",
                    file=xpm_path, line=line,
                ))
            # Check file exists in ZIP
            elif file_path not in sample_paths_in_zip:
                basename = os.path.basename(file_path)
                if basename not in sample_names_in_zip:
                    line = _find_line_number(raw_xml, file_path)
                    findings.append(Finding(
                        CRITICAL, "sample-missing",
                        f"Referenced sample not found in ZIP: {file_path}",
                        file=xpm_path, line=line,
                    ))
                else:
                    line = _find_line_number(raw_xml, file_path)
                    findings.append(Finding(
                        WARNING, "sample-path-mismatch",
                        f"Sample path mismatch — file exists but at different path: {file_path}",
                        file=xpm_path, line=line,
                    ))

        # Also track SampleFile references
        if sample_file_el is not None and sample_file_el.text and sample_file_el.text.strip():
            referenced_samples.add(sample_file_el.text.strip())


def _check_boolean_casing(root: ET.Element, raw_xml: str, xpm_path: str,
                          findings: list):
    """Check all known boolean elements for correct True/False casing."""
    for elem in root.iter():
        if elem.tag in BOOLEAN_ELEMENTS and elem.text is not None:
            val = elem.text.strip()
            if val and val not in VALID_BOOLEANS:
                line = _find_line_number(raw_xml, f"<{elem.tag}>{val}</{elem.tag}>")
                fixable = val.lower() in ("true", "false") or val in ("1", "0")
                findings.append(Finding(
                    WARNING, "bool-casing",
                    f"<{elem.tag}>{val}</{elem.tag}> — "
                    f"should be True or False (capitalized)",
                    file=xpm_path, line=line, fixable=fixable,
                ))


# =============================================================================
# PREVIEW FILE CHECKS
# =============================================================================

def check_previews(catalog: dict, findings: list):
    """
    Preview file must have the same base name as its .xpm (Rex Rule #4).
    """
    xpm_stems = set()
    for xpm in catalog["programs"]:
        stem = os.path.splitext(os.path.basename(xpm))[0]
        xpm_stems.add(stem)

    preview_stems = set()
    for prev in catalog["previews"]:
        stem = os.path.splitext(os.path.basename(prev))[0]
        preview_stems.add(stem)

    for stem in xpm_stems:
        if stem not in preview_stems:
            findings.append(Finding(
                INFO, "no-preview",
                f"No preview audio file for program '{stem}' "
                f"(expected Programs/{stem}.mp3 or .wav)",
            ))


# =============================================================================
# ARTWORK CHECKS
# =============================================================================

def check_artwork(catalog: dict, findings: list):
    """Check for cover artwork."""
    if not catalog["artwork"]:
        findings.append(Finding(
            INFO, "no-artwork",
            "No artwork.png found (optional but recommended)",
        ))


# =============================================================================
# AUDIO / SAMPLE CHECKS
# =============================================================================

def check_samples(zf: zipfile.ZipFile, catalog: dict,
                  referenced_samples: set, findings: list):
    """Validate sample audio files."""
    sample_rates = set()

    for sample_path in catalog["samples"]:
        info = zf.getinfo(sample_path)

        # Zero-length check
        if info.file_size == 0:
            findings.append(Finding(
                CRITICAL, "sample-zero-length",
                f"Zero-length audio file",
                file=sample_path,
            ))
            continue

        # WAV format validation
        if sample_path.lower().endswith(".wav"):
            try:
                data = zf.read(sample_path)
                sr = _validate_wav(data, sample_path, findings)
                if sr:
                    sample_rates.add(sr)
            except Exception as e:
                findings.append(Finding(
                    WARNING, "sample-read-error",
                    f"Cannot read sample: {e}",
                    file=sample_path,
                ))

    # Orphaned samples — in ZIP but not referenced by any program
    all_sample_paths = set(catalog["samples"])
    all_sample_basenames = {os.path.basename(s): s for s in catalog["samples"]}

    # Build full set of referenced paths (including basename matches)
    matched_samples = set()
    for ref in referenced_samples:
        if ref in all_sample_paths:
            matched_samples.add(ref)
        else:
            basename = os.path.basename(ref)
            if basename in all_sample_basenames:
                matched_samples.add(all_sample_basenames[basename])

    orphaned = all_sample_paths - matched_samples
    if orphaned:
        for orph in sorted(orphaned):
            findings.append(Finding(
                INFO, "orphaned-sample",
                f"Sample not referenced by any program",
                file=orph,
            ))

    # Sample rate consistency
    if len(sample_rates) > 1:
        rates_str = ", ".join(f"{r}Hz" for r in sorted(sample_rates))
        findings.append(Finding(
            WARNING, "sample-rate-mixed",
            f"Mixed sample rates in pack: {rates_str} — "
            f"MPC will resample, but 44100Hz is recommended",
        ))


def _validate_wav(data: bytes, path: str, findings: list) -> Optional[int]:
    """
    Basic WAV header validation. Returns sample rate or None.
    """
    if len(data) < 44:
        findings.append(Finding(
            CRITICAL, "wav-too-short",
            "WAV file too short for valid header (< 44 bytes)",
            file=path,
        ))
        return None

    # RIFF header
    if data[:4] != b"RIFF":
        findings.append(Finding(
            CRITICAL, "wav-no-riff",
            "Not a valid WAV file (missing RIFF header)",
            file=path,
        ))
        return None

    if data[8:12] != b"WAVE":
        findings.append(Finding(
            CRITICAL, "wav-no-wave",
            "Not a valid WAV file (missing WAVE identifier)",
            file=path,
        ))
        return None

    # Find fmt chunk
    pos = 12
    sample_rate = None
    found_fmt = False
    found_data = False

    while pos < len(data) - 8:
        chunk_id = data[pos:pos + 4]
        try:
            chunk_size = struct.unpack_from("<I", data, pos + 4)[0]
        except struct.error:
            break

        if chunk_id == b"fmt ":
            found_fmt = True
            if chunk_size >= 16:
                audio_format = struct.unpack_from("<H", data, pos + 8)[0]
                num_channels = struct.unpack_from("<H", data, pos + 10)[0]
                sample_rate = struct.unpack_from("<I", data, pos + 12)[0]
                bits_per_sample = struct.unpack_from("<H", data, pos + 22)[0]

                if audio_format not in (1, 3):  # 1=PCM, 3=IEEE float
                    findings.append(Finding(
                        WARNING, "wav-compressed",
                        f"WAV uses compressed format ({audio_format}) — "
                        f"PCM (1) or float (3) recommended",
                        file=path,
                    ))

                if bits_per_sample not in (16, 24, 32):
                    findings.append(Finding(
                        INFO, "wav-bit-depth",
                        f"Unusual bit depth: {bits_per_sample}-bit "
                        f"(16 or 24-bit recommended)",
                        file=path,
                    ))

        elif chunk_id == b"data":
            found_data = True
            if chunk_size == 0:
                findings.append(Finding(
                    CRITICAL, "wav-empty-data",
                    "WAV data chunk is empty (0 bytes of audio)",
                    file=path,
                ))

        pos += 8 + chunk_size
        # Chunks are word-aligned
        if chunk_size % 2 != 0:
            pos += 1

    if not found_fmt:
        findings.append(Finding(
            CRITICAL, "wav-no-fmt",
            "WAV missing fmt chunk",
            file=path,
        ))

    if not found_data:
        findings.append(Finding(
            CRITICAL, "wav-no-data",
            "WAV missing data chunk",
            file=path,
        ))

    return sample_rate


# =============================================================================
# AUTO-FIX
# =============================================================================

def apply_fixes(xpn_path: str, findings: list) -> int:
    """
    Apply auto-fixes for fixable findings. Rewrites the .xpn in place.
    Returns count of fixes applied.
    """
    fixable = [f for f in findings if f.fixable]
    if not fixable:
        print("  No fixable issues found.")
        return 0

    # Only boolean casing fixes are supported
    bool_fixes = [f for f in fixable if f.rule == "bool-casing"]
    if not bool_fixes:
        print("  No auto-fixable issues found.")
        return 0

    fix_count = 0
    tmp_path = xpn_path + ".tmp"

    try:
        with zipfile.ZipFile(xpn_path, "r") as zf_in:
            with zipfile.ZipFile(tmp_path, "w", zipfile.ZIP_DEFLATED) as zf_out:
                for item in zf_in.infolist():
                    data = zf_in.read(item.filename)

                    if item.filename.lower().endswith(".xpm"):
                        text = data.decode("utf-8")
                        original = text

                        # Fix boolean casing
                        for tag in BOOLEAN_ELEMENTS:
                            for bad, good in BOOL_FIX_MAP.items():
                                old = f"<{tag}>{bad}</{tag}>"
                                new = f"<{tag}>{good}</{tag}>"
                                if old in text:
                                    text = text.replace(old, new)

                        if text != original:
                            fix_count += text != original
                            data = text.encode("utf-8")

                    zf_out.writestr(item, data)

        # Replace original
        os.replace(tmp_path, xpn_path)
        print(f"  Fixed {fix_count} XPM file(s) with boolean casing corrections.")

    except Exception as e:
        print(f"  [ERROR] Fix failed: {e}")
        if os.path.exists(tmp_path):
            os.remove(tmp_path)

    return fix_count


# =============================================================================
# MAIN VALIDATOR
# =============================================================================

def validate_xpn(xpn_path: str) -> ValidationResult:
    """Run all validation checks on a single .xpn file."""
    result = ValidationResult(xpn_path=xpn_path)
    findings = result.findings

    # 1. Valid ZIP?
    zf = check_zip_valid(xpn_path, findings)
    if zf is None:
        return result

    with zf:
        # 2. Directory structure
        catalog = check_structure(zf, findings)

        # 3. Manifest
        check_manifest(zf, catalog, findings)

        # 4. XPM programs
        all_referenced_samples = set()
        for xpm_path in catalog["programs"]:
            refs = check_xpm(zf, xpm_path, catalog, findings)
            all_referenced_samples.update(refs)

        # 5. Preview files
        check_previews(catalog, findings)

        # 6. Artwork
        check_artwork(catalog, findings)

        # 7. Sample files + orphan detection
        check_samples(zf, catalog, all_referenced_samples, findings)

    return result


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Validator / Linter — XO_OX Designs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("path",
                        help="Path to .xpn file or directory of .xpn files")
    parser.add_argument("--json", action="store_true",
                        help="Output results as JSON")
    parser.add_argument("--fix", action="store_true",
                        help="Auto-fix trivial issues (boolean casing)")
    parser.add_argument("--severity", default="info",
                        choices=["critical", "warning", "info"],
                        help="Minimum severity to display (default: info)")
    args = parser.parse_args()

    target = Path(args.path)
    xpn_files = []

    if target.is_dir():
        xpn_files = sorted(target.glob("*.xpn"))
        if not xpn_files:
            print(f"No .xpn files found in {target}")
            return 1
    elif target.is_file():
        xpn_files = [target]
    else:
        print(f"Path not found: {target}")
        return 1

    severity_filter = {
        "critical": {CRITICAL},
        "warning":  {CRITICAL, WARNING},
        "info":     {CRITICAL, WARNING, INFO},
    }[args.severity]

    results = []
    any_failed = False

    for xpn in xpn_files:
        result = validate_xpn(str(xpn))

        if args.fix and not result.passed:
            apply_fixes(str(xpn), result.findings)
            # Re-validate after fixes
            result = validate_xpn(str(xpn))

        # Filter by severity
        result.findings = [f for f in result.findings if f.severity in severity_filter]

        results.append(result)
        if not result.passed:
            any_failed = True

    if args.json:
        output = {
            "results": [r.to_dict() for r in results],
            "total_files": len(results),
            "passed": sum(1 for r in results if r.passed),
            "failed": sum(1 for r in results if not r.passed),
        }
        print(json.dumps(output, indent=2))
    else:
        for result in results:
            result.print_report()

        if len(results) > 1:
            passed = sum(1 for r in results if r.passed)
            failed = len(results) - passed
            print(f"\nSummary: {passed} PASS / {failed} FAIL "
                  f"({len(results)} files)")

    return 1 if any_failed else 0


if __name__ == "__main__":
    sys.exit(main())
