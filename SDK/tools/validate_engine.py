#!/usr/bin/env python3
"""
validate_engine.py — XOlokun SDK Engine Validator
=====================================================

Reads an engine .h file and checks compliance with:
  - SynthEngine interface (all virtual methods present)
  - D001-D006 doctrine patterns
  - Parameter naming convention (prefix_ format)
  - Audio-thread safety (no banned patterns)
  - Coupling contract
  - Export macro

Usage:
  python3 SDK/tools/validate_engine.py <path/to/YourEngine.h>
  python3 SDK/tools/validate_engine.py SDK/examples/HelloEngine/HelloEngine.h

Exit code: 0 on full pass, 1 on any FAIL.
"""

import argparse
import sys
import re
import os
from dataclasses import dataclass, field
from typing import List, Optional


# ---------------------------------------------------------------------------
# Result tracking
# ---------------------------------------------------------------------------

@dataclass
class CheckResult:
    category: str
    name: str
    passed: bool
    message: str
    severity: str = "FAIL"   # FAIL | WARN | INFO


class ValidationReport:
    def __init__(self, path: str):
        self.path = path
        self.results: List[CheckResult] = []

    def add(self, category: str, name: str, passed: bool, message: str, severity: str = "FAIL"):
        self.results.append(CheckResult(category, name, passed, message, severity))

    def print_report(self):
        print()
        print(f"  XOlokun Engine Validator")
        print(f"  File: {self.path}")
        print(f"  {'=' * 60}")

        current_cat = None
        for r in self.results:
            if r.category != current_cat:
                current_cat = r.category
                print(f"\n  [{r.category}]")

            tag  = "PASS" if r.passed else r.severity
            icon = "+" if r.passed else ("!" if r.severity == "WARN" else "x")
            print(f"    [{icon}] {tag:<4}  {r.name}")
            if not r.passed:
                print(f"           {r.message}")

        fails  = [r for r in self.results if not r.passed and r.severity == "FAIL"]
        warns  = [r for r in self.results if not r.passed and r.severity == "WARN"]
        passes = [r for r in self.results if r.passed]

        print()
        print(f"  {'=' * 60}")
        print(f"  PASS: {len(passes)}   WARN: {len(warns)}   FAIL: {len(fails)}")
        if not fails:
            print("  RESULT: ENGINE IS VALID -- ready to submit")
        else:
            print(f"  RESULT: ENGINE FAILED -- fix {len(fails)} issue(s) before submitting")
        print()

    def is_passing(self) -> bool:
        return all(r.passed or r.severity != "FAIL" for r in self.results)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def read_file(path: str) -> Optional[str]:
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            return f.read()
    except OSError as e:
        print(f"ERROR: Cannot read file: {e}", file=sys.stderr)
        return None


def has_pattern(text: str, pattern: str, flags: int = re.MULTILINE) -> bool:
    return bool(re.search(pattern, text, flags))


def extract_prefix(text: str) -> Optional[str]:
    """
    Detect the parameter prefix used in this engine.
    Looks for strings like "abc_pitch" or "abc_cutoff" in getParameterDefs or setParameter.
    Returns the prefix string (e.g. "hlo_") or None.
    """
    # Look inside quoted strings that match word_word pattern
    candidates = re.findall(r'"([a-z][a-z0-9]{0,7}_[a-zA-Z][a-zA-Z0-9_]*)"', text)
    if not candidates:
        return None
    # Count prefix occurrences and return the most common one
    prefix_counts: dict = {}
    for c in candidates:
        parts = c.split("_", 1)
        if len(parts) == 2:
            prefix = parts[0] + "_"
            prefix_counts[prefix] = prefix_counts.get(prefix, 0) + 1
    if not prefix_counts:
        return None
    return max(prefix_counts, key=lambda k: prefix_counts[k])


# ---------------------------------------------------------------------------
# Check groups
# ---------------------------------------------------------------------------

