# UE5NetworkFps — 개발 문서

네트워크 FPS 프로젝트 **UE5NetworkFps**의 작업 기록·기술 정리입니다. 포트폴리오·온보딩·후속 개발 참고용으로 작성했습니다.

**저장소 루트:** [README.md](../README.md) — 실행 요약·Cursor MCP 빠른 링크

## 문서 목록

| 문서 | 내용 |
|------|------|
| [01-project-overview.md](./01-project-overview.md) | 프로젝트 목표, 기술 스택, 현재 구현 범위 |
| [02-content-architecture.md](./02-content-architecture.md) | 콘텐츠·설정·모듈 구조 |
| [03-first-person-character.md](./03-first-person-character.md) | 1인칭 캐릭터 시스템 (핵심 작업) |
| [04-technical-investigations.md](./04-technical-investigations.md) | 조사·설계 이슈 (카메라, 클리핑, Control Rig) |
| [05-development-log.md](./05-development-log.md) | 커밋·작업 타임라인·다음 단계 |
| [06-mcp-cursor-setup.md](./06-mcp-cursor-setup.md) | Cursor MCP · McpAutomationBridge 연동 |

## 빠른 요약 (포트폴리오용)

- **엔진:** Unreal Engine 5.7
- **형태:** Blueprint·Content 중심 FPS (C++ 게임 모듈은 최소 골격)
- **베이스:** Epic First Person / Third Person 템플릿 + Mannequin·무기 에셋
- **핵심 작업:** 1인칭 전신 렌더링 환경에서 **카메라·메시 클리핑**, **피치 제한**, **메시/카메라 오프셋**, **점프 시 Control Rig 안정화** 설계
- **진행 중:** `Content/NetworkFps/` 프로젝트 전용 레벨·캐릭터, 멀티플레이 네트워킹 (미구현)

## 스크린샷·데모 (추가 권장)

포트폴리오에 넣을 때 아래 캡처를 `Docs/images/` 등에 두고 각 문서에 링크하면 좋습니다.

- `Lvl_FirstPerson` PIE — 정면·아래 시선
- 점프 중 1P 메시·카메라 겹침 (Before)
- `CtrlRig_FPWarp` / AnimBP 그래프 (에디터)
- `L_PlayerCharacterDemo` (NetworkFps 데모 레벨)
