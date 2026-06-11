## Context

현재 `src/main.cpp`는 네트워크 주소를 파일 상단 상수로 고정한다:

```cpp
byte mac[]      = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x06 };
IPAddress ip(192, 168, 0, 76);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 255);
...
Ethernet.begin(mac, ip, gateway, gateway, subnet);   // setup() 내부
```

`Ethernet.begin(mac, ...)`(static)와 `Ethernet.begin(mac)`(DHCP)는 동일 라이브러리의 오버로드다. DHCP 형태는 `int`(성공 1 / 실패 0)를 반환하며 내부적으로 블로킹 타임아웃(기본 약 60초)을 가진다. SRAM 32KB 제약 하에서 `String` 미사용·`F()` 매크로 원칙을 유지해야 한다.

## Goals / Non-Goals

**Goals:**
- 단일 컴파일 타임 플래그로 DHCP / static 모드를 전환.
- `false`(static)를 기본값으로 두어 기존 배포와 100% 동일 동작 보장.
- DHCP 실패가 장치를 무한 정지시키지 않도록 진단 후 계속 진행.
- 부팅 로그에 모드와 실제 IP를 명확히 표기.

**Non-Goals:**
- 런타임(REST API/Web UI)에서의 모드 전환 — 본 변경 범위 밖(컴파일 타임 플래그만).
- static 폴백 자동 전환(DHCP 실패 시 static으로 자동 복귀) — 복잡도 대비 이득이 낮아 제외.
- IP/MAC을 EEPROM 등 영속 저장소에서 읽기.

## Decisions

**1. 컴파일 타임 `constexpr bool USE_DHCP` 사용 (런타임 변수 대신)**
- 플래그는 빌드 시 결정되며, `#if`가 아닌 일반 `if (USE_DHCP)` 분기로 작성한다. 컴파일러가 죽은 분기를 제거(dead-code elimination)하므로 메모리 낭비 없이 가독성을 확보한다.
- 대안: `#define`/`#if` 전처리 분기 → 가독성·타입 안전성 떨어져 기각. 대안: 런타임 `volatile` 변수 → 불필요한 코드/메모리 상주로 기각.

**2. 단일 `Ethernet.begin` 호출 지점에서 분기**
```cpp
bool ok = true;
if (USE_DHCP) {
    Serial.println(F("Network mode: DHCP"));
    ok = Ethernet.begin(mac);              // 반환 0 = 실패
    if (ok) {
        Serial.print(F("DHCP IP: "));
        Serial.println(Ethernet.localIP()); // 할당받은 IP 명시 출력
    } else {
        Serial.println(F("DHCP failed (timeout/no server)"));
    }
} else {
    Serial.println(F("Network mode: STATIC"));
    Ethernet.begin(mac, ip, gateway, gateway, subnet);
}
```
- static `Ethernet.begin`은 `void`를 반환하므로 성공 판정은 DHCP 경로에만 적용.

**3. 하드웨어/타임아웃 진단 분리**
- 기존 `Ethernet.hardwareStatus()`/`linkStatus()` 체크 블록을 유지·재사용한다. DHCP 실패 메시지는 `Ethernet.begin(mac)` 반환값으로, 하드웨어 미발견은 `hardwareStatus()`로 구분 출력한다.

**4. static 상수 유지**
- `ip`/`gateway`/`subnet` 상수는 DHCP 모드에서도 그대로 둔다(컴파일 경고는 무시 가능 수준이며, 모드 토글 시 재사용 위해 보존).

## Risks / Trade-offs

- **[DHCP 블로킹 타임아웃이 길다(기본 ~60초)]** → 부팅이 느려질 수 있음. 필요 시 `Ethernet.begin(mac, timeout_ms)` 오버로드로 타임아웃을 단축(예: 10초) 적용 가능 — 구현 시 짧은 타임아웃 지정 권장.
- **[DHCP 클라이언트 코드 링크로 Flash/SRAM 증가]** → `pio run`으로 빌드 후 SRAM 사용량이 32KB 한계 내인지 확인. 초과 시 경고.
- **[DHCP 실패 후 IP가 0.0.0.0]** → 웹서버가 접속 불가 상태로 기동될 수 있음. 진단 로그로 사용자에게 명확히 알리되, 자동 static 폴백은 Non-Goal로 두어 동작을 단순·예측 가능하게 유지.
- **[static 상수 미사용 경고]** → 빌드 경고 발생 가능하나 기능 영향 없음.
