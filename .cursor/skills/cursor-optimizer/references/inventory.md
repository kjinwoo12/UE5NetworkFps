# Cursor 지침 인벤토리 체크리스트

채점 전 아래를 수집한다. 결과는 `.result/cursor-optimizer.md`의 `## 인벤토리`에 붙인다.

## 수집 명령 (예시)

```bash
# rules
find .cursor/rules -type f \( -name '*.mdc' -o -name '*.md' \) | sort

# skills
find .cursor/skills -name 'SKILL.md' | sort

# commands
find .cursor/commands -type f | sort

# 보조 감사
py .cursor/skills/context-health/scripts/audit_context.py
```

## rules 표

| 파일 | alwaysApply | globs | bytes | tokens≈ |
|------|-------------|-------|-------|---------|
| … | | | | |

`T_always` = alwaysApply true 행의 tokens 합.

## skills 표

| skill | description 유무 | 줄 수 | references? | 깨진 링크 |
|-------|------------------|-------|-------------|-----------|
| … | | | | |

## commands 표

| command | 연결 skill | 비고 |
|---------|------------|------|
| … | | |

## 경계 스모크

```bash
rg -n 'AGENTS\\.md|\\.kilo/|\\.agents/|\\.roo/' .cursor/skills .cursor/rules .cursor/commands || true
```

- **읽기·링크만** → 경계 OK에 가깝다.
- **수정·이동·심링크·삭제 지시** → 축4 감점.
