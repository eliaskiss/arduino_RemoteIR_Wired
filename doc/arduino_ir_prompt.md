# Arduino IR Control System 개발

## 프로젝트 개요
Arduino UNO R4 Minima에 Ethernet Shield와 Sensor Shield를 적층하여,
15개의 IR Module(DFR0095)을 개별 제어하는 REST API 서버와 Web UI를 구현한다.

---

## 하드웨어 구성

### 부품 목록
- Arduino UNO R4 Minima x 1
- Arduino Ethernet Shield (W5100/W5500) x 1
- Arduino Sensor Shield x 1
- IR Module DFR0095 x 15
- Jumper Cable (Female/Male) x 15

### 적층 순서
Arduino UNO R4 Minima → Ethernet Shield → Sensor Shield 순으로 적층

### GPIO 맵 설계 조건
- Ethernet Shield가 점유하는 핀을 반드시 제외하고 IR Module용 GPIO를 배정할 것
- Ethernet Shield W5100 기준 사용 핀: D10(SS), D11(MOSI), D12(MISO), D13(SCK), D4(SD CS)
- 위 핀을 제외한 디지털 핀 중 15개를 IR Module에 1:1 매핑
- GPIO 맵은 코드 상단에 주석과 `#define` 또는 `const int` 배열로 명확히 정의
- 핀 번호와 IR Module ID(1~15) 매핑 테이블을 README에도 포함

---

## 참조 파일
- `doc/remote_control.cpp` : IRremote 라이브러리 송신 예제 코드 (첨부 참조)

위 두 파일을 분석하여 IR 신호 포맷, 프로토콜, 데이터 구조를 파악하고 Arduino 코드에 반영할 것.

---

## 기능 요구사항

### 1. 네트워크 설정
- 고정 IP 방식 사용 (DHCP 사용 안 함)
- 기본값 예시로 아래를 사용하되, 코드 상단에서 쉽게 수정 가능하도록 상수로 분리
  - IP: `192.168.1.100`
  - Gateway: `192.168.1.1`
  - Subnet: `255.255.255.0`
  - MAC: `DE:AD:BE:EF:FE:ED`
- HTTP 포트: `80`

### 2. IR Module 제어
- IRremote 라이브러리를 사용하여 IR 신호 송신
- 15개 IR Module을 각각 독립적으로 제어
- 각 모듈은 할당된 GPIO 핀을 통해 IRsend 인스턴스와 1:1 연결
- 참조 파일의 IR 신호 데이터(프로토콜, 주소, 커맨드 등)를 그대로 활용

### 3. REST API
모든 응답은 JSON 형식으로 반환

| Method | Endpoint | 설명 |
|--------|----------|------|
| GET | `/api/status` | 전체 모듈 상태 반환 |
| GET | `/api/ir/{id}` | 특정 모듈(1~15) 상태 반환 |
| POST | `/api/ir/{id}/send` | 특정 모듈로 IR 신호 송신 |
| POST | `/api/ir/all/send` | 전체 모듈 동시 IR 신호 송신 |

POST body 예시:
```json
{
  "protocol": "NEC",
  "address": "0x00",
  "command": "0x45"
}
```

### 4. Web UI
- 브라우저에서 `http://{IP}/` 접속 시 Web 페이지 제공
- 페이지 내 구성:
  - 15개 IR Module을 카드 형태로 표시 (Module ID, 연결 핀 번호 표시)
  - 각 모듈별 IR 신호 송신 버튼
  - 전체 동시 송신 버튼
  - 송신 결과(성공/실패) 실시간 표시
- HTML/CSS/JS를 Arduino의 PROGMEM에 저장하여 메모리 최적화
- JavaScript fetch API로 REST API 호출

---

## 산출물

1. `ir_control.ino` — 메인 Arduino 스케치
2. `gpio_map.h` — 핀 매핑 정의 헤더
3. `web_page.h` — PROGMEM에 저장할 HTML/CSS/JS 헤더
4. `README.md` — 핀 매핑 테이블, 회로 연결 방법, API 명세, 빌드/업로드 방법

---

## 개발 환경 및 제약

- Arduino IDE 또는 arduino-cli 기준으로 작성
- 사용 라이브러리:
  - `Ethernet` (Arduino 기본 내장)
  - `IRremote` (Arduino-IRremote v3.x 이상)
- Arduino UNO R4 Minima 호환 확인 (AVR 아닌 RA4M1 기반임을 인지할 것)
- SRAM 제한(32KB)을 고려하여 `String` 객체 사용 최소화, `F()` 매크로 적극 활용
- 웹 페이지 HTML은 반드시 PROGMEM에 저장

---

## 작업 순서 (Claude Code가 따를 것)

1. `python_sample.py`와 `IRremote_sample` 파일을 먼저 분석하여 IR 데이터 구조 파악
2. Ethernet Shield 점유 핀을 제외한 GPIO 맵 설계 및 `gpio_map.h` 작성
3. REST API 라우팅 및 HTTP 파싱 로직 구현
4. IRremote 기반 송신 함수 구현
5. Web UI HTML/CSS/JS 작성 후 `web_page.h`에 PROGMEM으로 변환
6. 전체를 통합한 `ir_control.ino` 작성
7. `README.md` 작성 (핀 테이블 포함)
8. arduino-cli로 컴파일 검증 (보드: `arduino:renesas_uno:minima`)

---

## 사용 방법

1. **참조 파일 준비** — `IRremote_sample` 코드와 `python_sample.py`를 프로젝트 폴더에 넣고 Claude Code에 첨부
2. **프롬프트 입력** — 위 내용을 Claude Code 채팅창에 붙여넣기
3. **파일 자동 분석** — Claude Code가 첨부 파일을 읽고 IR 프로토콜을 파악한 뒤 순서대로 파일을 생성

> 💡 **팁**: `python_sample.py`에 특정 IR 코드(예: TV 전원, 에어컨 온도 등)가 있다면
> 프롬프트 마지막에 *"python_sample.py의 커맨드 목록을 Web UI 드롭다운에 반영할 것"*
> 한 줄을 추가하면 더 완성도 높은 UI가 생성됩니다.
