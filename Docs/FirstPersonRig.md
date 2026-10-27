# 1인칭 바디 리그와 양손 무기 잡기

> 상태: 2026-10-19 기준 구현 (UE 5.8.3). 기본은 풀바디 1인칭(true first person). 소총 1종에 적용·검증.

본인 화면의 몸·팔·총·카메라·그림자를 어떻게 만드는지 정리한다. 다른 플레이어에게 보이는 3인칭 몸은 Lyra 애니메이션 그대로이며, 이 문서의 계산은 본인 화면 전용 메시와 본인 카메라에만 적용된다.

주요 결정의 배경과 버린 대안은 [설계 결정 기록](decisions/README.md)에 있다.

**설계 원칙**

- 몸은 3인칭 애니메이션 그대로 쓴다. 다리·골반·몸통이 그림자와 정확히 같고, 아래를 보면 자기 다리와 발이 보인다.
- 카메라는 애니메이션된 머리(눈)에 붙는다. 앉기·아래 보기·몸 기울기는 애니메이션이 만든다.
- 코드는 팔만 다시 푼다. 총을 화면에 고정하고, 손은 총에 고정하고, 팔이 닿지 않으면 어깨를 앞으로 내민다.
- 1인칭 관련 변경은 회귀 검사를 전부 통과해야 끝난다(8장).

**목표 구도:** 총은 화면 오른쪽 아래, 팔은 화면 하단에만 조금 보이고, 왼손은 핸드가드를 아래에서 받친다.

## 1. 전체 구조

```
Pawn (BP_FpsHero_*)
├─ CharacterMesh0 (SKM_Manny_Invis, ABP_Mannequin_Base)        ← 전체 애니메이션의 원본, 카메라가 이 머리를 따라감
├─ B_Manny_Fps (코스메틱 차일드 액터)
│   ├─ MeshComponent   (본인: 그림자 전용 / 남: 보임, ABP_Mannequin_CopyPose)
│   └─ FirstPersonMesh (OnlyOwnerSee, ABP_Mannequin_FirstPersonCopy) ← 본인이 보는 몸 (머리 숨김)
│        └─ 로컬 전용 총 (UFpsFirstPersonWeaponComponent가 생성, hand_r에 붙음)
├─ 복제되는 총 (B_FpsRifle, CharacterMesh0의 weapon_r, 본인에게는 OwnerNoSee·그림자 없음)
└─ 컴포넌트: UFpsFirstPersonWeaponComponent, UFpsAnimLayerComponent, UFpsLoadoutComponent
```

| 역할 | 클래스 / 에셋 | 위치 |
|---|---|---|
| 모드·카메라·팔 설정 | `UFpsFirstPersonSettings` (Project Settings > Game > Fps First Person) | `FpsCoreRuntime`, 값은 `Config/DefaultGame.ini` |
| 1인칭 카메라 | `ULyraCameraMode_FirstPerson` (CM BP `CM_FirstPerson`) | `FpsCoreRuntime` |
| 1인칭 애님 BP | `ABP_Mannequin_FirstPersonCopy` (부모 `UFpsFirstPersonAnimInstance`) | `/Game/Characters/Heroes/Mannequin/Animations/` |
| 1인칭 컨트롤 릭 | `CR_Mannequin_FirstPerson` | `/Game/Characters/Heroes/Mannequin/Rig/` |
| 무기별 1인칭 배치 | `UFpsWeaponGripProfile` (예: `DA_Grip_Rifle`) | `/Game/FpsWeapons/` |
| 무기 액터 표식 | `UFpsWeaponGripComponent` (예: `B_FpsRifle`의 `Grip`) | `/Game/FpsWeapons/` |
| 로컬 총 생성·손 고정·그림자 | `UFpsFirstPersonWeaponComponent` | `FpsCoreRuntime` (Experience가 Pawn에 추가) |
| 회귀 검사 | Automation 테스트 `FpsCore.FirstPersonRig` (`FpsFirstPersonRigTest.cpp`), 실행 스크립트 `run_fp_rig_test.ps1` | `FpsCoreEditor` 모듈, `Tools/FpRigRegression/` |
| 회귀 검사 레벨 | `L_FpsRigTest` (평지, 고정 스폰) | `/FpsMaps/Maps/` |

## 2. 프레임 흐름

