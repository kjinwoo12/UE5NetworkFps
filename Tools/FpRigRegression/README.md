# 1인칭 리그 회귀 검사

1인칭 몸·팔·총·카메라·그림자를 숫자로 재서 통과/실패를 내는 검사입니다. 한 증상을 고치면 예전 증상이 되살아나는 일이 반복돼서(몸통 통과 → 고침 → 발·그림자 어긋남 → 고침 → 몸통 통과 재발), 한 번이라도 나왔던 증상을 전부 항목으로 두고 1인칭 관련 변경은 모두 통과해야 끝난 것으로 봅니다. 구조와 계산은 [Docs/FirstPersonRig.md](../../Docs/FirstPersonRig.md)에 있습니다.

## 실행

검사는 언리얼 Automation 테스트 **`FpsCore.FirstPersonRig`** 입니다(`Quick` 6상태, `Full` 11상태). 코드는 [FpsFirstPersonRigTest.cpp](../../Plugins/GameFeatures/FpsCore/Source/FpsCoreEditor/Private/Tests/FpsFirstPersonRigTest.cpp)이고 FpsCore 플러그인의 에디터 모듈 `FpsCoreEditor`에 있습니다. 작업 중엔 `Quick`, 커밋 전엔 `Full`을 돌립니다.

**명령줄 (창·렌더링 없음)**

```powershell
$env:UE_ENGINE_DIR = 'D:\UE_5.8'                         # 엔진 루트 (또는 -EngineDir로 지정)
.\Tools\FpRigRegression\run_fp_rig_test.ps1 -Mode Quick   # 작업 중
.\Tools\FpRigRegression\run_fp_rig_test.ps1               # 커밋 전 (Full)
```

스크립트는 아래 명령을 숨김 창으로 실행하고, 끝나면 FAIL 줄과 소요 시간을 출력합니다. `ALL PASS`면 종료 코드 0입니다.

```
UnrealEditor-Cmd UE5NetworkFps.uproject -nullrhi -unattended -nosplash -nosound -UseFixedTimeStep -FPS=50
  -ExecCmds="Automation RunTests FpsCore.FirstPersonRig.Full; Quit" -TestExit="Automation Test Queue Empty"
  -ReportExportPath=Saved/FpRigCheck/automation
```

- 먼저 에디터를 빌드하고(`LyraEditor Win64 Development`) 이 프로젝트의 GUI 에디터를 닫습니다.
- 결과: `Saved/FpRigCheck/report_full.txt`(또는 `report_quick.txt`)에 항목마다 `PASS`/`FAIL` 한 줄과 측정값이 남고, 마지막 줄이 `ALL PASS` 또는 `N FAIL`입니다. Automation 표준 리포트는 `Saved/FpRigCheck/automation/`(`index.json`, `index.html`), 실행 로그는 `Saved/FpRigCheck/editor.log`입니다.
- 약 40초가 걸리고 대부분은 에디터 기동입니다. 고정 시간 간격(`-UseFixedTimeStep -FPS=50`)이라 검사 자체는 실시간보다 빠르고, 같은 빌드면 결과가 매번 같습니다.

**에디터 안에서**

Tools > Session Frontend > Automation 탭에서 `FpsCore.FirstPersonRig.Quick` 또는 `Full`을 체크하고 Start Tests를 누릅니다. 테스트가 `L_FpsRigTest`를 열고 PIE를 띄워 캐릭터를 자동으로 움직이며, 항목별 PASS는 정보 로그로, FAIL은 에러로 남습니다. 에디터는 실시간으로 돌아서 한계값 근처 항목(손목 등)이 명령줄 결과와 조금 다를 수 있습니다. 판정은 명령줄 결과로 합니다.

## 작동 방식

