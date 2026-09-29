#pragma once
namespace okanegachi::pins {
inline constexpr int SCK = 12, MOSI = 11, MISO = 13, LCD_CS = 10, LCD_DC = 2,
                     LCD_RST = -1, NFC_CS = 1;
inline constexpr int A = 4, B = 5, C = 6, PIEZO = 7;
// USB-only initial tests. Piezo requires the agreed 1k series resistor.
// ST7789 240x240: 3V3 supply. PN532 supply/selector: verify delivered module.
} // namespace okanegachi::pins
