# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Arduino UNO R4 Minima (Renesas RA4M1) 기반 15채널 IR 제어 시스템. Ethernet Shield로 REST API 서버와 Web UI를 제공하여 15개 DFR0095 IR 모듈을 개별/일괄 제어한다.

## Build

```bash
pio run                    # 컴파일
pio run -t upload          # 업로드
pio device monitor -b 115200  # 시리얼 모니터
```

보드: `uno_r4_minima` (Renesas RA platform, AVR 아님)

## Architecture

- **`src/main.cpp`** — Ethernet 웹서버, HTTP 파싱, REST API 라우팅, IR 송신 로직 통합
- **`include/gpio_map.h`** — 15개 IR 모듈 ↔ GPIO 핀 매핑 (Ethernet Shield 점유 핀 D4,D10-D13 제외)
- **`include/web_page.h`** — Web UI HTML/CSS/JS를 PROGMEM `R"rawliteral()"` 리터럴로 저장
- **`doc/remote_control.cpp`** — LG TV 리모컨 참조 구현 (WiFi 버전, IR 프로토콜/코드 참조용)
- **`doc/arduino_ir_prompt.md`** — 원본 요구사항 문서 (Korean)

IR 송신은 단일 `IrSender` 인스턴스에서 `setSendPin()`으로 핀을 동적 전환하는 방식. `IR_SEND_PIN`을 정의하지 않아야 동작한다.

## Key Constraints

- **SRAM 32KB** — `String` 객체 사용 금지, `F()` 매크로 필수, JSON은 수동 파서 사용 (ArduinoJson 미사용)
- **PROGMEM** — 웹 페이지 HTML은 반드시 PROGMEM에 저장, 청크 단위(256B)로 클라이언트에 전송
- **`#define DISABLE_CODE_FOR_RECEIVER`** — IRremote 수신 코드 비활성화 (송신 전용, 메모리 절약)
- D0/D1 핀은 Serial과 공유 — 해당 모듈 사용 시 시리얼 디버깅 불가

## REST API

| Method | Endpoint | 설명 |
|--------|----------|------|
| GET | `/api/status` | 전체 모듈 상태 |
| GET | `/api/ir/{id}` | 개별 모듈 상태 (1-15) |
| POST | `/api/ir/{id}/send` | 개별 IR 송신 |
| POST | `/api/ir/all/send` | 전체 IR 송신 |

POST body: `{"protocol":"NEC","address":"0x04","command":"0x08"}`

## Network Defaults

IP: `192.168.1.100`, Gateway: `192.168.1.1`, MAC: `DE:AD:BE:EF:FE:ED`, Port: `80` — `main.cpp` 상단 상수로 변경 가능
