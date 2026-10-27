# 설계 결정 기록 (ADR)

이 프로젝트의 주요 설계 결정을 짧은 기록(Architecture Decision Record)으로 남깁니다. 코드와 문서는 "지금 어떻게 되어 있는지"를 보여 주고, 이 기록은 "왜 그렇게 했고, 무엇을 버렸는지"를 남깁니다.

## 목록

| 번호 | 결정 | 상태 | 날짜 |
|---|---|---|---|
| [0001](0001-lyra-starter-game-base.md) | Lyra Starter Game을 기반으로 한다 | 채택 | 2026-06-20 |
| [0002](0002-keep-crouch.md) | 앉기를 유지한다 | 채택 | 2026-10-18 |
| [0003](0003-grip-module-position-only.md) | 양손 무기 잡기 모듈은 위치만 맞추고, 손 모양은 애니메이션에 맡긴다 | 채택 | 2026-10-18 |
| [0004](0004-full-body-first-person.md) | 풀바디 1인칭(true first person)으로 전환한다 | 채택 | 2026-10-19 |
| [0005](0005-engine-first-person-rendering.md) | 뷰모델은 엔진 First Person Rendering 레이어로 그린다 | 채택 | 2026-10-19 |
| [0006](0006-event-driven-weapon-sync.md) | 로컬 1인칭 총은 이벤트로 갱신한다 | 채택 | 2026-10-26 |
| [0007](0007-weapon-sway.md) | 총은 화면에 고정하되, 이동 중 몸의 움직임을 0.25배만 따라가게 한다 | 채택 | 2026-10-27 |
| [0008](0008-accept-walking-wrist-fail.md) | 걸을 때 오른손목 검사 실패는 알려진 제약으로 둔다 | 채택 | 2026-10-20 |
| [0009](0009-automation-regression-test.md) | 1인칭 리그 회귀 검사는 언리얼 Automation 테스트로 만든다 | 채택 | 2026-10-27 |
| [0010](0010-github-flow-without-pr.md) | 브랜치는 GitHub Flow를 따르되, PR 없이 main에 머지한다 | 채택 | 2026-10-27 |
| [0011](0011-self-hosted-ci-runner.md) | 회귀 검사를 집 데스크탑 self-hosted runner에서 자동 실행한다 | 채택 | 2026-10-28 |

## 쓰는 법

1. [template.md](template.md)를 복사해 `NNNN-짧은-영문-이름.md`로 저장합니다. 번호는 목록의 다음 번호입니다.
2. 배경, 결정, 대안, 결과를 짧게 씁니다. 잰 숫자나 재현된 증상이 있으면 함께 적습니다.
3. 위 목록에 한 줄을 추가하고, 결정과 같은 브랜치에서 커밋합니다.

이미 채택한 기록은 고쳐 쓰지 않습니다. 결정이 바뀌면 새 기록을 만들고, 예전 기록의 상태를 "NNNN으로 대체"로 바꿉니다. 오타나 링크 수정은 괜찮습니다.

구현 세부(계산식, 설정값)는 [FirstPersonRig.md](../FirstPersonRig.md) 같은 설계 문서에 두고, 여기서는 링크만 합니다.
