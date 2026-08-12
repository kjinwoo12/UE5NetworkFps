# A축 이식성 검증 (add-guideline 필수)

A(**AI 사용성**) 파일은 **다른 repo에 복사해도 의미가 유지**되어야 한다.  
P(**프로젝트 맥락**) 내용은 A 파일에 **링크로 대체하지 말고** P 파일 diff로 **이동**한다.

## 통과 조건

- [ ] 본문에 **repo 이름·제품명·고유 경로** 없음
- [ ] `Plugins/GameFeatures/…`, `LyraStarterGame`, `FpsCore`, `RULE-0x`+이 repo 해석, 팀 고유 클래스명 없음
- [ ] “적용” 절이 **패턴**만 서술 — **이 repo 매핑 표 없음**
- [ ] P 규칙 링크는 **선택적 한 줄** — P 표·표를 A에 중복하지 않음
- [ ] karpathy·solid·caveman **역할 중복** 없음 (solid=구조·계약, karpathy=범위, caveman=문체)

## 금지 예 (A 파일에 넣지 않음)

| 금지 | 이유 | 넣을 곳 |
|------|------|---------|
| `ULyraCameraMode_FirstPerson` + 경로 | UE5NetworkFps 타입 | `project-domain.mdc` / `stack-unreal.mdc` |
| `/FpsCore/Camera/CM_FirstPerson` | GF 경로 | `project-domain.mdc` |
| `McpAutomationBridge` 포트·env | 이 repo MCP | `project-domain.mdc` · README |
| RULE-02 + 게임 스레드 해석 표 | 불변+매핑 | `invariant-rules.mdc` + `stack-unreal.mdc` |

## 개선 A 파일 시 순서

1. A 파일 각 bullet에 P|A 라벨
2. **P 라벨 전부** → P 대상 파일용 **별도 diff 블록** 작성 (제안 2)
3. A diff에서 P 문장 **삭제** — “project-domain 참조” 한 줄로만 대체 가능
4. 아래 **grep 스모크** 후 출력에 `A축 이식성: PASS|FAIL` 명시

## grep 스모크 (이 repo 예)

```bash
rg -i 'UE5NetworkFps|LyraStarterGame|FpsCore|FpsMaps|ShooterCore|McpAutomationBridge|LyraCameraMode_FirstPerson|Plugins/GameFeatures|RULE-0' .cursor/rules/solid-principles.mdc .cursor/rules/karpathy-guidelines.mdc .cursor/rules/caveman-mode.mdc .cursor/rules/communication-style.mdc 2>/dev/null || true
```

매칭 **0건**이 A축 목표.

## 실패 시

`A축 이식성: FAIL` → A diff **병합 금지**. P diff를 먼저 제안·반영한 뒤 A만 다시 작성.