```
CharacterMesh0 애님 (3인칭 포즈)
FirstPersonMesh 애님 업데이트 (게임 스레드)
  UFpsFirstPersonAnimInstance::NativeUpdateAnimation
    ├─ View = (컨트롤 회전, 눈 위치 = CharacterMesh0.head + 회전 × EyeViewOffset)
    ├─ 정지 상태면 손-총 관계 캡처(잠금)
    ├─ 총·손·팔꿈치 목표 → ABP 변수 / 팔이 닿지 않으면 쇄골 회전 → 컨트롤 릭 변수
    └─ RigAlpha (풀바디 0, 레거시 1) → 컨트롤 릭 변수
애님 그래프: CopyPoseFromMesh → ControlRig(Reach) → TwoBoneIK hand_r → 회전 → TwoBoneIK hand_l → 회전
카메라 갱신: ULyraCameraMode_FirstPerson이 같은 눈 위치 공식 사용
```

애님과 카메라가 같은 함수(`UFpsFirstPersonSettings::GetTrueFirstPersonEyeLocation`)로 눈 위치를 구하므로 총이 카메라보다 늦게 따라오지 않는다.

## 3. 카메라와 몸 (풀바디)

- **눈 위치:** `CharacterMesh0`의 `EyeBone`(`head`) + 시점 회전 × `EyeViewOffset`(기본 앞 8, 위 10cm). 아래를 보면 애니메이션에서 머리가 앞으로 나가므로 카메라도 앞으로 나간다(아래 85°에서 약 28cm). 그래서 아래를 봐도 총이 몸통 앞에 있다.
- **몸:** `RigAlpha = 0`이라 컨트롤 릭의 머리 고정·어깨 회전·몸 빼기가 모두 꺼지고, 1인칭 몸은 3인칭 포즈와 같다. 발·골반은 그림자와 0cm 차이.
- **머리:** 본인 화면 메시에서만 숨긴다(`HiddenBoneNames = [head]`). 카메라와 목·쇄골 사이는 15cm 이상(검사 항목).
- 앉기 카메라 높이도 애니메이션이 정한다(레거시의 CrouchOffset은 쓰지 않음).

## 4. 양손 잡기

### 4.1 총과 주 손

총 뼈(`weapon_r`)를 시점 기준 고정 위치에 둔다. 주 손은 "총 뼈 대비 손" 관계로 총에 붙는다.

```
WeaponWorld      = (TrueFirstPersonWeaponViewRotation, TrueFirstPersonWeaponViewOffset) × View
PrimaryHandWorld = WeaponInHand⁻¹ × WeaponWorld
```

**손 고정(잠금).** Lyra 이동 애니메이션은 `weapon_r`을 손 안에서 움직인다(뒤로 걸을 때 5~9cm). 그대로 쓰면 화면에 고정된 총 위에서 손이 미끄러진다. 그래서 Pawn이 `GripCaptureSettleSeconds`(0.3초) 동안 정지(바닥, 앉지 않음, 몽타주 없음)하면 그 순간의 `WeaponInHand`와 `보조 손 in 총`을 캡처해 계속 쓴다. 로컬 총은 그때 `hand_r`에 이 관계로 다시 붙는다(`UFpsFirstPersonWeaponComponent::ApplyWeaponGrip`, 버전 확인 후 사용). 무기가 바뀌거나 애님 인스턴스가 다시 만들어지면 다시 캡처한다.

**로컬 총 동기화(이벤트).** `UFpsFirstPersonWeaponComponent`는 폴링하지 않는다. 아래 이벤트가 오면 다음 틱에 `SyncFirstPersonWeaponVisuals`를 한 번 돈다(같은 틱의 요청은 합친다).

| 이벤트 | 출처 |
|---|---|
| 빙의·컨트롤러 변경 (BeginPlay가 빙의보다 먼저 옴) | `APawn::ReceiveControllerChangedDelegate` |
| 장비 추가·제거, 클라에서 무기 액터 복제 | `ULyraEquipmentManagerComponent::OnEquipmentChanged` (EquipItem/UnequipItem, FastArray 복제, `ULyraEquipmentInstance::OnRep_SpawnedActors`) |
| 클라에서 무기 액터의 소켓 부착 복제 | 아직 안 붙은 무기 루트의 `TransformUpdated` (붙으면 해제) |
| 복제 무기 파괴 | 무기 액터 `OnEndPlay` |
| 코스메틱 교체(1인칭 메시 재생성) | `ULyraPawnComponent_CharacterParts::OnCharacterPartsChanged` |
| 1인칭 애님 인스턴스 (재)초기화 | 1인칭 메시 `OnAnimInitialized` |
| 손 고정(그립 잠금) | `UFpsFirstPersonAnimInstance::OnGripLocked` |
| 에디터에서 `UFpsFirstPersonSettings` 변경 | `OnSettingChanged` (에디터 전용) |

