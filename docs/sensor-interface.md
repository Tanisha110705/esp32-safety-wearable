# Sensor Interfaces

> **Provenance:** ECG, GSR, PIR and the panic button are **confirmed** inputs.
> Module types, pins, rates and processing are those of the **reference
> implementation** ([`include/config.h`](../include/config.h),
> [`src/main.cpp`](../src/main.cpp)). See [overview.md](overview.md).

## Interface summary

| Sensor | Interface | ESP32 pin | Signal type | Sampling | Processing | Role |
|---|---|---|---|---|---|---|
| ECG (AD8232-type front end) | Analog + 2 digital lead-off lines | GPIO34 (ADC1_CH6); LO+ GPIO32, LO− GPIO33 | Single-lead analog waveform, mid-rail biased | 250 Hz | Adaptive-threshold peak detection → beat-interval rate | Condition: rate ≥ 1.25 × resting baseline |
| GSR (Grove-type module) | Analog | GPIO35 (ADC1_CH7) | Slow-varying voltage related to skin conductance | 20 Hz | Exponential moving average (α = 0.10) | Condition: ≥ 15 % deviation from resting baseline |
| PIR (HC-SR501-type) | Digital | GPIO27 | HIGH while motion detected | Every loop iteration | Time of last HIGH | Condition: HIGH within the last 5 s |
| Panic button | Digital, internal pull-up | GPIO13 | LOW while pressed | Every loop iteration | 50 ms debounce, 1.5 s hold | Manual EMERGENCY (overrides sensor rules) |

## ADC configuration

```cpp
analogReadResolution(12);         // 0–4095 counts
analogSetAttenuation(ADC_11db);   // full scale ≈ 3.1 V
```

The 11 dB attenuation covers the AD8232 output swing and a 3.3 V-powered GSR
module. The ESP32 ADC is noticeably non-linear near both ends of its range.
The firmware uses only **relative** measures (a ratio to the wearer's own
baseline, and peak timing), never absolute voltages, so that non-linearity
and per-chip offset matter less. No per-chip ADC calibration is applied.

## Calibration

For the first 15 s after power-up (`CALIBRATION_MS`) the firmware averages:

- the filtered GSR reading → `gsrBaseline`
- every valid ECG rate estimate → `ecgBaselineBpm`

The wearer should be still and at rest during this window, while the yellow
LED blinks. If a sensor gives no usable signal during calibration (ECG
electrodes off, or a GSR baseline below 100 counts), that sensor's condition
is **disabled** until the next reboot. The firmware reports this on serial
rather than guessing a baseline.

There is **no stored or per-user calibration**. A new baseline is captured on
every boot.

## Per-sensor details

- [ECG](ecg.md)
- [GSR](gsr.md)
- [PIR](pir.md)
- [Panic button](panic-button.md)
- [Acquisition timing and data format](sensor-acquisition.md)