1. **준비:** `L_FpsRigTest`를 열고, 로컬 PIE 설정(리슨 서버 + 클라이언트 등)과 무관하게 1인 Standalone PIE를 띄웁니다(`RequestPlaySession`에 별도 `ULevelEditorPlaySettings`).
2. **자동 조작:** 레이턴트 커맨드가 매 프레임 한 번씩 돌며 상태 목록을 차례로 진행합니다. 상태마다 캐릭터를 고정 스폰 지점으로 순간이동시키고, 플레이어의 마우스·키 입력이 결국 호출하는 함수를 직접 부릅니다: `SetControlRotation`(시점. 회전 상태는 초당 각도 × 경과 시간), `AddMovementInput`(걷기·옆걸음), `Crouch`/`UnCrouch`. 키 입력 단계만 건너뛰므로 이동 컴포넌트·애니메이션·1인칭 보정 코드는 실제 플레이와 같은 경로를 탑니다.
3. **측정:** 상태마다 1.5초(그리고 15프레임) 기다린 뒤 15프레임 동안 1인칭 몸, 3인칭 몸(그림자), 1인칭 총의 뼈·소켓 위치를 읽어 몸통-총 거리, 발-그림자 차이, 손목 각도, 팔꿈치 높이 등을 구합니다. 그림자·렌더링 레이어 같은 설정은 한 번만 확인합니다.
4. **판정:** 아래 한계값과 비교해 항목마다 PASS/FAIL을 내고, FAIL은 Automation 에러로 올립니다.

`-nullrhi`에서는 메시가 렌더링되지 않아 기본 설정(`AlwaysTickPose`) 스켈레탈 메시가 뼈를 갱신하지 않습니다. 테스트가 PIE 메시를 `AlwaysTickPoseAndRefreshBones`로 바꾸며, 오프스크린 렌더링(`-Rendered`)으로 돌린 결과와 PASS/FAIL이 모두 같았습니다(측정값 차이 0.1 이하).

## 검사 항목

| 증상 | 검사 | 한계 |
|---|---|---|
| 아래를 볼 때 총이 몸통을 통과 | 골반·척추·허벅지 뼈 ↔ 1인칭 총 선분 최소거리 | ≥ 14cm |
| 그림자와 발 위치 어긋남 | 1인칭 vs 3인칭(그림자) `foot`/`ball` 차이 | 정지 ≤ 1cm, 이동 ≤ 3cm |
| 3인칭 총(그림자)이 1인칭에 보임 | 3인칭 총 `OwnerNoSee`, 1인칭 총·몸 그림자 없음 | 정적 검사 |
| 몸을 돌릴 때 총구가 허벅지 통과, 벽에 붙으면 총이 벽을 뚫음 | 1인칭 몸·총이 First Person 레이어, 다리는 월드 레이어 사본, 총구 × FirstPersonScale < 캡슐 반지름 | 정적 검사 |
| 캐릭터 그림자 없음 | 3인칭 몸 World Space Representation + `CastShadow` | 정적 검사 |
| 머리가 화면에 보임 | 1인칭 `head` 숨김 | 정적 검사 |
| 손목 꺾임 | 왼손목 꺾임 절대값, 양손목 3인칭 대비 증가량 | 왼손 ≤ 30°, 증가 ≤ 10° |
| 뒤로 걸을 때 손이 총에서 미끄러짐 | 상태 하나 안에서 총 기준 손 위치 변화 | ≤ 1.5cm |
| 총이 화면에서 흔들림 / 똑바로 안 잡음 | 정지 상태 총구 화면 위치 변화, 모든 상태 총열 방향 변화 | ≤ 2cm, ≤ 13° |
| 팔꿈치가 너무 위, 팔이 화면을 많이 차지 | 카메라 기준 팔꿈치 높이 | z ≤ -8cm |
| 걷거나 돌 때 팔꿈치가 위아래로 흔들림 | 이동·회전 상태 안에서 팔꿈치 높이 변화 | ≤ 1.5cm |
| 빠르게 좌우로 돌면 팔꿈치가 올라옴 | 회전 중 − 정지(stand_0) 팔꿈치 높이 | ≤ 1.5cm |
| 총이 따라 움직여도 팔꿈치가 꿈틀거림 | 이동·회전 상태 안에서 팔꿈치의 총 기준 위치 변화 | ≤ 1cm |
| 걸을 때 엄지가 벌어졌다 오므려짐 | 상태 안에서 손가락 마디의 손 기준 위치 변화 | ≤ 0.3cm |
| 걸을 때 화면 왼쪽에 어깨가 잘려 보임 | 카메라 ↔ upperarm·upperarm_twist_01 거리 | ≥ 20cm |
| 카메라가 목·몸 안에 들어감 | 카메라 ↔ 목·쇄골·spine_05 거리 | ≥ 14cm |

