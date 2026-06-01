# 05. 개발 로그·현재 상태·로드맵

## 타임라인

### 2026-05-30 — 프로젝트 초기화

**커밋 `2645dbe` — initialize**

- `UE5NetworkFps.uproject` (Engine 5.7)
- C++ 게임 모듈 `Source/UE5NetworkFps/` (Build.cs, Target, 기본 Module)
- `Config/`: DefaultEngine (Lumen, DX12 SM6), DefaultInput, DefaultGame
- `.gitignore` (UE 표준)

### 2026-05-30 — 템플릿 콘텐츠 추가

**커밋 `b19012f` — feat: add FirstPerson, ThirdPerson**

- **First Person:** BP 4종, `Lvl_FirstPerson`, `ABP_FP_Copy`, `CtrlRig_FPWarp`
- **Third Person:** BP 3종, `Lvl_ThirdPerson`
- **Characters/Mannequins:** 메시·리그·Pistol/Rifle/Unarmed/Death 애니 전체
- **Weapons:** Pistol, Rifle, GrenadeLauncher
- **Input:** Enhanced Input Actions·Touch
- **LevelPrototyping:** Door, JumpPad, Target 등
- `DefaultInput.ini`: Enhanced Input + WASD/Look 매핑 보강

### 2026-05-30 — 1P 캐릭터·카메라 기술 조사 (대화·에디터)

| 주제 | 산출 |
|------|------|
| 1P에서 몸 보임 | First Person Rendering — 정상 동작으로 문서화 |
| 카메라 아래 안 내려감 | `BP_FirstPersonCameraManager` ViewPitch 클램프 |
| 아래 보기 클리핑 | 대안 비교 (피치 분리, 리그, 가시성) |
| 몸 Y −20 / 카메라 Y +20 | 사용자 튜닝 적용 |
| 점프 겹침 | `CtrlRig_FPWarp` 공중 stabilize 설계 |
| McpAutomationBridge / MCP | BP 조사 워크플로 확립 |

**에이전트 코드/BP 직접 수정:** 없음 (탐구·설계 중심)  
**사용자 에디터 수정 (미커밋):**

- `BP_FirstPersonCharacter.uasset`
- `BP_FirstPersonCameraManager.uasset`
- `CtrlRig_FPWarp.uasset`

### 2026-05-30 — 프로젝트 전용 콘텐츠 시작

- `Content/NetworkFps/Level/Demo/L_PlayerCharacterDemo.umap` 추가 (untracked)

### 2026-05-30 — Cursor 에이전트 지침 갱신 (로컬)

- `.cursor/rules/project-domain.mdc` — UE5NetworkFps 도메인 반영
- `.cursor/rules/stack-unreal.mdc` — UE Content·게임 스레드 규칙 신규
- `.cursor/rules/invariant-rules.mdc` — RULE-02 게임 프로젝트 해당

### 2026-05-31 — 1P 오프셋 축·리그 지식 반영

- `@self-update` 승인 — `.cursor/rules/project-domain.mdc`, `stack-unreal.mdc`
- 오프셋 축 **Z → Y** (`FirstPersonMesh` Y −20, `FirstPersonCamera` Y +20)
- 리그 vs 컴포넌트: 몸 `BodyOffset.Y` 대체 가능, 카메라 BP 유지
- `Docs/03`, `04`, `05` 동기화

### 2026-06-01 — README·MCP 문서

- 루트 [README.md](../README.md) — 온보딩·MCP 빠른 링크
- [06-mcp-cursor-setup.md](./06-mcp-cursor-setup.md) — Cursor · Native / Node 브리지
- `mcp.cursor.example.json`, `mcp.cursor.node-bridge.example.json` — 설정 템플릿
- `Docs/01`, `02`, `04` — McpAutomationBridge 명칭 정리

---

## 현재 구현 상태

