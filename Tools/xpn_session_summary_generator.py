"""
xpn_session_summary_generator.py — XPN Tool Suite Comprehensive Session Summary Generator

Scans the Tools/ directory for all Python tools and Docs/specs/ for spec files,
then generates a comprehensive markdown summary report covering tool categories,
purposes, CLI usage patterns, and R&D spec inventory. An evolution of
xpn_session_handoff.py — intended to orient a new team member or a fresh session
in under 10 minutes.

Usage:
    python xpn_session_summary_generator.py [--tools-dir ./Tools] [--specs-dir ./Docs/specs] [--output session_summary.md] [--format markdown|text]

Options:
    --tools-dir   Directory to scan for .py tool files (default: ./Tools relative to script)
    --specs-dir   Directory to scan for spec .md files (default: ./Docs/specs relative to repo root)
    --output      Output path for the summary (default: session_summary_YYYY-MM-DD.md in specs-dir)
    --format      Output format: markdown or text (default: markdown)
"""

import argparse
import ast
import re
import sys
from datetime import date, datetime
from pathlib import Path


# ---------------------------------------------------------------------------
# Category definitions — (label, list_of_keyword_patterns)
# Patterns are matched against the lowercased filename stem.
# ---------------------------------------------------------------------------
CATEGORIES = [
    ("QA / Validation",    ["validator", "checker", "audit", "lint", "qa", "integrity"]),
    ("Build / Export",     ["export", "build", "bundle", "packager", "render"]),
    ("Analysis",           ["analyzer", "report", "coverage", "dashboard", "analytics", "score", "fingerprint"]),
    ("Editing / Fixing",   ["fixer", "editor", "normalizer", "renamer", "bumper", "trim", "migrate", "apply", "add_missing", "fix_"]),
    ("Generation",         ["generator", "builder", "designer", "creator", "breed", "generate", "maker"]),
    ("Search / Browse",    ["search", "browser", "browser", "find", "classify", "detect"]),
]

CORE_PIPELINE = {
    "oxport.py",
    "xpn_drum_export.py",
    "xpn_keygroup_export.py",
    "xpn_kit_expander.py",
    "xpn_bundle_builder.py",
    "xpn_cover_art.py",
    "xpn_packager.py",
    "xpn_sample_categorizer.py",
    "xpn_render_spec.py",
    "xpn_validator.py",
    "xpn_manifest_generator.py",
}


# ---------------------------------------------------------------------------
# Extraction helpers
# ---------------------------------------------------------------------------

def extract_module_docstring(filepath: Path) -> str:
    """Return the module-level docstring from a Python file, or empty string."""
    try:
        source = filepath.read_text(encoding="utf-8", errors="replace")
        tree = ast.parse(source)
        return ast.get_docstring(tree) or ""
    except SyntaxError:
        source = filepath.read_text(encoding="utf-8", errors="replace")
        m = re.match(r'^\s*(?:"""(.*?)"""|\'\'\'(.*?)\'\'\')', source, re.DOTALL)
        if m:
            return (m.group(1) or m.group(2)).strip()
        return ""


def first_docstring_line(docstring: str) -> str:
    """Return the first non-empty line of a docstring as the purpose summary."""
    for line in docstring.splitlines():
        stripped = line.strip()
        if stripped:
            return stripped
    return ""


def extract_synopsis(docstring: str, filepath: Path) -> str:
    """
    Extract a CLI usage pattern from a docstring or argparse source.
    Looks for 'Usage:' section or 'python <name>.py' lines.
    Falls back to argparse prog= scan. Returns '-' if nothing found.
    """
    if docstring:
        lines = docstring.splitlines()
        for i, line in enumerate(lines):
            stripped = line.strip()
            if re.match(r'(?i)usage\s*:', stripped):
                rest = re.sub(r'(?i)usage\s*:\s*', '', stripped).strip()
                if rest:
                    return rest
                for j in range(i + 1, min(i + 4, len(lines))):
                    candidate = lines[j].strip()
                    if candidate:
                        return candidate
            if re.match(r'python\s+\S+\.py', stripped, re.IGNORECASE):
                return stripped

    try:
        source = filepath.read_text(encoding="utf-8", errors="replace")
        m = re.search(r'ArgumentParser\([^)]*prog\s*=\s*["\']([^"\']+)["\']', source)
        if m:
            return m.group(1)
    except OSError:
        pass

    return f"python {filepath.name}"


def count_lines(filepath: Path) -> int:
    """Count total lines in a file."""
    try:
        return sum(1 for _ in filepath.open(encoding="utf-8", errors="replace"))
    except OSError:
        return 0


