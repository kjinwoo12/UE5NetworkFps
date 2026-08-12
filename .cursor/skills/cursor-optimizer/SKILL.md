---
name: cursor-optimizer
description: >-
  Cursor 지침(.cursor/rules·skills·commands)을 100점 루브릭으로 채점하고, 사용자가
  지정한 개선 범위 안에서 승인 없이 write한 뒤 합계 100까지 반복한다. /cursor-optimizer,
  커서 지침 최적화, alwaysApply 토큰 줄이기, rules skills commands 채점·개선,
  cursor instruction optimization 요청 시 사용. 산출물: .result/cursor-optimizer.md.
disable-model-invocation: true
---

# Skill: cursor-optimizer

**목표:** `.cursor` 지침을 [루브릭](references/rubric.md)으로 **0–100 채점** → 사용자 **개선 범위** 안에서 **즉시 write** → **합계 100**까지 재채점 반복.

## 산출물

| 경로 | 내용 |
|------|------|
| `.result/cursor-optimizer.md` | **현재 회차** 채점 정본 (매 회 **덮어쓰기**) |
| `.result/cursor-optimizer-loop.md` | 회차 로그·점수·변경 파일·종료 사유 (**누적**) |

`.result/` 없으면 생성. 채팅에는 회차·합계·미달 축·이번에 write한 path만 요약.

## 필수 입력 — 개선 범위

루프 **시작 전** 사용자가 개선 범위를 주어야 한다.

**별칭:**

| 말 | 범위 |
|----|------|
| `셀프 개선` | `.cursor/rules`+`skills`+`commands`+`README.md` |
| `@.cursor` / `커서 문서 전체` | 위 + `docs/**`(있으면) |

기타 예: `개선 범위: .cursor/rules/`

**범위가 없으면** 채점·인벤토리만 `.result/cursor-optimizer.md`에 쓰고 **write·루프 금지**. 범위 요청 후 종료.

채점은 항상 **rules + skills + commands 전부**. write는 **개선 범위 ∩ `.cursor/`** 만.

**기본 비대상(`허용:` 없으면 write 금지):** 루트 `AGENTS.md`, `.kilo/`, `.agents/`, `.roo/`, `Source/`·`Content/` 게임 코드.

## 종료 조건

**성공:** 점수 요약 **합계 ≥ 100**.

**중단 (로그에 사유):**

| 조건 | 동작 |
|------|------|
| `max_iterations` (기본 **8**) | 잔여 감점·범위 한계 보고 |
| 개선 범위 안 **조치 가능 항목 0** + 합계 &lt; 100 | 범위 확장 요청 후 중단 |
| 동일 합계 **2회 연속** + 동일 Top 감점 | 정체 중단 |
| 사용자 「중단」「여기까지」 | 즉시 종료 |

## 루프

```
┌─ 회차 N ──────────────────────────────────────────┐
│ 1. INVENTORY — references/inventory.md           │
│ 2. SCORE     — rubric.md → cursor-optimizer.md   │
│ 3. EXIT?     — 합계 ≥ 100                        │
│ 4. PLAN      — 미달 축·범위 내 최소 write 목록    │
│ 5. WRITE     — 승인 없이 범위 내 즉시 수정         │
│ 6. LOG       — cursor-optimizer-loop.md 누적     │
│ 7. N++       — 1으로                               │
└──────────────────────────────────────────────────┘
```

### 1. INVENTORY

[inventory.md](references/inventory.md)대로 `.cursor/rules`·`skills`·`commands` 목록·bytes·tokens≈·frontmatter 수집.

### 2. SCORE

[rubric.md](references/rubric.md) 7축 채점. `.result/cursor-optimizer.md`를 [report-template.md](references/report-template.md) 구조로 **덮어쓰기**.

### 3. EXIT

합계 ≥ 100이면 loop 로그에 `status: success` 후 종료.

### 4. PLAN

1. 배점 높은 미달 축 우선: 1 → 2 → 3·4 → 5·6 → 7  
2. **개선 범위 안**에서만 path 선정  
3. 한 회차 = 같은 축 위주 최소 diff (폭주 방지)  
4. 범위 밖만으로 100이 되면 → write하지 말고 범위 확장 요청 후 중단

### 5. WRITE

- **승인 대기 없음** — PLAN 직후 바로 파일 수정  
- `.cursor/` 밖·비대상 경로·범위 밖 **금지**  
- 중복은 정본 1곳 + 링크로 정리; alwaysApply 남발 금지; 도메인 규칙은 globs  
- skill `description`·command 본문은 트리거·절차만 짧게  
- 이 repo: 불변 = `invariant-rules.mdc` (projectoa `RULES.md` 역할). caveman always는 얇은 포인터 유지  
- 커밋은 사용자가 요청할 때만

### 6. LOG

`.result/cursor-optimizer-loop.md`에 회차 블록 **append**:

```markdown
## 회차 N
- **합계:** 72 → (write 후 재채점은 다음 회차 SCORE)
- **write:** `path1`, `path2`
- **미달 축:** 1, 2
- **메모:** …
```

성공/중단 시 파일 하단에:

```markdown
## 종료
- **status:** success | stalled | max_iterations | user_abort
- **최종 합계:** N
- **사유:** …
```

## 채팅 요약 템플릿

```markdown
### /cursor-optimizer 회차 N
- 합계: **X / 100**
- write: …
- 다음: (계속 루프 | 범위 확장 필요 | 완료)
- 전문: `.result/cursor-optimizer.md`
```

## 관련

- 채점 기준: [references/rubric.md](references/rubric.md)
- 인벤토리: [references/inventory.md](references/inventory.md)
- 정적 감사: [`context-health`](../context-health/SKILL.md)
- 지침 추가(P/A 분리): [`add-guideline`](../add-guideline/SKILL.md) — 토큰·루프는 **cursor-optimizer 우선**
