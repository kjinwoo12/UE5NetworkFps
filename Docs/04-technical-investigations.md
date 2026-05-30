# 04. 기술 조사·설계 노트

1인칭 캐릭터 작업 중 수행한 **문제 정의 → 원인 분석 → 대안 설계** 를 포트폴리오용으로 정리합니다.

---

## 조사 1: 1인칭인데 캐릭터 몸이 보인다

### 질문

`Lvl_FirstPerson` 1P 카메라에서 왜 캐릭터 몸이 보이는가?

### 결론

**의도된 동작.** UE5 First Person 템플릿은 First Person Rendering으로 **로컬 플레이어 전신**을 그립니다.

### 근거

1. `BP_FirstPersonGameMode` → Default Pawn = `BP_FirstPersonCharacter`
2. `FirstPersonMesh` + `EFirstPersonPrimitiveType::FirstPerson`
3. 3P `Mesh`는 Owner No See, 1P 메시는 Only Owner See
4. Epic 문서: 1P 템플릿은 다리·몸통·팔이 시야에 들어올 수 있음

### 포트폴리오 서술 예시

> “1P에서 몸이 보이는 현상을 버그로 오인하지 않고, Epic First Person Rendering 파이프라인을 분석해 **전신 1P vs 클래식 FPS** 설계 차이를 문서화했다.”

---

## 조사 2: 카메라가 완전히 아래로 내려가지 않는다

### 질문

`BP_FirstPersonCharacter` 조종 시 시선이 바닥(90°)까지 안 내려가는 이유?

### 결론

입력 문제가 아니라 **`BP_FirstPersonCameraManager`의 ViewPitchMin / ViewPitchMax 클램프**.

### 데이터 흐름

```
IA_Look / IA_MouseLook
  → AddControllerPitchInput (Character)
  → PlayerCameraManager 매 프레임 Pitch 클램프
  → ViewPitchMin ~ ViewPitchMax 밖으로 못 감
```

### 템플릿 값 (대략)

| 프로퍼티 | 값 | 효과 |
|----------|-----|------|
| ViewPitchMin | ~−70° | 위 시선 제한 |
| ViewPitchMax | ~+80° | **아래 시선이 90°까지 안 감** (~10° 부족) |

주석: 1P 캐릭터 **클리핑 에러 감소** 목적.

### 트레이드오프

| ViewPitchMax ↑ (89° 근처) | ViewPitch 좁게 유지 |
|---------------------------|---------------------|
| 바닥까지 시야 확보 | 다리·무기·몸통 클리핑 증가 |
| “완전 아래” 가능 | 템플릿 기본 — 연출 안정 |

### 포트폴리오 서술 예시

> “마우스 입력·Enhanced Input을 추적한 뒤 CameraManager 클래스 디폴트를 확인해, **게임플레이 카메라 제한이 1P 클리핑 방지용**임을 규명했다.”

---

## 조사 3: 아래를 볼 때 몸이 뚫려 보인다 (클리핑)

### 질문

피치 ±90° 유지하면서 아래 시선 시 몸통·팔이 카메라 안쪽으로 겹쳐 보임 — 완화 방법?

### 원인

- 컨트롤러 피치 100% → 카메라가 가슴·목 쪽으로 이동
- `FirstPersonMesh`가 카메라와 같이 회전
- First Person Scale/FOV만으로는 **메시 관통**이 완전히 사라지지 않음

### 검토한 대안

| # | 방법 | 설명 | ±90 유지 |
|---|------|------|----------|
| A | ViewPitch 클램프 | 아래 각도 제한 | ❌ |
| B | Owner No See / 메시 숨김 | 클래식 FPS | △ (몸 안 보임) |
| C | Near clip plane | 근접면 조정 | △ (팝핑) |
| D | **몸 피치 분리** | 카메라 100%, 메시 40~70% 또는 상한 | ✅ |
| E | **spine pullback (CtrlRig)** | 템플릿 기존 기능 | ✅ |
| F | First Person Rendering 튜닝 | Scale, Primitive Type | △ |

### 채택·진행: 몸 오프셋 + 카메라 보정

**사용자 적용:**

- `FirstPersonMesh` Z = **−20**
- `FirstPersonCamera` Z = **+20**
- 루트 기준 카메라 위치 **(0,0,0)** 유지

**결과:** 지상에서는 자연스러움. **점프 시** 겹침 재발.

### 포트폴리오 서술 예시

> “카메라 각도 제한 vs 메시 오프셋 vs Control Rig 후처리를 비교하고, **시선 자유도를 유지하는 오프셋 튜닝**을 적용한 뒤 점프 구간 한계를 분리해 기록했다.”

---

## 조사 4: 몸 오프셋만으로는 점프가 안 된다

### 질문

