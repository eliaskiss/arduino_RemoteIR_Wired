"""IR Remote Control REST API wrapper for the 15-channel IR control system."""

import requests


class IRRemoteError(Exception):
    """IR 제어 시스템 API 에러."""


# Preset key names (main.cpp KEY_MAP 기반)
VALID_KEYS = frozenset({
    "power", "on", "off", "energy", "input",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "cu", "cd", "vu", "vd", "mute",
    "bru", "brd", "3d", "arc", "psm",
    "up", "down", "left", "right", "ok", "back", "exit",
    "set", "home", "simplink", "auto", "1aA", "clear", "tile", "smenu", "wbal",
    "idon", "idoff",
    "yellow", "blue",
    "play", "stop", "pause", "rw", "ff",
    "swap", "mirror", "instart", "instop", "adj",
})

VALID_PROTOCOLS = ("NEC", "SONY", "RC5", "RC6", "SAMSUNG")


class IRRemote:
    """15채널 IR 제어 시스템 REST API 클라이언트.

    Usage::

        ir = IRRemote("192.168.1.100")
        ir.power()             # 전체 전원 토글
        ir.power(1)            # 모듈 1 전원
        ir.send(3, key="vu")   # 모듈 3 볼륨 업
        ir.send_all(protocol="NEC", address="0x04", command="0x08")
        ir.status()            # 전체 상태 조회
        ir.module_status(5)    # 모듈 5 상태
    """

    # ── Preset key 상수 ──
    POWER = "power"
    ON = "on"
    OFF = "off"
    ENERGY = "energy"
    INPUT = "input"
    CH_UP = "cu"
    CH_DOWN = "cd"
    VOL_UP = "vu"
    VOL_DOWN = "vd"
    MUTE = "mute"
    BRIGHTNESS_UP = "bru"
    BRIGHTNESS_DOWN = "brd"
    THREE_D = "3d"
    ARC = "arc"
    PSM = "psm"
    UP = "up"
    DOWN = "down"
    LEFT = "left"
    RIGHT = "right"
    OK = "ok"
    BACK = "back"
    EXIT = "exit"
    SETTINGS = "set"
    HOME = "home"
    SIMPLINK = "simplink"
    AUTO = "auto"
    CLEAR = "clear"
    TILE = "tile"
    PLAY = "play"
    STOP = "stop"
    PAUSE = "pause"
    REWIND = "rw"
    FAST_FORWARD = "ff"
    YELLOW = "yellow"
    BLUE = "blue"
    SWAP = "swap"
    MIRROR = "mirror"
    INSTART = "instart"
    INSTOP = "instop"
    ADJ = "adj"
    ID_ON = "idon"
    ID_OFF = "idoff"
    S_MENU = "smenu"
    W_BAL = "wbal"

    def __init__(self, host="192.168.1.100", port=80, timeout=5):
        self._base = f"http://{host}:{port}" if port != 80 else f"http://{host}"
        self._timeout = timeout

    def __repr__(self):
        return f"IRRemote({self._base})"

    # ── 내부 헬퍼 ──

    def _validate_module(self, module_id):
        if not (1 <= module_id <= 15):
            raise ValueError(f"module_id must be 1-15, got {module_id}")

    def _get(self, path):
        r = requests.get(f"{self._base}{path}", timeout=self._timeout)
        data = r.json()
        if r.status_code >= 400 or data.get("status") == "error":
            raise IRRemoteError(data.get("message", f"HTTP {r.status_code}"))
        return data

    def _build_body(self, key=None, protocol=None, address=None, command=None):
        body = {}
        if key is not None:
            if key not in VALID_KEYS:
                raise ValueError(f"Unknown key '{key}'. Valid keys: {sorted(VALID_KEYS)}")
            body["key"] = key
        if protocol is not None:
            p = protocol.upper()
            if p not in VALID_PROTOCOLS:
                raise ValueError(f"Unknown protocol '{protocol}'. Valid: {VALID_PROTOCOLS}")
            body["protocol"] = p
        if address is not None:
            body["address"] = address
        if command is not None:
            body["command"] = command
        if not body:
            raise ValueError("Provide key and/or protocol+address+command")
        return body

    def _post_send(self, path, **kwargs):
        body = self._build_body(**kwargs)
        r = requests.post(f"{self._base}{path}", json=body, timeout=self._timeout)
        data = r.json()
        if r.status_code >= 400 or data.get("status") == "error":
            raise IRRemoteError(data.get("message", f"HTTP {r.status_code}"))
        return data

    # ── Core API ──

    def status(self):
        """GET /api/status — 전체 모듈 상태 반환."""
        return self._get("/api/status")

    def module_status(self, module_id):
        """GET /api/ir/{id} — 개별 모듈 상태 반환."""
        self._validate_module(module_id)
        return self._get(f"/api/ir/{module_id}")

    def send(self, module_id, *, key=None, protocol=None, address=None, command=None):
        """POST /api/ir/{id}/send — 개별 모듈 IR 송신.

        Args:
            module_id: 모듈 번호 (1-15)
            key: 프리셋 키 이름 (e.g. "power", "vu")
            protocol: IR 프로토콜 ("NEC", "SONY", "RC5", "RC6", "SAMSUNG")
            address: 주소값 (e.g. "0x04")
            command: 커맨드값 (e.g. "0x08")
        """
        self._validate_module(module_id)
        return self._post_send(
            f"/api/ir/{module_id}/send",
            key=key, protocol=protocol, address=address, command=command,
        )

    def send_all(self, *, key=None, protocol=None, address=None, command=None):
        """POST /api/ir/all/send — 전체 모듈 IR 송신."""
        return self._post_send(
            "/api/ir/all/send",
            key=key, protocol=protocol, address=address, command=command,
        )

    # ── Shortcut 메서드 ──

    def power(self, module_id=None):
        """전원 토글. module_id 생략 시 전체."""
        return self.send(module_id, key="power") if module_id else self.send_all(key="power")

    def on(self, module_id=None):
        """전원 ON."""
        return self.send(module_id, key="on") if module_id else self.send_all(key="on")

    def off(self, module_id=None):
        """전원 OFF."""
        return self.send(module_id, key="off") if module_id else self.send_all(key="off")

    def vol_up(self, module_id=None):
        """볼륨 업."""
        return self.send(module_id, key="vu") if module_id else self.send_all(key="vu")

    def vol_down(self, module_id=None):
        """볼륨 다운."""
        return self.send(module_id, key="vd") if module_id else self.send_all(key="vd")

    def mute(self, module_id=None):
        """음소거."""
        return self.send(module_id, key="mute") if module_id else self.send_all(key="mute")

    def ch_up(self, module_id=None):
        """채널 업."""
        return self.send(module_id, key="cu") if module_id else self.send_all(key="cu")

    def ch_down(self, module_id=None):
        """채널 다운."""
        return self.send(module_id, key="cd") if module_id else self.send_all(key="cd")
