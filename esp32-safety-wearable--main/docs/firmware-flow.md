# Firmware Flow

> **Provenance:** Describes the **reference firmware** in this repository. See
> [overview.md](overview.md).

![Firmware flow](../assets/firmware-flow.svg)

## Framework

- **Arduino framework on ESP32**, built with PlatformIO
  ([`platformio.ini`](../platformio.ini), board `esp32dev`)
- Uses only core libraries: `WiFi`, `HTTPClient`, `WiFiClientSecure`. No
  third-party libraries.

## Source layout

| File | Contents | Depends on Arduino? |
|---|---|---|
| [`src/main.cpp`](../src/main.cpp) | `setup()`/`loop()`, sensor sampling, calibration, LEDs, Wi-Fi, HTTP alert, serial report | yes |
| [`include/config.h`](../include/config.h) | Pins, timings, thresholds | no |
| [`include/decision.h`](../include/decision.h) | `DecisionEngine` state machine | no |
| [`include/button.h`](../include/button.h) | `ButtonTracker` debounce + hold logic | no |
| [`include/ecg_rate.h`](../include/ecg_rate.h) | `EcgRateEstimator` | no |
| [`include/secrets.example.h`](../include/secrets.example.h) | Credential placeholders | no |

The decision, button and ECG logic is kept free of Arduino calls, taking time
and readings as arguments. That makes it possible to unit-test on a PC
([`tests/`](../tests)).

## `setup()`

1. Serial at 115200 baud.
2. GPIO modes: ECG lead-off and PIR as inputs, the button as `INPUT_PULLUP`,
   the three LEDs as outputs.
3. ADC: 12-bit resolution, 11 dB attenuation.
4. Wi-Fi station mode and a first, non-blocking connection attempt.

## `loop()`, in order

| Step | Function | Period |
|---|---|---|
| 1 | `sampleEcg()`: lead-off check, ADC read, feed estimator | 4 ms |
| 2 | `sampleGsr()`: ADC read, EMA filter | 50 ms |
| 3 | `samplePir()`: record time of last HIGH | every iteration |
| 4 | `updateCalibration()`: accumulate baselines, finalise at 15 s | 50 ms, first 15 s only |
| 5 | `button.update()`: debounce, Panic/Cancel events | every iteration |
| 6 | `evaluateConditions()`: ECG/GSR/PIR booleans | every iteration |
| 7 | `decision.update()`: new state | every iteration |
| 8 | On state change: log, then arm or clear the pending alert | on change |
| 9 | `updateLeds()` | every iteration |
| 10 | `maintainWifi()` | reconnect every 10 s if down |
| 11 | `serviceNotification()`: POST / retry | when pending, every 10 s |
| 12 | `report()`: serial debug line | 500 ms |

## Timing behaviour

Everything is cooperative and time-sliced on `millis()`/`micros()`, with no
`delay()` calls. The only blocking operation is `http.POST()`, which can take
up to the 5 s timeout. After such a pause the ECG sampler restarts its
schedule instead of bursting to catch up. Loop timing and sampling jitter
have not been measured.
