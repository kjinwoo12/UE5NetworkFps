# Cursor 지침 최적화 루브릭 (100점)

근거 없는 점수 금지. 각 축은 **정수 또는 한 자리 소수**로 채점하고, **가중 합산 후 반올림해 0–100**으로 보고한다.

**토큰 추정:** `tokens ≈ ceil(UTF-8 바이트 / 4)` (본문만, frontmatter 포함). 스크립트 없이 측정 시 `wc -c` 후 `/4` 올림.  
보조: `py .cursor/skills/context-health/scripts/audit_context.py`

**평가 범위(채점):** `.cursor/rules/**` · `.cursor/skills/**` · `.cursor/commands/**` · `.cursor/README.md`  
**쓰기 범위:** 사용자가 전달한 **개선 범위**만. `@.cursor` / `커서 문서 전체` / `셀프 개선` → 위 트리.  
**기본 금지:** 루트 `AGENTS.md`, `.kilo/`, `.agents/`, `.roo/`, 게임 `Source/`·`Content/` — `허용:` 없으면 write 금지.

---

## 점수 요약 표 (필수 산출)

```markdown
| # | 축 | 배점 | 획득 | 근거 요약 |
|---|-----|------|------|-----------|
| 1 | Always-on 예산 | 25 | | |
| 2 | 중복 제거 | 20 | | |
| 3 | 적용 범위 위생 | 15 | | |
| 4 | 경계 격리 | 15 | | |
| 5 | 발견성 | 10 | | |
| 6 | 점진적 공개 | 10 | | |
| 7 | 실행 가능성 | 5 | | |
| | **합계** | **100** | **N** | |
```

---

## 1. Always-on 예산 — 25점

**질문:** 매 턴 강제 로드되는 지침 토큰이 과도하지 않은가?

**측정:** `alwaysApply: true`인 `.cursor/rules/*.mdc`의 **토큰 합** `T_always`.

| 관찰 | 점수대 |
|------|--------|
| `T_always` ≤ 800 이고 alwaysApply 파일 ≤ 3 | 22–25 |
| `T_always` ≤ 1500 또는 파일 ≤ 4 | 16–21 |
| `T_always` ≤ 3000 | 10–15 |
| `T_always` > 3000 또는 alwaysApply ≥ 6 | 0–9 |

**증거:** 파일별 bytes/tokens 표.  
**이 repo 힌트:** `project-domain`·`instruction-ecosystem` 비대 시 분리·포인터화. caveman은 얇은 always + skill 본문.

---

## 2. 중복 제거 — 20점

**질문:** `.cursor` **내부**에서 같은 표·절·정책이 여러 파일에 복붙되어 있지 않은가?

| 관찰 | 점수대 |
|------|--------|
| 동일 표·정책이 단일 정본 + 링크만 | 17–20 |
| 경미한 중복 1–2곳 (요지 반복 수준) | 11–16 |
| 동일 표/장문 절이 3곳 이상 | 0–10 |

**참고:** 루트 README·사람용 Docs와의 중복은 감점하지 않음. caveman 강제 절이 ecosystem+caveman-mode에 이중이면 축2 감점.

---

## 3. 적용 범위 위생 — 15점

**질문:** 도메인 규칙이 `globs`로 제한되고, 전역이 필요한 것만 always-on인가?

| 관찰 | 점수대 |
|------|--------|
| UE/스택 규칙이 globs+`alwaysApply: false`, always는 불변·git·문체 최소 | 13–15 |
| 일부 도메인·공정 규칙이 불필요하게 always | 7–12 |
| 대부분 always 또는 globs 누락·오설정 | 0–6 |

**증거:** 각 `.mdc` frontmatter (`alwaysApply`, `globs`).  
**정본 예:** `stack-unreal` = Source/Content/Config globs; `karpathy` = 코드 globs 후보.

---

## 4. 경계 격리 — 15점

**질문:** Cursor 지침이 타 에이전트 트리를 수정하라고 강제하지 않는가? 깨진 필수 참조가 없는가?

| 관찰 | 점수대 |
|------|--------|
| `.cursor`만 지시, 깨진 링크 0, 타 트리 write 지시 없음 | 13–15 |
| 깨진 링크·존재하지 않는 경로 참조 있으나 write 지시는 없음 | 7–12 |
| 타 에이전트 경로 수정/삭제/심링크를 스킬이 강제 | 0–6 |

---

## 5. 발견성 — 10점

**질문:** skills `description`·commands가 트리거에 충분한가? command ↔ skill 연결이 있는가?

| 관찰 | 점수대 |
|------|--------|
| `/` 커맨드와 skill 정합, description에 when-to-use 포함, 죽은 커맨드 없음 | 8–10 |
| 일부 skill에 description 빈약 또는 커맨드만/스킬만 | 4–7 |
| 주요 워크플로가 발견 불가 | 0–3 |

---

## 6. 점진적 공개 — 10점

**질문:** SKILL/rule 본문이 슬림하고 상세는 `references/`인가? P/A 혼재로 always-on이 부풀어 있지 않은가?

**제외:** `.cursor/skills/skill-creator/` — 번들 장문 허용(축6 줄 수 판정에서 제외).

| 관찰 | 점수대 |
|------|--------|
| (제외 후) 장문(대략 >400줄) skill 없음, 상세는 references, always 본문 압축 | 8–10 |
| 일부 장문·혼재 | 4–7 |
| 거대 단일 파일·always에 템플릿/장문 가이드 전부 | 0–3 |

---

## 7. 실행 가능성 — 5점

**질문:** 채점 결과의 개선 항목이 `path` + 구체 조치로 바로 write 가능한가?

| 관찰 | 점수대 |
|------|--------|
| Top 개선이 path·조치·예상 점수 영향까지 명시 | 4–5 |
| 방향만 있고 path 모호 | 2–3 |
| 추상 조언만 | 0–1 |