**무기 흔들림(스웨이).** 총이 화면에 완전히 고정돼 있으면 몸의 움직임을 팔이 전부 받아 팔꿈치가 꿈틀거렸다. 그래서 `WeaponWorld = Sway × Pinned`로, 고정 위치(Pinned)에 몸이 총을 들고 있는 방식의 변화를 조금 더한다(`ComputeWeaponSway`).

- 몸의 총 위치 = 애니메이션 `hand_r` × 상체 보정 × `WeaponInHand`, 고정 위치 기준.
- 고역 통과: 이 값의 느린 평균(`WeaponSwayCenterSpeed` 2)과의 차이만 쓴다. 오래 남는 차이(서 있을 때 3.5cm 낮게 든 총)는 화면 배치를 바꾸지 않는다.
- 크기: `WeaponSwayMaxOffset` 5cm / `WeaponSwayMaxAngle` 6°로 자르고 `WeaponFollowsBody`(0.25)를 곱한다. 1이면 뒤로 걸을 때 총구가 좌우 19cm 흔들렸다.
- 이동할 때만: 지면 속도 / `WeaponSwayFullSpeed`(150cm/s). 제자리에서 앉거나 내려다볼 때 총이 화면에서 7cm 흘러갔다.
- 빠른 회전 중에는 상체 보정 가중치(4.4)만큼 끈다(몸이 시점보다 늦어 총이 반대로 9cm 휘었다). 대신 회전 관성: 100°/s당 `WeaponTurnLag` 1°, 최대 `WeaponTurnLagMax` 2°만큼 시점보다 늦게 따라온다.
- `WeaponSwaySmoothing`(15)으로 부드럽게 한다.

### 4.2 보조 손

잠금 후에는 `보조 손 in 총` × 총, 잠금 전에는 3인칭 애니메이션의 주 손 기준 관계를 쓴다. 그 뒤 총열 축(시점 X) 둘레로 `SupportGripRollDegrees` 돌린다. 소총은 30°로 옆면 파지를 아래 받침으로 바꾼다. 피벗(`SupportGripPivotViewOffset`)은 총 위치가 바뀐 만큼 함께 옮긴다.

### 4.3 팔이 닿지 않을 때: 쇄골 회전 (Reach)

화면에 고정한 총은 몸(어깨)과 함께 움직이지 않는다. 걷거나 옆으로 가면 어깨가 앞뒤로 ±6cm 흔들리고, 왼팔이 끝까지 펴진 채 손이 총에서 6~11cm 미끄러졌다. 사람은 이때 어깨를 앞으로 내민다. 그래서:

```
Shortfall = |손 목표 - 어깨(upperarm)| - 팔 길이 × ReachArmFraction(0.95)
FullGain  = 쇄골을 목표 쪽으로 완전히 돌렸을 때 줄어드는 거리
Alpha     = clamp(Shortfall / FullGain, 0, 1)
```

C++가 팔마다 "어깨가 갈 자리"(컴포넌트 공간)를 컨트롤 릭 변수(`PrimaryReachX/Y/Z/Alpha`, `SupportReach*`)에 쓰고, 컨트롤 릭의 `FP_Reach_Clavicle_L/R`(Aim 노드)이 쇄골을 그쪽으로 돌린다. 이후 TwoBoneIK가 팔을 푼다. 풀바디에서만 켜진다.

**어깨-카메라 거리.** 처음에는 쇄골을 손 목표 쪽으로 바로 돌렸는데, 핸드가드가 화면 가운데 앞에 있어서 왼어깨가 카메라 바로 아래(17cm)까지 끌려와 걸을 때 화면 왼쪽에 어깨가 잘려 보였다. 지금은 어깨 자리를 이렇게 정한다.

- 손 쪽으로 부족한 거리만큼 옮기되, 시점 기준 좌우(Y)로는 카메라 축 쪽으로 옮기지 않는다.
- 카메라에서 `MinShoulderCameraDistance`(24cm)보다 가까우면 바깥으로 민다. 애니메이션 자체가 어깨를 가깝게 가져올 때도 같다.

