---
name: self-update
description: >-
  .cursor/rules/*.mdc diff·반영. 기본 자동승인(「자동승인 해제」까지). self-update, 지식 반영.
  신규·P/A 분리는 add-guideline. 토큰·100점 루프는 cursor-optimizer.
disable-model-invocation: true
---

# Skill: self-update

팀 지식은 **`.cursor/rules/*.mdc`**. 에이전트용 장문 `.md` 신규 금지 — 예외: `README.md`, `commands/*.md`, 스킬 `SKILL.md`.

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
| Staging | `git-staging-noise.mdc` | git add 필터 |

스타일만 → invariant 금지. `AGENTS.md` 루트 중복 금지.

**형제:** 신규·P/A 분리 → [`add-guideline`](../add-guideline/SKILL.md). 토큰·채점 루프 → [`cursor-optimizer`](../cursor-optimizer/SKILL.md).

## Approval

| Mode | Trigger | Behavior |
|------|---------|----------|
| **Auto (기본)** | 「self-update 자동승인」 선언됨 (현재 repo) | diff 출력 → **즉시 반영** → 결과 보고 |
| Manual | 「self-update 수동승인」 / 「자동승인 해제」 | diff만 — 「승인」 대기 |

- Auto는 **`@self-update` diff 범위** (주로 `.cursor/rules/*.mdc`)만. skills/commands는 사용자 명시 또는 optimizer 범위.
- Manual 복귀: 「자동승인 해제」까지 Auto 유지.

## Process

1. `.cursor/rules/`에 없는 사실 나열.
2. 대상 `.mdc` 선택; ~80줄 넘으면 분리·`add-guideline` 검토.
3. diff 작성 → **Auto:** 즉시 write / **Manual:** 승인 대기.
4. **why** 중심.

## Forbidden

- `@self-update`·「커서 지침 업데이트」·`/cursor-optimizer` 개선 범위 **밖에서** rules/skills 편집
- `.cursor/` 에이전트용 장문 `.md` 신규 (`README.md`·`commands/*.md`·`SKILL.md` 예외)

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
**Auto:** 위 diff 반영 완료 (또는 변경 없음).
**Manual:** 승인 시 반영합니다.
```

없으면 `반영할 팀 지식 없음.`
