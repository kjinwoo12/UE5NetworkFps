---
name: task-start
description: >-
  Briefing — invariant-rules, grep으로 경로·심볼 확인, 범위 선언. 기능·로직이면 ue-llt-tdd
  Red 검토. task-start / 작업 시작 / 착수.
disable-model-invocation: true
---

# Skill: task-start

코드·파일 수정 **전에** 아래 순서. 대화·기억 경로만 쓰지 않는다.

**적용 생략:** 질문만·리뷰만·한 줄 답. 구현·리팩터·다중 파일 수정 시 필수.

## STEP 1 — invariant-rules.mdc

`.cursor/rules/invariant-rules.mdc` 읽기. 관련 RULE ID 나열. 없으면 `해당 RULE 없음`.

| 작업 유형 | 우선 확인 |
|-----------|-----------|
| 비밀·자격 | RULE-01 |
| 게임 스레드·I/O | RULE-02 |
| `.cursor` 지침 | RULE-05 |
| 공개 계약·지침 | RULE-06 |

## STEP 2 — 대상 파악 (검색 필수)

- 수정·조회 심볼은 **검색 도구**로 실제 경로 확인.
- `project-domain.mdc` **구현 상태** 표와 대조.
- 작업 경로에 맞는 규칙(파일 있을 때만):
  - C++/Content/Config → `stack-unreal.mdc`
  - `.cursor`·지침 → `instruction-ecosystem.mdc` · `add-guideline`

## STEP 3 — 작업 범위 선언

```markdown
### 수정할 파일·심볼
- path → symbol / 의도

### 확인만 (수정 안 함)
- path → 이유

### 범위 밖
- …
```

선언한 범위 안에서만 구현.

## STEP 4 — Git / lock

심볼 락: 사용자 지시 없으면 N/A. 브랜치만 기록.  
스테이징: `git-staging-noise.mdc` — `git add .` 금지.

## STEP 5 — TDD (기능·로직 변경 시)

프로젝트 LLT/TDD가 잡힌 경우 [`ue-llt-tdd`](../ue-llt-tdd/SKILL.md) 따름.  
미정이면 `TDD 생략 — 프로젝트 LLT 미정` 한 줄.  
에셋·Experience·카메라 BP만이면 `TDD 생략 — (이유)`.

## STEP 6 — Start

한 문장 후 **STEP 3 범위 안에서만** 작업.
