#!/usr/bin/env python3
"""Scan .cursor rules and skills for context-bloat signals. No network, no deps."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


def repo_root_from_script() -> Path:
    # .../.cursor/skills/context-health/scripts/audit_context.py -> repo root
    return Path(__file__).resolve().parents[4]


def parse_mdc_frontmatter(text: str) -> dict:
    meta: dict = {}
    if not text.startswith("---"):
        return meta
    end = text.find("---", 3)
    if end == -1:
        return meta
    block = text[3:end].strip()
    for line in block.splitlines():
        if ":" not in line:
            continue
        key, _, val = line.partition(":")
        key = key.strip()
        val = val.strip().strip('"').strip("'")
        if key == "alwaysApply":
            meta[key] = val.lower() == "true"
        elif key == "globs":
            meta[key] = val
        else:
            meta[key] = val
    return meta


def parse_skill_frontmatter(text: str) -> dict:
    meta: dict = {}
    if not text.startswith("---"):
        return meta
    end = text.find("---", 3)
    if end == -1:
        return meta
    block = text[3:end]
    # multiline YAML description with >
    name_m = re.search(r"^name:\s*(.+)$", block, re.M)
    if name_m:
        meta["name"] = name_m.group(1).strip().strip('"')
    desc_m = re.search(r"^description:\s*(?:>\s*)?\n((?:\s+.+\n?)+)", block, re.M)
    if desc_m:
        meta["description"] = " ".join(
            ln.strip() for ln in desc_m.group(1).splitlines()
        )
    else:
        desc_one = re.search(r"^description:\s*(.+)$", block, re.M)
        if desc_one:
            meta["description"] = desc_one.group(1).strip().strip('"')
    disable = re.search(r"^disable-model-invocation:\s*(.+)$", block, re.M)
    if disable:
        meta["disable_model_invocation"] = disable.group(1).strip().lower() == "true"
    return meta


def est_tokens(chars: int) -> int:
    return max(1, chars // 4)


def scan_rules(cursor_dir: Path) -> list[dict]:
    rows = []
    rules_dir = cursor_dir / "rules"
    if not rules_dir.is_dir():
        return rows
    for path in sorted(rules_dir.glob("*.mdc")):
        text = path.read_text(encoding="utf-8", errors="replace")
        meta = parse_mdc_frontmatter(text)
        body = text.split("---", 2)[-1] if text.startswith("---") else text
        rows.append(
            {
                "path": str(path.relative_to(cursor_dir.parent)),
                "always_apply": meta.get("alwaysApply", False),
                "globs": meta.get("globs"),
                "lines": text.count("\n") + 1,
                "chars": len(text),
                "est_tokens": est_tokens(len(text)),
                "body_chars": len(body),
            }
        )
    return rows


def scan_skills(cursor_dir: Path) -> list[dict]:
    rows = []
    skills_dir = cursor_dir / "skills"
    if not skills_dir.is_dir():
        return rows
    for path in sorted(skills_dir.glob("*/SKILL.md")):
        text = path.read_text(encoding="utf-8", errors="replace")
        meta = parse_skill_frontmatter(text)
        body_start = text.find("---", 3)
        body = text[body_start + 3 :].lstrip("-").lstrip() if body_start != -1 else text
        if body.startswith("---"):
            body = body.split("---", 1)[-1]
        desc = meta.get("description", "")
        rows.append(
            {
                "path": str(path.relative_to(cursor_dir.parent)),
                "name": meta.get("name", path.parent.name),
                "description_chars": len(desc),
                "description_est_tokens": est_tokens(len(desc)),
                "body_lines": body.count("\n") + 1,
                "body_chars": len(body),
                "body_est_tokens": est_tokens(len(body)),
                "disable_model_invocation": meta.get("disable_model_invocation", False),
                "flags": [],
            }
        )
        if len(body.splitlines()) > 500:
            rows[-1]["flags"].append("body>500_lines")
        if len(desc) > 400:
            rows[-1]["flags"].append("description>400_chars")
    return rows


def find_duplicate_headings(cursor_dir: Path) -> list[dict]:
    heading_re = re.compile(r"^#{1,3}\s+(.+)$", re.M)
    seen: dict[str, list[str]] = {}
    for path in list((cursor_dir / "rules").glob("*.mdc")) + list(
        (cursor_dir / "skills").glob("*/SKILL.md")
    ):
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        rel = str(path.relative_to(cursor_dir.parent))
        for m in heading_re.finditer(text):
            title = m.group(1).strip().lower()
            if len(title) < 8:
                continue
            seen.setdefault(title, []).append(rel)
    return [
        {"heading": h, "files": files}
        for h, files in sorted(seen.items())
        if len(files) > 1
    ][:15]


def summarize(rules: list[dict], skills: list[dict], dupes: list[dict]) -> dict:
    always = [r for r in rules if r["always_apply"]]
    always_tokens = sum(r["est_tokens"] for r in always)
    skill_meta_tokens = sum(s["description_est_tokens"] for s in skills)
    large_bodies = [s for s in skills if s["body_lines"] > 200]
    warnings = []
    if always_tokens > 4000:
        warnings.append(
            f"alwaysApply rules ~{always_tokens} est. input tokens/turn (high)"
        )
    if skill_meta_tokens > 1500:
        warnings.append(
            f"skill descriptions ~{skill_meta_tokens} est. tokens in skill list (high)"
        )
    if len(large_bodies) >= 3:
        warnings.append(f"{len(large_bodies)} skills with body >200 lines")
    if dupes:
        warnings.append(f"{len(dupes)} duplicate headings across files")
    return {
        "always_apply_rule_count": len(always),
        "always_apply_est_tokens": always_tokens,
        "skill_count": len(skills),
        "skill_metadata_est_tokens": skill_meta_tokens,
        "warnings": warnings,
    }


def render_markdown(summary: dict, rules: list[dict], skills: list[dict], dupes: list[dict]) -> str:
    lines = ["# Context health audit", ""]
    if summary["warnings"]:
        lines.append("## Warnings")
        for w in summary["warnings"]:
            lines.append(f"- {w}")
        lines.append("")
    lines.extend(
        [
            "## Summary",
            f"- alwaysApply rules: {summary['always_apply_rule_count']} "
            f"(~{summary['always_apply_est_tokens']} est. tokens/turn)",
            f"- skills: {summary['skill_count']} "
            f"(metadata ~{summary['skill_metadata_est_tokens']} est. tokens)",
            "",
            "## Rules (by est. tokens)",
        ]
    )
    for r in sorted(rules, key=lambda x: -x["est_tokens"]):
        flag = " **alwaysApply**" if r["always_apply"] else ""
        globs = f" globs={r['globs'][:40]}..." if r.get("globs") else ""
        lines.append(
            f"- `{r['path']}` — ~{r['est_tokens']} tok, {r['lines']} lines{flag}{globs}"
        )
    lines.append("")
    lines.append("## Skills (metadata + body)")
    for s in sorted(skills, key=lambda x: -x["body_est_tokens"]):
        flags = f" [{', '.join(s['flags'])}]" if s["flags"] else ""
        lines.append(
            f"- `{s['name']}` — desc ~{s['description_est_tokens']} tok, "
            f"body ~{s['body_est_tokens']} tok ({s['body_lines']} lines){flags}"
        )
    if dupes:
        lines.append("")
        lines.append("## Duplicate headings (sample)")
        for d in dupes[:8]:
            lines.append(f"- \"{d['heading']}\" in: {', '.join(d['files'])}")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    if hasattr(sys.stdout, "reconfigure"):
        try:
            sys.stdout.reconfigure(encoding="utf-8")
        except Exception:
            pass
    root = repo_root_from_script()
    cursor_dir = root / ".cursor"
    if not cursor_dir.is_dir():
        print(json.dumps({"error": f"No .cursor at {cursor_dir}"}), file=sys.stderr)
        return 1

    rules = scan_rules(cursor_dir)
    skills = scan_skills(cursor_dir)
    dupes = find_duplicate_headings(cursor_dir)
    summary = summarize(rules, skills, dupes)
    report = {
        "repo_root": str(root),
        "summary": summary,
        "rules": rules,
        "skills": skills,
        "duplicate_headings": dupes,
    }
    if "--json" in sys.argv:
        print(json.dumps(report, indent=2, ensure_ascii=False))
    else:
        print(render_markdown(summary, rules, skills, dupes))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
