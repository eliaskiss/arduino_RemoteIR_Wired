"""IR Control System — 큐/비블로킹 동작 검증 하니스 (change: nonblocking-ir-send)

tasks.md 6.2~6.8 의 수동 검증 항목을 자동화한다.
IR 수신기가 없으므로 "실제 송신 여부"는 GET /api/status 의 modules[].sendCount
증가분을 ground truth 로 삼아 판정한다.

실행:
    python test/test_queue.py [host] [port]
    # 또는 환경변수
    IR_HOST=http://192.168.0.75 IR_PORT=80 python test/test_queue.py

기본값은 main.cpp 의 static IP(192.168.0.75:80). DHCP 사용 시 시리얼에 찍힌 IP를 인자로 넘긴다.

전제: 보드가 네트워크에 연결되어 있고 펌웨어(nonblocking-ir-send)가 올라가 있어야 한다.
"""

import os
import sys
import time
import socket

import requests

LG_ADDRESS = "0x04"
PROTO = "NEC"

# main.cpp 튜닝 상수와 일치시켜야 하는 값들
IR_MODULE_COUNT = 15
IR_QUEUE_SIZE = 64
MODULE_GAP_MS = 20
HEADER_READ_TIMEOUT_MS = 800
BODY_READ_TIMEOUT_MS = 500

# 즉시 응답으로 간주할 상한(초) — 큐 적재 후 바로 반환되어야 한다
IMMEDIATE_RESP_LIMIT = 1.0

# 펌웨어에서 연속 중복 디듀프(enqueueJob)가 활성화돼 있는지.
# main.cpp 에서 주석 처리하면 False 로 바꾼다.
DEDUP_ENABLED = False


# ─────────── 결과 집계 ───────────
class Results:
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.skipped = 0

    def check(self, name, cond, detail=""):
        if cond:
            self.passed += 1
            print(f"  ✓ {name} {detail}")
        else:
            self.failed += 1
            print(f"  ✗ FAIL: {name} {detail}")
        return cond

    def skip(self, name, reason=""):
        self.skipped += 1
        print(f"  ⊘ SKIP: {name} — {reason}")


RES = Results()


# ─────────── HTTP 헬퍼 ───────────
def base(host, port):
    return f"{host}:{port}"


def send(host, port, target, command, address=LG_ADDRESS, protocol=PROTO, timeout=5):
    """직접 지정 모드로 단일(target=1..15) 또는 전체(target="all") 송신을 요청한다."""
    return requests.post(
        f"{base(host, port)}/api/ir/{target}/send",
        json={"protocol": protocol, "address": address, "command": command},
        timeout=timeout,
    )


# NOTE: 송신(/send)은 비멱등(상태 변경)이라 재시도 금지 — enqueue 후 응답만 끊긴 경우
# 재시도하면 중복 적재되어 카운트가 부풀려진다. 이 보드는 동시 TCP 연결에 약하므로
# 카운트 검증 테스트는 직렬(한 번에 하나)로만 요청한다.

def get_status(host, port, timeout=5):
    r = requests.get(f"{base(host, port)}/api/status", timeout=timeout)
    r.raise_for_status()
    return r.json()


def module_counts(host, port):
    """modules[].sendCount 리스트(인덱스 0 = 모듈 1)를 반환한다."""
    st = get_status(host, port)
    return [m["sendCount"] for m in st["modules"]]


