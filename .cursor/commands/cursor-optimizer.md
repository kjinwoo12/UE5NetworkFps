# cursor-optimizer

`.cursor/rules` · `.cursor/skills` · `.cursor/commands` 지침을 **100점 루브릭**으로 채점하고, 지정한 **개선 범위** 안에서 승인 없이 수정한 뒤 **합계 100**까지 반복한다.

## 사용 전

메시지에 **개선 범위**를 포함한다.

| 별칭 | 범위 |
|------|------|
| `셀프 개선` | rules+skills+commands+README |
| `@.cursor` / `커서 문서 전체` | 위 + docs(있으면) |

```
/cursor-optimizer
개선 범위: .cursor/rules/
```

```
/cursor-optimizer @.cursor 커서 문서 전체 개선
```

범위가 없으면 **채점만** 하고 write하지 않는다.

## 절차

1. `.cursor/skills/cursor-optimizer/SKILL.md`를 따른다.
2. 루브릭: `.cursor/skills/cursor-optimizer/references/rubric.md`
3. 산출물:
   - `.result/cursor-optimizer.md` (회차 정본, 덮어쓰기)
   - `.result/cursor-optimizer-loop.md` (이력 누적)

## 제약

- 기본 write 대상: `.cursor/`만
- `AGENTS.md`, `.kilo/`, `.agents/`, `.roo/`, 게임 Source/Content는 `허용:` 없으면 수정 금지
