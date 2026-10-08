# Pinout

> **Provenance:** These are the **reference pin assignments** defined in
> [`include/config.h`](../include/config.h). The original prototype's wiring
> was not recorded in this repository, so this is not a record of how the
> competition unit was wired. Change `config.h` if your wiring differs.

![Reference wiring](../assets/wiring-diagram.svg)

## Signal pins

| Function | ESP32 pin | Direction | Electrical | `config.h` constant |
|---|---|---|---|---|
| ECG analog output | GPIO34 (ADC1_CH6) | Input (analog) | 0–3.3 V, mid-rail ≈ 1.65 V | `PIN_ECG_OUT` |
| ECG lead-off LO+ | GPIO32 | Input (digital) | HIGH = electrode off | `PIN_ECG_LO_PLUS` |
| ECG lead-off LO− | GPIO33 | Input (digital) | HIGH = electrode off | `PIN_ECG_LO_MINUS` |
| GSR analog output | GPIO35 (ADC1_CH7) | Input (analog) | 0–3.3 V | `PIN_GSR` |
| PIR output | GPIO27 | Input (digital) | HIGH = motion (3.3 V logic from HC-SR501) | `PIN_PIR` |
| Panic button | GPIO13 | Input, internal pull-up | Pressed = LOW (switch to GND) | `PIN_PANIC_BUTTON` |
| LED green | GPIO18 | Output | HIGH = on, via 220–330 Ω | `PIN_LED_GREEN` |
| LED yellow | GPIO19 | Output | HIGH = on, via 220–330 Ω | `PIN_LED_YELLOW` |
| LED red | GPIO21 | Output | HIGH = on, via 220–330 Ω | `PIN_LED_RED` |

## Power pins

| Module | Supply | ESP32 pin |
|---|---|---|
| ECG front end (AD8232-type) | 3.3 V | 3V3 |
| GSR module (Grove-type) | 3.3 V | 3V3 |
| PIR module (HC-SR501-type) | 5 V (module needs ≥ 4.5 V) | VIN (5 V when USB-powered) |
| All modules | Ground | GND (common) |

## Why these pins

- **Analog inputs on ADC1 only.** ADC2 is shared with the Wi-Fi radio and
  cannot be read while Wi-Fi is active, so the ECG and GSR outputs use ADC1
  channels.
- **GPIO34/35 are input-only.** They have no output driver and no internal
  pull resistors. That suits analog sensor outputs and frees the other pins
  for outputs.
- **Boot strapping pins avoided.** GPIO0, 2, 5, 12 and 15 affect boot mode or
  flash voltage, so no sensor or button is placed on them.
- **GPIO13 for the button** has an internal pull-up and is not a strapping
  pin, so the button needs no external resistor.
