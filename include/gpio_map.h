/*
 * gpio_map.h — IR Module GPIO Pin Mapping
 *
 * Ethernet Shield 점유 핀 제외:
 *   D4  (SD Card CS)
 *   D10 (SPI SS)
 *   D11 (SPI MOSI)
 *   D12 (SPI MISO)
 *   D13 (SPI SCK)
 *
 * ┌──────────┬─────────┬─────────────────────┐
 * │ Module ID│ GPIO Pin│ 비고                 │
 * ├──────────┼─────────┼─────────────────────┤
 * │  1       │  D0     │ Serial RX (주의)     │
 * │  2       │  D1     │ Serial TX (주의)     │
 * │  3       │  D2     │                      │
 * │  4       │  D3     │                      │
 * │  5       │  D5     │                      │
 * │  6       │  D6     │                      │
 * │  7       │  D7     │                      │
 * │  8       │  D8     │                      │
 * │  9       │  D9     │                      │
 * │ 10       │  A0(14) │                      │
 * │ 11       │  A1(15) │                      │
 * │ 12       │  A2(16) │                      │
 * │ 13       │  A3(17) │                      │
 * │ 14       │  A4(18) │                      │
 * │ 15       │  A5(19) │                      │
 * └──────────┴─────────┴─────────────────────┘
 *
 * 주의: Module 1,2 (D0,D1) 사용 시 Serial 디버깅 불가.
 *       디버깅 시 해당 모듈의 점퍼를 분리할 것.
 */

#ifndef GPIO_MAP_H
#define GPIO_MAP_H

#include <Arduino.h>

#define IR_MODULE_COUNT 15

const uint8_t IR_PINS[IR_MODULE_COUNT] = {
    0,   // Module  1 — D0
    1,   // Module  2 — D1
    2,   // Module  3 — D2
    3,   // Module  4 — D3
    5,   // Module  5 — D5
    6,   // Module  6 — D6
    7,   // Module  7 — D7
    8,   // Module  8 — D8
    9,   // Module  9 — D9
    A0,  // Module 10 — A0
    A1,  // Module 11 — A1
    A2,  // Module 12 — A2
    A3,  // Module 13 — A3
    A4,  // Module 14 — A4
    A5,  // Module 15 — A5
};

#endif // GPIO_MAP_H