def extract_spec_first_content_line(filepath: Path) -> str:
    """Return the first non-empty, non-heading line of a markdown file."""
    try:
        skip_next = False
        for line in filepath.open(encoding="utf-8", errors="replace"):
            stripped = line.strip()
            if not stripped:
                continue
            # Skip date-like lines (e.g. "2026-03-14" or "Date: 2026-03-14")
            if re.match(r'^(?:date\s*:\s*)?\d{4}-\d{2}-\d{2}', stripped, re.IGNORECASE):
                continue
            # Skip pure heading lines
            if stripped.startswith("#"):
                continue
            return stripped[:120]
    except OSError:
        pass
    return ""


def extract_spec_title(filepath: Path) -> str:
    """Return the first # heading from a markdown file, or the filename stem."""
    try:
        for line in filepath.open(encoding="utf-8", errors="replace"):
            stripped = line.strip()
            if stripped.startswith("# "):
                return stripped[2:].strip()
    except OSError:
        pass
    return filepath.stem


# ---------------------------------------------------------------------------
# Categorisation
# ---------------------------------------------------------------------------

def categorise_tool(filename: str) -> str:
    """Return the category label for a tool filename."""
    if filename in CORE_PIPELINE:
        return "Core Pipeline"
    stem = filename.lower().replace("-", "_")
    for label, patterns in CATEGORIES:
        for pat in patterns:
            if pat in stem:
                return label
    return "Utility / Misc"


# ---------------------------------------------------------------------------
# Scanning
# ---------------------------------------------------------------------------

def scan_tools(tools_dir: Path) -> list[dict]:
    """Scan tools_dir for all .py files and return enriched dicts."""
    tools = []
    for py_file in sorted(tools_dir.glob("*.py")):
        docstring = extract_module_docstring(py_file)
        purpose = first_docstring_line(docstring)
        # Strip leading filename prefix that many tools include in their first line
        if purpose and "—" in purpose:
            purpose = purpose.split("—", 1)[1].strip()
        elif purpose and "-" in purpose[:60]:
            # Handle "foo.py - description" format
            m = re.match(r'^\S+\.py\s*[-–]\s*(.*)', purpose)
            if m:
                purpose = m.group(1).strip()
        synopsis = extract_synopsis(docstring, py_file)
        lines = count_lines(py_file)
        category = categorise_tool(py_file.name)
        tools.append({
            "name": py_file.name,
            "category": category,
            "purpose": purpose[:100] if purpose else "(no docstring)",
            "synopsis": synopsis[:100] if len(synopsis) > 100 else synopsis,
            "lines": lines,
        })
    return tools


def scan_specs(specs_dir: Path) -> list[dict]:
    """Scan specs_dir for all .md files and return enriched dicts."""
    specs = []
    for md_file in sorted(specs_dir.glob("*.md")):
        title = extract_spec_title(md_file)
        blurb = extract_spec_first_content_line(md_file)
        specs.append({
            "filename": md_file.name,
            "title": title,
            "blurb": blurb[:100] if blurb else "",
        })
    return specs


# ---------------------------------------------------------------------------
# Rendering helpers
# ---------------------------------------------------------------------------

def md_table(headers: list[str], rows: list[list[str]]) -> str:
    """Render a plain markdown table."""
    col_widths = [len(h) for h in headers]
    for row in rows:
        for i, cell in enumerate(row):
            col_widths[i] = max(col_widths[i], len(str(cell)))

    def fmt_row(cells):
        return "| " + " | ".join(str(c).ljust(col_widths[i]) for i, c in enumerate(cells)) + " |"

    sep = "| " + " | ".join("-" * w for w in col_widths) + " |"
    return "\n".join([fmt_row(headers), sep] + [fmt_row(r) for r in rows])


def group_by_category(tools: list[dict]) -> dict[str, list[dict]]:
    """Return tools grouped by category, preserving a sensible display order."""
    order = [
        "Core Pipeline",
        "QA / Validation",
        "Build / Export",
        "Generation",
        "Editing / Fixing",
        "Analysis",
        "Search / Browse",
        "Utility / Misc",
    ]
    groups: dict[str, list[dict]] = {label: [] for label in order}
    for tool in tools:
        cat = tool["category"]
        if cat not in groups:
            groups[cat] = []
        groups[cat].append(tool)
    return groups


# ---------------------------------------------------------------------------
# Report generation
# ---------------------------------------------------------------------------