**어깨 자리와 가슴 이동 (팔 모양 유지).** 손은 총을 잡고 있는데 어깨가 애니메이션대로 흔들리면 그 사이 팔꿈치가 위아래로 꿈틀거린다. 총이 흔들림(4.1)을 따라가게 한 뒤에도 마찬가지였다(손은 총을 따라가고 어깨는 그대로라서). 그래서 어깨 자리를 이렇게 정한다(`FilterShoulder`, `UpdateWeaponGrip`).

1. 애니메이션 어깨(시점 공간)를 저역 통과(`ShoulderSmoothingSpeed` 3)한다. 걸음 흔들림은 빠지고 에임 오프셋·앉기 같은 느린 변화는 지나간다. 빠른 회전 중에는 4.4의 가중치만큼 필터를 멈춰, 회전을 시작한 순간 자리에 둔다.
2. 그 자리를 무기 흔들림과 같이 옮긴다. 팔과 총이 한 덩어리로 움직이고 팔 모양은 변하지 않는다.
3. 쇄골 회전만으로는 어깨를 원하는 곳에 둘 수 없다(아래 가슴이 흔들리고 기울기 때문). 그래서 먼저 두 어깨 평균이 맞도록 1인칭 `spine_04` 위를 옮긴다(컨트롤 릭 `FP_ChestMove_*`, 변수 `ChestOffsetX/Y/Z`, 최대 `MaxChestOffset` 10cm). 빠른 회전 중 필요한 이동은 7~9cm였다(6cm로 자르면 팔꿈치가 총 대비 1cm 움직였다). 남은 차이는 쇄골 회전(위)이 맡는다.

### 4.4 상체 방향 고정 (빠른 회전)

마우스로 빠르게 돌면 Lyra 애니메이션의 상체(에임 오프셋·제자리 회전)가 시점을 30~40° 늦게 따라오고 회전 쪽으로 기운다. 그러면 왼어깨가 앞으로 나오고 왼팔꿈치가 화면 왼쪽 아래로 3~4cm 올라왔다.

그립을 잡을 때(4.1 잠금) 어깨선(upperarm_r→upperarm_l)의 방향을 시점 heading(yaw만) 기준으로 기억한다. 시점이 빠르게 돌 때만 지금 어깨선을 그 방향으로 되돌린다(`FindBetweenNormals`, 1인칭 `spine_04` 위, 컨트롤 릭 `FP_ChestYaw_*`, 변수 `ChestYawCorrection` + 축 `ChestCorrectionAxisX/Y/Z`, 최대 `MaxChestYawCorrection` 90°). 가중치는 시점 회전 속도 `ChestTurnRateStart` 60°/s에서 `ChestTurnRateFull` 180°/s까지 올라가고, 회전이 멈추면 `ChestCorrectionReleaseSpeed`(4)로 줄어든다. 서 있거나 걷거나 내려다볼 때 보정하면 오히려 나빠져서 회전 중으로 한정했다. 다리와 그림자는 애니메이션 그대로다.

### 4.5 팔꿈치

TwoBoneIK 폴 = Lerp(애니메이션 팔꿈치, 손목이 일직선이 되는 팔꿈치, `*WristStraighten`). 기본 주 손 0.5, 보조 손 1.0. 주 손을 1.0으로 하면 아래를 볼 때 팔꿈치가 화면으로 올라온다(-4.7cm, 한계 -8).

애니메이션 팔꿈치는 걸음마다 흔들려서 팔이 굽는 면을 매번 돌렸다. 그래서 폴을 총 기준 공간에서 저역 통과한다(`FilterElbowPole`, `ElbowPoleSmoothingSpeed` 3).

### 4.6 손가락

Lyra 이동 애니메이션은 걸음마다 손가락을 폈다 오므린다(뒤로·앞으로 걸을 때 엄지 마디가 손 기준 1~2cm 움직임). 3인칭에선 안 보이지만 1인칭은 손이 눈앞이라 엄지가 벌어졌다 오므려지는 게 그대로 보였다.

