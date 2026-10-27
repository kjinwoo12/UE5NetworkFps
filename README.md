# UE5NetworkFps

Unreal Engine 5.8.3과 **Lyra**를 바탕으로 만드는 **1인칭 5 vs 5 네트워크 히어로 슈터**(화물 운송) 개인 프로젝트입니다.
Lyra의 GAS·Enhanced Input·ReplicationGraph·Game Features 구조 위에 1인칭 시점과 히어로 전용 기능을 Game Feature 플러그인으로 얹고 있습니다.

| 항목 | 내용 |
|---|---|
| 엔진 | Unreal Engine 5.8.3 (`UE5NetworkFps.uproject`) |
| 베이스 | [Lyra Sample Game](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine) |
| 언어 | C++ (`Source/LyraGame`, `Plugins/GameFeatures/FpsCore`) + Blueprint |
| 목표 | 서버 권한 판정·랙 보상, GAS 예측, 데디케이티드 서버, Steam 세션이 동작하는 한 판 |
| 기획 | [Docs/GameDesign.md](Docs/GameDesign.md) |
| 설계 결정 | [Docs/decisions](Docs/decisions/README.md) |

## 개발 일지

작업 과정과 설계 이유는 블로그에 연재하고 있습니다.

- [#0 Lyra 프레임워크 기반 1인칭 슈팅 게임 만들기](https://duplicated.tistory.com/131)
- [#1 풀바디 1인칭 카메라](https://duplicated.tistory.com/132)
- [#2 1인칭 무기 장착과 양손 잡기](https://duplicated.tistory.com/134)
- [#2-1 1인칭 리그 회귀 검사](https://duplicated.tistory.com/135)

## 구현된 것

### 풀바디 1인칭

- 카메라가 애니메이션된 머리(눈)에 붙고, 몸은 3인칭 애니메이션을 그대로 씁니다. 아래를 보면 자기 다리와 발이 보이고, 본인 그림자와 몸이 어긋나지 않습니다.
- 1인칭은 Lyra 카메라 모드 스택 위의 `ULyraCameraMode_FirstPerson`으로 구현했습니다. `PlayerCameraManager`는 바꾸지 않았습니다.
- 본인 화면 전용 메시(OnlyOwnerSee)와 다른 플레이어에게 보이는 3인칭 메시를 분리했습니다. 1인칭 보정은 로컬 소유 Pawn에서만 돌아서 다른 클라이언트에는 영향이 없습니다.

### 1인칭 무기와 양손 잡기

- 장비 변경 이벤트를 받아 본인 화면 전용 총을 만들고, 총을 화면에 고정한 뒤 손을 총에 고정합니다(Two Bone IK). 팔이 닿지 않으면 컨트롤 릭이 쇄골을 앞으로 돌립니다.
- 무기별 배치는 `UFpsWeaponGripProfile` 데이터 에셋으로 분리해서, 새 무기는 코드 수정 없이 프로파일만 추가합니다.
- 정조준(ADS): 1인칭 카메라 모드 안에서 줌하고 총을 가늠자 자리로 블렌드합니다. 조준 태그는 복제되어 다른 플레이어에게는 3인칭 조준 자세가 보입니다.

### 헤드리스 회귀 검사

- 1인칭 리그를 바꿀 때마다 창 없이(`UnrealEditor-Cmd -nullrhi`) 평지 테스트 레벨에서 PIE를 고정 시간 간격으로 돌리고, 몸통-총 거리·손 미끄러짐·팔꿈치 높이·발-그림자 차이 등을 숫자로 재서 통과/실패를 냅니다.
- 한 번에 약 26초(대부분 에디터 기동)로, 기존의 에디터 창 + 실시간 PIE 방식보다 훨씬 빠르게 확인합니다. 스크립트와 검사 항목은 [Tools/FpRigRegression](Tools/FpRigRegression/README.md)에 있습니다.

구조, 계산, 설정값, 새 무기 추가 절차, 알려진 제약은 [Docs/FirstPersonRig.md](Docs/FirstPersonRig.md)에 정리했습니다.

### 네트워크 이동

리슨 서버 + 클라이언트 1에서 `Net PktLag=150`, `Net PktLoss=5`를 켠 상태로 이동·점프·앉기가 서버와 같은 위치·상태로 맞는 것을 확인했습니다.

## 진행 중 / 예정

[Docs/GameDesign.md](Docs/GameDesign.md)의 범위 기준입니다.

- 히어로 3명(공격·돌격·지원)과 히어로별 스킬
- 화물 운송 매치 규칙과 그레이박스 맵
- 서버 권한 히트 판정과 랙 보상
- 데디케이티드 서버, Steam 세션, 봇

## 프로젝트 구조

| 영역 | 경로 |
|---|---|
| Lyra 코어 C++ | `Source/LyraGame/`, 에디터 `Source/LyraEditor/` |
| 1인칭·히어로·로드아웃 (Game Feature) | `Plugins/GameFeatures/FpsCore` |
| FPS 맵·플레이리스트 | `Plugins/GameFeatures/FpsMaps` (`L_FpsFiringRange_WP`, `L_FpsRigTest`) |
| Lyra Shooter 무기·능력 | `Plugins/GameFeatures/Shooter*` |
| FPS 무기 | `Content/FpsWeapons/` |
| 문서 | `Docs/` |

## 시작하기

요구 사항: Unreal Engine 5.8.3, C++ 빌드 환경(Visual Studio 2022 + 게임 개발 C++ 워크로드).

1. 저장소를 clone합니다.
2. `UE5NetworkFps.uproject`를 열고, 모듈을 빌드하라는 창이 뜨면 빌드합니다.
3. `L_FpsFiringRange_WP`를 열고 플레이합니다(FPS Experience로 들어가야 1인칭이 적용됩니다).

## 기본 조작

Lyra Enhanced Input 매핑(`/Game/Input/Mappings/IMC_Default`)을 그대로 씁니다.

| 조작 | 키 |
|---|---|
| 이동 | WASD |
| 시점 | 마우스 |
| 사격 / 정조준 | 좌클릭 / 우클릭 |
| 점프 | Space |
| 앉기 (토글) | Left Ctrl |
| 대시 | Left Shift (히어로 스킬로 교체 예정) |

## 라이선스

Lyra Starter Game 기반 콘텐츠와 코드는 Epic Games의 [Unreal Engine EULA](https://www.unrealengine.com/eula)를 따릅니다.
