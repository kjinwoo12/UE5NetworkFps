---
name: ue-llt-tdd
description: >
  Unreal Engine Low-Level Tests (Catch2) TDD workflow for Lyra/C++ modules. Use whenever
  the user invokes /tdd-plan, /tdd-accept, or /tdd, or asks to write UE unit tests, LLT,
  Catch2 TEST_CASE, TestModuleRules, explicit test targets, or test-first C++ in this
  project—even if they do not say "TDD" or "low-level test" explicitly. Covers planning
  test cases to .result/tdd-plan.md, generating test scaffolding, and implementing code
  to pass tests. Do NOT use for Blueprint-only work, CQTest/ShooterTests functional tests,
  or PIE manual verification unless the plan explicitly branches to those.
disable-model-invocation: true
---

# UE Low-Level Tests — TDD

Epic [Low-Level Tests](https://dev.epicgames.com/documentation/unreal-engine/low-level-tests-in-unreal-engine) (Catch2) 기반 TDD. 상세 규칙·빌드 명령은 `references/epic-llt-summary.md` 참고.

**프로젝트:** `LyraStarterGame.uproject`, `Source/LyraGame/`, 플러그인은 `Plugins/*/Tests/`.

## 명령 라우팅

| 사용자 입력 | 모드 | 구현 코드 | 테스트 코드 |
|-------------|------|-----------|-------------|
| `/tdd-plan <내용>` | plan | **금지** | **금지** |
| `/tdd-accept` | accept | 허용 | 허용 |
| `/tdd <내용>` | full | 허용 (plan 직후) | 허용 (plan 직후) |

`<내용>` = 구현하려는 기능·클래스·버그 수정·리팩터 범위.

---

## 모드: `/tdd-plan`

**목표:** 테스트 케이스·use case 목록을 마크다운으로 작성하고 **사용자 검토를 받기 전까지 멈춤**.

### 절차

1. **범위 파악** — 채팅·`@파일`·`Source/`에서 대상 모듈·심볼·의존성 확인. `project-domain.mdc`·`stack-unreal.mdc` 읽기.
2. **LLT 적합성 판단** — 순수 C++/모듈 로직은 LLT. 맵·Blueprint·GAS 런타임 전체 흐름은 계획서에 "CQTest/PIE 대안" 표기.
3. **`.result/` 디렉터리** — 없으면 생성.
4. **`.result/tdd-plan.md` 작성** — 아래 템플릿 **그대로** 채움.
5. **사용자에게 요약 제시** — 파일 경로 링크, 검토 포인트 3줄 이내.
6. **중단** — "검토 후 `/tdd-accept` 또는 수정 요청" 안내. **소스·테스트 파일 생성하지 않음.**

### `.result/tdd-plan.md` 템플릿

```markdown
# TDD Plan: <제목>

**요청:** <사용자 구현 내용>
**대상 모듈:** <예: LyraGame>
**테스트 모듈:** <예: LyraGameTests — 신규 | 기존>
**방법론:** <unit | integration | smoke — 복수 가능>

## 구현 요약

<2~4문장. 무엇을 만들/바꿀지>

## 전제·의존성

- <Engine stub 필요 여부, bMockEngineDefaults 등>
- <테스트하지 않는 범위>

## 테스트 케이스

| ID | 방법론 | TEST_CASE 이름 (안) | Use case | Given | When | Then | 우선순위 |
|----|--------|---------------------|----------|-------|------|------|----------|
| TC-01 | unit | `LyraGame::...` | ... | ... | ... | ... | P0 |

## 파일 계획 (accept 단계)

| 역할 | 경로 (안) |
|------|-----------|
| 테스트 모듈 Build.cs | `Source/<Module>Tests/<Module>Tests.Build.cs` |
| 테스트 Target.cs | `Source/<Module>Tests/<Module>Tests.Target.cs` |
| 테스트 본문 | `Source/<Module>Tests/Private/Tests/<Feature>Tests.cpp` |
| 구현 (신규/수정) | `<생산 코드 경로>` |

## 빌드·실행 (안)

```bash
<UBT 빌드 명령>
<테스트 exe 실행 명령>
```

## 검토 체크리스트

- [ ] 누락 use case
- [ ] LLT vs CQTest 분류 동의
- [ ] 파일 경로·모듈명 동의
```

---

## 모드: `/tdd-accept`

**목표:** 승인된 계획대로 **테스트 먼저** 작성 → **실패 확인** → **구현** → **통과**.

### 전제

- `.result/tdd-plan.md` **필수**. 없으면 중단하고 `/tdd-plan` 요청.
- 계획과 다른 사용자 지시가 있으면 **계획서 먼저 갱신** (`/tdd-plan` 재실행 또는 diff 설명).

### Red → Green → Refactor

1. **스캐폴딩** — 테스트 모듈·Target 없으면 `references/epic-llt-summary.md`대로 생성. `UpdateBuildGraphPropertiesFile`, Engine 의존 시 stub 플래그·`InitAll`/`CleanupAll`.
2. **Red** — 계획의 TC-ID 순으로 `TEST_CASE` / `SECTION` 작성. `REQUIRE`로 기대 동작 명시. **아직 없는 API는 선언만 하거나 stub으로 컴파일 가능하게** 최소 생산 코드 골격 추가 가능.
3. **빌드 시도** — UBT 명령 실행. 실패 시 수정. (에이전트 환경에 UE 경로 없으면 사용자에게 빌드 명령 전달.)
4. **Green** — 테스트 통과하도록 생산 코드 구현. 계획 범위 밖 리팩터 금지.
5. **Refactor** — 테스트 유지하며 중복만 정리.
6. **결과 기록** — `.result/tdd-plan.md` 하단에 `## Accept 결과` 섹션 추가: 구현 파일, 테스트 파일, 빌드/실행 결과.

### 코딩 규칙

- include 순서: `CoreMinimal.h` → `TestHarness.h` → 그 외 (Epic 가이드).
- 테스트 이름: `Module::Area::Behavior`, 태그 `[unit][module][fast]`.
- Lyra 기존 `Source/LyraGame/Tests/`(Gauntlet)와 **혼동 금지** — LLT는 별도 `*Tests` 모듈.
- RULE-02: 테스트 코드에서 sync I/O·장시간 sleep 금지.

---

## 모드: `/tdd` (통합)

**목표:** plan + accept 한 흐름.

1. `/tdd-plan` 절차로 `.result/tdd-plan.md` 작성.
2. 채팅에 **짧은 요약**(테이블 1개 + TC 개수) 출력.
3. **별도 검토 대기 없이** `/tdd-accept` 절차 즉시 진행.
4. 사용자가 같은 메시지에서 "plan만" / "검토 후" 명시했으면 **plan 후 중단**.

---

## 산출물

| 산출물 | 경로 |
|--------|------|
| 계획서 | `.result/tdd-plan.md` |
| LLT 모듈 | `Source/<Module>Tests/` 또는 `Plugins/<P>/Tests/<P>Tests/` |
| 생산 코드 | 계획서 "파일 계획" |

`.result/` — 에이전트·로컬 TDD 산출물. 커밋 여부는 사용자 판단.

## 자주 하는 실수

- CQTest(`TEST_CLASS`)와 Catch2 LLT 혼용
- `TestHarness.h` 전에 다른 UE include
- Engine 모듈 테스트 시 stub 없이 에셋 로드
- `/tdd-plan`에서 구현까지 진행
- `TEST_CASE` 문자열 이름만 쓰다 링크 오류 — `TEST_CASE_NAMED` 검토

## 참고

- `references/epic-llt-summary.md` — Epic 문서 링크·스캐폴딩·빌드 요약
- Lyra CQTest 예: `Plugins/GameFeatures/ShooterTests/` (이 스킬 기본 경로 아님)
