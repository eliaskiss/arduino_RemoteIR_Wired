## 1. 상수 정리

- [x] 1.1 `BASE_URL` 상수를 `HOST`(스킴 포함 host)와 `PORT`(정수, 기본 80)로 분리

## 2. send_key 시그니처 변경

- [x] 2.1 `send_key`를 `send_key(url, module, key, port=80, timeout=5)`로 변경 (`port` 기본값 80)
- [x] 2.2 함수 내부에서 `f"{url}:{port}/api/ir/{module}/send"`로 요청 URL 구성
- [x] 2.3 함수 docstring을 새 파라미터(`port` 기본값 80 포함)에 맞게 갱신

## 3. 호출부 갱신

- [x] 3.1 `test_send_power_single_module`을 `send_key(HOST, 1, "power", port=PORT)`로 수정
- [x] 3.2 `test_send_power_all_modules`를 `send_key(HOST, "all", "power", port=PORT, timeout=10)`로 수정
- [x] 3.3 `test_send_all_keys_single_module`을 `send_key(HOST, 1, url_key, port=PORT)`로 수정
- [x] 3.4 `test_unknown_key_rejected`를 `send_key(HOST, 1, "nosuchkey", port=PORT)`로 수정

## 4. 검증

- [x] 4.1 `python3` 문법 검사 통과 (`ast.parse` / import 검증)
- [x] 4.2 `send_key`가 의도한 URL을 구성하는지 requests stub으로 확인
