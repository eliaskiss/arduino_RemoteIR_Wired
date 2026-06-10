"""IR Control System REST API Test

src/main.cpp 의 KeyMapping(KEY_MAP) 정의를 Python 으로 옮겨
프리셋 키 전송을 검증한다.
"""

import requests

# ─────────── LG TV NEC 코드 (main.cpp 참조) ───────────
LG_ADDRESS = 0x04

# 전원
KEY_POWER = 0x08
KEY_MONITOR_ON = 0xC4
KEY_MONITOR_OFF = 0xC5
KEY_ENERGY_SAVING = 0x95

# 입력
KEY_INPUT = 0x0B

# 숫자
KEY_NUM0 = 0x10
KEY_NUM1 = 0x11
KEY_NUM2 = 0x12
KEY_NUM3 = 0x13
KEY_NUM4 = 0x14
KEY_NUM5 = 0x15
KEY_NUM6 = 0x16
KEY_NUM7 = 0x17
KEY_NUM8 = 0x18
KEY_NUM9 = 0x19

# 채널 / 볼륨
KEY_CH_UP = 0x00
KEY_CH_DOWN = 0x01
KEY_VOL_UP = 0x02
KEY_VOL_DOWN = 0x03
KEY_MUTE = 0x09

# 화면
KEY_BRIGHTNESS_UP = 0xE0
KEY_BRIGHTNESS_DOWN = 0xE1
KEY_3D = 0xDC
KEY_ARC = 0x79
KEY_PSM = 0x4D

# 네비게이션
KEY_UP = 0x40
KEY_DOWN = 0x41
KEY_LEFT = 0x07
KEY_RIGHT = 0x06
KEY_OK = 0x44
KEY_BACK = 0x28
KEY_EXIT = 0x5B

# 기능
KEY_SETTINGS = 0x43
KEY_HOME = 0x7C
KEY_SIMPLINK = 0x7E
KEY_AUTO_CONFIG = 0x99
KEY_1_a_A = 0x32
KEY_CLEAR = 0x2F
KEY_TILE = 0x7B
KEY_S_MENU = 0x3F
KEY_W_BAL = 0x5F

# ID
KEY_ID_ON = 0x72
KEY_ID_OFF = 0x71

# 색상 버튼
KEY_YELLOW = 0x63
KEY_BLUE = 0x61

# 미디어
KEY_PLAY = 0xB0
KEY_STOP = 0xB1
KEY_PAUSE = 0xBA
KEY_BACKWARD = 0x8F
KEY_FORWARD = 0x8E

# 특수
KEY_SWAP = 0x97
KEY_MIRROR = 0x96
KEY_INSTART = 0xFB
KEY_INSTOP = 0xFA
KEY_ADJ = 0xFF


# ─────────── 키 매핑 (main.cpp KeyMapping 구조체와 동일 순서) ───────────
# (url_key, ir_code, name)
KEY_MAP = [
    # 전원
    ("power", KEY_POWER, "Power Toggle"),
    ("on", KEY_MONITOR_ON, "Power ON"),
    ("off", KEY_MONITOR_OFF, "Power OFF"),
    ("energy", KEY_ENERGY_SAVING, "Energy Saving"),
    # 입력
    ("input", KEY_INPUT, "Input"),
    # 숫자
    ("0", KEY_NUM0, "Num 0"),
    ("1", KEY_NUM1, "Num 1"),
    ("2", KEY_NUM2, "Num 2"),
    ("3", KEY_NUM3, "Num 3"),
    ("4", KEY_NUM4, "Num 4"),
    ("5", KEY_NUM5, "Num 5"),
    ("6", KEY_NUM6, "Num 6"),
    ("7", KEY_NUM7, "Num 7"),
    ("8", KEY_NUM8, "Num 8"),
    ("9", KEY_NUM9, "Num 9"),
    # 채널 / 볼륨
    ("cu", KEY_CH_UP, "CH+"),
    ("cd", KEY_CH_DOWN, "CH-"),
    ("vu", KEY_VOL_UP, "Vol+"),
    ("vd", KEY_VOL_DOWN, "Vol-"),
    ("mute", KEY_MUTE, "Mute"),
    # 화면
    ("bru", KEY_BRIGHTNESS_UP, "Brightness+"),
    ("brd", KEY_BRIGHTNESS_DOWN, "Brightness-"),
    ("3d", KEY_3D, "3D"),
    ("arc", KEY_ARC, "Aspect Ratio"),
    ("psm", KEY_PSM, "Picture Mode"),
    # 네비게이션
    ("up", KEY_UP, "Up"),
    ("down", KEY_DOWN, "Down"),
    ("left", KEY_LEFT, "Left"),
    ("right", KEY_RIGHT, "Right"),
    ("ok", KEY_OK, "OK"),
    ("back", KEY_BACK, "Back"),
    ("exit", KEY_EXIT, "Exit"),
    # 기능
    ("set", KEY_SETTINGS, "Settings"),
    ("home", KEY_HOME, "Home"),
    ("simplink", KEY_SIMPLINK, "SimpLink"),
    ("auto", KEY_AUTO_CONFIG, "Auto Config"),
    ("1aA", KEY_1_a_A, "1/a/A"),
    ("clear", KEY_CLEAR, "Clear"),
    ("tile", KEY_TILE, "Tile"),
    ("smenu", KEY_S_MENU, "S.Menu"),
    ("wbal", KEY_W_BAL, "W.Balance"),
    # ID
    ("idon", KEY_ID_ON, "ID On"),
    ("idoff", KEY_ID_OFF, "ID Off"),
    # 색상 버튼
    ("yellow", KEY_YELLOW, "Yellow"),
    ("blue", KEY_BLUE, "Blue"),
    # 미디어
    ("play", KEY_PLAY, "Play"),
    ("stop", KEY_STOP, "Stop"),
    ("pause", KEY_PAUSE, "Pause"),
    ("rw", KEY_BACKWARD, "Rewind"),
    ("ff", KEY_FORWARD, "Fast Forward"),
    # 특수
    ("swap", KEY_SWAP, "Swap"),
    ("mirror", KEY_MIRROR, "Mirror"),
    ("instart", KEY_INSTART, "InStart"),
    ("instop", KEY_INSTOP, "InStop"),
    ("adj", KEY_ADJ, "Adjust"),
]