def generate_summary(
    tools: list[dict],
    specs: list[dict],
    output_path: Path,
    fmt: str = "markdown",
) -> None:
    today = date.today().isoformat()
    total_lines = sum(t["lines"] for t in tools)
    groups = group_by_category(tools)

    lines_out: list[str] = []

    def h(level: int, text: str):
        lines_out.append(f"{'#' * level} {text}\n")

    def para(text: str):
        lines_out.append(text + "\n")

    def blank():
        lines_out.append("")

    # ---- Header ----
    h(1, "XPN Tool Suite — Comprehensive Session Summary")
    para(f"**Generated**: {today}  ")
    para(f"**Purpose**: Orient any team member or new Claude session to the full XPN toolkit in under 10 minutes.")
    blank()

    # ---- Stats ----
    h(2, "Suite Stats")
    stats_rows = [
        ["Total Python tools", str(len(tools))],
        ["Total lines of tool code", f"{total_lines:,}"],
        ["Total spec files (Docs/specs/)", str(len(specs))],
        ["Date generated", today],
    ]
    para(md_table(["Metric", "Value"], stats_rows))
    blank()

    # ---- Category breakdown ----
    h(2, "Tool Categories")
    cat_rows = []
    for cat, members in groups.items():
        if members:
            cat_rows.append([cat, str(len(members)), ", ".join(t["name"] for t in members[:4]) + (" …" if len(members) > 4 else "")])
    para(md_table(["Category", "Count", "Examples"], cat_rows))
    blank()

    # ---- Per-category tool listings ----
    h(2, "Tool Listing by Category")
    para("Each entry shows: **tool name**, purpose summary, and CLI usage pattern.")
    blank()

    for cat in [
        "Core Pipeline",
        "QA / Validation",
        "Build / Export",
        "Generation",
        "Editing / Fixing",
        "Analysis",
        "Search / Browse",
        "Utility / Misc",
    ]:
        members = groups.get(cat, [])
        if not members:
            continue
        h(3, f"{cat} ({len(members)} tools)")
        tool_rows = [
            [t["name"], t["purpose"], t["synopsis"]]
            for t in members
        ]
        para(md_table(["Tool", "Purpose", "CLI Usage"], tool_rows))
        blank()

    # ---- Spec listing ----
    h(2, f"R&D Spec Inventory ({len(specs)} files)")
    para("All files in `Docs/specs/`. Title extracted from first `#` heading.")
    blank()
    if specs:
        spec_rows = [[s["filename"], s["title"]] for s in specs]
        para(md_table(["File", "Title"], spec_rows))
    else:
        para("_No spec files found._")
    blank()

    # ---- Footer note ----
    h(2, "How to Use This Document")
    para("- **New to the toolkit?** Start with Core Pipeline — `oxport.py` is the main entry point.")
    para("- **Need to validate output?** Check QA / Validation tools.")
    para("- **Building a kit?** Start with Build / Export then run a QA pass.")
    para("- **R&D specs**: each `*_rnd.md` file is a focused research document on one topic.")
    para("- **Re-generate this file**: `python Tools/xpn_session_summary_generator.py`")
    blank()

    content = "\n".join(lines_out)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(content, encoding="utf-8")

    print(f"Summary written to: {output_path}")
    print(f"  {len(tools)} tools across {len([c for c, m in groups.items() if m])} categories | {total_lines:,} lines of code")
    print(f"  {len(specs)} spec files")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Generate a comprehensive XPN tool suite session summary."
    )
    parser.add_argument("--tools-dir", default=None, help="Directory containing .py tool files (default: script's own directory)")
    parser.add_argument("--specs-dir", default=None, help="Directory containing spec .md files (default: ../Docs/specs relative to script)")
    parser.add_argument("--output", default=None, help="Output path (default: session_summary_YYYY-MM-DD.md in specs-dir)")
    parser.add_argument("--format", choices=["markdown", "text"], default="markdown", help="Output format (default: markdown)")
    args = parser.parse_args()

    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent

    tools_dir = Path(args.tools_dir) if args.tools_dir else script_dir
    specs_dir = Path(args.specs_dir) if args.specs_dir else repo_root / "Docs" / "specs"

    today = date.today().isoformat()
    default_output = specs_dir / f"session_summary_{today}.md"
    output_path = Path(args.output) if args.output else default_output

    if not tools_dir.is_dir():
        print(f"Error: --tools-dir not found: {tools_dir}", file=sys.stderr)
        sys.exit(1)
    if not specs_dir.is_dir():
        print(f"Error: --specs-dir not found: {specs_dir}", file=sys.stderr)
        sys.exit(1)

    tools = scan_tools(tools_dir)
    specs = scan_specs(specs_dir)
    generate_summary(tools, specs, output_path, fmt=args.format)


if __name__ == "__main__":
    main()
