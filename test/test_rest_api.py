"""IR Control System REST API Test — POST power key"""

import requests

BASE_URL = "http://192.168.1.100"


def test_send_power_single_module():
    """Module 1에 power 키 전송"""
    r = requests.post(
        f"{BASE_URL}/api/ir/1/send",
        json={"key": "power"},
        timeout=5,
    )
    print(f"[Module 1] {r.status_code} {r.json()}")
    assert r.status_code == 200
    assert r.json()["status"] == "ok"


def test_send_power_all_modules():
    """전체 모듈에 power 키 전송"""
    r = requests.post(
        f"{BASE_URL}/api/ir/all/send",
        json={"key": "power"},
        timeout=10,
    )
    print(f"[ALL]      {r.status_code} {r.json()}")
    assert r.status_code == 200
    assert r.json()["status"] == "ok"


if __name__ == "__main__":
    test_send_power_single_module()
    test_send_power_all_modules()
    print("\nAll tests passed.")
