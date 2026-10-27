# 1인칭 리그 회귀 검사

1인칭 몸·팔·총·카메라·그림자를 숫자로 재서 통과/실패를 내는 검사입니다. 한 증상을 고치면 예전 증상이 되살아나는 일이 반복돼서(몸통 통과 → 고침 → 발·그림자 어긋남 → 고침 → 몸통 통과 재발), 한 번이라도 나왔던 증상을 전부 항목으로 두고 1인칭 관련 변경은 모두 통과해야 끝난 것으로 봅니다. 구조와 계산은 [Docs/FirstPersonRig.md](../../Docs/FirstPersonRig.md)에 있습니다.

## 실행

```powershell
$env:UE_ENGINE_DIR = 'D:\UE_5.8'                  # 엔진 루트 (또는 -EngineDir로 지정)
.\Tools\FpRigRegression\fp_rig_headless.ps1 -Mode quick   # 작업 중 (6상태)
.\Tools\FpRigRegression\fp_rig_headless.ps1               # 커밋 전 (11상태)
```

`UnrealEditor-Cmd -nullrhi -unattended`를 숨김 창으로 띄우고, `fp_rig_headless.py`가 레벨 로드 → PIE → `fp_rig_check.py` → `fp_rig_report.py` → 에디터 종료까지 진행합니다. FAIL 줄과 소요 시간을 출력하고 `ALL PASS`면 종료 코드 0입니다. 결과는 `Saved/FpRigCheck/headless/`(`report.txt`, `result.json`, `editor.log`)에 남습니다. 전체 약 26초이고 대부분이 에디터 기동입니다.

- 고정 시간 간격(`-UseFixedTimeStep -FPS=50`)이라 실시간보다 빠르고, 같은 빌드면 결과가 매번 같습니다.
- `-nullrhi`에서는 메시가 렌더링되지 않아 기본 설정(`AlwaysTickPose`) 스켈레탈 메시가 뼈를 갱신하지 않습니다. 드라이버가 PIE 메시를 `AlwaysTickPoseAndRefreshBones`로 바꾸며, 이 처리 후 `-Rendered`(오프스크린 렌더링) 결과와 리포트가 한 줄도 다르지 않았습니다.
- PIE는 로컬 설정(리슨 서버 + 클라이언트 등)과 무관하게 `-ini:` 오버라이드로 1인 Standalone으로 돕니다. 에디터가 종료하면서 사용자 설정 파일을 덮어쓰므로 실행 전 복사본으로 되돌립니다.
- `-ExecutePythonScript`는 스크립트가 반환되자마자 에디터를 닫으므로 `-ExecCmds="py <파일>"`을 씁니다.

에디터 안에서 직접 돌릴 수도 있습니다. `L_FpsRigTest`에서 PIE를 시작한 뒤 Python 콘솔에서 `fp_rig_check.py`를 실행하고(빠른 세트는 먼저 `FP_RIG_MODE = "quick"`), `Saved/FpRigCheck/result.json`이 갱신되면 `fp_rig_report.py`를 실행합니다. 실시간 결과는 한계값 근처 항목에서 조금 다를 수 있어서 판정은 헤드리스 결과로 합니다.

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

한계값(`fp_rig_report.py`의 `LIMITS`)은 실제로 나왔던 증상에서 정했습니다. 수정이 통과하도록 한계값을 느슨하게 바꾸지 않습니다. 새 증상이 나오면 고치기 전에 그 증상을 재는 항목부터 추가합니다(FAIL로 재현 → 수정 → PASS).

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
