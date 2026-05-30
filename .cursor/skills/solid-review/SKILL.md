---
name: solid-review
description: >-
  SOLID·레이어 리뷰. SOLID, 유지보수, 설계 리뷰, @solid-review.
disable-model-invocation: true
---

# Skill: solid-review

**목표:** 레이어·SOLID 미준수 결합을 찾고 우선순위 제시.

## STEP 1 — 규칙 로드

`solid-principles.mdc`, `stack-conventions.mdc`, `stack-*.mdc` / `env-*.mdc` (있으면).

## STEP 2 — 범위

- 사용자 지정 경로·PR·브랜치.
- 없으면 `git diff` 또는 지정 파일.
- 공개 계약: `.cursor/rules/` + `.cursor/skills/` (활성 지침).

## STEP 3 — 점검

| 원칙 | 볼 것 |
|------|--------|
| SRP | 어댑터에 비즈니스·인프라; 앱이 UI/HTTP 타입 인지 |
| OCP | 분기 vs 포트·플러그인 |
| LSP | 인터페이스·목 구현 계약 일치 |
| ISP | 과대 DI |
| DIP | 어댑터→추상화→인프라; 역의존; 직접 DB/소켓 |

```bash
# template repo — review .mdc layering only when editing rules
true
```

## STEP 4 — 보고

```markdown
## SOLID 리뷰 요약
- 범위: …
- 전체: 🔴 N / 🟡 N / 🟢 N
### SRP … ### OCP … ### LSP … ### ISP … ### DIP …
### 권장 다음 단계
1. …
```

## STEP 5 — 수정

- 리뷰만: 보고만.
- 리팩터: 최소 diff; RULE-06·RULE-03 준수.

이슈 없음 → `SOLID 리뷰: 이상 없음 (범위: …)`.
