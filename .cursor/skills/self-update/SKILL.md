---
name: self-update
description: >-
  .cursor/rules/*.mdc diff 제안. 승인 전 수정 금지. self-update, 지식 반영.
disable-model-invocation: true
---

# Skill: self-update

팀 지식은 **`.cursor/rules/*.mdc`**. `.cursor/` 아래 에이전트용 `.md` 신규 금지.

## Classification

| Layer | Path | 내용 |
|-------|------|------|
| Universal | (드묾) | 여러 repo·언어 공통 |
| Domain | `project-domain.mdc` | 레이아웃·계약·why |
| Immutable | `invariant-rules.mdc` | 배포/DB/보안·계약 붕괴만 |
| Conventions | `stack-conventions.mdc` | 레이어 (언어 무관) |
| Stack | `stack-*.mdc` | 오버레이에서 유래 |
| Environment | `env-*.mdc` | repo·클라이언트 경계 |
| Ephemeral | `agent-local.mdc` | 세션 (gitignore) |
| Process | `instruction-ecosystem.mdc` | 생태계만 |

스타일만 → invariant 금지. `AGENTS.md` 루트 중복 금지.

## Process

1. `.cursor/rules/`에 없는 사실 나열.
2. 대상 `.mdc` 선택; ~80줄 넘으면 분리·오버레이 추가 검토.
3. **diff만** — 승인 전 write 금지.
4. **why** 중심.

## Forbidden

- 승인 없이 rules/skills 편집
- 예외: 「커서 지침 업데이트」 (RULE-05)
- `.cursor/` 에이전트용 `.md` 신규

## Output

```markdown
## /self-update 결과
### 새 지식
- …
### 제안 변경
**대상:** `.cursor/rules/….mdc`
\`\`\`diff
…
\`\`\`
**이유:** …
---
승인 시 반영합니다.
```

없으면 `반영할 팀 지식 없음.`
