# 02. 콘텐츠·프로젝트 구조

## 디렉터리 개요

```
UE5NetworkFps/
├── UE5NetworkFps.uproject      # 엔진 5.7, UnrealMCP 플러그인
├── Config/                     # DefaultEngine, Input, Game, Editor
├── Content/
│   ├── FirstPerson/            # 1P 템플릿 (캐릭터·레벨·Anim·CtrlRig)
│   ├── ThirdPerson/            # 3P 템플릿
│   ├── NetworkFps/             # 프로젝트 전용 (데모 레벨 등)
│   ├── Characters/Mannequins/  # 메시·애니·리그·텍스처
│   ├── Weapons/                # Pistol, Rifle, GrenadeLauncher
│   ├── Input/                  # Enhanced Input Actions·Mapping Context
│   └── LevelPrototyping/       # 프로토타입 메시·인터랙터블
├── Source/UE5NetworkFps/       # C++ 게임 모듈 (최소)
└── Docs/                       # 본 문서
```

## Content/FirstPerson

| 경로 | 역할 |
|------|------|
| `Blueprints/BP_FirstPersonCharacter` | 1P 플레이어 폰 — `FirstPersonMesh`, `FirstPersonCamera`, `Aim` |
| `Blueprints/BP_FirstPersonPlayerController` | `PlayerCameraManagerClass` → CameraManager BP |
| `Blueprints/BP_FirstPersonCameraManager` | ViewPitch Min/Max 등 1P 카메라 제한 |
| `Blueprints/BP_FirstPersonGameMode` | 기본 폰·게임 규칙 |
| `Anims/ABP_FP_Copy` | 1P AnimBP — Control Rig 노드 |
| `Anims/CtrlRig_FPWarp` | spine pullback, FABRIK, `BodyOffset` 등 |
| `Lvl_FirstPerson.umap` | 1P 데모 레벨 (World Partition External Actors) |

## Content/ThirdPerson

| 경로 | 역할 |
|------|------|
| `Blueprints/BP_ThirdPersonCharacter` | 3P 캐릭터 |
| `Blueprints/BP_ThirdPersonGameMode` | 3P 게임모드 |
| `Lvl_ThirdPerson.umap` | 3P 데모 레벨 |

1P·3P 템플릿을 **동시에 유지**해 시점·애니·카메라 패턴을 비교·이식하기 쉽게 했습니다.

## Content/NetworkFps

프로젝트 고유 콘텐츠 영역입니다 (Git 추적 전 일부 워킹 트리에만 존재할 수 있음).

| 경로 | 역할 |
|------|------|
| `Level/Demo/L_PlayerCharacterDemo.umap` | 플레이어 캐릭터 데모용 레벨 (작업 중) |

향후 게임 모드·맵·전용 BP는 이 트리 아래 두는 것을 권장합니다.

## Content/Characters/Mannequins

Epic Mannequin 풀 세트:

- **메시:** `SKM_Manny_Simple`, `SKM_Quinn_Simple`, `SK_Mannequin`
- **리그:** `CR_Mannequin_Body`, Foot IK, Procedural
- **애니:** Unarmed / Pistol / Rifle — Idle, Walk, Jog, Jump, Fire, Reload, Death, HitReact 등

1P·3P·향후 네트워크 캐릭터의 **공통 스켈레톤·애니 소스**로 사용합니다.

## Content/Input

Enhanced Input 기반:

- `Input/Actions/` — `IA_Look`, `IA_MouseLook`, 이동·점프 등
- `Input/Touch/` — 터치 UI (모바일 대비)

`Config/DefaultInput.ini`에는 레거시 Axis/Action 매핑(WASD, 마우스)과 Enhanced Input 클래스 지정이 함께 있습니다.

## Config 요약

### DefaultEngine.ini

- **GameDefaultMap:** OpenWorld 템플릿 (에디터 기본; 레벨별 GameMode에서 덮어씀)
- **렌더:** Lumen, VSM, Ray Tracing, Substrate, SM6 (DX12)
- **모듈 리다이렉트:** `TP_Blank` → `UE5NetworkFps`

### DefaultInput.ini

- Enhanced Input 기본 클래스
- Jump, Move, Look 축/액션 매핑 (키보드·게임패드)

## Git 커밋 히스토리 (기준)

| 커밋 | 설명 |
|------|------|
| `2645dbe` initialize | `.uproject`, Source, Config, `.gitignore` |
| `b19012f` feat: add FirstPerson, ThirdPerson | Mannequin, 무기, 1P/3P 레벨·BP, Input, LevelPrototyping |

**워킹 트리 (미커밋, 2026-05-30 기준):**

- `BP_FirstPersonCharacter`, `BP_FirstPersonCameraManager`, `CtrlRig_FPWarp` — 1P 튜닝
- `Content/NetworkFps/` — 신규
- `Config/DefaultEditor.ini` — 에디터 프리뷰 프로필 (런타임 무관)

## 조사·에셋 확인 방법

`.uasset` / `.umap`은 텍스트 diff가 어렵습니다. 이 프로젝트에서는:

1. **Unreal Editor** — Components, Class Defaults, AnimGraph
2. **UnrealMCP** — `read_blueprint_content`, `analyze_blueprint_graph`
3. **에셋 바이너리 문자열 추출** — 프로퍼티 이름·주석 힌트 (보조)

포트폴리오·문서 작성 시 **에디터 스크린샷 + MCP 조사 결과**를 함께 두면 재현성이 높아집니다.
