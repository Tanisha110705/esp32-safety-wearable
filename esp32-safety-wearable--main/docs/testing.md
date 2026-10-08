# Testing

> **Provenance:** Test records from the original prototype (bench tests,
> demo-day behaviour) are **not in this repository**. The only test evidence
> here is for the reference firmware's logic.

## Result labels

| Label | Meaning |
|---|---|
| **Passed** | An automated test in this repository runs and passes |
| **Type-checked** | The code compiles against stub headers; not run on hardware |
| **Implemented** | The code exists, but there is no test evidence |
| **Not documented** | No evidence in the repository either way |

## Test matrix

| Test | Expected behaviour | Status | Evidence |
|---|---|---|---|
| Decision rules (states, 2-of-3, 3-of-3 for 10 s, latch, cancel) | Matches [decision-logic.md](decision-logic.md) | **Passed** (host) | `tests/test_logic.cpp`, synthetic inputs |
| Button debounce | 10 ms bounce ignored | **Passed** (host) | `tests/test_logic.cpp` |
| Panic hold | Event after 1.5 s, exactly once | **Passed** (host) | `tests/test_logic.cpp` |
| Cancel hold | Only for presses started in EMERGENCY, after 5 s | **Passed** (host) | `tests/test_logic.cpp` |
| ECG rate estimator | 800 ms / 500 ms synthetic periods → 75.0 / 120.0 per min; flat signal rejected; goes stale | **Passed** (host) | `tests/test_logic.cpp`, synthetic pulse train, not real ECG |
| Firmware compiles for ESP32 | `pio run` succeeds | **Not documented** | Not run (see note below) |
| `main.cpp` type-check | No compile errors against Arduino API stubs | **Type-checked** | Done during development with throwaway stubs; not part of the repo |
| ECG acquisition on hardware | Waveform read; lead-off detected | **Implemented** | No recording |
| GSR acquisition on hardware | Reading changes with contact / response | **Implemented** | No recording |
| PIR trigger on hardware | HIGH on motion | **Implemented** | No recording |
| Panic button on hardware | EMERGENCY after 1.5 s hold | **Implemented** | No recording |
| LED feedback on hardware | Patterns as in [indication.md](indication.md) | **Implemented** | No recording |
| Notification delivery | HTTP 2xx from endpoint; contact reached | **Implemented** | No recording |
| Combined sensor event | Automatic EMERGENCY under real conditions | **Implemented** | No recording |
| Original prototype at HerZion 2025 | Demonstrated working | **Not documented** | The award is confirmed; demo details are not recorded |

**ESP32 build note:** the package registries for the ESP32 toolchain
(PlatformIO registry, Arduino and Espressif downloads) were not reachable from
the environment where this reference firmware was written, so a full
cross-compile was not run. Run `pio run` locally to confirm. See
[reproduction.md](reproduction.md).

## Running the host tests

```bash
g++ -std=c++17 -Wall -Wextra -Iinclude tests/test_logic.cpp -o tests/test_logic
./tests/test_logic
```

Expected final line: `<N> checks, 0 failures` (most checks come from
per-iteration assertions inside loops).

## Suggested hardware test procedure

These are **procedures to run**, not results:

1. **Boot:** yellow LED blinks for 15 s, then `[CAL] done` appears on serial.
2. **ECG lead-off:** remove an electrode → `ecg_off=1`, `bpm=-1.0`.
3. **ECG rate:** compare `bpm` against a manual pulse count over 60 s, at rest.
4. **GSR:** hold the electrodes, wait for calibration, then breathe on them or
   tighten the grip → `gsr_dev` rises.
5. **PIR:** wave a hand in front of the sensor → `pir=1`, and `cond=xx1` for
   5 s.
6. **Button tap:** press for < 1 s → no state change.
7. **Panic:** hold for ≥ 1.5 s → `[STATE] ... -> EMERGENCY (trigger=manual)`,
   red LED fast blink.
8. **Notification:** use a test endpoint (for example a local HTTP server) →
   `[ALERT] delivered`, red LED solid.
9. **No Wi-Fi:** turn the access point off and repeat step 7 →
   `[ALERT] no Wi-Fi, will retry` every 10 s.
10. **Cancel:** new press held ≥ 5 s → NORMAL, green LED.

Use a test endpoint, not a real emergency contact, while testing.