그립을 잡을 때(4.1 잠금) 3인칭 메시의 손가락 로컬 트랜스폼(`LockedFingerRoots` 10개 뼈와 그 아래)을 함께 캡처하고, 애님 그래프가 끝난 뒤 그 값을 덮는다(`FFpsFirstPersonAnimInstanceProxy::Evaluate_WithRoot`). 몽타주(재장전·장착)가 재생 중이면 `FingerLockBlendSpeed`(초당 10)로 풀어서 애니메이션 손가락을 쓴다. 그래프 노드를 추가하지 않고 C++ 프록시에서 처리한다.

### 4.7 애님 그래프 노드 (ABP_Mannequin_FirstPersonCopy)

| 노드 | 설정 | 입력 |
|---|---|---|
| Control Rig `CR_Mannequin_FirstPerson` | | 변수는 C++가 직접 기록 |
| Two Bone IK `hand_r` | Effector·Joint 컴포넌트 공간, `bTakeRotationFromEffectorSpace=false` | `EffectorLocation_R`, `JointTarget_R`, 알파 `bWeaponEquipped` |
| Transform (Modify) Bone `hand_r` | 회전 Replace, 컴포넌트 공간 | `RightHandRotation` |
| Two Bone IK `hand_l` | Effector·Joint 컴포넌트 공간 | `EffectorLocation_L`, `JointTarget_L`, 알파 `bWeaponEquipped` |
| Transform (Modify) Bone `hand_l` | 회전 Replace, 컴포넌트 공간 | `LeftHandRotation` |

### 4.8 정조준 (ADS)

우클릭(`IA_ADS`, `InputTag.Weapon.ADS`)이면 화면이 조금 줌되고 총이 화면 가운데 가늠자 자리로 온다. 다른 플레이어에게는 Lyra 3인칭 조준 자세가 보인다.

| 역할 | 클래스 / 에셋 |
|---|---|
| 조준 어빌리티 | `GA_Fps_ADS` (`/FpsCore/Heroes/Shared/Abilities/`). Lyra `GA_ADS` 복제본에서 3인칭 ADS 카메라(`SetCameraMode`/`ClearCameraMode`) 실행선만 끊었다. 이동 속도·감도(`IMC_ADS_Speed`)·소리·UI 알림은 그대로 |
| 어빌리티 세트 | `AbilitySet_FpsHero` (`/FpsCore/Heroes/Shared/`) = `AbilitySet_ShooterHero`에서 `GA_ADS`만 `GA_Fps_ADS`로. `HeroData_Soldier`, `HeroData_FpsGame`이 사용 |
| 조준 상태 | `UFpsFirstPersonSettings::AimTag`(`Event.Movement.ADS`, 어빌리티 활성 태그)가 Pawn ASC에 있으면 조준 (`FpsAim::IsAiming`) |
| 줌 | `ULyraCameraMode_FirstPerson`: `FieldOfView × AimFieldOfViewScale` |
| 총 이동 | `UFpsFirstPersonAnimInstance`: 총 뼈를 허리 배치 → `AimWeaponViewOffset/Rotation`으로 블렌드 |

- 카메라와 애님이 각자 `FFpsAimBlend`를 같은 프로파일 값(`AimBlendTime`, 양끝 SmoothStep)으로 돌려서 줌과 총이 같이 움직인다. 카메라는 쥔 무기의 프로파일을 Pawn에 붙은 무기 액터(`UFpsWeaponGripComponent`)에서 찾는다(조준 중이거나 돌아오는 동안만).
- Lyra `GA_ADS`를 그대로 쓰면 `CM_ThirdPersonADS`로 바뀌어 1인칭에서 3인칭 어깨 카메라가 됐다. 그래서 1인칭 카메라 모드 안에서 줌한다(카메라 모드 둘을 쌓으면 아래 모드가 빠질 때 1인칭 렌더링 설정이 한 프레임 꺼진다).
- 조준 중에는 흔들림과 회전 관성을 `AimWeaponSwayScale`(0.25)배로 줄인다(가늠자가 화면 가운데에 있어야 함).
- 조준 자세에서는 손 고정(4.1)을 캡처하지 않는다(3인칭 ADS 자세는 대기 자세가 아님).
- 보조 손 회전 피벗과 총열 축은 레거시 배치 기준 총 공간에서 구해 총과 함께 옮긴다(허리 배치에서는 이전과 같은 값).
- 3인칭: `ReplicateActivationOwnedTags=True`라 `Event.Movement.ADS`가 다른 클라이언트에 복제되고, `ABP_Mannequin_Base`의 `GameplayTag_IsADS`가 조준 자세를 고른다(본인 그림자도 조준 자세).