def check_interface(text: str, report: ValidationReport):
    """SynthEngine interface compliance."""
    cat = "Interface"

    required_methods = [
        ("prepare",              r'\bvoid\s+prepare\s*\('),
        ("reset",                r'\bvoid\s+reset\s*\(\s*\)'),
        ("renderBlock",          r'\bvoid\s+renderBlock\s*\('),
        ("getParameterDefs",     r'\bgetParameterDefs\s*\('),
        ("setParameter",         r'\bvoid\s+setParameter\s*\('),
        ("getEngineId",          r'\bgetEngineId\s*\('),
        ("getAccentColour",      r'\bgetAccentColour\s*\('),
        ("getMaxVoices",         r'\bgetMaxVoices\s*\('),
    ]

    for name, pattern in required_methods:
        found = has_pattern(text, pattern)
        report.add(cat, f"method: {name}()", found,
                   f"No '{name}()' found. Every SynthEngine must implement this method.")

    # Inherits from SynthEngine
    inherits = has_pattern(text, r':\s*(?:public\s+)?(?:xolokun::)?SynthEngine')
    report.add(cat, "inherits SynthEngine", inherits,
               "Class must inherit from xolokun::SynthEngine (or SynthEngine with namespace using).")

    # Export macro
    has_export = has_pattern(text, r'XOLOKUN_EXPORT_ENGINE\s*\(')
    report.add(cat, "XOLOKUN_EXPORT_ENGINE macro", has_export,
               "Missing XOLOKUN_EXPORT_ENGINE() at file scope. Engines must be dynamically loadable.")

    # getSampleForCoupling (optional but strongly recommended for coupling sends)
    has_coupling_send = has_pattern(text, r'\bgetSampleForCoupling\s*\(')
    report.add(cat, "getSampleForCoupling() [coupling send]", has_coupling_send,
               "Implement getSampleForCoupling() so other engines can use this engine as a modulation source.",
               severity="WARN")

    # applyCouplingInput (optional but strongly recommended)
    has_coupling_recv = has_pattern(text, r'\bapplyCouplingInput\s*\(')
    report.add(cat, "applyCouplingInput() [coupling receive]", has_coupling_recv,
               "Implement applyCouplingInput() to receive modulation from other engines.",
               severity="WARN")

    # releaseResources (optional but expected)
    has_release = has_pattern(text, r'\breleaseResources\s*\(')
    report.add(cat, "releaseResources()", has_release,
               "Implement releaseResources() to free buffers when audio stops.",
               severity="WARN")

    # getActiveVoiceCount (optional)
    has_voice_count = has_pattern(text, r'\bgetActiveVoiceCount\s*\(')
    report.add(cat, "getActiveVoiceCount()", has_voice_count,
               "Implement getActiveVoiceCount() for UI voice indicator support.",
               severity="WARN")


def check_parameter_naming(text: str, report: ValidationReport):
    """Parameter ID convention: all IDs must be 'prefix_name'."""
    cat = "Parameters"

    prefix = extract_prefix(text)
    if prefix is None:
        report.add(cat, "parameter prefix detected", False,
                   'No parameter IDs found. Add parameters via getParameterDefs() '
                   'using the pattern "prefix_paramName" (e.g. "hlo_cutoff").')
        return

    report.add(cat, f"prefix detected: '{prefix}'", True, "")

    # Prefix must be all lowercase, 2-8 chars before underscore
    prefix_name = prefix.rstrip("_")
    valid_prefix = bool(re.match(r'^[a-z][a-z0-9]{1,7}$', prefix_name))
    report.add(cat, "prefix format valid (2-8 lowercase chars)", valid_prefix,
               f"Prefix '{prefix}' must be 2-8 lowercase alphanumeric characters followed by '_'. "
               f"Example: 'hlo_', 'onyx_', 'ort_'.")

    # All parameter IDs in getParameterDefs must use the detected prefix
    # Look for quoted strings inside getParameterDefs block
    param_block_match = re.search(
        r'getParameterDefs\s*\([^)]*\)[^{]*\{(.*?)\}',
        text, re.DOTALL
    )
    if param_block_match:
        param_block = param_block_match.group(1)
        quoted_ids = re.findall(r'"([^"]+)"', param_block)
        # Filter to those that look like param IDs (contain underscore, not display names)
        param_ids = [q for q in quoted_ids if '_' in q and not ' ' in q and len(q) < 40]
        bad_ids = [pid for pid in param_ids if not pid.startswith(prefix)]
        if param_ids:
            report.add(cat, "all param IDs use correct prefix", len(bad_ids) == 0,
                       f"These parameter IDs don't use prefix '{prefix}': {bad_ids}. "
                       f"All parameter IDs must be namespaced: '{prefix}paramName'.")
        else:
            report.add(cat, "parameter IDs found in getParameterDefs", False,
                       "getParameterDefs() body found but no quoted parameter IDs detected inside it.",
                       severity="WARN")

    # setParameter must handle all declared params (heuristic: check it references prefix)
    set_param = re.search(r'setParameter\s*\([^)]*\)[^{]*\{(.*?)\}', text, re.DOTALL)
    if set_param:
        sp_body = set_param.group(1)
        uses_prefix = prefix in sp_body
        report.add(cat, "setParameter() handles prefixed IDs", uses_prefix,
                   f"setParameter() body doesn't reference prefix '{prefix}'. "
                   f"Ensure all parameters are handled (D004 — no dead parameters).")


