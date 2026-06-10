## 1. 플래그 정의

- [x] 1.1 `src/main.cpp` 네트워크 상수 선언부 근처에 `constexpr bool USE_DHCP = false;` 추가 (기본값 static)
- [x] 1.2 기존 `ip`/`gateway`/`subnet` static 상수는 그대로 유지

## 2. 초기화 분기 구현

- [x] 2.1 `setup()`의 `Ethernet.begin(mac, ip, gateway, gateway, subnet)` 호출을 `if (USE_DHCP)` 분기로 감싸기
- [x] 2.2 DHCP 경로: `Ethernet.begin(mac)`(가능하면 짧은 타임아웃 오버로드) 호출, 반환값 0이면 "DHCP failed" 진단 출력
- [x] 2.5 DHCP 획득 성공 시 할당받은 `Ethernet.localIP()`를 시리얼에 명시적으로 출력 (`F()` 매크로 사용)
- [x] 2.3 static 경로: 기존 `Ethernet.begin(mac, ip, gateway, gateway, subnet)` 유지
- [x] 2.4 DHCP 실패 시에도 무한 대기 없이 후속 부팅 로직(server.begin 등) 계속 진행하도록 보장

## 3. 진단 로그

- [x] 3.1 부팅 시 현재 모드("Network mode: DHCP" / "STATIC")를 시리얼에 출력 (`F()` 매크로 사용)
- [x] 3.2 기존 하드웨어/링크 상태 체크 블록 유지하여 하드웨어 미발견과 DHCP 타임아웃을 구분 출력
- [x] 3.3 실제 적용된 `Ethernet.localIP()`를 부팅 로그에 출력 (기존 출력 재사용/확인)

## 4. 검증 및 문서화

- [x] 4.1 `pio run` 컴파일 성공 및 SRAM 사용량이 32KB 한계 내인지 확인
- [x] 4.2 `USE_DHCP=false`(기본)로 빌드 시 기존 static 동작과 동일함을 시리얼 로그로 확인
- [ ] 4.3 `USE_DHCP=true`로 빌드 후 DHCP 환경에서 IP 자동 획득 동작 확인
- [x] 4.4 `CLAUDE.md`의 "Network Defaults" 섹션에 `USE_DHCP` 플래그 설명 추가
