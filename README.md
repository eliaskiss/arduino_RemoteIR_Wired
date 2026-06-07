# IR Control System — REST API Server

Arduino UNO R4 Minima (Renesas RA4M1) 기반 **15채널 IR 제어 서버**.
Ethernet Shield로 REST API와 Web UI를 제공하여 15개 DFR0095 IR 모듈을 개별/일괄 제어한다.

- **보드:** Arduino UNO R4 Minima
- **실드:** Ethernet Shield (W5100/W5500) + Sensor Shield
- **IR 송신:** DFR0095 × 15 (단일 `IrSender` 인스턴스, `setSendPin()` 동적 핀 전환)
- **포트:** 80 (HTTP)

---

## 빌드 & 업로드

```bash
pio run                       # 컴파일
pio run -t upload             # 업로드
pio device monitor -b 115200  # 시리얼 모니터
```

---

## 네트워크 설정

`src/main.cpp` 상단 상수로 변경한다.

| 항목 | 기본값 |
|------|--------|
| IP | `192.168.0.3` |
| Gateway | `192.168.1.1` |
| Subnet | `255.255.255.0` |
| MAC | `DE:AD:BE:EF:FE:ED` |
| Port | `80` |

서버 기동 시 시리얼에 실제 할당 IP가 출력된다 (`HTTP server: http://...`).

핀 맵핑은 [`doc/pin_map.md`](doc/pin_map.md) 참고.

---

## REST API

모든 응답은 `application/json; charset=utf-8`, CORS 허용(`Access-Control-Allow-Origin: *`).

| Method | Endpoint | 설명 |
|--------|----------|------|
| GET | `/` | Web UI (HTML) |
| GET | `/api/status` | 전체 모듈 상태 |
| GET | `/api/ir/{id}` | 개별 모듈 상태 (id: 1~15) |
| POST | `/api/ir/{id}/send` | 개별 모듈 IR 송신 |
| POST | `/api/ir/all/send` | 전체 모듈 IR 송신 (모듈 간 20ms 간격) |
| OPTIONS | `*` | CORS preflight (204) |

### POST body — 두 가지 모드

**1) 키 프리셋 모드** (LG TV NEC 코드, `protocol=NEC`·`address=0x04` 자동)

```json
{ "key": "power" }
```

`protocol`, `address`를 함께 넣으면 오버라이드된다.

**2) 직접 지정 모드**

```json
{ "protocol": "NEC", "address": "0x04", "command": "0x08" }
```

- `address` / `command`는 16진수(`0x..`) 또는 10진수 문자열 모두 허용.
- 지원 프로토콜: `NEC`, `SONY`, `RC5`, `RC6`, `SAMSUNG`.
- `key` 또는 `command` 중 하나는 필수.

### 응답 예시

```json
// 성공
{ "status": "ok", "message": "IR signal sent" }

// 실패
{ "status": "error", "message": "Unknown key" }

// GET /api/status
{ "status": "ok", "modules": [
  { "id": 1, "pin": 0, "sendCount": 3, "lastSuccess": true },
  ...
]}
```

### 주요 에러 코드

| 코드 | 상황 |
|------|------|
| 400 Bad Request | 잘못된 모듈 ID, Unknown key, `key`/`command` 누락 |
| 404 Not Found | 매칭되는 라우트 없음 |
| 411 Length Required | POST에 `Content-Length` 헤더 없음 |
| 413 Payload Too Large | body가 255바이트 초과 |
| 500 Internal Server Error | 일괄 송신 중 일부 모듈 실패 |

---

## 키 프리셋 목록 (LG TV)

`{"key":"..."}`로 사용. 모두 NEC / address `0x04`.

| 분류 | urlKey |
|------|--------|
| 전원 | `power` `on` `off` `energy` |
| 입력 | `input` |
| 숫자 | `0`~`9` |
| 채널/볼륨 | `cu` `cd` `vu` `vd` `mute` |
| 화면 | `bru` `brd` `3d` `arc` `psm` |
| 네비게이션 | `up` `down` `left` `right` `ok` `back` `exit` |
| 기능 | `set` `home` `simplink` `auto` `1aA` `clear` `tile` `smenu` `wbal` |
| ID | `idon` `idoff` |
| 색상 | `yellow` `blue` |
| 미디어 | `play` `stop` `pause` `rw` `ff` |
| 특수 | `swap` `mirror` `instart` `instop` `adj` |

---

## 사용 예시 (curl)

```bash
# 전체 상태 조회
curl http://192.168.0.3/api/status

# 개별 모듈(3번) 상태
curl http://192.168.0.3/api/ir/3

# 키 프리셋으로 1번 모듈 전원 토글
curl -X POST http://192.168.0.3/api/ir/1/send \
  -H "Content-Type: application/json" \
  -d '{"key":"power"}'

# 직접 코드로 5번 모듈 송신
curl -X POST http://192.168.0.3/api/ir/5/send \
  -H "Content-Type: application/json" \
  -d '{"protocol":"NEC","address":"0x04","command":"0x08"}'

# 전체 모듈 일괄 송신 (음소거)
curl -X POST http://192.168.0.3/api/ir/all/send \
  -H "Content-Type: application/json" \
  -d '{"key":"mute"}'
```

---

## 구현 메모 (SRAM 32KB 제약)

- `String` 객체 미사용 — 고정 크기 `char[]` 버퍼만 사용.
- JSON은 ArduinoJson 대신 수동 파서(`jsonGetString`) 사용.
- 웹 페이지 HTML은 PROGMEM에 저장, 256B 청크 단위로 전송.
- `#define DISABLE_CODE_FOR_RECEIVER` — IR 수신 코드 비활성화(송신 전용).
- HTTP 헤더는 최대 512B, POST body는 최대 255B까지만 수용.
- POST는 `Content-Length` 필수 — 누락 시 411 반환하여 의도치 않은 `command=0` 송신을 차단.
```