def wait_idle(host, port, timeout=20.0):
    """큐가 비고 진행 작업이 끝날 때까지 대기. 성공 시 True."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        q = get_status(host, port)["queue"]
        if not q["active"] and q["pending"] == 0:
            return True
        time.sleep(0.05)
    return False


# ─────────── 6.2 단일 송신: 즉시 응답 + 실제 송신 ───────────
def test_single_immediate_and_sent(host, port):
    print("[6.2] 단일 송신 — 즉시 200(queued) + 실제 송신")
    assert wait_idle(host, port), "사전 idle 대기 실패"
    before = module_counts(host, port)

    t0 = time.time()
    r = send(host, port, 1, "0x08")
    elapsed = time.time() - t0

    RES.check("status_code == 200", r.status_code == 200, f"(got {r.status_code})")
    RES.check("message == queued", r.json().get("message") == "queued", f"({r.json()})")
    RES.check("즉시 응답", elapsed < IMMEDIATE_RESP_LIMIT, f"({elapsed*1000:.0f}ms)")

    assert wait_idle(host, port), "송신 완료 대기 실패"
    after = module_counts(host, port)
    RES.check("모듈1 sendCount +1", after[0] == before[0] + 1,
              f"({before[0]} -> {after[0]})")


# ─────────── 6.3 전체 송신: 즉시 응답 + 모듈 순차 송신 ───────────
def test_all_immediate_and_sequential(host, port):
    print("[6.3] 전체 송신 — 즉시 응답 + MODULE_GAP_MS 간격 순차 송신")
    assert wait_idle(host, port), "사전 idle 대기 실패"
    before = module_counts(host, port)

    t0 = time.time()
    r = send(host, port, "all", "0x09")
    elapsed = time.time() - t0
    RES.check("전체 송신 즉시 응답", elapsed < IMMEDIATE_RESP_LIMIT, f"({elapsed*1000:.0f}ms)")
    RES.check("message == queued", r.json().get("message") == "queued")

    # 진행 중간에 sent 가 증가하는지 관찰(순차 진행 증거)
    saw_progress = False
    deadline = time.time() + 20
    while time.time() < deadline:
        q = get_status(host, port)["queue"]
        if q["active"] and q["target"] == 0 and 0 < q["sent"] < q["total"]:
            saw_progress = True
            break
        if not q["active"] and q["pending"] == 0:
            break
        time.sleep(0.02)
    RES.check("진행 중 부분 송신 관찰(0<sent<total)", saw_progress)

    assert wait_idle(host, port), "전체 송신 완료 대기 실패"
    after = module_counts(host, port)
    all_incremented = all(after[i] == before[i] + 1 for i in range(IR_MODULE_COUNT))
    RES.check("전 모듈 sendCount +1", all_incremented,
              f"(delta={[after[i]-before[i] for i in range(IR_MODULE_COUNT)]})")


# ─────────── 6.4 연속 요청 무유실 ───────────
def test_no_loss_distinct(host, port):
    # 이 보드는 동시 TCP 연결에 약하므로(소켓 한계) "짧은 시간 연속"을 직렬 rapid-fire 로 검증한다.
    print("[6.4] 연속 요청 무유실 (모듈1에 10개 distinct command, 직렬 rapid-fire)")
    assert wait_idle(host, port), "사전 idle 대기 실패"
    before = module_counts(host, port)

    commands = [f"0x{0x10 + i:02X}" for i in range(10)]  # 0x10..0x19, 모두 다름
    codes = [send(host, port, 1, cmd).status_code for cmd in commands]

    RES.check("모든 요청 200", all(c == 200 for c in codes), f"({codes})")
    assert wait_idle(host, port), "송신 완료 대기 실패"
    after = module_counts(host, port)
    RES.check("모듈1 sendCount +10 (무유실)", after[0] == before[0] + 10,
              f"({before[0]} -> {after[0]})")


# ─────────── 6.5 연속 중복 디듀프 ───────────
def test_consecutive_dedup(host, port):
    state = "활성" if DEDUP_ENABLED else "비활성(주석 처리됨)"
    print(f"[6.5] 연속 중복 디듀프 — 현재 펌웨어: {state}")

    if DEDUP_ENABLED:
        # 디듀프 ON 검증은 "소비자가 바쁜 동안 동일 신호가 큐에 연속으로 쌓이는" 상황이 필요한데,
        # 이는 동시 요청(병렬 연결)에 의존한다. 이 보드는 동시 TCP 연결에 약해 신뢰성 있게
        # 재현하기 어렵다 → 디듀프 ON 일 때의 경계 검증은 SKIP.
        RES.skip("디듀프 ON 경계 검증", "동시 연결 필요 — 이 보드 소켓 한계로 미수행")
        return

    # 디듀프 OFF: 직렬로 동일 신호 5회 → 모두 적재되어 +5 여야 한다(유실/병합 없음).
    assert wait_idle(host, port), "사전 idle 대기 실패"
    before = module_counts(host, port)
    for _ in range(5):
        send(host, port, 2, "0x40")  # 모듈2, 완전히 동일한 신호 5회 (직렬)
    assert wait_idle(host, port), "송신 완료 대기 실패"
    after = module_counts(host, port)
    RES.check("동일 5연속 → 모듈2 +5 (디듀프 OFF, 모두 적재)",
              after[1] == before[1] + 5, f"({before[1]} -> {after[1]})")
    RES.skip("디듀프 경계 케이스(A,B,A / 동일command·다른protocol)",
             "디듀프 OFF 상태라 경계 의미 없음")


# ─────────── 6.6 status 의 queue 진행 반영 ───────────
def test_status_queue_fields(host, port):
    print("[6.6] GET /api/status 의 queue.pending/진행 반영")
    assert wait_idle(host, port), "사전 idle 대기 실패"

    idle = get_status(host, port)["queue"]
    RES.check("idle: active=false, pending=0",
              idle["active"] is False and idle["pending"] == 0, f"({idle})")

    # 전체 송신을 걸고 진행 필드가 채워지는지 확인
    send(host, port, "all", "0x95")
    seen_active = False
    seen_target0 = False
    deadline = time.time() + 20
    while time.time() < deadline:
        q = get_status(host, port)["queue"]
        if q["active"]:
            seen_active = True
            if q["target"] == 0 and q["total"] == IR_MODULE_COUNT:
                seen_target0 = True
        elif q["pending"] == 0:
            break
        time.sleep(0.02)
    RES.check("진행 중 active=true 관찰", seen_active)
    RES.check("전체 작업 target=0, total=15 관찰", seen_target0)
    assert wait_idle(host, port)


# ─────────── 6.7 큐 포화 시 503 ───────────
def test_queue_saturation_503(host, port):
    print("[6.7] 큐 포화 503")
    # 64-deep 큐를 직렬 요청(~19ms/건)으로 채우려면 소비 속도(20ms/건)와 비슷해 깊이가 안 쌓인다.
    # 깊게 채우려면 병렬 연결로 소비보다 빠르게 enqueue 해야 하는데, 이 보드는 동시 TCP 연결에
    # 약해(소켓 한계) 신뢰성 있는 폭주를 만들 수 없고, /send 재시도는 중복 적재를 유발한다.
    RES.skip("큐 포화 503",
             "직렬로는 큐를 못 채우고 병렬은 소켓 한계로 막힘 — "
             "정확 검증은 펌웨어 IR_QUEUE_SIZE 를 임시로 작게(예: 4) 잡고 재시도 권장")


# ─────────── 6.8 느린 클라이언트 타임아웃 ───────────
def test_slow_client_timeout(host, port):
    print("[6.8] 느린 클라이언트 — 헤더 미완성 시 타임아웃 정리")
    hostname = host.split("://", 1)[-1]

    # 헤더를 끝맺지 않는(\r\n\r\n 없이) 연결을 연다
    s = socket.create_connection((hostname, port), timeout=5)
    s.sendall(b"GET /api/status HTTP/1.1\r\nHost: x\r\n")  # 종료 CRLF 없음
    s.settimeout(HEADER_READ_TIMEOUT_MS / 1000 + 2.0)

    t0 = time.time()
    closed_within = False
    try:
        # 서버가 타임아웃 후 연결을 닫으면 recv 가 b"" 반환
        while time.time() - t0 < (HEADER_READ_TIMEOUT_MS / 1000 + 1.5):
            data = s.recv(256)
            if data == b"":
                closed_within = True
                break
    except socket.timeout:
        closed_within = False
    finally:
        s.close()
    elapsed = time.time() - t0
    RES.check("느린 헤더 연결이 타임아웃 내 정리됨", closed_within, f"({elapsed*1000:.0f}ms)")

    # 정상 요청은 여전히 즉시 처리되는지 확인(HOL 블로킹 없음)
    t0 = time.time()
    ok = get_status(host, port)["status"] == "ok"
    elapsed = time.time() - t0
    RES.check("정상 요청은 영향 없이 응답", ok and elapsed < IMMEDIATE_RESP_LIMIT,
              f"({elapsed*1000:.0f}ms)")


# ─────────── 러너 ───────────
def main():
    host = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("IR_HOST", "http://10.196.94.40")
    port = int(sys.argv[2]) if len(sys.argv) > 2 else int(os.environ.get("IR_PORT", "8074"))
    if "://" not in host:
        host = "http://" + host

    print(f"대상: {base(host, port)}\n")

    # 연결 확인
    try:
        get_status(host, port)
    except requests.RequestException as e:
        print(f"보드에 연결할 수 없습니다: {e}")
        print("펌웨어 업로드 및 네트워크 연결 후 host/port 를 확인하세요.")
        sys.exit(2)

    tests = [
        test_single_immediate_and_sent,
        test_all_immediate_and_sequential,
        test_no_loss_distinct,
        test_consecutive_dedup,
        test_status_queue_fields,
        test_queue_saturation_503,
        test_slow_client_timeout,
    ]
    for t in tests:
        try:
            t(host, port)
        except AssertionError as e:
            RES.failed += 1
            print(f"  ✗ ERROR in {t.__name__}: {e}")
        except Exception as e:  # noqa: BLE001
            RES.failed += 1
            print(f"  ✗ EXCEPTION in {t.__name__}: {type(e).__name__}: {e}")
        print()

    print(f"결과: {RES.passed} passed, {RES.failed} failed, {RES.skipped} skipped")
    sys.exit(1 if RES.failed else 0)


if __name__ == "__main__":
    main()