def check_doctrine_d001(text: str, report: ValidationReport):
    """D001: Velocity Must Shape Timbre."""
    cat = "D001 Velocity→Timbre"

    # Look for velocity being used in filter, cutoff, or timbre-related math
    velocity_patterns = [
        r'velocity\s*\*\s*\d',             # velocity * constant
        r'vel\s*\*\s*\d',                  # vel * constant
        r'getFloatVelocity',               # SDK velocity accessor
        r'velocity.*cutoff|cutoff.*velocity',
        r'vel.*cutoff|cutoff.*vel',
        r'velocity.*filter|filter.*velocity',
        r'velocity.*bright|bright.*velocity',
        r'velocity.*timbre|timbre.*velocity',
    ]

    found_d001 = any(has_pattern(text, p, re.IGNORECASE) for p in velocity_patterns)
    report.add(cat, "velocity shapes timbre (cutoff/brightness/harmonic content)", found_d001,
               "D001: Velocity must drive filter brightness or harmonic content, not just amplitude. "
               "Example: float velBoost = velocity * 4000.0f; filter.tick(cutoff + velBoost, ...);")


def check_doctrine_d002(text: str, report: ValidationReport):
    """D002: Modulation is the Lifeblood (LFO + mod wheel + aftertouch + macros)."""
    cat = "D002 Modulation"

    # LFO
    has_lfo = has_pattern(text, r'\blfo|LFO|oscillat.*rate|phase.*rate\b', re.IGNORECASE)
    report.add(cat, "LFO present", has_lfo,
               "D002: At least one LFO required. Implement a rate-controllable oscillator "
               "modulating pitch, filter, or amplitude.")

    # Mod wheel (CC1)
    has_modwheel = has_pattern(text,
        r'getControllerNumber\(\)\s*==\s*1|controllerNumber\s*==\s*1|CC\s*1\b|mod.wheel', re.IGNORECASE)
    report.add(cat, "mod wheel (CC1) handled", has_modwheel,
               "D002/D006: Handle CC1 (mod wheel) in MIDI parsing. Example: "
               "if (ev.getControllerNumber() == 1) modWheelNorm = ev.getControllerValue() / 127.0f;")

    # Aftertouch
    has_at = has_pattern(text,
        r'isChannelPressure|aftertouch|channel.pressure|getChannelPressure', re.IGNORECASE)
    report.add(cat, "aftertouch handled", has_at,
               "D002/D006: Handle channel pressure (aftertouch) in MIDI parsing. Example: "
               "if (ev.isChannelPressure()) aftertouchNorm = ev.getChannelPressureValue() / 127.0f;")

    # 4 macros — heuristic: at least 4 distinct parameters found
    param_ids = re.findall(r'"([a-z][a-z0-9]{1,7}_[a-zA-Z][a-zA-Z0-9_]*)"', text)
    unique_params = set(param_ids)
    has_four_params = len(unique_params) >= 4
    report.add(cat, f"at least 4 parameters (macros): found {len(unique_params)}", has_four_params,
               "D002: Four working macros required. Each must produce audible change. "
               f"Found {len(unique_params)} unique parameter IDs: {sorted(unique_params)[:8]}")


