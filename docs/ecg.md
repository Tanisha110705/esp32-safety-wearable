# ECG Acquisition

> **Provenance:** ECG sensing is a **confirmed** part of the prototype. The
> exact module and processing used in the original are not documented here.
> This page describes the **reference implementation**
> ([`include/ecg_rate.h`](../include/ecg_rate.h), `sampleEcg()` in
> [`src/main.cpp`](../src/main.cpp)).

## Scope

In this project the ECG channel feeds **one boolean input to an event-detection
rule**. The firmware estimates the interval between successive dominant peaks
and checks whether that rate has risen well above the wearer's own resting
value.

It does **not**:

- analyse waveform morphology (P wave, QRS complex, T wave)
- detect or classify arrhythmias, or any cardiac condition
- produce a heart rate validated against a reference instrument

The rate estimate has not been compared with a clinical monitor, so no
accuracy figure exists.

## Hardware interface (reference)

| Item | Value |
|---|---|
| Front end | AD8232-type single-lead analog ECG module |
| Supply | 3.3 V |
| Signal | `OUTPUT` → GPIO34 (ADC1_CH6), biased around mid-rail |
| Lead-off detection | `LO+` → GPIO32, `LO−` → GPIO33 (HIGH = electrode not in contact) |
| ADC | 12-bit, 11 dB attenuation |

The AD8232 performs the analog conditioning (instrumentation amplifier,
high-pass and low-pass filtering, gain) in hardware. The firmware adds no
digital band-pass filter.

## Acquisition

- **Sampling:** 250 Hz (`ECG_SAMPLE_PERIOD_US = 4000`), scheduled from
  `micros()` in the main loop.
- **Lead-off gating:** before each sample both lead-off lines are read. If
  either is HIGH, the sample is discarded, the estimator is reset, and the ECG
  condition is forced false. This stops a loose electrode, which produces a
  railed or noisy output, from being read as a rate change.
- **Resynchronisation:** if the loop was blocked (for example during an HTTP
  request), the sampler restarts its schedule from "now" instead of taking a
  burst of back-to-back samples.

## Processing: beat-interval estimate

Implemented in `EcgRateEstimator`:

1. **Adaptive range.** Track the minimum and maximum over a 2 s window
   (`ECG_WINDOW_MS`). Each window's range sets the threshold for the next
   window, so the threshold follows baseline wander and amplitude changes
   with electrode contact.
2. **Threshold.** `min + 0.60 × (max − min)` (`ECG_THRESHOLD_FRACTION`).
3. **Signal-quality gate.** If the peak-to-peak range is below 300 counts
   (`ECG_MIN_PEAK_TO_PEAK`), the signal is treated as unusable and no beats
   are counted.
4. **Beat detection.** A beat is an upward threshold crossing. Crossings within
   300 ms of the previous beat (`ECG_REFRACTORY_MS`) are ignored. This rejects
   a second crossing on the same cycle, for example from a tall T wave.
5. **Interval filter.** Intervals outside 300–2000 ms are discarded as
   implausible.
6. **Rate.** The mean of the last 4 accepted intervals, converted to per-minute.
   The value is reported invalid if no beat has been seen for 3 s
   (`ECG_STALE_MS`).

## Event condition

```
ecg_condition = leads_on
             && resting_baseline_valid
             && rate_valid
             && rate >= 1.25 × resting_baseline     (ECG_RATE_RISE_FACTOR)
```

A **relative** threshold, instead of a fixed number, was chosen because
resting rates differ between people. The 1.25 factor is an untuned reference
value. A rise in rate is also caused by ordinary activity such as walking
quickly or climbing stairs, so this condition alone is never enough to raise
an emergency (see [decision-logic.md](decision-logic.md)).

## Testing

`tests/test_logic.cpp` feeds the estimator **synthetic** pulse trains
generated in code (not recorded ECG). Periods of 800 ms and 500 ms are
estimated as 75.0 and 120.0 per minute. A flat, low-amplitude signal is
rejected, and the rate goes invalid when the signal stops. Behaviour on a real
wearer has not been tested in this repository.

## Known weaknesses

- Motion artefacts can create false crossings. The refractory period and
  interval limits reduce but do not remove them.
- Wrist-only ECG electrodes produce weak signals. A single-lead AD8232 normally
  needs electrodes across the body (for example both arms) for a clean trace,
  which is hard on a single-wrist wearable.
- If the wearer's rate is already elevated during calibration, the baseline is
  high and the condition becomes less sensitive.
