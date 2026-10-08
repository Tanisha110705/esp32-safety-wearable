# PIR Detection

> **Provenance:** PIR sensing is a **confirmed** part of the prototype. This
> page describes the **reference implementation** (`samplePir()` and the PIR
> condition in `evaluateConditions()`, [`src/main.cpp`](../src/main.cpp)).

## Purpose

A passive-infrared (PIR) sensor reports changes in infrared radiation across
its field of view, which typically means a warm body moving nearby. In this
wearable it is the **environmental** input. It indicates motion or presence
near the wearer, alongside the two body-worn signals (ECG, GSR).

It does not identify who or what moved, how many people are present, or what
they intend.

## Hardware interface (reference)

| Item | Value |
|---|---|
| Module | HC-SR501-type PIR |
| Supply | 5 V from VIN (module requires ≥ 4.5 V) |
| Output | `OUT` → GPIO27, 3.3 V logic (safe for the ESP32 directly) |
| Semantics | **HIGH = motion detected**, LOW = idle |
| Module settings | Sensitivity and hold-time potentiometers, plus a trigger-mode jumper, on the module. Not controlled by firmware. |

## Timing

Two separate timers apply:

1. **Module hold time.** The HC-SR501 keeps its output HIGH for a time set by
   its potentiometer (roughly a few seconds up to several minutes). It also
   ignores new motion for a short period after going LOW.
2. **Firmware hold.** The firmware stores the time of the last HIGH reading,
   and the PIR condition stays true for 5 s afterwards (`PIR_HOLD_MS`). This
   lets a short pulse still overlap with the slower ECG and GSR conditions in
   the decision rule.

```
pir_condition = (now − last_time_PIR_was_HIGH) <= 5000 ms
```

The PIR has no calibration. The module needs about a minute after power-up
to settle, and it may give false HIGHs during that time. In practice this
overlaps with the 15 s calibration window and the period after it.

## Role in decision logic

PIR is one of the three votes. Alone it can only contribute to a WARNING when
another condition is also true. It never raises an emergency by itself. See
[decision-logic.md](decision-logic.md).

## Known weaknesses

- **Self-motion.** On a moving wearer the sensor's view sweeps across warm
  objects, which can produce HIGH readings with no one else nearby. This is
  the main expected source of PIR false positives on a wearable.
- The field of view depends on how and where the module is mounted.
- PIR detects movement, not stationary presence.