def check_doctrine_d004(text: str, report: ValidationReport):
    """D004: Dead Parameters Are Broken Promises."""
    cat = "D004 No Dead Params"

    prefix = extract_prefix(text)
    if prefix is None:
        report.add(cat, "parameters detectable", False,
                   "No parameters found — cannot verify D004.", severity="WARN")
        return

    # Extract declared parameter IDs from getParameterDefs
    declared: List[str] = []
    param_block_match = re.search(
        r'getParameterDefs\s*\([^)]*\)[^{]*\{(.*?)\}',
        text, re.DOTALL
    )
    if param_block_match:
        quoted = re.findall(r'"([^"]+)"', param_block_match.group(1))
        declared = [q for q in quoted if q.startswith(prefix) and '_' in q]

    if not declared:
        report.add(cat, "declared parameters found", False,
                   f"No parameter IDs starting with '{prefix}' found in getParameterDefs().",
                   severity="WARN")
        return

    # Check each declared param appears in setParameter() and in some DSP context
    set_param_match = re.search(r'setParameter\s*\([^)]*\)[^{]*\{(.*?)\}', text, re.DOTALL)
    set_body = set_param_match.group(1) if set_param_match else ""

    dead = []
    for pid in declared:
        # Check it appears in setParameter handler
        in_set  = pid in set_body
        # Check it appears anywhere else (in renderBlock, DSP code, etc.)
        # Use a broader check: the param name (without prefix) should appear as a variable
        param_name = pid[len(prefix):]
        # Look for 'param{PascalCase}' or the full id string used in DSP
        in_dsp  = (pid in text and set_body.count(pid) < text.count(pid) - 1)
        if not in_set or not in_dsp:
            dead.append(pid)

    report.add(cat, f"all {len(declared)} declared params handled in setParameter()",
               len(dead) == 0,
               f"D004: These parameters appear in getParameterDefs() but may not be handled in "
               f"setParameter() or DSP: {dead}. Every declared parameter must affect audio output.")


def check_doctrine_d005(text: str, report: ValidationReport):
    """D005: An Engine That Cannot Breathe Is a Photograph (LFO rate floor <= 0.01 Hz)."""
    cat = "D005 LFO Breathing"

    # Check for LFO rate floor at or below 0.01 Hz
    # Common patterns: 0.01f, 0.005f, 0.001f in context of rate
    rate_floor_patterns = [
        r'0\.0[0-9]+f?\s*[+*]',           # 0.0x used in rate computation
        r'[+]\s*0\.0[0-9]+f?\b',          # + 0.0x
        r'rate.*0\.0[0-9]+|0\.0[0-9]+.*rate',
        r'lfo.*0\.0[0-9]|0\.0[0-9].*lfo',
        r'setRate\s*\(\s*0\.0[0-9]',       # StandardLFO::setRate(0.0x)
        r'floor.*0\.0[0-9]|0\.0[0-9].*floor',
    ]

    has_rate_floor = any(has_pattern(text, p, re.IGNORECASE) for p in rate_floor_patterns)
    report.add(cat, "LFO rate floor <= 0.01 Hz (autonomous breathing)", has_rate_floor,
               "D005: LFO minimum rate must be <= 0.01 Hz (one cycle per 100 seconds). "
               "Example: float activeLfoRate = 0.01f + modWheelNorm * 3.99f; "
               "This ensures the engine always breathes autonomously, even at minimum setting.")


def check_doctrine_d006(text: str, report: ValidationReport):
    """D006: Expression Input Is Not Optional (velocity + at least one CC)."""
    cat = "D006 Expression"

    # Velocity used (implied by D001 check, but re-check here)
    has_velocity = has_pattern(text, r'\bvelocity\b|\bvel\b|\bgetFloatVelocity\b')
    report.add(cat, "velocity input used", has_velocity,
               "D006: Velocity must be read from MIDI note-on and used in DSP.")

    # At least one continuous controller (mod wheel already checked in D002, but repeat here)
    has_cc = has_pattern(text,
        r'isController|getControllerNumber|isChannelPressure|isPitchBend|CC', re.IGNORECASE)
    report.add(cat, "at least one MIDI CC handled", has_cc,
               "D006: Handle at least one continuous controller. "
               "Minimum: CC1 (mod wheel) or channel pressure (aftertouch).")


