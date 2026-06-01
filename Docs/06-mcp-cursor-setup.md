# 06. Cursor MCP · McpAutomationBridge 설정

이 문서는 **UE5NetworkFps**에서 Cursor와 Unreal Editor를 MCP로 연결하는 방법을 정리합니다.  
플러그인 상세·전체 API는 [Plugins/McpAutomationBridge/README.md](../Plugins/McpAutomationBridge/README.md)를 참고합니다.

## 구성 요소

| 구성 | 경로·역할 |
|------|-----------|
| UE 플러그인 | `Plugins/McpAutomationBridge/` — 에디터 WebSocket 브리지 + (선택) Native HTTP MCP |
| 프로젝트 설정 | `Config/DefaultGame.ini` — `bEnableNativeMCP=True` |
| Cursor | 사용자 `~/.cursor/mcp.json` (또는 Cursor Settings → MCP) |
| Node 브리지 (방식 B) | [Unreal_mcp](https://github.com/ChiR24/Unreal_mcp) — `dist/cli.js` |

과거 문서의 **「UnrealMCP」** 표기는 이 repo의 **McpAutomationBridge + Unreal_mcp CLI** 조합을 가리킵니다.

## 사전 준비

1. **Unreal Engine 5.7**로 `UE5NetworkFps.uproject`를 연다.
2. **Edit → Plugins** → **MCP Automation Bridge** 활성화 후 에디터 재시작.
3. **Edit → Project Settings → Plugins → MCP Automation Bridge**에서 포트·Native MCP 확인:
   - WebSocket: 기본 `8090`, `8091` (브리지 방식 B)
   - Native MCP: 기본 HTTP `http://localhost:3000/mcp` (방식 A)

## 방식 A — Native MCP (Node 없음)

프로젝트에 이미 Native MCP가 켜져 있다.

```ini
; Config/DefaultGame.ini
[/Script/McpAutomationBridge.McpAutomationBridgeSettings]
bEnableNativeMCP=True
```

**Cursor `mcp.json`:**

```json
{
  "mcpServers": {
    "unreal-engine": {
      "url": "http://localhost:3000/mcp"
    }
  }
}
```

Unreal_mcp 클론 후 cli.js 사용 시

```json
{
  "mcpServers": {
    "unreal-engine": {
      "command": "node",
      "args": ["/Path/Unreal_mcp/dist/cli.js"],
      "env": {
        "UE_PROJECT_PATH": "C:/Path/To/YourProject",
        "MCP_AUTOMATION_PORT": "8091",
        "LOG_LEVEL": "info"
      }
    }
  }
}
```

1. 에디터 실행 → 상태 표시줄에 MCP 리스닝 확인 (포트 3000 등).
2. Cursor에서 MCP 서버 연결·도구 목록 확인.

## 방식 B — Node 브리지 (TypeScript CLI)

로컬에 Unreal_mcp를 빌드해 두고 Cursor가 `node …/cli.js`로 stdio MCP를 띄운 뒤, 에디터 WebSocket(**8091**)에 붙는 방식입니다.

### 1. Unreal_mcp 설치

```bash
git clone https://github.com/ChiR24/Unreal_mcp.git
cd Unreal_mcp
npm install
npm run build
```

`dist/cli.js` 경로를 기억해 둔다. (예: `D:/KimJinWoo/Unreal_mcp/dist/cli.js`)

### 2. Cursor 설정

저장소 루트의 [mcp.cursor.node-bridge.example.json](../mcp.cursor.node-bridge.example.json)을 복사해 `~/.cursor/mcp.json`에 반영하고 경로를 **본인 PC**에 맞게 수정한다. (Native만 쓸 때는 [mcp.cursor.example.json](../mcp.cursor.example.json))

| 환경 변수 | 이 프로젝트 권장 값 |
|-----------|-------------------|
| `UE_PROJECT_PATH` | `…/UE5NetworkFps/UE5NetworkFps.uproject` (**파일**까지) |
| `MCP_AUTOMATION_PORT` | `8091` (플러그인 Listen Ports와 일치) |
| `LOG_LEVEL` | `info` |

### 3. 실행 순서

1. Unreal Editor에서 프로젝트 연다.
2. Cursor MCP 재시작 (또는 IDE 재시작).
3. 채팅에서 Blueprint 조회 등 도구 호출로 연결 확인.

## 트러블슈팅

| 증상 | 확인 |
|------|------|
| MCP 도구 없음 | 에디터 실행 여부, 플러그인 Enabled |
| 연결 거부 (8091) | Project Settings Listen Ports, 방화벽 |
| Native 3000 실패 | `bEnableNativeMCP`, 포트 충돌, 다른 프로세스 점유 |
| 잘못된 프로젝트 열림 | `UE_PROJECT_PATH`가 **이 repo** `.uproject`인지 |
| Node `cli.js` 없음 | `npm run build`, `args` 경로 |

## 에이전트·문서에서의 활용

- `BP_FirstPersonCharacter`, `BP_FirstPersonCameraManager`, `ABP_FP_Copy` 등 **바이너리 BP** 구조 조사
- 포트폴리오 서술: 에디터 스크린샷 + MCP 조사 결과 병기 ([02-content-architecture.md](./02-content-architecture.md))

관련 조사 기록: [04-technical-investigations.md](./04-technical-investigations.md) 「조사 5」
