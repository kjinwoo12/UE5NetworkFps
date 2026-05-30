# 01. 프로젝트 개요

## 한 줄 소개

**UE5NetworkFps**는 Unreal Engine 5.7 기반 **네트워크 FPS**를 목표로 하는 게임 프로젝트입니다. 현재 단계에서는 Epic 템플릿(1인칭·3인칭)과 Mannequin 에셋을 기반으로 **1인칭 캐릭터·카메라·애니메이션 파이프라인**을 다듬는 작업이 중심입니다.

## 프로젝트 목표

| 단계 | 목표 | 상태 |
|------|------|------|
| 기반 | UE5.7 프로젝트·Git·Enhanced Input | ✅ 완료 |
| 콘텐츠 | First Person / Third Person 템플릿 이식 | ✅ 완료 |
| 1P 캐릭터 | 전신 1인칭 렌더링 + 클리핑·점프 이슈 해결 | 🔄 진행 중 |
| 프로젝트 전용 | `Content/NetworkFps/` 레벨·데모 | 🔄 시작 |
| 네트워크 | 멀티플레이 복제·예측·히트 등 | ⏳ 예정 |

## 기술 스택

| 구분 | 선택 |
|------|------|
| 엔진 | Unreal Engine **5.7** |
| 언어 | Blueprint (주), C++ (게임 모듈 껍데기) |
| 입력 | **Enhanced Input** (`DefaultPlayerInputClass` = `EnhancedPlayerInput`) |
| 렌더링 | Lumen GI·반사, Virtual Shadow Maps, Ray Tracing (프로젝트 DefaultEngine 설정) |
| 캐릭터 | UE5 Mannequin (`SKM_Manny_Simple` 등) |
| 1P 애니 | `ABP_FP_Copy` + **Control Rig** `CtrlRig_FPWarp` |
| 에디터 자동화 | **UnrealMCP** 플러그인 (블루프린트 조사·Cursor 연동) |
| 버전 관리 | Git (`main` 브랜치) |

## C++ 모듈

`Source/UE5NetworkFps/`는 기본 게임 모듈만 존재합니다.

- `UE5NetworkFps.Build.cs`: `Core`, `Engine`, `InputCore`, `EnhancedInput` 의존
- `UE5NetworkFps.cpp`: `IMPLEMENT_PRIMARY_GAME_MODULE` — **게임플레이 로직은 아직 BP·에셋 쪽**

향후 네트워크·무기·데미지 등 **성능·계약이 필요한 부분**부터 C++로 내릴 수 있는 구조입니다.

## 포트폴리오에서 강조할 포인트

1. **First Person Rendering** — Epic 최신 1P 템플릿(전신 메시) 이해와 한계 파악
2. **카메라·애니·리그 연동** — ViewPitch 클램프, 컴포넌트 오프셋, Control Rig 후처리
3. **문제 → 원인 → 대안** 형태의 기술 조사 (클리핑, 점프 겹침, 바닥 시야 제한)
4. **도구 활용** — 바이너리 `.uasset` 한계를 MCP·에디터로 보완한 조사 방법

## 관련 Epic 문서

- [First Person Template (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/first-person-template-in-unreal-engine)
- [First Person Rendering](https://dev.epicgames.com/documentation/en-us/unreal-engine/first-person-rendering)
