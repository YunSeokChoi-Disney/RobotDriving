# RobotDriving

Unreal Engine 5.7 기반의 **로봇 군집(Swarm) 시뮬레이션** 프로젝트입니다.
다수의 로봇이 중계소(Station)를 거쳐 통신하며 자율 주행하는 구조가 실제로 구현 가능한지를 검증하는 것을 목표로 합니다.

- **엔진**: Unreal Engine 5.7
- **개발 환경**: Blueprint + C++
- **주요 모듈**: `Sockets`, `Networking` (UE 내장 TCP 소켓)

---

## 구현 목적

**로봇 군집 시스템의 구현 가능성 확인**

여러 대의 로봇이 각자 상태 정보를 중앙으로 올려 보내고, 중앙에서 내려온 명령을 받아 행동을 바꾸는 흐름을
Unreal 환경 안에서 재현해 보고, 이런 구조가 실제로 동작하는지를 확인합니다.

### 구현 내용

1. **중계소(Relay Station) 기반 로직 구현**
   - 로봇이 서로 직접 통신하지 않고, 중계소를 경유해 메시지를 주고받는 구조
   - 중계소가 접속한 로봇에게 고유 ID를 부여하고 연결 목록을 관리

2. **TCP 프로그래밍 구현 (초기 단계)**
   - C++로 TCP 서버(중계소) / 클라이언트(로봇) 소켓 계층 작성
   - 소켓 처리는 C++에서 담당하고, 행동 로직은 Blueprint에서 처리하도록 델리게이트로 연결

---

## 구성 단계 및 진행 상황

> **진행 상황: 3단계까지 완료.**
> 2단계(물리적 움직임 구현)에서 시간을 많이 소요하여, 부득이하게 4단계까지는 진행하지 못한 상태입니다.
> 4단계의 TCP 통신 계층은 C++ 코드 수준까지 작성되어 있으나, 게임 로직과의 완전한 통합·검증은 남아 있습니다.

| 단계 | 내용 | 상태 |
| --- | --- | --- |
| **1단계** | DB 테이블 구성 및 위젯 UI 구성 | 완료 |
| **2단계** | 로봇의 물리적인 움직임 구현 (Nav Mesh Volume, Character Movement, 비헤이비어 트리 등) | 완료 |
| **3단계** | 로봇 종류별 스폰 수를 다르게 하기 위한 로봇 Spawner 블루프린트 로직 구성 | 완료 |
| **4단계** | 로봇 통신 방식 구현 (중계소 방식) | 진행 중 (초기 단계) |

### 단계별 상세

#### 1단계 — 데이터 테이블 및 UI
- `DT_RobotLibraries`, `DT_SpawnInfo` 데이터 테이블로 로봇 종류 / 스폰 정보를 관리
- `ST_RobotData`, `ST_RobotLibraries`, `ST_SpawnInfo` 구조체로 스키마 정의
- `RobotState_Widget`, `WBP_StationInfo` 위젯으로 로봇 상태와 중계소 정보를 화면에 표시

#### 2단계 — 로봇의 물리적 움직임
- Nav Mesh Volume 기반 경로 탐색
- `ACharacter` 파생 클래스 + Character Movement 로 이동 처리
- `BT_Robot` (비헤이비어 트리), `NewBlackboardData` (블랙보드), `AIC_Robot_Controller` (AI 컨트롤러) 로 행동 제어
- `BTS_UpdateRobotUI` 서비스로 주행 중 UI 갱신

#### 3단계 — 스포너
- `BP_RobotSpawner` 에서 데이터 테이블을 읽어 로봇 종류별로 서로 다른 수량을 스폰
- `BP_FleetManager` 가 스폰된 로봇들을 묶어서 관리

#### 4단계 — 중계소 방식 통신 (초기 단계)
- `ARobotStationBase` (중계소 / TCP 서버), `ARobot_Base` (로봇 / TCP 클라이언트) 를 C++로 구현
- 문자열 명령 프로토콜 (예: `CMD_SET_EVADE_TRUE`) 을 받아 Blueprint 이벤트로 전달

---

## 통신 구조

```
        ┌──────────────────────────┐
        │  ARobotStationBase       │   중계소 (TCP Server)
        │  - FTcpListener          │   Port 지정 후 StartServer()
        │  - TMap<ID, FSocket*>    │   접속 로봇 ID 관리
        └───────┬─────────┬────────┘
                │         │
        ┌───────┴──┐  ┌───┴──────┐
        │ Robot #1 │  │ Robot #2 │ ...   ARobot_Base (TCP Client)
        └──────────┘  └──────────┘
```

