#pragma once
// Proposed pin assignment; not a complete or hardware-tested firmware.
namespace BankBeastPins {
constexpr int SPI_SCK = 12;
constexpr int SPI_MOSI = 11;
constexpr int SPI_MISO = 13;
constexpr int TFT_CS = 10;
constexpr int TFT_DC = 2;
constexpr int TFT_RST = -1; // selected LCD ties RES high internally
constexpr int NFC_CS = 1;
constexpr int BUTTON_LEFT = 4;
constexpr int BUTTON_SELECT = 5;
constexpr int BUTTON_RIGHT = 6;
constexpr int BUZZER = 7; // Revision D: GPIO -> REQUIRED 1k series resistor -> passive piezo -> SYS_GND
}
namespace OkanegachiPins = BankBeastPins; // current name; retain existing firmware compatibility
// SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
// Use the same SPI object for Adafruit_ST7789 and Adafruit_PN532.
// Set both CS pins HIGH before initializing devices.
// 10k CS pull-ups are optional DNP parts, not required purchased components.
// Buttons connect GPIO to GND and use INPUT_PULLUP (pressed = LOW).
// NFC VCC -> USB-derived 5V, LCD VCC -> 3V3, all GND common.
// GND here means load-side SYS_GND; never externally bridge TP4056 B- to OUT-.
// Assembly E: single-board-reve/wire-map.csv. All earlier GPIO assignments remain unchanged.
// Bare two-terminal passive piezo only; no extra capacitor or transistor in this low-volume circuit.
// No vibration motor or fourth button. Start output LOW; stop tone and return LOW when muted.
