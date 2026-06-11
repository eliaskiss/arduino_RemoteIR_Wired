# rest-api-test-harness Specification

## Purpose
REST API 테스트 하니스(`test/`)에서 IR 송신 엔드포인트를 호출하는 헬퍼와 테스트의 동작을 정의한다.

## Requirements

### Requirement: send_key 대상 지정 파라미터
`send_key` 헬퍼는 호출 시 대상 host(`url`), `module`, `key`, `port`를 인자로 받아야 한다(SHALL). `port`는 기본값 `80`을 가져야 한다(SHALL). 함수는 `url`과 `port`로 요청 base URL을 구성하고, `/api/ir/{module}/send` 엔드포인트에 `{"key": key}` 본문으로 POST 요청을 보내야 한다(SHALL).

#### Scenario: url/port로 단일 모듈에 키 전송
- **WHEN** `send_key("http://192.168.0.75", 1, "power", port=80)`를 호출한다
- **THEN** 헬퍼는 `http://192.168.0.75:80/api/ir/1/send`로 `{"key":"power"}` 본문의 POST 요청을 보낸다
- **AND** 서버의 응답 객체를 반환한다

#### Scenario: port 생략 시 기본값 80 사용
- **WHEN** `send_key("http://192.168.0.75", 1, "power")`를 `port` 인자 없이 호출한다
- **THEN** 헬퍼는 기본 포트 `80`을 사용해 `http://192.168.0.75:80/api/ir/1/send`로 요청을 보낸다

#### Scenario: 전체 모듈 대상 전송
- **WHEN** `module` 인자로 `"all"`을 전달한다
- **THEN** 헬퍼는 `/api/ir/all/send` 엔드포인트로 요청을 보낸다

### Requirement: 호출부의 새 시그니처 사용
모든 테스트 함수는 `send_key`를 새 시그니처(`url, module, key, port=80`)로 호출해야 한다(SHALL). 대상 host와 port는 공통 상수로 정의하여 테스트 전반에서 재사용해야 한다(SHALL).

#### Scenario: 기존 테스트가 새 시그니처로 동작
- **WHEN** `test_send_power_single_module` 등 기존 테스트가 실행된다
- **THEN** 각 테스트는 공통 host/port 상수를 사용해 `send_key(url, module, key, port=...)` 형태로 호출한다
- **AND** 응답 상태코드 200과 `status == "ok"` 검증이 유지된다
