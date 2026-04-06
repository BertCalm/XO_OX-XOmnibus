"""
xpn_session_handoff.py — XPN Tool Suite Session Handoff Generator

Scans the Tools/ directory for xpn_*.py files and Docs/specs/ for *_rnd.md
files, then generates a markdown session handoff document summarizing the
current state of the XPN tool suite.

Usage:
    python xpn_session_handoff.py [--output PATH] [--tools-dir DIR] [--specs-dir DIR]

Options:
    --output     Output path for the handoff markdown (default: Docs/specs/session_handoff_YYYY-MM-DD.md)
    --tools-dir  Directory to scan for xpn_*.py files (default: ./Tools)
    --specs-dir  Directory to scan for *_rnd.md files (default: ./Docs/specs)
"""

import argparse
import ast
import os
import re
import sys
from datetime import date, datetime
from pathlib import Path


def extract_module_docstring(filepath: Path) -> str:
    """Return the module-level docstring from a Python file, or empty string."""
    try:
        source = filepath.read_text(encoding="utf-8", errors="replace")
        tree = ast.parse(source)
        docstring = ast.get_docstring(tree)
        return docstring or ""
    except SyntaxError:
        # Fall back to regex for files with syntax errors
        source = filepath.read_text(encoding="utf-8", errors="replace")
        m = re.match(r'^\s*(?:"""(.*?)"""|\'\'\'(.*?)\'\'\')', source, re.DOTALL)
        if m:
            return (m.group(1) or m.group(2)).strip()
        return ""


def extract_synopsis(docstring: str, filepath: Path) -> str:
    """
    Extract a short CLI synopsis from a docstring.
    Looks for a line containing 'usage:' (case-insensitive) or 'python <name>'.
    Falls back to scanning argparse prog= in the source.
    Returns a short single-line synopsis or '-'.
    """
    if docstring:
        lines = docstring.splitlines()
        for i, line in enumerate(lines):
            stripped = line.strip()
            if re.match(r'(?i)usage\s*:', stripped):
                # Return remainder of this line, or next non-empty line
                rest = re.sub(r'(?i)usage\s*:\s*', '', stripped).strip()
                if rest:
                    return rest
                # Look ahead
                for j in range(i + 1, min(i + 4, len(lines))):
                    candidate = lines[j].strip()
                    if candidate:
                        return candidate
            if re.match(r'python\s+\S+\.py', stripped, re.IGNORECASE):
                return stripped

    # Fallback: scan source for argparse prog=
    try:
        source = filepath.read_text(encoding="utf-8", errors="replace")
        m = re.search(r'ArgumentParser\([^)]*prog\s*=\s*["\']([^"\']+)["\']', source)
        if m:
            return m.group(1)
    except OSError as exc:
        print(f"[WARN] Reading script source for argparse prog name from {filepath.name}: {exc}", file=sys.stderr)

    return "-"


def extract_spec_title(filepath: Path) -> str:
    """Return the first # heading from a markdown file, or the filename stem."""
    try:
        for line in filepath.open(encoding="utf-8", errors="replace"):
            line = line.strip()
            if line.startswith("# "):
                return line[2:].strip()
    except OSError as exc:
        print(f"[WARN] Reading spec file {filepath.name} for title: {exc}", file=sys.stderr)
    return filepath.stem


def count_lines(filepath: Path) -> int:
    """Count non-empty lines in a file."""
    try:
        return sum(1 for _ in filepath.open(encoding="utf-8", errors="replace"))
    except OSError:
        return 0


def get_mtime(filepath: Path) -> str:
    """Return last-modified date as YYYY-MM-DD."""
    try:
        ts = filepath.stat().st_mtime
        return datetime.fromtimestamp(ts).strftime("%Y-%m-%d")
    except OSError:
        return "unknown"


def scan_tools(tools_dir: Path) -> list[dict]:
    """Scan tools_dir for xpn_*.py files and return list of tool info dicts."""
    tools = []
    for py_file in sorted(tools_dir.glob("xpn_*.py")):
        docstring = extract_module_docstring(py_file)
        synopsis = extract_synopsis(docstring, py_file)
        lines = count_lines(py_file)
        mtime = get_mtime(py_file)
        # Shorten synopsis for table readability
        if len(synopsis) > 80:
            synopsis = synopsis[:77] + "..."
        tools.append({
            "name": py_file.name,
            "lines": lines,
            "synopsis": synopsis,
            "mtime": mtime,
        })
    return tools


