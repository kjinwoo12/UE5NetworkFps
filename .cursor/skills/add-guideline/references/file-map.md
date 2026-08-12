# 지침 파일 축 매핑 (UE5NetworkFps · `.cursor`)

`add-guideline` 분류·배치 시 참고. **실존 파일만** 정본으로 적는다.

## 범례

| 축 | 설명 |
|----|------|
| **P** | 프로젝트 맥락 — Lyra·FpsCore·경로·도메인 |
| **A** | AI 사용성 — 에이전트 효율·오류 방지·공정 |
| **M** | 메타 — 지침 생태계 진입 |

## `.cursor` 루트

| 파일 | 축 | 비고 |
|------|-----|------|
| `README.md` | M | Cursor 트리 진입 인덱스 |
| `commands/*.md` | A | `/` 커맨드 진입점 |
| `mcp.json` | P | Unreal MCP 연결 |

## `rules/` (실존)

| 파일 | 축 | 비고 |
|------|-----|------|
| `invariant-rules.mdc` | P/M | 불변 RULE — always |
| `project-domain.mdc` | P | Lyra·GF·1P — always |
| `instruction-ecosystem.mdc` | M | 판단 트리·레이어 — always(슬림 목표) |
| `caveman-mode.mdc` | A | 문체 강제 — always(얇은 포인터) |
| `git-staging-noise.mdc` | A | git 노이즈 — always |
| `communication-style.mdc` | A | 서브에이전트 위임 — always→온디맨드 후보 |
| `karpathy-guidelines.mdc` | A | 코딩 행동 — globs 코드 후보 |
| `stack-unreal.mdc` | P | UE5 스택 — globs Source/Content/Config |
| `stack-conventions.mdc` | A | 레이어·계약 — globs |
| `solid-principles.mdc` | A | SOLID — globs |
| `agent-local.mdc` | P | 세션·gitignore |

## `skills/`

| 경로 | 축 | 비고 |
|------|-----|------|
| `task-start/` · `task-done/` | A | 착수·마무리 |
| `ue-llt-tdd/` | A/P | UE LLT TDD (공정 A + UE 경로 P 혼재 주의) |
| `self-update/` · `add-guideline/` | A | 지침 반영 / P·A 분리 |
| `cursor-optimizer/` | A | 100점 루프 → `.result/cursor-optimizer.md` |
| `context-health/` | A | 정적 토큰 감사 |
| `caveman*` · `cavecrew/` | A | 압축 문체·위임 |
| `solid-review/` | A | SOLID 리뷰 |
| `skill-creator/` | A | 번들(장문·축6 제외) |

## 신규 이름

| 축 | 패턴 | 예 |
|----|------|-----|
| P | `{도메인}.mdc` | `fps-camera.mdc` |
| A | `{검증|행동|공정}.mdc` | `commit-conventions.mdc` |