def check_audio_thread_safety(text: str, report: ValidationReport):
    """No allocation, no blocking I/O, no exceptions on the audio thread."""
    cat = "Audio Safety"

    # Check renderBlock body for banned patterns
    render_match = re.search(r'renderBlock\s*\([^)]*\)[^{]*\{', text)
    if not render_match:
        report.add(cat, "renderBlock() found for safety checks", False,
                   "renderBlock() not found — cannot perform audio-thread safety checks.")
        return

    # Extract everything after renderBlock opening brace to end of matching brace
    start = render_match.end()
    depth = 1
    pos = start
    while pos < len(text) and depth > 0:
        if text[pos] == '{': depth += 1
        elif text[pos] == '}': depth -= 1
        pos += 1
    render_body = text[start:pos]

    # Banned: heap allocation
    banned_alloc = [
        (r'\bnew\s+\w',        "operator new (heap allocation)"),
        (r'\bmalloc\s*\(',     "malloc()"),
        (r'\bcalloc\s*\(',     "calloc()"),
        (r'\.push_back\s*\(',  "push_back() (may reallocate)"),
        (r'\.resize\s*\(',     "resize() (may allocate)"),
        (r'\.assign\s*\(',     "assign() (may allocate)"),
    ]
    for pattern, label in banned_alloc:
        found = has_pattern(render_body, pattern)
        report.add(cat, f"no {label} in renderBlock()", not found,
                   f"AUDIO THREAD SAFETY: '{label}' found in renderBlock(). "
                   f"All memory must be pre-allocated in prepare(). Remove this from renderBlock().")

    # Banned: blocking I/O
    banned_io = [
        (r'\bfopen\s*\(|\bfclose\s*\(|\bfread\s*\(|\bfwrite\s*\(', "file I/O (fopen/fclose/fread)"),
        (r'\bprintf\s*\(|\bfprintf\s*\(|\bstd::cout',               "stdio / cout"),
        (r'\bsleep\s*\(|\bSleep\s*\(|\busleep\s*\(',                "sleep()"),
        (r'\bstd::mutex\b|\bstd::lock_guard\b|\bstd::unique_lock\b','mutex locking'),
    ]
    for pattern, label in banned_io:
        found = has_pattern(render_body, pattern)
        report.add(cat, f"no {label} in renderBlock()", not found,
                   f"AUDIO THREAD SAFETY: '{label}' detected in renderBlock(). "
                   f"Blocking operations cause dropouts. Move to a worker thread.",
                   severity="FAIL")

    # Exceptions: not banned but worth warning about throws in audio thread
    has_throw = has_pattern(render_body, r'\bthrow\b')
    report.add(cat, "no throw in renderBlock()", not has_throw,
               "AUDIO THREAD SAFETY: 'throw' found in renderBlock(). "
               "Exceptions may cause undefined behavior in real-time context. Use error flags instead.",
               severity="WARN")


def check_denormal_protection(text: str, report: ValidationReport):
    """Denormal protection required in all feedback/filter paths."""
    cat = "Denormal Protection"

    # Look for flush-denormal pattern or DAZ/FTZ flags
    denormal_patterns = [
        r'flushDenormal|flush_denormal|FLUSH_DENORMAL',
        r'1e-1[0-9]|1e-2[0-9]',          # very small number floor check
        r'fesetenv|_MM_SET_FLUSH_ZERO',   # FTZ/DAZ MXCSR
        r'std::fpclassify|isnormal\(',
        r'denorm|denormal',
    ]
    has_protection = any(has_pattern(text, p, re.IGNORECASE) for p in denormal_patterns)
    report.add(cat, "denormal protection present", has_protection,
               "ARCHITECTURE RULE: Denormal protection required in feedback/filter paths. "
               "Add: inline float flushDenormal(float x) { return (std::abs(x) < 1e-18f) ? 0.f : x; } "
               "and call it on filter state variables: state = flushDenormal((1-c)*in + c*state);",
               severity="WARN")


