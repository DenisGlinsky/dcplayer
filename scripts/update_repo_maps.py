#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXCLUDED_NAMES = {
    "build",
    "__pycache__",
    ".pytest_cache",
    "__MACOSX",
    ".git",
    ".idea",
    ".vscode",
    ".venv",
    ".cache",
}
EXCLUDED_SUFFIXES = {".pyc"}
EXCLUDED_FILE_NAMES = {".DS_Store"}


def include(path: Path) -> bool:
    if any(part in EXCLUDED_NAMES for part in path.parts):
        return False
    if path.name in EXCLUDED_FILE_NAMES:
        return False
    if path.suffix in EXCLUDED_SUFFIXES:
        return False
    if path.name.startswith("._"):
        return False
    return True


def tree_lines(path: Path, prefix: str = "") -> list[str]:
    entries = [p for p in sorted(path.iterdir(), key=lambda p: (not p.is_dir(), p.name.lower())) if include(p)]
    lines: list[str] = []
    for index, entry in enumerate(entries):
        connector = "└── " if index == len(entries) - 1 else "├── "
        suffix = "/" if entry.is_dir() else ""
        lines.append(f"{prefix}{connector}{entry.name}{suffix}")
        if entry.is_dir():
            extension = "    " if index == len(entries) - 1 else "│   "
            lines.extend(tree_lines(entry, prefix + extension))
    return lines


task_count = len(list((ROOT / "tasks").glob("T*/AGENTS.md")))
spec_count = len([p for p in (ROOT / "specs/objects").glob("*.md") if p.name != "README.md"])

manifest = {
    "plan": "docs/PLAN.md",
    "status": "docs/STATUS.md",
    "index": "tasks/INDEX.md",
    "tasks_count": task_count,
    "specs_baseline_count": spec_count,
    "generated_by": "scripts/update_repo_maps.py"
}
(ROOT / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

lines = [ROOT.name] + tree_lines(ROOT)
(ROOT / "TREE.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
