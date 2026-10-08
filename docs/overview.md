# Overview and Provenance

## What this project is

A wrist-worn **safety wearable prototype** built around an ESP32. It combines
three sensing inputs (ECG, GSR, PIR) with a manual panic button, applies
rule-based decision logic to decide when a situation should be flagged, shows
the system state on LEDs, and notifies an emergency contact when an emergency
is raised.

The project won **First Prize at the HerZion Ideathon 2025**.

It is a prototype for event detection. It is **not** a medical device, it
does not diagnose anything, and it does not guarantee that an emergency will
be detected or that help will arrive.

## What is confirmed vs. what is reconstructed

The original competition firmware, schematics, photographs and test records
were **not committed to this repository**. To keep the documentation honest,
every page separates two kinds of information:

| Category | Source | Examples |
|---|---|---|
| **Confirmed** | Stated by the project author | ESP32 as controller; ECG, GSR and PIR sensing; panic-button input; LED indication; emergency-contact notification; sensor-based (rule-based) event detection; First Prize, HerZion Ideathon 2025 |
| **Reference implementation** | Written for this repository to document the architecture in working code | Specific module types (AD8232-type ECG, Grove-type GSR, HC-SR501-type PIR); GPIO assignments; sampling rates; thresholds; the 2-of-3 / 3-of-3 rule; Wi-Fi + HTTP(S) webhook notification; LED meanings |

The reference firmware in [`src/`](../src) and [`include/`](../include) is a
complete, readable implementation of the confirmed architecture. It has
**not** been flashed to the original prototype, and its thresholds have
**not** been tuned on measured data. When the original firmware or wiring is
recovered, it should replace the reference values here, and the docs should be
updated to match.

## Documentation map

| Topic | Page |
|---|---|
| Architecture | [system-architecture.md](system-architecture.md) |
| Sensor interfaces and pin map | [sensor-interface.md](sensor-interface.md), [pinout.md](pinout.md) |
| Individual inputs | [ecg.md](ecg.md), [gsr.md](gsr.md), [pir.md](pir.md), [panic-button.md](panic-button.md) |
| Acquisition | [sensor-acquisition.md](sensor-acquisition.md) |
| Decision logic and thresholds | [decision-logic.md](decision-logic.md) |
| LED indication | [indication.md](indication.md) |
| Notification and networking | [notification.md](notification.md) |
| Firmware structure | [firmware-flow.md](firmware-flow.md) |
| Testing, debugging, results | [testing.md](testing.md), [debugging.md](debugging.md), [results.md](results.md) |
| Limitations and scope | [limitations.md](limitations.md) |
| Building it yourself | [reproduction.md](reproduction.md) |
