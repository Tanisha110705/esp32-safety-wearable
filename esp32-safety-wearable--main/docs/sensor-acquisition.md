# Sensor Data Acquisition

> **Provenance:** Describes the **reference firmware**. See
> [overview.md](overview.md).

## Data stored in this repository

| Data type | Present? |
|---|---|
| Raw sensor recordings | No |
| Serial-monitor logs | No |
| CSV exports | No |
| ECG / GSR waveform captures | No |
| Screenshots or photographs | No |
| Test data from the original prototype | No |

No measured data from the original prototype is in this repository. The
firmware does **not** store data on the device. Readings exist only in memory
and in the serial output. See [`data/README.md`](../data/README.md) for how to
capture and add recordings.

## Acquisition summary

| Channel | Pin | Rate | Resolution | On-device processing |
|---|---|---:|---|---|
| ECG | GPIO34 | 250 Hz | 12-bit | Lead-off gating, adaptive threshold, interval averaging |
| ECG lead-off | GPIO32/33 | 250 Hz (with ECG) | digital | Gate only |
| GSR | GPIO35 | 20 Hz | 12-bit | EMA, α = 0.10 |
| PIR | GPIO27 | every loop iteration | digital | Last-HIGH timestamp |
| Button | GPIO13 | every loop iteration | digital | 50 ms debounce |

## Serial output format

At 115200 baud the firmware prints one status line every 500 ms, plus event
lines.

**Status line (format):**

```
t=<ms> state=<STATE> ecg_off=<0|1> ecg_p2p=<counts> bpm=<rate|-1.0> gsr=<filtered counts> gsr_dev=<fraction> pir=<0|1> btn=<0|1> cond=<ecg><gsr><pir> wifi=<0|1>
```

| Field | Meaning |
|---|---|
| `t` | `millis()` since boot |
| `state` | CALIBRATING / NORMAL / WARNING / EMERGENCY |
| `ecg_off` | 1 if either AD8232 lead-off line is HIGH |
| `ecg_p2p` | ECG peak-to-peak range of the last completed 2 s window (ADC counts) |
| `bpm` | Beat-interval rate estimate, `-1.0` when invalid |
| `gsr` | EMA-filtered GSR reading (ADC counts) |
| `gsr_dev` | \|gsr − baseline\| / baseline (0 until calibrated) |
| `pir` | Current PIR pin level |
| `btn` | Debounced button state |
| `cond` | Three digits: ECG, GSR, PIR condition (1 = true) |
| `wifi` | 1 if associated to the access point |

**Event lines:** `[BOOT]`, `[CAL] done: ...`, `[STATE] A -> B (trigger=...)`,
`[WIFI] connecting...`, `[ALERT] attempt N -> HTTP <code>`,
`[ALERT] delivered`.

The status line is easy to capture with any serial terminal and convert to
CSV, which is the suggested way to build a dataset for threshold tuning.
