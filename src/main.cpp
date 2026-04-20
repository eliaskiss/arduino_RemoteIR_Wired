/*
 * ============================================================
 *  IR Control System — 15-Module REST API Server
 *  Board:  Arduino UNO R4 Minima
 *  Shield: Ethernet (W5100/W5500) + Sensor Shield
 *  IR TX:  DFR0095 x 15
 * ============================================================
 *  IRremote v3.x — IrSender.setSendPin() 으로 동적 핀 전환
 *  Ethernet — 고정 IP, HTTP 서버 (포트 80)
 * ============================================================
 */

#include <Arduino.h>

#define DISABLE_CODE_FOR_RECEIVER
// IR_SEND_PIN 미정의 → setSendPin() 동적 전환 사용
#include <IRremote.hpp>
#include <SPI.h>
#include <Ethernet.h>

#include "gpio_map.h"
#include "web_page.h"

// ─────────── 네트워크 설정 ───────────
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

EthernetServer server(80);

// ─────────── LG TV NEC 코드 (remote_control.cpp 참조) ───────────
#define LG_ADDRESS  0x04

// 전원
#define KEY_POWER           0x08
#define KEY_MONITOR_ON      0xC4
#define KEY_MONITOR_OFF     0xC5
#define KEY_ENERGY_SAVING   0x95

// 입력
#define KEY_INPUT           0x0B

// 숫자
#define KEY_NUM0            0x10
#define KEY_NUM1            0x11
#define KEY_NUM2            0x12
#define KEY_NUM3            0x13
#define KEY_NUM4            0x14
#define KEY_NUM5            0x15
#define KEY_NUM6            0x16
#define KEY_NUM7            0x17
#define KEY_NUM8            0x18
#define KEY_NUM9            0x19

// 채널 / 볼륨
#define KEY_CH_UP           0x00
#define KEY_CH_DOWN         0x01
#define KEY_VOL_UP          0x02
#define KEY_VOL_DOWN        0x03
#define KEY_MUTE            0x09

// 화면
#define KEY_BRIGHTNESS_UP   0xE0
#define KEY_BRIGHTNESS_DOWN 0xE1
#define KEY_3D              0xDC
#define KEY_ARC             0x79
#define KEY_PSM             0x4D

// 네비게이션
#define KEY_UP              0x40
#define KEY_DOWN            0x41
#define KEY_LEFT            0x07
#define KEY_RIGHT           0x06
#define KEY_OK              0x44
#define KEY_BACK            0x28
#define KEY_EXIT            0x5B

// 기능
#define KEY_SETTINGS        0x43
#define KEY_HOME            0x7C
#define KEY_SIMPLINK        0x7E
#define KEY_AUTO_CONFIG     0x99
#define KEY_1_a_A           0x32
#define KEY_CLEAR           0x2F
#define KEY_TILE            0x7B
#define KEY_S_MENU          0x3F
#define KEY_W_BAL           0x5F

// ID
#define KEY_ID_ON           0x72
#define KEY_ID_OFF          0x71

// 색상 버튼
#define KEY_YELLOW          0x63
#define KEY_BLUE            0x61

// 미디어
#define KEY_PLAY            0xB0
#define KEY_STOP            0xB1
#define KEY_PAUSE           0xBA
#define KEY_BACKWARD        0x8F
#define KEY_FORWARD         0x8E

// 특수
#define KEY_SWAP            0x97
#define KEY_MIRROR          0x96
#define KEY_INSTART         0xFB
#define KEY_INSTOP          0xFA
#define KEY_ADJ             0xFF

// ─────────── 키 매핑 구조체 ───────────
typedef struct {
    const char* urlKey;
    uint8_t     irCode;
    const char* name;
} KeyMapping;

