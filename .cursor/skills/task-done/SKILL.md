---
name: task-done
description: >-
  Closure — diff, RULE self-check, 테스트, optional self-update. task-done.
disable-model-invocation: true
---

# Skill: task-done

완료 표시 전 실행.

## STEP 1 — Change summary

의도 vs `git diff`; 의도 밖 변경 나열.

## STEP 2 — Self-check (`invariant-rules.mdc`)

프로젝트에 해당하는 항목만 체크:

- [ ] RULE-01 secrets
- [ ] RULE-02 blocking on async/UI/game main path
- [ ] RULE-03 migration if persistence schema changed
- [ ] RULE-04 code only in canonical repo / no forbidden copy path
- [ ] RULE-06 contract register + DTO/types + tests if **public** contract changed
- [ ] 테스트 스위트 없음 — 지침·문서 동작 변경 시 `result/README.md`와 `.cursor/` 일관성 확인
- [ ] 어댑터·서비스·DTO 터치 시 `solid-principles.mdc`; 필요 시 `@solid-review`

## STEP 3 — Branch / lock

브랜치 기록; 락 해제.

## STEP 4 — Worktree

사용자 worktree 사용 시만.

## STEP 5 — self-update?

재사용 패턴 → `@self-update` (대상 `.mdc`).

## STEP 6 — Report

짧게: 완료 / blocked / human 필요.

RULE 위반·미선언 변경 시 완료 처리 금지.
