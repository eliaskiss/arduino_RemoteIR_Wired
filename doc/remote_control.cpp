/*
 * ============================================================
 *  LG TV IR Remote Controller
 *  Board:  Arduino UNO R4 WiFi
 *  IR TX:  DFR0095 → Digital Pin 3
 * ============================================================
 *  IRremote SimpleSender 예제 기반.
 *  IrSender.sendNEC(address, command, repeats) 사용.
 * ============================================================
 *  배선:
 *    DFR0095 SIG  → Arduino D3
 *    DFR0095 VCC  → Arduino 5V
 *    DFR0095 GND  → Arduino GND
 * ============================================================
 */

#include <Arduino.h>

#define DISABLE_CODE_FOR_RECEIVER
// IR_SEND_PIN을 정의하지 않아야 setSendPin()으로 동적 핀 전환 가능
#include <IRremote.hpp>
#include <WiFiS3.h>
#include <ArduinoJson.h>

// ─────────── WiFi 설정 ───────────
const char* WIFI_SSID     = "iptime_TAS";
const char* WIFI_PASSWORD = "idqe123$";

IPAddress staticIP(192, 168, 0, 3);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns_server(8, 8, 8, 8);

// ─────────── LG TV NEC 코드 ───────────
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

// ─────────── 키 매핑 구조체 (반드시 함수보다 위에 선언) ───────────
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
  { "smenu",   KEY_S_MENU,         "S.Menu"          },
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

// ─────────── 복수 IR 송신기 (핀 배열) ───────────
const uint8_t IR_PINS[] = { 3 };  // 송신기 추가 시: { 3, 5, 6 }
const int IR_PIN_COUNT = sizeof(IR_PINS) / sizeof(IR_PINS[0]);

const KeyMapping* findKey(const String& urlKey) {
  for (int i = 0; i < KEY_MAP_SIZE; i++) {
    if (urlKey == KEY_MAP[i].urlKey) {
      return &KEY_MAP[i];
    }
  }
  return nullptr;
}

// ─────────── 웹서버 ───────────
WiFiServer server(80);

// ─────────── IR 전송 ───────────
void sendIrCommand(int irIndex, uint8_t command, uint8_t repeats = 0) {
  uint8_t pin = IR_PINS[irIndex];
  IrSender.setSendPin(pin);

  Serial.print(F("Send NEC: pin="));
  Serial.print(pin);
  Serial.print(F(", addr=0x"));
  Serial.print(LG_ADDRESS, HEX);
  Serial.print(F(", cmd=0x"));
  Serial.print(command, HEX);
  Serial.print(F(", repeats="));
  Serial.println(repeats);

  IrSender.sendNEC(LG_ADDRESS, command, repeats);

  Serial.println(F("  → Sent OK"));
}

// ─────────── IR LED 테스트 ───────────
void testIrLed() {
  Serial.println(F("\n[TEST] IR LED + POWER 신호 테스트"));
  Serial.println(F("  스마트폰 전면 카메라로 DFR0095 LED를 확인하세요."));

  for (int i = 0; i < 3; i++) {
    Serial.print(F("  POWER 전송 "));
    Serial.print(i + 1);
    Serial.println(F("/3"));
    sendIrCommand(0, KEY_POWER, 0);
    delay(1000);
  }

  Serial.println(F("[TEST] 완료\n"));
}

// ─────────── WiFi 연결 ───────────
void connectWiFi() {
  Serial.print(F("WiFi: "));
  Serial.println(WIFI_SSID);

  WiFi.config(staticIP, dns_server, gateway, subnet);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("\nConnected! IP: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("\nFailed! Restarting..."));
    delay(5000);
    NVIC_SystemReset();
  }
}

// ─────────── JSON 응답 헬퍼 ───────────
void sendJsonResponse(WiFiClient& client, int statusCode, const char* statusText, const String& json) {
  client.print(F("HTTP/1.1 "));
  client.print(statusCode);
  client.print(' ');
  client.println(statusText);
  client.println(F("Content-Type: application/json; charset=utf-8"));
  client.println(F("Access-Control-Allow-Origin: *"));
  client.println(F("Connection: close"));
  client.println();
  client.println(json);
}