def scan_specs(specs_dir: Path) -> list[dict]:
    """Scan specs_dir for *_rnd.md files and return list of spec info dicts."""
    specs = []
    for md_file in sorted(specs_dir.glob("*_rnd.md")):
        title = extract_spec_title(md_file)
        lines = count_lines(md_file)
        specs.append({
            "filename": md_file.name,
            "title": title,
            "lines": lines,
        })
    return specs


def md_table(headers: list[str], rows: list[list[str]]) -> str:
    """Render a simple markdown table."""
    col_widths = [len(h) for h in headers]
    for row in rows:
        for i, cell in enumerate(row):
            col_widths[i] = max(col_widths[i], len(str(cell)))

    def fmt_row(cells):
        return "| " + " | ".join(str(c).ljust(col_widths[i]) for i, c in enumerate(cells)) + " |"

    sep = "| " + " | ".join("-" * w for w in col_widths) + " |"
    lines = [fmt_row(headers), sep] + [fmt_row(r) for r in rows]
    return "\n".join(lines)


def generate_handoff(tools: list[dict], specs: list[dict], output_path: Path) -> None:
    today = date.today().isoformat()
    total_tool_lines = sum(t["lines"] for t in tools)
    total_spec_lines = sum(s["lines"] for s in specs)

    tool_rows = [[t["name"], str(t["lines"]), t["synopsis"], t["mtime"]] for t in tools]
    spec_rows = [[s["filename"], s["title"], str(s["lines"])] for s in specs]

    sections = []

    sections.append(f"# XPN Tool Suite — Session Handoff\n\n**Date**: {today}\n")

    sections.append("## Summary Stats\n")
    sections.append(
        f"| Metric | Value |\n"
        f"| --- | --- |\n"
        f"| Total xpn_*.py tools | {len(tools)} |\n"
        f"| Total lines of tool code | {total_tool_lines:,} |\n"
        f"| Total R&D specs (*_rnd.md) | {len(specs)} |\n"
        f"| Total spec lines | {total_spec_lines:,} |\n"
    )

    sections.append("## Tool Catalog\n")
    if tools:
        sections.append(md_table(["Tool", "Lines", "Synopsis", "Last Modified"], tool_rows))
    else:
        sections.append("_No xpn_*.py tools found._")
    sections.append("")

    sections.append("## R&D Spec Catalog\n")
    if specs:
        sections.append(md_table(["Spec File", "Title", "Lines"], spec_rows))
    else:
        sections.append("_No *_rnd.md specs found._")
    sections.append("")

    sections.append("## Next Steps\n")
    sections.append("_Fill in before handing off to next session._\n")
    sections.append("- [ ] \n")

    content = "\n".join(sections)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(content, encoding="utf-8")
    print(f"Handoff written to: {output_path}")
    print(f"  {len(tools)} tools | {total_tool_lines:,} lines of code")
    print(f"  {len(specs)} R&D specs | {total_spec_lines:,} spec lines")


def main():
    parser = argparse.ArgumentParser(
        description="Generate an XPN tool suite session handoff markdown document."
    )
    parser.add_argument("--tools-dir", default=None, help="Directory containing xpn_*.py files")
    parser.add_argument("--specs-dir", default=None, help="Directory containing *_rnd.md specs")
    parser.add_argument("--output", default=None, help="Output path for the handoff markdown")
    args = parser.parse_args()

    # Resolve defaults relative to this script's location
    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent

    tools_dir = Path(args.tools_dir) if args.tools_dir else script_dir
    specs_dir = Path(args.specs_dir) if args.specs_dir else repo_root / "Docs" / "specs"

    today = date.today().isoformat()
    default_output = specs_dir / f"session_handoff_{today}.md"
    output_path = Path(args.output) if args.output else default_output

    if not tools_dir.is_dir():
        print(f"Error: tools-dir not found: {tools_dir}", file=sys.stderr)
        sys.exit(1)
    if not specs_dir.is_dir():
        print(f"Error: specs-dir not found: {specs_dir}", file=sys.stderr)
        sys.exit(1)

    tools = scan_tools(tools_dir)
    specs = scan_specs(specs_dir)
    generate_handoff(tools, specs, output_path)


if __name__ == "__main__":
    main()