평지는 OK인데 점프할 때 1P 메시가 카메라와 겹침 — `CtrlRig_FPWarp`로 몸통 움직임 최소화?

### 원인

| 요인 | 점프 시 동작 |
|------|--------------|
| 캡슐 | 빠르게 상승 |
| 점프 애니 | pelvis·spine 추가 상승 |
| 컴포넌트 −20 | 지상 기준선 — 공중에서 상대적으로 메시가 눈 높이 침범 |

`CtrlRig_FPWarp`는 원래 **아래 보기 spine pullback**용. 점프 전용은 아니나 **같은 본 체인**으로 확장 가능.

### 권장 설계 (우선순위)

#### 1순위: 공중에서 spine Weight 증가

```
SpineWeight = BaseWeight × Lerp(1.0, AirMultiplier, IsInAirAlpha)
```

- `IsInAirAlpha`: `IsFalling` / `NOT IsMovingOnGround` → `FInterp` 0↔1
- `AirMultiplier`: 1.5 ~ 2.5 (PIE 튜닝)
- 기존 spine pullback 노드 Weight에 곱하기

#### 2순위: 공중 Leg FABRIK off

- `IsMovingOnGround == false` → `DisableLegIK = true` 또는 FABRIK Weight = 0
- 발 IK가 pelvis를 잡아당기는 부작용 감소

#### 3순위: CameraLock / Head 강화 (공중만)

- `spine_03` ~ `head`를 `Ctrl_CameraLock` 쪽으로 더 강하게 고정
- pelvis는 애니 일부 유지 → 다리 움직임 자연스러움

#### 4순위: BodyOffset Z 추가 (리그)

- 공중에서만 `BodyOffset.Z` 추가 하향 — `FInterp`로 부드럽게

#### 5순위: 점프 시작 spine 스냅샷

- `OnJumped` / AnimNotify → 상체 포즈 저장, 공중에서 Lerp 고정
- 가장 강한 고정, 연출은 다소 경직될 수 있음

### ABP_FP_Copy 연동 (미구현·다음 작업)

`Event Blueprint Update Animation`:

1. Pawn Owner → Character Movement
2. `IsFalling`, `Velocity.Z`, `IsMovingOnGround` 읽기
3. Control Rig 파라미터: spine stabilize alpha, Leg IK disable

### 함께 확인할 항목

| 항목 | 이유 |
|------|------|
| Root Motion | 점프 RM이 pelvis 밀면 리그만으로 부족 |
| Spine additive | 키프레임 크면 Weight 더 올려야 함 |
| Mesh 부모 관계 | 카메라 자식 + 리그 Z **이중 적용** |
| 네트워크 (향후) | 1P 리그 과보정은 **로컬 오너** 위주 |

### 구현 상태 (문서 작성 시점)

| 항목 | 상태 |
|------|------|
| 몸 −20 / 카메라 +20 | ✅ 사용자 적용 (미커밋) |
| ABP ↔ 리그 IsFalling 연동 | ⏳ 설계만 |
| 점프 PIE 검증 | ⏳ |
| CtrlRig_FPWarp 튜닝 | 🔄 워킹 트리 수정 있음 |

---

## 조사 5: 개발 환경 — UnrealMCP

### 배경

`.uasset` 바이너리는 Git diff·grep만으로 구조 파악이 어렵습니다.

### 사용

- `UE5NetworkFps.uproject`에 **UnrealMCP** 플러그인 Enabled
- Cursor MCP: `read_blueprint_content`, `analyze_blueprint_graph`, `get_blueprint_function_details`
- 에디터 실행 + MCP 서버 (`unreal_mcp_server_advanced.py`) 연동

### 효과

- `BP_FirstPersonCharacter` 컴포넌트·`Aim` 그래프 확인
- CameraManager EventGraph 추적
- **에디터 없이**도 BP 구조 문서화 가능

### 포트폴리오 서술 예시

> “블루프린트 바이너리 한계를 MCP 기반 에디터 연동으로 보완해, 1P 캐릭터·카메라 매니저 의존 관계를 자동 조사했다.”

---

## 문제–해결 매트릭스 (요약)

| 증상 | 주 원인 | 적용/권장 해결 |
|------|---------|----------------|
| 1P에서 몸 보임 | First Person Rendering 설계 | 의도 이해 / 클래식 FPS로 전환 시 가시성 분리 |
| 아래 90° 안 됨 | ViewPitchMax ~80° | CameraManager 값 조정 (클리핑 트레이드오프) |
| 아래 보기 뚫림 | 카메라·메시 동일 피치 | 오프셋, spine pullback, 피치 분리 |
| 지상 OK, 점프 겹침 | 캡슐+애니 상승 | CtrlRig 공중 Weight, Leg IK off |
| 리그·오프셋 이상 | 이중 Z/transform | 컴포넌트 계층·PIE 본 위치 확인 |
