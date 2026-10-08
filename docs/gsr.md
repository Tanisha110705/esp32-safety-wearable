# GSR Acquisition

> **Provenance:** GSR sensing is a **confirmed** part of the prototype. This
> page describes the **reference implementation** (`sampleGsr()`,
> `gsrDeviationFraction()` in [`src/main.cpp`](../src/main.cpp)).

## What is measured

A galvanic skin response (GSR) module passes a small current between two
finger or skin electrodes and outputs a voltage that depends on skin
conductance. Conductance changes with sweat-gland activity, but also with
temperature, humidity, electrode pressure and movement.

The firmware therefore treats GSR as **"conductance response relative to this
wearer's resting level"**. It does **not** treat it as a measurement of
stress, fear, anxiety or any other mental or medical state.

## Hardware interface (reference)

| Item | Value |
|---|---|
| Module | Grove-type GSR module (two electrodes + on-board amplifier) |
| Supply | 3.3 V (keeps the output inside the ESP32 ADC range) |
| Signal | `SIG` → GPIO35 (ADC1_CH7) |
| ADC | 12-bit, 11 dB attenuation |

On Grove-type modules the raw reading moves in the **opposite direction** to
conductance: higher conductance gives a lower reading. Because the firmware
uses the **absolute** deviation from baseline, it does not depend on that
direction.

## Acquisition and filtering

- **Sampling:** 20 Hz (`GSR_SAMPLE_PERIOD_MS = 50`). Conductance changes over
  seconds, so faster sampling would add nothing.
- **Filter:** exponential moving average,
  `filtered += 0.10 × (raw − filtered)` (`GSR_EMA_ALPHA`). At 20 Hz the time
  constant is roughly 0.5 s. That removes ADC noise and small contact jitter
  while still following a real response within about 1–2 s.

## Baseline and event condition

During the 15 s calibration window the filtered value is averaged to give
`gsrBaseline`. After that:

```
deviation     = |filtered − gsrBaseline| / gsrBaseline
gsr_condition = gsrBaseline >= 100  &&  deviation >= 0.15
```

| Parameter | Value | Constant | Origin |
|---|---:|---|---|
| Deviation threshold | 0.15 (15 %) | `GSR_DEVIATION_FRACTION` | Reference value, not tuned on data |
| Minimum valid baseline | 100 counts | `GSR_MIN_VALID_BASELINE` | Reference value; below this the sensor is assumed not to be in contact |
| Filter coefficient | 0.10 | `GSR_EMA_ALPHA` | Reference value |

Using absolute deviation means an electrode slipping off can also satisfy the
condition. That is one reason GSR is only one vote among three in the decision
rule.

## Testing

No GSR recordings or test results are in this repository. The filter and
threshold have not been evaluated on real data.

## Known weaknesses

- The signal depends strongly on electrode contact, skin moisture, ambient
  temperature and movement.
- The baseline drifts over long periods (minutes to hours), but it is captured
  only once at boot.
- Different people have very different resting conductance. The relative
  threshold reduces this effect but does not remove it.
