# Debugging

> **Provenance:** No debugging notes, issue history or logs from the original
> prototype are in this repository, so no debugging story is told here. This
> page lists (1) the debugging aids built into the reference firmware and
> (2) design decisions in the code that each guard against a specific,
> well-known failure mode. These are design measures, **not** records of bugs
> that were hit and fixed.

## Built-in debugging aids

| Aid | Where | Use |
|---|---|---|
| 500 ms serial status line | `report()` | Watch every intermediate value: lead-off, ECG range, rate, GSR deviation, conditions, Wi-Fi. Format in [sensor-acquisition.md](sensor-acquisition.md). |
| State-transition log | `loop()` | `[STATE] A -> B (trigger=...)` shows exactly why an emergency was raised |
| Calibration report | `updateCalibration()` | Prints the baselines and warns when a sensor was disabled for lack of signal |
| Alert log | `sendAlert()` | `[ALERT] attempt N -> HTTP <code>`; negative codes are `HTTPClient` connection errors |
| Green blink when Wi-Fi is down | `updateLeds()` | Shows on the device itself that alerts cannot currently be sent |
| Hardware-free logic tests | `tests/test_logic.cpp` | Check rule changes without flashing |

## Failure modes the design guards against

| Failure mode | Guard in the code |
|---|---|
| Button contact bounce counted as several presses | 50 ms stability debounce (`ButtonTracker`) |
| Accidental bump sends an alert | 1.5 s hold required |
| One long hold raises and then immediately cancels | Event type depends on the state when the press **started**; one event per press |
| Loose ECG electrode read as a rate change | AD8232 lead-off lines gate sampling and reset the estimator |
| Flat or noise-only ECG read as beats | Minimum peak-to-peak gate (300 counts) |
| Double-counting one cycle (tall T wave) | 300 ms refractory period |
| Analog reads failing while Wi-Fi is on | All analog inputs on ADC1, not ADC2 |
| Boot failures from strapping pins | No peripherals on GPIO0/2/5/12/15 |
| Single noisy sensor raising an alert | 3-of-3 agreement, sustained 10 s |
| Brief sensor lull clearing a real alert | EMERGENCY is latched |
| Sample burst after a blocking HTTP call | ECG scheduler resynchronises |
| Lost alert on a temporary network drop | Unlimited 10 s retries; independent Wi-Fi reconnect |
| GSR sensor missing at boot | Baseline < 100 counts disables the GSR condition |

## Troubleshooting guide

| Symptom | Likely cause | Check |
|---|---|---|
| `ecg_off=1` constantly | Electrodes not in contact, LO pins miswired | Electrode gel/contact; GPIO32/33 wiring |
| `bpm=-1.0` with `ecg_off=0` | Amplitude below 300 counts or noisy | `ecg_p2p`; electrode placement; lower `ECG_MIN_PEAK_TO_PEAK` |
| `bpm` about double the expected value | T wave also crossing the threshold | Raise `ECG_THRESHOLD_FRACTION` or `ECG_REFRACTORY_MS` |
| `[CAL] ... GSR condition disabled` | GSR not connected or no skin contact | Wiring to GPIO35; finger electrodes |
| `pir=1` almost always | Sensitivity or hold potentiometer too high; wearer movement | Adjust the potentiometers on the HC-SR501 |
| `[ALERT] ... HTTP -1` | Connection refused, DNS or TLS failure | `NOTIFY_URL`; endpoint reachable from the Wi-Fi network |
| `[ALERT] no Wi-Fi` | Wrong credentials or out of range | `secrets.h`; access point is 2.4 GHz (the ESP32 does not support 5 GHz) |
| Random resets when Wi-Fi transmits | Supply sag | Power source and USB cable; add bulk capacitance |
