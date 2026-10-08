# Reproduction

> These steps build the **reference firmware** in this repository. They do
> not reproduce the original prototype's exact hardware, which is not
> documented.

## Hardware required

| Qty | Part | Notes |
|---:|---|---|
| 1 | ESP32 DevKit V1 (ESP32-WROOM-32) | Any ESP32 dev board works if pins are adjusted |
| 1 | AD8232-type single-lead ECG module + electrodes | Analog output plus LO+/LO− |
| 1 | Grove-type GSR module + finger electrodes | Analog output |
| 1 | HC-SR501-type PIR module | Needs 5 V supply; 3.3 V output |
| 1 | Momentary push button | |
| 3 | LEDs: green, yellow, red | |
| 3 | 220–330 Ω resistors | LED current limiting |
| — | Breadboard / wires, USB cable | |

## Wiring

See [pinout.md](pinout.md) and the [wiring diagram](../assets/wiring-diagram.svg).

| Connection | ESP32 |
|---|---|
| ECG OUTPUT / LO+ / LO− | GPIO34 / GPIO32 / GPIO33 |
| GSR SIG | GPIO35 |
| PIR OUT | GPIO27 |
| Button | GPIO13 → switch → GND |
| LEDs green / yellow / red | GPIO18 / GPIO19 / GPIO21 → resistor → LED → GND |
| ECG, GSR VCC | 3V3 |
| PIR VCC | VIN (5 V) |
| All GND | GND |

## Firmware

- Framework: Arduino (ESP32 core) via PlatformIO
- Dependencies: none beyond the ESP32 Arduino core (`WiFi`, `HTTPClient`,
  `WiFiClientSecure`)
- Install PlatformIO: `pip install platformio`, or use the PlatformIO IDE
  extension for VS Code

## Configuration

```bash
cp include/secrets.example.h include/secrets.h
# edit include/secrets.h:
#   WIFI_SSID      "YOUR_WIFI_SSID"
#   WIFI_PASSWORD  "YOUR_WIFI_PASSWORD"
#   NOTIFY_URL     "https://YOUR_NOTIFICATION_ENDPOINT/alert"
#   NOTIFY_TOKEN   "YOUR_API_TOKEN"
```

`include/secrets.h` is git-ignored. Pins and thresholds are in
`include/config.h`.

For a first test, point `NOTIFY_URL` at a local HTTP server on the same
network instead of a real emergency contact.

## Build

```bash
pio run
```

## Flash

```bash
pio run -t upload
```

## Run

```bash
pio device monitor        # 115200 baud
```

Expected behaviour (by design, not a recorded result):

1. `[BOOT] ...`, and the yellow LED blinks for 15 s, so keep still.
2. `[CAL] done: gsr_baseline=... ecg_baseline_bpm=...`
3. Green LED: solid if Wi-Fi is connected, blinking if not.
4. Status lines every 500 ms (format in
   [sensor-acquisition.md](sensor-acquisition.md)).

## Test

Follow the hardware procedure in [testing.md](testing.md#suggested-hardware-test-procedure).

Host-side logic tests need only a C++17 compiler:

```bash
g++ -std=c++17 -Wall -Wextra -Iinclude tests/test_logic.cpp -o tests/test_logic
./tests/test_logic
```

## Arduino IDE (alternative)

Create a sketch folder, copy `src/main.cpp` in as `<sketch>.ino`, and copy all
of `include/*.h` beside it. Then select the board **ESP32 Dev Module**.
