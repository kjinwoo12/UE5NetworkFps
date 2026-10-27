# 0001. Lyra Starter Game을 기반으로 한다

- 상태: 채택
- 날짜: 2026-06-20

## 배경

처음에는 UE First Person 템플릿 위에 C++ `FpsCharacter`를 직접 만들었다. 목표는 오버워치식 팀 대전 네트워크 FPS([GameDesign.md](../GameDesign.md))라서 스킬, 복제, 장비, 입력, 게임 모드 같은 기반이 많이 필요했고, 템플릿에는 그중 거의 아무것도 없었다.

## 결정

Lyra Starter Game을 프로젝트 기반으로 쓴다. GAS(Gameplay Ability System), ReplicationGraph, Game Features, Enhanced Input, 장비·인벤토리, 카메라 모드 구조를 그대로 가져온다. 게임 고유 코드는 Game Feature 플러그인 `FpsCore`(`FpsCoreRuntime`)에 둔다.

- `.uproject` 이름은 `UE5NetworkFps`로 바꾸고, C++ 모듈·타깃은 `LyraGame` 그대로 둔다(이름을 바꾸면 리다이렉트와 에셋 참조 정리가 커진다).
- Lyra 코어 수정은 꼭 필요한 곳만, 작게 한다(예: 장비 변경 알림, 카메라 1인칭 스케일 전달).

## 대안

- **First Person 템플릿 + 직접 구현:** 복제·스킬·장비를 전부 새로 만들어야 해서 게임 규칙까지 가는 길이 너무 길다.
- **다른 샘플(예: 외부 FPS 템플릿):** 언리얼 공식 지원과 문서가 Lyra만큼 없다.

## 결과

- 이동·점프·앉기·복제는 처음부터 동작했다(리슨 서버 + 클라, 지연 150ms·손실 5%에서 위치 일치).
- 대신 Lyra가 3인칭 기준이라 1인칭 몸·팔·카메라는 직접 만들어야 했다([0004](0004-full-body-first-person.md) 이후).
- 저장소가 커서 큰 텍스처는 GitHub 파일 크기 제한에 맞춰 줄였다.
- 엔진은 이후 UE 5.8.3으로 올렸다(빌드 설정 V7).
