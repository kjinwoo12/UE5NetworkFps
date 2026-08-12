---
name: task-done
description: >-
  Closure — diff, RULE self-check, 린트/테스트, optional self-update. task-done / 작업 끝.
disable-model-invocation: true
---

# Skill: task-done

완료 표시 전 실행. 미선언 변경·RULE 위반 있으면 완료 금지.

**적용 생략:** 질문만·리뷰만·변경 없는 답.

## STEP 1 — Change summary

의도 vs `git diff`; 의도 밖 변경 나열. 착수 STEP 3 범위와 차이.

## STEP 2 — Self-check (`invariant-rules.mdc`)

해당분만:

- [ ] RULE-01 secrets
- [ ] RULE-02 blocking on game main / async path
- [ ] RULE-03 N/A unless persistence added
- [ ] RULE-04 N/A
- [ ] RULE-05 `.cursor` 무단 편집 없음 (또는 승인된 self-update/지침 업데이트)
- [ ] RULE-06 public contract·지침 등록부 동기화
- [ ] C++/스크립트 변경 → `ReadLints` (가능 시)
- [ ] 스테이징 노이즈 없음 (`git-staging-noise.mdc`)
- [ ] 로직 변경 + LLT 있으면 승인된 테스트 통과 (`ue-llt-tdd`)

## STEP 3 — Branch / lock

브랜치 기록; 락 있으면 해제.

## STEP 4 — Worktree

사용자 worktree 사용 시만.

## STEP 5 — self-update?

재사용 패턴 → `@self-update`. 신규·P/A 분리 → `/add-guideline`. 토큰 비대 → `/cursor-optimizer`.

## STEP 6 — Report

```markdown
## 완료
- …

## 검증
- …

## 후속 (있으면)
- …
```