| 영역 | 완료 | 진행 중 | 미착수 |
|------|------|---------|--------|
| 프로젝트·Git·Config | ✅ | | |
| 1P/3P 템플릿·Mannequin | ✅ | | |
| Enhanced Input | ✅ | | |
| 1P 클리핑·오프셋 튜닝 | | ✅ | |
| CtrlRig 점프 stabilize | | ✅ (설계) | |
| NetworkFps 데모 레벨 | | ✅ | |
| C++ 게임플레이 | | | ⏳ |
| 멀티플레이·복제 | | | ⏳ |
| 무기·데미지 게임루프 | | | ⏳ |
| 자동 테스트 | | | ⏳ |

---

## 워킹 트리 체크리스트 (커밋 전)

- [ ] `BP_FirstPersonCharacter` — 오프셋 의도 주석 또는 문서 링크
- [ ] `CtrlRig_FPWarp` / `ABP_FP_Copy` — 점프 stabilize 연동 여부
- [ ] 점프·아래 시선 PIE 녹화 또는 스크린샷
- [ ] `Content/NetworkFps/` — 데모 레벨 GameMode·폰 지정
- [ ] `Config/DefaultEditor.ini` — 커밋 포함 여부 결정 (에디터 전용)

---

## 다음 단계 (권장 순서)

### 단기 — 1P 품질

1. **ABP_FP_Copy**에서 `IsFalling` → Control Rig Weight / `DisableLegIK`
2. 점프·착지·아래 시선 **PIE 회귀 테스트**
3. 만족스러운 값 **CtrlRig 변수·AnimBP 기본값**으로 저장
4. 위 변경 + `NetworkFps` 데모 레벨 **Git 커밋**

### 중기 — 프로젝트 전용 캐릭터

1. `Content/NetworkFps/` 아래 **전용 Character BP** (1P 설정 상속 또는 Fork)
2. `L_PlayerCharacterDemo`를 기본 플레이 맵 후보로 `DefaultEngine` 또는 GameMode에서 연결
3. 3P 관찰용 스펙터 카메라 / 디버그 UI (멀티 준비)

### 장기 — 네트워크 FPS

1. **GameMode·PlayerState· replicated movement** 설계
2. 1P 리그·메시 보정 = **로컬만** / 3P는 `Mesh` + Mannequin 애니
3. 무기·히트스캔 or 투사체 — C++ 또는 BP + **서버 authority** 결정
4. 멀티 PIE / dedicated server 테스트

---

## 포트폴리오용 “역할·기여” 문장 예시

**짧은 버전**

> UE5.7 First Person 템플릿 기반 FPS 프로토타입에서 1인칭 전신 렌더링·카메라 클램프·Control Rig 클리핑 이슈를 분석하고, 메시/카메라 오프셋 튜닝 및 점프 구간 Control Rig stabilize 설계를 수행했습니다.

**긴 버전**

> Unreal Engine 5.7으로 네트워크 FPS를 목표하는 프로젝트를 구성하고 Epic First/Third Person 템플릿과 Mannequin 파이프라인을 이식했습니다. First Person Rendering 환경에서 “몸이 보이는 이유”, CameraManager ViewPitch 제한, 아래 시선 클리pping을 체계적으로 조사했으며, FirstPersonMesh/Camera **Y** 오프셋(−20/+20) 튜닝으로 지상 클리핑을 개선했습니다. 점프 시 메시·카메라 겹침에 대해서는 CtrlRig_FPWarp의 spine pullback·FABRIK를 활용한 공중 stabilize 방안을 AnimBP 연동까지 설계했습니다. MCP(McpAutomationBridge)를 활용해 Blueprint 바이너리 자산을 에디터 연동으로 분석하는 워크플로를 정립했습니다.

---

## 참고 링크

- [01-project-overview.md](./01-project-overview.md)
- [03-first-person-character.md](./03-first-person-character.md)
- [04-technical-investigations.md](./04-technical-investigations.md)
