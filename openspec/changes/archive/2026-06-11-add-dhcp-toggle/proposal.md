## Why

현재 네트워크 주소는 `main.cpp` 상단에 static IP로 하드코딩되어 있어, 다른 네트워크 환경(예: DHCP만 허용하는 사무실/공유기 환경)에 장비를 옮길 때마다 코드를 수정·재컴파일·재업로드해야 한다. DHCP 자동 할당을 선택할 수 있게 하면 환경 변경 시 재배포 부담이 사라진다.

## What Changes

- `main.cpp` 상단에 `USE_DHCP`(또는 `DHCP`) 컴파일 타임 플래그 상수를 추가한다.
- 플래그가 `true`이면 `Ethernet.begin(mac)`로 DHCP를 통해 IP/게이트웨이/서브넷을 자동 획득한다.
- 플래그가 `false`이면 기존과 동일하게 static IP(`ip`, `gateway`, `subnet`)로 설정한다 — 기본 동작 유지(기본값 `false`).
- DHCP 획득 실패 시 시리얼로 명확한 진단 메시지를 출력한다(타임아웃/하드웨어 없음 구분).
- 시리얼 부팅 로그에 현재 모드(DHCP/STATIC)와 실제 적용된 IP를 출력한다.

## Capabilities

### New Capabilities
- `network-config`: Ethernet 네트워크 주소 설정 방식(DHCP 자동 할당 vs static 고정)을 결정하는 플래그 기반 설정 동작과 부팅 시 진단 출력.

### Modified Capabilities
<!-- 기존 spec 없음 — 해당 없음 -->

## Impact

- **코드**: `src/main.cpp` — 네트워크 상수 선언부, `setup()`의 `Ethernet.begin(...)` 호출부, 부팅 시리얼 로그.
- **의존성**: 추가 라이브러리 없음. 기존 `Ethernet` 라이브러리의 DHCP 지원(`Ethernet.begin(mac)`) 사용.
- **메모리**: DHCP 클라이언트 코드가 링크되어 약간의 Flash/SRAM 증가 예상(SRAM 32KB 제약 내 확인 필요).
- **문서**: `CLAUDE.md`의 "Network Defaults" 섹션에 플래그 설명 추가 필요.