const KeyMapping KEY_MAP[] = {
    // 전원
    { "power",   KEY_POWER,           "Power Toggle"    },
    { "on",      KEY_MONITOR_ON,      "Power ON"        },
    { "off",     KEY_MONITOR_OFF,     "Power OFF"       },
    { "energy",  KEY_ENERGY_SAVING,   "Energy Saving"   },
    // 입력
    { "input",   KEY_INPUT,           "Input"           },
    // 숫자
    { "0",       KEY_NUM0,            "Num 0"           },
    { "1",       KEY_NUM1,            "Num 1"           },
    { "2",       KEY_NUM2,            "Num 2"           },
    { "3",       KEY_NUM3,            "Num 3"           },
    { "4",       KEY_NUM4,            "Num 4"           },
    { "5",       KEY_NUM5,            "Num 5"           },
    { "6",       KEY_NUM6,            "Num 6"           },
    { "7",       KEY_NUM7,            "Num 7"           },
    { "8",       KEY_NUM8,            "Num 8"           },
    { "9",       KEY_NUM9,            "Num 9"           },
    // 채널 / 볼륨
    { "cu",      KEY_CH_UP,           "CH+"             },
    { "cd",      KEY_CH_DOWN,         "CH-"             },
    { "vu",      KEY_VOL_UP,          "Vol+"            },
    { "vd",      KEY_VOL_DOWN,        "Vol-"            },
    { "mute",    KEY_MUTE,            "Mute"            },
    // 화면
    { "bru",     KEY_BRIGHTNESS_UP,   "Brightness+"     },
    { "brd",     KEY_BRIGHTNESS_DOWN, "Brightness-"     },
    { "3d",      KEY_3D,              "3D"              },
    { "arc",     KEY_ARC,             "Aspect Ratio"    },
    { "psm",     KEY_PSM,             "Picture Mode"    },
    // 네비게이션
    { "up",      KEY_UP,              "Up"              },
    { "down",    KEY_DOWN,            "Down"            },
    { "left",    KEY_LEFT,            "Left"            },
    { "right",   KEY_RIGHT,           "Right"           },
    { "ok",      KEY_OK,              "OK"              },
    { "back",    KEY_BACK,            "Back"            },
    { "exit",    KEY_EXIT,            "Exit"            },
    // 기능
    { "set",     KEY_SETTINGS,        "Settings"        },
    { "home",    KEY_HOME,            "Home"            },
    { "simplink",KEY_SIMPLINK,        "SimpLink"        },
    { "auto",    KEY_AUTO_CONFIG,     "Auto Config"     },
    { "1aA",     KEY_1_a_A,           "1/a/A"           },
    { "clear",   KEY_CLEAR,           "Clear"           },
    { "tile",    KEY_TILE,            "Tile"            },
    { "smenu",   KEY_S_MENU,          "S.Menu"          },
    { "wbal",    KEY_W_BAL,           "W.Balance"       },
    // ID
    { "idon",    KEY_ID_ON,           "ID On"           },
    { "idoff",   KEY_ID_OFF,          "ID Off"          },
    // 색상 버튼
    { "yellow",  KEY_YELLOW,          "Yellow"          },
    { "blue",    KEY_BLUE,            "Blue"            },
    // 미디어
    { "play",    KEY_PLAY,            "Play"            },
    { "stop",    KEY_STOP,            "Stop"            },
    { "pause",   KEY_PAUSE,           "Pause"           },
    { "rw",      KEY_BACKWARD,        "Rewind"          },
    { "ff",      KEY_FORWARD,         "Fast Forward"    },
    // 특수
    { "swap",    KEY_SWAP,            "Swap"            },
    { "mirror",  KEY_MIRROR,          "Mirror"          },
    { "instart", KEY_INSTART,         "InStart"         },
    { "instop",  KEY_INSTOP,          "InStop"          },
    { "adj",     KEY_ADJ,             "Adjust"          },
};

const int KEY_MAP_SIZE = sizeof(KEY_MAP) / sizeof(KEY_MAP[0]);

const KeyMapping* findKey(const char* urlKey) {
    for (int i = 0; i < KEY_MAP_SIZE; i++) {
        if (strcmp(urlKey, KEY_MAP[i].urlKey) == 0) {
            return &KEY_MAP[i];
        }
    }
    return nullptr;
}

// ─────────── 모듈 상태 추적 ───────────
struct ModuleStatus {
    uint32_t sendCount;
    bool     lastSuccess;
};

ModuleStatus moduleStatus[IR_MODULE_COUNT];

// ─────────── IR 전송 ───────────
bool sendIrSignal(uint8_t moduleIndex, const char* protocol,
                  uint16_t address, uint8_t command) {
    if (moduleIndex >= IR_MODULE_COUNT) return false;

    uint8_t pin = IR_PINS[moduleIndex];
    IrSender.setSendPin(pin);

    Serial.print(F("IR["));
    Serial.print(moduleIndex + 1);
    Serial.print(F("] pin="));
    Serial.print(pin);
    Serial.print(F(" proto="));
    Serial.print(protocol);
    Serial.print(F(" addr=0x"));
    Serial.print(address, HEX);
    Serial.print(F(" cmd=0x"));
    Serial.println(command, HEX);

    if (strcmp(protocol, "NEC") == 0) {
        IrSender.sendNEC(address, command, 0);
    } else if (strcmp(protocol, "SONY") == 0) {
        IrSender.sendSony(address, command, 0);
    } else if (strcmp(protocol, "RC5") == 0) {
        IrSender.sendRC5(address, command, 0);
    } else if (strcmp(protocol, "RC6") == 0) {
        IrSender.sendRC6(address, command, 0);
    } else if (strcmp(protocol, "SAMSUNG") == 0) {
        IrSender.sendSamsung(address, command, 0);
    } else {
        Serial.println(F("  Unknown protocol"));
        return false;
    }

    moduleStatus[moduleIndex].sendCount++;
    moduleStatus[moduleIndex].lastSuccess = true;
    Serial.println(F("  Sent OK"));
    return true;
}

