# System Architecture

> **Provenance:** The block structure (ESP32 + ECG + GSR + PIR + panic button +
> LEDs + emergency-contact notification) is **confirmed**. Module types, pins,
> rates and the notification transport describe the **reference
> implementation** in this repository. See [overview.md](overview.md).

![System architecture](../assets/system-architecture.svg)

## ESP32 as the central controller

All sensing, decision-making, indication and communication run on a single
ESP32 (reference board: ESP32 DevKit V1 / ESP32-WROOM-32). The firmware is a
single cooperative `loop()`. Each task checks its own timer and returns
immediately if it has nothing to do, so the ECG sampling keeps its 4 ms period
while slower tasks (GSR at 50 ms, serial report at 500 ms, Wi-Fi reconnect at
10 s) run between samples. No RTOS tasks or interrupts are used. The one
blocking call is an HTTP request during an emergency (up to 5 s).

The ESP32 was a good fit because it has:

- **Two 12-bit SAR ADCs.** The two analog sensors (ECG, GSR) read on ADC1,
  which still works while Wi-Fi is running.
- **Plenty of GPIO** for the digital PIR output, the button and three LEDs.
- **Integrated Wi-Fi**, so no extra communication module is needed for the
  notification path.

## Signal paths

| Path | From | Through | To |
|---|---|---|---|
| Sensor input | ECG, GSR (analog), PIR (digital) | ADC1 / GPIO, sampling, filtering | Per-sensor boolean conditions |
| Automatic decision | Three conditions | `DecisionEngine` (2-of-3 warning, 3-of-3 sustained emergency) | System state |
| Manual input | Panic button | Debounce + 1.5 s hold (`ButtonTracker`) | Immediate EMERGENCY |
| Local feedback | System state | `updateLeds()` | Green / yellow / red LED |
| Remote feedback | EMERGENCY state | Wi-Fi STA, HTTP(S) POST | User-provided endpoint, which forwards to the emergency contact |

The panic button bypasses the sensor rules entirely. It is the explicit,
deterministic way to raise an emergency, and it also works during the
calibration period, before any sensor baseline exists.

## Local user input and feedback

- **Input:** one momentary button. A long hold raises an emergency, and a
  longer hold during an emergency cancels it. See
  [panic-button.md](panic-button.md).
- **Feedback:** three single-colour LEDs, with exactly one lit at a time. See
  [indication.md](indication.md).

## Remote notification path

The device sends a JSON alert to a configurable HTTP(S) endpoint. Delivering
the message to a person, by SMS, a chat app or a call, is the endpoint's job
and is outside this firmware. The device stores no phone numbers or contact
addresses. See [notification.md](notification.md).

## Power

The power arrangement of the original prototype is **not documented** in this
repository. The reference wiring assumes the DevKit is powered over USB (5 V
on VIN), with the on-board 3.3 V regulator supplying the ECG and GSR modules
and 5 V supplying the PIR module. Battery operation, charging and power
management are listed as future work.