**소총 조준 값.** `SK_Rifle`에는 가늠자 소켓이 없어서 메시 정점으로 쟀다(`SM_Rifle` = `SK_Rifle` + (0, -17.7, -7.2)). 가늠쇠 끝과 가늠자 구멍 중심이 메시 z 16.7(SK 기준)에서 총열과 평행이고, 가늠자는 메시 y 1.5. 무기 액터는 `weapon_r`에 yaw -90으로 붙으므로 총 뼈 공간에서 가늠선은 (x, 0, 16.73), 가늠자는 x 1.5. 가늠자를 눈 앞 20cm에 두면 `AimWeaponViewOffset` = (18.5, 0, -16.73), 회전 0. PIE에서 가늠자·가늠쇠가 시점 (20, 0, 0), (52.5, 0, 0)에 왔다(오차 0.00cm).

## 5. 설정값

### 5.1 `UFpsFirstPersonSettings` (`[/Script/FpsCoreRuntime.FpsFirstPersonSettings]`)

PIE 중에도 CDO 값을 바꾸면 바로 반영된다.

| 속성 | 기본 | 의미 |
|---|---|---|
| `bTrueFirstPerson` | true | false = 레거시(6장) |
| `EyeBone`, `EyeViewOffset` | `head`, (8, 0, 10) | 눈 위치 |
| `WeaponFollowsBody` | 0.25 | 무기 흔들림 양(0 = 총 화면 고정) |
| `WeaponSwayMaxOffset`, `WeaponSwayMaxAngle` | 5cm, 6° | 흔들림 한계 |
| `WeaponSwaySmoothing`, `WeaponSwayCenterSpeed` | 15, 2 | 흔들림 스무딩, 고역 통과 기준의 따라가는 속도 |
| `WeaponSwayFullSpeed` | 150cm/s | 이 속도에서 흔들림 최대, 정지 시 0 |
| `WeaponTurnLag`, `WeaponTurnLagMax` | 1°/100°/s, 2° | 회전 관성 |
| `PrimaryWristStraighten`, `SupportWristStraighten` | 0.5, 1.0 | 팔꿈치 폴 블렌드 |
| `ElbowPoleSmoothingSpeed` | 3 | 폴 저역 통과(총 기준) |
| `LockedFingerRoots`, `FingerLockBlendSpeed` | 양손 엄지·검지~새끼 뿌리, 10/s | 손가락 고정(4.6) |
| `ReachArmFraction` | 0.95 | 이 비율을 넘으면 쇄골 회전 |
| `MinShoulderCameraDistance` | 24cm | 어깨(upperarm)와 카메라 최소 거리 |
| `ShoulderSmoothingSpeed` | 3 | 어깨 저역 통과(시점 기준) |
| `MaxChestOffset` | 10cm | 가슴 이동 최대값(0 = 끔) |
| `MaxChestYawCorrection` | 90° | 빠른 회전 때 상체 방향 보정 최대값(0 = 끔) |
| `ChestTurnRateStart`, `ChestTurnRateFull`, `ChestCorrectionReleaseSpeed` | 60, 180°/s, 4 | 상체 보정이 켜지는 회전 속도, 풀리는 속도 |
| `bFirstPersonRendering` | true | 엔진 1인칭 렌더링 레이어(7장) |
| `FirstPersonScale`, `FirstPersonFieldOfView` | 0.3, 0(장면과 같음) | 1인칭 레이어 깊이 배율, 뷰모델 FOV |
| `WorldLayerBones`, `FirstPersonLayerBones` | `thigh_l/r`, `spine_01` | 월드 레이어(다리) / 1인칭 레이어 경계 |
| `AimTag` | `Event.Movement.ADS` | 이 태그가 Pawn ASC에 있으면 정조준(4.8). 비우면 조준 안 함 |

`[/Script/FpsCoreRuntime.FpsFirstPersonAnimInstance]`: `GripCaptureSettleSeconds` = 0.3, `GripCaptureMaxTurnRate` = 45°/s(시점·상체가 이보다 빨리 돌면 캡처하지 않음. 장착 직후 대기 상태에서도 39°/s까지 흔들린다).

### 5.2 무기별 `UFpsWeaponGripProfile`