// ─────────── HTTP 응답 헬퍼 ───────────
void sendHttpHeader(EthernetClient& client, int code, const char* status,
                    const char* contentType) {
    client.print(F("HTTP/1.1 "));
    client.print(code);
    client.print(' ');
    client.println(status);
    client.print(F("Content-Type: "));
    client.println(contentType);
    client.println(F("Access-Control-Allow-Origin: *"));
    client.println(F("Connection: close"));
    client.println();
}

void sendJsonOk(EthernetClient& client, const char* msg) {
    sendHttpHeader(client, 200, "OK", "application/json; charset=utf-8");
    client.print(F("{\"status\":\"ok\",\"message\":\""));
    client.print(msg);
    client.println(F("\"}"));
}

void sendJsonError(EthernetClient& client, int code, const char* status,
                   const char* msg) {
    sendHttpHeader(client, code, status, "application/json; charset=utf-8");
    client.print(F("{\"status\":\"error\",\"message\":\""));
    client.print(msg);
    client.println(F("\"}"));
}

// ─────────── JSON 간이 파서 ───────────
// ArduinoJson 없이 메모리 절약 — body에서 키의 문자열 값을 추출
bool jsonGetString(const char* json, const char* key, char* out, uint8_t maxLen) {
    char search[32];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char* p = strstr(json, search);
    if (!p) return false;
    p = strchr(p + strlen(search), ':');
    if (!p) return false;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '"') {
        p++;
        uint8_t i = 0;
        while (*p && *p != '"' && i < maxLen - 1) {
            out[i++] = *p++;
        }
        out[i] = '\0';
        return true;
    }
    return false;
}

uint16_t parseHexOrDec(const char* s) {
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        return (uint16_t)strtoul(s, NULL, 16);
    }
    return (uint16_t)atoi(s);
}

// ─────────── URL에서 모듈 ID 추출 ───────────
// /api/ir/{id}/send → id (1~15), 실패 시 0 반환
uint8_t parseModuleId(const char* url) {
    // /api/ir/ = 8 chars
    if (strncmp(url, "/api/ir/", 8) != 0) return 0;
    const char* p = url + 8;
    if (*p == 'a') return 0; // "all"
    int id = atoi(p);
    if (id < 1 || id > IR_MODULE_COUNT) return 0;
    return (uint8_t)id;
}

// ─────────── 상태 JSON 출력 ───────────
void sendModuleStatusJson(EthernetClient& client, uint8_t idx) {
    client.print(F("{\"id\":"));
    client.print(idx + 1);
    client.print(F(",\"pin\":"));
    client.print(IR_PINS[idx]);
    client.print(F(",\"sendCount\":"));
    client.print(moduleStatus[idx].sendCount);
    client.print(F(",\"lastSuccess\":"));
    client.print(moduleStatus[idx].lastSuccess ? F("true") : F("false"));
    client.print('}');
}

void handleGetStatus(EthernetClient& client) {
    sendHttpHeader(client, 200, "OK", "application/json; charset=utf-8");
    client.print(F("{\"status\":\"ok\",\"modules\":["));
    for (uint8_t i = 0; i < IR_MODULE_COUNT; i++) {
        if (i > 0) client.print(',');
        sendModuleStatusJson(client, i);
    }
    client.println(F("]}"));
}

void handleGetModuleStatus(EthernetClient& client, uint8_t id) {
    if (id < 1 || id > IR_MODULE_COUNT) {
        sendJsonError(client, 400, "Bad Request", "Invalid module ID (1-15)");
        return;
    }
    sendHttpHeader(client, 200, "OK", "application/json; charset=utf-8");
    client.print(F("{\"status\":\"ok\",\"module\":"));
    sendModuleStatusJson(client, id - 1);
    client.println('}');
}