def find_key(url_key):
    """main.cpp findKey() 의 Python 버전 — url_key 로 매핑 항목을 찾는다."""
    for entry in KEY_MAP:
        if entry[0] == url_key:
            return entry
    return None


# ─────────── 테스트 ───────────
def send_key(url, module, key, port=80, timeout=5):
    """대상 url:port 의 모듈(또는 'all')에 프리셋 key 를 전송하고 응답을 반환한다.

    Args:
        url:     대상 host (스킴 포함, 포트 미포함). 예: "http://192.168.0.10"
        module:  모듈 ID(1~15) 또는 "all"
        key:     KEY_MAP 의 url_key (예: "power")
        port:    대상 포트 (기본값 80). 포트포워딩 시 외부 포트 지정 가능
        timeout: 요청 타임아웃(초)
    """
    r = requests.post(
        f"{url}:{port}/api/ir/{module}/send",
        json={"key": key},
        timeout=timeout,
    )
    return r

def test_send_power_single_module(HOST, PORT):
    """Module 1에 power 키 전송"""
    r = send_key(HOST, 1, "power", port=PORT)
    print(f"[Module 1] {r.status_code} {r.json()}")
    assert r.status_code == 200
    assert r.json()["status"] == "ok"


def test_send_power_all_modules(HOST, PORT):
    """전체 모듈에 power 키 전송"""
    r = send_key(HOST, "all", "power", port=PORT, timeout=10)
    print(f"[ALL]      {r.status_code} {r.json()}")
    assert r.status_code == 200
    assert r.json()["status"] == "ok"


def test_send_all_keys_single_module(HOST, PORT):
    """KEY_MAP 의 모든 프리셋 키를 Module 1에 순차 전송"""
    for url_key, ir_code, name in KEY_MAP:
        r = send_key(HOST, 1, url_key, port=PORT)
        print(f"[Module 1] {url_key:<8} (0x{ir_code:02X} {name}) -> "
              f"{r.status_code} {r.json()}")
        assert r.status_code == 200
        assert r.json()["status"] == "ok"


def test_unknown_key_rejected(HOST, PORT):
    """KEY_MAP 에 없는 키는 400 으로 거절"""
    r = send_key(HOST, 1, "nosuchkey", port=PORT)
    print(f"[Module 1] unknown-key -> {r.status_code} {r.json()}")
    assert r.status_code == 400
    assert r.json()["status"] == "error"


if __name__ == "__main__":
    # 대상 host(스킴 포함)와 port — 테스트 전반에서 재사용
    HOST = "http://192.168.1.100"
    PORT = 80

    test_send_power_single_module(HOST, PORT)
    test_send_power_all_modules(HOST, PORT)
    test_send_all_keys_single_module(HOST, PORT)
    test_unknown_key_rejected(HOST, PORT)
    print(f"\nAll tests passed. ({len(KEY_MAP)} keys in KEY_MAP)")