| 속성 | 소총 값 (`DA_Grip_Rifle`) | 의미 |
|---|---|---|
| `TrueFirstPersonWeaponViewOffset` | (23.7, 9.1, -25.7) | 풀바디 총 뼈 위치 (카메라 X 앞, Y 오른쪽, Z 위, cm) |
| `TrueFirstPersonWeaponViewRotation` | P -0.65, Y -3.01, R -0.95 | 총열이 시점과 거의 평행, 안쪽 3° |
| `WeaponViewOffset/Rotation` | 위와 같음 | 레거시 배치 |
| `bSupportHandFollowsAnimation` | true | 한 손 무기는 false |
| `SupportGripRollDegrees` | 30 | 보조 손을 총열 축 둘레로 돌림 |
| `SupportGripPivotViewOffset` | (47, 7.7, -16.8) | `WeaponViewOffset` 기준 총열 축 위 한 점 |
| `bCanAim` | true | 정조준 사용(끄면 줌·총 이동 없음) |
| `AimWeaponViewOffset/Rotation` | (18.5, 0, -16.73), 0 | 정조준 총 뼈 배치(가늠선이 시점 X축) |
| `AimBlendTime` | 0.2초 | 허리 ↔ 정조준 시간 |
| `AimFieldOfViewScale` | 0.8 | 정조준 시야각 배율(80° → 64°) |
| `AimWeaponSwayScale` | 0.25 | 정조준 중 흔들림·회전 관성 배율 |

## 6. 레거시 모드 (되돌리기)

`bTrueFirstPerson=False`로 두면 이전 방식으로 돌아간다. 카메라는 캡슐 눈높이, 컨트롤 릭(`RigAlpha=1`)이 머리를 카메라에 고정하고 어깨를 시점 피치만큼 돌리며, 아래를 보면 상체(`BodyOffset`)를 `LookDownBodyBack`(40cm)만큼 뒤로 민다. 이 방식은 몸통 통과와 그림자-발 어긋남을 동시에 없앨 수 없었다(골반까지 밀면 발이 그림자보다 27cm 뒤, 상체만 밀면 아래 85°에서 총이 골반을 통과). 그래서 풀바디로 바꿨다.

## 7. 1인칭 렌더링 레이어와 본인 그림자

**문제:** 총·팔은 시점에 고정돼 있고 몸·벽은 월드에 있어서, 같은 깊이 버퍼에서 그리면 몸을 돌릴 때(제자리 회전으로 하체가 늦게 따라옴) 총구가 허벅지를 통과하고, 벽에 붙으면 총이 벽 속으로 들어갔다.

**방식 (업계 표준, UE 5.5+ First Person Rendering):** 뷰모델을 별도 레이어로 그린다. 예전 게임은 뷰모델 전용 FOV나 별도 패스·깊이 압축으로 했고, UE 5.5부터 엔진이 프리미티브 단위로 지원한다. 카메라 매니저를 바꾸거나 메시 스케일을 직접 줄이는 방식은 판정·그림자·애니메이션이 함께 바뀌므로 쓰지 않는다.

- `FirstPersonMesh`(팔·상체)와 로컬 총: `FirstPersonPrimitiveType = FirstPerson`. 렌더링할 때만 카메라 쪽으로 `FirstPersonScale`(0.3)배 당겨 그린다. 화면 크기는 같고 깊이만 줄어서 벽·다리에 묻히지 않는다. 실제 위치·소켓(총구 등)은 그대로라 판정에 영향이 없다.
- 다리: `FirstPersonMesh`에서 `WorldLayerBones`(`thigh_l/r`)를 숨기고, 같은 메시의 사본 `FpsWorldLayerLegs`(본인 전용, 3인칭 몸의 CopyPose 애님이 부모 = 1인칭 몸 포즈를 복사)가 `FirstPersonLayerBones`(`spine_01`) 아래를 숨긴 채 다리만 월드 레이어로 그린다. 그래서 다리는 벽 뒤에 가려지고, 총은 항상 다리 앞에 그려진다.
- 카메라: `ULyraCameraMode_FirstPerson`이 카메라 컴포넌트의 First Person Scale/FOV를 켜고, `ULyraCameraComponent::GetCameraView`가 엔진 기본 카메라처럼 `FMinimalViewInfo`에 넘긴다(Lyra 코어 3줄 추가).
- 검사: 총구 거리 × `FirstPersonScale` < 캡슐 반지름이면 벽에 붙어도 총이 벽 앞에 그려진다.

**본인 그림자**