def check_silence_gate(text: str, report: ValidationReport):
    """SRO silence gate: zero-idle bypass."""
    cat = "SilenceGate (SRO)"

    # JUCE-native path
    has_juce_gate = has_pattern(text, r'silenceGate|SilenceGate|prepareSilenceGate|isSilenceGateBypassed')

    # SDK-native path (isSilent flag or similar)
    has_sdk_gate = has_pattern(text,
        r'isSilent|silenceHold|silenceBypass|silence_gate|bypass.*silence|silence.*bypass',
        re.IGNORECASE)

    has_gate = has_juce_gate or has_sdk_gate
    report.add(cat, "silence gate / idle bypass present", has_gate,
               "SRO: Implement a silence gate to avoid wasting CPU when engine is idle. "
               "JUCE engines: use silenceGate from SynthEngine base. "
               "SDK engines: track isSilent + silenceHold counter, clear buffer and return early.",
               severity="WARN")

    # Note-on wake
    if has_gate:
        has_wake = has_pattern(text,
            r'silenceGate\.wake|wakeSilenceGate|isSilent\s*=\s*false|isSilent\s*=\s*0',
            re.IGNORECASE)
        report.add(cat, "silence gate woken on note-on", has_wake,
                   "SRO: The silence gate must be woken on note-on BEFORE checking bypass. "
                   "Pattern: on note-on { isSilent = false; silenceHold = 0; }",
                   severity="WARN")


def check_coupling_contract(text: str, report: ValidationReport):
    """Coupling: getSampleForCoupling must be O(1)."""
    cat = "Coupling Contract"

    # Check getSampleForCoupling doesn't do heavy computation
    gsc_match = re.search(r'getSampleForCoupling\s*\([^)]*\)[^{]*\{(.*?)\}', text, re.DOTALL)
    if gsc_match:
        gsc_body = gsc_match.group(1)
        # Look for loops, heavy math, allocations — these would violate O(1)
        has_loop = has_pattern(gsc_body, r'\bfor\s*\(|\bwhile\s*\(')
        has_alloc = has_pattern(gsc_body, r'\bnew\b|\bmalloc\b')
        is_o1 = not has_loop and not has_alloc
        report.add(cat, "getSampleForCoupling() is O(1) (no loops/allocs)", is_o1,
                   "COUPLING CONTRACT: getSampleForCoupling() must return a cached value in O(1). "
                   "No loops, no allocations. Pre-compute and cache in renderBlock(), return here.",
                   severity="FAIL")

    # applyCouplingInput: should not write to buffer directly
    aci_match = re.search(r'applyCouplingInput\s*\([^)]*\)[^{]*\{(.*?)\}', text, re.DOTALL)
    if aci_match:
        aci_body = aci_match.group(1)
        # Check it accumulates rather than immediately applying (common correct pattern)
        has_accum = has_pattern(aci_body, r'[+]=|accum|Accum|Mod\s*[+]=|mod\s*[+]=|bump\s*[+]=')
        if has_pattern(text, r'applyCouplingInput'):
            report.add(cat, "applyCouplingInput() accumulates (not immediate write)", has_accum,
                       "COUPLING CONTRACT: applyCouplingInput() should accumulate modulation "
                       "(e.g. couplingFilterMod += rms * amount) for renderBlock() to consume. "
                       "Immediate audio writes here can cause thread-safety issues.",
                       severity="WARN")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def validate(path: str) -> bool:
    text = read_file(path)
    if text is None:
        return False

    report = ValidationReport(path)

    check_interface(text, report)
    check_parameter_naming(text, report)
    check_doctrine_d001(text, report)
    check_doctrine_d002(text, report)
    check_doctrine_d004(text, report)
    check_doctrine_d005(text, report)
    check_doctrine_d006(text, report)
    check_audio_thread_safety(text, report)
    check_denormal_protection(text, report)
    check_silence_gate(text, report)
    check_coupling_contract(text, report)

    report.print_report()
    return report.is_passing()


def main():
    parser = argparse.ArgumentParser(
        description="Validate an XOlokun engine header",
        epilog="Example: python3 validate_engine.py SDK/examples/HelloEngine/HelloEngine.h",
    )
    parser.add_argument("file", help="Path to the engine .h file")
    args = parser.parse_args()

    path = args.file
    if not os.path.isfile(path):
        print(f"ERROR: File not found: {path}", file=sys.stderr)
        sys.exit(1)

    passed = validate(path)
    sys.exit(0 if passed else 1)


if __name__ == "__main__":
    main()
