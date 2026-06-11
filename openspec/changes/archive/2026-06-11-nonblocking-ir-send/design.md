## Context

`src/main.cpp`의 `loop()`는 매 반복에서 `server.available()`로 클라이언트 1개를 꺼내 헤더 읽기 → 파싱 → 라우팅 → 응답 → `drainAndClose()`까지 동기적으로 수행한다. 이 직렬 구조에서 두 가지가 동시 요청 응답성과 무유실 처리를 해친다:

1. **동기 IR 송신** — `handleSendIr()`가 요청 처리 흐름 안에서 직접 IR을 송신한다. 특히 전체 분기(`main.cpp:418-422`)는 `for` 루프에서 15회 `sendIrSignal()` + `delay(20)`을 수행, NEC 프레임(~67ms)×15 + delay 누적으로 약 1.3~1.5초간 `loop()`가 반환하지 못한다. 이 동안 들어오는 다른 요청은 처리되지 못하거나 칩 소켓 한도를 넘겨 유실될 수 있다.
2. **긴 읽기 타임아웃** — 헤더 `millis() + 3000`(`main.cpp:537`), body `millis() + 2000`(`main.cpp:606`). 느리거나 끊긴 클라이언트 하나가 최대 3초간 다른 모든 요청을 막는다.

핵심 요구는 **모든 송신 요청을 유실 없이 처리**하는 것이다. 따라서 송신 진행 중 새 요청을 거절(409)하는 대신, HTTP 처리와 IR 송신을 분리하여 요청을 큐에 적재하고 즉시 응답한 뒤, 큐를 비블로킹으로 소비한다.

제약: SRAM 32KB, `String` 금지, 외부 라이브러리 추가 지양, 수동 JSON. `IrSender`는 단일 인스턴스를 `setSendPin()`으로 핀 전환하므로 송신은 본질적으로 직렬화된다 — 큐 소비를 단일 경로로 두면 이 제약이 오히려 자연스럽게 맞아떨어진다.

## Goals / Non-Goals

**Goals:**
- 모든 송신 요청을 큐에 적재하고 즉시 정상 응답하여 유실 없이 처리한다.
- 큐 소비(전체 송신의 모듈 간 대기 포함) 동안에도 `loop()`가 새 요청을 계속 받는다.
- `IrSender` 접근을 큐 소비 한 곳으로 일원화해 핀 전환·타이밍 경합을 제거한다.
- 단일 요청이 서버를 묶는 최대 시간(읽기 타임아웃)을 줄인다.
- 외부 라이브러리 없이, SRAM 부담 최소(고정 크기 링 버퍼)로 구현한다.

**Non-Goals:**
- 다중 소켓 동시 서비스나 비동기 I/O 프레임워크 도입.
- W5100→W5500 하드웨어 변경.
- IR 송신 자체의 병렬화(단일 `IrSender` 제약상 불가 — 큐로 직렬 처리).
- 영속 큐/재부팅 후 복구(전원 차단 시 미처리 작업은 유지하지 않음).

## Decisions

### 결정 1: 고정 크기 링 버퍼 IR 작업 큐
HTTP 핸들러는 송신 파라미터를 단위 작업으로 만들어 큐에 넣기만 한다. 작업은 단일 모듈 또는 전체(all)를 가리킨다.

```c
// 프로토콜을 char[12] 대신 enum 으로 저장해 메모리 절약 + 송신 시 분기 단순화
enum IrProtocol : uint8_t { PROTO_NEC, PROTO_SONY, PROTO_RC5, PROTO_RC6, PROTO_SAMSUNG, PROTO_UNKNOWN };

struct IrJob {
    uint8_t  target;     // 0 = 전체(all), 1..15 = 단일 모듈 id
    uint8_t  protocol;   // IrProtocol
    uint16_t address;
    uint8_t  command;
};                       // ≈ 6바이트

#define IR_QUEUE_SIZE 64 // 64 × 6B = 384B. 현실적 버스트(전체 1건 = 1엔트리) 충분 수용
IrJob   irQueue[IR_QUEUE_SIZE];
uint8_t qHead = 0, qTail = 0, qCount = 0;

// SRAM 폭주 방지 컴파일 가드 — 큐 정적 점유를 예산(예: 4KB) 이내로 강제
#define IR_QUEUE_SRAM_BUDGET 4096
static_assert(sizeof(IrJob) * IR_QUEUE_SIZE <= IR_QUEUE_SRAM_BUDGET,
              "IR_QUEUE_SIZE too large — exceeds SRAM budget; reduce it");
```

측정된 잔여 SRAM(약 29KB) 대비 64엔트리(384B)는 안전 마진이 매우 크다. `static_assert`는 향후 `IR_QUEUE_SIZE`를 무심코 키웠을 때 링크 전에 빌드를 실패시켜 스택 충돌을 예방한다(예산 4KB ≈ 약 682엔트리 상한).

