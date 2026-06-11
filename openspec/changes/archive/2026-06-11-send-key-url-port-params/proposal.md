## Why

현재 `test/test_rest_api.py`의 `send_key(module, key, timeout=5)`는 전역 상수 `BASE_URL`에 호스트/포트가 하드코딩되어 있어, 보드 IP나 포트가 바뀌면 함수를 호출하는 쪽에서 대상을 지정할 수 없다. URL과 포트를 인자로 받으면 여러 장치/포트를 대상으로 같은 테스트 헬퍼를 재사용할 수 있다.

## What Changes

- `send_key` 함수가 `url, port, module, key`를 인자로 받도록 변경한다. `port`의 기본값은 `80`이다.
- Python의 기본값 인자 배치 규칙상 기본값이 있는 `port`는 필수 인자 뒤에 오므로, 실제 시그니처는 `send_key(url, module, key, port=80, timeout=5)`로 둔다.
- 함수 내부에서 `url`과 `port`로 요청 대상 base URL을 구성한다(예: `f"{url}:{port}"`).
- 기존 호출부(`test_send_power_single_module`, `test_send_power_all_modules`, `test_send_all_keys_single_module`, `test_unknown_key_rejected`)를 새 시그니처에 맞게 갱신한다.
- 호출부가 공통으로 사용할 기본 host/port 상수를 정리한다(`BASE_URL` 분해).

## Capabilities

### New Capabilities
- `rest-api-test-harness`: REST API 테스트 헬퍼가 대상 host(url)와 port를 인자로 받아 임의의 장치/포트로 IR 키 전송을 검증하는 동작.

### Modified Capabilities
<!-- 기존 spec 없음 — 해당 없음 -->

## Impact

- **코드**: `test/test_rest_api.py` — `send_key` 정의 및 모든 호출부, base URL 상수.
- **의존성**: 추가 없음(`requests` 그대로 사용).
- **호환성**: `send_key`의 호출 규약이 바뀌므로 기존 호출 코드는 모두 갱신 필요(BREAKING은 테스트 파일 내부에 한정).