// ─────────── POST 핸들러 ───────────
// POST body 두 가지 모드 지원:
//   1) {"key":"power"}              — KEY_MAP 프리셋 (NEC, LG_ADDRESS 자동)
//   2) {"protocol":"NEC","address":"0x04","command":"0x08"} — 직접 지정
void handleSendIr(EthernetClient& client, const char* body,
                  uint8_t singleId) {
    char protocol[12] = "NEC";
    uint16_t address = LG_ADDRESS;
    uint8_t  command = 0;

    // "key" 파라미터 우선 확인
    char keyStr[16] = "";
    if (jsonGetString(body, "key", keyStr, sizeof(keyStr)) && keyStr[0] != '\0') {
        const KeyMapping* mapping = findKey(keyStr);
        if (!mapping) {
            sendJsonError(client, 400, "Bad Request", "Unknown key");
            return;
        }
        command = mapping->irCode;
        // key 모드에서도 protocol/address 오버라이드 허용
        char proto[12];
        if (jsonGetString(body, "protocol", proto, sizeof(proto))) {
            strncpy(protocol, proto, sizeof(protocol) - 1);
        }
        char addrStr[12];
        if (jsonGetString(body, "address", addrStr, sizeof(addrStr))) {
            address = parseHexOrDec(addrStr);
        }
    } else {
        // 직접 지정 모드
        char proto[12], addrStr[12], cmdStr[12];
        if (jsonGetString(body, "protocol", proto, sizeof(proto))) {
            strncpy(protocol, proto, sizeof(protocol) - 1);
        }
        if (jsonGetString(body, "address", addrStr, sizeof(addrStr))) {
            address = parseHexOrDec(addrStr);
        }
        if (jsonGetString(body, "command", cmdStr, sizeof(cmdStr))) {
            command = (uint8_t)parseHexOrDec(cmdStr);
        }
    }

    if (singleId > 0) {
        // 단일 모듈 송신
        if (sendIrSignal(singleId - 1, protocol, address, command)) {
            sendJsonOk(client, "IR signal sent");
        } else {
            sendJsonError(client, 400, "Bad Request", "Send failed");
        }
    } else {
        // 전체 모듈 송신
        uint8_t ok = 0;
        for (uint8_t i = 0; i < IR_MODULE_COUNT; i++) {
            if (sendIrSignal(i, protocol, address, command)) ok++;
            delay(20); // 모듈 간 간격
        }
        if (ok == IR_MODULE_COUNT) {
            sendJsonOk(client, "All modules sent");
        } else {
            sendJsonError(client, 500, "Internal Server Error",
                          "Some modules failed");
        }
    }
}

// ─────────── 클라이언트 정리 ───────────
void drainAndClose(EthernetClient& client) {
    unsigned long t = millis() + 200;
    while (client.available() && millis() < t) {
        client.read();
    }
    client.flush();
    delay(5);
    client.stop();
}

// ─────────── Setup ───────────
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println(F("=== IR Control System ==="));
    Serial.println(F("Board: Arduino UNO R4 Minima"));
    Serial.print(F("IRremote v"));
    Serial.println(F(VERSION_IRREMOTE));

    // IR 초기화
    IrSender.begin(IR_PINS[0]);
    Serial.print(F("IR modules: "));
    Serial.println(IR_MODULE_COUNT);

    for (uint8_t i = 0; i < IR_MODULE_COUNT; i++) {
        pinMode(IR_PINS[i], OUTPUT);
        moduleStatus[i].sendCount = 0;
        moduleStatus[i].lastSuccess = false;
    }

    // Ethernet 초기화
    Ethernet.begin(mac, ip, gateway, gateway, subnet);
    Serial.print(F("IP: "));
    Serial.println(Ethernet.localIP());

    server.begin();
    Serial.print(F("HTTP server: http://"));
    Serial.println(Ethernet.localIP());
    Serial.println(F("================================"));
}

