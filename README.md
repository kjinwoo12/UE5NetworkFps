# UE5NetworkFps

Unreal Engine **5.7** 기반 **Lyra** 샘플를 출발점으로 하는 **1인칭 네트워크 FPS** 프로젝트입니다.

Epic First Person 템플릿 기반 작업에서 전환했으며, Lyra의 **GAS·Enhanced Input·ReplicationGraph·Game Features·Shooter** 파이프라인 위에서 멀티플레이 FPS를 구축합니다.

| 항목 | 내용 |
|------|------|
| 엔진 | UE 5.7 (`LyraStarterGame.uproject`) |
| 베이스 | [Lyra Sample Game](https://docs.unrealengine.com/5.0/en-US/lyra-sample-game-in-unreal-engine/) |
| 목표 | 1인칭 시점 · 네트워크 FPS |
| 에디터 자동화 | [`Plugins/McpAutomationBridge`](./Plugins/McpAutomationBridge/README.md) + Cursor MCP |

## 요구 사항

- **Unreal Engine 5.7**
- **Git** (소스·콘텐츠 버전 관리)
- **Lyra 콘텐츠**
- **Cursor MCP (선택)** — Blueprint·에셋 조사·에디터 자동화
  - TypeScript 브리지: **Node.js 18+**
  - Native MCP만 쓸 경우: Node 불필요
  - [플러그인 Quick Start](./Plugins/McpAutomationBridge/README.md)
  - [MCP Cursor 설정](./Docs/06-mcp-cursor-setup.md)

## 시작하기

1. UE 5.7 설치
2. 이 repo clone
3. `LyraStarterGame.uproject` 더블클릭 → 에디터 열기
4. (선택) `Plugins/McpAutomationBridge` 활성화 후 Cursor MCP 연결
