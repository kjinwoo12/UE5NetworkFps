# Epic Low-Level Tests — 요약 참조

공식 문서 (UE 5.7+):

| 문서 | URL |
|------|-----|
| 개요 | https://dev.epicgames.com/documentation/unreal-engine/low-level-tests-in-unreal-engine |
| Explicit 테스트 타입 | https://dev.epicgames.com/documentation/unreal-engine/types-of-low-level-tests-in-unreal-engine |
| 작성 가이드 | https://dev.epicgames.com/documentation/unreal-engine/write-low-level-tests-in-unreal-engine |
| 빌드·실행 | https://dev.epicgames.com/documentation/unreal-engine/build-and-run-low-level-tests-in-unreal-engine |

## LLT vs Lyra CQTest

| | Low-Level Tests (LLT) | CQTest / Automation (ShooterTests) |
|---|---|---|
| 프레임워크 | Catch2 (`TestHarness.h`) | `CQTest.h`, `TEST_CLASS` |
| 속도 | 빠름 (초 단위) | 느림 (에디터·맵·에셋) |
| 범위 | 모듈·유닛·통합 | 기능·E2E·에디터 컨텍스트 |
| 빌드 | `TestModuleRules` + `TestTargetRules` | 게임/에디터 타겟 + `WITH_AUTOMATION_TESTS` |
| 이 스킬 | **기본** | 계획 단계에서 필요 시 별도 표기 |

## Explicit 테스트 스캐폴딩

### 모듈 (게임 모듈 `LyraGame` 테스트 예)

```
Source/LyraGameTests/
├── LyraGameTests.Build.cs    # class LyraGameTests : TestModuleRules
├── LyraGameTests.Target.cs   # class LyraGameTestsTarget : TestTargetRules
└── Private/Tests/
    └── FeatureTests.cpp
```

플러그인: `Plugins/<Name>/Tests/<Name>Tests/` (Source와 동급).

### Build.cs 핵심

```csharp
public class LyraGameTests : TestModuleRules
{
    public LyraGameTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "LyraGame" /* tested module */ });
        UpdateBuildGraphPropertiesFile(new Metadata("LyraGame", "Lyra Game"));
    }
}
```

### Target.cs — Engine 의존 시

```csharp
public LyraGameTestsTarget(TargetInfo Target) : base(Target)
{
    bUsePlatformFileStub = true;
    bMockEngineDefaults = true;
}
```

Engine 초기화가 필요하면 `Private/Tests/InitTests.cpp`:

```cpp
#include "TestCommon/Initialization.h"

GROUP_BEFORE_ALL(Catch::DefaultGroup) { InitAll(true, true); }
GROUP_AFTER_ALL(Catch::DefaultGroup)  { CleanupAll(); }
```

## Catch2 패턴 (UE)

```cpp
#include "CoreMinimal.h"
#include "TestHarness.h"
// 이후 다른 include

TEST_CASE("LyraGame::Feature::Behavior", "[unit][lyra][fast]")
{
    SECTION("valid input returns expected")
    {
        REQUIRE(Result == Expected);
    }
}
```

- 이름 충돌 시: `TEST_CASE_NAMED(FMyUniqueClass, "LyraGame::...", "[tags]")`
- `CHECK` — 실패해도 계속; `REQUIRE` — 즉시 중단
- 태그: `[unit]`, `[integration]`, `[smoke]`, `[ProductFilter]` 등
- `::` 계층으로 테스트 트리 구성

## 파일·경로 규칙

- `Alpha/Omega/SourceFile.cpp` → `Alpha/Omega/SourceFileTests.cpp` (또는 `Private/Tests/`)
- 리소스 파일: `SetResourcesFolder("TestFilesResources")` in Build.cs

## 빌드·실행 (Win64 예)

```bash
# UBT — 엔진 경로는 로컬 UE 설치에 맞게
"<UE_ROOT>/Engine/Build/BatchFiles/Build.bat" LyraGameTests Win64 Development -Project="<uproject>"

# 실행
Binaries/Win64/LyraGameTests/LyraGameTests.exe --log
```

Visual Studio: 테스트 타겟을 Development Editor 또는 Test 구성으로 빌드 후 exe 실행.

## 방법론 태그 (계획서용)

| 방법론 | LLT 적합 | 비고 |
|--------|----------|------|
| Unit | ◎ | 단일 함수·클래스, mock |
| Integration | ○ | 2+ 모듈, 최소 Engine stub |
| Smoke | ○ | 최소 수용 기준 |
| Functional | △ | 무거우면 CQTest 검토 |
| E2E | ✗ | LLT 비권장 |
