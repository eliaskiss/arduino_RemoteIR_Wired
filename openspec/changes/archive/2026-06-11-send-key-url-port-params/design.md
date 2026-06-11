## Context

현재 `test/test_rest_api.py`:

```python
BASE_URL = "http://192.168.1.100"

def send_key(module, key, timeout=5):
    r = requests.post(f"{BASE_URL}/api/ir/{module}/send", json={"key": key}, timeout=timeout)
    return r
```

host/port가 `BASE_URL` 한 상수에 묶여 있어 호출부에서 대상을 바꿀 수 없다. 요청은 `requests.post`로 단순하며, `module`은 정수(1~15) 또는 문자열 `"all"`을 받는다.

## Goals / Non-Goals

**Goals:**
- `send_key`가 `url, port, module, key`를 인자로 받아 대상을 호출 시점에 결정.
- base URL을 `f"{url}:{port}"`로 구성.
- 기존 4개 테스트 호출부를 새 시그니처로 갱신.
- 공통 host/port 상수 정리.

**Non-Goals:**
- `timeout`을 위치 인자로 승격 — 기존처럼 키워드 기본값으로 유지(시그니처는 `url, port, module, key` + `timeout=...`).
- 프로토콜(http/https) 자동 판별 — `url`에 스킴 포함을 전제로 둠.
- CLI 인자 파싱 등 외부 설정 주입.

## Decisions

**1. 시그니처: `send_key(url, module, key, port=80, timeout=5)`**
- 사용자가 요청한 인자는 `url, port, module, key`이며 `port` 기본값은 `80`이다. 그러나 Python은 기본값 인자(`port=80`)를 기본값 없는 인자(`module`, `key`) 앞에 둘 수 없으므로, `port`를 필수 인자 뒤로 옮겨 `send_key(url, module, key, port=80, timeout=5)`로 정의한다.
- `port`를 생략하면 80(main.cpp의 서버 포트)이 사용된다. 기존 `timeout`도 호환을 위해 키워드 기본값으로 유지한다.
- base URL 구성: `base = f"{url}:{port}"` → 요청 `f"{base}/api/ir/{module}/send"`.
- 대안 A: `url`에 포트까지 포함 → 사용자가 명시적으로 `port` 분리를 요청했으므로 기각.
- 대안 B: 순서를 그대로 `url, port, module, key`로 두고 모든 인자에 기본값 부여 → `module`/`key`에 의미 없는 기본값이 생겨 기각. 대안 C: `module`/`key`를 keyword-only(`*`)로 강제 → 호출부가 모두 키워드 호출로 바뀌어 가독성 저하, 기각.

**2. host/port 상수 분리**
- 기존 `BASE_URL = "http://192.168.1.100"`을 `HOST = "http://192.168.1.100"`, `PORT = 80`으로 분리하여 각 테스트가 `send_key(HOST, PORT, ...)`로 호출.
- main.cpp의 실제 포트(80)와 일치.

**3. 호출부 일괄 갱신**
- `test_send_power_single_module`, `test_send_power_all_modules`, `test_send_all_keys_single_module`, `test_unknown_key_rejected`의 `send_key(...)` 호출을 새 순서로 수정.

## Risks / Trade-offs

- **[호출 규약 변경(BREAKING)]** → 영향 범위는 이 테스트 파일 내부로 한정. 같은 파일의 모든 호출부를 함께 수정하여 깨짐 방지.
- **[`url`에 스킴/포트 중복 입력 시 잘못된 URL]** → 문서/주석으로 `url`은 스킴 포함·포트 미포함(`http://host`)임을 명시. 별도 검증 로직은 Non-Goal.
