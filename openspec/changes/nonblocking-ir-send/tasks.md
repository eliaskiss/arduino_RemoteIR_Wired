## 1. 상수 및 자료구조 추가

- [x] 1.1 `MODULE_GAP_MS`(큐 소비 시 송신 간 간격, 기존 `delay(20)` 대체), `IR_QUEUE_SIZE`(= 64), `IR_QUEUE_SRAM_BUDGET`(예: 4096) 상수 추가
- [x] 1.2 `HEADER_READ_TIMEOUT_MS`(예: 800), `BODY_READ_TIMEOUT_MS`(예: 500) 상수 추가
- [x] 1.3 `IrProtocol` enum(NEC/SONY/RC5/RC6/SAMSUNG/UNKNOWN)과 문자열→enum 변환 헬퍼 추가
- [x] 1.4 `IrJob` 구조체(target, protocol, address, command) 정의
- [x] 1.5 `IrJob` 정의 뒤 `static_assert(sizeof(IrJob) * IR_QUEUE_SIZE <= IR_QUEUE_SRAM_BUDGET, ...)` 컴파일 가드 추가(SRAM 예산 초과 시 빌드 실패)
- [x] 1.6 링 버퍼(`irQueue`, `qHead`/`qTail`/`qCount`)와 `enqueue()`/`dequeue()`/포화검사 헬퍼 구현
- [x] 1.7 `enqueue()`에 연속 중복 디듀프 추가 — 직전 작업(큐 꼬리, 비어있으면 `curJob`)과 `target`·`protocol`·`address`·`command`가 모두 같으면 적재 생략하고 성공 반환(`sameKey()` 헬퍼)

## 2. 큐 소비 비블로킹 상태머신

- [x] 2.1 `serviceIrQueue()` 구현 — `jobActive` 아니고 큐 비어있지 않으면 디큐, 다음 작업 시작
- [x] 2.2 `millis() - lastSendMs < MODULE_GAP_MS`면 즉시 반환(간격 대기)하는 비블로킹 게이팅
- [x] 2.3 전체 작업(`target==0`): `allIndex` 모듈 1회 송신 후 인덱스/`lastSendMs` 갱신, `IR_MODULE_COUNT` 도달 시 작업 완료
- [x] 2.4 단일 작업(`target>=1`): 해당 모듈 1회 송신 후 작업 완료
- [x] 2.5 `loop()` 진입부에서 `serviceIrQueue()` 호출
- [x] 2.6 Ethernet 링크 다운 감지 시 진행 작업/큐를 정리하는 방어 로직 추가

## 3. 핸들러를 적재 전용으로 변경

- [x] 3.1 `handleSendIr()`에서 파싱 후 직접 송신(`for`+`delay`/단일 `sendIrSignal`) 제거하고 `IrJob` 생성 → `enqueue()`로 변경
- [x] 3.2 적재 성공 시 즉시 `sendJsonOk(client, "queued")`(2xx) 응답
- [x] 3.3 큐 포화(`qCount==IR_QUEUE_SIZE`) 시 503("queue full") 응답 + 시리얼 로깅
- [x] 3.4 실제 `IrSender` 호출이 `serviceIrQueue()` 한 곳에서만 일어나도록 송신 로직 정리(`sendIrSignal`/`sendJob`)

## 4. 읽기 타임아웃 단축 적용

- [x] 4.1 헤더 읽기 루프의 `millis() + 3000`을 `HEADER_READ_TIMEOUT_MS` 상수 사용으로 교체
- [x] 4.2 body 읽기 루프의 `millis() + 2000`을 `BODY_READ_TIMEOUT_MS` 상수 사용으로 교체

## 5. 큐/진행 상태 API 노출

- [x] 5.1 `handleGetStatus()` 응답 최상위에 `queue` 객체(`pending`, `active`, `target`, `sent`, `total`) 추가(수동 JSON, `modules` 배열 유지)

## 6. 빌드 및 검증

- [x] 6.1 `pio run`으로 컴파일 성공 및 SRAM/Flash 사용량 확인
- [ ] 6.2 단일 송신 요청이 즉시 200(queued) 응답하고 이후 실제로 송신되는지 확인
- [ ] 6.3 전체 송신 요청이 즉시 응답되고 모듈이 `MODULE_GAP_MS` 간격으로 순차 송신되는지 확인
- [ ] 6.4 서로 다른 연속/동시 송신 요청(큐 용량 이내)이 하나도 유실되지 않고 모두 송신되는지 확인
- [ ] 6.5 완전히 동일한 신호(`target`·`protocol`·`address`·`command`) 연속 요청이 1건으로 디듀프되고, 사이에 다른 요청이 끼거나 일부 필드만 같으면 디듀프되지 않는지 확인
- [ ] 6.6 송신 진행 중 `GET /api/status`가 `queue.pending`/진행 정도를 정확히 반영하는지 확인
- [ ] 6.7 큐를 의도적으로 포화시켰을 때 503이 반환되고 기존 큐 작업은 정상 처리되는지 확인
- [ ] 6.8 느린/끊긴 클라이언트가 단축 타임아웃 내 정리되어 다른 요청/큐 소비를 묶지 않는지 확인
