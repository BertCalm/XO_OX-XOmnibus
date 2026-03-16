"""
xpn_cover_art_audit.py — XO_OX Cover Art Auditor

Audits cover art files inside a .xpn (ZIP) pack to verify they meet
XO_OX visual standards and MPC display requirements.

Uses only Python stdlib: zipfile, struct, zlib, argparse, json, os, io.

Usage:
    python xpn_cover_art_audit.py --xpn pack.xpn [--strict] [--format text|json]
"""

import argparse
import io
import json
import os
import struct
import sys
import zipfile

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

COMMON_COVER_NAMES = {"cover.png", "artwork.png", "cover_art.png", "art.png",
                      "cover.jpg", "cover.jpeg", "artwork.jpg", "artwork.jpeg"}

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"

# PNG color type IDs
PNG_COLOR_TYPES = {
    0: "Grayscale",
    2: "RGB",
    3: "Indexed",
    4: "Grayscale+Alpha",
    6: "RGBA",
}

MIN_SIZE = 200        # MPC browser minimum (px)
WARN_SIZE = 400       # recommended minimum (px)
MAX_FILE_BYTES = 2 * 1024 * 1024  # 2 MB


# ---------------------------------------------------------------------------
# PNG parser (binary, stdlib only)
# ---------------------------------------------------------------------------

def parse_png(data: bytes) -> dict:
    """Return dict with keys: width, height, bit_depth, color_type, color_type_name."""
    if data[:8] != PNG_SIGNATURE:
        raise ValueError("Not a valid PNG file (bad signature)")
    # IHDR is always the first chunk: 4-byte length, 4-byte type, data, 4-byte CRC
    length = struct.unpack(">I", data[8:12])[0]
    chunk_type = data[12:16]
    if chunk_type != b"IHDR":
        raise ValueError("First PNG chunk is not IHDR")
    ihdr = data[16:16 + length]
    width, height = struct.unpack(">II", ihdr[0:8])
    bit_depth = ihdr[8]
    color_type = ihdr[9]
    return {
        "width": width,
        "height": height,
        "bit_depth": bit_depth,
        "color_type": color_type,
        "color_type_name": PNG_COLOR_TYPES.get(color_type, f"Unknown({color_type})"),
    }


# ---------------------------------------------------------------------------
# JPEG parser (SOF0 / SOF2 markers)
# ---------------------------------------------------------------------------

def parse_jpeg(data: bytes) -> dict:
    """Return dict with keys: width, height."""
    if data[0:2] != b"\xff\xd8":
        raise ValueError("Not a valid JPEG file (bad SOI marker)")
    i = 2
    while i < len(data) - 1:
        if data[i] != 0xff:
            raise ValueError(f"Expected JPEG marker at offset {i}")
        marker = data[i + 1]
        i += 2
        # SOF markers that contain image dimensions
        if marker in (0xC0, 0xC1, 0xC2, 0xC3, 0xC5, 0xC6, 0xC7,
                      0xC9, 0xCA, 0xCB, 0xCD, 0xCE, 0xCF):
            # segment length (2 bytes) + precision (1) + height (2) + width (2)
            seg_length = struct.unpack(">H", data[i:i + 2])[0]
            precision = data[i + 2]  # noqa: F841
            height, width = struct.unpack(">HH", data[i + 3:i + 7])
            return {"width": width, "height": height}
        elif marker in (0xD8, 0xD9):
            # SOI / EOI — no length field
            continue
        else:
            seg_length = struct.unpack(">H", data[i:i + 2])[0]
            i += seg_length
    raise ValueError("Could not find SOF marker in JPEG")


# ---------------------------------------------------------------------------
# Issue helpers
# ---------------------------------------------------------------------------

def issue(severity: str, message: str) -> dict:
    return {"severity": severity, "message": message}


def audit_image(name: str, data: bytes) -> list:
    """Audit a single image file. Returns list of issue dicts."""
    issues = []
    ext = os.path.splitext(name)[1].lower()
    file_size = len(data)

    # File size check
    if file_size > MAX_FILE_BYTES:
        issues.append(issue("WARNING", f"File size {file_size / 1024 / 1024:.1f} MB exceeds 2 MB — excessive for MPC"))

    # Parse image dimensions
    try:
        if ext == ".png":
            info = parse_png(data)
            width, height = info["width"], info["height"]
            color_type = info["color_type"]
            # Color mode check
            if color_type == 0 or color_type == 4:  # Grayscale or Grayscale+Alpha
                issues.append(issue("WARNING", f"Color mode is {info['color_type_name']} — RGB or RGBA recommended"))
            elif color_type == 3:
                issues.append(issue("WARNING", f"Color mode is Indexed — RGB or RGBA recommended"))
        elif ext in (".jpg", ".jpeg"):
            info = parse_jpeg(data)
            width, height = info["width"], info["height"]
        else:
            issues.append(issue("INFO", f"Unrecognized extension '{ext}' — skipping dimension checks"))
            return issues
    except ValueError as e:
        issues.append(issue("ERROR", f"Failed to parse image: {e}"))
        return issues

    # Minimum size check
    if width < MIN_SIZE or height < MIN_SIZE:
        issues.append(issue("ERROR", f"Image is {width}×{height}px — minimum required is {MIN_SIZE}×{MIN_SIZE}px"))
    elif width < WARN_SIZE or height < WARN_SIZE:
        issues.append(issue("WARNING", f"Image is {width}×{height}px — {WARN_SIZE}×{WARN_SIZE}px recommended"))
    else:
        issues.append(issue("INFO", f"Size {width}×{height}px — OK"))

    # Square aspect ratio check (±5px tolerance)
    if abs(width - height) > 5:
        issues.append(issue("ERROR", f"Non-square: {width}×{height}px — cover art must be square"))

    return issues


