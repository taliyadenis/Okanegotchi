# Hardware profile and bring-up limits

Primary compile profile: Waveshare ESP32-S3-Zero, ESP32-S3FH4R2, 4MB flash, 2MB QSPI PSRAM. Verify the delivered replacement label/pinout before flash/wiring; purchase confirmation alone did not identify its exact variant. Keep board profile isolated. The Aideepen S3-Black candidate is not verified identical and uses LED GPIO48 instead of Waveshare GPIO21. Do not depend on either onboard LED or the candidate's battery pads.

| Part | Pins / firmware configuration |
|---|---|
| Shared hardware SPI | SCK12, MOSI11, MISO13 |
| 6-pin ST7789 240x240 | CS10, DC2, RST=-1, VCC3V3, GND; labels SCL/SDA mean SPI clock/MOSI |
| PN532 V3-style | CS1, SCK12, MOSI11, MISO13; IRQ/RSTO unconnected; intended module VCC5V with 3.3V logic, confirm module |
| Three four-leg buttons | logical switch GPIO4/5/6 -> ground, INPUT_PULLUP; verify internal leg pairs by continuity |
| Passive piezo | GPIO7 -> REQUIRED 1k resistor -> piezo -> ground; no motor |

Initialize both CS outputs HIGH, then shared SPI and devices using inspected driver APIs. PN532 switch legend should select SPI; existing selected variant intended S1OFF/S2ON but verify the delivered board's legend, not generic switch orientation. Screen has no separate reset/backlight pin; GPIO/PWM backlight control must not be invented.

Battery chain is protected1S LiPo -> TP4056 -> removable J_RUN -> MT3608 set to5.00V -> USB-C pigtail -> MCU. For charging unplug J_RUN. For computer flashing unplug J_RUN and replace the power pigtail with the USB data cable. Do not hardwire a second MCU5V supply or bridge charger B- to OUT-. No battery ADC/control wires exist. No percentage display, charge management or voltage setting can be implemented by firmware.

Start actual bring-up from computer USB alone: serial/profile/memory -> color screen -> buttons -> piezo -> NFC separately -> SPI coexistence -> Wi-Fi/TLS -> full system. Test sleep/wake/reset only if implemented; no deep sleep requirement in MVP. Enclosure mounts components above the18x24-hole junction perfboard; firmware must not change its pinout to simplify libraries.