### 중계소 — `ARobotStationBase`

| 항목 | 설명 |
| --- | --- |
| `StartServer(Port)` | 지정 포트(`0.0.0.0`)로 `FTcpListener` 를 열고 수신 대기 |
| `SendMessageToRobot(ID, Message)` | 특정 로봇 ID에게 UTF-8 문자열 전송 |
| `OnRobotConnected` | 로봇 접속 시 브로드캐스트되는 Blueprint 델리게이트 |
| `OnMessageReceived` | 로봇으로부터 메시지 수신 시 브로드캐스트 |

- 접속이 수락되면 `NextClientID` 를 증가시켜 로봇마다 고유 ID를 부여하고 `ConnectedRobots` 맵에 등록합니다.
- 0.01초 주기의 타이머(`CheckForIncomingData`)로 모든 소켓의 수신 데이터를 폴링하며, 연결이 끊긴 소켓은 정리합니다.

### 로봇 — `ARobot_Base`

`ACharacter` 를 상속하여 이동 로직과 통신 로직을 한 액터에서 처리합니다.

| 항목 | 설명 |
| --- | --- |
| `ConnectToServer(IP, Port)` | 중계소에 TCP 접속 (송·수신 버퍼 4KB) |
| `SendMessageToServer(Message)` | 개행(`\n`)을 붙여 UTF-8로 인코딩 후 전송 |
| `OnMessageReceived` | 수신 메시지를 Blueprint로 전달 |
| `OnReceiveEvadeCommand` | `CMD_SET_EVADE_TRUE` 수신 시 호출되는 `BlueprintImplementableEvent` |

- 수신 처리는 타이머에서 폴링하되, Blueprint 로직 호출은 `AsyncTask(ENamedThreads::GameThread, ...)` 로 게임 스레드에서 안전하게 실행합니다.

---

## 프로젝트 구조

```
Source/RobotDriving/
├── RobotStationBase.h / .cpp   # 중계소 (TCP 서버)
├── Robot_Base.h / .cpp         # 로봇 (TCP 클라이언트, ACharacter 파생)
└── RobotDriving.Build.cs       # Sockets, Networking 모듈 의존성

Content/
├── BluePrints/
│   ├── BP_Robot_base           # 로봇 베이스 블루프린트
│   ├── BP_RobotSpawner         # 종류별 스폰 수 제어
│   ├── BP_FleetManager         # 로봇 군집 관리
│   ├── AIC_Robot_Controller    # AI 컨트롤러
│   ├── BT_Robot / NewBlackboardData / BTS_UpdateRobotUI
│   └── RobotStation/           # BP_RobotStation, WBP_StationInfo
├── Data/
│   ├── DataTable/              # DT_RobotLibraries, DT_SpawnInfo
│   ├── Struct/                 # ST_RobotData, ST_RobotLibraries, ST_SpawnInfo
│   └── RobotState_Widget
└── Maps/RobotSimulation.umap   # 메인 시뮬레이션 레벨
```

---

## 실행 방법

1. Unreal Engine **5.7** 로 `RobotDriving.uproject` 를 엽니다.
   (C++ 모듈이 포함되어 있으므로 최초 실행 시 빌드가 필요합니다.)
2. `Content/Maps/RobotSimulation` 레벨을 로드합니다.
3. Play 를 실행하면 `BP_RobotSpawner` 가 데이터 테이블 설정에 따라 로봇을 스폰하고,
   각 로봇이 Nav Mesh 위에서 비헤이비어 트리에 따라 주행합니다.
4. 통신 테스트 시에는 중계소 액터에서 `StartServer(Port)` 를 호출한 뒤,
   로봇에서 `ConnectToServer(IP, Port)` 로 접속합니다.

---

## 남은 과제

- [ ] 중계소의 `OnConnectionAccepted` 에서 게임 스레드로 안전하게 `OnRobotConnected` 델리게이트를 브로드캐스트하도록 보완
- [ ] TCP 스트림의 메시지 경계 처리 (길이 프리픽스 또는 개행 기준 파싱) 정식 구현
- [ ] 폴링 방식 대신 수신 전용 스레드 도입 검토
- [ ] 명령 프로토콜 정식 정의 (현재는 `CMD_SET_EVADE_TRUE` 문자열 매칭 수준)
- [ ] 4단계 — 중계소 기반 군집 제어 로직을 게임 로직과 완전히 통합
