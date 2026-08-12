# .cursor — Cursor 전용 지침

타 에이전트(`AGENTS.md`, `.kilo/`, `.agents/`)와 **분리**. 이 폴더만 Cursor 최적화·편집 대상.

## 진입

| 경로 | 역할 |
|------|------|
| [`rules/invariant-rules.mdc`](rules/invariant-rules.mdc) | 불변 제약 RULE-01~06 |
| [`rules/`](rules/) | always-on / globs 규칙 |
| [`skills/`](skills/) | `/`·스킬 공정 |
| [`commands/`](commands/) | 슬래시 커맨드 |
| [`mcp.json`](mcp.json) | Unreal MCP 브리지·네이티브 |

## 자주 쓰는 커맨드

- `/cursor-optimizer` — 지침 100점 채점·개선 (범위 필수; `셀프 개선` = 이 트리)
- `/context-health` — alwaysApply·스킬 비대 감사
- `/task-start` → (로직이면 `/ue-llt-tdd`) → `/task-done`
- `/add-guideline` · `/self-update` · `/skill-creator`
- `/caveman` · `/solid-review`

## rules 요약

- **always (목표):** 합 ≤800 tok·파일 ≤3 — `invariant-rules`, `caveman-mode`, `git-staging-noise` (+ 필요 시 슬림 포인터)
- **도메인 always:** `project-domain.mdc` (Lyra·FpsCore·1P) — 비대하면 optimizer로 분리
- **글롭:** `stack-unreal` (Source/Content/Config), `solid`·`stack-conventions` (`.cursor`·result)
- **공정 always 후보 축소:** `instruction-ecosystem`·`communication-style`·`karpathy` → globs/온디맨드 검토

## Git

- 팀 공유: `rules/` · `skills/` · `commands/` · `README.md` · `mcp.json`
- 개인용: `rules/agent-local.mdc` (gitignore)
- 산출: `.result/` (optimizer·리뷰 보고서; 필요 시 추적)

상세 채점: `skills/cursor-optimizer/references/rubric.md`