- 3인칭 몸(`MeshComponent`)은 `FirstPersonPrimitiveType = WorldSpaceRepresentation`: 본인에게는 안 보이고 그림자만 드리운다(엔진 방식). 1인칭 렌더링을 끄면 예전 방식(`OwnerNoSee=false` + `RenderInMainPass=false`)으로 돌아간다. `OwnerNoSee`만 쓰면 그림자도 사라지고 `bCastHiddenShadow`도 스켈레탈 메시에는 효과가 없었다.
- 3인칭 총은 `OwnerNoSee`(그림자 없음). 그림자가 1인칭 총 위로 떨어져 총이 하나 더 보였다.
- 1인칭 몸·총은 그림자를 드리우지 않는다.
- 로컬 소유 Pawn에서만 처리하므로 다른 플레이어 화면에는 영향이 없다. 끄려면 `bOwnerSeesThirdPersonShadow=false`.

## 8. 검증 (회귀 검사)

1인칭 관련 코드·에셋·설정을 바꾸면 회귀 검사([Tools/FpRigRegression](../Tools/FpRigRegression/README.md))를 돌린다. 지금까지 나왔던 증상을 모두 숫자로 잰다: 몸통-총 거리, 발-그림자 차이, 그림자·머리·렌더링 레이어 설정, 손목(3인칭 대비), 손-총 미끄러짐, 정지 시 총 화면 위치, 총열 정렬, 팔꿈치 높이, 회전 중 팔꿈치 상승, 팔꿈치 위아래 흔들림(화면 기준), 팔꿈치의 총 대비 움직임, 손가락 마디 움직임, 카메라-목·어깨 거리. 사람 확인용 캡처는 `HighResShot 1600x900`으로 찍는다(PIE 뷰포트는 납작해 손이 잘림).

- **실행:** 언리얼 Automation 테스트 `FpsCore.FirstPersonRig.Quick`/`Full`(`Plugins/GameFeatures/FpsCore/Source/FpsCoreEditor/Private/Tests/FpsFirstPersonRigTest.cpp`). 에디터에서는 Session Frontend > Automation, 명령줄에서는 `Tools/FpRigRegression/run_fp_rig_test.ps1`이 `UnrealEditor-Cmd -nullrhi ... -ExecCmds="Automation RunTests FpsCore.FirstPersonRig.Full; Quit"`를 창 없이 고정 시간 간격으로 돌리고 종료 코드로 결과를 낸다. 약 40초(대부분 에디터 기동).
- **단계:** 작업 중엔 `Quick`(6상태), 커밋 전엔 `Full`(11상태).
- **상태 수:** 23 → 11. 검사 항목은 그대로이고, 64회 결과에서 한 번도 실패하지 않았거나 남긴 상태와 항상 같이만 실패한 상태를 뺐다.
- **레벨:** `/FpsMaps/Maps/L_FpsRigTest`(200m 평지, 고정 스폰 `FpRigStart`). 사격장은 스폰 위치에 따라 벽·낙하로 결과가 흔들렸다.

2026-10-20 결과(풀바디, 소총): 손목(오른손, 9장)을 빼고 전 상태 통과. 걷기 중 엄지 마디 움직임은 3인칭 애니메이션 최대 2.1cm → 1인칭 0cm.

## 9. 알려진 제약

- **걸을 때·빠르게 돌 때 오른손목:** 그립을 대기 자세에서 잠그므로(4.1), 걷기 애니메이션이 손목을 바꾸는 만큼 3인칭 같은 프레임과 12~34° 차이 난다(한계 10°). 16:9 화면에서 오른손은 거의 화면 밖이다. 폴 값(주 손 1.0)으로는 거의 줄지 않고 아래를 볼 때 팔꿈치가 올라와서 되돌렸다. 줄이려면 1인칭 전용 이동 포즈가 필요하다.
- 소총 1종만 검증했다. 손가락 모양은 그립 잠금 순간의 3인칭 포즈다(4.6).
- 3인칭 포즈는 지난 프레임 값일 수 있다(1인칭 메시가 원본보다 먼저 평가될 때).
- 1인칭 애님 BP와 컨트롤 릭이 `/Game/Characters/`에 있다(경로 정리 대상).
- 컨트롤 릭에 Python으로 FVector 멤버 변수를 추가하면 에디터가 멈춘다(RigVM assert). 그래서 Reach 목표·가슴 축·가슴 이동은 float 3개로 넘긴다.