// ─────────── Loop ───────────
void loop() {
    // Ethernet 링크 유지
    Ethernet.maintain();

    EthernetClient client = server.available();
    if (!client) return;

    // ── 헤더 읽기 (최대 2KB) ──
    char headers[512];
    uint16_t hLen = 0;
    unsigned long timeout = millis() + 3000;
    bool headerEnd = false;

    while (client.connected() && millis() < timeout && !headerEnd) {
        if (client.available()) {
            char c = client.read();
            if (hLen < sizeof(headers) - 1) {
                headers[hLen++] = c;
                headers[hLen] = '\0';
            }
            if (hLen >= 4 &&
                headers[hLen-4] == '\r' && headers[hLen-3] == '\n' &&
                headers[hLen-2] == '\r' && headers[hLen-1] == '\n') {
                headerEnd = true;
            }
        } else {
            delay(1);
        }
    }

    if (hLen == 0) {
        drainAndClose(client);
        return;
    }

    // ── Method + URL 파싱 ──
    char method[8] = "";
    char url[64] = "";
    {
        char* sp1 = strchr(headers, ' ');
        if (sp1) {
            uint8_t mLen = sp1 - headers;
            if (mLen < sizeof(method)) {
                strncpy(method, headers, mLen);
                method[mLen] = '\0';
            }
            char* sp2 = strchr(sp1 + 1, ' ');
            if (sp2) {
                uint8_t uLen = sp2 - sp1 - 1;
                if (uLen < sizeof(url)) {
                    strncpy(url, sp1 + 1, uLen);
                    url[uLen] = '\0';
                }
            }
        }
    }

    // ── Content-Length + POST body 읽기 ──
    char body[256] = "";
    if (strcmp(method, "POST") == 0) {
        int contentLength = 0;
        const char* cl = strstr(headers, "Content-Length: ");
        if (!cl) cl = strstr(headers, "content-length: ");
        if (cl) {
            contentLength = atoi(cl + 16);
        }
        if (contentLength > (int)sizeof(body) - 1) {
            contentLength = sizeof(body) - 1;
        }

        uint16_t bLen = 0;
        timeout = millis() + 2000;
        while ((int)bLen < contentLength && client.connected() && millis() < timeout) {
            if (client.available()) {
                body[bLen++] = client.read();
            } else {
                delay(1);
            }
        }
        body[bLen] = '\0';
    }

    Serial.print(method);
    Serial.print(' ');
    Serial.println(url);

    // ── 라우팅 ──

    // OPTIONS (CORS preflight)
    if (strcmp(method, "OPTIONS") == 0) {
        client.println(F("HTTP/1.1 204 No Content"));
        client.println(F("Access-Control-Allow-Origin: *"));
        client.println(F("Access-Control-Allow-Methods: GET, POST, OPTIONS"));
        client.println(F("Access-Control-Allow-Headers: Content-Type"));
        client.println(F("Access-Control-Max-Age: 86400"));
        client.println(F("Connection: close"));
        client.println();
    }
    // GET / — Web UI
    else if (strcmp(method, "GET") == 0 &&
             (strcmp(url, "/") == 0 || strcmp(url, "/index.html") == 0)) {
        client.println(F("HTTP/1.1 200 OK"));
        client.println(F("Content-Type: text/html; charset=utf-8"));
        client.println(F("Connection: close"));
        client.println();

        // PROGMEM에서 청크 단위로 전송 (메모리 절약)
        const uint16_t chunkSize = 256;
        uint16_t totalLen = strlen_P(HTML_PAGE);
        for (uint16_t i = 0; i < totalLen; i += chunkSize) {
            char buf[chunkSize + 1];
            uint16_t len = min((uint16_t)chunkSize, (uint16_t)(totalLen - i));
            memcpy_P(buf, HTML_PAGE + i, len);
            buf[len] = '\0';
            client.print(buf);
        }
    }
    // GET /api/status — 전체 상태
    else if (strcmp(method, "GET") == 0 && strcmp(url, "/api/status") == 0) {
        handleGetStatus(client);
    }
    // GET /api/ir/{id} — 개별 상태
    else if (strcmp(method, "GET") == 0 && strncmp(url, "/api/ir/", 8) == 0 &&
             strchr(url + 8, '/') == NULL) {
        uint8_t id = parseModuleId(url);
        handleGetModuleStatus(client, id);
    }
    // POST /api/ir/all/send — 전체 송신
    else if (strcmp(method, "POST") == 0 &&
             strcmp(url, "/api/ir/all/send") == 0) {
        handleSendIr(client, body, 0);
    }
    // POST /api/ir/{id}/send — 개별 송신
    else if (strcmp(method, "POST") == 0 && strncmp(url, "/api/ir/", 8) == 0) {
        uint8_t id = parseModuleId(url);
        if (id > 0) {
            handleSendIr(client, body, id);
        } else {
            sendJsonError(client, 400, "Bad Request",
                          "Invalid module ID (1-15)");
        }
    }
    // favicon
    else if (strcmp(url, "/favicon.ico") == 0) {
        client.println(F("HTTP/1.1 204 No Content"));
        client.println();
    }
    // 404
    else {
        sendJsonError(client, 404, "Not Found", "Not Found");
    }

    drainAndClose(client);
}