# ---------------------------------------------------------------------------
# Pack-level audit
# ---------------------------------------------------------------------------

def audit_xpn(xpn_path: str, strict: bool = False) -> dict:
    """
    Audit all cover art images in an .xpn pack.
    Returns a report dict with per-image results and overall pass/fail.
    """
    if not os.path.isfile(xpn_path):
        return {"error": f"File not found: {xpn_path}", "passed": False}

    image_extensions = {".png", ".jpg", ".jpeg"}
    results = []
    sizes_found = set()  # (width, height) tuples for multi-size bonus check
    has_common_cover = False

    try:
        with zipfile.ZipFile(xpn_path, "r") as zf:
            all_names = zf.namelist()
            image_files = [n for n in all_names
                           if os.path.splitext(n)[1].lower() in image_extensions]

            if not image_files:
                return {
                    "images": [],
                    "pack_issues": [issue("ERROR", "No image files found in pack")],
                    "passed": False,
                }

            for name in image_files:
                basename = os.path.basename(name).lower()
                if basename in COMMON_COVER_NAMES:
                    has_common_cover = True

                data = zf.read(name)
                img_issues = audit_image(name, data)

                # Track sizes for multi-size bonus check
                ext = os.path.splitext(name)[1].lower()
                try:
                    if ext == ".png":
                        info = parse_png(data)
                    elif ext in (".jpg", ".jpeg"):
                        info = parse_jpeg(data)
                    else:
                        info = None
                    if info:
                        sizes_found.add((info["width"], info["height"]))
                except ValueError:
                    pass

                results.append({"file": name, "issues": img_issues})

    except zipfile.BadZipFile:
        return {"error": f"Not a valid ZIP/XPN file: {xpn_path}", "passed": False}

    # Pack-level issues
    pack_issues = []

    if not has_common_cover:
        pack_issues.append(issue(
            "WARNING",
            "No standard-named cover art found. Expected one of: " +
            ", ".join(sorted(COMMON_COVER_NAMES))
        ))

    # Multi-size bonus check
    target_sizes = {(200, 200), (400, 400)}
    found_targets = target_sizes & sizes_found
    if len(found_targets) == 2:
        pack_issues.append(issue("INFO", "Both 200×200 and 400×400 variants present — excellent"))
    elif len(found_targets) == 1:
        pack_issues.append(issue("INFO", f"One target size variant found: {found_targets}"))

    # Determine overall pass/fail
    all_issues = pack_issues[:]
    for r in results:
        all_issues.extend(r["issues"])

    has_errors = any(i["severity"] == "ERROR" for i in all_issues)
    has_warnings = any(i["severity"] == "WARNING" for i in all_issues)

    passed = not has_errors
    if strict:
        passed = not has_errors and not has_warnings

    return {
        "xpn": xpn_path,
        "images": results,
        "pack_issues": pack_issues,
        "passed": passed,
        "strict": strict,
    }


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def format_text(report: dict) -> str:
    lines = []
    if "error" in report:
        lines.append(f"ERROR: {report['error']}")
        return "\n".join(lines)

    lines.append(f"XPN Cover Art Audit: {report['xpn']}")
    lines.append("=" * 60)

    for img in report["images"]:
        lines.append(f"\n  [{img['file']}]")
        for iss in img["issues"]:
            lines.append(f"    [{iss['severity']}] {iss['message']}")
        if not img["issues"]:
            lines.append("    [INFO] No issues")

    if report["pack_issues"]:
        lines.append("\n  [Pack-level checks]")
        for iss in report["pack_issues"]:
            lines.append(f"    [{iss['severity']}] {iss['message']}")

    lines.append("\n" + "=" * 60)
    result_str = "PASS" if report["passed"] else "FAIL"
    suffix = " (strict mode)" if report.get("strict") else ""
    lines.append(f"Result: {result_str}{suffix}")
    return "\n".join(lines)


def format_json(report: dict) -> str:
    return json.dumps(report, indent=2)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Audit cover art in a .xpn pack for XO_OX visual standards and MPC display requirements."
    )
    parser.add_argument("--xpn", required=True, help="Path to the .xpn pack file")
    parser.add_argument("--strict", action="store_true",
                        help="Fail on warnings as well as errors")
    parser.add_argument("--format", choices=["text", "json"], default="text",
                        help="Output format (default: text)")
    args = parser.parse_args()

    report = audit_xpn(args.xpn, strict=args.strict)

    if args.format == "json":
        print(format_json(report))
    else:
        print(format_text(report))

    sys.exit(0 if report.get("passed") else 1)


if __name__ == "__main__":
    main()