- 핵심: **전체 송신도 큐 1엔트리**(`target=0`)로 저장한다. 15개 모듈로의 확장은 큐에서 꺼낸 뒤 상태머신이 수행하므로, 큐 용량을 모듈 수만큼 소모하지 않는다.
- 프로토콜은 적재 시점에 문자열→enum으로 1회 파싱해 저장.

**대안**: STL 컨테이너/동적 할당 — Arduino SRAM·단편화 위험으로 기각. 작업마다 `char protocol[12]` 보관 — 메모리 낭비, enum으로 대체.

### 결정 2: 큐 소비 비블로킹 상태머신
`loop()` 상단에서 큐를 한 단위씩 소비한다. 현재 처리 중 작업과 전체 송신의 모듈 인덱스를 전역 상태로 유지한다.

```c
static IrJob    curJob;
static bool     jobActive = false;
static uint8_t  allIndex  = 0;     // 전체 송신 진행 인덱스
static uint8_t  allOk     = 0;
static uint32_t lastSendMs = 0;

void serviceIrQueue() {
    if (!jobActive && qCount > 0) {        // 다음 작업 디큐
        curJob = dequeue();
        jobActive = true; allIndex = 0; allOk = 0;
        lastSendMs = millis() - MODULE_GAP_MS; // 첫 송신 즉시 허용
    }
    if (!jobActive) return;
    if (millis() - lastSendMs < MODULE_GAP_MS) return; // 간격 대기 — 즉시 반환

    if (curJob.target == 0) {              // 전체 송신: 모듈 1개씩
        if (sendJob(allIndex, curJob)) allOk++;
        lastSendMs = millis();
        if (++allIndex >= IR_MODULE_COUNT) jobActive = false;
    } else {                               // 단일 송신
        sendJob(curJob.target - 1, curJob);
        lastSendMs = millis();
        jobActive = false;
    }
}
```

- `loop()` 진입 직후 `serviceIrQueue()` 호출 → 송신 시점이 아니면 즉시 반환하고 곧바로 `server.available()`로 새 요청 수신. 모듈 간 간격(`MODULE_GAP_MS`)이 곧 다른 요청을 받는 창이 된다.
- `MODULE_GAP_MS` 간격은 단일 작업 사이에도 적용되어 연속 송신 시 IR 프레임이 겹치지 않게 한다.

**대안**: 기존 블로킹 `for`+`delay` 유지 — 무유실/응답성 목표 미달로 기각. RTOS 태스크/타이머 ISR — UNO R4에서 복잡도·메모리 대비 이득 적어 기각(협조적 단일 스레드 모델이 자연스러움).

### 결정 3: 송신 경로 일원화 — 핸들러는 적재만
`handleSendIr()`는 파라미터 파싱 후 `enqueue()`만 수행하고 즉시 `sendJsonOk(client, "queued")` 응답. 실제 `IrSender.setSendPin()`/`sendXxx()` 호출은 `serviceIrQueue()`에서만 발생. 이로써 단일 `IrSender`에 대한 동시 접근이 구조적으로 불가능해져 별도 락이 필요 없다(협조적 스케줄링).

### 결정 4: 큐 포화 처리
무유실이 목표지만 고정 버퍼는 상한이 있으므로 방어가 필요하다. `qCount == IR_QUEUE_SIZE`일 때만 `503`(Service Unavailable, "queue full")로 응답하고, 그 외에는 항상 적재 성공. `IR_QUEUE_SIZE`는 현실적 버스트(여러 클라이언트의 연속 클릭, 전체 송신 다건)를 넉넉히 덮도록 **64**로 설정하며 상수로 조정 가능. 상수를 키울 경우를 대비해 `static_assert` 컴파일 가드(결정 1)로 SRAM 예산 초과를 빌드 단계에서 차단한다. 포화는 정상 경로가 아닌 예외로 시리얼 로깅.

### 결정 5: 연속 중복 디듀프
`enqueue()`는 적재 직전에 "직전 작업"과 `target`·`command`가 모두 같으면 큐에 추가하지 않고 디듀프 성공으로 처리한다(클라이언트에는 동일하게 200 응답). "직전 작업"은 큐 꼬리(가장 최근 적재 = `irQueue[(qTail + IR_QUEUE_SIZE - 1) % IR_QUEUE_SIZE]`)이며, 큐가 비어 있고 송신이 진행 중이면 `curJob`을 직전 작업으로 본다.

```c
bool sameKey(const IrJob& a, const IrJob& b) {  // 키: target + protocol + address + command (전체 신호 동일성)
    return a.target == b.target && a.protocol == b.protocol &&
           a.address == b.address && a.command == b.command;
}

// enqueue() 내부, 포화검사 통과 후:
const IrJob* prev = nullptr;
if (qCount > 0)        prev = &irQueue[(qTail + IR_QUEUE_SIZE - 1) % IR_QUEUE_SIZE];
else if (jobActive)   prev = &curJob;
if (prev && sameKey(*prev, job)) return true;  // 연속 중복 → 적재 생략, 성공 응답
```