void sendJsonError(WiFiClient& client, int statusCode, const char* statusText, const char* message) {
  JsonDocument doc;
  doc["status"] = "error";
  doc["message"] = message;
  String json;
  serializeJson(doc, json);
  sendJsonResponse(client, statusCode, statusText, json);
}

void sendJsonSuccess(WiFiClient& client, const char* command, const char* name) {
  JsonDocument doc;
  doc["status"] = "ok";
  doc["command"] = command;
  doc["name"] = name;
  String json;
  serializeJson(doc, json);
  sendJsonResponse(client, 200, "OK", json);
}

void sendCorsPreflightResponse(WiFiClient& client) {
  client.println(F("HTTP/1.1 204 No Content"));
  client.println(F("Access-Control-Allow-Origin: *"));
  client.println(F("Access-Control-Allow-Methods: GET, POST, OPTIONS"));
  client.println(F("Access-Control-Allow-Headers: Content-Type"));
  client.println(F("Access-Control-Max-Age: 86400"));
  client.println(F("Connection: close"));
  client.println();
}

// ─────────── 웹 페이지 ───────────
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ko">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>LG TV Remote</title>
  <style>
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:'Segoe UI',sans-serif;background:#1a1a2e;color:#eee;
         display:flex;justify-content:center;min-height:100vh;padding:20px}
    .r{background:linear-gradient(145deg,#16213e,#0f3460);border-radius:30px;
       padding:30px 20px;width:320px;box-shadow:0 20px 60px rgba(0,0,0,.5)}
    h1{text-align:center;font-size:1.2em;margin-bottom:5px;color:#e94560}
    .st{text-align:center;font-size:.7em;color:#666;margin-bottom:20px}
    .row{display:flex;justify-content:center;gap:10px;margin-bottom:10px}
    .b{border:none;border-radius:12px;padding:14px 18px;font-size:.85em;
       font-weight:bold;cursor:pointer;transition:all .15s;color:#fff;
       min-width:70px;text-align:center}
    .b:active{transform:scale(.92)}
    .pw{background:#e94560;min-width:130px;padding:16px;font-size:1.1em}
    .pw:hover{background:#ff6b81}
    .on{background:#27ae60}.on:hover{background:#2ecc71}
    .of{background:#c0392b}.of:hover{background:#e74c3c}
    .fn{background:#2c3e50;min-width:55px}.fn:hover{background:#34495e}
    .mt{background:#8e44ad}.mt:hover{background:#9b59b6}
    .in{background:#2980b9}.in:hover{background:#3498db}
    .ok{background:#e67e22;border-radius:50%;width:60px;height:60px;
        min-width:unset;padding:0;font-size:1em}
    .ok:hover{background:#f39c12}
    .ar{background:#34495e;min-width:50px;padding:12px}
    .ar:hover{background:#4a6a8a}
    .lb{text-align:center;font-size:.65em;color:#444;margin:14px 0 6px;
        text-transform:uppercase;letter-spacing:2px}
    hr{border:none;border-top:1px solid #2a2a4a;margin:12px 0}
    #m{text-align:center;font-size:.8em;min-height:20px;margin-top:12px;color:#2ecc71}
  </style>
</head>
<body>
<div class="r">
  <h1>LG TV Remote</h1>
  <p class="st">Arduino UNO R4 WiFi + DFR0095</p>

  <div class="lb">POWER</div>
  <div class="row"><button class="b pw" onclick="s('power')">POWER</button></div>
  <div class="row">
    <button class="b on" onclick="s('on')">ON</button>
    <button class="b of" onclick="s('off')">OFF</button>
  </div>

  <hr>
  <div class="lb">VOLUME</div>
  <div class="row">
    <button class="b fn" onclick="s('vu')">VOL+</button>
    <button class="b mt" onclick="s('mute')">MUTE</button>
    <button class="b fn" onclick="s('vd')">VOL-</button>
  </div>

  <hr>
  <div class="lb">CHANNEL</div>
  <div class="row">
    <button class="b fn" onclick="s('cu')">CH+</button>
    <button class="b fn" onclick="s('cd')">CH-</button>
  </div>

  <hr>
  <div class="lb">NAVIGATION</div>
  <div class="row"><button class="b ar" onclick="s('up')">&#9650;</button></div>
  <div class="row">
    <button class="b ar" onclick="s('left')">&#9664;</button>
    <button class="b ok" onclick="s('ok')">OK</button>
    <button class="b ar" onclick="s('right')">&#9654;</button>
  </div>
  <div class="row"><button class="b ar" onclick="s('down')">&#9660;</button></div>

  <hr>
  <div class="row">
    <button class="b in" onclick="s('input')">INPUT</button>
    <button class="b fn" onclick="s('home')">HOME</button>
    <button class="b fn" onclick="s('back')">BACK</button>
    <button class="b fn" onclick="s('exit')">EXIT</button>
  </div>

  <hr>
  <div class="lb">MEDIA</div>
  <div class="row">
    <button class="b fn" onclick="s('play')">&#9654; PLAY</button>
    <button class="b fn" onclick="s('pause')">&#10074;&#10074;</button>
    <button class="b fn" onclick="s('stop')">&#9632; STOP</button>
  </div>

  <div id="m"></div>
</div>
<script>
function s(c){
  const m=document.getElementById('m');
  m.textContent='sending...';m.style.color='#f39c12';
  fetch('/cmd?k='+c).then(r=>r.text()).then(t=>{
    m.style.color='#2ecc71';m.textContent=t;
    setTimeout(()=>m.textContent='',2000);
  }).catch(()=>{m.style.color='#e74c3c';m.textContent='Failed';});
}
</script>
</body>
</html>
)rawliteral";

// ─────────── Setup ───────────
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  delay(1000);

  Serial.println(F("START " __FILE__ " from " __DATE__));
  Serial.println(F("Using library version " VERSION_IRREMOTE));

  IrSender.begin(IR_PINS[0]);
  Serial.print(F("IR pins: "));
  for (int i = 0; i < IR_PIN_COUNT; i++) {
    if (i > 0) Serial.print(F(", "));
    Serial.print(IR_PINS[i]);
  }
  Serial.println();

  // Get Mac Address
  byte mac[6];
  WiFi.macAddress(mac);

  Serial.print("MAC Address: ");
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  // testIrLed();
  connectWiFi();

  server.begin();
  Serial.print(F("Remote: http://"));
  Serial.println(WiFi.localIP());
  Serial.println(F("================================"));
}

// ─────────── 클라이언트 정리 헬퍼 ───────────
void drainAndClose(WiFiClient& client) {
  // 남은 수신 데이터 드레인 — 소켓이 깔끔하게 해제되도록
  unsigned long drainTimeout = millis() + 200;
  while (client.available() && millis() < drainTimeout) {
    client.read();
  }

  client.flush();   // 송신 버퍼 완전 전송 대기
  delay(10);
  client.stop();
}

// ─────────── Loop ───────────
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi lost. Reconnecting..."));
    connectWiFi();
  }

  WiFiClient client = server.available();
  if (!client) return;

  // ── 헤더 읽기 (최대 2KB 제한) ──
  String headers = "";
  headers.reserve(512);
  unsigned long timeout = millis() + 3000;
  while (client.connected() && millis() < timeout) {
    if (client.available()) {
      char c = client.read();
      headers += c;
      if (headers.endsWith("\r\n\r\n")) break;
      if (headers.length() > 2048) {
        Serial.println(F("Header too large, dropping"));
        client.println(F("HTTP/1.1 413 Payload Too Large"));
        client.println(F("Connection: close"));
        client.println();
        drainAndClose(client);
        return;
      }
    } else {
      delay(1);  // WiFi 스택에 처리 시간 양보
    }
  }

  // ── 타임아웃 또는 빈 요청 처리 ──
  if (headers.length() == 0) {
    drainAndClose(client);
    return;
  }

  // ── Method + URL 파싱 ──
  String method = "";
  String url = "";
  int firstSpace = headers.indexOf(' ');
  if (firstSpace > 0) {
    method = headers.substring(0, firstSpace);
    int secondSpace = headers.indexOf(' ', firstSpace + 1);
    if (secondSpace > 0) {
      url = headers.substring(firstSpace + 1, secondSpace);
    }
  }

  // ── Content-Length 파싱 + POST body 읽기 ──
  String body = "";
  if (method == "POST") {
    int contentLength = 0;
    int clIndex = headers.indexOf("Content-Length: ");
    if (clIndex < 0) clIndex = headers.indexOf("content-length: ");
    if (clIndex >= 0) {
      int clEnd = headers.indexOf("\r\n", clIndex);
      contentLength = headers.substring(clIndex + 16, clEnd).toInt();
    }
    if (contentLength > 256) contentLength = 256;

    timeout = millis() + 2000;
    while (body.length() < (unsigned int)contentLength && client.connected() && millis() < timeout) {
      if (client.available()) {
        body += (char)client.read();
      } else {
        delay(1);  // WiFi 스택에 처리 시간 양보
      }
    }
  }

  Serial.print(method);
  Serial.print(' ');
  Serial.println(url);

  // ── OPTIONS (CORS preflight) ──
  if (method == "OPTIONS") {
    sendCorsPreflightResponse(client);
  }
  // ── POST /api/cmd ──
  else if (method == "POST" && url == "/api/cmd") {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);

    if (err) {
      sendJsonError(client, 400, "Bad Request", "Invalid JSON");
    } else {
      int irIndex = doc["ir_index"] | 0;
      const char* command = doc["command"] | "";

      if (irIndex < 0 || irIndex >= IR_PIN_COUNT) {
        sendJsonError(client, 400, "Bad Request", "Invalid ir_index");
      } else if (command[0] == '\0') {
        sendJsonError(client, 400, "Bad Request", "Missing command");
      } else {
        const KeyMapping* mapping = findKey(String(command));

        if (!mapping) {
          sendJsonError(client, 400, "Bad Request", "Unknown command");
        } else {
          sendIrCommand(irIndex, mapping->irCode, 0);
          sendJsonSuccess(client, command, mapping->name);
        }
      }
    }
  }
  // ── GET /cmd?k= (기존 유지) ──
  else if (method == "GET" && url.startsWith("/cmd?k=")) {
    String key = url.substring(7);
    const KeyMapping* mapping = findKey(key);

    if (mapping) {
      sendIrCommand(0, mapping->irCode, 0);

      String resp = String(mapping->name) + " OK";
      client.println(F("HTTP/1.1 200 OK"));
      client.println(F("Content-Type: text/plain; charset=utf-8"));
      client.println(F("Access-Control-Allow-Origin: *"));
      client.println(F("Connection: close"));
      client.println();
      client.println(resp);
    } else {
      client.println(F("HTTP/1.1 400 Bad Request"));
      client.println(F("Connection: close"));
      client.println();
      client.println(F("Unknown key"));
    }
  }
  // ── GET / (웹 UI) ──
  else if (method == "GET" && (url == "/" || url == "/index.html")) {
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: text/html; charset=utf-8"));
    client.println(F("Connection: close"));
    client.println();
    client.println(HTML_PAGE);
  }
  // ── favicon ──
  else if (url == "/favicon.ico") {
    client.println(F("HTTP/1.1 204 No Content"));
    client.println();
  }
  // ── 매칭되지 않는 요청 → 404 ──
  else {
    client.println(F("HTTP/1.1 404 Not Found"));
    client.println(F("Connection: close"));
    client.println();
    client.println(F("Not Found"));
  }

  drainAndClose(client);
}
