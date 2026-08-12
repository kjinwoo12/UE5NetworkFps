# Remediation playbook

Impact order: fix the biggest recurring input first, then session habits, then skill metadata.

## alwaysApply rules

Every user message pays this cost. Targets:

| Signal | Fix |
|--------|-----|
| Rule >80 lines, alwaysApply | Split: keep 1-screen summary in alwaysApply; move detail to globs or `references/` |
| Domain table rarely needed | `project-domain.mdc`: set `alwaysApply: false`, add globs for `Source/**`, `Plugins/**`, `Content/**` |
| Duplicate caveman/delegation text | Keep one canonical block in `instruction-ecosystem.mdc`; others link only |
| Karpathy + communication overlap | Merge or move karpathy to on-demand skill |

Apply rule edits via `@self-update` or 「커서 지침 업데이트」 (RULE-05).

## Skills

| Signal | Fix |
|--------|-----|
| SKILL.md body >500 lines | Progressive disclosure: move scripts/refs out; trim examples |
| Description >400 chars / "pushy" | Shorten; put trigger phrases in description only, not body |
| Skill rarely used but huge metadata | `disable-model-invocation: true` if manual-only (like `self-update`) |
| Overlap with a rule | Skill = workflow; rule = constraint. Delete duplicate prose from one side |

## Session / runtime (not in files)

| Symptom | Likely cause | Mitigation |
|---------|--------------|------------|
| Slow after 30+ turns | Context full; long history | New chat; summarize milestone first |
| Slow on code search | Vanilla Explore returns prose | `cavecrew` investigator (~1/3 tokens) |
| Slow after big MCP reads | Tool output injected verbatim | Narrow MCP queries; read slices not whole logs |
| Slow every turn, new chat too | alwaysApply + skill list bloat | Run `audit_context.py`; trim per table above |
| Numbers needed | `/caveman-stats` | Hook reads session log — not estimated |

## MCP

- Duplicate servers in `.cursor/mcp.json` → duplicate tool schemas in context
- Unused MCP servers → disable in mcp.json when not editing UE

## Do not

- Edit `.cursor/rules/*.mdc` inside this skill without `@self-update` approval path
- Delete skills/rules to "speed up" without user confirm
- Disable `invariant-rules.mdc` for performance
