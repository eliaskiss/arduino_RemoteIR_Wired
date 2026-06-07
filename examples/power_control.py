"""
IR Control System — Power On / Off / Toggle 예제
필요: pip install requests
"""
import requests

# ─────────── 서버 설정 ───────────
BASE_URL = "http://192.168.0.3"   # main.cpp 의 ip 와 일치시킬 것
TIMEOUT  = 5                       # 초


def send_key(module_id, key):
    """지정 모듈(1~15)에 키 프리셋 IR 송신.

    module_id: 1~15
    key:       'on' | 'off' | 'power' 등 (KEY_MAP urlKey)
    """
    url = f"{BASE_URL}/api/ir/{module_id}/send"
    resp = requests.post(url, json={"key": key}, timeout=TIMEOUT)
    resp.raise_for_status()
    return resp.json()


def power_on(module_id):
    """Power ON  (KEY_MONITOR_ON, 0xC4)"""
    return send_key(module_id, "on")


def power_off(module_id):
    """Power OFF (KEY_MONITOR_OFF, 0xC5)"""
    return send_key(module_id, "off")


def power_toggle(module_id):
    """Power Toggle (KEY_POWER, 0x08)"""
    return send_key(module_id, "power")


def power_all(key):
    """전체 15개 모듈 일괄 송신. key: 'on' | 'off' | 'power'"""
    url = f"{BASE_URL}/api/ir/all/send"
    resp = requests.post(url, json={"key": key}, timeout=TIMEOUT)
    resp.raise_for_status()
    return resp.json()


if __name__ == "__main__":
    MODULE = 1

    print("Power ON     :", power_on(MODULE))
    print("Power OFF    :", power_off(MODULE))
    print("Power Toggle :", power_toggle(MODULE))

    # 전체 모듈 한 번에 끄기
    # print("All OFF      :", power_all("off"))