한계값(테스트 코드의 `FpsRig::Limits`)은 실제로 나왔던 증상에서 정했습니다. 수정이 통과하도록 한계값을 느슨하게 바꾸지 않습니다. 새 증상이 나오면 고치기 전에 그 증상을 재는 항목부터 추가합니다(FAIL로 재현 → 수정 → PASS).

**상태(전체 11개):** 서기 위 60·0·아래 89, 앉아서 아래 85, 뒤로 걷기 아래 60, 앞으로 걷기 아래 85, 왼쪽·오른쪽 이동, 앉아서 뒤로 걷기 아래 60, 제자리 회전(초당 150°), 빠른 왼쪽 회전(초당 1250°). 빠른 세트(6개)는 서기 0·아래 89, 앞으로 걷기 아래 85, 왼쪽 이동, 앉아서 뒤로 걷기, 빠른 왼쪽 회전입니다. 검사 항목은 어느 세트든 전부 잽니다. 회전은 초당 각도로 주고(프레임당이면 에디터 FPS에 따라 결과가 달라짐), 상태마다 1.5초 워밍업 뒤에 잽니다.

## 검사 상태 축소

검사 항목은 하나도 빼지 않고, 같은 항목을 거의 같은 자세에서 반복 재던 상태를 23개에서 11개로 줄였습니다. 근거는 23상태 결과 64회분입니다. 뺀 상태는 한 번도 실패하지 않았거나, 실패할 때마다 남긴 상태도 같이 실패했습니다(혼자 실패 0회).

| 뺀 상태 | 64회 중 실패 | 혼자 실패 | 대신 잡는 상태 |
|---|---|---|---|
| stand_up30, stand_dn30 | 0 | 0 | stand_up60, stand_0 |
| stand_dn60 | 0 | 0 | stand_dn89 |
| stand_dn75, stand_dn85 | 3, 7 | 0 | stand_dn89 |
| crouch_0, crouch_dn60 | 0 | 0 | crouch_dn85, walk_back_crouch_dn60 |
| walk_back_0 | 9 | 0 | walk_back_dn60 |
| walk_fwd_0, walk_fwd_up30 | 7, 2 | 0 | walk_fwd_dn85 |
| turn_in_place_dn60 | 7 | 0 | turn_in_place_0 |
| turn_fast_r_0 | 1 | 0 | turn_fast_l_0 (좌우 대칭) |

빠른 세트는 남은 상태 중 혼자 실패한 횟수가 많은 것(walk_fwd_dn85 7회, walk_back_crouch_dn60 6회, turn_fast_l_0 3회, strafe_l_0 2회)에 stand_0(회전 중 팔꿈치 상승의 기준)과 stand_dn89(몸통-총 거리)를 더했습니다.

## 검사 레벨

`/FpsMaps/Maps/L_FpsRigTest`: 200m × 200m 평지, 태그 `FpRigStart`인 PlayerStart 하나, Experience는 `B_FpsGame_Default`. 상태마다 이 지점으로 순간이동한 뒤 움직이고, 한 상태 동안 15m 미만으로 움직입니다. 사격장에서는 스폰 위치에 따라 뒤로 걷다 벽에 막히거나 떨어져 결과가 흔들렸습니다.
