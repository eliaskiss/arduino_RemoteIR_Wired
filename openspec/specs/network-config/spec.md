# network-config

## Purpose

Ethernet 네트워크 주소 설정 방식(DHCP 자동 할당 vs static 고정)을 결정하는 플래그 기반 설정 동작과 부팅 시 진단 출력을 정의한다.

## Requirements

### Requirement: 네트워크 주소 설정 모드 플래그
시스템은 컴파일 타임 boolean 플래그(`USE_DHCP`)를 통해 네트워크 주소 설정 방식을 선택할 수 있어야 한다(SHALL). 플래그가 `true`이면 DHCP로 IP/게이트웨이/서브넷을 자동 획득하고, `false`이면 static 상수로 설정한다. 기본값은 기존 동작과의 호환을 위해 `false`(static)여야 한다(SHALL).

#### Scenario: 플래그가 false일 때 static 설정
- **WHEN** `USE_DHCP`가 `false`이고 장치가 부팅된다
- **THEN** 시스템은 코드 상단에 정의된 static `ip`, `gateway`, `subnet` 값으로 Ethernet을 초기화한다
- **AND** 적용된 IP는 정의된 static IP와 일치한다

#### Scenario: 플래그가 true일 때 DHCP 설정
- **WHEN** `USE_DHCP`가 `true`이고 DHCP 서버가 있는 네트워크에서 부팅된다
- **THEN** 시스템은 DHCP를 통해 IP/게이트웨이/서브넷을 자동 획득한다
- **AND** static `ip`/`gateway`/`subnet` 상수는 사용하지 않는다

### Requirement: DHCP 획득 실패 처리
`USE_DHCP`가 `true`인 상태에서 DHCP 주소 획득에 실패하면, 시스템은 무한 대기에 빠지지 않고 실패를 시리얼로 보고해야 한다(SHALL). 하드웨어 미발견과 DHCP 응답 타임아웃을 구분하여 진단 메시지를 출력해야 한다(SHALL).

#### Scenario: DHCP 서버 응답 없음(타임아웃)
- **WHEN** `USE_DHCP`가 `true`이고 DHCP 서버가 응답하지 않는다
- **THEN** 시스템은 타임아웃 후 "DHCP failed" 류의 진단 메시지를 시리얼에 출력한다
- **AND** 장치는 멈추지 않고 후속 부팅 로직(서버 시작 등)을 계속 진행한다

#### Scenario: Ethernet 하드웨어 미발견
- **WHEN** Ethernet Shield가 장착되지 않은 상태에서 부팅된다
- **THEN** 시스템은 "Ethernet shield NOT found" 류의 메시지를 출력하여 하드웨어 문제를 타임아웃과 구분해 알린다

### Requirement: 네트워크 모드 부팅 진단 출력
시스템은 부팅 시 현재 활성화된 네트워크 모드(DHCP 또는 STATIC)와 실제 적용된 로컬 IP 주소를 시리얼에 출력해야 한다(SHALL).

#### Scenario: 부팅 시 모드와 IP 로그
- **WHEN** 장치가 부팅을 완료한다
- **THEN** 시리얼 출력에 현재 모드("DHCP" 또는 "STATIC")가 표시된다
- **AND** 실제 적용된 로컬 IP 주소가 함께 출력된다

### Requirement: DHCP 획득 IP 시리얼 출력
`USE_DHCP`가 `true`이고 DHCP 주소 획득에 성공하면, 시스템은 DHCP로 할당받은 로컬 IP 주소를 시리얼에 명시적으로 출력해야 한다(SHALL).

#### Scenario: DHCP IP 획득 성공 시 출력
- **WHEN** `USE_DHCP`가 `true`이고 DHCP 서버로부터 IP를 정상 획득한다
- **THEN** 시스템은 DHCP로 할당받은 `Ethernet.localIP()` 값을 시리얼에 출력한다
- **AND** 해당 출력으로 사용자가 할당된 IP로 웹서버에 접속할 수 있다
