# UE5NetworkFps

Unreal Engine **5.7** 기반 **네트워크 FPS** 프로젝트입니다. Epic First/Third Person 템플릿과 Mannequin 파이프라인을 바탕으로, 현재는 **1인칭 캐릭터·카메라·Control Rig** 튜닝이 중심입니다.

| 항목 | 내용 |
|------|------|
| 엔진 | UE 5.7 (`UE5NetworkFps.uproject`) |
| 구현 | Blueprint·Content 중심 (C++ 게임 모듈은 최소 골격) |
| 상세 문서 | [`Docs/`](./Docs/README.md) |
| 에디터 자동화 | [`Plugins/McpAutomationBridge`](./Plugins/McpAutomationBridge/README.md) + Cursor MCP |

## 요구 사항

- **Unreal Engine 5.7**
- **Git** (소스·콘텐츠 버전 관리)
- **Cursor MCP (선택)** — Blueprint 조사·에디터 자동화  
  - TypeScript 브리지: **Node.js 18+**  
  - Native MCP만 쓸 경우: Node 불필요 
  - [플러그인 Quick Start](./Plugins/McpAutomationBridge/README.md)
  - [mcp cursor setup](./Docs/06-mcp-cursor-setup.md)

## 레이아웃

```
UE5NetworkFps/
├── UE5NetworkFps.uproject
├── Config/                 # Engine, Input, Game (MCP 플러그인 설정 포함)
├── Content/
│   ├── FirstPerson/        # 1P 템플릿·Anim·CtrlRig
│   ├── ThirdPerson/
│   ├── NetworkFps/         # 프로젝트 전용
│   └── …
├── Plugins/
│   └── McpAutomationBridge/
├── Source/UE5NetworkFps/   # C++ 모듈 골격
├── Docs/                   # 포트폴리오·기술 문서
├── mcp.cursor.example.json           # Native MCP 템플릿
└── mcp.cursor.node-bridge.example.json
```

## 라이선스·서드파티

- **McpAutomationBridge**: MIT — upstream [ChiR24/Unreal_mcp](https://github.com/ChiR24/Unreal_mcp)
- Epic 템플릿·Mannequin 에셋: Unreal Engine EULA에 따름

## 더 보기

- [Docs/README.md](./Docs/README.md) — 문서 목차
- [01-project-overview.md](./Docs/01-project-overview.md) — 목표·스택
- [03-first-person-character.md](./Docs/03-first-person-character.md) — 1P 캐릭터 핵심 작업
