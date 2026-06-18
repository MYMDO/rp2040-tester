# RP2040 Component Tester (M328-style)

C/Pico SDK port. SSD1306 I2C OLED. R/C/L/diode/BJT/FET/SCR detection.

## Pin map
| Signal | GPIO |
|---|---|
| TP1 (ADC0) | 26 |
| TP2 (ADC1) | 27 |
| TP3 (ADC2) | 28 |
| TP1_RH (680R) | 2 |
| TP1_RL (470k) | 3 |
| TP2_RH | 4 |
| TP2_RL | 5 |
| TP3_RH | 6 |
| TP3_RL | 7 |
| I2C SDA | 8 |
| I2C SCL | 9 |
| BTN_START | 10 |
| BTN_MODE | 11 |
| BUZZER | 12 |
| PWR_LATCH | 13 |

OLED addr 0x3C.

## Build
```
export PICO_SDK_PATH=/path/to/pico-sdk
mkdir build && cd build
cmake -DPICO_BOARD=pico ..
make -j4
```
`rp2040_tester.uf2` -> drag onto Pico in BOOTSEL mode. Verified clean build (50904B flash, 0 errors).

## Known TODOs
- ADC ref = board 3V3 rail (~1%), no precision bandgap like AVR -> calibrate against meter
- ESR estimate uncalibrated, electrolytics only
- Inductor measurement not implemented (falls through to OPEN/UNKNOWN)
- Zener Vf not implemented (needs boost circuit, 3V3 rail insufficient)
- SCR test is single-quadrant; Triac bidirectional confirm not implemented
- Font glyphs are first-pass stroke font, not visually tuned on hardware yet
