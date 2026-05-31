# 03. 1인칭 캐릭터 시스템

Epic **First Person Template (UE 5.7)** 를 기반으로 한 1P 캐릭터의 구조, 설계 의도, 그리고 이 프로젝트에서 진행한 튜닝을 정리합니다.

## 설계 패러다임: “전신 1인칭”

클래식 FPS(손·총만 보임)와 달리, UE5 1P 템플릿은 **First Person Rendering**으로 **로컬 플레이어 전신**을 1인칭 카메라에 그립니다.

| 패턴 | 클래식 FPS | UE5 FP 템플릿 |
|------|------------|----------------|
| 3P 메시 | Owner No See | World 표현·그림자용 |
| 1P 표현 | 팔/무기 Only Owner See | **FirstPersonMesh** + `EFirstPersonPrimitiveType::FirstPerson` |
| 시선 아래 | 보통 안 보임 | **다리·몸통이 보이는 것이 정상** |

따라서 `Lvl_FirstPerson`에서 몸이 보이는 것은 **버그가 아니라 템플릿 의도**입니다.

## BP_FirstPersonCharacter 구성

### 주요 컴포넌트

| 컴포넌트 | 역할 |
|----------|------|
| `CapsuleComponent` | 충돌·이동 (Character 기본) |
| `Mesh` / `CharacterMesh` | 3P용 스켈레탈 (`SKM_Manny_Simple`) — Owner No See 등 |
| `FirstPersonMesh` | 1P 전용 스켈레탈, AnimBP `ABP_FP_Copy` |
| `FirstPersonCamera` | 1P 시점 — First Person FOV/Scale 옵션 |

### 입력 · 시선

- Enhanced Input: `IA_Look`, `IA_MouseLook` 등
- `Aim` 함수: **`Add Controller Yaw Input`** + **`Add Controller Pitch Input`**
- 피치·요는 **PlayerController → CameraManager** 경로에서 최종 클램프

### 가시성 플래그 (템플릿 패턴)

- 3P `Mesh`: **Owner No See** (로컬 1P 카메라에 3P 메시 숨김)
- `FirstPersonMesh`: **Only Owner See** + First Person Primitive Type
- `EFirstPersonPrimitiveType::WorldSpaceRepresentation`: 그림자·반사 등

## BP_FirstPersonPlayerController · CameraManager

```
입력 (Look)
  → BP_FirstPersonCharacter::Aim
  → AddControllerPitchInput
  → PlayerCameraManager (BP_FirstPersonCameraManager)
  → ViewPitchMin / ViewPitchMax 로 클램프
  → FirstPersonCamera 최종 시선
```

`BP_FirstPersonPlayerController`의 **Player Camera Manager Class**가 `BP_FirstPersonCameraManager`를 가리킵니다.

### ViewPitch 제한 (템플릿 기본)

| 프로퍼티 | 역할 | 템플릿 대략값 |
|----------|------|----------------|
| `ViewPitchMin` | 위로 보는 최대 각도 | 약 **−70°** |
| `ViewPitchMax` | 아래로 보는 최대 각도 | 약 **+80°** |

에셋 주석: *"Modifies the pitch min and max to reduce clipping errors on FP character"*  
→ **±89°** 대신 좁혀 **1P 메시·무기가 바닥/몸에 뚫리는 클리핑**을 줄임.

**부호 참고 (UE 관례):**

- 아래 보기: Pitch **양수** 쪽 (`ViewPitchMax`)
- 위 보기: Pitch **음수** 쪽 (`ViewPitchMin`)

“완전히 바닥(90°)까지 안 내려간다” → **CameraManager 클램프**가 원인인 경우가 많습니다.

## 애니메이션 파이프라인

```
Locomotion / Jump (Mannequin 애니)
  → ABP_FP_Copy (AnimGraph)
  → Control Rig 노드
  → CtrlRig_FPWarp (포즈 후처리)
  → FirstPersonMesh 최종 포즈
```

### ABP_FP_Copy

- Mannequin 1P용 AnimBP
- **Control Rig** 슬롯에서 `CtrlRig_FPWarp` 실행
- Alpha·변수로 리그 강도 제어 가능 (점프 안정화 연동 지점)

### CtrlRig_FPWarp (템플릿·프로젝트 공통 요소)

에셋·Epic `TP_FirstPersonBP` 템플릿 분석 기준:

| 요소 | 추정 역할 |
|------|-----------|
| `BodyOffset` | 몸통 위치 보정 |
| `Ctrl_CameraLock`, `Ctrl_Head` | 카메라·머리 정렬 |
| `Ctrl_Shoulder_L/R` | 어깨 |
| `spine_01` ~ `spine_05`, `pelvis`, `head` | Set Transform + **Weight** |
| spine pullback 주석 | *"Pull back the spine and align the head. Avoids clipping through the..."* |
| Basic FABRIK | `ik_foot_*`, `ik_hand_*` |
| `DisableLegIK` / `DisableHandIK` | IK on/off |
| `TurnYawWeight`, `blendParent1` | 상체·요 블렌드 |

**원래 목적:** 아래를 볼 때 **spine·head pullback**으로 1P 클리핑 완화.  
**점프 안정화**는 같은 리그를 **공중에서 Weight만 키우는** 방식으로 확장 가능.

## 프로젝트 튜닝: 메시·카메라 Y 오프셋

### 적용한 오프셋 (사용자 에디터 작업)

| 컴포넌트 | Y 오프셋 (Relative Location) | 의도 |
|----------|------------------------------|------|
| `FirstPersonMesh` | **−20** | 몸·시선 분리 — 아래 시선 시 클리핑 완화 |
| `FirstPersonCamera` | **+20** | 카메라 보정 — **캡슐/루트 기준 시선 원점 유지** |

**축 참고:** UE 로컬 **Y = 좌우**. Details에서 Y만 조정했을 때 체감이 “아래/뒤”라면 **부모 소켓·컴포넌트 회전** 영향일 수 있음. PIE에서 pelvis **월드 이동 방향**으로 실제 분리 축을 확인한다.

**효과:**

- 지상·서기: 시선과 몸 위치가 자연스럽게 맞음
- 아래 보기: 카메라가 메시 내부로 덜 파고듦

**한계:**

- **점프 시** 캡슐 상승 + 점프 애니의 pelvis/spine 상승 → 1P 메시가 다시 **카메라와 겹침**
- 카메라 오프셋만으로는 **공중 상체 애니**를 상쇄하기 어려움 → Control Rig 쪽 보정 필요

### 주의: 이중 오프셋

`FirstPersonMesh`가 **카메라 자식**이면 컴포넌트 Y와 리그 `BodyOffset`(동축)이 **겹쳐 적용**될 수 있습니다. PIE에서 pelvis 월드 이동을 확인하며 튜닝해야 합니다.

**리그 대체:** 몸 Y 분리는 `BodyOffset.Y`로 옮길 수 있음. 카메라 Y는 BP `FirstPersonCamera` 유지.

## First Person Camera 옵션

`FirstPersonCamera`에서 템플릿이 켜는 항목:

- `bEnableFirstPersonFieldOfView`
- `bEnableFirstPersonScale`

장면 FOV와 1P 메시 FOV/스케일을 분리해 **팔·어깨 클리핑**을 줄이지만, **시야에 몸 일부가 들어오는 설계**는 유지됩니다.

## 레벨 연결

`Lvl_FirstPerson`:

- GameMode → `BP_FirstPersonGameMode`
- Default Pawn → `BP_FirstPersonCharacter`

`Content/NetworkFps/Level/Demo/L_PlayerCharacterDemo`는 프로젝트 전용 데모 맵으로, 동일 캐릭터·튜닝 검증용으로 확장 예정입니다.

## 클래식 FPS 스타일로 바꾸려면 (참고)

| 방법 | 설명 |
|------|------|
| `FirstPersonMesh` 비활성 / 팔만 | 전신 대신 무기·팔 메시만 |
| 3P Mesh Owner No See + 1P 팔 Only Owner See | 전통적 1P |
| 카메라 `head` 소켓 + 전방 오프셋 | True First Person 패턴 |
| ViewPitch 풀기 + 리그 pullback | 전신 유지하면서 클리핑만 완화 |

현재 프로젝트 방향은 **전신 1P 유지 + 리그·오프셋으로 품질 개선**입니다.

## 관련 에셋 경로 (에디터)

| 에셋 | Content Browser 경로 |
|------|------------------------|
| 캐릭터 | `/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter` |
| 카메라 매니저 | `/Game/FirstPerson/Blueprints/BP_FirstPersonCameraManager` |
| AnimBP | `/Game/FirstPerson/Anims/ABP_FP_Copy` |
| Control Rig | `/Game/FirstPerson/Anims/CtrlRig_FPWarp` |
| 레벨 | `/Game/FirstPerson/Lvl_FirstPerson` |
