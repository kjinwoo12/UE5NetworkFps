---
name: add-guideline
description: >-
  Cursor 지침 추가·개선. 프로젝트 맥락(P)과 AI 사용성(A)을 파일로 분리 — A는 repo 무관·이식 가능만.
  A 파일에 Lyra/FpsCore 경로·RULE 매핑 넣으면 FAIL. 지침 추가, 규칙 분리, solid-principles·
  project-domain 정리, mixed rule split, A축 이식성 검증 요청 시 사용.
disable-model-invocation: true
---

# Skill: add-guideline

팀 지침 **추가·개선·분리** 전용. **승인 전 write·커밋 금지** → 분류·배치·diff 제안만.  
세션 습득의 즉시 반영은 [`self-update`](../self-update/SKILL.md)(자동 write). 토큰·100점 루프는 [`cursor-optimizer`](../cursor-optimizer/SKILL.md).

## 두 축 (혼합 금지)

| 축 | 의미 | 파일 |
|----|------|------|
| **P — 프로젝트 맥락** | 이 repo만 — Lyra·GF·1P·경로·MCP | `project-domain.mdc`, `stack-unreal.mdc`, P축 `rules/*.mdc` |
| **A — AI 사용성** | **다른 repo에 복사해도 통용** — 에이전트 효율·오류 방지·공정 | `solid-principles.mdc`, `karpathy-guidelines.mdc`, `caveman*`, 범용 `skills/*` |

**A 파일에 P 내용을 “링크만 달고” 남겨두기 금지.** P 문장은 P 파일 diff로 **이동**하고 A에서는 삭제한다.

판단·매핑: [references/file-map.md](references/file-map.md) · A 검증: [references/portability-checklist.md](references/portability-checklist.md)

### 빠른 판별

```
다른 UE/게임·일반 repo에 그대로 써도 의미 있음?     → A
repo 이름·경로·GF·Experience 없으면 무의미?        → P
한 bullet에 둘 다?                                  → 두 diff·두 파일로 쪼갬
```

## A축 이식성 게이트 (A diff 전 필수)

[portability-checklist.md](references/portability-checklist.md) 순서:

1. A 초안에서 P 토큰 제거(경로, GF명, RULE-0x 해석, 고유 클래스명, P 매핑 표)
2. P에서 빠진 내용 → **제안 2 — P — …** diff에만 작성
3. grep 스모크 — A 대상 파일에 repo 고유 문자열 **0건** 목표
4. 출력에 `### A축 이식성 검증` → `PASS` | `FAIL` (FAIL이면 A diff 제출 금지)

## 배치

| 축 | 대상 | 신규 |
|----|------|------|
| P | `project-domain.mdc`, `stack-unreal.mdc`, P축 `rules/*.mdc` + `globs` | 도메인·GF 단위 |
| A | 범용 `.mdc`·`skills/` | 행동·검증·공정 — **globs에 repo 경로 넣지 않음**(P가 globs 담당) |
| RULE | [`invariant-rules.mdc`](../../rules/invariant-rules.mdc) | 측정 가능·붕괴 수준만 |
| 공정 | `skills/*/SKILL.md`, `commands/*` | 루프·스킬 절차 |
| M | `README.md`, `instruction-ecosystem.mdc` | 진입·판단 트리 |

**금지:** P 표를 A에 두기 · A에 `Plugins/GameFeatures/FpsCore` · “적용: LyraCameraMode…” · A 개선 시 P diff 없이 A만 수정

## 워크플로 — 신규

1. 의도 bullet
2. bullet마다 P|A (복합 → 분리)
3. README·기존 `.mdc` 중복 grep
4. 배치 — A는 ~80줄, **이식 가능** 한 주제
5. **A면 게이트 통과 후** diff
6. README `rules/` 표 — **축** 열

## 워크플로 — 기존 개선·분리

1. 대상 전체 읽기
2. **절마다 P|A 라벨** — 혼재 목록 필수
3. 조치 (동시에 plan)
   - **이동:** P 문장 → P 파일 (제안 2)
   - **분리:** `.mdc` → P + A 두 파일
   - **A 정리:** P 삭제 + 게이트
4. A-only 개선이라도 P 잔재 grep — 있으면 이동 diff **함께** 제출

## 출력 형식 (필수)

```markdown
## /add-guideline 제안

### 요청 요약
- …

### 분류
| 내용 | 축 | 조치 |
|------|-----|------|
| … | P \| A | … |

### A축 이식성 검증
- grep: (명령 또는 수동) — 매칭 N건
- **PASS** \| **FAIL** — (FAIL 시 A diff 생략, P 이동 먼저)

### 제안 1 — A — …
…

### 제안 2 — P — …
( P 이동·추가가 있을 때만 )

---
승인 시에만 반영합니다.
```

## `.mdc` frontmatter

- **A:** `alwaysApply: true` 또는 주제별 — **repo-only glob 금지**(코드 언어 globs OK: `**/*.{h,cpp}`)
- **P:** `globs: Source/**` · `Plugins/GameFeatures/**` 등 repo 경로 허용
- `description`: WHAT + WHEN (트리거)

## self-update와 역할

| 스킬 | 역할 |
|------|------|
| **add-guideline** | 추가·**P/A 분리**·A 이식성 게이트 |
| **self-update** | 세션 습득 사실 → 기존 지침 diff |
| **cursor-optimizer** | 토큰·100점 루프 |

## 금지

- 승인 없이 write
- A diff에 P 매핑·경로·GF명
- `A축 이식성: FAIL` 상태로 A 파일 반영
- file-map에 없는 `.mdc`를 문서화 없이 신규(file-map 갱신 필수)

## 참고

- [file-map.md](references/file-map.md) · [portability-checklist.md](references/portability-checklist.md)
- [`invariant-rules.mdc`](../../rules/invariant-rules.mdc)
