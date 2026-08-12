---
name: context-health
description: >
  Diagnose slow Cursor/Claude: audit .cursor rules and skills for context bloat,
  alwaysApply cost, duplicate guidance, oversized SKILL.md. Use when user says AI
  is slow, laggy, heavy context, high tokens, or asks 지침/스킬 점검.
  Triggers: /context-health, "느려", "속도", "토큰", "context audit".
  Pairs with /caveman-stats (runtime). Fixes go through @self-update — no silent rule edits.
---

# Context health

Slow agent ≠ always model lag. Often **input context** (rules every turn, skill list, chat history, fat tool results) dominates. This skill separates **static repo bloat** from **session/runtime** causes and gives ranked fixes.

## When to use vs siblings

| Need | Use |
|------|-----|
| "How many tokens this session?" | `/caveman-stats` (hook; real numbers) |
| "Trim my CLAUDE.md / memory file" | `caveman-compress` |
| "Delegate without bloating main thread" | `cavecrew` |
| "Apply rule changes safely" | `@self-update` / `/add-guideline` |
| "Score + loop-fix .cursor to 100" | `/cursor-optimizer` (범위 필수) |
| "Why slow + what to change in .cursor?" | **this skill** |

## Workflow

### 1. Run static audit (deterministic)

From repo root:

```bash
python .cursor/skills/context-health/scripts/audit_context.py
python .cursor/skills/context-health/scripts/audit_context.py --json
```

Read the report. Note `always_apply_est_tokens`, skill metadata totals, `body>500_lines`, `description>400_chars`, duplicate headings.

### 2. Quick manual checks

- **alwaysApply rules:** `rg "alwaysApply: true" .cursor/rules/` — each file is paid every turn.
- **Globs rules:** `alwaysApply: false` + globs = only when matching files touched (cheaper).
- **MCP:** `.cursor/mcp.json` — count servers; unused = extra tool schemas every session.
- **Chat length:** If audit is clean but user still slow → likely history/tool output, not files.

### 3. Classify findings

Bucket each issue:

| Bucket | Examples |
|--------|----------|
| **P0 static** | alwaysApply rule >80 lines; duplicate 3+ paragraph blocks across rules |
| **P1 static** | Skill body >500 lines; description war novel; 10+ skills with long descriptions |
| **P2 session** | Long thread; parallel subagents; full-file reads; vanilla Explore prose |
| **P3 external** | Model tier; network; UE MCP timeout — not .cursor fix |

### 4. Report to user

Use this template (Korean if user writes Korean):

```markdown
# Context health

## Verdict
<one line: static bloat | session bloat | mixed | looks OK>

## Top costs (static)
1. ...
2. ...

## Session factors (ask / infer)
- ...

## Recommended actions (impact ↓)
1. [P0] ... → @self-update / user confirm
2. [P1] ...
3. [P2] ... (new chat, cavecrew, narrower reads)

## Not recommended
- ...
```

Keep recommendations **actionable and minimal** — 3–5 items, not a rewrite of the whole `.cursor/`.

### 5. Apply changes

- **Rules:** propose diff → `@self-update` · `/cursor-optimizer`(개선 범위) · 「커서 지침 업데이트」. Never silent edit (RULE-05).
- **Skills:** user explicitly asked to fix skills, or optimizer/`@self-update` scope includes skills.
- **Session:** suggest new chat, `/caveman-stats`, cavecrew for searches — no file change.

Detailed fix patterns: read `references/remediation.md` when drafting P0/P1 actions.

## Red flags in this repo (common)

- `project-domain.mdc` alwaysApply + large tables → candidate for globs
- `instruction-ecosystem` + `communication-style` + `caveman-mode` overlap → dedupe
- `skill-creator` SKILL.md huge → only loads on trigger, but long description still in skill list
- Dual MCP (`unreal-native` + `unreal-bridge`) → schema cost when both enabled

## Principles

- **Measure before prune.** Run the script; don't guess token counts.
- **Recurring input > one-shot.** alwaysApply beats a skill read once.
- **Don't weaken invariants for speed.** `invariant-rules.mdc` stays.
- **Generalize fixes.** Recommend patterns (globs, progressive disclosure), not overfit to one audit run.

## Output discipline

- Audit report + ranked recommendations only unless user asked to implement.
- If implementing: smallest diff that fixes the cited P0/P1 item.
- After rule changes, suggest re-run audit to verify `always_apply_est_tokens` dropped.