- **연속(consecutive)만** 대상: 큐 꼬리/진행 작업 1건과만 비교하므로, 사이에 다른 요청이 끼면 키가 끊겨 자연히 별도 적재된다(전체 큐 스캔 디듀프 아님 → O(1), SRAM·시간 부담 없음).
- 키는 `target`+`protocol`+`address`+`command` 전체 — 즉 **완전히 동일한 IR 신호**가 연속될 때만 병합한다. `command`가 같아도 `protocol`/`address`가 다르면 서로 다른 신호이므로 각각 적재된다.
- 디듀프는 "연속 동일 신호를 한 번으로 합치는" 의도적 병합이며, 송신은 최소 1회 보장되므로 무유실 계약과 충돌하지 않는다.

### 결정 6: 읽기 타임아웃 단축 + 상수화
헤더/body 타임아웃을 상수로 추출하고 단축한다. 예: `HEADER_READ_TIMEOUT_MS = 800`, `BODY_READ_TIMEOUT_MS = 500`. LAN 내 정상 클라이언트는 수~수십 ms에 전송을 마치므로 여유가 충분하고, 비정상 클라이언트가 서버(및 큐 소비 진행)를 묶는 상한이 3초→0.8초로 감소.

### 결정 7: 큐/진행 상태를 `GET /api/status`에 노출
상태 JSON 최상위에 `queue` 객체(예: `{"pending":3,"active":true,"target":0,"sent":7,"total":15}`)를 추가. `modules` 배열은 유지. 수동 JSON이라 필드 추가만으로 충분.

## Risks / Trade-offs

- **[큐 포화로 인한 유실 가능성]** → `IR_QUEUE_SIZE`를 버스트 대비 넉넉히(32) 설정하고 상수로 노출. 포화 시 503으로 명확히 알려 클라이언트가 재시도. 필요 시 크기만 상향.
- **[송신 지연 체감]** → 큐가 길면 마지막 작업까지 지연이 누적(작업당 ~67ms~1.5s). 즉시 200 응답과 `GET /api/status`의 `pending`/진행 노출로 사용자가 상태를 인지하도록 보완. Web UI에서 대기열 표시 가능.
- **[타임아웃 과도 단축 시 정상 요청 실패]** → 0.8s/0.5s는 LAN 기준 충분하나, 보수적으로 설정하고 시리얼 로그로 타임아웃 발생 관찰 후 상수 조정.
- **[전원 차단 시 큐 휘발]** → 미처리 작업은 재부팅 후 복구하지 않음(Non-Goal). 임베디드 IR 제어 특성상 재요청이 자연스러워 수용.
- **[링 버퍼 인덱스 경합]** → 인큐(요청 처리 흐름)와 디큐(`serviceIrQueue`)는 모두 단일 스레드 `loop()` 안에서 순차 실행되어 인터럽트 경합이 없음 → 단순 인덱스 관리로 안전. (ISR에서 큐를 건드리지 않도록 유지.)

## Migration Plan

1. `IrProtocol` enum, `IrJob` 구조체, 링 버퍼(`irQueue`, `qHead/qTail/qCount`)와 `enqueue()`/`dequeue()` 추가.
2. `MODULE_GAP_MS`, `HEADER_READ_TIMEOUT_MS`, `BODY_READ_TIMEOUT_MS`, `IR_QUEUE_SIZE` 상수 정의(타임아웃 상수화 포함).
3. `handleSendIr()`를 "파싱 → `enqueue()` → 즉시 200(queued)"로 변경. 큐 포화 시 503.
4. `serviceIrQueue()` 구현, `loop()` 진입부에서 호출. 기존 동기 송신 `for`/`delay` 제거.
5. `handleGetStatus()`에 `queue` 진행 필드 추가.
6. 빌드(`pio run`) 후 단일/전체 송신, 연속 요청 무유실, 처리 중 GET 응답성, 큐 포화 503, 타임아웃 동작 수동 검증.

롤백: 변경은 `src/main.cpp` 국소 수정 — 커밋 단위 revert로 즉시 원복(외부 의존성·API 호환 깨짐 없음, 응답 본문 message만 변화).

## Open Questions

- `IR_QUEUE_SIZE` 적정값(현재 64) — 실제 운용 버스트 규모 측정 후 조정. 컴파일 가드 예산(4KB)이 상한을 강제.
- 포화 시 503 거절 대신 가장 오래된 작업을 덮어쓰는 정책이 더 나은 환경이 있는지(현재는 503 채택, 유실 가시화 우선).
- 연속 중복 디듀프(결정 5) 키는 `target`+`protocol`+`address`+`command` 전체로 확정 — 완전히 동일한 신호만 병합하므로 다중 프로토콜·주소 혼용에서도 안전.
