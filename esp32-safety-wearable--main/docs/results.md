# Results / Observed Behaviour

This page keeps four categories apart on purpose.

## Recognised

- **First Prize, HerZion Ideathon 2025.** The prototype was awarded first
  prize. *(Reported by the project author.)*

## Implemented (reference firmware in this repository)

- ESP32 Arduino firmware with a non-blocking cooperative loop
- ECG acquisition at 250 Hz with AD8232 lead-off gating and an
  adaptive-threshold beat-interval estimate
- GSR acquisition at 20 Hz with EMA smoothing
- PIR digital input with a 5 s condition hold
- 15 s resting-baseline calibration for ECG and GSR
- Rule-based state machine: CALIBRATING → NORMAL / WARNING / EMERGENCY,
  with 2-of-3 warning, 3-of-3-for-10-s automatic emergency, and a latched
  emergency
- Panic button: 50 ms debounce, 1.5 s hold to raise, 5 s hold to cancel
- Three-LED state indication
- Wi-Fi + HTTP(S) JSON alert with unlimited 10 s retries and an optional
  bearer token and CA certificate
- Serial debug output

## Tested

| What | How | Result |
|---|---|---|
| Decision rules | Host unit tests, synthetic inputs | Pass |
| Button debounce / hold / cancel | Host unit tests, synthetic inputs | Pass |
| ECG estimator | Host unit tests, synthetic pulse trains (800 ms → 75.0/min, 500 ms → 120.0/min) | Pass |
| `main.cpp` type-check | Compiled against Arduino API stubs | No errors |

No hardware tests of the reference firmware are recorded.

## Observed

No observations from the original prototype (bench sessions, the demo,
serial logs) are recorded in this repository. When they are recovered, add
them here with their source.

## Not quantified

- Detection accuracy, false-positive rate, false-negative rate
- ECG rate accuracy against a reference monitor
- GSR response magnitude or repeatability
- PIR detection range in wearable use
- Notification latency and delivery success rate
- Power consumption and battery life

## Where every number comes from

| Number | Source |
|---|---|
| 250 Hz, 20 Hz, 15 s, 5 s, 10 s, 1.5 s, 50 ms, 1.25×, 15 %, 300 counts, … | **Configured** reference values in `include/config.h` |
| 75.0 / 120.0 per minute | **Tested**: host unit test on synthetic input |
| First Prize | **Reported** by the project author |
