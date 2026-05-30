---
name: task-start
description: >-
  Briefing — invariant-rules, 코드로 구현·계약 확인, 범위 선언. task-start / 작업 시작.
disable-model-invocation: true
---

# Skill: task-start

브리핑 완료 전까지 코드 수정 금지.

## STEP 1 — invariant-rules.mdc

`.cursor/rules/invariant-rules.mdc` 읽기.  
`library` — RULE-02·03·04는 N/A. RULE-01·05·06(공개 지침 변경 시) 적용.

## STEP 2 — Locate targets

채팅·README만 믿지 않는다.

```bash
ls .cursor/rules .cursor/skills 2>/dev/null
ls result/cursor-templates/rules result/cursor-templates/skills 2>/dev/null
```

`project-domain.mdc` **구현 상태** 표와 대조. 경로·심볼·계약 이름 기록.

## STEP 3 — Scope declaration

```markdown
### 수정할 파일·심볼
- path → symbol / route / export

### 확인만
- path → reason

### 범위 밖
- …
```

## STEP 4 — Git / lock

심볼 락: 사용자 지시 없으면 N/A. 브랜치만 기록.

## STEP 5 — Start

한 문장 후 **STEP 3 범위 안에서만** 작업.
