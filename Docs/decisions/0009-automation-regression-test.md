# 0009. 1인칭 리그 회귀 검사는 언리얼 Automation 테스트로 만든다

- 상태: 채택
- 날짜: 2026-10-27

## 배경

1인칭 리그는 한 증상을 고치면 예전 증상이 되살아나는 일이 반복됐다(몸통 통과 → 고침 → 발·그림자 어긋남 → 고침 → 몸통 통과 재발). 그래서 나왔던 증상을 전부 숫자로 재는 회귀 검사를 만들었다. 처음에는 에디터 Python 스크립트였다.

- 에디터 창을 띄우고 실시간 PIE로 돌리면 기동 17초 + 검사 36초가 걸렸고, 에디터가 백그라운드면 몇 분이 걸렸다. 사용자 PC의 창 포커스도 가져갔다.
- 그 뒤 `UnrealEditor-Cmd -nullrhi` 고정 시간 간격으로 바꿔 약 26초, 창 없이 돌게 했다. 상태는 64회 결과를 근거로 23개에서 11개로 줄였다.
- 하지만 Python 스크립트는 언리얼 표준 테스트 도구(Session Frontend, `-ExecCmds="Automation RunTests"`, CI 리포트)와 연결되지 않았다.

## 결정

회귀 검사를 C++ Automation 테스트로 옮긴다.

- 에디터 모듈 `FpsCoreEditor`에 `FpsCore.FirstPersonRig.Quick`(6상태)와 `FpsCore.FirstPersonRig.Full`(11상태)을 둔다.
- 창 없이 `UnrealEditor-Cmd -nullrhi -unattended`로 돌리고, 종료 코드로 통과 여부를 낸다.
- 앞으로 새 검사도 Automation 프레임워크로 만들고, Python 스크립트식 검사는 새로 만들지 않는다.

## 대안

- **Python 스크립트 유지:** 고치기는 쉽지만(빌드 불필요) 표준 도구와 CI에 붙이기 어렵다.
- **Gauntlet:** 여러 프로세스·디바이스 테스트에는 맞지만, 지금 검사는 에디터 한 프로세스로 충분하다.
- **Functional Test 액터(레벨 배치형):** 상태 11개를 하나의 코드 표로 관리하는 쪽이 읽기 쉽다.

## 결과

- Session Frontend와 명령줄에서 같은 테스트를 돌리고, CI에 붙일 수 있다.
- 기존 Python 검사와 PASS/FAIL이 같다(값 차이 0.1 이하, 반올림 차이).
- 검사 항목이나 한계값을 고치려면 C++ 빌드가 필요하다.
- 검사 항목과 한계: [Tools/FpRigRegression/README.md](../../Tools/FpRigRegression/README.md).
